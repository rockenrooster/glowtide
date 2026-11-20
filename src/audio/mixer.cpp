#include "audio/mixer.h"

#include <cmath>
#include <cstring>

namespace gt::audio {

namespace {

// Voice limits by sound (GAME_DESIGN 12: "each sound has its own voice limit").
int limitOf(int s) {
    if (s >= kSndLadder0 && s < kSndLadder0 + 11) return 8;
    if (s >= kSndPluck0 && s < kSndPluck0 + 11) return 6;
    switch (s) {
        case kSndPip: return 4;
        case kSndTick: return 6;
        case kSndShatterBig: case kSndWinChord: case kSndLose: return 1;
        case kSndPulse: return 2;
        default: return 3;
    }
}

int categoryOf(int s) {
    if (s >= kSndLadder0 && s < kSndLadder0 + 11) return 1000;
    if (s >= kSndPluck0 && s < kSndPluck0 + 11) return 1001;
    return s;
}

bool isMusic(int s) { return s == kSndPulse || (s >= kSndPluck0 && s < kSndPluck0 + 11); }

constexpr float kEffectsGain = 0.33f;  // the flood render lands near -17 LUFS (GAME_DESIGN 12: -20 to -14)
constexpr float kBeat = 60.0f / 76.0f;
constexpr float kStep = kBeat * 0.5f;  // eighth notes

}  // namespace

void Mixer::init(int sampleRate, int timbre) {
    rate_ = sampleRate;
    bank_.build(sampleRate, timbre);
    for (Voice& v : voices_) v.sound = -1;
}

bool Mixer::push(const Command& c) {
    const u32 h = head_.load(std::memory_order_relaxed);
    const u32 t = tail_.load(std::memory_order_acquire);
    if (h - t >= u32(kRing)) {
        dropped_.fetch_add(1);
        return false;
    }
    ring_[h % kRing] = c;
    head_.store(h + 1, std::memory_order_release);
    return true;
}

void Mixer::play(int sound, float gain, float pan, float pitch, float delay) {
    Command c;
    c.type = 0;
    c.sound = u8(sound);
    c.gain = gain;
    c.pan = pan;
    c.pitch = pitch;
    c.delay = delay;
    push(c);
}

void Mixer::duck(float seconds) {
    Command c;
    c.type = 1;
    c.a = seconds;
    push(c);
}

void Mixer::setEnabled(bool effects, bool music) {
    Command c;
    c.type = 2;
    c.a = effects ? 1.0f : 0.0f;
    c.b = music ? 1.0f : 0.0f;
    push(c);
}

void Mixer::start(const Command& c) {
    const int s = c.sound;
    if (s < 0 || s >= kSndCount) return;
    if (!isMusic(s) && !effectsOn_) return;
    if (isMusic(s) && !musicOn_) return;
    const int cat = categoryOf(s), limit = limitOf(s);
    int count = 0, oldestCat = -1, freeV = -1, oldest = -1;
    u32 oldestCatAge = ~0u, oldestAge = ~0u;
    for (int i = 0; i < kVoices; ++i) {
        const Voice& v = voices_[i];
        if (v.sound < 0) {
            if (freeV < 0) freeV = i;
            continue;
        }
        if (categoryOf(v.sound) == cat) {
            ++count;
            if (v.age < oldestCatAge) {
                oldestCatAge = v.age;
                oldestCat = i;
            }
        }
        if (v.age < oldestAge && !isMusic(v.sound)) {
            oldestAge = v.age;
            oldest = i;
        }
    }
    int slot = count >= limit ? oldestCat : (freeV >= 0 ? freeV : oldest);
    if (slot < 0) return;
    Voice& v = voices_[slot];
    v.sound = s;
    v.pos = 0;
    v.step = c.pitch * float(bank_.rate()) / float(rate_);
    const float a = (clampv(c.pan, -1.0f, 1.0f) + 1.0f) * 0.25f * 3.14159265f;
    const float g = c.gain * (isMusic(s) ? musicGain_ : kEffectsGain);
    v.gl = g * std::cos(a);
    v.gr = g * std::sin(a);
    v.delay = int(c.delay * float(rate_));
    v.age = ++voiceAge_;
}

void Mixer::musicStep(int frames) {
    const float dt = float(frames) / float(rate_);
    // Below zero: HUSH (VARIETY_DESIGN 13): the pad alone, no pulse and no plucks.
    const float want = intensity_.load();
    const bool padOnly = want < 0;
    intensityNow_ += (maxv(0.0f, want) - intensityNow_) * minv(1.0f, dt * 2.0f);
    if (duckHold_ > 0) {
        duckHold_ -= dt;
        if (duckHold_ <= 0) duckTarget_ = 1.0f;
    }
    duck_ += (duckTarget_ - duck_) * minv(1.0f, dt / 0.05f);
    if (!musicOn_) return;
    musicClock_ += double(dt);
    // Eighth-note steps: the low pulse on beats one and three, sparse plucks from the chord's pentatonic notes.
    while (musicClock_ >= double(nextStep_) * double(kStep)) {
        const int step = nextStep_ % 32;
        const int bar = step / 8;
        if (step % 4 == 0 && !padOnly) {
            Command c;
            c.sound = kSndPulse;
            c.gain = 0.55f * duck_;
            start(c);
        }
        pluckRng_ = pluckRng_ * 1664525u + 1013904223u;
        const float roll = float(pluckRng_ >> 8) / 16777216.0f;
        if (!padOnly && roll < 0.10f + 0.45f * intensityNow_ && step % 2 == (bar & 1)) {
            static const int cNotes[6] = {0, 2, 3, 5, 7, 8};   // C E G C' E' G'
            static const int aNotes[6] = {4, 0, 2, 9, 5, 7};   // A C E A' C' E'
            pluckRng_ = pluckRng_ * 1664525u + 1013904223u;
            const int k = int((pluckRng_ >> 16) % 6u);
            Command c;
            c.sound = u8(kSndPluck0 + (bar < 2 ? cNotes[k] : aNotes[k]));
            c.gain = (0.35f + 0.25f * intensityNow_) * duck_;
            c.pan = (float(int((pluckRng_ >> 8) % 100u)) / 50.0f - 1.0f) * 0.6f;
            start(c);
        }
        ++nextStep_;
        if (nextStep_ >= 32 * 1000) {  // keep the clock small
            nextStep_ -= 32 * 1000;
            musicClock_ -= double(32 * 1000) * double(kStep);
        }
    }
}

void Mixer::mix(float* out, int frames) {
    // Commands from the game thread.
    u32 t = tail_.load(std::memory_order_relaxed);
    const u32 h = head_.load(std::memory_order_acquire);
    while (t != h) {
        const Command& c = ring_[t % kRing];
        if (c.type == 0) {
            start(c);
        } else if (c.type == 1) {
            duckTarget_ = 0.5f;  // -6 dB
            duckHold_ = c.a;
        } else if (c.type == 2) {
            effectsOn_ = c.a > 0.5f;
            musicOn_ = c.b > 0.5f;
        }
        ++t;
    }
    tail_.store(t, std::memory_order_release);
    // Any length (the offline renders ask for seconds at once): chunks of at most kChunk frames.
    for (int done = 0; done < frames; done += kChunk) mixChunk(out + done * 2, minv(kChunk, frames - done));
    musicLevelOut_.store(musicLevel_, std::memory_order_relaxed);
    mixCalls_.fetch_add(1, std::memory_order_relaxed);
}

void Mixer::mixChunk(float* out, int frames) {
    std::memset(out, 0, sizeof(float) * 2 * size_t(frames));
    // Music events in blocks of 64 frames, then every voice.
    for (int base = 0; base < frames; base += 64) musicStep(minv(64, frames - base));
    // The music's level glides to the MUSIC switch in about 0.1 s, so switching it off stops the bed without a
    // click; once silent the music voices are let go.
    const float glide = 1.0f - std::exp(-1.0f / (0.1f * float(rate_)));
    const float target = musicOn_ ? 1.0f : 0.0f;
    for (int i = 0; i < frames; ++i) {
        musicLevel_ += (target - musicLevel_) * glide;
        musicEnv_[i] = musicLevel_;
    }
    const bool musicSilent = !musicOn_ && musicLevel_ < 1e-3f;
    int inUse = 0;
    for (Voice& v : voices_) {
        if (v.sound < 0) continue;
        const bool music = isMusic(v.sound);
        if (music && musicSilent) {
            v.sound = -1;
            continue;
        }
        ++inUse;
        const Sound& s = bank_.get(v.sound);
        int i = 0;
        if (v.delay > 0) {
            const int d = minv(v.delay, frames);
            v.delay -= d;
            i = d;
        }
        for (; i < frames; ++i) {
            const int k = int(v.pos);
            if (k + 1 >= s.len) {
                v.sound = -1;
                break;
            }
            const float f = v.pos - float(k);
            const float x = (s.data[k] + (s.data[k + 1] - s.data[k]) * f) * (music ? musicEnv_[i] : 1.0f);
            out[i * 2] += x * v.gl;
            out[i * 2 + 1] += x * v.gr;
            v.pos += v.step;
        }
    }
    voicesInUse_.store(inUse);
    // The pad, crossfaded dark to bright with the tide; ducked under the win and lose.
    if (!musicSilent) {
        const float* dark = bank_.padDark();
        const float* bright = bank_.padBright();
        const int n = bank_.padLength();
        const float g = musicGain_ * 0.55f * duck_;
        const float b = intensityNow_;
        for (int i = 0; i < frames; ++i) {
            const int k = int(padPos_);
            const float x = (dark[k] + (bright[k] - dark[k]) * b) * g * musicEnv_[i];
            out[i * 2] += x;
            out[i * 2 + 1] += x;
            padPos_ += 1.0;
            if (padPos_ >= double(n)) padPos_ -= double(n);
        }
    }
    if (tone_.load()) {
        const double step = 440.0 * 6.283185307179586 / double(rate_);
        for (int i = 0; i < frames; ++i) {
            const float s = 0.12f * float(std::sin(tonePhase_));
            tonePhase_ += step;
            if (tonePhase_ > 6.283185307179586) tonePhase_ -= 6.283185307179586;
            out[i * 2] += s;
            out[i * 2 + 1] += s;
        }
    }
    // A soft limiter holds the output under -1 dBFS true peak: samples held at -1.5 dBFS leave room for the overs
    // between samples. Instant attack, ~100 ms release.
    const float thr = 0.841f;
    const float release = 1.0f - std::exp(-1.0f / (0.1f * float(rate_)));
    for (int i = 0; i < frames; ++i) {
        const float peak = std::fmax(std::fabs(out[i * 2]), std::fabs(out[i * 2 + 1]));
        if (peak * limGain_ > thr) limGain_ = thr / peak;
        else limGain_ += (1.0f - limGain_) * release;
        out[i * 2] *= limGain_;
        out[i * 2 + 1] *= limGain_;
    }
}

}  // namespace gt::audio

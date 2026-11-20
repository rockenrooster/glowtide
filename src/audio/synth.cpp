#include "audio/synth.h"

#include <cmath>

namespace gt::audio {

const int kLadderMidi[11] = {60, 62, 64, 67, 69, 72, 74, 76, 79, 81, 84};  // C major pentatonic, C4..C6

float midiHz(float midi) { return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f); }

namespace {

constexpr float kTwoPi = 6.283185307f;

struct Noise {
    u32 s = 0x1234567u;
    float next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return float(s & 0xFFFFFF) / float(0x7FFFFF) - 1.0f;
    }
};

// One-pole low-pass with cutoff in Hz.
struct OnePole {
    float y = 0;
    float run(float x, float hz, float rate) {
        const float a = 1.0f - std::exp(-kTwoPi * hz / rate);
        y += (x - y) * a;
        return y;
    }
};

std::vector<float> buffer(float seconds, int rate) { return std::vector<float>(size_t(seconds * float(rate)) + 1, 0.0f); }

void normalize(std::vector<float>& b, float peak) {
    float m = 0;
    for (float v : b) m = std::fmax(m, std::fabs(v));
    if (m <= 0) return;
    const float k = peak / m;
    for (float& v : b) v *= k;
}

// A plucked tone in one of three timbres; amplitude 1 peak before normalization.
void pluck(std::vector<float>& b, int rate, float f, int timbre, float decay, float start = 0, float gain = 1.0f) {
    Noise n;
    const int s0 = int(start * float(rate));
    for (size_t i = size_t(s0); i < b.size(); ++i) {
        const float t = float(i - size_t(s0)) / float(rate);
        float v;
        const float att = std::fmin(1.0f, t / 0.002f);
        if (timbre == 1) {         // glass: inharmonic, bright, long
            v = std::sin(kTwoPi * f * t) * std::exp(-t / (decay * 1.8f)) + 0.35f * std::sin(kTwoPi * f * 2.76f * t) * std::exp(-t / 0.15f) +
                0.15f * std::sin(kTwoPi * f * 5.4f * t) * std::exp(-t / 0.07f);
        } else if (timbre == 2) {  // bell
            v = std::sin(kTwoPi * f * t) * std::exp(-t / (decay * 2.6f)) + 0.45f * std::sin(kTwoPi * f * 2.0f * t) * std::exp(-t / 0.3f) +
                0.25f * std::sin(kTwoPi * f * 3.01f * t) * std::exp(-t / 0.2f) + 0.12f * std::sin(kTwoPi * f * 4.2f * t) * std::exp(-t / 0.12f);
        } else {                   // kalimba: fundamental, a quiet fourth harmonic, a 2 ms click
            v = std::sin(kTwoPi * f * t) * std::exp(-t / decay) + 0.25f * std::sin(kTwoPi * f * 4.0f * t) * std::exp(-t / 0.06f);
            if (t < 0.002f) v += n.next() * 0.3f * (1.0f - t / 0.002f);
        }
        b[i] += v * att * gain;
    }
}

}  // namespace

void Bank::build(int rate, int timbre) {
    rate_ = rate;
    const float R = float(rate);
    Noise n;
    auto make = [&](int id, std::vector<float>&& v, float peak) {
        normalize(v, peak);
        store_[id] = std::move(v);
        s_[id] = {store_[id].data(), int(store_[id].size())};
    };
    {  // pip: sine 880 Hz, 2 ms attack, 35 ms decay
        auto b = buffer(0.15f, rate);
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            b[i] = std::sin(kTwoPi * 880.0f * t) * std::fmin(1.0f, t / 0.002f) * std::exp(-t / 0.035f);
        }
        make(kSndPip, std::move(b), 0.5f);
    }
    for (int k = 0; k < 11; ++k) {  // the ladder and the music's plucks
        auto b = buffer(timbre == 0 ? 0.9f : 1.6f, rate);
        pluck(b, rate, midiHz(float(kLadderMidi[k])), timbre, 0.18f);
        make(kSndLadder0 + k, std::move(b), 0.9f);
        auto p = buffer(0.9f, rate);
        pluck(p, rate, midiHz(float(kLadderMidi[k])), 0, 0.25f);
        make(kSndPluck0 + k, std::move(p), 0.6f);
    }
    {  // tick: band-passed noise at ~3 kHz, 12 ms
        auto b = buffer(0.05f, rate);
        OnePole lp, hp;
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            const float x = n.next();
            const float low = lp.run(x, 4200.0f, R);
            const float band = low - hp.run(low, 2000.0f, R);
            b[i] = band * std::exp(-t / 0.012f);
        }
        make(kSndTick, std::move(b), 0.6f);
    }
    {  // pff: low-passed noise, 90 ms, falling cut-off
        auto b = buffer(0.2f, rate);
        OnePole lp;
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            b[i] = lp.run(n.next(), 2400.0f * std::exp(-t / 0.05f) + 200.0f, R) * std::exp(-t / 0.09f) * std::fmin(1.0f, t / 0.005f);
        }
        make(kSndPff, std::move(b), 0.6f);
    }
    auto burst = [&](float seconds, float decay, float cutoff, float pings, float pingBase) {
        auto b = buffer(seconds, rate);
        OnePole lp;
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            b[i] = lp.run(n.next(), cutoff * std::exp(-t / (decay * 0.6f)) + 300.0f, R) * std::exp(-t / decay);
        }
        for (int k = 0; k < int(pings); ++k)
            pluck(b, rate, pingBase * std::pow(1.26f, float(k)), 1, 0.12f, 0.02f + 0.045f * float(k), 0.35f);
        return b;
    };
    make(kSndChip, burst(0.08f, 0.03f, 6000.0f, 0, 0), 0.55f);
    make(kSndShatterSmall, burst(0.4f, 0.16f, 7000.0f, 2, 1568.0f), 0.8f);
    {
        auto b = burst(1.2f, 0.6f, 8000.0f, 4, 1318.5f);
        for (size_t i = 0; i < b.size(); ++i) {  // and a falling sine
            const float t = float(i) / R;
            const float f = 300.0f * std::exp(-t / 0.25f) + 50.0f;
            b[i] += 0.8f * std::sin(kTwoPi * f * t) * std::exp(-t / 0.35f);
        }
        make(kSndShatterBig, std::move(b), 0.95f);
    }
    {  // thump: a sine falling from 120 to 60 Hz in 80 ms
        auto b = buffer(0.25f, rate);
        float ph = 0;
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            const float f = 60.0f + 60.0f * std::exp(-t / 0.04f);
            ph += kTwoPi * f / R;
            b[i] = std::sin(ph) * std::exp(-t / 0.08f) * std::fmin(1.0f, t / 0.003f);
        }
        make(kSndThump, std::move(b), 0.85f);
    }
    {  // crack: a sharp burst plus a falling minor third (G5 -> E5)
        auto b = burst(0.5f, 0.05f, 9000.0f, 0, 0);
        pluck(b, rate, midiHz(79), 1, 0.08f, 0.0f, 0.5f);
        pluck(b, rate, midiHz(76), 1, 0.12f, 0.09f, 0.5f);
        make(kSndCrack, std::move(b), 0.85f);
    }
    {  // pop_low: 300 -> 90 Hz in 120 ms plus a click
        auto b = buffer(0.3f, rate);
        float ph = 0;
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            const float f = 90.0f + 210.0f * std::exp(-t / 0.05f);
            ph += kTwoPi * f / R;
            b[i] = std::sin(ph) * std::exp(-t / 0.12f) + (t < 0.003f ? n.next() * 0.5f : 0.0f);
        }
        make(kSndPopLow, std::move(b), 0.85f);
    }
    {  // core hit: two low sines a semitone apart, 250 ms
        auto b = buffer(0.4f, rate);
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            const float e = std::exp(-t / 0.2f) * std::fmin(1.0f, t / 0.005f);
            b[i] = (std::sin(kTwoPi * midiHz(40) * t) + std::sin(kTwoPi * midiHz(41) * t)) * e +
                   0.3f * std::sin(kTwoPi * midiHz(64) * t) * e;
        }
        make(kSndCoreHit, std::move(b), 0.85f);
    }
    {  // the win chord: C major ninth in plucks, rolled upward over 400 ms
        auto b = buffer(2.4f, rate);
        const int notes[5] = {48, 64, 67, 71, 74};
        for (int k = 0; k < 5; ++k) pluck(b, rate, midiHz(float(notes[k])), timbre, 0.6f, 0.1f * float(k), 0.8f);
        make(kSndWinChord, std::move(b), 0.9f);
    }
    const int stars[3] = {88, 91, 96};  // E6, G6, C7
    for (int k = 0; k < 3; ++k) {
        auto b = buffer(0.9f, rate);
        pluck(b, rate, midiHz(float(stars[k])), 2, 0.2f);
        make(kSndStar0 + k, std::move(b), 0.7f);
    }
    {  // lose: two soft notes falling a minor third (G4 -> E4), slow decay
        auto b = buffer(2.0f, rate);
        for (int k = 0; k < 2; ++k) {
            const float f = midiHz(k == 0 ? 67.0f : 64.0f), t0 = 0.35f * float(k);
            for (size_t i = size_t(t0 * R); i < b.size(); ++i) {
                const float t = float(i) / R - t0;
                const float vib = 1.0f + 0.004f * std::sin(kTwoPi * 5.0f * t);
                b[i] += (std::sin(kTwoPi * f * vib * t) + 0.2f * std::sin(kTwoPi * f * 2.0f * t)) * std::fmin(1.0f, t / 0.04f) *
                        std::exp(-t / 0.7f);
            }
        }
        make(kSndLose, std::move(b), 0.7f);
    }
    {  // UI tap: a short C5, quiet
        auto b = buffer(0.25f, rate);
        pluck(b, rate, midiHz(72), 0, 0.06f);
        make(kSndUiTap, std::move(b), 0.4f);
    }
    {  // the music's low pulse: C2, 200 ms
        auto b = buffer(0.4f, rate);
        for (size_t i = 0; i < b.size(); ++i) {
            const float t = float(i) / R;
            b[i] = (std::sin(kTwoPi * midiHz(36) * t) + 0.3f * std::sin(kTwoPi * midiHz(48) * t)) * std::fmin(1.0f, t / 0.01f) *
                   std::exp(-t / 0.18f);
        }
        make(kSndPulse, std::move(b), 0.8f);
    }
    // The pad: four bars at 76 bpm; chords crossfade on their bar lines; soft detuned partials through a low-pass.
    const float beat = 60.0f / 76.0f, loop = beat * 16.0f;
    const int len = int(loop * R);
    padDark_.assign(size_t(len), 0.0f);
    padBright_.assign(size_t(len), 0.0f);
    const int chords[2][5] = {{48, 52, 55, 59, 62}, {45, 48, 52, 55, 59}};  // C maj9, A min9
    for (int bright = 0; bright < 2; ++bright) {
        std::vector<float>& out = bright ? padBright_ : padDark_;
        OnePole lp;
        for (int i = 0; i < len; ++i) {
            const float t = float(i) / R;
            const float half = loop * 0.5f;
            const int c = t < half ? 0 : 1;
            const float local = c == 0 ? t : t - half;
            // Crossfade 0.6 s around each chord change (and the loop's seam).
            const float fadeIn = std::fmin(1.0f, local / 0.6f), fadeOut = std::fmin(1.0f, (half - local) / 0.6f);
            float v = 0;
            for (int k = 0; k < 5; ++k) {
                const float f = midiHz(float(chords[c][k]));
                for (int d = 0; d < 2; ++d) {
                    const float ff = f * (d ? 1.003f : 0.997f);
                    v += std::sin(kTwoPi * ff * t + float(k)) + (bright ? 0.35f : 0.12f) * std::sin(kTwoPi * ff * 2.0f * t) +
                         (bright ? 0.18f : 0.04f) * std::sin(kTwoPi * ff * 3.0f * t);
                }
            }
            v *= 0.5f + 0.5f * std::sin(kTwoPi * t / beat * 0.25f) * 0.2f;
            out[size_t(i)] = lp.run(v, bright ? 3000.0f : 900.0f, R) * fadeIn * fadeOut;
        }
        normalize(out, 0.5f);
    }
}

}  // namespace gt::audio

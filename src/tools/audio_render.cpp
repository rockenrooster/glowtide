#include "tools/audio_render.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "audio/mixer.h"
#include "game/feedback.h"
#include "sim/bots.h"
#include "sim/world.h"

using namespace gt;

namespace {

constexpr int kRate = 48000;
constexpr int kTickFrames = kRate / 60;  // one simulation tick of sound

bool writeWav(const char* path, const std::vector<float>& stereo) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const u32 frames = u32(stereo.size() / 2);
    const u32 dataBytes = frames * 4;
    auto u32le = [&](u32 v) { std::fwrite(&v, 4, 1, f); };
    auto u16le = [&](u16 v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32le(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32le(16);
    u16le(1);  // PCM
    u16le(2);
    u32le(kRate);
    u32le(kRate * 4);
    u16le(4);
    u16le(16);
    std::fwrite("data", 1, 4, f);
    u32le(dataBytes);
    std::vector<i16> pcm(stereo.size());
    for (size_t i = 0; i < stereo.size(); ++i) {
        const float x = stereo[i] < -1.0f ? -1.0f : (stereo[i] > 1.0f ? 1.0f : stereo[i]);
        pcm[i] = i16(std::lround(x * 32767.0f));
    }
    std::fwrite(pcm.data(), sizeof(i16), pcm.size(), f);
    std::fclose(f);
    return true;
}

// Mixes `frames` more frames of the mixer onto the end of `out`.
void mixInto(audio::Mixer& m, std::vector<float>& out, int frames) {
    const size_t at = out.size();
    out.resize(at + size_t(frames) * 2);
    m.mix(out.data() + at, frames);
}

// A fake event list: Feedback reads the world's events, so the result sounds go through the game's own mapping.
void sendEvent(Feedback& fb, sim::World& w, u8 type, float t) {
    w.eventCount = 1;
    w.events[0] = {type, 0, 0, 0, 0};
    fb.onTick(w, t);
    w.eventCount = 0;
}

// The flood: the greedy bot plays a level; every tick's events go through Feedback into the mixer, like the game.
// After a result the level starts again 3 s later, so the stream keeps flowing.
std::vector<float> renderFlood(int timbre, float seconds, int level) {
    audio::Mixer m;
    m.init(kRate, timbre);
    Feedback fb;
    fb.init(&m);
    static sim::World w;
    sim::Bot bot;
    const sim::LevelDef def = sim::levelFor(level);
    w.init(def, 1);
    bot.init(sim::BotKind::Greedy, 1);
    std::vector<float> out;
    out.reserve(size_t(seconds * float(kRate)) * 2 + 4096);
    const int ticks = int(seconds * 60.0f);
    int restartAt = -1;
    for (int t = 0; t < ticks; ++t) {
        const float now = float(t) / 60.0f;
        if (w.result == sim::Result::None) {
            w.tick(bot.think(w));
            fb.onTick(w, now);
            if (w.result != sim::Result::None) restartAt = t + 180;
        } else if (t >= restartAt) {
            w.init(def, u64(t));
            bot.init(sim::BotKind::Greedy, u64(t));
        }
        mixInto(m, out, kTickFrames);
    }
    return out;
}

}  // namespace

int renderAudio(const char* scene, const char* out, float seconds, int level) {
    if (!out) {
        std::fprintf(stderr, "--render-audio needs --out x.wav\n");
        return 2;
    }
    std::vector<float> pcm;
    if (!std::strcmp(scene, "timbres")) {
        // The flood through each ladder timbre: <out>_kalimba.wav, <out>_glass.wav, <out>_bell.wav.
        static const char* const names[3] = {"kalimba", "glass", "bell"};
        char base[400];
        std::snprintf(base, sizeof base, "%s", out);
        if (char* dot = std::strrchr(base, '.')) *dot = 0;
        for (int k = 0; k < 3; ++k) {
            char path[512];
            std::snprintf(path, sizeof path, "%s_%s.wav", base, names[k]);
            if (!writeWav(path, renderFlood(k, seconds > 0 ? seconds : 30.0f, level))) return 1;
            std::printf("render-audio timbres: %s\n", path);
        }
        return 0;
    }
    if (!std::strcmp(scene, "flood")) {
        pcm = renderFlood(0, seconds > 0 ? seconds : 30.0f, level);
    } else if (!std::strcmp(scene, "ladder")) {
        // The eleven ladder notes, one every half second, alone (the pitch check reads each window).
        audio::Mixer m;
        m.init(kRate, 0);
        m.setEnabled(true, false);
        mixInto(m, pcm, int(0.1f * float(kRate)));
        for (int k = 0; k < 11; ++k) {  // each note when it is due (queued ahead they would steal each other's voices)
            m.play(audio::kSndLadder0 + k, 0.8f, 0);
            mixInto(m, pcm, kRate / 2);
        }
        mixInto(m, pcm, int(0.4f * float(kRate)));
    } else if (!std::strcmp(scene, "ui")) {
        audio::Mixer m;
        m.init(kRate, 0);
        m.setEnabled(true, false);
        Feedback fb;
        fb.init(&m);
        for (int k = 0; k < 4; ++k) {
            fb.uiTap();
            mixInto(m, pcm, int(0.35f * float(kRate)));
        }
        mixInto(m, pcm, int(0.4f * float(kRate)));
        for (int k = 0; k < 3; ++k) {  // the stars land 250 ms apart
            fb.star(k);
            mixInto(m, pcm, int(0.25f * float(kRate)));
        }
        mixInto(m, pcm, int(1.5f * float(kRate)));
    } else if (!std::strcmp(scene, "win") || !std::strcmp(scene, "lose")) {
        // Two seconds of the music bed, the result through Feedback (with its duck), the stars on a win.
        const bool win = !std::strcmp(scene, "win");
        audio::Mixer m;
        m.init(kRate, 0);
        m.setIntensity(0.6f);
        Feedback fb;
        fb.init(&m);
        static sim::World w;
        w.init(sim::levelFor(5), 1);
        mixInto(m, pcm, 2 * kRate);
        sendEvent(fb, w, win ? sim::kEvWin : sim::kEvLose, 2.0f);
        const float total = seconds > 0 ? seconds : 6.0f;
        if (win) {
            mixInto(m, pcm, int(1.6f * float(kRate)));
            for (int k = 0; k < 3; ++k) {
                fb.star(k);
                mixInto(m, pcm, int(0.25f * float(kRate)));
            }
        }
        const int left = int(total * float(kRate)) - int(pcm.size() / 2);
        if (left > 0) mixInto(m, pcm, left);
    } else if (!std::strcmp(scene, "music")) {
        // The bed alone at a mid tide (no effects), long enough to repeat the four-bar loop several times.
        audio::Mixer m;
        m.init(kRate, 0);
        m.setEnabled(false, true);
        m.setIntensity(0.5f);
        const int frames = int((seconds > 0 ? seconds : 60.0f) * float(kRate));
        for (int done = 0; done < frames; done += 1024) mixInto(m, pcm, done + 1024 <= frames ? 1024 : frames - done);
    } else {
        std::fprintf(stderr, "unknown scene %s (ladder, flood, ui, win, lose, music, timbres)\n", scene);
        return 2;
    }
    if (!writeWav(out, pcm)) return 1;
    std::printf("render-audio %s: %s (%.1f s)\n", scene, out, double(pcm.size() / 2) / double(kRate));
    return 0;
}

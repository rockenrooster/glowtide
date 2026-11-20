#pragma once
// The mixer (TECH_SPEC 6): up to 32 voices of baked sounds, the music bed, a soft limiter at -1 dBFS. Pure code: no
// SDL. The game thread sends commands through a lock-free single-producer single-consumer ring; the audio thread
// mixes. The callback never locks, allocates or logs.

#include <atomic>

#include "audio/synth.h"

namespace gt::audio {

struct Command {
    u8 type = 0;   // 0 play, 1 duck, 2 settings
    u8 sound = 0;
    float gain = 1, pan = 0, pitch = 1, delay = 0;  // delay in seconds
    float a = 0, b = 0;
};

class Mixer {
public:
    static constexpr int kVoices = 32;
    void init(int sampleRate, int timbre);
    // Game thread.
    bool push(const Command& c);  // false if the ring is full (the command is dropped and counted)
    void play(int sound, float gain, float pan, float pitch = 1.0f, float delay = 0.0f);
    void duck(float seconds);
    void setEnabled(bool effects, bool music);
    void setIntensity(float x) { intensity_.store(x); }  // 0..1 from the tide; below 0 the pad alone (HUSH)
    void setTone(bool on) { tone_.store(on); }
    int dropped() const { return dropped_.load(); }
    int pushed() const { return int(head_.load()); }  // commands accepted since init (tests)
    // The music bed's current level (0 silent .. 1) and how many times the audio thread has mixed (checks).
    float musicLevel() const { return musicLevelOut_.load(std::memory_order_relaxed); }
    int mixCalls() const { return mixCalls_.load(std::memory_order_relaxed); }
    int voicesInUse() const { return voicesInUse_.load(); }
    // Audio thread.
    void mix(float* stereo, int frames);
    const Bank& bank() const { return bank_; }

private:
    struct Voice {
        int sound = -1;
        float pos = 0, step = 1;
        float gl = 0, gr = 0;
        int delay = 0;
        u32 age = 0;
    };
    void start(const Command& c);
    void mixChunk(float* out, int frames);
    void musicStep(int frames);

    Bank bank_;
    int rate_ = 48000;
    Voice voices_[kVoices];
    static constexpr int kRing = 512;
    Command ring_[kRing];
    std::atomic<u32> head_{0}, tail_{0};
    std::atomic<int> dropped_{0}, voicesInUse_{0};
    std::atomic<float> intensity_{0};
    std::atomic<bool> tone_{false};
    bool effectsOn_ = true, musicOn_ = true;
    float musicGain_ = 0.54f;  // the music render sits 9 dB under the flood render (P4 measurements)
    float duck_ = 1, duckTarget_ = 1, duckHold_ = 0;
    double padPos_ = 0;
    double musicClock_ = 0;  // seconds
    int nextStep_ = 0;
    u32 pluckRng_ = 0x9E3779B9u;
    float limGain_ = 1;
    u32 voiceAge_ = 0;
    double tonePhase_ = 0;
    float intensityNow_ = 0;
    static constexpr int kChunk = 1024;
    float musicLevel_ = 1;
    std::atomic<float> musicLevelOut_{1};
    std::atomic<int> mixCalls_{0};
    float musicEnv_[kChunk] = {};
};

}  // namespace gt::audio

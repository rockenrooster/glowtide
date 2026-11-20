#pragma once
// The audio device (TECH_SPEC 6): an SDL audio stream, float32 stereo 48 kHz, filled from its callback by the mixer.
// --silent runs everything and zeroes the buffer just before it is handed over; --no-audio opens no device.

#include "core/types.h"

struct SDL_AudioStream;

namespace gt {

using AudioFill = void (*)(void* user, float* stereo, int frames);

class AudioOut {
public:
    bool open(bool silent, AudioFill fill, void* user);
    void close();
    bool isOpen() const { return stream_ != nullptr; }
    // Callback statistics (seconds): the longest fill and the longest gap since the last reset.
    float maxFillSeconds() const { return maxFill_; }
    float maxGapSeconds() const { return maxGap_; }
    float lastPeakBeforeMute() const { return peakBeforeMute_; }
    int lateCallbacks() const { return late_; }
    int callbacks() const { return calls_; }
    float bufferSeconds() const { return bufferSeconds_; }
    void resetStats() {
        maxFill_ = maxGap_ = 0;
        late_ = 0;
        calls_ = 0;
    }

private:
    static void callback(void* user, SDL_AudioStream* stream, int additional, int total);
    SDL_AudioStream* stream_ = nullptr;
    AudioFill fill_ = nullptr;
    void* user_ = nullptr;
    bool silent_ = false;
    double lastCall_ = 0;
    float maxFill_ = 0, maxGap_ = 0, bufferSeconds_ = 0;
    volatile float peakBeforeMute_ = 0;
    int late_ = 0, calls_ = 0;
    float buf_[2 * 4096];
};

}  // namespace gt

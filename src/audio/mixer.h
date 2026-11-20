#pragma once
// The mixer (TECH_SPEC 6): voices of synthesized sounds, the music bed, a soft limiter. Pure code: no SDL.
// P0: a test tone only; P4 fills it in.

#include "core/types.h"

namespace gt::audio {

class Mixer {
public:
    void init(int sampleRate);
    void setTone(bool on) { tone_ = on; }
    void mix(float* stereo, int frames);

private:
    int rate_ = 48000;
    bool tone_ = false;
    double phase_ = 0;
};

}  // namespace gt::audio

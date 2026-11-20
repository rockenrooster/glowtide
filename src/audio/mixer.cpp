#include "audio/mixer.h"

#include <cmath>
#include <cstring>

namespace gt::audio {

void Mixer::init(int sampleRate) { rate_ = sampleRate; }

void Mixer::mix(float* out, int frames) {
    std::memset(out, 0, sizeof(float) * 2 * size_t(frames));
    if (!tone_) return;
    const double step = 440.0 * 2.0 * 3.14159265358979 / double(rate_);
    for (int i = 0; i < frames; ++i) {
        const float s = 0.12f * float(std::sin(phase_));
        phase_ += step;
        if (phase_ > 6.283185307179586) phase_ -= 6.283185307179586;
        out[i * 2] = s;
        out[i * 2 + 1] = s;
    }
}

}  // namespace gt::audio

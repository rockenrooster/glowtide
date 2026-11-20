#include "game/audio_out.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstring>

#include "core/clock.h"
#include "core/log.h"

namespace gt {

bool AudioOut::open(bool silent, AudioFill fill, void* user) {
    silent_ = silent;
    fill_ = fill;
    user_ = user;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        GT_WARN("audio: %s", SDL_GetError());
        return false;
    }
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 48000;
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioOut::callback, this);
    if (!stream_) {
        GT_WARN("audio: no device (%s)", SDL_GetError());
        return false;
    }
    int frames = 0;
    SDL_AudioSpec dev;
    if (SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream_), &dev, &frames)) {
        bufferSeconds_ = frames > 0 && dev.freq > 0 ? float(frames) / float(dev.freq) : 0.01f;
        GT_LOG("audio: device %d Hz, %d channels, %d frames a buffer%s", dev.freq, dev.channels, frames,
               silent_ ? " (silent: output zeroed)" : "");
    }
    SDL_ResumeAudioStreamDevice(stream_);
    return true;
}

void AudioOut::close() {
    if (stream_) SDL_DestroyAudioStream(stream_);
    stream_ = nullptr;
}

void AudioOut::callback(void* user, SDL_AudioStream* stream, int additional, int) {
    AudioOut* self = static_cast<AudioOut*>(user);
    const double t0 = nowSeconds();
    if (self->lastCall_ > 0) {
        const float gap = float(t0 - self->lastCall_);
        if (gap > self->maxGap_) self->maxGap_ = gap;
        if (self->bufferSeconds_ > 0 && gap > 2.0f * self->bufferSeconds_ + 0.004f) ++self->late_;
    }
    self->lastCall_ = t0;
    ++self->calls_;
    int frames = additional / int(sizeof(float) * 2);
    while (frames > 0) {
        const int n = frames < 4096 ? frames : 4096;
        if (self->fill_) {
            self->fill_(self->user_, self->buf_, n);
        } else {
            std::memset(self->buf_, 0, sizeof(float) * 2 * size_t(n));
        }
        if (self->silent_) {
            float peak = 0;
            for (int i = 0; i < n * 2; ++i) peak = std::fmax(peak, std::fabs(self->buf_[i]));
            if (peak > 0) self->peakBeforeMute_ = peak;
            std::memset(self->buf_, 0, sizeof(float) * 2 * size_t(n));
        }
        SDL_PutAudioStreamData(stream, self->buf_, int(sizeof(float) * 2 * size_t(n)));
        frames -= n;
    }
    const float fill = float(nowSeconds() - t0);
    if (fill > self->maxFill_) self->maxFill_ = fill;
}

}  // namespace gt

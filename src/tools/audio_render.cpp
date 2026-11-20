#include "tools/audio_render.h"

#include <cstdio>

int renderAudio(const char* scene, const char* out, float seconds) {
    (void)seconds;
    std::fprintf(stderr, "render-audio %s -> %s: not built yet (P4)\n", scene, out ? out : "?");
    return 2;
}

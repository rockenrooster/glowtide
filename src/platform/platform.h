#pragma once
// Platform specifics behind one small interface (TECH_SPEC 3.3, 3.5, 8.3): haptics, the panel's refresh rate,
// where output files go, and the display's present timestamps (Android EGL_ANDROID_get_frame_timestamps).

#include <cstddef>

#include "core/types.h"

struct SDL_Window;

namespace gt::platform {

bool isAndroid();
// Pins the calling thread to the CPUs with the highest capacity (Android big cores); returns how many (0: none).
int pinToFastCores(char* desc, size_t cap);
void init(SDL_Window* window);

// Haptic kinds (GAME_DESIGN 13): 0 light tick, 1 medium, 2 strong, 3 win pattern, 4 lose buzz, 5 double tick.
void haptic(int kind);

float refreshRateHz(SDL_Window* window);
// Asks the display for the mode nearest `hz` (Android; the one-way 60 Hz fallback of TECH_SPEC 5.10).
void requestRefreshRate(float hz);
// The updater (docs/RELEASE_PIPELINE.md): asks GitHub for the latest release's build number on a thread of its own
// (Android; a no-op on the desktop); latestBuild() is 0 until an answer arrives.
void checkForUpdate();
int latestBuild();
constexpr const char* kReleasesUrl = "https://github.com/rockenrooster/glowtide/releases/latest";

// A path for an output file: absolute paths stay; relative ones go to the app's external files directory on
// Android and stay relative (to the working directory) on the desktop.
void outputPath(const char* in, char* out, size_t n);

// Present timestamps. beforeSwap() once a frame before SDL_GL_SwapWindow; collect() returns display times (seconds)
// of frames that have been shown since the last call, in order. available() is false when the extension is missing.
void presentInit(SDL_Window* window);
bool presentAvailable();
void presentBeforeSwap();
int presentCollect(double* out, int max);

}  // namespace gt::platform

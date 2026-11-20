# GLOWTIDE — third-party code and assets

Everything not listed here is the project's own: code, shaders, generated meshes, synthesized sounds and music, the
app icon (`tools/make_icon.py`).

| Path | What | Origin | Author | Licence | Date added | Notes |
|---|---|---|---|---|---|---|
| `third_party/SDL/` (not committed) | SDL 3.4.20, commit `f00dc919ca35b9756316e86c4777392de442f9b2` | Cloned from RAVEFORM's local checkout of `github.com/libsdl-org/SDL`, tag `release-3.4.20` | Sam Lantinga and contributors | zlib | 2026-11-20 | Static in the desktop build, `libSDL3.so` in the APK; its Java glue is compiled into the APK |
| `third_party/khronos/` (not committed) | `GLES3/gl3.h`, `GLES3/gl3platform.h`, `KHR/khrplatform.h` | Copied from NDK r29's sysroot | The Khronos Group | Apache 2.0 / MIT-style (per the notice in each header) | 2026-11-20 | Desktop build only |
| `assets/font/Roboto-Black.ttf` | Roboto Black | Android SDK `platforms/android-29/data/fonts/` (AOSP) | Christian Robertson / Google | Apache 2.0 (`assets/font/LICENSE.txt`) | 2026-11-20 | Only a signed-distance-field atlas baked from it ships (`src/gfx/font_atlas.inc`); its corners are rounded at bake time |

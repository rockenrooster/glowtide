# GLOWTIDE — release checklist

What the release build is, what was checked, and the steps only the owner can take. Built with
`scripts\build-android.ps1 -Config release -Offline`; the APK is
`android\app\build\outputs\apk\release\app-release.apk`.

## The build

| Item | Value | Checked how |
|---|---|---|
| Package | `dev.glowtide.game`, version 0.1.0 (code 1) | `aapt2 dump badging` |
| Android | minSdk 29 (Android 10), targetSdk 36 | `android/app/build.gradle` |
| ABI | arm64-v8a only | `aapt2 dump badging` (`native-code`) |
| Native code | CMake `Release`, stripped; `libmain.so` 1.06 MB, `libSDL3.so` 1.78 MB | `unzip -l` |
| Size | 3.31 MB (limit 15 MB) | file size |
| 16 KB pages | Shared libraries stored uncompressed and 16 KB aligned in the APK; every `LOAD` segment aligned to 0x4000 | `zipalign -c -P 16 -v 4` (build-tools 36), `llvm-readelf -l` |
| Debuggable | No | `aapt2 dump badging` (no `application-debuggable`) |
| App arguments | Ignored: `getArguments()` returns nothing unless the build is debuggable | `GlowtideActivity.java`; launch with `--es args` on the Moto (see PROGRESS.md) |
| Permissions | One: `android.permission.VIBRATE` (haptics) | `aapt2 dump badging` |
| Network | None: no INTERNET permission, no analytics, no ads, no accounts | Manifest; there is no network code |
| Data collected | None. The only file written is the save (`save.txt`: level, stars, settings) in the app's private storage | `src/game/save.cpp` |
| Orientation | Portrait | Manifest and `SDL_HINT_ORIENTATIONS` |
| Name and icon | "GLOWTIDE"; adaptive icon (a gold wisp on the twilight gradient) from `tools/make_icon.py` | `aapt2 dump badging` (`application-label`) |

## Content notes for a store listing

- **Content rating:** no violence beyond abstract lights cancelling out, no text chat, no user content, no purchases,
  no ads, no data sharing. Expected rating: Everyone / PEGI 3 (the questionnaire is the owner's to fill).
- **Photosensitivity:** the game never changes the whole picture's brightness more than three times a second
  (measured by `scripts\flash-audit.ps1` on the win, the boss win, the lose and the core hit, with and without REDUCE
  EFFECTS: at most two swings of 10% in any second). REDUCE EFFECTS removes camera shake, slow motion and the edge
  flashes and halves the bloom.
- **Accessibility:** AUTO-FIRE for players who cannot hold; gates carry symbols (× and ÷), not only colours; the two
  crowds differ by shape (teardrops against horned blobs), checked on a greyscale sheet.
- **Credits** (in SETTINGS): Roboto (Apache 2.0), SDL 3 (zlib). Everything else, the sound and the music included,
  is made in code. See `docs/LICENSES.md`.

## The owner's own steps (not done for you)

1. **A release key.** The release build is signed with the debug key so it installs for testing. Create your upload
   key (Android Studio, *Generate Signed Bundle*, or `keytool`), keep it and its passwords out of this repository,
   and add a `signingConfigs.release` block to `android/app/build.gradle` that reads them from your own
   `keystore.properties` (not committed).
2. **An App Bundle** for Google Play: `gradlew bundleRelease` with your key (Play wants an `.aab`, not an `.apk`).
3. **The store listing:** the name, a short and a long description, the icon at 512 px (`tools/make_icon.py` can
   render it larger), a feature graphic, and phone screenshots (`scripts\capture-android.ps1` takes them from a
   device; `scripts\review.ps1` renders them on the PC).
4. **The content rating questionnaire, the data-safety form** ("no data collected or shared") and the target audience.
5. **A privacy policy URL** if the store asks for one even with no data collected (a one-paragraph page saying so).
6. **Version numbers:** raise `versionCode` in `android/app/build.gradle` and `project(... VERSION)` in
   `CMakeLists.txt` for every upload.

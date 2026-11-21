# GLOWTIDE — release checklist

What the release build is, what was checked, and the steps only the owner can take. Built with
`scripts\build-android.ps1 -Config release -Offline`; the APK is
`android\app\build\outputs\apk\release\app-release.apk`.

## The build

| Item | Value | Checked how |
|---|---|---|
| Package | `dev.glowtide.game`, version 0.2.0 (code 1; GitHub builds number themselves `build-<n>`) | `aapt2 dump badging` |
| Android | minSdk 29 (Android 10), targetSdk 36 | `android/app/build.gradle` |
| ABI | arm64-v8a only | `aapt2 dump badging` (`native-code`) |
| Native code | CMake `Release`, stripped; `libmain.so` 1.37 MB (the 600-level table, the recipes and the ten worlds' music in it), `libSDL3.so` 1.78 MB | `unzip -l` |
| Size | 3.60 MB (limit 15 MB; 0.1.0 was 3.31 MB) | file size |
| 16 KB pages | Shared libraries stored uncompressed and 16 KB aligned in the APK; every `LOAD` segment aligned to 0x4000 | `zipalign -c -P 16 -v 4` (build-tools 36), `llvm-readelf -l` |
| Debuggable | No | `aapt2 dump badging` (no `application-debuggable`) |
| App arguments | Ignored: `getArguments()` returns nothing unless the build is debuggable | `GlowtideActivity.java`; launch with `--es args` on the Moto (see PROGRESS.md) |
| Permissions | `android.permission.VIBRATE` (haptics) and `android.permission.INTERNET` (the update check); GitHub builds also `android.permission.REQUEST_INSTALL_PACKAGES` (the in-app updater; never in a build for Play) | `aapt2 dump badging` |
| Network | One request a launch, from GitHub builds only: the latest release's number from GitHub's API (docs/RELEASE_PIPELINE.md). No analytics, no ads, no accounts | `GlowtideActivity.checkForUpdate` |
| Data collected | None. The only file written is the save (`save.txt`: level, stars, settings) in the app's private storage; the update request sends nothing but itself | `src/game/save.cpp` |
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
  crowds differ by shape (teardrops against horned blobs), and a geode is the only round faceted thing with a number,
  checked on greyscale sheets.
- **What the listing can say (0.2):** 600 levels, no two alike within forty of each other, then mirrored replays; ten
  worlds with their own colours, sky, props and music; six kinds of level (sieges, holdouts, colossi, twin spires,
  festivals, bosses), geodes holding seven boons, eight twists, twelve set pieces. Still no ads, purchases, accounts
  or data collected.
- **Credits** (in SETTINGS): Roboto (Apache 2.0), SDL 3 (zlib). Everything else, the sound and the music included,
  is made in code. See `docs/LICENSES.md`.

## GitHub releases (done 2026-11-20)

Every push to `main` of the public repository `rockenrooster/glowtide` builds and publishes a release APK
(`docs/RELEASE_PIPELINE.md`). Since build 26 every build is signed with the owner's release key (set 2026-11-21), and from
build 27 the game's UPDATE button downloads and installs the newer build itself. Builds 1 to 23 had keys of their own:
a copy of one of them is uninstalled once and a newer build installed.

## The owner's own steps (not done for you)

1. **The release key: done 2026-11-21** (`scripts\release-key.ps1`; certificate CN=GLOWTIDE). Back up
   `keys\glowtide-release.keystore` and its password off this PC: without them no later build can update an installed
   copy. A local release build signs with it when `GT_KEYSTORE` and its three companions are set
   (`android/app/build.gradle`).
2. **An App Bundle** for Google Play: `gradlew bundleRelease` with your key (Play wants an `.aab`, not an `.apk`), built
   without `GT_BUILD_NUMBER` so it has no updater (Play's policy: apps update through Play).
3. **The store listing:** the name, a short and a long description, the icon at 512 px (`tools/make_icon.py` can
   render it larger), a feature graphic, and phone screenshots (`scripts\capture-android.ps1` takes them from a
   device; `scripts\review.ps1` renders them on the PC).
4. **The content rating questionnaire, the data-safety form** ("no data collected or shared") and the target audience.
5. **A privacy policy URL** if the store asks for one even with no data collected (a one-paragraph page saying so).
6. **Version numbers:** raise `versionCode` in `android/app/build.gradle` and `project(... VERSION)` in
   `CMakeLists.txt` for every upload.

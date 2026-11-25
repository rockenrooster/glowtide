# GLOWTIDE — the release pipeline (GitHub) and the updater

Set up on 2026-11-20 at the owner's request: "public repo with the release, with an updater like raveform". It follows
RAVEFORM's pipeline (`Raveform/docs/RELEASE_PIPELINE.md`) point for point.

## Where

- The repository: `rockenrooster/glowtide` on GitHub, **public**, the remote `origin` of this checkout. Sessions may
  push without asking (the owner, 2026-11-21; CLAUDE.md, Conventions); nothing secret or personal goes in a commit.
- The workflow: `.github/workflows/release.yml` ("Release APK").

## What runs, and when

- Every push to `main`, unless everything it changes is documentation (`docs/**`, any `*.md`) or the browser build's own
  files (`web/**`, `scripts/*-web.ps1`, `tools/serve_web.py`, `tools/web_check.mjs`: Netlify serves that build, the APK
  has none of them). A push whose last commit's message holds `[skip ci]` runs nothing (GitHub's rule): the browser port
  went up that way on 2026-11-25, since its changes to the shared source build only into the browser version and the
  Android game stayed as build 29 has it.
- By hand: `gh workflow run release.yml -R rockenrooster/glowtide`, or the repository's Actions tab, "Release APK", "Run
  workflow". Run from another branch, the APK stays with the run (Actions, the run's page) for 7 days and no release is made.
- One build at a time: a newer push cancels a build still running (the newer build has its commits too).

The job (GitHub's `ubuntu-24.04` runner) does what `scripts\build-android.ps1 -Config release` does on Windows:

1. JDK 17 (Temurin); the Android SDK's NDK `29.0.14206865`, CMake `3.22.1` and platform 36 (`sdkmanager`).
2. `scripts/fetch-deps.sh`: SDL at the tag and commit `scripts/fetch-deps.ps1` pins (`release-3.4.20`,
   `f00dc919ca35b9756316e86c4777392de442f9b2`), cloned from GitHub and checked.
3. `gradlew :app:assembleRelease`: native Release, the libraries stripped, not debuggable, no app arguments, with
   `GT_BUILD_NUMBER` set (below).
4. The release `build-<N>`, N the number of commits on `main` (it only grows), titled "GLOWTIDE <versionName> build <N>"
   and marked latest. Its files: `glowtide.apk`, `glowtide.apk.sha256`, and `glowtide-symbols.zip` (the unstripped native
   libraries, for reading a crash report with `ndk-stack`). Its notes: the commit, the build time, the signing certificate
   and the commits since the previous build. The job also checks the APK's 16 KB page alignment (`zipalign -c -P 16`).

## Downloading

- The newest build is always at `https://github.com/rockenrooster/glowtide/releases/latest/download/glowtide.apk`; every
  build is on the repository's Releases page. Anyone can download them, without a GitHub account.
- On a phone: open the link (or the release page) in a browser, download, open the file; Android asks once to allow
  installs from that browser.
- On this PC: `gh release download -R rockenrooster/glowtide -p glowtide.apk`, then `adb -s <serial> install -r glowtide.apk`
  (the shared-phone rules of CLAUDE.md apply).

## Signing

The workflow signs with the key in four repository secrets, when they are set (`android/app/build.gradle` reads them as the
environment variables `GT_KEYSTORE`, `GT_KEYSTORE_PASSWORD`, `GT_KEY_ALIAS`, `GT_KEY_PASSWORD`):

| Secret | Holds |
|---|---|
| `GT_KEYSTORE_BASE64` | the keystore file, in base64 |
| `GT_KEYSTORE_PASSWORD` | the keystore's password |
| `GT_KEY_ALIAS` | the key's alias |
| `GT_KEY_PASSWORD` | the key's password |

**The owner makes the key** with `scripts\release-key.ps1` (keytool asks for a new password; the script asks for it once
more, checks that it opens the keystore, sets the four secrets through gh's standard input, never on a command line, prints
the certificate's SHA-256 and starts a release build; `-SecretsOnly` uploads an existing keystore again, `-NoBuild` starts
no build). The keystore goes to `keys\glowtide-release.keystore`, ignored by git (`/keys/`, `*.keystore`, `*.jks`, `*.p12`,
`*.pfx`): never commit it, the repository is public. Back the keystore and its password up off this PC: a build signed with
any other key cannot update an installed copy.

**The key is set** (2026-11-21, from build 26 on; certificate CN=GLOWTIDE, SHA-256 `82cb59e0...c9b3` in full in the
release notes). Builds 1 to 23 were signed with a key of their own: a copy of one of them cannot be updated, by the
updater or by Android; uninstall it once and install a newer build.

**Without the secrets** (as on the first builds) the APK is signed with a debug key the run makes for itself, a different
one every run. Android installs an update only when it is signed with the same key as the installed app, so such an APK
installs only where GLOWTIDE is not installed: uninstall first, and the phone's progress goes with it. The release notes and
the run's summary say which key signed each build. This PC's builds (`deploy-android.ps1`) stay debug-signed with this PC's
own key, so a GitHub build and a deployed build replace each other only through an uninstall; to test beside a GitHub
build instead, build the dev APK with `GT_APP_ID_SUFFIX=.dev` (package `dev.glowtide.game.dev`).

## The updater

A button that shows when an update is available; from build 27 it downloads the release in the game, checks it and opens
Android's own update dialog (Trueform's in-app updater, `ApkUpdater.java` and `ApkInstallReceiver.java`).

- Every GitHub build carries its build number: the workflow sets `GT_BUILD_NUMBER` (the commit count, the number in its tag
  `build-<n>`), `android/app/build.gradle` passes it to CMake (`-DGT_BUILD_NUMBER`), and `cmake/BuildStamp.cmake` writes it
  into `build_stamp.h`. SETTINGS shows it (`VERSION 0.1.0 BUILD 17 (...)`). A local build has 0 and never asks.
- A build with a number asks GitHub once a launch, as HOME first shows (`Game::drawHome`, `platform::checkForUpdate`,
  `GlowtideActivity.checkForUpdate`): on a thread of its own it reads
  `https://api.github.com/repos/rockenrooster/glowtide/releases/latest` (no account; GitHub allows 60 such requests an hour
  from an address), takes the number from `tag_name`, and hands it to native code (`nativeUpdateFound`). Nothing is sent
  but the request; no answer (offline, rate-limited) means no button. This is the only use of the INTERNET permission.
- While that number is higher than the build's own, HOME shows a gold `UPDATE: BUILD <n>` pill under the top row. Tapping
  it (`platform::startUpdate`, `GlowtideActivity.startUpdate`, `ApkUpdater.downloadAndInstall`): the first time Android
  opens "Install unknown apps" for GLOWTIDE (allow, back, tap again); then the APK downloads into the app's cache with
  its progress on the pill (`DOWNLOADING 42%`), is checked (its size, the SHA-256 in `glowtide.apk.sha256`, the same
  package, a higher `versionCode`, the same signing key) and goes to a `PackageInstaller` session, whose confirmation
  (`ApkInstallReceiver`) is Android's "Do you want to update this app?". Any failed check is a message (a Toast) and
  nothing installed. The desktop opens the release page instead.
- The permission it needs (`REQUEST_INSTALL_PACKAGES`) and its receiver are in `android/app/src/github/AndroidManifest.xml`,
  which `build.gradle` merges only into a release built with a build number: a local or Play build has neither.
- The picture `--picture home_update` shows the pill as if GitHub had build 999 (desktop builds never ask).
- A build for the Play store must not have it (Play's policy: an app updates only through Play): build it without
  `GT_BUILD_NUMBER`.

## Version

`versionName` is `android/app/build.gradle`'s (kept equal to `CMakeLists.txt`'s project VERSION); SETTINGS shows it with
the build number and the commit. `versionCode` is the build number on GitHub builds (from build 27; Android and the
updater know "newer" by it) and 1 on local builds.

## Cost

GitHub's standard runners cost nothing for a public repository. Release files count toward no storage quota.
Documentation-only pushes do not build.

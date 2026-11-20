# P0 — Bootstrap

Size M. Needs nothing. Not a gate.

**Goal.** An empty but real app on the PC and on a phone, with every tool the later phases lean on:
one command builds and runs the desktop build silent and off screen, one command builds, installs and
launches on a named device, and screenshots, frame strips, benchmarks and tests all work. No game yet.

## Read first

`PLAN.md` (all of it). `TECH_SPEC.md` sections 1, 2, 3, 8.2, 8.3, 9 and 10. Then skim the RAVEFORM files
listed in TECH_SPEC 9: most of this phase is adapting them.

## Ask the owner first (one message, then keep working on what does not depend on the answers)

1. Confirm or change the decisions in PLAN 2, with `docs/concept/concept_gameplay.png` in front of them:
   name, theme and look, portrait and one thumb, the custom engine, no meta game.
2. `git init` inside `Glowtide\`, local only?
3. May SDL be cloned from RAVEFORM's local checkout and the Khronos headers copied from the NDK? (No
   download; this is the dependency list of TECH_SPEC 1 minus the font, which P1 asks for.)
4. Which test devices are plugged in or reachable now, and may they be used under PLAN 3.3?

The name appears in exactly three places so a rename stays cheap: `$GT_APP_ID` and `$GT_ACTIVITY` in
`env.ps1`, `applicationId`/`namespace` in `app/build.gradle`, and the title string.

## Tasks

1. **Skeleton.** The folders of PLAN 4. `.gitignore` (`build/`, `captures/`, `third_party/`,
   `android/.gradle/`, `android/app/build/`, `android/app/.cxx/`, `*.apk`). `.gitattributes` (`gradlew`
   is LF; PNG is binary).
2. **`scripts/env.ps1`**, adapted from RAVEFORM's: tool paths per process, the four-core affinity code
   unchanged, `$GT_JOBS = 4`, the device table and name resolution, the adb and shared-device helpers,
   plus two helpers that take and release the device lock in the shared folder (PLAN 3.3, rule 4; the
   logic of RAVEFORM's `audio-device.ps1`, lines 23–34). Drop everything about Vulkan.
3. **`scripts/fetch-deps.ps1`.** Clone SDL from `..\Raveform\third_party\SDL` at `release-3.4.20` with
   the Git for Windows binary and check that `HEAD` is the commit in TECH_SPEC 1. If cloning from that
   shallow checkout fails, copy the tree without `.git` and write the commit to `third_party\SDL.commit`.
   Copy the three Khronos headers into `third_party\khronos\`. Re-running does nothing.
4. **CMake** per TECH_SPEC 2.1, with just enough source to link every target: `gt_core` (arguments,
   log, random stream, histogram, PNG writer, small vector and matrix types), `gt_sim` holding a
   placeholder `World` (a seeded cloud of points that move at fixed velocities and wrap; it has
   `tick`, `hash` and read-only arrays, so the real one drops in later), `gt_audio` as a stub, the app,
   the headless tool with `--sim determinism`, and the tests.
5. **Desktop app.** Window and context with the ES-first probe and the debug-context flag, the function
   loader, the debug callback, the frame loop of TECH_SPEC 3.2. Two scenes: `bench_idle` draws the
   placeholder world as 1,000 instanced quads placed between ticks in the vertex shader (this proves
   instancing, attribute divisors and the in-between-ticks placement the real units will use);
   `touch_test` draws the pointer, a launcher marker moved by the drag, and the frame interval as a bar.
   The camera does not exist yet, so the drag uses the reference rig's constant: 92 pixels per metre at
   1,080 pixels of frame width, times the gain of GAME_DESIGN 4. The arguments of TECH_SPEC 2.5 that
   already mean something (including `--res`, `--dp-scale`, `--safe`), and the exit codes.
6. **Captures.** The framebuffer-object target sized by `--res`, read-back and PNG; `--screenshot`,
   `--frames`, `--strip`, `--strip-step`; `tools/strip.py`; `scripts/run-desktop.ps1` with
   `-Screenshot`, `-Small`, `-Sound`, `-Watch`.
7. **Audio stub.** Open the SDL stream and feed it silence, honouring `--silent` and `--no-audio`, so
   opening and closing the device across the Android lifecycle is exercised from day one. `--tone`
   plays a quiet 440 Hz tone (for a phone; PC runs stay silent).
8. **Timing.** The interval histogram, the main-thread phases, the heap allocation counter, the
   `GTBENCH` JSON line and file, the Android present timestamps and the late-frame count (TECH_SPEC 8.3).
9. **Android.** The Gradle project, manifest and activity of TECH_SPEC 2.4, all of it: the orientation
   and back-key hints, the display mode choice behind `--fps`, the `haptic` method. A generated
   placeholder icon. `build-android.ps1`, `deploy-android.ps1`, `capture-android.ps1`,
   `lifecycle-android.ps1`, `bench-android.ps1`. Build with `-Offline` first; if Gradle needs the
   network, stop and ask.
10. **Device bring-up.** On every reachable device: log the GL strings, extensions, EGL config, dp
    scale and refresh rates and put them in `PROGRESS.md`; correct the dp scales in TECH_SPEC 5.2's
    table; confirm the surface equals the display mode in portrait; run `bench_idle`.
11. **Tests.** `glowtide_tests` with the first real asserts (random stream reference values, histogram
    percentiles, argument parsing, the PNG writer's output opened by PIL in `test.ps1`); headless
    determinism on the placeholder world; `scripts/test.ps1`.
12. **Docs.** `CLAUDE.md` (only commands that work today; the owner's PC and device rules; where the
    docs are). `docs/PROGRESS.md` (phase status table, measured device facts, benchmark table,
    deviations, open questions, what was copied from RAVEFORM). `docs/LICENSES.md` (SDL, Khronos headers).

## Exit checks

Desktop checks use the `debug` configuration (TECH_SPEC 2.2).

- [ ] `scripts\build-desktop.ps1` succeeds from a clean tree after `scripts\fetch-deps.ps1`.
- [ ] `scripts\run-desktop.ps1 -AppArgs '--scenario bench_idle --seconds 60'` exits 0: no GL error, no
      debug message (the log confirms a debug context), shaders validated at build time. The log says
      which context path is in use.
- [ ] `scripts\run-desktop.ps1 -Screenshot captures\p0\idle.png -Res 1080x2400 -Small` writes a
      1080×2400 PNG with the window off screen; you opened it and saw the quads.
- [ ] `--strip 8 --strip-step 5` writes eight frames and a sheet; the quads' logged positions match
      the scripted velocities (a numeric check in the test script, not an impression).
- [ ] The run made no sound and showed no window; `env.ps1` reports an affinity of at most four CPUs.
- [ ] `scripts\test.ps1` exits 0.
- [ ] `scripts\build-android.ps1 -Offline` produces the APK. (If a networked run was needed: the owner
      approved it and `PROGRESS.md` lists what was fetched.)
- [ ] `scripts\deploy-android.ps1 -Serial moto` installs and launches; the log shows portrait, the
      whole panel (1080×2400), no system bars.
- [ ] On the tablet: launched with the default and with `--fps 60`, the granted refresh rate is logged
      and is the one asked for.
- [ ] The back key reaches the app on the Moto and on the tablet (`input keyevent BACK`, sent only
      while our app is in front; the app logs it and does not exit).
- [ ] `bench_idle` for 30 s on the Moto and on one tablet: late share at most 5%, worst frame at most
      50 ms, zero heap allocations per frame, present timestamps available (or their absence recorded).
- [ ] `scripts\lifecycle-android.ps1 -Serial moto -Cycles 20`: no crash, drawing resumes every time,
      the audio stream reopens (`GTLIFE` lines).
- [ ] If reachable, the Fire and the OnePlus: GL strings and `bench_idle` recorded. Not required.
- [ ] APK size recorded (expect well under 10 MB).
- [ ] `CLAUDE.md`, `docs/PROGRESS.md` and `docs/LICENSES.md` exist and are true.

## Owner checkpoint

The four answers above, if any is still open. Then thirty seconds of their time: run `touch_test` on
the Moto and drag a thumb. Does the marker stay under control, without lag? Does the picture stay
portrait when they turn the phone?

## Not in this phase

Gameplay, art, any shader beyond the test quad, the font, sounds, screens.

## Notes

- Do not polish `bench-android.ps1` beyond what the exit checks need; RAVEFORM's grew to 31 KB over
  months for reasons this game does not have.
- If the desktop driver refuses an ES 3.0 context, the OpenGL 3.3 path is fine; shaders are still
  validated as ESSL at build time, which is what protects the phones.
- Everything a later phase needs to know about a device (GL version, extensions, dp scale, refresh
  rates, whether present timestamps work) goes into `PROGRESS.md` now.

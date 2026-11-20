# GLOWTIDE — Master Plan

Status: written 2026-11-19 by a planning session for a different executor (a coding agent). Nothing is
built: only this `docs/` folder exists. The decisions in section 2 are the planner's proposals. The owner
has seen the concept frame but has not confirmed the decisions; phase P0 starts by getting that
confirmation.

| Doc | Purpose | When to read |
|---|---|---|
| `PLAN.md` (this) | Decisions, verified environment, performance contract, phases, working rules | Start of every session |
| `GAME_DESIGN.md` | What the game is: rules, numbers, levels, look, feel, screens, sound | Sections named by the phase |
| `TECH_SPEC.md` | How it is built: stack, build, simulation, renderer, audio, tooling | Sections named by the phase |
| `subplans/P0_bootstrap.md` … `P6_polish_release.md` | One work order per phase: tasks, files, exit checks, owner checkpoint | The phase you are executing |
| `concept/concept_gameplay.png` | The look target (rendered by `concept/make_concept.py`) | P1, and whenever a screenshot is judged |
| `concept/flux_model.py` | A toy model of the flood-against-enemy contest, behind GAME_DESIGN 5.9's numbers | P2, to compare with the real simulation |
| `PROGRESS.md` | Create in P0. Phase status, benchmark numbers per device, deviations, open questions | Start of every session; update at the end |

---

## 0. Executor briefing

1. **You cannot see motion or hear audio.** Verify with numbers, screenshots and frame strips. The owner
   is the eyes, ears and thumbs. Never report that something "looks", "sounds" or "feels" good unless the
   owner said so; report what you measured and what still needs their judgment.
2. **Keep it small.** This is a simple game on purpose. Before writing anything, ask in this order: does
   it need to exist; is it already in this repo; does the standard library or the platform do it; does a
   dependency we already have do it; can it be one line. No interface with one implementation, no
   configuration for a value that never changes, no scaffolding for later. A deliberate shortcut with a
   known ceiling gets a `// ceiling:` comment naming the limit and the upgrade path.
3. **De-risk first.** Two things can sink this: the frame rate with a full crowd on the floor phone, and
   a look that is not beautiful. P1 proves both before any gameplay is built on top. The third risk, that
   aiming the stream is not fun, is the owner checkpoint of P2.
4. **Specs are starting points, the contract is not.** Numbers in `GAME_DESIGN.md` and `TECH_SPEC.md`
   are initial values; measurement and the owner's play override them. Log each deviation in
   `PROGRESS.md` with the reason. Section 2 (decisions) and section 5 (performance contract) change only
   with the owner's explicit OK.
5. **Originality.** GLOWTIDE is an original game in the multiplier-gate crowd genre. The owner's
   reference is how that genre plays in advertisements (GAME_DESIGN 1); it is a reference for feel only.
   Do not use any existing game's name, characters, art, sounds, user interface, level layouts or
   trademarks, and do not rebuild any one of them under a new name. No stick figures, no blue-versus-red
   armies, no "champions". The theme, creatures, palette and names in GAME_DESIGN are ours.
6. **Permissions.** Downloading anything (dependencies, fonts, tools) needs the owner's explicit yes in
   the current session: list name, source, version, licence and size first. Do not change system
   environment variables or system settings; set the environment per process in scripts. Keystores,
   passwords and store accounts are the owner's to handle.
7. **The owner's standing rules** (from their CLAUDE.md files): keep chat replies short; prefer `rg`;
   small focused changes; no destructive git or shell commands unless asked; do not revert unrelated
   changes; make a manual backup before major or breaking changes (use
   `D:\Stuff\DevFolder\_backups\Glowtide\`); run builds and tests only when needed to verify; measure
   before optimizing; UI must be readable, space-efficient and have clear high-contrast pressed states.
8. **PC rules** (the owner set these for the sibling project RAVEFORM; they hold here until the owner
   says otherwise). Everything run on this PC is **silent** (`--silent`), **off screen** (`--offscreen`)
   and uses **at most 4 CPU cores** (TECH_SPEC 2.3). The owner is at the machine while you work.
9. **The test devices are shared** with other agent sessions running at the same time. Follow section
   3.3 to the letter and put it in the brief of any subagent that touches a device.
10. **RAVEFORM is a reference, not a dependency.** `D:\Stuff\DevFolder\Android_Games\Raveform\` is the
    owner's other game. It already solved this PC's toolchain traps and the shared-device etiquette
    (TECH_SPEC 9 lists where). Read it and copy what helps; note what you copied in `PROGRESS.md`. Never
    edit, build or commit inside it. The one exception is the shared device lock folder (section 3.3).

---

## 1. What we are building

GLOWTIDE is a one-thumb Android game. You hold and drag a lantern-cannon that pours a stream of little
light creatures (wisps) up a floating causeway. Gates multiply every wisp that passes through them, so a
thin stream becomes a flood. The flood meets the enemy's crowd (glooms) head on, cancels it one for one,
and what is left breaks the enemy's spire. A level lasts about half a minute. Then the next one starts.

That is the whole game. There is no base building, no cards, no currencies, no upgrades to grind, no
advertisements and no pop-ups: the owner asked for the game the advertisements of this genre show, not
the game those products turn out to be.

Pillars, in priority order:

1. **The flood.** Multiplication is visible, countable and loud: one becomes two becomes six becomes
   thirty, and a thousand wisps on screen is normal.
2. **One thumb, no friction.** Playing within two seconds of launch. Hold to pour, drag to aim, release
   to stop. Retry and next level are one tap.
3. **Beautiful and readable.** Glowing creatures on a twilight causeway; every number legible at arm's
   length; the crowd never hides a gate's label.
4. **Never drops a frame.** 60 FPS at native resolution with the full crowd on the floor phone, measured
   in every phase.
5. **A real choice every two seconds.** Which gate, the wall or the open lane, now or after the slider
   comes back, and how to hold the line when a wave comes out of the spire.

---

## 2. Decisions (proposed by the planner; the owner confirms or changes them in P0)

| Topic | Decision | Reason |
|---|---|---|
| Name | GLOWTIDE (working title) | A web search on 2026-11-19 found no game or app with this name (not a trademark clearance) |
| Theme | Wisps of light against glooms and their spire, at twilight on a floating causeway. Gold player, violet enemy, aqua good gates, rose bad gates | Original, cheap to render, glow is where a simple game looks rich; see `concept/concept_gameplay.png` |
| Orientation, input | Portrait only. One thumb: hold to fire, drag to aim (relative drag, anywhere on screen) | The genre's advertisements are portrait and one-handed |
| Engine | Custom, small: C++20, SDL 3.4.20, OpenGL ES 3.0 on Android, the same code on a Windows dev build | A crowd of thousands at 60 FPS needs native code; no engine is installed on this PC; this exact toolchain already builds and ships RAVEFORM here; fully code-driven, so an agent can build, run, measure and screenshot it. OpenGL ES instead of RAVEFORM's Vulkan: far less plumbing, and this game needs nothing Vulkan offers |
| Targets | Android arm64, minSdk 29, targetSdk 36. Windows desktop build for development only | Same as RAVEFORM, proven on the test devices |
| Performance | 60 FPS P95 at native resolution with the full crowd on the Motorola One 5G Ace; the same picture at a higher refresh rate on stronger devices. One quality level, no graphics settings; resolution is never lowered | The owner's standard on RAVEFORM, minus the tiers this game does not need |
| Scope | Level after level, generated and bot-validated. Persisted: level reached, stars, settings. No meta game | "Plays like the ads" |
| Money, network | No advertisements, purchases, analytics, accounts or internet permission | Sideloaded APK for now, kept ready for the Play Store |
| Assets | Procedural: meshes, creatures, effects, sounds and music are generated in code. One OFL font is the only downloaded asset | No art pipeline, tiny APK, nothing to license but the font |
| Dependencies | SDL (zlib licence) and nothing else in the app. Khronos headers from the NDK. Python (already installed) for build-time tools | Fewer dependencies, fewer download approvals |

---

## 3. Verified environment (checked 2026-11-19 unless noted)

### 3.1 Development PC (Windows 11 Pro, PowerShell 7.6 primary)

| Item | Fact |
|---|---|
| CPU / GPU | Ryzen 9 5950X (16 cores, 32 threads) / Radeon RX 9070 XT; monitor 2560×1440 at 60 Hz (from RAVEFORM's plan) |
| Android SDK | `C:\Users\Tyler\AppData\Local\Android\Sdk` (`ANDROID_HOME` is set to it) |
| NDK | 27.0.12077973, 27.2.12479018, 28.2.13676358, **29.0.14206865 — use this** |
| SDK platforms / build-tools | android-29, 30, 33, 35, 36, 37.0 / up to 36.0.0 |
| SDK CMake | 3.22.1 with its own `ninja.exe` — used by Gradle's externalNativeBuild |
| Native CMake | `C:\Program Files\CMake\bin\cmake.exe`, 4.4.4 — **use this for desktop builds** |
| Compiler | llvm-mingw clang 22.1.7 on PATH, target `x86_64-w64-windows-gnu` |
| Ninja / Python | ninja 1.13.2 on PATH; Python 3.14.10 with `numpy` 2.5.4 and `PIL` 13.0.0 (no scipy, no matplotlib) |
| Git | `C:\Program Files\Git\cmd\git.exe` — **use this**, with `-c http.sslBackend=schannel` for HTTPS |
| JDK | `C:\Program Files\Eclipse Adoptium\jdk-17.0.21.9-hotspot` (has `javac`) |
| Gradle | Not on PATH. Wrapper distribution 8.13 is in `~\.gradle\wrapper\dists`; Android Gradle Plugin 8.13.0 is in the Gradle cache. RAVEFORM builds with exactly this pair |
| Shader validator | `C:\VulkanSDK\1.4.363.0\Bin\glslangValidator.exe` (validates `#version 300 es` shaders at build time) |
| SDL | 3.4.20 checked out at `..\Raveform\third_party\SDL` (tag `release-3.4.20`, commit `f00dc919ca35b9756316e86c4777392de442f9b2`, shallow, 64 MB) — clone it locally, no download |
| OpenGL ES headers | `<NDK>\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include\GLES3\gl3.h`, `KHR\khrplatform.h`, `EGL\eglext.h` (declares `EGL_ANDROID_get_frame_timestamps`) |
| ffmpeg | 8.0.1 full build on PATH (no SVG support) — frame strips, WAV analysis |
| Debug keystore | `~\.android\debug.keystore` exists |
| Not installed | Android Studio, Unity, Unreal, Godot |

Environment traps — handle them in `scripts\env.ps1`, per process, without touching system settings:

- `cmake` and `git` on PATH are MSYS2 builds (`c:\devkitPro\msys2\usr\bin\`). Never use them; call the
  native ones above by full path.
- `java` on PATH is an old Oracle shim, not the JDK. Scripts set `JAVA_HOME` to the Adoptium JDK.
- The owner's `~/.gitconfig` points `http.sslcainfo` at a file that no longer exists; pass
  `-c http.sslBackend=schannel` per invocation. Do not edit the config file.
- The Gradle user home is shared with other sessions: never run `gradlew --stop`; give this project's
  daemon its own JVM options (TECH_SPEC 2.3).
- `D:\Stuff\DevFolder\Android_Games` is not a git repository. P0 asks the owner whether to `git init`
  inside `Glowtide\`.
- The Agent tool's `isolation: "worktree"` fails on this PC (it picks the MSYS git). If a parallel
  worktree is wanted, create it by hand with the Git for Windows binary as a sibling folder.

### 3.2 Test devices (from RAVEFORM's docs, measured there 2026-11-10 to 2026-11-16; OpenGL ES facts are not measured yet)

| | Floor phone | Tablet | High-tier phone | Low end |
|---|---|---|---|---|
| Device | Motorola One 5G Ace | Samsung Galaxy Tab A11+ (SM-X230) | OnePlus 12R | Amazon Fire 7 (2022) |
| adb serial | `ZY22BXMJG3` (second unit `ZY22BZ8BDZ`) | `R5GL362BEHX`, `R5GL362AKGK`, `R5GL362AGPA` | `a0e622a5` (usually over Wi-Fi) | `GN41WE04450600BJ` (over Wi-Fi) |
| SoC / GPU | Snapdragon 750G / Adreno 619 | MediaTek MT8775 / Mali-G615 MC2 | Snapdragon 8 Gen 2 / Adreno 740 | MT8168 / Mali-G52 MC1 |
| Display | 1080×2400, 60 Hz only | 1200×1920, 90 / 60 Hz | 1264×2780, up to 120 Hz | 1024×600, 60 Hz |
| Android | 11 (API 30) | 16 (API 36) | 14 or newer | Fire OS 8 (Android 11) |
| RAM | 6 GB | 6 GB | 8 or 16 GB | 2 GB |
| Role | **The contract is graded here** | 90 Hz, Mali driver | 120 Hz reference | Report only; not a gate |

P0 logs `GL_VENDOR`, `GL_RENDERER`, `GL_VERSION`, the extension list, the EGL config, the dp scale and
the refresh rates on every device that is reachable and records them in `PROGRESS.md`. Until then, assume OpenGL ES 3.0 core and nothing
more (TECH_SPEC 5.1). In portrait the tablet is 1200×1920 (5:8) and the Fire is 600×1024: the layout
must hold from 9:22 to 3:4 (TECH_SPEC 5.2).

### 3.3 Shared test devices (the owner's rule; other sessions use the same phones)

1. Nothing device-wide: no `adb kill-server` or `start-server`, no reboot, no `adb logcat -c` (read with
   `logcat -d --pid=<our pid>`), no settings changes (`settings put`, `wm size`, rotation locks), and no
   clear, uninstall or force-stop of any package but `dev.glowtide.game`.
2. Every adb command passes `-s <serial>`.
3. Before launching, benchmarking, sending key events or taking a screenshot, check the foreground
   activity. If an app that is neither the launcher nor ours is in front, someone else is using the
   device: wait and re-check, or use another device. Never start our activity over someone else's app.
4. Locks between sessions: before using a device, write
   `D:\Stuff\DevFolder\Android_Games\Raveform\captures\locks\<hardware serial>.lock` containing
   `glowtide-<phase> <date and time>`, and delete it when the run ends. The first word is the owner's
   name and the file's time is its age: a lock under 30 minutes old whose first word is not yours means
   the device is busy. This is the folder and the format RAVEFORM's sessions already use (TECH_SPEC 9);
   sharing it is the point. It is the only thing this project ever writes inside RAVEFORM.
5. Keep device runs short and leave the device tidy: after any run,
   `adb -s <serial> shell am force-stop dev.glowtide.game`.
6. A benchmark is valid only if GLOWTIDE kept focus for the whole run. The app reports focus loss; the
   script retries later. Numbers come from shared devices: say so when reporting.
7. Phones keep their sound; only PC runs are silent.

---

## 4. Architecture at a glance

```
            ┌──────────────── main thread ─────────────────┐       ┌─ audio thread (SDL callback) ─┐
 touch ───▶ │ events → input → sim ticks (fixed 60 Hz)     │ notes │ voices → mix → soft limiter   │──▶ device
            │   → sim events → view (camera, effects, UI)  │ ────▶ │ (no locks, no allocation)     │
            │   → build instance buffers → GL draw → swap  │       └───────────────────────────────┘
            └──────────────────────────────────────────────┘        one lock-free queue, game → audio
```

| Layer | Choice |
|---|---|
| Language | C++20, clang on both platforms, no exceptions, no RTTI, warnings are errors in our code |
| Platform | SDL 3.4.20: window, GL context, touch and mouse, audio device, preferences path. Android through SDL's `SDLActivity` |
| Simulation | `src/sim`: a static library with no SDL, no GL and no audio. Deterministic, fixed 60 Hz, structure-of-arrays, no allocation after start. The game, the headless tool and the tests all link it |
| Renderer | OpenGL ES 3.0 core. Units are instanced impostor quads (two triangles each), drawn far to near. A low-resolution light map lets the crowd light the ground. Bloom comes from a quarter-resolution glow buffer added in one full-screen blend; the scene is never copied at full resolution |
| Audio | SDL audio stream, own 32-voice mixer, every sound synthesized at start-up |
| UI | Own tiny layer: rounded rectangles and signed-distance-field text, in density-independent units inside the safe area |
| Tooling | Scenarios, scripted pictures, frame strips, bots, a headless simulation tool, an offline audio renderer, a layout lint, one plain test executable |
| Build | One CMake tree; a Gradle project for the APK; PowerShell scripts wrap both |

Repository layout (create in P0):

```
Glowtide/
  CLAUDE.md            build/run/test commands and conventions (write in P0, real commands only)
  docs/                PLAN.md, GAME_DESIGN.md, TECH_SPEC.md, subplans/, concept/, PROGRESS.md, LICENSES.md
  CMakeLists.txt
  src/core/            math, rng, args, log, png, timing histogram, tweakables
  src/sim/             world, units, gates, clash, enemies, levels, generator, bots (no SDL, no GL)
  src/gfx/             GL loader, shaders, meshes, renderer passes, text, particles
  src/audio/           mixer, synth recipes, music, note queue
  src/game/            app loop, states and screens, HUD, camera, juice, input, save, pictures
  src/platform/        desktop and Android specifics (haptics, frame timestamps, arguments)
  src/tools/           headless simulation tool, level bake, tests
  shaders/             *.vert, *.frag (#version 300 es), embedded at build time
  assets/font/         the OFL font and its licence
  tools/               bake_font.py, make_icon.py, strip.py, wav_metrics.py
  android/             Gradle project (application id dev.glowtide.game)
  scripts/             env.ps1, fetch-deps.ps1, build-*.ps1, run-desktop.ps1, deploy-android.ps1, bench-android.ps1, …
  third_party/SDL/     local clone of the pinned SDL (git-ignored)
  captures/            screenshots, strips, benchmark results (git-ignored)
```

---

## 5. Performance contract

**Definition.** "60 FPS P95" means: over a 60-second `bench_flood` run started after a 3-minute warm
soak of the same scene, at least 95% of the frames are on time and no frame takes longer than 50 ms.
Frames are timed on the panel side, from the display's present timestamps
(`EGL_ANDROID_get_frame_timestamps`, TECH_SPEC 8.3), counted in whole refresh periods: a frame is
**late** when it was shown two or more periods after the one before it. So the contract is two numbers:
the **late share**, at most 5%, and the **worst frame**, at most 50 ms. The interval between swap calls
is reported but not graded: it jitters by more than the margin even when no frame is late.
`bench_flood` is the heaviest picture the game can produce: both crowds at their caps, every gate
firing, particles at their budget, bloom on (TECH_SPEC 8.1). Resolution is always the panel's native
resolution; soft low-frequency effects (bloom, light map) use small buffers because that is what they
are.

| Device | Refresh target | Required |
|---|---|---|
| Motorola One 5G Ace | 60 Hz | Late share ≤ 5%, worst frame ≤ 50 ms — **the gate** |
| Galaxy Tab A11+ | 90 Hz when it holds, else 60 Hz | The same at 90 Hz, or at 60 Hz with the reason recorded |
| OnePlus 12R | 120 Hz | The same at 120 Hz |
| Fire 7 (2022) | 60 Hz | Measured and reported. Never blocks a phase (TECH_SPEC 5.10) |

Budgets on the floor phone in `bench_flood` (initial; P1 refines them):

| What | Budget |
|---|---|
| Main thread CPU | ≤ 6 ms P95: simulation tick 2.5, instance build and upload 1.5, effects and UI 1.0, slack 1.0 |
| GPU | ≤ 11 ms P95: light map 0.6, glow buffer and its blur 1.6, scene 6.0, final blend and UI 1.8, slack 1.0 |
| Audio callback | ≤ 25% of the buffer period at P99, no late callbacks |
| Heap allocations per frame | 0 in steady state (global `operator new` is counted; benchmarks report it) |
| Memory / APK | < 250 MB RSS / < 15 MB |
| Start-up | cold start to an interactive home screen ≤ 2 s on the floor phone; a touch there starts the level and the first wisp leaves in the next tick |

Rules that make the contract achievable (TECH_SPEC 4 and 5):

- The crowd caps (`CAP_W` wisps, `CAP_G` glooms; 2,400 and 1,000 to start) are set in P1, by the
  owner's eye on the density pictures and by measurement on the floor phone, and are then the same on
  every device. Past a cap the crowd grows in power, not in number (GAME_DESIGN 5.3), so nothing is
  lost at the cap.
- A unit costs two triangles, about 20 bytes of instance data and no per-unit draw call.
- Particles are stateless: written once at spawn, moved by the vertex shader.
- No heap allocation, file access or shader compile during play. The only JNI call is the haptic one,
  at most twelve times a second.
- Every phase ends with the benchmark on the floor phone and the tablet; numbers go in `PROGRESS.md`.
  A regression blocks the phase: the late share up by more than two points, or the main thread's CPU
  P95 up by more than 10%, against the previous phase's run (TECH_SPEC 8.3).

---

## 6. Assets

Everything is procedural and lives in code or build-time scripts: creature shapes (fragment shader),
props (generated meshes), particles, the backdrop, every sound and the music, the app icon
(`tools/make_icon.py`).

The one downloaded asset is the font: a rounded bold typeface under the SIL Open Font Licence. Proposed:
**Fredoka** (bold instance), from the Google Fonts repository. `tools/bake_font.py` renders it once into a
signed-distance-field atlas that is embedded in the binary; the font file and `OFL.txt` are kept in
`assets/font/`. P1 asks the owner for the download (name, source, licence, size) and shows a rendered
sample before it is locked.

Every non-procedural file and every third-party library gets a row in `docs/LICENSES.md`: path, origin,
author, licence, date. The credits screen (P6) is generated from it.

---

## 7. Phases

Sizes are relative (S, M, L). Every phase ends with: the benchmark on the floor phone and the tablet, an
update to `PROGRESS.md`, a short note to the owner saying what to try, and the owner checkpoint. While
waiting for the owner, continue with work that does not depend on the answer.

| Phase | Size | Needs | Outcome | Subplan |
|---|---|---|---|---|
| P0 Bootstrap | M | — | One command builds and runs on the PC; one command builds, installs and launches on a device; screenshots, strips and benchmarks work | `subplans/P0_bootstrap.md` |
| P1 Look and performance proof — **gate** | L | P0 | The full frame of the concept image, rendered by the engine with a synthetic full crowd, inside the contract on the floor phone. The owner approves the look | `subplans/P1_look_and_perf.md` |
| P2 Core gameplay | L | P1 | The real simulation: stream, gates, clash, spire, core, an enemy crowd sized by the bake. Levels 1–5 playable on a phone; bots and determinism tests | `subplans/P2_core_gameplay.md` |
| P3 Game loop and UI | M | P2 | Home, HUD, pause, win, lose, settings, saving, haptics, lifecycle. A complete game around five levels | `subplans/P3_game_loop_ui.md` |
| P4 Audio | M | P2 | Mixer, synthesized sounds, the pentatonic pop ladder, ambient music. Can run beside P3 | `subplans/P4_audio.md` |
| P5 Content | L | P2 (P3 for the owner's playtest) | Every lane element, the level generator, 300 bot-validated levels, five worlds, boss levels | `subplans/P5_content.md` |
| P6 Polish and release prep | M | P3, P4, P5 | Win and lose sequences, camera, transitions, icon, accessibility, soak, release build, checklist | `subplans/P6_polish_release.md` |

Order: P0 → P1 → P2, then P3, P4 and P5, then P6. P4 touches only `src/audio` and a few call sites, so
it can run in a second worktree beside P3 or P5. One writer per set of files.

Gates and checkpoints (the full exit lists are in the subplans):

- **P1 is go/no-go.** If `bench_flood` misses the contract after the fallbacks in TECH_SPEC 8.5, stop
  and bring the measured options to the owner. Do not quietly lower the resolution or
  the caps below 1,200 wisps.
- **P1 owner checkpoint:** does it look beautiful on the phone, next to the concept frame? Palette and
  font are chosen here.
- **P2 owner checkpoint:** is pouring and aiming the stream fun for thirty seconds? Drag sensitivity,
  fire rate, how fast the flood builds. This is the most important checkpoint of the project; do not
  build P3–P5 on a core the owner has not played.
- **P4 owner checkpoint:** rendered WAV files and the live build: do the pops sound good as a flood?
- **P5 owner checkpoint:** play twenty levels. Difficulty ramp, variety, level length.
- **P6:** sign-off on feel, then the final benchmark matrix on every device.

---

## 8. Working agreements

- **Verification loops.** Looks: run a scripted picture on the PC with `--screenshot`, then open the PNG;
  on a device use `adb exec-out screencap -p`. Motion: `--strip N` and tile the frames into one sheet.
  Rules and balance: the headless tool and the bots, which print one JSON line per run. Sound: render
  offline to WAV, measure with numpy, hand the file to the owner. Performance: only numbers from the
  devices count.
- **PC first, device always.** Develop against the Windows build; run on the floor phone and the tablet
  before calling anything done. Their two GPU drivers disagree with the PC's and with each other.
- **Tooling is part of the product.** Scenarios, pictures, bots, the headless tool and the layout lint are
  what make an executor that cannot see or hear effective. Build them early and keep them working.
- **Content is data.** Levels, palettes, tuning numbers, sound recipes and strings live in tables,
  separate from logic. Tunable numbers are overridable from the command line (`--set name=value`).
- **Owner-facing steps.** Say plainly when you need them: approve a download, accept an install or
  debugging prompt on a device, look, listen, play.
- **Checkpoints are real.** Do not mark a phase complete without its owner checkpoint. Report failures
  with the numbers.
- **Git** (if the owner says yes to `git init`): commit at working checkpoints with plain messages;
  never push, add a remote or rewrite history unless the owner asks. Build output, `captures/` and
  `third_party/` are ignored.

---

## 9. Risks and fallbacks

| # | Risk | Level | Mitigation / fallback order |
|---|---|---|---|
| 1 | Fill rate on the Adreno 619 at 1080×2400 with a full crowd, bloom and translucent gates | High | P1 gate. TECH_SPEC 8.5: fewer glow levels, a smaller light map, fewer particles, smaller impostor quads, a plainer backdrop, then lower caps (not below 1,200 wisps). Resolution stays native |
| 2 | "Beautiful" is subjective and the executor cannot judge it | High | The concept frame is the target; P1 ends with on-device screenshots beside it and palette variants for the owner to pick; juice is specified in numbers (GAME_DESIGN 10) |
| 3 | The core loop is not fun, or has too little threat: the flood-against-flood contest has a sharp edge, safe on one side and a collapse on the other (GAME_DESIGN 5.9) | High | P2 checkpoint before any content is built; every feel number is a tweakable; GAME_DESIGN 5.12 lists the levers that add threat, none built in advance |
| 4 | Level balance without a human player: an enemy that is either erased by the flood or overwhelming | Medium | The bake sizes each level's enemy to cancel a set share of what a bot delivers on that layout (13% early, 30% late) and its spire from the rest; a level ships only if the bot tables pass (GAME_DESIGN 7.4). A toy model run while planning (`concept/flux_model.py`) supports the rule; P2 confirms it with the real simulation. The owner plays twenty in P5 |
| 5 | GPU driver differences (2021 Adreno driver, Mali, the PC's driver) | Medium | ES 3.0 core only, shaders validated as ESSL 3.00 at build time, precision rules (TECH_SPEC 5.1), devices tested every phase |
| 6 | Simulation cost with thousands of units | Medium | Structure of arrays, one uniform grid, caps, measured by `bench_flood` and by `bench_play` on the heaviest level; fallback: separation every second tick |
| 7 | Touch feel: latency or wrong sensitivity | Medium | Input applied in the frame it arrives; relative drag with a tweakable gain; owner checkpoint in P2 |
| 8 | OpenGL context loss on Android resume | Low–Med | SDL keeps the context when it can; all GL objects can be rebuilt from CPU state; 20 background/foreground cycles in P0 and P3 |
| 9 | Synthesized sound is thin or harsh; the executor cannot hear | Medium | Offline renders with measured peak and loudness, the owner listens in P4, two or three recipe variants to choose from |
| 10 | Shared devices: another session disturbs a benchmark or is disturbed by ours | Medium | Section 3.3; the app detects focus loss; invalid runs are repeated |
| 11 | Photosensitivity | Safety | No full-screen flashes above three per second; a "reduce effects" setting (GAME_DESIGN 14) |
| 12 | Portrait lock ignored on large screens (Android 16) | Low | `android:appCategory="game"` exempts games; the layout also holds at 3:4 |
| 13 | OEM game modes downscale or cap the frame rate | Low | Game-mode configuration opting out; the app logs surface size against the display mode at start-up |
| 14 | Scope creep into a bigger game | Medium | Section 10's out-of-scope list; pillar 2; briefing rule 2 |

---

## 10. Assumed defaults (the owner can change any of these)

- Working title GLOWTIDE; `applicationId` `dev.glowtide.game`; versionName starts at 0.1.0.
- Portrait only. English only, strings in one table.
- Hold to fire. An "auto-fire" setting (fire without holding, drag still aims) exists for accessibility.
- Level 1 is unlosable; levels 2–5 teach one thing each; from level 6 the generator takes over.
- Stars are for speed (one to three per level, against the level's target time); nothing is bought or
  unlocked with them in version 1.
- The executor may copy build scripts and platform code from RAVEFORM (same owner) and records what it
  copied.
- Out of scope for version 1, not scheduled: upgrades or currencies, daily rewards, advertisements,
  purchases, online leaderboards, cloud saves, multiplayer, an endless mode, cosmetic skins, iOS,
  localisation, gamepad support, quality tiers or graphics settings, a lighter build for devices below
  the floor phone.
- Optional levers, built only if the owner asks for one after playing (GAME_DESIGN 5.12): FRENZY (a
  meter that fills as the spire takes damage and triples the fire rate for four seconds), FLANKERS
  (a few glooms down the lane's edge that must be swept), ENGAGE (wisps steer at nearby glooms).

---

## 11. Sources used while planning

- RAVEFORM's `docs/PLAN.md`, `CLAUDE.md`, `scripts/` and `android/` (the PC's toolchain, the device list,
  the owner's rules), read on 2026-11-19.
- Tool paths and versions in section 3.1 were checked on the PC on 2026-11-19.
- The name check was two web searches on 2026-11-19; nothing called "Glowtide" came up.

Library versions and API signatures in the specs come from the planner's knowledge. Check signatures
against the pinned SDL headers and the NDK's headers rather than against this text.

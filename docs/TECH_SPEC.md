# GLOWTIDE — Technical Specification

How the game is built. It locks the stack and the shape of each part, names the traps, and leaves the
code to the executor. Where this text and a pinned header disagree, the header is right. Where a simpler
way meets the same exit checks, take it and note it in `PROGRESS.md`. Section numbers are referred to
from `PLAN.md`, `GAME_DESIGN.md` and the subplans: keep them stable.

---

## 1. Stack and dependencies

| Dependency | Version | Where it comes from | Licence | Download |
|---|---|---|---|---|
| SDL | 3.4.20, commit `f00dc919ca35b9756316e86c4777392de442f9b2` | Local clone of `..\Raveform\third_party\SDL` into `third_party\SDL` | zlib | None. If the sibling checkout is gone: clone tag `release-3.4.20` from GitHub (about 65 MB) after asking |
| Khronos headers (`GLES3/gl3.h`, `GLES3/gl3platform.h`, `KHR/khrplatform.h`) | NDK r29's | Copied from the NDK sysroot into `third_party\khronos\` for the desktop build only | Per the notice in each header | None |
| Android Gradle Plugin, Gradle | 8.13.0, 8.13 | Gradle cache and wrapper distributions already on the PC | Apache 2.0 | Try `--offline` first; ask before any networked Gradle run |
| Font: Fredoka, bold | Current | `github.com/google/fonts`, `ofl/fredoka` | SIL OFL 1.1 | Yes: ask in P1 (well under 1 MB) |
| Python 3.14 with PIL and numpy | Installed | — | — | None |

Deliberately not used: Dear ImGui, GLM, stb, glad, Oboe, the frame pacing library, a JSON library, a
test framework, a physics engine, an entity-component system. Each would cost a download approval and
more code than the few lines it replaces here. (The frame pacing library 2.1.3 is in the Gradle cache
and is the fallback if plain swap-interval pacing proves uneven on a device.)

Language: C++20, clang on both platforms, `-fno-exceptions -fno-rtti`, `-Wall -Wextra -Werror` on our
code. One small Java class (the activity). PowerShell 7 scripts. Python for build-time tools only.

---

## 2. Repository, build and scripts

The layout is in PLAN 4.

### 2.1 CMake

One root `CMakeLists.txt`, valid for CMake 3.22.1 (the SDK's, used by Gradle) and 4.4.4 (desktop).

| Target | Kind | Contents |
|---|---|---|
| `gt_core` | static | `src/core`: math, rng, args, log, png, histogram, tweakables. No SDL |
| `gt_sim` | static | `src/sim`. Links `gt_core` only. Built with `-ffp-contract=off` and never with fast-math (4.2) |
| `gt_audio` | static | `src/audio` mixer, synth and music: pure code, no SDL (the device is opened by the app) |
| `glowtide` | Desktop: executable. Android: shared library named `main` (SDL's convention) | `src/gfx`, `src/game`, `src/platform`; links SDL3 and the three libraries; on Android also `GLESv3`, `EGL`, `android`, `log` |
| `glowtide_headless` | Desktop executable | Simulation runs, bots, the level bake, offline audio renders. No window, no audio device |
| `glowtide_tests` | Desktop executable | Plain asserts, exit code 0 or 1 (8.4) |

- SDL is built from source: static on the desktop, shared on Android (as RAVEFORM does; section 9).
- The desktop executable links statically (`-static`) so it runs without llvm-mingw's DLLs on PATH.
- A Ninja job pool limits compile and link jobs to `GT_MAX_JOBS` = 4 on every generator, including the
  Ninja that Gradle starts.
- Shaders: every file in `shaders/` gets the shared prelude prepended (version line, precision,
  `grade()`), is validated in that final form with `glslangValidator` (as `#version 300 es`) and is
  then embedded as a string in a generated header. Validation runs in the desktop build;
  `build-android.ps1` runs that step before Gradle so an invalid shader never reaches a device.
- The font atlas and its metrics are generated files that are **committed** (`src/gfx/font_atlas.inc`,
  `font_metrics.inc`), so no build needs Python. Re-run `tools/bake_font.py` only when the font or the
  glyph set changes.
- Every build writes `generated/build_stamp.h` (version, commit if any, date). The settings screen and
  every benchmark result show it.

### 2.2 Scripts (PowerShell 7, run from the repository root; each dot-sources `scripts\env.ps1`)

| Script | Does |
|---|---|
| `env.ps1` | Per-process environment (2.3), tool paths, device names, adb helpers, shared-device checks, and taking and releasing the device lock (PLAN 3.3) |
| `fetch-deps.ps1` | Clones SDL from the sibling checkout, verifies the commit, copies the Khronos headers. Safe to re-run |
| `build-desktop.ps1 [-Config debug\|dev] [-Clean]` | CMake + Ninja + llvm-mingw. `debug` = Debug with a GL debug context; `dev` = RelWithDebInfo |
| `run-desktop.ps1 [-Config] [-NoBuild] [-AppArgs '…'] [-Screenshot <png>] [-Frames n] [-Res WxH] [-Small] [-Sound] [-Watch]` | Runs the app silent and off screen unless `-Sound` / `-Watch`; returns the app's exit code; `-Small` also writes a reduced copy for review |
| `test.ps1` | Builds and runs `glowtide_tests` and the headless checks (8.4). Exit 0 = all passed |
| `build-android.ps1 [-Config dev\|debug\|release] [-Offline] [-Clean]` | Gradle wrapper, at most 4 workers, this project's own daemon |
| `deploy-android.ps1 -Serial <name\|serial> [-NoBuild] [-NoLaunch] [-AppArgs '…']` | Installs and launches with the shared-device checks |
| `bench-android.ps1 -Serial … -Scenario … [-Seconds] [-Warmup] [-AppArgs] [-Deploy]` | Runs one benchmark, pulls the result, grades it against the contract (8.3) |
| `capture-android.ps1 -Serial … [-Small]` | A screenshot of the panel, only while our app is in front |
| `lifecycle-android.ps1 -Serial … [-Cycles 20]` | Background/foreground cycles, then checks the app is alive and drawing |
| `review.ps1 -Set <name> [-Only a,b]` | Renders a phase's picture set into `captures\<set>\` with small copies and a contact sheet (8.1) |

Keep this list short. A script that is used once belongs in `PROGRESS.md` as a command line, not in
`scripts\`. Exit checks that say "exits 0" or "no GL message" are run with the `debug` configuration.

### 2.3 The PC rules, as implemented

- **Four cores.** `env.ps1` confines its own PowerShell process to one logical CPU on each of four
  physical cores (an affinity mask read from the processor topology); everything it starts inherits
  that. Builds also limit themselves: `cmake --build --parallel 4`, Gradle `--max-workers=4`, the Ninja
  job pool. Gradle gets `-Dorg.gradle.jvmargs=-Xmx2048m -XX:ActiveProcessorCount=4
  -XX:ErrorFile=<a path inside android\.gradle>`: the last two options make this project's daemon
  distinct, so it never reuses a daemon another session started without the limit. Never run
  `gradlew --stop`. The level bake runs at most four processes.
- **Silent.** `--silent` opens the audio device, runs and measures the mixer, and zeroes the output
  buffer just before it is handed over. `run-desktop.ps1` and `review.ps1` pass it unless `-Sound`. A
  desktop process started by hand gets `--silent` too. The headless tool never opens a device.
- **Off screen.** `--offscreen` creates the window hidden, as a utility window that cannot take focus,
  positioned left of every display. Rendering is unchanged because captures never read the window
  (5.3). `run-desktop.ps1` and `review.ps1` pass it unless `-Watch`. Nothing in the tooling may depend
  on a key press in that window.

### 2.4 Android project

`android/` is a plain Gradle project modelled on RAVEFORM's (section 9), minus everything this game does
not need.

- `settings.gradle`, `build.gradle` (the plugin at 8.13.0), `gradle.properties`, the 8.13 wrapper.
- `app/build.gradle`: namespace and applicationId `dev.glowtide.game`; compileSdk 36, minSdk 29,
  targetSdk 36; `ndkVersion '29.0.14206865'`; `abiFilters 'arm64-v8a'`; externalNativeBuild CMake
  `../../CMakeLists.txt`, version `3.22.1`, `-DANDROID_STL=c++_static`; build types `debug` (native
  Debug), `dev` (native RelWithDebInfo, debuggable, debug-signed: the benchmarked build) and `release`
  (native Release, not debuggable, debug-signed until the owner supplies a key); SDL's Java sources added
  with `sourceSets.main.java.srcDirs += '../../third_party/SDL/android-project/app/src/main/java'`. No
  `dependencies` block.
- `AndroidManifest.xml`: `<uses-feature android:glEsVersion="0x00030000" android:required="true"/>`;
  the `VIBRATE` permission and no other; `android:appCategory="game"`, `android:isGame="true"`,
  `android:allowBackup="false"`, `android:enableOnBackInvokedCallback="false"` (without it the back key
  never reaches the game on Android 16); the game-mode configuration that refuses downscaling and
  frame-rate overrides; one activity, `android:screenOrientation="portrait"`,
  `launchMode="singleInstance"`, the full `configChanges` list,
  `android:preferMinimalPostProcessing="true"`.
- **Orientation.** SDL sets the activity's orientation itself when the window is created and would
  undo the manifest: set the hint `SDL_ORIENTATIONS` to `Portrait` before creating the window, and do
  not make the window resizable on Android.
- **Back key.** Set the hint `SDL_ANDROID_TRAP_BACK_BUTTON` to `1`, or back leaves the app from every
  screen. The game then gets the back key as a key event (GAME_DESIGN 4).
- `GlowtideActivity extends SDLActivity`: returns the libraries `SDL3` and `main`; turns the string
  extra `args` into the native argument list in debuggable builds only; immersive sticky full screen
  with `setDecorFitsSystemWindows(false)` and short-edge cutout mode, so the surface is the whole panel;
  asks for a display mode of the current size with the refresh rate nearest to the one wanted (the
  highest by default, `--fps` to override) through the window's `preferredDisplayModeId`; one static
  method `haptic(int kind)` that posts a prebuilt vibration effect to the UI thread (a vibrate call
  from the game thread is a cross-process call inside the frame), with durations only for a vibrator
  without amplitude control. Nothing else.
- At start-up the app logs the surface size beside the display mode, and the refresh rate it was
  granted. Size must match; a difference means the system is scaling us and is a bug.

Launch by hand:
`adb -s <serial> shell "am start -n dev.glowtide.game/.GlowtideActivity --es args '--scenario bench_idle --seconds 30'"`.

### 2.5 App arguments, exit codes, log tags

Arguments (desktop `argv`; on Android the `args` extra):

| Argument | Meaning |
|---|---|
| `--scenario play\|touch_test\|bench_idle\|bench_flood\|bench_play` | What runs (8.1). Default `play` |
| `--level N`, `--seed N`, `--world N` | Start at this level; override the seed or the palette |
| `--bot greedy\|naive\|sweep\|idle` | A bot holds the thumb |
| `--picture <name>` | A scripted still or moment (8.1); implies `--fixed-step` |
| `--screenshot <png>`, `--frames N`, `--strip K`, `--strip-step F` | Capture frame N; with `--strip`, the K frames ending at N, F frames apart. On a device a picture holds frame N on screen until the app is stopped |
| `--seconds S`, `--warmup S` | Measured time and the soak before it |
| `--fixed-step`, `--speed X` | One tick per frame exactly; time scale |
| `--res WxH`, `--dp-scale X`, `--safe T,B` | Desktop: the frame's size in pixels (default 540×1200), its dp scale (default from the matrix in 5.2) and its top and bottom safe insets in dp (default 32,16). The capture target, the viewport and the layout come from these, never from the window |
| `--set name=value` | Override a tweakable (7.4); repeatable |
| `--silent`, `--no-audio`, `--offscreen`, `--no-hud`, `--reduce-effects` | As named |
| `--rebuild-gl-at N` | Destroy and rebuild every GL object before frame N (the context-loss path, 3.3) |
| `--flow-test` | Drive every screen by synthetic touches and check the results (8.4) |
| `--fps HZ` | Android: the refresh rate to ask for |

Exit codes: 0 ok; 1 start-up failed (including a shader that does not compile or link); 2 bad arguments;
3 GL errors or debug messages were reported; 4 benchmark invalid; 5 capture failed; 6 flow test failed;
7 a lint failed.

Log tags: `GLOWTIDE` (everything), `GTBENCH` (one JSON line per benchmark), `GTSIM` (one JSON line per
headless run), `GTLIFE` (background, foreground, first frame after resume), `FLOW` (flow-test checks).

---

## 3. Platform layer

### 3.1 Window and GL context

- SDL attributes are set before the window is created: OpenGL ES profile, version 3.0, 8 bits per colour
  channel, **no alpha**, a 24-bit depth buffer (the scene is drawn straight into the default
  framebuffer, 5.3), no stencil, no multisampling; in debug builds the debug-context flag. Swap
  interval 1.
- Desktop: ask for an ES 3.0 context first (the Radeon driver is expected to offer one through WGL). If
  that fails, create an OpenGL 3.3 core context and switch the shader prelude from `#version 300 es` to
  `#version 330 core`. Log which path is in use. The shader sources are the same either way.
- GL functions: on Android include `<GLES3/gl3.h>` and link `GLESv3`. On the desktop include the copied
  Khronos header with `GL_GLES_PROTOTYPES` set to 0 and declare the functions the renderer uses as
  pointers in one X-macro list, resolved with `SDL_GL_GetProcAddress` at start-up (a missing one fails
  start-up by name).
- Debug builds install a `KHR_debug` callback when the extension exists and count messages of low
  severity and above; the desktop debug build exits with code 3 if there were any. Debug builds also
  check `glGetError` once per frame.
- Start-up logs `GL_VENDOR`, `GL_RENDERER`, `GL_VERSION`, the shading language version, the extension
  list, the frame size in pixels, the dp scale, the display's refresh rate and the safe area.

### 3.2 Frame loop

```
each frame:
  pump events → input (3.4)
  dt = clamp(now − last, 0, 0.1) × speed;  acc += dt
  while acc ≥ 1/60 and ticks this frame < 4:  world.tick(input);  acc −= 1/60
  acc = min(acc, 1/60)                 after a stall, drop the backlog instead of racing
  alpha = acc × 60
  view.update(dt, world.events())      camera, animations, effects, UI
  renderer.draw(view, alpha)           units drawn at  pos − lastStep × (1 − alpha)
  swap
```

The simulation tick is fixed at 60 Hz whatever the display's rate. Each unit's instance carries its
displacement over the last tick (`lastStep`), so the vertex shader places it exactly between its last
two positions, pushes and all, and 90 and 120 Hz panels are smooth with no extra simulation work.
`--fixed-step` makes every frame exactly one tick (pictures, strips, the flow test).

The game layer does not call `world.tick` while a level is waiting for its first touch (7.1).

### 3.3 Android specifics

- **Lifecycle.** SDL blocks the event pump while the app is in the background. Handle
  `SDL_EVENT_WILL_ENTER_BACKGROUND`, `DID_ENTER_BACKGROUND`, `WILL_ENTER_FOREGROUND` and
  `DID_ENTER_FOREGROUND` in an event watch, not in the main loop: on the way out, pause the game (PLAY
  becomes PAUSE), write the save, stop issuing GL calls.
- **Context loss.** SDL keeps the GL context across a pause when the system lets it. When it cannot,
  SDL creates a new context on return and sends `SDL_EVENT_RENDER_DEVICE_RESET`: on that event rebuild
  everything through `Renderer::createDeviceObjects()`. Every GL object must be creatable from
  CPU-side state at any time. `--rebuild-gl-at N` exercises the same path on the PC.
- **Safe area.** `SDL_GetWindowSafeArea` gives the insets for the cutout and system bars; the UI
  anchors to it and adds a fixed 24 dp at the bottom for the gesture handle.
- **Refresh rate.** 2.4 and 5.10.
- **Frame timestamps** for benchmarks: 8.3.
- No JNI call happens during play except the haptic one, at most twelve times a second.

### 3.4 Input

- One finger at a time: the oldest finger still down owns the gesture. Mouse events synthesised from
  touch are turned off (SDL hint), or the launcher moves twice.
- Finger motion arrives normalised to the window; all motion events since the last frame are summed.
  Horizontal pixels become lane metres with the camera's pixels-per-metre at the launcher line (5.2),
  times the drag gain.
- UI first: a touch that begins on a button belongs to the UI until it ends. Anything else is the game's.
- The simulation never sees events. It sees `struct Input { float launcherTargetX; bool firing; }`,
  which bots also produce.

### 3.5 Haptics and files

- Haptics: one function, `haptic(kind)`, rate-limited to twelve calls a second. Android calls the
  activity's static `haptic` through SDL's JNI accessors (2.4). Desktop does nothing.
- Files: the save lives in `SDL_GetPrefPath("glowtide", "glowtide")`. Nothing else is written at run
  time except captures and benchmark results asked for on the command line.

---

## 4. Simulation

### 4.1 Shape

`src/sim` is a library with no dependencies but `gt_core`. `World` has `init(level, tuning)`,
`tick(input)`, `hash()`, read-only views of its arrays for the renderer, and the events of the last
tick. No globals, no I/O, no allocation after `init`: every array is sized by the caps. The game, the
headless tool, the bots, the bake and the tests all use this one library.

### 4.2 Determinism

Same binary, same level, same input sequence → the same `hash()` after every tick.

- Fixed tick. No wall clock. One integer random stream for the simulation, seeded from the level; the
  view's effects use a separate stream so pictures can change without changing outcomes.
- No `sin`, `cos`, `pow` or `exp` in `src/sim` (`sqrt` is fine). Sliding gates and snuffers use a
  triangle wave eased with a cubic at its turning points.
- Iterate arrays in index order. Remove dead units in one pass in a defined order.
- `gt_sim` is compiled with floating-point contraction off and without fast-math.
- With those rules the PC and the phones are expected to produce the same hashes. P2 checks that once
  (a headless-style determinism run on the Moto prints its hash beside the PC's) and records the
  answer. Nothing depends on it: the baked table carries only a seed and two numbers per level.

### 4.3 Data layout

Structure of arrays, one set per team, sized `CAP_W` and `CAP_G`:
`x`, `z`, `xPrev`, `zPrev` (the position at the start of the tick: for the gate test and for drawing
between ticks), `vx` (the sideways push; forward speed comes from the team and the flags), `power`,
`gateBits`, `bornTick`, `flags` (the `brute` mark), `seed`. Under 30 bytes a unit.

Level objects (gates with their divide carry, barriers, bumpers, snuffers, emitters) are small fixed
arrays inside `World`.

### 4.4 Spatial grid

One uniform grid over the lane, cells 0.8 m square, rebuilt once every tick after movement by a counting
sort (cell start indices plus one sorted index array per team; no per-cell lists, no allocation).
Separation and the clash read a 3 × 3 neighbourhood. A unit of tier 2 or 3 is wider than a cell's reach:
for those, widen the neighbourhood by the unit's radius (they are rare). Separation and bumpers move a
unit by at most 0.1 m after the grid is built, which the neighbourhood's margin covers.

A 10 × 36 m lane holds about 1,700 wisps side by side, so at the starting caps the densest crowd is
about 30 candidates per neighbourhood: tens of thousands of distance checks a tick, not millions.

### 4.5 Tick

The order is GAME_DESIGN 5.13. Notes:

- `xPrev` and `zPrev` are written at the start of the tick. Gates test `zPrev < gateZ ≤ z` in step 9,
  with `z` as it stands after separation and bumpers. Clones are appended to the end of the array with
  the gate's bit set, so they are not tested again this tick.
- Gate passes, clashes and spire hits are counted per tick into aggregated events (4.6); the view never
  inspects units to discover what happened.
- The **bench rig** (8.1) is three switches on `World`: no clash, no spire or core damage, and units
  that reach the end of their run re-enter at its start.
- Debug builds assert after every tick: all positions finite and inside the lane; counts within caps;
  every live unit has power ≥ 1; the tide changed across a multiply gate by exactly the factor.

### 4.6 Events

A fixed array of the last tick's events, each `{type, x, z, a, b}`:
`Fire`, `GateMul` (gate, passes this tick, mean x), `GateDiv`, `Clash` (up to 64 a tick, the rest
counted in one summary event), `BarrierHit`, `BarrierBreak`, `Snuffed`, `BruteDown`, `SpireHit` (one a
tick: total damage, mean x), `SpireStage` (75, 50, 25%), `CoreHit`, `TideMark`, `Win`, `Lose`.

`World` also keeps running totals the bake needs: power produced (fired plus added by gates), power
cancelled in clashes, power delivered to the spire.

### 4.7 Levels and the generator

`LevelDef` is a plain structure (GAME_DESIGN 7.1). `levels_intro.cpp` holds the layouts of levels 1 to
5, `generate.cpp` the generator, `levels_baked.h` the bake's table (generated, committed: a seed, a
spire health and a rate scale per level). `levelFor(index)` returns a ready `LevelDef`.

### 4.8 Bots, the headless tool, the bake

Bots (GAME_DESIGN 8) live in `src/sim` and produce `Input`. `glowtide_headless`:

| Command | Does |
|---|---|
| `--sim determinism [--level N] [--seconds S]` | Runs twice, compares the hash every tick, prints the last hash. Exit 0 or 1 |
| `--sim run --level N --bot B [--seeds K] [--seconds S]` | One `GTSIM` JSON line per run: result, time, core left, power produced, cancelled and delivered, the lowest `z` a gloom reached, peak tide, peak counts, tick time (p50, p95, max, in microseconds) |
| `--sim table --from A --to B [--bots greedy,naive,idle] [--seeds 10]` | A markdown table of results per level and bot |
| `--sim soak --from A --to B [--bot greedy]` | One pass over the levels with the debug assertions on, moving on at each win or loss. Exit 0 = clean |
| `--bake-levels --from 2 --to 300 [--jobs 4] --out src/sim/levels_baked.h --report captures/levels/report.md` | GAME_DESIGN 7.4. Levels are independent, so the range is split between the jobs |
| `--render-audio <scene> --out x.wav [--seconds S]` | Section 6 |

All of them accept `--set name=value`.

---

## 5. Renderer

### 5.1 OpenGL ES rules

- **ES 3.0 core and nothing else.** Used: vertex array objects, instancing with attribute divisors,
  framebuffer objects with `RGBA8` colour and `DEPTH_COMPONENT24`, `glMapBufferRange` or orphaning,
  `R8` and `RGBA8` textures with linear filtering, `dFdx`/`dFdy`/`fwidth`.
- **Not used:** floating-point colour targets, `gl_FragDepth`, multiple render targets, multisampling,
  sRGB framebuffers, compute or geometry shaders, any extension without the core path kept beside it.
- **Shaders** are `#version 300 es`, plain and small: no `switch`, no arrays of structures as
  uniforms, loops with constant bounds, no implicit integer-to-float conversion.
- **Precision.** Vertex shaders are `highp`. Fragment shaders declare `precision mediump float;` and
  mark `highp` whatever carries world positions, light-map or atlas coordinates, or time. Mali really
  computes `mediump` in 16 bits. Time uniforms are wrapped to a short period before upload.
- Every shader must pass `glslangValidator` at build time. At start-up a shader that fails to compile
  or a program that fails to link ends the run with exit code 1 and the log printed. A log that is not
  empty on success is printed as a warning (drivers add notes); the desktop debug build counts it
  toward exit code 3.
- **Colour** is authored and blended in display space. There is no linear-light pipeline; do not mix
  the two.
- **Buffers.** Each dynamic buffer is orphaned and filled once per frame. Never update a buffer twice
  in a frame and never read GL state back in the frame.

### 5.2 Camera and aspect ratios

A perspective camera on a fixed rig that is scaled to fit. The reference rig: eye (0, 24, −11), target
(0, 0, 14.4), vertical field of view 56°, near 2 m, far 160 m.

Fit rule, solved at level load and when the frame's size changes (a binary search that projects three
points): `target.z = 0.40 · LEN`; `eye = target + s · (eyeRef − targetRef)`; with `topLine` = the safe
area's top + 64 dp (the bottom of the HUD row) and `bottomLine` = the safe area's bottom − 24 dp, choose
the smallest `s` between 0.6 and 1.4 for which

1. the launcher's base, the point (0, 0, −1.5), is above `bottomLine`;
2. the spire's top, (0, 6.3, `LEN` + 1.4), is below `topLine`;
3. the lane's edge at the launcher line with the camera's follow added, (±(`HW` + 0.53), 0, 0), is
   inside the safe width.

On tall screens rule 3 binds and the spare height goes above the spire. On wide screens rules 1 and 2
bind, the lane takes less of the width, and the backdrop and islets fill the sides. A test asserts that
an `s` exists for every `LEN` from 30 to 44 in every size of the matrix. The lane's slab runs on below
the screen's bottom edge; that is fine.

The camera follows 12% of the launcher's `x` on a spring; shake is added last. The camera exposes
pixels-per-metre at the launcher line for the input mapping (3.4).

The aspect matrix used by pictures and lints. The dp scale is what makes a desktop picture lay out like
the device it stands for; P0 logs each real device's scale and corrects this table.

| Frame | dp scale | Stands for |
|---|---|---|
| 1080×2400 | 2.6 | The Moto (9:20) |
| 540×1200 | 1.3 | The same at half size: the default desktop frame |
| 1080×2640 | 2.6 | A 9:22 phone |
| 1080×1920 | 2.6 | A 9:16 phone |
| 600×1024 | 1.0 | The Fire 7 |
| 1200×1920 | 1.5 | The tablet (5:8) |
| 1080×1440 | 2.0 | A 3:4 screen |

### 5.3 Frame structure

Four small passes. There is **no full-resolution off-screen copy of the scene**: on the floor phone
that copy alone would cost a noticeable part of the budget.

Blending rule for everything that glows: **premultiplied "over"**, `(ONE, ONE_MINUS_SRC_ALPHA)`, never a
plain sum. A plain sum of a dozen gold discs clips to white in an 8-bit target; "over" makes a crowd
converge on its own colour, which is the look of the concept frame. A source with alpha 0 still adds
(a spark); a source with alpha above 0 also covers what is behind it (a shard, a puff).

1. **Light map** → `lightTex` (lane space, `RGBA8`, 128 × 512), cleared to black. One soft disc per
   unit from the unit instance buffer, plus a few slow lights (5.5), blended "over".
2. **Glow buffer** → `glowTex` (screen space at a quarter of the resolution, `RGBA8`), cleared to
   black. Everything that should bloom is drawn here again in a cheap form: a soft disc per unit
   ("over"), the emissive parts of props, gate edges, particles, a faint copy of the labels. Then it is
   blurred by a short down-and-up chain (5.9).
3. **Scene** → the default framebuffer at native resolution, with depth:
   1. opaque props (one static buffer per level plus a few moving ones), depth write on;
   2. the backdrop, as one full-screen triangle at the far plane with depth test `LEQUAL`, so it only
      shades pixels nothing else covered;
   3. units and gate panels, **far to near**: units are sorted by `z` (a counting sort on centimetres),
      both teams merged, and drawn as premultiplied impostors with analytic edges, depth test on, depth
      write off; the sorted stream is cut at each gate row so that row's translucent panels are drawn
      between the units behind them and the units in front of them;
   4. particles, one premultiplied draw, depth test on, depth write off;
   5. labels (signed-distance text anchored in the world), depth test off, far to near: labels win
      (GAME_DESIGN 9.4).
4. **Bloom and UI** → the default framebuffer: one full-screen draw of the blurred glow with the
   *screen* blend, `(ONE, ONE_MINUS_SRC_COLOR)`, which brightens without ever clipping; then the UI
   batch.

The vignette and the win and lose colour changes (brighten, desaturate, the fade between levels) are
uniforms applied by a shared `grade()` function in every scene shader's prelude, not a post-process.
The backdrop dithers its own gradient (± half a level) so 8-bit banding never shows.

**Captures.** With `--screenshot`, `--strip` or `--offscreen`, passes 3 and 4 target an `RGBA8` + depth
framebuffer object of the size given by `--res` and the result is read back with `glReadPixels` (and
blitted to the window if it is visible). The frame's size never comes from the window: the monitor is
smaller than a phone's 2,400 rows, and a hidden window's default framebuffer is undefined on Windows.
On a device, captures come from `adb exec-out screencap -p` while a picture holds its frame.

### 5.4 Units

A unit is one instanced quad: two triangles, at most 20 bytes of instance data (position, the last
tick's displacement, flags and tier, a seed, ticks since birth).

- Vertex shader: world centre = (`x`, radius + hop, `z`) − `lastStep × (1 − alpha)`; a camera-facing
  quad 1.5 radii in half-size; the hop, the landing squash and the birth pop (GAME_DESIGN 10) are
  computed here from time, seed and age. The quad must never dip below the lane's surface, or the depth
  test cuts it.
- Fragment shader: a signed distance field of the body in the quad's local coordinates. Wisp: a circle
  joined smoothly to a tip that leans and wobbles. Gloom: a circle with two horn bumps. Shading is a
  ramp of the team's three colours: rim of the darker hue at the edge, body, a pale inner light offset
  up, a bright core; for glooms two glowing eyes that blink; for wisps two eyes that appear when
  `uCheer` rises at the win. Edges are anti-aliased from the distance with `fwidth`, clamped so tiny
  far units do not dissolve. Output is premultiplied.
- No level of detail and no mesh: every unit is the same two triangles at any distance.

### 5.5 Light map

The crowd lights the ground. `lightTex` covers the lane plus a margin:
`u = (x + HW + 1) / (2·HW + 2)`, `v = (z + 3) / (LEN + 6)`.

- Each unit lays a soft disc in its team's colour "over" what is there: radius 0.9 m plus the tier,
  about 0.13 alpha at its centre, a little smaller and brighter when the unit is on the ground than at
  the top of its hop. A crowd converges on a full pool of its colour; where gold and violet meet, they
  mix.
- A few slow lights are laid every frame: a bar under each gate in its colour, a rose pool at the
  spire's foot that pulses, a gold pool under the launcher, strips along the rails, and short flashes
  from clash, barrier and spire events (a small fixed array).
- The lane's fragment shader shows its pattern (tile seams, chevrons) multiplied by ambient plus the
  light map, and adds a little of the light map on top so a dark lane visibly lights up. Props take a
  tint from the light map at their foot, per vertex.

### 5.6 Props, gates and labels

- **Props are generated meshes**: box, cylinder, lathe, faceted crystal, slab. Flat normals. Each
  vertex gets its colour when the mesh is built, from a three-tone ramp of the world's prop colours by
  the face's angle to a fixed key light, darkened toward the ground. The prop shader is then only
  vertex colour, fog toward the backdrop's far colour, the light-map tint and `grade()`.
- Everything static in a level (lane, rails, lantern posts, islets, spire, bumpers, fixed gate posts)
  is merged into **one vertex buffer and one draw call** at level load. Moving things (launcher,
  sliding gates' posts, snuffers, barriers while they shake or break, the spire's shards at the win)
  are small separate draws with a model matrix. Emissive parts are a second index range of the same
  buffers, drawn into the glow buffer.
- **Gate panels**: a quad with a small shader (slow vertical shimmer, bright top and bottom edges, a
  flash uniform), alpha-blended in the unit stream (5.3).
- **Labels**: text quads that face the camera, anchored at the upper half of the panel (or above a
  barrier, a brute, on the spire's face). Their size follows perspective but never falls below 3% of
  the screen's height. White fill, a stroke in the element's dark hue at least 3 px wide, both from the
  distance field. The punch animation scales them.

### 5.7 Text and the font bake

- `tools/bake_font.py` (PIL and numpy): renders the glyph set from the font at 96 px, builds a signed
  distance field with a spread of 14 px (room for a stroke and a soft shadow), packs the glyphs at
  about 48 px into one `R8` atlas no larger than 512 × 512, and writes the atlas bytes and the metrics
  (texture rectangle, bearing, advance; digits padded to one width so numbers do not jitter as they
  count). Kerning is ignored. A variable font is set to its bold instance through PIL's variation API.
- The glyph set is what the game shows and no more: digits, `A`–`Z`, space, `. , : ! ? ' - + %`, `×`
  and `÷`. All strings are upper case. The script **fails** if the font lacks a glyph; it never bakes
  a box. Stars and other icons are drawn as UI shapes, not as text.
- The same script renders a **sample sheet** (digits, every label, every UI string, at the sizes the
  game uses) for the owner to approve in P1.
- Run time: one texture, one function that appends quads to the UI batch or the label batch. The text
  shader takes fill, stroke and a soft shadow from one distance sample.

### 5.8 Particles

One ring buffer of 4,000 instances, **written once at spawn and never touched again**: origin,
velocity, birth time, life, size, colour index, kind (spark, puff, shard, mote, ring). The vertex shader
computes where each one is from its age (each kind has its own gravity and drag) and collapses the dead
to zero area. The whole ring is drawn every frame in one premultiplied draw (5.3): sparks, motes and
rings with alpha 0, so they add; shards and puffs with alpha, so they cover. The same draw goes into
the glow buffer. A per-frame spawn budget is enforced; effects beyond it are dropped, never queued.

### 5.9 Glow chain

`glowTex` starts at a quarter of the resolution. Downsample to 1/8 and 1/16 with a 4-tap filter, then
upsample and add back up to the quarter-resolution level. The bloom pass (5.3, pass 4) samples that with
bilinear filtering. Strength and the per-source glow amounts are tweakables; `REDUCE EFFECTS` halves the
strength.

### 5.10 Quality: one level

There are no quality tiers, no quality setting, no governor and no multisampling. The picture that
ships is the one that passes the contract on the floor phone; 8.5 is how it is trimmed to fit, at
development time. Stronger devices get the same picture at a higher refresh rate. `REDUCE EFFECTS`
(GAME_DESIGN 14) is the only switch.

**Refresh rate.** At start the app asks for the display's highest refresh rate at its current
resolution (2.4); `--fps` overrides. One fallback, built in P6: on a panel above 60 Hz, if more than 5%
of a level's frames were late, the app asks for 60 Hz before the next level and remembers that in the
save. It never steps back up by itself.

A weaker class of device (the Fire 7) is measured and reported, nothing more. If the owner wants it
supported, the first step is a second, lighter level built from 8.5's list. Do not build that in
advance.

### 5.11 UI layer

Immediate mode and tiny: `ui.rect`, `ui.text`, `ui.button(id, rect, label) → pressed`. Units are dp:
pixels divided by the dp scale, which is the display's content scale on a device and the frame's
`--dp-scale` on the desktop (5.2). Anchors are relative to the safe area (3.3; `--safe` on the
desktop). One batch, one shader (a rounded-rectangle distance field or a text distance sample, chosen
by an attribute), one draw call. Buttons are at least 48 dp; pressed state per GAME_DESIGN 10. Every
rectangle drawn in a frame is also recorded for the layout lint (8.4).

---

## 6. Audio

- `gt_audio` is pure code. `Synth` renders every sound of GAME_DESIGN 12 into mono 48 kHz float
  buffers at start-up from small recipes (oscillators, noise, one-pole filters, envelopes), including
  the eleven ladder notes. `Mixer` plays up to 32 voices (buffer, position, left and right gain, pitch
  ratio with linear interpolation) and runs the music generator on its sample clock.
- Game thread → audio thread: one lock-free single-producer single-consumer ring of small commands
  (sound, gain, pan, pitch). A full ring drops the command.
- Rate limits and aggregation (for example a gate's twenty notes a second, louder with more passes)
  are applied on the game thread, where the events are.
- Device: an SDL audio stream, 32-bit float stereo at 48 kHz, filled from a callback that calls
  `Mixer::mix`. The callback never locks, allocates, logs or calls into Java. It measures its own
  duration and the gap since the previous callback into small histograms that benchmarks report; a
  **late callback** is a gap above two buffer periods.
- A soft limiter on the master holds the output at −1 dBFS.
- `--silent` and `--no-audio`: 2.3.
- Offline: `glowtide_headless --render-audio ladder|flood|ui|win|lose|music --out x.wav` drives the
  same mixer from a scripted event list (`flood` replays the events of a real bot run of a late level)
  and writes a 16-bit WAV. `tools/wav_metrics.py` prints sample peak, RMS, DC offset, clipped samples
  and the share of silence; `ffmpeg -i x.wav -af ebur128=peak=true -f null -` gives integrated loudness
  and true peak. Targets: GAME_DESIGN 12.

---

## 7. Game layer

### 7.1 States

`enum class Screen { Home, Play, Pause, Win, Lose, Settings }` and a `switch`. No state-machine
framework.

There is one `World`. It is initialised for a level when that level is entered: at launch, on `NEXT`,
on `RETRY`, `REPLAY` and `RESTART` (the load budget is 50 ms on the floor phone). HOME shows that
initialised world without ticking it: the view animates, the simulation waits. The first touch that is
not on a button switches to PLAY and ticking begins (GAME_DESIGN 4 and 11). Bot and headless runs skip
HOME and tick from 0.

### 7.2 The view

`View` turns the simulation into pictures and sounds and never changes it. It owns the camera, each
gate's animation state (punch, flash), the spire's crack stage, banners, the particle spawner, light
flashes, haptic and sound requests. It consumes the tick's events (4.6) and reads the unit arrays to
build the sorted instance buffer.

### 7.3 Save

`save.txt` in the preferences path, plain `key=value` lines: `version`, `level`, `stars` (one digit per
level), the settings switches, `refresh60` (5.10), `hints` (a bit mask). Written when a level ends, when
a setting changes and when the app goes to the background: to `save.tmp`, then renamed. A missing or
damaged file means defaults; it never stops the game. `adb shell pm clear dev.glowtide.game` brings the
first launch back.

### 7.4 Tweakables

Every number in GAME_DESIGN 6 and 10 and the renderer's art numbers (glow strengths, sizes, timings,
palette entries) are fields of one `Tuning` structure with defaults in `tuning.h`, listed once in an
X-macro that also gives each its name. `--set name=value` overrides one at start-up, in the app and in
the headless tool. There is no in-game editor and no live reload: change, relaunch, look.

---

## 8. Verification and performance

### 8.1 Scenarios and pictures

Scenarios:

- `play`: the game.
- `touch_test`: draws touch points and the launcher mapping (P0).
- `bench_idle`: an empty lane.
- `bench_flood`: **the contract's scene.** Both crowds held at their caps, evenly spread and flowing,
  on a five-row level with every gate passing, particles at their budget, labels punching, no win or
  lose. In P1 a small crowd script feeds it. From P2 it is the real `World` in its bench rig (4.5),
  with scripted launchers and emitters holding the caps. It measures everything except the clash.
- `bench_play`: the greedy bot plays `--level N` and the run is measured. On the heaviest baked level
  this covers the clash.

Pictures (`--picture <name>`): scripted, fixed-step and repeatable.

| Picture | Shows |
|---|---|
| `concept` | The concept frame's layout: the same gates, launcher position and crowd distribution, for a side-by-side comparison |
| `elements` | One of every element on an empty lane, named |
| `unit_tiers` | Wisps and glooms at each power tier |
| `density_1200`, `density_2400`, `density_4000` | The same flood at three caps |
| `label_over_flood` | Every kind of label with a dense gold crowd behind it |
| `world_1` … `world_5` | The `concept` layout in each palette |
| `home`, `hud`, `pause`, `win`, `lose`, `settings` | Each screen at rest |
| `gate_strip`, `clash_strip`, `core_hit_strip`, `win_strip`, `lose_strip` | Moments, captured with `--strip` |

`review.ps1 -Set <name>` renders the pictures a phase lists at 1080×2400, the screens also across the
aspect matrix (5.2), writes reduced copies (at most 1,000 px tall, for viewing) and one contact sheet.

### 8.2 Screenshots and strips

`--screenshot <png> --frames N` writes frame N. `--strip K --strip-step F` also writes the K − 1 frames
before it, F frames apart, and `tools/strip.py` tiles them into one sheet with frame numbers. The PNG
writer is our own minimal one (stored blocks, CRC-32, Adler-32; section 9).

### 8.3 Benchmarks and frame timing

Every frame the app records the swap-to-swap interval, the main thread's phases (events, simulation,
view, buffer build, GL submission, swap) and, on Android, the frame's **display present time**:

- once: `eglSurfaceAttrib(display, surface, EGL_TIMESTAMPS_ANDROID, EGL_TRUE)`;
- before each swap: `eglGetNextFrameIdANDROID`;
- a few frames later: `eglGetFrameTimestampsANDROID` for `EGL_DISPLAY_PRESENT_TIME_ANDROID` (it
  reports "pending" until the frame has been shown);
- the display and surface come from SDL's EGL accessors, the functions from `eglGetProcAddress`. If
  the extension is missing, say so in the result and grade on swap intervals with a warning.

Present intervals are counted in whole refresh periods. A frame is **late** when it was shown two or
more periods after the one before it. The contract (PLAN 5) is then two numbers: the **late share** and
the **worst frame**. A GPU time per frame from `EXT_disjoint_timer_query` is reported when the
extension exists and is never required.

The result is one `GTBENCH` JSON line and a full JSON file (the app's external files directory on
Android, `captures\bench\` on the PC): device, build stamp, GL strings, scenario, refresh rate granted,
seconds, frame count, the late share, the worst frame, `interval_ms` as [p50, p95, p99, max], the
phases, the main thread's CPU time per frame without the swap as [p50, p95, max], simulation tick time,
unit and particle counts, heap allocations in the measured window, audio callback timing, and `valid`
(false if the app lost focus).

A phase **regresses** when, in `bench_flood` on the floor phone, the late share rises by more than two
points or the main thread's CPU P95 rises by more than 10% against the previous phase's recorded run.

`bench-android.ps1` checks the device is free and takes the lock (PLAN 3.3), deploys if asked, launches
with the arguments, waits for the process to end, reads `logcat -d --pid`, pulls the JSON, prints the
grade against PLAN 5, and repeats an invalid run once later.

The contract run:
`scripts\bench-android.ps1 -Serial moto -Scenario bench_flood -Warmup 180 -Seconds 60 -Deploy`.

### 8.4 Tests and lints

`scripts\test.ps1` runs all of these; exit 0 means all passed.

- **`glowtide_tests`** (plain asserts): gate arithmetic including cap overflow (power is multiplied
  exactly) and division with its carry (exactly one `k`-th of any mix of powers gets through); clash;
  barrier; bumper push-out; generator invariants over 1,000 seeds (rows in range, no gap under 1.2 m,
  no two objects overlapping, a reachable multiply gate in every row, object counts within limits);
  save round trip and damaged-file handling; text metrics; random stream reference values; zero heap
  allocations in `World::tick`; the camera fit across the matrix (5.2).
- **Headless**: determinism on levels 1 to 5 and on ten baked levels; the acceptance tables of
  GAME_DESIGN 7.4 and 8.
- **Palette lint**: the rules of GAME_DESIGN 9.2 for every world.
- **Layout lint** (exit 7; run on every screen picture across the aspect matrix): every interactive
  rectangle at least 48 dp; none closer than 8 dp to another; all UI inside the safe area; no text
  clipped by its container; text contrast against its panel at least 4.5 to 1.
- **Label check**: the renderer reports each label's height and stroke width in pixels; fail under 3%
  of the screen's height or under 3 px of stroke.
- **Rebuild check**: a capture of frame N + 1 with `--rebuild-gl-at N` equals the same capture without
  it, pixel for pixel.
- **Flow test** (`--flow-test`, exit 6; from P3): synthetic touches, found by label, start level 1 with
  the bot, win, NEXT, pause, RESTART, lose with the idle bot, RETRY, every settings switch, HOME, then a
  second start checks the save. One `FLOW` line per check.

No test framework, no fixtures. A check that only the owner can make (looks, feel, sound, playing by
touch) is listed in the phase's owner checkpoint instead of being faked.

### 8.5 Performance fallback order

Apply on the floor phone in this order, re-measure after each, and log each in `PROGRESS.md`:

1. Glow chain to two levels; glow buffer at a sixth of the resolution.
2. Light map 96 × 384; disc radius down a quarter.
3. Particle ring to 2,000; spark counts halved.
4. Unit quads tightened from 1.5 to 1.25 radii; the inner light of the unit shader removed.
5. Backdrop reduced to gradient and glow (no mist, no islets).
6. Separation on alternate ticks.
7. Caps down in steps of 400, not below 1,200 wisps.
8. Stop and bring the numbers to the owner. Never lower the resolution on your own.

---

## 9. Where RAVEFORM already solved it

Read these in `D:\Stuff\DevFolder\Android_Games\Raveform\` and copy what helps (PLAN 0, rule 10). It is a
Vulkan, landscape game: do **not** copy its renderer, swapchain or pre-rotation code.

| File | Take from it |
|---|---|
| `scripts\env.ps1` | Tool paths per process, the four-core affinity code, adb helpers, the device table and name resolution (USB first, then wireless), the shared-device helpers (is the device free, wait until idle, start our app without covering another) |
| `scripts\audio-device.ps1`, lines 23–34 | The device lock: one file per hardware serial in `captures\locks\`, the first word is the owner, the file's time is its age, 30 minutes. (RAVEFORM's plain bench, deploy and lifecycle scripts do not take the lock; ours do) |
| `scripts\build-desktop.ps1`, `run-desktop.ps1` | The CMake + Ninja + llvm-mingw invocation; silent and off-screen defaults |
| `scripts\build-android.ps1` | The Gradle invocation: worker limit, the daemon's own JVM options |
| `scripts\deploy-android.ps1`, `capture-android.ps1`, `lifecycle-android.ps1`, `bench-android.ps1` | Device flows. The benchmark script is large; take its ideas (foreground checks, validity, log by pid), not its size |
| `CMakeLists.txt`, lines 40, 60–64 and 78–84 | The static link flag; the Ninja job pool; SDL static on the desktop and shared on Android |
| `android\settings.gradle`, `build.gradle`, `gradle.properties`, `gradlew`, `gradlew.bat`, `gradle\wrapper\`, `app\build.gradle` | A Gradle project that builds on this PC (drop Oboe, frame pacing and the signing extras) |
| `android\app\src\main\AndroidManifest.xml`, `res\xml\game_mode_config.xml`, `res\values\styles.xml` | Manifest patterns (replace the Vulkan feature with `glEsVersion`, landscape with portrait, drop the network permissions, keep `enableOnBackInvokedCallback="false"`) |
| `android\app\src\main\java\dev\raveform\game\RaveformActivity.java` | Arguments from the intent extra, debuggable builds only (line 38); `setDecorFitsSystemWindows(false)` (line 88); `getDisplayRefreshRate` (line 103); `haptic(int kind)` (line 176): prebuilt effects, posted to the UI thread, with a path for vibrators without amplitude control. It does **not** select a display mode: ours must (2.4) |
| `frame_pacer.cpp` (under `src\`), lines 39–40 | A native way to ask for a refresh rate: `ANativeWindow_setFrameRate` found with `dlsym`. An alternative to the activity's display mode id |
| `src\platform\app.cpp` | The `--offscreen` window |
| `src\core\png.cpp`, `png.h` | The minimal PNG writer (without the rotation) |
| `scripts\shrink_png.py`, `make_icon.py`, `sanitize.ps1`, `apk_size.py` | Reduced copies, adaptive icon layers, sanitizer builds with llvm-mingw (tests and the headless tool only), APK size by part |
| `CLAUDE.md`: "PC runs are silent", "PC runs are off screen", "PC CPU limit", "Shared test devices" | The owner's rules as they were implemented and refined |

---

## 10. Traps

- **Mali and `mediump`.** A time or world coordinate in a `mediump` fragment variable works on the PC
  and on Adreno and breaks on the tablet (5.1).
- **The Moto's GPU driver is from 2021.** Keep shaders plain (5.1). Test on it before believing the PC.
- **Sums clip.** Anything that glows is blended "over" or with the screen blend, never added (5.3).
- **Hidden windows have no defined default framebuffer** on Windows, and the monitor is smaller than a
  phone's frame: capture from a framebuffer object sized by `--res`.
- **SDL GL attributes** must be set before the window exists.
- **A debug callback needs a debug context.** Without the flag (3.1), "no debug message" proves nothing.
- **Default framebuffer alpha.** Ask for no alpha: a compositor may otherwise blend the game with what
  is behind it.
- **SDL overrides the manifest's orientation** unless the `SDL_ORIENTATIONS` hint says `Portrait`; a
  resizable window gets full-user rotation (2.4).
- **Back leaves the app** unless the back-button hint is set, and on Android 16 the key never arrives
  without `enableOnBackInvokedCallback="false"` (2.4). Check back on the tablet as well as the Moto.
- **Context loss** is announced by `SDL_EVENT_RENDER_DEVICE_RESET`, after SDL has already made a new
  context (3.3).
- **A vibrate call on the game thread** blocks inside the frame: post it (2.4).
- **Touch arrives twice** if SDL's touch-to-mouse synthesis is left on.
- **Pixels, not points.** Size the viewport from the frame's pixel size; lay out the UI in dp (5.11).
- **Background on Android.** No GL call after the app has entered the background; lifecycle events are
  handled in an event watch (3.3).
- **Instancing state** (attribute divisors) belongs to the vertex array object; rebasing attribute
  pointers per gate-row segment (5.3) must restore nothing else. ES 3.0 has no base-instance draw.
- **Billboards under the floor.** A unit quad that reaches below `y` = 0 is cut by the lane's depth.
- **`fwidth` on tiny far units** makes them dissolve; clamp the edge width.
- **Gradle and the network.** A first build may want to download something that is not in the cache.
  Run `--offline` first and ask before a networked run.
- **PowerShell quoting** of `adb shell am start … --es args '…'`: build the command as one string, as
  RAVEFORM's `Start-RfApp` does.
- **CMake 3.22.** The root file is read by the SDK's CMake too; use nothing newer.
- **`gradlew` line endings.** Keep it LF with `.gitattributes`.
- **Git worktrees and agents.** Section 3.1 of the plan: create worktrees by hand with Git for Windows.
- **Sorting by `z`** is sorting by depth here only because the camera never rolls or yaws. If the
  camera ever turns, sort by view depth.

# GLOWTIDE — Progress

Executor: Claude (Opus 5.5) in one session, coding directly, from 2026-11-20. The owner set the goal: "Implement the
plan to completion. No subagents. Use moto and tablet for testing, should also work on 12R. If you have questions,
just do what you think is best and let me know your choices."

## Phase status

| Phase | Status | Notes |
|---|---|---|
| P0 Bootstrap | Done 2026-11-20 | Desktop and Moto pipelines, benchmarks with present timestamps, lifecycle 20/20 |
| P1 Look and performance | Done 2026-11-20 | `bench_flood` inside the contract on the Moto after a 180 s soak; review set `captures/p1/contact.png` |
| P2 Core gameplay | Done 2026-11-20 | Tests, determinism (PC = Moto), acceptance 1-5, strips with count checks, flux findings (GAME_DESIGN 5.9 corrected) |
| P3 Game loop and UI | Done 2026-11-20 | Flow test on PC and Moto, lint over 43 renders, save, lifecycle, cold start 1.1 s; the tablet's back-key check waits for the tablet |
| P4 Audio | Not started | |
| P5 Content | Not started | |
| P6 Polish and release | Not started | |

## Choices made for the owner (the owner asked for best judgment instead of questions)

| Question | Choice | Why |
|---|---|---|
| PLAN 2 decisions (name, theme, engine, scope) | Kept as proposed | The owner set the goal after reading them |
| `git init` | Yes, local only; commits at phase checkpoints, no remote, no push | Backups before breaking changes (owner's CLAUDE.md) |
| SDL source | Cloned from RAVEFORM's local checkout, verified at the pinned commit | No download |
| Font | **Roboto Black from the Android SDK** (Apache 2.0), corners rounded at bake time, instead of downloading Fredoka | Avoids a download that needed the owner's yes; same rounded, heavy look |
| Test devices | Moto `ZY22BXMJG3` (over Wi-Fi, 192.168.137.193); tablet and 12R when they are reachable | Only the Moto was attached on 2026-11-20 |
| Glow when the flood is dense (P1) | Bloom samples three levels with bounded weights; labels drawn after the bloom; glowUnit 0.34, warmer glow colour | The additive chain burnt 12.8% of a 2,400-wisp frame to white; now 1.1% |
| Emerald Aurora's backdrop | Top colour 0x48B48A | The palette lint (backdrop at most 45% luminance) failed at 0.51 |
| Gold button pressed state (P3) | Deep amber #8A4E0C with white text, not white | White on gold is 1.6:1; the exit check (and the owner's CLAUDE.md) want a high-contrast pressed state: now 4.2:1 |
| Haptic rate limit | Twelve a second of wall-clock time | Game time runs at 0.35x in the win's slow motion, which let 14 a second through |
| `bench_play` after a result | Starts the same level again 3 s later | The measured window is play, wins and level loads, not a still result screen |
| A dev-only level stepper | In SETTINGS of Debug and RelWithDebInfo builds (`GT_DEV`); not in Release | P3 task 12: the owner's playtest jumps between levels |
| Device test scripts | `flow-android.ps1`, `save-android.ps1`, `backkey-android.ps1`, `coldstart-android.ps1` | Repeatable P3 checks on any device, with the lock and the foreground checks |

## Measured device facts

| Device | GL | Display | dp scale | Present timestamps |
|---|---|---|---|---|
| Moto One 5G Ace `ZY22BXMJG3` | OpenGL ES 3.2 V@0502.0 (2021-07-14), Adreno 619 | 1080x2400, 60 Hz; safe top 112 px, bottom 120 px | 2.5 (400 dpi) | `EGL_ANDROID_get_frame_timestamps` with display present time: yes |
| Desktop | ES 3.0 context through WGL on the RX 9070 XT (ES 3.2 driver) | frames from `--res` | from the matrix | — |

## Benchmarks

| Date | Device | Scenario | Late share | Worst frame | Display p95 | CPU p95 (thread) | Tick p95 | Allocs | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 2026-11-20 | Moto | bench_idle 20 s | 0.84% | 35.9 ms | 17.10 ms | 3.67 ms | 0.15 ms | 0 | P0: 1,000 debug quads |
| 2026-11-20 | Moto | bench_flood 60 s after 180 s | 0.87% | 35.5 ms | 17.0 ms | 4.08 ms | 1.45 ms | 0 | P1 contract run: 2,400 wisps, 1,000 glooms |
| 2026-11-20 | Moto | bench_play level 5, 90 s | 0.90% | 34.8 ms | 17.14 ms | 3.68 ms | 0.11 ms | 0 | P2/P3, sound on: audio 0 late callbacks, fill p99 0.38 ms of a 20 ms period; haptics at most 12 a second |
| 2026-11-20 | Moto | bench_flood 60 s after 180 s | 0.76% | 34.7 ms | 16.97 ms | 4.20 ms | 1.47 ms | 0 | P3 regression check against P1: +2.9% CPU (limit 10%) |
| 2026-11-20 | PC | bench_play level 5 | — | — | — | — | 0.028 ms | 0 | P2: simulation P95 under 0.5 ms |

Other measurements (Moto unless named):

| Check | Result |
|---|---|
| Determinism, levels 1-5, greedy bot, 60 s | PC and Moto hashes equal: 6ab53c6b7de9902a, 75fbb176a58bbe28, b555264948d891cc, 7988d22d19d3694a, 2c2b421ea381a278 |
| Flow test (`--flow-test`, 42 checks incl. input latency) | PC 42/42; Moto 39/39 before the three input checks were added |
| Layout lint, 6 screens x 7 frames + button strip | 0 problems; `captures/p3/contact.png` |
| Pressed vs normal fill (button strip) | 4.24:1 |
| Rebuild check (`--rebuild-gl-at`) for `hud` and `win` | Pixel-identical |
| Save through a force-stop mid-level | Level, stars and settings intact (`save-android.ps1`); damaged-file unit test passes |
| Lifecycle, 20 cycles during a level | 20/20 back on PAUSE, no errors |
| Cold start to the first frame (launch request to first swap) | Median 1.14 s, worst 1.33 s (target 2 s) |
| Back key | Moto: play to pause to play, HOME leaves the app |
| Strips: counts from the simulation for the captured ticks | 0 disagreements (gate: k out per one in; glooms lose power only to clashes and the lantern) |

## Deviations from the specs

- **Present timestamps on Android** use `eglGetCurrentSurface(EGL_DRAW)`: `SDL_EGL_GetWindowSurface` is not implemented
  by SDL's Android backend (it returns NULL).
- **Benchmark CPU time** is the main thread's CPU time (`CLOCK_THREAD_CPUTIME_ID`), not wall time: on Android the
  driver blocks for the next buffer inside GL calls, which wall time would count as work.
- **The baked table stores the enemy's base flow** (power a second) beside the seed and the spire's health, instead of
  the rate scale: the game needs the flow, and the flow needs `P0`, which only the bake measures.
- **dp scale of the Moto is 2.5** (400 dpi), not 2.6 as TECH_SPEC 5.2's table guessed.
- **Separation** runs on cell-ordered copies of the units with a half stencil (each pair once), not on the unit arrays:
  3.4x faster on the Moto (7.8 ms to 1.45 ms tick P95 at the caps).
- **The main thread is pinned to the fast cores** on Android (`sched_setaffinity` to the CPUs with the highest
  `cpuinfo_max_freq`): the scheduler kept it on a little core.
- **GAME_DESIGN 5.9 corrected after P2** (`glowtide_headless --sim flux`, table in `captures/flux_findings.md`): the
  toy model's findings 1 and 4 hold; the edge (finding 2) is sharper (a three-row x30 layout collapses already at an
  enemy flow of 0.9 times the flood) and the pressure ramp finishes a half-strength player on level 5 (finding 3).
- **The clash strip** on level 5 shows little clashing (its `fight` is 15%): the P6 strips use later levels.

## Copied from RAVEFORM

`scripts/env.ps1` (tool paths, 4-core affinity, device helpers; lock helpers rewritten from `audio-device.ps1`),
`build-desktop.ps1`, `run-desktop.ps1`, `build-android.ps1` (adapted), the Gradle wrapper, `src/core/png.cpp` (without
the rotation), `src/core/alloc_counter.cpp`, the activity's immersive mode, argument and haptic patterns,
`tools/shrink_png.py` (height limit added).

## Open questions for the owner

- None blocking. The look (P1), the fun (P2), the sound (P4) and the difficulty ramp (P5) need the owner's eyes, ears
  and thumbs; the checks only they can make are listed per phase below as they come up.

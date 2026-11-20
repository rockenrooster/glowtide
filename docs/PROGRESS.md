# GLOWTIDE — Progress

Executor: Claude (Opus 5.5) in one session, coding directly, from 2026-11-20. The owner set the goal: "Implement the
plan to completion. No subagents. Use moto and tablet for testing, should also work on 12R. If you have questions,
just do what you think is best and let me know your choices."

## Phase status

| Phase | Status | Notes |
|---|---|---|
| P0 Bootstrap | Done 2026-11-20 | Desktop and Moto pipelines, benchmarks with present timestamps, lifecycle 20/20 |
| P1 Look and performance | In progress | |
| P2 Core gameplay | Not started | |
| P3 Game loop and UI | Not started | |
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

## Measured device facts

| Device | GL | Display | dp scale | Present timestamps |
|---|---|---|---|---|
| Moto One 5G Ace `ZY22BXMJG3` | OpenGL ES 3.2 V@0502.0 (2021-07-14), Adreno 619 | 1080x2400, 60 Hz; safe top 112 px, bottom 120 px | 2.5 (400 dpi) | `EGL_ANDROID_get_frame_timestamps` with display present time: yes |
| Desktop | ES 3.0 context through WGL on the RX 9070 XT (ES 3.2 driver) | frames from `--res` | from the matrix | — |

## Benchmarks

| Date | Device | Scenario | Late share | Worst frame | Display p95 | CPU p95 (thread) | Tick p95 | Allocs | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 2026-11-20 | Moto | bench_idle 20 s | 0.84% | 35.9 ms | 17.10 ms | 3.67 ms | 0.15 ms | 0 | P0: 1,000 debug quads |

## Deviations from the specs

- **Present timestamps on Android** use `eglGetCurrentSurface(EGL_DRAW)`: `SDL_EGL_GetWindowSurface` is not implemented
  by SDL's Android backend (it returns NULL).
- **Benchmark CPU time** is the main thread's CPU time (`CLOCK_THREAD_CPUTIME_ID`), not wall time: on Android the
  driver blocks for the next buffer inside GL calls, which wall time would count as work.
- **The baked table stores the enemy's base flow** (power a second) beside the seed and the spire's health, instead of
  the rate scale: the game needs the flow, and the flow needs `P0`, which only the bake measures.
- **dp scale of the Moto is 2.5** (400 dpi), not 2.6 as TECH_SPEC 5.2's table guessed.

## Copied from RAVEFORM

`scripts/env.ps1` (tool paths, 4-core affinity, device helpers; lock helpers rewritten from `audio-device.ps1`),
`build-desktop.ps1`, `run-desktop.ps1`, `build-android.ps1` (adapted), the Gradle wrapper, `src/core/png.cpp` (without
the rotation), `src/core/alloc_counter.cpp`, the activity's immersive mode, argument and haptic patterns,
`tools/shrink_png.py` (height limit added).

## Open questions for the owner

- None blocking. The look (P1), the fun (P2), the sound (P4) and the difficulty ramp (P5) need the owner's eyes, ears
  and thumbs; the checks only they can make are listed per phase below as they come up.

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
| P3 Game loop and UI | Done 2026-11-20 | Flow test on PC, Moto and tablet, lint over 43 renders, save, lifecycle, cold start 0.43 s (1.1 s before the P4 synthesis fix), back key on the Moto and the tablet |
| P4 Audio | Done 2026-11-20 | Renders in `captures/p4/` (three ladder timbres through the flood); 10 min of `bench_flood` with sound on the Moto and on the tablet: no late callback on either |
| P5 Content | Done 2026-11-20 | Levels 2-300 baked and accepted (4 min, 4 jobs); band tables in `captures/levels/bands.md`; 60 thumbnails reviewed; heaviest level (87) inside the contract on the Moto |
| P6 Polish and release | Done 2026-11-20 | Strips reviewed, flashing audit, greyscale sheet, 20-minute soak, sanitizers, release APK; device matrix on the Moto and the tablet (all checks pass); the OnePlus 12R skipped at the owner's word |

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
| Mix levels (P4) | Effects at 0.33, music at 0.54 of full gain, limiter holding samples at -1.5 dBFS | The flood render lands at -14.9 LUFS and -2.6 dBTP; the music sits 9-10 dB under it |
| The pad's voicing (P4) | An octave up (from C4 and A3) | A phone speaker has no bass: 11% of the bed's energy was above 300 Hz, now 45% |
| Synthesis cost (P4) | Oscillators by rotation and decays by multiplication instead of sin() and exp() per sample | The bank took 849 ms on the Moto (limit 150 ms); now 117 ms, no worker thread needed; cold start fell to 0.43 s |
| Acceptance of a baked level (P5) | Two lines added to GAME_DESIGN 7.4 step 5: the cancelled share within 5 points of `fight`, and from level 21 the naive bot must lose; up to two corrections of the rate scale before the next seed | The first full bake passed 7.4 but 13 levels cancelled 6-9 points over `fight` and three bands let the naive bot win twice |
| Crossing sliders (P5) | The two gates 1.6 m apart in z | At the same z they passed through each other (labels on top of each other, panels fighting for the same depth) |
| The spire at the win (P6) | It disappears at the shatter (its own mesh range); the shards fly from where it stood | It stayed standing behind its own shards |
| Level changes (P6) | The labels and the HUD fade with the picture, 150 ms out and in | They stayed at full brightness over a black frame |
| Clash sparks (P6) | Six per clash for the first six clashes of a tick, none after | Six for every clash (up to 64 a tick) would empty the particle budget the gates and the win need |
| The shot (P6) | A 10% squash for 70 ms and a lens ring on every shot; fired wisps grow 0.5 to 1 in 90 ms | It was a wobble while the finger was down |
| 60 Hz fallback (P6, TECH_SPEC 5.10) | A frame 1.5 periods or more after the one before counts as late; the decision needs at least two seconds of a level's frames | The contract's rounding rule, and a level that ends at once proves nothing |
| FRENZY and the other levers of GAME_DESIGN 5.12 | Not built; the unused `engage` and `frenzy` tweakables were removed | The design builds them only if the owner asks after playing |
| Release signing | The release APK is signed with the debug key | The release key is the owner's (docs/RELEASE_CHECKLIST.md) |
| `.gitattributes` | `* text=auto` kept | Windows tools here write CRLF; the repository stays LF |

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
| 2026-11-20 | Moto | bench_flood 600 s with sound | 0.54% | 35.6 ms | 16.96 ms | 4.23 ms | 1.48 ms | 0 | P4: 30,000 callbacks, fill p99 0.33 ms of a 20 ms period, gap max 33.7 ms, 0 late |
| 2026-11-20 | Moto | bench_play level 87, 90 s | 0.79% | 33.9 ms | 17.00 ms | 4.57 ms | 1.90 ms | 0 | P5: the heaviest baked level (3,080 units at peak): 2,323 wisps, 757 glooms |
| 2026-11-20 | Moto | bench_play level 87, last 60 s of 20 min | 0.59% | 33.7 ms | — | 4.47 ms | — | 0 | P6 heat soak, battery 28.8 C at the end |
| 2026-11-20 | Moto | bench_flood 60 s after 60 s | 0.90% | 35.4 ms | 16.96 ms | 4.26 ms | 1.47 ms | 0 | P6 final build (no regression against P1: CPU +4%) |
| 2026-11-20 | Tablet SM-X230 | bench_flood 60 s after 60 s | 0.00% | 16.7 ms | 16.58 ms | 5.31 ms | 1.48 ms | 0 | 60 Hz (Motion smoothness: Standard) |
| 2026-11-20 | Tablet SM-X230 | bench_play level 87, 90 s | 0.00% | 16.9 ms | 16.58 ms | 5.29 ms | 1.67 ms | 0 | the heaviest level |
| 2026-11-20 | Tablet SM-X230 | bench_flood 600 s with sound | 0.00% | 16.8 ms | 16.58 ms | 5.40 ms | 1.49 ms | 0 | P4: 30,001 callbacks, fill p99 0.53 ms of a 20 ms period, 0 late |

Other measurements (Moto unless named):

| Check | Result |
|---|---|
| Determinism, levels 1-5, greedy bot, 60 s | PC and Moto hashes equal: 6ab53c6b7de9902a, 75fbb176a58bbe28, b555264948d891cc, 7988d22d19d3694a, 2c2b421ea381a278 |
| Flow test (`--flow-test`: screens, settings, save, input latency, SOUND and MUSIC end to end) | 45/45 on the PC and on the Moto (final build) |
| Layout lint, 6 screens x 7 frames + button strip | 0 problems; `captures/p3/contact.png` |
| Pressed vs normal fill (button strip) | 4.24:1 |
| Rebuild check (`--rebuild-gl-at`) for `hud` and `win` | Pixel-identical |
| Save through a force-stop mid-level | Level, stars and settings intact (`save-android.ps1`); damaged-file unit test passes |
| Lifecycle, 20 cycles during a level | 20/20 back on PAUSE, no errors |
| Cold start to the first frame (launch request to first swap) | Median 1.14 s, worst 1.33 s (target 2 s) |
| Back key | Moto: play to pause to play, HOME leaves the app |
| Strips: counts from the simulation for the captured ticks | 0 disagreements (gate: k out per one in; glooms lose power only to clashes and the lantern) |
| Audio renders (`review.ps1 -Set p4`, table in `captures/p4/metrics.md`) | Flood -14.9 LUFS, -2.6 dBTP, 0 clipped, DC 0.0004, 0% silence; music -25.4 LUFS (10.5 dB under), largest jump 0.033; ladder within 0.06 cents |
| Silence on the PC | With `--silent`: peak 0.534 before the mute, 0 non-zero samples handed to the device |
| Sound bank build | Moto 117 ms (was 849 ms) |
| Level bake | 299 levels accepted, 0 seeds rejected beyond the first try in most bands; determinism on 10 baked levels; soak 1-300 (greedy and sweep, invariants every tick): 600 runs, 0 failed |
| Level load (generate + meshes) on the Moto | 0.8 ms (limit 50 ms) |
| Flashing audit (`flash-audit.ps1`) | Win, boss win, lose and core hit, with and without REDUCE EFFECTS: at most 2 swings of 10% in any second (limit 3) |
| Heat soak, 20 min of `bench_play` on level 87 (`soak-android.ps1`) | Last 60 s inside the contract: late 0.59%, worst 33.7 ms, CPU p95 4.47 ms, valid. Battery 28.3 to 28.8 C. Memory (PSS, median of three reads on each minute mark) 95.5 MB at minute 1, 93.8 MB at minute 20 (-1.8%). `captures/soak/ZY22BXMJG3_soak_20261120_044332.csv`. (A first run sampled at 30 s and during a level reload and read +6.4%; its minutes 2-19 were flat, so the sampling was fixed, not the game.) |
| Sanitizers (ASan + UBSan: unit tests, determinism, acceptance, soak 1-50, an audio render, the flux table) | Clean: every run exit 0, no report |
| Greyscale sheet (`captures/p6/greyscale.png`) | Gates read by symbol (x and ÷); the crowds by shape (teardrops against horned blobs) |
| Win, boss win, lose and level-change strips (`captures/p6/`) | Slow motion 0.8 s, "CLEARED!" from 1.4 s, stars 250 ms apart, the fade 150 ms each way (by frame count) |
| Release APK | 3.31 MB; native Release, stripped (no `.debug` or `.symtab`); not debuggable; one permission (VIBRATE); `zipalign -c -P 16` passes and every `LOAD` segment is 0x4000-aligned; on the Moto it starts, ignores `--es args` (still running, scenario play) and `run-as` answers "package not debuggable" (`release-check-android.ps1`) |

## The final device matrix

| Device | Reached | Result |
|---|---|---|
| Moto One 5G Ace (`ZY22BXMJG3`, the floor phone) | Yes, over Wi-Fi | Inside the contract everywhere: `bench_flood` 0.90% late / 35.4 ms worst on the final build (and 600 s with sound: 0.54%, 0 late audio callbacks); `bench_play` level 5 0.90%, level 87 (the heaviest) 0.79%; the 20-minute soak; flow test 45/45; lifecycle 20/20 back on PAUSE; cold start 0.43 s; back key; save through a force-stop; determinism equal to the PC; the release APK |
| Galaxy Tab A11+ SM-X230 (`R5GL362BEHX`), Android 16, 1200x1920 at 240 dpi | Yes, over Wi-Fi (the owner switched Wireless debugging on) | All ten checks pass (`captures/matrix/R5GL362BEHX.md`): flow test 45/45, back key, save through a force-stop, cold start 0.52 s, lifecycle 20/20 back on PAUSE, `bench_flood` 0.00% late / 16.7 ms worst, `bench_play` level 87 0.00% late / 16.9 ms worst, CPU p95 5.3 ms. It ran at 60 Hz: the panel has a 90 Hz mode, but the tablet's Motion smoothness is set to Standard (`refresh_rate_mode` 0), which offers apps 60 Hz only; the game asked for the highest rate offered. 90 Hz needs that setting on Adaptive (the owner's to change), then `matrix-android.ps1 -Serial tablet` again |
| OnePlus 12R (`a0e622a5`) | Skipped by the owner on 2026-11-20 (its Wireless debugging was off) | Not run. Its screen (1264x2780 at 450 dpi) is in the layout matrix and passes the lint on the PC; the 120 Hz panel and the fallback are untested on a device. `matrix-android.ps1 -Serial op12r` runs it all |
| Fire 7 (`GN41WE04450600BJ`) | No | Open: a report only (TECH_SPEC 5.10) |

How the tablet and the 12R were looked for (2026-11-20, at the end of P6):

- **USB:** nothing but the Moto in `adb devices` all session.
- **Wireless debugging by mDNS** (how RAVEFORM finds the OnePlus): `adb mdns services` and a plain mDNS query from
  this PC got no answer from any device, the Moto included, so discovery does not work from here today.
- **Legacy `adb tcpip` (port 5555)** on the ten hosts of 192.168.137.x with phone-style (randomized) MAC addresses:
  every one refused.
- **Where they were last:** RAVEFORM's logs put the 12R at 192.168.137.112 and the tablet R5GL362BEHX at .169; adb's
  own log shows both connected to this PC over wireless debugging on 2026-11-19 (12R port 39437, with a TLS handshake
  that succeeded, so it still trusts this PC; tablet port 37505). Both hosts are on the network today.
- **A scan of 30000-61000** on them (Linux picks the port there; the Moto's is 38215) finds no adbd: the 12R has
  37567, 46888 and 47888 open and the tablet 44171, and each closes the connection on adb's first packet with no TLS
  handshake, so none is the debugging port. Wireless debugging is switched off on both (Android turns it off on a
  reboot or a change of network). A third host, .167:38975, behaves the same way.
- **Every other host** on 192.168.137.x, scanned over 30000-61000 too: the open ports on .147, .151, .167, .180 and
  .200 all close on adb's first packet; no adbd answers anywhere but on the Moto.
- **On the PC instead:** the screens are rendered and linted at the 12R's own panel (1264x2780 at 450 dpi, now in the
  layout matrix of `review.ps1 -Set p3`) and at the tablet's (1200x1920): every screen passes, nothing clipped. That
  checks layout only, not the devices.

To fill the open rows (one command runs every device check, each with the lock and the foreground check, and writes
`captures\matrix\<serial>.md`; on the Moto all ten checks passed: `captures\matrix\ZY22BXMJG3.md`):

1. On the device: Developer options, switch **Wireless debugging** on (the 12R and the tablet are already paired with
   this PC), or plug the tablet into USB. Then `scripts\find-wireless-adb.ps1` scans their last addresses and connects
   adb without mDNS (a device that no longer trusts this PC needs `adb pair` with the code its screen shows).
2. `scripts\matrix-android.ps1 -Serial tablet` (or `op12r`, or the `ip:port`). It installs the dev APK and runs the flow
   test, the back key, the save, cold start, 20 lifecycle cycles, `bench_flood`, `bench_play` on level 87, and records
   the panel's modes and the rate the game asked for (90 Hz on the tablet, 120 Hz on the 12R; the 60 Hz fallback logs
   `refresh:` if it trips).

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

## After version 1 (ideas, not built)

- The levers of GAME_DESIGN 5.12 (FRENZY and the others), if the contest needs more threat after playing.
- A lighter level of effects for the Fire 7 class (TECH_SPEC 5.10), if that device matters.
- Recorded samples instead of synthesis for any sound that still grates (a download: the owner's yes first).

## Open questions for the owner (the phase checkpoints)

1. **The look (P1):** `captures/p1/contact.png`, `captures/p6/contact.png` and the game on the phone. Anything to change?
2. **The fun (P2):** is the contest right? GAME_DESIGN 5.9 now says what the real simulation does (a sharp edge; a weak
   player loses late levels to the pressure ramp).
3. **The sound (P4):** which ladder timbre (`captures/p4/timbres_kalimba.wav`, `_glass.wav`, `_bell.wav`; the game uses
   the kalimba)? Is the big flood pleasant or busy? Keep the music, change its level, or off by default?
4. **The difficulty (P5):** play levels 1-20 with the dev build, then jump to 50, 100 and 200 (SETTINGS has a level
   stepper in dev builds). Right pace? Samey? Right length? Any unfair or dull level (its number is enough)? Change any of
   the five worlds? Try FRENZY?
5. **Sign-off (P6):** which phone do you carry? Then the icon and the name, and your own release steps in
   `docs/RELEASE_CHECKLIST.md` (the key, the store listing).
6. **The tablet and the 12R:** attach them (USB or wireless debugging) for their rows of the matrix.

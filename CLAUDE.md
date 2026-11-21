# GLOWTIDE — notes for Claude sessions

Read `docs/PLAN.md` first, then `docs/PROGRESS.md` (phase status, measured numbers, deviations, choices made for the
owner). `docs/GAME_DESIGN.md` and `docs/TECH_SPEC.md` by the sections a phase names; `docs/subplans/P*.md` per phase.

**Next work, planned and not built (2026-11-20): the variety and content pass** (upgrade pods on the lane, six kinds
of level, recipes, a calendar, no repeated levels). Read `docs/VARIETY_PLAN.md` first; it names the sections of
`docs/VARIETY_DESIGN.md` and `docs/VARIETY_TECH.md` and the work orders `docs/subplans/V0_*.md` to `V6_*.md`.

## Commands (PowerShell 7, repo root; every script dot-sources `scripts\env.ps1`)

| Task | Command |
|---|---|
| Third-party sources (SDL cloned from RAVEFORM's checkout, Khronos headers from the NDK; no download) | `scripts\fetch-deps.ps1` |
| Build desktop (`debug` = Debug + GL debug context, `dev` = RelWithDebInfo; `-Sanitize`: ASan/UBSan tests + headless) | `scripts\build-desktop.ps1 [-Config debug\|dev] [-Clean] [-Sanitize]` |
| Run desktop (silent and off screen unless `-Sound` / `-Watch`) | `scripts\run-desktop.ps1 [-Config dev] [-NoBuild] [-AppArgs '...'] [-Screenshot x.png] [-Frames N] [-Res WxH] [-Small]` |
| Tests (unit tests, PNG check, headless determinism; `-Quick` skips the long bot runs) | `scripts\test.ps1 [-Config dev] [-NoBuild] [-Quick]` |
| Build the APK (`dev` is the benchmarked build; with `$env:GT_APP_ID_SUFFIX = '.dev'` the dev APK is `dev.glowtide.game.dev`, installs beside a GitHub build, and every device script drives it) | `scripts\build-android.ps1 [-Config dev\|debug\|release] [-Offline] [-Clean]` |
| Install and launch on a device | `scripts\deploy-android.ps1 -Serial moto\|tablet\|op12r\|<serial> [-NoBuild] [-AppArgs '...']` |
| Benchmark on a device (lock, foreground checks, pulls `captures\bench\*.json`, grades the contract) | `scripts\bench-android.ps1 -Serial moto -Scenario bench_idle\|bench_flood\|bench_play [-Seconds 60] [-Warmup 180] [-AppArgs '...'] [-Deploy]` |
| Screenshot of a device's panel (GLOWTIDE in front only) | `scripts\capture-android.ps1 -Serial moto [-Out x.png] [-Small]` |
| 20 background/foreground cycles | `scripts\lifecycle-android.ps1 -Serial moto [-Cycles 20] [-AppArgs '...']` |
| Picture sets with contact sheets (`levels`: 60 top-down level thumbnails; `p4`: audio renders and their table; `v1`: geodes, lanterns, the boon tray over eight frames; `v2`: the elements with a hopper, a breathing gate, a sliding wall, a side door, runners and a comet, the KINDLE and COMET strips, levels 16 and 38 from above; `v3`: each kind, each boss, each twist, the colossus arriving and falling, dawn, and with `-Lint` each kind's HUD over the eight frames) | `scripts\review.ps1 -Set p1\|p2\|p3\|p4\|p5\|p6\|v1\|v2\|v3\|levels [-Only a,b] [-Lint]` |
| Connect a phone on wireless debugging when mDNS finds nothing (scans its address, `adb connect`s the adbd port) | `scripts\find-wireless-adb.ps1 -Ip <address>[,<address>]` |
| Every device check on one device (install, flow, picture checks, back key, save, cold start, lifecycle, both benches, refresh rate) -> `captures\matrix\<serial>.md` | `scripts\matrix-android.ps1 -Serial tablet\|op12r\|moto [-NoInstall] [-Pictures concept,geodes]` |
| The device picture check: a phone's render of a picture against the PC's at the phone's frame (VARIETY_TECH 10) | `scripts\picture-android.ps1 -Serial moto [-Picture concept] [-Against elements] [-Install]` |
| The variety report: likeness of levels, the no-repeat rule, counts per hundred levels (exit 1 if the rule breaks) | `glowtide_headless --sim variety --from 1 --to 300 [--pairs]` |
| Greedy's decisions on one level, a line a second (its aim, the geodes it sees, each line's score with and without the boons' worth); `--no-enemy` as in the bake's Pa run | `glowtide_headless --sim decisions --level 37 [--seconds 40] [--no-enemy]` |
| Re-bake a few levels into the table as it is built (the rows past `--to` up to `--keep-to` are kept) | `glowtide_headless --bake-levels --from 11 --to 11 --keep-to 300 --jobs 4 --out src\sim\levels_baked.h --report <scratch>.md` |
| Determinism on a phone against the PC (levels 1-5 and four geode levels) | `scripts\simhash-android.ps1 -Serial moto [-Install]` |
| Level sheets from above: thirty consecutive levels, or a list | `scripts\review.ps1 -Set levels -Config dev -From 40 -Count 30`, `-Levels 12,40,77` |
| Flow test on a device (42+ checks: screens, settings, save, input) | `scripts\flow-android.ps1 -Serial moto [-NoInstall]` |
| Save through a force-stop / back key / cold start | `scripts\save-android.ps1`, `backkey-android.ps1`, `coldstart-android.ps1 -Serial moto` |
| Heat soak: 20 min of `bench_play` on the heaviest level, temperature and memory per minute | `scripts\soak-android.ps1 -Serial moto [-Minutes 20] [-Level 87]` |
| The release APK on a device: starts, ignores arguments, not debuggable (puts the dev build back) | `scripts\release-check-android.ps1 -Serial moto` |
| Flashing audit of the win, boss win, lose and core-hit strips | `scripts\flash-audit.ps1 [-NoBuild]` |
| Level bake (writes `src\sim\levels_baked.h`; about 4 min at 4 jobs) | `build\desktop-dev\glowtide_headless.exe --bake-levels --from 2 --to 300 --jobs 4 --out src\sim\levels_baked.h --report captures\levels\report.md` |
| Band tables and the P5 checks from the report | `python -I -B tools\band_table.py captures\levels\report.md captures\levels\bands.md` |
| Offline audio and its numbers | `glowtide_headless --render-audio ladder\|flood\|ui\|win\|lose\|music\|timbres --out x.wav`; `python -I -B tools\wav_metrics.py x.wav [--ladder ladder.wav]` |
| GAME_DESIGN 5.9's findings with the real simulation | `glowtide_headless --sim flux` |
| Review copy of a PNG (at most 1600 x 1000) | `python -I -B tools\shrink_png.py x.png` |
| App icon layers | `python -I -B tools\make_icon.py` |
| The release APK on GitHub (`.github/workflows/release.yml`, `docs/RELEASE_PIPELINE.md`): each push to `main` publishes release `build-<commit count>`; by hand, watch, download; the release key (the owner runs it: it asks for the password) | `gh workflow run release.yml -R rockenrooster/glowtide`, `gh run watch -R rockenrooster/glowtide`, `gh release download -R rockenrooster/glowtide -p glowtide.apk`; `scripts\release-key.ps1 [-SecretsOnly] [-NoBuild]` |

Desktop exit codes: 0 ok, 1 start-up failed, 2 bad arguments, 3 GL errors or debug messages, 4 benchmark invalid,
5 capture failed, 6 flow test failed, 7 a lint failed.

App arguments: TECH_SPEC 2.5 (`--scenario`, `--level`, `--bot`, `--picture`, `--screenshot`, `--frames`, `--strip`,
`--strip-step`, `--seconds`, `--warmup`, `--res`, `--dp-scale`, `--safe`, `--set name=value`, `--silent`, `--no-audio`,
`--offscreen`, `--tone`, `--fps`, ...). On Android they come from the `args` intent extra (debuggable builds only):
`adb -s <serial> shell "am start -n dev.glowtide.game/.GlowtideActivity --es args '--scenario bench_idle --seconds 30'"`.

Log tags: `GLOWTIDE`, `GTBENCH` (one JSON line per benchmark; the full JSON is `bench_<scenario>.json` in the app's
external files directory on Android), `GTSIM`, `GTLIFE`, `FLOW`.

## Owner's rules (they hold for every run)

- **PC runs are silent and off screen**: the scripts pass `--silent` and `--offscreen`; a process started by hand gets
  them too. Phones keep their sound.
- **At most 4 CPU cores** on the PC: `env.ps1` confines the process tree; builds use 4 jobs; Gradle gets its own daemon
  (`-XX:ActiveProcessorCount=4`); never `gradlew --stop`.
- **Shared devices** (PLAN 3.3): every adb command with `-s <serial>`; nothing device-wide (no kill-server, reboot,
  `logcat -c`, settings); check the foreground app before launching or capturing; take the lock file in
  `D:\Stuff\DevFolder\Android_Games\Raveform\captures\locks\` (the scripts do); force-stop only `dev.glowtide.game`.
- Downloads need the owner's yes. No destructive git or shell commands. Short chat replies. The repository is public:
  nothing secret in it (keys, tokens, passwords, personal addresses).
- Never edit, build or commit inside `..\Raveform\` (only its lock folder is shared).

## Conventions

- C++20, no exceptions, no RTTI, warnings are errors. `src/sim` is deterministic: no wall clock, no libm beyond
  `sqrt`/`floor`, no allocation after `World::init`, `-ffp-contract=off`.
- OpenGL ES 3.0 core only; shaders are `#version 300 es` with the prelude (`shaders/prelude*.glsl`), validated by
  glslangValidator at build time and embedded. Fragment shaders are `mediump` unless a value needs `highp`.
- No heap allocation in the steady-state frame (global `operator new` is counted; benchmarks report `heap_allocs`).
- Git: commit at working checkpoints. `origin` is the public GitHub repository `rockenrooster/glowtide` (since
  2026-11-20): push only when the owner asks; every push to `main` builds and publishes a release APK
  (`docs/RELEASE_PIPELINE.md`). `keys/` holds the release key: gitignored; never commit it, copy it or print it.

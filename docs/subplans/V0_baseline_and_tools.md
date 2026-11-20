# V0 — Baseline and tools

Size S. Needs nothing. Changes nothing a player can see.

**Goal.** The instruments this pass is judged by exist before the first new rule does: the variety
report, the device picture check, the table-version test. The game as it is today is measured with
them, so every later phase has a number to beat.

## Read first

`VARIETY_PLAN.md` 0, 2, 5. `VARIETY_TECH.md` 2 (the two version numbers), 5 (what the bake prints), 6
(all), 10 (the device picture check), 12 (all ten traps). `PROGRESS.md`. `CLAUDE.md`.

## Tasks

**Before task 1: start from green.** Confirm the tree works as you found it, before you change a line:
`scripts\fetch-deps.ps1`, `scripts\build-desktop.ps1 -Config debug`, `scripts\build-desktop.ps1 -Config
dev`, `scripts\test.ps1`, the flow test on the PC (`scripts\run-desktop.ps1 -Config dev -NoBuild
-AppArgs '--flow-test'`: 45 checks, exit 0), `scripts\build-android.ps1 -Config dev -Offline`. If any of
these fails on an untouched tree, stop and tell the owner: nothing in this plan explains it.

1. **A safety copy.** The owner's rule (PLAN 0 rule 7): before a breaking change, copy the working tree to
   `D:\Stuff\DevFolder\_backups\Glowtide\` (without `build\`, `third_party\`, `android\.gradle\`,
   `android\app\build\`, `android\app\.cxx\`). Tag the commit locally (`v0.1.0`); do not push the tag
   unless the owner asks.
2. **The bake is reproducible.** Bake levels 2 to 300 into a scratch file
   (`build\desktop-dev\glowtide_headless.exe --bake-levels --from 2 --to 300 --jobs 4 --out <scratch> --report captures\levels\report.md`)
   and compare it with the committed `src\sim\levels_baked.h`: they must be identical. If they are not,
   find out why before anything else: every later comparison rests on this. Then run
   `python -I -B docs\variety\baseline_stats.py` and check it prints `VARIETY_PLAN.md` 2's numbers.
   (The planner did both on 2026-11-20 at commit `7990557`: the table and the report came out byte for
   byte the same, in about four minutes. `captures\` is not in the repository, so the report exists
   only after a bake.)
3. **Split `src\sim\levels.cpp`** with no change in behaviour: `levels_hand.cpp` (levels 1 to 5),
   `recipes.cpp` (today's generator as the recipe "Terraces"), `calendar.cpp` (world, target time,
   `fight`, what a level may use: today's band table), `levels.cpp` (entry points, the baked table).
   The proof that nothing changed: task 2's bake is still identical, and the determinism hashes of
   levels 1 to 5 in `PROGRESS.md` are unchanged.
4. **Version numbers** (VT 2): `kGeneratorVersion` and `kRulesVersion` in `level.h`, written into
   `levels_baked.h`'s header by the bake, and a unit test that fails when the table's differ from the
   code's. Show once that it fails (bump one, run the tests, see the failure, bake, see it pass).
5. **Fingerprint and likeness** (VT 6.1, 6.2) for today's level data: kind, recipe, plan and length
   class are constants for now; the map and the boon list are real. `glowtide_headless --sim variety`
   with `--from`, `--to`, `--pairs` (VT 6.4). Add it to `scripts\test.ps1` only from V1 (today's table
   fails it by design).
6. **Measure the baseline with it.** Run the report on levels 6 to 300. Record in `PROGRESS.md`: the
   share of neighbour pairs (1 to 12 apart) above 0.45, of pairs 13 to 40 apart above 0.60, the number
   of pairs above 0.85, the longest run with one kind and one recipe (all 295), and the five most alike
   pairs with their thumbnails side by side.
7. **Consecutive-level sheets.** `scripts\review.ps1 -Set levels` renders every fifth level today. Add
   `-From N -Count M` so `-From 40 -Count 30` renders thirty consecutive levels from above onto one
   sheet. Render 40–69 and 200–229 and look at them: this is the "before" picture for the owner.
8. **The device picture check** (VT 10, VT 12 trap 1). `scripts\picture-android.ps1 -Serial <name>
   -Picture <name>`: with the device lock, awake and unlocked, start the dev build with
   `--picture <name> --frames 120 --screenshot <file>` (a debuggable build takes arguments; the app
   writes the PNG into its external files directory and holds the frame), pull the PNG, render the same
   picture on the PC at the device's frame size, dp scale and safe insets (the app logs them:
   `frame WxH px, dp scale …, safe top … bottom … px`; the PC takes `--res WxH --dp-scale S --safe
   top,bottom` with the insets in dp, so divide the logged pixels by the dp scale), and compare with
   `tools\picture_compare.py`:
   the share of gold pixels and of violet pixels in the lane's area each within 20% of the PC's, and
   the mean difference of the two pictures shrunk to 64 px wide under a threshold you set from three
   good runs. Show that it can fail: compare a device's `concept` with the PC's `elements`. Add the
   check to `scripts\matrix-android.ps1` with the picture `concept`, and run the matrix on the Moto,
   the tablet and the 12R.
9. **The stars slot** (VT 9.3): stop writing stars for levels past the array's end.

## Exit checks

- [ ] A fresh bake of levels 2 to 300 is identical to the committed table, before and after the split
      of `levels.cpp`.
- [ ] `scripts\test.ps1` exits 0, with the version test in it; the test was seen to fail on a bumped
      version.
- [ ] `--sim variety` runs on levels 6 to 300 and its baseline numbers are in `PROGRESS.md`. (It is
      expected to exit 1: today's levels break the rule. Say by how much.)
- [ ] The two consecutive-level sheets exist and you looked at every thumbnail.
- [ ] `matrix-android.ps1` passes on the Moto, the tablet and the 12R with the picture check in it; the
      picture check was seen to fail against a wrong reference.
- [ ] No change in `bench_flood` on the Moto (nothing in the frame changed; one 60 s run after a 60 s
      warm-up is enough here).
- [ ] The flow test (PC and Moto) and the layout lint still pass.

## Owner checkpoint

None. Send a short note: the baseline numbers, the two "before" sheets, and that V1 starts.

## Not in this phase

Any new rule, any change to a level, any new art or sound.

## Notes

- The picture check compares a phone's own render with the PC's. The phones' GPUs dither and blend
  slightly differently, so compare shares and a shrunken picture, never pixels.
- If discovery does not find a phone, VT 12 trap 4.
- Keep `docs\variety\baseline_stats.py` until V4; it is the only measure that predates the pass.

# V6 — Balance, polish, release

Size M. Needs V4 and its checkpoint (and V5 if it was built).

**Goal.** The pass is finished the way version 0.1.0 was: the final bake with the owner's answers in
it, every check on three phones, the documents true, and a release the owner signs off.

## Read first

`VARIETY_PLAN.md` 5, 6, 7. `VARIETY_DESIGN.md` 12. `VARIETY_TECH.md` 10, 11, 12.
`subplans\P6_polish_release.md` (what the first pass's last phase checked: do all of it again).
`RELEASE_PIPELINE.md`, `RELEASE_CHECKLIST.md`.

## Tasks

1. **The owner's answers** from V1, V3 and V4 that are not in yet: each as a change to a number, a
   recipe, the calendar or an acceptance line, logged in `PROGRESS.md` with what they said.
2. **The final bake** of 600 levels with final rules. Both version numbers bumped and matching. The
   band tables by level and by kind. Every line, every band: a miss is fixed or brought to the owner
   with its numbers, never left.
3. **A second look at balance, by kind and by boon**, from the report: for each boon, how often
   `greedy` takes it when offered and what it gains (a boon never taken is dead weight, one always
   taken is not a choice); for each plan and twist, the greedy bot's lowest core; for each recipe, the
   share of seeds rejected. Tune the outliers; bake again.
4. **Juice, second pass.** Every row of VD 13's two tables: built, seen in a strip, inside its rate
   limit, with a sound and a haptic where the table says so. The camera's trauma from the new sources
   stays under GD 10's ceiling.
5. **Everything the first pass's P6 checked, again**: the flashing audit with every new effect, with
   and without `REDUCE EFFECTS`; the greyscale sheet with every new thing; the win, lose and next
   strips for every kind; the rebuild check (`--rebuild-gl-at`) on a geode level and a festival; the
   sanitizer build (unit tests, determinism, acceptance, the soak on 1 to 100 with `greedy` and
   `hungry`, an audio render); the layout lint in eight frames for every screen and every kind's HUD.
6. **Heat and memory**: `scripts\soak-android.ps1` on the Moto for twenty minutes on the heaviest
   level, and twenty minutes of a festival looping (the busiest picture for props and labels). The last
   60 s inside the contract; memory at minute 20 within 5% of minute 1.
7. **The device matrix**: `matrix-android.ps1` on the Moto, the tablet and the 12R, with picture checks
   on `concept`, `geodes`, a colossus and a festival. If the owner has set the tablet or the 12R to its
   high refresh rate, the 90 and 120 Hz rows of PLAN 5 at last, and the 60 Hz fallback (TS 5.10) if a
   level is late.
8. **Sound**: `scripts\review.ps1 -Set p4` with the `geodes` scene; the callback's timing in a
   ten-minute `bench_flood` with sound on the Moto (no late callback); the bank's build time.
9. **The release build**: `build-android.ps1 -Config release`; under 15 MB; 16 KB alignment
   (`zipalign -c -P 16`); not debuggable; `release-check-android.ps1` on the Moto.
10. **Documents made true.** `PROGRESS.md`: this pass's phase table, every number, every choice made
    for the owner, every deviation from VD and VT. `GAME_DESIGN.md` and `TECH_SPEC.md`: a short section
    each that says which of their sections VD and VT now replace, so a reader of the old documents is
    not misled. `CLAUDE.md`: every new command and script. `README.md`: the game as it now is.
    `RELEASE_CHECKLIST.md`: sizes, and anything a store listing would say differently. `LICENSES.md` if
    anything was added (nothing should have been).
11. **Clean up**: delete what nothing uses (`Burst` if twists replaced it, `docs\variety\baseline_stats.py`,
    tweakables no code reads, debug views); no half-built stretch item left in the tree.
12. **Release**, on the owner's word: `versionName` (0.2.x), push, watch the build
    (`gh run watch -R rockenrooster/glowtide`; a 429 from Maven Central is a re-run, VT 12 trap 7),
    check the release's files, and check on a phone that an older GitHub build shows `UPDATE: BUILD n`.

## Exit checks

- [ ] The three contracts of VARIETY_PLAN 5 hold on the final table and the final build, with the
      numbers in `PROGRESS.md` beside V0's baseline.
- [ ] `scripts\test.ps1`, the flow test (PC and all three phones), the layout lint, the flashing audit,
      the sanitizer run, the rebuild check: all pass on the commit the release is built from.
- [ ] The soaks: inside the contract in the last 60 s; battery temperature logged; memory within 5%.
- [ ] `matrix-android.ps1` passes on the Moto, the tablet and the 12R with the four picture checks.
- [ ] The release APK: under 15 MB, aligned, not debuggable, ignores arguments, starts on the Moto.
- [ ] No stretch item is half-built; no unused code from this pass remains; `git status` is clean.
- [ ] `PROGRESS.md`, `CLAUDE.md`, `README.md`, `RELEASE_CHECKLIST.md` are complete and true; the old
      design and tech documents point to the new ones where they were replaced.
- [ ] After the owner's go: the GitHub release exists with its three files and the updater shows it.

## Owner checkpoint

The sign-off session on the phone they carry: any twenty levels of their choosing. Then the release.
Anything they still want changed is a numbered list in `PROGRESS.md` under "after 0.2", not a late
change to a finished phase.

## Not in this phase

New content of any kind. Store submission, the release key (the owner runs
`scripts\release-key.ps1`), the Play store's steps (`RELEASE_CHECKLIST.md`).

## Notes

- Until the owner has run `scripts\release-key.ps1`, each GitHub build is signed with a key made for
  that run, so the `UPDATE` button's download does not install over the build that showed it
  (`RELEASE_PIPELINE.md`, Signing). Say so in the note to the owner with the release.
- The first pass found its worst bug (a phone drawing no crowd) only when the owner looked. The picture
  checks exist so this pass finds its own. If a picture check fails, believe it.
- Polish is where scope creeps (the first pass's P6 said the same). Each addition here is a row of VD
  13 or a line of this file.

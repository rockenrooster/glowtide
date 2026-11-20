# P6 — Polish and release prep

Size M. Needs P3, P4 and P5.

**Goal.** The game feels finished and is safe to hand to other people: the win and lose sequences,
every row of the juice table, the icon, accessibility, a thermal soak, sanitizer runs, a release build
and a checklist of what is left for the owner.

## Read first

`GAME_DESIGN.md` 10 and 14. `TECH_SPEC.md` 3.3, 5.10 and 8. `PLAN.md` 5, 9 and 10.

## Tasks

1. **The win, the boss win and the lose** exactly as GAME_DESIGN 10 describes them: slow motion, the
   spire's shards, the wave of popping glooms, the dawn, the wisps turning round to cheer, the stars;
   the lantern going dark, the loss of colour. Strips of each.
2. **A second pass over the juice table:** every row is implemented, visible in a strip, and inside
   its rate limits. The camera's follow and shake are as specified. The fade between levels is 150 ms
   each way through `grade()`, with the world re-initialised in between.
3. **Refresh rate:** the one-way 60 Hz fallback of TECH_SPEC 5.10; check the tablet at 90 Hz and the
   OnePlus at 120 Hz.
4. **Heat:** twenty minutes of `bench_play` looping on the Moto. If it cannot hold, trim with
   TECH_SPEC 8.5; never lower the resolution.
5. **Accessibility** (GAME_DESIGN 14): REDUCE EFFECTS complete; a greyscale sheet of the main pictures;
   a flashing audit: a script that takes the mean brightness of every frame of the win, lose and
   core-hit strips and counts, per second, the times it moves by more than 10% of full scale.
6. **The icon:** `tools/make_icon.py` draws a gold wisp on the twilight gradient as adaptive icon
   layers. The app's name string.
7. **Credits and version** in SETTINGS: three lines in the string table (GAME_DESIGN 11).
8. **The flow test, end to end:** now that P3 and P4 are both in, extend it to the SOUND and MUSIC
   switches.
9. **Sanitizers** on the desktop: an AddressSanitizer and UndefinedBehaviorSanitizer build of the
   tests and the headless tool, running `test.ps1`'s checks and a soak over levels 1 to 50. Not the
   windowed app: that would put the GPU driver under the sanitizer.
10. **The release build:** native Release, stripped, not debuggable, arguments ignored; APK size by
    part; the shared libraries aligned for 16 KB pages; `docs/RELEASE_CHECKLIST.md` (target API, the
    single permission, no data collection, content rating notes, the photosensitivity note, and the
    owner's own steps: the release key, the store listing and its pictures).
11. **The final benchmark matrix** on every device that can be reached.
12. **Clean up:** delete scaffolding nothing uses any more, unused tweakables and stale notes; bring
    `CLAUDE.md` and `PROGRESS.md` up to date.

## Exit checks

- [ ] The strips of the win, the boss win, the lose and a level change were reviewed; their durations
      match GAME_DESIGN 10 by frame count.
- [ ] The flashing audit passes with and without REDUCE EFFECTS: no more than three such changes in
      any second.
- [ ] The greyscale sheet: gates are told apart by symbol and teams by silhouette (you checked; the
      owner confirms).
- [ ] The soak: in the last 60 s of twenty minutes the Moto is inside the contract; battery temperature
      is logged; memory at minute 20 is within 5% of minute 1.
- [ ] On the tablet and the OnePlus (if reachable) the game runs at the panel's rate inside the
      contract, or the fallback took it to 60 Hz and the log says why.
- [ ] The sanitizer run exits 0 with no report.
- [ ] The release APK is under 15 MB, starts on the Moto, ignores arguments, is not debuggable, and
      its libraries are 16 KB aligned.
- [ ] The final matrix is in `PROGRESS.md`: the Moto, the tablet, the OnePlus, the Fire as a report.
- [ ] `scripts\test.ps1`, the flow test and the layout lint pass on the code the release was built from.
- [ ] `PROGRESS.md`, `CLAUDE.md`, `LICENSES.md` and `RELEASE_CHECKLIST.md` are complete and true.

## Owner checkpoint

The sign-off session on the phone they actually carry (ask which). Then: the icon and the name. Then
hand over the checklist of their own steps. Signing with their key and anything about a store are
theirs; do not attempt them.

## Not in this phase

Store submission, new content, new mechanics, quality tiers or graphics settings, anything on PLAN 10's
out-of-scope list.

## Notes

- Polish is where scope creeps. Each addition must be a row of GAME_DESIGN 10 or a line of this file.
  Ideas beyond that go into `PROGRESS.md` under "after version 1" for the owner to choose from.
- The win is the reward for the whole level. If the budget is tight anywhere in this phase, spend it
  on the win.
- If the Fire 7 matters to the owner after seeing its numbers, that is a new, small phase (a lighter
  level from TECH_SPEC 8.5's list), not something to slip in here.

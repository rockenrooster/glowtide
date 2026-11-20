# P3 — Game loop and UI

Size M. Needs P2 and the owner's "yes, it is fun". Can run beside P4.

**Goal.** A complete game around the five levels: every screen of GAME_DESIGN 11, the HUD, hints,
settings, saving, haptics and the Android lifecycle. It passes the flow test and the layout lint at
every aspect ratio, and starts in two seconds.

## Read first

`GAME_DESIGN.md` 4, 10 (the UI motion lines), 11, 13 and 14. `TECH_SPEC.md` 3.3, 3.5, 5.11, 7 and 8.4.

## Tasks

1. **UI layer, complete** (TECH_SPEC 5.11): buttons with the pressed state of GAME_DESIGN 10,
   switches, the hold-to-confirm button, anchors, the safe area, dp scaling, the string table.
2. **Screens** (GAME_DESIGN 11): HOME, a level waiting for its first touch; the HUD (level chip, spire
   bar, pause, the tide counter with its punch and the flood banners); PAUSE; WIN (stars, time, highest
   tide, NEXT, REPLAY); LOSE (an advice line chosen from what happened, RETRY, HOME); SETTINGS.
3. **Flow** (TECH_SPEC 7.1): the screen switch; one `World`, initialised when a level is entered
   (launch, NEXT, RETRY, REPLAY, RESTART) and not ticked until the first touch; retry and restart keep
   the seed; the level counter. Between levels a short fade through `grade()` (GAME_DESIGN 10).
4. **Save and settings** (TECH_SPEC 7.3): stars, level, switches, hints. The SOUND and MUSIC switches
   are stored now and obeyed in P4.
5. **Hints**, each shown once (GAME_DESIGN 11).
6. **Android:** the back key (GAME_DESIGN 4; TECH_SPEC 2.4); pause and save on the way to the
   background; the handler for `SDL_EVENT_RENDER_DEVICE_RESET` and `--rebuild-gl-at`
   (TECH_SPEC 3.3); haptics (TECH_SPEC 3.5, GAME_DESIGN 13); immersive mode re-applied when focus
   returns.
7. **Results:** stars by the rule of GAME_DESIGN 5.11; tide marks.
8. **Auto-fire and REDUCE EFFECTS** wired to what exists so far.
9. **Layout lint** complete; the screen pictures (`home`, `hud`, `pause`, `win`, `lose`, `settings`)
   across the aspect matrix; a `button_strip`; `scripts/review.ps1 -Set p3`.
10. **Flow test** (`--flow-test`, TECH_SPEC 8.4).
11. **Start-up time:** measure cold start to the first interactive frame on the Moto and remove
    whatever makes it exceed two seconds.
12. **A development-only level stepper** in SETTINGS (previous, next, jump by ten), behind the
    debuggable flag: P5's playtest needs it.

## Exit checks

- [ ] `--flow-test` exits 0 on the desktop and, launched through its arguments, on the Moto.
- [ ] The layout lint exits 0 for every screen in all seven frames; you reviewed the contact sheets.
- [ ] `button_strip` shows normal, pressed and released; the pressed fill differs from the normal
      fill by a contrast of at least 3 to 1 (measured from the picture).
- [ ] Save: force-stop the app in the middle of a level, start it again: the level number, stars and
      settings are intact. The damaged-file test passes.
- [ ] Lifecycle: `scripts\lifecycle-android.ps1 -Serial moto -Cycles 20` during a level: it comes back
      paused every time, nothing crashes.
- [ ] The rebuild check of TECH_SPEC 8.4 passes on the desktop for `hud` and `win`.
- [ ] Cold start to an interactive HOME is at most 2 s on the Moto (`am start -W` plus the first-frame
      log line). On HOME, a touch starts the level and the first wisp leaves in the next tick.
- [ ] The back key does what GAME_DESIGN 4 says on the Moto and on the tablet (sent with
      `input keyevent BACK` only while our app is in front).
- [ ] Haptic calls stay at or under twelve a second in `bench_play` (counted in the log).
- [ ] `scripts\test.ps1` exits 0; no regression in `bench_flood` and `bench_play` (TECH_SPEC 8.3).

## Owner checkpoint

From a cold start: play through level 5, lose once on purpose, pause, change settings, close and reopen
the app. Ask: is anything in the way between levels? Is every text readable at arm's length? Are the
haptics too strong, too weak, too frequent?

## Not in this phase

Sounds and music (P4), generated levels (P5), the big win and lose sequences (P6; here a short fade and
the panel are enough), a level-select screen (not in version 1), any graphics setting (there is one
quality level, TECH_SPEC 5.10).

## Notes

- HOME is the game with a hint on top, not a menu scene. If a design choice adds a tap between
  launching and pouring, it is wrong (PLAN 1, pillar 2).
- Buttons are found by label in the flow test, so labels are part of the interface: change them in the
  string table and the test together.
- Nothing in the tooling may need a key press in the desktop window: it cannot take focus when off
  screen (TECH_SPEC 2.3). Use arguments.

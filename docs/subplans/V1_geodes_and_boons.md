# V1 — Geodes and boons

Size L. Needs V0. **This phase is the gate for the idea**: nothing else is built until the owner has
played it.

**Goal.** Upgrades come down the lane. Geodes (plain and shielded) with four boons (`LANTERN`,
`QUICKEN`, `BRIGHT`, `WARD`) are in the game as it is today: placed in today's levels by simple rules,
balanced by the new bake, drawn, heard, measured on three phones. The owner plays levels 6 to 25 and
says whether this is the upgrade they meant.

## Read first

`VARIETY_DESIGN.md` 1, 2, 3, 9.3, 12, 13, 14. `VARIETY_TECH.md` 2, 3, 4, 5, 7, 8, 9, 10, 11.
`GAME_DESIGN.md` 5.2, 5.3, 5.6 (a geode is a barrier that moves), 7.4, 8, 9.4, 10.

## Tasks

Build one plain `LANTERN` geode on level 6 from end to end (rule, bot, bake, prop, label, sound,
picture, phone) before adding the second boon. The order below is that path.

1. **Data** (VT 2): `geodes[16]` and `ref` in `LevelDef`; the baked row gains `ref`; bump both
   version numbers.
2. **Rules** (VD 2.1, 3, 3.1, 3.2; VT 3): geodes appear, drift, slide, slip past; plates; the absorb
   step in its place in the tick; opening; the four boons; the row of lanterns and the aim range; the
   cap with more than one lantern; `spent`; events; everything new in the hash; every number a
   tweakable. A unit test for each rule (VT 10's first row, the parts that exist in this phase).
3. **Bots** (VD 12.1, VT 4): `greedy` values geodes; `podless`; `hungry`. Check by hand on three levels
   that `greedy`'s choices are sane: print, a decision at a time, the worth and the cost it saw.
4. **Placement in today's levels** (VD 9.3; the generator is still "Terraces"): one to three geodes a
   level from level 6, with the budget, the timing and the detour rule; shielded ones from 11; `BRIGHT`
   from 12; `WARD` from 23. Hand-made levels 6 (the first geode: one plain `LANTERN` on the open side
   of a two-gate lane), 11 (the first shield) and 23 (the first `WARD`, on a lane where the enemy gets
   close), each with its hint (VD 14). Levels 1 to 5 do not change.
5. **Bake v2 for Siege** (VD 12.2, VT 5): `Pa`, `Pn`, `m`, `R`, geode health, the enemy, the spire,
   lines a to i, the band checks of step 8; the report's new columns; `tools\band_table.py`. Bake 2 to
   300. Read the band table before going on: it is this phase's first real result.
6. **View** (VD 13, VT 7): the geode's shell, four icons, the plate, the side lanterns; the numbers;
   the pool of light; the effects for hit, plate break, opening with the icon's flight, each boon, and
   slipping past.
7. **HUD and words** (VD 14): the boon tray, the banner queue with the boon banner, `BOONS n/m` on the
   result line, the three hints of this phase, the two new lines of advice on LOSE. Layout lint in all
   eight frames.
8. **Sound and haptics** (VD 13, VT 8): `tink`, `geode_open`, the four motifs, the slip. A `geodes`
   scene in `--render-audio`; `scripts\review.ps1 -Set p4` inside GD 12's targets.
9. **Pictures** (VT 10): `geodes`, `geode_strip`, `lanterns_3`, the HUD with a full tray; the strip's
   count check (power given equals health lost); the greyscale sheet with a geode beside a gate, a
   gloom, a brute and a snuffer; the flashing audit with a geode opening and `WARD`.
10. **Flow test**: a geode level played by `greedy`: a boon is applied, its chip shows, the result line
    counts it.
11. **The bench scene** (VT 11): three lanterns with `QUICKEN` and `BRIGHT` at their limits and four
    unbreakable geodes in the stream. `bench_flood` on the Moto after the three-minute soak;
    `bench_play` on the heaviest baked level with `greedy` taking geodes.
12. **Phones**: `matrix-android.ps1` on the Moto, the tablet and the 12R, with the picture check on
    `geodes` as well as `concept`.

## Exit checks

- [ ] Unit tests: a geode takes exactly the power given and passes the rest on; a plate goes first
      from any side; opening applies the boon in that tick; slipping past costs nothing; each boon's
      arithmetic and limit; the aim range for one, two and three lanterns; at the cap three lanterns
      still feed three columns. No allocation in a tick. Determinism holds on levels 1 to 5 and on
      three geode levels, and the Moto's hashes equal the PC's.
- [ ] The bake finished: every level 2 to 300 passes lines a to i within eight seeds. `PROGRESS.md` has
      the band table and says, per band: `greedy`'s geodes opened of those offered; how much slower
      `podless` is; the share of levels where `hungry` is slower than `greedy`; the share where
      `greedy` loses a pip.
- [ ] Step 8 holds on every band from level 21 on, or `PROGRESS.md` says which share missed, by how
      much, and what was tried. (A miss here is something to bring to the owner, not to hide.)
- [ ] `--sim variety` on the new table: the likeness of neighbours has fallen against V0's baseline
      (it will still fail the rule: one recipe, one kind). Record the numbers.
- [ ] The performance contract holds on the Moto with the new bench scene; no regression against
      `PROGRESS.md`'s last `bench_flood` (late share within two points, CPU P95 within 10%); zero heap
      allocations.
- [ ] `geode_strip`'s counts agree; the greyscale sheet tells a geode from everything else at the
      smallest frame (600 × 1024); the flashing audit passes with and without `REDUCE EFFECTS`.
- [ ] The flood render with geodes is inside GD 12's loudness and peak targets; the sound bank still
      builds in under 150 ms on the Moto.
- [ ] `scripts\test.ps1`, the flow test (PC, Moto), the layout lint (eight frames) and
      `matrix-android.ps1` (three phones, with the picture checks) pass.

## Owner checkpoint (do not start V2 before it is answered)

Put the dev build on the phone the owner names. Ask them to play levels 6 to 25, then jump to 60 and
150 with SETTINGS' level stepper. Ask:

1. Is this the upgrade you meant: something coming at you that you decide to shoot?
2. Did you find yourself choosing between a geode and the enemy, or was it always obvious?
3. Too many geodes, too few? Too tough, too soft? Do they come too fast?
4. Two and three lanterns: fun? Too strong?
5. Should a boon ever last past its level? (The plan says no: VARIETY_PLAN 3, decision 2.)
6. Left open by version 0.1.0: which ladder sound (kalimba, glass, bell; `captures\p4\`), and is the
   music's level right?

Their answers change VD 2 and 3 first, the numbers second. If they accept the phase: version 0.2.0
(VARIETY_PLAN 7), and push when they say so.

## Not in this phase

`KINDLE`, `COMET`, `HUSH`; choice pairs, vaults, runner geodes; new gates, walls, plans; kinds; twists;
recipes; the no-repeat rule in the bake.

## Notes

- The whole point is the decision (VD 2.3). If the band table shows `hungry` is as fast as `greedy`,
  geodes are free: raise their health or move them further from the best line. If `podless` is as fast
  as `greedy`, they are worthless: the level is not sized for them (raise `m`) or they come too late.
- A geode must never sit where no stream can reach it for its whole life (behind a ÷ gate's full
  width, inside a bumper's shadow): add that to the generator's lints.
- Three lanterns through a kindled lane is next phase's problem; this phase's caps test is three
  lanterns, `QUICKEN` ×3, `BRIGHT` ×2 on the richest baked lane.
- The number on a geode is the main thing a player reads. The number itself changes in the tick the
  health does, so it is seen to fall even under a thin stream; only its punch animation is limited (one
  per 80 ms).

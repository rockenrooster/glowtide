# V2 — Lane and enemy

Size L. Needs V1 and its checkpoint.

**Goal.** More things a lane can hold and more ways the enemy can come: hopper and breathing gates,
sliding walls, side doors, runners, eight enemy plans, and the three boons that change the lane or the
moment (`KINDLE`, `COMET`, `HUSH`). Each arrives complete. Today's generator uses them, so they are in
play before recipes exist.

## Read first

`VARIETY_DESIGN.md` 3 (the last three boons), 4, 5, 13. `VARIETY_TECH.md` 2, 3, 4, 6.1 (a cell code for
each new thing), 7, 10, 11. `GAME_DESIGN.md` 5.4, 5.6, 5.9, 7.3.

## Tasks

A thing arrives complete or not at all (VARIETY_PLAN 7): rule and unit test, the bot's handling, prop
and label, cue, its code in the fingerprint, its place in the `elements` picture, a strip, and for the
first time it appears, a hand-made level with a hint. Do the must items in this order; then the stretch
items in theirs, only while the phase is inside its contracts.

1. **Hopper gates** (VD 4 #1): the gate's mode in the level data (VT 2); the jump without `sin`; the
   ghost frame at the other place; the bot takes the gate where it will be on arrival. Hand-made level
   16.
2. **Breathing gates** (VD 4 #2). Hand-made level 38.
3. **Sliding walls** (VD 4 #3): the barrier's span as it is this tick; the view moves the barrier's
   mesh with the offset it already has (VT 7).
4. **Side doors** (VD 4 #4): emitters gain a height, a side and a share (VT 2).
5. **Runners** (VD 4 #5): the unit flag, the speed, how they are drawn (a gloom leaning forward with a
   streak: the unit shader's `flags`, no new program).
6. **Plans** (VD 5.1): the eight shapes as one small table and one function of (plan, time ÷ `T`,
   emitter); a unit test that each plan's flow averaged over `T` equals the base flow. Today's two
   bursts and the brute every 8 s leave the generator: a level has them only through its plan (VD 5.2).
7. **`KINDLE`, `COMET`, `HUSH`** (VD 3): rules, bot worth, icons, effects (`KINDLE`'s count-up run up
   the lane is the showpiece: a strip of it), motifs.
8. **Today's generator learns the new gates and plans**: hopper and breathing gates as patterns beside
   Slider; sliding walls in Wall and prize; a plan per level by the calendar's "from level" numbers (VD
   5.1); the boon pool by VD 3's. Bump the versions, bake, read the band table.
9. **The bench scene** gains runners, a hopper, a breathing gate and a kindled lane (VT 11).
10. **Stretch 1: snatchers** (VD 4 #6). **Stretch 2: currents** (VD 4 #7; the bot's line bends through
    them). **Stretch 3: portals** (VD 4 #8; a wisp that comes out of ring B must not be drawn streaking
    across the lane: set its previous position too).

## Exit checks

- [ ] Unit tests: hopper and breathing gate positions and openings at chosen ticks; a sliding wall is
      crossed where it is, not where it started; a side door's glooms enter and turn down; runner speed;
      each plan's average; `KINDLE` at ×5 gives ×6 and stops there, and does not multiply a wisp that
      already passed; `COMET`'s power; `HUSH` stops and restarts the emitters on the tick. Each built
      stretch item has its own.
- [ ] The generator's lints pass on 1,000 seeds with the new patterns: no overlap, the gap rule, a
      reachable multiply gate; a hopper or a breathing gate is never also a slider.
- [ ] The bake finished and every level passes lines a to i; the band table shows no acceptance line
      lost against V1. `PROGRESS.md` says how each plan moved the greedy bot's lowest core and the
      lowest `z` a gloom reached (the point of plans is that the enemy is felt).
- [ ] `--sim variety`: neighbours are less alike than at V1 (numbers recorded).
- [ ] The performance contract holds on the Moto with the grown bench scene; `bench_play` on the
      heaviest level; zero heap allocations.
- [ ] Pictures reviewed: the `elements` picture with every new thing named; a strip a new thing; the
      greyscale sheet (a runner is told from a gloom, a side door from a gate post); the flashing audit
      with `KINDLE` and `COMET`.
- [ ] `scripts\test.ps1`, the flow test, the layout lint and `matrix-android.ps1` on three phones pass.
      The picture check covers a picture with runners and a kindled lane (new unit drawing is where a
      GPU driver bit last time: VT 12 trap 1).

## Owner checkpoint

A soft one: it does not block V3. Send the build and ask after ten levels between 16 and 60: does any
new gate annoy rather than interest (hoppers are the candidate)? Do side doors read as doors? Is
`KINDLE` as good as it should be?

## Not in this phase

Kinds, twists, bosses' scripts, recipes, the calendar, the no-repeat rule, new worlds.

## Notes

- A plan must not change a level's total enemy. If the band table moves when only the plans changed,
  the average test is wrong or the bake's window is not `T`.
- Runner packs come out of the base flow (VD 5.1): 14 runners cost 14 power of that second's flow.
- `HUSH` is the one boon that can make the contest's edge (GD 5.9) easier to stand on. Watch its effect
  on `podless` in the band table; it may need to be rarer than its "from level" suggests.
- A stretch item that is not finished by the phase's end is removed, not left half in: its calendar slot
  goes to a hand-made level that combines two things already in (VD 10.1).

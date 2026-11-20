# P2 — Core gameplay

Size L. Needs P1. Ends with the most important owner checkpoint of the project.

**Goal.** The real game for levels 1 to 5 on a phone: hold to pour, drag to aim, gates multiply, the
flood meets an enemy crowd sized to resist it, the spire breaks or the core falls. The simulation is
deterministic and allocation-free, bots can play it, the bake's measuring steps size the enemy and the
spire of each level, tests pin the arithmetic, and the owner has played it and said whether it is fun.

## Read first

`GAME_DESIGN.md` 2 to 8 (the rules, the numbers, levels 1–5, the bake, the bots). `TECH_SPEC.md` 3.2,
3.4, 4 (all), 7.1, 7.2 and 8.4. Run `docs/concept/flux_model.py` once and read its output beside
GAME_DESIGN 5.9: it is the planner's evidence for the enemy rule, and this phase replaces it.

## Tasks

1. **Simulation skeleton** (TECH_SPEC 4.1–4.4): the unit arrays, the grid, the tick in the order of
   GAME_DESIGN 5.13 with only firing (the one-shot counter of 5.2), movement and the lane's walls
   filled in. The view builds its instance buffer from the real arrays; `play` no longer uses P1's
   crowd script (keep the script only for the exact-layout pictures).
2. **Input** (TECH_SPEC 3.4, GAME_DESIGN 4): drag to launcher through the camera's scale; hold to
   fire; the oldest-finger rule; mouse and keys on the desktop. A level waits for its first touch
   (TECH_SPEC 7.1).
3. **Multiply gates:** the pass test, clones, the fan, gate bits, cap overflow into power, the
   aggregated events; the view's punch, flash and sparks.
4. **Spire:** the funnel, damage, the number, the crack stages; a minimal win (freeze, a line of text,
   tap for the next level).
5. **The enemy** (GAME_DESIGN 5.9): emitters, the base flow and bursts, glooms, homing, the pressure
   ramp; the clash; separation; the core and its pips; a minimal lose (text, tap to retry). The running
   totals of TECH_SPEC 4.6.
6. **What levels 4 and 5 need:** ÷2 gates with their carry, sliding gates, barriers, bumpers, a
   health-triggered burst. (Snuffers and brutes belong to P5. `ENGAGE` stays off and unbuilt unless the
   owner's play asks for it.)
7. **Levels 1 to 5** laid out in `levels_intro.cpp` (GAME_DESIGN 7.2); `levelFor`; `--level`.
8. **Bots** (greedy, naive, sweep, idle) and `--bot`; the headless commands `determinism`, `run`,
   `table`, `soak` (TECH_SPEC 4.8).
9. **The bake's core** (GAME_DESIGN 7.4, steps 2 to 5): measure `P0`, bisect the rate scale until the
   enemy cancels its `fight` share, size the spire, run the acceptance lines. Run it on levels 2 to 5
   (`--bake-levels --from 2 --to 5`) and commit the table. The generator, seeds and the full run come
   in P5 and reuse this code unchanged.
10. **Tests** (TECH_SPEC 8.4): gate arithmetic with and without cap overflow, division with its carry,
    clash, barrier, bumper, zero allocations in a tick, determinism, and the acceptance tables of
    GAME_DESIGN 7.4 and 8 for levels 1 to 5.
11. **A debug read-out** (development builds): frame rate, unit counts, tide, tick time, power produced
    and cancelled; `--set debugReadout=1` turns it on, and a three-finger tap toggles it on a device.
12. **Make levels 2 to 5 acceptable.** Where the bake reports a hand-made level, change its layout
    (not the rules) and bake again; log the final layouts and numbers.
13. **Strips**: `gate_strip`, `clash_strip`, `core_hit_strip`.
14. **Devices:** `bench_play --level 5 --bot greedy` on the Moto and the tablet; move `bench_flood` to
    the real `World` in its bench rig (TECH_SPEC 4.5, 8.1); print the determinism hash of level 5 on
    the Moto and compare it with the PC's (TECH_SPEC 4.2).

## Exit checks

- [ ] `scripts\test.ps1` exits 0, including determinism (levels 1 to 5, 60 s each, two runs equal on
      every tick) and the acceptance tables.
- [ ] `glowtide_headless --sim soak --from 1 --to 5 --bot greedy` and the same with `--bot sweep` end
      clean with the debug assertions on.
- [ ] Power is conserved in the tests: across a multiply gate the tide is multiplied by exactly `k`
      with the cap forced low and with it high; across a divide gate exactly one `k`-th of a mixed
      stream's power gets through.
- [ ] On every level from 2 to 5 the bake's report shows the cancelled share within 5 points of the
      level's `fight`.
- [ ] The four findings of GAME_DESIGN 5.9 ("What this contest is like") were tested with the real
      simulation on level 5's layout and on a three-row ×30 layout (the cases of
      `docs/concept/flux_model.py`), and `PROGRESS.md` says for each whether it holds. Where one does
      not, GAME_DESIGN is corrected before the owner plays.
- [ ] Tick time: on the PC, level 5 with the greedy bot, P95 under 0.5 ms; on the Moto, `bench_play
      --level 5` reports a simulation P95 of at most 2.5 ms.
- [ ] `bench_flood` in the bench rig is inside the contract on the Moto with no regression against P1
      (TECH_SPEC 8.3); zero heap allocations per tick and per frame.
- [ ] The strips agree with the rules by count, checked from the simulation's log for the same frames
      (for example: three times as many wisps leave the ×3 gate as entered it).
- [ ] Input: the launcher moves in the frame its motion event is read (a log check), and a press with
      a stored shot emits a wisp in the next tick.
- [ ] The Moto's hash for level 5 is recorded beside the PC's, equal or not.

## Owner checkpoint (do not start P3 or P5 before this is answered)

Hand over the build on the Moto. Five minutes on levels 1 to 5, played by touch from launch to win or
lose and on to the next level. Ask exactly these:

1. Is pouring and sweeping the stream fun?
2. The drag: too slow, right, or too twitchy? (Three installs with `--set dragGain=1.0`, `1.15`, `1.4`
   are one command each.)
3. Does the flood build fast enough? (Fire rate, fan width, wisp speed.)
4. The enemy: is there enough threat? When they lost, did it feel fair or sudden? (`fight`, bursts,
   the pressure ramp; and the levers of GAME_DESIGN 5.12, which are not built: describe them and ask.)
5. Levels 4 and 5: too easy, right, too hard?
6. Was anything confusing?

Change the numbers they ask for, bake again, rebuild, and let them try again. If the answer to
question 1 stays "no" after two rounds, stop and discuss the design with the owner before building
anything on top.

## Not in this phase

Real screens, saving and haptics (P3); sound (P4); snuffers, brutes, the generator and the full bake
(P5); the win and lose spectacle (P6).

## Notes

- Write the rules as plain loops over arrays in the tick order. No component system, no inheritance,
  no event bus: the event array of TECH_SPEC 4.6 is the only channel to the view.
- The bots are players, not cheats: they produce the same `Input` a thumb does and see only what is
  on the lane.
- The bake is how a level gets its numbers. Do not type spire health or gloom rates into a level by
  hand, not even for a quick test: use `--set` for experiments and the bake for what is committed.
- When a test and GAME_DESIGN disagree about a rule, GAME_DESIGN wins unless the owner's play changed
  it; then update GAME_DESIGN in the same change.

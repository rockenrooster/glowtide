# P5 — Content

Size L. Needs P2 (and P3 for the owner's playtest). Can overlap with P4.

**Goal.** Every lane element of GAME_DESIGN 5, the level generator, three hundred levels whose enemy
and spire were sized and checked by bots, five worlds and boss levels. The owner plays twenty of them.

## Read first

`GAME_DESIGN.md` 5.6 to 5.9, 7 (all), 8, 9.2 and 9.3. `TECH_SPEC.md` 4.7, 4.8 and 8.1.

## Tasks

1. **The remaining elements**, in the simulation and the view: snuffers with their patrol line; brutes
   (the `brute` mark, the number, the minimum size); ÷3; crossing sliders; sliding emitters; the two
   time bursts. Update the `elements` picture; add a unit test per rule.
2. **The generator** (GAME_DESIGN 7.3): rows, the pattern library with its weights, the factor rules,
   the enemy, the world. Plus the invariants and overlap tests over 1,000 seeds (TECH_SPEC 8.4).
3. **The full bake** (GAME_DESIGN 7.4): P2 built steps 2 to 5; add the seed loop for generated levels
   and the report. Run it for levels 6 to 300 with at most four processes, commit
   `src/sim/levels_baked.h`, keep the report.
4. **Worlds 2 to 5** (GAME_DESIGN 9.2) with the palette lint, the ten-level cycle, and **boss levels**
   (GAME_DESIGN 7.5): the broader spire with its crown, the two health-triggered bursts with brutes,
   the longer target time, all inside the bake's runs.
5. **After level 300** the game repeats levels 101 to 300.
6. **Level sheets:** a picture mode that draws a level from straight above as a thumbnail with its
   labels; contact sheets of sixty levels (every fifth) in `captures\levels\`.
7. **The difficulty report as tables** (no plotting library is installed): per band of ten levels, the
   greedy bot's median time and core, `fight` and the cancelled share reached, the naive bot's loss
   rate, the lowest `z` a gloom reached, the highest tide, the highest unit counts, seeds tried.
8. **Performance:** find the baked level with the highest unit counts and run `bench_play` on it on
   the Moto (this is the clash's benchmark); measure level load time.
9. **FRENZY** (GAME_DESIGN 5.12) only if the owner has asked for it.

## Exit checks

- [ ] The generator's invariants and overlap tests pass over 1,000 seeds.
- [ ] The bake finished: every level from 6 to 300 is accepted by every line of GAME_DESIGN 7.4,
      step 5, within eight seeds. The report lists seeds tried and the rate scale per level. Levels 2
      to 5 are baked again with the final rules and still pass.
- [ ] The band table shows a ramp and a fight: the naive bot's loss rate does not fall from one band
      to the next by more than ten points, and every level's cancelled share is within 5 points of its
      `fight`.
- [ ] Determinism holds on ten baked levels; `glowtide_headless --sim soak --from 1 --to 300` ends
      clean with the debug assertions on.
- [ ] You reviewed the sixty thumbnails: labels legible, rows evenly spaced, nothing overlapping.
- [ ] The heaviest level is inside the contract on the Moto; no regression in `bench_flood`
      (TECH_SPEC 8.3).
- [ ] `world_1` to `world_5`: the palette lint and the label check pass in each.
- [ ] Loading a level (generate, build meshes) takes at most 50 ms on the Moto.
- [ ] `scripts\test.ps1` exits 0, with the new rule tests in it.

## Owner checkpoint

A build with the development level stepper. Ask them to play levels 1 to 20, then jump to 50, 100 and
200. Ask:

1. Does it get harder at the right pace?
2. Do levels feel different from each other, or samey?
3. Are levels the right length?
4. Was any level unfair or dull? (Its number is enough: the seed reproduces it.)
5. The five worlds: any to change?
6. Do they want FRENZY tried?

A level they call unfair is evidence about the acceptance rule, not just about that level: look at its
row in the report before changing its seed.

## Not in this phase

Mechanics that are not in GAME_DESIGN 5, any meta game, an endless mode, hand-made levels beyond 1–5.

## Notes

- The bake is the balance. Resist tuning single levels by hand; change the generator's tables, the
  `fight` curve or the acceptance rule and bake again.
- The bake is slow on purpose (a dozen or more runs a level). Run it in the background, at most four
  processes, and keep working; it needs no window and no device.
- If the greedy bot wins everything easily while the owner finds levels hard (or the reverse), the bot
  is the wrong yardstick: adjust its reaction time and aim noise until its results track the owner's,
  then bake again.

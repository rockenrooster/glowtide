# V4 — Recipes, calendar, no repeats

Size L. Needs V3 and its checkpoint. **This phase is the gate for variety.**

**Goal.** Levels are composed, scheduled and provably unlike their neighbours: twenty-four or more
layout recipes, a calendar that brings something new every few levels for the first hundred and gives
each world a rhythm, hand-made introductions and twelve set pieces, the no-repeat rule inside the bake,
and 600 baked levels. The owner plays ninety of them in a row at three places.

## Read first

`VARIETY_DESIGN.md` 9, 10, 11, 12, 16. `VARIETY_TECH.md` 5 (the three passes), 6 (all), 10.
`VARIETY_PLAN.md` 5 (the variety contract), 8 (risk 3). `GAME_DESIGN.md` 7.2, 7.3 (the lints every
layout must still pass).

## Tasks

1. **The recipe interface** (`recipes.cpp`): a recipe is a function (level number, random stream,
   hardness, what the calendar allows) that fills a `LevelDef`, and a row in a table: its name, its
   one-sentence idea, the kinds it serves, the first level it may appear, whether it has a mirrored
   twin. "Terraces" (today's generator) is the first row.
2. **Lane lengths** (VD 9.1): Sprint, Standard, Long; the target time by class and kind; five rows on a
   Long lane (today's break at `z` > 32 goes).
3. **The twenty recipes of VD 9.2**, one at a time. For each: write it; render twenty seeds from above
   (`level_top`); look at all twenty; run the lints on 200 seeds; run `greedy` on the twenty and check
   its best line is not the same place on more than half; then add its row. A recipe that fails these
   is changed until it passes or is dropped. Then **at least four of your own**, each with an idea that
   names what the player does differently (VD 9.2's last paragraph).
4. **Geode variants and placement, complete** (VD 2.2, 9.3): choice pairs, vaults, runner geodes; the
   count, budget, timing, detour and pool rules as lints.
5. **The calendar** (`calendar.cpp`; VD 10): for a level number it gives the world, the place in the
   world and its role, the hardness `h`, the kind (festivals on 5, bosses on 0 in rotation, the other
   kinds by VD 6's shares and never twice in a row), the length class, the recipes, plans, twists and
   boons allowed (each from its introduction level), and whether the level is hand-made. `h` moves
   `fight`, the reliance `m` and the twists (VD 10.2).
6. **Hand-made levels** (`levels_hand.cpp`): every introduction of VD 10.1 that is not yet hand-made
   (the smallest lane that shows the new thing, one hint), and the twelve set pieces at 49, 99 … 599
   (VD 10.3), each with a name for its chip and a one-line idea. List the ideas in `PROGRESS.md`.
7. **The fingerprint, complete** (VT 6): every kind, recipe, plan, twist and element has its code.
   **Tune the weights and thresholds** (VT 6.3): render the twenty most alike pairs the report names and
   look at them; if pairs it calls alike do not look alike, or pairs that look alike score low, move
   the weights. Record the final numbers.
8. **The bake's three passes** (VT 5): layouts in level order under the rule; balance in parallel;
   repair. The report's new columns. **Bake 600 levels.**
9. **After the last level** (VD 16): mirrored replays five worlds on, in `levelForPlay` and
   `levelFor`. A test: a mirrored level's layout is the mirror of its source, and ten `greedy` runs on
   each give the same median time within 5%.
10. **Look at the result.** `review.ps1 -Set levels -From N -Count 30` at 1, 100, 300 and 570: four
    sheets of thirty consecutive levels. Look at every sheet for what the report cannot see: the same
    shape with other numbers, three levels running that all play on one side, a recipe that always
    looks the same. Fix by changing recipes or the calendar, not by loosening the rule.
11. **Band tables** for 600 levels: by level band and by kind, with VD 12.2's lines and step 8.
12. **Checks on the whole set**: determinism on one level of every kind and every recipe (PC, and the
    Moto's hashes equal); the soak on 1 to 600 with `greedy`, `hungry` and `sweep`; `bench_play` on the
    heaviest level of each kind on the Moto.

## Exit checks

- [ ] The catalogue has at least twenty-four recipes; each has its sentence, its twenty thumbnails and
      its bot check recorded; the `recipes` sheet (three seeds of each) is in `captures\`.
- [ ] The generator's lints pass for every recipe on 200 seeds, and for the calendar's 600 levels.
- [ ] **Variety contract:** `--sim variety --from 1 --to 600` exits 0 with the tuned thresholds. Per
      hundred levels: each kind within three points of VD 6's share; every recipe that is allowed there
      appears at least twice and none more than twelve times; no kind but Siege twice in a row; the
      introductions of VD 10.1 come in order, never more than five levels apart.
- [ ] **Balance contract:** all 600 levels pass their lines within eight seeds; every band of ten
      passes step 8. The bake took under 30 minutes.
- [ ] Against V0's baseline, in `PROGRESS.md`: the share of neighbour pairs above each threshold
      (was 100%); levels the greedy bot is hit on (was 0 of 295); the spread of target times (was one
      value from level 30); distinct (kind, recipe, plan) triples (was 1).
- [ ] The four consecutive sheets were looked at thumbnail by thumbnail and what was found and fixed is
      written down.
- [ ] Level 601 is level 101 mirrored, in another world (VD 16); the mirror test passes.
- [ ] The performance contract on the Moto; the heaviest level of each kind inside it; level load
      (generate and build meshes) still under 50 ms on the Moto for the busiest level.
- [ ] `scripts\test.ps1` (with `--sim variety` and the version test in it), the flow test, the layout
      lint, the flashing audit and `matrix-android.ps1` on three phones pass.

## Owner checkpoint (the gate: do not start V5 or V6 before it is answered)

The dev build on their phone, with SETTINGS' level stepper. Ask them to play thirty levels in a row at
three places: 1 to 30, 100 to 130, 300 to 330. Send the four sheets too. Ask:

1. Did it ever feel like a level you had just played? Where? (A number is enough.)
2. Which levels were the best? Which were dull or unfair?
3. Is something new arriving at the right pace in the first hundred?
4. Is the game getting harder at the right pace? Is the last level of a world (the hard one, then the
   boss) the peak it should be?
5. Are levels the right length, now that they differ?
6. The set pieces (49, 99 …): worth having more?

A level they call dull or unfair is evidence about its recipe or about an acceptance line, not only
about that level: look at its row in the report and at its recipe's other levels before changing its
seed.

## Not in this phase

New rules or elements (they are V2's and V3's), new worlds and music (V5), release work (V6).

## Notes

- The rule can only reject; it cannot invent. If pass 1 needs dozens of seeds to find a level unlike
  its neighbours, the catalogue is too small for the calendar: add a recipe or widen one, do not raise
  a threshold.
- "Creative" here means an idea a player could say: "the gate will not stay", "this boon or that one",
  "find the thin wall". A recipe whose sentence is "gates and a wall" is Terraces again.
- A recipe that is fun for the bot may be unreadable for a person: more than seven or eight numbers on
  the lane at once is too many. Count labels on the thumbnails.
- Hardness is relative to the level's neighbours, not a ladder to the sky: by level 100 every rule is
  in, and after that the game varies, it does not escalate (VD 12.3).
- Bake time: pass 2 is the cost. If 600 levels pass 30 minutes, profile the bot runs before cutting
  runs from the acceptance.

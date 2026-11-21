# GLOWTIDE — Progress

Executor: Claude (Opus 5.5) in one session, coding directly, from 2026-11-20. The owner set the goal: "Implement the
plan to completion. No subagents. Use moto and tablet for testing, should also work on 12R. If you have questions,
just do what you think is best and let me know your choices."

## Phase status

| Phase | Status | Notes |
|---|---|---|
| P0 Bootstrap | Done 2026-11-20 | Desktop and Moto pipelines, benchmarks with present timestamps, lifecycle 20/20 |
| P1 Look and performance | Done 2026-11-20 | `bench_flood` inside the contract on the Moto after a 180 s soak; review set `captures/p1/contact.png` |
| P2 Core gameplay | Done 2026-11-20 | Tests, determinism (PC = Moto), acceptance 1-5, strips with count checks, flux findings (GAME_DESIGN 5.9 corrected) |
| P3 Game loop and UI | Done 2026-11-20 | Flow test on PC, Moto and tablet, lint over 43 renders, save, lifecycle, cold start 0.43 s (1.1 s before the P4 synthesis fix), back key on the Moto and the tablet |
| P4 Audio | Done 2026-11-20 | Renders in `captures/p4/` (three ladder timbres through the flood); 10 min of `bench_flood` with sound on the Moto and on the tablet: no late callback on either |
| P5 Content | Done 2026-11-20 | Levels 2-300 baked and accepted (4 min, 4 jobs); band tables in `captures/levels/bands.md`; 60 thumbnails reviewed; heaviest level (87) inside the contract on the Moto |
| P6 Polish and release | Done 2026-11-20 | Strips reviewed, flashing audit, greyscale sheet, 20-minute soak, sanitizers, release APK; device matrix on the Moto, the tablet and the OnePlus 12R: every check passes on all three |

## Variety and content pass (`VARIETY_PLAN.md`)

Executor: Claude (Opus 5.5), coding directly, from 2026-11-20. The owner set the goal again for this plan: "Implement the
plan to completion. No subagents. Use moto and tablet for testing, should also work on 12R. If you have questions, just
do what you think is best and let me know your choices." So the owner checkpoints of the subplans are decided here and
listed under "Choices" below; the owner can reverse any of them.

| Phase | Status | Notes |
|---|---|---|
| V0 Baseline and tools | Done 2026-11-20 (device matrix deferred to V1's runs) | Split of `levels.cpp` proven neutral; version stamps and their test; `--sim variety`; baseline below; consecutive-level sheets; `picture-android.ps1`; stars past level 1,000 not recorded. Also the owner's barrel fix (below) |
| V1 Geodes and boons | Done 2026-11-20 (version 0.2.0) | Geodes, plates, four boons, three lanterns, bake v2; levels 2-300 baked and accepted; step 8 missed (below); the checkpoint decided here (Choices) |
| V2 Lane and enemy | Done 2026-11-20 (phones: below) | Hoppers, breathing gates, sliding walls, side doors, runners, eight plans, `KINDLE`, `COMET`, `HUSH`; levels 2-300 baked again and accepted; stretch items not built |
| V3 Kinds, twists, bosses | Done on the PC 2026-11-20 (phones: waiting) | Holdout, Colossus, Twins, Festival, four bosses, eight twists; every kind's lines pass in the bake; Siege unchanged |

### V0: the baseline (version 0.1.0's table, levels 6 to 300)

`glowtide_headless --sim variety --from 6 --to 300 --pairs` (`captures/variety_baseline.md`): every level is Siege,
"Terraces", Steady, one length class, no boons.

| Levels apart | Pairs | Limit | Above it | Likeness p10 / p50 / p90 | Most alike |
|---|---|---|---|---|---|
| 1-12 | 3,462 | 0.45 | 3,462 (100%) | 0.60 / 0.64 / 0.70 | 251 and 254 (0.87) |
| 13-40 | 7,518 | 0.60 | 5,902 (78.5%) | 0.60 / 0.64 / 0.69 | 74 and 108 (0.88) |
| 41+ | 32,385 | 0.85 | 14 | - | 220 and 284 (0.92) |

Counts: a recipe twice within 6 levels 1,460 times; a (recipe, kind) pair twice within 30 levels 8,120 times; the
longest run of one kind and one recipe is all 295 levels. The rule breaks 18,958 times (exit 1, as expected). The five
most alike neighbours (251/254, 260/262, 8/15, 13/16, 226/236) are in `captures/levels/sheet_levels.png`; the two
"before" sheets of thirty consecutive levels are `captures/levels/sheet_40_69.png` and `sheet_200_229.png`. Looked at
one by one: from level 40 every lane is four rows of one or two gates with the spire's number on top, the same grammar
with other numbers; nothing a player would call a different level.

### V0: what else changed

- **Shots leave the barrel's tip** (the owner, 2026-11-20: "they should spawn at the tip of the barrel, not at turret
  origin", most visible when panning fast). The simulation's muzzle moved from `z` 1.0 (the lantern's base) to 2.55
  (the lens at the barrel's end); a fresh wisp is drawn leaving the lens 1.13 m up and dropping to the lane in an arc
  that lands 4.5 m on (`muzzleLift` in `shaders/prelude.glsl`, the glow pass too); the shot's flash is at the lens.
  The `pan_strip` picture swings the lantern across the lane every half second while it fires
  (`captures/variety/barrel_tip_fix.png`). Rules version 2; levels 2-300 baked again (all accepted, 7 new seeds).
  Level 1's spire went from 60 to 66: shots arrive 0.2 s sooner and greedy won in 5.8 s, under GAME_DESIGN 8's 6 s
  (now 6.1 s). New determinism hashes, levels 1-5: 8ef4ba40c9607ee4, c2ed3023e7b5fbd0, 9a3cdfb9d736a6ee,
  39e10af4e915d968, df6733a3f4f84515 (the Moto's to be compared in V1).
- `levels.cpp` split into `levels_hand.cpp`, `recipes.cpp` ("Terraces"), `calendar.cpp` and `levels.cpp`, with the
  builder helpers in `level_build.h`. Proof: a bake of 2-300 after the split equalled the committed table byte for byte,
  and the report too; the determinism hashes of levels 1-5 did not move.
- `kGeneratorVersion` and `kRulesVersion` (`level.h`) are written into `levels_baked.h`; a unit test fails while they
  differ (seen failing on the bumped rules version before the bake). The bake does not write its date into the table
  (VARIETY_TECH 5 asked for it): a date would make two bakes of one state differ, and git has the date.
- `review.ps1 -Set levels -From N -Count M` and `-Levels a,b,c`; `--sim variety [--from --to --pairs]`.
- **The device picture check** (`scripts/picture-android.ps1`, `tools/picture_compare.py`, in `matrix-android.ps1`):
  the tablet's `concept` against the PC's at the tablet's frame (1200x1920, dp 1.5, insets 30/48 dp): gold 5.07% / 5.07%,
  violet 24.60% / 24.62%, mean difference 0.1. Against the PC's `elements` it fails (gold 5.07% / 2.33%); that picture
  scored a mean difference of 11.6, so the limit is 4, not the 12 first set.
- Stars are not recorded past the save's thousand levels (they were written into the last slot).
- A safety copy of version 0.1.0 is in `D:\Stuff\DevFolder\_backups\Glowtide\2026-11-20_v0.1.0` (no `keys/`, no build
  folders) and the commit is tagged `v0.1.0` locally.

### V1: geodes and boons — what was built and what changed from the design

Built: geodes (plain and shielded) that appear, drift, slide, slip past and open; the four boons `LANTERN`, `QUICKEN`,
`BRIGHT`, `WARD`; up to three lanterns; `podless` and `hungry`; geodes placed in generated levels (VD 9.3) and hand-made
levels 6 (the first geode), 11 (the first shield) and 23 (the first `WARD`); bake v2 for Siege (Pa, Pn, m, R, lines a to
i, step 8 in `tools/band_table.py`); the shell, the badges, the plate, side lanterns, numbers, lights and effects; the
boon tray, a banner queue, icons flying to the tray, `BOONS n/m` on the result line, three hints, two lines of advice;
`tink`, `geode_open`, the four motifs; pictures `geodes`, `geode_strip`, `ward_strip`, `lanterns_3`, `hud_boons` (eight
frames, layout lint), `pan_strip`; the flow test's geode steps; the bench rig's three lanterns and four geodes;
`--sim decisions`; `scripts\simhash-android.ps1`.

Changes from VARIETY_DESIGN, each forced by a measurement (the bake and `--sim decisions` traces):

| Design said | Built | Why |
|---|---|---|
| Geode health a share of R (4-6%, plate 6-9%) | Seconds of the stream that reaches the geode's own column where the stream meets it, counting only the share of that fanned-out stream the geode (1.8 m) or its plate (3 m) is wide enough to catch: plain 2-3 s, plate 3-4 s with 1-1.5 s behind it, `WARD` 1.5-2 s | R is the best line's power. A column off the best line carries a tenth to a fiftieth of it, so a share of R took 7 s of raw fire on level 6 and was out of reach from level 30: greedy never took one. Without the fan share a plate behind three doubling gates took 8 s, not 2.4 |
| Detours 2 m off the best line, appearing at the top of the lane | Off the best line by the stream's fan as well (each multiply gate throws clones about (0.9 + 0.5k) x 0.5 m sideways) plus the geode's radius plus 1 m; where that leaves no room at the top, the geode appears lower down, where the stream is narrower | From about level 30 the flood fans across the whole lane near the top: geodes there were cracked by spill, both bots got the boon, and there was no decision (Pa equal to Pn). With 0.4 m instead of 1 m the spill still opened a third of the detours |
| Shields and slides on any geode | On detours only; slides from level 40, 0.5-0.9 m | A shielded geode riding the best line was a 500-health wall moving down the middle of the stream; greedy kept missing sliding geodes low on the lane |
| Lanterns in a row centred on the aim point | The first lantern at the aim point, the second 0.9 m to its right, the third 0.9 m to its left; the side ones are the lantern at 70% width standing forward so every lens is at the muzzle | The thumb stays on the lantern with the pips, and no lantern jumps when one arrives |
| The bot's worth of a boon: its gain over the time left | The time it saves (the spire falls in rest / (1 + g) instead of rest), with commitment: greedy finishes a geode it started unless another line is three halves better, and gives up when the stream has had 2.5 s there and taken less than a tenth; a line that would not open a geode before it slips past gains nothing from it | Without these greedy dithered between the gates and a geode (spending 218 of 230 health and leaving), refused anything that took longer than its 8 s horizon, and gave up on far geodes before its stream arrived |
| Reliance m to 0.8 by level 66 | m to 0.6 by level 52, plus (h - 0.5) x 0.15 for the level's hardness, at most 0.7 | Sized that far for the boons, the enemy overran a greedy bot that the pressure kept from reaching them; the bake could not size those levels. With hardness moving m by plus or minus 0.15 (VD 10.2) the hard ones (h 0.8 and 1.0) failed every seed |
| The enemy bisected on one run a step | The median of three runs a step; a seed whose bisection misses its target by 8 points is dropped before the acceptance runs | With boons a run snowballs or does not; one run sized spires of a few hundred on levels greedy then won in 10 s |
| Acceptance: all runs, then the lines | In stages, greedy first, podless only when greedy passes, hungry only on the accepted seed; line g forecast from Pa and Pn (12%) before the enemy runs; twelve seeds, not eight, then a repair pass of 24 more for a level still not accepted | The bake took over an hour for 300 levels; most seeds fail line g, and those now cost two runs. The final bake: 12 of 299 levels needed the repair pass (accepted after 13 to 22 seeds) |
| Line h: from level 80, on a level of hardness 0.65 or more, `podless` wins fewer than three of five | `podless` may win three of five there if it is at least 25% slower than greedy | The bake could not size the hard levels that far: an enemy strong enough to beat `podless` three times in five beat greedy too. "Twenty-five percent slower" keeps the boons worth taking there |
| A geode 1.8 m across | Drawn 1.2 times its touch radius | Next to 3 m gates it read as a badge |
| Level 1 spire 60 (version 0.1.0) | 66 | V0's barrel fix |

### V1: measured

**The bake** (generator 5, rules 3; `captures/levels/report.md`): levels 2-300 all accepted, 1,122 s at 4 jobs. Seeds
thrown away, by the line they failed: g 807 (with no enemy greedy did not deliver 1.12 times what `podless` did), a 33,
c 32, h 27, n 23, x 23 (the bisection missed its target), e 19, f 12. The band table (`tools/band_table.py`; columns:
greedy's median time as a share of `T`, its median core, the cancelled share against its target, the geodes greedy
opened of those it was shown, how much slower `podless` is (median), the levels where `hungry` is slower or loses, the
three-geode levels where greedy leaves one, the levels where greedy loses a pip, the levels where a gloom gets past the
first gate row, the seeds tried):

| Levels | Greedy time (T) | Core | Cancelled / target | Geodes opened | Podless slower | Hungry slower or loses | 3+ geodes, one left | Pip lost | Gloom past row 1 | Seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| 2-10 | 0.91 | 10 | 14% / 14% | 5/5 | 22% | 0% | - | 0% | 22% | 15 |
| 11-20 | 0.91 | 10 | 18% / 17% | 10/10 | 32% | 30% | - | 0% | 40% | 26 |
| 21-30 | 0.92 | 10 | 21% / 20% | 23/25 | 55% | 70% | 40% | 0% | 20% | 18 |
| 31-40 | 0.91 | 10 | 24% / 24% | 21/24 | 47% | 80% | 75% | 0% | 50% | 29 |
| 41-50 | 0.93 | 10 | 26% / 26% | 22/24 | 57% | 60% | 50% | 0% | 30% | 24 |
| 51-60 | 0.96 | 10 | 27% / 26% | 22/23 | 34% | 50% | 33% | 0% | 40% | 25 |
| 61-70 | 0.93 | 10 | 26% / 26% | 28/32 | 44% | 80% | 44% | 0% | 10% | 40 |
| 71-80 | 0.93 | 10 | 28% / 26% | 25/29 | 39% | 90% | 57% | 0% | 30% | 40 |
| 81-90 | 0.92 | 10 | 28% / 28% | 22/23 | 33% | 80% | 33% | 0% | 20% | 48 |
| 91-100 | 0.93 | 10 | 27% / 26% | 23/26 | 52% | 60% | 50% | 0% | 60% | 60 |
| 101-110 | 0.95 | 10 | 28% / 28% | 24/26 | 31% | 80% | 33% | 0% | 40% | 37 |
| 111-120 | 0.96 | 10 | 27% / 27% | 24/29 | 40% | 70% | 71% | 0% | 40% | 51 |
| 121-130 | 0.93 | 10 | 27% / 27% | 22/29 | 35% | 100% | 100% | 0% | 30% | 65 |
| 131-140 | 0.93 | 10 | 26% / 26% | 23/28 | 43% | 90% | 71% | 0% | 40% | 41 |
| 141-150 | 0.92 | 10 | 28% / 26% | 27/31 | 37% | 80% | 57% | 0% | 30% | 50 |
| 151-160 | 0.92 | 10 | 27% / 26% | 21/22 | 50% | 80% | 50% | 0% | 40% | 64 |
| 161-170 | 0.93 | 10 | 26% / 26% | 28/30 | 36% | 80% | 25% | 0% | 10% | 38 |
| 171-180 | 0.92 | 10 | 28% / 28% | 27/32 | 38% | 70% | 62% | 0% | 30% | 60 |
| 181-190 | 0.92 | 10 | 28% / 26% | 24/28 | 52% | 90% | 57% | 0% | 40% | 68 |
| 191-200 | 0.93 | 10 | 27% / 27% | 24/27 | 49% | 70% | 60% | 0% | 30% | 52 |
| 201-210 | 0.93 | 10 | 27% / 27% | 26/27 | 51% | 60% | 17% | 0% | 20% | 48 |
| 211-220 | 0.93 | 10 | 26% / 28% | 21/25 | 28% | 70% | 100% | 0% | 40% | 23 |
| 221-230 | 0.94 | 10 | 28% / 27% | 27/28 | 39% | 70% | 17% | 0% | 30% | 35 |
| 231-240 | 0.92 | 10 | 27% / 26% | 30/35 | 41% | 90% | 56% | 0% | 30% | 33 |
| 241-250 | 0.93 | 10 | 28% / 27% | 25/28 | 33% | 60% | 60% | 0% | 20% | 34 |
| 251-260 | 0.93 | 10 | 27% / 27% | 26/29 | 40% | 70% | 43% | 0% | 50% | 52 |
| 261-270 | 0.92 | 10 | 26% / 26% | 27/30 | 48% | 80% | 38% | 0% | 20% | 51 |
| 271-280 | 0.95 | 10 | 28% / 28% | 25/29 | 35% | 70% | 57% | 0% | 40% | 48 |
| 281-290 | 0.95 | 10 | 26% / 26% | 23/26 | 40% | 70% | 60% | 0% | 40% | 50 |
| 291-300 | 0.94 | 10 | 26% / 27% | 26/31 | 40% | 70% | 62% | 0% | 30% | 50 |

So the boons are worth taking (without them the same player is 22% to 57% slower), greedy takes most of them (76% to
100% per band) and `hungry`, who chases every geode, is slower or loses on 60% to 100% of the levels from 21 on: there is
a choice, and taking every geode is the wrong one.

**Step 8 missed, and why** (VARIETY_DESIGN 12.2 step 8 asks, from level 21: greedy loses a pip on 15% to 40% of the
levels, a gloom gets past the first gate row on 40% or more, a three-geode level leaves one geode on half or more):

- **Greedy never loses a pip** (0% in every band). The enemy is sized to cancel its share of the stream (the
  cancelled share is within 2 points of target everywhere), and greedy's defend rule plus the sized flow keep the
  glooms off the core. Raising `fight` to make greedy lose pips is the same lever that let the enemy overrun greedy
  when the bake tried to size hard levels (see the m and line h rows above).
- **A gloom past the first row** on 10% to 60% per band (20 of 28 bands under 40%).
- **One geode left** on three-geode levels: 17% to 100% (10 of 26 bands under half).
- Tried: m to 0.75 and hardness moving m by 0.15 (the bake could not size the hard levels: the enemy beat greedy),
  detours further from the flood (that moved the "one left" share, not the pips). Not tried: a higher `fight`. The
  levers VARIETY_DESIGN sends for exactly this are V2's (the plans: Pulse, Late surge, Early rush and Brute line put the
  enemy's flow where the stream is not) and V3's twists; V2's band table says how far they move it.

**The variety report** (`--sim variety --from 6 --to 300`): the likeness of neighbours did not fall against V0's
baseline: levels 1-12 apart p10 / p50 / p90 0.59 / 0.66 / 0.73 (V0 0.60 / 0.64 / 0.70); 13-40 apart 84.1% above 0.60
(V0 78.5%); a boon list twice within 8 levels 134 times. The boon term (J) went from 1 for every pair (no boons) to the
Jaccard index of two lists drawn from the same four boons, and a geode's path down the lane fills map cells that the
same detour columns fill in most levels. The rule still breaks everywhere, as expected with one recipe and one kind
(V4's job).

**Other checks** (PC unless named): unit tests 83 checks, 0 failed; determinism (two runs equal on every tick) on levels
1-5 and 6, 11, 23, 47; the Moto's hashes equal the PC's on all nine (`scripts\simhash-android.ps1`): levels 1-5
8ef4ba40c9607ee4, c2ed3023e7b5fbd0, 9a3cdfb9d736a6ee, 9f9402a54227d0c5, df6733a3f4f84515; 6 bae9e87ae7c229b0, 11
bb65b0d61ef54ad1, 23 067fccc04f70bbce, 47 9dcc8b6e48aac4cb. Acceptance 1-5 passes. Soak 1-300 with `greedy`, `hungry`
and `sweep` (invariants every tick): 900 runs, 0 failed. Flashing audit with the geode and `WARD` strips, with and
without REDUCE EFFECTS: at most 2 swings in a second (limit 3). Audio (`review.ps1 -Set p4`): the flood -14.6 LUFS,
-1.8 dBTP; the `geodes` scene (four geodes, greedy) -17.9 LUFS, -3.9 dBTP; the bell timbre's flood is -13.8 LUFS, 0.2
over GD 12's -14 (the kalimba, which the game uses, -14.6; the glass -14.7): if the owner picks the bell, lower its
gain by 1 dB.

**V1 on the phones** (the dev build as `dev.glowtide.game.dev`): the Moto's matrix (`captures/matrix/ZY22BXMJG3.md`):
flow test 48/48, determinism equal on nine levels, the `concept` and `geodes` picture checks (gold 6.39% / 6.41% and
2.03% / 2.03%, mean difference 0.1), save through a force-stop, cold start 432 ms, lifecycle 20/20, `bench_flood` 0.62%
late, worst 34.5 ms, CPU p95 4.40 ms (P6 4.26 ms: +3.3%, limit 10%), tick p95 1.55 ms, 0 allocations. The back key and
`bench_play` rows failed inside the matrix with no output (the scripts stopped before their first line, as when the
phone is briefly busy) and passed when run again alone: back key PASS; `bench_play` level 87 0.41% late, CPU p95
3.44 ms. Level 87 is no longer the heaviest level: by the bake's peak units it is 298 (3,336 units at the caps), and
`bench_play` there: 0.47% late, worst 35.0 ms, CPU p95 5.09 ms, **tick p95 2.65 ms, over P5's 2.5 ms budget** (frames
are fine: the tick runs inside the frame's budget; to look at in V2: the clash step and greedy's scoring on a full
lane). The tablet was in use by someone else all through V1's runs (another app in front); its checks wait for it.
The 12R's wireless debugging is off (`adb` lists it offline).

### V2: the lane and the enemy — what was built and what changed from the design

Built: hopper gates (two places, 1.5-2.5 s at each, a 0.25 s smoothstep jump; a faint ghost frame where it lands
next), breathing gates (the opening swings 60% to 120% of the half-width over 3-5 s; a wisp passes if it is inside the
opening of that tick), sliding walls, side doors (an emitter on a rail at a height, with a share of the flow: its glooms
step in at 1.5 m/s for a second, fading in, then walk down), runners (5 m/s), the eight plans, `KINDLE`, `COMET` and
`HUSH` (rules, the bot's worth, badges, chips, effects, motifs; `KINDLE`'s labels count up row by row, 80 ms apart;
`COMET` a tier-3 wisp with a tail from each lantern; `HUSH` dims the emitters' glow, plays the music's pad alone and
drains its chip); the generator's hopper and breathing patterns, sliding walls in Wall-and-prize rows from level 46, a
plan for every generated level, side doors for Flank; hand-made levels 16 (the first hopper, with its hint) and 38 (the
first breathing gates); the bench scene's runners, hopper, breathing gate and kindled lane; pictures `elements` (every
new thing), `kindle_strip`, `comet_strip`; a plan column in the bake report and a by-plan table in `band_table.py`.
Not built: the three stretch items (snatchers, currents, portals): their calendar slots (76, 83, 91) go to V4's
hand-made levels that combine two things already in (VARIETY_DESIGN 10.1).

| Design said | Built | Why |
|---|---|---|
| Pulse: 2.5 s at 1.8 times, 2.5 s at 0.2 | The same shape, its period stretched so a whole number of pulses fits `T` (4.6 to 5.4 s) | So every plan's flow averages the base flow over `T` exactly (V2's unit test; with a fixed 5 s a level of 24 s averaged 8% over) |
| Early rush: 1.6 for 8 s, then 0.85 | Then (`T` - 12.8) / (`T` - 8): 0.82 to 0.88 | The same reason |
| Brute line: half the flow as brutes, one every 4 s | Each brute 4 s of half the flow, with no 16-power minimum | A minimum would add enemy on low-flow levels |
| Today's bursts and brutes leave every level but through the plan (VD 5.2) | The two time bursts and the brute every 8 s are gone from every level; the bosses keep their two health bursts with brutes until V3's scripts | V3 replaces bursts with twists |
| A side door "a dark arch on the rail" | The arch stands across the rail with its opening toward the lantern, a rose glow in it | Side-on to the camera it read as a dark slab; facing the camera it reads as a doorway (greyscale sheet) |
| The introduction levels show their new thing | Levels 16 and 38 have no geode | Hand-placed geodes there sat in some stream's spill: `podless` cracked them as often as `greedy`, so line g could not pass |
| `KINDLE` at 36, `COMET` at 52, `HUSH` at 63 (VD 10.1) | On those generated levels the first geode holds the new boon; later a level offers `COMET` (weight 1.2) and `HUSH` (0.7) at most once each, never as the first geode up to level 60, never after 75% of `T` | The introduction without a hand-made level; `HUSH` rarer as V2's notes ask |
| (A bug, found through `KINDLE`) | Past the unit cap a shot's power joins the newest unpassed wisp near its lantern anywhere in the array, not only among the last 64 | Clones made at the gates since the last shot pushed it out of the 64: the shot's power was lost |
| `COMET` anywhere a geode goes | Level 52's `COMET` is its second geode, on the best line | A comet fires from where the lanterns are when it opens: opened on a detour it climbs the detour's weak line, so chasing it made greedy slower than `podless` (level 52 failed all 36 seeds; on the line it passed on the first). Elsewhere `COMET` stays where the generator puts it (V4's recipes place geodes: put comets on the line there) |
| The boon tray: one row, up to four chips | Two chips a row, the third and fourth above | Four chips in a row met the tide counter on narrow frames (the layout lint) |

### V2: measured

**The bake** (generator 7, rules 4): levels 2-300 all accepted, 1,262 s at 4 jobs, then level 52 again (above). 27
levels needed the repair pass (13 to 27 seeds). Seeds thrown away by line: g 999, x 90, a 60, c 44, h 34, f 19, e 14,
n 14, i 1. The band table:

| Levels | Greedy time (T) | Core | Cancelled / target | Geodes opened | Podless slower | Hungry slower or loses | 3+ geodes, one left | Pip lost | Gloom past row 1 | Seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| 2-10 | 0.90 | 10 | 11% / 14% | 5/5 | 29% | 20% | - | 0% | 22% | 10 |
| 11-20 | 0.92 | 10 | 16% / 16% | 9/9 | 53% | 22% | - | 0% | 30% | 28 |
| 21-30 | 0.92 | 10 | 20% / 20% | 20/23 | 34% | 60% | 100% | 0% | 50% | 37 |
| 31-40 | 0.92 | 10 | 24% / 24% | 18/21 | 36% | 67% | 100% | 0% | 50% | 33 |
| 41-50 | 0.91 | 10 | 26% / 26% | 22/23 | 25% | 80% | 33% | 0% | 20% | 45 |
| 51-60 | 0.93 | 10 | 26% / 26% | 20/24 | 42% | 50% | 100% | 0% | 30% | 31 |
| 61-70 | 0.95 | 10 | 26% / 26% | 26/28 | 31% | 50% | 40% | 0% | 20% | 63 |
| 71-80 | 0.93 | 10 | 24% / 26% | 26/27 | 67% | 80% | 20% | 0% | 20% | 86 |
| 81-90 | 0.94 | 10 | 27% / 27% | 29/30 | 33% | 80% | 12% | 0% | 30% | 48 |
| 91-100 | 0.94 | 10 | 25% / 26% | 27/35 | 42% | 90% | 78% | 0% | 30% | 54 |
| 101-110 | 0.92 | 10 | 24% / 26% | 30/34 | 65% | 90% | 44% | 0% | 50% | 52 |
| 111-120 | 0.93 | 10 | 26% / 26% | 28/30 | 30% | 100% | 29% | 0% | 50% | 60 |
| 121-130 | 0.92 | 10 | 24% / 26% | 23/26 | 60% | 80% | 60% | 0% | 20% | 29 |
| 131-140 | 0.93 | 10 | 25% / 27% | 28/30 | 60% | 80% | 14% | 0% | 20% | 36 |
| 141-150 | 0.93 | 10 | 25% / 26% | 23/27 | 63% | 80% | 67% | 0% | 40% | 74 |
| 151-160 | 0.93 | 10 | 25% / 27% | 25/28 | 32% | 70% | 60% | 0% | 40% | 57 |
| 161-170 | 0.94 | 10 | 26% / 28% | 26/26 | 31% | 60% | 0% | 0% | 40% | 45 |
| 171-180 | 0.94 | 10 | 26% / 26% | 24/28 | 60% | 80% | 57% | 0% | 70% | 42 |
| 181-190 | 0.92 | 10 | 25% / 26% | 25/28 | 55% | 70% | 50% | 0% | 30% | 58 |
| 191-200 | 0.91 | 10 | 24% / 27% | 27/30 | 41% | 80% | 38% | 0% | 10% | 52 |
| 201-210 | 0.93 | 10 | 24% / 27% | 27/28 | 41% | 80% | 17% | 0% | 20% | 42 |
| 211-220 | 0.93 | 10 | 25% / 26% | 26/29 | 37% | 70% | 50% | 0% | 30% | 58 |
| 221-230 | 0.92 | 10 | 26% / 28% | 25/26 | 42% | 60% | 17% | 0% | 30% | 32 |
| 231-240 | 0.94 | 10 | 28% / 28% | 30/33 | 31% | 80% | 33% | 0% | 30% | 54 |
| 241-250 | 0.94 | 10 | 26% / 28% | 26/30 | 42% | 90% | 50% | 0% | 40% | 81 |
| 251-260 | 0.92 | 10 | 25% / 27% | 27/29 | 25% | 60% | 25% | 0% | 20% | 81 |
| 261-270 | 0.93 | 10 | 25% / 27% | 27/30 | 37% | 60% | 50% | 0% | 30% | 52 |
| 271-280 | 0.93 | 10 | 24% / 26% | 25/27 | 81% | 70% | 50% | 0% | 40% | 78 |
| 281-290 | 0.95 | 10 | 27% / 27% | 26/27 | 34% | 40% | 20% | 0% | 20% | 44 |
| 291-300 | 0.94 | 10 | 25% / 27% | 26/28 | 38% | 80% | 33% | 0% | 20% | 76 |

No acceptance line was lost against V1 (every level passes a to i). **How each plan moved the enemy** (levels 21 on;
the lowest z a gloom reached in greedy's median run; the share of levels where a gloom got past the first gate row in
half of greedy's runs or more):

| Plan | Levels | Greedy's lowest median core | Lowest gloom z, median | Gloom past row 1 |
|---|---|---|---|---|
| Flank | 19 | 10 | 7.9 | 68% |
| Early rush | 27 | 10 | 10.7 | 44% |
| Pincer | 39 | 10 | 14.2 | 38% |
| Pulse | 39 | 10 | 15.3 | 26% |
| Brute line | 29 | 10 | 16.3 | 10% |
| Steady | 53 | 10 | 17.3 | 36% |
| Late surge | 43 | 10 | 22.2 | 16% |
| Runner packs | 31 | 10 | 22.5 | 35% |

The side doors and the early rush bring the enemy well down the lane (median 7.9 m and 10.7 m against Steady's 17.3);
Late surge and Runner packs the least (their extra comes late, or as runners the stream eats). None of them costs greedy
a pip: its median core is 10 on every level, so step 8's pip line still misses everywhere (0% against 15-40%; V3's
twists and V4's hardness are the next levers). Step 8's other lines: `hungry` slower or losing on 40-100% of a band's
levels, one geode left on 0-100% of three-geode levels, a gloom past the first row on 10-70%.

**The variety report:** neighbours are less alike than at V1: levels 1-12 apart p10 / p50 / p90 0.49 / 0.55 / 0.65 (V1
0.59 / 0.66 / 0.73), 97.5% still above 0.45; 13-40 apart 23.8% above 0.60 (V1 84.1%); none above 0.85 at 41 or more; a
boon list twice within 8 levels 88 times (V1 134). Plans per hundred levels (6-105): Steady 28, Runner packs 16, Pulse
12, Pincer 11, Brute line 11, Late surge 10, Flank 7, Early rush 5; boons offered: LANTERN 49, QUICKEN 57, BRIGHT 54,
WARD 41, KINDLE 13, COMET 20, HUSH 9.

**Other checks** (PC): unit tests 97 checks, 0 failed (new: the hopper's places at chosen times and the world's hopper
on them, the breathing gate's opening and a wisp missing the narrow opening and passing the wide one, a sliding wall
hit where it is, a side door's gloom stepping in and turning down, runner speed, every plan's shape averaging 1 over
four `T`s and every plan sending the level's total over `T` within 6%, `KINDLE` x5 to x6 and x6 staying x6 and a wisp
already through untouched, `COMET`'s power a lantern, `HUSH` six quiet seconds then the flow on the tick it ends, three
lanterns at their fastest and heaviest through a kindled lane under the cap, no allocation on levels 16, 28, 30 and 38,
the generator's lints over 1,000 seeds with hoppers, breathing gates, sliding walls, side doors and plans). Determinism
on levels 1-5 and 6, 11, 23, 47 (every hash moved: the barrier arrays grew to 16). Acceptance 1-5. Soak 1-300 with
three bots: 900 runs, 0 failed. Flashing audit with the `KINDLE` and `COMET` strips added: pass, with and without
REDUCE EFFECTS. Layout lint over the eight frames with a full tray of four chips: pass. Greyscale: a side door reads
as a doorway (a lit opening in a dark arch), a runner by its streak (`captures/v2/`). Greedy's z-ordered walk of the
lane is built once a tick instead of once a scored line (42 lines a decision): the same results (every determinism
hash equal before and after).

### V3: kinds, twists, bosses — what was built and what changed from the design

Built: the five goals (VD 6) in the data and the rules: Siege as before (levels 1 to 5 hash as they did), Holdout (a dark
gate instead of the spire, won at dawn with the core above zero, the enemy growing 0.6 to 1.4 times across the night,
the backdrop brightening), Colossus (a body with the spire's health that steps out at 3 s, walks to the lantern by 1.25 T
swaying 1.5 m, absorbs every wisp that touches it, and loses the level at z 3; drawn by the unit shader at 1.8 m with a
crown), Twins (two spires at x -2.6 and +2.6 with a range each, damage by side, the funnel toward the nearer living
spire, an emitter each that stops with its spire, a fallen spire's side wasted), Festival (no enemy, 10 to 14 small
geodes over 20 s, a paper lantern in gold where the spire stands, cannot be lost, stars by geodes opened); bosses (the
four scripts as twists at 66% and 33%: THE OLD SPIRE, THE WARDEN with WARD WALL, THE TEMPEST with STAMPEDE and ECLIPSE,
THE HYDRA with CUBs; the name as the goal banner and under the chip on HOME, notches on the goal bar). Twists replace
bursts (SURGE and BRUTE GUARD are the old bursts; GEODE RAIN with runner geodes, ECLIPSE, STAMPEDE, SECOND WIND,
GOLDEN GATE drawn gold, WARD WALL, CUB), each with a banner and a cue. The schedule of V3 (task 8). The goal bar, the
goal banner, the result titles (CLEARED!, DAWN!, FESTIVAL!) and stars by kind. Hand-made introductions 15, 21, 31, 41
with their hints. Bots by kind (the colossus where the stream meets it, the living twin's side, Holdout's defence over
60% of the lane, a festival's geodes in turn). The bake by kind, its report's kind column and a by-kind table. Pictures
`kind_*`, `boss_*`, `twist_*`, `colossus_strip`, `colossus_fall_strip`, `dawn_strip`, `hud_*`; the flow test's kind
steps; a level of each kind in the determinism checks (PC and phone).

| Design said | Built | Why |
|---|---|---|
| Holdout: bisect the flow until greedy's median core at dawn is 7 to 9 | The strongest enemy greedy holds with a median core of 7 or more (five runs a bisection step), then the flow stepped down until greedy wins nine of ten | Near the edge a holdout is all or nothing: a step of the flow turned a median core of 10 into a collapse, so a band of 7 to 9 was not reachable; greedy's median core is 10 on most holdouts (lowest 7), and the first lost pips of the whole game are there (10% of the 21-30 band) |
| Holdout's line: `podless` loses | `podless` loses, or keeps fewer pips than greedy, on holdouts with a lasting boon | As above: the core is the measure, and "loses" alone threw away every holdout where both held |
| A goal resized (line c) freely | Once, by at most 20% (the colossus 6%) | The colossus's time is steep in its health (the nearer it gets the weaker the stream): a 33% step turned a 24 s win into ten losses |
| Lines g, h, i on every level with a geode | On the level's own geodes: SECOND WIND's `WARD` (from 23 on every level) comes only when the core falls to 3 | Otherwise every introduction level without geodes failed lines h and i |
| THE OLD SPIRE: SURGE and BRUTE GUARD | A surge with two brutes at 66% (4 s), two brutes at 33% | Today's boss, split into the two twists |
| Every level from 43 has one twist; SECOND WIND from 23 | As designed, the twist drawn from SURGE, BRUTE GUARD, GEODE RAIN, ECLIPSE, STAMPEDE and GOLDEN GATE by the seed | GATE STORM (stretch) not built |
| Introductions laid out by hand | 15, 21, 31, 41 by hand with no geode of their own | One new thing each, as 16 and 38 |
| The festival's three stars: greedy's median geodes opened, less one | As designed; naive must not earn them | — |

### V3: measured

**The bake** (generator 8, rules 5): levels 2-300 accepted, 1,894 s at 4 jobs (V2: 1,262 s; the holdout bisection's five
runs a step and 33 levels in the repair pass, 13 to 27 seeds). Seeds thrown away by line: g 968, x 173, a 75, f 38,
h 35, c 33, n 29, e 6, i 1. By kind (`captures/levels/bands.md`):

| Kind | Levels | Greedy time (T), median and range | Core, median and lowest | Cancelled / target | Pip lost |
|---|---|---|---|---|---|
| Siege | 178 | 0.93 (0.76-1.09) | 10 (10) | 25% / 26% | 0% |
| Festival | 29 | 1.00 (its 20 s) | - | - | - |
| Holdout | 20 | 1.00 (dawn) | 10 (7) | 44% / - | 10% |
| Colossus | 20 | 0.79 (0.73-0.96) | 10 (10) | 26% / 28% | 0% |
| Twins | 18 | 1.00 (0.85-1.17) | 10 (10) | 26% / 27% | 0% |
| THE OLD SPIRE | 8 | 0.96 (0.91-0.99) | 10 (10) | 30% / 28% | 0% |
| THE WARDEN | 8 | 0.95 (0.90-1.02) | 10 (10) | 26% / 28% | 0% |
| THE TEMPEST | 7 | 0.92 (0.90-1.02) | 10 (10) | 27% / 28% | 0% |
| THE HYDRA | 7 | 0.97 (0.87-0.98) | 10 (10) | 24% / 27% | 0% |

Every level passes its kind's lines (Colossus and Twins inside 0.7 to 1.2 `T`; every festival's three stars earned by
greedy in nine runs of ten and not by naive). Step 8 still misses where it did (greedy's pips: 10% in two bands, 0%
elsewhere). Siege's band numbers match V2's within the bake's noise.

**The variety report:** neighbours 1-12 apart p10 / p50 / p90 0.33 / 0.44 / 0.64 (V2 0.49 / 0.55 / 0.65), 47.6% above
0.45 (V2 97.5%); 13-40 apart 17.2% above 0.60 (V2 23.8%). Kinds per hundred levels: Holdout 7, 7, 6; Colossus 7, 6, 7;
Twins 5, 7, 6; Festival 10; Boss 10. The longest run of one kind is 4 (Siege): no other kind twice in a row.

**Other checks** (PC): unit tests 111 checks, 0 failed (each kind's win and lose on a minimal level, the colossus's
absorbing and arriving, the fallen twin's wasted side, a festival that cannot be lost and ends on its tick, each twist
starting once on its trigger, GOLDEN GATE's two more, WARD WALL's rising, a cub's health and its three pips);
determinism on levels 1-5 (the hashes of V2: a0044282ee3746e4, 601366e2542f5bd0, 5d5bae122c1f906e, c0487fc8f504ccc5,
17c5087585247415), the geode levels and a level of each kind; acceptance 1-5; soak 1-300 with greedy, hungry and sweep
(900 runs, 0 failed); flow test 53 checks, 0 failed; layout lint over every kind's HUD in the eight frames; flashing
audit with ECLIPSE, dawn and the colossus's fall (with and without REDUCE EFFECTS): pass; an eclipsed frame in greyscale
tells the wisps (bright teardrops) from the glooms.

**On the phones:** waiting. The Moto has shown a Google Play Protect prompt about the dev build since 17:48 (the owner's
privacy choice: not touched) and the tablet has been in use by someone else.

## Choices made for the owner (the owner asked for best judgment instead of questions)

| Question | Choice | Why |
|---|---|---|
| PLAN 2 decisions (name, theme, engine, scope) | Kept as proposed | The owner set the goal after reading them |
| `git init` | Yes, local only; commits at phase checkpoints, no remote, no push | Backups before breaking changes (owner's CLAUDE.md) |
| SDL source | Cloned from RAVEFORM's local checkout, verified at the pinned commit | No download |
| Font | **Roboto Black from the Android SDK** (Apache 2.0), corners rounded at bake time, instead of downloading Fredoka | Avoids a download that needed the owner's yes; same rounded, heavy look |
| Test devices | Moto `ZY22BXMJG3` (over Wi-Fi); tablet and 12R when they are reachable | Only the Moto was attached on 2026-11-20 |
| Glow when the flood is dense (P1) | Bloom samples three levels with bounded weights; labels drawn after the bloom; glowUnit 0.34, warmer glow colour | The additive chain burnt 12.8% of a 2,400-wisp frame to white; now 1.1% |
| Emerald Aurora's backdrop | Top colour 0x48B48A | The palette lint (backdrop at most 45% luminance) failed at 0.51 |
| Gold button pressed state (P3) | Deep amber #8A4E0C with white text, not white | White on gold is 1.6:1; the exit check (and the owner's CLAUDE.md) want a high-contrast pressed state: now 4.2:1 |
| Haptic rate limit | Twelve a second of wall-clock time | Game time runs at 0.35x in the win's slow motion, which let 14 a second through |
| `bench_play` after a result | Starts the same level again 3 s later | The measured window is play, wins and level loads, not a still result screen |
| A dev-only level stepper | In SETTINGS of Debug and RelWithDebInfo builds (`GT_DEV`); not in Release | P3 task 12: the owner's playtest jumps between levels |
| Device test scripts | `flow-android.ps1`, `save-android.ps1`, `backkey-android.ps1`, `coldstart-android.ps1` | Repeatable P3 checks on any device, with the lock and the foreground checks |
| Mix levels (P4) | Effects at 0.33, music at 0.54 of full gain, limiter holding samples at -1.5 dBFS | The flood render lands at -14.9 LUFS and -2.6 dBTP; the music sits 9-10 dB under it |
| The pad's voicing (P4) | An octave up (from C4 and A3) | A phone speaker has no bass: 11% of the bed's energy was above 300 Hz, now 45% |
| Synthesis cost (P4) | Oscillators by rotation and decays by multiplication instead of sin() and exp() per sample | The bank took 849 ms on the Moto (limit 150 ms); now 117 ms, no worker thread needed; cold start fell to 0.43 s |
| Acceptance of a baked level (P5) | Two lines added to GAME_DESIGN 7.4 step 5: the cancelled share within 5 points of `fight`, and from level 21 the naive bot must lose; up to two corrections of the rate scale before the next seed | The first full bake passed 7.4 but 13 levels cancelled 6-9 points over `fight` and three bands let the naive bot win twice |
| Crossing sliders (P5) | The two gates 1.6 m apart in z | At the same z they passed through each other (labels on top of each other, panels fighting for the same depth) |
| The spire at the win (P6) | It disappears at the shatter (its own mesh range); the shards fly from where it stood | It stayed standing behind its own shards |
| Level changes (P6) | The labels and the HUD fade with the picture, 150 ms out and in | They stayed at full brightness over a black frame |
| Clash sparks (P6) | Six per clash for the first six clashes of a tick, none after | Six for every clash (up to 64 a tick) would empty the particle budget the gates and the win need |
| The shot (P6) | A 10% squash for 70 ms and a lens ring on every shot; fired wisps grow 0.5 to 1 in 90 ms | It was a wobble while the finger was down |
| 60 Hz fallback (P6, TECH_SPEC 5.10) | A frame 1.5 periods or more after the one before counts as late; the decision needs at least two seconds of a level's frames | The contract's rounding rule, and a level that ends at once proves nothing |
| FRENZY and the other levers of GAME_DESIGN 5.12 | Not built; the unused `engage` and `frenzy` tweakables were removed | The design builds them only if the owner asks after playing |
| Release signing | The release APK is signed with the debug key | The release key is the owner's (docs/RELEASE_CHECKLIST.md) |
| `.gitattributes` | `* text=auto` kept | Windows tools here write CRLF; the repository stays LF |
| Public repository and releases (the owner, 2026-11-20: "public repo with the release, with an updater like raveform") | `rockenrooster/glowtide`, RAVEFORM's pipeline point for point: each push to `main` builds `build-<n>` with the APK; HOME's gold UPDATE pill opens the latest release when it is newer (`docs/RELEASE_PIPELINE.md`) | The device serials stay in the scripts (RAVEFORM's public repository lists the same ones); home-network addresses were taken out of the files; no licence file was added (RAVEFORM has none either: the owner's choice) |
| The INTERNET permission | Added for the update check alone | Nothing else in the game uses the network |
| V1's owner checkpoint (play 6-25, then 60 and 150; six questions) | Answered here: geodes as built (health in seconds of the stream that reaches them, plates and slides on detours only, two to four a level), boons last for their level only, two and three lanterns as built, the kalimba stays; V2 started | The owner asked for best judgment instead of questions; the band table says the decision exists (`podless` 22-57% slower, `hungry` slower or losing on most levels from 21). The questions stay open below for the owner to answer after playing |
| V1's dev APK on the phones | `GT_APP_ID_SUFFIX=.dev` on every phone (`dev.glowtide.game.dev`) | It installs beside the owner's GitHub copy instead of replacing it (and its save) |
| V2's soft checkpoint (do hoppers annoy, do side doors read as doors, is `KINDLE` good enough) | Hoppers kept as built (1.5-2.5 s at each place, a ghost frame where they land; level 16 introduces them with its hint); side doors redrawn to face the lantern; `KINDLE` as designed (its run up the lane is the strip) | The design's own numbers; the greyscale sheet for the doors. Open below for the owner |
| V2's stretch items | Not built (snatchers, currents, portals) | The must items and their bake came first; their calendar slots go to hand-made levels in V4 (VARIETY_DESIGN 10.1) |

## Measured device facts

| Device | GL | Display | dp scale | Present timestamps |
|---|---|---|---|---|
| Moto One 5G Ace `ZY22BXMJG3` | OpenGL ES 3.2 V@0502.0 (2021-07-14), Adreno 619 | 1080x2400, 60 Hz; safe top 112 px, bottom 120 px | 2.5 (400 dpi) | `EGL_ANDROID_get_frame_timestamps` with display present time: yes |
| Desktop | ES 3.0 context through WGL on the RX 9070 XT (ES 3.2 driver) | frames from `--res` | from the matrix | — |

## Benchmarks

| Date | Device | Scenario | Late share | Worst frame | Display p95 | CPU p95 (thread) | Tick p95 | Allocs | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 2026-11-20 | Moto | bench_idle 20 s | 0.84% | 35.9 ms | 17.10 ms | 3.67 ms | 0.15 ms | 0 | P0: 1,000 debug quads |
| 2026-11-20 | Moto | bench_flood 60 s after 180 s | 0.87% | 35.5 ms | 17.0 ms | 4.08 ms | 1.45 ms | 0 | P1 contract run: 2,400 wisps, 1,000 glooms |
| 2026-11-20 | Moto | bench_play level 5, 90 s | 0.90% | 34.8 ms | 17.14 ms | 3.68 ms | 0.11 ms | 0 | P2/P3, sound on: audio 0 late callbacks, fill p99 0.38 ms of a 20 ms period; haptics at most 12 a second |
| 2026-11-20 | Moto | bench_flood 60 s after 180 s | 0.76% | 34.7 ms | 16.97 ms | 4.20 ms | 1.47 ms | 0 | P3 regression check against P1: +2.9% CPU (limit 10%) |
| 2026-11-20 | PC | bench_play level 5 | — | — | — | — | 0.028 ms | 0 | P2: simulation P95 under 0.5 ms |
| 2026-11-20 | Moto | bench_flood 600 s with sound | 0.54% | 35.6 ms | 16.96 ms | 4.23 ms | 1.48 ms | 0 | P4: 30,000 callbacks, fill p99 0.33 ms of a 20 ms period, gap max 33.7 ms, 0 late |
| 2026-11-20 | Moto | bench_play level 87, 90 s | 0.79% | 33.9 ms | 17.00 ms | 4.57 ms | 1.90 ms | 0 | P5: the heaviest baked level (3,080 units at peak): 2,323 wisps, 757 glooms |
| 2026-11-20 | Moto | bench_play level 87, last 60 s of 20 min | 0.59% | 33.7 ms | — | 4.47 ms | — | 0 | P6 heat soak, battery 28.8 C at the end |
| 2026-11-20 | Moto | bench_flood 60 s after 60 s | 0.90% | 35.4 ms | 16.96 ms | 4.26 ms | 1.47 ms | 0 | P6 final build (no regression against P1: CPU +4%) |
| 2026-11-20 | Tablet SM-X230 | bench_flood 60 s after 60 s | 0.00% | 16.7 ms | 16.58 ms | 5.31 ms | 1.48 ms | 0 | 60 Hz (Motion smoothness: Standard) |
| 2026-11-20 | Tablet SM-X230 | bench_play level 87, 90 s | 0.00% | 16.9 ms | 16.58 ms | 5.29 ms | 1.67 ms | 0 | the heaviest level |
| 2026-11-20 | Tablet SM-X230 | bench_flood 600 s with sound | 0.00% | 16.8 ms | 16.58 ms | 5.40 ms | 1.49 ms | 0 | P4: 30,001 callbacks, fill p99 0.53 ms of a 20 ms period, 0 late |

Other measurements (Moto unless named):

| Check | Result |
|---|---|
| Determinism, levels 1-5, greedy bot, 60 s | PC and Moto hashes equal: 6ab53c6b7de9902a, 75fbb176a58bbe28, b555264948d891cc, 7988d22d19d3694a, 2c2b421ea381a278 |
| Flow test (`--flow-test`: screens, settings, save, input latency, SOUND and MUSIC end to end) | 45/45 on the PC and on the Moto (final build) |
| Layout lint, 6 screens x 7 frames + button strip | 0 problems; `captures/p3/contact.png` |
| Pressed vs normal fill (button strip) | 4.24:1 |
| Rebuild check (`--rebuild-gl-at`) for `hud` and `win` | Pixel-identical |
| Save through a force-stop mid-level | Level, stars and settings intact (`save-android.ps1`); damaged-file unit test passes |
| Lifecycle, 20 cycles during a level | 20/20 back on PAUSE, no errors |
| Cold start to the first frame (launch request to first swap) | Median 1.14 s, worst 1.33 s (target 2 s) |
| Back key | Moto: play to pause to play, HOME leaves the app |
| Strips: counts from the simulation for the captured ticks | 0 disagreements (gate: k out per one in; glooms lose power only to clashes and the lantern) |
| Audio renders (`review.ps1 -Set p4`, table in `captures/p4/metrics.md`) | Flood -14.9 LUFS, -2.6 dBTP, 0 clipped, DC 0.0004, 0% silence; music -25.4 LUFS (10.5 dB under), largest jump 0.033; ladder within 0.06 cents |
| Silence on the PC | With `--silent`: peak 0.534 before the mute, 0 non-zero samples handed to the device |
| Sound bank build | Moto 117 ms (was 849 ms) |
| Level bake | 299 levels accepted, 0 seeds rejected beyond the first try in most bands; determinism on 10 baked levels; soak 1-300 (greedy and sweep, invariants every tick): 600 runs, 0 failed |
| Level load (generate + meshes) on the Moto | 0.8 ms (limit 50 ms) |
| Flashing audit (`flash-audit.ps1`) | Win, boss win, lose and core hit, with and without REDUCE EFFECTS: at most 2 swings of 10% in any second (limit 3) |
| Heat soak, 20 min of `bench_play` on level 87 (`soak-android.ps1`) | Last 60 s inside the contract: late 0.59%, worst 33.7 ms, CPU p95 4.47 ms, valid. Battery 28.3 to 28.8 C. Memory (PSS, median of three reads on each minute mark) 95.5 MB at minute 1, 93.8 MB at minute 20 (-1.8%). `captures/soak/ZY22BXMJG3_soak_20261120_044332.csv`. (A first run sampled at 30 s and during a level reload and read +6.4%; its minutes 2-19 were flat, so the sampling was fixed, not the game.) |
| Sanitizers (ASan + UBSan: unit tests, determinism, acceptance, soak 1-50, an audio render, the flux table) | Clean: every run exit 0, no report |
| Greyscale sheet (`captures/p6/greyscale.png`) | Gates read by symbol (x and ÷); the crowds by shape (teardrops against horned blobs) |
| Win, boss win, lose and level-change strips (`captures/p6/`) | Slow motion 0.8 s, "CLEARED!" from 1.4 s, stars 250 ms apart, the fade 150 ms each way (by frame count) |
| Release APK | 3.31 MB; native Release, stripped (no `.debug` or `.symtab`); not debuggable; one permission (VIBRATE); `zipalign -c -P 16` passes and every `LOAD` segment is 0x4000-aligned; on the Moto it starts, ignores `--es args` (still running, scenario play) and `run-as` answers "package not debuggable" (`release-check-android.ps1`) |

## The final device matrix

| Device | Reached | Result |
|---|---|---|
| Moto One 5G Ace (`ZY22BXMJG3`, the floor phone) | Yes, over Wi-Fi | Inside the contract everywhere: `bench_flood` 0.90% late / 35.4 ms worst on the final build (and 600 s with sound: 0.54%, 0 late audio callbacks); `bench_play` level 5 0.90%, level 87 (the heaviest) 0.79%; the 20-minute soak; flow test 45/45; lifecycle 20/20 back on PAUSE; cold start 0.43 s; back key; save through a force-stop; determinism equal to the PC; the release APK |
| Galaxy Tab A11+ SM-X230 (`R5GL362BEHX`), Android 16, 1200x1920 at 240 dpi | Yes, over Wi-Fi (the owner switched Wireless debugging on) | All ten checks pass (`captures/matrix/R5GL362BEHX.md`): flow test 45/45, back key, save through a force-stop, cold start 0.52 s, lifecycle 20/20 back on PAUSE, `bench_flood` 0.00% late / 16.7 ms worst, `bench_play` level 87 0.00% late / 16.9 ms worst, CPU p95 5.3 ms. It ran at 60 Hz: the panel has a 90 Hz mode, but the tablet's Motion smoothness is set to Standard (`refresh_rate_mode` 0), which offers apps 60 Hz only; the game asked for the highest rate offered. 90 Hz needs that setting on Adaptive (the owner's to change), then `matrix-android.ps1 -Serial tablet` again |
| OnePlus 12R CPH2611 (`a0e622a5`), Android 16, Adreno 740, 1080x2376 at 2.55 dp | Yes, over Wi-Fi (the owner switched Wireless debugging on after first skipping it) | All ten checks pass on a dev build beside the owner's GitHub copy (`GT_APP_ID_SUFFIX=.dev`; `captures/matrix/a0e622a5.md`): flow test 45/45, back key, save through a force-stop, cold start 0.41 s, lifecycle 20/20 back on PAUSE, `bench_flood` 0.00% late / 16.6 ms worst, `bench_play` level 87 0.00% late / 16.6 ms worst, CPU p95 4.9 ms. The owner's report that the crowds were invisible there was a driver bug (Deviations), fixed in `build-14`. It ran at 60 Hz: the phone's screen-rate setting offers apps 60 Hz only (`peak_refresh_rate` 60); 120 Hz needs that setting raised |
| Fire 7 (`GN41WE04450600BJ`) | No | Open: a report only (TECH_SPEC 5.10) |

How the tablet and the 12R were looked for (2026-11-20, at the end of P6):

- **USB:** nothing but the Moto in `adb devices` all session.
- **Wireless debugging by mDNS** (how RAVEFORM finds the OnePlus): `adb mdns services` and a plain mDNS query from
  this PC got no answer from any device, the Moto included, so discovery does not work from here today.
- **Legacy `adb tcpip` (port 5555)** on the ten hosts of the local network with phone-style (randomized) MAC addresses:
  every one refused.
- **Where they were last:** RAVEFORM's logs gave the 12R's and the tablet R5GL362BEHX's addresses; adb's
  own log shows both connected to this PC over wireless debugging on 2026-11-19 (12R port 39437, with a TLS handshake
  that succeeded, so it still trusts this PC; tablet port 37505). Both hosts are on the network today.
- **A scan of 30000-61000** on them (Linux picks the port there; the Moto's is 38215) finds no adbd: the 12R has
  37567, 46888 and 47888 open and the tablet 44171, and each closes the connection on adb's first packet with no TLS
  handshake, so none is the debugging port. Wireless debugging is switched off on both (Android turns it off on a
  reboot or a change of network). A third host, .167:38975, behaves the same way.
- **Every other host** on the network, scanned over 30000-61000 too: the open ports on five more hosts all
  close on adb's first packet; no adbd answers anywhere but on the Moto.
- **On the PC instead:** the screens are rendered and linted at the 12R's own panel (1264x2780 at 450 dpi, now in the
  layout matrix of `review.ps1 -Set p3`) and at the tablet's (1200x1920): every screen passes, nothing clipped. That
  checks layout only, not the devices.

To fill the open rows (one command runs every device check, each with the lock and the foreground check, and writes
`captures\matrix\<serial>.md`; on the Moto all ten checks passed: `captures\matrix\ZY22BXMJG3.md`):

1. On the device: Developer options, switch **Wireless debugging** on (the 12R and the tablet are already paired with
   this PC), or plug the tablet into USB. Then `scripts\find-wireless-adb.ps1` scans their last addresses and connects
   adb without mDNS (a device that no longer trusts this PC needs `adb pair` with the code its screen shows).
2. `scripts\matrix-android.ps1 -Serial tablet` (or `op12r`, or the `ip:port`). It installs the dev APK and runs the flow
   test, the back key, the save, cold start, 20 lifecycle cycles, `bench_flood`, `bench_play` on level 87, and records
   the panel's modes and the rate the game asked for (90 Hz on the tablet, 120 Hz on the 12R; the 60 Hz fallback logs
   `refresh:` if it trips).

## Deviations from the specs

- **Present timestamps on Android** use `eglGetCurrentSurface(EGL_DRAW)`: `SDL_EGL_GetWindowSurface` is not implemented
  by SDL's Android backend (it returns NULL).
- **Benchmark CPU time** is the main thread's CPU time (`CLOCK_THREAD_CPUTIME_ID`), not wall time: on Android the
  driver blocks for the next buffer inside GL calls, which wall time would count as work.
- **The baked table stores the enemy's base flow** (power a second) beside the seed and the spire's health, instead of
  the rate scale: the game needs the flow, and the flow needs `P0`, which only the bake measures.
- **dp scale of the Moto is 2.5** (400 dpi), not 2.6 as TECH_SPEC 5.2's table guessed.
- **Separation** runs on cell-ordered copies of the units with a half stencil (each pair once), not on the unit arrays:
  3.4x faster on the Moto (7.8 ms to 1.45 ms tick P95 at the caps).
- **The main thread is pinned to the fast cores** on Android (`sched_setaffinity` to the CPUs with the highest
  `cpuinfo_max_freq`): the scheduler kept it on a little core.
- **GAME_DESIGN 5.9 corrected after P2** (`glowtide_headless --sim flux`, table in `captures/flux_findings.md`): the
  toy model's findings 1 and 4 hold; the edge (finding 2) is sharper (a three-row x30 layout collapses already at an
  enemy flow of 0.9 times the flood) and the pressure ramp finishes a half-strength player on level 5 (finding 3).
- **Device runs keep the phone awake** (the owner: a phone that sleeps locks): `Assert-GtUnlocked` before a run, and
  `Start-GtKeepAwake` wakes the screen and sends a Shift key every 8 s while GLOWTIDE or the launcher is in front (no
  settings change); the lifecycle check streams the app's log to a file during the run (the 12R's 256 KiB log buffer
  holds about a minute).
- **No const arrays indexed by a varying value in the shaders** (2026-11-20): on the OnePlus 12R (Adreno 740, driver
  V@0676, Android 16) a `const float[4]` indexed by a tier read from an attribute gave no usable radius, so every wisp
  and gloom had no size and the crowds were invisible (the owner's report: "12r doesn't show bullets"). Found on the
  device by elimination; the radius is chosen by selects in `prelude_vert.glsl` (`unitRadius`). Fixed in `build-14`.
- **The clash strip** on level 5 shows little clashing (its `fight` is 15%): the P6 strips use later levels.

## Copied from RAVEFORM

`scripts/env.ps1` (tool paths, 4-core affinity, device helpers; lock helpers rewritten from `audio-device.ps1`),
`build-desktop.ps1`, `run-desktop.ps1`, `build-android.ps1` (adapted), the Gradle wrapper, `src/core/png.cpp` (without
the rotation), `src/core/alloc_counter.cpp`, the activity's immersive mode, argument and haptic patterns,
`tools/shrink_png.py` (height limit added).

## After version 1 (ideas, not built)

**Planned on 2026-11-20, not built: the variety and content pass.** The owner asked for upgrades that come down the
lane toward the lantern ("you have to decide to hit enemies or grab the upgrades which re sometimes behind walls with
hp you have to destroy") and for "level variety and creativity too so there's no stale or repeated levels", and then
for a plan another agent executes. It is `VARIETY_PLAN.md` (read first), `VARIETY_DESIGN.md`, `VARIETY_TECH.md` and
`subplans/V0_*.md` to `V6_*.md`. Its baseline: of today's 295 generated levels, 45% share their gate pattern with
another level, every level has one goal and one enemy plan, and the greedy bot is never hit
(`python -I -B docs/variety/baseline_stats.py`). The executor of that pass records its phases in this file.

- The levers of GAME_DESIGN 5.12 (FRENZY and the others), if the contest needs more threat after playing.
- A lighter level of effects for the Fire 7 class (TECH_SPEC 5.10), if that device matters.
- Recorded samples instead of synthesis for any sound that still grates (a download: the owner's yes first).

## Open questions for the owner (the phase checkpoints)

1. **The look (P1):** `captures/p1/contact.png`, `captures/p6/contact.png` and the game on the phone. Anything to change?
2. **The fun (P2):** is the contest right? GAME_DESIGN 5.9 now says what the real simulation does (a sharp edge; a weak
   player loses late levels to the pressure ramp).
3. **The sound (P4):** which ladder timbre (`captures/p4/timbres_kalimba.wav`, `_glass.wav`, `_bell.wav`; the game uses
   the kalimba)? Is the big flood pleasant or busy? Keep the music, change its level, or off by default?
4. **The difficulty (P5):** play levels 1-20 with the dev build, then jump to 50, 100 and 200 (SETTINGS has a level
   stepper in dev builds). Right pace? Samey? Right length? Any unfair or dull level (its number is enough)? Change any of
   the five worlds? Try FRENZY?
5. **Sign-off (P6):** which phone do you carry? Then the icon and the name, and your own release steps in
   `docs/RELEASE_CHECKLIST.md` (the key, the store listing).
6. **The tablet and the 12R:** attach them (USB or wireless debugging) for their rows of the matrix.
7. **Geodes (V1):** play levels 6 to 25 of the dev build (`dev.glowtide.game.dev`), then jump to 60 and 150 with
   SETTINGS' level stepper. Is this the upgrade you meant? Did you choose between a geode and the enemy, or was it always
   obvious? Too many, too few, too tough, too soft, too fast? Two and three lanterns: fun, too strong? Should a boon ever
   last past its level? (Answered by best judgment for now: Choices.)
9. **Kinds, twists, bosses (V3):** levels 15 (festival), 21 (holdout), 31 (colossus), 41 (twins), the bosses 10, 20,
   30, 40, and twists from 43. Which kinds are fun, any to drop? Holdout tense or tedious, the colossus fair, do you see
   that the twins make you cross over? Is a festival every tenth level welcome? Did ECLIPSE and GOLDEN GATE read without
   being told? Are the bosses different from each other? (Answered by best judgment for now: kept as built.)
8. **The lane and the enemy (V2):** ten levels between 16 and 60. Does any new gate annoy rather than interest (hoppers
   are the candidate)? Do side doors read as doors? Is `KINDLE` as good as it should be?

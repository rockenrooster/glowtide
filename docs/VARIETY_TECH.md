# GLOWTIDE — Variety and Content: Technical Notes

Where `VARIETY_DESIGN.md` (written "VD 2.1" here) lands in the code as it stands on 2026-11-20 (commit
`7990557`). `TECH_SPEC.md` ("TS") still holds: the simulation is deterministic and allocation-free, the
renderer is OpenGL ES 3.0 core, PC runs are silent and off screen, and so on. This document names the
files, the limits that must grow, the order things happen in, and the new tools. It does not design the
code: keep the shapes the code already has.

Facts below were read from the source while planning. Check them against the source before relying on
one: the source wins.

---

## 1. What is touched, what is not

| Area | Files | Change |
|---|---|---|
| Level data | `src/sim/level.h` | New arrays and fields (section 2) |
| Layouts | `src/sim/levels.cpp` (levels 1–5 and the generator today) | Split: `levels_hand.cpp` (hand-made levels), `recipes.cpp` (VD 9.2), `calendar.cpp` (VD 10: kind, hardness, world, what a level may use), `levels.cpp` (the entry points) |
| Rules | `src/sim/world.h`, `world.cpp` | New state and tick steps (section 3) |
| Bots | `src/sim/bots.h`, `bots.cpp` | `greedy` values geodes; `podless`, `hungry` (section 4) |
| Bake | `src/sim/bake.h`, `bake.cpp`, `levels_baked.h` | Bake v2 and its table (section 5) |
| Variety | `src/sim/fingerprint.h`, `fingerprint.cpp` (new), `src/tools/headless.cpp` | The fingerprint, the likeness, the report (section 6) |
| View | `src/game/view.h`, `view.cpp`, `src/gfx/mesh.h`, `mesh.cpp`, `renderer.h`, `renderer.cpp` | Props, labels, goal models (section 7) |
| HUD and screens | `src/game/hud.h`, `hud.cpp`, `screens.cpp`, `game.h`, `game.cpp` | Goal bar, boon tray, banners, hints, stars by kind (section 9) |
| Sound | `src/audio/synth.h`, `synth.cpp`, `src/game/feedback.h`, `feedback.cpp` | New sounds and cues (section 8) |
| Pictures and tests | `src/game/pictures.cpp`, `src/tools/tests_sim.cpp`, `tests.cpp`, `scripts\review.ps1`, `test.ps1` | Section 10 |
| Shaders | `shaders/prelude_vert.glsl`, `unit.vert`, `backdrop.frag` | Two unit sizes; sky features in V5. Nothing else needs a new shader |

Not touched: the platform layer, the frame loop, the audio device, the save's format (one field grows,
9.3), the build, the release workflow, the updater.

---

## 2. Level data

`LevelDef` (`level.h`) today: index, seed, `len`, world, `targetT`, `spireHealth`, `coreHealth`,
`baseFlow`, `fight`, `boss`, brute numbers, and fixed arrays: 24 gates, 8 barriers, 16 bumpers, 8
snuffers, 3 emitters, 8 bursts, 6 rows. A wisp carries one bit per gate in a `u32`.

Add (names are suggestions; sizes are requirements):

| Field | Type and size | Notes |
|---|---|---|
| `kind` | `u8` | Siege, Holdout, Colossus, Twins, Festival, Boss (VD 6) |
| `recipe`, `plan`, `lengthClass`, `hardness`, `boss` type | `u8`, `u8`, `u8`, `float`, `u8` | Set by the calendar and the recipe; the fingerprint reads them |
| `geodes[16]` | `x`, `z0`, `t`, `speed`, slide (`amp`, `period`, `phase`), `radius`, `boon`, `group`, `shielded`, `anchoredUntil`, `hpShare`, `shieldShare`, and the baked `hp`, `shield` | 8 used outside festivals |
| `twists[4]` | trigger (time fraction, goal fraction or core), type, parameter | Replaces `bursts[8]`: `SURGE` and `BRUTE GUARD` are twists (VD 7). Keep `Burst` until V3, then migrate |
| `currents[4]`, `portals[2]` | Stretch (VD 4) | Only if built |
| Gates | `slides` grows from a flag to a mode: still, slide, hop; plus `breath` (period, 0 = none) | The bot and the view read the mode |
| Barriers | 8 → 16; slide (`amp`, `period`, `phase`) | Toll road needs six spans; the ward wall one |
| Emitters | 3 → 6; `z`, `side` (0 foot, −1 / +1 a side door), `share`, `group` (Twins: which spire) | |
| Goal | `goal`, `goal2` (`i32`) replace `spireHealth` | Siege and Boss: the spire. Twins: left and right. Colossus: its health. Holdout: unused. Festival: the three-star count |
| `ref` | `float` | `R` (VD 12.2); geode health = share × `ref` at load |

Limits to check when sizes grow: `kMaxGates` stays 24 (the `u32` of bits; a level that needs more is a
level that is too busy). `kMaxRows` 6 is enough for five rows. The view's `PropDraw props_[64]`
(`view.h`) grows to 128; `kMaxPanels` 32 (`renderer.cpp`) is enough while gates stay at 24.

Two version numbers in `level.h`, both written into `levels_baked.h` by the bake:

- `kGeneratorVersion`: bump when anything that changes a layout changes (a recipe, the calendar, a
  hand-made level).
- `kRulesVersion`: bump when anything that changes a bot run changes (a rule in `world.cpp`, a tuning
  default, a bot).

A unit test fails when the table's versions differ from the code's. A stale table is the one mistake
that ships a broken game silently; this makes it loud.

---

## 3. The simulation

### 3.1 State (all of it in `World::hash()`)

Geodes (alive, `x`, `z`, health, shield, per slot); boons (lanterns, `quicken`, `bright`, `kindle`, the
tick `HUSH` ends); the goal by kind (`goal`, `goal2`, the colossus's place and health, the dawn tick);
twists fired; the plan's phase; hopper and breathing gate state if it is not a pure function of the
tick; `spent` (power given to geodes and plates). `gloomSpawned` and other bookkeeping stay out of the
hash, as now.

Unit flags (`u8 flags`, today only `kBrute`): add `kRunner`, `kSnatcher`. The colossus and its cubs are
not units: they are bodies with health, like geodes, so they absorb every wisp that touches them. A
gloom may clash once a tick (GD 5.5); a brute with the colossus's health would take minutes to kill.

### 3.2 A tick, with the new steps in bold

1. Input: move the aim point; **its range depends on the lantern count** (VD 3.1).
2. Fire: **rate from `QUICKEN`, one wisp a lantern, power from `BRIGHT`; `COMET` wisps**.
3. Emitters: **the plan's shape (VD 5.1), side doors, runner packs, `HUSH`**; brutes; the pressure ramp.
   **Twists that are due start. Geodes that are due appear.**
4. Steer and move: funnel (**toward the nearer living spire in Twins**), homing (**runners' speed,
   snatchers' steering**), **currents**, fan decay, lane walls.
5. Advance sliding gates, **hoppers, breathing gates, sliding walls**, snuffers, **geodes (drift,
   slide, slipping past), the colossus**.
6. Grid. 7. Separation. 8. Bumpers.
9. Gates: **`KINDLE`'s extra one; a breathing gate's opening as it is this tick; portals**.
10. Barriers (**their span as it is this tick**).
11. **Plates, then geodes, then the colossus and cubs: each takes up to the power of every wisp that
    touches it. A geode at zero opens: its boon applies now and shows from the next tick.**
12. Snuffers. 13. Clash. **14. Snatchers that touch a geode take it.**
15. The goal and the core, **by kind**.
16. Remove the dead; tide marks; events.
17. Win and lose, **by kind** (VD 6).

Plates and geodes come after barriers so a wisp that just broke a wall can still reach the vault behind
it in the same tick, and before the clash so a geode in the middle of the front line is not starved by
it.

### 3.3 Rules that are easy to get wrong

- **Crossing a moving plate.** A barrier is crossed when a wisp's `z` was below it at the start of the
  tick and is at or above it now. A plate moves, so compare against the plate's `z` at the start of the
  tick and now.
- **The cap.** `emitWisp` at the cap adds the shot's power to a recent wisp. With three lanterns that
  must still work per lantern (the recent wisp nearest that lantern), or the stream collapses onto one
  column at the cap.
- **`KINDLE` and bits.** A wisp that already passed a gate is not multiplied again when the gate's
  factor grows.
- **Determinism.** No `sin`, `cos`, `pow`, `exp` in `src/sim` (TS 4.2): the hopper's 0.25 s crossing
  and the breathing gate use `easedWave` or plain arithmetic. No allocation after `World::init`: every
  new array is fixed-size or allocated there.
- **Events.** `kMaxEvents` is 256 a tick and clash events already use up to 64. New events: geode hit
  (one a geode a tick, with the damage), plate break, geode open (with the boon), geode lost (slipped
  past, snatched, dissolved), boon applied, twist started, goal progress (Twins: which spire), dawn,
  colossus step.

### 3.4 Tuning

Every new number is a tweakable in `src/core/tuning.h`'s `GT_TUNING` list (`--set name=value` on the
app and the headless tool), like the existing ones: geode speeds and radius, boon sizes and limits, plan
shapes, twist lengths, the colossus's pace, the reliance curve.

---

## 4. Bots (`bots.cpp`)

`Bot::score(world, x)` today walks the gates, barriers and snuffers on a straight line from `x` in `z`
order, carrying the stream's width and multiplier, and returns the power delivered in the next 8 s; a
barrier takes `health ÷ stream` seconds out of that budget. `Bot::think` scores 21 places every 0.2 s,
has a defend rule, and moves like a hand (0.15 s reaction, 10 m/s, ± 0.25 m of noise).

- **Geodes in `score`.** A geode whose body (or plate) the line meets, taken where it will be when the
  stream arrives: subtract its remaining health ÷ the stream's power there from the budget, then add
  its worth: for a lasting boon, the stream's gain (VD 3) × the power it would deliver in the time the
  level has left (`def.targetT` − now − the cost), at least 0; for `WARD`, a value for each missing pip
  and each gloom below `z` 14; for `COMET` and `HUSH`, a fixed fraction of a second's delivery. A
  geode off the line is worth nothing to that line.
- **Lanterns.** The line is as wide as the row of lanterns.
- **Moving things.** Hoppers and breathing gates are taken as they will be on arrival, the way sliders
  are now. Currents shift the line as it climbs. A portal moves it.
- **Goals.** Colossus: the line's worth is what reaches the colossus where it will be. Twins: only the
  living spire's side counts. Holdout: the defend rule's `z` = 12 threshold becomes the whole lane, and
  delivery counts for nothing: the worth of a line is the glooms it will meet. Festival: geodes only.
- **`podless`**: `greedy` with every geode's worth set to zero. **`hungry`**: the nearest geode's `x`
  while one is alive, else `greedy`.
- The app's `--bot` and the headless tool's `--bot` take the new names (`botFromName`).

Keep the bot honest: it must not read anything a player cannot see (it may read a geode's boon and
health: they are drawn).

---

## 5. Bake v2 (`bake.cpp`, `glowtide_headless --bake-levels`)

Today: per level, in parallel over 4 threads: P0, seven bisection steps, the spire, acceptance (ten
greedy runs, three naive, one idle), up to eight seeds; then up to two corrections of the rate scale.
299 levels take 245 s. The table row is `{seed, spire, baseFlow}`.

V2, in three passes:

1. **Layouts, one thread, in level order.** For each level: the calendar gives its kind, hardness, world
   and what it may use; try seeds until the layout passes the generator's lints and section 6's rule
   against every level already chosen (at most 64 seeds; report a level that finds none). Generating a
   layout takes microseconds, so this pass is seconds.
2. **Balance, in parallel.** VD 12.2's steps 2 to 7 for each level with its layout fixed.
3. **Repair.** For each level that was not accepted, in order: take the next seed that passes the rule
   against the levels on both sides of it, balance it again; at most eight seeds; then report it.

The row becomes `{seed, goal, goal2, baseFlow, ref}` and the header carries the two version numbers
(section 2), the level count and the bake's date. `levelFor` (`levels.cpp`) reads it; `levelForPlay`
implements VD 16 (mirror and world shift past the last baked level: mirror every `x`, slide phase and
side in the layout after generating it).

The report (`captures\levels\report.md`) keeps a row a level and gains: kind, recipe, plan, twists,
boons offered, `Pa`, `Pn`, `m`, `R`, geodes opened by `greedy`, `podless`'s and `hungry`'s results, the
greedy bot's lowest core and the lowest `z` a gloom reached. `tools\band_table.py` gains VD 12.2's step
8 and prints, per band of ten: the three shares of step 8 and whether each acceptance line held.

Budget: about 45 bot runs a level instead of 25; 600 levels should take about 15 minutes at 4 jobs.
Above 30 minutes, find out why before going on. The bake stays inside the PC's four cores
(`scripts\env.ps1`).

The hand-made levels (1 to 5, the introductions, the set pieces) go through pass 2 like the rest. They
have no next seed: one that is not accepted is changed by hand.

---

## 6. The variety report (`glowtide_headless --sim variety`)

### 6.1 Fingerprint (`fingerprint.cpp`, pure functions of a `LevelDef`)

- Scalars: kind, recipe, plan, length class, the set of twists, the ordered list of boons offered.
- A map of 12 bands along the lane (from `z` = 4 to `LEN` − 4) × 8 columns across (1.25 m each). A cell
  holds the code of the strongest thing that covers it: empty; multiply gate ×2 to ×6; divide gate;
  wall; bumper; snuffer's path; geode's path; side door; current; portal. A gate that moves fills the
  cells of its whole travel and sets a "moves" bit in the code.

### 6.2 Likeness of two levels, 0 to 1

`0.20 [same kind] + 0.15 [same recipe] + 0.10 [same plan] + 0.05 [same length class] + 0.10 J + 0.40 M`

`J` is the Jaccard index of the two boon sets (1 when both are empty). `M` is the share of cells that
hold the same code among the cells that are not empty in at least one of the two maps, taken as the
larger of two comparisons: the second map as it is, and mirrored left to right.

Today's 295 generated levels are all Siege, one recipe, one plan, one length class, no boons: every
pair scores at least 0.60 before the map is looked at. That is the baseline the rule exists to break.

### 6.3 The rule and the counts (VD 11)

At most 0.45 for levels 1 to 12 apart, 0.60 for 13 to 40, 0.85 anywhere; no recipe twice within 6
levels; no (recipe, kind) pair twice within 30; no boon list twice within 8. The weights and thresholds
are starting values: V0 prints the baseline's distribution, V4 tunes them so the rule is both
satisfiable (pass 1 finds a seed within 64 tries for every level) and meaningful (the levels it rejects
are ones a person looking at two thumbnails would call the same). Record the final numbers in
`PROGRESS.md`.

### 6.4 Output

`--sim variety [--from A --to B]` prints: the worst pair in each distance class; how many pairs break
each line of the rule (exit code 1 if any); the count of each kind, recipe, plan, twist and boon per
hundred levels; the longest run of levels with the same kind and with the same recipe; and, with
`--pairs`, the twenty most alike pairs so their thumbnails can be put side by side
(`scripts\review.ps1 -Set levels` renders `level_top` thumbnails; give it `-From` and `-Count` so thirty
consecutive levels fit one sheet).

`docs\variety\baseline_stats.py` is the planning-time stand-in: it reads only the pattern letters of the
bake report. Keep it until this report exists, then delete it.

---

## 7. View, renderer, HUD

- **Props** (`mesh.cpp` `buildPropMeshes`, `PropMeshId`): today launcher, lens, pip, two gate posts,
  snuffer. Add: geode shell; one icon a boon (seven); plate; side door arch; hopper ghost frame; portal
  ring; festival lantern; holdout arch. The side lanterns are the launcher's meshes with a scaled model
  matrix. Everything is flat-shaded faceted geometry through the existing prop and prop-glow programs:
  **no new shader**.
- **Order** (`Renderer::draw`, step 3): props are drawn opaque and depth-written right after the lane
  and before the crowd, and the crowd is depth-tested against them. A geode therefore hides the units
  behind it and is hidden by the units in front of it with no change to the passes. Each prop is one
  draw call, and one more in the glow pass when its `glow` is above zero (section 11).
- **Level mesh** (`buildLevelMesh`, rebuilt per level): two spires for Twins (each its own index range,
  as the one spire has today in `LevelMeshData::spire`, so a fallen one can be hidden), no crystal for
  Holdout and Colossus, the boss's crown. Barriers already have a range each and an offset each frame
  (`Scene::barrierOffset`, used for the hit shake): a sliding wall is that offset.
- **The colossus and cubs** are drawn by the unit shader as glooms of two new sizes. The radius comes
  from `unitRadius(gloom, tier)` in `shaders/prelude_vert.glsl`: add tiers 4 (cub, 1.1 m) and 5
  (colossus, 1.8 m) to its selects. **Do not use a constant array there** (section 12).
- **Labels** (`View::label`, drawn after the bloom so numbers always win): geode and plate health, the
  colossus's health. `UiBatch::kMaxQuads` is 3,000; a festival adds sixteen numbers.
- **Light.** A geode puts one soft aqua light in the light map (`View::addLight`), like the launcher's.
- **Events to effects** (`View::onTick`) and **events to sound and haptics** (`Feedback::onTick`): VD
  13's table, with its rate limits applied in `Feedback` on the game thread, as the existing cues are.
- **Particles**: the per-frame spawn budget (`tune.particleBudget`, 220) stands. A geode opening is
  twenty shards; `KINDLE`'s run is labels, not particles.
- **HUD** (`hud.cpp`, `HudState`): the goal bar by kind; the boon tray; banners. Today's one banner
  (`bannerMark`, `FLOOD ×N`) becomes a short queue of text banners (goal, boon, twist, tide mark), one
  showing at a time for 900 ms.
- **Layout lint** (`Ui::lint`, `--lint`): the tray and the goal bar are recorded like other controls
  and must pass in all eight frames of `scripts\review.ps1 -Set p3` (the eighth is the 12R's 1264 ×
  2780).

---

## 8. Sound

`SoundId` (`synth.h`) gains: `tink` (the geode hit: the glass pluck the synthesizer already has, short),
`geode_open`, `geode_lost` (or reuse `pff`), a cue per twist (five), `colossus_step`. Boon motifs are
three existing ladder notes played with delays, the way the tide mark's run is (`Feedback::onTick`,
`kEvTideMark`): no new samples. The bank is built at start-up in 117 ms on the Moto (limit 150 ms):
measure after adding sounds.

Per-world music (V5): the pad is two pre-rendered four-bar loops (dark and bright) built in
`Bank::build`. A world's pad needs its own pair. Measure one pair's build time on the Moto before
choosing where to build it (VD 15).

Re-run `scripts\review.ps1 -Set p4` after any sound change: the flood render must stay inside GD 12's
targets (true peak at most −1 dBFS, −20 to −14 LUFS), and add a `geodes` scene to
`--render-audio` (a level with four geodes played by `greedy`).

---

## 9. Game layer

### 9.1 Screens

`Screen::Win` and `Lose` are decided by `world_.result` today (`Game::tick` → `finishLevel`). Kinds
change what `finishLevel` computes (stars by time, by core, by geodes) and what the result screen says
(`CLEARED!`, `DAWN!`, `FESTIVAL!`). HOME shows the level's kind under the `LEVEL N` chip for a kind
other than Siege.

### 9.2 Hints

`kHints[]` (`screens.cpp`) has six lines; `SaveData::hints` is a `u32` of "shown" bits and
`Game::showHint` refuses ids of 32 and above. VD 14 adds about ten: still inside 32.

### 9.3 Save

`save.txt` keeps `level`, `stars` (one digit a level, 1,000 levels), five settings, `refresh60`,
`hints`. Re-baking changes what level N is; a player's level number and stars stay, which is fine.
`finishLevel` clamps the star index to 999: past level 1,000 every level writes the last slot. Stop
recording stars past the array instead.

### 9.4 Development helpers that already exist

SETTINGS has a level stepper (`LEVEL -`, `LEVEL +`, `+10`) in Debug and RelWithDebInfo builds
(`GT_DEV`). `--level N` starts at a level; `--bot greedy` plays it; `--fresh` ignores the save.
`--picture level_top --level N` draws a level from above.

---

## 10. Tests, pictures, device checks

What exists and must keep passing: `scripts\test.ps1` (65 unit checks; determinism on levels 1 to 5;
acceptance and a soak on 1 to 5); the flow test (`--flow-test`, 45 checks, PC and device); the layout
lint over 49 screen renders; the rebuild check; `scripts\flash-audit.ps1`; the sanitizer build
(`build-desktop.ps1 -Sanitize`); `scripts\matrix-android.ps1` (ten checks on a device).

Add:

| What | Where | Checks |
|---|---|---|
| Rule tests | `tests_sim.cpp` | One a rule: a geode takes exactly the power given and passes the rest on; a plate first, from any side; opening applies the boon that tick; slipping past costs nothing; a choice pair dissolves; each boon's arithmetic (rate, lanterns, power, `KINDLE` at ×5 → ×6 and not past); the aim range by lantern count; the cap with three lanterns; each kind's win and lose; each twist's trigger; hopper and breathing gate positions; runner speed; the plan's average flow equals the base flow over `T` (each plan) |
| Versions | `tests.cpp` | The table's `kGeneratorVersion` and `kRulesVersion` equal the code's |
| Generator | `tests_sim.cpp` (`testGenerator` runs 1,000 seeds today) | Every recipe × 200 seeds: no overlap, GD 5.7's gaps, a reachable multiply gate, counts within the arrays, geode rules of VD 9.3 |
| Determinism | `test.ps1` | One level of every kind and every recipe, two runs equal on every tick; the Moto's hashes equal the PC's (`--scenario sim_hash`: extend its level list) |
| Soak | headless `--sim soak` | Levels 1 to the last, `greedy`, `hungry` and `sweep`, invariants every tick |
| Variety | `test.ps1` | `--sim variety` exits 0 on the committed table |
| Flow | `screens.cpp` `flowStep` | A geode level: a boon is applied and its chip shows; one level of each kind reaches its result screen; the result line shows the boons |
| Pictures | `pictures.cpp`, `review.ps1` | `geodes` (every boon's geode; plain, shielded, pair, vault, runner), `geode_strip` (hit, crack, open, the icon's flight), `lanterns_3`, `kindle_strip`, `kind_holdout`, `kind_colossus`, `kind_twins`, `kind_festival`, `boss_<name>`, one strip a twist, `recipes` (three seeds of each recipe from above), thirty consecutive levels from above at three places |
| Counts in strips | `Game::stripCheck` | The existing check (what entered a gate against what left) gains: power given to a geode equals its health lost |
| Greyscale | `tools\greyscale.py` | The `geodes` and kind pictures: a geode is told from a gate, a gloom, a brute and a snuffer |
| Flashing | `flash-audit.ps1` | Add a geode opening, `KINDLE`, `WARD`, `ECLIPSE`, the colossus's fall, dawn |
| **Device picture check** | New: `scripts\picture-android.ps1`, in `matrix-android.ps1` | Section 12's first trap: run a picture on the device, pull the PNG, compare with the PC's render of the same picture (gold and violet pixel counts within 20%) |

---

## 11. Performance

The contract is PLAN 5, unchanged: on the Moto, `bench_flood` after a three-minute soak has at most 5%
late frames and no frame over 50 ms; a phase may not raise the late share by more than two points or
the main thread's CPU P95 by more than 10% against the phase before. Where it stands (PROGRESS.md):
`bench_flood` 0.9% late, CPU P95 4.3 ms, tick P95 1.47 ms; `bench_play` on the heaviest level (87) 0.8%
late, tick P95 1.9 ms (budget 2.5).

- **The bench rig** (`benchFloodLevel` in `bake.cpp`, `Game::setupBenchFlood`): add three lanterns
  firing with `QUICKEN` and `BRIGHT` at their limits, four unbreakable geodes in the stream (so the
  absorb step and its numbers run every tick), runners among the glooms, a hopper and a breathing gate.
  Do it in V1 and again in V2, so `bench_flood` stays the heaviest picture the game can make.
- **New per-tick work** is small next to separation: geodes and plates test every wisp against at most
  16 bodies (use the grid if it shows in the tick time), currents and portals are one test a wisp.
- **Draw calls.** Today a level's moving props are a handful (launcher, lens, pips, posts, snuffers).
  A geode is two or three props (shell, icon, plate), each a draw call in the scene and one in the glow
  pass: a festival with sixteen geodes and three lanterns adds about a hundred. The benchmark does not
  report the count yet: in V1 add `draw_calls` (the largest `Renderer::lastDrawCalls()` of the run) to
  its JSON and watch `phases_p95_ms.submit`. If the submit phase grows by more than 0.3 ms on the Moto,
  draw each geode mesh instanced (one call a mesh) instead of raising the budget.
- **`bench_play`** on the heaviest baked level of each kind, with `greedy` taking geodes.
- **Devices**: the Moto is the gate; the tablet and the 12R run `matrix-android.ps1` every phase. All
  three ran at 60 Hz on 2026-11-20 because the tablet's and the 12R's own screen settings offered apps
  60 Hz; the 90 and 120 Hz rows of PLAN 5 are still unmeasured (PROGRESS.md says which setting).
- Zero heap allocations per frame and per tick, as now (the benchmark reports the count).

---

## 12. Traps found while building version 0.1.0

1. **A device can pass every check and draw nothing.** On the OnePlus 12R (Adreno 740, driver V@0676,
   Android 16) a `const float[4]` indexed by a value read from a vertex attribute returned no usable
   value: every wisp and gloom had no size, and the flow test, the benchmarks and the lifecycle check
   all passed on an empty lane. Rules: no constant arrays indexed by anything but a constant in a
   shader (use selects); and section 10's device picture check on all three phones, every phase.
2. **A phone that sleeps locks**, and then nothing comes to the front: `am start -W` hangs and every
   check fails with no output. `Assert-GtUnlocked` and `Start-GtKeepAwake` (`scripts\env.ps1`) exist;
   `matrix-android.ps1` uses them. A PIN cannot be entered from here: ask the owner.
3. **The 12R's log buffer holds about a minute.** Stream the app's log to a file during a long check
   (`lifecycle-android.ps1` does); do not read it back at the end.
4. **Wireless debugging ports change** each time it is switched on, and it switches off when a phone
   reboots or changes network. Use the names (`-Serial moto|tablet|op12r`); when discovery finds
   nothing, `scripts\find-wireless-adb.ps1 -Ip <address>`.
5. **A GitHub build and a local build do not replace each other** (different signing keys). To test on
   a phone that has the owner's GitHub build, build with `$env:GT_APP_ID_SUFFIX = '.dev'`: the dev APK
   is then `dev.glowtide.game.dev` and every device script drives it. Uninstall it afterwards.
6. **Every push to `main` that touches more than documentation publishes a release**, and installed
   GitHub builds then show `UPDATE`. Push only when the owner says so, and only a state that passed its
   phase's exit checks.
7. **GitHub's build can fail on Maven Central's rate limit** (HTTP 429 while Gradle fetches plugins).
   It is not the code: run it again.
8. **Editing files from shell one-liners corrupted them twice**: Python string escapes turned `\r`,
   `\f` and `\t` inside Windows paths into control characters, and patch scripts wrote CRLF. Use the
   editing tools; when a script must write a file, use raw strings and check for control characters.
9. **A shell heredoc piped into `python -` or a bare `cat >` hangs** the tool until its timeout. Write
   a script file and run it.
10. **Stale tables.** Changing the generator or a rule without re-baking ships levels sized for other
    rules. Section 2's version test exists so this cannot pass `test.ps1`.

# GLOWTIDE — Variety and Content Plan

Status: written 2026-11-20 by a planning session for a different executor (a coding agent). Version 0.1.0
exists: `PLAN.md`'s phases P0 to P6 are built, measured on three devices and published
(`PROGRESS.md`, `RELEASE_PIPELINE.md`). **Nothing in this plan is built.** Section 3's decisions are the
planner's proposals around three things the owner said (section 1); the owner confirms or changes them
at the checkpoints of section 6.

| Doc | Purpose | When to read |
|---|---|---|
| `VARIETY_PLAN.md` (this) | What the owner asked, where the game stands, decisions, contracts, phases, rules | Start of every session of this pass |
| `VARIETY_DESIGN.md` ("VD") | The rules and numbers of everything new | Sections named by the phase |
| `VARIETY_TECH.md` ("VT") | Where it lands in the code, the bake, the variety report, tests, traps | Sections named by the phase |
| `subplans/V0_…` to `V6_…` | One work order per phase: tasks, exit checks, owner checkpoint | The phase you are executing |
| `variety/baseline_stats.py` | The measurement behind section 2 | V0, to reproduce it |
| `variety/concept_geodes.png` | A mock of two geodes and the boon tray painted over a real frame (`variety/make_geode_concept.py` makes it). It shows where things sit and what the numbers say; the shapes are placeholders | V1, before drawing a geode |
| `PLAN.md`, `GAME_DESIGN.md` ("GD"), `TECH_SPEC.md` ("TS") | The game as built. Still binding wherever this pass does not change them | `PLAN.md` 0, 3.3, 5 and 8 before anything else |
| `PROGRESS.md` | What exists, every measured number, every deviation and choice so far | Start of every session; update at the end |
| `../CLAUDE.md` | Every command, the owner's rules as implemented | Start of every session |

---

## 0. Executor briefing

1. **Read first:** `PLAN.md` section 0 (all ten rules still hold), 3.3 (shared devices), 5 (the
   performance contract) and 8 (working agreements); then `PROGRESS.md`; then `CLAUDE.md`; then this.
2. **You cannot see motion, hear sound or feel a level.** Pictures, strips, bot tables and device
   numbers are your evidence; the owner is the judge of fun. Never write that something plays well.
3. **One verb.** The player holds one thumb down and aims one stream. Everything this pass adds is
   something the stream touches or something that comes at the lantern. No tapping on things, no second
   input, no choice screens between levels, no currency, no shop, nothing that persists between levels
   except the level number and the stars.
4. **Every phase leaves a whole game.** At the end of each phase the levels are baked with that phase's
   rules, `scripts\test.ps1`, the flow test and the layout lint pass, and the three phones pass
   `scripts\matrix-android.ps1`. The repository is public and a push is a release (rule 9).
5. **Bots decide balance; the owner decides fun.** VD 12's acceptance lines are the balance contract.
   When a number in VD and a bot table disagree, the table wins and the number is logged in
   `PROGRESS.md`. When the owner and a table disagree, the owner wins and the line is changed.
6. **Variety is measured, then looked at.** VT 6's report is an exit check from V1 on. It can prove
   two levels differ; it cannot prove either is interesting. So every phase that adds levels also ends
   with contact sheets of consecutive levels that you have looked at one by one, and with the owner
   playing.
7. **Small.** Rule 2 of PLAN 0 holds: each new thing is one rule, a few fields of level data, one
   prop, one cue. If a rule needs a paragraph of exceptions, it is the wrong rule. Section 8 lists what
   to cut first.
8. **Originality.** Rule 5 of PLAN 0 holds. The owner's reference is how the genre's advertisements
   play; this pass takes mechanics from the genre, never a name, a look, a character, a sound or a
   level layout from any one game. VD 17 lists our names.
9. **The repository is public** (`rockenrooster/glowtide`). Push only when the owner says so. Every
   push to `main` that changes more than documentation builds and publishes a release, and installed
   builds then show an `UPDATE` button (`RELEASE_PIPELINE.md`). Never commit a key, a token, a
   password, a personal address or a home-network address.
10. **No subagents unless the owner says otherwise.** The owner had version 0.1.0 built by one agent
    coding directly; keep to that.
11. **Read VT 12 before touching a device or a shader.** Ten traps cost the first pass hours; the worst
    was a phone that passed every check while drawing no crowd at all.
12. **PC rules** (PLAN 0 rule 8, `CLAUDE.md`): every run on this PC is silent and off screen and uses
    at most four cores. **Downloads need the owner's yes** in the session; this pass needs none.

---

## 1. What the owner asked

In the owner's words, 2026-11-20, after playing version 0.1.0:

- "are there any powerups/upgrades like [the products of this genre have]? the adding more "shooters"
  and such. will need some rebalancing probably"
- "no i mean that the upgrades are things coming towards you, you have to decide to hit enemies or grab
  the upgrades which re sometimes behind walls with hp you have to destroy"
- "I want level variety and creativity too so there's no stale or repeated levels"
- "make a new plan to add a variety and content pass to be executed by another agent"

So, in order of weight: **upgrades that come down the lane and compete with the enemy for the stream**
(some behind walls with health); **more shooters** among them; **rebalancing** to match; and **levels
that are varied, creative, never stale and never repeated**.

The owner has not yet answered version 0.1.0's own checkpoints (`PROGRESS.md`, "Open questions"): the
ladder's timbre and the music's level, the difficulty's pace, the worlds, the icon and name. Section 6
folds them into this pass's checkpoints.

---

## 2. Where the game stands

Measured on the 300 levels of 2026-11-20 (`python -I -B docs\variety\baseline_stats.py`, from the bake
report `captures\levels\report.md`; re-bake to regenerate that file, VT 5):

| Fact | Number |
|---|---|
| Kinds of level | 1 (flood the spire) |
| Ways a lane is laid out | 1 generator; 8 row patterns; 3 or 4 rows from level 40 on |
| Generated levels (6–300) whose row-pattern string another level also has | 133 of 295 (45%); 13 repeat within ten levels, 38 within thirty |
| First row from level 40 on | A choice of two gates in 85% of levels |
| Target time, every non-boss level from 30 | 36 s |
| Enemy's share (`fight`), every level from 30 | 30% |
| Greedy bot's time ÷ target time, every band | 0.88 to 0.90 |
| Levels on which the greedy bot loses a single core pip | 0 of 295 |
| Where the front line stands for the greedy bot (median lowest gloom, levels 40 on) | `z` 25 to 27 on lanes 36 to 44 long: the top third |
| Upgrades, pickups, anything to choose between besides gates | None |
| After level 300 | Levels 101 to 300 again, unchanged |

In words: from level 40 to 300 every level is statistically the same level, a competent player is never
in danger, and there is one thing to decide (which gate). The first pass's balance work is sound and
stays (the bake, the bots, GD 5.9's contest); what it lacks is anything for that machinery to vary.

What one more shooter would do, measured with the real simulation (the fire rate raised from the first
tick, the enemy and the spire as baked): on levels 12, 50 and 150 a doubled rate ends the level in 62 to
74% of the time and a tripled rate in 39 to 61%. So a second lantern taken early is worth about a third
of a level's length. Boons of that size need the levels sized for them (VD 12.2), or they trivialise
the game; that is the rebalancing the owner expects.

---

## 3. Decisions (proposed by the planner; the owner confirms or changes them at the checkpoints)

| # | Decision | Why |
|---|---|---|
| 1 | Upgrades are **geodes**: crystals with a health number that drift down the lane; pour the stream on one to crack it and take its **boon**. Some carry a shield wall to break first (VD 2) | The owner's second message, in GLOWTIDE's own crystal-and-light vocabulary |
| 2 | A boon lasts **for that level only**. Nothing carries over; no currency; stars still buy nothing | Every level stays a fair fight the bots can balance on its own; the game stays one screen and one thumb |
| 3 | Seven boons: `LANTERN` (one more shooter, up to three), `QUICKEN`, `BRIGHT`, `WARD`, then `KINDLE`, `COMET`, `HUSH` (VD 3) | "More shooters and such": wide, dense, heavy, safe, and three that change the lane or the moment |
| 4 | The choice is real by measurement: taking geodes must pay, ignoring them must cost more as the game goes on, and chasing all of them must not be best (VD 12.2 lines g, h, i and step 8) | "You have to decide to hit enemies or grab the upgrades" |
| 5 | **Six kinds of level**: Siege, Holdout, Colossus, Twins, Festival, Boss with four named bosses (VD 6, 8) | The largest single source of sameness is that every level has one goal |
| 6 | Lanes come from **recipes** (twenty-four or more, each with one idea), a **calendar** that introduces something at least every five levels for the first hundred, hand-made introduction levels and twelve hand-made set pieces (VD 9, 10) | "Variety and creativity" |
| 7 | **No repeats, as a rule the level set must pass**: every level has a fingerprint; likeness limits by distance; no recipe, kind pair or boon list reused within set windows (VD 11, VT 6) | "No stale or repeated levels", made checkable |
| 8 | The enemy gets **plans** and levels get **twists**; today's bursts and brutes stop being in every level (VD 5, 7) | The same two bursts at the same two moments are part of the staleness |
| 9 | **600 baked levels**; from 601 the levels 101–600 return mirrored and five worlds on (VD 16) | Today 301 repeats 101 exactly. An exact repeat then first comes after 1,100 levels |
| 10 | The competent player should be in some danger: on 15 to 40% of a band's levels the greedy bot loses a pip (VD 12.2 step 8) | Today it is never touched; a choice between fighting and upgrading needs a fight |
| 11 | Ten worlds, a signature a world, sky features and per-world music are the **last phase and the first cut** (VD 15) | They dress variety; they are not it |
| 12 | Version **0.2.0** when V1 is accepted; one release a phase the owner accepts | Installed builds learn of each through the updater |

Unchanged on purpose: the controls, the caps (2,400 wisps, 1,000 glooms), the four gameplay colours, the
performance contract, the save's format, levels 1 to 5.

---

## 4. What gets built

| System | New | Specified in |
|---|---|---|
| Geodes | Plain, shielded, choice pair, vault, runner | VD 2 |
| Boons | Seven; up to three lanterns | VD 3 |
| Lane | Hopper and breathing gates, sliding walls, side doors; stretch: snatchers, currents, portals | VD 4 |
| Enemy | Eight plans; runners | VD 5 |
| Kinds | Holdout, Colossus, Twins, Festival beside Siege; four bosses | VD 6, 8 |
| Twists | Geode rain, eclipse, stampede, second wind, golden gate, ward wall, cub; stretch: gate storm | VD 7 |
| Layout | 24+ recipes, three lane lengths, geode placement rules | VD 9 |
| Calendar | Introductions to level 100, a rhythm inside each world, set pieces | VD 10 |
| No repeats | Fingerprint, likeness, the rule; the variety report | VD 11, VT 6 |
| Balance | Bake v2: reference power, reliance, new bots, nine acceptance lines, band checks | VD 12, VT 4, 5 |
| Presentation | Props, juice, sounds, the goal bar, the boon tray, banners, hints | VD 13, 14, VT 7, 8 |
| Dressing | Ten worlds, signatures, props, sky, music a world | VD 15 |
| Tools | The variety report, a device picture check, table version test, new pictures and tests | VT 6, 10 |

---

## 5. Contracts

Three contracts. A phase is not done while one is broken.

1. **Performance** (PLAN 5, unchanged): on the Moto, `bench_flood` after a three-minute soak: at most 5%
   late frames, no frame over 50 ms; no phase raises the late share by more than two points or the main
   thread's CPU P95 by more than 10% against the phase before; zero heap allocations a frame. The bench
   scene grows with the game so it stays the heaviest picture (VT 11). The tablet and the 12R pass
   `matrix-android.ps1`.
2. **Balance** (VD 12.2): every baked level passes acceptance lines a to i, and every band of ten
   passes step 8, with the numbers as tuned and recorded in `PROGRESS.md`.
3. **Variety** (VD 11, VT 6): `glowtide_headless --sim variety` exits 0 on the committed level table.
   Its weights and thresholds are set in V4 and recorded.

---

## 6. Phases

Sizes are relative. Each phase ends with: the three contracts, `PROGRESS.md` updated, a short note to
the owner saying what to try, and its checkpoint. While waiting for the owner, go on with work that does
not depend on the answer.

| Phase | Size | Needs | Outcome | Subplan |
|---|---|---|---|---|
| V0 Baseline and tools | S | — | The variety report, the device picture check and the table-version test exist; the baseline is measured with them; nothing in the game changes | `subplans/V0_baseline_and_tools.md` |
| V1 Geodes and boons — **gate** | L | V0 | Geodes with `LANTERN`, `QUICKEN`, `BRIGHT`, `WARD` in the game as it is: placed by simple rules in today's levels, balanced by bake v2, drawn, heard, measured | `subplans/V1_geodes_and_boons.md` |
| V2 Lane and enemy | L | V1 | The new gates and walls, side doors, runners, the eight plans, `KINDLE`, `COMET`, `HUSH`; stretch elements if time allows | `subplans/V2_lane_and_enemy.md` |
| V3 Kinds, twists, bosses | L | V2 | Holdout, Colossus, Twins, Festival; the twists; four bosses; each with its goal bar, result and bot rules | `subplans/V3_kinds_twists_bosses.md` |
| V4 Recipes, calendar, no repeats — **gate** | L | V3 | The recipe catalogue, the calendar, hand-made introductions and set pieces, the no-repeat rule in the bake, 600 levels | `subplans/V4_recipes_calendar_bake.md` |
| V5 Worlds and dressing | M | V4 | Ten worlds with signatures, props, sky features, music a world | `subplans/V5_worlds_and_dressing.md` |
| V6 Balance, polish, release | M | V4 (V5 if built) | The final bake and band tables, every check on three devices, the soak, version 0.2.x released | `subplans/V6_balance_polish_release.md` |

Order: V0 → V1 → V2 → V3 → V4 → (V5) → V6. One writer; no parallel phases (they all touch
`src/sim`).

Gates and checkpoints:

- **V1 is the gate for the idea.** The owner plays levels 6 to 20 with geodes on a phone. Is this the
  upgrade they meant? Do they find themselves choosing? Is three lanterns fun? If not, change VD 2 and 3
  with them before building anything on top. Also ask here what version 0.1.0 left open: the ladder's
  timbre, the music's level.
- **V3 checkpoint:** one level of each kind and each boss. Which kinds stay, which go?
- **V4 is the gate for variety.** The owner plays thirty consecutive levels at three places (1–30,
  100–130, 300–330) with the dev build's level stepper. Stale anywhere? Any level unfair or dull (its
  number is enough)? Right pace? This also answers version 0.1.0's difficulty question.
- **V5 checkpoint:** the ten worlds' sheets and a level in each; the music per world on a phone.
- **V6:** sign-off, then the release.

---

## 7. Working agreements (on top of PLAN 8)

- **Bake after every rule change.** It takes minutes. Bump `kRulesVersion` or `kGeneratorVersion`
  (VT 2) with the change, bake, read the band table, then commit code and table together.
- **A new thing arrives complete or not at all:** its rule and unit test, its place in the bot, its
  prop and label, its cue and haptic, its picture, its hint if it has one, its line in the fingerprint.
  A half-built element in a baked level is a bug in a released game.
- **Hand-made levels are data** in `levels_hand.cpp` with a one-line idea each; the bots and a
  thumbnail check each one; their ideas are listed for the owner in `PROGRESS.md`.
- **Look at consecutive levels.** Before every owner checkpoint that involves levels, render thirty
  consecutive levels from above at three places and look at each sheet for sameness the report cannot
  see (the same shape with other numbers; three levels in a row that play left-to-right).
- **Devices:** the Moto, the tablet and the 12R are all reachable over wireless debugging when it is
  switched on (the owner switches it on; it switches itself off on reboot). `matrix-android.ps1` keeps
  a phone awake. Leave each phone with the current dev build and no test packages.
- **Commits** at working checkpoints with plain messages; `git` through `Invoke-GtGit`; push on the
  owner's word only (briefing rule 9). A release's notes are its commit subjects: write them for a
  player to read.
- **Version:** `versionName` in `android/app/build.gradle` and `project(... VERSION)` in
  `CMakeLists.txt`, kept equal: 0.2.0 at V1's acceptance.

---

## 8. Risks, and what to cut

| # | Risk | Level | Mitigation |
|---|---|---|---|
| 1 | Geodes are not the upgrade the owner pictured, or the choice is not felt | High | V1 is small on purpose and ends with the owner playing before anything depends on it. Speeds, health and worth are tweakables |
| 2 | Boons swing a level so far that balance breaks: trivial with them, hopeless without | High | VD 12.2: the reference power `R` and the reliance `m`; the budget of VD 9.3; lines g and h are measured per level; `podless` and `hungry` are run on every level |
| 3 | Variety on paper, sameness in play: the rule passes and levels still feel alike | High | Recipes must have an idea a player could say; consecutive-level sheets; the V4 gate is the owner playing ninety levels |
| 4 | Scope: six kinds, eight plans, eight twists, twenty-four recipes, forty hand-made levels | High | Cut order below. Each phase ships alone |
| 5 | The contest's sharp edge (GD 5.9) makes Holdout and Colossus swingy: safe or collapsing | Medium | Their own acceptance lines (median core 7 to 9; time 0.7 to 1.2 `T`); `SECOND WIND`; if a kind cannot be made to pass on eight seeds a level, drop the kind, not the line |
| 6 | The bot is a poor stand-in for a person choosing between a geode and the enemy | Medium | Three bots bracket the person (`podless`, `greedy`, `hungry`); the owner's play at V1 and V4 calibrates which lines to move |
| 7 | A new shader path breaks on one GPU as `unitRadius` did | Medium | No new shaders are needed; VT 12 trap 1; the device picture check every phase |
| 8 | The bake grows slow as bots and levels multiply | Low | Budget in VT 5; pass 1 is single-threaded and cheap; profile before adding runs |
| 9 | Readability: more things with numbers on a small lane | Medium | Labels still win (GD 9.4); at most four geodes alive outside a festival; greyscale and smallest-frame checks in VT 10 |
| 10 | Photosensitivity from new effects | Safety | GD 14's rule, measured by `flash-audit.ps1` on every new effect |

**Cut order when time is short** (cut from the top; never cut a contract): V5 whole; the stretch
elements (portals, currents, snatchers, gate storm); `HUSH` and `COMET`; Twins; set pieces from twelve
to six; plans from eight to five; recipes from twenty-four to sixteen; 600 levels to 400. **Never cut:**
geodes with `LANTERN`, the no-repeat rule, the acceptance lines, the device checks.

---

## 9. Assumed defaults (the owner can change any of these)

- The on-screen word for an upgrade pod is `GEODE`; the upgrade is a `BOON`.
- Boons end with their level. A festival's boons end with the festival.
- Levels 1 to 5 stay as they are; geodes arrive on level 6.
- Every level ending in 5 is a festival and every level ending in 0 a boss.
- A player's saved level number and stars are kept across the re-bake, though level N is a new level.
- Hold-to-fire and `AUTO-FIRE` are unchanged; nothing new needs a tap.
- English only; strings in the one table.

---

## 10. Out of scope

Anything that persists between levels beyond the level number and stars: currencies, shops, permanent
upgrades, unlock trees, daily rewards, lives, timers. Advertisements, purchases, accounts, leaderboards,
cloud saves, multiplayer. A level editor. New inputs (taps, swipes, tilt). Quality tiers or graphics
settings. iOS, localisation, gamepad support. A lighter build for devices below the Moto. The Play
store's own steps (`RELEASE_CHECKLIST.md`).

One line of the first plan changes. PLAN 10 put "upgrades or currencies" out of scope for version 1,
and PLAN 1 says "no upgrades to grind". On the owner's word (section 1), upgrades that are won and
spent inside one level are now in. The rest stands: nothing to grind, nothing kept, nothing to buy.

---

## 11. Sources used while planning

- The repository at commit `7990557` (2026-11-20): `src/sim/level.h`, `levels.cpp`, `world.h`,
  `world.cpp`, `bots.cpp`, `bake.cpp`; `src/game/view.h`, `view.cpp`, `hud.cpp`, `screens.cpp`;
  `src/gfx/renderer.h`, `camera.cpp`; the tests and scripts. Facts quoted from them are in VT.
- The bake report of that commit's level table, through `docs/variety/baseline_stats.py` (section 2).
- Thirty-six runs of `glowtide_headless --sim run --bot greedy --seeds 3 --set fireRate=…` (12, 18, 24
  and 36 shots a second on levels 12, 50 and 150; section 2's last paragraph).
- No experiment was run with geodes, kinds, plans or recipes: none exists yet. Every number about them
  in VD is a reasoned starting value, to be replaced by measurement.

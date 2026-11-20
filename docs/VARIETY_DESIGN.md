# GLOWTIDE — Variety and Content: Design

What the variety pass adds to the game, as rules and numbers. `VARIETY_PLAN.md` says why and in what
order; `VARIETY_TECH.md` says where it goes in the code. `GAME_DESIGN.md` still holds for everything this
document does not change; section numbers written as "GD 5.9" point there.

Every number here is a starting value. Two things override it: the bots (section 12's acceptance lines
are the balance contract) and the owner's play (the checkpoints of `VARIETY_PLAN.md` 6). Log each change
in `PROGRESS.md`.

---

## 1. The idea

Today a level is one situation: rows of gates, a stream from the top, flood the spire. From level 40 to
300 nothing about that changes (`VARIETY_PLAN.md` 2). The pass changes it along five lines, all of which
keep the one verb the game has, **aim the stream**:

1. **Upgrades come down the lane.** A *geode* is a crystal with a health number that drifts toward the
   lantern. Pour the stream on it and it cracks and gives a *boon* for the rest of the level: another
   lantern, a faster or heavier shot, stronger gates, a healed core. Some have a wall to break first.
   Every wisp that cracks a geode is a wisp that did not fight, so each one is a decision.
2. **Levels have kinds.** Flood the spire, hold until dawn, stop a colossus that walks at you, bring down
   two spires, or a short festival where nothing can be lost.
3. **Lanes are composed.** Twenty-odd layout *recipes*, each with one idea, instead of one row
   generator; new things the lane can hold; three lane lengths.
4. **The enemy has a plan** (pulses, pincers, a late surge, runners, side doors) and levels have
   *twists*: a named moment in the middle (an eclipse, a rain of geodes, a golden gate).
5. **Nothing repeats.** A calendar brings something new every few levels for the first hundred; a
   fingerprint rule keeps every level unlike its neighbours; hand-made set pieces mark the milestones.

What does not change: one thumb, hold and drag; a level is 20 to 50 seconds; no currency, shop, meta
progression, timers or lives; the four gameplay colours and what they mean (GD 9.1); the performance
contract (PLAN 5).

---

## 2. Geodes

A geode is the upgrade pod the owner described: a thing coming toward you that you choose to shoot.
`variety/concept_geodes.png` is a mock of it over a real frame of level 25: a plain geode with its
health, a shielded one with its plate's health, the boon tray. Take the layout from it, not the shapes.

### 2.1 Rules

- A level holds up to 8 geodes (16 in a festival). Each has a place and a time to appear, a speed, an
  optional sideways slide (the gate's eased wave, GD 5.4), a **health** number, an optional **shield**
  and one boon.
- It appears at its time at (`x`, `z0`), `z0` = `LEN` − 3 unless the recipe says otherwise, and drifts
  down the lane at its speed (1.2 to 2.0 m/s; glooms walk at 3.2). A body of radius 0.9 m that floats
  above the lane: it does not touch gates, barriers, bumpers or glooms. Glooms walk through it.
- **A wisp that touches it gives up to its power**, exactly like a barrier (GD 5.6): the geode's health
  drops by that much, the wisp's power too, a wisp at zero dies, a wisp with power left carries on.
- **Shield.** A shielded geode carries a plate 1.2 m in front of it (on the lantern's side), 3.0 m wide,
  with its own health number. The plate is a barrier that moves with its geode. While the plate stands,
  the geode takes nothing, from any side.
- At zero health the geode **opens**: its boon applies in that tick (section 3) and it is gone.
- A geode that reaches `z` ≤ 2.5 unopened **slips past**: it is gone, with no penalty.
- Wisps spent on geodes and plates count as neither delivered nor cancelled; the simulation keeps the
  total (`spent`) for the bake and the result screen.

### 2.2 Variants

| Variant | What differs | From level |
|---|---|---|
| Plain | — | 6 |
| Shielded | A plate in front | 11 |
| Choice pair | Two geodes side by side (3 m apart) with different boons; opening one dissolves the other | 26 |
| Vault | Anchored (speed 0) at a lane side behind a static barrier (GD 5.6); leaves at 80% of the level's time | 56 |
| Runner | Small (radius 0.6), 3.0 m/s, low health; comes in threes (the GEODE RAIN twist, the festival) | 43 |

### 2.3 Why it is a decision (the executor must keep these true)

- **Power spent is power not delivered.** A geode costs its health in wisps wherever it is hit.
- **It gets dearer as it comes.** Far up the lane the stream that reaches it has been multiplied by
  every gate below it, so cracking it takes little time. Below the gates only raw shots reach it, and
  each raw shot absorbed there would have been worth the whole chain's product at the top. A geode with
  health 87 on level 5 (5% of that level's power) is about a second of the full stream at the top and
  over seven seconds of raw fire at the bottom.
- **The enemy is in the way.** The far end is where the front line is: glooms cancel the wisps before
  they reach a far geode. A player who is losing the line cannot reach it until it has drifted below
  the front, where it is dear.
- **It is usually off the best line.** The generator puts at least half of a level's geodes two metres
  or more from the column of the best gates (9.3), so aiming at one gives up multiplication and leaves
  the enemy's column unanswered while the stream is away. Glooms drift toward the lantern (GD 5.9), so
  they follow, late.
- **Early is worth more.** A lasting boon pays for the rest of the level; the same geode at 70% of the
  level's time is worth a third of what it was at 10%.

### 2.4 Health

The bake sets every geode's health as a share of the level's reference power `R` (12.2), rounded to two
significant figures like the spire's:

| Part | Share of `R` |
|---|---|
| Plain geode | 4 to 6% |
| Shielded: plate / geode | 6 to 9% / 2 to 3% |
| Vault: barrier / geode | twice GD 7.3's barrier rule / 2% |
| Runner | 1.5 to 2% |
| Each geode of a choice pair | 4 to 5% |

---

## 3. Boons

A boon lasts for the level it was won in. Nothing carries to the next level; stars still buy nothing.

| Boon | Kind | Effect | Limit | From level |
|---|---|---|---|---|
| `LANTERN` | Lasting | One more lantern (3.1) | 3 lanterns | 6 |
| `QUICKEN` | Lasting | Fire rate + 25% of the base rate | 3 (21 shots a second) | 7 |
| `BRIGHT` | Lasting | Every fired wisp has 1 more power | 2 (power 3) | 12 |
| `WARD` | Instant | The core is back at 10; every gloom below `z` = 14 pops | — | 23 |
| `KINDLE` | Lasting | Every multiply gate counts one more (×2 becomes ×3; at most ×6) | 1 | 36 |
| `COMET` | Instant | Each lantern fires one comet: a wisp with the power of 4 s of that lantern's fire | — | 52 |
| `HUSH` | 6 s | The emitters send nothing | — | 63 |

A boon at its limit is not offered again in that level (the generator knows the limits). The first four
are phase V1; `KINDLE`, `COMET` and `HUSH` are phase V2.

What a boon is worth to the stream (the generator's budget, 9.3): `LANTERN` ×2 then ×1.5; `QUICKEN`
×1.25, ×1.2, ×1.17; `BRIGHT` ×2 then ×1.5; `KINDLE` the product of (`k` + 1) / `k` over the best line's
gates (×2.4 for three ×3 gates, ×1.7 for three ×5). They differ in what the stream becomes, not only in
how much: lanterns make it wide, `QUICKEN` dense, `BRIGHT` heavy (a wisp of power 3 clashes three times
before it dies, breaks walls three times as fast and still counts as one unit under the cap), `KINDLE`
changes the lane.

### 3.1 More lanterns

- Lanterns stand in a row 0.9 m apart, centred on the aim point (the launcher's `x`). Three lanterns
  span 1.8 m, which still fits the narrowest gate (half-width 1.2).
- All fire on the same tick from one shot counter: a shot is one wisp from each lantern.
- The aim point's range shrinks so no lantern leaves its rail: `|x|` ≤ 4.4 − 0.45 × (lanterns − 1).
- Only the first lantern carries the core's pips. A gloom that reaches `z` ≤ 0.8 anywhere hits the core,
  as now.
- The side lanterns are drawn at 70% size so the base's ring of pips stays readable.

### 3.2 Caps

The crowd caps do not move (GD 5.3). Three lanterns at 21 shots a second with `BRIGHT` put more power on
the lane, not more units: past the cap gates add power to the wisp that passed and shots join the last
wisp fired. So the heaviest picture is still `bench_flood`'s.

---

## 4. New things on the lane

Each is one small rule. "Must" items are phase V2; "stretch" items are built only if V2's must items
are done and inside the contract, in the order listed.

| # | Element | Rule | Seen as | Tier |
|---|---|---|---|---|
| 1 | Hopper gate | A gate that jumps between two places: `cx0` ± `amp`, staying 1.5 to 2.5 s, crossing in 0.25 s | A gate with a double arrow; a faint ghost frame at the other place | Must |
| 2 | Breathing gate | Its half-width swings between 60% and 120% of `hw`, period 3 to 5 s. A wisp passes if it is inside the opening as it is in that tick | The posts move in and out | Must |
| 3 | Sliding wall | A barrier whose span slides sideways (the gate's wave) | The barrier, moving | Must |
| 4 | Side door | An emitter on a lane edge at `z` between 14 and `LEN` − 8. Its glooms step in sideways (1.5 m/s, fading in 1 s) and then walk down | A dark arch on the rail with a rose glow | Must |
| 5 | Runner | A gloom with the `runner` mark: 5.0 m/s, power 1, comes in packs | A small gloom leaning forward with a streak | Must |
| 6 | Snatcher | A gloom with the `snatcher` mark: it steers at the nearest geode (up to 2.5 m/s sideways); if it touches one the geode is lost and the snatcher dies | A gloom with a rose halo and reaching horns | Stretch 1 |
| 7 | Current | A marked stretch of lane (4 to 8 m long, 3 to 5 m wide) that carries wisps sideways at 1.0 to 2.0 m/s while they are in it. Glooms are not carried | Chevrons flowing across the lane | Stretch 2 |
| 8 | Portal pair | A wisp that passes ring A (half-width 1.2) comes out of ring B, further up the lane, keeping its offset from the centre; once per wisp. Glooms ignore it | Two rings of one colour with a thread of motes between them | Stretch 3 |

Rules for all of them: no object overlaps another; GD 5.7's gap rule holds; every level has at least one
multiply gate a straight stream can reach from a place the launcher can stand; a hopper or a breathing
gate is never also a slider.

---

## 5. The enemy

### 5.1 Plans

The bake still sizes the enemy's average flow (12.2). A **plan** shapes it in time and place. The
average over the level's target time is the same for every plan, so the bake's arithmetic holds.

| Plan | Shape | From level |
|---|---|---|
| Steady | Today's: a constant flow | 2 |
| Pulse | 2.5 s at 1.8 times the flow, 2.5 s at 0.2 times | 18 |
| Pincer | Two emitters far apart; one at a time, swapping every 4 s | 22 |
| Late surge | 0.6 times in the first half, 1.4 times in the second | 27 |
| Early rush | 1.6 times for the first 8 s, then 0.85 times | 34 |
| Flank | A third of the flow comes through one or two side doors (4) | 28 |
| Brute line | Half the flow is brutes, one every 4 s (GD 5.9) | 37 |
| Runner packs | Every 6 s a pack of 8 to 14 runners (4), taken out of the flow | 30 |

Emitters still stand at the spire's foot unless a plan or a recipe moves them. GD 5.9's pressure ramp
past 1.5 `T` stays.

### 5.2 Brutes and the two time bursts

Today every level from 6 has bursts at 35% and 70% of `T`, and from 8 a brute every 8 s. Both become a
matter of the plan and the twists: a level has them only if its plan or a twist says so. That alone
removes a beat every level shares.

---

## 6. Kinds

A level's kind is its goal. The HUD's top bar always shows the goal's progress, and the level's first
second shows the goal as a banner (14).

| Kind | Banner | Win | Lose | Stars | Share of levels |
|---|---|---|---|---|---|
| Siege | — | The spire's health reaches zero (today's rule) | Core at zero | Time: ★★★ within `T`, ★★ within 1.3 `T` | About 55% |
| Holdout | `HOLD UNTIL DAWN` | The clock reaches `T` with the core above zero | Core at zero | Core left: ★★★ 8 or more, ★★ 5 or more | About 10% |
| Colossus | `STOP THE COLOSSUS` | The colossus's health reaches zero | It reaches the lantern (`z` ≤ 3), or core at zero | Time, as Siege | About 8% |
| Twins | `TWO SPIRES` | Both spires at zero | Core at zero | Time, as Siege | About 7% |
| Festival | `LANTERN FESTIVAL` | Always, when its 20 s end | Cannot be lost | Geodes opened: ★★★ 85% of them, ★★ 60% | Every level ending in 5 |
| Boss | The boss's name | As Siege | Core at zero | Time | Every level ending in 0 |

- **Holdout.** No spire health. The spire's place is a dark gate the glooms pour from. Wisps that reach
  the top are simply gone. The enemy grows through the level (the late surge shape on top of its plan).
  The backdrop brightens from night to dawn as the clock runs, so the picture is the progress bar.
- **Colossus.** No spire. A colossus appears at `z` = `LEN` − 2 three seconds in: a giant gloom, radius
  1.8 m, with the health a spire would have. It absorbs every wisp that touches it, like a geode. It
  walks down at (`LEN` − 6) / (1.25 `T`) m/s, swaying 1.5 m to each side, so a level that is not won by
  about 1.25 `T` is lost. Emitters run as usual. It is the spire that comes to you: the nearer it gets,
  the fewer gates lie between the lantern and it, and the weaker the stream that hits it.
- **Twins.** Two spires at `x` = −2.6 and +2.6, each with half the health (or 40% and 60%). A wisp at
  the top damages the spire on its side of the lane's middle; the funnel (GD 5.10) steers toward the
  nearer living spire. Each spire has its own emitters, and they stop when it falls. Wisps that arrive
  on a fallen spire's side are wasted, so the stream must cross over.
- **Festival.** No enemy. Rich gates, 10 to 14 geodes with small health raining down, a paper-lantern
  target at the top that soaks up the flood for show. It is the breather after four levels and the
  place where three lanterns and a kindled lane are simply fun. Its boons end with it.
- No kind other than Siege comes twice in a row.

---

## 7. Twists

A twist is a named moment in the middle of a level, started by the clock (a fraction of `T`), by the
goal's progress (the spire, the colossus or the dawn clock at a fraction) or by the core. A level has
none, one or two; a boss has two or three. Each shows a banner for 900 ms and has a sound.

| Twist | What happens | Tier |
|---|---|---|
| `SURGE` | Today's burst: twice the flow for 3 s (GD 5.9) | Exists |
| `BRUTE GUARD` | Two brutes at once (today's boss burst) | Exists |
| `GEODE RAIN` | Three runner geodes at once, boons from the level's pool | Must |
| `ECLIPSE` | 6 s: the picture loses 35% of its colour and the backdrop dims; glooms walk 30% faster | Must |
| `STAMPEDE` | A pack of 16 runners from one emitter over 2 s | Must |
| `SECOND WIND` | Once a level, when the core falls to 3 or less: a `WARD` geode with small health appears mid-lane | Must |
| `GOLDEN GATE` | 8 s: the strongest gate of the middle row counts two more and is drawn gold | Must |
| `GATE STORM` | 8 s: every still multiply gate slides (1.5 m, period 2.5 s) and sliders move twice as fast | Stretch |
| `WARD WALL` | A wall across the whole lane rises 6 m below the spire, health 6% of the spire's | Must (bosses) |
| `CUB` | A small colossus (health 5% of the spire's) starts down the lane | Must (bosses) |

`ECLIPSE` obeys GD 14: its change of brightness takes 0.5 s each way, and `REDUCE EFFECTS` halves it.

---

## 8. Bosses

Every tenth level is a Siege against a named boss spire, broader and crowned (GD 7.5), with a target
time of 1.4 `T`. What makes one boss unlike the next is its script: twists at 66% and 33% of its health.

| Boss | At 66% and at 33% | Its plan |
|---|---|---|
| `THE WARDEN` | `WARD WALL` and `BRUTE GUARD` | Steady |
| `THE TEMPEST` | `STAMPEDE` and `ECLIPSE` | Runner packs |
| `THE HYDRA` | `CUB` (two at 33%) | Pulse |
| `THE OLD SPIRE` | `SURGE` and `BRUTE GUARD` (today's boss) | Steady |

They rotate, starting with `THE OLD SPIRE` on level 10 so the first boss is the one already built.
Every boss level has at least two geodes before its first threshold.

---

## 9. How a level is put together

### 9.1 Length

| Class | `LEN` | Rows | `T` at level 30 and after |
|---|---|---|---|
| Sprint | 30 | 2 | 22 s |
| Standard | 36 to 40 | 3 to 4 | 32 s |
| Long | 44 | 4 to 5 | 44 s |

`T` grows as now until level 30: the class's time × (0.8 + 0.2 × min(level, 30) / 30). Holdout uses 40
s, Colossus the class's time, Twins 1.2 times it, Festival 20 s, Boss 1.4 times the Standard time.
Today's rule that stops at four rows (the generator breaks at `z` > 32) goes: a Long lane has room for
five. The camera is proven for `LEN` 30 to 44 in every frame (`testCameraFit`); no length outside that.

### 9.2 Recipes

A recipe is a way of laying out a lane with one idea a player could say in a sentence. It takes the
level number, a random stream and the level's difficulty, and fills the level's data. Asymmetric recipes
have a mirrored twin, chosen by the seed.

| # | Recipe | The idea | How it is built |
|---|---|---|---|
| 1 | Terraces | Rows of gates, pick the best of each | Today's generator (GD 7.3) with the new gates of section 4 among its patterns |
| 2 | Funnel | Aim finer as the stream climbs | One column. Row 1 ×2 (half-width 1.8), row 2 ×3 (1.5), row 3 ×4 or ×5 (1.2), each centre up to 0.6 m off the one below; ÷2 gates on both sides of the top one |
| 3 | Twin rivers | Commit to a side, cross when you must | Four or five bumpers down the middle from `z` 10 to 26. Each side has its own chain of three gates with unequal products; the stronger side has a ÷ or a wall in it. One emitter a side, unequal shares |
| 4 | Checker | One clean column, one rich and dirty | Three columns (`x` −3.2, 0, 3.2), three or four rows, each cell ×, ÷ or empty: exactly one column is all ×; another has a higher product with one ÷ in it |
| 5 | The vault | Pay to open the jackpot | A ×5 at the top row behind a wall worth 4 s of the stream; a side line of ×2, ×2; one geode behind a second, smaller wall |
| 6 | Toll road | Find the thin wall | Two walls across the whole lane (`z` 12 and 22), each in three spans with health in the ratio 1 : 2 : 4, shuffled; gates between and after |
| 7 | Pendulum | Ride the wave | Every gate slides with one period; each row a third of a period behind the one below |
| 8 | Stagger | Odd rows or even rows | A Long lane; five rows with one narrow gate each, alternating between `x` −1.6 and +1.6: a straight stream takes rows 1, 3, 5 or rows 2, 4, with different products |
| 9 | Islands | Thread the channels | Two or three clusters of three bumpers; a gate in each channel between and beside them |
| 10 | Crossfire | They come from the sides | Gates down the middle; side doors at two heights on opposite sides (plan Flank); a `WARD` geode mid-level |
| 11 | The breach | Fight for the rich side or farm the quiet one | Every emitter on one side at 1.5 times its share; the best gates on that side; weaker gates and a geode on the other |
| 12 | Caravan | Three gifts in a row, on the wrong side | Standard rows; three geodes in one column 6 m apart on the side away from the best gates |
| 13 | Crossroads | This boon or that one | Standard rows; two choice pairs, at 20% and 55% of the time |
| 14 | Treasure walls | Something locked on each side | Gates in the middle; a vault on each side at `z` 20 |
| 15 | Hopscotch | The gate will not stay | One hopper gate a row, nothing else in the row |
| 16 | The narrows | A chokepoint | Bumpers and walls leave one 3 m gap mid-lane; a strong gate right behind it; glooms and wisps meet in the gap |
| 17 | Dawn line (Holdout) | Hold a line across the lane | Two rows of wide gates low on the lane (`z` 7 and 13) so the flood is wide where the glooms arrive; geodes often |
| 18 | The long walk (Colossus) | Grow the stream before it arrives | A Long lane, four rows evenly spaced, the strongest gates lowest |
| 19 | Two towers (Twins) | Which spire first | A mirrored pair of gate chains, one side stronger; the weaker side's spire has the stronger emitters |
| 20 | Festival field | Everything at once | Three rows of wide strong gates, the geode rain |

Phase V4 builds these twenty and at least four more of the executor's own. A recipe is accepted into
the catalogue when it has: its sentence; thumbnails of twenty seeds that pass the generator's lints
(4, and GD 7.3's rules); and bot runs showing the greedy bot's best line is not the same place on more
than half of those seeds. A recipe that produces levels the bake keeps rejecting is changed, not forced.

### 9.3 Where geodes go

- Levels 6 to 20: one or two geodes a level. 21 to 60: two or three. After 60: two to four. A festival
  has its own rain; a boss at least two before 66%.
- **Budget.** The product of the worth (section 3) of a level's lasting boons is at most 2.0 up to level
  20, 2.5 up to 60, 3.0 after.
- **Timing.** The first geode appears between 8% and 25% of `T`; the others at least 5 s apart; none
  after 75% of `T` except a `WARD` or a twist's.
- **At once.** Outside a festival, never more than four geodes are on the lane together, a twist's rain
  included (a choice pair counts as two). With more numbers than that the lane stops being readable;
  the generator moves a geode's time before it breaks this.
- **Place.** At least half of a level's geodes (rounded up) are detours: at the height of the middle
  gate row their centre is 2 m or more from the centre of the best line. The rest ride the best line or
  hang behind the weaker gate.
- **Pool.** A level offers at most three different boons; the first geode of a level from 6 to 60 is a
  lasting boon.

---

## 10. The calendar

### 10.1 The first hundred levels

Something the player has not seen arrives at least every five levels, often sooner. The level that
introduces a thing is laid out by hand, the way levels 1 to 5 are (GD 7.2): the smallest lane that
shows it, one hint line.

| Level | New | Level | New |
|---|---|---|---|
| 1–5 | As today | 46 | Sliding walls; recipe Toll road |
| 6 | Geodes, `LANTERN` | 48 | Twist `ECLIPSE` |
| 7 | `QUICKEN` | 49 | Set piece 1 (10.3) |
| 10 | Boss `THE OLD SPIRE` | 52 | `COMET` |
| 11 | Shielded geodes | 56 | Vaults; recipe Treasure walls |
| 12 | `BRIGHT` | 58 | Recipe Checker |
| 15 | The first festival | 61 | A Long lane; recipe Stagger |
| 16 | Hopper gates | 63 | `HUSH` |
| 18 | Plan Pulse | 66 | Twist `GOLDEN GATE` |
| 20 | Boss `THE WARDEN` | 68 | Recipe Pendulum |
| 21 | Kind Holdout | 72 | Recipe The vault |
| 23 | `WARD`; twist `SECOND WIND` from here on | 76 | Snatchers (stretch) |
| 26 | Choice pairs; recipe Crossroads | 78 | Recipe The narrows |
| 28 | Side doors; plan Flank; recipe Crossfire | 83 | Currents (stretch) |
| 30 | Boss `THE TEMPEST`: runners | 86 | Recipe Islands |
| 31 | Kind Colossus | 88 | Twist `GATE STORM` (stretch) |
| 33 | Recipe Twin rivers | 91 | Portals (stretch) |
| 36 | `KINDLE` | 93 | Recipe The breach |
| 38 | Breathing gates | 96 | Recipe Caravan with shields |
| 40 | Boss `THE HYDRA` | 99 | Set piece 2 |
| 41 | Kind Twins | | |
| 43 | Twist `GEODE RAIN`; runner geodes | | |

The plans the table does not name arrive at 5.1's levels, on generated levels, without an introduction
of their own. A level number may move by three either way if the bake or a recipe needs it, as long as
the order holds and no gap passes five levels. A stretch item that is not built leaves its slot to a
hand-made level that combines two things already introduced.

### 10.2 Rhythm inside a world

A world is still ten levels (GD 7.5). Its levels have roles, and a level's **hardness** `h` comes from
its role:

| Place in the world | Role | `h` |
|---|---|---|
| 1 | The world's opening: its signature (15) in an easy lane | 0.2 |
| 2, 3, 4 | Rising | 0.4, 0.5, 0.6 |
| 5 | Festival | — |
| 6, 7, 8 | Rising again | 0.5, 0.65, 0.8 |
| 9 | The hard one | 1.0 |
| 10 | Boss | 0.8 |

`h` moves three numbers: `fight` (GD 6) by −4 to +4 points across `h` 0 to 1; the reliance on boons
(12.2) by −0.15 to +0.15; and whether a twist is allowed (none below 0.4, two from 0.8). Sprint lanes
sit at places 1 and 6, Long lanes at 4, 8 or 9.

### 10.3 Set pieces

Twelve levels are laid out wholly by hand as show pieces: the hard level before every fiftieth, so 49,
99, 149 and on to 599 (besides levels 1 to 5 and the introduction levels of 10.1, which are hand-made
too). A set piece combines things in a way no recipe does, has a name on its `LEVEL N` chip for its
first second (`LEVEL 99  THE LONG DARK`), and is baked like any other level. The executor writes them
as data, checks each with the bots and a thumbnail, and lists their one-line ideas in `PROGRESS.md` for
the owner.

### 10.4 After level 100

No new rules arrive. Variety comes from the catalogue: recipes × kinds × plans × twists × worlds ×
lengths × boon pools, under section 11's rule, with a set piece every 50 levels.

---

## 11. No repeats

Every level has a **fingerprint** made from its data: its kind, recipe, plan, length class, twists, the
set of boons it offers, and a coarse map of what stands where (12 bands along the lane × 8 across, each
cell holding the strongest thing in it: multiply gate by factor, divide gate, wall, bumper, snuffer,
geode, side door, current, portal). The **likeness** of two levels is a number from 0 to 1
(`VARIETY_TECH.md` 6 has the weights).

The rule the level set must pass:

| Two levels this far apart | May be alike by at most |
|---|---|
| 1 to 12 | 0.45 |
| 13 to 40 | 0.60 |
| Anywhere | 0.85 |

And three counts: no recipe twice within 6 levels; no (recipe, kind) pair twice within 30; no boon pool
(the same set of boons in the same order) twice within 8. The generator picks each level's seed so the
rule holds against every earlier level, before the bake balances it (12.2). The rule and the counts are
printed by the variety report (`VARIETY_TECH.md` 6) and are an exit check of every phase from V1 on.

---

## 12. Balance

### 12.1 Bots

| Bot | Behaviour | Stands for |
|---|---|---|
| `greedy` | Today's (GD 8), and it values geodes: a geode on a line costs its health ÷ the stream's power there (seconds, as a barrier does) and is worth its boon's gain over the time left (for `WARD`: the pips missing and the glooms it would clear) | The competent player |
| `podless` | `greedy` that gives a geode no worth: a geode on its line is only a wall | A player who ignores upgrades |
| `hungry` | Aims at the nearest geode whenever one is on the lane; otherwise `greedy` | A player who chases every upgrade |
| `naive`, `sweep`, `idle` | As today | As today |

(`podless` keeps its name from the planning talk: the owner and the planner called geodes pods.)

### 12.2 The bake, per level

`R`, the level's reference power, replaces `P0` × `T` (GD 7.4) wherever a size is derived.

1. The layout comes from the level's seed (section 11 chose it).
2. **Two outputs, no enemy, an unbreakable goal, `T` seconds:** `Pa` = the power `greedy` delivers,
   `Pn` = the power `podless` delivers. Geode health for this step is 5% of `Pn` each (a first guess).
3. **Reliance.** `m` = clamp((level − 10) / 70, 0, 0.8) + (`h` − 0.5) × 0.3, kept in 0 to 0.9.
   `R` = `Pn` + `m` × (`Pa` − `Pn`). With `m` = 0 the level is sized for a player who ignores geodes
   and boons are a bonus; with `m` near 1 it is sized for one who takes them.
4. **Geode health** from `R` (2.4). Then step 2 again with the real health.
5. **The enemy.** Base flow = `fight` × `R` / `T` × `rateScale`. The share the enemy should cancel of
   what `greedy` itself produces is therefore `fight` × `R` / `Pa`: lower than `fight` when the level
   leans little on boons. Bisect `rateScale` (GD 7.4) until `greedy`'s cancelled share (cancelled ÷
   produced, measured as today, enemy on, goal unbreakable, `T` seconds) is within 5 points of that.
6. **The goal.** Siege, Twins, Colossus, Boss: goal health = 0.85 × the power `greedy` delivered to it
   in that run. Holdout: instead of step 5's target, bisect the flow until `greedy`'s median core at
   dawn is 7 to 9. Festival: no enemy; the three-star count is `greedy`'s median geodes opened less one.
7. **Accept** (ten `greedy` runs, five each of `podless` and `hungry`, three `naive`, one `idle`):

   | Line | Rule |
   |---|---|
   | a | `greedy` wins at least 9 of 10 |
   | b | `greedy`'s median core is 6 or more; on a level with `h` ≤ 0.4 it is 9 or more |
   | c | `greedy`'s median time is 0.7 to 1.2 `T` (Holdout and Festival: not applicable) |
   | d | `idle` loses (Festival: not applicable) |
   | e | From level 21 `naive` loses (Festival: not applicable) |
   | f | `greedy`'s median cancelled share is within 5 points of step 5's target (Siege, Twins, Colossus, Boss) |
   | g | On a level with a lasting boon, `greedy` is at least 10% faster than `podless`, or `podless` loses |
   | h | From level 30, `podless` does not earn three stars; from level 80 with `h` ≥ 0.65, `podless` wins fewer than half its runs |
   | i | `greedy` opens at least one geode on every level that has one |

   Not accepted: the next seed that passes section 11 (up to eight), as today.
8. **Per band of ten levels** (reported, and an exit check of V4 and V6): `hungry` is slower than
   `greedy` or loses on at least 60% of the band's levels; on at least half of the band's levels with
   three or more geodes `greedy` leaves one unopened; `greedy` loses a core pip on 15 to 40% of the
   band's levels; and on at least 40% of them, from level 21 on, `greedy`'s run sees a gloom below the
   first gate row.

Lines g, h, i and step 8 are how "decide whether to hit enemies or grab the upgrade" is measured: taking
geodes must pay, ignoring them must cost more as the game goes on, taking all of them must not be best,
and the competent player must feel the enemy. Today the last one fails everywhere: the greedy bot's core
is 10 on all 295 generated levels.

### 12.3 Difficulty over the whole game

Three lines already rise to level 30 and then stop (GD 7.3). Two more keep moving after that: the
reliance `m` (to level 80) and the world's rhythm `h` (forever). Nothing else scales with the level
number: a player at level 250 is tested by composition, not by bigger numbers.

---

## 13. Look, juice, sound, haptics

The four gameplay colours keep their meaning (GD 9.1). A geode is a good thing to hit, so it is aqua
with a gold heart; its shield is a barrier, so it has the barrier's rose glow.

| Thing | How it looks |
|---|---|
| Geode | A faceted aqua crystal shell, 1.8 m across, floating 0.5 m up and turning slowly; inside it a gold emissive icon of its boon (a small lantern, two chevrons, a heavy teardrop, a ring, a gate frame, a comet, a crescent). Its health as a number above it. A soft aqua pool of light on the lane under it |
| Plate | A low crystal wall like a barrier (GD 9.3), curved slightly round its geode, with its number |
| Choice pair | A thin aqua thread between the two; when one opens the other greys and sinks |
| Extra lantern | The launcher at 70% size, no pips |
| Colossus | A gloom 3.6 m across with a crown of horn bumps, heavy slow steps (the camera's trauma +0.1 a step inside 12 m), its health above it |
| Twin spires | Two spires at 70% width |
| Holdout's gate | The spire's base without the crystal: an arch with the gloom light pouring out |
| Festival target | A great paper lantern in gold where the spire stands |
| Side door | A dark arch on the rail |
| Runner, snatcher | Section 4 |

A geode must be told from a gate, a gloom, a brute and a snuffer in greyscale and at its smallest (the
far end of the lane on the 600 × 1024 frame): it is the only round bright faceted thing with a number.

| Moment | Picture | Sound | Haptic |
|---|---|---|---|
| Geode hit | Its number ticks down with a punch (at most every 80 ms); chips fly; the shell flashes | A glassy `tink`, at most 15 a second, stepping up the ladder's notes (GD 12) as the health falls | — |
| Plate breaks | The barrier's break (GD 10) | `shatter_small` | Medium |
| Geode opens | The shell bursts into twenty shards; the icon flies to the HUD's boon tray in 0.4 s; a banner (`+1 LANTERN`) for 900 ms; the boon's own effect below | `geode_open` (a bright crack) then the boon's motif: three ladder notes, a different shape for each boon | Double tick |
| `LANTERN` | The new lantern pops out of the old one with a squash | — | — |
| `QUICKEN`, `BRIGHT` | A ring runs up the barrel; the lens changes (two rings for rate, a larger lens for power) | — | — |
| `KINDLE` | Every multiply gate's label counts up one with a punch, row by row up the lane, 80 ms apart | A ladder run up | Light ticks |
| `WARD` | A gold ring spreads from the lantern to `z` = 14, popping glooms; the pips relight one by one | A soft chord | Medium |
| `COMET` | A tier-3 wisp with a long tail leaves each lantern; trauma +0.2 | `thump` and a rising tone | Strong |
| `HUSH` | The emitters' glow goes out for 6 s; the music's pad alone | A falling note | — |
| Geode slips past | It dims and sinks through the lane | `pff` | — |
| Twist | Its banner for 900 ms | One cue per twist | Light |
| Dawn (Holdout won) | The win's brightening (GD 10), from the dawn the level has been heading for | `win_chord` | The win pattern |
| Colossus falls | The spire's shatter, a hundred shards | `shatter_big` twice | The win pattern |

Every new sound is pentatonic or unpitched (GD 12). Every cue has a picture (GD 14). The flashing rule
(GD 14) covers every new effect and is measured (`scripts\flash-audit.ps1`).

---

## 14. HUD, hints, words

- **The goal bar** (top centre) is the spire's health today. Per kind: Holdout a dawn clock that fills;
  Colossus its health; Twins two half-bars; Festival the 20 s clock and `GEODES 7/12`; Boss the bar with
  two notches at 66% and 33%.
- **The boon tray** (bottom left, above the gesture inset, mirroring the tide counter): one chip a boon
  held, with its icon and count (`×2`); a timed boon's chip drains. At most four chips; it is empty and
  invisible until the first boon.
- **The goal banner**: the kind's banner (section 6) for the level's first 1.2 s, where the game's name
  stands on a cold start.
- **The result line**: `TIME 31.4 S   TIDE 1,204   BOONS 2/3`.
- **Hints**, each once (GD 11): level 6 `CRACK A GEODE TO GAIN ITS BOON`; 11 `BREAK ITS SHIELD FIRST`;
  the first geode that slips past `IT GOT AWAY: CATCH THEM EARLY`; 15 `NOTHING CAN GO WRONG HERE`; 21
  `KEEP THEM OFF YOUR LANTERN UNTIL DAWN`; 26 `ONE OR THE OTHER`; 31 `BRING IT DOWN BEFORE IT ARRIVES`;
  41 `BOTH MUST FALL`; the first hopper `IT JUMPS: WATCH THE GHOST`.
- **Lose advice** gains lines: `THE GEODES WOULD HAVE HELPED` (lost with every geode unopened),
  `DO NOT CHASE EVERY GEODE` (lost while the stream was on a geode for more than a third of the level).
- Strings are upper case, English, in the one table. The font has `0-9 A-Z . , : ! ? ' - + % / ( ) × ÷`
  and a space; a new glyph means baking the atlas again (`tools/bake_font.py`, no download).

---

## 15. Worlds and dressing (phase V5)

- **Ten worlds instead of five**, so a palette returns every hundred levels, not every fifty. The five
  new ones follow GD 9.2's rules and pass the palette lint: Frost Glass (pale blue to indigo), Storm
  Indigo (slate and violet with a yellow rail), Gold Noon (the lightest backdrop the lint allows),
  Moonlit Jade (deep green, silver rail), Crimson Dusk (red-violet). The executor proposes the colours;
  the owner picks from rendered sheets.
- **A signature per world.** Each world leans its recipes and plans toward one thing, so worlds play
  differently: Violet Dusk sliders, Deep Teal hoppers, Rose Dawn geodes and caravans, Emerald Aurora
  twin rivers and islands, Ember walls and brutes, Frost Glass breathing gates, Storm Indigo twists,
  Gold Noon festivals' kin (rich gates, many geodes), Moonlit Jade snuffers and narrows, Crimson Dusk
  colossi and side doors. A signature doubles its recipes' weights in that world; it forbids nothing.
- **Props.** The backdrop's floating islets (GD 9.3) get two more families (arches, lantern clusters)
  and each world uses one. A sky feature per world from a small set the backdrop shader can draw without
  a texture: aurora ribbons, a low moon, falling motes, a far second spire.
- **Music.** One four-bar loop plays for the whole game today. Each world gets its own chord pair and
  tempo (70 to 84 beats a minute) from a table, in the same pentatonic family so the ladder still fits;
  a boss adds the low pulse on every beat. Built by the synthesizer at the world's first level (measure
  the time; inside the level fade if it is under 150 ms on the Moto, else on a worker thread).

This phase is the first to cut if time is short (`VARIETY_PLAN.md` 8).

---

## 16. After the last baked level

The pass bakes **600 levels** (today 300, and 301 onward replays 101 to 300 unchanged). From 601 the game
replays 101 to 600 **mirrored left to right, in the world five places on** (with the ten worlds of
section 15; two places on if there are still five), with the same baked numbers (the rules are
symmetric, so the balance holds). A player meets an exact repeat only after 1,100 levels,
about ten hours of play.

---

## 17. Names

| Name | Meaning |
|---|---|
| Geode | An upgrade pod: a crystal that drifts down the lane and gives a boon when cracked |
| Plate, shield | The wall a shielded geode carries |
| Boon | What a geode gives, for the rest of the level |
| Kind | A level's goal: Siege, Holdout, Colossus, Twins, Festival, Boss |
| Colossus, cub | The giant gloom that is a level's goal, and its small form in a boss fight |
| Recipe | A way of laying out a lane |
| Plan | The shape of the enemy's flow in time and place |
| Twist | A named moment in the middle of a level |
| Hardness `h` | A level's place in its world's rhythm, 0 to 1 |
| Reliance `m` | How far a level is sized for a player who takes its boons |
| `R`, `Pa`, `Pn` | The level's reference power; what `greedy` and `podless` deliver with no enemy |
| Fingerprint, likeness | What a level is, as data; how alike two levels are, 0 to 1 |
| Runner, snatcher, side door | Section 4 |

Names to avoid (PLAN 0 rule 5): nothing from another game. The boons, bosses and kinds above are ours;
"champion" is not used anywhere.

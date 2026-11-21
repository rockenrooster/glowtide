# GLOWTIDE — Game Design

What the game is. Every number here is a starting value and a tweakable (TECH_SPEC 7.4); the owner's
play and the bot tables override them. Changes are logged in `PROGRESS.md`. Section numbers are referred
to from `PLAN.md`, `TECH_SPEC.md` and the subplans: keep them stable.

**Version 0.2's variety pass (`VARIETY_DESIGN.md`, "VD") replaces parts of this document; where the two
differ, VD is right.** 5.9's brutes and bursts: VD 5 (eight plans; the bursts are the SURGE twist). 7.3
the generator and 7.5 worlds and bosses: VD 6 (six kinds), 8 (four bosses), 9 (recipes), 10 (the
calendar, set pieces) and 15 (ten worlds of ten levels, each with its own palette, sky, props, music and
signature). 7.4 and 8, the bake and the bots: VD 11 (no repeats) and 12 (more bots, more lines, 600
levels). 9.2's palettes: VD 15 (ten). 9.3's elements, 10's juice, 12's sound and 13's haptics: VD 4
and 13 add to them. 11's HUD: VD 14 (the boon tray, level names, hint lines). New in VD: geodes and
boons (2, 3), twists (7), after level 600 (16). `PROGRESS.md` records how each was built.

---

## 1. The feel target

The owner likes how multiplier-gate crowd games play in their advertisements and is let down by the
products behind them. What those thirty-second clips have, and what this game must have in every level:

1. **One thumb, no interface.** A finger goes down and things pour out. Nothing to read first.
2. **A stream, not shots.** The launcher is a hose. Sweeping it across the lane is the whole skill.
3. **Multiplication you can watch.** One creature goes into a gate and several come out, on screen, each
   one countable. Chains of gates turn a thread into a flood within three seconds.
4. **Numbers everywhere, always moving.** Gates carry their factor, walls and big enemies their remaining
   strength, the enemy base a number that races down.
5. **Two crowds.** The enemy is a crowd too. Where the crowds meet there is a front line: a strong flood
   pins it against the enemy base, a weak one lets it come down the lane.
6. **The flood wins.** A level ends with the screen full of your crowd pouring into the enemy base.
7. **A choice every two seconds.** The bigger gate is narrower, or moving, or behind a wall; a wave is
   about to come out of the base.
8. **No friction.** No menus between levels, no currencies, no waiting. Lose, tap, try again.

What the products add and this game refuses: base building, card collections, upgrade grinds, energy,
daily rewards, advertisements, pop-up offers.

Everything else about the game is ours: the theme, the creatures, the look, the names, the levels
(PLAN 0, rule 5).

---

## 2. One level, second by second

| Time | What happens |
|---|---|
| before | The lane is on screen: lantern at the bottom, gates up the lane, the spire at the top with its number. A hint pulses: HOLD AND DRAG. Nothing moves until the player touches |
| touch | The level starts. The lantern pours wisps, twelve a second, straight up the lane from wherever the thumb puts it. The spire starts sending glooms 1.5 s later |
| +1 s | The stream reaches the first gate. Every wisp that passes ×2 becomes two; a note plays; the label punches |
| +2–4 s | Second and third rows. ×2 then ×3 then ×5 turns 12 a second into 360 a second. The tide counter races. The stream has fanned out into a flood |
| +4 s on | The flood meets the glooms. They cancel one for one in a line of sparks. A full flood pins that line at the spire's foot; a thin one lets it creep down the lane, and bursts from the spire push it down for a few seconds at a time |
| meanwhile | The player chooses: which gate, when to follow a slider, whether to chew through a wall for a bigger gate, and how fast to get the stream back on its best line when the enemy's line comes down |
| +20–35 s | The spire's number reaches zero. Slow motion, it shatters, every gloom pops, dawn breaks, the wisps cheer. CLEARED, stars, NEXT |
| or | The stream was wasted for too long: the enemy's line reaches the gates, the flood thins, and it all comes down fast. Ten glooms' worth reach the lantern. It goes dark. SNUFFED OUT, RETRY |

---

## 3. The lane and the camera

World units are metres. `+x` is right, `+y` is up, `+z` is up the lane, away from the player.

| Thing | Value |
|---|---|
| Lane | A flat causeway, `x` from −5.0 to +5.0 (`HW` = 5.0), `z` from 0 to `LEN`. `LEN` is 30 to 44 by level; the concept frame uses 36 |
| Launcher line | `z` = 0. The launcher slides on a rail at `z` = 0.2, `x` from −4.4 to +4.4. The muzzle is at `z` = 1.0 |
| Core line | `z` = 0.8: a gloom that gets this far hits the core |
| Rows | Gates and obstacles sit in rows. First row no nearer than `z` = 8, last row no further than `LEN` − 10, rows 6 to 8 apart |
| Spire | Its face is at `z` = `LEN`, 5.6 wide (`x` from −2.8 to +2.8), about 6.3 tall |
| Walls | The lane's edges are solid for units: they slide along them, nothing falls off |

**The whole playfield is on screen at all times**, from the launcher's base to the top of the spire. The
camera does not scroll. It sits above and behind the launcher looking down the lane, so the lane is a
trapezoid: about 85% of the screen's width at the launcher, about 38% at the spire. The concept frame
(`concept/make_concept.py`) uses, for a 9:20 screen and `LEN` 36: eye (0, 24, −11), target
(0, 0, 14.5), vertical field of view 56°. TECH_SPEC 5.2 has the rule that fits every lane length and
aspect ratio; it lands close to those numbers, not exactly on them.

Screen bands in portrait, inside the safe area: a HUD row at the top; below it the spire; then the lane;
at the bottom the launcher, above the system gesture inset. The thumb rests in the lower third and never
covers a gate.

---

## 4. Controls

- **Hold anywhere to fire.** While a finger is down, the launcher pours. Lifting stops it.
- **Drag to aim.** Relative drag: the launcher moves by the finger's horizontal movement, wherever the
  finger is. Gain 1.15: a finger movement that spans one lane metre on screen at the launcher line moves
  the launcher 1.15 m. No acceleration curve. The launcher follows through a 40 ms smoothing filter and
  is clamped to its rail.
- One finger at a time: the oldest finger still down owns the gesture; others are ignored until it
  lifts.
- **Every level starts on a touch.** The first touch that is not on a button starts the level, and it
  already fires and aims. This holds on HOME and after RETRY and RESTART (section 11).
- **Auto-fire** (setting, off by default): the launcher pours without holding once the level has started;
  dragging still aims.
- Desktop (development): left mouse button held is the finger; `A`/`D` or the arrow keys move the
  launcher at 9 m/s with `Space` held to fire; `Esc` is back; `P` pauses.
- Android back: play → pause; pause → resume; settings → back; home → leave the app.

---

## 5. Rules

The simulation is two-dimensional, on the lane's plane (`x`, `z`). Height is only drawn. It runs at a
fixed 60 ticks per second (TECH_SPEC 4).

### 5.1 Units and power

There are two teams: **wisps** (the player's) and **glooms** (the spire's). Every unit has a position, a
velocity and a **power**, a whole number of at least 1. A fired wisp and a plain gloom have power 1.
Power is what a unit is worth in a clash and what it takes off a spire, a barrier or the core.

A unit's size shows its power:

| Tier | Power | Radius (wisp / gloom) |
|---|---|---|
| 0 | 1–3 | 0.26 / 0.29 |
| 1 | 4–15 | 0.36 / 0.40 |
| 2 | 16–63 | 0.50 / 0.55 |
| 3 | 64 and up | 0.65 / 0.72 |

The **tide** is the sum of the power of all wisps on the lane. The HUD's counter shows it.

### 5.2 Firing

The launcher fires `FIRE_RATE` = 12 wisps a second. It keeps a shot counter that fills at that rate
whether or not the thumb is down, capped at one shot; while firing, each tick with a full shot emits a
wisp. So a press fires at once, and tapping can never beat holding. A wisp starts at
(`launcherX` ± 0.07, 1.0) and moves straight up the lane at `WISP_SPEED` = 8.0 m/s. The stream has no
spread of its own: all the fanning out comes from gates.

### 5.3 Caps and overflow

At most `CAP_W` wisps and `CAP_G` glooms exist: 2,400 and 1,000 to start. P1 sets the final values, by
the owner's eye and by measurement on the floor phone (PLAN 5), and they are then the same on every
device. The caps are about looks as much as speed: a 10 × 36 m lane holds about 1,700 wisps side by
side, so far more than that reads as one solid mass instead of a crowd. A typical level's flood is 600
to 1,800 wisps; only the biggest chains reach the cap.

Past a cap, the crowd grows in power instead of in number, so nothing is lost at the cap:

- A gate that would create clones with no room left adds the missing clones' power to the wisp that
  passed (5.4). Sixty-four slots of `CAP_W` are reserved for firing so gates can never starve the stream.
- If even the reserve is full, a shot's power is added to the most recently fired wisp that is still
  alive (or the shot is skipped if there is none).
- An emitter that cannot spawn adds the power it owes to the next gloom it does spawn.

### 5.4 Gates

A gate is an opening across part of the lane at one `z`: a centre `cx`, a half-width `hw` (1.2 to 1.8),
a kind and a factor `k`. Only wisps use gates. Glooms walk through every gate untouched.

A wisp **passes** a gate in the tick in which its `z` at the start of the tick was below the gate's `z`
and its `z` at the end of the tick's movement (after separation and bumpers) is at or above it, if its
centre is within the opening and it has not passed this gate before. Each unit carries one bit per
gate; clones inherit their parent's bits, including this gate's.

- **Multiply, ×k** (`k` 2 to 5): the wisp becomes `k` wisps. `k` − 1 clones appear at its position with
  the same power; all `k` get a sideways push of up to (0.9 + 0.5 `k`) m/s, spread evenly across the
  group with a little noise, that dies away with a time constant of 0.5 s. If only `c` clones fit under
  the cap, the passing wisp's power is multiplied by (`k` − `c`) instead. Total power is always exactly
  multiplied by `k`.
- **Divide, ÷k** (`k` 2 or 3): the gate keeps a carry, starting at 0. For each passing wisp:
  `t = carry + power`; the wisp's power becomes `t / k` (whole-number division); the carry becomes
  `t mod k`. A wisp left with power 0 is snuffed. For plain wisps this lets every `k`-th one live; for
  any mix of powers exactly one `k`-th of the power gets through.

**Sliding gates** move their centre left and right: `cx(t) = cx0 + amp · wave(t / period + phase)`,
where `wave` is a triangle wave eased at its turning points (no sine in the simulation; TECH_SPEC 4.2).
Amplitude up to `HW − hw − 0.3`, period 3 to 6 s.

### 5.5 Clash

When a wisp and a gloom overlap (distance below the sum of their radii), both lose
`min(their powers)`. A unit at zero dies. A unit clashes at most once a tick. With plain units this is
one for one: both pop.

Units of the same team push each other apart softly so a crowd reads as a crowd, not as one blob
(separation radius 96% of the two radii, push up to 6 m/s; it never moves a unit backwards down its
lane).

Wisps do not chase glooms: glooms come to the stream (5.9).

### 5.6 Barriers

A barrier is a wall across part of the lane with a **health number** on it (10 to 400). A wisp that
reaches it gives up to its power to the wall: the wall's health drops by that much, the wisp's power
too, and a wisp at zero dies. When the health reaches zero the barrier breaks and is gone; a wisp with
power left over carries on. Glooms pass through barriers: they are the spire's. A barrier guards a
strong gate and covers that gate's whole opening.

### 5.7 Bumpers

A bumper is a solid pillar (radius 0.5 to 1.0). Units of both teams slide round it. Bumpers split a
stream and make narrow passages. Between a bumper and anything else solid (a wall, a gate's post,
another bumper) the gap is at least 1.2 m.

### 5.8 Snuffers

A snuffer is a dark orb (radius 0.6) that patrols a straight path back and forth (eased triangle wave,
2 to 4 m/s). A wisp that touches it is snuffed, whatever its power. Glooms are not affected. It never
runs out.

### 5.9 The enemy

The spire has one to three **emitters** at its foot (`z` = `LEN` − 0.5, each at an `x`, fixed or
sliding).

- **Strength.** The enemy is sized against what the player's side can produce, or the flood would
  simply erase it and there would be no front line. `P0` is the power per second a competent player
  delivers on that layout with no enemy; the bake measures it (7.4). The level's **base flow** is
  `fight × P0 × rateScale` power per second. `fight` is the share of the player's output the enemy is
  meant to cancel: `0.12 + 0.006 × min(level, 30)`, so 13% on level 2 and 30% from level 30.
  `rateScale` is the bake's correction (1.0 to start). The base flow runs all level long, split evenly
  between the emitters.
- **Bursts.** A burst is twice the base flow for 3 s. From level 6 there are two, at 35% and 70% of
  the level's target time. A burst can also be triggered by the spire's health falling below a
  fraction (level 5 and boss levels).
- **Gloom:** power 1. Walks down the lane at `GLOOM_SPEED` = 3.2 m/s and drifts sideways toward the
  launcher's `x` at up to `HOMING` = 0.8 m/s. That drift brings the enemy into the stream.
- **Brute:** a gloom spawned with the `brute` mark: power `min(60, 16 + level / 2)`, speed 2.0 m/s,
  drawn no smaller than tier 2 whatever power it has left, with its remaining power as a number above
  it. From level 8 one emitter sends a brute every 8 s; its power is taken out of the base flow, so the
  total stays the same. It clashes and dies like any gloom.
- **Pressure ramp:** past 1.5 times the level's target time `T` (section 6) every flow is multiplied
  by `1 + (t − 1.5 T) / 20`, so a level that drags on is decided, one way or the other, by about 3 `T`.

The simulation's clock starts at its first tick. The game does not tick a level until the player's
first touch (section 4), so for a player the clock starts when they do. Emitters start 1.5 s after the
clock. A player who then never fires loses within about twenty seconds.

**What this contest is like.** `concept/flux_model.py` (a toy model) predicted four findings while
planning; P2 tested them with the real simulation (`glowtide_headless --sim flux`, level 5's layout and a
three-row ×30 layout with `T` = 30 s; the table is in `PROGRESS.md`). What holds:

1. With the bake's rate scale the enemy cancels about what `fight` asks, a few points over: 14–16%,
   20–21% and 29–35% of the player's output for a `fight` of 13%, 18% and 30%. No gloom reaches the
   lantern, bursts included, and a full-strength player finishes at 0.85–0.91 `T`. The front is not a
   line at the spire's foot, though: single glooms drift down the sides of the lane, the deepest
   10–12 m below the spire on level 5 and 18–23 m on the ×30 layout, still above the first gate row.
2. It is a contest with a sharper edge than the model said. On level 5 an enemy flow held at 0.9 times
   the flood's own size still loses, slowly (3 `T`); at 1.2 times the player collapses within 0.75 `T`.
   On the ×30 layout the collapse comes already at 0.9 times: the glooms that reach the low stream
   cancel wisps before they are multiplied, so the root of the stream is what the enemy wins. There is
   no stalled middle: past the edge, losing is a collapse, not a trickle.
3. `fight` sets how much of a competent player's output a player needs, and the pressure ramp (5.9)
   decides the slow cases. On the ×30 layout a player who makes only half the flood still wins at a
   `fight` of 13% or 18% (1.85 `T`, 2.5 `T`) and collapses at 30% (1.1 `T`). On level 5 the half player
   loses at every `fight` (2.3–3.6 `T`: past 1.5 `T` the ramp finishes them). A player who makes two
   thirds of the flood wins everywhere, at 1.4–2.0 `T`.
4. A player who wins is not hit: every win in the table ends with the core at 10, every loss at 0. The
   core is what a collapse destroys, not a bar that wears down. The skill shows in the time (and so in
   the stars, 5.11); tension comes from bursts, brutes, the clock, and the glooms slipping down the
   sides while the stream is wasted.

If the owner wants more threat than that after playing, or finds a slow loss dull, 5.12 lists the
levers and the ramp is one number.

### 5.10 The spire and the core

- **Spire:** health `HS` (60 on level 1, thousands later). In the last 7 m, wisps steer toward the
  spire's face (`|x|` ≤ 2.5) at up to 3 m/s sideways, so the flood funnels in. A wisp that reaches
  `z` ≥ `LEN` − 0.3 takes its power off the spire and is gone.
- **Core:** the lantern's health `HC` = 10, drawn as ten pips round its base. A gloom that reaches
  `z` ≤ 0.8 takes its power off the core and is gone.

### 5.11 Winning, losing, stars

Checked every tick after damage is applied: spire at zero → **win** (even if the core fell in the same
tick); otherwise core at zero → **lose**.

Stars on a win are for speed, because speed is what good aim buys: ★ for winning, ★★ within 1.3 times
the level's target time `T`, ★★★ within `T`. The best star count per level is kept. Stars buy nothing.

The result screen also shows the time and the highest tide reached.

### 5.12 Optional levers (none is built unless the owner asks for it after playing)

- **FRENZY.** A meter fills with damage dealt to the spire. When full (10% of the spire's starting
  health), the launcher fires at three times the rate for 4 s, the wisps of that burst are drawn
  white-gold, and the meter empties. No extra input.
- **FLANKERS**, if the game needs more threat: one emitter at a lane edge sends two or three glooms a
  second (a fixed number, not a share of the flood) straight down the side with little homing. They
  miss the flood and must be swept with the stream. It adds a defence chore; it may also annoy.
- **ENGAGE**, if glooms are seen slipping through a flood: a wisp steers sideways at up to 2.5 m/s
  toward a gloom within 1.0 m to its side and 1.6 m ahead.

### 5.13 Order within a tick

1. Input: move the launcher.
2. Fire: emit wisps.
3. Emitters: spawn glooms and brutes.
4. Steer and move every unit (funnel, homing, sideways push decay, integrate, lane walls).
5. Advance sliding gates and snuffers.
6. Rebuild the spatial grid.
7. Separation.
8. Bumpers.
9. Gates (each wisp's `z` at the start of the tick against its `z` now).
10. Barriers.
11. Snuffers.
12. Clash.
13. Spire and core damage.
14. Remove the dead; publish events for the view.
15. Win and lose.

---

## 6. Numbers (starting values)

| Name | Value | Notes |
|---|---|---|
| `TICK_HZ` | 60 | |
| `HW`, `LEN` | 5.0, 30–44 | Lane half-width, length |
| `FIRE_RATE` | 12 /s | One stored shot at most |
| `WISP_SPEED` | 8.0 m/s | 4.5 s to cross a 36 m lane |
| `GLOOM_SPEED`, brute | 3.2, 2.0 m/s | |
| `HOMING` | 0.8 m/s | Glooms drift toward the launcher's `x` |
| `FUNNEL` | last 7 m, toward `|x|` ≤ 2.5, 3 m/s | |
| `FAN(k)` | 0.9 + 0.5 `k` m/s, 0.5 s decay | Sideways push after ×k |
| Separation | 96% of radii, 6 m/s | |
| `CAP_W`, `CAP_G`, reserve | 2,400, 1,000, 64 | Set in P1 (range 1,200–4,000 wisps); one value for all devices |
| `HC` | 10 | Core health |
| Enemy `fight` | 0.12 + 0.006 × min(level, 30) | The share of the player's output the enemy should cancel (5.9); the bake solves `rateScale` to hit it |
| Burst | 2× the base flow for 3 s | |
| Brute power | min(60, 16 + level / 2) | From level 8, one every 8 s |
| Drag gain, smoothing | 1.15, 40 ms | |
| Emitter delay | 1.5 s | After the level starts |
| Pressure ramp | × (1 + (t − 1.5 T) / 20) past 1.5 `T` | Ends a stalled level |
| Tier thresholds | 4, 16, 64 | |
| Gate half-width | 1.2–1.8 | The bigger factor gets the narrower gate |
| Target level time `T` | 24 + 0.4 × min(level, 30) s | 8, 14, 18, 22, 26 s on levels 1–5 |

---

## 7. Levels

### 7.1 Level data

A level is a small plain structure (TECH_SPEC 4.7): seed, index, `LEN`, world (palette), target time,
spire health, core health, up to 24 gates, 8 barriers, 16 bumpers, 8 snuffers, 3 emitters with their
waves, and the enemy's rate scale. Levels 1–5 are laid out by hand in that
structure. Level 6 onward comes from the generator. Every level's spire health and rate scale come from
the bake (7.4), except level 1.

### 7.2 Levels 1 to 5 (each teaches one thing)

Gates are given as centre and half-width.

| # | `LEN` | `T` | Layout | Enemy | Teaches |
|---|---|---|---|---|---|
| 1 | 30 | 8 s | Row `z` 12: ×2 at 0 (1.8) | None. Spire 60, fixed. Cannot be lost | Hold, pour, a gate doubles |
| 2 | 30 | 14 s | Row `z` 10: ×2 at −2.5 (1.8), ×3 at +2.5 (1.8) | One emitter at `x` 0 | Read the number; glooms and the core exist |
| 3 | 36 | 18 s | Row `z` 9: ×2 at 0 (1.8). Row `z` 17: ×3 at 0 (1.5) | One emitter at `x` +2 | Chains multiply: ×6 |
| 4 | 36 | 22 s | Row `z` 9: ×3 at −2.7 (1.5), ÷2 at +2.7 (1.5). Row `z` 17: ×2 sliding about 0 (1.6), amplitude 3.0, period 4 s | One emitter at `x` +2 | Bad gates; follow a moving gate |
| 5 | 34 | 26 s | Row `z` 8: ×2 at 0 (1.8). Row `z` 15: ×2 at −2.6 (1.5) on the open side; on the other side ×4 at +2.6 (1.5) behind a barrier of 40 at `z` 13, `x` 1.0…4.2. Row `z` 22: ×2 at 0 (1.2) between bumpers (radius 0.7) at `x` −3.1 and +3.1 | Emitters at `x` −2 and +2; one burst when the spire falls to 50% | Breaking a wall pays: ×8 behind it against ×4 beside it |

Levels 2 to 5 have a base flow and no time bursts; `fight` is the formula of section 6. They get
their spire health and rate scale from the bake's steps 2 to 5 run on these fixed layouts. There is no
reseeding for a hand-made level: if one cannot be accepted, change its layout.

### 7.3 The generator (level 6 onward)

`generate(index, seed)` is a pure function that returns a level's layout. It uses only the project's
own integer random stream and plain arithmetic.

1. **Rows.** Row count by level (table below). The first row at `z` 8 to 10, the others 6 to 8 apart.
   `LEN` = last row + 12, clamped to 30…44.
2. **A pattern per row**, drawn with the level's weights from the patterns it may use:

   | Pattern | What it is |
   |---|---|
   | Open | One wide gate (half-width 1.8), anywhere |
   | Choice | Two gates side by side, ×a and ×b; the bigger is narrower (1.2 against 1.8) |
   | Trap | A multiply gate beside a ÷2 (÷3 from level 25) |
   | Slider | One sliding gate |
   | Wall and prize | A barrier covering a strong gate 2 m behind it; beside it a weaker open gate |
   | Pillars | Two or three bumpers with gates in the passages |
   | Patrol | A snuffer patrolling in front of a strong gate |
   | Crossing sliders | Two sliders in opposite phase, a multiply and a ÷2, 1.6 m apart in z so they pass each other |

   Factors: a plain gate draws from ×2 up to the level's strongest; the gate of a Slider, a Wall and
   prize or a Patrol is one step stronger, never above ×5. Rules: the first row is always Open or
   Choice; no two rows in a row are both Trap or Patrol; every row has at least one multiply gate that
   a straight stream can reach; no two objects overlap; 5.7's gap rule holds. A barrier's health is
   1.5 s of the stream a player would have at that row (12 a second times the strongest gate of each
   earlier row).
3. **The enemy** per 5.9: one emitter up to level 9, two up to level 29, then three, spread across the
   spire's foot (`|x|` ≤ 2.5); the base flow, the two time bursts, brutes from level 8.
4. **World** = the palette for the level (7.5).

| Levels | Rows | Strongest plain gate | Weights: Open / Choice / Trap / Slider / Wall / Pillars / Patrol / Crossing |
|---|---|---|---|
| 6–9 | 2–3 | ×3 | 30 / 30 / 15 / 15 / 5 / 5 / 0 / 0 |
| 10–19 | 3 | ×4 | 15 / 25 / 15 / 20 / 10 / 5 / 10 / 0 |
| 20–39 | 3–4 | ×5 | 10 / 20 / 15 / 20 / 12 / 8 / 10 / 5 |
| 40–99 | 4 | ×5 | 5 / 20 / 15 / 20 / 12 / 8 / 12 / 8 |
| 100+ | 4–5 | ×5 | 5 / 15 / 15 / 20 / 15 / 8 / 12 / 10 |

Difficulty rises along three lines only: more rows and trickier patterns (this table), a bigger `fight`
(section 6), and brutes.

### 7.4 Balance by bot (the bake)

Nobody tunes 300 levels by hand, and the executor cannot play. The offline bake tool
(`glowtide_headless --bake-levels`, TECH_SPEC 4.8) does this for each level:

1. Take the layout (generated from the level's seed, or the hand-made one for levels 2 to 5).
2. **Measure `P0`.** Run the **greedy** bot (section 8) with no enemy and an unbreakable spire for the
   target time `T`. `P0` = power delivered ÷ `T`.
3. **Size the enemy.** Base flow = `fight × P0 × rateScale`. Find `rateScale` by bisection between 0.3
   and 3.0 (at most six steps): in a greedy run with the enemy on and an unbreakable spire for `T`, the
   glooms must cancel `fight` of the power the player's side produced (every wisp fired plus everything
   gates added), within 5 points. The cancelled share rises with `rateScale`, so the bisection is sound.
4. **Size the spire.** Spire health = 0.85 × the power delivered in that last run, rounded to two
   significant figures.
5. **Accept** the level if, over 10 runs with different input noise:
   - (a) greedy wins at least 9;
   - (b) greedy's median core at the end is 6 or more;
   - (c) greedy's median time is within 0.7 `T` … 1.3 `T`;
   - (d) from level 8, the **naive** bot either loses or needs at least 1.6 times greedy's time;
   - (e) the **idle** bot loses;
   - (f) from level 21 the **naive** bot loses (added after the first full bake: three bands let it win twice);
   - (g) for a generated level, greedy's median cancelled share is within 5 points of `fight` (added with (f):
     the bisection's runs keep the spire whole for `T`, the real runs end at the win and cancel a few points more).
     Before giving up on the seed, the bake corrects the rate scale toward `fight` (scale × fight ÷ share) up to
     twice, sizing the spire again each time.

   Not accepted: a generated level moves to its next seed (at most eight, then the bake stops and
   reports the level); a hand-made level is reported.

The result is `src/sim/levels_baked.h`: one row per level (seed, spire health, the enemy's base flow). At run time
the game regenerates the layout from the seed and takes the two numbers from the table. The bake also
writes `captures/levels/report.md`, one row per level: rows, patterns, `P0`, `fight` and the cancelled
share reached, greedy's time and core, naive's result, peak tide, peak unit counts, the lowest `z` a
gloom reached in greedy's runs, seeds tried.

Levels 6 to 300 are baked. After level 300 the game repeats levels 101 to 300, unchanged.

### 7.5 Worlds and boss levels

Every ten levels the palette changes (9.2): 1–10 Violet Dusk, 11–20 Deep Teal, 21–30 Rose Dawn, 31–40
Emerald Aurora, 41–50 Ember, then round again.

Every tenth level is a **boss level**. Its spire is drawn broader, with a crown of crystals, at the
same height (so the camera rule does not change). Its enemy adds two bursts, when the spire falls to
66% and to 33%, each with two brutes. It is baked like every other level, with those waves in the runs
and a target time of 1.4 `T`. Its win is the ordinary win, louder (section 10).

---

## 8. Bots

Bots drive the launcher through the same input the player uses. They exist for tests, tuning, the bake,
benchmarks and pictures; they are not a game feature. A bot's level starts at tick 0: there is no
waiting for a first touch in a bot run.

| Bot | Behaviour | Used for |
|---|---|---|
| `greedy` | Every 0.2 s it scores 21 launcher positions by the power a straight stream from there would deliver in the next 8 s: 12 a second × the product of the multiply gates on that line (each gate taken where it will be when the stream arrives) × the time left after chewing through any barrier on the line (its health ÷ the stream's power at that row). A line through a ÷ gate or a snuffer's path scores zero. **Defend rule:** if a gloom is nearer than `z` = 12 and no wisp is within 0.8 m of its line, it defends that line for 0.6 s first. Human limits: 0.15 s reaction, 10 m/s launcher speed, ±0.25 m aim noise | The competent player: bake, acceptance, benchmarks |
| `naive` | Sits at `x` = 0 and fires | The player who does not aim |
| `sweep` | Fires while sweeping left and right at 3 m/s | A child's play |
| `idle` | The level runs and it never fires | Must lose (except level 1, which it never finishes) |

Acceptance for the hand-made levels (P2), on top of 7.4's lines for levels 2 to 5:

| Level | `greedy` | `naive` | `idle` |
|---|---|---|---|
| 1 | Wins in 6–10 s | Wins | Neither wins nor loses in 60 s |
| 2–5 | 7.4 (a) to (c) | Reported, no rule (on level 2 it stands between the gates and passes neither; on level 3 it passes both) | Loses within 30 s |

---

## 9. Look

`concept/concept_gameplay.png` is the target. It is a mock drawn by a script, so judge its layout,
palette, density and reading order, not its exact shapes.

### 9.1 Direction

**A lantern festival seen from above.** Twilight depth below a floating causeway; small solid creatures
that glow; the crowd is the light source and visibly lights the ground it runs on.

- Bodies are solid and readable, not transparent blobs of light. The glow comes from bloom and from the
  light the crowd throws on the lane (TECH_SPEC 5.5).
- A crowd of light must stay a crowd of its colour: where glows pile up they converge on gold or
  violet, they do not burn out to white (TECH_SPEC 5.3).
- Props are faceted, flat-shaded and two-toned, in the backdrop's hue family, so they recede.
- Four gameplay colours carry all meaning and **never change between worlds**: gold = yours,
  violet = theirs, aqua = good gate, rose = bad or dangerous. Worlds change only the backdrop, the lane
  and the rail accent.
- The backdrop never goes to black and never above 45% luminance: it is deep, not empty.
- Nothing is outlined in black. Separation between overlapping creatures comes from a darker rim of
  their own colour.

### 9.2 Palettes

Gameplay colours (all worlds):

| Role | Colour | Role | Colour |
|---|---|---|---|
| Wisp body / light / rim | `#FFC654` / `#FFEEB0` / `#CE7022` | Good gate | `#54F2E0` |
| Gloom body / light / rim | `#7A3CC8` / `#A878F0` / `#2A1260` | Bad gate, snuffer, danger | `#FF547C` |
| Gloom eyes | `#FF9CEA` | Spire body / light / dark | `#281A54` / `#4E368C` / `#160E34` |
| Launcher body / light / dark | `#226884` / `#46B0C8` / `#1A4E68` | Spire cracks and heart, barrier glow | `#FF60D6` |
| UI ink (panels at 80% alpha) | `#0E0E2C` | UI text | `#FFFFFF` |

World colours (backdrop from far to near, lane far and near, rail accent, prop body / light / dark):

| World | Backdrop | Lane far / near | Rail | Props |
|---|---|---|---|---|
| 1 Violet Dusk | `#8458AC` `#403080` `#18184C` `#0A0A26` | `#3C4488` / `#1E2254` | `#60F4E8` | `#2E3C72` / `#5C70B2` / `#1A2048` |
| 2 Deep Teal | `#2E8C9A` `#14506E` `#0A2844` `#04101E` | `#1C5066` / `#0E2C42` | `#7CE0FF` | `#1E4A66` / `#4E90AC` / `#102A40` |
| 3 Rose Dawn | `#E89C8C` `#B0608E` `#5A3278` `#1E1440` | `#5E4078` / `#2C1C48` | `#FFD9A0` | `#5A3A78` / `#A070B0` / `#2E1E4A` |
| 4 Emerald Aurora | `#48B48A` `#1E7882` `#102E5A` `#080E28` | `#1E5460` / `#122848` | `#B4FF9C` | `#1E5068` / `#4A9A9C` / `#102C44` |
| 5 Ember | `#F08A50` `#B43C5A` `#46184E` `#140A26` | `#6E3458` / `#2A1840` | `#FFC878` | `#5A2A58` / `#A85A7C` / `#2C1638` |

Worlds 2 to 5 are the planner's untested guesses; P1 renders 1 to 3 for the owner and P5 the rest.

**Palette rules**, checked by a small script whenever a palette changes (relative luminance and
contrast ratio as in WCAG): the lane's luminance is at most 0.08 at both ends; the backdrop's at most
0.45; gold and aqua have at least 3 to 1 against both ends of the lane; rose at least 2.5 to 1; a
gloom's eyes at least 2.5 to 1 against its body and against the lane. A gloom's body is close to the
lane in brightness on purpose (it is a creature of the dark): it is read by its eyes, its dark rim, its
horns and the violet light it stands in, and that is checked on a greyscale picture, not by a ratio.

### 9.3 Elements

| Element | How it looks |
|---|---|
| Wisp | A teardrop of light: a round body with one soft tip that trails back and wobbles as it hops. Gold with a pale core and an amber rim. Seen from behind during play (it runs away from the camera), so it has no face until the win, when every wisp turns round, shows two happy eyes and bounces. Hops as it runs (height 0.25 m, 5 hops a second, phase by unit) and squashes on landing |
| Gloom | A round violet blob with **two small horn bumps** (the silhouette that tells it from a wisp at any size) and two glowing rose eyes facing the player. Bobs as it walks |
| Brute | A big gloom, never drawn smaller than tier 2, with its remaining power as a number above it |
| Launcher (the lantern) | A round teal base on a rail, a short barrel pointing up the lane with a glowing gold lens. Ten gold pips round the base are the core's health; a lost pip goes dark. The barrel squashes with each shot and leans into sideways movement |
| Spire | A tall faceted dark crystal, lit from one side, with rose cracks that spread as its health falls (at 75%, 50%, 25%) and a glowing heart near the top. Its health is a big number on its face. Small crystals at its foot; glooms pour from between them. A boss spire is broader, with a crown |
| Gate | Two posts with a glowing strip, a translucent panel of the gate's colour between them (3.0 m tall) with slow vertical shimmer, bright top and bottom edges, and the label (`×3`, `÷2`) in the upper half of the panel. A sliding gate shows a small arrow on each side of its label |
| Barrier | A low crystal wall (1.6 m) with teeth along the top, a rose glow at its foot and its health as a number |
| Bumper | A short pillar with an aqua disc on top |
| Snuffer | A dark orb with a rose ring and three slow sparks orbiting it; a faint rose line shows its patrol path |
| Lane | A dark slab, lighter with distance, with faint tile seams and forward chevrons, glowing rails in the world's accent along both edges, a small lantern post every 8 m, and a dark front face at the near end |
| Backdrop | A vertical gradient of the world's four colours with a soft warm glow behind the spire, drifting mist bands, slow motes in three colours, and a few floating islets with tiny lights off both sides of the lane |

### 9.4 Readability rules

- **Labels win.** Gate labels, barrier and brute numbers and the spire's number are drawn after every
  unit, so no crowd ever hides a number. A label is at least 3% of the screen's height tall.
- Every number uses the same bold rounded font in white with a dark stroke of the element's own hue.
  The stroke is at least 3 px wide at the label's smallest size, so a white number holds over the gold
  flood.
- Good and bad gates differ by symbol (× and ÷) as well as by colour; wisps and glooms by silhouette as
  well as by colour. The game must be playable in greyscale (a P6 check).
- The launcher is the nearest thing to the camera and nothing is drawn over it but the HUD.
- Nothing the player needs is under the system gesture inset or a display cutout.

---

## 10. Juice

"Trauma" is camera shake: offset = trauma² × 0.35 m × smooth noise, decaying at 2.2 per second.

| Moment | Picture | Sound (section 12) | Haptic (section 13) |
|---|---|---|---|
| Shot | Barrel squashes 10% for 70 ms; a ring flashes at the lens; the wisp scales 0.5 → 1 in 90 ms | `pip`, every second shot | — |
| Wisp through ×k | Each clone scales 0.3 → 1.15 → 1 in 180 ms; 4 to 6 sparks; the panel brightens 40% and decays in 200 ms; the label punches 1 → 1.22 → 1 in 160 ms (at most every 80 ms) | The gate's ladder note, at most 20 a second per gate, louder with the rate | Light tick, at most 10 a second |
| Wisp through ÷k | Snuffed wisps shrink to nothing in 120 ms with a grey-rose puff; the panel flashes | `pff` | — |
| Clash | Six sparks, gold and rose; a flash in the ground light | `tick`, at most 30 a second | — |
| Barrier hit / break | The number punches, chips fly / twenty shards, a ground ring, the number pops | `chip` / `shatter_small` | — / medium |
| Brute dies | Big pop, sixteen sparks, a ground ring | `pop_low` | Medium |
| Snuffed by an orb | Puff; the orb's ring flares | `pff` | — |
| Spire hit | The face flashes rose-white, brighter with the damage rate; the number ticks down with a punch; at 75 / 50 / 25% a crack spreads and trauma +0.3 | `thump`, at most 15 a second; `crack` at each stage | A soft pulse train while damage flows, at most 8 a second |
| Core hit | The screen's edge flashes rose for 250 ms; trauma +0.5; one pip goes dark with a puff | `core_hit` | Strong |
| Tide passes 100, 500, 1,000, 2,500, 5,000, 10,000 | The counter punches to 1.3×; a banner "FLOOD ×1,000" for 900 ms | `rise` (a quick run up the scale) | Double tick |
| Win | Time runs at 0.35× for 0.8 s; the spire shatters into sixty shards; every gloom pops in a wave that runs down the lane in 0.6 s; the backdrop brightens toward the world's lightest colour over 2 s; wisps turn round and bounce; motes rise like confetti; "CLEARED!" drops in, stars land 250 ms apart | `shatter_big`, `win_chord`, a `star` ping each | A long pattern |
| Boss win | The win, with trauma +0.5 at the shatter, a hundred shards and `shatter_big` twice, 120 ms apart | As the win | As the win |
| Lose | The lantern's lights go out; the picture loses 60% of its colour over 0.6 s; glooms bounce | `lose` | A long low buzz |

Always on: the camera follows 12% of the launcher's `x` on a spring; each unit hops on its own phase;
the spire's heart pulses once a second; lane lanterns flicker; gate panels shimmer.

UI motion: a screen eases in over 220 ms (scale 0.92 → 1, fade). A pressed button shrinks to 0.94 and
inverts: an ink button turns white with ink text, the gold primary turns deep amber (#8A4E0C) with white
text (white on gold would be only 1.6:1); it springs back in 120 ms. Between levels the picture fades out and in
through its colour grade, 150 ms each way.

---

## 11. Screens and flow

```
launch ──▶ HOME ──touch──▶ PLAY ──spire 0──▶ WIN ──NEXT──▶ HOME (the next level, waiting)
             │              │  └──core 0──▶ LOSE ──RETRY──▶ HOME (the same level and seed, waiting)
             └─cog─▶ SETTINGS   └─pause─▶ PAUSE ──▶ RESUME / RESTART (→ HOME, same level) / SETTINGS / HOME
```

HOME is not a menu: it is a level, waiting for a touch. Nothing in it ticks; its view animates. Sizes
are in dp; every touch target is at least 48 × 48 dp; everything sits inside the safe area.

| Screen | Contents |
|---|---|
| HOME | The level's lane (lanterns flicker, the spire pulses). The game's name across the upper third (cold start only; it fades on first touch). A `LEVEL N` chip, the star total, a settings cog at the top right. A pulsing hint `HOLD AND DRAG` with a small arc that sweeps left and right in the lower third. Any touch that is not on the cog starts the level and already fires |
| PLAY (HUD) | Top row: `LEVEL N` chip (left), the spire's health bar (centre), pause (right). Bottom right, above the gesture inset: the tide counter. In the world: the spire's number, the core's pips, the labels |
| PAUSE | The picture dims to 40%. `RESUME` (primary), `RESTART`, `SETTINGS`, `HOME`. After `RESUME` time ramps back up over 0.5 s |
| WIN | `CLEARED!`, one to three stars, a line with the time and the highest tide, `NEXT` (primary, large, bottom centre), `REPLAY` |
| LOSE | `SNUFFED OUT`, one line of advice chosen by what happened (for example `KEEP THE STREAM ON THE BIGGEST GATES`), `RETRY` (primary), `HOME` |
| SETTINGS | Switches: `SOUND`, `MUSIC`, `HAPTICS`, `AUTO-FIRE`, `REDUCE EFFECTS`. `RESET PROGRESS` (hold for one second, then confirm). The version. Three lines of credits (the font, SDL, the licence of each) |

Hints (each shown once, remembered in the save): level 1 `HOLD AND DRAG`; first pass through a gate
`EVERY WISP THAT PASSES IS DOUBLED`; level 2 `PICK THE BIGGER GATE`; level 4 `÷2 HALVES YOUR STREAM`;
level 5 `BREAK THE WALL TO REACH ×4`; the first time the enemy's line comes down past a gate row
`A THIN STREAM LETS THEM THROUGH`.

All strings are upper case, in one table, English only.

---

## 12. Sound

Everything is synthesized at start-up from small recipes (TECH_SPEC 6). No samples, no files.

**The pop ladder.** Every gate has a note from the C major pentatonic scale
(C4 D4 E4 G4 A4 C5 D5 E5 G5 A5 C6). A gate's note index is `2 × its row (from the launcher, starting at
0) + (k − 2)`, clamped to the scale: gates further up the lane and stronger gates sound higher. When
wisps pass, the gate plays its note, at most 20 times a second, louder as more pass. A flood climbing
three rows of gates plays a rising, consonant chatter instead of noise. This is the game's signature
sound: spend the effort here.

| Sound | Recipe (starting point) |
|---|---|
| `pip` (shot) | Sine at 880 Hz ± a semitone, 2 ms attack, 35 ms decay, quiet |
| Ladder note | A kalimba-like pluck: sine at `f`, plus a sine at 4`f` at a quarter of the level decaying in 60 ms, a 2 ms noise click; decay 180 ms |
| `tick` (clash) | Band-passed noise at about 3 kHz, 12 ms, random pitch ±20% |
| `pff` (snuffed) | Low-passed noise, 90 ms, falling cut-off |
| `chip`, `shatter_small`, `shatter_big` | Noise bursts of 30 / 160 / 600 ms with a falling filter; the big one adds a falling sine and four glassy pings |
| `thump` (spire hit) | Sine falling from 120 to 60 Hz in 80 ms |
| `crack` | A sharp noise burst plus a falling minor third |
| `pop_low` (brute) | Sine falling from 300 to 90 Hz in 120 ms plus a click |
| `core_hit` | Two low sines a semitone apart, 250 ms |
| `rise` | Five ladder notes up the scale, 40 ms apart |
| `win_chord`, `star` | A C major ninth chord in plucks, rolled upward over 400 ms; one high ping per star (E6, G6, C7) |
| `lose` | Two soft notes falling a minor third, slow decay |
| `ui_tap` | A short ladder note at C5, quiet |

**Music.** A quiet ambient bed in the same key so the ladder always fits: a four-bar loop at 76 beats a
minute, a slow pad on C major ninth for two bars and A minor ninth for two, a soft low pulse on beats
one and three, and sparse plucks from a seeded pattern. It follows the tide on a logarithmic scale: more
plucks and a brighter pad as the flood grows. It ducks 6 dB for 1.5 s under the win and lose sounds.

**Mix.** 48 kHz stereo. 32 voices; each sound has its own voice limit and rate limit; the oldest quiet
voice is stolen first. Sounds are placed left to right by the event's `x`. The music sits 9 dB under the
effects. A soft limiter holds the output at −1 dBFS. Targets for the offline "flood" render: true peak
at most −1 dBFS and integrated loudness between −20 and −14 LUFS (measured with ffmpeg's `ebur128`).

---

## 13. Haptics

| Name | Shape |
|---|---|
| Light tick | 8 ms, low amplitude |
| Medium | 30 ms, mid amplitude |
| Strong | 45 ms, full amplitude |
| Pulse train (spire) | Light ticks, at most 8 a second, while damage flows |
| Win | 30 ms, pause 60 ms, 30 ms, pause 60 ms, 90 ms |
| Lose | 250 ms, low amplitude |

All of them respect the `HAPTICS` switch and a global limit of 12 events a second. A vibrator without
amplitude control gets durations only.

---

## 14. Accessibility and safety

- `AUTO-FIRE` for players who cannot hold; `REDUCE EFFECTS` removes camera shake, slow motion and edge
  flashes and halves bloom.
- No full-screen change of brightness happens more than three times a second, in any mode. The win's
  brightening takes 2 s; the core-hit flash is at the edges only.
- Meaning never depends on colour alone (9.4).
- Text is large: no string is smaller than 14 dp; numbers in the world are far larger.
- The game is silent-safe: every sound cue also has a picture cue.
- No data leaves the device. There is nothing to collect it.

---

## 15. Names

| Name | Meaning |
|---|---|
| Wisp | The player's unit |
| Gloom, brute | The enemy's unit, and its strong form |
| Lantern, launcher | The player's cannon |
| Core | The lantern's health (ten pips) |
| Spire | The enemy's base |
| Gate | A multiply (×) or divide (÷) opening |
| Barrier, bumper, snuffer | Wall with health, solid pillar, patrolling orb |
| Tide | The total power of the wisps on the lane |
| Base flow, `fight`, `P0` | The enemy's steady strength, as a share of what the player's side can deliver (5.9) |
| World | A palette, shared by ten levels |

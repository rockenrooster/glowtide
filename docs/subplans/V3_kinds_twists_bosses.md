# V3 — Kinds, twists, bosses

Size L. Needs V2.

**Goal.** A level is no longer always "flood the spire". Holdout, Colossus, Twins and Festival exist
beside Siege, each with its own goal bar, result and bot rules; levels have twists; the tenth levels are
four different named bosses. The owner plays one of each.

## Read first

`VARIETY_DESIGN.md` 6, 7, 8, 12.2 (step 6 and the lines that differ by kind), 13, 14.
`VARIETY_TECH.md` 2 (`kind`, `goal`, `goal2`, `twists`), 3, 4 (goals in the bot), 7 (level mesh, the
colossus's sizes), 9.1, 10. `GAME_DESIGN.md` 5.10, 5.11, 7.5, 10 (the win and lose sequences), 11.

## Tasks

1. **The goal in the data and the rules**: `kind`, `goal`, `goal2` replace `spireHealth`; step 15 and
   17 of the tick by kind (VT 3.2); stars by kind in `Game::finishLevel`; the HUD's goal bar by kind;
   the goal banner in the level's first 1.2 s; the result's title (`CLEARED!`, `DAWN!`, `FESTIVAL!`).
   Siege must be bit-for-bit what it was: the determinism hashes of levels 1 to 5 do not change in
   this task.
2. **Twists** (VD 7): `twists[4]` replaces `bursts[8]`; triggers by time, by the goal's progress, by
   the core; `SURGE` and `BRUTE GUARD` are the old bursts moved over (the hand-made level 5's burst at
   50% too). Then `GEODE RAIN` with runner geodes, `ECLIPSE`, `STAMPEDE`, `SECOND WIND`,
   `GOLDEN GATE`; a banner and a cue each; a strip each. Stretch: `GATE STORM`.
3. **Holdout** (VD 6): no spire health; the dawn clock; the dark arch; the backdrop going from night to
   dawn through the colour grade the win already uses; the bot's worth of a line is the glooms it
   meets; the bake bisects the flow for a median core of 7 to 9 (VD 12.2 step 6). Hand-made level 21.
4. **Colossus** (VD 6): a body with health, not a unit (VT 3.1); its pace and sway; drawn by the unit
   shader at a new size (VT 7: selects, not an array); its number; the steps' trauma; lose when it
   arrives; the spire's shatter when it falls. Hand-made level 31.
5. **Twins** (VD 6): two spires in the level mesh with a range each; damage by side; the funnel toward
   the nearer living spire; an emitter group a spire that stops with it; wisps wasted on a fallen
   spire's side; two half-bars. Hand-made level 41.
6. **Festival** (VD 6): no enemy, no losing; the rain of geodes (up to 16 alive); the paper-lantern
   target; stars by geodes opened; `FESTIVAL!`. Hand-made level 15.
7. **Bosses** (VD 8): the four scripts as twists at 66% and 33%; `WARD WALL` and `CUB`; the boss's name
   on the `LEVEL N` chip and as the goal banner; notches on the goal bar. They rotate from level 10.
8. **A simple schedule for now** (the calendar proper is V4): levels ending in 5 are festivals, in 0
   bosses; places 3 and 7 of each world take Holdout, Colossus and Twins in rotation, each from its
   introduction level on; no kind but Siege twice in a row. Twists by hardness is V4: here every level
   from 43 has one twist from those built, chosen by the seed.
9. **Bots and the bake by kind** (VT 4, VD 12.2): the goal's size per kind; the lines that apply per
   kind; the report's kind column; the band table by kind as well as by level.
10. **Flow test**: one level of each kind reaches its result screen with the right title and stars;
    `SECOND WIND` fires when the core is forced to 3.
11. **Pictures**: `kind_holdout`, `kind_colossus`, `kind_twins`, `kind_festival`, `boss_<name>` ×4; a
    strip of the colossus arriving (the lose), of dawn, of each twist; each kind's HUD in the eight
    frames with the lint.

## Exit checks

- [ ] Siege is unchanged: levels 1 to 5 hash as before task 1; the Siege levels' band table matches
      V2's within the bake's own noise (the same seeds give the same numbers).
- [ ] Unit tests: each kind's win and lose on a minimal level; stars by kind; a wisp on a fallen twin's
      side does no damage; the colossus absorbs every wisp that touches it in one tick and loses the
      level at `z` ≤ 3; a festival cannot be lost and ends on its tick; each twist's trigger fires once.
- [ ] The bake: every level passes its kind's lines within eight seeds. Holdout: `greedy`'s median core
      7 to 9 and `podless` loses. Colossus and Twins: `greedy`'s median time 0.7 to 1.2 `T`. Festival:
      `greedy` earns three stars, `naive` one or two. If a kind cannot be made to pass, say so with the
      table and propose dropping it (VARIETY_PLAN 8, risk 5).
- [ ] `--sim variety` on the new table: every kind appears at least five times in each hundred levels
      from 50 on; no kind but Siege twice in a row. (VD 6's shares are V4's check.)
- [ ] The flashing audit passes with `ECLIPSE`, dawn and the colossus's fall, with and without `REDUCE
      EFFECTS`. `ECLIPSE` is still readable: the greyscale sheet of an eclipsed frame tells wisps from
      glooms.
- [ ] The performance contract on the Moto; `bench_play` on the heaviest level of each kind (a
      festival with sixteen geodes and three lanterns is the new candidate for the heaviest picture: if
      it beats `bench_flood`, grow the bench scene, VT 11).
- [ ] `scripts\test.ps1`, the flow test (PC, Moto), the layout lint (eight frames, every kind's HUD),
      `matrix-android.ps1` on three phones with a picture check on a colossus picture.

## Owner checkpoint

The dev build with a list of level numbers: one of each kind, the four bosses, a level with each twist.
Ask:

1. Which kinds are fun? Any to drop or to see more of?
2. Holdout: tense or tedious? Colossus: fair? Twins: do you see that you must cross over?
3. Is the festival a welcome breather every tenth level, or too often?
4. Do twists read (did you understand what `ECLIPSE` and `GOLDEN GATE` did without being told)?
5. Are the bosses different from each other?

A kind the owner drops leaves its share to Siege and its introduction slot to a hand-made level.

## Not in this phase

Recipes, the calendar's rhythm and hardness, the no-repeat rule in the bake, set pieces, 600 levels,
new worlds.

## Notes

- Holdout and Colossus stand closest to the contest's edge (GD 5.9): a little too much enemy and the
  level collapses, a little too little and nothing happens. Their lines are medians over ten runs for
  that reason; look at the spread too, and report it.
- The colossus is the one new thing drawn through the unit shader. It is exactly the kind of change
  that broke the 12R: test its picture on all three phones before building on it.
- `SECOND WIND` is the game's one comeback. It must not make losing impossible: it appears once a
  level, and its geode still has to be cracked while the enemy is at the door. Check with the `idle`
  and `naive` bots that they still lose the levels they lost before it existed.
- A festival's three-star count is one less than the greedy bot's median, so a person who plays well
  gets three stars and a person who is perfect is not asked for more than the bot.

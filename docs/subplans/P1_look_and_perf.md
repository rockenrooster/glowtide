# P1 — Look and performance proof

Size L. Needs P0. **This is the go/no-go gate of the project.**

**Goal.** The engine draws the concept frame for real: the lane, the backdrop, gates with labels, a
synthetic crowd at the caps, the light the crowd throws, particles, glow and the HUD. It does so inside
the performance contract on the floor phone, and the owner has looked at it on that phone and approved
the look. No game rules yet: the crowd follows a script.

## Read first

`PLAN.md` 5 and 9. `GAME_DESIGN.md` 3, 9 and the "always on" lines of 10. `TECH_SPEC.md` 5 (all), 7.4,
8.1, 8.3 and 8.5. Open `docs/concept/concept_gameplay.png` and read `docs/concept/make_concept.py`: its
camera, lane and palette numbers are the starting values.

## Ask the owner first

The font download: Fredoka (bold) from `github.com/google/fonts`, directory `ofl/fredoka`, SIL OFL 1.1,
well under 1 MB; the files kept are the font and `OFL.txt`. Start on tasks 1 to 7, which need no font.

## Tasks

Ordered so that a picture exists by the end of task 4 and gets richer from there.

1. **Camera** with the fit rule and the aspect matrix (TECH_SPEC 5.2), and the test that sweeps `LEN`
   30 to 44 across the seven frames.
2. **Shader plumbing and tweakables.** The shared prelude (version line, precision, `grade()` with the
   vignette), embedding, build-time validation, start-up compile and link checks (TECH_SPEC 5.1). The
   `Tuning` structure and `--set` (TECH_SPEC 7.4): every art number of this phase is one from the start.
3. **Palette and props.** The world colours as one structure and the palette lint (GAME_DESIGN 9.2);
   the mesh generators; the static level buffer with the lane, rails, lantern posts, islets, spire, gate
   posts, bumpers and a barrier; the launcher as a moving prop; the prop shader with fog (TECH_SPEC 5.6).
4. **Backdrop** (gradient, glow behind the spire, mist, motes, dither). `--picture elements` now shows
   an empty world.
5. **The crowd script.** One small file that produces unit arrays without the simulation: a stream,
   its fans after each gate, a flood, a front line and the enemy's side, from seeded analytic motion. It
   reproduces the concept frame's layout for `--picture concept` and holds both crowds at given counts
   for `bench_flood`. It is scaffolding: keep it tiny.
6. **Units** (TECH_SPEC 5.4): the wisp and gloom impostors, power tiers, hop, squash and birth pop, the
   far-to-near counting sort, the instance buffer.
7. **Light map** and the lane's lighting (TECH_SPEC 5.5).
8. **Gates and labels.** `tools/bake_font.py`, the committed atlas and metrics, the sample sheet, the
   text shader; gate panels cut into the sorted unit stream; world labels with the minimum size and
   stroke rules (TECH_SPEC 5.3, 5.6, 5.7).
9. **Particles** (TECH_SPEC 5.8), fed by scripted events from the crowd script: sparks along the front
   line, pops at the gates, hits on the spire.
10. **Glow buffer, blur chain, bloom pass** (TECH_SPEC 5.3, 5.9), with the blend functions exactly as
    specified: "over" for what glows, the screen blend for the bloom.
11. **The HUD of the concept frame**, static: level chip, spire bar, pause, tide counter. This needs
    the first cut of the UI layer (rectangles and text, dp scale, safe area). Logic comes in P3.
12. **Pictures and the review set**: `concept`, `elements`, `unit_tiers`, `density_1200`,
    `density_2400`, `density_4000`, `label_over_flood`, `world_1` to `world_3`, and `concept` at three
    glow strengths (through `--set`); `scripts/review.ps1 -Set p1`.
13. **`bench_flood`** and the contract run on the Moto. If it misses, apply TECH_SPEC 8.5 in order,
    one step at a time, measuring after each. Then the tablet, and the Fire and the OnePlus if reachable.
14. **The comparison sheet**: the concept frame, the desktop `concept` picture and the Moto's screen
    capture of it, side by side in one image.

## Exit checks

- [ ] `scripts\review.ps1 -Set p1` exits 0: every picture rendered, no GL message, shaders valid.
- [ ] You compared the `concept` picture with the concept frame and wrote the answers in
      `PROGRESS.md`, each backed by a number where one exists:
      - with the glow strength set to 0, the four gameplay colours sampled at known unit and gate
        pixels are within 8 levels per channel of GAME_DESIGN 9.2;
      - the label check passes in all seven frames (height at least 3% of the frame, stroke at least
        3 px), and in `label_over_flood` every label is legible to you;
      - in a greyscale copy, wisps and glooms are still told apart;
      - the lane under the flood is at least 1.5 times as bright as the empty lane beside it;
      - in `density_2400`, fewer than 2% of the flood's pixels are 255 in all three channels (the
        crowd is gold, not burnt out);
      - in a 1080×2400 capture of the empty world, the mean values of neighbouring backdrop rows never
        differ by more than 2 levels (no banding);
      - no unit or prop is cut by the lane or drawn under it.
- [ ] The palette lint passes for worlds 1 to 3; the camera fit test passes.
- [ ] **The contract:** `bench_flood` on the Moto after a 180 s soak, 60 s measured: late share at most
      5%, worst frame at most 50 ms, zero heap allocations per frame, with at least 1,200 wisps (the
      aim is 2,400 wisps and 1,000 glooms). The CPU phases and the unit and particle counts are recorded.
- [ ] The tablet: the contract at 90 Hz, or at 60 Hz with the reason recorded. Its capture of `concept`
      shows no defect the desktop's does not (reviewed by eye, and the mean difference of the two
      images scaled to 64 pixels wide stays under 12 levels).
- [ ] `scripts\test.ps1` still exits 0. APK size recorded.
- [ ] If the contract is still missed after step 7 of TECH_SPEC 8.5: stop here and report the measured
      options to the owner.

## Owner checkpoint

Put `bench_flood` on the Moto in their hand (it is the look in motion) and send the comparison sheet.
Ask:

1. Is it beautiful? If not: too dark, too busy, too plain, wrong colours?
2. Crowd density: which of the three density pictures?
3. The font: the sample sheet.
4. Glow: which of the three strengths?
5. Worlds 1 to 3: any they dislike?

Their answers become the defaults of the tweakables and the caps, and are logged. If they want a
different look altogether, change `make_concept.py` first, agree on the frame, then the engine.

## Not in this phase

The real simulation and its rules, input beyond P0's touch test, screen logic, sound, levels.

## Notes

- There is one quality level (TECH_SPEC 5.10). Build the picture that passes on the Moto; do not add
  switches for stronger devices.
- Look at the Moto's and the tablet's captures after every renderer task, not at the end: the two
  drivers disagree with the PC, and `mediump` only bites on the tablet (TECH_SPEC 10).
- The far-to-near order, the gate-row cuts and "labels win" are what make a dense crowd readable. Do
  not replace them with depth writes and alpha test to save an hour.
- If a crowd burns out to white, the blend function is wrong, not the art (TECH_SPEC 5.3).

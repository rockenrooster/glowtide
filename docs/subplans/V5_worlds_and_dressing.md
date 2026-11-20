# V5 — Worlds and dressing

Size M. Needs V4 and its checkpoint. **The first phase to cut** (VARIETY_PLAN 8): if time is short,
skip it whole and go to V6.

**Goal.** A world is more than five colours returning every fifty levels: ten worlds, each leaning
toward its own kind of lane, with its own props, a sky feature and its own music. The owner picks the
palettes and hears the music on a phone.

## Read first

`VARIETY_DESIGN.md` 15, 10.2. `VARIETY_TECH.md` 7, 8, 12 (trap 1 above all). `GAME_DESIGN.md` 9.1, 9.2
(the palette rules), 12 (the music and the mix), 14.

## Tasks

1. **Five new palettes** (VD 15), each passing the palette lint (`testPalettes`: the lane's luminance,
   the backdrop's ceiling of 0.45, gold, aqua and rose against both ends of the lane, the gloom's
   eyes). Render `world_6` … `world_10` and, for each, two alternatives; put all fifteen on one sheet
   for the owner. The four gameplay colours do not change (GD 9.1).
2. **Signatures** (VD 15): a table in `calendar.cpp` doubling the weights of a world's recipes and
   plans. Bump `kGeneratorVersion`, bake, and check both contracts again: a signature must not make a
   world's ten levels alike (the variety rule still holds inside every world).
3. **Props**: two more families beside the islets (arches, lantern clusters), built by `MeshBuilder`
   like the islets; one family a world.
4. **Sky features** in `shaders\backdrop.frag`: aurora ribbons, a low moon, falling motes, a far second
   spire; one a world, chosen by a uniform the frame block already has room for or a new member of it
   (std140: keep `FrameUniforms`' size assert true on both sides). No textures. No constant array
   indexed by anything but a constant. Measure the backdrop's cost on the Moto before and after.
5. **The world's name** as the goal banner on a world's first level (`WORLD 6  FROST GLASS`).
6. **Music a world** (VD 15, VT 8): a table of chord pairs and tempos; the pad pair built for the
   world at its first level. Measure one pad pair's build time on the Moto first: under 150 ms, build
   it inside the level fade; over, on a worker thread with the old pad playing until it is ready. A
   boss adds the pulse on every beat. `--render-audio music --world N` for each world; the music
   stays 9 ± 2 dB under the flood (GD 12) and has no jump at the loop's seam.
7. **Pictures**: `world_1` … `world_10` with the label check; the greyscale sheet for the three
   lightest backdrops (a light backdrop is where a pale geode or a gate label is lost).

## Exit checks

- [ ] The palette lint passes for all ten worlds; the label check passes in each.
- [ ] Both contracts hold on the re-baked table: `--sim variety` exits 0; every level passes its lines.
- [ ] The performance contract on the Moto in the world whose backdrop costs most; the picture check on
      all three phones for every sky feature (four pictures a phone).
- [ ] Music renders: each world's loop is seamless (no sample-to-sample jump above 0.2 of full scale),
      9 ± 2 dB under the flood render, and every note is in the pentatonic family (list the notes of
      each world's chords in `PROGRESS.md`).
- [ ] The sound bank's build at start-up is still under 150 ms on the Moto; a world change does not
      drop a frame (a `bench_play` run across a world's first level: no frame over 50 ms).
- [ ] `scripts\test.ps1`, the flow test, the layout lint and `matrix-android.ps1` on three phones pass.

## Owner checkpoint

The sheet of fifteen palette candidates; a level in each of the ten worlds on a phone; the ten music
loops as WAV files and on the phone. Ask:

1. For each new world: which of the three palettes, or none?
2. Do the worlds feel different to play, or only to look at?
3. The music: keep one loop a world? Any world's loop to change or drop?
4. Sky features: any that distracts from the lane?

## Not in this phase

New rules, kinds or recipes; changes to the four gameplay colours; recorded samples or music from a
file (a download: the owner's yes first).

## Notes

- The backdrop is drawn only where nothing else is (TS 5.3), so a sky feature costs fill rate at
  the top of the screen and the sides only. Still measure: the Moto's margin is real but not large.
- Gold Noon is the risk: the lightest backdrop the lint allows, under gold wisps. If the greyscale
  sheet loses the flood against it, darken it rather than argue with the lint.
- A signature is a lean, not a wall: a world that only ever uses its own two recipes is ten alike
  levels in a row, which is what this pass exists to end.

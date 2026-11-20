# P4 — Audio

Size M. Needs P2. Touches `src/audio`, the app's device code and a few call sites in the view, so it can
run in a second worktree beside P3 or P5.

**Goal.** The game sounds like a flood of chimes: every sound of GAME_DESIGN 12 synthesized in code,
the pop ladder that climbs as the stream multiplies, a quiet ambient bed that follows the tide. PC runs
stay silent. Everything is measured from offline renders, and the owner has listened.

## Read first

`GAME_DESIGN.md` 10 (the sound column), 12 and 14. `TECH_SPEC.md` 2.3 (silent runs) and 6.

Remember PLAN 0, rule 1: you cannot hear. Never write that something sounds good. Report peaks,
loudness, pitch accuracy and timing, and hand the files to the owner.

## Tasks

1. **Synth** (`gt_audio`): oscillators, noise, one-pole filters, envelopes; a recipe table for every
   sound of GAME_DESIGN 12, including the eleven ladder notes. Built at start-up. Measure the build
   time on the Moto; above 150 ms, build it on a worker thread while HOME is showing.
2. **Mixer:** 32 voices, per-sound voice and rate limits, stealing the oldest quiet voice, pan, pitch,
   the master limiter; the command ring from the game thread.
3. **Device** in the app: the SDL stream and its callback, `--silent`, `--no-audio`, stopping and
   restarting across the Android lifecycle, callback timing.
4. **Events to sounds** in the view: the mapping of GAME_DESIGN 10, with rate limits and aggregation
   on the game thread; the ladder's note rule; pan from the event's `x`.
5. **Music:** the pad, the pulse and the plucks; following the tide; ducking under the win and lose
   sounds; the SOUND and MUSIC switches.
6. **Offline renders** (`--render-audio`, TECH_SPEC 6): `ladder`, `flood`, `ui`, `win`, `lose`,
   `music`; `tools/wav_metrics.py`; loudness and true peak through ffmpeg; `scripts/review.ps1 -Set p4`
   writes the WAV files and one table of their numbers into `captures\p4\`.
7. **Three ladder timbres** for the owner to choose from (the kalimba of GAME_DESIGN 12, a glass tone,
   a soft bell), each rendered through the same `flood` scene.
8. **Device check:** `bench_flood` with sound on the Moto and the tablet for ten minutes.

## Exit checks

- [ ] The `flood` render (30 s): true peak at most −1 dBFS, integrated loudness between −20 and
      −14 LUFS, no clipped samples, DC offset under 0.001, less than 5% silence.
- [ ] The `ladder` render: the fundamental of each of the eleven notes is within 5 cents of its pitch
      (a numpy FFT check).
- [ ] The `music` render (60 s): 9 ± 2 dB quieter than `flood`; no sample-to-sample jump above 0.2 of
      full scale anywhere, including where the four-bar loop repeats.
- [ ] Silence on the PC is proven, not assumed: with `--silent` the app reports a non-zero peak before
      the mute and an all-zero buffer handed to the device.
- [ ] On the Moto, `bench_flood` with sound: the callback's P99 duration is at most 25% of its period,
      no late callback in ten minutes (a gap above two buffer periods, TECH_SPEC 6), and the frame
      contract still holds.
- [ ] The callback allocates nothing and takes no lock (the allocation counter covers its thread; the
      code has no mutex).
- [ ] Stress: two hundred commands in one frame leave at most 32 voices, no crash, and the dropped
      commands are counted.
- [ ] A test on the view's sound routing: with the SOUND setting off no command reaches the mixer;
      with MUSIC off the music voices stop within a second. (The switches on the SETTINGS screen are
      P3's; the flow test covers them end to end in P6, when both phases are in.)
- [ ] `scripts\test.ps1` exits 0.

## Owner checkpoint

Hand over `captures\p4\` (the three ladder timbres through the flood, the win, the lose, the music
alone) and the build on a phone. Ask:

1. Which ladder timbre?
2. When the flood is big, is it pleasant or too busy? (Rate limits and levels are tweakables.)
3. Music: keep it, change its level, or leave it off by default?
4. Any sound that grates?

Two rounds. If synthesized sound still does not satisfy, propose a small set of CC0 samples as the
fallback: that is a download and needs their yes, with source and licence listed.

## Not in this phase

Recorded samples, voice, music beyond the ambient bed, per-world music.

## Notes

- The ladder is the signature. If time is short, spend it there and keep every other sound plain.
- Keep all gameplay sounds in the pentatonic scale or unpitched. A stray semitone under a thousand
  pops is what makes a casual game sound cheap.
- A phone speaker has no bass. Check each render's energy above 300 Hz: a sound that lives only below
  that will not be heard (report it; do not judge it).

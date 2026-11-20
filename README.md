# GLOWTIDE

A one-thumb portrait game for Android: hold and drag to aim a lantern that pours a stream of light-wisps up a
causeway. Multiply gates (×2 … ×5) turn a trickle into a flood, divide gates (÷2, ÷3) waste it, and the gloom's crowd
comes down to meet you. Flood the spire to win; let the glooms reach your lantern and the light goes out. 300 levels,
five worlds, a boss every tenth level. Every sound, the music, the meshes and the font atlas are made in code.

**Download:** [the latest build](https://github.com/rockenrooster/glowtide/releases/latest/download/glowtide.apk)
(Android 10 or later, 64-bit ARM, OpenGL ES 3.0). On the phone, open the downloaded file; Android asks once to allow
installs from the browser. Every push to `main` builds a new release ([all builds](https://github.com/rockenrooster/glowtide/releases)),
and the game shows an UPDATE button on its home screen when a newer one is out.

## Building

C++20, SDL 3, OpenGL ES 3.0. Windows with PowerShell 7 is the development host (the scripts in `scripts\`); GitHub's Linux
runner builds the releases (`.github/workflows/release.yml`).

| Task | Command |
|---|---|
| Third-party sources (SDL, pinned) | `scripts\fetch-deps.ps1` (Linux: `scripts/fetch-deps.sh`) |
| Desktop build and tests | `scripts\build-desktop.ps1`, `scripts\test.ps1` |
| The APK | `scripts\build-android.ps1 -Config dev\|release` |

`CLAUDE.md` lists every command; `docs/PLAN.md`, `docs/GAME_DESIGN.md` and `docs/TECH_SPEC.md` are the design;
`docs/PROGRESS.md` records what was built, measured and decided; `docs/RELEASE_PIPELINE.md` the releases and the updater.

## Licences

GLOWTIDE's own code and assets carry no licence yet (all rights reserved). Third-party: SDL 3 (zlib), Roboto (Apache
2.0, only a baked atlas ships). See `docs/LICENSES.md`.

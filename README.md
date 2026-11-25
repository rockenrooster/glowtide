# GLOWTIDE

A one-thumb portrait game for Android: hold and drag to aim a lantern that pours a stream of light-wisps up a
causeway. Multiply gates (×2 … ×5) turn a trickle into a flood, divide gates (÷2, ÷3) waste it, and the gloom's crowd
comes down to meet you. Flood the spire to win; let the glooms reach your lantern and the light goes out.

Crystal geodes on the lane hold boons for the level (more lanterns, a faster or brighter stream, a kindled lane, a
ward, a comet, a hush); hoppers, breathing gates, sliding walls and side doors move the lane; holdouts, colossi, twin
spires, festivals and four bosses break up the sieges, and twists turn a level halfway through. 600 levels, no two
alike within forty of each other, twelve set pieces among them, then the game goes on with mirrored replays. Ten
worlds of ten levels, each with its own colours, sky, props, music and a kind of lane it leans to. Every sound, the
music, the meshes and the font atlas are made in code.

**Download:** [the latest build](https://github.com/rockenrooster/glowtide/releases/latest/download/glowtide.apk)
(Android 10 or later, 64-bit ARM, OpenGL ES 3.0). On the phone, open the downloaded file; Android asks once to allow
installs from the browser. Every push to `main` builds a new release ([all builds](https://github.com/rockenrooster/glowtide/releases)),
and the game shows an UPDATE button on its home screen when a newer one is out.

**In a browser (iPhone, iPad, a PC):** the same game built with Emscripten for WebAssembly and WebGL 2. On the PC that
hosts it, `scripts\build-web.ps1` then `scripts\serve-web.ps1`; a phone on the same network opens the address it prints
in Safari (Share, then Add to Home Screen, gives it the whole screen). Progress is kept in the browser.

## Building

C++20, SDL 3, OpenGL ES 3.0. Windows with PowerShell 7 is the development host (the scripts in `scripts\`); GitHub's Linux
runner builds the releases (`.github/workflows/release.yml`).

| Task | Command |
|---|---|
| Third-party sources (SDL, pinned) | `scripts\fetch-deps.ps1` (Linux: `scripts/fetch-deps.sh`) |
| Desktop build and tests | `scripts\build-desktop.ps1`, `scripts\test.ps1` |
| The APK | `scripts\build-android.ps1 -Config dev\|release` |
| The browser build, and serving it to phones on this network | `scripts\build-web.ps1`, `scripts\serve-web.ps1` |

`CLAUDE.md` lists every command; `docs/PLAN.md`, `docs/GAME_DESIGN.md` and `docs/TECH_SPEC.md` are the design;
`docs/PROGRESS.md` records what was built, measured and decided; `docs/RELEASE_PIPELINE.md` the releases and the updater.

## Licences

GLOWTIDE's own code and assets carry no licence yet (all rights reserved). Third-party: SDL 3 (zlib), Roboto (Apache
2.0, only a baked atlas ships). See `docs/LICENSES.md`.

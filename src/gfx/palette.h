#pragma once
// Colours (GAME_DESIGN 9.2): four gameplay colours that never change, and ten worlds (VARIETY_DESIGN 15) that change the
// backdrop, the lane, the rail accent and the props. Values are display-space RGB 0..1.

#include "core/mathx.h"

namespace gt {

struct WorldPalette {
    const char* name;
    V3 backdrop[4];  // far (top of the screen) to near
    V3 laneFar, laneNear, rail;
    V3 propBody, propLight, propDark;
    V3 brighten;     // the dawn at the win
};

struct GamePalette {
    V3 wisp, wispLight, wispRim;
    V3 gloom, gloomLight, gloomRim, gloomEye;
    V3 good, bad;
    V3 spire, spireLight, spireDark, crack;
    V3 launcher, launcherLight, launcherDark;
    V3 ink, text;
};

constexpr int kPaletteCount = 10;
const WorldPalette& worldPalette(int world);  // 1..10
// The two alternatives drawn for each of worlds 6 to 10 (alt 1 or 2) for the owner's sheet; alt 0 is the world's own.
const WorldPalette& worldPaletteAlt(int world, int alt);
// What a world draws besides its colours (VARIETY_DESIGN 15): its sky feature (0 none, 1 aurora ribbons, 2 a low moon,
// 3 falling motes, 4 a far second spire) and its family of floating props (0 islets, 1 arches, 2 lantern clusters).
int skyFeature(int world);
int propFamily(int world);
const GamePalette& gamePalette();
V3 hexColor(unsigned rgb);

// WCAG relative luminance and contrast ratio (the palette lint, GAME_DESIGN 9.2).
float luminance(V3 c);
float contrast(V3 a, V3 b);

}  // namespace gt

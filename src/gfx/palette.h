#pragma once
// Colours (GAME_DESIGN 9.2): four gameplay colours that never change, and five worlds that change the backdrop, the
// lane, the rail accent and the props. Values are display-space RGB 0..1.

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

const WorldPalette& worldPalette(int world);  // 1..5
const GamePalette& gamePalette();
V3 hexColor(unsigned rgb);

// WCAG relative luminance and contrast ratio (the palette lint, GAME_DESIGN 9.2).
float luminance(V3 c);
float contrast(V3 a, V3 b);

}  // namespace gt

#pragma once
// The HUD and screen layout helpers (GAME_DESIGN 11, TECH_SPEC 5.11): dp units inside the safe area.

#include "game/input.h"
#include "gfx/text.h"

namespace gt {

struct Layout {
    int w = 1, h = 1;
    float dp = 1;
    SafeArea safe;
    float px(float dpv) const { return dpv * dp; }
    float top() const { return safe.top; }
    float bottom() const { return float(h) - safe.bottom; }
};

struct HudState {
    int level = 1;
    float spireFrac = 1;   // 0..1
    long long tide = 0;
    float tidePunch = 0;
    // The banner showing now (a boon, the flood's marks; later a goal or a twist): one at a time, 0.9 s.
    char banner[40] = "";
    float bannerTime = 0;  // seconds left
    // Boons held this level (VARIETY_DESIGN 14): the tray, bottom left, a chip a boon in the order they came.
    int trayCount = 0;
    u8 trayBoon[4] = {};
    int trayHeld[4] = {};
    float trayPunch[4] = {};
    float trayDrain[4] = {};  // a timed boon (HUSH): the share of its time left, drawn as a bar draining along the chip
    // Icons flying from an opened geode to its chip (0.4 s): where they started on screen and how far they are (0..1).
    struct Fly {
        u8 boon = 0;
        float x0 = 0, y0 = 0, t = 1;
    };
    Fly fly[4];
    bool pausePressed = false;
    bool drawPause = true;
    float opacity = 1;
};

// Ink panels, white text, the accent colours (GAME_DESIGN 9.2).
constexpr u32 kInk = 0x0E0E2C;
constexpr u32 kWhite = 0xFFFFFF;
constexpr u32 kGold = 0xFFC654;
constexpr u32 kRose = 0xFF60D6;

struct Rect {
    float x, y, w, h;  // pixels, top-left
    bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    float cx() const { return x + w * 0.5f; }
    float cy() const { return y + h * 0.5f; }
};

// The pause button's touch rectangle (48 dp or more).
Rect hudPauseRect(const Layout& L);
// Where the boon tray's chip `i` is (bottom left, mirroring the tide counter), and the tide counter itself.
Rect hudTrayChip(const Layout& L, int i);
Rect hudTideRect(const Layout& L, long long tide);
// A boon's icon in gold (the geode's badge drawn flat): a lantern, two chevrons, a teardrop, a ring, a gate, a comet, a
// crescent.
void drawBoonIcon(UiBatch& ui, int boon, float cx, float cy, float size, Rgba c);
void drawHud(UiBatch& ui, const Layout& L, const HudState& s);
void drawStar(UiBatch& ui, float cx, float cy, float r, Rgba c);
void formatThousands(long long v, char* buf, int n);

}  // namespace gt

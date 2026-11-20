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
    int bannerMark = 0;
    float bannerTime = 0;  // seconds left
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
void drawHud(UiBatch& ui, const Layout& L, const HudState& s);
void drawStar(UiBatch& ui, float cx, float cy, float r, Rgba c);
void formatThousands(long long v, char* buf, int n);

}  // namespace gt

#include "game/hud.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace gt {

void formatThousands(long long v, char* buf, int n) {
    char tmp[32];
    std::snprintf(tmp, sizeof tmp, "%lld", v < 0 ? 0LL : v);
    const int len = int(std::strlen(tmp));
    int o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        buf[o++] = tmp[i];
        const int rest = len - 1 - i;
        if (rest > 0 && rest % 3 == 0 && o < n - 1) buf[o++] = ',';
    }
    buf[o] = 0;
}

void drawStar(UiBatch& ui, float cx, float cy, float r, Rgba c) { ui.star(cx, cy, r, c); }

Rect hudPauseRect(const Layout& L) {
    const float s = L.px(48);
    return {float(L.w) - L.safe.right - L.px(12) - s, L.top() + L.px(8), s, s};
}

void drawHud(UiBatch& ui, const Layout& L, const HudState& s) {
    const Rgba ink = rgba(kInk, u8(205 * s.opacity));
    const Rgba edge = rgba(kWhite, u8(46 * s.opacity));
    const Rgba white = rgba(kWhite, u8(255 * s.opacity));
    const Rgba inkStroke = rgba(kInk, u8(255 * s.opacity));
    const float top = L.top() + L.px(8);
    const float rowH = L.px(48);
    // Level chip.
    char buf[48];
    std::snprintf(buf, sizeof buf, "LEVEL %d", s.level);
    const float textSize = L.px(19);
    const float chipW = UiBatch::textWidth(buf, textSize) + L.px(30);
    const float chipX = L.safe.left + L.px(12);
    ui.rect(chipX + chipW * 0.5f, top + rowH * 0.5f, chipW, L.px(40), L.px(20), ink, edge, L.px(1));
    ui.text(buf, chipX + chipW * 0.5f, top + rowH * 0.5f, textSize, white, inkStroke, 0, Align::Center);
    // Pause.
    const Rect pr = hudPauseRect(L);
    if (s.drawPause) {
    const Rgba pfill = s.pausePressed ? rgba(kWhite, u8(255 * s.opacity)) : ink;
    const Rgba pbar = s.pausePressed ? rgba(kInk, u8(255 * s.opacity)) : white;
    ui.rect(pr.cx(), pr.cy(), pr.w * (s.pausePressed ? 0.94f : 1.0f), pr.h * (s.pausePressed ? 0.94f : 1.0f), L.px(14), pfill, edge,
            L.px(1));
    ui.rect(pr.cx() - L.px(6), pr.cy(), L.px(6), L.px(20), L.px(3), pbar);
    ui.rect(pr.cx() + L.px(6), pr.cy(), L.px(6), L.px(20), L.px(3), pbar);
    }
    // The spire's health bar between them.
    const float bx0 = chipX + chipW + L.px(14), bx1 = pr.x - L.px(14);
    if (bx1 - bx0 > L.px(40)) {
        const float bh = L.px(14), by = top + rowH * 0.5f;
        ui.rect((bx0 + bx1) * 0.5f, by, bx1 - bx0, bh, bh * 0.5f, ink, edge, L.px(1));
        const float inner = (bx1 - bx0 - L.px(4)) * clampv(s.spireFrac, 0.0f, 1.0f);
        if (inner > L.px(2)) {
            ui.rect(bx0 + L.px(2) + inner * 0.5f, by, inner, bh - L.px(4), (bh - L.px(4)) * 0.5f, rgba(kRose, u8(255 * s.opacity)));
            ui.rect(bx0 + L.px(2) + inner * 0.5f, by - L.px(2), maxv(0.0f, inner - L.px(4)), L.px(3), L.px(1.5f),
                    rgba(0xFFAAE8, u8(200 * s.opacity)));
        }
    }
    // The tide counter, bottom right, above the gesture inset.
    formatThousands(s.tide, buf, sizeof buf);
    const float punch = 1.0f + 0.3f * s.tidePunch;
    const float ts = L.px(24) * punch;
    const float tw = UiBatch::textWidth(buf, ts) + L.px(62);
    const float ty = L.bottom() - L.px(24) - L.px(26);
    const float tx = float(L.w) - L.safe.right - L.px(12) - tw;
    ui.rect(tx + tw * 0.5f, ty, tw, L.px(48) * punch, L.px(24) * punch, ink, rgba(kGold, u8(150 * s.opacity)), L.px(1.5f));
    drawStar(ui, tx + L.px(26), ty, L.px(11) * punch, rgba(kGold, u8(255 * s.opacity)));
    ui.text(buf, tx + L.px(44), ty, ts, white, inkStroke, 0, Align::Left);
    // The flood banner (GAME_DESIGN 10).
    if (s.bannerTime > 0 && s.bannerMark > 0) {
        char b2[48];
        char num[24];
        formatThousands(s.bannerMark, num, sizeof num);
        std::snprintf(b2, sizeof b2, "FLOOD \xC3\x97%s", num);
        const float a = minv(1.0f, s.bannerTime / 0.25f) * minv(1.0f, (0.9f - s.bannerTime) / 0.12f + 0.2f);
        const float size = L.px(40) * (1.0f + 0.15f * maxv(0.0f, s.bannerTime - 0.7f) / 0.2f);
        ui.text(b2, float(L.w) * 0.5f, float(L.h) * 0.36f, size, rgba(kGold, u8(255 * a * s.opacity)), rgba(0x5A2A08, u8(255 * a * s.opacity)),
                maxv(3.0f, size * 0.09f), Align::Center);
    }
}

}  // namespace gt

#include "game/hud.h"

#include "sim/level.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

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

Rect hudTrayChip(const Layout& L, int i) {
    // Two chips a row, the third and fourth above the first two, so a full tray never reaches the tide counter.
    const float w = L.px(66), h = L.px(44);
    const float y = L.bottom() - L.px(24) - L.px(26) - h * 0.5f - float(i / 2) * (h + L.px(6));
    return {L.safe.left + L.px(12) + float(i % 2) * (w + L.px(6)), y, w, h};
}

Rect hudTideRect(const Layout& L, long long tide) {
    char buf[32];
    formatThousands(tide, buf, sizeof buf);
    const float tw = UiBatch::textWidth(buf, L.px(24)) + L.px(62), h = L.px(48);
    const float ty = L.bottom() - L.px(24) - L.px(26);
    return {float(L.w) - L.safe.right - L.px(12) - tw, ty - h * 0.5f, tw, h};
}

void drawBoonIcon(UiBatch& ui, int boon, float cx, float cy, float s, Rgba c) {
    const float k = s / 0.5f;  // the badge's units: the icon spans about -0.25..0.25
    auto box = [&](float u0, float v0, float u1, float v1) {
        ui.rect(cx + (u0 + u1) * 0.5f * k, cy - (v0 + v1) * 0.5f * k, (u1 - u0) * k, (v1 - v0) * k, 0.02f * k, c);
    };
    auto bar = [&](float ua, float va, float ub, float vb, float th) {
        const float du = ub - ua, dv = vb - va;
        ui.rectRotated(cx + (ua + ub) * 0.5f * k, cy - (va + vb) * 0.5f * k, std::sqrt(du * du + dv * dv) * k, th * k, th * 0.5f * k,
                       -std::atan2(dv, du), c);
    };
    switch (boon) {
        case 0:  // LANTERN
            box(-0.13f, -0.13f, 0.13f, 0.15f);
            box(-0.07f, 0.15f, 0.07f, 0.23f);
            box(-0.17f, -0.21f, 0.17f, -0.13f);
            break;
        case 1:  // QUICKEN
            for (float v : {-0.12f, 0.06f}) {
                bar(-0.19f, v, 0.0f, v + 0.14f, 0.075f);
                bar(0.0f, v + 0.14f, 0.19f, v, 0.075f);
            }
            break;
        case 2:  // BRIGHT
            ui.rect(cx, cy + 0.06f * k, 0.3f * k, 0.3f * k, 0.15f * k, c);
            bar(-0.08f, 0.02f, 0.0f, 0.24f, 0.09f);
            bar(0.08f, 0.02f, 0.0f, 0.24f, 0.09f);
            break;
        case 3:  // WARD
            ui.rect(cx, cy, 0.44f * k, 0.44f * k, 0.22f * k, Rgba{0, 0, 0, 0}, c, 0.1f * k);
            break;
        case 4:  // KINDLE
            box(-0.2f, -0.2f, -0.12f, 0.18f);
            box(0.12f, -0.2f, 0.2f, 0.18f);
            box(-0.2f, 0.14f, 0.2f, 0.22f);
            break;
        case 5:  // COMET
            ui.rect(cx + 0.09f * k, cy - 0.09f * k, 0.22f * k, 0.22f * k, 0.11f * k, c);
            bar(0.06f, 0.06f, -0.22f, -0.2f, 0.08f);
            break;
        default:  // HUSH
            ui.rect(cx, cy, 0.44f * k, 0.44f * k, 0.22f * k, c);
            ui.rect(cx + 0.12f * k, cy - 0.06f * k, 0.38f * k, 0.38f * k, 0.19f * k, rgba(kInk, c.a));
            break;
    }
}

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
    // The goal bar between them (the spire's health; by kind, VARIETY_DESIGN 14).
    const float bx0 = chipX + chipW + L.px(14), bx1 = pr.x - L.px(14);
    if (bx1 - bx0 > L.px(40)) {
        const float bh = L.px(14), by = top + rowH * 0.5f;
        auto bar = [&](float x0, float x1, float frac, u32 fill, u32 shine) {
            ui.rect((x0 + x1) * 0.5f, by, x1 - x0, bh, bh * 0.5f, ink, edge, L.px(1));
            const float inner = (x1 - x0 - L.px(4)) * clampv(frac, 0.0f, 1.0f);
            if (inner > L.px(2)) {
                ui.rect(x0 + L.px(2) + inner * 0.5f, by, inner, bh - L.px(4), (bh - L.px(4)) * 0.5f, rgba(fill, u8(255 * s.opacity)));
                ui.rect(x0 + L.px(2) + inner * 0.5f, by - L.px(2), maxv(0.0f, inner - L.px(4)), L.px(3), L.px(1.5f), rgba(shine, u8(200 * s.opacity)));
            }
        };
        if (s.kind == sim::kTwins) {
            const float mid = (bx0 + bx1) * 0.5f;
            bar(bx0, mid - L.px(4), s.spireFrac, kRose, 0xFFAAE8);
            bar(mid + L.px(4), bx1, s.spire2Frac, kRose, 0xFFAAE8);
        } else if (s.kind == sim::kHoldout) {
            bar(bx0, bx1, s.clockFrac, 0xFFB45A, 0xFFE2A8);  // the dawn coming
        } else if (s.kind == sim::kFestival) {
            bar(bx0, bx1, 1.0f - s.clockFrac, kGold, 0xFFE9B0);
            char g[32];
            std::snprintf(g, sizeof g, "GEODES %d/%d", s.geodesOpened, s.geodesTotal);
            ui.text(g, (bx0 + bx1) * 0.5f, by + L.px(22), L.px(15), white, inkStroke, L.px(2), Align::Center);
        } else {
            bar(bx0, bx1, s.spireFrac, kRose, 0xFFAAE8);
            if (s.kind == sim::kBoss) {  // the script's thresholds
                for (float f : {0.66f, 0.33f}) {
                    const float x = bx0 + L.px(2) + (bx1 - bx0 - L.px(4)) * f;
                    ui.rect(x, by, L.px(3), bh + L.px(6), L.px(1.5f), rgba(kWhite, u8(220 * s.opacity)));
                }
            }
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
    // The boon tray: a chip a boon held, its icon and how many; it is empty and invisible until the first boon.
    for (int i = 0; i < s.trayCount && i < 4; ++i) {
        const Rect r = hudTrayChip(L, i);
        const float p = 1.0f + 0.25f * s.trayPunch[i];
        ui.rect(r.cx(), r.cy(), r.w * p, r.h * p, r.h * 0.5f * p, ink, rgba(kGold, u8(170 * s.opacity)), L.px(1.5f));
        if (s.trayDrain[i] > 0) {  // a timed boon drains along the chip's foot
            const float bw = (r.w - L.px(16)) * s.trayDrain[i];
            ui.rect(r.x + L.px(8) + bw * 0.5f, r.y + r.h - L.px(7), bw, L.px(4), L.px(2), rgba(kGold, u8(220 * s.opacity)));
        }
        drawBoonIcon(ui, s.trayBoon[i], r.x + L.px(22), r.cy(), L.px(28) * p, rgba(kGold, u8(255 * s.opacity)));
        std::snprintf(buf, sizeof buf, "\xC3\x97%d", s.trayHeld[i]);
        ui.text(buf, r.x + L.px(38), r.cy(), L.px(18) * p, white, inkStroke, 0, Align::Left);
    }
    // Icons on their way from a geode to the tray.
    for (const HudState::Fly& f : s.fly) {
        if (f.t >= 1.0f) continue;
        int slot = 0;
        for (int i = 0; i < s.trayCount; ++i)
            if (s.trayBoon[i] == f.boon) slot = i;
        const Rect r = hudTrayChip(L, slot);
        const float e = f.t * f.t * (3.0f - 2.0f * f.t);
        const float x = f.x0 + (r.x + L.px(22) - f.x0) * e, y = f.y0 + (r.cy() - f.y0) * e - L.px(60) * 4.0f * f.t * (1.0f - f.t);
        drawBoonIcon(ui, f.boon, x, y, L.px(44 - 14 * e), rgba(kGold, u8(255 * s.opacity)));
    }
    // The banner (GAME_DESIGN 10): FLOOD xN, +1 LANTERN; one at a time.
    if (s.bannerTime > 0 && s.banner[0]) {
        const float a = minv(1.0f, s.bannerTime / 0.25f) * minv(1.0f, (0.9f - s.bannerTime) / 0.12f + 0.2f);
        const float size = L.px(40) * (1.0f + 0.15f * maxv(0.0f, s.bannerTime - 0.7f) / 0.2f);
        ui.text(s.banner, float(L.w) * 0.5f, float(L.h) * 0.36f, size, rgba(kGold, u8(255 * a * s.opacity)),
                rgba(0x5A2A08, u8(255 * a * s.opacity)), maxv(3.0f, size * 0.09f), Align::Center);
    }
}

}  // namespace gt

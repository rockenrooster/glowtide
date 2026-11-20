// The screens (GAME_DESIGN 11) and the flow test (TECH_SPEC 8.4).
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "build_stamp.h"
#include "core/log.h"
#include "core/tuning.h"
#include "game/game.h"

namespace gt {

namespace {

const char* kHints[] = {
    "HOLD AND DRAG",
    "EVERY WISP THAT PASSES IS DOUBLED",
    "PICK THE BIGGER GATE",
    "\xC3\xB7" "2 HALVES YOUR STREAM",
    "BREAK THE WALL TO REACH \xC3\x97" "4",
    "A THIN STREAM LETS THEM THROUGH",
};

Rect centered(const Layout& L, float cy, float wDp, float hDp) {
    const float w = L.px(wDp), h = L.px(hDp);
    return {float(L.w) * 0.5f - w * 0.5f, cy - h * 0.5f, w, h};
}

void pauseGlyph(UiBatch& b, const Rect& r, const Layout& L, bool pressed) {
    const Rgba c = rgba(pressed ? kInk : kWhite);
    b.rect(r.cx() - L.px(6), r.cy(), L.px(6), L.px(20), L.px(3), c);
    b.rect(r.cx() + L.px(6), r.cy(), L.px(6), L.px(20), L.px(3), c);
}

void cogGlyph(UiBatch& b, const Rect& r, const Layout& L) {
    // A settings knob: a ring with four teeth and a hub.
    const Rgba c = rgba(kWhite);
    for (int k = 0; k < 2; ++k) b.rect(r.cx(), r.cy(), k ? L.px(26) : L.px(7), k ? L.px(7) : L.px(26), L.px(2.5f), c);
    b.rect(r.cx(), r.cy(), L.px(18), L.px(18), L.px(9), c);
    b.rect(r.cx(), r.cy(), L.px(7), L.px(7), L.px(3.5f), rgba(kInk));
}

}  // namespace

void Game::drawHint() {
    if (hint_ < 0 || hintTime_ <= 0 || screen_ != Screen::Play) return;
    const float a = minv(1.0f, hintTime_ / 0.3f) * minv(1.0f, (3.2f - hintTime_) / 0.25f);
    const char* s = kHints[hint_];
    const float size = layout_.px(17);
    const float w = UiBatch::textWidth(s, size) + layout_.px(36);
    const float y = layout_.top() + layout_.px(84);
    uiBatch_.rect(float(layout_.w) * 0.5f, y, w, layout_.px(40), layout_.px(20), rgba(kInk, u8(220 * a)), rgba(kGold, u8(150 * a)),
                  layout_.px(1.5f));
    uiBatch_.text(s, float(layout_.w) * 0.5f, y, size, rgba(kWhite, u8(255 * a)), rgba(kInk, 0), 0, Align::Center);
}

void Game::drawHome(float dt) {
    (void)dt;
    const Layout& L = layout_;
    // The game's name across the upper third (cold start only).
    if (coldStart_ || titleFade_ > 0) {
        const float a = coldStart_ ? minv(1.0f, screenTime_ / 0.5f) : titleFade_;
        const float size = L.px(72);
        uiBatch_.text("GLOWTIDE", float(L.w) * 0.5f, float(L.h) * 0.30f, size, rgba(kGold, u8(255 * a)), rgba(0x5A2A08, u8(255 * a)),
                      L.px(5), Align::Center);
    }
    // Level chip and the star total.
    char buf[48];
    std::snprintf(buf, sizeof buf, "LEVEL %d", level_);
    const float top = L.top() + L.px(8);
    const float chipW = UiBatch::textWidth(buf, L.px(19)) + L.px(30);
    const float chipX = L.safe.left + L.px(12);
    uiBatch_.rect(chipX + chipW * 0.5f, top + L.px(24), chipW, L.px(40), L.px(20), rgba(kInk, 205), rgba(kWhite, 46), L.px(1));
    uiBatch_.text(buf, chipX + chipW * 0.5f, top + L.px(24), L.px(19), rgba(kWhite), rgba(kInk, 0), 0, Align::Center);
    std::snprintf(buf, sizeof buf, "%d", save_.totalStars());
    const float sw = UiBatch::textWidth(buf, L.px(19)) + L.px(52);
    uiBatch_.rect(chipX + chipW + L.px(10) + sw * 0.5f, top + L.px(24), sw, L.px(40), L.px(20), rgba(kInk, 205), rgba(kGold, 120), L.px(1));
    uiBatch_.star(chipX + chipW + L.px(30), top + L.px(24), L.px(10), rgba(kGold));
    uiBatch_.text(buf, chipX + chipW + L.px(44), top + L.px(24), L.px(19), rgba(kWhite), rgba(kInk, 0), 0, Align::Left);
    // Settings.
    const Rect cog = hudPauseRect(L);
    if (ui_.button("#SETTINGS", cog, Ui::kRound)) {
        feedback_.uiTap();
        settingsReturn_ = Screen::Home;
        setScreen(Screen::Settings);
        return;
    }
    cogGlyph(uiBatch_, cog, L);
    // The hint: HOLD AND DRAG, pulsing, with a thumb sweeping an arc.
    const float pulse = 0.75f + 0.25f * std::sin(screenTime_ * 4.0f);
    const float hy = float(L.h) * 0.66f;
    uiBatch_.text("HOLD AND DRAG", float(L.w) * 0.5f, hy, L.px(24) * (0.96f + 0.04f * pulse), rgba(kWhite, u8(255 * pulse)),
                  rgba(kInk, u8(230 * pulse)), L.px(3.5f), Align::Center);
    const float sweep = std::sin(screenTime_ * 2.2f);
    const float ax = float(L.w) * 0.5f + sweep * L.px(70);
    const float ay = hy + L.px(58) + (1.0f - sweep * sweep) * L.px(-12);
    for (int k = 1; k <= 6; ++k) {  // the arc's trail
        const float s2 = std::sin(screenTime_ * 2.2f - float(k) * 0.12f);
        uiBatch_.rect(float(L.w) * 0.5f + s2 * L.px(70), hy + L.px(58) + (1.0f - s2 * s2) * L.px(-12), L.px(8), L.px(8), L.px(4),
                      rgba(kGold, u8(140 - k * 20)));
    }
    uiBatch_.rect(ax, ay, L.px(26), L.px(26), L.px(13), rgba(kWhite, 230), rgba(kGold, 255), L.px(3));
}

void Game::drawPause() {
    const Layout& L = layout_;
    uiBatch_.text("PAUSED", float(L.w) * 0.5f, float(L.h) * 0.28f, L.px(44), rgba(kWhite), rgba(kInk), L.px(4), Align::Center);
    float y = float(L.h) * 0.42f;
    if (ui_.button("RESUME", centered(L, y, 260, 66), Ui::kPrimary)) {
        feedback_.uiTap();
        setScreen(Screen::Play);
        resumeRamp_ = 0;
        tickBudget_ = 0;
        return;
    }
    y += L.px(80);
    if (ui_.button("RESTART", centered(L, y, 240, 56))) {
        feedback_.uiTap();
        enterLevel(level_, true);
        return;
    }
    y += L.px(68);
    if (ui_.button("SETTINGS", centered(L, y, 240, 56))) {
        feedback_.uiTap();
        settingsReturn_ = Screen::Pause;
        setScreen(Screen::Settings);
        return;
    }
    y += L.px(68);
    if (ui_.button("HOME", centered(L, y, 240, 56))) {
        feedback_.uiTap();
        enterLevel(level_, true);
        return;
    }
}

void Game::drawResult(bool won) {
    const Layout& L = layout_;
    const float appear = won ? 1.4f : 1.0f;
    if (resultTime_ < appear) return;
    const float t = resultTime_ - appear;
    const float in = minv(1.0f, t / 0.22f);
    const float scale = 0.92f + 0.08f * in;
    ui_.opacity = in;
    char buf[96];
    // A line of small text over the flood sits on an ink pill so it reads against any lane colour.
    auto pill = [&](const char* s, float y, float sizeDp) {
        const float w = UiBatch::textWidth(s, L.px(sizeDp)) + L.px(28);
        uiBatch_.rect(float(L.w) * 0.5f, y, w, L.px(sizeDp + 16), L.px((sizeDp + 16) * 0.5f), rgba(kInk, u8(190 * in)));
    };
    if (won) {
        // CLEARED! drops in; stars land 250 ms apart.
        const float drop = (1.0f - in) * L.px(-40);
        uiBatch_.text("CLEARED!", float(L.w) * 0.5f, float(L.h) * 0.24f + drop, L.px(56) * scale, rgba(kGold, u8(255 * in)),
                      rgba(0x5A2A08, u8(255 * in)), L.px(5), Align::Center);
        for (int k = 0; k < 3; ++k) {
            const float landAt = 0.2f + 0.25f * float(k);
            const bool earned = k < stars_;
            const float x = float(L.w) * 0.5f + float(k - 1) * L.px(78);
            const float y = float(L.h) * 0.355f - (k == 1 ? L.px(14) : 0.0f);
            uiBatch_.star(x, y, L.px(34), rgba(kInk, u8(200 * in)), rgba(kWhite, u8(40 * in)), L.px(2));
            if (earned && t >= landAt) {
                const float s = minv(1.0f, (t - landAt) / 0.18f);
                const float pop = s < 1.0f ? 1.3f - 0.3f * s : 1.0f;
                uiBatch_.star(x, y, L.px(30) * pop, rgba(kGold), rgba(0x5A2A08), L.px(3));
                if (starsLanded_ <= k) {
                    starsLanded_ = k + 1;
                    feedback_.star(k);
                }
            }
        }
        char tide[32];
        formatThousands(world_.peakTide, tide, sizeof tide);
        std::snprintf(buf, sizeof buf, "TIME %.1f S   TIDE %s", double(world_.time()), tide);
        pill(buf, float(L.h) * 0.45f, 18);
        ui_.text(buf, float(L.w) * 0.5f, float(L.h) * 0.45f, 18, kWhite);
        if (ui_.button("NEXT", centered(L, float(L.h) * 0.74f, 280, 74), Ui::kPrimary, 1.2f)) {
            feedback_.uiTap();
            enterLevel(level_ + 1, true);
        } else if (ui_.button("REPLAY", centered(L, float(L.h) * 0.74f + L.px(84), 200, 56))) {
            feedback_.uiTap();
            enterLevel(level_, true);
        }
    } else {
        uiBatch_.text("SNUFFED OUT", float(L.w) * 0.5f, float(L.h) * 0.28f, L.px(48) * scale, rgba(kWhite, u8(255 * in)),
                      rgba(0x4A1030, u8(255 * in)), L.px(4.5f), Align::Center);
        if (advice_[0]) pill(advice_, float(L.h) * 0.36f, 16);
        ui_.text(advice_, float(L.w) * 0.5f, float(L.h) * 0.36f, 16, 0xFFD0E8);
        if (ui_.button("RETRY", centered(L, float(L.h) * 0.70f, 280, 74), Ui::kPrimary, 1.2f)) {
            feedback_.uiTap();
            enterLevel(level_, true);
        } else if (ui_.button("HOME", centered(L, float(L.h) * 0.70f + L.px(84), 200, 56))) {
            feedback_.uiTap();
            enterLevel(level_, true);
        }
    }
    ui_.opacity = 1;
}

void Game::drawSettings(float dt) {
    const Layout& L = layout_;
    const float usable = L.bottom() - L.top();
    uiBatch_.rect(float(L.w) * 0.5f, (L.top() + L.bottom()) * 0.5f, float(L.w) - L.safe.left - L.safe.right - L.px(16), usable - L.px(16),
                  L.px(24), rgba(kInk, 236), rgba(kWhite, 40), L.px(1));
    float y = L.top() + L.px(44);
    ui_.text("SETTINGS", float(L.w) * 0.5f, y, 30, kWhite);
    const float rowH = 54, gap = usable / L.dp > 760 ? 12.0f : 8.0f;
    y += L.px(52);
    bool changed = false;
    struct Sw {
        const char* name;
        bool* v;
    } sws[] = {{"SOUND", &save_.sound}, {"MUSIC", &save_.music}, {"HAPTICS", &save_.haptics}, {"AUTO-FIRE", &save_.autoFire},
               {"REDUCE EFFECTS", &save_.reduceEffects}};
    for (Sw& s : sws) {
        if (ui_.toggle(s.name, centered(L, y, 330, rowH), *s.v)) changed = true;
        y += L.px(rowH + gap);
    }
    if (changed) {
        feedback_.uiTap();
        applySettings();
        saveNow();
    }
    y += L.px(4);
    if (!confirmReset_) {
        if (ui_.holdButton("HOLD TO RESET PROGRESS", centered(L, y, 330, rowH), 1.0f, dt, holdProgress_)) confirmReset_ = true;
    } else {
        if (ui_.button("CONFIRM RESET", {float(L.w) * 0.5f - L.px(169), y - L.px(rowH * 0.5f), L.px(184), L.px(rowH)})) {
            const SaveData keep = save_;
            save_ = SaveData();
            save_.sound = keep.sound;
            save_.music = keep.music;
            save_.haptics = keep.haptics;
            save_.autoFire = keep.autoFire;
            save_.reduceEffects = keep.reduceEffects;
            saveNow();
            confirmReset_ = false;
            settingsReturn_ = Screen::Home;
            enterLevel(1, true);
            return;
        }
        if (ui_.button("CANCEL", {float(L.w) * 0.5f + L.px(25), y - L.px(rowH * 0.5f), L.px(144), L.px(rowH)})) confirmReset_ = false;
    }
    y += L.px(rowH + gap);
#if !defined(NDEBUG) || defined(GT_DEV)
    // A development-only level stepper (P3 task 12): the owner's playtest jumps through the levels.
    {
        char buf[32];
        const float w3 = L.px(102);
        const float x0 = float(L.w) * 0.5f - L.px(165);
        if (ui_.button("LEVEL -", {x0, y - L.px(rowH * 0.5f), w3, L.px(rowH)})) enterLevel(maxv(1, level_ - 1), false), setScreen(Screen::Settings);
        if (ui_.button("LEVEL +", {x0 + w3 + L.px(12), y - L.px(rowH * 0.5f), w3, L.px(rowH)})) enterLevel(level_ + 1, false), setScreen(Screen::Settings);
        if (ui_.button("+10", {x0 + 2 * (w3 + L.px(12)), y - L.px(rowH * 0.5f), w3, L.px(rowH)})) enterLevel(level_ + 10, false), setScreen(Screen::Settings);
        std::snprintf(buf, sizeof buf, "DEV  LEVEL %d", level_);
        y += L.px(rowH * 0.5f + 16);
        ui_.text(buf, float(L.w) * 0.5f, y, 14, 0xA0A0C8);
        y += L.px(gap + 10);
    }
#endif
    char ver[64];
    std::snprintf(ver, sizeof ver, "VERSION %s (%s)", GT_VERSION, GT_COMMIT);
    for (char* c = ver; *c; ++c) *c = char(std::toupper(static_cast<unsigned char>(*c)));  // the font is capitals only
    ui_.text(ver, float(L.w) * 0.5f, y + L.px(4), 14, 0xA0A0C8);
    y += L.px(24);
    ui_.text("FONT: ROBOTO, APACHE 2.0", float(L.w) * 0.5f, y, 13, 0xA0A0C8);
    y += L.px(19);
    ui_.text("SDL 3: ZLIB LICENCE", float(L.w) * 0.5f, y, 13, 0xA0A0C8);
    y += L.px(19);
    ui_.text("SOUND AND ART MADE IN CODE", float(L.w) * 0.5f, y, 13, 0xA0A0C8);
    const float backY = minv(L.bottom() - L.px(48), y + L.px(64));
    if (ui_.button("BACK", centered(L, backY, 220, 60), Ui::kPrimary)) {
        feedback_.uiTap();
        setScreen(settingsReturn_);
    }
}

void Game::drawScreens(float dt) {
    if (mode_ == Mode::Picture && pictureScreen_[0] == 0) {
        // World pictures: the HUD as in play.
    }
    const bool hud = screen_ == Screen::Play || screen_ == Screen::Pause;
    if (hud || (mode_ == Mode::Picture && screen_ == Screen::Play)) {
        HudState hs;
        hs.level = level_;
        hs.spireFrac = float(world_.spire) / float(maxv<i32>(1, world_.spireStart));
        hs.tide = world_.tide;
        hs.tidePunch = view_.tidePunch;
        hs.bannerMark = view_.bannerMark;
        hs.bannerTime = view_.bannerTime;
        hs.drawPause = false;
        hs.opacity = screen_ == Screen::Pause ? 0.5f : 1.0f;
        drawHud(uiBatch_, layout_, hs);
        if (screen_ == Screen::Play) {
            const Rect pr = hudPauseRect(layout_);
            if (ui_.button("#PAUSE", pr, Ui::kRound)) {
                feedback_.uiTap();
                setScreen(Screen::Pause);
            }
            pauseGlyph(uiBatch_, pr, layout_, false);
            drawHint();
        }
    }
    // The core-hit flash at the screen's edges (GAME_DESIGN 10), never full-screen.
    if (view_.coreFlash > 0 && !save_.reduceEffects) {
        const float a = view_.coreFlash;
        const float e = layout_.px(26);
        const Rgba c = rgba(0xFF547C, u8(150 * a));
        uiBatch_.rect(float(layout_.w) * 0.5f, e * 0.5f, float(layout_.w), e, 0, c);
        uiBatch_.rect(float(layout_.w) * 0.5f, float(layout_.h) - e * 0.5f, float(layout_.w), e, 0, c);
        uiBatch_.rect(e * 0.5f, float(layout_.h) * 0.5f, e, float(layout_.h), 0, c);
        uiBatch_.rect(float(layout_.w) - e * 0.5f, float(layout_.h) * 0.5f, e, float(layout_.h), 0, c);
    }
    switch (screen_) {
        case Screen::Home: drawHome(dt); break;
        case Screen::Pause: drawPause(); break;
        case Screen::Win: drawResult(true); break;
        case Screen::Lose: drawResult(false); break;
        case Screen::Settings: drawSettings(dt); break;
        default: break;
    }
}

// ---- The flow test: synthetic touches, found by label --------------------------------------------------------------

void Game::tap(float x, float y) {
    PointerEvent d{PtrType::Down, 77, x, y, 0, 0};
    pointer(d);
    flowTapPending_ = true;
    fingerX_ = x;
    fingerY_ = y;
}

void Game::flowStep() {
    if (flowTapPending_) {
        PointerEvent u{PtrType::Up, 77, fingerX_, fingerY_, 0, 0};
        pointer(u);
        flowTapPending_ = false;
        return;
    }
    ++flowWait_;
    auto check = [&](bool ok, const char* name) {
        ++flowChecks_;
        if (!ok) ++flowFailures_;
        GT_TAG("FLOW", "%s %s", ok ? "ok" : "FAIL", name);
    };
    auto tapLabel = [&](const char* label) {
        const UiControl* c = ui_.find(label);
        if (!c) return false;
        tap(c->r.cx(), c->r.cy());
        return true;
    };
    auto next = [&]() {
        ++flowStep_;
        flowWait_ = 0;
    };
    const float cx = float(layout_.w) * 0.5f, cy = float(layout_.h) * 0.55f;
    switch (flowStep_) {
        case 1:
            if (savePath_[0]) {
                std::snprintf(savePath_ + std::strlen(savePath_) - std::strlen("save.txt"), 32, "flow_save.txt");
                std::remove(savePath_);
                saveEnabled_ = true;
                save_ = SaveData();
                enterLevel(1, false);
            }
            check(screen_ == Screen::Home && level_ == 1, "starts on HOME at level 1");
            bot_.init(sim::BotKind::Greedy, 1);
            useBot_ = true;
            tap(cx, cy);
            next();
            break;
        case 2:
            check(screen_ == Screen::Play, "a touch on HOME starts the level");
            next();
            break;
        case 3:
            if (screen_ == Screen::Win && resultTime_ > 2.4f) {
                check(true, "level 1 won by the greedy bot");
                check(tapLabel("NEXT"), "WIN shows NEXT");
                next();
            } else if (flowWait_ > 60 * 40) {
                check(false, "level 1 won by the greedy bot");
                next();
            }
            break;
        case 4:
            if (flowWait_ > 30) {
                check(screen_ == Screen::Home && level_ == 2, "NEXT goes to HOME of level 2");
                tap(cx, cy);
                next();
            }
            break;
        case 5:
            if (flowWait_ > 60) {
                check(tapLabel("#PAUSE"), "the HUD has a pause button");
                next();
            }
            break;
        case 6:
            if (flowWait_ > 2) {
                check(screen_ == Screen::Pause, "pause opens PAUSE");
                check(tapLabel("RESTART"), "PAUSE shows RESTART");
                next();
            }
            break;
        case 7:
            if (flowWait_ > 30) {
                check(screen_ == Screen::Home && level_ == 2, "RESTART goes to HOME of the same level");
                bot_.init(sim::BotKind::Idle, 1);
                tap(cx, cy);
                next();
            }
            break;
        case 8:
            if (screen_ == Screen::Lose && resultTime_ > 1.6f) {
                check(true, "the idle bot loses");
                check(tapLabel("RETRY"), "LOSE shows RETRY");
                next();
            } else if (flowWait_ > 60 * 60) {
                check(false, "the idle bot loses");
                next();
            }
            break;
        case 9:
            if (flowWait_ > 30) {
                check(screen_ == Screen::Home && level_ == 2, "RETRY goes to HOME of the same level");
                check(tapLabel("#SETTINGS"), "HOME has a settings button");
                next();
            }
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: {
            static const char* names[5] = {"SOUND", "MUSIC", "HAPTICS", "AUTO-FIRE", "REDUCE EFFECTS"};
            bool* vals[5] = {&save_.sound, &save_.music, &save_.haptics, &save_.autoFire, &save_.reduceEffects};
            const int k = flowStep_ - 10;
            static bool before = false;
            // MUSIC stays off for a second: the bed must be silent by then (P4, end to end in P6).
            const int offFor = k == 1 ? 64 : 4;
            if (flowWait_ == 2) {
                check(screen_ == Screen::Settings, "SETTINGS is open");
                before = *vals[k];
                check(tapLabel(names[k]), names[k]);
            } else if (flowWait_ == 2 + offFor) {
                check(*vals[k] != before, "a switch changes its setting");
                if (k == 0 && !save_.sound) {
                    const int pushed = mixer_.pushed();
                    feedback_.uiTap();
                    feedback_.star(0);
                    check(mixer_.pushed() == pushed, "SOUND off: no command reaches the mixer");
                }
                if (k == 1 && !save_.music) {
                    if (mixer_.mixCalls() > 0) check(mixer_.musicLevel() < 0.01f, "MUSIC off: the bed is silent within a second");
                    else GT_TAG("FLOW", "skip MUSIC level check: no audio device");
                }
                tapLabel(names[k]);
            } else if (flowWait_ == 6 + offFor) {
                check(*vals[k] == before, "and changes it back");
                if (k == 0 && save_.sound) {
                    const int pushed = mixer_.pushed();
                    feedback_.uiTap();
                    check(mixer_.pushed() > pushed, "SOUND on: the tap reaches the mixer");
                }
                next();
            }
            break;
        }
        case 15:
            if (flowWait_ > 2) {
                check(tapLabel("BACK"), "SETTINGS shows BACK");
                next();
            }
            break;
        case 16:
            if (flowWait_ > 2) {
                check(screen_ == Screen::Home, "BACK returns to HOME");
                bot_.init(sim::BotKind::Greedy, 2);
                tap(cx, cy);
                next();
            }
            break;
        case 17:
            if (flowWait_ > 40) {
                check(tapLabel("#PAUSE"), "pause again");
                next();
            }
            break;
        case 18:
            if (flowWait_ > 2) {
                check(tapLabel("HOME"), "PAUSE shows HOME");
                next();
            }
            break;
        case 19:
            if (flowWait_ > 30) {
                check(screen_ == Screen::Home, "PAUSE's HOME goes to HOME");
                // A second start reads the save written by this run.
                SaveData again;
                const bool loaded = loadSave(savePath_, again);
                check(loaded && again.level == 2 && again.stars[0] >= 1, "the save keeps the level and the stars");
                // Input (P2 exit check), played by hand from here: start the level with a tap, no bot.
                useBot_ = false;
                tap(cx, cy);
                next();
            }
            break;
        case 20:
            if (flowWait_ > 20) {
                // A press with a stored shot: the wisp leaves in the tick that reads the press.
                flowProduced_ = world_.produced;
                PointerEvent d{PtrType::Down, 88, cx, cy, 0, 0};
                pointer(d);
                next();
            }
            break;
        case 21: {
            check(screen_ == Screen::Play && world_.produced > flowProduced_, "a press fires in the next tick");
            // A drag: the target moves in the frame its motion event is read, the launcher in that frame's tick.
            const float before = target_;
            flowLauncher_ = world_.launcherX;
            PointerEvent m{PtrType::Move, 88, cx + 120, cy, 120, 0};
            pointer(m);
            check(target_ > before, "a drag moves the target at once");
            next();
            break;
        }
        case 22: {
            check(world_.launcherX > flowLauncher_, "the launcher moves in the same frame");
            PointerEvent u{PtrType::Up, 88, cx + 120, cy, 0, 0};
            pointer(u);
            GT_TAG("FLOW", "done: %d checks, %d failed", flowChecks_, flowFailures_);
            std::remove(savePath_);
            quit_ = true;
            exitCode_ = flowFailures_ ? 6 : 0;
            next();
            break;
        }
        default:
            break;
    }
}

}  // namespace gt

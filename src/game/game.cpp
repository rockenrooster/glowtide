#include "game/game.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "core/clock.h"
#include "core/log.h"
#include "core/rng.h"
#include "core/tuning.h"
#include "gfx/gl.h"
#include "gfx/shader.h"
#include "platform/platform.h"
#include "sim/bake.h"

namespace gt {

bool Game::init(const Args& args, const GameEnv& env) {
    args_ = args;
    env_ = env;
    layout_ = {env.frameW, env.frameH, env.dpScale, env.safe};
    view_.init({env.frameW, env.frameH, env.dpScale, env.safe});

    // The save lives in the preferences path; pictures, tests and benchmarks never touch it.
    saveEnabled_ = !args.fresh && !args.picture[0] && std::strncmp(args.scenario, "bench_", 6) && !args.bot[0];
    if (char* pref = SDL_GetPrefPath("glowtide", "glowtide")) {
        std::snprintf(savePath_, sizeof savePath_, "%ssave.txt", pref);
        SDL_free(pref);
    }
    if (saveEnabled_ && savePath_[0]) {
        if (!loadSave(savePath_, save_)) {
            GT_LOG("save: none or damaged at %s; defaults", savePath_);
        } else {
            GT_LOG("save: level %d, %d stars, sound %d music %d haptics %d auto-fire %d reduce-effects %d", save_.level,
                   save_.totalStars(), save_.sound, save_.music, save_.haptics, save_.autoFire, save_.reduceEffects);
        }
    }
    if (args.reduceEffects) save_.reduceEffects = true;
    {
        const double t0 = nowSeconds();
        mixer_.init(48000, 0);  // synthesizes every sound (P4: measured on the Moto)
        GT_LOG("audio: sound bank built in %.0f ms", (nowSeconds() - t0) * 1000.0);
    }
    feedback_.init(&mixer_);
    mixer_.setTone(args.tone);
    applySettings();
    if (save_.refresh60 && env.refreshHz > 61.0f && args.fps <= 0.0f) {
        GT_LOG("refresh: the save asks for 60 Hz (an earlier level was late at %.0f Hz)", double(env.refreshHz));
        platform::requestRefreshRate(60.0f);
        env_.refreshHz = 60.0f;
    }

    if (!std::strcmp(args.scenario, "sim_hash")) {
        // The determinism hash on this device (P2): levels 1-5, four geode levels (VARIETY_PLAN V1), one level of each kind
        // (V3: a boss, a festival, a holdout, a colossus, twins) and of each recipe, a set piece and a mirrored replay (V4),
        // the greedy bot, 60 s each, the same runs as glowtide_headless --sim determinism on the PC (seed 1 unless --seed;
        // scripts\simhash-android.ps1 compares them).
        mode_ = Mode::Debug;
        const u64 seed = args.seed >= 0 ? u64(args.seed) : 1u;
        for (int level : sim::kHashLevels) {
            world_.init(sim::levelFor(level), seed);
            bot_.init(sim::BotKind::Greedy, seed);
            for (int t = 0; t < 60 * 60; ++t) world_.tick(bot_.think(world_));
            GT_TAG("GTSIM", "determinism level %d: hash %016llx", level, (unsigned long long)world_.hash());
        }
        quit_ = true;
        return true;
    }
    if (!std::strcmp(args.scenario, "touch_test") || !std::strcmp(args.scenario, "bench_idle")) {
        mode_ = Mode::Debug;
        Rng rng(args.seed >= 0 ? u64(args.seed) : 7u);
        cloudN_ = kCloud;
        for (int i = 0; i < cloudN_; ++i) {
            cx_[i] = rng.range(0, float(env.frameW));
            cy_[i] = rng.range(0, float(env.frameH));
            cvx_[i] = rng.range(-120, 120);
            cvy_[i] = rng.range(-240, 240);
            cdx_[i] = cdy_[i] = 0;
            const u32 r = u32(rng.rangei(150, 255)), g = u32(rng.rangei(120, 220)), b = u32(rng.rangei(40, 120));
            ccol_[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
        markerX_ = float(env.frameW) * 0.5f;
        return true;
    }
    if (args.picture[0]) {
        mode_ = Mode::Picture;
        if (!setupPicture(args.picture)) {
            GT_ERR("unknown picture %s", args.picture);
            return false;
        }
        return true;
    }
    if (!std::strcmp(args.scenario, "bench_flood")) {
        mode_ = Mode::BenchFlood;
        setupBenchFlood();
        return true;
    }
    mode_ = Mode::Play;
    level_ = args.level > 0 ? args.level : maxv(1, save_.level);
    enterLevel(level_, false);
    if (args.bot[0]) {
        sim::BotKind k;
        if (sim::botFromName(args.bot, k)) {
            bot_.init(k, args.seed >= 0 ? u64(args.seed) : 1u);
            useBot_ = true;
        }
    } else if (!std::strcmp(args.scenario, "bench_play")) {
        bot_.init(sim::BotKind::Greedy, 1);
        useBot_ = true;
    }
    if (useBot_ || !std::strcmp(args.scenario, "bench_play")) {
        coldStart_ = false;
        startPlaying();
    }
    if (args.flowTest) {
        coldStart_ = false;
        flowStep_ = 1;
    }
    return true;
}

void Game::applySettings() {
    mixer_.setEnabled(save_.sound, save_.music);
    feedback_.setEnabled(save_.sound, save_.haptics);
    view_.reduceEffects = save_.reduceEffects;
}

void Game::saveNow() {
    if (!saveEnabled_ || !savePath_[0]) return;
    if (!writeSave(savePath_, save_)) GT_WARN("save: could not write %s", savePath_);
}

void Game::enterLevel(int index, bool fade) {
    if (fade && graphicsReady_) {
        fadeOut_ = 0.15f;  // 150 ms out, then in (GAME_DESIGN 10)
        pendingLevel_ = index;
        return;
    }
    level_ = maxv(1, index);
    // Level load time (P5: at most 50 ms on the Moto): generate, then the meshes.
    const double t0 = nowSeconds();
    sim::LevelDef def = sim::levelFor(level_);
    if (args_.world > 0) def.world = args_.world;  // --world: the colours, the sky and the props of another world
    world_.init(def, args_.seed >= 0 ? u64(args_.seed) : 0u);
    view_.setLevel(def, args_.world);
    mixer_.setWorld(def.world, def.kind == sim::kBoss);  // the world's music (VARIETY_DESIGN 15), a boss's pulse
    const double t1 = nowSeconds();
    if (graphicsReady_) renderer_.setLevel(def, *view_.wp, *view_.gp);
    GT_LOG("level %d loaded in %.1f ms (generate %.1f ms, meshes %.1f ms)", level_, (nowSeconds() - t0) * 1000.0, (t1 - t0) * 1000.0,
           (nowSeconds() - t1) * 1000.0);
    target_ = 0;
    worldTicks_ = false;
    fingerCount_ = 0;
    levelTime_ = 0;
    levelFrames_ = levelLate_ = 0;
    firingTicks_ = divPasses_ = mulPasses_ = geodeAimTicks_ = 0;
    bannerQueued_ = 0;
    bannerNow_[0] = 0;
    bannerLeft_ = 0;
    for (float& p : trayPunch_) p = 0;
    for (HudState::Fly& f : fly_) f.t = 1;
    popWaveZ_ = 0;
    setScreen(Screen::Home);
    fadeIn_ = fade ? 0.0f : 1.0f;
    // Hints tied to a level (GAME_DESIGN 11).
    if (level_ == 2) showHint(2);
    else if (level_ == 4) showHint(3);
    else if (level_ == 5) showHint(4);
    else if (level_ == 16) showHint(9);
    else if (level_ == 15) showHint(10);
    else if (level_ == 22) showHint(11);
    else if (level_ == 32) showHint(12);
    else if (level_ == 42) showHint(13);
}

const char* screenName(Screen s) {
    static const char* const names[] = {"HOME", "PLAY", "PAUSE", "WIN", "LOSE", "SETTINGS"};
    return names[int(s)];
}

void Game::setScreen(Screen s) {
    if (s != screen_ && mode_ == Mode::Play) GT_TAG("GTLIFE", "screen %s -> %s", screenName(screen_), screenName(s));
    screen_ = s;
    screenTime_ = 0;
    pausePressed_ = false;
    confirmReset_ = false;
}

void Game::startPlaying() {
    if (screen_ != Screen::Home) return;
    setScreen(Screen::Play);
    worldTicks_ = true;
    titleFade_ = coldStart_ ? 1.0f : 0.0f;
    coldStart_ = false;
    resumeRamp_ = 1;
    if (level_ == 1) showHint(0);
    if (level_ > 1 && (level_ - 1) % 10 == 0) {  // a world's first level: its number and name (VARIETY_DESIGN 15)
        char wb[48];
        std::snprintf(wb, sizeof wb, "WORLD %d  %s", (level_ - 1) / 10 + 1, worldPalette(world_.def.world).name);
        pushBanner(wb);
    }
    if (const char* b = goalBanner(world_.def)) pushBanner(b);  // the goal, for the level's first second (VARIETY_DESIGN 14)
    // A set piece's name, or the recipe the level introduces, then its idea as the hint line (VARIETY_DESIGN 10.1, 10.3).
    char name[48];
    if (levelName(world_.def.index, name, sizeof name)) {
        if (!goalBanner(world_.def)) pushBanner(name);
        if (const char* idea = sim::levelIdea(world_.def.index)) showLine(idea);
    }
}

// A level's name in the font's capitals (a set piece's, or the recipe it introduces); false for any other level.
bool levelName(int index, char* out, size_t n) {
    const char* t = sim::levelTitle(index);
    if (!t || n == 0) return false;
    size_t i = 0;
    for (; t[i] && i + 1 < n; ++i) out[i] = t[i] >= 'a' && t[i] <= 'z' ? char(t[i] - 'a' + 'A') : t[i];
    out[i] = 0;
    return true;
}

// The goal's banner (VARIETY_DESIGN 6, 8): none for a siege.
const char* goalBanner(const sim::LevelDef& d) {
    switch (d.kind) {
        case sim::kHoldout: return "HOLD UNTIL DAWN";
        case sim::kColossus: return "STOP THE COLOSSUS";
        case sim::kTwins: return "TWO SPIRES";
        case sim::kFestival: return "LANTERN FESTIVAL";
        case sim::kBoss: return sim::bossName(d.bossType);
        default: return nullptr;
    }
}

void Game::finishLevel(bool won) {
    resultTime_ = 0;
    starsLanded_ = 0;
    // The one-way 60 Hz fallback (TECH_SPEC 5.10), decided on this level's frames; the next level runs at 60 Hz.
    if (mode_ == Mode::Play && !save_.refresh60 && shouldFallBackTo60(env_.refreshHz, levelFrames_, levelLate_)) {
        GT_TAG("GTLIFE", "refresh: %d of level %d's %d frames were late at %.0f Hz: 60 Hz from now on", levelLate_, level_, levelFrames_,
               double(env_.refreshHz));
        save_.refresh60 = true;
        platform::requestRefreshRate(60.0f);
        env_.refreshHz = 60.0f;
    }
    if (won) {
        const float t = world_.time(), T = world_.def.targetT;
        stars_ = t <= T ? 3 : t <= 1.3f * T ? 2 : 1;  // GAME_DESIGN 5.11: stars are for speed
        if (world_.def.kind == sim::kHoldout) stars_ = world_.core >= 8 ? 3 : world_.core >= 5 ? 2 : 1;  // by the core left
        if (world_.def.kind == sim::kFestival)  // by geodes opened (VARIETY_DESIGN 6)
            stars_ = world_.geodesOpened >= world_.def.goal2 ? 3 : float(world_.geodesOpened) >= 0.6f * float(world_.def.geodeCount) ? 2 : 1;
        const int idx = level_ - 1;  // levels past the array keep no stars (they replay earlier ones)
        if (idx >= 0 && idx < int(sizeof save_.stars) && save_.stars[idx] < stars_) save_.stars[idx] = u8(stars_);
        save_.level = maxv(save_.level, level_ + 1);
        setScreen(Screen::Win);
    } else {
        stars_ = 0;
        // One line of advice chosen by what happened.
        const float fired = float(firingTicks_) / maxv(1.0f, float(world_.tickCount));
        if (fired < 0.5f) std::snprintf(advice_, sizeof advice_, "HOLD TO POUR A STEADY STREAM");
        else if (divPasses_ > mulPasses_ / 3) std::snprintf(advice_, sizeof advice_, "STEER CLEAR OF THE \xC3\xB7 GATES");
        else if (world_.geodesShown > 0 && world_.geodesOpened == 0) std::snprintf(advice_, sizeof advice_, "THE GEODES WOULD HAVE HELPED");
        else if (geodeAimTicks_ * 3 > world_.tickCount) std::snprintf(advice_, sizeof advice_, "DO NOT CHASE EVERY GEODE");
        else std::snprintf(advice_, sizeof advice_, "KEEP THE STREAM ON THE BIGGEST GATES");
        setScreen(Screen::Lose);
    }
    saveNow();
}

void Game::pushBanner(const char* text) {
    if (bannerLeft_ <= 0 && bannerQueued_ == 0) {
        std::snprintf(bannerNow_, sizeof bannerNow_, "%s", text);
        bannerLeft_ = 0.9f;
    } else if (bannerQueued_ < 4) {
        std::snprintf(bannerQueue_[bannerQueued_++], sizeof bannerQueue_[0], "%s", text);
    }
}

// A hint line that is not one of kHints: a level's idea, shown each time the level starts (VARIETY_DESIGN 10.1).
void Game::showLine(const char* text) {
    size_t i = 0;
    for (; text[i] && i + 1 < sizeof lineText_; ++i) lineText_[i] = text[i] >= 'a' && text[i] <= 'z' ? char(text[i] - 'a' + 'A') : text[i];
    lineText_[i] = 0;
    hint_ = kLineHint;
    hintTime_ = 3.2f;
}

void Game::showHint(int id) {
    if (id < 0 || id >= 32 || (save_.hints & (1u << id))) return;
    save_.hints |= 1u << id;
    hint_ = id;
    hintTime_ = 3.2f;
    saveNow();
}

bool Game::initGraphics() {
    if (!debug_.init()) return false;
    if (!renderer_.init(env_.frameW, env_.frameH)) return false;
    graphicsReady_ = true;
    if (mode_ != Mode::Debug) renderer_.setLevel(world_.def, *view_.wp, *view_.gp);
    checkGl("Game::initGraphics");
    return true;
}

void Game::destroyGraphics() {
    renderer_.destroy();
    debug_.destroy();
    graphicsReady_ = false;
}

void Game::resize(const GameEnv& env) {
    env_ = env;
    if (save_.refresh60 && args_.fps <= 0.0f) env_.refreshHz = minv(env_.refreshHz, 60.0f);  // the fallback holds
    layout_ = {env.frameW, env.frameH, env.dpScale, env.safe};
    view_.resize({env.frameW, env.frameH, env.dpScale, env.safe});
}

void Game::pointer(const PointerEvent& e) {
    fingerX_ = e.x;
    fingerY_ = e.y;
    auto removeFrom = [](i64* list, int& n, i64 id) {
        for (int i = 0; i < n; ++i)
            if (list[i] == id) {
                for (int j = i; j + 1 < n; ++j) list[j] = list[j + 1];
                --n;
                return true;
            }
        return false;
    };
    if (mode_ == Mode::Debug) {
        if (e.type == PtrType::Down) fingers_[0] = e.id, fingerCount_ = 1;
        if (e.type == PtrType::Up) fingerCount_ = 0;
        if (e.type == PtrType::Move && fingerCount_) markerX_ = clampv(markerX_ + e.dx * tune.dragGain, 0.0f, float(env_.frameW));
        return;
    }
    if (e.type == PtrType::Down) {
        // UI first: a touch that begins on a control belongs to the UI until it ends.
        if (ui_.pointerDown(e.id, e.x, e.y)) {
            if (uiFingerCount_ < kFingers) uiFingers_[uiFingerCount_++] = e.id;
            return;
        }
        if (screen_ == Screen::Home) startPlaying();
        if (screen_ == Screen::Play && fingerCount_ < kFingers) fingers_[fingerCount_++] = e.id;
        return;
    }
    if (e.type == PtrType::Move) {
        ui_.pointerMove(e.id, e.x, e.y);
        // Relative drag (GAME_DESIGN 4): only the owner (the oldest finger still down) moves the launcher.
        if (fingerCount_ && fingers_[0] == e.id && screen_ == Screen::Play) {
            const float ppm = view_.cam.pixelsPerMetre(V3(world_.launcherX, 0, 0.2f));
            target_ = clampv(target_ + e.dx / maxv(1.0f, ppm) * tune.dragGain, -sim::kLauncherRange, sim::kLauncherRange);
        }
        return;
    }
    if (removeFrom(uiFingers_, uiFingerCount_, e.id)) {
        ui_.pointerUp(e.id, e.x, e.y);
        return;
    }
    removeFrom(fingers_, fingerCount_, e.id);
}

void Game::key(Key k, bool down) {
    switch (k) {
        case Key::Back:
            if (!down) break;
            if (mode_ != Mode::Play) {
                quit_ = true;
                break;
            }
            switch (screen_) {
                case Screen::Play: setScreen(Screen::Pause); break;
                case Screen::Pause:
                    setScreen(Screen::Play);
                    resumeRamp_ = 0;
                    break;
                case Screen::Settings: setScreen(settingsReturn_); break;
                case Screen::Home: quit_ = true; break;
                default: enterLevel(level_ + (screen_ == Screen::Win ? 1 : 0), true); break;
            }
            break;
        case Key::Pause:
            if (down && screen_ == Screen::Play) setScreen(Screen::Pause);
            break;
        case Key::Left: keyLeft_ = down; break;
        case Key::Right: keyRight_ = down; break;
        case Key::Fire:
            keyFire_ = down;
            if (down && screen_ == Screen::Home) startPlaying();
            break;
        case Key::Debug:
            if (down) debugReadout_ = !debugReadout_;
            break;
    }
}

void Game::background() {
    GT_TAG("GTLIFE", "game: paused");
    if (mode_ == Mode::Play && screen_ == Screen::Play) setScreen(Screen::Pause);
    fingerCount_ = uiFingerCount_ = 0;
    saveNow();
}

void Game::foreground() { GT_TAG("GTLIFE", "game: resumed on %s", screenName(screen_)); }

void Game::popGloomsAbove(float z) {
    sim::Units& g = world_.g;
    int o = 0;
    for (int i = 0; i < g.n; ++i) {
        if (g.z[i] > z) {
            view_.spawn(V3(g.x[i], 0.4f, g.z[i]), V3(0, 1.2f, 0), view_.gp->gloomLight, 0.7f, 0.45f, 0.45f, 1);
            view_.spawn(V3(g.x[i], 0.5f, g.z[i]), V3(0, 2.5f, -0.5f), view_.gp->gloomEye, 0.9f, 0.2f, 0.4f, 0);
            continue;
        }
        if (o != i) {
            g.x[o] = g.x[i];
            g.z[o] = g.z[i];
            g.xp[o] = g.xp[i];
            g.zp[o] = g.zp[i];
            g.vx[o] = g.vx[i];
            g.power[o] = g.power[i];
            g.bits[o] = g.bits[i];
            g.born[o] = g.born[i];
            g.flags[o] = g.flags[i];
            g.seed[o] = g.seed[i];
            g.clashed[o] = g.clashed[i];
        }
        ++o;
    }
    g.n = o;
}

void Game::tick() {
    const double t0 = nowSeconds();
    if (mode_ == Mode::Debug) {
        const float dt = 1.0f / 60.0f;
        const float W = float(env_.frameW), H = float(env_.frameH);
        for (int i = 0; i < cloudN_; ++i) {
            const float ox = cx_[i], oy = cy_[i];
            cx_[i] += cvx_[i] * dt;
            cy_[i] += cvy_[i] * dt;
            if (cx_[i] < 0) cx_[i] += W;
            if (cx_[i] >= W) cx_[i] -= W;
            if (cy_[i] < 0) cy_[i] += H;
            if (cy_[i] >= H) cy_[i] -= H;
            cdx_[i] = cx_[i] - ox;
            cdy_[i] = cy_[i] - oy;
            if (std::fabs(cdx_[i]) > W * 0.5f || std::fabs(cdy_[i]) > H * 0.5f) cdx_[i] = cdy_[i] = 0;
        }
        lastTick_ = float(nowSeconds() - t0);
        return;
    }
    if (mode_ == Mode::Picture) tickPictureScript();
    if (args_.flowTest) flowStep();
    const bool ticking = mode_ == Mode::BenchFlood || (worldTicks_ && screen_ == Screen::Play);
    if (!ticking) {
        lastTick_ = 0;
        return;
    }
    // After RESUME time ramps back up over 0.5 s: ticks are skipped in proportion.
    if (resumeRamp_ < 1.0f) {
        resumeRamp_ = minv(1.0f, resumeRamp_ + 1.0f / 30.0f);
        tickBudget_ += resumeRamp_;
        if (tickBudget_ < 1.0f) {
            lastTick_ = 0;
            return;
        }
        tickBudget_ -= 1.0f;
    }
    sim::Input in;
    if (mode_ == Mode::BenchFlood) {
        in.launcherTargetX = 3.6f * sim::easedWave(world_.time() / 5.0f);
        in.firing = true;
    } else if (panScript_) {
        in.launcherTargetX = (world_.tickCount / 30) % 2 ? sim::kLauncherRange : -sim::kLauncherRange;
        in.firing = true;
    } else if (useBot_) {
        in = bot_.think(world_);
    } else {
        if (keyLeft_) target_ -= 9.0f / 60.0f;
        if (keyRight_) target_ += 9.0f / 60.0f;
        target_ = clampv(target_, -sim::kLauncherRange, sim::kLauncherRange);
        in.launcherTargetX = target_;
        in.firing = fingerCount_ > 0 || keyFire_ || save_.autoFire;
    }
    if (in.firing) ++firingTicks_;
    const StripPre stripPre = stripBefore();
    world_.tick(in);
    if (stripPre.active) stripCheck(stripPre);
    view_.onTick(world_);
    feedback_.onTick(world_, float(view_.time));
    for (int e = 0; e < world_.eventCount; ++e) {
        const sim::Event& ev = world_.events[e];
        if (ev.type == sim::kEvGateMul) {
            mulPasses_ += ev.b;
            showHint(1);
        } else if (ev.type == sim::kEvGateDiv) {
            divPasses_ += ev.b;
        } else if (ev.type == sim::kEvTideMark) {
            char num[24], b[40];
            formatThousands(ev.a, num, sizeof num);
            std::snprintf(b, sizeof b, "FLOOD \xC3\x97%s", num);
            pushBanner(b);
        } else if (ev.type == sim::kEvTwist) {
            pushBanner(sim::twistName(ev.b));
        } else if (ev.type == sim::kEvGeodeAppear) {
            showHint(world_.def.geodes[ev.a].shield > 0 && (save_.hints & (1u << 6)) ? 7 : 6);
        } else if (ev.type == sim::kEvGeodeLost && ev.b == 0) {
            showHint(8);
        } else if (ev.type == sim::kEvGeodeOpen) {
            char b[40];
            std::snprintf(b, sizeof b, "+1 %s", sim::boonName(ev.b));
            pushBanner(b);
            float sx, sy;
            if (view_.cam.project(V3(ev.x, 1.4f, ev.z), sx, sy) > 0) {
                for (HudState::Fly& f : fly_) {
                    if (f.t < 1.0f) continue;
                    f.boon = u8(ev.b);
                    f.x0 = sx;
                    f.y0 = sy;
                    f.t = 0;
                    break;
                }
            }
        } else if (ev.type == sim::kEvBoon && ev.a < sim::kBoonCount) {
            trayPunch_[ev.a] = 1.0f;
        }
    }
    for (int g = 0; g < world_.def.geodeCount; ++g) {
        if (world_.geodeState[g] == sim::kGeodeAlive && std::fabs(world_.geodeX[g] - world_.launcherX) < 1.0f) {
            ++geodeAimTicks_;
            break;
        }
    }
    // The first time the enemy's line comes down past the first gate row (GAME_DESIGN 11).
    if (world_.def.rowCount > 0 && world_.lowestGloomZ < world_.def.rowZ[0]) showHint(5);
    if (world_.result != sim::Result::None && mode_ == Mode::Play) finishLevel(world_.result == sim::Result::Win);
    lastTick_ = float(nowSeconds() - t0);
}

void Game::frameShown(float intervalSeconds) {
    if (screen_ != Screen::Play || mode_ != Mode::Play) return;
    ++levelFrames_;
    if (lateFrame(intervalSeconds, env_.refreshHz)) ++levelLate_;
}

void Game::update(float dt, float alpha) {
    alpha_ = alpha;
    frameMs_ = frameMs_ * 0.9f + dt * 1000.0f * 0.1f;
    frameHist_[frameHistHead_] = dt * 1000.0f;
    frameHistHead_ = (frameHistHead_ + 1) % 120;
    if (mode_ == Mode::Debug) return;
    screenTime_ += dt;
    if (screen_ == Screen::Play) levelTime_ += dt;
    if (hintTime_ > 0) hintTime_ -= dt;
    bannerLeft_ -= dt;
    if (bannerLeft_ <= 0 && bannerQueued_ > 0) {
        std::snprintf(bannerNow_, sizeof bannerNow_, "%s", bannerQueue_[0]);
        for (int i = 1; i < bannerQueued_; ++i) std::snprintf(bannerQueue_[i - 1], sizeof bannerQueue_[0], "%s", bannerQueue_[i]);
        --bannerQueued_;
        bannerLeft_ = 0.9f;
    }
    for (HudState::Fly& f : fly_) f.t = minv(1.0f, f.t + dt / 0.4f);
    for (float& p : trayPunch_) p = maxv(0.0f, p - dt / 0.25f);
    if (titleFade_ > 0 && screen_ != Screen::Home) titleFade_ = maxv(0.0f, titleFade_ - dt / 0.4f);
    // Fades between levels: 150 ms out, enter, 150 ms in.
    if (fadeOut_ > 0) {
        fadeOut_ -= dt;
        view_.fade = clampv(fadeOut_ / 0.15f, 0.0f, 1.0f);
        if (fadeOut_ <= 0) {
            fadeOut_ = 0;
            enterLevel(pendingLevel_, false);
            fadeIn_ = 0;
        }
    } else if (fadeIn_ < 1) {
        fadeIn_ = minv(1.0f, fadeIn_ + dt / 0.15f);
        view_.fade = fadeIn_;
    } else {
        view_.fade = 1;
    }
    if (screen_ == Screen::Pause) view_.fade = minv(view_.fade, 0.4f);
    // The win and the lose (GAME_DESIGN 10).
    float viewDt = dt;
    if (screen_ == Screen::Win) {
        resultTime_ += dt;
        if (!save_.reduceEffects && resultTime_ < 0.8f) viewDt = dt * 0.35f;  // slow motion
        view_.cheer = minv(1.0f, resultTime_ / 0.5f);
        view_.brighten = minv(1.0f, resultTime_ / 2.0f) * (save_.reduceEffects ? 0.5f : 1.0f);
        if (resultTime_ < 0.65f) {
            popWaveZ_ = world_.def.len * (1.0f - resultTime_ / 0.6f);
            popGloomsAbove(popWaveZ_);
        } else if (world_.g.n) {
            popGloomsAbove(-10.0f);
        }
        if (resultTime_ < 2.0f && int(resultTime_ * 60.0f) % 3 == 0) {  // motes rise like confetti
            const float x = float((int(resultTime_ * 977.0f) % 100) - 50) / 10.0f;
            view_.spawn(V3(x, 0.3f, 4.0f + float(int(resultTime_ * 613.0f) % 28)), V3(0, 1.6f, 0),
                        int(resultTime_ * 31.0f) % 2 ? view_.gp->wispLight : view_.wp->rail, 0.9f, 0.18f, 1.8f, 3);
        }
    } else if (screen_ == Screen::Lose) {
        resultTime_ += dt;
        view_.desaturate = minv(0.6f, resultTime_ / 0.6f * 0.6f);
        view_.gloomCheer = minv(1.0f, resultTime_ / 0.5f);  // the glooms bounce
    }
    // bench_play keeps playing: 3 s after the result the same level starts again, so the measured window is play,
    // win sequences and level loads rather than a still result screen. With --alternate it plays --level N and N + 1 in
    // turn: across a world's first level when N ends a world (the music's pad is built and swapped each time).
    if ((screen_ == Screen::Win || screen_ == Screen::Lose) && resultTime_ > 3.0f && !std::strcmp(args_.scenario, "bench_play")) {
        enterLevel(args_.alternate && args_.level > 0 && level_ == args_.level ? args_.level + 1 : args_.level > 0 ? args_.level : level_, false);
        coldStart_ = false;
        bot_.init(sim::BotKind::Greedy, 1);
        startPlaying();
    } else if (screen_ == Screen::Home || screen_ == Screen::Play) {
        view_.cheer = view_.brighten = view_.desaturate = view_.gloomCheer = 0;
        // Holdout: the night brightens toward the dawn the level is heading for (VARIETY_DESIGN 6).
        if (world_.def.kind == sim::kHoldout && screen_ == Screen::Play)
            view_.brighten = 0.4f * minv(1.0f, world_.time() / maxv(1.0f, world_.def.targetT)) * (save_.reduceEffects ? 0.5f : 1.0f);
    }
    view_.update(viewDt * view_.timeScale, world_);
    // Haptics the feedback asked for.
    int kinds[16];
    const int n = feedback_.takeHaptics(kinds, 16);
    for (int i = 0; i < n; ++i) {
        platform::haptic(kinds[i]);
        // Count them on the wall clock: the most sent within any one second (P3 exit check: at most 12).
        const double now = nowSeconds();
        hapticRing_[hapticsSent_ % kHapticRing] = now;
        ++hapticsSent_;
        int inLastSecond = 0;
        for (double t : hapticRing_)
            if (t > 0 && now - t < 1.0) ++inLastSecond;
        hapticPeak_ = maxv(hapticPeak_, inLastSecond);
    }
}

Game::StripPre Game::stripBefore() const {
    StripPre p;
    if (mode_ != Mode::Picture || args_.strip < 2) return p;
    if (std::strcmp(pictureScreen_, "gate_strip") && std::strcmp(pictureScreen_, "clash_strip") &&
        std::strcmp(pictureScreen_, "core_hit_strip") && std::strcmp(pictureScreen_, "geode_strip") && std::strcmp(pictureScreen_, "ward_strip"))
        return p;
    // Only the captured frames (and the ticks between them).
    const int first = args_.frames - (args_.strip - 1) * args_.stripStep;
    if (pictureTick_ < first || pictureTick_ > args_.frames) return p;
    p.active = true;
    for (int i = 0; i < world_.g.n; ++i) p.gloomPower += world_.g.power[i];
    p.cancelled = world_.cancelled;
    p.spawned = world_.gloomSpawned;
    p.core = world_.core;
    p.spent = world_.spent;
    p.warded = world_.warded;
    for (int g = 0; g < world_.def.geodeCount; ++g)
        if (world_.geodeState[g] == sim::kGeodeAlive) p.geodeHealth += world_.geodeHp[g] + world_.geodeShield[g];
    return p;
}

void Game::stripCheck(const StripPre& pre) {
    const sim::World& W = world_;
    const int born = W.tickCount - 1;  // units made in the tick that just ran
    int bad = 0;
    char line[256];
    int n = std::snprintf(line, sizeof line, "frame %d:", pictureTick_);
    // Multiply gates: every wisp that entered leaves as k wisps (below the cap).
    for (int e = 0; e < W.eventCount; ++e) {
        const sim::Event& ev = W.events[e];
        if (ev.type != sim::kEvGateMul) continue;
        const sim::Gate& G = W.def.gates[ev.a];
        const u32 bit = 1u << ev.a;
        int clones = 0;
        // Clones are made at the gate, at most one step plus 0.12 m past it, inside its span.
        for (int i = 0; i < W.w.n; ++i)
            if (W.w.born[i] == born && (W.w.bits[i] & bit) && W.w.z[i] >= G.z && W.w.z[i] < G.z + 0.3f &&
                std::fabs(W.w.x[i] - W.gateX[ev.a]) <= W.gateHw[ev.a] + 0.15f)
                ++clones;
        const int out = ev.b + clones;
        const int kk = minv(int(tune.kindleMax), int(G.k) + W.kindle);  // KINDLE counts one more
        const bool capped = W.w.n >= sim::kMaxWisps - 64;
        // A clone can meet a gloom and go in the same tick: fewer out is only wrong if nothing clashed.
        bool clashed = false;
        for (int c = 0; c < W.eventCount; ++c) clashed = clashed || W.events[c].type == sim::kEvClashCount;
        const bool ok = capped || out == kk * ev.b || (clashed && out < kk * ev.b);
        bad += ok ? 0 : 1;
        n += std::snprintf(line + n, sizeof line - size_t(n), " x%d in %d out %d%s;", kk, ev.b, out, ok ? "" : " MISMATCH");
        if (n > 200) break;
    }
    // Clash and core: glooms lose power only to clashes (one for one with wisps) and at the lantern.
    i64 post = 0;
    for (int i = 0; i < W.g.n; ++i) post += W.g.power[i];
    const i64 spawned = W.gloomSpawned - pre.spawned;
    i64 coreDamage = 0;
    int clashes = 0;
    for (int e = 0; e < W.eventCount; ++e) {
        if (W.events[e].type == sim::kEvCoreHit) coreDamage += W.events[e].a;
        if (W.events[e].type == sim::kEvClashCount) clashes += W.events[e].a;
    }
    const i64 cancelled = W.cancelled - pre.cancelled;
    // Geodes: the power given to them and their plates is exactly the health they lost (VARIETY_TECH 10).
    i64 geodeAfter = 0, appeared = 0;
    for (int g = 0; g < W.def.geodeCount; ++g) {
        const bool counted = W.geodeState[g] == sim::kGeodeAlive || W.geodeState[g] == sim::kGeodeOpened;
        if (counted) geodeAfter += W.geodeHp[g] + W.geodeShield[g];
    }
    for (int e = 0; e < W.eventCount; ++e)
        if (W.events[e].type == sim::kEvGeodeAppear)
            appeared += maxv<i32>(1, W.def.geodes[W.events[e].a].hp) + maxv<i32>(0, W.def.geodes[W.events[e].a].shield);
    i64 vanished = 0;  // geodes that slipped past or dissolved this tick take their health with them
    for (int e = 0; e < W.eventCount; ++e)
        if (W.events[e].type == sim::kEvGeodeLost) vanished += W.geodeHp[W.events[e].a] + W.geodeShield[W.events[e].a];
    const bool geodeOk = pre.geodeHealth + appeared - vanished - geodeAfter == W.spent - pre.spent;
    bad += geodeOk ? 0 : 1;
    if (W.spent != pre.spent && n < 180)
        n += std::snprintf(line + n, sizeof line - size_t(n), " geodes took %lld of %lld%s;", (long long)(W.spent - pre.spent),
                           (long long)(pre.geodeHealth + appeared - vanished - geodeAfter), geodeOk ? "" : " MISMATCH");
    const bool gloomOk = pre.gloomPower + spawned - cancelled - coreDamage - (W.warded - pre.warded) == post;
    const bool coreOk = i64(pre.core) - i64(W.core) == coreDamage;
    bad += (gloomOk ? 0 : 1) + (coreOk ? 0 : 1);
    if (n < 200)
        std::snprintf(line + n, sizeof line - size_t(n), " clashes %d cancel %lld; core %d->%d hits %lld%s", clashes,
                      (long long)cancelled, pre.core, W.core, (long long)coreDamage, gloomOk && coreOk ? "" : " MISMATCH");
    GT_TAG("GTSIM", "strip %s", line);
    ++stripFrames_;
    stripFailures_ += bad ? 1 : 0;
    if (pictureTick_ == args_.frames) {
        GT_TAG("GTSIM", "strip check: %d ticks, %d disagree", stripFrames_, stripFailures_);
        if (stripFailures_) {
            lintRan_ = true;  // a failed count check fails the run like a lint (exit 7)
            lintProblems_ += stripFailures_;
        }
    }
}

void Game::drawDebug(int w, int h) {
    debug_.clear();
    const float s = float(w) / 1080.0f;
    if (mode_ == Mode::Debug) {
        if (!std::strcmp(args_.scenario, "touch_test")) {
            debug_.rect(0, float(h) * 0.8f, float(w), 4 * s, 0x60F4E8FF);
            debug_.add({markerX_, float(h) * 0.8f, 0, 0, 90 * s, 90 * s, 34, 104, 132, 255});
            if (fingerCount_) debug_.add({fingerX_, fingerY_, 0, 0, 120 * s, 120 * s, 255, 198, 84, 160});
            for (int i = 0; i < 120; ++i) {
                const float ms = frameHist_[(frameHistHead_ + i) % 120];
                const float bh = ms * 6 * s;
                debug_.rect(40 * s + float(i) * 8 * s, 300 * s - bh, 6 * s, bh, ms > 25 ? 0xFF547CFF : 0x54F2E0FF);
            }
            debug_.rect(40 * s, 300 * s - 16.7f * 6 * s, 960 * s, 2 * s, 0xFFFFFF80);
        } else {
            for (int i = 0; i < cloudN_; ++i) {
                const u32 c = ccol_[i];
                debug_.add({cx_[i], cy_[i], cdx_[i], cdy_[i], 18 * s, 18 * s, u8(c >> 24), u8(c >> 16), u8(c >> 8), 220});
            }
        }
    }
    debug_.draw();
}

void Game::render(unsigned fbo, int w, int h) {
    if (mode_ == Mode::Debug) {
        std::memset(&scene_.fu, 0, sizeof scene_.fu);
        scene_.fu.time = V4(0, alpha_, 0, 0);
        scene_.fu.screen = V4(float(w), float(h), 1.0f / float(w), 1.0f / float(h));
        scene_.fu.backdrop[3] = V4(0.04f, 0.04f, 0.15f, 1);
        scene_.world = false;
        uiBatch_.clear();
        scene_.ui = &uiBatch_;
        renderer_.draw(scene_, fbo, w, h);
        drawDebug(w, h);
        checkGl("Game::render");
        return;
    }
    scene_.world = true;
    view_.showLabels = screen_ != Screen::Win || resultTime_ < 0.05f;
    view_.build(scene_, world_, alpha_, labels_);
    uiBatch_.clear();
    ui_.begin(&uiBatch_, layout_);
    if (!args_.noHud && mode_ != Mode::BenchFlood) drawScreens(frameMs_ / 1000.0f);
    if (debugReadout_ || tune.debugReadout > 0) {
        char buf[160];
        std::snprintf(buf, sizeof buf, "%.1f MS  W %d  G %d  TIDE %lld  TICK %.2f MS", double(frameMs_), world_.w.n, world_.g.n,
                      (long long)world_.tide, double(lastTick_ * 1000.0f));
        uiBatch_.text(buf, layout_.px(12), layout_.top() + layout_.px(80), layout_.px(14), rgba(kWhite), rgba(kInk), layout_.px(3));
    }
    ui_.end();
    if (args_.lint && args_.frames > 0) {
        lintRan_ = true;
        lintProblems_ = ui_.lint(layout_);
        // The label check: cap height at least labelMinFrac of the frame and a stroke of 3 px.
        if (view_.minLabelCapPx < 1e8f && (view_.minLabelCapPx < tune.labelMinFrac * float(h) - 0.5f || view_.minLabelStrokePx < 3.0f)) {
            GT_WARN("lint: a label is %.0f px tall with a %.1f px stroke", double(view_.minLabelCapPx), double(view_.minLabelStrokePx));
            ++lintProblems_;
        }
    }
    scene_.ui = &uiBatch_;
    // Between levels the labels and the HUD fade with the picture (GAME_DESIGN 10: 150 ms out, 150 ms in); PAUSE
    // only dims the world, so its menu stays whole.
    const bool levelFade = fadeOut_ > 0 || fadeIn_ < 1;
    scene_.labelOpacity = view_.fade;
    scene_.uiOpacity = levelFade ? view_.fade : 1.0f;
    renderer_.draw(scene_, fbo, w, h);
    checkGl("Game::render");
}

}  // namespace gt

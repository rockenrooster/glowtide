// Scripted pictures and the bench rig (TECH_SPEC 8.1). The crowd script places units without simulating them, to
// reproduce the concept frame's layout and to hold crowds at given counts.
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "core/log.h"
#include "core/rng.h"
#include "core/tuning.h"
#include "game/game.h"
#include "sim/bake.h"

namespace gt {

namespace {

constexpr float kLX = -2.1f;  // the concept frame's launcher

sim::Gate gate(float cx, float hw, float z, int k, bool mul, int row) {
    sim::Gate g;
    g.cx = cx;
    g.hw = hw;
    g.z = z;
    g.k = u8(k);
    g.kind = mul ? sim::kGateMul : sim::kGateDiv;
    g.row = u8(row);
    return g;
}

sim::LevelDef conceptLevel() {
    sim::LevelDef d;
    d.index = 12;
    d.seed = 7;
    d.len = 36;
    d.world = 1;
    d.targetT = 28;
    d.spireHealth = 2480;
    d.coreHealth = 10;
    d.gates[d.gateCount++] = gate(-2.15f, 1.65f, 8, 2, true, 0);
    d.gates[d.gateCount++] = gate(2.35f, 1.65f, 8, 2, false, 0);
    sim::Gate s = gate(-1.6f, 1.8f, 16, 3, true, 1);
    s.slides = 1;
    s.amp = 1.2f;
    s.phase = 0.25f;  // easedWave(0.25) = 0: at its centre at t = 0
    s.period = 5;
    d.gates[d.gateCount++] = s;
    d.gates[d.gateCount++] = gate(-1.0f, 1.4f, 24, 5, true, 2);
    d.barriers[d.barrierCount++] = {1.5f, 4.6f, 15.3f, 40};
    d.bumpers[d.bumperCount++] = {-4.0f, 24, 0.7f};
    d.bumpers[d.bumperCount++] = {2.1f, 24, 0.7f};
    d.emitters[d.emitterCount++] = {2.5f, 0, 5, 0};
    d.rowCount = 3;
    d.rowZ[0] = 8;
    d.rowZ[1] = 16;
    d.rowZ[2] = 24;
    return d;
}

void addUnit(sim::Units& u, int cap, float x, float z, i32 power, u8 flags, Rng& r) {
    if (u.n >= cap) return;
    const int i = u.n++;
    u.x[i] = x;
    u.z[i] = z;
    u.xp[i] = x;
    u.zp[i] = z;
    u.vx[i] = 0;
    u.power[i] = power;
    u.bits[i] = 0;
    u.born[i] = -1000;
    u.flags[i] = flags;
    u.seed[i] = u8(r.next());
    u.clashed[i] = 0;
}

float front(float x) { return 33.0f - 2.3f * maxv(0.0f, x - 0.5f); }

// The concept frame's crowd (docs/concept/make_concept.py), from the project's own random stream.
void conceptCrowd(sim::World& w, int wispTarget, Rng& r) {
    const float LEN = w.def.len;
    w.w.n = w.g.n = 0;
    auto wisp = [&](float x, float z, int tier) {
        const i32 p = tier == 2 ? 20 : tier == 1 ? 6 : 1;
        addUnit(w.w, sim::kMaxWisps, clampv(x, -sim::kHW + 0.3f, sim::kHW - 0.3f), z, p, 0, r);
    };
    for (float z = sim::kMuzzleZ + 0.1f; z < 8.0f; z += 0.62f) wisp(kLX + r.range(-0.07f, 0.07f), z, 0);
    for (float z = 8.3f; z < 16.0f; z += 0.62f)
        for (int k = 0; k < 2; ++k) wisp(kLX + (float(k) - 0.5f) * 0.8f + r.range(-0.15f, 0.15f), z + r.range(-0.2f, 0.2f), 0);
    for (float z = 16.3f; z < 24.0f; z += 0.62f)
        for (int k = 0; k < 6; ++k) wisp(kLX + r.range(-1.5f, 1.5f), z + r.range(-0.28f, 0.28f), 0);
    const int per = maxv(1, wispTarget / 420);
    for (float z = 24.3f; z < LEN - 0.3f; z += 0.62f) {
        for (int k = 0; k < 6 * per; ++k) {
            const float x0 = kLX + r.range(-1.5f, 1.5f), zz = z + r.range(-0.28f, 0.28f);
            if (x0 >= -2.4f && x0 <= 0.4f) {
                for (int j = 0; j < 5; ++j) {
                    const float x = clampv(x0 + r.range(-2.9f, 2.9f), -sim::kHW + 0.4f, sim::kHW - 0.4f);
                    const float zj = zz + r.range(-0.3f, 0.3f);
                    if (zj < front(x)) {
                        const float roll = r.f01();
                        wisp(x, zj, roll < 0.03f ? 2 : roll < 0.15f ? 1 : 0);
                    }
                }
            } else if (zz < front(x0)) {
                wisp(x0, zz, 0);
            }
        }
    }
    for (int k = 0; k < 74; ++k) {
        const float x = r.range(0.7f, 4.6f);
        addUnit(w.g, sim::kMaxGlooms, x, r.range(front(x) + 0.5f, LEN - 0.3f), 1, 0, r);
    }
    for (int k = 0; k < 24; ++k) {
        const float x = r.range(2.7f, 4.6f);
        addUnit(w.g, sim::kMaxGlooms, x, r.range(9.6f, front(x)), 1, 0, r);
    }
    addUnit(w.g, sim::kMaxGlooms, 3.4f, 20.4f, 12, sim::kBrute, r);
    w.tide = 0;
    for (int i = 0; i < w.w.n; ++i) w.tide += w.w.power[i];
}

// A flood of n wisps over the lane, thicker up the lane, plus a crowd of glooms at the spire.
void floodCrowd(sim::World& w, int n, int glooms, Rng& r) {
    w.w.n = w.g.n = 0;
    const float LEN = w.def.len;
    int placed = 0, guard = 0;
    while (placed < n && guard++ < n * 50) {
        const float z = r.range(1.2f, LEN - 3.0f);
        const float spread = clampv(0.2f + (z - 8.0f) * 0.32f, 0.2f, 4.6f);
        const float x = kLX * clampv(1.0f - (z - 8.0f) / 20.0f, 0.0f, 1.0f) + r.range(-spread, spread);
        if (std::fabs(x) > sim::kHW - 0.3f) continue;
        if (r.f01() > clampv(0.15f + (z - 4.0f) / 26.0f, 0.15f, 1.0f)) continue;
        const float roll = r.f01();
        addUnit(w.w, sim::kMaxWisps, x, z, roll < 0.02f ? 20 : roll < 0.1f ? 6 : 1, 0, r);
        ++placed;
    }
    for (int k = 0; k < glooms; ++k) {
        const float x = r.range(-4.4f, 4.4f);
        addUnit(w.g, sim::kMaxGlooms, x, r.range(LEN - 4.0f, LEN - 0.4f), 1, 0, r);
    }
    w.tide = 0;
    for (int i = 0; i < w.w.n; ++i) w.tide += w.w.power[i];
}

}  // namespace

bool Game::setupPicture(const char* name) {
    Rng r(11);
    worldTicks_ = false;
    pictureScreen_[0] = 0;
    // Screen pictures and moments: a real level, played by a bot ahead of the first frame where needed.
    auto presim = [&](int level, sim::BotKind bot, int maxTicks) {
        enterLevel(level, false);
        coldStart_ = false;
        bot_.init(bot, 1);
        useBot_ = true;
        startPlaying();
        for (int t = 0; t < maxTicks && world_.result == sim::Result::None; ++t) {
            world_.tick(bot_.think(world_));
            view_.onTick(world_);
            if (t % 2 == 0) view_.update(1.0f / 30.0f, world_);
        }
    };
    if (!std::strcmp(name, "home_update")) {
        // HOME with the updater's pill, as if GitHub had build 999 (the desktop never asks).
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        enterLevel(3, false);
        coldStart_ = true;
        updateOverride_ = 999;
        return true;
    }
    if (!std::strcmp(name, "level_top")) {
        // A level from straight above with its labels (P5 level sheets); --level picks it.
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        enterLevel(args_.level > 0 ? args_.level : 1, false);
        coldStart_ = false;
        view_.topDown = true;
        setScreen(Screen::Play);
        return true;
    }
    const struct {
        const char* name;
        int level;
    } screens[] = {{"home", 3}, {"hud", 3}, {"pause", 3}, {"win", 3}, {"lose", 2}, {"settings", 3}, {"button_strip", 3}};
    for (const auto& sc : screens) {
        if (std::strcmp(name, sc.name)) continue;
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        if (!std::strcmp(name, "home") || !std::strcmp(name, "settings")) {
            enterLevel(sc.level, false);
            coldStart_ = !std::strcmp(name, "home");
            if (!std::strcmp(name, "settings")) setScreen(Screen::Settings);
            return true;
        }
        if (!std::strcmp(name, "hud") || !std::strcmp(name, "pause")) {
            presim(sc.level, sim::BotKind::Greedy, 60 * 7);
            worldTicks_ = true;
            if (!std::strcmp(name, "pause")) setScreen(Screen::Pause);
            return true;
        }
        if (!std::strcmp(name, "win") || !std::strcmp(name, "button_strip")) {
            presim(sc.level, sim::BotKind::Greedy, 60 * 120);
            finishLevel(world_.result == sim::Result::Win);
            resultTime_ = 3.0f;
            starsLanded_ = stars_;
            popGloomsAbove(-10.0f);
            return true;
        }
        if (!std::strcmp(name, "lose")) {
            presim(sc.level, sim::BotKind::Idle, 60 * 120);
            finishLevel(false);
            resultTime_ = 2.0f;
            return true;
        }
    }
    if (!std::strcmp(name, "lanterns_3") || !std::strcmp(name, "hud_boons")) {
        // Three lanterns with QUICKEN and BRIGHT at their limits on a real lane (VARIETY_PLAN V1): the stream, the lenses
        // and a full boon tray (hud_boons: the same, for the layout lint across the frames).
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        enterLevel(3, false);
        coldStart_ = false;
        bot_.init(sim::BotKind::Greedy, 1);
        useBot_ = true;
        startPlaying();
        world_.lanterns = 3;
        world_.quicken = 3;
        world_.bright = 2;
        world_.kindle = 1;  // V2: through a kindled lane, and a full tray of four chips
        for (int t = 0; t < 60 * 4 && world_.result == sim::Result::None; ++t) {
            world_.tick(bot_.think(world_));
            view_.onTick(world_);
            if (t % 2 == 0) view_.update(1.0f / 30.0f, world_);
        }
        worldTicks_ = true;
        return true;
    }
    if (!std::strcmp(name, "geode_strip") || !std::strcmp(name, "ward_strip") || !std::strcmp(name, "kindle_strip") ||
        !std::strcmp(name, "comet_strip")) {
        // A geode cracking and opening, its icon flying to the tray (level 6's LANTERN), WARD's ring (level 23's second
        // geode), KINDLE's count-up run up the lane (level 36's first) or COMET's tails (level 52's second): played until the
        // geode is under 40% of its health (greedy; hungry for the later two, which are detours); the frames run the rest.
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        const bool ward = !std::strcmp(name, "ward_strip"), kindle = !std::strcmp(name, "kindle_strip"),
                   comet = !std::strcmp(name, "comet_strip");
        enterLevel(ward ? 23 : kindle ? 36 : comet ? 52 : 6, false);
        int gi = ward ? 1 : 0;
        for (int g = world_.def.geodeCount - 1; g >= 0; --g)  // the level's first geode of the strip's boon
            if (world_.def.geodes[g].boon == (kindle ? sim::kBoonKindle : comet ? sim::kBoonComet : world_.def.geodes[gi].boon)) gi = g;
        coldStart_ = false;
        bot_.init(kindle || comet ? sim::BotKind::Hungry : sim::BotKind::Greedy, 1);
        useBot_ = true;
        startPlaying();
        for (int t = 0; t < 60 * 60 && world_.result == sim::Result::None; ++t) {
            const bool near = world_.geodeState[gi] == sim::kGeodeAlive && world_.geodeHp[gi] * 10 < world_.def.geodes[gi].hp * 4;
            if (near || world_.geodeState[gi] > sim::kGeodeAlive) break;
            world_.tick(bot_.think(world_));
            view_.onTick(world_);
            if (t % 2 == 0) view_.update(1.0f / 30.0f, world_);
        }
        worldTicks_ = true;
        return true;
    }
    if (!std::strcmp(name, "pan_strip")) {
        // The thumb flicks the lantern across the lane every half second while it fires: every shot must leave the
        // barrel's tip, wherever the lantern has gone since (VARIETY_PLAN work, 2026-11-20).
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        enterLevel(3, false);
        coldStart_ = false;
        useBot_ = false;
        panScript_ = true;
        startPlaying();
        worldTicks_ = true;
        return true;
    }
    const struct {
        const char* name;
        int level;
        sim::BotKind bot;
        int ticks;
    } moments[] = {{"gate_strip", 3, sim::BotKind::Greedy, 150},   {"clash_strip", 25, sim::BotKind::Greedy, 60 * 13},
                   {"core_hit_strip", 2, sim::BotKind::Idle, 0},   {"win_strip", 3, sim::BotKind::Greedy, 0},
                   {"boss_win_strip", 10, sim::BotKind::Greedy, 0}, {"lose_strip", 2, sim::BotKind::Idle, 0},
                   {"next_strip", 3, sim::BotKind::Greedy, 0}};
    for (const auto& m : moments) {
        if (std::strcmp(name, m.name)) continue;
        std::snprintf(pictureScreen_, sizeof pictureScreen_, "%s", name);
        if (m.ticks > 0) {
            presim(m.level, m.bot, m.ticks);
            worldTicks_ = true;
        } else if (!std::strcmp(name, "core_hit_strip")) {
            enterLevel(m.level, false);
            coldStart_ = false;
            bot_.init(m.bot, 1);
            useBot_ = true;
            startPlaying();
            for (int t = 0; t < 60 * 60; ++t) {  // play until a gloom is about to reach the lantern
                bool near = false;
                for (int i = 0; i < world_.g.n; ++i) near = near || world_.g.z[i] < 2.4f;
                if (near) break;
                world_.tick(bot_.think(world_));
            }
            worldTicks_ = true;
        } else {
            // Win and lose moments: play to the end, then let the frames run through the sequence.
            presim(m.level, m.bot, 60 * 150);
            const bool won = world_.result == sim::Result::Win;
            view_.onTick(world_);
            finishLevel(won);
            if (!std::strcmp(name, "next_strip")) {
                resultTime_ = 3.0f;
                starsLanded_ = stars_;
                popGloomsAbove(-10.0f);
            }
        }
        return true;
    }
    sim::LevelDef def = conceptLevel();
    int world = args_.world;
    if (!std::strncmp(name, "world_", 6)) world = std::atoi(name + 6);
    if (!std::strcmp(name, "geodes")) {
        // Every boon's geode, a shielded one, and beside them a gate, a divide gate, a snuffer, glooms and a brute: the
        // greyscale check tells a geode from each (VARIETY_DESIGN 13).
        def = sim::LevelDef();
        def.index = 25;
        def.len = 36;
        def.spireHealth = 2400;
        def.coreHealth = 10;
        def.gates[def.gateCount++] = gate(-2.6f, 1.6f, 8, 3, true, 0);
        def.gates[def.gateCount++] = gate(2.6f, 1.6f, 8, 2, false, 0);
        def.snuffers[def.snufferCount++] = {-3.6f, -0.6f, 30.0f, 3.0f, 0.25f};
        def.emitters[def.emitterCount++] = {1.5f, 0, 5, 0};
        const struct {
            float x, z;
            u8 boon;
            i32 hp, shield;
        } gs[] = {{-2.7f, 13.5f, sim::kBoonLantern, 120, 0}, {0.0f, 17.0f, sim::kBoonQuicken, 85, 0}, {2.7f, 13.5f, sim::kBoonBright, 60, 0},
                  {-2.2f, 23.0f, sim::kBoonWard, 40, 0},       {2.3f, 25.5f, sim::kBoonLantern, 30, 200}};
        for (const auto& q : gs) {
            sim::Geode g;
            g.x = q.x;
            g.z0 = q.z;
            g.speed = 0;
            g.boon = q.boon;
            g.hp = q.hp;
            g.shield = q.shield;
            def.geodes[def.geodeCount++] = g;
        }
    }
    if (!std::strcmp(name, "geodes")) {
        world_.init(def, 0);
        world_.enemyOff = true;
        world_.tick(sim::Input{});  // they appear
        world_.lanterns = 3;
        world_.quicken = 2;
        world_.bright = 1;
        world_.launcherX = -0.4f;
        world_.w.n = world_.g.n = 0;
        for (int k = 0; k < 3; ++k)
            for (float z = sim::kMuzzleZ + 0.2f; z < 7.0f; z += 0.7f) addUnit(world_.w, sim::kMaxWisps, world_.lanternX(k) + r.range(-0.06f, 0.06f), z, 3, 0, r);
        for (int k = 0; k < 24; ++k) addUnit(world_.g, sim::kMaxGlooms, r.range(-1.0f, 3.5f), r.range(31.0f, 34.0f), 1, 0, r);
        addUnit(world_.g, sim::kMaxGlooms, 0.8f, 28.5f, 30, sim::kBrute, r);
        view_.setLevel(def, 2);
        if (graphicsReady_) renderer_.setLevel(def, *view_.wp, *view_.gp);
        level_ = 25;
        setScreen(Screen::Play);
        pictureTick_ = 0;
        return true;
    }
    if (!std::strcmp(name, "elements")) {
        def = sim::LevelDef();
        def.index = 1;
        def.len = 36;
        def.spireHealth = 900;
        def.coreHealth = 10;
        def.gates[def.gateCount++] = gate(-2.6f, 1.6f, 8, 2, true, 0);
        def.gates[def.gateCount++] = gate(2.6f, 1.6f, 8, 2, false, 0);
        sim::Gate s = gate(-1.4f, 1.6f, 14, 3, true, 1);
        s.slides = 1;
        s.amp = 1.6f;
        s.phase = 0.25f;
        def.gates[def.gateCount++] = s;
        sim::Gate br = gate(2.6f, 1.5f, 22, 4, true, 2);  // V2: a breathing gate, here at its widest
        br.breath = 4.0f;
        br.phase = 0.5f;
        def.gates[def.gateCount++] = br;
        def.gates[def.gateCount++] = gate(2.7f, 1.2f, 14.8f, 3, false, 1);  // a divide-by-three
        sim::Gate h = gate(-0.2f, 1.3f, 30, 2, true, 3);  // a hopper on its left, its ghost frame on the right
        h.slides = sim::kGateHop;
        h.amp = 2.2f;
        h.period = 2.0f;
        def.gates[def.gateCount++] = h;
        def.barriers[def.barrierCount++] = {1.0f, 4.2f, 20.0f, 40, 0.6f, 5.0f, 0.0f};  // a sliding wall, shifted left
        def.bumpers[def.bumperCount++] = {-3.1f, 21.0f, 0.7f};
        def.snuffers[def.snufferCount++] = {-3.6f, -0.6f, 26.5f, 3.0f, 0.25f};
        def.emitters[def.emitterCount++] = {0, 0, 5, 0};
        sim::Emitter door;  // a side door on the left rail
        door.side = -1;
        door.z = 17.5f;
        door.x = -(sim::kHW - 0.45f);
        def.emitters[def.emitterCount++] = door;
    }
    world_.init(def, 0);
    world_.launcherX = kLX;
    world_.core = 8;
    view_.setLevel(def, world);
    if (graphicsReady_) renderer_.setLevel(def, *view_.wp, *view_.gp);
    level_ = 12;
    setScreen(Screen::Play);
    pictureTick_ = 0;
    if (!std::strcmp(name, "concept") || !std::strncmp(name, "world_", 6)) {
        conceptCrowd(world_, 420, r);
    } else if (!std::strncmp(name, "density_", 8)) {
        const int n = clampv(std::atoi(name + 8), 100, sim::kMaxWisps);
        floodCrowd(world_, n, 300, r);
    } else if (!std::strcmp(name, "label_over_flood")) {
        floodCrowd(world_, 2400, 200, r);
    } else if (!std::strcmp(name, "unit_tiers") || !std::strcmp(name, "elements")) {
        world_.w.n = world_.g.n = 0;
        const i32 powers[4] = {1, 6, 20, 80};
        for (int t = 0; t < 4; ++t) {
            addUnit(world_.w, sim::kMaxWisps, -3.6f + float(t) * 1.15f, 4.0f, powers[t], 0, r);
            addUnit(world_.g, sim::kMaxGlooms, 0.6f + float(t) * 1.15f, 4.0f, powers[t], 0, r);
        }
        addUnit(world_.g, sim::kMaxGlooms, 2.6f, 29.0f, 30, sim::kBrute, r);
        if (!std::strcmp(name, "elements")) {
            for (int k = 0; k < 40; ++k) addUnit(world_.w, sim::kMaxWisps, kLX + r.range(-0.4f, 0.4f), r.range(sim::kMuzzleZ + 0.1f, 7.5f), 1, 0, r);
            for (int k = 0; k < 30; ++k) addUnit(world_.g, sim::kMaxGlooms, r.range(-3, 3), r.range(31, 35), 1, 0, r);
            // V2: a pack of runners, two glooms stepping in at the side door, a comet leaving the lantern; a kindled lane.
            for (int k = 0; k < 9; ++k) addUnit(world_.g, sim::kMaxGlooms, 0.8f + r.range(-0.9f, 0.9f), 11.0f + r.range(-1.2f, 1.2f), 1, sim::kRunner, r);
            for (int k = 0; k < 2; ++k) addUnit(world_.g, sim::kMaxGlooms, -4.2f + 0.7f * float(k), 17.5f + 0.5f * float(k), 1, sim::kStepping, r);
            addUnit(world_.w, sim::kMaxWisps, kLX + 0.6f, 9.5f, 96, sim::kComet, r);
            world_.kindle = 1;
        }
    } else {
        return false;
    }
    return true;
}

void Game::tickPictureScript() {
    ++pictureTick_;
    if (pictureScreen_[0]) {
        // button_strip: NEXT normal, then pressed, then released (the release fires it on the last frame).
        if (!std::strcmp(pictureScreen_, "button_strip")) {
            const int last = args_.frames;
            const UiControl* c = ui_.find("NEXT");
            if (c && pictureTick_ == last - 8) {
                PointerEvent d{PtrType::Down, 5, c->r.cx(), c->r.cy(), 0, 0};
                pointer(d);
            }
            if (c && pictureTick_ == last) {
                PointerEvent u{PtrType::Up, 5, c->r.cx(), c->r.cy(), 0, 0};
                pointer(u);
            }
        }
        if (!std::strcmp(pictureScreen_, "next_strip") && pictureTick_ == args_.frames - 26) {
            if (const UiControl* c = ui_.find("NEXT")) {
                PointerEvent d{PtrType::Down, 5, c->r.cx(), c->r.cy(), 0, 0};
                pointer(d);
                PointerEvent u{PtrType::Up, 5, c->r.cx(), c->r.cy(), 0, 0};
                pointer(u);
            }
        }
        return;
    }
    // Feed the view the moments of a busy frame: gate passes, clashes along the front, hits on the spire.
    world_.eventCount = 0;
    Rng r(u64(pictureTick_) * 31u + 5u);
    auto ev = [&](u8 type, float x, float z, int a, int b) {
        if (world_.eventCount < sim::kMaxEvents) world_.events[world_.eventCount++] = {type, x, z, a, b};
    };
    if (!std::strncmp(args_.picture, "concept", 7) || !std::strncmp(args_.picture, "world_", 6) ||
        !std::strncmp(args_.picture, "density_", 8) || !std::strcmp(args_.picture, "label_over_flood")) {
        if (pictureTick_ % 3 == 0) ev(sim::kEvGateMul, kLX, 8, 0, 2);
        if (pictureTick_ % 4 == 1) ev(sim::kEvGateMul, -1.6f, 16, 2, 4);
        if (pictureTick_ % 3 == 2) ev(sim::kEvGateMul, -0.8f, 24, 3, 6);
        for (int k = 0; k < 4; ++k) {
            const float x = r.range(0.7f, 4.4f);
            ev(sim::kEvClash, x, front(x) + r.range(-0.4f, 0.4f), 1, 0);
        }
        if (pictureTick_ % 2 == 0) ev(sim::kEvSpireHit, r.range(-1.5f, 0.5f), world_.def.len, 12, 0);
        world_.firing = true;
    }
    view_.onTick(world_);
    world_.eventCount = 0;
}

void Game::setupBenchFlood() {
    // The contract's scene (TECH_SPEC 8.1): both crowds held at their caps, flowing over a five-row level with every
    // gate passing; the real World in its bench rig (no clash, no damage, units wrap).
    sim::initBenchWorld(world_);
    const sim::LevelDef d = world_.def;
    view_.setLevel(d, 0);
    worldTicks_ = true;
}

}  // namespace gt

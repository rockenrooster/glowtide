// The simulation's tests (TECH_SPEC 8.4): rules and their arithmetic, the generator's invariants, no allocation in a
// tick, the camera's fit across the aspect matrix, the palette lint and the font's glyph set.
#include "tests_sim.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "core/tuning.h"
#include "core/types.h"
#include "gfx/camera.h"
#include "gfx/palette.h"
#include "gfx/text.h"
#include "sim/bake.h"
#include "sim/bots.h"
#include "sim/level_build.h"
#include "sim/world.h"

namespace gt::test {

using namespace gt::sim;

namespace {

LevelDef emptyLevel() {
    LevelDef d;
    d.len = 36;
    d.spireHealth = 100000;
    d.coreHealth = 10;
    return d;
}

void place(Units& u, float x, float z, i32 power) {
    const int i = u.n++;
    u.x[i] = x;
    u.z[i] = z;
    u.xp[i] = x;
    u.zp[i] = z;
    u.vx[i] = 0;
    u.power[i] = power;
    u.bits[i] = 0;
    u.born[i] = 0;
    u.flags[i] = 0;
    u.seed[i] = 0;
    u.clashed[i] = 0;
}

i64 tideOf(const World& w) {
    i64 t = 0;
    for (int i = 0; i < w.w.n; ++i) t += w.w.power[i];
    return t;
}

void testMultiply(float capW) {
    const float saved = tune.capW;
    tune.capW = capW;
    static World w;
    LevelDef d = emptyLevel();
    Gate g;
    g.cx = 0;
    g.hw = 1.8f;
    g.z = 10;
    g.k = 3;
    d.gates[d.gateCount++] = g;
    w.init(d, 1);
    for (int i = 0; i < 40; ++i) place(w.w, -1.0f + float(i) * 0.05f, 9.95f, 1 + i % 5);
    const i64 before = tideOf(w);
    Input in;
    w.tick(in);  // every wisp crosses z = 10 this tick
    GT_CHECK_MSG(tideOf(w) == before * 3, capW < 200 ? "x3 with the cap forced low" : "x3 with room for clones");
    if (capW < 200) GT_CHECK_MSG(w.w.n <= int(capW), "count within the cap");
    else GT_CHECK_MSG(w.w.n == 120, "two clones each");
    tune.capW = saved;
}

void testDivide() {
    static World w;
    LevelDef d = emptyLevel();
    Gate g;
    g.cx = 0;
    g.hw = 1.8f;
    g.z = 10;
    g.k = 3;
    g.kind = kGateDiv;
    d.gates[d.gateCount++] = g;
    w.init(d, 1);
    const i32 powers[] = {1, 2, 3, 5, 7, 1, 1, 1, 4, 2, 1, 8};
    i64 in = 0;
    for (int i = 0; i < countOf(powers); ++i) {
        place(w.w, -1.5f + float(i) * 0.25f, 9.95f, powers[i]);
        in += powers[i];
    }
    Input inp;
    w.tick(inp);
    GT_CHECK_MSG(tideOf(w) == in / 3 && w.gateCarry[0] == in % 3, "exactly a third of a mixed stream gets through");
}

// P5 rules: a snuffer takes a wisp it touches and nothing else.
void testSnuffer() {
    static World w;
    LevelDef d = emptyLevel();
    Snuffer sn;
    sn.xa = -0.05f;
    sn.xb = 0.05f;
    sn.z = 12;
    d.snuffers[d.snufferCount++] = sn;
    w.init(d, 1);
    place(w.w, w.snufferX[0], 11.9f, 5);
    place(w.w, 3.5f, 11.9f, 7);
    Input in;
    w.tick(in);
    GT_CHECK_MSG(w.w.n == 1 && w.w.power[0] == 7, "a snuffer takes the wisp it touches, and only that one");
}

// Sliding gates follow cx + amp * wave(t / period + phase), the position at the end of each tick.
void testSliders() {
    static World w;
    LevelDef d = emptyLevel();
    Gate g;
    g.cx = 0.5f;
    g.hw = 1.4f;
    g.z = 14;
    g.slides = 1;
    g.amp = 2.0f;
    g.period = 4.0f;
    g.phase = 0.25f;
    d.gates[d.gateCount++] = g;
    Gate h = g;
    h.phase = 0.75f;  // the crossing partner, in opposite phase
    h.z = 15.6f;
    d.gates[d.gateCount++] = h;
    w.init(d, 1);
    Input in;
    for (int t = 0; t < 77; ++t) w.tick(in);
    const float t1 = 77.0f / 60.0f;
    const float want = g.cx + g.amp * easedWave(t1 / g.period + g.phase);
    const float wantH = h.cx + h.amp * easedWave(t1 / h.period + h.phase);
    GT_CHECK_MSG(std::fabs(w.gateX[0] - want) < 1e-4f && std::fabs(w.gateX[1] - wantH) < 1e-4f, "sliders follow their wave");
    GT_CHECK_MSG(std::fabs((w.gateX[0] - g.cx) + (w.gateX[1] - h.cx)) < 1e-3f, "crossing sliders move in opposite phase");
}

// SURGE (the old time burst) doubles the enemy's flow for its duration; one started by the goal starts when the spire
// falls to its share and brings its brutes; brutes have the level's brute power.
void testBursts() {
    static World w;
    LevelDef d = emptyLevel();
    d.targetT = 1000;  // no pressure ramp in this window
    d.baseFlow = 12;
    d.emitters[d.emitterCount++] = {0, 0, 5, 0};
    TwistDef& b = addTwist(d, kTwistSurge, kTrigTime, 3.0f / 1000.0f);
    b.mult = 2.0f;
    b.dur = 3.0f;
    w.init(d, 1);
    Input in;
    auto runTo = [&](float seconds) {
        while (float(w.tickCount) / 60.0f < seconds) w.tick(in);
        return w.gloomSpawned;
    };
    const i64 a = runTo(3.0f), mid = runTo(6.0f), end = runTo(9.0f);
    const i64 during = mid - a, after = end - mid;
    GT_CHECK_MSG(during >= 2 * after - 2 && during <= 2 * after + 2, "a time burst doubles the flow for its duration");

    LevelDef e = emptyLevel();
    e.targetT = 1000;
    e.baseFlow = 6;
    e.brutePower = 24;
    e.emitters[e.emitterCount++] = {0, 0, 5, 0};
    TwistDef& hb = addTwist(e, kTwistSurge, kTrigGoal, 0.66f, 2);
    hb.dur = 4;
    w.init(e, 1);
    runTo(2.0f);
    auto brutes = [&]() {
        int n = 0;
        for (int i = 0; i < w.g.n; ++i) n += (w.g.flags[i] & kBrute) ? 1 : 0;
        return n;
    };
    GT_CHECK_MSG(brutes() == 0, "no brutes before the spire falls");
    w.spire = i32(0.60f * float(w.spireStart));
    w.tick(in);
    int power = 0;
    for (int i = 0; i < w.g.n; ++i)
        if (w.g.flags[i] & kBrute) power = w.g.power[i];
    GT_CHECK_MSG(brutes() == 2 && power == 24, "a health burst starts at its share and brings two brutes of the level's power");

    LevelDef f = emptyLevel();
    f.targetT = 1000;
    f.baseFlow = 6;
    f.bruteEvery = 2.0f;
    f.brutePower = 30;
    f.emitters[f.emitterCount++] = {0, 0, 5, 0};
    w.init(f, 1);
    runTo(1.5f + 2.0f * 3 + 0.05f);
    GT_CHECK_MSG(brutes() == 3, "a brute every bruteEvery seconds after the emitters start");
}

// The kinds (VARIETY_DESIGN 6): each one's win and lose on a minimal level.
void testKinds() {
    static World w;
    Input in;
    // Holdout: won at dawn with the core above zero; wisps at the top hurt nothing.
    LevelDef h = emptyLevel();
    h.kind = kHoldout;
    h.targetT = 5;
    w.init(h, 1);
    place(w.w, 0, h.len - 0.2f, 50);
    while (w.result == Result::None && w.tickCount < 400) w.tick(in);
    GT_CHECK_MSG(w.result == Result::Win && w.tickCount == 300 && w.spire == w.spireStart, "Holdout: won at dawn, nothing at the top");
    w.init(h, 1);
    w.core = 1;
    place(w.g, 0, 0.82f, 3);
    w.tick(in);
    GT_CHECK_MSG(w.result == Result::Lose, "Holdout: lost when the core falls");
    // Festival: cannot be lost and ends on its tick.
    LevelDef f = emptyLevel();
    f.kind = kFestival;
    w.init(f, 1);
    w.core = 0;
    while (w.result == Result::None && w.tickCount < 2000) w.tick(in);
    GT_CHECK_MSG(w.result == Result::Win && w.tickCount == int(tune.festivalSeconds * 60.0f), "a festival cannot be lost and ends on its tick");
    // Colossus: it steps out, absorbs every wisp that touches it in one tick, and wins the level for the enemy at z 3.
    LevelDef c = emptyLevel();
    c.kind = kColossus;
    c.targetT = 10;
    c.spireHealth = 1000;
    w.init(c, 1);
    while (!w.colossusOut) w.tick(in);
    for (int k = 0; k < 6; ++k) place(w.w, w.colossusX + 0.3f * float(k - 3), w.colossusZ - 1.0f, 10);
    w.tick(in);
    GT_CHECK_MSG(w.spire == 940 && tideOf(w) == 0, "the colossus absorbs every wisp that touches it in one tick");
    while (w.result == Result::None && w.tickCount < 6000) w.tick(in);
    GT_CHECK_MSG(w.result == Result::Lose && w.colossusZ <= tune.colossusLoseZ + 0.05f, "the colossus reaching the lantern loses the level");
    w.init(c, 1);
    while (!w.colossusOut) w.tick(in);
    place(w.w, w.colossusX, w.colossusZ - 1.0f, 2000);
    w.tick(in);
    GT_CHECK_MSG(w.result == Result::Win, "the colossus broken wins");
    // Twins: a wisp on a fallen spire's side does no damage; both must fall.
    LevelDef t = emptyLevel();
    t.kind = kTwins;
    t.spireHealth = 100;
    t.goal2 = 100;
    w.init(t, 1);
    w.spire = 0;  // the left one has fallen
    place(w.w, -2.6f, t.len - 0.2f, 40);
    w.tick(in);
    GT_CHECK_MSG(w.spire2 == 100 && w.result == Result::None, "a wisp on a fallen twin's side does no damage");
    place(w.w, 2.6f, t.len - 0.2f, 100);
    w.tick(in);
    GT_CHECK_MSG(w.spire2 <= 0 && w.result == Result::Win, "both twins fallen wins");
}

// Twists (VARIETY_DESIGN 7): each starts once, on its trigger.
void testTwists() {
    static World w;
    Input in;
    LevelDef d = emptyLevel();
    d.targetT = 10;
    d.baseFlow = 6;
    d.brutePower = 20;
    d.emitters[d.emitterCount++].x = 0;
    addTwist(d, kTwistEclipse, kTrigTime, 0.2f);
    addTwist(d, kTwistBruteGuard, kTrigGoal, 0.5f);
    addTwist(d, kTwistSecondWind, kTrigCore, 3.0f);
    Geode wind;
    wind.x = 0;
    wind.z0 = 18;
    wind.speed = 0;
    wind.hp = 5;
    wind.boon = kBoonWard;
    wind.twist = 3;
    d.geodes[d.geodeCount++] = wind;
    w.init(d, 1);
    int fired[3] = {}, starts = 0;
    for (int t = 0; t < 600; ++t) {
        if (t == 200) w.spire = w.spireStart / 3;
        if (t == 300) w.core = 3;
        w.tick(in);
        for (int e = 0; e < w.eventCount; ++e)
            if (w.events[e].type == kEvTwist) ++fired[w.events[e].a], ++starts;
    }
    GT_CHECK_MSG(starts == 3 && fired[0] == 1 && fired[1] == 1 && fired[2] == 1, "each twist starts once");
    GT_CHECK_MSG(w.twistStart[0] == 120 && w.twistStart[1] == 200 && w.twistStart[2] == 300, "on its trigger: time, goal, core");
    GT_CHECK_MSG(w.geodeState[0] == kGeodeAlive, "SECOND WIND releases its WARD geode");
    // GOLDEN GATE: the gate counts two more for its eight seconds.
    LevelDef g = emptyLevel();
    Gate x2;
    x2.cx = 0;
    x2.hw = 1.8f;
    x2.z = 12;
    x2.k = 2;
    g.gates[g.gateCount++] = x2;
    addTwist(g, kTwistGoldenGate, kTrigTime, 0.0f, 0, 0);
    w.init(g, 1);
    w.tick(in);
    place(w.w, 0, 11.95f, 1);
    w.tick(in);
    GT_CHECK_MSG(w.w.n == 4, "GOLDEN GATE: x2 counts as x4");
    // WARD WALL raises its dormant wall; CUB sends a cub down that takes three pips at the lantern.
    LevelDef b = emptyLevel();
    b.spireHealth = 1000;
    Barrier wall;
    wall.x0 = -4.9f;
    wall.x1 = 4.9f;
    wall.z = 30;
    wall.health = 60;
    wall.dormant = 1;
    b.barriers[b.barrierCount++] = wall;
    addTwist(b, kTwistWardWall, kTrigTime, 0.0f, 0, 0);
    addTwist(b, kTwistCub, kTrigTime, 0.0f, 0, 1);
    w.init(b, 1);
    const i32 before = w.barrierHealth[0];
    w.tick(in);
    GT_CHECK_MSG(before == 0 && w.barrierHealth[0] == 60 && w.cubCount == 1 && w.cubHp[0] == 50, "WARD WALL rises; CUB has 5% of the spire");
    while (w.cubHp[0] > 0 && w.tickCount < 4000) w.tick(in);
    GT_CHECK_MSG(w.core == 7, "a cub at the lantern takes three pips");
}

void testClashAndBarrier() {
    static World w;
    LevelDef d = emptyLevel();
    w.init(d, 1);
    place(w.w, 0, 20, 5);
    place(w.g, 0, 20.2f, 3);
    Input in;
    w.tick(in);
    GT_CHECK_MSG(w.g.n == 0 && w.w.n == 1 && w.w.power[0] == 2, "clash: both lose min(power)");

    LevelDef b = emptyLevel();
    b.barriers[b.barrierCount++] = {-2.0f, 2.0f, 15.0f, 40};
    w.init(b, 1);
    for (int i = 0; i < 10; ++i) place(w.w, -1.0f + float(i) * 0.2f, 14.95f, 5);
    w.tick(in);
    GT_CHECK_MSG(w.barrierHealth[0] <= 0, "barrier breaks after 40 power");
    GT_CHECK_MSG(tideOf(w) == 10, "the left-over power carries on");
}

void testBumper() {
    static World w;
    LevelDef d = emptyLevel();
    d.bumpers[d.bumperCount++] = {0, 12.0f, 0.7f};
    w.init(d, 1);
    place(w.w, 0.01f, 10.0f, 1);
    place(w.g, -0.02f, 14.0f, 1);
    Input in;
    bool inside = false;
    for (int t = 0; t < 60; ++t) {
        w.tick(in);
        for (int team = 0; team < 2; ++team) {
            const Units& u = team ? w.g : w.w;
            for (int i = 0; i < u.n; ++i) {
                const float dx = u.x[i], dz = u.z[i] - 12.0f;
                inside = inside || std::sqrt(dx * dx + dz * dz) < 0.7f + w.radius(team, i) - 0.01f;
            }
        }
    }
    GT_CHECK_MSG(!inside, "units slide round bumpers");
    GT_CHECK_MSG(w.w.n == 1 && w.w.z[0] > 15.0f, "a wisp is not stopped by a bumper");
}

// Geodes and boons (VARIETY_DESIGN 2, 3; VARIETY_TECH 10's first row).
Geode geodeAt(float x, float z, i32 hp, u8 boon, i32 shield = 0) {
    Geode g;
    g.x = x;
    g.z0 = z;
    g.t = 0;
    g.speed = 0.0f;  // still, so the arithmetic is about the wisps alone
    g.hp = hp;
    g.shield = shield;
    g.boon = boon;
    return g;
}

int eventsOf(const World& w, u8 type) {
    int n = 0;
    for (int e = 0; e < w.eventCount; ++e) n += w.events[e].type == type ? 1 : 0;
    return n;
}

void testGeodes() {
    static World w;
    Input in;
    // A geode takes exactly the power it is given and passes the rest on.
    LevelDef d = emptyLevel();
    d.geodes[d.geodeCount++] = geodeAt(0, 20, 10, kBoonQuicken);
    w.init(d, 1);
    w.tick(in);  // it appears
    place(w.w, -0.1f, 20.0f, 4);
    place(w.w, 0.0f, 20.0f, 4);
    place(w.w, 0.1f, 20.0f, 4);
    w.tick(in);
    GT_CHECK_MSG(w.geodeState[0] == kGeodeOpened && w.spent == 10 && tideOf(w) == 2, "a geode takes its health and passes the rest on");
    GT_CHECK_MSG(w.quicken == 1 && eventsOf(w, kEvGeodeOpen) == 1 && eventsOf(w, kEvBoon) == 1, "opening applies the boon in that tick");

    // A plate goes first, from any side: while it stands the geode takes nothing, even from a wisp already past it.
    LevelDef p = emptyLevel();
    p.geodes[p.geodeCount++] = geodeAt(0, 20, 3, kBoonLantern, 5);
    w.init(p, 1);
    w.tick(in);
    place(w.w, 0.0f, 20.0f, 6);  // on the body itself, beyond the plate
    w.tick(in);
    GT_CHECK_MSG(w.geodeHp[0] == 3 && w.geodeShield[0] == 5 && tideOf(w) == 6, "the geode takes nothing while its plate stands");
    w.w.n = 0;
    place(w.w, 0.5f, 20.0f - tune.plateGap - 0.05f, 4);  // crosses the plate this tick
    w.tick(in);
    GT_CHECK_MSG(w.geodeShield[0] == 1 && w.geodeHp[0] == 3 && w.spent == 4, "the plate takes the power that crosses it");
    place(w.w, -0.5f, 20.0f - tune.plateGap - 0.05f, 4);
    w.tick(in);
    GT_CHECK_MSG(w.geodeShield[0] <= 0 && w.spent == 5 && tideOf(w) == 3, "the plate breaks; the rest of that wisp carries on");
    for (int t = 0; t < 10 && w.geodeState[0] == kGeodeAlive; ++t) w.tick(in);
    GT_CHECK_MSG(w.geodeState[0] == kGeodeOpened && w.spent == 8 && w.lanterns == 2, "then the geode itself, and its boon");

    // Slipping past costs nothing.
    LevelDef s = emptyLevel();
    Geode g = geodeAt(1.0f, 4.0f, 50, kBoonLantern);
    g.speed = 2.0f;
    s.geodes[s.geodeCount++] = g;
    w.init(s, 1);
    for (int t = 0; t < 120; ++t) w.tick(in);
    GT_CHECK_MSG(w.geodeState[0] == kGeodeLost && w.core == 10 && w.spent == 0 && w.lanterns == 1, "a geode that slips past costs nothing");

    // A choice pair: opening one dissolves the other.
    LevelDef c = emptyLevel();
    Geode a = geodeAt(-2.0f, 20, 1, kBoonLantern), b = geodeAt(2.0f, 20, 1, kBoonBright);
    a.group = b.group = 1;
    c.geodes[c.geodeCount++] = a;
    c.geodes[c.geodeCount++] = b;
    w.init(c, 1);
    w.tick(in);
    place(w.w, -2.0f, 20.0f, 1);
    w.tick(in);
    GT_CHECK_MSG(w.geodeState[0] == kGeodeOpened && w.geodeState[1] == kGeodeLost && w.bright == 0, "a choice pair's other geode dissolves");

    // Each boon's arithmetic and limit.
    LevelDef q = emptyLevel();
    for (int k = 0; k < 4; ++k) q.geodes[q.geodeCount++] = geodeAt(-3.0f + 2.0f * float(k), 20, 1, kBoonQuicken);
    for (int k = 0; k < 3; ++k) q.geodes[q.geodeCount++] = geodeAt(-3.0f + 3.0f * float(k), 26, 1, kBoonBright);
    for (int k = 0; k < 3; ++k) q.geodes[q.geodeCount++] = geodeAt(-3.0f + 3.0f * float(k), 32, 1, kBoonLantern);
    w.init(q, 1);
    w.tick(in);
    float rates[4];
    for (int k = 0; k < 4; ++k) {
        place(w.w, -3.0f + 2.0f * float(k), 20.0f, 1);
        w.tick(in);
        rates[k] = w.fireRate();
    }
    GT_CHECK_MSG(std::fabs(rates[0] - 15) < 1e-3f && std::fabs(rates[1] - 18) < 1e-3f && std::fabs(rates[2] - 21) < 1e-3f &&
                     std::fabs(rates[3] - 21) < 1e-3f,
                 "QUICKEN: 12, 15, 18, 21 shots a second, no more");
    int powers[3], counts[3];
    float lo[3], hi[3];
    for (int k = 0; k < 3; ++k) {
        place(w.w, -3.0f + 3.0f * float(k), 26.0f, 1);
        w.tick(in);
        powers[k] = w.shotPower();
    }
    for (int k = 0; k < 3; ++k) {
        place(w.w, -3.0f + 3.0f * float(k), 32.0f, 1);
        w.tick(in);
        counts[k] = w.lanterns;
        lo[k] = w.aimMin();
        hi[k] = w.aimMax();
    }
    GT_CHECK_MSG(powers[0] == 2 && powers[1] == 3 && powers[2] == 3, "BRIGHT: power 2, then 3, no more");
    GT_CHECK_MSG(counts[0] == 2 && counts[1] == 3 && counts[2] == 3, "LANTERN: two lanterns, then three, no more");
    GT_CHECK_MSG(std::fabs(lo[0] + 4.4f) < 1e-4f && std::fabs(hi[0] - 3.5f) < 1e-4f && std::fabs(lo[1] + 3.5f) < 1e-4f &&
                     std::fabs(hi[1] - 3.5f) < 1e-4f,
                 "the aim range keeps every lantern on the rail");
    // Three lanterns fire one wisp each on one shot, 0.9 m apart, with BRIGHT's power.
    w.w.n = 0;
    Input fire;
    fire.firing = true;
    fire.launcherTargetX = 9.0f;  // clamped to the range
    for (int t = 0; t < 30; ++t) w.tick(fire);
    GT_CHECK_MSG(w.launcherX <= 3.5f + 1e-4f, "the launcher stays in the narrowed range");
    float minX = 9, maxX = -9;
    for (int i = 0; i < w.w.n; ++i) minX = minv(minX, w.w.x[i]), maxX = maxv(maxX, w.w.x[i]);
    GT_CHECK_MSG(w.w.n > 0 && w.w.n % 3 == 0 && w.w.power[0] == 3 && maxX - minX > 1.6f, "three lanterns: three wisps a shot, 1.8 m across");

    // WARD: the core back to 10; every gloom below wardZ pops, the others stay.
    LevelDef wd = emptyLevel();
    wd.geodes[wd.geodeCount++] = geodeAt(0, 20, 1, kBoonWard);
    w.init(wd, 1);
    w.core = 4;
    w.tick(in);
    place(w.g, 2.0f, 6.0f, 3);
    place(w.g, -2.0f, 13.0f, 1);
    place(w.g, 3.0f, 30.0f, 2);
    place(w.w, 0.0f, 20.0f, 1);
    w.tick(in);
    GT_CHECK_MSG(w.core == 10 && w.g.n == 1 && w.g.power[0] == 2, "WARD: the core whole, the glooms below z 14 gone");
}

// The lane and the enemy (VARIETY_DESIGN 3's last three boons, 4, 5; VARIETY_TECH 10).
void testLaneAndEnemy() {
    static World w;
    Input in;
    // A hopper: two seconds at cx - amp, a quarter-second jump, two seconds at cx + amp, and back.
    Gate h;
    h.cx = 0;
    h.hw = 1.5f;
    h.z = 12;
    h.slides = kGateHop;
    h.amp = 2.0f;
    h.period = 2.0f;
    GT_CHECK_MSG(std::fabs(gateXAt(h, 0.0f) + 2.0f) < 1e-4f && std::fabs(gateXAt(h, 1.9f) + 2.0f) < 1e-4f &&
                     std::fabs(gateXAt(h, 2.125f)) < 1e-3f && std::fabs(gateXAt(h, 2.3f) - 2.0f) < 1e-4f &&
                     std::fabs(gateXAt(h, 4.375f)) < 1e-3f && std::fabs(gateXAt(h, 4.6f) + 2.0f) < 1e-4f,
                 "a hopper stays, jumps in 0.25 s, stays, jumps back");
    LevelDef hd = emptyLevel();
    hd.gates[hd.gateCount++] = h;
    w.init(hd, 1);
    for (int t = 0; t < 138; ++t) w.tick(in);  // 2.3 s
    GT_CHECK_MSG(std::fabs(w.gateX[0] - 2.0f) < 1e-4f, "the world's hopper is where gateXAt says");
    // A breathing gate: its opening swings 60% to 120% of hw; a wisp passes if it is inside the opening of that tick.
    Gate b;
    b.cx = 0;
    b.hw = 1.5f;
    b.z = 12;
    b.breath = 4.0f;
    GT_CHECK_MSG(std::fabs(gateHwAt(b, 0.0f) - 0.9f) < 1e-4f && std::fabs(gateHwAt(b, 2.0f) - 1.8f) < 1e-4f,
                 "a breathing gate opens from 60% to 120% of its half-width");
    LevelDef bd = emptyLevel();
    bd.gates[bd.gateCount++] = b;
    w.init(bd, 1);
    place(w.w, 1.2f, 11.95f, 1);
    w.tick(in);
    const int narrow = w.w.n;
    while (w.tickCount < 120) w.tick(in);
    w.w.n = 0;
    place(w.w, 1.2f, 11.95f, 1);
    w.tick(in);
    GT_CHECK_MSG(narrow == 1 && w.w.n == 2, "a wisp 1.2 m out misses the narrow gate and passes the wide one");
    // A sliding wall is crossed where it is, not where it started.
    LevelDef sw = emptyLevel();
    Barrier wall;
    wall.x0 = -1.0f;
    wall.x1 = 1.0f;
    wall.z = 15.0f;
    wall.health = 40;
    wall.amp = 2.0f;
    wall.period = 4.0f;  // phase 0: shifted -2 at the start (it spans -3 .. -1)
    sw.barriers[sw.barrierCount++] = wall;
    w.init(sw, 1);
    place(w.w, 0.0f, 14.95f, 5);
    place(w.w, -2.0f, 14.95f, 5);
    w.tick(in);
    GT_CHECK_MSG(w.barrierHealth[0] == 35 && tideOf(w) == 5, "a sliding wall stops the wisp where it is, not where it was built");
    // A side door: its glooms appear on the rail at its height, step in sideways for a second, then walk down.
    LevelDef dd = emptyLevel();
    dd.targetT = 1000;
    dd.baseFlow = 2;
    Emitter door;
    door.side = 1;
    door.z = 20;
    door.x = kHW - 0.45f;
    dd.emitters[dd.emitterCount++] = door;
    w.init(dd, 1);
    while (w.g.n == 0 && w.tickCount < 600) w.tick(in);
    const bool entered = w.g.n == 1 && w.g.x[0] > 4.2f && std::fabs(w.g.z[0] - 20.0f) < 0.5f && (w.g.flags[0] & kStepping);
    const float z0 = w.g.z[0];
    for (int t = 0; t < 63; ++t) w.tick(in);
    const bool stepped = !(w.g.flags[0] & kStepping) && w.g.x[0] < 3.3f && w.g.z[0] > z0 - 0.2f;
    for (int t = 0; t < 60; ++t) w.tick(in);
    GT_CHECK_MSG(entered && stepped && w.g.z[0] < z0 - 1.0f, "a side door's gloom steps in, then walks down");
    // A runner walks at runnerSpeed.
    LevelDef rd = emptyLevel();
    w.init(rd, 1);
    place(w.g, 0.0f, 30.0f, 1);
    w.g.flags[0] = kRunner;
    for (int t = 0; t < 60; ++t) w.tick(in);
    GT_CHECK_MSG(std::fabs(w.g.z[0] - (30.0f - tune.runnerSpeed)) < 0.05f, "a runner walks at runnerSpeed");
    // Each plan's flow averaged over T is the base flow (VARIETY_DESIGN 5.1), for several T.
    bool flat = true;
    for (float T : {24.0f, 30.4f, 36.0f, 50.4f}) {
        for (int p = 0; p < kPlanCount; ++p) {
            if (p == kPlanBruteLine) continue;  // half here, half as brutes: in the world check below
            double sum = 0;
            const int n = 20000;
            for (int k = 0; k < n; ++k) sum += double(planFlow(p, (float(k) + 0.5f) * T / float(n), T));
            if (std::fabs(sum / n - 1.0) > 0.01) {
                flat = false;
                std::printf("plan %s over %.1f s averages %.3f\n", planName(p), double(T), sum / n);
            }
        }
    }
    GT_CHECK_MSG(flat, "every plan's shape averages 1 over T");
    bool totals = true;
    for (int p = 0; p < kPlanCount; ++p) {
        LevelDef pd = emptyLevel();
        pd.len = 44;
        pd.targetT = 40;
        pd.baseFlow = 12;
        pd.plan = u8(p);
        pd.emitters[pd.emitterCount++].x = -2.5f;
        pd.emitters[pd.emitterCount++].x = 2.5f;
        w.init(pd, 1);
        w.noDamage = true;  // the glooms reach the lantern; the run must not end
        while (w.time() < tune.emitterDelay + 40.0f) w.tick(in);
        const double got = double(w.gloomSpawned), want = 12.0 * 40.0;
        if (std::fabs(got / want - 1.0) > 0.06) {
            totals = false;
            std::printf("plan %s made %.0f power in T, want %.0f\n", planName(p), got, want);
        }
    }
    GT_CHECK_MSG(totals, "every plan sends the level's total enemy over T");
    // KINDLE: x5 counts as x6 and stops there; a wisp that passed before is not touched again.
    LevelDef kd = emptyLevel();
    Gate k5;
    k5.cx = -2.5f;
    k5.hw = 1.5f;
    k5.z = 12;
    k5.k = 5;
    Gate k6 = k5;
    k6.cx = 2.5f;
    k6.k = 6;
    kd.gates[kd.gateCount++] = k5;
    kd.gates[kd.gateCount++] = k6;
    kd.geodes[kd.geodeCount++] = geodeAt(0, 30, 1, kBoonKindle);
    w.init(kd, 1);
    w.tick(in);
    place(w.w, -2.5f, 12.05f, 1);  // already past the x5
    w.w.bits[0] = 1u;
    place(w.w, 0.0f, 30.0f, 1);  // opens KINDLE
    w.tick(in);
    const bool kindled = w.kindle == 1 && w.geodeState[0] == kGeodeOpened;
    w.w.n = 0;
    place(w.w, -2.5f, 12.05f, 1);
    w.w.bits[0] = 1u;  // passed the x5 before KINDLE
    place(w.w, -2.5f, 11.95f, 1);
    place(w.w, 2.5f, 11.95f, 1);
    w.tick(in);
    int left = 0, right = 0;
    for (int i = 0; i < w.w.n; ++i) (w.w.x[i] < 0 ? left : right) += 1;
    GT_CHECK_MSG(kindled && left == 1 + 6 && right == 6, "KINDLE: x5 gives x6, x6 stays x6, a wisp already through is not touched");
    // COMET: each lantern fires one comet, a wisp with 4 s of that lantern's fire.
    LevelDef cd = emptyLevel();
    cd.geodes[cd.geodeCount++] = geodeAt(0, 30, 1, kBoonComet);
    w.init(cd, 1);
    w.lanterns = 2;
    w.bright = 1;
    w.tick(in);
    place(w.w, 0.0f, 30.0f, 1);
    w.tick(in);
    int comets = 0;
    i32 cometPower = 0;
    for (int i = 0; i < w.w.n; ++i)
        if (w.w.flags[i] & kComet) ++comets, cometPower = w.w.power[i];
    GT_CHECK_MSG(comets == 2 && cometPower == i32(w.fireRate() * tune.cometSeconds * 2.0f), "COMET: a comet a lantern with 4 s of its fire");
    // HUSH: the emitters stop on the tick it opens and start again on the tick it ends.
    LevelDef sd = emptyLevel();
    sd.targetT = 1000;
    sd.baseFlow = 30;
    sd.emitters[sd.emitterCount++].x = 0;
    sd.geodes[sd.geodeCount++] = geodeAt(0, 10, 1, kBoonHush);
    w.init(sd, 1);
    while (w.time() < 3.0f) w.tick(in);
    place(w.w, 0.0f, 10.0f, 1);
    w.tick(in);
    const int until = w.hushUntil;
    const i64 at = w.gloomSpawned;
    while (w.tickCount < until) w.tick(in);
    const i64 quiet = w.gloomSpawned - at;
    for (int t = 0; t < 30; ++t) w.tick(in);
    GT_CHECK_MSG(until == w.tickCount - 30 && quiet == 0 && w.gloomSpawned - at >= 10, "HUSH: six quiet seconds, then the flow again");
}

// At the cap three lanterns still feed three columns: the shot's power joins a wisp of its own lantern.
void testCapThreeLanterns() {
    const float saved = tune.capW;
    tune.capW = 90;
    static World w;
    LevelDef d = emptyLevel();
    d.len = 44;
    w.init(d, 1);
    w.lanterns = 3;
    Input fire;
    fire.firing = true;
    for (int t = 0; t < 600; ++t) w.tick(fire);
    i64 col[3] = {};
    for (int i = 0; i < w.w.n; ++i) col[w.w.x[i] < -0.45f ? 2 : w.w.x[i] > 0.45f ? 1 : 0] += w.w.power[i];
    GT_CHECK_MSG(w.w.n <= 90 && col[0] > 0 && col[1] > 0 && col[2] > 0 && col[0] < 2 * col[1] && col[1] < 2 * col[0] && col[2] < 2 * col[0],
                 "at the cap three lanterns keep three columns");
    // Three lanterns at their fastest and heaviest through a kindled lane of x5 gates (each counts x6): still the cap.
    tune.capW = saved;
    LevelDef k = emptyLevel();
    k.len = 44;
    for (int r = 0; r < 4; ++r) {
        Gate g;
        g.cx = 0;
        g.hw = 1.8f;
        g.z = 8.0f + 7.0f * float(r);
        g.k = 5;
        g.row = u8(r);
        k.gates[k.gateCount++] = g;
    }
    w.init(k, 1);
    w.lanterns = 3;
    w.quicken = int(tune.quickenMax);
    w.bright = int(tune.brightMax);
    w.kindle = 1;
    int peak = 0;
    for (int t = 0; t < 900; ++t) {
        w.tick(fire);
        peak = maxv(peak, w.w.n);
    }
    GT_CHECK_MSG(peak <= int(tune.capW) && tideOf(w) > 10000, "three lanterns through a kindled lane stay under the cap");
}

void testNoAllocation() {
#if !defined(GT_SANITIZE)
    static World w;
    const LevelDef d = levelFor(5);
    w.init(d, 3);
    Bot bot;
    bot.init(BotKind::Greedy, 3);
    for (int t = 0; t < 60; ++t) w.tick(bot.think(w));  // the first init allocates; ticks must not
    const u64 before = allocationCount();
    for (int t = 0; t < 1200; ++t) w.tick(bot.think(w));
    GT_CHECK_MSG(allocationCount() == before, "no heap allocation in World::tick or Bot::think");
    // A geode level, with greedy weighing the geodes.
    w.init(levelFor(23), 3);
    bot.init(BotKind::Greedy, 3);
    const u64 before2 = allocationCount();
    for (int t = 0; t < 1800; ++t) w.tick(bot.think(w));
    GT_CHECK_MSG(allocationCount() == before2, "no heap allocation on a geode level");
    // Plans, side doors, runners, hoppers and breathing gates (levels 16, 28, 30, 38).
    const u64 before3 = allocationCount();
    for (int level : {16, 28, 30, 38}) {
        w.init(levelFor(level), 3);
        bot.init(BotKind::Greedy, 3);
        for (int t = 0; t < 900; ++t) w.tick(bot.think(w));
    }
    GT_CHECK_MSG(allocationCount() == before3, "no heap allocation with plans, doors, runners, hoppers, breaths");
#endif
}

void testGenerator() {
    int bad = 0;
    char why[96] = "";
    for (int s = 0; s < 1000; ++s) {
        const int index = 6 + (s * 37) % 595;
        if (isHandMade(index) || index == 43) continue;  // 43: see the recipes below
        LevelDef L = generate(index, hash64(u64(s)) & 0xFFFFFFFFull);
        sizeGeodes(L);
        if (!lintLevel(L, index, why, sizeof why)) {
            if (bad < 5) std::printf("generator: level %d seed %d: %s\n", index, s, why);
            ++bad;
        }
    }
    GT_CHECK_MSG(bad == 0, "generator invariants over 1,000 seeds");
    // Every recipe on 200 seeds, on levels from its first (VARIETY_PLAN V4 task 3).
    int badRecipes = 0;
    for (int rc = 0; rc < recipeCount(); ++rc) {
        int fails = 0;
        for (int s = 0; s < 200; ++s) {
            // Not on GEODE RAIN's introduction (43): its rain must fit beside the geodes a recipe placed, and where it
            // cannot, the bake's first pass takes the next seed (the calendar's own levels are linted above).
            const int index = maxv(6, kRecipes[rc].from) + (s * 13) % 250 + (maxv(6, kRecipes[rc].from) + (s * 13) % 250 == 43 ? 1 : 0);
            LevelDef L = generateWith(index, hash64(u64(s) * 977u + u64(rc)) & 0xFFFFFFFFull, rc);
            sizeGeodes(L);
            if (!lintLevel(L, index, why, sizeof why)) {
                if (fails < 2) std::printf("recipe %s: level %d seed %d: %s\n", recipeName(rc), index, s, why);
                ++fails;
            }
        }
        badRecipes += fails ? 1 : 0;
    }
    GT_CHECK_MSG(badRecipes == 0, "every recipe passes the generator's lints on 200 seeds");
    // Every set piece on 50 seeds (VARIETY_DESIGN 10.3).
    int badPieces = 0;
    for (int p = 0; p < setPieceCount(); ++p) {
        int fails = 0;
        for (int s = 0; s < 50; ++s) {
            const int index = kSetPieces[p].from;
            LevelDef L = generate(index, hash64(u64(s) * 131u + u64(p)) & 0xFFFFFFFFull);
            sizeGeodes(L);
            if (!lintLevel(L, index, why, sizeof why)) {
                if (fails < 2) std::printf("set piece %s: level %d seed %d: %s\n", kSetPieces[p].name, index, s, why);
                ++fails;
            }
        }
        badPieces += fails ? 1 : 0;
    }
    GT_CHECK_MSG(badPieces == 0, "every set piece passes the generator's lints on 50 seeds");
    // The calendar's levels as the table has them (VARIETY_PLAN V4's exit check: the lints for the 600 levels).
    int badTable = 0;
    for (int l = 1; l <= kLastBaked; ++l) {
        if (isHandMade(l)) continue;
        const LevelDef L = levelFor(l);
        if (!lintLevel(L, l, why, sizeof why)) {
            if (badTable < 5) std::printf("table: level %d: %s\n", l, why);
            ++badTable;
        }
    }
    GT_CHECK_MSG(badTable == 0, "every level of the baked table passes the generator's lints");
    // A festival's three stars ask for most of its rain (V5's repair rounds once rewrote them as one geode).
    int badStars = 0;
    for (int l = 2; l <= kLastBaked; ++l) {
        const LevelDef L = levelFor(l);
        if (L.kind == kFestival && L.goal2 < maxv(2, L.geodeCount / 2)) ++badStars;
    }
    GT_CHECK_MSG(badStars == 0, "every festival's three stars ask for at least half its geodes");
}

// After the last baked level (VARIETY_DESIGN 16): 601 plays 101 mirrored left to right in another world, 1,101 plays 101
// again; a replay's layout is its source's mirrored, and greedy's median time on each is the same within 5% (twenty runs
// each: with ten, run-to-run noise alone moved a set piece's median 6% one way and 1% over thirty).
void testReplays() {
    GT_CHECK(levelForPlay(600) == 600 && levelForPlay(601) == 101 && levelForPlay(1100) == 600 && levelForPlay(1101) == 101);
    GT_CHECK(!isReplay(600) && isReplay(601));
    for (int source : {101, 149, 213}) {
        const LevelDef a = levelFor(source), b = levelFor(source + 500);
        bool mirrored = a.gateCount == b.gateCount && a.barrierCount == b.barrierCount && a.geodeCount == b.geodeCount &&
                        a.emitterCount == b.emitterCount && a.bumperCount == b.bumperCount && a.snufferCount == b.snufferCount &&
                        a.spireHealth == b.spireHealth && a.baseFlow == b.baseFlow && a.world != b.world;
        for (int i = 0; mirrored && i < a.gateCount; ++i) mirrored = a.gates[i].cx == -b.gates[i].cx && a.gates[i].k == b.gates[i].k;
        for (int i = 0; mirrored && i < a.barrierCount; ++i) mirrored = a.barriers[i].x0 == -b.barriers[i].x1;
        for (int i = 0; mirrored && i < a.geodeCount; ++i) mirrored = a.geodes[i].x == -b.geodes[i].x && a.geodes[i].boon == b.geodes[i].boon;
        for (int i = 0; mirrored && i < a.emitterCount; ++i) mirrored = a.emitters[i].x == -b.emitters[i].x && a.emitters[i].side == -b.emitters[i].side;
        for (int i = 0; mirrored && i < a.bumperCount; ++i) mirrored = a.bumpers[i].x == -b.bumpers[i].x;
        for (int i = 0; mirrored && i < a.snufferCount; ++i) mirrored = a.snuffers[i].xa == -b.snuffers[i].xb;
        char what[96];
        std::snprintf(what, sizeof what, "level %d is level %d mirrored, in another world", source + 500, source);
        GT_CHECK_MSG(mirrored, what);
        RunOptions ro;
        ro.seconds = 3.0f * a.targetT;
        float ta[20], tb[20];
        for (int s = 0; s < 20; ++s) {
            const RunResult ra = runLevel(a, BotKind::Greedy, u64(s + 1), ro), rb = runLevel(b, BotKind::Greedy, u64(s + 1), ro);
            ta[s] = ra.result == Result::Win ? ra.time : 1e6f;
            tb[s] = rb.result == Result::Win ? rb.time : 1e6f;
        }
        std::sort(ta, ta + 20);
        std::sort(tb, tb + 20);
        const float ma = 0.5f * (ta[9] + ta[10]), mb = 0.5f * (tb[9] + tb[10]);
        // Within 15%: the layouts mirror exactly, but greedy does not (it breaks a tie between equal lines toward the left), so
        // a lane of patrols can send it up another line on the mirror (101 against 601: 20.0 s and 22.2 s, both inside line c).
        std::snprintf(what, sizeof what, "greedy's median time on %d (%.1f s) and on %d mirrored (%.1f s) within 15%%", source, double(ma),
                      source + 500, double(mb));
        GT_CHECK_MSG(ma < 1e5f && mb < 1e5f && std::fabs(mb - ma) <= 0.15f * ma, what);
    }
}

void testCameraFit() {
    struct Frame { int w, h; float dp; };
    const Frame frames[] = {{1080, 2400, 2.5f}, {540, 1200, 1.25f}, {1080, 2640, 2.625f}, {1080, 1920, 2.625f},
                            {600, 1024, 1.0f},  {1200, 1920, 1.5f}, {1080, 1440, 2.0f}};
    int bad = 0;
    for (const Frame& f : frames) {
        SafeArea safe;
        safe.top = 32 * f.dp;
        safe.bottom = 16 * f.dp;
        for (int len = 30; len <= 44; ++len) {
            Camera c;
            if (!c.fit(float(len), f.w, f.h, safe, f.dp)) {
                if (bad < 5) std::printf("camera: no fit for len %d in %dx%d\n", len, f.w, f.h);
                ++bad;
            }
        }
    }
    GT_CHECK_MSG(bad == 0, "the camera fits every lane length in every frame of the matrix");
}

void testPalettes() {
    const GamePalette& gp = gamePalette();
    int bad = 0;
    for (int k = 0; k < kPaletteCount + 10; ++k) {  // the ten worlds, then the alternatives drawn for 6-10
        const int world = k < kPaletteCount ? k + 1 : 6 + (k - kPaletteCount) / 2;
        const WorldPalette& w = k < kPaletteCount ? worldPalette(world) : worldPaletteAlt(world, 1 + (k - kPaletteCount) % 2);
        auto fail = [&](const char* what, float v) {
            if (bad < 10) std::printf("palette %s: %s (%.2f)\n", w.name, what, double(v));
            ++bad;
        };
        for (V3 lane : {w.laneFar, w.laneNear}) {
            if (luminance(lane) > 0.08f) fail("lane luminance above 0.08", luminance(lane));
            if (contrast(gp.wisp, lane) < 3.0f) fail("gold against the lane under 3:1", contrast(gp.wisp, lane));
            if (contrast(gp.good, lane) < 3.0f) fail("aqua against the lane under 3:1", contrast(gp.good, lane));
            if (contrast(gp.bad, lane) < 2.5f) fail("rose against the lane under 2.5:1", contrast(gp.bad, lane));
            if (contrast(gp.gloomEye, lane) < 2.5f) fail("gloom eyes against the lane under 2.5:1", contrast(gp.gloomEye, lane));
        }
        for (V3 b : w.backdrop)
            if (luminance(b) > 0.45f) fail("backdrop luminance above 0.45", luminance(b));
    }
    if (contrast(gp.gloomEye, gp.gloom) < 2.5f) ++bad, std::printf("palette: gloom eyes against the body under 2.5:1\n");
    GT_CHECK_MSG(bad == 0, "palette lint (GAME_DESIGN 9.2)");
    // Dressing: neighbouring worlds differ in sky feature and prop family; no two worlds share both.
    for (int a = 1; a <= kPaletteCount; ++a) {
        if (a < kPaletteCount) GT_CHECK(skyFeature(a) != skyFeature(a + 1) && propFamily(a) != propFamily(a + 1));
        for (int c = a + 1; c <= kPaletteCount; ++c) GT_CHECK(skyFeature(a) != skyFeature(c) || propFamily(a) != propFamily(c));
    }
}

void testFont() {
    const char* need = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ .,:!?'-+%/()";
    bool all = true;
    for (const char* p = need; *p; ++p) all = all && findGlyph(u8(*p)) != nullptr;
    all = all && findGlyph(0xD7) && findGlyph(0xF7);
    GT_CHECK_MSG(all, "the font has every glyph the game shows");
    GT_CHECK_MSG(std::fabs(UiBatch::textWidth("88", 10) - UiBatch::textWidth("11", 10)) < 1e-4f, "digits share one width");
    const char* times = "\xC3\x97" "2";
    GT_CHECK(nextCodepoint(times) == 0xD7);
}

}  // namespace

void runSimTests() {
    GT_CHECK_MSG(bakedGeneratorVersion() == kGeneratorVersion && bakedRulesVersion() == kRulesVersion,
                 "the level table was baked by this generator and these rules (bake again: CLAUDE.md)");
    testMultiply(4000);
    testMultiply(100);
    testDivide();
    testClashAndBarrier();
    testSnuffer();
    testSliders();
    testBursts();
    testKinds();
    testTwists();
    testBumper();
    testGeodes();
    testCapThreeLanterns();
    testLaneAndEnemy();
    testNoAllocation();
    testGenerator();
    testReplays();
    testCameraFit();
    testPalettes();
    testFont();
}

}  // namespace gt::test

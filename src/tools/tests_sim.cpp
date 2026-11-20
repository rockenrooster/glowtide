// The simulation's tests (TECH_SPEC 8.4): rules and their arithmetic, the generator's invariants, no allocation in a
// tick, the camera's fit across the aspect matrix, the palette lint and the font's glyph set.
#include "tests_sim.h"

#include <cmath>
#include <cstring>
#include <initializer_list>

#include "core/tuning.h"
#include "core/types.h"
#include "gfx/camera.h"
#include "gfx/palette.h"
#include "gfx/text.h"
#include "sim/bots.h"
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

// Time bursts double the enemy's flow for their duration; health bursts start when the spire falls to their share and
// bring their brutes; brutes have the level's brute power.
void testBursts() {
    static World w;
    LevelDef d = emptyLevel();
    d.targetT = 1000;  // no pressure ramp in this window
    d.baseFlow = 12;
    d.emitters[d.emitterCount++] = {0, 0, 5, 0};
    Burst b;
    b.t = 3.0f;
    b.mult = 2.0f;
    b.dur = 3.0f;
    d.bursts[d.burstCount++] = b;
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
    Burst hb;
    hb.t = -1;
    hb.healthFrac = 0.66f;
    hb.mult = 2;
    hb.dur = 4;
    hb.brutes = 2;
    e.bursts[e.burstCount++] = hb;
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
    tune.capW = saved;
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
#endif
}

void testGenerator() {
    int bad = 0;
    for (int s = 0; s < 1000; ++s) {
        const int index = 6 + (s * 37) % 295;
        LevelDef L = generate(index, hash64(u64(s)) & 0xFFFFFFFFull);
        sizeGeodes(L);
        char why[96] = "";
        bool ok = L.rowCount >= 2 && L.len >= 30 && L.len <= 44 && L.gateCount <= kMaxGates && L.emitterCount >= 1;
        if (!ok) std::snprintf(why, sizeof why, "rows %d len %.1f gates %d", L.rowCount, double(L.len), L.gateCount);
        if (ok && (L.rowZ[0] < 8 || L.rowZ[0] > 10)) ok = false, std::snprintf(why, sizeof why, "first row at %.1f", double(L.rowZ[0]));
        for (int r = 1; ok && r < L.rowCount; ++r) {
            const float gap = L.rowZ[r] - L.rowZ[r - 1];
            if (gap < 6 - 1e-3f || gap > 8 + 1e-3f) ok = false, std::snprintf(why, sizeof why, "rows %.1f apart", double(gap));
        }
        if (ok && L.rowZ[L.rowCount - 1] > L.len - 10 + 1e-3f) ok = false, std::snprintf(why, sizeof why, "last row too near the spire");
        for (int r = 0; ok && r < L.rowCount; ++r) {
            bool mul = false;
            for (int g = 0; g < L.gateCount; ++g) mul = mul || (L.gates[g].row == r && L.gates[g].kind == kGateMul);
            if (!mul) ok = false, std::snprintf(why, sizeof why, "row %d has no multiply gate", r);
        }
        // Gates of a row never overlap (crossing sliders cross on purpose); openings stay inside the lane.
        for (int a = 0; ok && a < L.gateCount; ++a) {
            const Gate& A = L.gates[a];
            const float reach = A.slides ? A.amp : 0.0f;
            if (std::fabs(A.cx) + reach + A.hw > kHW + 1e-3f) ok = false, std::snprintf(why, sizeof why, "gate %d leaves the lane", a);
            if (A.hw < 1.2f - 1e-3f || A.hw > 1.8f + 1e-3f) ok = false, std::snprintf(why, sizeof why, "gate %d half-width %.2f", a, double(A.hw));
            for (int b = a + 1; ok && b < L.gateCount; ++b) {
                const Gate& B = L.gates[b];
                if (std::fabs(A.z - B.z) > 0.01f || A.slides || B.slides) continue;
                if (std::fabs(A.cx - B.cx) < A.hw + B.hw - 1e-3f) ok = false, std::snprintf(why, sizeof why, "gates %d and %d overlap", a, b);
            }
        }
        // Gaps: a bumper to the walls, to other bumpers and to gate posts at least 1.2 m.
        for (int a = 0; ok && a < L.bumperCount; ++a) {
            const Bumper& A = L.bumpers[a];
            if (kHW - std::fabs(A.x) - A.r < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumper %d near a wall", a);
            for (int b = a + 1; ok && b < L.bumperCount; ++b) {
                const Bumper& B = L.bumpers[b];
                const float d = std::sqrt((A.x - B.x) * (A.x - B.x) + (A.z - B.z) * (A.z - B.z)) - A.r - B.r;
                if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumpers %d and %d %.2f m apart", a, b, double(d));
            }
            for (int g = 0; ok && g < L.gateCount; ++g) {
                const Gate& G = L.gates[g];
                if (G.slides) continue;
                for (float s2 : {-1.0f, 1.0f}) {
                    const float px = G.cx + s2 * G.hw;
                    const float d = std::sqrt((A.x - px) * (A.x - px) + (A.z - G.z) * (A.z - G.z)) - A.r - 0.18f;
                    if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumper %d %.2f m from a post", a, double(d));
                }
            }
        }
        // Geodes (VARIETY_DESIGN 9.3): how many, when, how far apart, at most three kinds, the first lasting up to 60, the
        // budget, and inside the lane where a stream can reach them.
        const int maxGeodes = index <= 20 ? 2 : index <= 60 ? 3 : 4;
        if (ok && (L.geodeCount > maxv(maxGeodes, L.boss ? 2 : 0) || (index >= 6 && L.geodeCount == 0)))
            ok = false, std::snprintf(why, sizeof why, "%d geodes", L.geodeCount);
        float worth = 1;
        int lanterns = 1, quicken = 0, bright = 0, kinds = 0;
        bool seen[kBoonCount] = {};
        for (int g = 0; ok && g < L.geodeCount; ++g) {
            const Geode& G = L.geodes[g];
            const bool lasting = G.boon != kBoonWard;
            if (g == 0 && (G.t < 0.08f * L.targetT - 1e-3f || G.t > 0.25f * L.targetT + 1e-3f))
                ok = false, std::snprintf(why, sizeof why, "first geode at %.1f s of %.0f", double(G.t), double(L.targetT));
            if (g > 0 && G.t < L.geodes[g - 1].t + 5.0f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "geodes under 5 s apart");
            if (lasting && G.t > 0.75f * L.targetT + 1e-3f) ok = false, std::snprintf(why, sizeof why, "a lasting boon after 75%% of T");
            if (g == 0 && index <= 60 && !lasting) ok = false, std::snprintf(why, sizeof why, "the first geode is a WARD");
            if (std::fabs(G.x) + G.amp > 3.9f + 1e-3f) ok = false, std::snprintf(why, sizeof why, "geode %d outside the reachable lane", g);
            if (G.hp <= 0 || (G.shieldSeconds > 0 && G.shield <= 0)) ok = false, std::snprintf(why, sizeof why, "geode %d has no health", g);
            if (!seen[G.boon]) seen[G.boon] = true, ++kinds;
            worth *= 1.0f + boonGain(G.boon, lanterns, quicken, bright);
            lanterns += G.boon == kBoonLantern;
            quicken += G.boon == kBoonQuicken;
            bright += G.boon == kBoonBright;
        }
        if (ok && kinds > 3) ok = false, std::snprintf(why, sizeof why, "%d kinds of boon", kinds);
        if (ok && worth > (index <= 20 ? 2.0f : index <= 60 ? 2.5f : 3.0f) + 1e-3f)
            ok = false, std::snprintf(why, sizeof why, "boons worth x%.2f over the budget", double(worth));
        if (!ok) {
            if (bad < 5) std::printf("generator: level %d seed %d: %s\n", index, s, why);
            ++bad;
        }
    }
    GT_CHECK_MSG(bad == 0, "generator invariants over 1,000 seeds");
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
    for (int world = 1; world <= 5; ++world) {
        const WorldPalette& w = worldPalette(world);
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
    testBumper();
    testGeodes();
    testCapThreeLanterns();
    testNoAllocation();
    testGenerator();
    testCameraFit();
    testPalettes();
    testFont();
}

}  // namespace gt::test

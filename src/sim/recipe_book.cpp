// The recipe catalogue (VARIETY_DESIGN 9.2): ways of laying out a lane, each with one idea a player could say in a
// sentence. A recipe lays out the lane: rows, gates, walls, bumpers, snuffers, and where its idea needs them the
// emitters, the plan or some geodes; generate() (recipes.cpp) does the rest for what it left empty. Every recipe keeps
// GAME_DESIGN 7.3's rules (tests_sim.cpp checks them on 200 seeds of each): rows 6 to 8 m apart from z 8 to 10, the last
// at most len - 10; a multiply gate in every row; gates of half-width 1.2 to 1.8 inside the lane and not overlapping;
// bumpers 1.2 m clear of the rails, of each other and of gate posts.
#include <cmath>
#include <initializer_list>

#include "core/rng.h"
#include "sim/level_build.h"

namespace gt::sim {

namespace {

constexpr u32 bit(int kind) { return 1u << kind; }
constexpr u32 kFight = bit(kSiege) | bit(kBoss);  // the recipes of a plain contest

// Rows for the length class (VARIETY_DESIGN 9.1): Sprint 2 rows on 30 m, Standard 3 or 4 on 36 to 40, Long 4 or 5 on 44.
// `wide` asks for 7.5 m between rows (room for bumpers between them).
int layRows(LevelDef& L, Rng& r, const RecipeCtx& c, float* z, bool wide = false) {
    int n = c.lengthClass == kSprint ? 2 : c.lengthClass == kLong ? r.rangei(4, 5) : r.rangei(3, 4);
    if (c.rows > 0) n = c.rows;
    float z0 = r.range(8.0f, 9.5f), gap;
    if (c.lengthClass == kLong) gap = n >= 5 ? r.range(6.0f, 6.3f) : r.range(7.3f, 7.8f);
    else if (c.lengthClass == kSprint) gap = r.range(6.5f, 7.5f);
    else gap = n >= 4 ? r.range(6.2f, 6.8f) : r.range(6.6f, 7.6f);
    if (wide) gap = maxv(gap, 7.5f), n = minv(n, c.lengthClass == kLong ? 4 : 3);
    if (n > 1) gap = minv(gap, ((c.lengthClass == kLong ? 33.8f : c.lengthClass == kSprint ? 19.8f : 29.8f) - z0) / float(n - 1));
    gap = maxv(gap, 6.0f);
    for (int k = 0; k < n; ++k) {
        z[k] = z0 + gap * float(k);
        addRow(L, z[k], kPatHand);
    }
    const float last = z[n - 1];
    L.len = c.lengthClass == kSprint ? 30.0f : c.lengthClass == kLong ? 44.0f : clampv(last + r.range(10.5f, 11.5f), 36.0f, 40.0f);
    L.len = maxv(L.len, last + 10.0f);
    return n;
}

int factor(Rng& r, int lo, int hi) { return r.rangei(lo, maxv(lo, hi)); }

// The stream a player has after the gates below z on the column x (for a wall's health).
float streamAt(const LevelDef& L, float x, float z) { return tune.fireRate * columnMult(L, x, z); }

Barrier wall(float x0, float x1, float z, i32 health) {
    Barrier b;
    b.x0 = maxv(-kHW + 0.1f, x0);
    b.x1 = minv(kHW - 0.1f, x1);
    b.z = z;
    b.health = maxv<i32>(10, twoSignificant(float(health)));
    return b;
}

void addWall(LevelDef& L, const Barrier& b) {
    if (L.barrierCount < kMaxBarriers) L.barriers[L.barrierCount++] = b;
}

void addBumper(LevelDef& L, float x, float z, float r) {
    if (L.bumperCount < kMaxBumpers) L.bumpers[L.bumperCount++] = {x, z, r};
}

Geode geode(float x, float z0, float t, u8 boon, float hpSeconds) {
    Geode g;
    g.x = x;
    g.z0 = z0;
    g.t = t;
    g.boon = boon;
    g.hpSeconds = hpSeconds;
    g.speed = 1.5f;
    return g;
}

void addGeode(LevelDef& L, const Geode& g) {
    if (L.geodeCount < kMaxGeodes) L.geodes[L.geodeCount++] = g;
}

// A lasting boon the level may offer (its first levels, VARIETY_DESIGN 3), drawn by the seed.
u8 lastingBoon(Rng& r, int index) {
    u8 pool[4];
    int n = 0;
    pool[n++] = kBoonLantern;
    if (index >= 7) pool[n++] = kBoonQuicken;
    if (index >= 12) pool[n++] = kBoonBright;
    if (index >= 36) pool[n++] = kBoonKindle;
    return pool[r.rangei(0, n - 1)];
}

// Boons for n geodes a recipe places (VARIETY_DESIGN 9.3): the lasting ones' worth inside the budget, at most three kinds,
// the first one lasting; an instant (WARD from 23, COMET from 52) where nothing lasting fits.
void budgetBoons(Rng& r, int index, int n, u8* out) {
    const float budget = index <= 20 ? 2.0f : index <= 60 ? 2.5f : 3.0f;
    int lanterns = 1, quicken = 0, bright = 0, kinds = 0;
    bool seen[kBoonCount] = {};
    float worth = 1;
    for (int k = 0; k < n; ++k) {
        int pick = -1;
        for (int tries = 0; tries < 12 && pick < 0; ++tries) {
            const u8 b = tries < 8 ? lastingBoon(r, index) : u8(kBoonQuicken);
            if (b == kBoonKindle || (!seen[b] && kinds >= 3)) continue;
            const float g = boonGain(b, lanterns, quicken, bright);
            if (g > 0 && worth * (1.0f + g) <= budget + 1e-3f) {
                pick = b;
                worth *= 1.0f + g;
                lanterns += b == kBoonLantern;
                quicken += b == kBoonQuicken;
                bright += b == kBoonBright;
            }
        }
        if (pick < 0) pick = index >= 52 && (seen[kBoonComet] || kinds < 3) ? kBoonComet : kBoonWard;
        if (k == 0 && (pick == kBoonWard || pick == kBoonComet)) pick = kBoonQuicken;
        if (!seen[pick]) seen[pick] = true, ++kinds;
        out[k] = u8(pick);
    }
}

// ---- 1. Terraces: rows of gates, pick the best of each (GAME_DESIGN 7.3's patterns) ----
void terraces(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float* wt = c.patternWeight;
    const int strongest = c.strongest;
    float stream = tune.fireRate;
    int prev = -1;
    for (int row = 0; row < rows; ++row) {
        int p = 0;
        for (int tries = 0; tries < 20; ++tries) {
            float total = 0;
            for (int k = 0; k < kPatCount; ++k) total += wt[k];
            float pick = r.range(0, total);
            p = 0;
            while (p < kPatCount - 1 && pick >= wt[p]) pick -= wt[p++];
            if (row == 0 && p != kPatOpen && p != kPatChoice) continue;
            const bool hard = p == kPatTrap || p == kPatPatrol;
            const bool prevHard = prev == kPatTrap || prev == kPatPatrol;
            if (hard && prevHard) continue;
            if (p == kPatWall && row == rows - 1) continue;  // its prize stands 2 m behind the row
            break;
        }
        if (row == 0 && p != kPatOpen && p != kPatChoice) p = kPatChoice;
        if (p == kPatWall && row == rows - 1) p = kPatChoice;
        L.rowPattern[row] = u8(p);
        const float s = r.f01() < 0.5f ? -1.0f : 1.0f, zz = z[row];
        const int plain = r.rangei(2, strongest);
        const int special = minv(5, plain + 1);
        int best = plain;
        switch (p) {
            case kPatOpen: addGate(L, mulGate(r.range(-2.9f, 2.9f), 1.8f, zz, plain, row)); break;
            case kPatChoice: {
                const int b = r.rangei(3, maxv(3, strongest));
                const int a = r.rangei(2, b - 1);
                addGate(L, mulGate(s * r.range(2.5f, 2.9f), 1.8f, zz, a, row));
                addGate(L, mulGate(-s * r.range(2.3f, 2.9f), 1.2f, zz, b, row));
                best = b;
                break;
            }
            case kPatTrap:
                addGate(L, mulGate(s * 2.6f, 1.5f, zz, plain, row));
                addGate(L, divGate(-s * 2.6f, 1.5f, zz, c.index >= 25 && r.f01() < 0.5f ? 3 : 2, row));
                break;
            case kPatSlider: {
                Gate gs = mulGate(r.range(-0.5f, 0.5f), 1.6f, zz, special, row);
                gs.slides = kGateSlide;
                gs.amp = minv(kHW - gs.hw - 0.3f - std::fabs(gs.cx), r.range(1.8f, 3.0f));
                gs.period = r.range(3, 6);
                gs.phase = r.f01();
                addGate(L, gs);
                best = special;
                break;
            }
            case kPatWall: {
                const float cx = s * 2.6f;
                addGate(L, mulGate(cx, 1.5f, zz + 2.0f, special, row));
                addGate(L, mulGate(-s * 2.6f, 1.5f, zz, r.rangei(2, maxv(2, strongest - 1)), row));
                Barrier b = wall(cx - 1.6f, cx + 1.6f, zz, i32(1.5f * stream));
                if (c.index >= 46 && r.f01() < 0.4f) {  // a sliding wall (VARIETY_DESIGN 4 #3)
                    b.x0 = cx - 1.2f;
                    b.x1 = cx + 1.2f;
                    b.amp = minv(kHW - 0.1f - std::fabs(cx) - 1.2f, r.range(0.8f, 1.1f));
                    b.period = r.range(4.0f, 6.0f);
                    b.phase = r.f01();
                }
                addWall(L, b);
                best = special;
                break;
            }
            case kPatPillars: {
                const int a = r.rangei(2, strongest), b = r.rangei(2, strongest);
                addGate(L, mulGate(-3.3f, 1.3f, zz, a, row));
                addGate(L, mulGate(3.3f, 1.3f, zz, b, row));
                addBumper(L, -1.25f, zz - 1.8f, 0.55f);
                addBumper(L, 1.25f, zz - 1.8f, 0.55f);
                best = maxv(a, b);
                break;
            }
            case kPatPatrol: {
                const float cx = r.range(-2.6f, 2.6f);
                addGate(L, mulGate(cx, 1.5f, zz, special, row));
                if (std::fabs(cx) > 1.0f) addGate(L, mulGate(cx > 0 ? -3.2f : 3.2f, 1.5f, zz, 2, row));
                Snuffer sn;
                sn.xa = maxv(-kLauncherRange, cx - 2.2f);
                sn.xb = minv(kLauncherRange, cx + 2.2f);
                sn.z = zz - 2.0f;
                sn.speed = r.range(2, 4);
                sn.phase = r.f01();
                if (L.snufferCount < kMaxSnuffers) L.snuffers[L.snufferCount++] = sn;
                best = special;
                break;
            }
            case kPatHopper: {
                Gate h = mulGate(r.range(-0.4f, 0.4f), 1.5f, zz, special, row);
                h.slides = kGateHop;
                h.amp = minv(kHW - h.hw - 0.3f - std::fabs(h.cx), r.range(1.8f, 2.6f));
                h.period = r.range(1.5f, 2.5f);
                h.phase = r.f01();
                addGate(L, h);
                best = special;
                break;
            }
            case kPatBreathing: {
                const float cx = s * r.range(1.4f, 2.0f);
                Gate b = mulGate(cx, 1.6f, zz, special, row);
                b.breath = r.range(3.0f, 5.0f);
                b.phase = r.f01();
                addGate(L, b);
                addGate(L, mulGate(-s * 3.4f, 1.2f, zz, plain, row));
                best = special;
                break;
            }
            default: {  // Crossing sliders, 1.6 m apart in z so they pass each other instead of through each other
                const float period = r.range(3.5f, 5.5f), phase = r.f01();
                Gate a = mulGate(0, 1.4f, zz + 0.8f, plain, row);
                a.slides = kGateSlide;
                a.amp = 2.6f;
                a.period = period;
                a.phase = phase;
                Gate d = divGate(0, 1.4f, zz - 0.8f, 2, row);
                d.slides = kGateSlide;
                d.amp = 2.6f;
                d.period = period;
                d.phase = phase + 0.5f;
                addGate(L, a);
                addGate(L, d);
                break;
            }
        }
        stream *= float(best);
        prev = p;
    }
}

// ---- 2. Funnel: aim finer as the stream climbs ----
void funnel(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    static const float hw[5] = {1.8f, 1.5f, 1.3f, 1.2f, 1.2f};
    float x = r.range(-2.4f, 2.4f);
    for (int k = 0; k < rows; ++k) {
        if (k) x = clampv(x + r.range(-0.6f, 0.6f), -2.6f, 2.6f);
        const int f = k == rows - 1 ? minv(5, c.strongest + 1) : minv(c.strongest, 2 + k / 2);
        addGate(L, mulGate(x, hw[minv(k, 4)], z[k], f, k));
        if (k == rows - 1)
            for (float s : {-1.0f, 1.0f}) {
                const float dx = x + s * (hw[minv(k, 4)] + 1.3f);
                if (std::fabs(dx) + 1.2f <= kHW - 0.05f) addGate(L, divGate(dx, 1.2f, z[k], 2, k));
            }
    }
}

// ---- 3. Twin rivers: commit to a side, cross when you must ----
void twinRivers(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx cc = c;
    cc.rows = c.lengthClass == kSprint ? 2 : 3;
    float z[kMaxRows];
    const int rows = layRows(L, r, cc, z, true);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f;  // the stronger side
    const int strong[3] = {3, factor(r, 3, c.strongest), factor(r, 3, minv(5, c.strongest + 1))};
    const int weak[3] = {2, factor(r, 2, 3), 2};
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(s * 2.9f, 1.3f, z[k], strong[k], k));
        addGate(L, mulGate(-s * 2.9f, 1.3f, z[k], weak[k], k));
        if (k + 1 < rows) addBumper(L, 0, 0.5f * (z[k] + z[k + 1]), 0.55f);  // the middle, between the rows
    }
    // The stronger side has a wall in front of its middle gate.
    addWall(L, wall(s * 2.9f - 1.6f, s * 2.9f + 1.6f, z[1] - 1.5f, i32(1.5f * streamAt(L, s * 2.9f, z[1]))));
    // One emitter a side, unequal shares: more of the enemy comes down the strong side.
    addEmitter(L, s * 2.4f);
    addEmitter(L, -s * 2.4f);
    L.emitters[0].share = 0.6f;
    L.emitters[1].share = 0.4f;
}

// ---- 4. Checker: one clean column, one rich and dirty ----
void checker(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx cc = c;
    cc.rows = c.lengthClass == kSprint ? 2 : r.rangei(3, 4);
    float z[kMaxRows];
    const int rows = layRows(L, r, cc, z);
    const float xs[3] = {-3.2f, 0.0f, 3.2f};
    int order[3] = {0, 1, 2};
    for (int i = 2; i > 0; --i) {
        const int j = r.rangei(0, i);
        const int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    const int clean = order[0], dirty = order[1], mixed = order[2];
    const int divRow = r.rangei(0, rows - 1);
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(xs[clean], 1.2f, z[k], 2, k));
        if (k == divRow) addGate(L, divGate(xs[dirty], 1.2f, z[k], 2, k));
        else addGate(L, mulGate(xs[dirty], 1.2f, z[k], factor(r, 3, c.strongest), k));
        if (r.f01() < 0.4f) addGate(L, (r.f01() < 0.5f ? divGate : mulGate)(xs[mixed], 1.2f, z[k], 2, k));
    }
}

// ---- 5. The vault: pay to open the jackpot ----
void vault(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx cc = c;
    cc.rows = 3;
    cc.lengthClass = c.lengthClass == kSprint ? kStandard : c.lengthClass;
    float z[kMaxRows];
    const int rows = layRows(L, r, cc, z);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f;  // the safe side
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(s * 2.6f, 1.6f, z[k], 2, k));  // the side line of x2s
        if (k < rows - 1) addGate(L, mulGate(-s * 2.6f, 1.4f, z[k], k == 0 ? 3 : 2, k));
    }
    // The jackpot: a x5 at the top row behind a wall worth 4 s of the stream that reaches it.
    const float vx = -s * 2.4f, top = z[rows - 1];
    addGate(L, mulGate(vx, 1.4f, top, 5, rows - 1));
    addWall(L, wall(vx - 1.6f, vx + 1.6f, top - 1.5f, i32(4.0f * streamAt(L, vx, top))));
    // A geode behind a second, smaller wall, anchored at the safe side until 80% of the level's time.
    const float gz = z[0] + 3.5f;
    u8 vb[1];
    budgetBoons(r, c.index, 1, vb);
    Geode g = geode(s * 3.4f, gz, 0.1f * L.targetT, vb[0], 1.5f);
    g.leaveAt = 0.8f * L.targetT;
    g.speed = 1.6f;
    addGeode(L, g);
    addWall(L, wall(s * 3.4f - 1.3f, s * 3.4f + 1.3f, gz - 1.5f, i32(1.5f * streamAt(L, s * 3.4f, gz))));
}

// ---- 6. Toll road: find the thin wall ----
void tollRoad(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    addGate(L, mulGate(0, 1.8f, z[0], 2, 0));
    for (int k = 1; k < rows; ++k) {
        addGate(L, mulGate(-2.6f, 1.5f, z[k], factor(r, 2, c.strongest), k));
        addGate(L, mulGate(2.6f, 1.5f, z[k], factor(r, 2, c.strongest), k));
    }
    // Two walls across the whole lane, each in three spans with health 1 : 2 : 4 shuffled.
    const float edges[4] = {-kHW + 0.1f, -1.63f, 1.63f, kHW - 0.1f};
    for (int w = 0; w < 2 && w + 1 < rows; ++w) {
        const float wz = z[w] + 3.2f;
        int mult[3] = {1, 2, 4};
        for (int i = 2; i > 0; --i) {
            const int j = r.rangei(0, i);
            const int t = mult[i];
            mult[i] = mult[j];
            mult[j] = t;
        }
        const float base = 0.8f * streamAt(L, 0, wz);
        for (int sp = 0; sp < 3; ++sp) addWall(L, wall(edges[sp] + 0.02f, edges[sp + 1] - 0.02f, wz, i32(base * float(mult[sp]))));
    }
}

// ---- 7. Pendulum: ride the wave ----
void pendulum(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float period = r.range(3.5f, 5.0f), amp = r.range(1.0f, 1.3f);
    for (int k = 0; k < rows; ++k) {
        const float phase = float(k) / 3.0f;
        const bool two = r.f01() < 0.6f;
        if (two) {
            Gate a = mulGate(-2.2f, 1.4f, z[k], factor(r, 2, c.strongest), k);
            Gate b = mulGate(2.2f, 1.4f, z[k], factor(r, 2, c.strongest), k);
            for (Gate* g : {&a, &b}) {
                g->slides = kGateSlide;
                g->amp = amp;
                g->period = period;
                g->phase = phase;
                addGate(L, *g);
            }
        } else {
            Gate g = mulGate(0, 1.6f, z[k], factor(r, 3, minv(5, c.strongest + 1)), k);
            g.slides = kGateSlide;
            g.amp = minv(2.8f, kHW - 1.6f - 0.3f);
            g.period = period;
            g.phase = phase;
            addGate(L, g);
        }
    }
}

// ---- 8. Stagger: odd rows or even rows (a Long lane) ----
void stagger(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx lc = c;
    lc.lengthClass = kLong;
    lc.rows = 5;
    float z[kMaxRows];
    const int rows = layRows(L, r, lc, z);
    L.lengthClass = kLong;
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f, oddX = r.range(1.4f, 3.0f), evenX = r.range(1.4f, 3.0f);
    // Odd rows on one side, even rows on the other, with different products.
    for (int k = 0; k < rows; ++k) {
        const bool odd = k % 2 == 0;
        addGate(L, mulGate((odd ? s * oddX : -s * evenX), 1.2f, z[k], odd ? factor(r, 2, 3) : factor(r, 3, c.strongest), k));
    }
}

// ---- 9. Islands: thread the channels ----
void islands(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z, true);
    const bool middle = r.f01() < 0.5f;  // two islands and the middle channel, or one island in the middle and two channels
    const float rich = r.f01() < 0.5f ? -1.0f : 1.0f;
    for (int k = 0; k < rows; ++k) {
        if (middle) {
            addGate(L, mulGate(0, 1.2f, z[k], factor(r, 2, c.strongest), k));
            if (r.f01() < 0.7f) addGate(L, mulGate(r.f01() < 0.5f ? -3.7f : 3.7f, 1.2f, z[k], factor(r, 2, 3), k));
        } else {
            addGate(L, mulGate(rich * 3.1f, 1.3f, z[k], factor(r, 3, c.strongest), k));
            addGate(L, mulGate(-rich * 3.1f, 1.3f, z[k], 2, k));
        }
        if (k + 1 < rows) {  // islands between the rows: three bumpers each
            const float zm = 0.5f * (z[k] + z[k + 1]);
            if (middle) {
                for (float s : {-1.0f, 1.0f}) {
                    addBumper(L, s * 2.3f - 1.05f, zm - 0.6f, 0.45f);
                    addBumper(L, s * 2.3f + 1.05f, zm - 0.6f, 0.45f);
                    addBumper(L, s * 2.3f, zm + 1.3f, 0.45f);
                }
            } else {
                addBumper(L, -1.05f, zm - 0.6f, 0.45f);
                addBumper(L, 1.05f, zm - 0.6f, 0.45f);
                addBumper(L, 0, zm + 1.3f, 0.45f);
            }
        }
    }
}

// ---- 10. Crossfire: they come from the sides ----
void crossfire(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float col = r.range(-2.4f, 2.4f);
    for (int k = 0; k < rows; ++k)
        addGate(L, mulGate(clampv(col + r.range(-0.4f, 0.4f), -3.1f, 3.1f), 1.7f, z[k], k == 0 ? 2 : factor(r, 2, c.strongest), k));
    L.plan = kPlanFlank;
    addEmitter(L, -2.0f);
    addEmitter(L, 2.0f);
    L.emitters[0].share = L.emitters[1].share = 1.0f / 3.0f;
    const int side = r.f01() < 0.5f ? -1 : 1;
    float d0 = maxv(14.0f, z[0] + 5.0f), d1 = maxv(d0 + 6.0f, L.len - 9.0f);
    for (float* d : {&d0, &d1})
        for (int i = 0; i < rows; ++i)
            if (std::fabs(*d - z[i]) < 1.5f) *d = z[i] + 1.6f;
    d1 = minv(d1, L.len - 8.0f);
    d0 = minv(d0, L.len - 8.0f);
    addDoor(L, side, d0, 1.0f / 6.0f);
    addDoor(L, -side, d1, 1.0f / 6.0f);
    u8 b[1];
    budgetBoons(r, c.index, 1, b);
    Geode lasting = geode(0, 0, 0.15f * L.targetT, b[0], 2.0f);
    placeDetour(L, lasting, bestLineX(L), r.f01() < 0.5f, 0.4f);  // off the line and outside the stream's spill
    addGeode(L, lasting);
    if (c.index >= 23) addGeode(L, geode(0, 0.5f * L.len, 0.45f * L.targetT, kBoonWard, 1.5f));  // a WARD mid-level
}

// ---- 11. The breach: fight for the rich side or farm the quiet one ----
void breach(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f;  // the rich side
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(s * 2.6f, 1.5f, z[k], factor(r, 3, minv(5, c.strongest + 1)), k));
        addGate(L, mulGate(-s * 2.6f, 1.5f, z[k], 2, k));
    }
    addEmitter(L, s * 1.8f);
    addEmitter(L, s * 3.2f);
    addEmitter(L, -s * 2.6f);
    L.emitters[0].share = L.emitters[1].share = 0.375f;  // every emitter on the rich side at 1.5 times its share
    L.emitters[2].share = 0.25f;
}

// ---- 12. Caravan: three gifts in a row, on the wrong side ----
void caravan(LevelDef& L, Rng& r, const RecipeCtx& c) {
    terraces(L, r, c);
    // Three geodes in one column on the side away from the best line.
    const float x = bestLineX(L) < 0 ? 3.4f : -3.4f;
    const float t = r.range(0.1f, 0.2f) * L.targetT;
    u8 b[3];
    budgetBoons(r, c.index, 3, b);
    for (int k = 0; k < 3; ++k) {
        Geode g = geode(x, L.len - 3.0f - 6.0f * float(k), t, b[k], 1.5f);
        if (c.index >= 96 && k == 1) g.shieldSeconds = 2.5f, g.hpSeconds = 1.0f;  // with shields from level 96
        addGeode(L, g);
    }
}

// ---- 13. Crossroads: this boon or that one ----
void crossroads(LevelDef& L, Rng& r, const RecipeCtx& c) {
    terraces(L, r, c);
    u8 firsts[2];
    budgetBoons(r, c.index, 2, firsts);
    for (int p = 0; p < 2; ++p) {
        const float best = bestLineX(L), t = (p == 0 ? 0.2f : 0.55f) * L.targetT;
        // The pair's middle 2.4 m off the best line on the side with room, low on the lane where the stream is still
        // narrow: a stream passing by does not open one of them by accident (VARIETY_DESIGN 9.3: the far one a detour).
        const bool right = best + 2.4f <= 2.4f && (best - 2.4f < -2.4f || r.f01() < 0.5f);
        const float cx = clampv(best + (right ? 2.4f : -2.4f), -2.4f, 2.4f), z0 = L.rowZ[0] + 2.5f + 3.0f * float(p);
        const u8 a = firsts[p];
        u8 b = lastingBoon(r, c.index);
        for (int tries = 0; tries < 8 && (b == a || b == kBoonKindle); ++tries) b = lastingBoon(r, c.index);
        if (b == a || b == kBoonKindle) b = a == kBoonLantern ? kBoonQuicken : kBoonLantern;
        if (p == 1 && b != firsts[0] && b != a && a != firsts[0]) b = firsts[0];  // at most three kinds of boon
        Geode ga = geode(cx - 1.5f, z0, t, a, 2.0f), gb = geode(cx + 1.5f, z0, t, b, 2.0f);
        ga.speed = gb.speed = 1.0f;
        ga.group = gb.group = u8(p + 1);
        addGeode(L, ga);
        addGeode(L, gb);
    }
}

// ---- 14. Treasure walls: something locked on each side ----
void treasureWalls(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float col = r.range(-1.5f, 1.5f);
    for (int k = 0; k < rows; ++k) addGate(L, mulGate(col + r.range(-0.3f, 0.3f), 1.4f, z[k], k == 0 ? 2 : factor(r, 2, c.strongest), k));
    float vz = 20.0f;
    for (int i = 0; i < rows; ++i)
        if (std::fabs(vz - z[i]) < 2.0f) vz = z[i] + 2.5f;
    u8 tb[2];
    budgetBoons(r, c.index, 2, tb);
    for (float s : {-1.0f, 1.0f}) {
        Geode g = geode(s * 3.4f, vz, s < 0 ? 0.08f * L.targetT : 0.08f * L.targetT + 5.0f, tb[s < 0 ? 0 : 1], 1.2f);
        g.leaveAt = 0.8f * L.targetT;
        addGeode(L, g);
        addWall(L, wall(s * 3.4f - 1.3f, s * 3.4f + 1.3f, vz - 1.6f, i32(2.0f * streamAt(L, s * 3.4f, vz))));
    }
}

// ---- 15. Hopscotch: the gate will not stay ----
void hopscotch(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    for (int k = 0; k < rows; ++k) {
        Gate h = mulGate(r.range(-0.4f, 0.4f), 1.5f, z[k], factor(r, 2, minv(5, c.strongest + 1)), k);
        h.slides = kGateHop;
        h.amp = minv(kHW - h.hw - 0.3f - std::fabs(h.cx), r.range(1.8f, 2.4f));
        h.period = r.range(1.5f, 2.5f);
        h.phase = r.f01();
        addGate(L, h);
    }
}

// ---- 16. The narrows: a chokepoint ----
void narrows(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const int nr = rows >= 3 ? 1 : 0;  // the narrows row
    const float gx = r.range(-2.4f, 2.4f);
    for (int k = 0; k < rows; ++k) {
        if (k == nr) {
            addGate(L, mulGate(gx, 1.4f, z[k], minv(5, c.strongest + 1), k));  // the strong gate right behind the gap
            const float wz = z[k] - 1.6f, h = 3.0f * streamAt(L, gx, wz);
            addWall(L, wall(-kHW + 0.1f, gx - 1.5f, wz, i32(h)));
            addWall(L, wall(gx + 1.5f, kHW - 0.1f, wz, i32(h)));
        } else {
            addGate(L, mulGate(-2.6f, 1.5f, z[k], factor(r, 2, c.strongest), k));
            addGate(L, mulGate(2.6f, 1.5f, z[k], factor(r, 2, c.strongest), k));
        }
    }
}

// ---- 17. Dawn line (Holdout): hold a line across the lane ----
void dawnLine(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    addGate(L, mulGate(-2.4f, 1.8f, z[0], 2, 0));  // wide gates low, so the flood is wide where the glooms arrive
    addGate(L, mulGate(2.4f, 1.8f, z[0], 2, 0));
    addGate(L, mulGate(0, 1.8f, z[1], 3, 1));
    for (float s : {-1.0f, 1.0f}) addGate(L, mulGate(s * 3.6f, 1.2f, z[1], 2, 1));
    for (int k = 2; k < rows; ++k) addGate(L, mulGate(r.range(-2.0f, 2.0f), 1.6f, z[k], factor(r, 2, c.strongest), k));
}

// ---- 18. The long walk (Colossus): grow the stream before it arrives ----
void longWalk(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx lc = c;
    lc.lengthClass = kLong;
    lc.rows = 4;
    float z[kMaxRows];
    const int rows = layRows(L, r, lc, z);
    L.lengthClass = kLong;
    const float col = (r.f01() < 0.5f ? -1.0f : 1.0f) * r.range(0.9f, 1.9f);
    for (int k = 0; k < rows; ++k) {  // the strongest gates lowest
        const int f = maxv(2, minv(5, c.strongest + 1) - k);
        const float x = clampv(col + r.range(-0.5f, 0.5f), -1.9f, 1.9f);
        addGate(L, mulGate(x, 1.6f, z[k], f, k));
        if (r.f01() < 0.5f) addGate(L, mulGate(x > 0 ? -3.6f : 3.6f, 1.2f, z[k], 2, k));
    }
}

// ---- 19. Two towers (Twins): which spire first ----
void twoTowers(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f;  // the stronger chain
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(s * 2.6f, 1.5f, z[k], factor(r, 3, c.strongest), k));
        addGate(L, mulGate(-s * 2.6f, 1.5f, z[k], 2, k));
    }
    // The weaker side's spire has the stronger emitters.
    addEmitter(L, -tune.twinX);
    addEmitter(L, tune.twinX);
    L.emitters[1].group = 1;
    L.emitters[s < 0 ? 1 : 0].share = 0.65f;
    L.emitters[s < 0 ? 0 : 1].share = 0.35f;
}

// ---- 20. Festival field: everything at once ----
void festivalField(LevelDef& L, Rng& r, const RecipeCtx& c) {
    RecipeCtx fc = c;
    fc.rows = 3;
    fc.lengthClass = kStandard;
    float z[kMaxRows];
    const int rows = layRows(L, r, fc, z);
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(-2.5f, 1.7f, z[k], factor(r, 3, minv(5, c.strongest + 1)), k));
        addGate(L, mulGate(2.5f, 1.7f, z[k], factor(r, 3, minv(5, c.strongest + 1)), k));
    }
}

// ---- 21. Breathing ladder (the executor's): wait for the gate to open ----
void breathingLadder(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float breath = r.range(3.0f, 4.5f), s = r.f01() < 0.5f ? -1.0f : 1.0f;
    for (int k = 0; k < rows; ++k) {
        Gate b = mulGate(s * r.range(1.2f, 1.6f), 1.5f, z[k], factor(r, 3, minv(5, c.strongest + 1)), k);
        b.breath = breath;
        b.phase = float(k) / float(rows);  // the openings come one after another up the lane
        addGate(L, b);
        addGate(L, mulGate(-s * 3.6f, 1.2f, z[k], 2, k));  // the narrow safe way
    }
}

// ---- 22. Gauntlet (the executor's): slip between the patrols ----
void gauntlet(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float col = (r.f01() < 0.5f ? -1.0f : 1.0f) * r.range(0.9f, 1.8f);
    for (int k = 0; k < rows; ++k) {
        const bool patrolled = k % 2 == 1 || (k == rows - 1 && rows == 2);
        addGate(L, mulGate(col, 1.5f, z[k], patrolled ? minv(5, c.strongest + 1) : 2, k));
        if (r.f01() < 0.7f) addGate(L, mulGate(col > 0 ? col - 3.4f : col + 3.4f, 1.2f, z[k], 2, k));  // the unpatrolled way
        if (patrolled && L.snufferCount < kMaxSnuffers) {
            Snuffer sn;
            sn.xa = maxv(-kLauncherRange, col - 2.2f);
            sn.xb = minv(kLauncherRange, col + 2.2f);
            sn.z = z[k] - 2.0f;
            sn.speed = r.range(2.5f, 3.5f);
            sn.phase = r.f01();
            L.snuffers[L.snufferCount++] = sn;
        }
    }
}

// ---- 23. Shifting window (the executor's): the window moves ----
void shiftingWindow(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f, period = r.range(4.0f, 6.0f);
    for (int k = 0; k < rows; ++k) {
        if (k >= 1 && k <= 2) {  // a strong gate behind a sliding wall, the two walls in opposite phase
            const float gx = (k == 1 ? s : -s) * 2.2f;
            addGate(L, mulGate(gx, 1.4f, z[k], k == 1 ? minv(5, c.strongest + 1) : c.strongest, k));
            Barrier b = wall(gx - 1.2f, gx + 1.2f, z[k] - 1.6f, i32(2.5f * streamAt(L, gx, z[k])));
            b.amp = minv(kHW - 0.1f - std::fabs(gx) - 1.2f, 1.0f);
            b.period = period;
            b.phase = k == 1 ? 0.0f : 0.5f;
            addWall(L, b);
            addGate(L, mulGate(-gx * 1.6f, 1.2f, z[k], 2, k));  // a plain way on the other side
        } else {
            addGate(L, mulGate(r.range(-1.0f, 1.0f), 1.8f, z[k], 2, k));
        }
    }
}

// ---- 24. Late bloom (the executor's): pay early for the big finish ----
void lateBloom(LevelDef& L, Rng& r, const RecipeCtx& c) {
    float z[kMaxRows];
    const int rows = layRows(L, r, c, z);
    const float s = r.f01() < 0.5f ? -1.0f : 1.0f;  // the blooming side
    for (int k = 0; k < rows; ++k) {
        addGate(L, mulGate(-s * 2.6f, 1.5f, z[k], 2, k));  // the clean side: x2 all the way
        if (k == 0) addGate(L, divGate(s * 2.6f, 1.5f, z[k], 2, k));
        else addGate(L, mulGate(s * 2.6f, 1.4f, z[k], k == rows - 1 ? minv(6, c.strongest + 1) : minv(5, 2 + k), k));
    }
}

// ---- The set pieces (VARIETY_DESIGN 10.3): show pieces laid out by hand, the hard level before every fiftieth. Each
// combines things in a way no recipe does. The seed only mirrors one and moves its geodes by half a second, so the bake
// has a few versions of it to balance. ----

// A set piece's lane: its length class, its length and its rows.
void pieceLane(LevelDef& L, u8 cls, float len, std::initializer_list<float> rows) {
    L.lengthClass = cls;
    L.len = len;
    L.targetT = targetTime(L.index, cls);
    for (float z : rows) addRow(L, z, kPatHand);
}

Gate moving(Gate g, u8 how, float amp, float period, float phase) {
    g.slides = how;
    g.amp = amp;
    g.period = period;
    g.phase = phase;
    return g;
}

Gate breathing(Gate g, float breath, float phase) {
    g.breath = breath;
    g.phase = phase;
    return g;
}

// A geode at a share of T, half a second either way by the seed.
Geode pieceGeode(const LevelDef& L, Rng& r, float x, float at, u8 boon, float hpSeconds) {
    Geode g = geode(x, 0, maxv(0.09f * L.targetT, at * L.targetT + r.range(-0.5f, 0.5f)), boon, hpSeconds);
    g.speed = 1.6f;
    return g;
}

// A detour geode (VARIETY_DESIGN 9.3) on the given side: off the best line and outside the stream's fan, where
// placeGeodes would put it, so only a stream aimed at it opens it. Call it after the gates.
Geode pieceDetour(const LevelDef& L, Rng& r, bool right, float at, u8 boon, float hpSeconds, float amp = 0) {
    Geode g = pieceGeode(L, r, 0, at, boon, hpSeconds);
    g.amp = amp;
    placeDetour(L, g, bestLineX(L), right, 0.4f);
    return g;
}

// A choice pair as two sides to choose between: the same moment, one boon left and one right, each a detour.
void piecePair(LevelDef& L, Rng& r, float at, u8 left, u8 right, u8 group) {
    Geode a = pieceDetour(L, r, false, at, left, 2.0f), b = pieceDetour(L, r, true, at, right, 2.0f);
    b.t = a.t;
    a.group = b.group = group;
    addGeode(L, a);
    addGeode(L, b);
}

// A vault (VARIETY_DESIGN 2.2): anchored at z0 behind a small wall worth `wallSeconds` of the stream there, until 80% of T.
void pieceVault(LevelDef& L, float x, float z0, float t, u8 boon, float wallSeconds) {
    Geode g = geode(x, z0, t, boon, 1.4f);
    g.leaveAt = 0.8f * L.targetT;
    g.speed = 1.6f;
    addGeode(L, g);
    addWall(L, wall(x - 1.3f, x + 1.3f, z0 - 1.5f, i32(wallSeconds * streamAt(L, x, z0))));
}

// 49. The staircase: climb the jumping gates. A hopper a row, each a quarter of a beat after the one below, a plain x2
// on the far side of each; the runners of a GEODE RAIN.
void pieceStaircase(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kLong, 44, {9.0f, 15.2f, 21.4f, 27.6f, 33.8f});
    static const int k[5] = {2, 2, 3, 3, 4};
    for (int row = 0; row < 5; ++row) {
        const float s = row % 2 ? 1.0f : -1.0f;
        addGate(L, moving(mulGate(0, 1.3f, L.rowZ[row], k[row], row), kGateHop, 1.2f, 1.8f, 0.25f * float(row)));
        addGate(L, mulGate(-s * 3.8f, 1.2f, L.rowZ[row], 2, row));
    }
    L.plan = kPlanPulse;
    addGeode(L, pieceDetour(L, r, false, 0.12f, kBoonQuicken, 2.0f));
    addGeode(L, pieceDetour(L, r, true, 0.45f, kBoonKindle, 2.5f));
    addTwist(L, kTwistGeodeRain, kTrigTime, 0.74f);  // when the first geode has drifted past: four on the lane at most
    static const u8 rain[3] = {kBoonLantern, kBoonBright, kBoonQuicken};
    for (int j = 0; j < 3; ++j) addTwistGeode(L, 0, -2.6f + 2.6f * float(j), 0, 3.0f, 0.6f, 0.8f, rain[j]);
}

// 99. The long dark: find the gates that breathe in the dark. Breathing gates, a x5 behind a sliding wall; ECLIPSE,
// then a stampede.
void pieceLongDark(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kLong, 44, {9.0f, 16.5f, 24.0f, 31.5f});
    addGate(L, breathing(mulGate(0, 1.5f, 9.0f, 3, 0), 3.5f, 0));
    addGate(L, mulGate(-3.5f, 1.2f, 9.0f, 2, 0));
    addGate(L, breathing(mulGate(-2.2f, 1.4f, 16.5f, 4, 1), 4.0f, 0.5f));
    addGate(L, breathing(mulGate(2.4f, 1.4f, 16.5f, 2, 1), 4.0f, 0));
    addGate(L, mulGate(2.4f, 1.3f, 24.0f, 5, 2));
    addGate(L, mulGate(-2.6f, 1.5f, 24.0f, 2, 2));
    Barrier b = wall(1.1f, 3.7f, 22.4f, i32(2.5f * streamAt(L, 2.4f, 24.0f)));
    b.amp = 1.0f;
    b.period = 5.0f;
    addWall(L, b);
    addGate(L, breathing(mulGate(0, 1.6f, 31.5f, 3, 3), 3.0f, 0.25f));
    addGate(L, mulGate(3.6f, 1.2f, 31.5f, 2, 3));
    L.plan = kPlanLateSurge;
    pieceVault(L, 3.7f, 20.0f, 0.12f * L.targetT + r.range(-0.5f, 0.5f), kBoonBright, 1.2f);
    addGeode(L, pieceDetour(L, r, false, 0.4f, kBoonHush, 1.5f));
    Geode k = pieceGeode(L, r, bestLineX(L), 0.62f, kBoonKindle, 1.5f);
    k.shieldSeconds = 1.5f;
    addGeode(L, k);
    addTwist(L, kTwistEclipse, kTrigTime, 0.3f);
    addTwist(L, kTwistStampede, kTrigTime, 0.68f);
}

// 149. The tollgate: thin wall or rich wall. One wall across the lane in five spans, 1 : 2 : 3 : 4 : 8 shuffled; the x5
// stands behind the thickest.
void pieceTollgate(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    addGate(L, mulGate(0, 1.8f, 9.0f, 2, 0));
    addGate(L, mulGate(-2.6f, 1.5f, 15.5f, 3, 1));
    addGate(L, mulGate(2.6f, 1.5f, 15.5f, 2, 1));
    static const float edges[6] = {-4.9f, -3.0f, -1.0f, 1.0f, 3.0f, 4.9f};
    int mult[5] = {1, 2, 3, 4, 8};
    for (int i = 4; i > 0; --i) {
        const int j = r.rangei(0, i), t = mult[i];
        mult[i] = mult[j];
        mult[j] = t;
    }
    int thick = 0;
    for (int sp = 0; sp < 5; ++sp) {
        const float cx = 0.5f * (edges[sp] + edges[sp + 1]);
        addWall(L, wall(edges[sp] + 0.02f, edges[sp + 1] - 0.02f, 18.75f, i32(1.2f * float(mult[sp]) * streamAt(L, cx, 18.75f))));
        if (mult[sp] == 8) thick = sp;
    }
    const float x5 = clampv(0.5f * (edges[thick] + edges[thick + 1]), -3.7f, 3.7f);
    addGate(L, mulGate(x5, 1.2f, 22.0f, 5, 2));
    addGate(L, mulGate(x5 < 0 ? 3.7f : -3.7f, 1.2f, 22.0f, 2, 2));
    addGate(L, mulGate(-2.4f, 1.4f, 28.5f, 3, 3));
    addGate(L, mulGate(2.4f, 1.4f, 28.5f, 3, 3));
    L.plan = kPlanBruteLine;
    addGeode(L, pieceDetour(L, r, false, 0.12f, kBoonQuicken, 2.0f));
    addGeode(L, pieceDetour(L, r, true, 0.4f, kBoonBright, 2.0f));
    addGeode(L, pieceGeode(L, r, bestLineX(L), 0.62f, kBoonComet, 1.5f));
    addTwist(L, kTwistSurge, kTrigTime, 0.5f);
}

// 199. Clockwork: everything moves in time. Every gate slides on one beat, row against row; a patrol and a wall on the
// same beat; geodes that sway with it.
void pieceClockwork(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    static const int k[4][2] = {{2, 3}, {3, 2}, {4, 2}, {2, 5}};
    for (int row = 0; row < 4; ++row)
        for (int s = 0; s < 2; ++s)
            addGate(L, moving(mulGate(s ? 2.2f : -2.2f, 1.4f, L.rowZ[row], k[row][s], row), kGateSlide, 1.0f, 4.0f, row % 2 ? 0.5f : 0.0f));
    Snuffer sn;
    sn.xa = -3.5f;
    sn.xb = 3.5f;
    sn.z = 18.75f;
    sn.speed = 3.5f;
    L.snuffers[L.snufferCount++] = sn;
    Barrier b = wall(1.0f, 3.4f, 26.9f, i32(2.0f * streamAt(L, 2.2f, 28.5f)));
    b.amp = 1.0f;
    b.period = 4.0f;
    addWall(L, b);
    L.plan = kPlanPulse;
    const float t0 = 0.12f * L.targetT + r.range(-0.5f, 0.5f);
    pieceVault(L, 3.7f, 21.5f, t0, kBoonQuicken, 1.2f);
    pieceVault(L, -3.7f, 31.0f, t0 + 0.33f * L.targetT, kBoonLantern, 1.2f);
    addTwist(L, kTwistStampede, kTrigTime, 0.6f);
}

// 249. The fortress: three walls, three keys. Each wall across the lane is thin in one span (left, middle, right), the
// best gate behind it; a vault above the last.
void pieceFortress(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kLong, 44, {9.0f, 16.5f, 24.0f, 31.5f});
    addGate(L, mulGate(0, 1.8f, 9.0f, 2, 0));
    static const float edges[4] = {-4.9f, -1.63f, 1.63f, 4.9f};
    static const float wz[3] = {12.75f, 20.25f, 27.75f};
    for (int w = 0; w < 3; ++w)
        for (int sp = 0; sp < 3; ++sp) {
            const float cx = 0.5f * (edges[sp] + edges[sp + 1]);
            addWall(L, wall(edges[sp] + 0.02f, edges[sp + 1] - 0.02f, wz[w], i32((sp == w ? 1.0f : 4.0f) * streamAt(L, cx, wz[w]))));
        }
    addGate(L, mulGate(-2.6f, 1.5f, 16.5f, 3, 1));
    addGate(L, mulGate(2.6f, 1.5f, 16.5f, 2, 1));
    addGate(L, mulGate(0, 1.5f, 24.0f, 3, 2));
    addGate(L, mulGate(-3.6f, 1.2f, 24.0f, 2, 2));
    addGate(L, mulGate(2.6f, 1.5f, 31.5f, 4, 3));
    addGate(L, mulGate(-2.6f, 1.5f, 31.5f, 2, 3));
    addGeode(L, pieceGeode(L, r, 3.4f, 0.12f, kBoonQuicken, 2.0f));
    pieceVault(L, -3.4f, 36.0f, L.geodes[0].t + 6.0f, kBoonLantern, 2.0f);
    addGeode(L, pieceGeode(L, r, 0.0f, 0.58f, kBoonComet, 1.5f));
    addTwist(L, kTwistBruteGuard, kTrigTime, 0.45f, 2);
}

// 299. Hall of mirrors: both halves alike, until one turns gold. A symmetric lane, two choice pairs down its middle, and
// GOLDEN GATE on one side; the enemy in a pincer.
void pieceMirrors(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    for (float s : {-1.0f, 1.0f}) addGate(L, mulGate(s * 2.4f, 1.6f, 9.0f, 2, 0));
    for (float s : {-1.0f, 1.0f}) addGate(L, mulGate(s * 2.8f, 1.4f, 15.5f, 3, 1));
    addGate(L, divGate(0, 1.2f, 15.5f, 2, 1));
    const int golden = L.gateCount + (r.f01() < 0.5f ? 0 : 1);
    for (float s : {-1.0f, 1.0f}) addGate(L, mulGate(s * 2.4f, 1.5f, 22.0f, 3, 2));
    for (float s : {-1.0f, 1.0f}) addGate(L, mulGate(s * 2.4f, 1.5f, 28.5f, 4, 3));
    L.plan = kPlanPulse;
    piecePair(L, r, 0.18f, kBoonQuicken, kBoonLantern, 1);
    piecePair(L, r, 0.52f, kBoonQuicken, kBoonKindle, 2);
    addTwist(L, kTwistGoldenGate, kTrigTime, 0.4f, 0, u8(golden));
}

// 349. Siege of doors: they come from every side. Four side doors and one emitter at the foot, gates down the middle.
void pieceDoors(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 16.0f, 23.0f, 30.0f});
    addGate(L, mulGate(0.3f, 1.6f, 9.0f, 2, 0));
    addGate(L, mulGate(-0.4f, 1.6f, 16.0f, 3, 1));
    addGate(L, mulGate(3.6f, 1.2f, 16.0f, 2, 1));
    addGate(L, mulGate(0.4f, 1.5f, 23.0f, 4, 2));
    addGate(L, mulGate(-0.3f, 1.6f, 30.0f, 3, 3));
    addGate(L, mulGate(-3.6f, 1.2f, 30.0f, 2, 3));
    L.plan = kPlanFlank;
    addEmitter(L, 0);
    L.emitters[0].share = 0.6f;
    addDoor(L, -1, 20.5f, 0.1f);
    addDoor(L, 1, 20.5f, 0.1f);
    addDoor(L, -1, 26.5f, 0.1f);
    addDoor(L, 1, 32.0f, 0.1f);
    addGeode(L, pieceDetour(L, r, true, 0.12f, kBoonHush, 1.5f));
    addGeode(L, pieceDetour(L, r, false, 0.35f, kBoonLantern, 2.0f));
    addGeode(L, pieceGeode(L, r, bestLineX(L), 0.56f, kBoonWard, 1.5f));
    addTwist(L, kTwistStampede, kTrigTime, 0.6f, 0, 1);
}

// 399. Rush hour: runners pour through the doors. Runner packs from the foot and two side doors, hoppers in the middle.
void pieceRushHour(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    addGate(L, mulGate(-2.4f, 1.6f, 9.0f, 2, 0));
    addGate(L, mulGate(2.4f, 1.6f, 9.0f, 2, 0));
    addGate(L, moving(mulGate(0, 1.4f, 15.5f, 3, 1), kGateHop, 2.2f, 2.0f, 0));
    addGate(L, moving(mulGate(0, 1.4f, 22.0f, 4, 2), kGateHop, 2.2f, 1.6f, 0.5f));
    addGate(L, mulGate(-2.6f, 1.5f, 28.5f, 3, 3));
    addGate(L, mulGate(2.6f, 1.5f, 28.5f, 4, 3));
    L.plan = kPlanRunnerPacks;
    addEmitter(L, -2.0f);
    addEmitter(L, 2.0f);
    L.emitters[0].share = L.emitters[1].share = 0.3f;
    addDoor(L, -1, 18.75f, 0.2f);
    addDoor(L, 1, 25.25f, 0.2f);
    addGeode(L, pieceDetour(L, r, true, 0.12f, kBoonComet, 1.5f));
    addGeode(L, pieceDetour(L, r, false, 0.35f, kBoonBright, 2.0f));
    addGeode(L, pieceDetour(L, r, true, 0.6f, kBoonQuicken, 2.0f));
    addTwist(L, kTwistSurge, kTrigTime, 0.45f);
}

// 449. The treasury: three vaults, three prices. Gates down the middle; vaults on alternating sides behind walls of one,
// two and three seconds of the stream.
void pieceTreasury(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    addGate(L, mulGate(1.9f, 1.5f, 9.0f, 2, 0));
    addGate(L, mulGate(1.9f, 1.5f, 15.5f, 3, 1));
    addGate(L, mulGate(-2.1f, 1.4f, 15.5f, 2, 1));
    addGate(L, mulGate(1.9f, 1.5f, 22.0f, 3, 2));
    addGate(L, mulGate(-2.1f, 1.4f, 22.0f, 2, 2));
    addGate(L, mulGate(1.9f, 1.5f, 28.5f, 4, 3));
    const float t0 = 0.1f * L.targetT + r.range(0.0f, 0.5f);
    pieceVault(L, -3.7f, 14.0f, t0, kBoonLantern, 0.5f);
    pieceVault(L, 3.7f, 21.0f, t0 + 5.5f, kBoonQuicken, 1.0f);
    pieceVault(L, -3.7f, 31.5f, t0 + 11.0f, kBoonComet, 1.5f);
    addTwist(L, kTwistSurge, kTrigTime, 0.55f);
}

// 499. The storm: hold on through the storm. An early rush, a surge, then an eclipse; breathing and swinging gates.
void pieceStorm(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    addGate(L, breathing(mulGate(-2.0f, 1.5f, 9.0f, 3, 0), 3.0f, 0));
    addGate(L, mulGate(2.6f, 1.5f, 9.0f, 2, 0));
    addGate(L, moving(mulGate(-2.2f, 1.4f, 15.5f, 3, 1), kGateSlide, 1.0f, 4.5f, 0));
    addGate(L, moving(mulGate(2.2f, 1.4f, 15.5f, 2, 1), kGateSlide, 1.0f, 4.5f, 0));
    addGate(L, breathing(mulGate(2.0f, 1.5f, 22.0f, 4, 2), 3.5f, 0.5f));
    addGate(L, mulGate(-2.6f, 1.5f, 22.0f, 2, 2));
    addGate(L, moving(mulGate(0, 1.6f, 28.5f, 4, 3), kGateSlide, 2.0f, 5.0f, 0.25f));
    L.plan = kPlanEarlyRush;
    addGeode(L, pieceDetour(L, r, true, 0.12f, kBoonBright, 2.0f));
    addGeode(L, pieceDetour(L, r, false, 0.4f, kBoonWard, 1.5f));
    addGeode(L, pieceGeode(L, r, bestLineX(L), 0.65f, kBoonWard, 1.5f));
    addTwist(L, kTwistSurge, kTrigTime, 0.3f);
    addTwist(L, kTwistEclipse, kTrigTime, 0.6f);
}

// 549. The needle: thread two needles. Two narrows whose gaps overlap by 1.2 m, a hopper behind each.
void pieceNeedle(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kStandard, 40, {9.0f, 15.5f, 22.0f, 28.5f});
    addGate(L, mulGate(-2.6f, 1.5f, 9.0f, 2, 0));
    addGate(L, mulGate(2.6f, 1.5f, 9.0f, 2, 0));
    addGate(L, moving(mulGate(-0.9f, 1.3f, 15.5f, 4, 1), kGateHop, 1.0f, 1.8f, 0));
    addGate(L, mulGate(-2.4f, 1.5f, 22.0f, 3, 2));
    addGate(L, mulGate(2.4f, 1.5f, 22.0f, 3, 2));
    addGate(L, moving(mulGate(0.9f, 1.3f, 28.5f, 5, 3), kGateHop, 1.0f, 1.8f, 0.5f));
    // The two needles: gaps of 3 m at x -0.9 and +0.9, so a straight stream threads both only down the 1.2 m between.
    for (int n = 0; n < 2; ++n) {
        const float gx = n ? 0.9f : -0.9f, wz = n ? 26.5f : 13.5f, h = 3.0f * streamAt(L, gx, wz);
        addWall(L, wall(-kHW + 0.1f, gx - 1.5f, wz, i32(h)));
        addWall(L, wall(gx + 1.5f, kHW - 0.1f, wz, i32(h)));
    }
    // Before the first needle, beside the first gates: a pair (one boon a side), then a BRIGHT, low on the lane where the
    // stream down the needles' line does not reach.
    const float t = 0.15f * L.targetT + r.range(-0.5f, 0.5f);
    Geode a = geode(-3.5f, 11.5f, t, kBoonQuicken, 1.5f), b = geode(3.5f, 11.5f, t, kBoonLantern, 1.5f);
    a.group = b.group = 1;
    a.speed = b.speed = 1.0f;
    addGeode(L, a);
    addGeode(L, b);
    Geode c = geode(-3.5f, 11.5f, 0.45f * L.targetT + r.range(-0.5f, 0.5f), kBoonBright, 1.5f);
    c.speed = 1.0f;
    addGeode(L, c);
    addTwist(L, kTwistBruteGuard, kTrigTime, 0.5f, 2);
}

// 599. The last light: everything you have learned. A breathing gate, a hopper, a x5 behind a sliding wall, a swinging
// pair, a x6 at the top; a side door; a choice pair and a vault; GOLDEN GATE and a surge.
void pieceLastLight(LevelDef& L, Rng& r, const RecipeCtx&) {
    pieceLane(L, kLong, 44, {9.0f, 15.2f, 21.4f, 27.6f, 33.8f});
    addGate(L, breathing(mulGate(0, 1.6f, 9.0f, 2, 0), 3.5f, 0));
    addGate(L, mulGate(-3.7f, 1.2f, 9.0f, 2, 0));
    addGate(L, moving(mulGate(1.0f, 1.3f, 15.2f, 3, 1), kGateHop, 1.9f, 2.0f, 0));
    addGate(L, mulGate(-3.6f, 1.2f, 15.2f, 2, 1));
    addGate(L, mulGate(-2.4f, 1.3f, 21.4f, 5, 2));
    addGate(L, mulGate(2.6f, 1.5f, 21.4f, 2, 2));
    Barrier b = wall(-3.7f, -1.1f, 19.6f, i32(2.0f * streamAt(L, -2.4f, 21.4f)));
    b.amp = 1.0f;
    b.period = 4.5f;
    addWall(L, b);
    const int golden = L.gateCount;
    addGate(L, moving(mulGate(-2.2f, 1.4f, 27.6f, 2, 3), kGateSlide, 1.0f, 4.0f, 0));
    addGate(L, moving(mulGate(2.2f, 1.4f, 27.6f, 2, 3), kGateSlide, 1.0f, 4.0f, 0));
    addGate(L, mulGate(0, 1.3f, 33.8f, 6, 4));
    addGate(L, mulGate(3.6f, 1.2f, 33.8f, 2, 4));
    L.plan = kPlanFlank;
    addEmitter(L, -2.0f);
    addEmitter(L, 2.0f);
    L.emitters[0].share = L.emitters[1].share = 0.35f;
    addDoor(L, -1, 24.5f, 0.3f);
    piecePair(L, r, 0.12f, kBoonQuicken, kBoonBright, 1);
    pieceVault(L, 3.5f, 30.0f, L.geodes[0].t + 6.0f, kBoonLantern, 2.0f);
    addTwist(L, kTwistGoldenGate, kTrigTime, 0.4f, 0, u8(golden));
    addTwist(L, kTwistSurge, kTrigTime, 0.65f);
}

}  // namespace

// The catalogue: name, idea, kinds, first level, mirrored twin, builder. Its order is the recipe number in the data.
const RecipeRow kRecipes[] = {
    {"Terraces", "Rows of gates, pick the best of each", kFight | bit(kHoldout) | bit(kColossus) | bit(kTwins) | bit(kFestival), 1, false, terraces},
    {"Funnel", "Aim finer as the stream climbs", kFight | bit(kHoldout) | bit(kColossus) | bit(kFestival), 8, false, funnel},
    {"Twin rivers", "Commit to a side, cross when you must", kFight | bit(kTwins), 33, true, twinRivers},
    {"Checker", "One clean column, one rich and dirty", kFight | bit(kTwins), 58, false, checker},
    {"The vault", "Pay to open the jackpot", kFight, 73, true, vault},
    {"Toll road", "Find the thin wall", kFight | bit(kHoldout), 46, false, tollRoad},
    {"Pendulum", "Ride the wave", kFight | bit(kColossus) | bit(kFestival), 68, false, pendulum},
    {"Stagger", "Odd rows or even rows", kFight | bit(kColossus), 61, true, stagger},
    {"Islands", "Thread the channels", kFight | bit(kHoldout), 83, false, islands},
    {"Crossfire", "They come from the sides", kFight, 28, true, crossfire},
    {"The breach", "Fight for the rich side or farm the quiet one", kFight, 93, true, breach},
    {"Caravan", "Three gifts in a row, on the wrong side", kFight, 96, true, caravan},
    {"Crossroads", "This boon or that one", kFight, 26, false, crossroads},
    {"Treasure walls", "Something locked on each side", kFight, 56, false, treasureWalls},
    {"Hopscotch", "The gate will not stay", kFight | bit(kHoldout) | bit(kFestival), 17, false, hopscotch},
    {"The narrows", "A chokepoint", kFight | bit(kHoldout), 78, false, narrows},
    {"Dawn line", "Hold a line across the lane", bit(kHoldout), 22, false, dawnLine},
    {"The long walk", "Grow the stream before it arrives", bit(kColossus), 32, false, longWalk},
    {"Two towers", "Which spire first", bit(kTwins), 42, true, twoTowers},
    {"Festival field", "Everything at once", bit(kFestival), 15, false, festivalField},
    {"Breathing ladder", "Wait for the gate to open", kFight | bit(kFestival) | bit(kColossus), 39, true, breathingLadder},
    {"Gauntlet", "Slip between the patrols", kFight | bit(kHoldout), 24, false, gauntlet},
    {"Shifting window", "The window moves", kFight | bit(kTwins), 91, true, shiftingWindow},
    {"Late bloom", "Pay early for the big finish", kFight | bit(kColossus), 88, true, lateBloom},
};

// The set pieces (VARIETY_DESIGN 10.3), in level order: 49, 99 ... 599. No kind draws them; each is its own level's.
const RecipeRow kSetPieces[] = {
    {"The staircase", "Climb the jumping gates", 0, 49, false, pieceStaircase},
    {"The long dark", "Find the gates that breathe in the dark", 0, 99, true, pieceLongDark},
    {"The tollgate", "Thin wall or rich wall", 0, 149, true, pieceTollgate},
    {"Clockwork", "Everything moves in time", 0, 199, true, pieceClockwork},
    {"The fortress", "Three walls, three keys", 0, 249, true, pieceFortress},
    {"Hall of mirrors", "Both halves alike, until one turns gold", 0, 299, false, pieceMirrors},
    {"Siege of doors", "They come from every side", 0, 349, true, pieceDoors},
    {"Rush hour", "Runners pour through the doors", 0, 399, true, pieceRushHour},
    {"The treasury", "Three vaults, three prices", 0, 449, true, pieceTreasury},
    {"The storm", "Hold on through the storm", 0, 499, true, pieceStorm},
    {"The needle", "Thread two needles", 0, 549, true, pieceNeedle},
    {"The last light", "Everything you have learned", 0, 599, true, pieceLastLight},
};

int recipeCount() { return int(sizeof kRecipes / sizeof kRecipes[0]); }
int setPieceCount() { return int(sizeof kSetPieces / sizeof kSetPieces[0]); }

// A recipe's name in the data: the catalogue's, then the set pieces' (recipeCount() + k).
const char* recipeName(int recipe) {
    if (recipe >= 0 && recipe < recipeCount()) return kRecipes[recipe].name;
    const int p = recipe - recipeCount();
    return p >= 0 && p < setPieceCount() ? kSetPieces[p].name : "?";
}

}  // namespace gt::sim

// Recipes: ways of laying out a lane, each with one idea (VARIETY_DESIGN 9.2). "Terraces" is the generator of version
// 0.1.0 (GAME_DESIGN 7.3): rows of gates, pick the best of each.
#include <cmath>

#include "core/rng.h"
#include "sim/level_build.h"

namespace gt::sim {

const char* recipeName(int recipe) {
    static const char* const n[] = {"Terraces"};
    return recipe >= 0 && recipe < countOf(n) ? n[recipe] : "?";
}

namespace {

// The column a straight stream does best in, on the layout alone: the gates' factors where they stand, a slider
// counted for the share of its travel it spends over the column.
float bestLineX(const LevelDef& L) {
    float bestX = 0, best = -1;
    for (int s = 0; s <= 44; ++s) {
        const float x = -kLauncherRange + 0.2f * float(s);
        float m = columnMult(L, x, L.len);
        for (int i = 0; i < L.barrierCount; ++i)
            if (x >= L.barriers[i].x0 && x <= L.barriers[i].x1) m *= 0.8f;
        if (m > best + 1e-4f || (std::fabs(m - best) <= 1e-4f && std::fabs(x) < std::fabs(bestX))) {
            best = m;
            bestX = x;
        }
    }
    return bestX;
}

}  // namespace

// Geodes in a generated level (VARIETY_DESIGN 9.3): how many, their boons within the budget, when they come, and where:
// at least half of them away from the best line, the rest on it.
void placeGeodes(LevelDef& L, Rng& r) {
    const int index = L.index;
    if (index < 6) return;
    int n = r.rangei(index <= 20 ? 1 : 2, index <= 20 ? 2 : index <= 60 ? 3 : 4);
    if (L.boss) n = maxv(n, 2);
    const float budget = index <= 20 ? 2.0f : index <= 60 ? 2.5f : 3.0f;
    const float T = L.targetT, best = bestLineX(L);
    int lanterns = 1, quicken = 0, bright = 0, wards = 0, kinds = 0;
    // The level that introduces a boon offers it first (VARIETY_DESIGN 10.1): KINDLE 36, COMET 52, HUSH 63.
    const int intro = index == 36 ? kBoonKindle : index == 52 ? kBoonComet : index == 63 ? kBoonHush : -1;
    bool used[kBoonCount] = {};
    float worth = 1, t = r.range(0.08f, 0.25f) * T;
    for (int k = 0; k < n && L.geodeCount < kMaxGeodes; ++k) {
        if (k > 0) t += r.range(5.0f, 8.0f);
        if (L.boss && k == 1) t = minv(t, maxv(L.geodes[0].t + 5.0f, 0.4f * T));
        // The boons this geode may hold: from their first level, under their limits and the budget, at most three kinds a
        // level; the first geode up to level 60 lasts; a WARD at most once, never first, never at 75% of T or later.
        const float late = t > 0.75f * T;
        float wt[kBoonCount] = {};
        auto gainOf = [&](int b) {
            return b == kBoonKindle ? (used[kBoonKindle] ? 0.0f : kindleGain(L, best)) : boonGain(b, lanterns, quicken, bright);
        };
        auto allow = [&](int b, int from, float weight) {
            if (index < from || (!used[b] && kinds >= 3)) return;
            const float gain = gainOf(b);
            if (gain <= 0 || late || worth * (1.0f + gain) > budget + 1e-3f) return;
            wt[b] = weight;
        };
        allow(kBoonLantern, 6, k == 0 ? 4.0f : 3.0f);  // the first one is a detour: lean it toward a boon worth one
        allow(kBoonQuicken, 7, k == 0 ? 1.5f : 3.0f);
        allow(kBoonBright, 12, k == 0 ? 3.5f : 2.5f);
        allow(kBoonKindle, 36, 2.0f);
        if (index >= 23 && wards == 0 && (k > 0 || index > 60) && t < 0.85f * T && (used[kBoonWard] || kinds < 3)) wt[kBoonWard] = 1.5f;
        // The instants: COMET and HUSH at most once a level each; HUSH is rarer (it eases the contest: V2's notes).
        if (index >= 52 && !used[kBoonComet] && !late && (k > 0 || index > 60) && kinds < 3) wt[kBoonComet] = 1.2f;
        if (index >= 63 && !used[kBoonHush] && !late && kinds < 3) wt[kBoonHush] = 0.7f;
        // The introduction: the first geode; a COMET the second, which rides the best line (a comet fires from where the
        // lantern is when it opens: opened on a detour it climbs the detour's weak line).
        if (intro >= 0 && k == (intro == kBoonComet ? 1 : 0)) {
            for (float& x : wt) x = 0;
            wt[intro] = 1;
        }
        float total = 0;
        for (float x : wt) total += x;
        if (total <= 0) break;
        float pick = r.range(0, total);
        int boon = 0;
        while (boon < kBoonCount - 1 && pick >= wt[boon]) pick -= wt[boon++];
        if (wt[boon] <= 0) break;
        if (!used[boon]) used[boon] = true, ++kinds;
        worth *= 1.0f + gainOf(boon);
        if (boon == kBoonLantern) ++lanterns;
        if (boon == kBoonQuicken) ++quicken;
        if (boon == kBoonBright) ++bright;
        if (boon == kBoonWard) ++wards;

        Geode g;
        g.t = t;
        g.boon = u8(boon);
        g.speed = r.range(1.2f, 2.0f);
        const bool detour = k % 2 == 0;
        if (detour && index >= 40 && r.f01() < 0.25f) {  // a slide, on detours only: a moving geode is harder to hit
            g.amp = r.range(0.5f, 0.9f);
            g.period = r.range(3.0f, 5.0f);
            g.phase = r.f01();
        }
        const float edge = 3.9f - g.amp;
        if (detour) {
            // A detour: off the best line by 2 m or more and outside the fan of the stream there, so only a stream aimed at
            // it opens it. Where the flood fills the top of the lane, it appears lower down, where the stream is narrower.
            const bool preferRight = r.f01() < 0.5f;
            const float extra = r.range(0.2f, 0.8f);
            g.x = clampv(best + (preferRight ? 1.0f : -1.0f) * (2.0f + g.amp + extra), -edge, edge);
            const float top = L.len - 3.0f, low = L.rowCount ? L.rowZ[0] + 2.0f : top;
            for (float z = top; z >= low; z -= 1.0f) {
                const float off = maxv(2.0f, fanSpread(L, best, z) + g.radius + 1.0f) + g.amp + extra;  // clear of the spill
                const bool rightOk = best + off <= edge, leftOk = best - off >= -edge;
                if (!rightOk && !leftOk) continue;
                g.x = rightOk && (!leftOk || preferRight) ? best + off : best - off;
                g.z0 = z < top ? z : 0.0f;
                break;
            }
        } else {  // on the line
            g.x = clampv(best + r.range(-0.6f, 0.6f), -edge, edge);
        }
        if (detour && index >= 11 && boon != kBoonWard && r.f01() < (index <= 20 ? 0.3f : 0.4f)) {  // plates on detours only
            g.hpSeconds = r.range(1.0f, 1.5f);
            g.shieldSeconds = r.range(3.0f, 4.0f);
        } else {
            g.hpSeconds = boon == kBoonWard ? r.range(1.5f, 2.0f) : r.range(2.0f, 3.0f);
        }
        L.geodes[L.geodeCount++] = g;
    }
}

// A festival's rain (VARIETY_DESIGN 6): 10 to 14 small geodes falling over its 20 s, boons from the pool and under their
// limits (no WARD or HUSH: there is no enemy), COMET for the rest.
void placeFestivalGeodes(LevelDef& L, Rng& r) {
    const int n = r.rangei(10, 14);
    int lanterns = 1, quicken = 0, bright = 0, kindle = 0;
    for (int k = 0; k < n && L.geodeCount < kMaxGeodes; ++k) {
        Geode g;
        g.t = 1.0f + 15.0f * float(k) / float(n) + r.range(0.0f, 0.6f);
        g.x = r.range(-3.6f, 3.6f);
        g.speed = r.range(1.8f, 2.6f);
        g.radius = 0.6f;
        g.hpSeconds = r.range(0.5f, 0.9f);
        int b = kBoonComet;
        const float roll = r.f01();
        if (roll < 0.25f && lanterns < int(tune.lanternMax)) b = kBoonLantern, ++lanterns;
        else if (roll < 0.5f && quicken < int(tune.quickenMax)) b = kBoonQuicken, ++quicken;
        else if (roll < 0.7f && bright < int(tune.brightMax)) b = kBoonBright, ++bright;
        else if (roll < 0.8f && kindle < 1 && L.index >= 36) b = kBoonKindle, ++kindle;
        g.boon = u8(b);
        L.geodes[L.geodeCount++] = g;
    }
}

// A dormant wall across the whole lane, 6 m below the spire, raised by WARD WALL; its health is set with the spire's.
static u8 addWardWall(LevelDef& L) {
    if (L.barrierCount >= kMaxBarriers) return 0;
    Barrier b;
    b.x0 = -kHW + 0.1f;
    b.x1 = kHW - 0.1f;
    b.z = L.len - 6.0f;
    b.health = 10;
    b.dormant = 1;
    L.barriers[L.barrierCount] = b;
    return u8(L.barrierCount++);
}

// The boss's script (VARIETY_DESIGN 8): two twists at 66% and 33% of its health.
void addBossScript(LevelDef& L) {
    switch (L.bossType) {
        case kBossWarden:
            addTwist(L, kTwistWardWall, kTrigGoal, 0.66f, 0, addWardWall(L));
            addTwist(L, kTwistBruteGuard, kTrigGoal, 0.33f, 2);
            break;
        case kBossTempest:
            addTwist(L, kTwistStampede, kTrigGoal, 0.66f, 0, 0);
            addTwist(L, kTwistEclipse, kTrigGoal, 0.33f);
            break;
        case kBossHydra:
            addTwist(L, kTwistCub, kTrigGoal, 0.66f, 0, 1);
            addTwist(L, kTwistCub, kTrigGoal, 0.33f, 0, 2);
            break;
        default: {  // THE OLD SPIRE: today's boss, a surge with two brutes at each threshold
            TwistDef& a = addTwist(L, kTwistSurge, kTrigGoal, 0.66f, 2);
            a.dur = 4.0f;
            TwistDef& b = addTwist(L, kTwistBruteGuard, kTrigGoal, 0.33f, 2);
            b.dur = 4.0f;
            break;
        }
    }
}

// A geode a twist releases (GEODE RAIN's runners, SECOND WIND's WARD).
static void addTwistGeode(LevelDef& L, int twist, float x, float z0, float speed, float radius, float hpSeconds, u8 boon) {
    if (L.geodeCount >= kMaxGeodes) return;
    Geode g;
    g.x = x;
    g.z0 = z0;
    g.speed = speed;
    g.radius = radius;
    g.hpSeconds = hpSeconds;
    g.boon = boon;
    g.twist = u8(twist + 1);
    L.geodes[L.geodeCount++] = g;
}

// Twists on a generated level (VARIETY_PLAN V3 task 8): SECOND WIND from level 23 wherever there is an enemy (the one
// comeback: it fires only when the core falls to 3); from level 43 one more, chosen by the seed, on levels that are not
// bosses or festivals.
void addTwists(LevelDef& L, Rng& r) {
    if (!L.hasEnemy()) return;
    if (L.index >= 43 && L.kind != kBoss && L.kind != kFestival && L.twistCount < kMaxTwists - 1) {
        static const u8 pool[6] = {kTwistSurge, kTwistBruteGuard, kTwistGeodeRain, kTwistEclipse, kTwistStampede, kTwistGoldenGate};
        const u8 type = pool[r.rangei(0, 5)];
        const int k = L.twistCount;
        TwistDef& x = addTwist(L, type, kTrigTime, r.range(0.3f, 0.65f), type == kTwistBruteGuard ? 2 : 0);
        if (type == kTwistStampede) x.param = u8(r.rangei(0, L.emitterCount - 1));
        if (type == kTwistGoldenGate) {  // the strongest multiply gate of the middle row
            const int row = L.rowCount / 2;
            int best = -1;
            for (int g = 0; g < L.gateCount; ++g)
                if (L.gates[g].row == row && L.gates[g].kind == kGateMul && (best < 0 || L.gates[g].k > L.gates[best].k)) best = g;
            x.param = u8(maxv(0, best));
        }
        if (type == kTwistGeodeRain) {  // three runner geodes at once, across the lane
            static const u8 boons[3] = {kBoonQuicken, kBoonBright, kBoonLantern};
            for (int j = 0; j < 3; ++j)
                addTwistGeode(L, k, -2.6f + 2.6f * float(j) + r.range(-0.4f, 0.4f), 0, 3.0f, 0.6f, r.range(0.6f, 0.9f), boons[(j + r.rangei(0, 2)) % 3]);
        }
    }
    if (L.index >= 23 && L.twistCount < kMaxTwists) {
        const int k = L.twistCount;
        addTwist(L, kTwistSecondWind, kTrigCore, 3.0f);
        addTwistGeode(L, k, 0.0f, 0.5f * L.len, 1.0f, 0.9f, 0.8f, kBoonWard);
    }
}

LevelDef generate(int index, u64 seed) {
    LevelDef L;
    L.index = index;
    L.seed = seed;
    L.world = worldFor(index);
    L.targetT = targetTime(index);
    L.hardness = hardnessFor(index);
    L.fight = fightFor(index) + (L.hardness - 0.5f) * 0.08f;  // a world's rhythm moves the enemy's share (VARIETY_DESIGN 10.2)
    L.kind = u8(kindFor(index));
    L.boss = L.kind == kBoss;
    if (L.boss) L.bossType = u8((index / 10 - 1) % kBossCount);
    const bool festival = L.kind == kFestival;
    Rng r(seed, 7);

    Band band = bandFor(index);
    if (festival) {  // rich gates: no traps, patrols or crossing divides, one factor more
        band.strongest = minv(5, band.strongest + 1);
        band.patternWeight[kPatTrap] = band.patternWeight[kPatPatrol] = band.patternWeight[kPatCrossing] = 0;
    }
    const int strongest = band.strongest;
    const float* wt = band.patternWeight;  // Open, Choice, Trap, Slider, Wall, Pillars, Patrol, Crossing, Hopper, Breathing
    const int rows = r.rangei(band.rowsLo, band.rowsHi);
    float z = r.range(8, 10);
    float stream = tune.fireRate;  // the stream a player has at the current row (for barrier health)
    int prev = -1;
    for (int row = 0; row < rows; ++row) {
        if (z > 32.0f) break;
        // Pick a pattern: the first row is Open or Choice; no two Trap or Patrol rows in a row.
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
            break;
        }
        if (row == 0 && p != kPatOpen && p != kPatChoice) p = kPatChoice;
        const float s = r.f01() < 0.5f ? -1.0f : 1.0f;
        const int plain = r.rangei(2, strongest);
        const int special = minv(5, plain + 1);
        int best = plain;
        switch (p) {
            case kPatOpen:
                addGate(L, mulGate(r.range(-2.9f, 2.9f), 1.8f, z, plain, row));
                break;
            case kPatChoice: {
                const int b = r.rangei(3, maxv(3, strongest));
                const int a = r.rangei(2, b - 1);
                addGate(L, mulGate(s * r.range(2.5f, 2.9f), 1.8f, z, a, row));
                addGate(L, mulGate(-s * r.range(2.3f, 2.9f), 1.2f, z, b, row));
                best = b;
                break;
            }
            case kPatTrap:
                addGate(L, mulGate(s * 2.6f, 1.5f, z, plain, row));
                addGate(L, divGate(-s * 2.6f, 1.5f, z, index >= 25 && r.f01() < 0.5f ? 3 : 2, row));
                break;
            case kPatSlider: {
                Gate gs = mulGate(r.range(-0.5f, 0.5f), 1.6f, z, special, row);
                gs.slides = 1;
                gs.amp = minv(kHW - gs.hw - 0.3f - std::fabs(gs.cx), r.range(1.8f, 3.0f));
                gs.period = r.range(3, 6);
                gs.phase = r.f01();
                addGate(L, gs);
                best = special;
                break;
            }
            case kPatWall: {
                const float cx = s * 2.6f;
                addGate(L, mulGate(cx, 1.5f, z + 2.0f, special, row));
                const int weak = r.rangei(2, maxv(2, strongest - 1));
                addGate(L, mulGate(-s * 2.6f, 1.5f, z, weak, row));
                Barrier b;
                b.x0 = maxv(-kHW + 0.1f, cx - 1.6f);
                b.x1 = minv(kHW - 0.1f, cx + 1.6f);
                b.z = z;
                b.health = maxv<i32>(10, i32(1.5f * stream));
                if (index >= 46 && r.f01() < 0.4f) {
                    // A sliding wall (VARIETY_DESIGN 4 #3): narrower, sliding so a window to the prize opens and closes.
                    b.x0 = cx - 1.2f;
                    b.x1 = cx + 1.2f;
                    b.amp = minv(kHW - 0.1f - std::fabs(cx) - 1.2f, r.range(0.8f, 1.1f));
                    b.period = r.range(4.0f, 6.0f);
                    b.phase = r.f01();
                }
                if (L.barrierCount < kMaxBarriers) L.barriers[L.barrierCount++] = b;
                best = special;
                break;
            }
            case kPatPillars: {
                const int a = r.rangei(2, strongest), b = r.rangei(2, strongest);
                addGate(L, mulGate(-3.3f, 1.3f, z, a, row));
                addGate(L, mulGate(3.3f, 1.3f, z, b, row));
                if (L.bumperCount + 2 <= kMaxBumpers) {
                    L.bumpers[L.bumperCount++] = {-1.25f, z - 1.8f, 0.55f};
                    L.bumpers[L.bumperCount++] = {1.25f, z - 1.8f, 0.55f};
                }
                best = maxv(a, b);
                break;
            }
            case kPatPatrol: {
                const float cx = r.range(-2.6f, 2.6f);
                addGate(L, mulGate(cx, 1.5f, z, special, row));
                if (std::fabs(cx) > 1.0f) addGate(L, mulGate(cx > 0 ? -3.2f : 3.2f, 1.5f, z, 2, row));
                Snuffer sn;
                sn.xa = maxv(-kLauncherRange, cx - 2.2f);
                sn.xb = minv(kLauncherRange, cx + 2.2f);
                sn.z = z - 2.0f;
                sn.speed = r.range(2, 4);
                sn.phase = r.f01();
                if (L.snufferCount < kMaxSnuffers) L.snuffers[L.snufferCount++] = sn;
                best = special;
                break;
            }
            case kPatHopper: {  // a gate that jumps between two places (VARIETY_DESIGN 4 #1)
                Gate h = mulGate(r.range(-0.4f, 0.4f), 1.5f, z, special, row);
                h.slides = kGateHop;
                h.amp = minv(kHW - h.hw - 0.3f - std::fabs(h.cx), r.range(1.8f, 2.6f));
                h.period = r.range(1.5f, 2.5f);
                h.phase = r.f01();
                addGate(L, h);
                best = special;
                break;
            }
            case kPatBreathing: {  // a strong gate whose opening swings 60% to 120% (VARIETY_DESIGN 4 #2), a plain one aside
                const float cx = s * r.range(1.4f, 2.0f);
                Gate b = mulGate(cx, 1.6f, z, special, row);
                b.breath = r.range(3.0f, 5.0f);
                b.phase = r.f01();
                addGate(L, b);
                addGate(L, mulGate(-s * 3.4f, 1.2f, z, plain, row));
                best = special;
                break;
            }
            default: {  // Crossing sliders, 1.6 m apart in z so they pass each other instead of through each other
                const float period = r.range(3.5f, 5.5f), phase = r.f01();
                Gate a = mulGate(0, 1.4f, z + 0.8f, plain, row);
                a.slides = 1;
                a.amp = 2.6f;
                a.period = period;
                a.phase = phase;
                Gate d = divGate(0, 1.4f, z - 0.8f, 2, row);
                d.slides = 1;
                d.amp = 2.6f;
                d.period = period;
                d.phase = phase + 0.5f;
                addGate(L, a);
                addGate(L, d);
                p = kPatCrossing;
                break;
            }
        }
        addRow(L, z, Pattern(p));
        stream *= float(best);
        prev = p;
        z += r.range(6, 8);
    }
    const float last = L.rowZ[L.rowCount - 1] + (L.rowPattern[L.rowCount - 1] == kPatWall ? 2.0f : 0.0f);
    L.len = clampv(last + 12.0f, 30.0f, 44.0f);

    // The enemy (GAME_DESIGN 5.9, 7.3 step 3) and its plan (VARIETY_DESIGN 5.1); a boss's plan is its own (8).
    L.plan = u8(planFor(index, r));
    if (L.boss) {
        static const u8 bossPlan[kBossCount] = {kPlanSteady, kPlanSteady, kPlanRunnerPacks, kPlanPulse};
        L.plan = bossPlan[L.bossType];
    }
    if (L.kind == kTwins && (L.plan == kPlanPincer || L.plan == kPlanFlank)) L.plan = kPlanPulse;  // each spire has its emitter
    if (festival) L.plan = kPlanSteady;
    const int emitters = index <= 9 ? 1 : index <= 29 ? 2 : 3;
    if (festival) {
        // No enemy (VARIETY_DESIGN 6).
    } else if (L.kind == kTwins) {  // an emitter at each spire's foot; it stops when its spire falls
        L.twinShare = r.f01() < 0.5f ? 0.5f : (r.f01() < 0.5f ? 0.4f : 0.6f);
        addEmitter(L, -tune.twinX);
        addEmitter(L, tune.twinX);
        L.emitters[1].group = 1;
    } else if (L.plan == kPlanPincer) {  // two emitters far apart, one at a time
        const float x = r.range(2.8f, 3.4f);
        addEmitter(L, -x);
        addEmitter(L, x);
    } else if (emitters == 1) {
        addEmitter(L, r.range(-2.0f, 2.0f));
    } else if (emitters == 2) {
        const float x = r.range(1.5f, 2.5f);
        addEmitter(L, -x);
        addEmitter(L, x);
    } else {
        addEmitter(L, -2.5f);
        addEmitter(L, 0);
        addEmitter(L, 2.5f);
    }
    if (index >= 20 && L.plan != kPlanPincer && L.emitterCount > 0 && r.f01() < 0.4f) {
        Emitter& e = L.emitters[r.rangei(0, L.emitterCount - 1)];
        e.amp = r.range(1.0f, 1.5f);
        e.period = r.range(4, 7);
        e.x = clampv(e.x, -2.5f + e.amp, 2.5f - e.amp);
    }
    if (L.plan == kPlanFlank) {
        // A third of the flow through one or two side doors, between z 14 and len - 8, clear of the rows by 1.5 m.
        const int foot = L.emitterCount, doors = r.f01() < 0.5f ? 1 : 2;
        for (int e = 0; e < foot; ++e) L.emitters[e].share = (2.0f / 3.0f) / float(foot);
        const int side = r.f01() < 0.5f ? -1 : 1;
        for (int d = 0; d < doors; ++d) {
            float dz = 0;
            for (int tries = 0; tries < 16 && dz == 0; ++tries) {
                const float c = r.range(14.0f, maxv(14.5f, L.len - 8.0f));
                bool clear = true;
                for (int i = 0; i < L.rowCount; ++i) clear = clear && std::fabs(c - L.rowZ[i]) >= 1.5f;
                if (clear) dz = c;
            }
            if (dz == 0) dz = maxv(14.0f, L.len - 8.0f);
            addDoor(L, d == 0 ? side : -side, dz, (1.0f / 3.0f) / float(doors));
        }
    }
    if (L.boss) addBossScript(L);
    applyEnemyDefaults(L);
    if (festival) placeFestivalGeodes(L, r);
    else placeGeodes(L, r);
    addTwists(L, r);
    return L;
}

}  // namespace gt::sim

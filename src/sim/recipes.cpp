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
    bool used[kBoonCount] = {};
    float worth = 1, t = r.range(0.08f, 0.25f) * T;
    for (int k = 0; k < n && L.geodeCount < kMaxGeodes; ++k) {
        if (k > 0) t += r.range(5.0f, 8.0f);
        if (L.boss && k == 1) t = minv(t, maxv(L.geodes[0].t + 5.0f, 0.4f * T));
        // The boons this geode may hold: from their first level, under their limits and the budget, at most three kinds a
        // level; the first geode up to level 60 lasts; a WARD at most once, never first, never at 75% of T or later.
        const float late = t > 0.75f * T;
        float wt[kBoonCount] = {};
        auto allow = [&](int b, int from, float weight) {
            if (index < from || (!used[b] && kinds >= 3)) return;
            const float gain = boonGain(b, lanterns, quicken, bright);
            if (gain <= 0 || late || worth * (1.0f + gain) > budget + 1e-3f) return;
            wt[b] = weight;
        };
        allow(kBoonLantern, 6, k == 0 ? 4.0f : 3.0f);  // the first one is a detour: lean it toward a boon worth one
        allow(kBoonQuicken, 7, k == 0 ? 1.5f : 3.0f);
        allow(kBoonBright, 12, k == 0 ? 3.5f : 2.5f);
        if (index >= 23 && wards == 0 && (k > 0 || index > 60) && t < 0.85f * T && (used[kBoonWard] || kinds < 3)) wt[kBoonWard] = 1.5f;
        float total = 0;
        for (float x : wt) total += x;
        if (total <= 0) break;
        float pick = r.range(0, total);
        int boon = 0;
        while (boon < kBoonCount - 1 && pick >= wt[boon]) pick -= wt[boon++];
        if (wt[boon] <= 0) break;
        if (!used[boon]) used[boon] = true, ++kinds;
        worth *= 1.0f + boonGain(boon, lanterns, quicken, bright);
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

LevelDef generate(int index, u64 seed) {
    LevelDef L;
    L.index = index;
    L.seed = seed;
    L.world = worldFor(index);
    L.targetT = targetTime(index);
    L.hardness = hardnessFor(index);
    L.fight = fightFor(index) + (L.hardness - 0.5f) * 0.08f;  // a world's rhythm moves the enemy's share (VARIETY_DESIGN 10.2)
    L.boss = index % 10 == 0;
    Rng r(seed, 7);

    const Band band = bandFor(index);
    const int strongest = band.strongest;
    const float* wt = band.patternWeight;  // Open, Choice, Trap, Slider, Wall, Pillars, Patrol, Crossing
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
            for (int k = 0; k < 8; ++k) total += wt[k];
            float pick = r.range(0, total);
            p = 0;
            while (p < 7 && pick >= wt[p]) pick -= wt[p++];
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

    // The enemy (GAME_DESIGN 5.9, 7.3 step 3).
    const int emitters = index <= 9 ? 1 : index <= 29 ? 2 : 3;
    if (emitters == 1) {
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
    if (index >= 20 && r.f01() < 0.4f) {
        Emitter& e = L.emitters[r.rangei(0, L.emitterCount - 1)];
        e.amp = r.range(1.0f, 1.5f);
        e.period = r.range(4, 7);
        e.x = clampv(e.x, -2.5f + e.amp, 2.5f - e.amp);
    }
    addBurst(L, 0.35f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
    addBurst(L, 0.70f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
    if (L.boss) {
        addBurst(L, -1, 0.66f, tune.burstMult, 4.0f, 2);
        addBurst(L, -1, 0.33f, tune.burstMult, 4.0f, 2);
    }
    applyEnemyDefaults(L);
    placeGeodes(L, r);
    return L;
}

}  // namespace gt::sim

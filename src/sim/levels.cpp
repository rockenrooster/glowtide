// Levels 1-5 (GAME_DESIGN 7.2), the generator (7.3), and the numbers the bake fixed (7.4, levels_baked.h).
#include <cmath>

#include "core/rng.h"
#include "core/tuning.h"
#include "sim/level.h"

namespace gt::sim {

namespace {

struct BakedLevel {
    u64 seed;
    i32 spire;
    float baseFlow;
};
#include "sim/levels_baked.h"

Gate mul(float cx, float hw, float z, int k, int row) {
    Gate g;
    g.cx = cx;
    g.hw = hw;
    g.z = z;
    g.kind = kGateMul;
    g.k = u8(k);
    g.row = u8(row);
    return g;
}

Gate divg(float cx, float hw, float z, int k, int row) {
    Gate g = mul(cx, hw, z, k, row);
    g.kind = kGateDiv;
    return g;
}

void addGate(LevelDef& L, const Gate& g) {
    if (L.gateCount < kMaxGates) L.gates[L.gateCount++] = g;
}

void addRow(LevelDef& L, float z, Pattern p) {
    if (L.rowCount < kMaxRows) {
        L.rowZ[L.rowCount] = z;
        L.rowPattern[L.rowCount] = p;
        ++L.rowCount;
    }
}

void addEmitter(LevelDef& L, float x, float amp = 0, float period = 5, float phase = 0) {
    if (L.emitterCount < kMaxEmitters) L.emitters[L.emitterCount++] = {x, amp, period, phase};
}

void addBurst(LevelDef& L, float t, float frac, float mult, float dur, int brutes) {
    if (L.burstCount < kMaxBursts) L.bursts[L.burstCount++] = {t, frac, mult, dur, brutes};
}

// What a straight, competent stream delivers a second on this layout with no enemy (a first guess used before the
// bake has measured the level).
float estimateP0(const LevelDef& L) {
    float best = 1;
    for (int r = 0; r < L.rowCount; ++r) {
        int k = 1;
        for (int i = 0; i < L.gateCount; ++i)
            if (L.gates[i].row == r && L.gates[i].kind == kGateMul) k = maxv(k, int(L.gates[i].k));
        best *= float(k);
    }
    return tune.fireRate * best * 0.8f;
}

void applyEnemyDefaults(LevelDef& L) {
    L.coreHealth = int(tune.coreHealth);
    if (!L.hasEnemy()) return;
    if (L.index >= int(tune.bruteFromLevel)) {
        L.bruteEvery = tune.bruteEvery;
        L.brutePower = int(minv(tune.brutePowerMax, tune.brutePowerBase + tune.brutePowerPerLevel * float(L.index)));
    }
}

}  // namespace

float targetTime(int index) {
    static const float intro[5] = {8, 14, 18, 22, 26};
    if (index >= 1 && index <= 5) return intro[index - 1];
    const float t = 24.0f + 0.4f * float(minv(index, 30));
    return index % 10 == 0 ? t * 1.4f : t;
}

float fightFor(int index) { return tune.fightBase + tune.fightPerLevel * float(minv(float(index), tune.fightLevelCap)); }

int worldFor(int index) { return ((index - 1) / 10) % 5 + 1; }

u64 defaultSeed(int index) { return hash64(u64(index) * 7919u + 17u) & 0xFFFFFFFFull; }

int levelForPlay(int index) {
    if (index <= 300) return index;
    return 101 + (index - 301) % 200;
}

LevelDef introLevel(int index) {
    LevelDef L;
    L.index = index;
    L.seed = defaultSeed(index);
    L.world = 1;
    L.targetT = targetTime(index);
    L.fight = fightFor(index);
    switch (index) {
        case 1:
            L.len = 30;
            addRow(L, 12, kPatHand);
            addGate(L, mul(0, 1.8f, 12, 2, 0));
            L.spireHealth = 60;
            break;
        case 2:
            L.len = 30;
            addRow(L, 10, kPatHand);
            addGate(L, mul(-2.5f, 1.8f, 10, 2, 0));
            addGate(L, mul(2.5f, 1.8f, 10, 3, 0));
            addEmitter(L, 0);
            break;
        case 3:
            L.len = 36;
            addRow(L, 9, kPatHand);
            addRow(L, 17, kPatHand);
            addGate(L, mul(0, 1.8f, 9, 2, 0));
            addGate(L, mul(0, 1.5f, 17, 3, 1));
            addEmitter(L, 2);
            break;
        case 4: {
            L.len = 36;
            addRow(L, 9, kPatHand);
            addRow(L, 17, kPatHand);
            addGate(L, mul(-2.7f, 1.5f, 9, 3, 0));
            addGate(L, divg(2.7f, 1.5f, 9, 2, 0));
            Gate s = mul(0, 1.6f, 17, 2, 1);
            s.slides = 1;
            s.amp = 3.0f;
            s.period = 4.0f;
            addGate(L, s);
            addEmitter(L, 2);
            break;
        }
        default: {  // 5
            L.len = 34;
            addRow(L, 8, kPatHand);
            addRow(L, 15, kPatHand);
            addRow(L, 22, kPatHand);
            addGate(L, mul(0, 1.8f, 8, 2, 0));
            addGate(L, mul(-2.6f, 1.5f, 15, 2, 1));
            addGate(L, mul(2.6f, 1.5f, 15, 4, 1));
            L.barriers[L.barrierCount++] = {1.0f, 4.2f, 13.0f, 40};
            addGate(L, mul(0, 1.2f, 22, 2, 2));
            L.bumpers[L.bumperCount++] = {-3.1f, 22.0f, 0.7f};
            L.bumpers[L.bumperCount++] = {3.1f, 22.0f, 0.7f};
            addEmitter(L, -2);
            addEmitter(L, 2);
            addBurst(L, -1, 0.5f, tune.burstMult, tune.burstSeconds, 0);
            break;
        }
    }
    applyEnemyDefaults(L);
    return L;
}

LevelDef generate(int index, u64 seed) {
    LevelDef L;
    L.index = index;
    L.seed = seed;
    L.world = worldFor(index);
    L.targetT = targetTime(index);
    L.fight = fightFor(index);
    L.boss = index % 10 == 0;
    Rng r(seed, 7);

    // Bands (GAME_DESIGN 7.3).
    int rowsLo, rowsHi, strongest;
    float wt[8];  // Open, Choice, Trap, Slider, Wall, Pillars, Patrol, Crossing
    auto setW = [&](float a, float b, float c, float d, float e, float f, float gg, float h) {
        wt[0] = a; wt[1] = b; wt[2] = c; wt[3] = d; wt[4] = e; wt[5] = f; wt[6] = gg; wt[7] = h;
    };
    if (index < 10) {
        rowsLo = 2; rowsHi = 3; strongest = 3;
        setW(30, 30, 15, 15, 5, 5, 0, 0);
    } else if (index < 20) {
        rowsLo = rowsHi = 3; strongest = 4;
        setW(15, 25, 15, 20, 10, 5, 10, 0);
    } else if (index < 40) {
        rowsLo = 3; rowsHi = 4; strongest = 5;
        setW(10, 20, 15, 20, 12, 8, 10, 5);
    } else if (index < 100) {
        rowsLo = rowsHi = 4; strongest = 5;
        setW(5, 20, 15, 20, 12, 8, 12, 8);
    } else {
        rowsLo = 4; rowsHi = 5; strongest = 5;
        setW(5, 15, 15, 20, 15, 8, 12, 10);
    }
    const int rows = r.rangei(rowsLo, rowsHi);
    float z = r.range(8, 10);
    float stream = tune.fireRate;  // the stream a player has at the current row (for barrier health)
    int prev = -1;
    for (int row = 0; row < rows; ++row) {
        if (z > 32.0f) break;
        // Pick a pattern: the first row is Open or Choice; no two Trap or Patrol rows in a row.
        int p = 0;
        for (int tries = 0; tries < 20; ++tries) {
            float total = 0;
            for (float x : wt) total += x;
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
                addGate(L, mul(r.range(-2.9f, 2.9f), 1.8f, z, plain, row));
                break;
            case kPatChoice: {
                const int b = r.rangei(3, maxv(3, strongest));
                const int a = r.rangei(2, b - 1);
                addGate(L, mul(s * r.range(2.5f, 2.9f), 1.8f, z, a, row));
                addGate(L, mul(-s * r.range(2.3f, 2.9f), 1.2f, z, b, row));
                best = b;
                break;
            }
            case kPatTrap:
                addGate(L, mul(s * 2.6f, 1.5f, z, plain, row));
                addGate(L, divg(-s * 2.6f, 1.5f, z, index >= 25 && r.f01() < 0.5f ? 3 : 2, row));
                break;
            case kPatSlider: {
                Gate gs = mul(r.range(-0.5f, 0.5f), 1.6f, z, special, row);
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
                addGate(L, mul(cx, 1.5f, z + 2.0f, special, row));
                const int weak = r.rangei(2, maxv(2, strongest - 1));
                addGate(L, mul(-s * 2.6f, 1.5f, z, weak, row));
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
                addGate(L, mul(-3.3f, 1.3f, z, a, row));
                addGate(L, mul(3.3f, 1.3f, z, b, row));
                if (L.bumperCount + 2 <= kMaxBumpers) {
                    L.bumpers[L.bumperCount++] = {-1.25f, z - 1.8f, 0.55f};
                    L.bumpers[L.bumperCount++] = {1.25f, z - 1.8f, 0.55f};
                }
                best = maxv(a, b);
                break;
            }
            case kPatPatrol: {
                const float cx = r.range(-2.6f, 2.6f);
                addGate(L, mul(cx, 1.5f, z, special, row));
                if (std::fabs(cx) > 1.0f) addGate(L, mul(cx > 0 ? -3.2f : 3.2f, 1.5f, z, 2, row));
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
                Gate a = mul(0, 1.4f, z + 0.8f, plain, row);
                a.slides = 1;
                a.amp = 2.6f;
                a.period = period;
                a.phase = phase;
                Gate d = divg(0, 1.4f, z - 0.8f, 2, row);
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
    return L;
}

LevelDef layoutFor(int index, u64 seed) { return index <= 5 ? introLevel(index) : generate(index, seed); }

LevelDef levelFor(int playIndex) {
    const int index = levelForPlay(maxv(1, playIndex));
    const bool baked = index >= 1 && index <= kBakedCount && kBaked[index - 1].spire > 0;
    LevelDef L = layoutFor(index, baked ? kBaked[index - 1].seed : defaultSeed(index));
    if (index == 1) return L;
    if (baked) {
        L.spireHealth = kBaked[index - 1].spire;
        L.baseFlow = kBaked[index - 1].baseFlow;
    } else {
        // Before the bake: a first guess from the layout (the bake replaces it).
        const float p0 = estimateP0(L);
        L.baseFlow = L.fight * p0;
        L.spireHealth = maxv<i32>(40, i32(0.85f * p0 * L.targetT * (1.0f - L.fight)));
    }
    return L;
}

}  // namespace gt::sim

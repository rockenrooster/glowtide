// The levels the game plays: hand-made (levels_hand.cpp) or generated (recipes.cpp), with the numbers the bake fixed
// (GAME_DESIGN 7.4, VARIETY_DESIGN 12.2; levels_baked.h).
#include <cmath>

#include "core/rng.h"
#include "sim/level_build.h"

namespace gt::sim {

namespace {

struct BakedLevel {
    u64 seed;
    i32 spire;
    float baseFlow;
    float ref;  // R: geode health is a share of it
};
#include "sim/levels_baked.h"

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

}  // namespace

int bakedGeneratorVersion() { return kBakedGeneratorVersion; }
int bakedRulesVersion() { return kBakedRulesVersion; }

i32 twoSignificant(float v) {
    if (v < 10) return maxv<i32>(1, i32(v + 0.5f));
    float mag = 1;
    while (v / mag >= 100.0f) mag *= 10.0f;
    return i32(std::floor(v / mag + 0.5f) * mag);
}

float columnMult(const LevelDef& L, float x, float z) {
    float m = 1;
    for (int i = 0; i < L.gateCount; ++i) {
        const Gate& G = L.gates[i];
        const float reach = G.slides ? G.amp : 0.0f;
        if (G.z >= z || std::fabs(x - G.cx) > G.hw + reach) continue;
        const float share = G.slides ? G.hw / (G.hw + G.amp) : 1.0f;
        m *= G.kind == kGateMul ? 1.0f + share * float(G.k - 1) : 1.0f - share * (1.0f - 1.0f / float(G.k));
    }
    return m;
}

float fanSpread(const LevelDef& L, float x, float z) {
    float s = 0.4f;
    for (int i = 0; i < L.gateCount; ++i) {
        const Gate& G = L.gates[i];
        if (G.kind != kGateMul || G.z >= z || std::fabs(x - G.cx) > G.hw + (G.slides ? G.amp : 0.0f)) continue;
        s += (tune.fanBase + tune.fanPerK * float(G.k)) * tune.fanDecay;
    }
    return s;
}

void sizeGeodes(LevelDef& L) {
    for (int i = 0; i < L.geodeCount; ++i) {
        Geode& G = L.geodes[i];
        // The stream fired as it appears meets it lower down (it drifts while the stream climbs): the gates below there,
        // and only the share of that fanned-out stream that the geode, or its plate, is wide enough to catch.
        const float z = 0.86f * (G.z0 > 0 ? G.z0 : L.len - 3.0f);
        const float stream = tune.fireRate * columnMult(L, G.x, z), width = 2.0f * fanSpread(L, G.x, z);
        G.hp = twoSignificant(maxv(1.0f, G.hpSeconds * stream * minv(1.0f, 2.0f * G.radius / width)));
        G.shield = G.shieldSeconds > 0 ? twoSignificant(maxv(1.0f, G.shieldSeconds * stream * minv(1.0f, tune.plateWidth / width))) : 0;
    }
}

float boonGain(int boon, int lanterns, int quicken, int bright) {
    switch (boon) {
        case kBoonLantern: return lanterns >= int(tune.lanternMax) ? 0.0f : float(lanterns + 1) / float(lanterns) - 1.0f;
        case kBoonQuicken:
            return quicken >= int(tune.quickenMax) ? 0.0f
                                                   : (1.0f + tune.quickenStep * float(quicken + 1)) / (1.0f + tune.quickenStep * float(quicken)) - 1.0f;
        case kBoonBright: return bright >= int(tune.brightMax) ? 0.0f : float(2 + bright) / float(1 + bright) - 1.0f;
        default: return 0.0f;
    }
}

u64 defaultSeed(int index) { return hash64(u64(index) * 7919u + 17u) & 0xFFFFFFFFull; }

int levelForPlay(int index) {
    if (index <= 300) return index;
    return 101 + (index - 301) % 200;
}

LevelDef layoutFor(int index, u64 seed) {
    LevelDef L = isHandMade(index) ? introLevel(index) : generate(index, seed);
    sizeGeodes(L);
    return L;
}

LevelDef levelFor(int playIndex) {
    const int index = levelForPlay(maxv(1, playIndex));
    const bool baked = index >= 1 && index <= kBakedCount && kBaked[index - 1].spire > 0;
    LevelDef L = layoutFor(index, baked ? kBaked[index - 1].seed : defaultSeed(index));
    if (index == 1) return L;
    if (baked) {
        L.spireHealth = kBaked[index - 1].spire;
        L.baseFlow = kBaked[index - 1].baseFlow;
        L.ref = kBaked[index - 1].ref;
    } else {
        // Before the bake: a first guess from the layout (the bake replaces it).
        const float p0 = estimateP0(L);
        L.baseFlow = L.fight * p0;
        L.spireHealth = maxv<i32>(40, i32(0.85f * p0 * L.targetT * (1.0f - L.fight)));
        L.ref = p0 * L.targetT;
    }
    return L;
}

}  // namespace gt::sim

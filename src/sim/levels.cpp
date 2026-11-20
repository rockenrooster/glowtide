// The levels the game plays: hand-made (levels_hand.cpp) or generated (recipes.cpp), with the numbers the bake fixed
// (GAME_DESIGN 7.4, levels_baked.h).
#include "core/rng.h"
#include "sim/level_build.h"

namespace gt::sim {

namespace {

struct BakedLevel {
    u64 seed;
    i32 spire;
    float baseFlow;
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

u64 defaultSeed(int index) { return hash64(u64(index) * 7919u + 17u) & 0xFFFFFFFFull; }

int levelForPlay(int index) {
    if (index <= 300) return index;
    return 101 + (index - 301) % 200;
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

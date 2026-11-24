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
    float ref;         // R: geode health is a share of it
    i32 recipe = -1;   // the recipe the bake chose (-1: the calendar's draw from the seed)
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
int bakedLevelCount() { return kBakedCount; }
bool bakedRow(int index, u64& seed, i32& spire, float& baseFlow, float& ref, int& recipe) {
    if (index < 1 || index > kBakedCount) return false;
    const BakedLevel& b = kBaked[index - 1];
    seed = b.seed, spire = b.spire, baseFlow = b.baseFlow, ref = b.ref, recipe = b.recipe;
    return true;
}
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

float kindleGain(const LevelDef& L, float x) {
    float m = 1;
    for (int i = 0; i < L.gateCount; ++i) {
        const Gate& G = L.gates[i];
        if (G.kind != kGateMul || std::fabs(x - G.cx) > G.hw + (G.slides ? G.amp : 0.0f)) continue;
        const float share = G.slides ? G.hw / (G.hw + G.amp) : 1.0f;
        const float k = float(G.k), k1 = float(minv(int(tune.kindleMax), int(G.k) + 1));
        m *= (1.0f + share * (k1 - 1.0f)) / (1.0f + share * (k - 1.0f));
    }
    return m - 1.0f;
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
    if (index <= kLastBaked) return index;
    return 101 + (index - kLastBaked - 1) % (kLastBaked - 100);
}

bool isReplay(int index) { return index > kLastBaked; }

// The table's goal number by kind (VARIETY_DESIGN 12.2 step 6): Twins split it between the spires; a festival's is the
// geodes opened for three stars; a WARD WALL and a cub are shares of a boss's spire.
void setGoals(LevelDef& L) {
    if (L.kind == kTwins) {
        const i32 total = L.spireHealth;
        L.spireHealth = twoSignificant(float(total) * L.twinShare);
        L.goal2 = maxv<i32>(1, twoSignificant(float(total) - float(L.spireHealth)));
    } else if (L.kind == kFestival) {
        L.goal2 = maxv<i32>(1, L.spireHealth);
        L.spireHealth = 1;
    }
    for (int b = 0; b < L.barrierCount; ++b)
        if (L.barriers[b].dormant) L.barriers[b].health = maxv<i32>(10, twoSignificant(tune.wardWallShare * float(L.spireHealth)));
}

LevelDef layoutFor(int index, u64 seed, int recipe) {
    LevelDef L = isHandMade(index) ? introLevel(index) : recipe >= 0 ? generateWith(index, seed, recipe) : generate(index, seed);
    sizeGeodes(L);
    return L;
}

LevelDef levelFor(int playIndex) {
    const int index = levelForPlay(maxv(1, playIndex));
    // A row's layout is the level's even when the bake left it without numbers (spire 0: no strength of the enemy let
    // greedy win); its numbers are then the first guess below, as before the bake.
    const bool row = index >= 1 && index <= kBakedCount;
    const bool baked = row && kBaked[index - 1].spire > 0;
    LevelDef L = layoutFor(index, row ? kBaked[index - 1].seed : defaultSeed(index), row ? kBaked[index - 1].recipe : -1);
    if (isReplay(playIndex)) {  // VARIETY_DESIGN 16: mirrored, in another world; the rules are symmetric, so the numbers hold
        mirrorLayout(L);
        L.world = (L.world - 1 + kReplayWorldShift) % kWorldCount + 1;
    }
    if (index == 1) return L;
    if (baked) {
        L.spireHealth = kBaked[index - 1].spire;
        L.baseFlow = kBaked[index - 1].baseFlow;
        L.ref = kBaked[index - 1].ref;
        setGoals(L);
    } else {
        // Before the bake: a first guess from the layout (the bake replaces it).
        const float p0 = estimateP0(L);
        L.baseFlow = L.fight * p0;
        L.spireHealth = maxv<i32>(40, i32(0.85f * p0 * L.targetT * (1.0f - L.fight)));
        L.ref = p0 * L.targetT;
        if (L.kind == kFestival) L.spireHealth = maxv(1, L.geodeCount - 2);
        setGoals(L);
    }
    return L;
}

}  // namespace gt::sim

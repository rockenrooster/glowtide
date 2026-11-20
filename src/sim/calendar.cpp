// What a level number may use (VARIETY_DESIGN 10): its world, its target time, the enemy's share and the generator's
// band (GAME_DESIGN 7.3).
#include "sim/level_build.h"

namespace gt::sim {

namespace {
template <int N>
const char* nameIn(const char* const (&names)[N], int i) {
    return i >= 0 && i < N ? names[i] : "?";
}
}  // namespace

const char* kindName(int kind) {
    static const char* const n[kKindCount] = {"Siege", "Holdout", "Colossus", "Twins", "Festival", "Boss"};
    return nameIn(n, kind);
}
const char* lengthClassName(int c) {
    static const char* const n[kLengthCount] = {"Sprint", "Standard", "Long"};
    return nameIn(n, c);
}
const char* planName(int plan) {
    static const char* const n[kPlanCount] = {"Steady", "Pulse", "Pincer", "Late surge", "Early rush", "Flank", "Brute line", "Runner packs"};
    return nameIn(n, plan);
}
const char* twistName(int twist) {
    static const char* const n[kTwistCount] = {"SURGE",  "BRUTE GUARD", "GEODE RAIN", "ECLIPSE",   "STAMPEDE",
                                              "SECOND WIND", "GOLDEN GATE", "GATE STORM", "WARD WALL", "CUB"};
    return nameIn(n, twist);
}
const char* boonName(int boon) {
    static const char* const n[kBoonCount] = {"LANTERN", "QUICKEN", "BRIGHT", "WARD", "KINDLE", "COMET", "HUSH"};
    return nameIn(n, boon);
}

float targetTime(int index) {
    static const float intro[5] = {8, 14, 18, 22, 26};
    if (index >= 1 && index <= 5) return intro[index - 1];
    const float t = 24.0f + 0.4f * float(minv(index, 30));
    return index % 10 == 0 ? t * 1.4f : t;
}

float fightFor(int index) { return tune.fightBase + tune.fightPerLevel * float(minv(float(index), tune.fightLevelCap)); }

int worldFor(int index) { return ((index - 1) / 10) % 5 + 1; }

float hardnessFor(int index) {
    if (index <= 5) return 0.2f;
    // A world's rhythm (VARIETY_DESIGN 10.2): its opening, rising, a breather at 5, rising again, the hard one, the boss.
    static const float byPlace[10] = {0.2f, 0.4f, 0.5f, 0.6f, 0.4f, 0.5f, 0.65f, 0.8f, 1.0f, 0.8f};
    return byPlace[(index - 1) % 10];
}

bool isHandMade(int index) { return index <= 6 || index == 11 || index == 16 || index == 23 || index == 38; }

int planFrom(int plan) {
    static const int from[kPlanCount] = {2, 18, 22, 27, 34, 28, 37, 30};
    return plan >= 0 && plan < kPlanCount ? from[plan] : 1 << 20;
}

// A plan whose first level has come, Steady a little more often; on a plan's first level, that plan (VARIETY_DESIGN 5.1).
int planFor(int index, Rng& r) {
    for (int p = 1; p < kPlanCount; ++p)
        if (index == planFrom(p)) return p;
    float wt[kPlanCount] = {}, total = 0;
    for (int p = 0; p < kPlanCount; ++p)
        if (index >= planFrom(p)) total += wt[p] = p == kPlanSteady ? 1.5f : 1.0f;
    float pick = r.range(0, total);
    int p = 0;
    while (p < kPlanCount - 1 && pick >= wt[p]) pick -= wt[p++];
    return p;
}

Band bandFor(int index) {
    Band b;
    auto set = [&](int lo, int hi, int strongest, float a, float c, float t, float s, float w, float p, float x, float n) {
        b.rowsLo = lo;
        b.rowsHi = hi;
        b.strongest = strongest;
        const float wt[8] = {a, c, t, s, w, p, x, n};
        for (int i = 0; i < 8; ++i) b.patternWeight[i] = wt[i];
        // Hoppers and breathing gates after the levels that introduce them (16, 38), as often as a slider.
        b.patternWeight[kPatHopper] = index > 16 ? 0.6f * s : 0.0f;
        b.patternWeight[kPatBreathing] = index > 38 ? 0.6f * s : 0.0f;
    };
    if (index < 10) set(2, 3, 3, 30, 30, 15, 15, 5, 5, 0, 0);
    else if (index < 20) set(3, 3, 4, 15, 25, 15, 20, 10, 5, 10, 0);
    else if (index < 40) set(3, 4, 5, 10, 20, 15, 20, 12, 8, 10, 5);
    else if (index < 100) set(4, 4, 5, 5, 20, 15, 20, 12, 8, 12, 8);
    else set(4, 5, 5, 5, 15, 15, 20, 15, 8, 12, 10);
    return b;
}

// Brutes and the time bursts are a matter of the plan and the twists now (VARIETY_DESIGN 5.2): the brute's power is set
// for the bursts that bring brutes (the bosses').
void applyEnemyDefaults(LevelDef& L) {
    L.coreHealth = int(tune.coreHealth);
    if (!L.hasEnemy()) return;
    L.bruteEvery = 0;
    L.brutePower = int(minv(tune.brutePowerMax, tune.brutePowerBase + tune.brutePowerPerLevel * float(L.index)));
}

}  // namespace gt::sim

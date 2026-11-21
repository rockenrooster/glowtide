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
const char* bossName(int boss) {
    static const char* const n[kBossCount] = {"THE OLD SPIRE", "THE WARDEN", "THE TEMPEST", "THE HYDRA"};
    return nameIn(n, boss);
}
const char* boonName(int boon) {
    static const char* const n[kBoonCount] = {"LANTERN", "QUICKEN", "BRIGHT", "WARD", "KINDLE", "COMET", "HUSH"};
    return nameIn(n, boon);
}

namespace {
// A level that introduces something other than a kind (VARIETY_DESIGN 10.1): a recipe, a twist, a boon, a hand-made
// lesson or a set piece. A kind's slot moves off it, so the new thing comes on a plain siege.
bool introducesSomething(int index) {
    return introRecipe(index) >= 0 || introTwist(index) >= 0 || index == 7 || index == 12 || index == 36 || index == 52 || index == 63 ||
           index == 76 ||
           (isHandMade(index) && index != 22 && index != 32 && index != 42) || setPieceAt(index) >= 0;
}

// The place (2, 3, 7 or 8) of world w's slot `which` (0 or 1) for Holdout, Colossus or Twins, or 0 when it has none.
// Never next to a festival (5) or a boss (10, and the next world's 1), so no kind but Siege comes twice in a row. The
// places alternate between worlds so they do not all look alike; worlds 3 to 5 open on place 2, where the kind they
// introduce stands (22, 32, 42).
int slotPlace(int w, int which) {
    if (w < 2) return 0;
    if (which == 0 && w <= 4) return 2;
    const int a = which == 0 ? (w % 2 ? 2 : 3) : (w % 2 ? 8 : 7);
    const int b = which == 0 ? 5 - a : 15 - a;
    if (!introducesSomething(w * 10 + a)) return a;
    if (!introducesSomething(w * 10 + b)) return b;
    return 0;
}
}  // namespace

// The kinds (VARIETY_DESIGN 6, 10): festivals on 5 from 15, bosses on 0, and a world's two slots (slotPlace) shared out
// among Holdout, Colossus and Twins by their weights 10 : 8 : 7 in a smooth weighted turn, each from the level that
// introduces it. VARIETY_DESIGN 10.1 put those at 21, 31 and 41, right after a boss: they moved one level on (10.1 allows
// three), so no kind but Siege comes twice in a row.
int kindFor(int index) {
    if (index <= 5) return kSiege;
    if (index % 10 == 0) return kBoss;
    if (index % 10 == 5 && index >= 15) return kFestival;
    const int w = (index - 1) / 10, place = (index - 1) % 10 + 1;
    if (place != slotPlace(w, 0) && place != slotPlace(w, 1)) return kSiege;
    static const int kinds[3] = {kHoldout, kColossus, kTwins};
    static const float weight[3] = {10, 8, 7};
    static const int from[3] = {22, 32, 42};
    float credit[3] = {};
    int pick = 0;
    for (int wi = 2; wi <= w; ++wi)
        for (int s = 0; s < 2; ++s) {
            const int p = slotPlace(wi, s), i = wi * 10 + p;
            if (!p || i > index) continue;
            float total = 0;
            for (int k = 0; k < 3; ++k) total += i >= from[k] ? weight[k] : 0;
            int best = -1;
            for (int k = 0; k < 3; ++k) {
                if (i < from[k]) continue;
                credit[k] += weight[k] / total;
                if (best < 0 || credit[k] > credit[best]) best = k;
            }
            for (int k = 0; k < 3; ++k) best = i == from[k] ? k : best;  // an introduction is its kind
            credit[best] -= 1.0f;
            pick = best;
        }
    return kinds[pick];
}

// Lengths (VARIETY_DESIGN 9.1, 10.2): Sprint lanes at a world's places 1 and 6, Long lanes at 4, 8 or 9 (one a world,
// from level 61, which introduces them); Standard otherwise.
int lengthClassFor(int index) {
    if (index <= 10 || isHandMade(index)) return kStandard;  // a hand-made lane is a Standard one
    const int place = (index - 1) % 10 + 1, w = (index - 1) / 10;
    const int kind = kindFor(index);
    if (kind == kFestival || kind == kBoss) return kStandard;
    if (place == 1 || place == 6) return kSprint;
    static const int longPlace[3] = {4, 8, 9};
    if (index >= 61 && place == longPlace[w % 3]) return kLong;
    if (index == 61) return kLong;
    return kStandard;
}

// The target time by length class and kind (VARIETY_DESIGN 9.1): the class's time grows until level 30.
float targetTime(int index, int lengthClass) {
    static const float intro[5] = {8, 14, 18, 22, 26};
    if (index >= 1 && index <= 5) return intro[index - 1];
    const int kind = kindFor(index);
    if (kind == kFestival) return tune.festivalSeconds;
    static const float byClass[kLengthCount] = {22, 32, 44};
    const float ramp = 0.8f + 0.2f * float(minv(index, 30)) / 30.0f;
    const float cls = byClass[clampv(lengthClass, 0, kLengthCount - 1)] * ramp;
    switch (kind) {
        case kHoldout: return 40.0f * ramp;
        case kTwins: return 1.2f * cls;
        case kBoss: return 1.4f * byClass[kStandard] * ramp;
        default: return cls;
    }
}

float targetTime(int index) {
    static const float intro[5] = {8, 14, 18, 22, 26};
    if (index >= 1 && index <= 5) return intro[index - 1];
    if (index > 5) return targetTime(index, lengthClassFor(index));
    if (kindFor(index) == kFestival) return tune.festivalSeconds;
    const float t = 24.0f + 0.4f * float(minv(index, 30));
    return index % 10 == 0 ? t * 1.4f : t;
}

float fightFor(int index) { return tune.fightBase + tune.fightPerLevel * float(minv(float(index), tune.fightLevelCap)); }

int worldFor(int index) { return ((index - 1) / 10) % kWorldCount + 1; }

float hardnessFor(int index) {
    if (index <= 5) return 0.2f;
    // A world's rhythm (VARIETY_DESIGN 10.2): its opening, rising, a breather at 5, rising again, the hard one, the boss.
    static const float byPlace[10] = {0.2f, 0.4f, 0.5f, 0.6f, 0.4f, 0.5f, 0.65f, 0.8f, 1.0f, 0.8f};
    return byPlace[(index - 1) % 10];
}

bool isHandMade(int index) {
    return index <= 6 || index == 11 || index == 15 || index == 16 || index == 22 || index == 23 || index == 32 || index == 38 || index == 42;
}

// The set pieces (VARIETY_DESIGN 10.3): the hard level before every fiftieth, 49 to 599; -1 elsewhere.
int setPieceAt(int index) { return index % 50 == 49 && index <= 599 ? (index - 49) / 50 : -1; }

// The name a level shows under its LEVEL chip and in its first second, and its one hint line (VARIETY_DESIGN 10.1, 10.3):
// a set piece's, or the recipe the level introduces; null for any other level.
const char* levelTitle(int index) {
    const int p = setPieceAt(index), r = introRecipe(index);
    return p >= 0 ? recipeName(recipeCount() + p) : r >= 0 ? kRecipes[r].name : nullptr;
}
const char* levelIdea(int index) {
    const int p = setPieceAt(index), r = introRecipe(index);
    return p >= 0 ? kSetPieces[p].idea : r >= 0 ? kRecipes[r].idea : nullptr;
}

// The introductions of VARIETY_DESIGN 10.1 that are not hand-made: on its level a recipe, a plan or a twist comes for the
// first time, forced there (a generated level of that recipe, with its name and idea as the hint: see PROGRESS.md). The
// stretch items that were not built leave their levels (76, 83, 88, 91) to two things already in, together (76 side doors
// on a recipe the seed draws, 83 runner packs, 88 ECLIPSE, 91 GOLDEN GATE), and the executor's last two recipes come there.
int introRecipe(int index) {
    switch (index) {
        case 17: return 14;  // Hopscotch (hoppers came on 16)
        case 24: return 21;  // Gauntlet (the executor's)
        case 26: return 12;  // Crossroads: choice pairs
        case 28: return 9;   // Crossfire: side doors, plan Flank
        case 33: return 2;   // Twin rivers
        case 39: return 20;  // Breathing ladder (the executor's; breathing gates came on 38)
        case 46: return 5;   // Toll road: sliding walls arrive with it
        case 56: return 13;  // Treasure walls: vaults
        case 58: return 3;   // Checker
        case 61: return 7;   // Stagger: the first Long lane
        case 68: return 6;   // Pendulum
        case 73: return 4;   // The vault (a level on: something new every five levels from 68 to 88)
        case 78: return 15;  // The narrows
        case 83: return 8;   // Islands, three levels early (VARIETY_DESIGN 10.1 allows it), with runner packs
        case 88: return 23;  // Late bloom (the executor's) under ECLIPSE: GATE STORM's level
        case 91: return 22;  // Shifting window (the executor's) with GOLDEN GATE: the portals' level
        case 93: return 10;  // The breach
        case 96: return 11;  // Caravan, with shields
        default: return -1;
    }
}

int introPlanFor(int index) {
    if (index == 76) return kPlanFlank;
    if (index == 83) return kPlanRunnerPacks;
    for (int p = 1; p < kPlanCount; ++p)
        if (index == planFrom(p)) return p;
    return -1;
}

int introTwist(int index) {
    switch (index) {
        case 43: return kTwistGeodeRain;
        case 48: return kTwistEclipse;
        case 66: return kTwistGoldenGate;
        case 88: return kTwistEclipse;
        case 91: return kTwistGoldenGate;
        default: return -1;
    }
}

// The recipe a generated level gets when no introduction forces one: any recipe that serves its kind and has come by
// then, drawn by the seed (the bake's first pass tries seeds until the no-repeat rule holds: VARIETY_DESIGN 11).
// The worlds' signatures (VARIETY_DESIGN 15), recipes by their number in the catalogue (recipe_book.cpp).
const Signature& signatureOf(int world) {
    static const Signature s[kWorldCount] = {
        {"sliders", {6, 22, -1, -1}, -1, false, false},                         // Violet Dusk: Pendulum, Shifting window
        {"hoppers", {14, -1, -1, -1}, kPlanPincer, false, false},               // Deep Teal: Hopscotch
        {"geodes and caravans", {11, 12, 13, 4}, -1, false, false},             // Rose Dawn: Caravan, Crossroads, Treasure walls, The vault
        {"twin rivers and islands", {2, 8, -1, -1}, -1, false, false},          // Emerald Aurora
        {"walls and brutes", {5, 4, 15, -1}, kPlanBruteLine, false, false},     // Ember: Toll road, The vault, The narrows
        {"breathing gates", {20, -1, -1, -1}, kPlanPulse, false, false},        // Frost Glass: Breathing ladder
        {"twists", {-1, -1, -1, -1}, kPlanLateSurge, true, false},              // Storm Indigo
        {"rich gates, many geodes", {23, 12, 19, -1}, -1, false, true},         // Gold Noon: Late bloom, Crossroads, Festival field
        {"snuffers and narrows", {21, 15, -1, -1}, -1, false, false},           // Moonlit Jade: Gauntlet, The narrows
        {"colossi and side doors", {9, 10, 17, 23}, kPlanFlank, false, false},  // Crimson Dusk: Crossfire, The breach, The long walk, Late bloom
    };
    return s[clampv(world, 1, kWorldCount) - 1];
}

// A recipe made for one kind (Dawn line, The long walk, Two towers, Festival field) is drawn three times as often on its
// kind: those come six to ten times a hundred levels, and each of their recipes should show at least twice. A world's
// signature doubles its recipes (VARIETY_DESIGN 15: a lean, not a wall).
float recipeWeight(int index, int kind, int recipe) {
    if (recipe < 0 || recipe >= recipeCount() || !(kRecipes[recipe].kinds >> kind & 1u) || index < kRecipes[recipe].from) return 0;
    const u32 k = kRecipes[recipe].kinds;
    float w = (k & (k - 1)) == 0 ? 3.0f : 1.0f;
    for (int j : signatureOf(worldFor(index)).recipes) w *= j == recipe ? 2.0f : 1.0f;
    return w;
}

bool forcedRecipe(int index) { return isHandMade(index) || introRecipe(index) >= 0 || setPieceAt(index) >= 0; }

int pickRecipe(int index, int kind, Rng& r) {
    int allowed[32], n = 0;
    float weight[32], total = 0;
    for (int i = 0; i < recipeCount() && n < 32; ++i)
        if ((kRecipes[i].kinds >> kind & 1u) && index >= kRecipes[i].from) {
            weight[n] = recipeWeight(index, kind, i);
            total += weight[n];
            allowed[n++] = i;
        }
    if (n == 0) return 0;
    float pick = r.range(0, total);
    int i = 0;
    while (i < n - 1 && pick >= weight[i]) pick -= weight[i++];
    return allowed[i];
}

int planFrom(int plan) {
    static const int from[kPlanCount] = {2, 18, 24, 27, 34, 28, 37, 30};  // Pincer on 24: 22 is the first holdout
    return plan >= 0 && plan < kPlanCount ? from[plan] : 1 << 20;
}

// A plan whose first level has come, Steady a little more often; on a plan's first level, that plan (VARIETY_DESIGN 5.1).
int planFor(int index, Rng& r) {
    for (int p = 1; p < kPlanCount; ++p)
        if (index == planFrom(p)) return p;
    float wt[kPlanCount] = {}, total = 0;
    const int lean = signatureOf(worldFor(index)).plan;  // a world's signature doubles its plan
    for (int p = 0; p < kPlanCount; ++p)
        if (index >= planFrom(p)) total += wt[p] = (p == kPlanSteady ? 1.5f : 1.0f) * (p == lean ? 2.0f : 1.0f);
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

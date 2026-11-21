#pragma once
// What a level is, as data, and how alike two levels are (VARIETY_DESIGN 11, VARIETY_TECH 6). Pure functions of a
// LevelDef; the variety report (glowtide_headless --sim variety) and the bake's no-repeat rule use them.

#include <vector>

#include "sim/level.h"

namespace gt::sim {

constexpr int kFpBands = 12, kFpCols = 8;  // along the lane from z = 4 to len - 4, and across it (1.25 m a column)

// A cell holds the strongest thing that covers it (a higher code is stronger). kFpMoves is or-ed in when the thing
// that holds the cell moves (a slider's or hopper's whole travel is covered).
enum FpCode : u8 {
    kFpEmpty = 0,
    kFpBumper = 1,
    kFpSnuffer = 2,  // its patrol
    kFpDiv = 3,
    kFpWall = 4,
    kFpGeode = 5,  // its path down the lane
    kFpDoor = 6,
    kFpCurrent = 7,
    kFpPortal = 8,
    kFpMul = 10,  // + the factor: x2 is 12, x6 is 16
    kFpMoves = 0x80,
};

struct Fingerprint {
    u16 level = 0;
    u16 kindOrdinal = 0;  // how many levels of its kind came before it (the calendar's)
    u8 choices = 1;  // how many recipes the calendar could draw for this level's kind here (VARIETY_DESIGN 9.2)
    u8 kind = 0, recipe = 0, plan = 0, lengthClass = 0;  // a hand-made level's recipe is 128 + its number
    u8 twistCount = 0, twists[4] = {};
    u8 boonCount = 0, boons[16] = {};  // the boons offered, in the order the level offers them
    u8 cells[kFpBands * kFpCols] = {};
};

Fingerprint fingerprintOf(const LevelDef& L);
// The likeness weights and the rule's limits as tuned in V4 (VARIETY_TECH 6.3; PROGRESS.md has the numbers). VARIETY_TECH
// 6.2 started at 0.20 kind + 0.15 recipe + 0.10 plan + 0.05 length + 0.10 J + 0.40 M with limits 0.45 / 0.60 / 0.85: the
// calendar fixes the kind every five levels and a festival's plan and length, so two festivals ten levels apart scored
// 0.45 before their lanes were looked at, and the first twenty levels have one or two recipes.
constexpr float kLikeKind = 0.10f, kLikeRecipe = 0.15f, kLikePlan = 0.05f, kLikeLength = 0.05f, kLikeBoons = 0.10f, kLikeMap = 0.55f;
constexpr float kLimitNear = 0.55f, kLimitMid = 0.65f, kLimitFar = 0.85f;  // 1 to 12 levels apart, 13 to 40, anywhere
float likeness(const Fingerprint& a, const Fingerprint& b);
float mapLikeness(const Fingerprint& a, const Fingerprint& b);  // M: the larger of as-is and mirrored
float likenessLimit(int distance);
// Levels this far apart or closer may not share a recipe, a (recipe, kind) pair, a boon list (VARIETY_DESIGN 11). Where
// the calendar offers fewer recipes than a window holds (the first levels, a kind just introduced, and Siege, which has
// some eighteen levels in every thirty), a recipe may come back once most of the others have had their turn: the recipe
// window is at most two less than the later level's choices, and a (recipe, kind) pair may come back after the lesser of
// choices - 2 and two thirds of choices other levels of that kind (with one less, a boss between sieges, which draws from
// the same recipes, left level 21 nothing it could be; with all of them, a siege had two or three recipes left).
constexpr int kRecipeWindow = 5, kRecipeKindWindow = 29, kBoonListWindow = 7;
// Whether two levels `distance` apart break the rule; `why` says which line when it is not null.
bool breaksRule(const Fingerprint& a, const Fingerprint& b, int distance, const char** why = nullptr);

// --sim variety: the report over levels from..to; exit 1 if any line of the rule breaks. `pairs` lists the pairs
// furthest over their limit.
int varietyReport(int from, int to, bool pairs);
int varietyReportOn(const std::vector<Fingerprint>& fps, int from, int to, bool pairs);

// VARIETY_TECH 5's first pass (one thread, in level order): for each level from `from` to `to`, the first of `maxSeeds`
// seed numbers whose layout passes the lints and the rule against every level already chosen (the levels outside
// from..to, up to `keepTo`, stay as the table has them; a hand-made level has its one layout). A level where none passes
// takes the seed that breaks the fewest pairs, and says so. fps[l] is level l's fingerprint (index 0 unused).
struct LayoutChoice {
    int tries = 0;       // the seed's number: seed = defaultSeed(level) + tries * 1000003
    int looked = 0;      // seeds looked at
    int broken = 0;      // pairs the chosen layout breaks (0: the rule held)
    int lintFails = 0;   // seeds the lints threw away
    int unviable = 0;    // candidates the viability test threw away
    char why[64] = "";   // the first line it breaks
};
u64 seedFor(int level, int tries);
bool ruleHolds(const std::vector<Fingerprint>& fps, const std::vector<bool>& fixed, int level, const Fingerprint& f, int* broken,
               const char** why, int stopAt = 1 << 30);
// `viable`, when given, is a cheaper test of balance (the bake's line g forecast): of the candidates that keep the rule
// the pass takes the first, in its order of preference, that passes it (`jobs` tested at a time); none passing, the
// first. Without it the pass is seconds.
using LayoutTest = bool (*)(const LevelDef& def);
std::vector<LayoutChoice> chooseLayouts(int from, int to, int keepTo, std::vector<Fingerprint>& fps, int maxSeeds = 64,
                                        LayoutTest viable = nullptr, int jobs = 1);
// --sim layouts: the first pass alone, with what it needed.
int layoutsReport(int from, int to);

}  // namespace gt::sim

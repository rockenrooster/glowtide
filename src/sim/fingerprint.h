#pragma once
// What a level is, as data, and how alike two levels are (VARIETY_DESIGN 11, VARIETY_TECH 6). Pure functions of a
// LevelDef; the variety report (glowtide_headless --sim variety) and the bake's no-repeat rule use them.

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
    u8 kind = 0, recipe = 0, plan = 0, lengthClass = 0;
    u8 twistCount = 0, twists[4] = {};
    u8 boonCount = 0, boons[16] = {};  // the boons offered, in the order the level offers them
    u8 cells[kFpBands * kFpCols] = {};
};

Fingerprint fingerprintOf(const LevelDef& L);
// 0.20 same kind + 0.15 same recipe + 0.10 same plan + 0.05 same length class + 0.10 J + 0.40 M (VARIETY_TECH 6.2).
float likeness(const Fingerprint& a, const Fingerprint& b);
float mapLikeness(const Fingerprint& a, const Fingerprint& b);  // M: the larger of as-is and mirrored
float likenessLimit(int distance);  // 0.45 for 1 to 12 levels apart, 0.60 for 13 to 40, 0.85 beyond
// Levels this far apart or closer may not share a recipe, a (recipe, kind) pair, a boon list (VARIETY_DESIGN 11).
constexpr int kRecipeWindow = 5, kRecipeKindWindow = 29, kBoonListWindow = 7;
// Whether two levels `distance` apart break the rule; `why` says which line when it is not null.
bool breaksRule(const Fingerprint& a, const Fingerprint& b, int distance, const char** why = nullptr);

// --sim variety: the report over levels from..to; exit 1 if any line of the rule breaks. `pairs` lists the pairs
// furthest over their limit.
int varietyReport(int from, int to, bool pairs);

}  // namespace gt::sim

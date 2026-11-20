#pragma once
// Helpers shared by the files that lay out levels: levels_hand.cpp (hand-made levels), recipes.cpp (VARIETY_DESIGN 9.2),
// calendar.cpp (what a level number may use) and levels.cpp (the entry points and the baked table).

#include "core/rng.h"
#include "core/tuning.h"
#include "sim/level.h"

namespace gt::sim {

inline Gate mulGate(float cx, float hw, float z, int k, int row) {
    Gate g;
    g.cx = cx;
    g.hw = hw;
    g.z = z;
    g.kind = kGateMul;
    g.k = u8(k);
    g.row = u8(row);
    return g;
}

inline Gate divGate(float cx, float hw, float z, int k, int row) {
    Gate g = mulGate(cx, hw, z, k, row);
    g.kind = kGateDiv;
    return g;
}

inline void addGate(LevelDef& L, const Gate& g) {
    if (L.gateCount < kMaxGates) L.gates[L.gateCount++] = g;
}

inline void addRow(LevelDef& L, float z, Pattern p) {
    if (L.rowCount < kMaxRows) {
        L.rowZ[L.rowCount] = z;
        L.rowPattern[L.rowCount] = p;
        ++L.rowCount;
    }
}

inline void addEmitter(LevelDef& L, float x, float amp = 0, float period = 5, float phase = 0) {
    if (L.emitterCount < kMaxEmitters) L.emitters[L.emitterCount++] = {x, amp, period, phase};
}

inline void addBurst(LevelDef& L, float t, float frac, float mult, float dur, int brutes) {
    if (L.burstCount < kMaxBursts) L.bursts[L.burstCount++] = {t, frac, mult, dur, brutes};
}

// The generator's band for a level number (GAME_DESIGN 7.3): rows, the strongest multiply gate, pattern weights in
// the order Open, Choice, Trap, Slider, Wall, Pillars, Patrol, Crossing.
struct Band {
    int rowsLo = 2, rowsHi = 3, strongest = 3;
    float patternWeight[8] = {};
};
Band bandFor(int index);           // calendar.cpp
void applyEnemyDefaults(LevelDef& L);  // calendar.cpp: the core and the level's brutes
void placeGeodes(LevelDef& L, Rng& r);  // recipes.cpp: VARIETY_DESIGN 9.3

}  // namespace gt::sim

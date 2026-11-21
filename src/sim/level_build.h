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
    if (L.emitterCount < kMaxEmitters) {
        Emitter& e = L.emitters[L.emitterCount++];
        e = Emitter{};
        e.x = x;
        e.amp = amp;
        e.period = period;
        e.phase = phase;
    }
}

// A side door (VARIETY_DESIGN 4 #4): an emitter on the rail at height z, with its share of the flow.
inline void addDoor(LevelDef& L, int side, float z, float share) {
    if (L.emitterCount < kMaxEmitters) {
        Emitter& e = L.emitters[L.emitterCount++];
        e = Emitter{};
        e.side = i8(side);
        e.z = z;
        e.x = float(side) * (kHW - 0.45f);
        e.share = share;
    }
}

// A twist (VARIETY_DESIGN 7); SURGE's multiple and length are the old bursts' (tune.burstMult, tune.burstSeconds).
inline TwistDef& addTwist(LevelDef& L, u8 type, u8 trigger, float at, int brutes = 0, u8 param = 0) {
    static TwistDef spare;
    if (L.twistCount >= kMaxTwists) return spare = TwistDef{};
    TwistDef& x = L.twists[L.twistCount++];
    x = TwistDef{};
    x.type = type;
    x.trigger = trigger;
    x.at = at;
    x.mult = tune.burstMult;
    x.dur = tune.burstSeconds;
    x.brutes = brutes;
    x.param = param;
    return x;
}

// The generator's band for a level number (GAME_DESIGN 7.3): rows, the strongest multiply gate, pattern weights in
// the order Open, Choice, Trap, Slider, Wall, Pillars, Patrol, Crossing, Hopper, Breathing.
struct Band {
    int rowsLo = 2, rowsHi = 3, strongest = 3;
    float patternWeight[kPatCount] = {};
};
Band bandFor(int index);           // calendar.cpp
void applyEnemyDefaults(LevelDef& L);  // calendar.cpp: the core
int planFor(int index, Rng& r);        // calendar.cpp: the enemy's plan for a generated level
void placeGeodes(LevelDef& L, Rng& r);  // recipes.cpp: VARIETY_DESIGN 9.3
// recipes.cpp: a festival's rain of small geodes (VARIETY_DESIGN 6); a boss's script and the twists of a generated level
// (7, 8), with the geodes and the wall they release.
void placeFestivalGeodes(LevelDef& L, Rng& r);
void addBossScript(LevelDef& L);
void addTwists(LevelDef& L, Rng& r);

}  // namespace gt::sim

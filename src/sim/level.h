#pragma once
// A level (GAME_DESIGN 7.1): a small plain structure. Levels 1-5 are laid out by hand (levels.cpp), level 6 on comes
// from the generator (generate.cpp); spire health and the enemy's base flow come from the bake (levels_baked.h).

#include "core/types.h"

namespace gt::sim {

constexpr float kHW = 5.0f;              // lane half-width (GAME_DESIGN 3)
constexpr float kLauncherRange = 4.4f;   // the launcher's rail: x in [-4.4, 4.4]
constexpr float kMuzzleZ = 1.0f;
constexpr float kCoreZ = 0.8f;
constexpr int kMaxGates = 24, kMaxBarriers = 8, kMaxBumpers = 16, kMaxSnuffers = 8, kMaxEmitters = 3, kMaxBursts = 8,
              kMaxRows = 6;

enum GateKind : u8 { kGateMul = 0, kGateDiv = 1 };

struct Gate {
    float cx = 0, hw = 1.5f, z = 10;
    u8 kind = kGateMul, k = 2, row = 0, slides = 0;
    float amp = 0, period = 4, phase = 0;  // sliding: cx(t) = cx + amp * wave(t / period + phase)
};

struct Barrier {
    float x0 = 0, x1 = 0, z = 0;
    i32 health = 0;
};

struct Bumper {
    float x = 0, z = 0, r = 0.7f;
};

struct Snuffer {
    float xa = 0, xb = 0, z = 0, speed = 3, phase = 0;  // patrols x between xa and xb
};

struct Emitter {
    float x = 0, amp = 0, period = 5, phase = 0;
};

// A burst of the enemy flow: time-triggered (t >= 0, seconds after the level starts) or health-triggered (t < 0 and
// healthFrac: when the spire falls to that fraction). `brutes` brutes come with it.
struct Burst {
    float t = -1, healthFrac = 0, mult = 2, dur = 3;
    i32 brutes = 0;
};

enum Pattern : u8 { kPatOpen, kPatChoice, kPatTrap, kPatSlider, kPatWall, kPatPillars, kPatPatrol, kPatCrossing, kPatHand };

struct LevelDef {
    int index = 1;
    u64 seed = 1;
    float len = 36;
    int world = 1;          // palette, 1..5
    float targetT = 24;     // seconds
    i32 spireHealth = 60;
    i32 coreHealth = 10;
    float baseFlow = 0;     // enemy power a second (GAME_DESIGN 5.9); 0 with no enemy
    float fight = 0;        // the share of the player's output the enemy should cancel
    bool boss = false;
    float bruteEvery = 0;   // seconds; 0: no brutes
    i32 brutePower = 0;

    int gateCount = 0;
    Gate gates[kMaxGates];
    int barrierCount = 0;
    Barrier barriers[kMaxBarriers];
    int bumperCount = 0;
    Bumper bumpers[kMaxBumpers];
    int snufferCount = 0;
    Snuffer snuffers[kMaxSnuffers];
    int emitterCount = 0;
    Emitter emitters[kMaxEmitters];
    int burstCount = 0;
    Burst bursts[kMaxBursts];
    int rowCount = 0;
    float rowZ[kMaxRows] = {};
    u8 rowPattern[kMaxRows] = {};

    bool hasEnemy() const { return emitterCount > 0; }
};

// The level the game plays at `index` (1-based): hand-made, or generated from the baked seed.
LevelDef levelFor(int index);
// The layout alone (no baked numbers): hand-made for 1..5, else generated from (index, seed).
LevelDef layoutFor(int index, u64 seed);
LevelDef introLevel(int index);  // 1..5
LevelDef generate(int index, u64 seed);
float targetTime(int index);
float fightFor(int index);
int worldFor(int index);
u64 defaultSeed(int index);
// The repeat after level 300 (GAME_DESIGN 7.4): 301 plays 101, ...
int levelForPlay(int index);

// An eased triangle wave in [-1, 1] with period 1 (no trigonometry: TECH_SPEC 4.2).
float easedWave(float p);

}  // namespace gt::sim

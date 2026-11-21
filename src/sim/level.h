#pragma once
// A level (GAME_DESIGN 7.1): a small plain structure. Hand-made levels are in levels_hand.cpp, the others come from a
// recipe (recipes.cpp) chosen by the calendar (calendar.cpp); the numbers that size a level come from the bake
// (levels_baked.h, read by levels.cpp).

#include "core/types.h"

namespace gt::sim {

// Bump kGeneratorVersion when anything that changes a layout changes (a recipe, the calendar, a hand-made level), and
// kRulesVersion when anything that changes a bot's run changes (a rule in world.cpp, a tuning default, a bot). The bake
// writes both into levels_baked.h; a unit test fails while the table's differ from these (VARIETY_TECH 2).
constexpr int kGeneratorVersion = 8;  // 8: kinds, twists, bosses; hand-made 15, 21, 31, 41
constexpr int kRulesVersion = 5;      // 5: kinds (holdout, colossus, twins, festival, boss), twists, cubs
int bakedGeneratorVersion();
int bakedRulesVersion();

constexpr float kHW = 5.0f;              // lane half-width (GAME_DESIGN 3)
constexpr float kLauncherRange = 4.4f;   // the launcher's rail: x in [-4.4, 4.4]
constexpr float kMuzzleZ = 2.55f;        // the barrel's tip: where the lens is (gfx/mesh.cpp's launcher, placed at z 0.2)
constexpr float kCoreZ = 0.8f;
constexpr int kMaxGates = 24, kMaxBarriers = 16, kMaxBumpers = 16, kMaxSnuffers = 8, kMaxEmitters = 6, kMaxTwists = 4,
              kMaxRows = 6, kMaxGeodes = 16, kMaxCubs = 2;

enum GateKind : u8 { kGateMul = 0, kGateDiv = 1 };
// How a gate moves (Gate::slides): still, sliding on the gates' wave (GAME_DESIGN 5.4), or hopping between two places
// (VARIETY_DESIGN 4 #1). A breathing gate (Gate::breath, VARIETY_DESIGN 4 #2) is never also a slider or a hopper.
enum GateMove : u8 { kGateStill = 0, kGateSlide = 1, kGateHop = 2 };
enum Pattern : u8 {
    kPatOpen,
    kPatChoice,
    kPatTrap,
    kPatSlider,
    kPatWall,
    kPatPillars,
    kPatPatrol,
    kPatCrossing,
    kPatHopper,
    kPatBreathing,
    kPatHand,
    kPatCount = kPatHand
};

// A level's goal (VARIETY_DESIGN 6), its lane's length class (9.1), the enemy's plan (5.1), the named moments (7) and
// what geodes give (3). Their names (for reports and pictures) are in calendar.cpp; recipes' names in recipes.cpp.
enum Kind : u8 { kSiege, kHoldout, kColossus, kTwins, kFestival, kBoss, kKindCount };
enum LengthClass : u8 { kSprint, kStandard, kLong, kLengthCount };
enum Plan : u8 { kPlanSteady, kPlanPulse, kPlanPincer, kPlanLateSurge, kPlanEarlyRush, kPlanFlank, kPlanBruteLine, kPlanRunnerPacks, kPlanCount };
enum Twist : u8 {
    kTwistSurge,
    kTwistBruteGuard,
    kTwistGeodeRain,
    kTwistEclipse,
    kTwistStampede,
    kTwistSecondWind,
    kTwistGoldenGate,
    kTwistGateStorm,
    kTwistWardWall,
    kTwistCub,
    kTwistCount
};
enum Boon : u8 { kBoonLantern, kBoonQuicken, kBoonBright, kBoonWard, kBoonKindle, kBoonComet, kBoonHush, kBoonCount };
// What starts a twist: the clock (a share of T), the goal (the share of it still to go), the core (at or below a count).
enum TwistTrigger : u8 { kTrigTime, kTrigGoal, kTrigCore };
// The four bosses (VARIETY_DESIGN 8), in the order they come from level 10.
enum BossType : u8 { kBossOldSpire, kBossWarden, kBossTempest, kBossHydra, kBossCount };
const char* bossName(int boss);
const char* kindName(int kind);
const char* lengthClassName(int lengthClass);
const char* planName(int plan);
const char* twistName(int twist);
const char* boonName(int boon);
const char* recipeName(int recipe);

struct Gate {
    float cx = 0, hw = 1.5f, z = 10;
    u8 kind = kGateMul, k = 2, row = 0, slides = 0;  // slides: a GateMove
    float amp = 0, period = 4, phase = 0;  // slide: cx(t) = cx + amp * wave(t / period + phase); hop: period = seconds at each place
    float breath = 0;                      // seconds a breath takes (0: none): the half-width swings 60% to 120%
};

struct Barrier {
    float x0 = 0, x1 = 0, z = 0;
    i32 health = 0;
    float amp = 0, period = 4, phase = 0;  // a sliding wall (VARIETY_DESIGN 4 #3): its span shifts by amp * wave(...)
    u8 dormant = 0;                        // raised by a twist (WARD WALL): no health until then
};

struct Bumper {
    float x = 0, z = 0, r = 0.7f;
};

struct Snuffer {
    float xa = 0, xb = 0, z = 0, speed = 3, phase = 0;  // patrols x between xa and xb
};

struct Emitter {
    float x = 0, amp = 0, period = 5, phase = 0;
    float z = 0;     // a side door's height on the lane (side != 0)
    i8 side = 0;     // 0 at the spire's foot; -1 / +1 a side door on the left / right rail (VARIETY_DESIGN 4 #4)
    float share = 0; // its share of the flow (0: an equal share with the other emitters)
    u8 group = 0;    // Twins: which spire it belongs to
};

// A twist (VARIETY_DESIGN 7): a named moment that starts once, by the clock, the goal or the core. SURGE multiplies the
// flow for `dur` seconds (with `brutes` brutes: the old bursts); BRUTE GUARD sends two brutes; the others are in world.cpp.
struct TwistDef {
    u8 type = kTwistSurge, trigger = kTrigTime;
    float at = 0.5f;          // kTrigTime: a share of T; kTrigGoal: the share of the goal left; kTrigCore: the core at or below
    float mult = 2, dur = 3;  // SURGE
    i32 brutes = 0;
    u8 param = 0;             // GOLDEN GATE: the gate; WARD WALL: the barrier; CUB: how many; STAMPEDE: the emitter
};

// A geode (VARIETY_DESIGN 2): a crystal with health that drifts down the lane toward the lantern; the stream cracks it
// open for its boon. A shielded one carries a plate (a barrier that moves with it) that must break first.
struct Geode {
    float x = 0, z0 = 0, t = 0;              // where and when it appears (z0 0: the lane's length - 3)
    float speed = 1.6f;                      // m/s down the lane
    float amp = 0, period = 4, phase = 0;    // a sideways slide, the gates' wave (amp 0: none)
    float radius = 0.9f;
    float leaveAt = 0;                       // a vault waits at z0 until this time (0: drifts from the start)
    // Health in seconds of the stream that reaches the geode's column from straight below where it appears (the
    // gates it passes multiply it); the plate's the same way; 0: no plate. sizeGeodes turns them into numbers.
    float hpSeconds = 2.5f, shieldSeconds = 0;
    i32 hp = 0, shield = 0;
    u8 boon = kBoonLantern;
    u8 group = 0;                            // a choice pair shares a group above 0: opening one dissolves the other
    u8 twist = 0;                            // 1-based: released by that twist (GEODE RAIN, SECOND WIND) instead of at t
};

struct LevelDef {
    int index = 1;
    u64 seed = 1;
    u8 kind = kSiege, recipe = 0, plan = 0, lengthClass = kStandard;
    float len = 36;
    int world = 1;          // palette, 1..5
    float targetT = 24;     // seconds
    i32 spireHealth = 60;   // the goal: Siege and Boss the spire, Twins the left spire, Colossus its health
    i32 goal2 = 0;          // Twins: the right spire; Festival: the geodes opened for three stars
    float twinShare = 0.5f; // Twins: the left spire's share of the goal
    u8 bossType = 0;        // a BossType when kind is kBoss
    i32 coreHealth = 10;
    float baseFlow = 0;     // enemy power a second (GAME_DESIGN 5.9); 0 with no enemy
    float fight = 0;        // the share of the player's output the enemy should cancel
    bool boss = false;
    float bruteEvery = 0;   // seconds; 0: no brutes
    i32 brutePower = 0;
    float ref = 0;          // the reference power R the bake sized the level by (VARIETY_DESIGN 12.2)
    float hardness = 0.5f;  // the level's place in its world's rhythm, 0..1 (VARIETY_DESIGN 10.2)

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
    int twistCount = 0;
    TwistDef twists[kMaxTwists];
    int rowCount = 0;
    float rowZ[kMaxRows] = {};
    u8 rowPattern[kMaxRows] = {};
    int geodeCount = 0;
    Geode geodes[kMaxGeodes];

    bool hasEnemy() const { return emitterCount > 0; }
};

// Geode and plate health from their seconds and the stream that reaches their column, rounded to two significant
// figures like the spire's.
void sizeGeodes(LevelDef& L);
// What a straight stream at x has been multiplied by below z: the gates' factors, a slider's for the share of its
// travel it spends over x.
float columnMult(const LevelDef& L, float x, float z);
// How far a stream aimed at x has fanned out to each side below z: each multiply gate it passes throws its clones
// sideways (GAME_DESIGN 5.4), about (fanBase + fanPerK k) * fanDecay metres.
float fanSpread(const LevelDef& L, float x, float z);
// What KINDLE multiplies a straight stream at x by, minus one: each multiply gate it passes counts one more.
float kindleGain(const LevelDef& L, float x);
// The first level a plan may come on (VARIETY_DESIGN 5.1), and the plan a generated level gets (calendar.cpp).
int planFrom(int plan);
i32 twoSignificant(float v);
// Whether a level number is laid out by hand (levels_hand.cpp) rather than by a recipe.
bool isHandMade(int index);
float hardnessFor(int index);  // VARIETY_DESIGN 10.2
// What a boon taken now multiplies the stream's power by, minus one, given what the stream already has (VARIETY_DESIGN
// 3); 0 at its limit and for an instant boon.
float boonGain(int boon, int lanterns, int quicken, int bright);

// The level the game plays at `index` (1-based): hand-made, or generated from the baked seed.
LevelDef levelFor(int index);
// The layout alone (no baked numbers): hand-made or generated from (index, seed).
LevelDef layoutFor(int index, u64 seed);
void setGoals(LevelDef& L);  // the baked goal number into each kind's goals (levels.cpp)
LevelDef introLevel(int index);  // the hand-made levels
LevelDef generate(int index, u64 seed);
float targetTime(int index);
int kindFor(int index);  // calendar.cpp: the simple schedule of V3 (VARIETY_PLAN V3 task 8)
float fightFor(int index);
int worldFor(int index);
u64 defaultSeed(int index);
// The repeat after level 300 (GAME_DESIGN 7.4): 301 plays 101, ...
int levelForPlay(int index);

// An eased triangle wave in [-1, 1] with period 1 (no trigonometry: TECH_SPEC 4.2).
float easedWave(float p);
// A gate's centre and half-width at time t (slides, hops, breaths); a wall's sideways shift; the enemy's flow at time t
// as a multiple of the base flow, by plan (VARIETY_DESIGN 5.1: every plan averages 1 over the level's T).
float gateXAt(const Gate& g, float t);
float gateHwAt(const Gate& g, float t);
float barrierShiftAt(const Barrier& b, float t);
float planFlow(int plan, float t, float T);

}  // namespace gt::sim

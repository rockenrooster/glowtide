#pragma once
// The simulation (GAME_DESIGN 5, TECH_SPEC 4): deterministic, fixed 60 Hz, structure of arrays, no allocation after
// the first init. The game, the headless tool, the bots, the bake and the tests all use it.

#include "core/rng.h"
#include "sim/level.h"

namespace gt::sim {

constexpr int kMaxWisps = 4096;   // array sizes; the caps (tune.capW, capG) are at most these
constexpr int kMaxGlooms = 2048;
constexpr int kMaxEvents = 256;
constexpr float kTick = 1.0f / 60.0f;

enum UnitFlag : u8 { kBrute = 1 };

struct Units {
    int n = 0;
    float* x = nullptr;
    float* z = nullptr;
    float* xp = nullptr;  // position at the start of the tick
    float* zp = nullptr;
    float* vx = nullptr;  // sideways push (fan), decays
    i32* power = nullptr;
    u32* bits = nullptr;  // gates passed
    i32* born = nullptr;  // tick
    u8* flags = nullptr;
    u8* seed = nullptr;
    u8* clashed = nullptr;
};

struct Input {
    float launcherTargetX = 0;
    bool firing = false;
};

enum EventType : u8 {
    kEvFire,
    kEvGateMul,      // a = gate, b = passes this tick; x = mean x
    kEvGateDiv,      // a = gate, b = wisps snuffed
    kEvClash,        // x, z
    kEvClashCount,   // a = clashes this tick
    kEvBarrierHit,   // a = barrier, b = damage
    kEvBarrierBreak, // a = barrier
    kEvSnuffed,      // x, z
    kEvBruteDown,    // x, z
    kEvSpireHit,     // a = damage this tick, x = mean x
    kEvSpireStage,   // a = 75, 50 or 25
    kEvCoreHit,      // a = damage, x
    kEvTideMark,     // a = mark
    kEvWin,
    kEvLose,
    kEvGeodeAppear,  // a = geode
    kEvGeodeHit,     // a = geode, b = damage this tick
    kEvPlateHit,     // a = geode, b = damage this tick
    kEvPlateBreak,   // a = geode
    kEvGeodeOpen,    // a = geode, b = boon
    kEvGeodeLost,    // a = geode, b = 0 slipped past, 1 dissolved (its pair was opened)
    kEvBoon,         // a = boon, b = how many of it the stream holds now (WARD: glooms popped)
};

enum GeodeState : u8 { kGeodeWaiting, kGeodeAlive, kGeodeOpened, kGeodeLost };

struct Event {
    u8 type;
    float x, z;
    i32 a, b;
};

enum class Result : u8 { None, Win, Lose };

class World {
public:
    World() = default;
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void init(const LevelDef& def, u64 noiseSeed = 0);
    void tick(const Input& in);
    u64 hash() const;
    float time() const { return float(tickCount) * kTick; }
    float radius(int team, int i) const;  // team 0 wisps, 1 glooms
    // Lanterns (VARIETY_DESIGN 3.1): the first, with the core's pips, at the aim point; the second 0.9 m to its right, the
    // third 0.9 m to its left. The aim point's range shrinks so no lantern leaves the rail.
    float lanternX(int k) const;
    float aimMin() const;
    float aimMax() const;
    float fireRate() const;  // shots a second, with QUICKEN
    i32 shotPower() const { return 1 + bright; }

    // Switches for the bench rig (TECH_SPEC 4.5) and the bake (GAME_DESIGN 7.4).
    bool noClash = false;
    bool noDamage = false;      // no spire or core damage
    bool wrap = false;          // units re-enter at the start of their run
    bool unbreakableSpire = false;
    bool enemyOff = false;
    float flowScale = 1.0f;     // multiplies the base flow (the bake's rate scale)

    LevelDef def;
    int tickCount = 0;
    float launcherX = 0;
    float shot = 1;
    bool firing = false;
    Units w, g;
    float gateX[kMaxGates] = {};
    i32 gateCarry[kMaxGates] = {};
    i32 barrierHealth[kMaxBarriers] = {};
    float snufferX[kMaxSnuffers] = {};
    float emitterX[kMaxEmitters] = {};
    i32 spire = 0, spireStart = 0, core = 0;
    // Geodes (VARIETY_DESIGN 2) and the boons held this level (3).
    u8 geodeState[kMaxGeodes] = {};
    float geodeX[kMaxGeodes] = {}, geodeZ[kMaxGeodes] = {};
    i32 geodeHp[kMaxGeodes] = {}, geodeShield[kMaxGeodes] = {};
    int lanterns = 1, quicken = 0, bright = 0;
    int geodesShown = 0, geodesOpened = 0;
    i64 spent = 0;  // power given to geodes and plates: neither delivered nor cancelled
    int spireStage = 0;
    Result result = Result::None;
    int endTick = -1;

    // Totals (the bake and the result screen).
    i64 produced = 0, cancelled = 0, delivered = 0;
    i64 gloomSpawned = 0;  // power of every gloom made (bookkeeping for checks; not in the hash)
    i64 warded = 0;        // power of the glooms WARD popped (bookkeeping, not in the hash)
    i64 tide = 0, peakTide = 0;
    int peakWisps = 0, peakGlooms = 0;
    float lowestGloomZ = 1e9f;
    bool coreHit = false;

    Event events[kMaxEvents];
    int eventCount = 0;

private:
    void emitWisp(float x, float z, i32 power, float lanternAt);
    void openGeode(int g);
    void emitGloom(float x, i32 power, bool brute);
    void addEvent(u8 type, float x, float z, i32 a, i32 b);
    void buildGrid(const Units& u, int* start, int* order);
    void separate(Units& u, const int* start, const int* order, int team);
    void removeDead(Units& u);
    int cellOf(float x, float z) const;

    Rng rng_;
    float owed_ = 0, nextBrute_ = 0;
    int emitterNext_ = 0;
    int burstStart_[kMaxBursts] = {};
    int tideMark_ = 0;
    float geodePrevZ_[kMaxGeodes] = {};
    int capW_ = 2400, capG_ = 1000, reserve_ = 64;
    float smooth_ = 0.3f, fanKeep_ = 0.97f;
    // Grid: 0.8 m cells over x in [-6, 6] and z in [-4, len + 4].
    static constexpr float kCell = 0.8f;
    static constexpr int kGridW = 15;
    static constexpr int kGridMaxH = 72;
    int gridH_ = 0;
    int* wStart_ = nullptr;
    int* wOrder_ = nullptr;
    int* gStart_ = nullptr;
    int* gOrder_ = nullptr;
    float* pushX_ = nullptr;
    float* pushZ_ = nullptr;
    int* cursor_ = nullptr;
    float* sx_ = nullptr;  // cell-ordered scratch for the separation
    float* sz_ = nullptr;
    float* sr_ = nullptr;
    bool allocated_ = false;
};

}  // namespace gt::sim

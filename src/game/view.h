#pragma once
// The view (TECH_SPEC 7.2): turns the simulation into pictures and cues and never changes it. It owns the camera,
// every animation state (gate punch and flash, spire cracks, barriers shaking, banners, trauma), the particle ring
// and the slow lights, and builds the renderer's Scene each frame.

#include "game/input.h"
#include "gfx/camera.h"
#include "gfx/palette.h"
#include "gfx/renderer.h"
#include "sim/world.h"

namespace gt {

struct ViewEnv {
    int w = 1, h = 1;
    float dp = 1;
    SafeArea safe;
};

class View {
public:
    void init(const ViewEnv& env);
    void resize(const ViewEnv& env);
    void setLevel(const sim::LevelDef& def, int worldOverride = 0);
    // Events of the tick that just ran.
    void onTick(const sim::World& w);
    void update(float dt, const sim::World& w);
    // Fills the scene (instances, lights, props, labels, uniforms) for this frame.
    void build(Scene& s, const sim::World& w, float alpha, UiBatch& labels);

    // Effects that the screens drive (P3/P6).
    float cheer = 0;          // wisps turn round
    float gloomCheer = 0;     // glooms bounce (the lose)
    float brighten = 0;       // the dawn
    float desaturate = 0;     // the lose
    float fade = 1;           // 0 black .. 1 visible
    float timeScale = 1;      // slow motion
    bool reduceEffects = false;
    bool showLabels = true;
    bool topDown = false;  // level thumbnails (P5)

    Camera cam;
    const WorldPalette* wp = nullptr;
    const GamePalette* gp = nullptr;
    sim::LevelDef def;
    double time = 0;          // view time, seconds
    float trauma = 0;
    float coreFlash = 0;
    float tidePunch = 0;
    int bannerMark = 0;
    float bannerTime = 0;

    // Particles (TECH_SPEC 5.8): ring buffer, written once at spawn.
    static constexpr int kParticles = 4000;
    void spawn(V3 p, V3 v, V3 color, float alpha, float size, float life, int kind, float spin = 0);
    int particlesAlive() const;
    // Picture support: make every particle old.
    void clearParticles();

    // Labels (TECH_SPEC 5.6): the measured smallest label (pixels of cap height) and stroke, for the label check.
    float minLabelCapPx = 1e9f, minLabelStrokePx = 1e9f;

private:
    void buildUnits(const sim::World& w);
    void addLight(float x, float z, float rx, float rz, V3 c, float a, int shape);
    void label(const char* text, V3 at, float emMetres, V3 strokeColor, float punch, UiBatch& b, float alpha = 1.0f);

    ViewEnv env_;
    float followX_ = 0, followV_ = 0;
    float gateFlash_[sim::kMaxGates] = {}, gatePunch_[sim::kMaxGates] = {};
    float spireFlash_ = 0, spirePunch_ = 0;
    double lastShot_ = -1;    // the shot's barrel squash and lens ring (GAME_DESIGN 10)
    float snufferFlare_[sim::kMaxSnuffers] = {};
    float barrierShake_[sim::kMaxBarriers] = {};
    float barrierPunch_[sim::kMaxBarriers] = {};
    // Geodes (VARIETY_DESIGN 13): the number's punch (at most every 80 ms), the shell's flash, the plate's shake, and
    // what is left to show once a geode has opened or gone (it bursts, or dims and sinks).
    float geodePunch_[sim::kMaxGeodes] = {}, geodeFlash_[sim::kMaxGeodes] = {}, plateShake_[sim::kMaxGeodes] = {};
    double geodePunchAt_[sim::kMaxGeodes] = {};
    u8 geodeEnd_[sim::kMaxGeodes] = {};  // 0 none, 1 opened, 2 slipped past, 3 dissolved
    float geodeEndT_[sim::kMaxGeodes] = {}, geodeEndX_[sim::kMaxGeodes] = {}, geodeEndZ_[sim::kMaxGeodes] = {};
    float lanternPop_[3] = {};  // a new lantern pops out of the old one
    float wardRing_ = 0;        // WARD's gold ring spreading up the lane
    float barrelRing_ = 0;      // QUICKEN or BRIGHT: a ring runs up the barrel
    float kindleT_ = -1;        // KINDLE: seconds since it was taken (-1: not this level); the labels count up row by row
    int kindleRow_ = 0;         // the rows whose labels have counted up
    float pipsLit_ = 10;        // the pips relight one by one
    float launcherZ_[3] = {0.2f, 0.896f, 0.896f};  // side lanterns stand forward so every lens is at the muzzle

    struct Flash {
        float x, z, r, t0, life;
        V3 c;
        float a;
    };
    static constexpr int kFlashes = 96;
    Flash flashes_[kFlashes] = {};
    int flashHead_ = 0;

    static constexpr int kMaxUnits = sim::kMaxWisps + sim::kMaxGlooms;
    UnitInstance units_[kMaxUnits];
    UnitInstance scratch_[kMaxUnits];
    u16 keys_[kMaxUnits];
    int unitCount_ = 0;
    static constexpr int kBuckets = 5200;
    int bucket_[kBuckets + 1];
    PanelInstance panels_[32];
    RowCut cuts_[16];
    int panelCount_ = 0, cutCount_ = 0;
    LightInstance lights_[512];
    int lightCount_ = 0;
    ParticleInstance particles_[kParticles];
    int particleHead_ = 0;
    int spawnedThisFrame_ = 0;
    static constexpr int kMaxProps = 128;
    PropDraw props_[kMaxProps];
    int propCount_ = 0;
};

const char* gateLabel(const sim::Gate& g, char* buf, int n, int k = 0);  // k: the factor shown (0: the gate's own)

}  // namespace gt

#pragma once
// The renderer (TECH_SPEC 5.3): light map, glow buffer and its blur, the scene at native resolution (props, lane,
// backdrop, far-to-near units cut by gate rows, particles, labels), bloom, UI. It draws a Scene and owns GL objects.

#include "gfx/frame_uniforms.h"
#include "gfx/gl.h"
#include "gfx/mesh.h"
#include "gfx/target.h"
#include "gfx/text.h"

namespace gt {

struct UnitInstance {
    float x, z;
    i16 dx, dz;  // last tick's displacement x 4096
    u8 flags;    // 1 gloom, 2 brute, 8 white (frenzy)
    u8 seed, age, tier;
};
static_assert(sizeof(UnitInstance) == 16, "unit instance");

struct PanelInstance {
    float x0, x1, z, flash;
    float kind, arrows, punch, pad;
};

struct LightInstance {
    float x, z, rx, rz;
    u8 r, g, b, a;
    float shape, pad0, pad1, pad2;
};

struct ParticleInstance {
    float ox, oy, oz, t0;
    float vx, vy, vz, life;
    u8 r, g, b, a;
    float size, kind, seed, spin;
};

// Units [previous cut's end, unitEnd) are drawn, then panels [panelFirst, panelFirst + panelCount).
struct RowCut {
    int unitEnd = 0, panelFirst = 0, panelCount = 0;
};

struct PropDraw {
    int mesh = 0;  // PropMeshId
    M4 model;
    V3 tint{1, 1, 1};
    float glow = 1.0f;  // emissive gain in the glow pass (0: not drawn there)
};

struct Scene {
    FrameUniforms fu;
    const UnitInstance* units = nullptr;
    int unitCount = 0;
    const RowCut* cuts = nullptr;
    int cutCount = 0;
    const PanelInstance* panels = nullptr;
    int panelCount = 0;
    const LightInstance* lights = nullptr;
    int lightCount = 0;
    const ParticleInstance* particles = nullptr;
    int particleCapacity = 0;
    bool particlesDirty = true;
    const PropDraw* props = nullptr;
    int propCount = 0;
    int spireStage = 0;        // cracks shown: 0..3
    float heartGain = 1;
    bool barrierAlive[sim::kMaxBarriers] = {};
    V3 barrierOffset[sim::kMaxBarriers];
    float emissiveGain = 1.0f;
    const UiBatch* labels = nullptr;
    const UiBatch* ui = nullptr;
    float uiOpacity = 1.0f;
    bool world = true;         // false: only the UI (and the debug quads)
};

class Renderer {
public:
    bool init(int w, int h);
    void destroy();
    void resize(int w, int h);
    // Rebuilds the level's static mesh and the prop meshes for a palette.
    void setLevel(const sim::LevelDef& def, const WorldPalette& wp, const GamePalette& gp);
    void draw(const Scene& s, GLuint fbo, int w, int h);
    // GPU time is not measured; counters for the benchmark and the label check.
    int lastDrawCalls() const { return drawCalls_; }

private:
    void uploadInstances(const Scene& s);
    void drawProps(const Scene& s, bool glowPass);
    void drawUnits(int first, int count);
    void drawUnitsWith(GLuint prog, int first, int count);
    void drawBatch(const UiBatch& b, float opacity);
    void bindUnitAttribs(int firstInstance);

    int w_ = 0, h_ = 0;
    GLuint ubo_ = 0;
    // Programs
    GLuint pProp_ = 0, pPropGlow_ = 0, pLane_ = 0, pBackdrop_ = 0, pUnit_ = 0, pLightUnit_ = 0, pLightQuad_ = 0, pGlowUnit_ = 0,
           pPanel_ = 0, pParticle_ = 0, pText_ = 0, pBlur_ = 0, pBloom_ = 0;
    GLint uPropModel_ = -1, uPropTint_ = -1, uPropGlowModel_ = -1, uPropGlowTint_ = -1, uPanelGlow_ = -1, uParticleGlow_ = -1,
          uTextSpread_ = -1, uTextOpacity_ = -1, uBlurTexel_ = -1, uBlurGain_ = -1;
    // Buffers
    GLuint vaoUnits_ = 0, vboUnits_ = 0;
    GLuint vaoPanels_ = 0, vboPanels_ = 0;
    GLuint vaoLights_ = 0, vboLights_ = 0;
    GLuint vaoParticles_ = 0, vboParticles_ = 0;
    GLuint vaoText_ = 0, vboText_ = 0, iboQuads_ = 0;
    GLuint vaoLane_ = 0, vboLane_ = 0;
    GLuint vaoEmpty_ = 0;
    GLuint vaoLevel_ = 0, vboLevel_ = 0, iboLevel_ = 0, iboLevelEmit_ = 0;
    GLuint vaoProps_ = 0, vboProps_ = 0, iboProps_ = 0, iboPropsEmit_ = 0;
    GLuint atlas_ = 0;
    // Targets
    Target light_, glow4_, glow8_, glow16_;
    LevelMeshData level_;
    PropMeshData props_;
    float laneLen_ = 36;
    int drawCalls_ = 0;
    int unitCapacity_ = 0, particleCapacity_ = 0;
};

}  // namespace gt

#pragma once
// Generated props (TECH_SPEC 5.6): flat-shaded, colours baked per vertex from a three-tone ramp by the face's angle
// to a key light. Vertex alpha 255 marks emissive geometry (drawn again into the glow buffer).

#include <vector>

#include "core/mathx.h"
#include "gfx/palette.h"
#include "sim/level.h"

namespace gt {

struct PropVertex {
    float x, y, z;
    u8 r, g, b, a;
};

struct Ramp {
    V3 body, light, dark;
};

class MeshBuilder {
public:
    std::vector<PropVertex> v;
    std::vector<u32> idx;      // every triangle (the scene pass)
    std::vector<u32> emit;     // emissive triangles again (the glow pass)

    void clear() {
        v.clear();
        idx.clear();
        emit.clear();
    }
    // A flat-shaded triangle: its colour from the ramp by its normal, darker toward the ground.
    void tri(V3 a, V3 b, V3 c, const Ramp& ramp);
    void quad(V3 a, V3 b, V3 c, V3 d, const Ramp& ramp) {
        tri(a, b, c, ramp);
        tri(a, c, d, ramp);
    }
    // Emissive (unlit) geometry in one colour.
    void triE(V3 a, V3 b, V3 c, V3 color);
    void quadE(V3 a, V3 b, V3 c, V3 d, V3 color) {
        triE(a, b, c, color);
        triE(a, c, d, color);
    }
    void box(V3 lo, V3 hi, const Ramp& ramp, bool bottom = false);
    void prism(V3 base, float rx, float rz, float height, int sides, const Ramp& ramp, bool top = true, float topScale = 1.0f,
               float phase = 0.0f);
    void octa(V3 c, float r, V3 color, bool emissive, const Ramp* ramp = nullptr);
    void discE(V3 c, float r, V3 color, int sides = 12);           // flat on the ground plane, facing up
    void ringE(V3 c, float r0, float r1, V3 color, int sides = 20);  // flat ring
    void crystal(V3 base, float halfW, float halfD, float height, float tipX, const Ramp& ramp);
    int vertexCount() const { return int(v.size()); }
};

struct Range {
    int first = 0, count = 0;
};

// Static geometry of one level, merged into one buffer (one draw), with ranges for the parts that change.
struct LevelMeshData {
    MeshBuilder mb;
    Range opaque, emissive;                 // everything static / its emissive part
    Range spire, spireEmit;                 // the spire (gone once it shatters)
    Range cracks[3], cracksEmit[3];         // the spire's cracks by stage (75, 50, 25%)
    Range heart, heartEmit;
    int barrierCount = 0;
    Range barrier[sim::kMaxBarriers], barrierEmit[sim::kMaxBarriers];
};

void buildLevelMesh(LevelMeshData& out, const sim::LevelDef& def, const WorldPalette& wp, const GamePalette& gp);

// Meshes drawn with a model matrix: built once.
enum PropMeshId { kMeshLauncher, kMeshLens, kMeshPip, kMeshPostGood, kMeshPostBad, kMeshSnuffer, kMeshCount };
struct PropMeshData {
    MeshBuilder mb;
    Range part[kMeshCount], partEmit[kMeshCount];
};
void buildPropMeshes(PropMeshData& out, const WorldPalette& wp, const GamePalette& gp);

}  // namespace gt

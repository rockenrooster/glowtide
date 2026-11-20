#include "gfx/mesh.h"

#include <cmath>

#include "core/rng.h"

namespace gt {

namespace {

const V3 kKeyLight = normalize(V3(-0.55f, 0.75f, -0.35f));

u8 to8(float v) { return u8(clampv(v, 0.0f, 1.0f) * 255.0f + 0.5f); }

V3 shadeOf(const Ramp& r, V3 n, float y) {
    const float k = dot(n, kKeyLight);
    V3 c = k >= 0 ? lerp(r.body, r.light, k) : lerp(r.body, r.dark, minv(1.0f, -k * 1.1f));
    const float ground = 0.72f + 0.28f * clampv(y / 2.0f, 0.0f, 1.0f);
    return c * ground;
}

V3 g_center;  // outward hint of the object being built

}  // namespace

void MeshBuilder::tri(V3 a, V3 b, V3 c, const Ramp& ramp) {
    V3 n = normalize(cross(b - a, c - a));
    const V3 mid = (a + b + c) * (1.0f / 3.0f);
    if (dot(n, mid - g_center) < 0) n = n * -1.0f;
    const V3 col = shadeOf(ramp, n, mid.y);
    const u32 base = u32(v.size());
    for (V3 p : {a, b, c}) v.push_back({p.x, p.y, p.z, to8(col.x), to8(col.y), to8(col.z), 0});
    idx.push_back(base);
    idx.push_back(base + 1);
    idx.push_back(base + 2);
}

void MeshBuilder::triE(V3 a, V3 b, V3 c, V3 color) {
    const u32 base = u32(v.size());
    for (V3 p : {a, b, c}) v.push_back({p.x, p.y, p.z, to8(color.x), to8(color.y), to8(color.z), 255});
    for (u32 i = 0; i < 3; ++i) {
        idx.push_back(base + i);
        emit.push_back(base + i);
    }
}

void MeshBuilder::box(V3 lo, V3 hi, const Ramp& ramp, bool bottom) {
    g_center = (lo + hi) * 0.5f;
    const V3 p[8] = {{lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
                     {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}};
    quad(p[0], p[1], p[2], p[3], ramp);  // front (-z)
    quad(p[5], p[4], p[7], p[6], ramp);  // back
    quad(p[4], p[0], p[3], p[7], ramp);  // left
    quad(p[1], p[5], p[6], p[2], ramp);  // right
    quad(p[3], p[2], p[6], p[7], ramp);  // top
    if (bottom) quad(p[4], p[5], p[1], p[0], ramp);
}

void MeshBuilder::prism(V3 base, float rx, float rz, float height, int sides, const Ramp& ramp, bool top, float topScale,
                        float phase) {
    g_center = base + V3(0, height * 0.5f, 0);
    for (int i = 0; i < sides; ++i) {
        const float a0 = (float(i) + phase) / float(sides) * 2.0f * kPi, a1 = (float(i + 1) + phase) / float(sides) * 2.0f * kPi;
        const V3 b0 = base + V3(std::cos(a0) * rx, 0, std::sin(a0) * rz);
        const V3 b1 = base + V3(std::cos(a1) * rx, 0, std::sin(a1) * rz);
        const V3 t0 = base + V3(std::cos(a0) * rx * topScale, height, std::sin(a0) * rz * topScale);
        const V3 t1 = base + V3(std::cos(a1) * rx * topScale, height, std::sin(a1) * rz * topScale);
        quad(b0, b1, t1, t0, ramp);
        if (top) {
            g_center = base;  // the cap faces up
            tri(base + V3(0, height, 0), t0, t1, ramp);
            g_center = base + V3(0, height * 0.5f, 0);
        }
    }
}

void MeshBuilder::octa(V3 c, float r, V3 color, bool emissive, const Ramp* ramp) {
    const V3 px = c + V3(r, 0, 0), nx = c + V3(-r, 0, 0), py = c + V3(0, r, 0), ny = c + V3(0, -r, 0), pz = c + V3(0, 0, r),
             nz = c + V3(0, 0, -r);
    const V3 faces[8][3] = {{py, px, pz}, {py, pz, nx}, {py, nx, nz}, {py, nz, px},
                            {ny, pz, px}, {ny, nx, pz}, {ny, nz, nx}, {ny, px, nz}};
    g_center = c;
    for (auto& f : faces) {
        if (emissive)
            triE(f[0], f[1], f[2], color);
        else
            tri(f[0], f[1], f[2], ramp ? *ramp : Ramp{color, color, color});
    }
}

void MeshBuilder::discE(V3 c, float r, V3 color, int sides) {
    for (int i = 0; i < sides; ++i) {
        const float a0 = float(i) / float(sides) * 2.0f * kPi, a1 = float(i + 1) / float(sides) * 2.0f * kPi;
        triE(c, c + V3(std::cos(a0) * r, 0, std::sin(a0) * r), c + V3(std::cos(a1) * r, 0, std::sin(a1) * r), color);
    }
}

void MeshBuilder::ringE(V3 c, float r0, float r1, V3 color, int sides) {
    for (int i = 0; i < sides; ++i) {
        const float a0 = float(i) / float(sides) * 2.0f * kPi, a1 = float(i + 1) / float(sides) * 2.0f * kPi;
        const V3 i0 = c + V3(std::cos(a0) * r0, 0, std::sin(a0) * r0), i1 = c + V3(std::cos(a1) * r0, 0, std::sin(a1) * r0);
        const V3 o0 = c + V3(std::cos(a0) * r1, 0, std::sin(a0) * r1), o1 = c + V3(std::cos(a1) * r1, 0, std::sin(a1) * r1);
        quadE(i0, o0, o1, i1, color);
    }
}

void MeshBuilder::crystal(V3 base, float halfW, float halfD, float height, float tipX, const Ramp& ramp) {
    // A faceted crystal: a six-sided base, a narrower ring at 72% of the height, a tip.
    const int n = 6;
    V3 b[n], m[n];
    for (int i = 0; i < n; ++i) {
        const float a = (float(i) + 0.5f) / float(n) * 2.0f * kPi;
        b[i] = base + V3(std::cos(a) * halfW, 0, std::sin(a) * halfD);
        m[i] = base + V3(std::cos(a) * halfW * 0.5f + tipX * 0.4f, height * 0.72f, std::sin(a) * halfD * 0.5f);
    }
    const V3 tip = base + V3(tipX, height, 0);
    g_center = base + V3(0, height * 0.4f, 0);
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        quad(b[i], b[j], m[j], m[i], ramp);
        tri(m[i], m[j], tip, ramp);
    }
}

void buildLevelMesh(LevelMeshData& out, const sim::LevelDef& def, const WorldPalette& wp, const GamePalette& gp) {
    MeshBuilder& mb = out.mb;
    mb.clear();
    const float HW = sim::kHW, len = def.len;
    const Ramp prop{wp.propBody, wp.propLight, wp.propDark};
    const Ramp dark{hexColor(0x0E1032), hexColor(0x1C2050), hexColor(0x080A20)};

    // Lane front face and its glowing lower edge.
    g_center = V3(0, -0.75f, 0);
    mb.quad(V3(-HW, 0, -2.4f), V3(HW, 0, -2.4f), V3(HW, -1.5f, -2.4f), V3(-HW, -1.5f, -2.4f), dark);
    mb.quadE(V3(-HW, -1.44f, -2.42f), V3(HW, -1.44f, -2.42f), V3(HW, -1.5f, -2.42f), V3(-HW, -1.5f, -2.42f), wp.rail * 0.55f);

    // Rails along both edges.
    for (float s : {-1.0f, 1.0f}) {
        const float x0 = s * (HW - 0.15f), x1 = s * HW;
        mb.quadE(V3(x0, 0.015f, -2.4f), V3(x1, 0.015f, -2.4f), V3(x1, 0.015f, len + 1.7f), V3(x0, 0.015f, len + 1.7f), wp.rail);
        mb.quadE(V3(x1, 0.015f, -2.4f), V3(x1, -0.14f, -2.4f), V3(x1, -0.14f, len + 1.7f), V3(x1, 0.015f, len + 1.7f), wp.rail * 0.6f);
        for (float z = 4.0f; z < len; z += 8.0f) {  // lantern posts
            mb.prism(V3(s * (HW + 0.3f), 0, z), 0.05f, 0.05f, 0.75f, 5, prop, false);
            mb.octa(V3(s * (HW + 0.3f), 0.82f, z), 0.2f, hexColor(0xFFE2A0), true);
        }
    }

    // Floating islets off both sides, with tiny lights.
    Rng r(def.seed ^ 0xA11E7ull, 3);
    for (int i = 0; i < 8; ++i) {
        const float side = (i & 1) ? 1.0f : -1.0f;
        const float x = side * r.range(7.5f, 13.0f);
        const float z = r.range(-4.0f, len + 14.0f);
        const float y = r.range(-6.0f, -2.5f);
        const float size = r.range(0.9f, 2.0f);
        const Ramp islet{lerp(wp.propBody, wp.backdrop[1], 0.45f), lerp(wp.propLight, wp.backdrop[0], 0.4f),
                         lerp(wp.propDark, wp.backdrop[2], 0.3f)};
        g_center = V3(x, y - size, z);
        const int n = 7;
        for (int k = 0; k < n; ++k) {  // inverted cone with a flat top
            const float a0 = float(k) / float(n) * 2.0f * kPi, a1 = float(k + 1) / float(n) * 2.0f * kPi;
            const V3 t0 = V3(x + std::cos(a0) * size, y, z + std::sin(a0) * size * 0.6f);
            const V3 t1 = V3(x + std::cos(a1) * size, y, z + std::sin(a1) * size * 0.6f);
            mb.tri(t0, t1, V3(x + size * 0.15f, y - size * 1.6f, z), islet);
            g_center = V3(x, y - 10.0f, z);
            mb.tri(V3(x, y, z), t0, t1, islet);
            g_center = V3(x, y - size, z);
        }
        for (int k = 0; k < 2; ++k)
            mb.octa(V3(x + (float(k) - 0.5f) * size * 0.6f, y + 0.1f, z + r.range(-0.3f, 0.3f)), 0.08f, hexColor(0xFFD7A0), true);
    }

    // Gate posts of the fixed gates (the view draws the posts that move: sliders, hoppers, breathing gates).
    for (int i = 0; i < def.gateCount; ++i) {
        const sim::Gate& G = def.gates[i];
        if (G.slides || G.breath > 0) continue;
        const V3 col = G.kind == sim::kGateMul ? gp.good : gp.bad;
        for (float s : {-1.0f, 1.0f}) {
            const float x = G.cx + s * G.hw;
            mb.prism(V3(x, 0, G.z), 0.18f, 0.18f, 3.3f, 8, prop, true);
            mb.quadE(V3(x - 0.06f, 0.25f, G.z - 0.19f), V3(x + 0.06f, 0.25f, G.z - 0.19f), V3(x + 0.06f, 3.1f, G.z - 0.19f),
                     V3(x - 0.06f, 3.1f, G.z - 0.19f), col);
            mb.octa(V3(x, 3.45f, G.z), 0.24f, wp.propLight, false, &prop);
        }
    }

    // Bumpers.
    for (int i = 0; i < def.bumperCount; ++i) {
        const sim::Bumper& B = def.bumpers[i];
        mb.prism(V3(B.x, 0, B.z), B.r, B.r, 1.3f, 10, prop, true);
        mb.discE(V3(B.x, 1.31f, B.z), B.r * 0.5f, gp.good);
    }

    // Side doors (VARIETY_DESIGN 4 #4, 13): a dark arch standing across the rail, its opening toward the lantern so the
    // rose glow inside reads as a doorway from the camera; its glooms step out of the opening's inner edge.
    for (int i = 0; i < def.emitterCount; ++i) {
        const sim::Emitter& E = def.emitters[i];
        if (!E.side) continue;
        const float s = float(E.side), cx = s * (HW + 0.25f), z = E.z;
        const Ramp arch{hexColor(0x2A1646), hexColor(0x4C2A78), hexColor(0x120A22)};
        mb.box(V3(cx - 1.05f, 0, z - 0.25f), V3(cx - 0.72f, 2.4f, z + 0.25f), arch, false);
        mb.box(V3(cx + 0.72f, 0, z - 0.25f), V3(cx + 1.05f, 2.4f, z + 0.25f), arch, false);
        mb.box(V3(cx - 1.15f, 2.4f, z - 0.3f), V3(cx + 1.15f, 2.85f, z + 0.3f), arch, false);
        mb.quadE(V3(cx - 0.72f, 0.02f, z - 0.05f), V3(cx + 0.72f, 0.02f, z - 0.05f), V3(cx + 0.72f, 2.4f, z - 0.05f),
                 V3(cx - 0.72f, 2.4f, z - 0.05f), gp.crack * 0.75f);
    }

    // The spire: its own range, so the win can take it away when it shatters.
    const u32 spireI0 = u32(mb.idx.size()), spireE0 = u32(mb.emit.size());
    const float zc = len + 1.4f;
    const Ramp spire{gp.spire, gp.spireLight, gp.spireDark};
    const float halfW = def.boss ? 3.4f : 2.8f;
    mb.crystal(V3(0, 0, zc), halfW, 1.4f, 6.3f, 0.15f, spire);
    const float feet[4][3] = {{-3.8f, 1.7f, 0.5f}, {3.6f, 1.6f, 0.45f}, {-2.7f, 2.5f, 0.55f}, {2.6f, 2.4f, 0.5f}};
    for (const auto& f : feet) mb.crystal(V3(f[0] * (def.boss ? 1.15f : 1.0f), 0, zc - 0.2f), f[2], f[2] * 0.7f, f[1], 0.1f, spire);
    if (def.boss) {  // the crown
        for (int k = 0; k < 5; ++k) {
            const float a = (float(k) - 2.0f) * 0.42f;
            mb.crystal(V3(std::sin(a) * 1.5f, 3.6f + std::cos(a) * 0.6f, zc - 0.6f), 0.32f, 0.25f, 2.2f - std::fabs(a) * 0.9f,
                       std::sin(a) * 0.5f, spire);
        }
    }
    out.opaque.first = 0;
    // Cracks by stage, on the front faces; the heart near the top.
    auto faceZ = [&](float y) { return zc - 1.4f * 0.87f * (1.0f - y / 6.3f) - 0.06f; };
    const float cracks[3][5][2] = {{{-2.0f, 0.2f}, {-1.3f, 1.2f}, {-1.6f, 2.0f}, {-1.0f, 3.3f}, {-0.8f, 3.9f}},
                                   {{1.9f, 0.3f}, {1.3f, 1.1f}, {1.6f, 2.0f}, {1.0f, 2.9f}, {1.1f, 3.9f}},
                                   {{-0.3f, 0.2f}, {0.3f, 0.8f}, {-0.1f, 1.6f}, {0.35f, 2.6f}, {0.0f, 3.6f}}};
    const u32 beforeCracks = u32(mb.idx.size()), beforeCracksE = u32(mb.emit.size());
    for (int s = 0; s < 3; ++s) {
        const u32 i0 = u32(mb.idx.size()), e0 = u32(mb.emit.size());
        for (int k = 0; k < 4; ++k) {
            const float x0 = cracks[s][k][0], y0 = cracks[s][k][1], x1 = cracks[s][k + 1][0], y1 = cracks[s][k + 1][1];
            const float wdt = 0.07f;
            mb.quadE(V3(x0 - wdt, y0, faceZ(y0)), V3(x0 + wdt, y0, faceZ(y0)), V3(x1 + wdt, y1, faceZ(y1)), V3(x1 - wdt, y1, faceZ(y1)),
                     gp.crack);
        }
        out.cracks[s] = {int(i0), int(mb.idx.size() - i0)};
        out.cracksEmit[s] = {int(e0), int(mb.emit.size() - e0)};
    }
    {
        const u32 i0 = u32(mb.idx.size()), e0 = u32(mb.emit.size());
        const float hy = 4.7f, hz = faceZ(hy) - 0.05f, hr = 0.36f;
        mb.quadE(V3(0.05f, hy - hr * 1.5f, hz), V3(0.05f + hr, hy, hz), V3(0.05f, hy + hr * 1.5f, hz), V3(0.05f - hr, hy, hz),
                 hexColor(0xFFC8F0));
        out.heart = {int(i0), int(mb.idx.size() - i0)};
        out.heartEmit = {int(e0), int(mb.emit.size() - e0)};
    }
    out.opaque.count = int(spireI0);
    out.emissive = {0, int(spireE0)};
    out.spire = {int(spireI0), int(beforeCracks - spireI0)};
    out.spireEmit = {int(spireE0), int(beforeCracksE - spireE0)};

    // Barriers: one range each (they shake and break).
    out.barrierCount = def.barrierCount;
    for (int b = 0; b < def.barrierCount; ++b) {
        const sim::Barrier& B = def.barriers[b];
        const u32 i0 = u32(mb.idx.size()), e0 = u32(mb.emit.size());
        const Ramp wall{hexColor(0x4A2884), hexColor(0x7446C4), hexColor(0x241048)};
        mb.box(V3(B.x0, 0, B.z - 0.25f), V3(B.x1, 1.6f, B.z + 0.25f), wall);
        const int teeth = maxv(2, int((B.x1 - B.x0) / 0.55f + 0.5f));
        const float tw = (B.x1 - B.x0) / float(teeth);
        const Ramp tooth{hexColor(0x7446C4), hexColor(0x9C70E0), hexColor(0x3A1C6C)};
        for (int t = 0; t < teeth; ++t) {
            const float xa = B.x0 + float(t) * tw, xb = xa + tw, xm = (xa + xb) * 0.5f;
            g_center = V3(xm, 1.6f, B.z);
            mb.tri(V3(xa, 1.6f, B.z - 0.25f), V3(xb, 1.6f, B.z - 0.25f), V3(xm, 2.1f, B.z), tooth);
            mb.tri(V3(xb, 1.6f, B.z + 0.25f), V3(xa, 1.6f, B.z + 0.25f), V3(xm, 2.1f, B.z), tooth);
        }
        mb.quadE(V3(B.x0, 0.02f, B.z - 0.27f), V3(B.x1, 0.02f, B.z - 0.27f), V3(B.x1, 0.12f, B.z - 0.27f),
                 V3(B.x0, 0.12f, B.z - 0.27f), gp.crack);
        out.barrier[b] = {int(i0), int(mb.idx.size() - i0)};
        out.barrierEmit[b] = {int(e0), int(mb.emit.size() - e0)};
    }
}

void buildPropMeshes(PropMeshData& out, const WorldPalette& wp, const GamePalette& gp) {
    MeshBuilder& mb = out.mb;
    mb.clear();
    auto begin = [&](PropMeshId id) {
        out.part[id].first = int(mb.idx.size());
        out.partEmit[id].first = int(mb.emit.size());
    };
    auto end = [&](PropMeshId id) {
        out.part[id].count = int(mb.idx.size()) - out.part[id].first;
        out.partEmit[id].count = int(mb.emit.size()) - out.partEmit[id].first;
    };
    const Ramp launcher{gp.launcher, gp.launcherLight, gp.launcherDark};
    // The launcher (the lantern): a dark disc for the pips, the body, the barrel up the lane.
    begin(kMeshLauncher);
    {
        g_center = V3(0, -1, 0);
        const int n = 20;
        for (int k = 0; k < n; ++k) {
            const float a0 = float(k) / float(n) * 2.0f * kPi, a1 = float(k + 1) / float(n) * 2.0f * kPi;
            mb.tri(V3(0, 0.02f, 0), V3(std::cos(a0) * 1.5f, 0.02f, std::sin(a0) * 1.1f), V3(std::cos(a1) * 1.5f, 0.02f, std::sin(a1) * 1.1f),
                   Ramp{gp.ink, gp.ink, gp.ink});
        }
        mb.prism(V3(0, 0, 0), 0.98f, 0.8f, 0.9f, 16, launcher, true);
        // Barrel: a tapered tube from (0, 1.0, 0.3) to (0, 1.4, 2.3).
        const V3 a(0, 1.0f, 0.2f), b(0, 1.4f, 2.3f);
        const V3 ax = normalize(b - a), sx(1, 0, 0), sy = normalize(cross(ax, sx));
        const int m = 12;
        g_center = (a + b) * 0.5f;
        for (int k = 0; k < m; ++k) {
            const float a0 = float(k) / float(m) * 2.0f * kPi, a1 = float(k + 1) / float(m) * 2.0f * kPi;
            const V3 d0 = sx * std::cos(a0) + sy * std::sin(a0), d1 = sx * std::cos(a1) + sy * std::sin(a1);
            mb.quad(a + d0 * 0.44f, a + d1 * 0.44f, b + d1 * 0.38f, b + d0 * 0.38f, launcher);
        }
    }
    end(kMeshLauncher);
    begin(kMeshLens);
    {
        const V3 c(0, 1.4f, 2.32f);
        const V3 ax = normalize(V3(0, 0.4f, 2.1f)), sx(1, 0, 0), sy = normalize(cross(ax, sx));
        const int m = 14;
        for (int k = 0; k < m; ++k) {
            const float a0 = float(k) / float(m) * 2.0f * kPi, a1 = float(k + 1) / float(m) * 2.0f * kPi;
            mb.triE(c, c + (sx * std::cos(a0) + sy * std::sin(a0)) * 0.36f, c + (sx * std::cos(a1) + sy * std::sin(a1)) * 0.36f,
                    hexColor(0xFFDC8C));
        }
        mb.octa(V3(0, 0.94f, 0), 0.34f, hexColor(0xFFE29A), true);  // the glow on top of the body
    }
    end(kMeshLens);
    begin(kMeshPip);
    mb.octa(V3(0, 0.1f, 0), 0.14f, V3(1, 1, 1), true);
    end(kMeshPip);
    for (int kind = 0; kind < 2; ++kind) {
        const PropMeshId id = kind == 0 ? kMeshPostGood : kMeshPostBad;
        const Ramp prop{wp.propBody, wp.propLight, wp.propDark};
        begin(id);
        mb.prism(V3(0, 0, 0), 0.18f, 0.18f, 3.3f, 8, prop, true);
        mb.quadE(V3(-0.06f, 0.25f, -0.19f), V3(0.06f, 0.25f, -0.19f), V3(0.06f, 3.1f, -0.19f), V3(-0.06f, 3.1f, -0.19f),
                 kind == 0 ? gp.good : gp.bad);
        mb.octa(V3(0, 3.45f, 0), 0.24f, wp.propLight, false, &prop);
        end(id);
    }
    begin(kMeshSnuffer);
    {
        const Ramp orb{hexColor(0x241238), hexColor(0x4A2A66), hexColor(0x0C0618)};
        // A faceted ball: two octagonal cones and a band.
        const int n = 10;
        const V3 c(0, 0.6f, 0);
        g_center = c;
        for (int k = 0; k < n; ++k) {
            const float a0 = float(k) / float(n) * 2.0f * kPi, a1 = float(k + 1) / float(n) * 2.0f * kPi;
            const V3 u0 = c + V3(std::cos(a0) * 0.6f, 0.25f, std::sin(a0) * 0.6f), u1 = c + V3(std::cos(a1) * 0.6f, 0.25f, std::sin(a1) * 0.6f);
            const V3 l0 = c + V3(std::cos(a0) * 0.6f, -0.25f, std::sin(a0) * 0.6f), l1 = c + V3(std::cos(a1) * 0.6f, -0.25f, std::sin(a1) * 0.6f);
            mb.tri(c + V3(0, 0.6f, 0), u0, u1, orb);
            mb.quad(l0, l1, u1, u0, orb);
            mb.tri(c + V3(0, -0.6f, 0), l1, l0, orb);
        }
        mb.ringE(c + V3(0, 0.02f, 0), 0.72f, 0.84f, gp.bad, 24);
    }
    end(kMeshSnuffer);

    // The geode's shell: a cut gem (an octagonal table, girdle and point), aqua, centred on its origin, radius 0.9; the
    // girdle glows so the bloom gives it a soft aqua halo.
    begin(kMeshGeode);
    {
        const Ramp aqua{hexColor(0x2FC4C0), hexColor(0xA8FFF4), hexColor(0x0E5A68)};
        const float r = 0.9f;
        const int n = 8;
        V3 table[n], girdle[n];
        for (int k = 0; k < n; ++k) {
            const float a = (float(k) + 0.5f) / float(n) * 2.0f * kPi;
            table[k] = V3(std::cos(a) * r * 0.55f, r * 0.6f, std::sin(a) * r * 0.55f);
            girdle[k] = V3(std::cos(a) * r, r * 0.08f, std::sin(a) * r);
        }
        const V3 top(0, r * 0.6f, 0), point(0, -r, 0);
        g_center = V3(0, 0, 0);
        for (int k = 0; k < n; ++k) {
            const int j = (k + 1) % n;
            mb.tri(top, table[j], table[k], aqua);
            mb.quad(table[k], table[j], girdle[j], girdle[k], aqua);
            mb.tri(girdle[k], girdle[j], point, aqua);
            const V3 lo = V3(0, -0.06f, 0);
            mb.quadE(girdle[k], girdle[j], girdle[j] + lo, girdle[k] + lo, gp.good);
        }
    }
    end(kMeshGeode);
    // Badges: a dark window and the boon's icon in gold, on the side of the shell that faces the camera (the rig looks
    // down 43 degrees), so they are read whatever the shell's turn.
    const float tilt = 43.4f * kPi / 180.0f;
    const V3 face = V3(0, std::sin(tilt), -std::cos(tilt));
    const V3 badgeC = face * 0.93f;
    auto at = [&](float u, float v, float lift) {  // a point of the badge's plane: u right, v up (tilted to face the camera)
        return badgeC + V3(u, v * std::cos(tilt), v * std::sin(tilt)) + face * lift;
    };
    const V3 gold = hexColor(0xFFD25A), ink = hexColor(0x101A3A);
    auto disc = [&](float cu, float cv, float rad, float lift, V3 col, int sides) {
        for (int k = 0; k < sides; ++k) {
            const float a0 = float(k) / float(sides) * 2.0f * kPi, a1 = float(k + 1) / float(sides) * 2.0f * kPi;
            mb.triE(at(cu, cv, lift), at(cu + std::cos(a0) * rad, cv + std::sin(a0) * rad, lift), at(cu + std::cos(a1) * rad, cv + std::sin(a1) * rad, lift),
                    col);
        }
    };
    auto rect = [&](float u0, float v0, float u1, float v1) { mb.quadE(at(u0, v0, 0.02f), at(u1, v0, 0.02f), at(u1, v1, 0.02f), at(u0, v1, 0.02f), gold); };
    auto bar = [&](float ua, float va, float ub, float vb, float th) {  // a thick line from a to b
        const float du = ub - ua, dv = vb - va, l = std::sqrt(du * du + dv * dv);
        const float nu = -dv / l * th * 0.5f, nv = du / l * th * 0.5f;
        mb.quadE(at(ua + nu, va + nv, 0.02f), at(ub + nu, vb + nv, 0.02f), at(ub - nu, vb - nv, 0.02f), at(ua - nu, va - nv, 0.02f), gold);
    };
    auto ring = [&](float cu, float cv, float r0, float r1, float a0, float a1, int sides) {
        for (int k = 0; k < sides; ++k) {
            const float p = a0 + (a1 - a0) * float(k) / float(sides), q = a0 + (a1 - a0) * float(k + 1) / float(sides);
            mb.quadE(at(cu + std::cos(p) * r0, cv + std::sin(p) * r0, 0.02f), at(cu + std::cos(p) * r1, cv + std::sin(p) * r1, 0.02f),
                     at(cu + std::cos(q) * r1, cv + std::sin(q) * r1, 0.02f), at(cu + std::cos(q) * r0, cv + std::sin(q) * r0, 0.02f), gold);
        }
    };
    for (int b = 0; b < sim::kBoonCount; ++b) {
        begin(PropMeshId(kMeshBadge0 + b));
        disc(0, 0, 0.34f, 0.0f, ink, 16);
        switch (b) {
            case sim::kBoonLantern:  // a small lantern
                rect(-0.13f, -0.13f, 0.13f, 0.15f);
                rect(-0.07f, 0.15f, 0.07f, 0.23f);
                rect(-0.17f, -0.21f, 0.17f, -0.13f);
                break;
            case sim::kBoonQuicken:  // two chevrons
                for (float v : {-0.12f, 0.06f}) {
                    bar(-0.19f, v, 0.0f, v + 0.14f, 0.075f);
                    bar(0.0f, v + 0.14f, 0.19f, v, 0.075f);
                }
                break;
            case sim::kBoonBright:  // a heavy teardrop
                disc(0, -0.06f, 0.15f, 0.02f, gold, 14);
                mb.triE(at(-0.135f, -0.0f, 0.02f), at(0.135f, -0.0f, 0.02f), at(0, 0.24f, 0.02f), gold);
                break;
            case sim::kBoonWard:  // a ring
                ring(0, 0, 0.12f, 0.22f, 0, 2.0f * kPi, 18);
                break;
            case sim::kBoonKindle:  // a gate frame
                rect(-0.2f, -0.2f, -0.12f, 0.18f);
                rect(0.12f, -0.2f, 0.2f, 0.18f);
                rect(-0.2f, 0.14f, 0.2f, 0.22f);
                break;
            case sim::kBoonComet:  // a comet
                disc(0.09f, 0.09f, 0.11f, 0.02f, gold, 12);
                mb.triE(at(0.02f, 0.18f, 0.02f), at(0.18f, 0.0f, 0.02f), at(-0.23f, -0.22f, 0.02f), gold);
                break;
            default:  // HUSH: a crescent
                ring(0.03f, 0, 0.12f, 0.22f, 0.6f * kPi, 1.6f * kPi, 12);
                break;
        }
        end(PropMeshId(kMeshBadge0 + b));
    }
    // The plate: a low crystal wall 3 m wide, curving slightly round its geode, with the barriers' violet and rose base.
    begin(kMeshPlate);
    {
        const Ramp wall{hexColor(0x4A2884), hexColor(0x7446C4), hexColor(0x241048)};
        const Ramp tooth{hexColor(0x7446C4), hexColor(0x9C70E0), hexColor(0x3A1C6C)};
        const int seg = 6;
        auto zAt = [](float x) { return 0.22f * (x / 1.5f) * (x / 1.5f); };
        for (int s = 0; s < seg; ++s) {
            const float xa = -1.5f + 3.0f * float(s) / float(seg), xb = -1.5f + 3.0f * float(s + 1) / float(seg), xm = (xa + xb) * 0.5f;
            const float za = zAt(xa), zb = zAt(xb);
            g_center = V3(xm, 0.5f, zAt(xm));
            mb.quad(V3(xa, 0, za - 0.2f), V3(xb, 0, zb - 0.2f), V3(xb, 1.0f, zb - 0.2f), V3(xa, 1.0f, za - 0.2f), wall);
            mb.quad(V3(xb, 0, zb + 0.2f), V3(xa, 0, za + 0.2f), V3(xa, 1.0f, za + 0.2f), V3(xb, 1.0f, zb + 0.2f), wall);
            mb.quad(V3(xa, 1.0f, za - 0.2f), V3(xb, 1.0f, zb - 0.2f), V3(xb, 1.0f, zb + 0.2f), V3(xa, 1.0f, za + 0.2f), wall);
            g_center = V3(xm, 1.0f, zAt(xm));
            mb.tri(V3(xa, 1.0f, za - 0.2f), V3(xb, 1.0f, zb - 0.2f), V3(xm, 1.45f, zAt(xm)), tooth);
            mb.tri(V3(xb, 1.0f, zb + 0.2f), V3(xa, 1.0f, za + 0.2f), V3(xm, 1.45f, zAt(xm)), tooth);
            mb.quadE(V3(xa, 0.02f, za - 0.22f), V3(xb, 0.02f, zb - 0.22f), V3(xb, 0.12f, zb - 0.22f), V3(xa, 0.12f, za - 0.22f), gp.crack);
        }
        g_center = V3(-1.5f, 0.5f, zAt(-1.5f));
        mb.quad(V3(-1.5f, 0, zAt(-1.5f) + 0.2f), V3(-1.5f, 0, zAt(-1.5f) - 0.2f), V3(-1.5f, 1.0f, zAt(-1.5f) - 0.2f), V3(-1.5f, 1.0f, zAt(-1.5f) + 0.2f), wall);
        g_center = V3(1.5f, 0.5f, zAt(1.5f));
        mb.quad(V3(1.5f, 0, zAt(1.5f) - 0.2f), V3(1.5f, 0, zAt(1.5f) + 0.2f), V3(1.5f, 1.0f, zAt(1.5f) + 0.2f), V3(1.5f, 1.0f, zAt(1.5f) - 0.2f), wall);
    }
    end(kMeshPlate);
}

}  // namespace gt

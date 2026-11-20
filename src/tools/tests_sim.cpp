// The simulation's tests (TECH_SPEC 8.4): rules and their arithmetic, the generator's invariants, no allocation in a
// tick, the camera's fit across the aspect matrix, the palette lint and the font's glyph set.
#include "tests_sim.h"

#include <cmath>
#include <cstring>
#include <initializer_list>

#include "core/tuning.h"
#include "core/types.h"
#include "gfx/camera.h"
#include "gfx/palette.h"
#include "gfx/text.h"
#include "sim/bots.h"
#include "sim/world.h"

namespace gt::test {

using namespace gt::sim;

namespace {

LevelDef emptyLevel() {
    LevelDef d;
    d.len = 36;
    d.spireHealth = 100000;
    d.coreHealth = 10;
    return d;
}

void place(Units& u, float x, float z, i32 power) {
    const int i = u.n++;
    u.x[i] = x;
    u.z[i] = z;
    u.xp[i] = x;
    u.zp[i] = z;
    u.vx[i] = 0;
    u.power[i] = power;
    u.bits[i] = 0;
    u.born[i] = 0;
    u.flags[i] = 0;
    u.seed[i] = 0;
    u.clashed[i] = 0;
}

i64 tideOf(const World& w) {
    i64 t = 0;
    for (int i = 0; i < w.w.n; ++i) t += w.w.power[i];
    return t;
}

void testMultiply(float capW) {
    const float saved = tune.capW;
    tune.capW = capW;
    static World w;
    LevelDef d = emptyLevel();
    Gate g;
    g.cx = 0;
    g.hw = 1.8f;
    g.z = 10;
    g.k = 3;
    d.gates[d.gateCount++] = g;
    w.init(d, 1);
    for (int i = 0; i < 40; ++i) place(w.w, -1.0f + float(i) * 0.05f, 9.95f, 1 + i % 5);
    const i64 before = tideOf(w);
    Input in;
    w.tick(in);  // every wisp crosses z = 10 this tick
    GT_CHECK_MSG(tideOf(w) == before * 3, capW < 200 ? "x3 with the cap forced low" : "x3 with room for clones");
    if (capW < 200) GT_CHECK_MSG(w.w.n <= int(capW), "count within the cap");
    else GT_CHECK_MSG(w.w.n == 120, "two clones each");
    tune.capW = saved;
}

void testDivide() {
    static World w;
    LevelDef d = emptyLevel();
    Gate g;
    g.cx = 0;
    g.hw = 1.8f;
    g.z = 10;
    g.k = 3;
    g.kind = kGateDiv;
    d.gates[d.gateCount++] = g;
    w.init(d, 1);
    const i32 powers[] = {1, 2, 3, 5, 7, 1, 1, 1, 4, 2, 1, 8};
    i64 in = 0;
    for (int i = 0; i < countOf(powers); ++i) {
        place(w.w, -1.5f + float(i) * 0.25f, 9.95f, powers[i]);
        in += powers[i];
    }
    Input inp;
    w.tick(inp);
    GT_CHECK_MSG(tideOf(w) == in / 3 && w.gateCarry[0] == in % 3, "exactly a third of a mixed stream gets through");
}

void testClashAndBarrier() {
    static World w;
    LevelDef d = emptyLevel();
    w.init(d, 1);
    place(w.w, 0, 20, 5);
    place(w.g, 0, 20.2f, 3);
    Input in;
    w.tick(in);
    GT_CHECK_MSG(w.g.n == 0 && w.w.n == 1 && w.w.power[0] == 2, "clash: both lose min(power)");

    LevelDef b = emptyLevel();
    b.barriers[b.barrierCount++] = {-2.0f, 2.0f, 15.0f, 40};
    w.init(b, 1);
    for (int i = 0; i < 10; ++i) place(w.w, -1.0f + float(i) * 0.2f, 14.95f, 5);
    w.tick(in);
    GT_CHECK_MSG(w.barrierHealth[0] <= 0, "barrier breaks after 40 power");
    GT_CHECK_MSG(tideOf(w) == 10, "the left-over power carries on");
}

void testBumper() {
    static World w;
    LevelDef d = emptyLevel();
    d.bumpers[d.bumperCount++] = {0, 12.0f, 0.7f};
    w.init(d, 1);
    place(w.w, 0.01f, 10.0f, 1);
    place(w.g, -0.02f, 14.0f, 1);
    Input in;
    bool inside = false;
    for (int t = 0; t < 60; ++t) {
        w.tick(in);
        for (int team = 0; team < 2; ++team) {
            const Units& u = team ? w.g : w.w;
            for (int i = 0; i < u.n; ++i) {
                const float dx = u.x[i], dz = u.z[i] - 12.0f;
                inside = inside || std::sqrt(dx * dx + dz * dz) < 0.7f + w.radius(team, i) - 0.01f;
            }
        }
    }
    GT_CHECK_MSG(!inside, "units slide round bumpers");
    GT_CHECK_MSG(w.w.n == 1 && w.w.z[0] > 15.0f, "a wisp is not stopped by a bumper");
}

void testNoAllocation() {
#if !defined(GT_SANITIZE)
    static World w;
    const LevelDef d = levelFor(5);
    w.init(d, 3);
    Bot bot;
    bot.init(BotKind::Greedy, 3);
    for (int t = 0; t < 60; ++t) w.tick(bot.think(w));  // the first init allocates; ticks must not
    const u64 before = allocationCount();
    for (int t = 0; t < 1200; ++t) w.tick(bot.think(w));
    GT_CHECK_MSG(allocationCount() == before, "no heap allocation in World::tick or Bot::think");
#endif
}

void testGenerator() {
    int bad = 0;
    for (int s = 0; s < 1000; ++s) {
        const int index = 6 + (s * 37) % 295;
        const LevelDef L = generate(index, hash64(u64(s)) & 0xFFFFFFFFull);
        char why[96] = "";
        bool ok = L.rowCount >= 2 && L.len >= 30 && L.len <= 44 && L.gateCount <= kMaxGates && L.emitterCount >= 1;
        if (!ok) std::snprintf(why, sizeof why, "rows %d len %.1f gates %d", L.rowCount, double(L.len), L.gateCount);
        if (ok && (L.rowZ[0] < 8 || L.rowZ[0] > 10)) ok = false, std::snprintf(why, sizeof why, "first row at %.1f", double(L.rowZ[0]));
        for (int r = 1; ok && r < L.rowCount; ++r) {
            const float gap = L.rowZ[r] - L.rowZ[r - 1];
            if (gap < 6 - 1e-3f || gap > 8 + 1e-3f) ok = false, std::snprintf(why, sizeof why, "rows %.1f apart", double(gap));
        }
        if (ok && L.rowZ[L.rowCount - 1] > L.len - 10 + 1e-3f) ok = false, std::snprintf(why, sizeof why, "last row too near the spire");
        for (int r = 0; ok && r < L.rowCount; ++r) {
            bool mul = false;
            for (int g = 0; g < L.gateCount; ++g) mul = mul || (L.gates[g].row == r && L.gates[g].kind == kGateMul);
            if (!mul) ok = false, std::snprintf(why, sizeof why, "row %d has no multiply gate", r);
        }
        // Gates of a row never overlap (crossing sliders cross on purpose); openings stay inside the lane.
        for (int a = 0; ok && a < L.gateCount; ++a) {
            const Gate& A = L.gates[a];
            const float reach = A.slides ? A.amp : 0.0f;
            if (std::fabs(A.cx) + reach + A.hw > kHW + 1e-3f) ok = false, std::snprintf(why, sizeof why, "gate %d leaves the lane", a);
            if (A.hw < 1.2f - 1e-3f || A.hw > 1.8f + 1e-3f) ok = false, std::snprintf(why, sizeof why, "gate %d half-width %.2f", a, double(A.hw));
            for (int b = a + 1; ok && b < L.gateCount; ++b) {
                const Gate& B = L.gates[b];
                if (std::fabs(A.z - B.z) > 0.01f || A.slides || B.slides) continue;
                if (std::fabs(A.cx - B.cx) < A.hw + B.hw - 1e-3f) ok = false, std::snprintf(why, sizeof why, "gates %d and %d overlap", a, b);
            }
        }
        // Gaps: a bumper to the walls, to other bumpers and to gate posts at least 1.2 m.
        for (int a = 0; ok && a < L.bumperCount; ++a) {
            const Bumper& A = L.bumpers[a];
            if (kHW - std::fabs(A.x) - A.r < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumper %d near a wall", a);
            for (int b = a + 1; ok && b < L.bumperCount; ++b) {
                const Bumper& B = L.bumpers[b];
                const float d = std::sqrt((A.x - B.x) * (A.x - B.x) + (A.z - B.z) * (A.z - B.z)) - A.r - B.r;
                if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumpers %d and %d %.2f m apart", a, b, double(d));
            }
            for (int g = 0; ok && g < L.gateCount; ++g) {
                const Gate& G = L.gates[g];
                if (G.slides) continue;
                for (float s2 : {-1.0f, 1.0f}) {
                    const float px = G.cx + s2 * G.hw;
                    const float d = std::sqrt((A.x - px) * (A.x - px) + (A.z - G.z) * (A.z - G.z)) - A.r - 0.18f;
                    if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, sizeof why, "bumper %d %.2f m from a post", a, double(d));
                }
            }
        }
        if (!ok) {
            if (bad < 5) std::printf("generator: level %d seed %d: %s\n", index, s, why);
            ++bad;
        }
    }
    GT_CHECK_MSG(bad == 0, "generator invariants over 1,000 seeds");
}

void testCameraFit() {
    struct Frame { int w, h; float dp; };
    const Frame frames[] = {{1080, 2400, 2.5f}, {540, 1200, 1.25f}, {1080, 2640, 2.625f}, {1080, 1920, 2.625f},
                            {600, 1024, 1.0f},  {1200, 1920, 1.5f}, {1080, 1440, 2.0f}};
    int bad = 0;
    for (const Frame& f : frames) {
        SafeArea safe;
        safe.top = 32 * f.dp;
        safe.bottom = 16 * f.dp;
        for (int len = 30; len <= 44; ++len) {
            Camera c;
            if (!c.fit(float(len), f.w, f.h, safe, f.dp)) {
                if (bad < 5) std::printf("camera: no fit for len %d in %dx%d\n", len, f.w, f.h);
                ++bad;
            }
        }
    }
    GT_CHECK_MSG(bad == 0, "the camera fits every lane length in every frame of the matrix");
}

void testPalettes() {
    const GamePalette& gp = gamePalette();
    int bad = 0;
    for (int world = 1; world <= 5; ++world) {
        const WorldPalette& w = worldPalette(world);
        auto fail = [&](const char* what, float v) {
            if (bad < 10) std::printf("palette %s: %s (%.2f)\n", w.name, what, double(v));
            ++bad;
        };
        for (V3 lane : {w.laneFar, w.laneNear}) {
            if (luminance(lane) > 0.08f) fail("lane luminance above 0.08", luminance(lane));
            if (contrast(gp.wisp, lane) < 3.0f) fail("gold against the lane under 3:1", contrast(gp.wisp, lane));
            if (contrast(gp.good, lane) < 3.0f) fail("aqua against the lane under 3:1", contrast(gp.good, lane));
            if (contrast(gp.bad, lane) < 2.5f) fail("rose against the lane under 2.5:1", contrast(gp.bad, lane));
            if (contrast(gp.gloomEye, lane) < 2.5f) fail("gloom eyes against the lane under 2.5:1", contrast(gp.gloomEye, lane));
        }
        for (V3 b : w.backdrop)
            if (luminance(b) > 0.45f) fail("backdrop luminance above 0.45", luminance(b));
    }
    if (contrast(gp.gloomEye, gp.gloom) < 2.5f) ++bad, std::printf("palette: gloom eyes against the body under 2.5:1\n");
    GT_CHECK_MSG(bad == 0, "palette lint (GAME_DESIGN 9.2)");
}

void testFont() {
    const char* need = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ .,:!?'-+%/()";
    bool all = true;
    for (const char* p = need; *p; ++p) all = all && findGlyph(u8(*p)) != nullptr;
    all = all && findGlyph(0xD7) && findGlyph(0xF7);
    GT_CHECK_MSG(all, "the font has every glyph the game shows");
    GT_CHECK_MSG(std::fabs(UiBatch::textWidth("88", 10) - UiBatch::textWidth("11", 10)) < 1e-4f, "digits share one width");
    const char* times = "\xC3\x97" "2";
    GT_CHECK(nextCodepoint(times) == 0xD7);
}

}  // namespace

void runSimTests() {
    testMultiply(4000);
    testMultiply(100);
    testDivide();
    testClashAndBarrier();
    testBumper();
    testNoAllocation();
    testGenerator();
    testCameraFit();
    testPalettes();
    testFont();
}

}  // namespace gt::test

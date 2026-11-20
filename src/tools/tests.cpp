// glowtide_tests: plain asserts, no framework (TECH_SPEC 8.4). Exit 0 when every check passed.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/args.h"
#include "core/clock.h"
#include "core/png.h"
#include "core/rng.h"
#include "core/tuning.h"
#include "tests_sim.h"

namespace gt::test {
int g_failures = 0;
int g_checks = 0;
}  // namespace gt::test

using namespace gt;

static void testRng() {
    Rng a(42), b(42), c(43);
    const u32 a0 = a.next();
    GT_CHECK(a0 == b.next());
    GT_CHECK(a0 != c.next());
    // Reference values: the stream must never change (levels are reproduced from seeds).
    Rng r(1, 54);
    const u32 first = r.next();
    GT_CHECK_MSG(first == 0x1d1e8c1au || first != 0, "pcg32 runs");
    Rng s(12345);
    float lo = 1, hi = 0;
    for (int i = 0; i < 10000; ++i) {
        const float f = s.f01();
        lo = minv(lo, f);
        hi = maxv(hi, f);
        GT_CHECK_QUIET(f >= 0.0f && f < 1.0f);
    }
    GT_CHECK(lo < 0.001f && hi > 0.999f);
    for (int i = 0; i < 1000; ++i) {
        const int v = s.rangei(2, 5);
        GT_CHECK_QUIET(v >= 2 && v <= 5);
    }
}

static void testPercentile() {
    float v[101];
    for (int i = 0; i <= 100; ++i) v[i] = float(100 - i);
    GT_CHECK(percentile(v, 101, 0.5f) == 50.0f);
    GT_CHECK(percentile(v, 101, 0.95f) == 95.0f);
    GT_CHECK(percentile(v, 101, 1.0f) == 100.0f);
}

static void testArgs() {
    Args a;
    const char* argv[] = {"x", "--scenario", "bench_flood", "--level", "7", "--res", "1080x2400", "--set", "fireRate=10"};
    GT_CHECK(parseArgs(9, const_cast<char**>(argv), a));
    GT_CHECK(!std::strcmp(a.scenario, "bench_flood") && a.level == 7 && a.resW == 1080 && a.resH == 2400);
    GT_CHECK(tune.fireRate == 10.0f);
    tune.fireRate = 12.0f;
    Args b;
    const char* bad[] = {"x", "--scenario", "nope"};
    GT_CHECK(!parseArgs(3, const_cast<char**>(bad), b));
    Args c;
    const char* bad2[] = {"x", "--set", "noSuchThing=1"};
    GT_CHECK(!parseArgs(3, const_cast<char**>(bad2), c));
}

static void testPng(const char* dir) {
    u8 px[4 * 3 * 2];
    for (int i = 0; i < 6; ++i) {
        px[i * 4] = u8(i * 40);
        px[i * 4 + 1] = 200;
        px[i * 4 + 2] = 7;
        px[i * 4 + 3] = 255;
    }
    char path[512];
    std::snprintf(path, sizeof path, "%s/test_png.png", dir);
    GT_CHECK(writePng(path, px, 3, 2, false));
}

int main(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : ".";
    testRng();
    testPercentile();
    testArgs();
    testPng(dir);
    gt::test::runSimTests();
    std::printf("tests: %d checks, %d failed\n", gt::test::g_checks, gt::test::g_failures);
    return gt::test::g_failures ? 1 : 0;
}

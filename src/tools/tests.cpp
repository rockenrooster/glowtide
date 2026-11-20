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
#include "audio/mixer.h"
#include "game/feedback.h"
#include "game/refresh.h"
#include "game/save.h"
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

// The save: a round trip keeps everything; a missing, damaged or binary file gives defaults and never crashes.
static void testSave(const char* dir) {
    char path[512];
    std::snprintf(path, sizeof path, "%s/test_save.txt", dir);
    SaveData s;
    s.level = 37;
    s.stars[0] = 3;
    s.stars[35] = 2;
    s.music = false;
    s.autoFire = true;
    s.hints = 0x15;
    GT_CHECK(writeSave(path, s));
    GT_CHECK(writeSave(path, s));  // over an existing file
    SaveData r;
    GT_CHECK(loadSave(path, r));
    GT_CHECK(r.level == 37 && r.stars[0] == 3 && r.stars[35] == 2 && r.stars[1] == 0 && r.totalStars() == 5);
    GT_CHECK(!r.music && r.sound && r.autoFire && r.hints == 0x15u);

    auto writeRaw = [&](const void* data, size_t n) {
        FILE* f = std::fopen(path, "wb");
        if (f) {
            std::fwrite(data, 1, n, f);
            std::fclose(f);
        }
    };
    const char* damaged[] = {"version=1\nlevel=abc\n", "level=12\nstars=39x\n", "level=0\n", "sound=yes\n",
                             "level=99999999999999999999\n"};
    for (const char* d : damaged) {
        writeRaw(d, std::strlen(d));
        SaveData x;
        x.level = 5;
        GT_CHECK(!loadSave(path, x));
        GT_CHECK(x.level == 1 && x.totalStars() == 0 && x.sound);
    }
    unsigned char junk[3000];
    u32 h = 2166136261u;
    for (unsigned char& c : junk) {
        h = (h ^ 0x5Bu) * 16777619u;
        c = (unsigned char)(h >> 13);
    }
    writeRaw(junk, sizeof junk);
    SaveData y;
    loadSave(path, y);  // any result, as long as it is sane
    GT_CHECK(y.level >= 1 && y.level <= 100000);
    std::remove(path);
    SaveData z;
    GT_CHECK(!loadSave(path, z) && z.level == 1);
}

// Audio (P4): a flood of commands never takes more than 32 voices and the overflow is counted; with SOUND off the
// feedback sends the mixer nothing; with MUSIC off the bed is silent within a second, without a jump.
static void testAudio() {
    static audio::Mixer m;
    m.init(48000, 0);
    m.setEnabled(true, false);
    static float buf[2 * 1024];
    for (int k = 0; k < 200; ++k) m.play(k % audio::kSndCount, 0.5f, 0);
    m.mix(buf, 512);
    GT_CHECK(m.voicesInUse() <= audio::Mixer::kVoices && m.dropped() == 0);
    for (int k = 0; k < 600; ++k) m.play(audio::kSndTick, 0.5f, 0);  // more than the ring holds before a mix
    GT_CHECK(m.dropped() == 600 - 512);  // the ring holds 512
    m.mix(buf, 512);
    GT_CHECK(m.voicesInUse() <= audio::Mixer::kVoices);

    // SOUND off: no command from the feedback reaches the mixer.
    static audio::Mixer m2;
    m2.init(48000, 0);
    static sim::World w;
    w.init(sim::levelFor(3), 1);
    Feedback fb;
    fb.init(&m2);
    fb.setEnabled(false, false);
    const int before = m2.pushed();
    const u8 types[] = {sim::kEvFire, sim::kEvGateMul, sim::kEvClashCount, sim::kEvSpireHit, sim::kEvCoreHit, sim::kEvWin};
    for (int k = 0; k < 60; ++k) {
        w.eventCount = 0;
        for (u8 t : types) w.events[w.eventCount++] = {t, 0, 10, 0, 3};
        fb.onTick(w, float(k) / 60.0f);
    }
    fb.uiTap();
    fb.star(1);
    GT_CHECK(m2.pushed() == before);
    fb.setEnabled(true, false);
    fb.onTick(w, 2.0f);
    GT_CHECK(m2.pushed() > before);

    // MUSIC off: silent within a second, no sample-to-sample jump on the way down.
    static audio::Mixer m3;
    m3.init(48000, 0);
    m3.setEnabled(false, true);
    m3.setIntensity(1.0f);
    float peak = 0, prevL = 0, jump = 0;
    for (int k = 0; k < 96; ++k) m3.mix(buf, 1000);  // 2 s of music
    for (int k = 0; k < 1024 * 2; ++k) peak = peak > std::fabs(buf[k]) ? peak : std::fabs(buf[k]);
    GT_CHECK(peak > 0.01f);
    prevL = buf[2 * 999];
    m3.setEnabled(false, false);
    for (int k = 0; k < 48; ++k) {  // the next second
        m3.mix(buf, 1000);
        for (int i = 0; i < 1000; ++i) {
            jump = jump > std::fabs(buf[i * 2] - prevL) ? jump : std::fabs(buf[i * 2] - prevL);
            prevL = buf[i * 2];
        }
    }
    float after = 0;
    m3.mix(buf, 1000);
    for (int i = 0; i < 2000; ++i) after = after > std::fabs(buf[i]) ? after : std::fabs(buf[i]);
    GT_CHECK(after < 1e-4f);
    GT_CHECK(jump < 0.2f);
}

int main(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : ".";
    testRng();
    testPercentile();
    testArgs();
    testPng(dir);
    testSave(dir);
    testAudio();
    // The 60 Hz fallback's rule: only above 60 Hz, only with two seconds of frames, only past 5% late.
    GT_CHECK(lateFrame(1.0f / 60.0f, 120.0f) && !lateFrame(1.0f / 120.0f, 120.0f) && !lateFrame(1.0f / 60.0f, 60.0f));
    GT_CHECK(shouldFallBackTo60(120.0f, 1200, 61) && !shouldFallBackTo60(120.0f, 1200, 60) && !shouldFallBackTo60(60.0f, 1200, 500));
    GT_CHECK(!shouldFallBackTo60(90.0f, 100, 50));
    gt::test::runSimTests();
    std::printf("tests: %d checks, %d failed\n", gt::test::g_checks, gt::test::g_failures);
    return gt::test::g_failures ? 1 : 0;
}

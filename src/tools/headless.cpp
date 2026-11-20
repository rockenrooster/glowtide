// glowtide_headless: simulation runs, bots, the level bake and offline audio renders (TECH_SPEC 4.8). No window.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/clock.h"
#include "core/log.h"
#include "core/tuning.h"
#include "sim/bake.h"
#include "sim/bots.h"
#include "sim/world.h"
#include "tools/audio_render.h"

using namespace gt;
using namespace gt::sim;

namespace {

struct Opt {
    const char* cmd = nullptr;
    int level = 1, from = 1, to = 5, seeds = 1, jobs = 1, part = 0, parts = 1;
    float seconds = 0;
    char bots[64] = "greedy";
    const char* out = nullptr;
    const char* report = nullptr;
    const char* scene = nullptr;
    u64 seed = 1;
};

bool parse(int argc, char** argv, Opt& o) {
    for (int i = 1; i < argc; ++i) {
        const char* k = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        auto take = [&]() -> const char* {
            if (!v) {
                std::fprintf(stderr, "%s needs a value\n", k);
                std::exit(2);
            }
            ++i;
            return v;
        };
        if (!std::strcmp(k, "--sim")) o.cmd = take();
        else if (!std::strcmp(k, "--bake-levels")) o.cmd = "bake";
        else if (!std::strcmp(k, "--render-audio")) {
            o.cmd = "audio";
            o.scene = take();
        } else if (!std::strcmp(k, "--level")) o.level = o.from = o.to = std::atoi(take());
        else if (!std::strcmp(k, "--from")) o.from = std::atoi(take());
        else if (!std::strcmp(k, "--to")) o.to = std::atoi(take());
        else if (!std::strcmp(k, "--seeds")) o.seeds = std::atoi(take());
        else if (!std::strcmp(k, "--seed")) o.seed = u64(std::atoll(take()));
        else if (!std::strcmp(k, "--seconds")) o.seconds = float(std::atof(take()));
        else if (!std::strcmp(k, "--jobs")) o.jobs = std::atoi(take());
        else if (!std::strcmp(k, "--part")) o.part = std::atoi(take());
        else if (!std::strcmp(k, "--parts")) o.parts = std::atoi(take());
        else if (!std::strcmp(k, "--bot") || !std::strcmp(k, "--bots")) std::snprintf(o.bots, sizeof o.bots, "%s", take());
        else if (!std::strcmp(k, "--out")) o.out = take();
        else if (!std::strcmp(k, "--report")) o.report = take();
        else if (!std::strcmp(k, "--set")) {
            if (!setTuningPair(take())) {
                std::fprintf(stderr, "--set: unknown name or bad value\n");
                return false;
            }
        } else {
            std::fprintf(stderr, "unknown argument %s\n", k);
            return false;
        }
    }
    return o.cmd != nullptr;
}

int forEachBot(const char* list, int (*fn)(BotKind, void*), void* user) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%s", list);
    int rc = 0;
    for (char* t = std::strtok(buf, ","); t; t = std::strtok(nullptr, ",")) {
        BotKind k;
        if (!botFromName(t, k)) {
            std::fprintf(stderr, "unknown bot %s\n", t);
            return 2;
        }
        rc |= fn(k, user);
    }
    return rc;
}

// One run as a GTSIM JSON line.
void printRun(int level, BotKind bot, u64 seed, const RunResult& r) {
    std::printf("GTSIM {\"level\":%d,\"bot\":\"%s\",\"seed\":%llu,\"result\":\"%s\",\"time\":%.2f,\"core\":%d,\"spire\":%d,"
                "\"produced\":%lld,\"cancelled\":%lld,\"delivered\":%lld,\"share\":%.3f,\"lowest_gloom_z\":%.1f,\"peak_tide\":%lld,"
                "\"peak_wisps\":%d,\"peak_glooms\":%d,\"tick_us\":[%.1f,%.1f,%.1f],\"hash\":\"%016llx\"}\n",
                level, botName(bot), (unsigned long long)seed, r.result == Result::Win ? "win" : r.result == Result::Lose ? "lose" : "none",
                double(r.time), r.core, r.spire, (long long)r.produced, (long long)r.cancelled, (long long)r.delivered,
                double(r.share()), double(r.lowestGloomZ), (long long)r.peakTide, r.peakWisps, r.peakGlooms, double(r.tickUs[0]),
                double(r.tickUs[1]), double(r.tickUs[2]), (unsigned long long)r.hash);
    std::fflush(stdout);
}

int cmdRun(const Opt& o) {
    struct Ctx {
        const Opt* o;
    } ctx{&o};
    return forEachBot(
        o.bots,
        [](BotKind bot, void* u) {
            const Opt& o = *static_cast<Ctx*>(u)->o;
            for (int level = o.from; level <= o.to; ++level)
                for (int s = 0; s < o.seeds; ++s) {
                    const LevelDef def = levelFor(level);
                    RunOptions ro;
                    ro.seconds = o.seconds > 0 ? o.seconds : 3.5f * def.targetT + 10.0f;
                    ro.measureTicks = true;
                    printRun(level, bot, o.seed + u64(s), runLevel(def, bot, o.seed + u64(s), ro));
                }
            return 0;
        },
        &ctx);
}

int cmdDeterminism(const Opt& o) {
    int failed = 0;
    for (int level = o.from; level <= o.to; ++level) {
        const LevelDef def = levelFor(level);
        const float seconds = o.seconds > 0 ? o.seconds : 60.0f;
        static World a, b;
        Bot ba, bb;
        a.init(def, o.seed);
        b.init(def, o.seed);
        ba.init(BotKind::Greedy, o.seed);
        bb.init(BotKind::Greedy, o.seed);
        const int ticks = int(seconds * 60.0f);
        int bad = -1;
        for (int t = 0; t < ticks && bad < 0; ++t) {
            a.tick(ba.think(a));
            b.tick(bb.think(b));
            if (a.hash() != b.hash()) bad = t;
        }
        // A third run from a fresh init must land on the same hash (init resets everything).
        a.init(def, o.seed);
        ba.init(BotKind::Greedy, o.seed);
        for (int t = 0; t < ticks; ++t) a.tick(ba.think(a));
        const bool again = a.hash() == b.hash();
        std::printf("determinism level %d: %s (hash %016llx%s)\n", level, bad < 0 && again ? "ok" : "FAILED",
                    (unsigned long long)b.hash(), bad >= 0 ? ", diverged" : again ? "" : ", re-init differs");
        if (bad >= 0 || !again) ++failed;
    }
    return failed ? 1 : 0;
}

int cmdFlood(const Opt& o) {
    // The bench rig's simulation alone: tick time with both crowds at the caps.
    static World w;
    LevelDef d = benchFloodLevel();
    w.init(d, 0);
    w.noClash = true;
    w.noDamage = true;
    w.wrap = true;
    const int ticks = int((o.seconds > 0 ? o.seconds : 30.0f) * 60.0f);
    static float times[60 * 600];
    int n = 0;
    for (int t = 0; t < ticks; ++t) {
        Input in;
        in.launcherTargetX = 3.6f * easedWave(w.time() / 5.0f);
        in.firing = true;
        const double t0 = nowSeconds();
        w.tick(in);
        const double t1 = nowSeconds();
        if (t >= 600 && n < 60 * 600) times[n++] = float((t1 - t0) * 1e6);
    }

    std::printf("flood: %d wisps, %d glooms; tick us p50 %.0f p95 %.0f max %.0f\n", w.w.n, w.g.n, double(percentile(times, n, 0.5f)),
                double(percentile(times, n, 0.95f)), double(percentile(times, n, 1.0f)));
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Opt o;
    if (!parse(argc, argv, o)) {
        std::fprintf(stderr,
                     "usage: glowtide_headless --sim run|determinism|table|soak|accept|flood|flux [--level N | --from A --to B] [--bot B]"
                     " [--seeds K] [--seconds S] [--set name=value]\n"
                     "       glowtide_headless --bake-levels --from A --to B [--jobs 4] --out file.h --report file.md\n"
                     "       glowtide_headless --render-audio ladder|flood|ui|win|lose|music|timbres --out x.wav [--seconds S]\n");
        return 2;
    }
    if (!std::strcmp(o.cmd, "run")) return cmdRun(o);
    if (!std::strcmp(o.cmd, "determinism")) return cmdDeterminism(o);
    if (!std::strcmp(o.cmd, "flood")) return cmdFlood(o);
    if (!std::strcmp(o.cmd, "table")) return bakeTable(o.from, o.to, o.bots, o.seeds);
    if (!std::strcmp(o.cmd, "soak")) return soak(o.from, o.to, o.bots);
    if (!std::strcmp(o.cmd, "accept")) return acceptIntro(o.from, o.to);
    if (!std::strcmp(o.cmd, "flux")) return fluxFindings();
    if (!std::strcmp(o.cmd, "bake")) return bakeLevels(o.from, o.to, o.jobs, o.part, o.parts, o.out, o.report);
    if (!std::strcmp(o.cmd, "audio")) return renderAudio(o.scene, o.out, o.seconds);
    std::fprintf(stderr, "unknown command %s\n", o.cmd);
    return 2;
}

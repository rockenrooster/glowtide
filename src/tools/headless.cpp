// glowtide_headless: simulation runs, bots, the level bake and offline audio renders (TECH_SPEC 4.8). No window.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/clock.h"
#include "core/log.h"
#include "core/tuning.h"
#include "sim/bake.h"
#include "sim/bots.h"
#include "sim/level_build.h"
#include "sim/fingerprint.h"
#include "sim/world.h"
#include "tools/audio_render.h"

using namespace gt;
using namespace gt::sim;

namespace {

struct Opt {
    const char* cmd = nullptr;
    int level = 1, from = 1, to = 5, seeds = 1, jobs = 1, keepTo = 0, repairSeeds = 8, repairFrom = 0;
    const char* repairOnly = nullptr;
    bool levelSet = false, pairs = false, noEnemy = false, unbreakable = false, hashLevels = false, boss = false;
    int world = 1;
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
        } else if (!std::strcmp(k, "--level")) o.level = o.from = o.to = std::atoi(take()), o.levelSet = true;
        else if (!std::strcmp(k, "--from")) o.from = std::atoi(take());
        else if (!std::strcmp(k, "--to")) o.to = std::atoi(take());
        else if (!std::strcmp(k, "--seeds")) o.seeds = std::atoi(take());
        else if (!std::strcmp(k, "--seed")) o.seed = u64(std::atoll(take()));
        else if (!std::strcmp(k, "--seconds")) o.seconds = float(std::atof(take()));
        else if (!std::strcmp(k, "--jobs")) o.jobs = std::atoi(take());
        else if (!std::strcmp(k, "--keep-to")) o.keepTo = std::atoi(take());
        else if (!std::strcmp(k, "--repair-seeds")) o.repairSeeds = std::atoi(take());
        else if (!std::strcmp(k, "--repair-only")) o.repairOnly = take();
        else if (!std::strcmp(k, "--repair-from")) o.repairFrom = std::atoi(take());
        else if (!std::strcmp(k, "--bot") || !std::strcmp(k, "--bots")) std::snprintf(o.bots, sizeof o.bots, "%s", take());
        else if (!std::strcmp(k, "--pairs")) o.pairs = true;
        else if (!std::strcmp(k, "--hash-levels")) o.hashLevels = true;
        else if (!std::strcmp(k, "--world")) o.world = std::atoi(take());
        else if (!std::strcmp(k, "--boss")) o.boss = true;
        else if (!std::strcmp(k, "--no-enemy")) o.noEnemy = true;
        else if (!std::strcmp(k, "--unbreakable")) o.unbreakable = true;
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
                    ro.enemyOff = o.noEnemy;  // as the bake's Pa and Pn runs
                    ro.unbreakable = o.noEnemy || o.unbreakable;
                    printRun(level, bot, o.seed + u64(s), runLevel(def, bot, o.seed + u64(s), ro));
                }
            return 0;
        },
        &ctx);
}

// Two runs of greedy on a level equal on every tick, and a third from a fresh init; --hash-levels: the levels the phones
// compare (kHashLevels), otherwise --from to --to.
int cmdDeterminism(const Opt& o) {
    int failed = 0;
    int list[64], n = 0;
    if (o.hashLevels)
        for (int l : kHashLevels) list[n++] = l;
    else
        for (int l = o.from; l <= o.to && n < 64; ++l) list[n++] = l;
    for (int i = 0; i < n; ++i) {
        const int level = list[i];
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

// --sim recipes (VARIETY_PLAN V4 task 3): every recipe on twenty seeds; greedy plays each for 20 s with no enemy and a
// spire that cannot fall, and the column (1.25 m wide) where its launcher spent the most time is that seed's best line.
// A recipe passes when no column is the best line on more than half of its seeds. Also the numbers on the lane (gates,
// walls, geodes: VARIETY_PLAN V4 notes, readable at seven or eight) and the lane's length.
int cmdRecipes() {
    static World w;
    std::printf("| Recipe | Idea | Seeds | Busiest best-line column | Numbers on the lane, mean (max) | Length, mean | Verdict |\n");
    std::printf("|---|---|---|---|---|---|---|\n");
    int failed = 0;
    for (int rc = 0; rc < recipeCount(); ++rc) {
        int bins[8] = {}, numbersMax = 0;
        float numbers = 0, len = 0;
        const int seeds = 20;
        for (int s = 0; s < seeds; ++s) {
            const int index = maxv(6, kRecipes[rc].from) + (s * 11) % 200;
            LevelDef def = generateWith(index, hash64(u64(s) * 31u + u64(rc) * 1009u) & 0xFFFFFFFFull, rc);
            sizeGeodes(def);
            def.spireHealth = 1000000;
            int nums = def.gateCount + def.barrierCount;
            for (int g = 0; g < def.geodeCount; ++g) nums += def.geodes[g].twist ? 0 : 1;
            numbers += float(nums);
            numbersMax = maxv(numbersMax, nums);
            len += def.len;
            w.init(def, 1);
            w.enemyOff = true;
            w.unbreakableSpire = true;
            Bot bot;
            bot.init(BotKind::Greedy, 1);
            int at[8] = {};
            for (int t = 0; t < 60 * 20; ++t) {
                w.tick(bot.think(w));
                if (t >= 60 * 5) ++at[clampv(int((w.launcherX + kHW) / 1.25f), 0, 7)];
            }
            int best = 0;
            for (int k = 1; k < 8; ++k)
                if (at[k] > at[best]) best = k;
            ++bins[best];
        }
        int top = 0;
        for (int k = 1; k < 8; ++k)
            if (bins[k] > bins[top]) top = k;
        const bool ok = bins[top] * 2 <= seeds;
        failed += ok ? 0 : 1;
        std::printf("| %s | %s | %d | %d of %d (x %.1f..%.1f) | %.1f (%d) | %.0f m | %s |\n", kRecipes[rc].name, kRecipes[rc].idea, seeds,
                    bins[top], seeds, double(-kHW + 1.25f * float(top)), double(-kHW + 1.25f * float(top + 1)), double(numbers / float(seeds)),
                    numbersMax, double(len / float(seeds)), ok ? "pass" : "the same line too often");
    }
    std::printf("\nrecipes: %d of %d pass\n", recipeCount() - failed, recipeCount());
    return failed ? 1 : 0;
}

// --sim decisions: greedy on one level, a line a second: where it aims, the geodes it sees (place, health, boon) and
// the best line's score without and with the boons' worth (VARIETY_PLAN V1 task 3: are its choices sane?).
int cmdDecisions(const Opt& o) {
    const LevelDef def = levelFor(o.level);
    static World w;
    Bot bot;
    w.init(def, o.seed);
    w.enemyOff = o.noEnemy;  // the bake's Pa run
    w.unbreakableSpire = o.noEnemy || o.unbreakable;  // its enemy runs
    bot.init(BotKind::Greedy, o.seed);
    std::printf("level %d: T %.0f s, spire %d, R %.0f, %d geodes\n", def.index, double(def.targetT), def.spireHealth, double(def.ref), def.geodeCount);
    for (int g = 0; g < def.gateCount; ++g)
        std::printf("  gate %d: %s%d at x %.1f (half-width %.1f%s), z %.1f\n", g, def.gates[g].kind == kGateMul ? "x" : "/", def.gates[g].k,
                    double(def.gates[g].cx), double(def.gates[g].hw),
                    def.gates[g].slides == kGateHop ? ", hops" : def.gates[g].slides ? ", slides" : def.gates[g].breath > 0 ? ", breathes" : "",
                    double(def.gates[g].z));
    for (int b = 0; b < def.barrierCount; ++b)
        std::printf("  wall %d: x %.1f..%.1f, z %.1f, health %d\n", b, double(def.barriers[b].x0), double(def.barriers[b].x1),
                    double(def.barriers[b].z), def.barriers[b].health);
    for (int g = 0; g < def.geodeCount; ++g)
        std::printf("  geode %d: %s at x %.1f from %.1f s, %.1f m/s, health %d, plate %d\n", g, boonName(def.geodes[g].boon),
                    double(def.geodes[g].x), double(def.geodes[g].t), double(def.geodes[g].speed), def.geodes[g].hp, def.geodes[g].shield);
    const int ticks = int((o.seconds > 0 ? o.seconds : 2.0f * def.targetT) * 60.0f);
    for (int t = 0; t < ticks && w.result == Result::None; ++t) {
        const Input in = bot.think(w);
        if (t % 60 == 0) {
            float plain = -1, plainX = 0, rich = -1, richX = 0;
            for (int c = 0; c <= 20; ++c) {
                const float x = w.aimMin() + (w.aimMax() - w.aimMin()) * float(c) / 20.0f;
                const float s = bot.score(w, x);
                if (s > plain) plain = s, plainX = x;
            }
            for (int c = 0; c <= 20; ++c) {
                const float x = w.aimMin() + (w.aimMax() - w.aimMin()) * float(c) / 20.0f;
                const float s = bot.score(w, x, true, plain / 8.0f);
                if (s > rich) rich = s, richX = x;
            }
            std::printf("%5.1f s  aim %5.2f  lanterns %d q %d b %d core %2d spire %6d | plain best x %5.2f (%.0f) | with worth x %5.2f (%.0f) |",
                        double(w.time()), double(w.launcherX), w.lanterns, w.quicken, w.bright, w.core, w.spire, double(plainX),
                        double(plain), double(richX), double(rich));
            for (int g = 0; g < def.geodeCount; ++g)
                if (w.geodeState[g] == kGeodeAlive)
                    std::printf(" %s(%.1f,%.1f) %d/%d [%.0f/%.0f]", boonName(def.geodes[g].boon), double(w.geodeX[g]), double(w.geodeZ[g]),
                                w.geodeHp[g], w.geodeShield[g], double(bot.score(w, w.geodeX[g])),
                                double(bot.score(w, w.geodeX[g], true, plain / 8.0f)));
            std::printf("\n");
        }
        w.tick(in);
        for (int e = 0; e < w.eventCount; ++e) {
            const Event& ev = w.events[e];
            if (ev.type == kEvGeodeOpen) std::printf("%5.1f s  OPENED %s\n", double(w.time()), boonName(ev.b));
            if (ev.type == kEvGeodeLost) std::printf("%5.1f s  lost geode %d (%s)\n", double(w.time()), ev.a, ev.b ? "dissolved" : "slipped past");
        }
    }
    std::printf("result %s at %.1f s, core %d, opened %d of %d, spent %lld\n", w.result == Result::Win ? "win" : w.result == Result::Lose ? "lose" : "none",
                double(w.time()), w.core, w.geodesOpened, w.geodesShown, (long long)w.spent);
    return 0;
}

int cmdFlood(const Opt& o) {
    // The bench rig's simulation alone: tick time with both crowds at the caps.
    static World w;
    initBenchWorld(w);
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
                     "usage: glowtide_headless --sim run|determinism|table|soak|accept|flood|flux|variety [--level N | --from A --to B]"
                     " [--bot B] [--seeds K] [--seconds S] [--pairs] [--set name=value]\n"
                     "       glowtide_headless --bake-levels --from A --to B [--jobs 4] [--keep-to N] --out file.h --report file.md\n"
                     "       glowtide_headless --render-audio ladder|flood|ui|win|lose|music|timbres --out x.wav [--seconds S]\n");
        return 2;
    }
    if (!std::strcmp(o.cmd, "run")) return cmdRun(o);
    if (!std::strcmp(o.cmd, "determinism")) return cmdDeterminism(o);
    if (!std::strcmp(o.cmd, "flood")) return cmdFlood(o);
    if (!std::strcmp(o.cmd, "decisions")) return cmdDecisions(o);
    if (!std::strcmp(o.cmd, "recipes")) return cmdRecipes();
    if (!std::strcmp(o.cmd, "table")) return bakeTable(o.from, o.to, o.bots, o.seeds);
    if (!std::strcmp(o.cmd, "soak")) return soak(o.from, o.to, o.bots);
    if (!std::strcmp(o.cmd, "accept")) return acceptIntro(o.from, o.to);
    if (!std::strcmp(o.cmd, "flux")) return fluxFindings();
    if (!std::strcmp(o.cmd, "variety")) return varietyReport(o.from, o.to, o.pairs);
    if (!std::strcmp(o.cmd, "layouts")) return layoutsReport(o.from, o.to);
    if (!std::strcmp(o.cmd, "forecast")) return o.levelSet ? forecastReport(o.level, maxv(1, o.seeds)) : forecastRecipes(maxv(1, o.seeds));
    if (!std::strcmp(o.cmd, "balance")) return balanceReport(maxv(1, o.from), o.to);
    if (!std::strcmp(o.cmd, "bake")) return bakeLevels(o.from, o.to, o.jobs, o.keepTo, o.out, o.report, o.repairSeeds, o.repairOnly, o.repairFrom);
    if (!std::strcmp(o.cmd, "audio")) return renderAudio(o.scene, o.out, o.seconds, o.levelSet ? o.level : 25, o.world, o.boss);
    std::fprintf(stderr, "unknown command %s\n", o.cmd);
    return 2;
}

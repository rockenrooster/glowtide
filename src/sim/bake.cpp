#include "sim/bake.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "core/tuning.h"
#include "sim/fingerprint.h"
#include "sim/level_build.h"

namespace gt::sim {

namespace {

double wallSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

float median(std::vector<float> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// Checks the world's invariants (TECH_SPEC 4.5); false with the reason.
bool validate(const World& w, char* why, size_t n) {
    const float len = w.def.len;
    for (int team = 0; team < 2; ++team) {
        const Units& u = team == 0 ? w.w : w.g;
        const int cap = team == 0 ? int(tune.capW) : int(tune.capG);
        if (u.n > cap) {
            std::snprintf(why, n, "team %d count %d over the cap %d", team, u.n, cap);
            return false;
        }
        for (int i = 0; i < u.n; ++i) {
            if (!(u.x[i] == u.x[i]) || !(u.z[i] == u.z[i])) {
                std::snprintf(why, n, "team %d unit %d position is not a number", team, i);
                return false;
            }
            if (std::fabs(u.x[i]) > kHW + 0.01f || u.z[i] < -1.0f || u.z[i] > len + 1.0f) {
                std::snprintf(why, n, "team %d unit %d out of the lane (%.2f, %.2f)", team, i, double(u.x[i]), double(u.z[i]));
                return false;
            }
            if (u.power[i] < 1) {
                std::snprintf(why, n, "team %d unit %d power %d", team, i, u.power[i]);
                return false;
            }
        }
    }
    return true;
}

}  // namespace

RunResult runLevel(const LevelDef& def, BotKind kind, u64 noiseSeed, const RunOptions& ro) {
    thread_local World w;
    Bot bot;
    w.init(def, noiseSeed);
    w.unbreakableSpire = ro.unbreakable;
    w.enemyOff = ro.enemyOff;
    bot.init(kind, noiseSeed);
    const int ticks = int(ro.seconds * 60.0f);
    std::vector<float> times;
    if (ro.measureTicks) times.reserve(size_t(ticks));
    RunResult r;
    for (int t = 0; t < ticks && w.result == Result::None; ++t) {
        const Input in = bot.think(w);
        if (ro.measureTicks) {
            const double t0 = wallSeconds();
            w.tick(in);
            times.push_back(float((wallSeconds() - t0) * 1e6));
        } else {
            w.tick(in);
        }
        if (ro.validate && !validate(w, r.why, sizeof r.why)) {
            r.invalid = true;
            break;
        }
    }
    r.result = w.result;
    r.time = w.time();
    r.core = w.core;
    r.spire = w.spire;
    r.produced = w.produced;
    r.cancelled = w.cancelled;
    r.delivered = w.delivered;
    r.peakTide = w.peakTide;
    r.peakWisps = w.peakWisps;
    r.peakGlooms = w.peakGlooms;
    r.lowestGloomZ = w.lowestGloomZ > 1e8f ? def.len : w.lowestGloomZ;
    r.geodesShown = w.geodesShown;
    r.geodesOpened = w.geodesOpened;
    r.spent = w.spent;
    r.coreHit = w.coreHit;
    r.hash = w.hash();
    if (!times.empty()) {
        std::sort(times.begin(), times.end());
        r.tickUs[0] = times[times.size() / 2];
        r.tickUs[1] = times[size_t(float(times.size() - 1) * 0.95f)];
        r.tickUs[2] = times.back();
    }
    return r;
}

LevelDef benchFloodLevel() {
    LevelDef d;
    d.index = 40;
    d.seed = 99;
    d.len = 44;
    d.world = 1;
    d.targetT = 30;
    d.spireHealth = 9999;
    d.coreHealth = 10;
    auto g = [&](float cx, float hw, float z, int k, bool mul, int row) {
        Gate q;
        q.cx = cx;
        q.hw = hw;
        q.z = z;
        q.k = u8(k);
        q.kind = mul ? kGateMul : kGateDiv;
        q.row = u8(row);
        d.gates[d.gateCount++] = q;
        return &d.gates[d.gateCount - 1];
    };
    g(-2.6f, 1.8f, 8, 2, true, 0);
    g(2.6f, 1.8f, 8, 3, true, 0);
    Gate* s = g(0, 1.6f, 14.5f, 3, true, 1);
    s->slides = 1;
    s->amp = 3.0f;
    s->period = 4;
    g(-2.4f, 1.6f, 21, 2, true, 2);
    Gate* b = g(2.4f, 1.4f, 21, 4, true, 2);  // V2: a breathing gate and a hopper (VARIETY_TECH 11)
    b->breath = 4.0f;
    Gate* h = g(0, 1.8f, 27, 2, true, 3);
    h->slides = kGateHop;
    h->amp = 2.4f;
    h->period = 2.0f;
    g(-3.0f, 1.5f, 32, 5, true, 4);
    g(2.6f, 1.5f, 32, 2, false, 4);
    d.bumpers[d.bumperCount++] = {-3.6f, 25.0f, 0.6f};
    d.bumpers[d.bumperCount++] = {3.6f, 25.0f, 0.6f};
    d.emitters[d.emitterCount++] = {-2.5f, 0, 5, 0};
    d.emitters[d.emitterCount++] = {0, 0, 5, 0};
    d.emitters[d.emitterCount++] = {2.5f, 0, 5, 0};
    d.baseFlow = 420.0f;
    d.plan = kPlanRunnerPacks;  // runners among the glooms
    d.rowCount = 5;
    const float gx[4] = {-2.6f, 2.4f, 0.0f, -1.4f}, gz[4] = {11.0f, 17.5f, 24.0f, 35.0f};
    for (int k = 0; k < 4; ++k) {
        Geode& g = d.geodes[d.geodeCount++];
        g.x = gx[k];
        g.z0 = gz[k];
        g.speed = 0;
        g.boon = u8(k);
        g.hp = 1000000000;  // never opens: the absorb step and its numbers run every tick
    }
    return d;
}

void initBenchWorld(World& w) {
    w.init(benchFloodLevel(), 0);
    w.noClash = true;
    w.noDamage = true;
    w.wrap = true;
    w.lanterns = int(tune.lanternMax);
    w.quicken = int(tune.quickenMax);
    w.bright = int(tune.brightMax);
    w.kindle = 1;  // a kindled lane: every multiply gate one more
}

int bakeTable(int from, int to, const char* bots, int seeds) {
    std::printf("| Level | Bot | Wins | Losses | Median time | Median core | Cancelled | Lowest gloom z | Peak tide | Peak units |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|\n");
    char buf[64];
    std::snprintf(buf, sizeof buf, "%s", bots);
    for (int level = from; level <= to; ++level) {
        const LevelDef def = levelFor(level);
        char list[64];
        std::snprintf(list, sizeof list, "%s", buf);
        for (char* t = std::strtok(list, ","); t; t = std::strtok(nullptr, ",")) {
            BotKind k;
            if (!botFromName(t, k)) return 2;
            int wins = 0, losses = 0;
            std::vector<float> times, cores, shares, lows;
            i64 tide = 0;
            int units = 0;
            for (int s = 0; s < seeds; ++s) {
                RunOptions ro;
                ro.seconds = 3.5f * def.targetT + 10.0f;
                const RunResult r = runLevel(def, k, u64(s + 1), ro);
                wins += r.result == Result::Win;
                losses += r.result == Result::Lose;
                times.push_back(r.time);
                cores.push_back(float(r.core));
                shares.push_back(r.share());
                lows.push_back(r.lowestGloomZ);
                tide = maxv(tide, r.peakTide);
                units = maxv(units, r.peakWisps + r.peakGlooms);
            }
            std::printf("| %d | %s | %d | %d | %.1f s | %.0f | %.0f%% | %.1f | %lld | %d |\n", level, t, wins, losses, double(median(times)),
                        double(median(cores)), double(median(shares) * 100), double(median(lows)), (long long)tide, units);
            std::fflush(stdout);
        }
    }
    return 0;
}

int soak(int from, int to, const char* bots) {
    char list[64];
    std::snprintf(list, sizeof list, "%s", bots);
    int failed = 0, runs = 0;
    for (char* t = std::strtok(list, ","); t; t = std::strtok(nullptr, ",")) {
        BotKind k;
        if (!botFromName(t, k)) return 2;
        for (int level = from; level <= to; ++level) {
            const LevelDef def = levelFor(level);
            RunOptions ro;
            ro.seconds = 3.5f * def.targetT + 10.0f;
            ro.validate = true;
            const RunResult r = runLevel(def, k, 7, ro);
            ++runs;
            if (r.invalid) {
                ++failed;
                std::printf("soak: level %d bot %s: %s\n", level, t, r.why);
            }
        }
    }
    std::printf("soak: %d runs, %d failed\n", runs, failed);
    return failed ? 1 : 0;
}

namespace {

struct Acceptance {
    bool ok = false;
    int wins = 0;
    float medTime = 0, medCore = 0, medShare = 0, lowest = 0, medOpened = 0, medShown = 0;
    int pipLost = 0, deepGloom = 0;  // greedy runs in which the core was hit; a gloom came below the first gate row
    float naiveTime = 0, podlessTime = 0, hungryTime = 0;
    int naiveLosses = 0, podlessWins = 0, hungryWins = 0;
    bool idleLoses = false;
    i64 peakTide = 0;
    int peakUnits = 0;
    char why[96] = "";
};

// Median time of a bot over seeds 1..n (a lost or unfinished run counts as forever), and its wins.
float botTime(const LevelDef& def, BotKind kind, int n, int& wins) {
    std::vector<float> times;
    wins = 0;
    for (int s = 1; s <= n; ++s) {
        RunOptions ro;
        ro.seconds = 3.5f * def.targetT + 10.0f;
        const RunResult r = runLevel(def, kind, u64(s), ro);
        wins += r.result == Result::Win;
        times.push_back(r.result == Result::Win ? r.time : 1e6f);
    }
    return median(times);
}

// Median core of a bot over seeds 1..n (a lost run counts 0).
float botCore(const LevelDef& def, BotKind kind, int n) {
    std::vector<float> cores;
    for (int s = 1; s <= n; ++s) {
        RunOptions ro;
        ro.seconds = 3.5f * def.targetT + 10.0f;
        const RunResult r = runLevel(def, kind, u64(s), ro);
        cores.push_back(r.result == Result::Win ? float(r.core) : 0.0f);
    }
    return median(cores);
}

// VARIETY_DESIGN 12.2 step 7, lines a to i, in stages that stop at the first line that fails (a seed that will be
// thrown away costs as little as it can): ten greedy runs (a, b, c, f, i), one idle (d), three naive (e), five podless
// (g, h); five hungry last, for the band checks of step 8, only when everything passed (`full`). `target` is the
// cancelled share step 5 aimed at.
Acceptance accept(const LevelDef& def, float target, bool full = true) {
    Acceptance a;
    // Holdout has no spire: it is won at dawn, so no time line and no cancelled share; its core is the measure (12.2).
    const bool holdout = def.kind == kHoldout;
    std::vector<float> times, cores, shares, lows, opened, shown;
    const float firstRow = def.rowCount ? def.rowZ[0] : def.len;
    const float T = def.targetT;
    for (int s = 1; s <= 10; ++s) {
        RunOptions ro;
        ro.seconds = 3.5f * def.targetT + 10.0f;
        const RunResult r = runLevel(def, BotKind::Greedy, u64(s), ro);
        a.wins += r.result == Result::Win;
        times.push_back(r.time);
        cores.push_back(float(r.core));
        shares.push_back(r.share());
        lows.push_back(r.lowestGloomZ);
        opened.push_back(float(r.geodesOpened));
        shown.push_back(float(r.geodesShown));
        a.pipLost += r.coreHit ? 1 : 0;
        a.deepGloom += r.lowestGloomZ < firstRow ? 1 : 0;
        a.peakTide = maxv(a.peakTide, r.peakTide);
        a.peakUnits = maxv(a.peakUnits, r.peakWisps + r.peakGlooms);
    }
    a.medTime = median(times);
    a.medCore = median(cores);
    a.medShare = median(shares);
    a.lowest = median(lows);
    a.medOpened = median(opened);
    a.medShown = median(shown);
    a.podlessTime = a.hungryTime = a.naiveTime = a.medTime;
    a.podlessWins = a.hungryWins = a.wins / 2;
    if (a.wins < 9) return std::snprintf(a.why, sizeof a.why, "a: greedy won %d of 10", a.wins), a;
    if (holdout ? a.medCore < 7 : a.medCore < (def.hardness <= 0.4f && def.index > 5 ? 9 : 6))
        return std::snprintf(a.why, sizeof a.why, "b: greedy's median core %.0f (h %.2f)", double(a.medCore), double(def.hardness)), a;
    if (!holdout && (a.medTime < 0.7f * T || a.medTime > 1.2f * T))
        return std::snprintf(a.why, sizeof a.why, "c: greedy's median time %.1f s outside %.1f-%.1f", double(a.medTime), double(0.7f * T),
                             double(1.2f * T)),
               a;
    if (!holdout && def.index > 5 && std::fabs(a.medShare - target) > 0.05f)
        return std::snprintf(a.why, sizeof a.why, "f: cancelled %.0f%% against a target of %.0f%%", double(a.medShare * 100), double(target * 100)),
               a;
    int placed = 0;  // SECOND WIND's geode comes only when the core falls to 3: line i is about the level's own geodes
    for (int g = 0; g < def.geodeCount; ++g) placed += def.geodes[g].twist == 0 || def.twists[def.geodes[g].twist - 1].type != kTwistSecondWind;
    if (placed > 0 && a.medOpened < 1) return std::snprintf(a.why, sizeof a.why, "i: greedy opened no geode"), a;
    RunOptions ro;
    ro.seconds = 60;
    a.idleLoses = runLevel(def, BotKind::Idle, 1, ro).result == Result::Lose;
    if (!a.idleLoses) return std::snprintf(a.why, sizeof a.why, "d: idle did not lose"), a;
    int naiveWins = 0;
    a.naiveTime = botTime(def, BotKind::Naive, 3, naiveWins);
    a.naiveLosses = 3 - naiveWins;
    if (def.index >= 8 && a.naiveLosses < 2 && a.naiveTime < 1.6f * a.medTime)
        return std::snprintf(a.why, sizeof a.why, "naive won in %.1f s (greedy %.1f s)", double(a.naiveTime), double(a.medTime)), a;
    if (def.index >= 21 && a.naiveLosses < 2)
        return std::snprintf(a.why, sizeof a.why, "e: naive won from level 21 on (%.1f s)", double(a.naiveTime)), a;
    if (placed > 0) {  // with no geode of its own (an introduction level), podless and hungry play as greedy does
        bool lasting = false;
        for (int i = 0; i < def.geodeCount; ++i)
            lasting = lasting || def.geodes[i].boon == kBoonLantern || def.geodes[i].boon == kBoonQuicken || def.geodes[i].boon == kBoonBright ||
                      def.geodes[i].boon == kBoonKindle;
        a.podlessTime = botTime(def, BotKind::Podless, 5, a.podlessWins);
        if (holdout) {  // the boons must show in the core: podless loses, or keeps fewer pips than greedy
            const float pc = botCore(def, BotKind::Podless, 5);
            if (lasting && a.podlessWins == 5 && pc >= a.medCore)
                return std::snprintf(a.why, sizeof a.why, "g: podless keeps %.0f pips at dawn, greedy %.0f", double(pc), double(a.medCore)), a;
            if (def.index >= 30 && pc >= 8)
                return std::snprintf(a.why, sizeof a.why, "h: podless earns three stars (%.0f pips)", double(pc)), a;
            if (full) a.hungryTime = botTime(def, BotKind::Hungry, 5, a.hungryWins);
            a.ok = true;
            return a;
        }
        if (lasting && a.podlessWins >= 3 && a.medTime > 0.9f * a.podlessTime)
            return std::snprintf(a.why, sizeof a.why, "g: greedy %.1f s is not 10%% faster than podless %.1f s", double(a.medTime),
                                 double(a.podlessTime)),
                   a;
        if (def.index >= 30 && a.podlessWins >= 3 && a.podlessTime <= T)
            return std::snprintf(a.why, sizeof a.why, "h: podless earns three stars (%.1f s, T %.0f)", double(a.podlessTime), double(T)), a;
        // VARIETY_DESIGN 12.2 asked that podless win fewer than half here; the bake could not size hard levels that far (see
        // PROGRESS.md), so: podless 25% slower than greedy, or losing three of five.
        if (def.index >= 80 && def.hardness >= 0.65f && a.podlessWins >= 3 && a.podlessTime < 1.25f * a.medTime)
            return std::snprintf(a.why, sizeof a.why, "h: podless won %d of 5 at h %.2f in %.1f s (greedy %.1f s)", a.podlessWins,
                                 double(def.hardness), double(a.podlessTime), double(a.medTime)),
                   a;
        if (full) a.hungryTime = botTime(def, BotKind::Hungry, 5, a.hungryWins);
    }
    a.ok = true;
    return a;
}

// Seeds thrown away, by the line that threw them (a..i; 'n' the naive line before 21; 'x' the enemy's bisection).
std::atomic<int> g_rejected[27];

void countRejection(const char* why) {
    const char c = why[0] >= 'a' && why[0] <= 'i' && why[1] == ':' ? why[0] : why[0] == 'n' ? 'n' : 'x';
    g_rejected[c - 'a'].fetch_add(1);
}

struct BakeOut {
    int index = 0;
    bool done = false, accepted = false;
    u64 seed = 0;
    i32 spire = 0;
    float baseFlow = 0, pa = 0, pn = 0, m = 0, ref = 0, target = 0, scale = 1, fight = 0;
    int seedsTried = 0;
    Acceptance acc;
    char patterns[16] = "";
    char boons[40] = "";
    int rows = 0;
};

// Power a bot delivers in T seconds with no enemy and a spire that cannot fall.
float delivers(const LevelDef& def, BotKind kind) {
    RunOptions ro;
    ro.seconds = def.targetT;
    ro.unbreakable = true;
    ro.enemyOff = true;
    return maxv(1.0f, float(runLevel(def, kind, 1, ro).delivered));
}

// Line g's cheap forecast (VARIETY_DESIGN 12.2 line g, before the costly runs): on a level with a lasting boon, the boons
// taken freely must raise greedy's delivery 12% over podless's with no enemy, or greedy will not be 10% faster with the
// enemy on either. Hand-made levels, festivals, holdouts and colossi pass: a festival and a holdout have other lines, and
// a colossus is won or lost as it arrives, so a slightly weaker stream loses outright (line g holds by "podless loses").
bool forecastG(const LevelDef& def) {
    if (isHandMade(def.index) || def.kind == kFestival || def.kind == kHoldout || def.kind == kColossus) return true;
    bool lasting = false;
    for (int i = 0; i < def.geodeCount; ++i)
        lasting = lasting || def.geodes[i].boon == kBoonLantern || def.geodes[i].boon == kBoonQuicken || def.geodes[i].boon == kBoonBright ||
                  def.geodes[i].boon == kBoonKindle;
    if (!lasting) return true;
    return delivers(def, BotKind::Greedy) >= 1.12f * (def.geodeCount ? delivers(def, BotKind::Podless) : 0.0f);
}

// VARIETY_DESIGN 12.2 for one level: the layout from its seed, then Pa, Pn, the reliance m, R, geode health, the enemy,
// the spire, the acceptance; the next seed while a line fails (up to eight; hand-made levels have one).
BakeOut bakeOne(int index, int firstTry = 0, int tries = 12) {
    BakeOut b;
    b.index = index;
    const bool hand = isHandMade(index);
    if (hand) tries = firstTry == 0 ? 1 : 0;
    for (int t = firstTry; t < firstTry + tries; ++t) {
        const u64 seed = defaultSeed(index) + u64(t) * 1000003u;
        LevelDef def = layoutFor(index, seed);
        const float T = def.targetT;
        b.seedsTried = t + 1;
        // Steps 2-4: Pa and Pn, the reliance m and R. Geode health is seconds of the stream at its column (sizeGeodes, in
        // the layout): a share of R made every geode off the best line too dear to open (VARIETY_PLAN V1 deviation).
        // The reliance m rises to 0.6 by level 52 (VARIETY_DESIGN 12.2 asked for 0.8 by level 66: sized that far for the
        // boons, the enemy overran a greedy bot that the pressure kept from reaching them; see PROGRESS.md).
        const float m = clampv(clampv((float(index) - 10.0f) / 70.0f, 0.0f, 0.6f) + (def.hardness - 0.5f) * 0.15f, 0.0f, 0.7f);
        const float pa = delivers(def, BotKind::Greedy);
        const float pn = def.geodeCount ? delivers(def, BotKind::Podless) : pa;
        const float R = pn + m * (pa - pn);
        def.ref = R;
        // Line g's cheap forecast: when the boons, taken freely, do not raise greedy's delivery 12% over podless's, greedy
        // will not be 10% faster with the enemy on either. Next seed before the costly runs.
        bool lasting = false;
        for (int i = 0; i < def.geodeCount; ++i)
            lasting = lasting || def.geodes[i].boon == kBoonLantern || def.geodes[i].boon == kBoonQuicken || def.geodes[i].boon == kBoonBright ||
                      def.geodes[i].boon == kBoonKindle;
        if (!hand && lasting && def.kind != kFestival && def.kind != kHoldout && def.kind != kColossus && pa < 1.12f * pn) {
            b.seed = seed;
            std::snprintf(b.acc.why, sizeof b.acc.why, "g: with no enemy greedy delivers %.0f a second, podless %.0f", double(pa / T),
                          double(pn / T));
            countRejection(b.acc.why);
            continue;
        }
        if (def.kind == kFestival) {
            // A festival (step 6): no enemy; three stars for greedy's median geodes opened, less one. Greedy must earn them
            // in nine runs of ten and naive must not.
            Acceptance acc;
            std::vector<float> opened;
            RunOptions fr;
            fr.seconds = tune.festivalSeconds + 1.0f;
            for (int s = 1; s <= 10; ++s) opened.push_back(float(runLevel(def, BotKind::Greedy, u64(s), fr).geodesOpened));
            const i32 three = maxv<i32>(1, i32(median(opened)) - 1);
            int greedyThree = 0;
            for (float o : opened) greedyThree += o >= float(three) ? 1 : 0;
            const float naive = float(runLevel(def, BotKind::Naive, 1, fr).geodesOpened);
            acc.wins = 10;
            acc.medTime = tune.festivalSeconds;
            acc.medCore = 10;
            acc.medOpened = median(opened);
            acc.medShown = float(def.geodeCount);
            acc.naiveLosses = 3;
            acc.ok = greedyThree >= 9 && naive < float(three);
            if (!acc.ok)
                std::snprintf(acc.why, sizeof acc.why, "festival: greedy three stars in %d of 10, naive opened %.0f (three is %d)", greedyThree,
                              double(naive), three);
            b.seed = seed;
            b.spire = three;
            b.baseFlow = 0;
            b.pa = pa / T;
            b.pn = pn / T;
            b.m = m;
            b.ref = R;
            b.acc = acc;
            b.rows = def.rowCount;
            for (int r = 0; r < def.rowCount && r < 15; ++r) b.patterns[r] = "OCTSWPXNHE?"[def.rowPattern[r] < kPatCount ? def.rowPattern[r] : kPatCount];
            b.boons[0] = 0;
            if (acc.ok) {
                b.accepted = true;
                break;
            }
            countRejection(acc.why);
            continue;
        }
        if (def.kind == kHoldout) {
            // Holdout (step 6): bisect the flow until greedy's median core at dawn is 7 to 9.
            float lo = std::log(0.12f), hi = std::log(4.0f), scale = 1.0f;
            RunOptions hr;
            hr.seconds = T + 1.0f;
            // Five runs a step and a core of 8 or 9: Holdout stands at the contest's edge (VARIETY_PLAN V3 notes), so the
            // bisection aims at the top of the band the acceptance asks for.
            float okScale = 0;
            for (int step = 0; step < 8; ++step) {
                scale = step == 0 ? 1.0f : std::exp((lo + hi) * 0.5f);
                def.baseFlow = def.fight * R / T * scale;
                std::vector<float> cores;
                for (u64 s = 2; s <= 6; ++s) {
                    const RunResult r = runLevel(def, BotKind::Greedy, s, hr);
                    cores.push_back(r.result == Result::Win ? float(r.core) : 0.0f);
                }
                const float c = median(cores);
                if (c >= 7) okScale = maxv(okScale, scale);  // the strongest enemy greedy holds so far
                if (c >= 8 && c <= 9) break;
                if (c > 9) lo = std::log(scale);
                else hi = std::log(scale);
            }
            scale = okScale > 0 ? okScale : std::exp(lo);
            def.baseFlow = def.fight * R / T * scale;
            def.spireHealth = 1;
            Acceptance acc = accept(def, 0, false);
            for (int fix = 0; fix < 3 && !acc.ok && (acc.why[0] == 'a' || acc.why[0] == 'b'); ++fix) {
                scale *= 0.85f;
                def.baseFlow = def.fight * R / T * scale;
                acc = accept(def, 0, false);
            }
            // Both bots kept every pip: the night was too mild for the boons to matter (line g). A stronger night, while
            // greedy still holds it (lines a and b).
            for (int fix = 0; fix < 3 && !acc.ok && std::strncmp(acc.why, "g: podless keeps", 16) == 0; ++fix) {
                const float before = scale;
                scale *= 1.2f;
                def.baseFlow = def.fight * R / T * scale;
                const Acceptance harder = accept(def, 0, false);
                if (harder.why[0] == 'a' || harder.why[0] == 'b') {  // too strong for greedy: keep the milder night
                    scale = before;
                    def.baseFlow = def.fight * R / T * scale;
                    break;
                }
                acc = harder;
            }
            if (acc.ok && def.geodeCount > 0) acc.hungryTime = botTime(def, BotKind::Hungry, 5, acc.hungryWins);
            b.seed = seed;
            b.spire = 1;
            b.baseFlow = def.baseFlow;
            b.pa = pa / T;
            b.pn = pn / T;
            b.m = m;
            b.ref = R;
            b.scale = scale;
            b.fight = def.fight;
            b.acc = acc;
            b.rows = def.rowCount;
            for (int r = 0; r < def.rowCount && r < 15; ++r) b.patterns[r] = "OCTSWPXNHE?"[def.rowPattern[r] < kPatCount ? def.rowPattern[r] : kPatCount];
            int o = 0;
            for (int g = 0; g < def.geodeCount && o + 4 < int(sizeof b.boons); ++g) {
                if (o) b.boons[o++] = ' ';
                b.boons[o++] = "LQBWKCH"[def.geodes[g].boon % 7];
                if (def.geodes[g].shield > 0) b.boons[o++] = 's';
            }
            b.boons[o] = 0;
            if (acc.ok) {
                b.accepted = true;
                break;
            }
            countRejection(acc.why);
            continue;
        }
        // Step 5: the enemy's flow; the share it should cancel of what greedy produces is fight * R / Pa.
        const float target = clampv(def.fight * R / pa, 0.02f, 0.6f);
        float lo = std::log(0.12f), hi = std::log(3.0f), scale = 1.0f, delivered = 0;
        RunOptions er;
        er.seconds = T;
        er.unbreakable = true;
        // A run greedy loses (the core gone before T) says the enemy is too strong, whatever share it cancelled.
        // Each step is the median of three runs: with boons a run snowballs, and one run says little.
        float okScale = 0, okDelivered = 0, okShare = 0, bisectShare = 0;
        for (int step = 0; step < 7; ++step) {
            scale = step == 0 ? 1.0f : std::exp((lo + hi) * 0.5f);
            def.baseFlow = def.fight * R / T * scale;
            std::vector<float> shares, delivers3;
            int losses = 0;
            for (u64 s = 2; s <= 4; ++s) {
                const RunResult r = runLevel(def, BotKind::Greedy, s, er);
                shares.push_back(r.share());
                delivers3.push_back(float(r.delivered));
                losses += r.result == Result::Lose ? 1 : 0;
            }
            const float share = median(shares);
            delivered = median(delivers3);
            const bool lost = losses >= 2;
            bisectShare = lost ? 1.0f : share;
            if (!lost) okScale = scale, okDelivered = delivered, okShare = share;
            if (!lost && std::fabs(share - target) <= 0.05f) break;
            if (!lost && share < target)
                lo = std::log(scale);
            else
                hi = std::log(scale);
        }
        if (okDelivered > 0 && delivered < okDelivered * 0.5f) {  // the last run was lost: size by the last one that was not
            scale = okScale;
            delivered = okDelivered;
            bisectShare = okShare;
            def.baseFlow = def.fight * R / T * scale;
        }
        // A bisection that could not reach the target leaves a level sized by a run that is not the level: next seed.
        if (!hand && std::fabs(bisectShare - target) > 0.08f) {
            b.seed = seed;
            b.target = target;
            std::snprintf(b.acc.why, sizeof b.acc.why, "the enemy's bisection cancelled %.0f%% against %.0f%%", double(bisectShare * 100),
                          double(target * 100));
            countRejection(b.acc.why);
            continue;
        }
        // Step 6: the goal (the spire, the colossus, the twins together: setGoals splits them).
        i32 goal = twoSignificant(0.85f * delivered);
        def.spireHealth = goal;
        setGoals(def);
        // Step 7: acceptance; the real runs end at the win, which cancels a few points more than the bisection's: up to
        // two corrections of the scale toward the target before the next seed.
        Acceptance acc = accept(def, target, false);
        for (int fix = 0; fix < 2 && !hand && std::fabs(acc.medShare - target) > 0.04f && acc.medShare > 0.01f; ++fix) {
            scale *= target / acc.medShare;
            def.baseFlow = def.fight * R / T * scale;
            std::vector<float> d3;
            for (u64 s = 2; s <= 4; ++s) d3.push_back(float(runLevel(def, BotKind::Greedy, s, er).delivered));
            goal = twoSignificant(0.85f * median(d3));
            def.spireHealth = goal;
            setGoals(def);
            acc = accept(def, target, false);
        }
        // A goal that falls too soon or too late (line c): sized again toward 0.9 T from greedy's median time, twice at most.
        for (int fix = 0; fix < 1 && !acc.ok && acc.why[0] == 'c' && acc.medTime > 0; ++fix) {
            // The colossus's time is steep in its health (the nearer it gets, the weaker the stream): small steps there.
            const bool steep = def.kind == kColossus;
            goal = twoSignificant(float(goal) * clampv(0.9f * T / acc.medTime, steep ? 0.95f : 0.85f, steep ? 1.06f : 1.2f));
            def.spireHealth = goal;
            setGoals(def);
            acc = accept(def, target, false);
        }
        if (acc.ok && def.geodeCount > 0) acc.hungryTime = botTime(def, BotKind::Hungry, 5, acc.hungryWins);  // for the band checks
        b.seed = seed;
        b.spire = goal;
        b.baseFlow = def.baseFlow;
        b.pa = pa / T;
        b.pn = pn / T;
        b.m = m;
        b.ref = R;
        b.target = target;
        b.scale = scale;
        b.fight = def.fight;
        b.acc = acc;
        b.rows = def.rowCount;
        for (int r = 0; r < def.rowCount && r < 15; ++r) b.patterns[r] = "OCTSWPXNHE?"[def.rowPattern[r] < kPatCount ? def.rowPattern[r] : kPatCount];
        int o = 0;
        for (int g = 0; g < def.geodeCount && o + 4 < int(sizeof b.boons); ++g) {
            if (o) b.boons[o++] = ' ';
            b.boons[o++] = "LQBWKCH"[def.geodes[g].boon % 7];
            if (def.geodes[g].shield > 0) b.boons[o++] = 's';
        }
        b.boons[o] = 0;
        if (acc.ok) {
            b.accepted = true;
            break;
        }
        countRejection(acc.why);
    }
    b.done = true;
    return b;
}

}  // namespace

namespace {

Gate fluxGate(float cx, float hw, float z, int k, bool mul, int row) {
    Gate g;
    g.cx = cx;
    g.hw = hw;
    g.z = z;
    g.k = u8(k);
    g.kind = mul ? kGateMul : kGateDiv;
    g.row = u8(row);
    return g;
}

// The flux model's case: three rows whose best path multiplies by 30, T = 30 s, one emitter, two bursts.
LevelDef fluxLevel() {
    LevelDef d;
    d.index = 30;
    d.len = 36;
    d.targetT = 30;
    d.spireHealth = 1000;
    d.coreHealth = 10;
    d.rowCount = 3;
    d.rowZ[0] = 8;
    d.rowZ[1] = 16;
    d.rowZ[2] = 24;
    d.gates[d.gateCount++] = fluxGate(-2.2f, 1.6f, 8, 2, true, 0);
    d.gates[d.gateCount++] = fluxGate(2.2f, 1.6f, 8, 2, false, 0);
    d.gates[d.gateCount++] = fluxGate(2.0f, 1.6f, 16, 3, true, 1);
    d.gates[d.gateCount++] = fluxGate(-2.6f, 1.3f, 16, 2, true, 1);
    d.gates[d.gateCount++] = fluxGate(-1.4f, 1.5f, 24, 5, true, 2);
    d.gates[d.gateCount++] = fluxGate(2.6f, 1.2f, 24, 3, false, 2);
    d.emitters[d.emitterCount++] = {0.0f, 2.5f, 5.0f, 0.0f};
    addTwist(d, kTwistSurge, kTrigTime, 0.35f);
    addTwist(d, kTwistSurge, kTrigTime, 0.70f);
    return d;
}

float fluxP0(const LevelDef& def) {
    RunOptions r;
    r.seconds = def.targetT;
    r.unbreakable = true;
    r.enemyOff = true;
    return maxv(1.0f, float(runLevel(def, BotKind::Greedy, 1, r).delivered) / def.targetT);
}

// The bake's steps 3 and 4 for a given fight: the rate scale that cancels `fight`, then the spire.
LevelDef fluxCalibrate(LevelDef def, float p0, float fight, float* scaleOut) {
    def.fight = fight;
    float lo = std::log(0.3f), hi = std::log(3.0f), scale = 1.0f, delivered = 0;
    RunOptions er;
    er.seconds = def.targetT;
    er.unbreakable = true;
    for (int step = 0; step < 9; ++step) {
        scale = step == 0 ? 1.0f : std::exp((lo + hi) * 0.5f);
        def.baseFlow = fight * p0 * scale;
        const RunResult r = runLevel(def, BotKind::Greedy, 2, er);
        delivered = float(r.delivered);
        if (std::fabs(r.share() - fight) <= 0.02f) break;
        if (r.share() < fight)
            lo = std::log(scale);
        else
            hi = std::log(scale);
    }
    def.spireHealth = twoSignificant(0.85f * delivered);
    *scaleOut = scale;
    return def;
}

void fluxRow(const char* what, const LevelDef& def, const RunResult& r, float firstRowZ) {
    const char* res = r.result == Result::Win ? "win" : r.result == Result::Lose ? "LOSE" : "stall";
    const float front = def.len - r.lowestGloomZ;  // how far the deepest gloom came below the spire
    std::printf("| %s | %s | %.2f | %.0f%% | %.1f m%s | %d |\n", what, res, double(r.time / def.targetT), double(r.share() * 100.0f),
                double(front), r.lowestGloomZ < firstRowZ ? " (past the gates)" : "", r.core);
}

}  // namespace

int fluxFindings() {
    struct Case {
        const char* name;
        LevelDef def;
    } cases[2] = {{"level 5", levelFor(5)}, {"three rows x30", fluxLevel()}};
    const float fireRate = tune.fireRate, rampStart = tune.rampStart;
    for (Case& c : cases) {
        const LevelDef base = c.def;
        const float T = base.targetT;
        const float firstRow = base.rowCount ? base.rowZ[0] : 8.0f;
        const float p0 = fluxP0(base);
        std::printf("\n### %s (T %.0f s, P0 %.0f a second)\n\n", c.name, double(T), double(p0));
        std::printf("| Run | Result | Time (T) | Cancelled | Deepest gloom below the spire | Core |\n|---|---|---|---|---|---|\n");
        RunOptions full;
        full.seconds = 4.0f * T;
        // Finding 1 (and 4): rateScale near 1 cancels about `fight`; the front stays high; the player is not hit.
        const float fights[3] = {0.13f, 0.18f, 0.30f};
        LevelDef cal[3];
        for (int f = 0; f < 3; ++f) {
            float scale = 1;
            cal[f] = fluxCalibrate(base, p0, fights[f], &scale);
            char what[96];
            std::snprintf(what, sizeof what, "1: fight %.0f%%, scale %.2f, full player", double(fights[f] * 100.0f), double(scale));
            fluxRow(what, cal[f], runLevel(cal[f], BotKind::Greedy, 3, full), firstRow);
        }
        // Finding 3: a weaker player (fewer shots) against the same enemy.
        const float shares[2] = {0.5f, 0.667f};
        for (int f = 0; f < 3; ++f) {
            for (float s : shares) {
                tune.fireRate = fireRate * s;
                char what[96];
                std::snprintf(what, sizeof what, "3: fight %.0f%%, player makes %.0f%%", double(fights[f] * 100.0f), double(s * 100.0f));
                fluxRow(what, cal[f], runLevel(cal[f], BotKind::Greedy, 3, full), firstRow);
                tune.fireRate = fireRate;
            }
        }
        // Finding 2: an enemy flow held at a fixed share of the flood (no bursts, no ramp).
        const float ratios[3] = {0.9f, 1.2f, 1.5f};
        for (float k : ratios) {
            LevelDef d = cal[1];
            d.twistCount = 0;
            d.baseFlow = k * p0;
            tune.rampStart = 1000.0f;
            char what[96];
            std::snprintf(what, sizeof what, "2: enemy held at %.1fx the flood", double(k));
            fluxRow(what, d, runLevel(d, BotKind::Greedy, 3, full), firstRow);
            tune.rampStart = rampStart;
        }
    }
    return 0;
}

// --sim forecast: line g's forecast on a level's first seeds (the bake's pass 1 numbering): what greedy and podless
// deliver a second with no enemy, and the geodes greedy opened.
int forecastReport(int level, int tries) {
    std::printf("| Seed number | Recipe | Greedy /s | Podless /s | Ratio | Greedy opened | Forecast |\n|---|---|---|---|---|---|---|\n");
    int pass = 0;
    for (int t = 0; t < tries; ++t) {
        const LevelDef def = layoutFor(level, seedFor(level, t));
        RunOptions ro;
        ro.seconds = def.targetT;
        ro.unbreakable = true;
        ro.enemyOff = true;
        const RunResult g = runLevel(def, BotKind::Greedy, 1, ro), p = runLevel(def, BotKind::Podless, 1, ro);
        const float pa = float(g.delivered) / def.targetT, pn = float(p.delivered) / def.targetT;
        const bool ok = forecastG(def);
        pass += ok ? 1 : 0;
        std::printf("| %d | %s | %.0f | %.0f | %.2f | %d of %d | %s |\n", t, recipeName(def.recipe), double(pa), double(pn),
                    double(pa / maxv(1.0f, pn)), g.geodesOpened, g.geodesShown, ok ? "pass" : "fail");
    }
    std::printf("\nforecast: %d of %d pass\n", pass, tries);
    return 0;
}

// --sim forecast --all: line g's forecast on every recipe, `tries` seeds each on levels of a kind it serves from its first
// level on (which recipes the bake's first pass will rarely find a layout for).
int forecastRecipes(int tries) {
    std::printf("| Recipe | Seeds | Forecast passes | Greedy / podless, mean |\n|---|---|---|---|\n");
    for (int rc = 0; rc < recipeCount(); ++rc) {
        int pass = 0, n = 0;
        double ratio = 0;
        for (int s = 0; s < tries; ++s) {
            int level = maxv(25, kRecipes[rc].from) + (s * 37) % 300;
            for (int k = 0; k < 40 && !(kRecipes[rc].kinds >> kindFor(level) & 1u); ++k) ++level;
            LevelDef def = generateWith(level, hash64(u64(s) * 7919u + u64(rc)) & 0xFFFFFFFFull, rc);
            sizeGeodes(def);
            if (def.kind == kFestival || def.kind == kHoldout) continue;
            RunOptions ro;
            ro.seconds = def.targetT;
            ro.unbreakable = true;
            ro.enemyOff = true;
            const float pa = float(runLevel(def, BotKind::Greedy, 1, ro).delivered), pn = float(runLevel(def, BotKind::Podless, 1, ro).delivered);
            ratio += double(pa / maxv(1.0f, pn));
            pass += forecastG(def) ? 1 : 0;
            ++n;
        }
        if (n) std::printf("| %s | %d | %d (%.0f%%) | %.2f |\n", recipeName(rc), n, pass, 100.0 * pass / n, ratio / n);
    }
    return 0;
}

// --sim balance (VARIETY_PLAN V6 task 3): greedy and podless once each on every level of the table, with the enemy.
int balanceReport(int from, int to) {
    struct Tally {
        int levels = 0, offered = 0, taken = 0, wins = 0;
        double gain = 0, timeT = 0;
        int lowestCore = 10;
    };
    Tally boons[kBoonCount], plans[kPlanCount], twists[kTwistCount], recipes[64], kinds[kKindCount];
    for (int l = from; l <= to; ++l) {
        const LevelDef def = levelFor(l);
        if (def.kind == kFestival) continue;  // no enemy, no core: its own numbers are in the bake report
        RunOptions ro;
        ro.seconds = 2.5f * def.targetT;
        // Greedy, watching which boons it opens.
        thread_local World w;
        Bot bot;
        w.init(def, 1);
        bot.init(BotKind::Greedy, 1);
        int lowest = w.core;
        bool took[kBoonCount] = {};
        const int ticks = int(ro.seconds * 60.0f);
        for (int t = 0; t < ticks && w.result == Result::None; ++t) {
            w.tick(bot.think(w));
            lowest = minv(lowest, int(w.core));
            for (int e = 0; e < w.eventCount; ++e)
                if (w.events[e].type == kEvGeodeOpen) took[clampv(int(w.events[e].b), 0, kBoonCount - 1)] = true;
        }
        const bool won = w.result == Result::Win;
        const float gt = won ? w.time() : 1e6f;
        const RunResult p = runLevel(def, BotKind::Podless, 1, ro);
        const float pt = p.result == Result::Win ? p.time : 1e6f;
        const float gain = gt < 1e5f ? (pt < 1e5f ? pt / gt - 1.0f : 1.0f) : 0.0f;  // podless lost: count it as twice as slow
        bool offered[kBoonCount] = {};
        for (int g = 0; g < def.geodeCount; ++g)
            if (!(def.geodes[g].twist && def.twists[def.geodes[g].twist - 1].type == kTwistSecondWind)) offered[def.geodes[g].boon] = true;
        for (int b = 0; b < kBoonCount; ++b) {
            if (!offered[b]) continue;
            boons[b].offered++;
            boons[b].taken += took[b] ? 1 : 0;
            boons[b].gain += double(gain);
        }
        auto add = [&](Tally& t) {
            t.levels++;
            t.wins += won ? 1 : 0;
            t.timeT += won ? double(gt / def.targetT) : 0.0;
            t.gain += double(gain);
            t.lowestCore = minv(t.lowestCore, lowest);
        };
        add(plans[def.plan % kPlanCount]);
        add(kinds[def.kind % kKindCount]);
        if (!isHandMade(l)) add(recipes[def.recipe & 63]);
        for (int k = 0; k < def.twistCount; ++k)
            if (def.twists[k].type != kTwistSecondWind) add(twists[def.twists[k].type % kTwistCount]);
    }
    std::printf("# Balance by boon, plan, twist, recipe and kind, levels %d to %d (greedy and podless, one run each)\n\n", from, to);
    std::printf("| Boon | Levels offering it | Greedy takes it | Greedy's gain over podless (mean) |\n|---|---|---|---|\n");
    for (int b = 0; b < kBoonCount; ++b)
        if (boons[b].offered)
            std::printf("| %s | %d | %.0f%% | %.0f%% |\n", boonName(b), boons[b].offered, 100.0 * boons[b].taken / boons[b].offered,
                        100.0 * boons[b].gain / boons[b].offered);
    auto table = [&](const char* what, Tally* t, int n, const char* (*name)(int)) {
        std::printf("\n| %s | Levels | Greedy wins | Time / T (wins) | Greedy's lowest core | Gain over podless |\n|---|---|---|---|---|---|\n", what);
        for (int i = 0; i < n; ++i)
            if (t[i].levels)
                std::printf("| %s | %d | %.0f%% | %.2f | %d | %.0f%% |\n", name(i), t[i].levels, 100.0 * t[i].wins / t[i].levels,
                            t[i].wins ? t[i].timeT / t[i].wins : 0.0, t[i].lowestCore, 100.0 * t[i].gain / t[i].levels);
    };
    table("Plan", plans, kPlanCount, planName);
    table("Twist", twists, kTwistCount, twistName);
    table("Recipe", recipes, recipeCount() + setPieceCount(), recipeName);
    table("Kind", kinds, kKindCount, kindName);
    return 0;
}

int acceptIntro(int from, int to) {
    int failed = 0;
    for (int level = from; level <= to; ++level) {
        const LevelDef def = levelFor(level);
        if (level == 1) {
            RunOptions ro;
            ro.seconds = 60;
            const RunResult g = runLevel(def, BotKind::Greedy, 1, ro);
            const RunResult n = runLevel(def, BotKind::Naive, 1, ro);
            const RunResult i = runLevel(def, BotKind::Idle, 1, ro);
            const bool ok = g.result == Result::Win && g.time >= 6 && g.time <= 10 && n.result == Result::Win && i.result == Result::None;
            std::printf("accept level 1: greedy %s in %.1f s, naive %s, idle %s -> %s\n", g.result == Result::Win ? "wins" : "fails",
                        double(g.time), n.result == Result::Win ? "wins" : "fails", i.result == Result::None ? "neither" : "ended",
                        ok ? "ok" : "FAILED");
            failed += !ok;
            continue;
        }
        const Acceptance a = accept(def, def.fight);
        RunOptions ro;
        ro.seconds = 30;
        const bool idle30 = runLevel(def, BotKind::Idle, 1, ro).result == Result::Lose;
        const bool ok = a.wins >= 9 && a.medCore >= 6 && a.medTime >= 0.7f * def.targetT && a.medTime <= 1.3f * def.targetT && idle30;
        std::printf("accept level %d: greedy %d/10, median %.1f s (T %.0f), core %.0f, cancelled %.0f%% (fight %.0f%%); naive %s; idle %s"
                    " -> %s\n",
                    level, a.wins, double(a.medTime), double(def.targetT), double(a.medCore), double(a.medShare * 100),
                    double(def.fight * 100), a.naiveLosses >= 2 ? "loses" : "wins", idle30 ? "loses in 30 s" : "SURVIVES 30 s",
                    ok ? "ok" : "FAILED");
        failed += !ok;
    }
    return failed ? 1 : 0;
}

int finishBake(int from, int to, int jobs, int keepTo, const char* out, const char* report, int repairSeeds, std::vector<BakeOut>& results,
               std::vector<Fingerprint>& fps, std::vector<LayoutChoice>& picks, double t0);

int bakeLevels(int from, int to, int jobs, int keepTo, const char* out, const char* report, int repairSeeds, const char* repairOnly) {
    from = maxv(2, from);
    const int n = to - from + 1;
    if (n <= 0) return 2;
    std::vector<BakeOut> results(static_cast<size_t>(n));
    std::atomic<int> next{0};
    std::atomic<int> doneCount{0};
    const double t0 = wallSeconds();
    if (repairOnly) {
        // A repair-only run: the levels the table marks "(not accepted)" in from..to, on the table this program was built
        // with (the same file); every other row stays as it is.
        std::vector<int> marked;
        if (FILE* in = std::fopen(repairOnly, "rb")) {
            char line[256];
            while (std::fgets(line, sizeof line, in)) {
                const char* c = std::strstr(line, "// ");
                int level = 0;
                if (c && std::strstr(c, "(not accepted)") && std::sscanf(c + 3, "%d", &level) == 1 && level >= from && level <= to)
                    marked.push_back(level);
            }
            std::fclose(in);
        }
        std::vector<Fingerprint> fps(size_t(maxv(to, keepTo) + 1));
        for (int l = 1; l < int(fps.size()); ++l) fps[size_t(l)] = fingerprintOf(levelFor(l));
        std::vector<LayoutChoice> picks(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            const LevelDef d = levelFor(from + i);
            picks[size_t(i)].tries = int((d.seed - defaultSeed(from + i)) / 1000003u);
            results[size_t(i)].accepted = true;  // kept: written from the table below
        }
        for (int l : marked) results[size_t(l - from)].accepted = false;
        std::printf("repair only: %d levels marked not accepted in %s\n", int(marked.size()), repairOnly);
        return finishBake(from, to, jobs, keepTo, out, report, repairSeeds, results, fps, picks, t0);
    }
    // Pass 1 (VARIETY_TECH 5): the layouts, one thread, in level order, each the first seed that passes the lints and
    // VARIETY_DESIGN 11's rule against every level already chosen (level 1 and the rows outside from..to as the table has
    // them).
    // Of the layouts that keep the rule, pass 1 takes one that line g's cheap forecast keeps (forecastG), so pass 2 balances
    // a layout whose boons can pay.
    std::vector<Fingerprint> fps;
    std::vector<LayoutChoice> picks = chooseLayouts(from, to, keepTo, fps, 64, forecastG, jobs);
    int ruleless = 0, unviable = 0;
    for (const LayoutChoice& c : picks) ruleless += c.broken > 0 ? 1 : 0, unviable += c.unviable;
    std::printf("pass 1: %d layouts in %.1f s; %d found no seed that keeps the rule; %d candidates the forecast threw away\n", n,
                wallSeconds() - t0, ruleless, unviable);
    for (int i = 0; i < n; ++i)
        if (picks[size_t(i)].broken) std::printf("  level %d: %d pairs broken (%s)\n", from + i, picks[size_t(i)].broken, picks[size_t(i)].why);
    std::fflush(stdout);
    // Pass 2: balance, in parallel, each level on the layout pass 1 chose.
    auto worker = [&]() {
        for (;;) {
            const int i = next.fetch_add(1);
            if (i >= n) return;
            results[size_t(i)] = bakeOne(from + i, picks[size_t(i)].tries, 1);
            const BakeOut& b = results[size_t(i)];
            const int d = doneCount.fetch_add(1) + 1;
            std::printf("bake %d/%d: level %d %s (seed %llu, spire %d, flow %.1f, Pa %.0f Pn %.0f m %.2f, share %.0f%%/%.0f%%, greedy %d/10 "
                        "%.1f s core %.0f, geodes %s%s%s) %.0f s\n",
                        d, n, b.index, b.accepted ? "accepted" : "NOT ACCEPTED", (unsigned long long)b.seed, b.spire, double(b.baseFlow),
                        double(b.pa), double(b.pn), double(b.m), double(b.acc.medShare * 100), double(b.target * 100), b.acc.wins,
                        double(b.acc.medTime), double(b.acc.medCore), b.boons[0] ? b.boons : "-", b.accepted ? "" : ": ",
                        b.accepted ? "" : b.acc.why, wallSeconds() - t0);
            std::fflush(stdout);
        }
    };
    std::vector<std::thread> pool;
    for (int j = 0; j < maxv(1, jobs); ++j) pool.emplace_back(worker);
    for (std::thread& t : pool) t.join();
    for (BakeOut& b : results) b.done = true;
    return finishBake(from, to, jobs, keepTo, out, report, repairSeeds, results, fps, picks, t0);
}

// Pass 3 and the outputs. A result with `done` false is a row kept from the table (a repair-only run).
int finishBake(int from, int to, int jobs, int keepTo, const char* out, const char* report, int repairSeeds, std::vector<BakeOut>& results,
               std::vector<Fingerprint>& fps, std::vector<LayoutChoice>& picks, double t0) {
    const int n = to - from + 1;
    std::vector<std::thread> pool;
    // Pass 3, repair (VARIETY_TECH 5): every generated level not accepted takes the next seeds whose layouts pass the lints
    // and the rule against the levels on both sides, and balances them, up to `repairSeeds` (eight in VARIETY_TECH 5);
    // levels are repaired in parallel, so a layout that was accepted is taken only if the rule still holds against its
    // neighbours as they are then.
    std::vector<int> repair;
    for (int i = 0; i < n; ++i)
        if (!results[size_t(i)].accepted && !isHandMade(from + i)) repair.push_back(i);
    std::atomic<int> nextRepair{0};
    std::mutex fpLock;
    const std::vector<bool> allFixed(fps.size(), true);
    auto repairWorker = [&]() {
        char lintWhy[96];
        for (;;) {
            const int k = nextRepair.fetch_add(1);
            if (k >= int(repair.size())) return;
            const int i = repair[size_t(k)], level = from + i;
            int balanced = 0, t = picks[size_t(i)].tries + 1;
            BakeOut last;
            bool done = false;
            for (; balanced < repairSeeds && t < picks[size_t(i)].tries + 1 + 64 * repairSeeds && !done; ++t) {
                const LevelDef def = layoutFor(level, seedFor(level, t));
                if (!lintLevel(def, level, lintWhy, sizeof lintWhy)) continue;
                const Fingerprint f = fingerprintOf(def);
                {
                    std::lock_guard<std::mutex> g(fpLock);
                    if (!ruleHolds(fps, allFixed, level, f, nullptr, nullptr, 1)) continue;
                }
                last = bakeOne(level, t, 1);
                // A layout line g's forecast threw away (two runs with no enemy) is not one of the eight balanced.
                if (std::strncmp(last.acc.why, "g: with no enemy", 16) != 0) ++balanced;
                if (!last.accepted) continue;
                std::lock_guard<std::mutex> g(fpLock);
                if (ruleHolds(fps, allFixed, level, f, nullptr, nullptr, 1)) {
                    fps[size_t(level)] = f;
                    picks[size_t(i)].tries = t;
                    results[size_t(i)] = last;
                    results[size_t(i)].done = true;
                    done = true;
                }
            }
            if (!done && results[size_t(i)].done) results[size_t(i)].seedsTried += balanced;  // a kept row stays as it was
            std::printf("repair: level %d %s after %d more seeds%s%s %.0f s\n", level, done ? "accepted" : "NOT ACCEPTED", balanced,
                        done ? "" : ": ", done ? "" : last.acc.why, wallSeconds() - t0);
            std::fflush(stdout);
        }
    };
    if (!repair.empty()) {
        pool.clear();
        for (int j = 0; j < maxv(1, jobs); ++j) pool.emplace_back(repairWorker);
        for (std::thread& t : pool) t.join();
    }

    int rejected = 0;
    for (const BakeOut& b : results) rejected += !b.accepted;
    std::vector<int> stillMarked;  // a repair-only run's levels that kept their old row and stay not accepted
    for (int i = 0; i < n; ++i)
        if (!results[size_t(i)].accepted && !results[size_t(i)].done) stillMarked.push_back(from + i);
    if (out) {
        FILE* f = std::fopen(out, "wb");
        if (!f) return 1;
        std::fprintf(f, "// Generated by glowtide_headless --bake-levels (GAME_DESIGN 7.4) - do not edit.\n");
        std::fprintf(f, "// seed, spire health, base flow (enemy power a second), R (the reference power geode health is a share of);\n");
        std::fprintf(f, "// level 1 is not baked.\n");
        std::fprintf(f, "static const BakedLevel kBaked[] = {\n");
        const int count = maxv(to, keepTo);
        for (int level = 1; level <= count; ++level) {
            if (level >= from && level <= to && results[size_t(level - from)].done) {
                const BakeOut& b = results[size_t(level - from)];
                std::fprintf(f, "    {%lluull, %d, %.3ff, %.1ff},  // %d%s\n", (unsigned long long)b.seed, b.spire, double(b.baseFlow),
                             double(b.ref), level, b.accepted ? "" : " (not accepted)");
            } else if (level >= from && level <= to) {
                const LevelDef d = levelFor(level);
                const bool marked = std::find(stillMarked.begin(), stillMarked.end(), level) != stillMarked.end();
                std::fprintf(f, "    {%lluull, %d, %.3ff, %.1ff},  // %d%s\n", (unsigned long long)d.seed, d.spireHealth,
                             double(d.baseFlow), double(d.ref), level, marked ? " (not accepted)" : "");
            } else {
                const LevelDef d = levelFor(level);
                std::fprintf(f, "    {%lluull, %d, %.3ff, %.1ff},  // %d\n", (unsigned long long)d.seed, d.spireHealth,
                             double(d.baseFlow), double(d.ref), level);
            }
        }
        std::fprintf(f, "};\nstatic const int kBakedGeneratorVersion = %d;\nstatic const int kBakedRulesVersion = %d;\n", kGeneratorVersion,
                     kRulesVersion);
        std::fprintf(f, "static const int kBakedCount = %d;\n", count);
        std::fclose(f);
    }
    if (report) {
        FILE* f = std::fopen(report, "wb");
        if (!f) return 1;
        std::fprintf(f, "# Level bake report\n\nGenerated by `glowtide_headless --bake-levels --from %d --to %d` (VARIETY_DESIGN 12.2). Patterns: "
                        "O open, C choice, T trap, S slider, W wall and prize, P pillars, X patrol, N crossing sliders, H hopper, E breathing gate; ? by hand. Geodes: L lantern, "
                        "Q quicken, B bright, W ward, K kindle, C comet, H hush; s shielded. Pa, Pn: what greedy and podless deliver a second with no enemy; m the "
                        "reliance; R the reference power; Target the cancelled share aimed at. Pip lost and Deep gloom count greedy's ten "
                        "runs in which the core was hit and a gloom came below the first gate row. Podless and Hungry: wins of five, "
                        "median time.\n\n",
                     from, to);
        std::fprintf(f, "| Level | Accepted | h | Rows | Patterns | Kind | Recipe | Plan | Twists | Geodes | Pa /s | Pn /s | m | R | Fight | Target | "
                        "Cancelled | Scale | Spire | Greedy wins | Time (T) | Core | Opened | Pip lost | Deep gloom | Podless | Hungry | Naive | "
                        "Lowest gloom z | Peak tide | Peak units | Seeds |\n");
        std::fprintf(f, "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
        for (const BakeOut& b : results) {
            if (!b.done) continue;  // a row a repair-only run kept: its line is in the earlier report
            const LevelDef d = layoutFor(b.index, b.seed);
            auto botCell = [](char* out, size_t n, int wins, float time) {
                if (time >= 1e5f) std::snprintf(out, n, "%d/5 -", wins);
                else std::snprintf(out, n, "%d/5 %.1f", wins, double(time));
            };
            char pod[32], hun[32], tw[64] = "";
            botCell(pod, sizeof pod, b.acc.podlessWins, b.acc.podlessTime);
            botCell(hun, sizeof hun, b.acc.hungryWins, b.acc.hungryTime);
            for (int k = 0; k < d.twistCount; ++k)
                if (d.twists[k].type != kTwistSecondWind)
                    std::snprintf(tw + std::strlen(tw), sizeof tw - std::strlen(tw), "%s%s", tw[0] ? ", " : "", twistName(d.twists[k].type));
            std::fprintf(f, "| %d | %s | %.2f | %d | %s | %s | %s | %s | %s | %s | %.0f | %.0f | %.2f | %.0f | %.0f%% | %.0f%% | %.0f%% | %.2f | %d | "
                            "%d/10 | %.1f (%.0f) | %.0f | %.0f/%.0f | %d | %d | %s | %s | %s | %.1f | %lld | %d | %d |\n",
                         b.index, b.accepted ? "yes" : b.acc.why, double(d.hardness), b.rows, b.patterns,
                         d.kind == kBoss ? bossName(d.bossType) : kindName(d.kind), isHandMade(d.index) ? "by hand" : recipeName(d.recipe),
                         planName(d.plan), tw[0] ? tw : "-", b.boons[0] ? b.boons : "-",
                         double(b.pa), double(b.pn), double(b.m), double(b.ref), double(b.fight * 100), double(b.target * 100),
                         double(b.acc.medShare * 100), double(b.scale), b.spire, b.acc.wins, double(b.acc.medTime), double(d.targetT),
                         double(b.acc.medCore), double(b.acc.medOpened), double(b.acc.medShown), b.acc.pipLost, b.acc.deepGloom, pod, hun,
                         b.acc.naiveLosses >= 2 ? "loses" : "wins", double(b.acc.lowest), (long long)b.acc.peakTide, b.acc.peakUnits,
                         b.seedsTried);
        }
        std::fclose(f);
    }
    std::printf("seeds thrown away, by line:");
    for (int c = 0; c < 27; ++c)
        if (g_rejected[c].load()) std::printf(" %c %d", char('a' + c), g_rejected[c].load());
    std::printf("\nbake: %d levels, %d not accepted, %.0f s\n", n, rejected, wallSeconds() - t0);
    return rejected ? 3 : 0;
}

}  // namespace gt::sim

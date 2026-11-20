#include "sim/bake.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#include "core/tuning.h"

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

i32 twoSignificant(float v) {
    if (v < 10) return maxv<i32>(1, i32(v + 0.5f));
    float mag = 1;
    while (v / mag >= 100.0f) mag *= 10.0f;
    return i32(std::floor(v / mag + 0.5f) * mag);
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
    g(2.4f, 1.4f, 21, 4, true, 2);
    g(0, 1.8f, 27, 2, true, 3);
    g(-3.0f, 1.5f, 32, 5, true, 4);
    g(2.6f, 1.5f, 32, 2, false, 4);
    d.bumpers[d.bumperCount++] = {-3.6f, 25.0f, 0.6f};
    d.bumpers[d.bumperCount++] = {3.6f, 25.0f, 0.6f};
    d.emitters[d.emitterCount++] = {-2.5f, 0, 5, 0};
    d.emitters[d.emitterCount++] = {0, 0, 5, 0};
    d.emitters[d.emitterCount++] = {2.5f, 0, 5, 0};
    d.baseFlow = 420.0f;
    d.rowCount = 5;
    return d;
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
    float medTime = 0, medCore = 0, medShare = 0, lowest = 0;
    float naiveTime = 0;
    int naiveLosses = 0;
    bool idleLoses = false;
    i64 peakTide = 0;
    int peakUnits = 0;
    char why[96] = "";
};

Acceptance accept(const LevelDef& def) {
    Acceptance a;
    std::vector<float> times, cores, shares, lows;
    for (int s = 1; s <= 10; ++s) {
        RunOptions ro;
        ro.seconds = 3.5f * def.targetT + 10.0f;
        const RunResult r = runLevel(def, BotKind::Greedy, u64(s), ro);
        a.wins += r.result == Result::Win;
        times.push_back(r.time);
        cores.push_back(float(r.core));
        shares.push_back(r.share());
        lows.push_back(r.lowestGloomZ);
        a.peakTide = maxv(a.peakTide, r.peakTide);
        a.peakUnits = maxv(a.peakUnits, r.peakWisps + r.peakGlooms);
    }
    a.medTime = median(times);
    a.medCore = median(cores);
    a.medShare = median(shares);
    a.lowest = median(lows);
    std::vector<float> naive;
    for (int s = 1; s <= 3; ++s) {
        RunOptions ro;
        ro.seconds = 3.5f * def.targetT + 10.0f;
        const RunResult r = runLevel(def, BotKind::Naive, u64(s), ro);
        a.naiveLosses += r.result != Result::Win;
        naive.push_back(r.result == Result::Win ? r.time : 1e6f);
    }
    a.naiveTime = median(naive);
    RunOptions ro;
    ro.seconds = 60;
    a.idleLoses = runLevel(def, BotKind::Idle, 1, ro).result == Result::Lose;
    const float T = def.targetT;
    if (a.wins < 9) std::snprintf(a.why, sizeof a.why, "greedy won %d of 10", a.wins);
    else if (a.medCore < 6) std::snprintf(a.why, sizeof a.why, "greedy's median core %.0f", double(a.medCore));
    else if (a.medTime < 0.7f * T || a.medTime > 1.3f * T)
        std::snprintf(a.why, sizeof a.why, "greedy's median time %.1f s outside %.1f-%.1f", double(a.medTime), double(0.7f * T), double(1.3f * T));
    else if (def.index >= 8 && a.naiveLosses < 2 && a.naiveTime < 1.6f * a.medTime)
        std::snprintf(a.why, sizeof a.why, "naive won in %.1f s (greedy %.1f s)", double(a.naiveTime), double(a.medTime));
    else if (def.index >= 21 && a.naiveLosses < 2)
        std::snprintf(a.why, sizeof a.why, "naive won from level 21 on (%.1f s)", double(a.naiveTime));
    else if (def.index > 5 && std::fabs(a.medShare - def.fight) > 0.05f)
        std::snprintf(a.why, sizeof a.why, "cancelled %.0f%% against a fight of %.0f%%", double(a.medShare * 100), double(def.fight * 100));
    else if (!a.idleLoses) std::snprintf(a.why, sizeof a.why, "idle did not lose");
    else a.ok = true;
    return a;
}

struct BakeOut {
    int index = 0;
    bool done = false, accepted = false;
    u64 seed = 0;
    i32 spire = 0;
    float baseFlow = 0, p0 = 0, scale = 1, fight = 0;
    int seedsTried = 0;
    Acceptance acc;
    char patterns[16] = "";
    int rows = 0;
};

BakeOut bakeOne(int index) {
    BakeOut b;
    b.index = index;
    const bool hand = index <= 5;
    const int tries = hand ? 1 : 8;
    for (int t = 0; t < tries; ++t) {
        const u64 seed = defaultSeed(index) + u64(t) * 1000003u;
        LevelDef def = layoutFor(index, seed);
        const float T = def.targetT;
        b.seedsTried = t + 1;
        // Step 2: P0, with no enemy and an unbreakable spire.
        RunOptions p0r;
        p0r.seconds = T;
        p0r.unbreakable = true;
        p0r.enemyOff = true;
        const float p0 = maxv(1.0f, float(runLevel(def, BotKind::Greedy, 1, p0r).delivered) / T);
        // Step 3: bisect the rate scale until the glooms cancel `fight` of what the player's side produced.
        float lo = std::log(0.3f), hi = std::log(3.0f), scale = 1.0f;
        float delivered = 0, share = 0;
        RunOptions er;
        er.seconds = T;
        er.unbreakable = true;
        for (int step = 0; step < 7; ++step) {
            scale = step == 0 ? 1.0f : std::exp((lo + hi) * 0.5f);
            def.baseFlow = def.fight * p0 * scale;
            const RunResult r = runLevel(def, BotKind::Greedy, 2, er);
            share = r.share();
            delivered = float(r.delivered);
            if (std::fabs(share - def.fight) <= 0.05f) break;
            if (share < def.fight)
                lo = std::log(scale);
            else
                hi = std::log(scale);
        }
        // Step 4: the spire.
        def.spireHealth = twoSignificant(0.85f * delivered);
        // Step 5: acceptance. The bisection's runs keep the spire whole for T; the real runs end at the win, which
        // cancels a few points more: up to two corrections of the scale toward fight before the next seed.
        Acceptance acc = accept(def);
        for (int fix = 0; fix < 2 && !hand && std::fabs(acc.medShare - def.fight) > 0.04f && acc.medShare > 0.01f; ++fix) {
            scale *= def.fight / acc.medShare;
            def.baseFlow = def.fight * p0 * scale;
            const RunResult r = runLevel(def, BotKind::Greedy, 2, er);
            def.spireHealth = twoSignificant(0.85f * float(r.delivered));
            acc = accept(def);
        }
        b.seed = seed;
        b.spire = def.spireHealth;
        b.baseFlow = def.baseFlow;
        b.p0 = p0;
        b.scale = scale;
        b.fight = def.fight;
        b.acc = acc;
        b.rows = def.rowCount;
        for (int r = 0; r < def.rowCount && r < 15; ++r) b.patterns[r] = "OCTSWPXN?"[def.rowPattern[r] < 8 ? def.rowPattern[r] : 8];
        if (acc.ok) {
            b.accepted = true;
            break;
        }
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
    d.bursts[d.burstCount++] = {0.35f * 30.0f, 0, 2, 3, 0};
    d.bursts[d.burstCount++] = {0.70f * 30.0f, 0, 2, 3, 0};
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
            d.burstCount = 0;
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
        const Acceptance a = accept(def);
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

int bakeLevels(int from, int to, int jobs, int part, int parts, const char* out, const char* report) {
    (void)part;
    (void)parts;
    from = maxv(2, from);
    const int n = to - from + 1;
    if (n <= 0) return 2;
    std::vector<BakeOut> results(static_cast<size_t>(n));
    std::atomic<int> next{0};
    std::atomic<int> doneCount{0};
    const double t0 = wallSeconds();
    auto worker = [&]() {
        for (;;) {
            const int i = next.fetch_add(1);
            if (i >= n) return;
            results[size_t(i)] = bakeOne(from + i);
            const BakeOut& b = results[size_t(i)];
            const int d = doneCount.fetch_add(1) + 1;
            std::printf("bake %d/%d: level %d %s (seed %llu, spire %d, flow %.1f, P0 %.0f, scale %.2f, share %.0f%%/%.0f%%, greedy %d/10 "
                        "%.1f s core %.0f%s%s) %.0f s\n",
                        d, n, b.index, b.accepted ? "accepted" : "NOT ACCEPTED", (unsigned long long)b.seed, b.spire, double(b.baseFlow),
                        double(b.p0), double(b.scale), double(b.acc.medShare * 100), double(b.fight * 100), b.acc.wins,
                        double(b.acc.medTime), double(b.acc.medCore), b.accepted ? "" : ": ", b.accepted ? "" : b.acc.why,
                        wallSeconds() - t0);
            std::fflush(stdout);
        }
    };
    std::vector<std::thread> pool;
    for (int j = 0; j < maxv(1, jobs); ++j) pool.emplace_back(worker);
    for (std::thread& t : pool) t.join();

    int rejected = 0;
    for (const BakeOut& b : results) rejected += !b.accepted;
    if (out) {
        FILE* f = std::fopen(out, "wb");
        if (!f) return 1;
        std::fprintf(f, "// Generated by glowtide_headless --bake-levels (GAME_DESIGN 7.4) - do not edit.\n");
        std::fprintf(f, "// seed, spire health, base flow (enemy power a second); level 1 is not baked.\n");
        std::fprintf(f, "static const BakedLevel kBaked[] = {\n");
        for (int level = 1; level <= to; ++level) {
            if (level >= from) {
                const BakeOut& b = results[size_t(level - from)];
                std::fprintf(f, "    {%lluull, %d, %.3ff},  // %d%s\n", (unsigned long long)b.seed, b.spire, double(b.baseFlow), level,
                             b.accepted ? "" : " (not accepted)");
            } else {
                const LevelDef d = levelFor(level);
                std::fprintf(f, "    {%lluull, %d, %.3ff},  // %d\n", (unsigned long long)d.seed, d.spireHealth,
                             double(d.baseFlow), level);
            }
        }
        std::fprintf(f, "};\nstatic const int kBakedGeneratorVersion = %d;\nstatic const int kBakedRulesVersion = %d;\n", kGeneratorVersion,
                     kRulesVersion);
        std::fprintf(f, "static const int kBakedCount = %d;\n", to);
        std::fclose(f);
    }
    if (report) {
        FILE* f = std::fopen(report, "wb");
        if (!f) return 1;
        std::fprintf(f, "# Level bake report\n\nGenerated by `glowtide_headless --bake-levels --from %d --to %d`. Patterns: O open, C choice, "
                        "T trap, S slider, W wall and prize, P pillars, X patrol, N crossing sliders.\n\n",
                     from, to);
        std::fprintf(f, "| Level | Accepted | Rows | Patterns | P0 /s | Fight | Cancelled | Scale | Spire | Greedy wins | Time (T) | Core | "
                        "Naive | Lowest gloom z | Peak tide | Peak units | Seeds |\n");
        std::fprintf(f, "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
        for (const BakeOut& b : results) {
            const LevelDef d = layoutFor(b.index, b.seed);
            std::fprintf(f, "| %d | %s | %d | %s | %.0f | %.0f%% | %.0f%% | %.2f | %d | %d/10 | %.1f (%.0f) | %.0f | %s | %.1f | %lld | %d | %d |\n",
                         b.index, b.accepted ? "yes" : b.acc.why, b.rows, b.patterns, double(b.p0), double(b.fight * 100),
                         double(b.acc.medShare * 100), double(b.scale), b.spire, b.acc.wins, double(b.acc.medTime), double(d.targetT),
                         double(b.acc.medCore), b.acc.naiveLosses >= 2 ? "loses" : "wins", double(b.acc.lowest), (long long)b.acc.peakTide,
                         b.acc.peakUnits, b.seedsTried);
        }
        std::fclose(f);
    }
    std::printf("bake: %d levels, %d not accepted, %.0f s\n", n, rejected, wallSeconds() - t0);
    return rejected ? 3 : 0;
}

}  // namespace gt::sim

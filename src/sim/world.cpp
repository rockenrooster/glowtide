#include "sim/world.h"

#include <cmath>
#include <cstring>

#include "core/tuning.h"

namespace gt::sim {

namespace {

constexpr float kWispR[4] = {0.26f, 0.36f, 0.50f, 0.65f};
constexpr float kGloomR[4] = {0.29f, 0.40f, 0.55f, 0.72f};
constexpr int kTideMarks[] = {100, 500, 1000, 2500, 5000, 10000};

inline int tierOf(i32 p) { return p >= 64 ? 3 : p >= 16 ? 2 : p >= 4 ? 1 : 0; }

void allocUnits(Units& u, int n) {
    u.x = new float[n];
    u.z = new float[n];
    u.xp = new float[n];
    u.zp = new float[n];
    u.vx = new float[n];
    u.power = new i32[n];
    u.bits = new u32[n];
    u.born = new i32[n];
    u.flags = new u8[n];
    u.seed = new u8[n];
    u.clashed = new u8[n];
}

void freeUnits(Units& u) {
    delete[] u.x;
    delete[] u.z;
    delete[] u.xp;
    delete[] u.zp;
    delete[] u.vx;
    delete[] u.power;
    delete[] u.bits;
    delete[] u.born;
    delete[] u.flags;
    delete[] u.seed;
    delete[] u.clashed;
}

inline void copyUnit(Units& u, int to, int from) {
    u.x[to] = u.x[from];
    u.z[to] = u.z[from];
    u.xp[to] = u.xp[from];
    u.zp[to] = u.zp[from];
    u.vx[to] = u.vx[from];
    u.power[to] = u.power[from];
    u.bits[to] = u.bits[from];
    u.born[to] = u.born[from];
    u.flags[to] = u.flags[from];
    u.seed[to] = u.seed[from];
    u.clashed[to] = u.clashed[from];
}

inline u64 fnv(u64 h, const void* p, size_t n) {
    const u8* b = static_cast<const u8*>(p);
    for (size_t i = 0; i < n; ++i) {
        h ^= b[i];
        h *= 1099511628211ULL;
    }
    return h;
}

}  // namespace

float easedWave(float p) {
    p -= std::floor(p);
    const float s = p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f;  // 0 -> 1 -> 0
    const float e = s * s * (3.0f - 2.0f * s);
    return 2.0f * e - 1.0f;
}

World::~World() {
    if (!allocated_) return;
    freeUnits(w);
    freeUnits(g);
    delete[] wStart_;
    delete[] wOrder_;
    delete[] gStart_;
    delete[] gOrder_;
    delete[] pushX_;
    delete[] pushZ_;
    delete[] cursor_;
}

float World::radius(int team, int i) const {
    if (team == 0) return kWispR[tierOf(w.power[i])];
    int t = tierOf(g.power[i]);
    if ((g.flags[i] & kBrute) && t < 2) t = 2;
    return kGloomR[t];
}

void World::init(const LevelDef& d, u64 noiseSeed) {
    if (!allocated_) {
        allocUnits(w, kMaxWisps);
        allocUnits(g, kMaxGlooms);
        const int cells = kGridW * kGridMaxH + 1;
        wStart_ = new int[cells + 1];
        gStart_ = new int[cells + 1];
        wOrder_ = new int[kMaxWisps];
        gOrder_ = new int[kMaxGlooms];
        pushX_ = new float[kMaxWisps];
        pushZ_ = new float[kMaxWisps];
        cursor_ = new int[cells];
        allocated_ = true;
    }
    def = d;
    tickCount = 0;
    launcherX = 0;
    shot = 1;
    firing = false;
    w.n = g.n = 0;
    for (int i = 0; i < def.gateCount; ++i) {
        const Gate& G = def.gates[i];
        gateX[i] = G.slides ? G.cx + G.amp * easedWave(G.phase) : G.cx;
        gateCarry[i] = 0;
    }
    for (int i = 0; i < def.barrierCount; ++i) barrierHealth[i] = def.barriers[i].health;
    for (int i = 0; i < def.snufferCount; ++i) {
        const Snuffer& S = def.snuffers[i];
        snufferX[i] = (S.xa + S.xb) * 0.5f + (S.xb - S.xa) * 0.5f * easedWave(S.phase);
    }
    for (int i = 0; i < def.emitterCount; ++i) emitterX[i] = def.emitters[i].x;
    spire = spireStart = maxv<i32>(1, def.spireHealth);
    core = def.coreHealth;
    spireStage = 0;
    result = Result::None;
    endTick = -1;
    produced = cancelled = delivered = 0;
    tide = peakTide = 0;
    peakWisps = peakGlooms = 0;
    lowestGloomZ = 1e9f;
    coreHit = false;
    eventCount = 0;
    rng_.reseed(def.seed ^ hash64(noiseSeed + 0x51ED27u), 0x2545F4914F6CDD1DULL);
    owed_ = 0;
    nextBrute_ = tune.emitterDelay + def.bruteEvery;
    emitterNext_ = 0;
    for (int i = 0; i < kMaxBursts; ++i) burstStart_[i] = -1;
    tideMark_ = 0;
    capW_ = clampv(int(tune.capW), 64, kMaxWisps);
    capG_ = clampv(int(tune.capG), 16, kMaxGlooms);
    reserve_ = clampv(int(tune.capReserve), 0, capW_ / 4);
    // Backward-Euler forms of the exponential decays: no libm in the tick (TECH_SPEC 4.2).
    smooth_ = kTick / (tune.dragSmoothing + kTick);
    fanKeep_ = tune.fanDecay / (tune.fanDecay + kTick);
    gridH_ = clampv(int((def.len + 8.0f) / kCell) + 1, 8, kGridMaxH);
}

void World::addEvent(u8 type, float x, float z, i32 a, i32 b) {
    if (eventCount < kMaxEvents) events[eventCount++] = {type, x, z, a, b};
}

void World::emitWisp(float x, float z, i32 power) {
    if (w.n >= capW_) {
        // Past the cap the shot's power joins the most recently fired wisp still near the launcher (GAME_DESIGN 5.3).
        for (int i = w.n - 1; i >= 0 && i >= w.n - 64; --i) {
            if (w.power[i] > 0 && w.bits[i] == 0) {
                w.power[i] += power;
                produced += power;
                return;
            }
        }
        return;
    }
    const int i = w.n++;
    w.x[i] = x;
    w.z[i] = z;
    w.xp[i] = x;
    w.zp[i] = z;
    w.vx[i] = 0;
    w.power[i] = power;
    w.bits[i] = 0;
    w.born[i] = tickCount;
    w.flags[i] = 0;
    w.seed[i] = u8(rng_.next());
    w.clashed[i] = 0;
    produced += power;
}

void World::emitGloom(float x, i32 power, bool brute) {
    const int i = g.n++;
    g.x[i] = x;
    g.z[i] = def.len - 0.5f;
    g.xp[i] = g.x[i];
    g.zp[i] = g.z[i];
    g.vx[i] = 0;
    g.power[i] = power;
    g.bits[i] = 0;
    g.born[i] = tickCount;
    g.flags[i] = brute ? kBrute : 0;
    g.seed[i] = u8(rng_.next());
    g.clashed[i] = 0;
}

int World::cellOf(float x, float z) const {
    const int cx = clampv(int((x + 6.0f) * (1.0f / kCell)), 0, kGridW - 1);
    const int cz = clampv(int((z + 4.0f) * (1.0f / kCell)), 0, gridH_ - 1);
    return cz * kGridW + cx;
}

void World::buildGrid(const Units& u, int* start, int* order) {
    const int cells = kGridW * gridH_;
    std::memset(start, 0, sizeof(int) * size_t(cells + 1));
    for (int i = 0; i < u.n; ++i) ++start[cellOf(u.x[i], u.z[i]) + 1];
    for (int c = 0; c < cells; ++c) start[c + 1] += start[c];
    std::memcpy(cursor_, start, sizeof(int) * size_t(cells));
    for (int i = 0; i < u.n; ++i) order[cursor_[cellOf(u.x[i], u.z[i])]++] = i;
}

void World::separate(Units& u, const int* start, const int* order, int team) {
    const float k = tune.sepRadius;
    const float maxPush = tune.sepSpeed * kTick;
    for (int i = 0; i < u.n; ++i) pushX_[i] = pushZ_[i] = 0;
    for (int i = 0; i < u.n; ++i) {
        const float ri = radius(team, i);
        const float xi = u.x[i], zi = u.z[i];
        const int c = cellOf(xi, zi);
        const int cx = c % kGridW, cz = c / kGridW;
        const int range = ri > 0.4f ? 2 : 1;
        float px = 0, pz = 0;
        for (int zz = maxv(0, cz - range); zz <= minv(gridH_ - 1, cz + range); ++zz) {
            for (int xx = maxv(0, cx - range); xx <= minv(kGridW - 1, cx + range); ++xx) {
                const int cell = zz * kGridW + xx;
                for (int s = start[cell]; s < start[cell + 1]; ++s) {
                    const int j = order[s];
                    if (j == i) continue;
                    const float dx = xi - u.x[j], dz = zi - u.z[j];
                    const float minD = (ri + radius(team, j)) * k;
                    const float d2 = dx * dx + dz * dz;
                    if (d2 >= minD * minD) continue;
                    if (d2 < 1e-8f) {
                        px += (i < j ? 0.5f : -0.5f) * minD;
                        continue;
                    }
                    const float d = std::sqrt(d2);
                    const float o = (minD - d) * 0.5f / d;
                    px += dx * o;
                    pz += dz * o;
                }
            }
        }
        pushX_[i] = px;
        pushZ_[i] = pz;
    }
    for (int i = 0; i < u.n; ++i) {
        float px = pushX_[i], pz = pushZ_[i];
        if (team == 0 && pz < 0) pz = 0;  // never backwards down its lane
        if (team == 1 && pz > 0) pz = 0;
        const float l2 = px * px + pz * pz;
        if (l2 > maxPush * maxPush) {
            const float s = maxPush / std::sqrt(l2);
            px *= s;
            pz *= s;
        }
        const float r = radius(team, i);
        u.x[i] = clampv(u.x[i] + px, -kHW + r, kHW - r);
        u.z[i] += pz;
    }
}

void World::removeDead(Units& u) {
    int o = 0;
    for (int i = 0; i < u.n; ++i) {
        if (u.power[i] <= 0) continue;
        if (o != i) copyUnit(u, o, i);
        ++o;
    }
    u.n = o;
}

void World::tick(const Input& in) {
    eventCount = 0;
    if (result != Result::None) return;
    const float dt = kTick;
    const float t = float(tickCount) * dt;  // time at the start of this tick
    const float len = def.len;

    // 1. Input.
    const float target = clampv(in.launcherTargetX, -kLauncherRange, kLauncherRange);
    launcherX += (target - launcherX) * smooth_;
    firing = in.firing;

    // 2. Fire: a stored shot, so a press fires at once and tapping never beats holding (GAME_DESIGN 5.2).
    shot = minv(1.0f, shot + tune.fireRate * dt);
    if (in.firing && shot >= 1.0f) {
        shot -= 1.0f;
        emitWisp(launcherX + rng_.range(-0.07f, 0.07f), kMuzzleZ, 1);
        addEvent(kEvFire, launcherX, kMuzzleZ, 0, 0);
    }

    // 3. Emitters (GAME_DESIGN 5.9).
    if (def.hasEnemy() && !enemyOff && t >= tune.emitterDelay) {
        for (int e = 0; e < def.emitterCount; ++e) {
            const Emitter& E = def.emitters[e];
            emitterX[e] = E.amp > 0 ? E.x + E.amp * easedWave(t / E.period + E.phase) : E.x;
        }
        float mult = 1.0f;
        int brutesNow = 0;
        for (int b = 0; b < def.burstCount; ++b) {
            const Burst& B = def.bursts[b];
            if (burstStart_[b] < 0) {
                const bool go = B.t >= 0 ? t >= B.t : (spire <= i32(B.healthFrac * float(spireStart)));
                if (go) {
                    burstStart_[b] = tickCount;
                    brutesNow += B.brutes;
                }
            }
            if (burstStart_[b] >= 0 && float(tickCount - burstStart_[b]) * dt < B.dur) mult = maxv(mult, B.mult);
        }
        float ramp = 1.0f;
        const float rampT = tune.rampStart * def.targetT;
        if (t > rampT) ramp += (t - rampT) / tune.rampSlope;
        owed_ += def.baseFlow * flowScale * mult * ramp * dt;
        if (def.bruteEvery > 0 && t >= nextBrute_) {
            ++brutesNow;
            nextBrute_ += def.bruteEvery;
        }
        for (int b = 0; b < brutesNow && g.n < capG_; ++b) {
            const float x = emitterX[emitterNext_] + rng_.range(-0.4f, 0.4f);
            emitterNext_ = (emitterNext_ + 1) % def.emitterCount;
            emitGloom(x, maxv<i32>(def.brutePower, 16), true);
            owed_ -= float(maxv<i32>(def.brutePower, 16));
        }
        int spawned = 0;
        while (owed_ >= 1.0f && spawned < 30 && g.n < capG_) {
            const float x = emitterX[emitterNext_] + rng_.range(-0.4f, 0.4f);
            emitterNext_ = (emitterNext_ + 1) % def.emitterCount;
            // At the cap the owed power waits and joins the next gloom that fits (GAME_DESIGN 5.3).
            const i32 p = spawned == 0 && owed_ >= 2.0f && g.n >= capG_ - 1 ? i32(owed_) : 1;
            emitGloom(x, p, false);
            owed_ -= float(p);
            ++spawned;
        }
    }

    // 4. Steer and move.
    const float wispStep = tune.wispSpeed * dt;
    const float funnelZ = len - tune.funnelLen;
    const float funnelStep = tune.funnelSpeed * dt;
    for (int i = 0; i < w.n; ++i) {
        w.xp[i] = w.x[i];
        w.zp[i] = w.z[i];
        w.clashed[i] = 0;
        float x = w.x[i] + w.vx[i] * dt;
        w.vx[i] *= fanKeep_;
        const float z = w.z[i] + wispStep;
        if (z > funnelZ) {
            const float fx = clampv(x, -tune.funnelX, tune.funnelX);
            x += clampv(fx - x, -funnelStep, funnelStep);
        }
        const float r = radius(0, i);
        w.x[i] = clampv(x, -kHW + r, kHW - r);
        w.z[i] = z;
    }
    const float homeStep = tune.homing * dt;
    for (int i = 0; i < g.n; ++i) {
        g.xp[i] = g.x[i];
        g.zp[i] = g.z[i];
        g.clashed[i] = 0;
        const float speed = (g.flags[i] & kBrute) ? tune.bruteSpeed : tune.gloomSpeed;
        const float x = g.x[i] + clampv(launcherX - g.x[i], -homeStep, homeStep);
        const float r = radius(1, i);
        g.x[i] = clampv(x, -kHW + r, kHW - r);
        g.z[i] -= speed * dt;
    }

    // 5. Sliding gates and snuffers (positions at the end of this tick).
    const float t1 = t + dt;
    for (int i = 0; i < def.gateCount; ++i) {
        const Gate& G = def.gates[i];
        if (G.slides) gateX[i] = G.cx + G.amp * easedWave(t1 / G.period + G.phase);
    }
    for (int i = 0; i < def.snufferCount; ++i) {
        const Snuffer& S = def.snuffers[i];
        const float span = maxv(0.1f, S.xb - S.xa);
        const float period = 2.0f * span / maxv(0.1f, S.speed);
        snufferX[i] = (S.xa + S.xb) * 0.5f + span * 0.5f * easedWave(t1 / period + S.phase);
    }

    // 6-7. Grid and separation.
    buildGrid(w, wStart_, wOrder_);
    separate(w, wStart_, wOrder_, 0);
    buildGrid(g, gStart_, gOrder_);
    separate(g, gStart_, gOrder_, 1);

    // 8. Bumpers: units slide round them (sideways only, so nothing is pushed back down its lane).
    for (int b = 0; b < def.bumperCount; ++b) {
        const Bumper& B = def.bumpers[b];
        for (int team = 0; team < 2; ++team) {
            Units& u = team == 0 ? w : g;
            for (int i = 0; i < u.n; ++i) {
                const float dz = u.z[i] - B.z;
                const float r = radius(team, i);
                const float minD = B.r + r;
                if (dz >= minD || dz <= -minD) continue;
                const float dx = u.x[i] - B.x;
                if (dx * dx + dz * dz >= minD * minD) continue;
                const float side = dx > 0.0f || (dx == 0.0f && (i & 1)) ? 1.0f : -1.0f;
                const float nx = B.x + side * std::sqrt(minD * minD - dz * dz);
                u.x[i] = clampv(nx, -kHW + r, kHW - r);
            }
        }
    }

    // 9. Gates.
    i32 passes[kMaxGates] = {};
    float passX[kMaxGates] = {};
    i32 divSnuffed[kMaxGates] = {};
    const int nBefore = w.n;
    for (int i = 0; i < nBefore; ++i) {
        if (w.power[i] <= 0) continue;
        for (int gi = 0; gi < def.gateCount; ++gi) {
            const Gate& G = def.gates[gi];
            if (!(w.zp[i] < G.z && w.z[i] >= G.z)) continue;
            if (std::fabs(w.x[i] - gateX[gi]) > G.hw) continue;
            const u32 bit = 1u << gi;
            if (w.bits[i] & bit) continue;
            w.bits[i] |= bit;
            const int k = G.k;
            if (G.kind == kGateMul) {
                const i32 p = w.power[i];
                const int room = maxv(0, capW_ - reserve_ - w.n);
                const int c = minv(k - 1, room);
                const float fan = tune.fanBase + tune.fanPerK * float(k);
                const int r0 = int(rng_.next() % u32(k));
                auto spread = [&](int slot) {
                    const int s = (slot + r0) % k;
                    return fan * (-1.0f + 2.0f * (float(s) + 0.5f) / float(k) + rng_.range(-0.5f, 0.5f) / float(k));
                };
                w.vx[i] = spread(0);
                for (int j = 1; j <= c; ++j) {
                    const int n = w.n++;
                    w.x[n] = w.x[i] + rng_.range(-0.03f, 0.03f);
                    w.z[n] = w.z[i] + rng_.range(0.0f, 0.12f);
                    w.xp[n] = w.x[n];
                    w.zp[n] = w.z[n];
                    w.vx[n] = spread(j);
                    w.power[n] = p;
                    w.bits[n] = w.bits[i];
                    w.born[n] = tickCount;
                    w.flags[n] = 0;
                    w.seed[n] = u8(rng_.next());
                    w.clashed[n] = 0;
                }
                if (c < k - 1) w.power[i] = p * (k - c);  // past the cap: power, not numbers (GAME_DESIGN 5.3)
                produced += i64(p) * (k - 1);
                ++passes[gi];
                passX[gi] += w.x[i];
            } else {
                const i32 total = gateCarry[gi] + w.power[i];
                w.power[i] = total / k;
                gateCarry[gi] = total % k;
                if (w.power[i] <= 0) ++divSnuffed[gi];
                ++passes[gi];
                passX[gi] += w.x[i];
            }
        }
    }
    for (int gi = 0; gi < def.gateCount; ++gi) {
        if (!passes[gi]) continue;
        if (def.gates[gi].kind == kGateMul)
            addEvent(kEvGateMul, passX[gi] / float(passes[gi]), def.gates[gi].z, gi, passes[gi]);
        else
            addEvent(kEvGateDiv, passX[gi] / float(passes[gi]), def.gates[gi].z, gi, divSnuffed[gi]);
    }

    // 10. Barriers.
    for (int b = 0; b < def.barrierCount; ++b) {
        if (barrierHealth[b] <= 0) continue;
        const Barrier& B = def.barriers[b];
        i32 dmg = 0;
        for (int i = 0; i < w.n && barrierHealth[b] > 0; ++i) {
            if (w.power[i] <= 0 || !(w.zp[i] < B.z && w.z[i] >= B.z) || w.x[i] < B.x0 || w.x[i] > B.x1) continue;
            const i32 give = minv(w.power[i], barrierHealth[b]);
            barrierHealth[b] -= give;
            w.power[i] -= give;
            dmg += give;
        }
        if (dmg) addEvent(kEvBarrierHit, (B.x0 + B.x1) * 0.5f, B.z, b, dmg);
        if (barrierHealth[b] <= 0) addEvent(kEvBarrierBreak, (B.x0 + B.x1) * 0.5f, B.z, b, 0);
    }

    // 11. Snuffers.
    for (int s = 0; s < def.snufferCount; ++s) {
        const float sx = snufferX[s], sz = def.snuffers[s].z;
        for (int i = 0; i < w.n; ++i) {
            if (w.power[i] <= 0) continue;
            const float dz = w.z[i] - sz;
            if (dz > 1.3f || dz < -1.3f) continue;
            const float dx = w.x[i] - sx;
            const float rr = 0.6f + radius(0, i);
            if (dx * dx + dz * dz < rr * rr) {
                w.power[i] = 0;
                addEvent(kEvSnuffed, w.x[i], w.z[i], 0, 0);
            }
        }
    }

    // 12. Clash: overlapping wisp and gloom both lose min(power); each unit at most once a tick.
    if (!noClash) {
        int clashes = 0;
        for (int gi = 0; gi < g.n; ++gi) {
            if (g.power[gi] <= 0) continue;
            const float rg = radius(1, gi);
            const float gx = g.x[gi], gz = g.z[gi];
            const int c = cellOf(gx, gz);
            const int cx = c % kGridW, cz = c / kGridW;
            const int range = rg + 0.65f > kCell ? 2 : 1;
            int best = -1;
            float bestD = 1e9f;
            for (int zz = maxv(0, cz - range); zz <= minv(gridH_ - 1, cz + range); ++zz) {
                for (int xx = maxv(0, cx - range); xx <= minv(kGridW - 1, cx + range); ++xx) {
                    const int cell = zz * kGridW + xx;
                    for (int s = wStart_[cell]; s < wStart_[cell + 1]; ++s) {
                        const int j = wOrder_[s];
                        if (w.clashed[j] || w.power[j] <= 0) continue;
                        const float dx = gx - w.x[j], dz = gz - w.z[j];
                        const float d2 = dx * dx + dz * dz;
                        const float rr = rg + radius(0, j);
                        if (d2 < rr * rr && d2 < bestD) {
                            bestD = d2;
                            best = j;
                        }
                    }
                }
            }
            if (best < 0) continue;
            const i32 m = minv(g.power[gi], w.power[best]);
            g.power[gi] -= m;
            w.power[best] -= m;
            g.clashed[gi] = 1;
            w.clashed[best] = 1;
            cancelled += m;
            ++clashes;
            const float ex = (gx + w.x[best]) * 0.5f, ez = (gz + w.z[best]) * 0.5f;
            if (g.power[gi] <= 0 && (g.flags[gi] & kBrute)) addEvent(kEvBruteDown, gx, gz, 0, 0);
            if (clashes <= 64) addEvent(kEvClash, ex, ez, m, 0);
        }
        if (clashes) addEvent(kEvClashCount, 0, 0, clashes, 0);
    }

    // 13. Spire and core.
    i32 spireDmg = 0, n = 0;
    float sumX = 0;
    for (int i = 0; i < w.n; ++i) {
        if (w.power[i] <= 0 || w.z[i] < len - 0.3f) continue;
        spireDmg += w.power[i];
        sumX += w.x[i];
        ++n;
        delivered += w.power[i];
        if (wrap) {
            w.z[i] = kMuzzleZ;
            w.zp[i] = w.z[i];
            w.bits[i] = 0;
            w.power[i] = 1;
        } else {
            w.power[i] = 0;
        }
    }
    if (spireDmg) {
        if (!noDamage && !unbreakableSpire) spire -= spireDmg;
        addEvent(kEvSpireHit, sumX / float(n), len, spireDmg, 0);
        static const int stages[3] = {75, 50, 25};
        while (spireStage < 3 && spire * 100 <= spireStart * stages[spireStage]) {
            addEvent(kEvSpireStage, 0, len, stages[spireStage], 0);
            ++spireStage;
        }
    }
    for (int i = 0; i < g.n; ++i) {
        if (g.power[i] <= 0) continue;
        lowestGloomZ = minv(lowestGloomZ, g.z[i]);
        if (g.z[i] > kCoreZ) continue;
        if (wrap) {
            g.z[i] = len - 0.5f;
            g.zp[i] = g.z[i];
            continue;
        }
        if (!noDamage) core -= g.power[i];
        coreHit = true;
        addEvent(kEvCoreHit, g.x[i], kCoreZ, g.power[i], 0);
        g.power[i] = 0;
    }

    // 14. Remove the dead.
    removeDead(w);
    removeDead(g);
    tide = 0;
    for (int i = 0; i < w.n; ++i) tide += w.power[i];
    peakTide = maxv(peakTide, tide);
    peakWisps = maxv(peakWisps, w.n);
    peakGlooms = maxv(peakGlooms, g.n);
    while (tideMark_ < countOf(kTideMarks) && tide >= kTideMarks[tideMark_]) {
        addEvent(kEvTideMark, 0, 0, kTideMarks[tideMark_], 0);
        ++tideMark_;
    }

    // 15. Win and lose.
    ++tickCount;
    if (!noDamage && !unbreakableSpire && spire <= 0) {
        spire = 0;
        result = Result::Win;
        endTick = tickCount;
        addEvent(kEvWin, 0, len, 0, 0);
    } else if (!noDamage && core <= 0) {
        core = 0;
        result = Result::Lose;
        endTick = tickCount;
        addEvent(kEvLose, launcherX, 0, 0, 0);
    }
}

u64 World::hash() const {
    u64 h = 1469598103934665603ULL;
    h = fnv(h, &tickCount, sizeof tickCount);
    h = fnv(h, &launcherX, sizeof launcherX);
    h = fnv(h, &shot, sizeof shot);
    h = fnv(h, &w.n, sizeof w.n);
    h = fnv(h, &g.n, sizeof g.n);
    h = fnv(h, w.x, sizeof(float) * size_t(w.n));
    h = fnv(h, w.z, sizeof(float) * size_t(w.n));
    h = fnv(h, w.vx, sizeof(float) * size_t(w.n));
    h = fnv(h, w.power, sizeof(i32) * size_t(w.n));
    h = fnv(h, w.bits, sizeof(u32) * size_t(w.n));
    h = fnv(h, g.x, sizeof(float) * size_t(g.n));
    h = fnv(h, g.z, sizeof(float) * size_t(g.n));
    h = fnv(h, g.power, sizeof(i32) * size_t(g.n));
    h = fnv(h, &spire, sizeof spire);
    h = fnv(h, &core, sizeof core);
    h = fnv(h, &owed_, sizeof owed_);
    h = fnv(h, barrierHealth, sizeof barrierHealth);
    h = fnv(h, gateCarry, sizeof gateCarry);
    h = fnv(h, &rng_.state, sizeof rng_.state);
    return h;
}

}  // namespace gt::sim

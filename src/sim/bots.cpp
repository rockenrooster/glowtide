#include "sim/bots.h"

#include <cmath>
#include <cstring>

#include "core/tuning.h"

namespace gt::sim {

namespace {
constexpr int kReactionTicks = 9;      // 0.15 s
constexpr float kHandSpeed = 10.0f;    // m/s
constexpr float kAimNoise = 0.25f;     // m
constexpr int kDecisionTicks = 12;     // 0.2 s
constexpr int kDefendTicks = 36;       // 0.6 s
constexpr int kItems = kMaxGates + kMaxBarriers + kMaxSnuffers + 2 * kMaxGeodes;

// Where the stream fired now meets a geode: its place then (it drifts and slides while the stream climbs).
void geodeArrival(const World& w, int gi, float& x, float& z) {
    const Geode& G = w.def.geodes[gi];
    const float t = w.time();
    const float speed = G.leaveAt > 0 && t < G.leaveAt ? 0.0f : G.speed;
    const float ta = maxv(0.0f, w.geodeZ[gi] - kMuzzleZ) / (tune.wispSpeed + speed);
    z = w.geodeZ[gi] - speed * ta;
    x = G.amp > 0 ? G.x + G.amp * easedWave((t + ta) / G.period + G.phase) : w.geodeX[gi];
}
}  // namespace

bool botFromName(const char* name, BotKind& out) {
    if (!std::strcmp(name, "greedy")) out = BotKind::Greedy;
    else if (!std::strcmp(name, "naive")) out = BotKind::Naive;
    else if (!std::strcmp(name, "sweep")) out = BotKind::Sweep;
    else if (!std::strcmp(name, "idle")) out = BotKind::Idle;
    else if (!std::strcmp(name, "podless")) out = BotKind::Podless;
    else if (!std::strcmp(name, "hungry")) out = BotKind::Hungry;
    else return false;
    return true;
}

const char* botName(BotKind k) {
    switch (k) {
        case BotKind::Greedy: return "greedy";
        case BotKind::Naive: return "naive";
        case BotKind::Sweep: return "sweep";
        case BotKind::Podless: return "podless";
        case BotKind::Hungry: return "hungry";
        default: return "idle";
    }
}

void Bot::init(BotKind k, u64 noiseSeed) {
    kind = k;
    rng_.reseed(noiseSeed * 7919u + 3u, 11);
    hand_ = aim_ = applied_ = 0;
    nextDecision_ = 0;
    qHead_ = qCount_ = 0;
    defendUntil_ = -1;
    sweepDir_ = 1;
    commit_ = -1;
    commitDeadline_ = 0;
    commitHealth_ = 0;
    givenUp_ = 0;
    rate_ = 0;
}

int Bot::geodeOnLine(const World& w, float x) const {
    const float lo = x - 0.07f - (w.lanterns >= 3 ? tune.lanternGap : 0.0f), hi = x + 0.07f + (w.lanterns >= 2 ? tune.lanternGap : 0.0f);
    int best = -1;
    float bestZ = 1e9f;
    for (int gi = 0; gi < w.def.geodeCount; ++gi) {
        if (w.geodeState[gi] != kGeodeAlive) continue;
        float gx, gz;
        geodeArrival(w, gi, gx, gz);
        const float half = w.geodeShield[gi] > 0 ? 0.5f * tune.plateWidth : w.def.geodes[gi].radius;
        if (gx + half <= lo || gx - half >= hi || gz >= bestZ) continue;
        best = gi;
        bestZ = gz;
    }
    return best;
}

int Bot::nearestGeode(const World& w, float* x) const {
    int best = -1;
    float bestZ = 1e9f;
    for (int gi = 0; gi < w.def.geodeCount; ++gi) {
        if (w.geodeState[gi] != kGeodeAlive || w.geodeZ[gi] >= bestZ) continue;
        best = gi;
        bestZ = w.geodeZ[gi];
    }
    if (best >= 0 && x) {
        float z;
        geodeArrival(w, best, *x, z);
    }
    return best;
}

float Bot::score(const World& w, float x, bool worth, float bestRate) const {
    const LevelDef& d = w.def;
    // The line is as wide as the row of lanterns (VARIETY_TECH 4).
    float lo = x - 0.07f - (w.lanterns >= 3 ? tune.lanternGap : 0.0f), hi = x + 0.07f + (w.lanterns >= 2 ? tune.lanternGap : 0.0f);
    float mult = 1.0f;
    float budget = 8.0f;
    float bonus = 0;
    const float t = w.time();
    const float base = w.fireRate() * float(w.shotPower()) * float(w.lanterns);
    // Walk the gates, barriers, snuffers, plates and geodes up the lane in z order.
    int order[kItems];
    float zs[kItems];
    float geoX[kMaxGeodes] = {};
    int n = 0;
    for (int i = 0; i < d.gateCount; ++i) {
        order[n] = i;
        zs[n++] = d.gates[i].z;
    }
    for (int i = 0; i < d.barrierCount; ++i) {
        order[n] = 100 + i;
        zs[n++] = d.barriers[i].z;
    }
    for (int i = 0; i < d.snufferCount; ++i) {
        order[n] = 200 + i;
        zs[n++] = d.snuffers[i].z;
    }
    for (int gi = 0; gi < d.geodeCount; ++gi) {
        if (w.geodeState[gi] != kGeodeAlive) continue;
        float gz;
        geodeArrival(w, gi, geoX[gi], gz);
        if (w.geodeShield[gi] > 0) {
            order[n] = 400 + gi;
            zs[n++] = gz - tune.plateGap;
        }
        order[n] = 300 + gi;
        zs[n++] = gz;
    }
    for (int a = 0; a < n; ++a)
        for (int b = a + 1; b < n; ++b)
            if (zs[b] < zs[a]) {
                float tz = zs[a];
                zs[a] = zs[b];
                zs[b] = tz;
                const int to = order[a];
                order[a] = order[b];
                order[b] = to;
            }
    auto overlap = [&](float a0, float a1) {
        const float o = minv(hi, a1) - maxv(lo, a0);
        return o <= 0 ? 0.0f : o / maxv(0.01f, hi - lo);
    };
    float rowGain = 0, rowZ = -1;
    float spread = 0;
    auto closeRow = [&]() {
        if (rowZ < 0) return;
        mult *= maxv(0.05f, 1.0f + rowGain);
        lo -= spread;
        hi += spread;
        rowGain = 0;
        spread = 0;
    };
    for (int k = 0; k < n; ++k) {
        const int id = order[k];
        if (std::fabs(zs[k] - rowZ) > 0.01f) {
            closeRow();
            rowZ = zs[k];
        }
        const float arrive = t + (zs[k] - kMuzzleZ) / tune.wispSpeed;
        if (id < 100) {
            const Gate& G = d.gates[id];
            const float gx = G.slides ? G.cx + G.amp * easedWave(arrive / G.period + G.phase) : G.cx;
            const float f = overlap(gx - G.hw, gx + G.hw);
            if (f <= 0) continue;
            if (G.kind == kGateMul) {
                rowGain += f * float(G.k - 1);
                spread = maxv(spread, f * (tune.fanBase + tune.fanPerK * float(G.k)) * tune.fanDecay);
            } else {
                rowGain -= f * (1.0f - 1.0f / float(G.k));
            }
        } else if (id < 200) {
            const int b = id - 100;
            if (w.barrierHealth[b] <= 0) continue;
            const Barrier& B = d.barriers[b];
            const float f = overlap(B.x0, B.x1);
            if (f <= 0) continue;
            closeRow();
            const float stream = base * mult * f;
            budget -= float(w.barrierHealth[b]) / maxv(1.0f, stream);
        } else if (id < 300) {
            const Snuffer& S = d.snuffers[id - 200];
            const float f = overlap(S.xa - 0.6f, S.xb + 0.6f);
            if (f > 0) rowGain -= f * 0.45f;
        } else if (id < 400) {
            // A geode costs its health in the stream it takes (seconds, as a wall does) and, opened, is worth its boon's
            // gain over the rest of the level (VARIETY_DESIGN 12.1).
            const int gi = id - 300;
            const Geode& G = d.geodes[gi];
            const float f = overlap(geoX[gi] - G.radius, geoX[gi] + G.radius);
            if (f <= 0) continue;
            closeRow();
            budget -= float(w.geodeHp[gi]) / maxv(1.0f, base * mult * f);
            if (!worth || (givenUp_ >> gi) & 1u) continue;
            // A line that would not open it before it slips past gains nothing from it.
            const float spentTime = 8.0f - budget;
            const float speed = G.leaveAt > 0 && t < G.leaveAt ? 0.0f : G.speed;
            if (speed > 0 && spentTime > (zs[k] - tune.geodeSlipZ) / speed) continue;
            if (G.boon == kBoonWard) {
                int near = 0;
                for (int i = 0; i < w.g.n; ++i) near += w.g.z[i] < tune.wardZ ? 1 : 0;
                bonus += bestRate * (2.0f * float(maxv(0, d.coreHealth - w.core)) + 0.5f * float(near));
            } else {
                // What a lasting boon is worth: the time it saves, in the best plain line's power. With the spire left to
                // break at rate r, it falls rest = spire / r from now; a stream (1 + g) times as strong takes rest / (1 + g),
                // so g / (1 + g) of the rest is saved (the time spent opening it is already out of this line's budget).
                // When the spire cannot fall (the bake's measuring runs) the window's end bounds it instead.
                const float g = boonGain(G.boon, w.lanterns, w.quicken, w.bright);
                const float rest = w.unbreakableSpire ? maxv(0.0f, d.targetT - t - spentTime) * (1.0f + g)
                                                      : minv(float(w.spire) / maxv(1.0f, bestRate), 2.0f * d.targetT);
                bonus += bestRate * rest * g / (1.0f + g);
            }
        } else {
            const int gi = id - 400;  // its plate: a wall that moves with it
            const float f = overlap(geoX[gi] - 0.5f * tune.plateWidth, geoX[gi] + 0.5f * tune.plateWidth);
            if (f <= 0) continue;
            closeRow();
            budget -= float(w.geodeShield[gi]) / maxv(1.0f, base * mult * f);
        }
    }
    closeRow();
    return base * mult * maxv(0.0f, budget) + bonus;
}

Input Bot::think(const World& w) {
    Input in;
    const int tick = w.tickCount;
    switch (kind) {
        case BotKind::Idle:
            in.firing = false;
            in.launcherTargetX = 0;
            return in;
        case BotKind::Naive:
            in.firing = true;
            in.launcherTargetX = 0;
            return in;
        case BotKind::Sweep: {
            hand_ += sweepDir_ * 3.0f * kTick;
            if (hand_ > kLauncherRange) {
                hand_ = kLauncherRange;
                sweepDir_ = -1;
            } else if (hand_ < -kLauncherRange) {
                hand_ = -kLauncherRange;
                sweepDir_ = 1;
            }
            in.firing = true;
            in.launcherTargetX = hand_;
            return in;
        }
        default:
            break;
    }
    if (tick >= nextDecision_) {
        nextDecision_ = tick + kDecisionTicks;
        float decided = aim_;
        bool defend = false;
        // Defend rule: a gloom nearer than z = 12 with no wisp within 0.8 m of its line.
        if (tick >= defendUntil_) {
            float nearest = 1e9f, gx = 0;
            for (int i = 0; i < w.g.n; ++i) {
                if (w.g.z[i] >= 12.0f || w.g.z[i] >= nearest) continue;
                bool covered = false;
                for (int j = 0; j < w.w.n && !covered; ++j)
                    covered = w.w.z[j] < w.g.z[i] && std::fabs(w.w.x[j] - w.g.x[i]) < 0.8f;
                if (!covered) {
                    nearest = w.g.z[i];
                    gx = w.g.x[i];
                }
            }
            if (nearest < 1e8f) {
                defend = true;
                decided = gx;
                defendUntil_ = tick + kDefendTicks;
            }
        } else {
            defend = true;
        }
        if (!defend) {
            float gx = 0;
            if (kind == BotKind::Hungry && nearestGeode(w, &gx) >= 0) {
                decided = gx;
            } else {
                const float lo = w.aimMin(), hi = w.aimMax();
                float best = -1, bestX = 0;
                for (int c = 0; c <= 20; ++c) {
                    const float x = lo + (hi - lo) * float(c) / 20.0f;
                    const float s = score(w, x);
                    if (s > best) {
                        best = s;
                        bestX = x;
                    }
                }
                float cur = score(w, aim_);
                if (kind != BotKind::Podless && nearestGeode(w, nullptr) >= 0) {
                    // Again with the boons' worth, against the best plain line's rate (smoothed: sliders make it jump).
                    rate_ = rate_ > 0 ? 0.75f * rate_ + 0.25f * best / 8.0f : best / 8.0f;
                    best = -1;
                    for (int c = 0; c <= 20; ++c) {
                        const float x = lo + (hi - lo) * float(c) / 20.0f;
                        const float s = score(w, x, true, rate_);
                        if (s > best) {
                            best = s;
                            bestX = x;
                        }
                    }
                    // A player who has started on a geode finishes it unless another line is far better, or gives up on
                    // it when the stream has had 2.5 s there and taken less than a tenth of its health.
                    if (commit_ >= 0 && w.geodeState[commit_] != kGeodeAlive) commit_ = -1;
                    if (commit_ >= 0 && tick > commitDeadline_ && 10 * (w.geodeHp[commit_] + w.geodeShield[commit_]) > 9 * commitHealth_) {
                        givenUp_ |= 1u << commit_;
                        commit_ = -1;
                    }
                    float commitX = aim_;
                    if (commit_ >= 0) {
                        float gz;
                        geodeArrival(w, commit_, commitX, gz);
                        commitX = clampv(commitX, lo, hi);
                    }
                    cur = score(w, commitX, true, rate_);
                    if (cur >= best * (commit_ >= 0 ? 0.67f : 0.9f)) {
                        decided = commitX;
                    } else {
                        decided = bestX;
                        const int g = geodeOnLine(w, bestX);
                        commit_ = g >= 0 && score(w, bestX, true, rate_) > score(w, bestX) + 1.0f ? g : -1;
                        if (commit_ >= 0) {
                            const float travel = maxv(0.0f, w.geodeZ[commit_] - kMuzzleZ) / tune.wispSpeed;
                            commitDeadline_ = tick + int((travel + 2.5f) * 60.0f);
                            commitHealth_ = w.geodeHp[commit_] + w.geodeShield[commit_];
                        }
                    }
                } else {
                    commit_ = -1;
                    decided = cur >= best * 0.9f ? aim_ : bestX;
                }
            }
        }
        if (!defend || tick >= defendUntil_ - kDefendTicks) {
            const float noisy = clampv(decided + rng_.range(-kAimNoise, kAimNoise), w.aimMin(), w.aimMax());
            if (qCount_ < 16) {
                queued_[(qHead_ + qCount_) % 16] = noisy;
                queuedTick_[(qHead_ + qCount_) % 16] = tick + kReactionTicks;
                ++qCount_;
            }
            aim_ = decided;
        }
    }
    while (qCount_ > 0 && queuedTick_[qHead_] <= tick) {
        applied_ = queued_[qHead_];
        qHead_ = (qHead_ + 1) % 16;
        --qCount_;
    }
    const float step = kHandSpeed * kTick;
    hand_ += clampv(applied_ - hand_, -step, step);
    in.firing = true;
    in.launcherTargetX = hand_;
    return in;
}

}  // namespace gt::sim

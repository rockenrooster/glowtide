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
}  // namespace

bool botFromName(const char* name, BotKind& out) {
    if (!std::strcmp(name, "greedy")) out = BotKind::Greedy;
    else if (!std::strcmp(name, "naive")) out = BotKind::Naive;
    else if (!std::strcmp(name, "sweep")) out = BotKind::Sweep;
    else if (!std::strcmp(name, "idle")) out = BotKind::Idle;
    else return false;
    return true;
}

const char* botName(BotKind k) {
    switch (k) {
        case BotKind::Greedy: return "greedy";
        case BotKind::Naive: return "naive";
        case BotKind::Sweep: return "sweep";
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
}

float Bot::score(const World& w, float x) const {
    const LevelDef& d = w.def;
    float lo = x - 0.07f, hi = x + 0.07f;
    float mult = 1.0f;
    float budget = 8.0f;
    const float t = w.time();
    // Walk the gates and barriers up the lane in z order.
    int order[kMaxGates + kMaxBarriers + kMaxSnuffers];
    float zs[kMaxGates + kMaxBarriers + kMaxSnuffers];
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
            const float stream = tune.fireRate * mult * f;
            budget -= float(w.barrierHealth[b]) / maxv(1.0f, stream);
        } else {
            const Snuffer& S = d.snuffers[id - 200];
            const float f = overlap(S.xa - 0.6f, S.xb + 0.6f);
            if (f > 0) rowGain -= f * 0.45f;
        }
    }
    closeRow();
    return tune.fireRate * mult * maxv(0.0f, budget);
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
        case BotKind::Greedy:
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
            float best = -1, bestX = 0;
            for (int c = 0; c <= 20; ++c) {
                const float x = -kLauncherRange + 2.0f * kLauncherRange * float(c) / 20.0f;
                const float s = score(w, x);
                if (s > best) {
                    best = s;
                    bestX = x;
                }
            }
            const float cur = score(w, aim_);
            decided = cur >= best * 0.9f ? aim_ : bestX;
        }
        if (!defend || tick >= defendUntil_ - kDefendTicks) {
            const float noisy = clampv(decided + rng_.range(-kAimNoise, kAimNoise), -kLauncherRange, kLauncherRange);
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

#include "game/view.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/tuning.h"

namespace gt {

namespace {

inline int tierOf(i32 p) { return p >= 64 ? 3 : p >= 16 ? 2 : p >= 4 ? 1 : 0; }

// A smooth pseudo-noise in [-1, 1] for the camera's shake.
inline float wobble(float t, float seed) {
    return 0.6f * std::sin(t * 13.1f + seed) + 0.4f * std::sin(t * 29.7f + seed * 2.3f);
}

void formatThousands(int v, char* buf, int n) {
    char tmp[32];
    std::snprintf(tmp, sizeof tmp, "%d", v < 0 ? 0 : v);
    const int len = int(std::strlen(tmp));
    int o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        buf[o++] = tmp[i];
        const int rest = len - 1 - i;
        if (rest > 0 && rest % 3 == 0 && o < n - 1) buf[o++] = ',';
    }
    buf[o] = 0;
}

// Punch: 1 -> 1.22 -> 1 over 160 ms (GAME_DESIGN 10); p runs 1 -> 0.
inline float punchScale(float p) {
    if (p <= 0) return 1.0f;
    const float t = 1.0f - p;  // 0..1
    return t < 0.35f ? 1.0f + 0.22f * (t / 0.35f) : 1.0f + 0.22f * (1.0f - (t - 0.35f) / 0.65f);
}

}  // namespace

// GOLDEN GATE's gate while it lasts, or -1.
int goldenGate(const sim::World& w) {
    for (int k = 0; k < w.def.twistCount; ++k)
        if (w.def.twists[k].type == sim::kTwistGoldenGate && w.twistStart[k] >= 0 && float(w.tickCount - w.twistStart[k]) * sim::kTick < tune.goldenSeconds)
            return w.def.twists[k].param;
    return -1;
}

const char* gateLabel(const sim::Gate& g, char* buf, int n, int k) {
    std::snprintf(buf, size_t(n), "%s%d", g.kind == sim::kGateMul ? "\xC3\x97" : "\xC3\xB7", k > 0 ? k : int(g.k));
    return buf;
}


void View::init(const ViewEnv& env) {
    env_ = env;
    clearParticles();
}

void View::resize(const ViewEnv& env) {
    env_ = env;
    cam.fit(def.len, env.w, env.h, env.safe, env.dp);
}

void View::setLevel(const sim::LevelDef& d, int worldOverride, int paletteAlt) {
    def = d;
    skyWorld = worldOverride > 0 ? worldOverride : d.world;
    wp = &worldPaletteAlt(skyWorld, paletteAlt);
    gp = &gamePalette();
    cam.fit(def.len, env_.w, env_.h, env_.safe, env_.dp);
    followX_ = followV_ = 0;
    trauma = coreFlash = tidePunch = 0;
    bannerTime = 0;
    bannerMark = 0;
    spireFlash_ = spirePunch_ = 0;
    cheer = brighten = desaturate = gloomCheer = 0;
    timeScale = 1;
    for (int i = 0; i < sim::kMaxGates; ++i) gateFlash_[i] = gatePunch_[i] = 0;
    for (int i = 0; i < sim::kMaxBarriers; ++i) barrierShake_[i] = barrierPunch_[i] = 0;
    for (int i = 0; i < sim::kMaxGeodes; ++i) {
        geodePunch_[i] = geodeFlash_[i] = plateShake_[i] = geodeEndT_[i] = 0;
        geodePunchAt_[i] = -1;
        geodeEnd_[i] = 0;
    }
    for (float& p : lanternPop_) p = 0;
    wardRing_ = barrelRing_ = 0;
    kindleT_ = -1;
    kindleRow_ = 0;
    eclipse_ = 0;
    twinDown_[0] = twinDown_[1] = false;
    pipsLit_ = float(d.coreHealth);
    for (Flash& f : flashes_) f.life = 0;
    clearParticles();
}

void View::clearParticles() {
    for (ParticleInstance& p : particles_) {
        std::memset(&p, 0, sizeof p);
        p.t0 = -1000.0f;
        p.life = 0.001f;
    }
    particleHead_ = 0;
}

int View::particlesAlive() const {
    int n = 0;
    for (const ParticleInstance& p : particles_)
        if (float(time) - p.t0 < p.life) ++n;
    return n;
}

void View::spawn(V3 p, V3 v, V3 color, float alpha, float size, float life, int kind, float spin) {
    if (spawnedThisFrame_ >= int(tune.particleBudget)) return;  // dropped, never queued
    ++spawnedThisFrame_;
    ParticleInstance& q = particles_[particleHead_];
    particleHead_ = (particleHead_ + 1) % kParticles;
    q.ox = p.x;
    q.oy = p.y;
    q.oz = p.z;
    q.t0 = float(time);
    q.vx = v.x;
    q.vy = v.y;
    q.vz = v.z;
    q.life = life;
    q.r = u8(clampv(color.x, 0.0f, 1.0f) * 255.0f);
    q.g = u8(clampv(color.y, 0.0f, 1.0f) * 255.0f);
    q.b = u8(clampv(color.z, 0.0f, 1.0f) * 255.0f);
    q.a = u8(clampv(alpha, 0.0f, 1.0f) * 255.0f);
    q.size = size;
    q.kind = float(kind);
    q.seed = float((particleHead_ * 37) % 100) * 0.0628f;
    q.spin = spin;
}

void View::onTick(const sim::World& w) {
    const float t = float(time);
    // Deterministic-enough variety for effects: hash of the tick.
    u32 h = u32(w.tickCount) * 2654435761u;
    auto rnd = [&h]() {
        h ^= h << 13;
        h ^= h >> 17;
        h ^= h << 5;
        return float(h & 0xFFFF) / 65535.0f;
    };
    int clashFx = 0;
    for (int e = 0; e < w.eventCount; ++e) {
        const sim::Event& ev = w.events[e];
        switch (ev.type) {
            case sim::kEvFire:
                lastShot_ = time;
                for (int k = 0; k < maxv(1, ev.a); ++k)  // at each lens
                    spawn(V3(w.lanternX(k), 1.42f, sim::kMuzzleZ + 0.05f), V3(0, 0, 0), gp->wispLight, 0.75f, k ? 0.3f : 0.42f, 0.12f, 4);
                break;
            case sim::kEvGeodeAppear:
                geodeEnd_[ev.a] = 0;
                geodeFlash_[ev.a] = 1.0f;
                for (int k = 0; k < 8; ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 1.6f, 1.0f + rnd() * 1.4f, ev.z), V3((rnd() - 0.5f) * 1.5f, 1 + rnd() * 1.5f, 0), gp->good, 0.9f, 0.14f,
                          0.5f, 0);
                break;
            case sim::kEvGeodeHit:
                if (time - geodePunchAt_[ev.a] >= 0.08) {  // the number punches at most every 80 ms; it counts down every tick
                    geodePunch_[ev.a] = 1.0f;
                    geodePunchAt_[ev.a] = time;
                    spawn(V3(ev.x + (rnd() - 0.5f) * 1.2f, 1.2f + rnd() * 0.6f, ev.z - 0.6f), V3((rnd() - 0.5f) * 3, 2 + rnd() * 2, -1.5f),
                          rnd() < 0.5f ? gp->good : V3(0.8f, 1.0f, 0.98f), 1.0f, 0.15f, 0.45f, 2, (rnd() - 0.5f) * 10);
                }
                geodeFlash_[ev.a] = minv(1.0f, geodeFlash_[ev.a] + 0.35f);
                break;
            case sim::kEvPlateHit:
                plateShake_[ev.a] = 1.0f;
                if (rnd() < 0.5f)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 2.5f, 1.0f, ev.z - 0.3f), V3((rnd() - 0.5f) * 3, 2.5f, -1.5f), V3(0.45f, 0.28f, 0.78f), 1.0f, 0.16f,
                          0.5f, 2, 8.0f);
                break;
            case sim::kEvPlateBreak:
                for (int k = 0; k < 16; ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 3.0f, 0.4f + rnd() * 1.0f, ev.z), V3((rnd() - 0.5f) * 5, 2 + rnd() * 4, (rnd() - 0.6f) * 4),
                          k & 1 ? V3(0.46f, 0.28f, 0.78f) : V3(0.7f, 0.45f, 0.95f), 1.0f, 0.2f, 0.8f, 2, (rnd() - 0.5f) * 12);
                spawn(V3(ev.x, 0.05f, ev.z), V3(0, 0, 0), gp->crack, 0.8f, 2.0f, 0.4f, 4);
                trauma = minv(1.0f, trauma + 0.1f);
                break;
            case sim::kEvGeodeOpen: {
                geodeEnd_[ev.a] = 1;
                geodeEndT_[ev.a] = 0;
                geodeEndX_[ev.a] = ev.x;
                geodeEndZ_[ev.a] = ev.z;
                for (int k = 0; k < 20; ++k)  // the shell bursts into twenty shards
                    spawn(V3(ev.x + (rnd() - 0.5f) * 1.2f, 1.0f + rnd() * 0.9f, ev.z + (rnd() - 0.5f) * 1.2f),
                          V3((rnd() - 0.5f) * 7, 2 + rnd() * 5, (rnd() - 0.5f) * 6), k % 3 == 0 ? hexColor(0xFFD25A) : (k % 3 == 1 ? gp->good : V3(0.85f, 1, 1)),
                          1.0f, 0.2f + rnd() * 0.12f, 0.9f, 2, (rnd() - 0.5f) * 14);
                spawn(V3(ev.x, 1.4f, ev.z), V3(0, 0, 0), V3(1, 0.95f, 0.8f), 0.9f, 1.6f, 0.25f, 4);
                Flash& f = flashes_[flashHead_];
                flashHead_ = (flashHead_ + 1) % kFlashes;
                f = {ev.x, ev.z, 2.2f, t, 0.4f, hexColor(0xFFD25A), 0.55f};
                trauma = minv(1.0f, trauma + 0.12f);
                break;
            }
            case sim::kEvGeodeLost:
                geodeEnd_[ev.a] = ev.b ? 3 : 2;
                geodeEndT_[ev.a] = 0;
                geodeEndX_[ev.a] = ev.x;
                geodeEndZ_[ev.a] = ev.z;
                break;
            case sim::kEvBoon:
                if (ev.a == sim::kBoonLantern) {
                    lanternPop_[clampv(ev.b - 1, 0, 2)] = 1.0f;
                } else if (ev.a == sim::kBoonQuicken || ev.a == sim::kBoonBright) {
                    barrelRing_ = 1.0f;
                } else if (ev.a == sim::kBoonWard) {
                    wardRing_ = 1.0f;
                    trauma = minv(1.0f, trauma + 0.15f);
                } else if (ev.a == sim::kBoonKindle) {
                    kindleT_ = 0;  // the labels count up, row by row up the lane
                    kindleRow_ = 0;
                } else if (ev.a == sim::kBoonComet) {
                    trauma = minv(1.0f, trauma + 0.2f);
                }
                break;
            case sim::kEvGateMul: {
                const int g = ev.a;
                gateFlash_[g] = 1.0f;
                if (gatePunch_[g] < 0.5f) gatePunch_[g] = 1.0f;
                const sim::Gate& G = def.gates[g];
                const int sparks = minv(6, 3 + ev.b);  // 4 to 6
                for (int k = 0; k < sparks; ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 0.8f, 0.5f + rnd() * 0.8f, G.z), V3((rnd() - 0.5f) * 3, 1.5f + rnd() * 2, (rnd() - 0.3f) * 2),
                          k & 1 ? gp->good : gp->wispLight, 0.9f, 0.16f, 0.45f, 0);
                break;
            }
            case sim::kEvGateDiv: {
                gateFlash_[ev.a] = 1.0f;
                if (ev.b > 0) {
                    spawn(V3(ev.x, 0.4f, def.gates[ev.a].z), V3(0, 0.8f, 0.3f), lerp(gp->bad, V3(0.6f, 0.6f, 0.65f), 0.5f), 0.55f, 0.35f,
                          0.5f, 1);
                }
                break;
            }
            case sim::kEvClash: {
                if (++clashFx > 6) break;
                for (int k = 0; k < 6; ++k)
                    spawn(V3(ev.x, 0.45f, ev.z), V3((rnd() - 0.5f) * 4, 1.2f + rnd() * 2.5f, (rnd() - 0.5f) * 3),
                          k % 2 == 0 ? gp->wispLight : gp->gloomEye, 0.95f, 0.16f + rnd() * 0.1f, 0.4f, 0);  // gold and rose
                Flash& f = flashes_[flashHead_];
                flashHead_ = (flashHead_ + 1) % kFlashes;
                f = {ev.x, ev.z, 1.1f, t, 0.25f, V3(1.0f, 0.85f, 0.75f), 0.3f};
                break;
            }
            case sim::kEvBarrierHit: {
                barrierShake_[ev.a] = 1.0f;
                barrierPunch_[ev.a] = 1.0f;
                spawn(V3(ev.x + (rnd() - 0.5f) * 2.5f, 1.0f, ev.z - 0.3f), V3((rnd() - 0.5f) * 3, 2.5f, -1.5f), V3(0.45f, 0.28f, 0.78f),
                      1.0f, 0.18f, 0.6f, 2, 8.0f);
                break;
            }
            case sim::kEvBarrierBreak: {
                const sim::Barrier& B = def.barriers[ev.a];
                for (int k = 0; k < 20; ++k)
                    spawn(V3(B.x0 + rnd() * (B.x1 - B.x0), 0.4f + rnd() * 1.4f, B.z), V3((rnd() - 0.5f) * 5, 2 + rnd() * 4, (rnd() - 0.6f) * 4),
                          k & 1 ? V3(0.46f, 0.28f, 0.78f) : V3(0.7f, 0.45f, 0.95f), 1.0f, 0.22f, 0.9f, 2, (rnd() - 0.5f) * 12);
                spawn(V3((B.x0 + B.x1) * 0.5f, 0.05f, B.z), V3(0, 0, 0), gp->crack, 0.8f, 2.5f, 0.5f, 4);
                trauma = minv(1.0f, trauma + 0.15f);
                break;
            }
            case sim::kEvSnuffed: {
                spawn(V3(ev.x, 0.45f, ev.z), V3(0, 0.6f, 0), V3(0.55f, 0.5f, 0.6f), 0.5f, 0.3f, 0.45f, 1);
                int best = -1;
                float bd = 1e9f;
                for (int s = 0; s < def.snufferCount; ++s) {
                    const float dx = w.snufferX[s] - ev.x, dz = def.snuffers[s].z - ev.z, d = dx * dx + dz * dz;
                    if (d < bd) bd = d, best = s;
                }
                if (best >= 0) snufferFlare_[best] = 1.0f;
                break;
            }
            case sim::kEvBruteDown:
                for (int k = 0; k < 16; ++k)
                    spawn(V3(ev.x, 0.7f, ev.z), V3((rnd() - 0.5f) * 7, 1 + rnd() * 4, (rnd() - 0.5f) * 6), k & 1 ? gp->gloomEye : gp->gloomLight,
                          1.0f, 0.22f, 0.6f, 0);
                spawn(V3(ev.x, 0.05f, ev.z), V3(0, 0, 0), gp->gloomEye, 0.8f, 1.8f, 0.45f, 4);
                trauma = minv(1.0f, trauma + 0.1f);
                break;
            case sim::kEvSpireHit: {
                spireFlash_ = minv(1.0f, spireFlash_ + 0.12f + float(ev.a) * 0.002f);
                if (spirePunch_ < 0.4f) spirePunch_ = 1.0f;
                for (int k = 0; k < minv(4, 1 + ev.a / 8); ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 2, 0.6f + rnd() * 2.5f, def.len + 0.2f), V3((rnd() - 0.5f) * 3, 1 + rnd() * 2, -1.5f - rnd()),
                          k & 1 ? gp->crack : V3(1, 0.95f, 0.85f), 0.9f, 0.2f, 0.5f, 0);
                break;
            }
            case sim::kEvSpireStage:
                trauma = minv(1.0f, trauma + 0.3f);
                for (int k = 0; k < 10; ++k)
                    spawn(V3((rnd() - 0.5f) * 3, 1 + rnd() * 4, def.len + 0.6f), V3((rnd() - 0.5f) * 4, 2 + rnd() * 3, -2 - rnd() * 2),
                          gp->spireLight, 1.0f, 0.25f, 0.9f, 2, (rnd() - 0.5f) * 10);
                break;
            case sim::kEvCoreHit:
                coreFlash = 1.0f;
                trauma = minv(1.0f, trauma + 0.5f);
                spawn(V3(ev.x, 0.5f, 0.6f), V3(0, 1.2f, 0), V3(0.75f, 0.45f, 0.65f), 0.7f, 0.6f, 0.6f, 1);
                break;
            case sim::kEvTideMark:
                tidePunch = 1.0f;
                bannerMark = ev.a;
                bannerTime = 0.9f;
                break;
            case sim::kEvWin: {
                // The goal shatters where it is: the spire, the last twin, the colossus (a hundred shards); a festival's
                // lantern throws gold; dawn is the grade's brightening alone.
                if (def.kind == sim::kHoldout) break;
                float cx = 0, cz = def.len + 1.4f - 0.5f, wide = 4.5f;
                int shards = def.boss ? 100 : 60;
                if (def.kind == sim::kColossus) cx = w.colossusX, cz = w.colossusZ, shards = 100, wide = 3.6f;
                if (def.kind == sim::kTwins) cx = w.spire2 <= 0 && !twinDown_[1] ? tune.twinX : -tune.twinX, wide = 3.0f;
                const V3 gold = hexColor(0xFFD25A);
                for (int k = 0; k < shards; ++k)
                    spawn(V3(cx + (rnd() - 0.5f) * wide, 0.3f + rnd() * 6.0f, cz), V3((rnd() - 0.5f) * 9, 2 + rnd() * 7, -2 - rnd() * 6),
                          def.kind == sim::kFestival ? gold : k % 3 == 0 ? gp->crack : (k % 3 == 1 ? gp->spireLight : V3(1, 0.85f, 0.95f)),
                          1.0f, 0.25f + rnd() * 0.25f, 1.6f, 2, (rnd() - 0.5f) * 14);
                trauma = minv(1.0f, trauma + (def.boss || def.kind == sim::kColossus ? 0.5f : 0.35f));
                break;
            }
            case sim::kEvColossusStep:  // heavy steps: the camera feels them inside 12 m of the lantern
                if (ev.z < 12.0f) trauma = minv(1.0f, trauma + 0.1f);
                for (int k = 0; k < 6; ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 3, 0.1f, ev.z - 1.2f), V3((rnd() - 0.5f) * 2, 0.6f + rnd(), -rnd()), gp->gloomLight, 0.6f,
                          0.18f, 0.5f, 0);
                break;
            case sim::kEvColossusHit:
                spireFlash_ = minv(1.0f, spireFlash_ + 0.1f);
                if (spirePunch_ < 0.4f) spirePunch_ = 1.0f;
                break;
            case sim::kEvCubHit:
                spawn(V3(ev.x + (rnd() - 0.5f), 1.0f + rnd(), ev.z - 1.0f), V3((rnd() - 0.5f) * 2, 1 + rnd(), -1), gp->gloomEye, 0.8f, 0.16f, 0.4f, 0);
                break;
            case sim::kEvCubDown:
                for (int k = 0; k < 24; ++k)
                    spawn(V3(ev.x + (rnd() - 0.5f) * 2, 0.4f + rnd() * 2, ev.z), V3((rnd() - 0.5f) * 6, 1 + rnd() * 4, (rnd() - 0.5f) * 4),
                          k & 1 ? gp->gloomEye : gp->gloomLight, 1.0f, 0.22f, 0.7f, 0);
                trauma = minv(1.0f, trauma + 0.15f);
                break;
            case sim::kEvTwist:
                if (ev.b == sim::kTwistWardWall && ev.a < def.twistCount) {  // the wall rises across the lane
                    const int b = def.twists[ev.a].param;
                    if (b < def.barrierCount) {
                        const sim::Barrier& B = def.barriers[b];
                        for (int k = 0; k < 30; ++k)
                            spawn(V3(B.x0 + (B.x1 - B.x0) * rnd(), 0.2f, B.z), V3(0, 2 + rnd() * 3, 0), gp->crack, 0.9f, 0.2f, 0.6f, 0);
                        barrierShake_[b] = 1.0f;
                    }
                    trauma = minv(1.0f, trauma + 0.2f);
                } else if (ev.b == sim::kTwistGoldenGate && ev.a < def.twistCount) {
                    const int g = def.twists[ev.a].param;
                    if (g < def.gateCount) gateFlash_[g] = gatePunch_[g] = 1.0f;
                } else if (ev.b == sim::kTwistCub || ev.b == sim::kTwistBruteGuard || ev.b == sim::kTwistStampede) {
                    trauma = minv(1.0f, trauma + 0.12f);
                }
                break;
            case sim::kEvLose:
                break;
            default:
                break;
        }
    }
}

void View::update(float dt, const sim::World& w) {
    time += double(dt);
    spawnedThisFrame_ = 0;
    const float k = dt;
    for (int i = 0; i < sim::kMaxGates; ++i) {
        gateFlash_[i] = maxv(0.0f, gateFlash_[i] - k / 0.2f);
        gatePunch_[i] = maxv(0.0f, gatePunch_[i] - k / 0.16f);
    }
    for (int i = 0; i < sim::kMaxBarriers; ++i) {
        barrierShake_[i] = maxv(0.0f, barrierShake_[i] - k / 0.25f);
        barrierPunch_[i] = maxv(0.0f, barrierPunch_[i] - k / 0.16f);
    }
    spireFlash_ = maxv(0.0f, spireFlash_ - k / 0.35f);
    spirePunch_ = maxv(0.0f, spirePunch_ - k / 0.16f);
    trauma = maxv(0.0f, trauma - tune.traumaDecay * k);
    coreFlash = maxv(0.0f, coreFlash - k / 0.25f);
    for (float& f : snufferFlare_) f = maxv(0.0f, f - k / 0.25f);
    tidePunch = maxv(0.0f, tidePunch - k / 0.2f);
    bannerTime = maxv(0.0f, bannerTime - k);
    for (int i = 0; i < sim::kMaxGeodes; ++i) {
        geodePunch_[i] = maxv(0.0f, geodePunch_[i] - k / 0.16f);
        geodeFlash_[i] = maxv(0.0f, geodeFlash_[i] - k / 0.25f);
        plateShake_[i] = maxv(0.0f, plateShake_[i] - k / 0.25f);
        geodeEndT_[i] += k;
    }
    for (float& p : lanternPop_) p = maxv(0.0f, p - k / 0.3f);
    wardRing_ = maxv(0.0f, wardRing_ - k / 0.7f);
    // ECLIPSE: the picture loses 35% of its colour, eased 0.5 s each way (GAME_DESIGN 14; REDUCE EFFECTS halves it).
    const float eclipseWant = def.twistCount > 0 && w.twistOn(sim::kTwistEclipse, tune.eclipseSeconds) ? 0.35f : 0.0f;
    eclipse_ += clampv(eclipseWant - eclipse_, -k / 0.5f * 0.35f, k / 0.5f * 0.35f);
    if (def.kind == sim::kTwins) {  // a twin that falls shatters where it stands
        for (int s = 0; s < 2; ++s) {
            if (twinDown_[s] || (s == 0 ? w.spire : w.spire2) > 0) continue;
            twinDown_[s] = true;
            const float cx = s == 0 ? -tune.twinX : tune.twinX;
            for (int j = 0; j < 50; ++j) {
                const float a = float(j) * 0.618f;
                spawn(V3(cx + (a - std::floor(a) - 0.5f) * 3.0f, 0.3f + float(j % 7) * 0.8f, def.len + 0.9f),
                      V3((a - std::floor(a) - 0.5f) * 8, 2 + float(j % 5), -2 - float(j % 4)), j % 2 ? gp->crack : gp->spireLight, 1.0f, 0.25f,
                      1.4f, 2, float(j % 9) - 4.0f);
            }
            trauma = minv(1.0f, trauma + 0.35f);
        }
    }
    if (kindleT_ >= 0) {  // KINDLE's run: each row's multiply gates count up with a punch, 80 ms apart
        kindleT_ += k;
        while (kindleRow_ < sim::kMaxRows && kindleT_ >= 0.08f * float(kindleRow_)) {
            for (int g = 0; g < def.gateCount; ++g)
                if (def.gates[g].row == kindleRow_ && def.gates[g].kind == sim::kGateMul) gatePunch_[g] = 1.0f, gateFlash_[g] = 1.0f;
            ++kindleRow_;
        }
    }
    barrelRing_ = maxv(0.0f, barrelRing_ - k / 0.35f);
    // The pips relight one by one (WARD), and go out at once.
    pipsLit_ = float(w.core) < pipsLit_ ? float(w.core) : minv(float(w.core), pipsLit_ + k * 20.0f);
    // Camera follow: a critically damped spring toward 12% of the launcher's x.
    const float target = w.launcherX * tune.camFollow;
    const float omega = 6.0f;
    const float a = -2.0f * omega * followV_ - omega * omega * (followX_ - target);
    followV_ += a * k;
    followX_ += followV_ * k;
}

void View::addLight(float x, float z, float rx, float rz, V3 c, float a, int shape) {
    if (lightCount_ >= 512 || a <= 0.003f) return;
    LightInstance& l = lights_[lightCount_++];
    l.x = x;
    l.z = z;
    l.rx = rx;
    l.rz = rz;
    l.r = u8(clampv(c.x, 0.0f, 1.0f) * 255.0f);
    l.g = u8(clampv(c.y, 0.0f, 1.0f) * 255.0f);
    l.b = u8(clampv(c.z, 0.0f, 1.0f) * 255.0f);
    l.a = u8(clampv(a, 0.0f, 1.0f) * 255.0f);
    l.shape = float(shape);
}

void View::buildUnits(const sim::World& w) {
    int n = 0;
    const int tick = w.tickCount;
    auto put = [&](const sim::Units& u, int i, u8 flags, int tier) {
        UnitInstance& q = scratch_[n];
        q.x = u.x[i];
        q.z = u.z[i];
        q.dx = i16(clampv((u.x[i] - u.xp[i]) * 4096.0f, -32000.0f, 32000.0f));
        q.dz = i16(clampv((u.z[i] - u.zp[i]) * 4096.0f, -32000.0f, 32000.0f));
        q.flags = flags;
        q.seed = u.seed[i];
        q.age = u8(clampv(tick - u.born[i], 0, 255));
        q.tier = u8(tier);
        keys_[n] = u16(clampv(int((u.z[i] + 3.0f) * 100.0f), 0, kBuckets - 1));
        ++n;
    };
    // Unit flags for the shaders: 1 gloom, 2 brute, 4 just fired, 8 a comet (a tail), 16 a runner (a streak), 32 a side
    // door's gloom stepping in (it fades in).
    for (int i = 0; i < w.w.n; ++i) {
        const bool comet = (w.w.flags[i] & sim::kComet) != 0;
        put(w.w, i, u8((tick - w.w.born[i] < 12 && w.w.z[i] < sim::kMuzzleZ + 2.0f ? 4 : 0) | (comet ? 8 : 0)), comet ? 3 : tierOf(w.w.power[i]));
    }
    for (int i = 0; i < w.g.n; ++i) {
        const u8 f = w.g.flags[i];
        const bool brute = (f & sim::kBrute) != 0;
        int t = tierOf(w.g.power[i]);
        if (brute && t < 2) t = 2;
        put(w.g, i, u8(1 | (brute ? 2 : 0) | (f & sim::kRunner ? 16 : 0) | (f & sim::kStepping ? 32 : 0)), t);
    }
    // The colossus (tier 5, crowned: flag 64) and the cubs (tier 4): glooms of two new sizes (VARIETY_TECH 7).
    auto body = [&](float x, float z, u8 flags, int tier, u8 seed) {
        if (n >= kMaxUnits) return;
        UnitInstance& q = scratch_[n];
        q.x = x;
        q.z = z;
        q.dx = q.dz = 0;
        q.flags = flags;
        q.seed = seed;
        q.age = 255;
        q.tier = u8(tier);
        keys_[n] = u16(clampv(int((z + 3.0f) * 100.0f), 0, kBuckets - 1));
        ++n;
    };
    if (def.kind == sim::kColossus && w.colossusOut && w.spire > 0) body(w.colossusX, w.colossusZ, 1 | 64, 5, 77);
    for (int c = 0; c < w.cubCount; ++c)
        if (w.cubHp[c] > 0) body(w.cubX[c], w.cubZ[c], 1, 4, u8(31 + c));
    unitCount_ = n;
    // Counting sort on z (centimetres), far to near.
    std::memset(bucket_, 0, sizeof bucket_);
    for (int i = 0; i < n; ++i) ++bucket_[keys_[i]];
    int pos = 0;
    for (int b = kBuckets - 1; b >= 0; --b) {
        const int c = bucket_[b];
        bucket_[b] = pos;
        pos += c;
    }
    for (int i = 0; i < n; ++i) units_[bucket_[keys_[i]]++] = scratch_[i];

    // Row cuts: each distinct gate z, far to near.
    panelCount_ = cutCount_ = 0;
    const int golden = goldenGate(w);
    float zs[sim::kMaxGates];
    int nz = 0;
    for (int g = 0; g < def.gateCount; ++g) {
        bool seen = false;
        for (int j = 0; j < nz; ++j) seen = seen || std::fabs(zs[j] - def.gates[g].z) < 0.01f;
        if (!seen) zs[nz++] = def.gates[g].z;
    }
    for (int a = 0; a < nz; ++a)
        for (int b = a + 1; b < nz; ++b)
            if (zs[b] > zs[a]) {
                const float t = zs[a];
                zs[a] = zs[b];
                zs[b] = t;
            }
    int cursor = 0;
    for (int r = 0; r < nz && cutCount_ < 16; ++r) {
        while (cursor < n && units_[cursor].z > zs[r]) ++cursor;
        RowCut& cut = cuts_[cutCount_++];
        cut.unitEnd = cursor;
        cut.panelFirst = panelCount_;
        for (int g = 0; g < def.gateCount; ++g) {
            const sim::Gate& G = def.gates[g];
            if (std::fabs(G.z - zs[r]) > 0.01f || panelCount_ >= 32) continue;
            PanelInstance& p = panels_[panelCount_++];
            p.x0 = w.gateX[g] - w.gateHw[g];  // a breathing gate's opening as it is now
            p.x1 = w.gateX[g] + w.gateHw[g];
            p.z = G.z;
            p.flash = gateFlash_[g];
            p.kind = g == golden ? 2.0f : G.kind == sim::kGateMul ? 0.0f : 1.0f;  // GOLDEN GATE is drawn gold
            p.arrows = G.slides ? 1.0f : 0.0f;
            p.punch = gatePunch_[g];
            p.pad = 0;
            if (G.slides == sim::kGateHop && panelCount_ < 32) {
                // A hopper's ghost frame where it will land next (VARIETY_DESIGN 4 #1): faint, edges only.
                PanelInstance& q = panels_[panelCount_++];
                q = p;
                const float other = w.gateX[g] >= G.cx ? G.cx - G.amp : G.cx + G.amp;
                q.x0 = other - G.hw;
                q.x1 = other + G.hw;
                q.flash = 0;
                q.pad = 1;
            }
        }
        cut.panelCount = panelCount_ - cut.panelFirst;
    }
}

void View::label(const char* text, V3 at, float emMetres, V3 strokeColor, float punch, UiBatch& b, float alpha, Align align) {
    float sx, sy;
    if (cam.project(at, sx, sy) <= cam.nearZ) return;
    const float ppm = cam.pixelsPerMetre(at);
    const float minEm = tune.labelMinFrac * float(env_.h) / UiBatch::capHeight();
    float em = maxv(emMetres * ppm, minEm) * punchScale(punch);
    const float stroke = maxv(3.0f, em * 0.085f);
    minLabelCapPx = minv(minLabelCapPx, em * UiBatch::capHeight());
    minLabelStrokePx = minv(minLabelStrokePx, stroke);
    const Rgba fill{255, 255, 255, u8(255 * alpha)};
    const Rgba st{u8(strokeColor.x * 255), u8(strokeColor.y * 255), u8(strokeColor.z * 255), u8(255 * alpha)};
    b.text(text, sx, sy, em, fill, st, stroke, align, true);
}

void View::build(Scene& s, const sim::World& w, float alpha, UiBatch& labels) {
    // Camera with follow and shake.
    const float shakeAmt = reduceEffects ? 0.0f : trauma * trauma * tune.shakeMax;
    const float tt = float(time);
    if (topDown) {
        cam.topDown(def.len);
    } else {
        cam.place(followX_, V3(wobble(tt, 1.3f) * shakeAmt, wobble(tt, 7.7f) * shakeAmt * 0.6f, 0));
    }

    FrameUniforms& fu = s.fu;
    std::memcpy(fu.viewProj, cam.viewProj.m, sizeof fu.viewProj);
    fu.camRight = V4(cam.right, 0);
    fu.camUp = V4(cam.up, 0);
    fu.camPos = V4(cam.eyeNow, def.len);
    fu.time = V4(tt, alpha, cheer > 0.0f ? cheer : -gloomCheer, float(std::fmod(time, 60.0)));
    fu.grade = V4(brighten, maxv(desaturate, eclipse_ * (reduceEffects ? 0.5f : 1.0f)), fade, 0.42f);
    fu.screen = V4(float(env_.w), float(env_.h), 1.0f / float(env_.w), 1.0f / float(env_.h));
    fu.lane = V4(sim::kHW, def.len, 1.0f / (2.0f * sim::kHW + 2.0f), 1.0f / (def.len + 6.0f));
    fu.fog = V4(wp->backdrop[2], 0.55f);
    for (int i = 0; i < 4; ++i) fu.backdrop[i] = V4(wp->backdrop[i], 1);
    fu.backdrop[0].w = float(skyFeature(skyWorld));  // the world's sky feature (backdrop.frag)
    fu.laneFar = V4(wp->laneFar, 1);
    fu.laneNear = V4(wp->laneNear, 1);
    fu.rail = V4(wp->rail, 1);
    fu.brighten = V4(wp->brighten, 1);
    fu.wisp[0] = V4(gp->wisp, 1);
    fu.wisp[1] = V4(gp->wispLight, 1);
    fu.wisp[2] = V4(gp->wispRim, 1);
    fu.gloom[0] = V4(gp->gloom, 1);
    fu.gloom[1] = V4(gp->gloomLight, 1);
    fu.gloom[2] = V4(gp->gloomRim, 1);
    fu.gloom[3] = V4(gp->gloomEye, 1);
    fu.good = V4(gp->good, 1);
    fu.bad = V4(gp->bad, 1);
    const float pulse = 0.5f + 0.5f * std::sin(tt * 6.2832f);
    fu.spire = V4(gp->crack, pulse);
    fu.params = V4(tune.glowStrength * (reduceEffects ? 0.5f : 1.0f), tune.glowUnit, tune.lightUnit, tune.hopHeight);
    fu.params2 = V4(tune.hopRate, tune.lightRadius, tune.laneLightAdd, tune.glowUnitSize);
    float sx, sy;
    cam.project(V3(0, 4.0f, def.len + 1.4f), sx, sy);
    fu.spireScreen = V4(sx / float(env_.w), 1.0f - sy / float(env_.h), 0.33f, sim::kMuzzleZ);

    buildUnits(w);
    s.units = units_;
    s.unitCount = unitCount_;
    s.cuts = cuts_;
    s.cutCount = cutCount_;
    s.panels = panels_;
    s.panelCount = panelCount_;

    // Slow lights in the light map.
    lightCount_ = 0;
    const float HW = sim::kHW, len = def.len;
    for (int g = 0; g < def.gateCount; ++g) {
        const sim::Gate& G = def.gates[g];
        addLight(w.gateX[g], G.z - 0.55f, w.gateHw[g], 0.65f, G.kind == sim::kGateMul ? gp->good : gp->bad, 0.32f + gateFlash_[g] * 0.4f, 1);
    }
    for (float sgn : {-1.0f, 1.0f}) addLight(sgn * (HW - 0.25f), len * 0.5f, 0.45f, len * 0.5f + 2.5f, wp->rail, 0.2f, 1);
    // The emitters' glow at the spire's foot goes out while HUSH holds them (VARIETY_DESIGN 13).
    const float hush = w.tickCount < w.hushUntil ? 0.3f : 1.0f;
    if (def.kind == sim::kTwins) {
        for (int s = 0; s < 2; ++s)
            if ((s == 0 ? w.spire : w.spire2) > 0)
                addLight(s == 0 ? -tune.twinX : tune.twinX, len + 0.3f, 2.6f, 1.6f, gp->crack, (0.34f + 0.06f * pulse) * hush + spireFlash_ * 0.4f, 0);
    } else if (def.kind == sim::kHoldout) {
        addLight(0, len + 0.3f, 3.6f, 2.2f, gp->gloom, (0.5f + 0.08f * pulse) * hush, 0);  // the gate's gloom light
    } else if (def.kind == sim::kFestival) {
        addLight(0, len + 0.6f, 4.0f, 2.4f, hexColor(0xFFC654), 0.55f + 0.05f * pulse, 0);
    } else if (def.kind == sim::kColossus) {
        if (w.colossusOut && w.spire > 0) addLight(w.colossusX, w.colossusZ, 3.0f, 2.4f, gp->gloom, 0.4f + spireFlash_ * 0.4f, 0);
    } else {
        addLight(0, len + 0.3f, 4.4f, 1.8f, gp->crack, (0.34f + 0.06f * pulse) * hush + spireFlash_ * 0.45f, 0);
    }
    for (int e = 0; e < def.emitterCount; ++e) {  // side doors: a rose glow on the lane in front of the arch
        const sim::Emitter& E = def.emitters[e];
        if (E.side) addLight(float(E.side) * (HW - 0.8f), E.z, 0.9f, 1.0f, gp->crack, 0.4f * hush, 0);
    }
    addLight(w.launcherX, 0.4f, 2.2f, 1.5f, gp->wisp, 0.3f, 0);
    for (int b = 0; b < def.barrierCount; ++b) {
        if (w.barrierHealth[b] <= 0) continue;
        const sim::Barrier& B = def.barriers[b];
        addLight((B.x0 + B.x1) * 0.5f + w.barrierShift[b], B.z - 0.45f, (B.x1 - B.x0) * 0.5f, 0.55f, gp->crack, 0.22f + barrierShake_[b] * 0.3f, 1);
    }
    for (int i = 0; i < def.snufferCount; ++i) {
        const sim::Snuffer& S = def.snuffers[i];
        addLight((S.xa + S.xb) * 0.5f, S.z, (S.xb - S.xa) * 0.5f + 0.3f, 0.12f, gp->bad, 0.28f, 1);
        addLight(w.snufferX[i], S.z, 1.0f, 1.0f, gp->bad, 0.35f, 0);
    }
    for (int g = 0; g < def.geodeCount; ++g) {
        if (w.geodeState[g] != sim::kGeodeAlive) continue;
        addLight(w.geodeX[g], w.geodeZ[g], 1.25f, 1.25f, gp->good, 0.3f + 0.3f * geodeFlash_[g], 0);
        if (w.geodeShield[g] > 0)
            addLight(w.geodeX[g], w.geodeZ[g] - tune.plateGap, 1.5f, 0.5f, gp->crack, 0.22f + 0.3f * plateShake_[g], 1);
    }
    if (wardRing_ > 0) {  // WARD: a gold wash spreading from the lantern up the lane
        const float e = 1.0f - wardRing_, reach = 1.5f + e * (tune.wardZ - 1.5f);
        addLight(w.launcherX, reach * 0.5f, 5.5f, reach * 0.55f + 0.5f, hexColor(0xFFD25A), 0.5f * wardRing_, 0);
    }
    for (const Flash& f : flashes_) {
        const float age = tt - f.t0;
        if (f.life <= 0 || age < 0 || age > f.life) continue;
        addLight(f.x, f.z, f.r, f.r, f.c, f.a * (1.0f - age / f.life), 0);
    }
    s.lights = lights_;
    s.lightCount = lightCount_;
    s.particles = particles_;
    s.particleCapacity = kParticles;

    // Props that move: the launcher and its pips, sliding gate posts, snuffers.
    propCount_ = 0;
    auto nextProp = [&]() -> PropDraw& {  // a fresh slot: no tint or glow left over from last frame's user
        PropDraw& p = props_[propCount_++];
        p = PropDraw();
        return p;
    };
    {
        const float lx = w.launcherX;
        const float kick = lastShot_ >= 0 ? 0.10f * clampv(1.0f - float(time - lastShot_) / 0.07f, 0.0f, 1.0f) : 0.0f;
        // Every lantern: the first carries the pips; the others are the same lantern at 70% width, standing forward so
        // all the lenses are at the muzzle, and pop out of the first when they arrive (VARIETY_DESIGN 3.1, 13).
        for (int k = 0; k < w.lanterns && k < 3; ++k) {
            const float pop = lanternPop_[k] > 0 ? 1.0f - lanternPop_[k] : 1.0f;
            const float grow = pop < 0.6f ? 1.15f * pop / 0.6f : 1.15f - 0.15f * (pop - 0.6f) / 0.4f;
            const float sxz = (k == 0 ? 1.0f : 0.7f) * (k == 0 ? 1.0f : grow), sy = k == 0 ? 1.0f : grow;
            const float x = k == 0 ? lx : lx + (w.lanternX(k) - lx) * minv(1.0f, pop * 1.4f);
            PropDraw& d = nextProp();
            d.mesh = kMeshLauncher;
            d.model = translation(V3(x, 0, launcherZ_[k])) * scaling(V3(sxz * (1.0f + kick * 0.5f), sy * (1.0f - kick), sxz));
            d.glow = 0;
            PropDraw& l = nextProp();
            l = d;
            l.mesh = kMeshLens;
            // The lens burns brighter with QUICKEN and BRIGHT; a flash runs up the barrel when one is taken.
            l.glow = 1.0f + (w.firing ? 0.4f : 0.0f) + 1.5f * barrelRing_ + 0.3f * float(w.quicken) + 0.4f * float(w.bright);
        }
        const int core = int(pipsLit_ + 0.001f);
        for (int k = 0; k < 10; ++k) {
            const float a = kPi * (0.06f + 0.88f * float(k) / 9.0f);
            PropDraw& p = nextProp();
            p.mesh = kMeshPip;
            p.model = translation(V3(lx - std::cos(a) * 1.27f, 0.0f, 0.2f - std::sin(a) * 0.93f));
            const bool on = k < core;
            p.tint = on ? V3(1.0f, 0.84f, 0.47f) : V3(0.2f, 0.22f, 0.38f);
            p.glow = on ? 0.45f : 0.0f;
        }
    }
    for (int g = 0; g < def.gateCount && propCount_ < kMaxProps - 2; ++g) {  // posts that move: sliders, hoppers, breaths
        const sim::Gate& G = def.gates[g];
        if (!G.slides && G.breath <= 0) continue;
        for (float sgn : {-1.0f, 1.0f}) {
            PropDraw& p = nextProp();
            p.mesh = G.kind == sim::kGateMul ? kMeshPostGood : kMeshPostBad;
            p.model = translation(V3(w.gateX[g] + sgn * w.gateHw[g], 0, G.z));
            p.glow = 1;
        }
    }
    for (int i = 0; i < def.snufferCount && propCount_ < kMaxProps; ++i) {
        PropDraw& p = nextProp();
        p.mesh = kMeshSnuffer;
        p.model = translation(V3(w.snufferX[i], 0.08f * std::sin(tt * 3.0f + float(i)), def.snuffers[i].z));
        p.glow = 1.0f + 1.5f * snufferFlare_[i];
    }
    // Geodes: the turning shell, the badge facing the camera, the plate; one that slipped past dims and sinks.
    for (int g = 0; g < def.geodeCount && propCount_ + 3 <= kMaxProps; ++g) {
        const sim::Geode& G = def.geodes[g];
        const bool alive = w.geodeState[g] == sim::kGeodeAlive;
        const bool sinking = !alive && (geodeEnd_[g] == 2 || geodeEnd_[g] == 3) && geodeEndT_[g] < 0.5f;
        if (!alive && !sinking) continue;
        const float x = alive ? w.geodeX[g] : geodeEndX_[g], z = alive ? w.geodeZ[g] : geodeEndZ_[g];
        const float e = sinking ? geodeEndT_[g] / 0.5f : 0.0f;
        const float y = 1.4f + 0.08f * std::sin(tt * 2.0f + float(g) * 1.7f) - 1.6f * e * e;
        const float s = 1.2f * (G.radius / 0.9f) * (1.0f + 0.5f * (punchScale(geodePunch_[g]) - 1.0f)) * (1.0f - 0.4f * e);  // drawn a little larger than it touches
        const float dim = sinking ? (geodeEnd_[g] == 3 ? 0.35f : 0.55f) * (1.0f - e) + 0.2f : 1.0f;
        PropDraw& sh = nextProp();
        sh.mesh = kMeshGeode;
        sh.model = translation(V3(x, y, z)) * rotationY(tt * 0.6f + float(g) * 0.9f) * scaling(V3(s, s, s));
        sh.tint = V3(1, 1, 1) * (dim * (1.0f + 0.5f * geodeFlash_[g]));
        sh.glow = dim * (1.0f + geodeFlash_[g]);
        PropDraw& bd = nextProp();
        bd.mesh = kMeshBadge0 + G.boon;
        bd.model = translation(V3(x, y, z)) * scaling(V3(s, s, s));
        bd.tint = V3(dim, dim, dim);
        bd.glow = 1.3f * dim;
        if (alive && w.geodeShield[g] > 0) {
            PropDraw& pl = nextProp();
            pl.mesh = kMeshPlate;
            pl.model = translation(V3(x + std::sin(tt * 90.0f + float(g)) * plateShake_[g] * 0.06f, 0, z - tune.plateGap));
            pl.glow = 1.0f + plateShake_[g];
        }
    }
    s.props = props_;
    s.propCount = propCount_;
    s.spireStage = def.kind == sim::kSiege || def.kind == sim::kBoss ? w.spireStage : 0;
    s.spireGone = def.kind == sim::kTwins ? twinDown_[0] : w.result == sim::Result::Win;
    s.spire2Gone = def.kind != sim::kTwins || twinDown_[1];
    s.heartGain = 0.8f + 0.5f * pulse + spireFlash_;
    for (int b = 0; b < sim::kMaxBarriers; ++b) {
        s.barrierAlive[b] = b < def.barrierCount && w.barrierHealth[b] > 0;
        const float sh = barrierShake_[b] * 0.06f;
        // A sliding wall is drawn where it is (VARIETY_TECH 7: the offset the hit shake already uses).
        s.barrierOffset[b] = V3((b < def.barrierCount ? w.barrierShift[b] : 0.0f) + std::sin(tt * 90.0f + float(b)) * sh, 0, 0);
    }
    s.emissiveGain = 1.0f;

    // Labels win: drawn after every unit (TECH_SPEC 5.3), never smaller than tune.labelMinFrac of the frame.
    labels.clear();
    minLabelCapPx = minLabelStrokePx = 1e9f;
    if (showLabels && tune.labels > 0) {
        char buf[32];
        const int golden = goldenGate(w);
        for (int g = 0; g < def.gateCount; ++g) {
            const sim::Gate& G = def.gates[g];
            const V3 col = G.kind == sim::kGateMul ? gp->good : gp->bad;
            // KINDLE: a multiply gate shows its new factor once the run up the lane has reached its row.
            const bool kindled = w.kindle && G.kind == sim::kGateMul && (kindleT_ < 0 || G.row < kindleRow_);
            int k = kindled ? minv(int(tune.kindleMax), int(G.k) + 1) : int(G.k);
            if (g == golden) k += int(tune.goldenExtra);  // GOLDEN GATE: two more, in gold
            label(gateLabel(G, buf, sizeof buf, k), V3(w.gateX[g], 1.95f, G.z - 0.05f), 1.45f, g == golden ? V3(0.45f, 0.25f, 0.02f) : col * 0.2f,
                  gatePunch_[g], labels);
        }
        for (int b = 0; b < def.barrierCount; ++b) {
            if (w.barrierHealth[b] <= 0) continue;
            const sim::Barrier& B = def.barriers[b];
            std::snprintf(buf, sizeof buf, "%d", int(w.barrierHealth[b]));
            // Spans of one wall (The tollgate's five, a toll road's three) take turns high and low, so narrow neighbours'
            // numbers do not run together.
            int rank = 0;
            for (int o = 0; o < def.barrierCount; ++o)
                rank += o != b && std::fabs(def.barriers[o].z - B.z) < 0.5f && def.barriers[o].x0 < B.x0 ? 1 : 0;
            label(buf, V3((B.x0 + B.x1) * 0.5f + w.barrierShift[b], rank % 2 ? 1.9f : 0.85f, B.z - 0.3f), 1.1f, gp->spireDark,
                  barrierPunch_[b], labels);
        }
        for (int i = 0; i < w.g.n; ++i) {
            if (!(w.g.flags[i] & sim::kBrute)) continue;
            std::snprintf(buf, sizeof buf, "%d", int(w.g.power[i]));
            label(buf, V3(w.g.x[i], 1.75f, w.g.z[i]), 0.9f, gp->gloomRim, 0, labels);
        }
        for (int g = 0; g < def.geodeCount; ++g) {
            if (w.geodeState[g] != sim::kGeodeAlive) continue;
            const float x = w.geodeX[g], z = w.geodeZ[g];
            formatThousands(int(w.geodeHp[g]), buf, sizeof buf);
            label(buf, V3(x, 2.75f, z), 1.0f, V3(0.05f, 0.3f, 0.34f), geodePunch_[g], labels);
            if (w.geodeShield[g] > 0) {
                formatThousands(int(w.geodeShield[g]), buf, sizeof buf);
                label(buf, V3(x, 0.75f, z - tune.plateGap - 0.3f), 0.95f, gp->spireDark, plateShake_[g], labels);
            }
        }
        const float zc = def.len + 1.4f;
        if (w.result != sim::Result::Win && (def.kind == sim::kSiege || def.kind == sim::kBoss)) {
            formatThousands(int(w.spire), buf, sizeof buf);
            label(buf, V3(0, 2.6f, zc - 1.4f * 0.87f * (1.0f - 2.6f / 6.3f) - 0.3f), 1.7f, gp->spireDark, spirePunch_, labels);
        } else if (def.kind == sim::kTwins) {
            for (int s = 0; s < 2; ++s) {
                const i32 hp = s == 0 ? w.spire : w.spire2;
                if (hp <= 0) continue;
                formatThousands(int(hp), buf, sizeof buf);
                // Each grows outward from the middle, so two six-figure goals never run into one number.
                label(buf, V3(s == 0 ? -0.4f : 0.4f, 2.2f, zc - 0.9f), 1.1f, gp->spireDark, spirePunch_, labels, 1.0f,
                      s == 0 ? Align::Right : Align::Left);
            }
        } else if (def.kind == sim::kColossus && w.colossusOut && w.spire > 0) {
            formatThousands(int(w.spire), buf, sizeof buf);
            label(buf, V3(w.colossusX, 4.6f, w.colossusZ), 1.5f, gp->gloomRim, spirePunch_, labels);
        }
        for (int c = 0; c < w.cubCount; ++c) {
            if (w.cubHp[c] <= 0) continue;
            formatThousands(int(w.cubHp[c]), buf, sizeof buf);
            label(buf, V3(w.cubX[c], 2.9f, w.cubZ[c]), 1.1f, gp->gloomRim, 0, labels);
        }
    }
    s.labels = &labels;
}

}  // namespace gt

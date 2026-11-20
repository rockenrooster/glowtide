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

const char* gateLabel(const sim::Gate& g, char* buf, int n) {
    std::snprintf(buf, size_t(n), "%s%d", g.kind == sim::kGateMul ? "\xC3\x97" : "\xC3\xB7", int(g.k));
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

void View::setLevel(const sim::LevelDef& d, int worldOverride) {
    def = d;
    wp = &worldPalette(worldOverride > 0 ? worldOverride : d.world);
    gp = &gamePalette();
    cam.fit(def.len, env_.w, env_.h, env_.safe, env_.dp);
    followX_ = followV_ = 0;
    trauma = coreFlash = tidePunch = 0;
    bannerTime = 0;
    bannerMark = 0;
    spireFlash_ = spirePunch_ = 0;
    cheer = brighten = desaturate = 0;
    timeScale = 1;
    for (int i = 0; i < sim::kMaxGates; ++i) gateFlash_[i] = gatePunch_[i] = 0;
    for (int i = 0; i < sim::kMaxBarriers; ++i) barrierShake_[i] = barrierPunch_[i] = 0;
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
    for (int e = 0; e < w.eventCount; ++e) {
        const sim::Event& ev = w.events[e];
        switch (ev.type) {
            case sim::kEvFire:
                break;
            case sim::kEvGateMul: {
                const int g = ev.a;
                gateFlash_[g] = 1.0f;
                if (gatePunch_[g] < 0.5f) gatePunch_[g] = 1.0f;
                const sim::Gate& G = def.gates[g];
                const int sparks = minv(6, 2 + ev.b);
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
                for (int k = 0; k < 3; ++k)
                    spawn(V3(ev.x, 0.45f, ev.z), V3((rnd() - 0.5f) * 4, 1.2f + rnd() * 2.5f, (rnd() - 0.5f) * 3),
                          k == 0 ? gp->wispLight : (k == 1 ? gp->gloomEye : V3(1, 1, 1)), 0.95f, 0.16f + rnd() * 0.1f, 0.4f, 0);
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
            case sim::kEvSnuffed:
                spawn(V3(ev.x, 0.45f, ev.z), V3(0, 0.6f, 0), V3(0.55f, 0.5f, 0.6f), 0.5f, 0.3f, 0.45f, 1);
                break;
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
                const float zc = def.len + 1.4f;
                for (int k = 0; k < (def.boss ? 100 : 60); ++k)
                    spawn(V3((rnd() - 0.5f) * 4.5f, 0.3f + rnd() * 6.0f, zc - 0.5f), V3((rnd() - 0.5f) * 9, 2 + rnd() * 7, -2 - rnd() * 6),
                          k % 3 == 0 ? gp->crack : (k % 3 == 1 ? gp->spireLight : V3(1, 0.85f, 0.95f)), 1.0f, 0.25f + rnd() * 0.25f,
                          1.6f, 2, (rnd() - 0.5f) * 14);
                trauma = minv(1.0f, trauma + (def.boss ? 0.6f : 0.35f));
                break;
            }
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
    tidePunch = maxv(0.0f, tidePunch - k / 0.2f);
    bannerTime = maxv(0.0f, bannerTime - k);
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
    for (int i = 0; i < w.w.n; ++i) put(w.w, i, 0, tierOf(w.w.power[i]));
    for (int i = 0; i < w.g.n; ++i) {
        const bool brute = (w.g.flags[i] & sim::kBrute) != 0;
        int t = tierOf(w.g.power[i]);
        if (brute && t < 2) t = 2;
        put(w.g, i, u8(1 | (brute ? 2 : 0)), t);
    }
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
            p.x0 = w.gateX[g] - G.hw;
            p.x1 = w.gateX[g] + G.hw;
            p.z = G.z;
            p.flash = gateFlash_[g];
            p.kind = G.kind == sim::kGateMul ? 0.0f : 1.0f;
            p.arrows = G.slides ? 1.0f : 0.0f;
            p.punch = gatePunch_[g];
            p.pad = 0;
        }
        cut.panelCount = panelCount_ - cut.panelFirst;
    }
}

void View::label(const char* text, V3 at, float emMetres, V3 strokeColor, float punch, UiBatch& b, float alpha) {
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
    b.text(text, sx, sy, em, fill, st, stroke, Align::Center, true);
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
    fu.time = V4(tt, alpha, cheer, float(std::fmod(time, 60.0)));
    fu.grade = V4(brighten, desaturate, fade, 0.42f);
    fu.screen = V4(float(env_.w), float(env_.h), 1.0f / float(env_.w), 1.0f / float(env_.h));
    fu.lane = V4(sim::kHW, def.len, 1.0f / (2.0f * sim::kHW + 2.0f), 1.0f / (def.len + 6.0f));
    fu.fog = V4(wp->backdrop[2], 0.55f);
    for (int i = 0; i < 4; ++i) fu.backdrop[i] = V4(wp->backdrop[i], 1);
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
    fu.spireScreen = V4(sx / float(env_.w), 1.0f - sy / float(env_.h), 0.33f, 0);

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
        addLight(w.gateX[g], G.z - 0.55f, G.hw, 0.65f, G.kind == sim::kGateMul ? gp->good : gp->bad, 0.32f + gateFlash_[g] * 0.4f, 1);
    }
    for (float sgn : {-1.0f, 1.0f}) addLight(sgn * (HW - 0.25f), len * 0.5f, 0.45f, len * 0.5f + 2.5f, wp->rail, 0.2f, 1);
    addLight(0, len + 0.3f, 4.4f, 1.8f, gp->crack, 0.34f + 0.06f * pulse + spireFlash_ * 0.45f, 0);
    addLight(w.launcherX, 0.4f, 2.2f, 1.5f, gp->wisp, 0.3f, 0);
    for (int b = 0; b < def.barrierCount; ++b) {
        if (w.barrierHealth[b] <= 0) continue;
        const sim::Barrier& B = def.barriers[b];
        addLight((B.x0 + B.x1) * 0.5f, B.z - 0.45f, (B.x1 - B.x0) * 0.5f, 0.55f, gp->crack, 0.22f + barrierShake_[b] * 0.3f, 1);
    }
    for (int i = 0; i < def.snufferCount; ++i) {
        const sim::Snuffer& S = def.snuffers[i];
        addLight((S.xa + S.xb) * 0.5f, S.z, (S.xb - S.xa) * 0.5f + 0.3f, 0.12f, gp->bad, 0.28f, 1);
        addLight(w.snufferX[i], S.z, 1.0f, 1.0f, gp->bad, 0.35f, 0);
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
    {
        const float lx = w.launcherX;
        const float kick = w.firing ? 0.06f * (0.5f + 0.5f * std::sin(tt * 75.0f)) : 0.0f;
        PropDraw& d = props_[propCount_++];
        d.mesh = kMeshLauncher;
        d.model = translation(V3(lx, 0, 0.2f)) * scaling(V3(1.0f + kick * 0.5f, 1.0f - kick, 1.0f));
        d.glow = 0;
        PropDraw& l = props_[propCount_++];
        l = d;
        l.mesh = kMeshLens;
        l.glow = 1.0f + (w.firing ? 0.4f : 0.0f);
        const int core = w.core;
        for (int k = 0; k < 10; ++k) {
            const float a = kPi * (0.06f + 0.88f * float(k) / 9.0f);
            PropDraw& p = props_[propCount_++];
            p.mesh = kMeshPip;
            p.model = translation(V3(lx - std::cos(a) * 1.27f, 0.0f, 0.2f - std::sin(a) * 0.93f));
            const bool on = k < core;
            p.tint = on ? V3(1.0f, 0.84f, 0.47f) : V3(0.2f, 0.22f, 0.38f);
            p.glow = on ? 0.45f : 0.0f;
        }
    }
    for (int g = 0; g < def.gateCount && propCount_ < 60; ++g) {
        const sim::Gate& G = def.gates[g];
        if (!G.slides) continue;
        for (float sgn : {-1.0f, 1.0f}) {
            PropDraw& p = props_[propCount_++];
            p.mesh = G.kind == sim::kGateMul ? kMeshPostGood : kMeshPostBad;
            p.model = translation(V3(w.gateX[g] + sgn * G.hw, 0, G.z));
            p.glow = 1;
        }
    }
    for (int i = 0; i < def.snufferCount && propCount_ < 64; ++i) {
        PropDraw& p = props_[propCount_++];
        p.mesh = kMeshSnuffer;
        p.model = translation(V3(w.snufferX[i], 0.08f * std::sin(tt * 3.0f + float(i)), def.snuffers[i].z));
        p.glow = 1;
    }
    s.props = props_;
    s.propCount = propCount_;
    s.spireStage = w.spireStage;
    s.spireGone = w.result == sim::Result::Win;
    s.heartGain = 0.8f + 0.5f * pulse + spireFlash_;
    for (int b = 0; b < sim::kMaxBarriers; ++b) {
        s.barrierAlive[b] = b < def.barrierCount && w.barrierHealth[b] > 0;
        const float sh = barrierShake_[b] * 0.06f;
        s.barrierOffset[b] = V3(std::sin(tt * 90.0f + float(b)) * sh, 0, 0);
    }
    s.emissiveGain = 1.0f;

    // Labels win: drawn after every unit (TECH_SPEC 5.3), never smaller than tune.labelMinFrac of the frame.
    labels.clear();
    minLabelCapPx = minLabelStrokePx = 1e9f;
    if (showLabels && tune.labels > 0) {
        char buf[32];
        for (int g = 0; g < def.gateCount; ++g) {
            const sim::Gate& G = def.gates[g];
            const V3 col = G.kind == sim::kGateMul ? gp->good : gp->bad;
            label(gateLabel(G, buf, sizeof buf), V3(w.gateX[g], 1.95f, G.z - 0.05f), 1.45f, col * 0.2f, gatePunch_[g], labels);
        }
        for (int b = 0; b < def.barrierCount; ++b) {
            if (w.barrierHealth[b] <= 0) continue;
            const sim::Barrier& B = def.barriers[b];
            std::snprintf(buf, sizeof buf, "%d", int(w.barrierHealth[b]));
            label(buf, V3((B.x0 + B.x1) * 0.5f, 0.85f, B.z - 0.3f), 1.1f, gp->spireDark, barrierPunch_[b], labels);
        }
        for (int i = 0; i < w.g.n; ++i) {
            if (!(w.g.flags[i] & sim::kBrute)) continue;
            std::snprintf(buf, sizeof buf, "%d", int(w.g.power[i]));
            label(buf, V3(w.g.x[i], 1.75f, w.g.z[i]), 0.9f, gp->gloomRim, 0, labels);
        }
        if (w.result != sim::Result::Win) {
            formatThousands(int(w.spire), buf, sizeof buf);
            const float zc = def.len + 1.4f;
            label(buf, V3(0, 2.6f, zc - 1.4f * 0.87f * (1.0f - 2.6f / 6.3f) - 0.3f), 1.7f, gp->spireDark, spirePunch_, labels);
        }
    }
    s.labels = &labels;
}

}  // namespace gt

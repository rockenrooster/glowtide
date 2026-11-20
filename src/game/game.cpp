#include "game/game.h"

#include <cmath>
#include <cstring>

#include "core/clock.h"
#include "core/log.h"
#include "core/rng.h"
#include "core/tuning.h"
#include "gfx/gl.h"
#include "gfx/shader.h"

namespace gt {

bool Game::init(const Args& args, const GameEnv& env) {
    args_ = args;
    env_ = env;
    Rng rng(args.seed >= 0 ? u64(args.seed) : 7u);
    cloudN_ = kCloud;
    for (int i = 0; i < cloudN_; ++i) {
        cx_[i] = rng.range(0, float(env.frameW));
        cy_[i] = rng.range(0, float(env.frameH));
        cvx_[i] = rng.range(-120, 120);
        cvy_[i] = rng.range(-240, 240);
        cdx_[i] = cdy_[i] = 0;
        const u32 r = u32(rng.rangei(150, 255)), g = u32(rng.rangei(120, 220)), b = u32(rng.rangei(40, 120));
        ccol_[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
    markerX_ = float(env.frameW) * 0.5f;
    return true;
}

bool Game::initGraphics() {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUniforms), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kFrameBinding, ubo_);
    if (!debug_.init()) return false;
    checkGl("Game::initGraphics");
    return true;
}

void Game::destroyGraphics() {
    debug_.destroy();
    if (ubo_) glDeleteBuffers(1, &ubo_);
    ubo_ = 0;
}

void Game::resize(const GameEnv& env) { env_ = env; }

void Game::pointer(const PointerEvent& e) {
    if (e.type == PtrType::Down) fingerDown_ = true;
    if (e.type == PtrType::Up) fingerDown_ = false;
    fingerX_ = e.x;
    fingerY_ = e.y;
    if (e.type == PtrType::Move && fingerDown_) {
        // P0 drag mapping: the reference rig shows the lane at about 92 px a metre on a 1080 px wide frame
        // (TECH_SPEC 5.2); the launcher moves gain x the finger's movement in lane metres.
        const float pxPerMetre = 92.0f * float(env_.frameW) / 1080.0f;
        const float metres = e.dx / pxPerMetre * tune.dragGain;
        markerX_ = clampv(markerX_ + metres * pxPerMetre, 0.0f, float(env_.frameW));
    }
}

void Game::key(Key k, bool down) {
    if (down && k == Key::Back) quit_ = true;
}

void Game::background() { GT_TAG("GTLIFE", "game: paused"); }
void Game::foreground() { GT_TAG("GTLIFE", "game: resumed"); }

void Game::tick() {
    const double t0 = nowSeconds();
    const float dt = 1.0f / 60.0f;
    const float W = float(env_.frameW), H = float(env_.frameH);
    for (int i = 0; i < cloudN_; ++i) {
        const float ox = cx_[i], oy = cy_[i];
        cx_[i] += cvx_[i] * dt;
        cy_[i] += cvy_[i] * dt;
        if (cx_[i] < 0) cx_[i] += W;
        if (cx_[i] >= W) cx_[i] -= W;
        if (cy_[i] < 0) cy_[i] += H;
        if (cy_[i] >= H) cy_[i] -= H;
        cdx_[i] = cx_[i] - ox;
        cdy_[i] = cy_[i] - oy;
        if (std::fabs(cdx_[i]) > W * 0.5f || std::fabs(cdy_[i]) > H * 0.5f) cdx_[i] = cdy_[i] = 0;  // wrapped
    }
    ++tickCount_;
    lastTick_ = float(nowSeconds() - t0);
}

void Game::update(float dt, float alpha) {
    time_ += dt;
    alpha_ = alpha;
    frameMs_[frameMsHead_] = dt * 1000.0f;
    frameMsHead_ = (frameMsHead_ + 1) % 120;
}

void Game::render(unsigned fbo, int w, int h) {
    std::memset(&fu_, 0, sizeof fu_);
    fu_.time = V4(float(time_), alpha_, 0, float(std::fmod(time_, 60.0)));
    fu_.screen = V4(float(w), float(h), 1.0f / float(w), 1.0f / float(h));
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof fu_, &fu_, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kFrameBinding, ubo_);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    glClearColor(0.04f, 0.04f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    debug_.clear();
    const float s = float(w) / 1080.0f;
    if (!std::strcmp(args_.scenario, "touch_test")) {
        debug_.rect(0, float(h) * 0.8f, float(w), 4 * s, 0x60F4E8FF);
        debug_.add({markerX_, float(h) * 0.8f, 0, 0, 90 * s, 90 * s, 34, 104, 132, 255});
        if (fingerDown_) debug_.add({fingerX_, fingerY_, 0, 0, 120 * s, 120 * s, 255, 198, 84, 160});
        for (int i = 0; i < 120; ++i) {
            const float ms = frameMs_[(frameMsHead_ + i) % 120];
            const float bh = ms * 6 * s;
            const u32 c = ms > 25 ? 0xFF547CFF : 0x54F2E0FF;
            debug_.rect(40 * s + float(i) * 8 * s, 300 * s - bh, 6 * s, bh, c);
        }
        debug_.rect(40 * s, 300 * s - 16.7f * 6 * s, 960 * s, 2 * s, 0xFFFFFF80);
    } else {
        for (int i = 0; i < cloudN_; ++i) {
            const u32 c = ccol_[i];
            debug_.add({cx_[i], cy_[i], cdx_[i], cdy_[i], 18 * s, 18 * s, u8(c >> 24), u8(c >> 16), u8(c >> 8), 220});
        }
    }
    debug_.draw();
    checkGl("Game::render");
}

}  // namespace gt

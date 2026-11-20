#pragma once
// The game: scenarios, screens, the world, the view and their drawing. App drives it.

#include "core/args.h"
#include "game/input.h"
#include "gfx/debug_draw.h"
#include "gfx/frame_uniforms.h"

namespace gt {

struct GameEnv {
    int frameW = 0, frameH = 0;
    float dpScale = 1;
    SafeArea safe;
    bool android = false;
    float refreshHz = 60;
};

class Game {
public:
    bool init(const Args& args, const GameEnv& env);
    bool initGraphics();
    void destroyGraphics();
    void resize(const GameEnv& env);

    void pointer(const PointerEvent& e);
    void key(Key k, bool down);
    void background();
    void foreground();

    void tick();                       // one fixed 1/60 s step
    void update(float dt, float alpha);  // once a frame
    void render(unsigned fbo, int w, int h);

    bool quitRequested() const { return quit_; }
    int exitCode() const { return exitCode_; }
    // For benchmarks.
    int wisps() const { return cloudN_; }
    int glooms() const { return 0; }
    int particles() const { return 0; }
    float lastTickSeconds() const { return lastTick_; }

private:
    Args args_;
    GameEnv env_;
    bool quit_ = false;
    int exitCode_ = 0;
    float lastTick_ = 0;
    double time_ = 0;
    float alpha_ = 1;
    int tickCount_ = 0;

    FrameUniforms fu_;
    unsigned ubo_ = 0;
    DebugDraw debug_;

    // P0 placeholder world: a seeded cloud of points with fixed velocities that wrap.
    static constexpr int kCloud = 1000;
    int cloudN_ = 0;
    float cx_[kCloud], cy_[kCloud], cvx_[kCloud], cvy_[kCloud], cdx_[kCloud], cdy_[kCloud];
    u32 ccol_[kCloud];
    // touch_test
    bool fingerDown_ = false;
    float fingerX_ = 0, fingerY_ = 0, markerX_ = 0;
    float frameMs_[120] = {};
    int frameMsHead_ = 0;
};

}  // namespace gt

#pragma once
// The game (TECH_SPEC 7): screens, the world, the view, feedback and their drawing. App drives it.

#include "audio/mixer.h"
#include "core/args.h"
#include "game/feedback.h"
#include "game/hud.h"
#include "game/input.h"
#include "game/refresh.h"
#include "game/save.h"
#include "game/ui.h"
#include "game/view.h"
#include "gfx/debug_draw.h"
#include "gfx/renderer.h"
#include "sim/bots.h"
#include "sim/world.h"

namespace gt {

struct GameEnv {
    int frameW = 0, frameH = 0;
    float dpScale = 1;
    SafeArea safe;
    bool android = false;
    float refreshHz = 60;
};

enum class Screen { Home, Play, Pause, Win, Lose, Settings };
const char* screenName(Screen s);

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

    void tick();                         // one fixed 1/60 s step
    void update(float dt, float alpha);  // once a frame
    // The frame's interval on the display (present times, else swaps): the fallback counts late frames per level.
    void frameShown(float intervalSeconds);
    void render(unsigned fbo, int w, int h);

    bool quitRequested() const { return quit_; }
    int exitCode() const { return exitCode_; }
    int wisps() const { return world_.w.n; }
    int glooms() const { return world_.g.n; }
    int particles() const { return view_.particlesAlive(); }
    float lastTickSeconds() const { return lastTick_; }
    audio::Mixer& mixer() { return mixer_; }
    // The screen's layout lint result after a picture (exit 7 when it fails).
    int lintProblems() const { return lintProblems_; }
    bool lintRan() const { return lintRan_; }
    // Haptic pulses sent to the platform, and the most within any one second.
    int hapticsSent() const { return hapticsSent_; }
    int hapticPeakPerSecond() const { return hapticPeak_; }

private:
    // Levels and screens.
    void enterLevel(int index, bool fade);
    void startPlaying();
    void setScreen(Screen s);
    void finishLevel(bool won);
    void saveNow();
    void applySettings();
    void drawScreens(float dt);
    void drawHome(float dt);
    void drawPause();
    void drawResult(bool won);
    void drawSettings(float dt);
    void drawHint();
    void showHint(int id);
    void popGloomsAbove(float z);
    // Pictures and test scenes.
    bool setupPicture(const char* name);
    void setupBenchFlood();
    void tickPictureScript();
    void drawDebug(int w, int h);
    // P2 exit check: the moment strips agree with the rules by count, from the simulation's state.
    struct StripPre {
        i64 gloomPower = 0, cancelled = 0, spawned = 0;
        i32 core = 0;
        bool active = false;
    };
    StripPre stripBefore() const;
    void stripCheck(const StripPre& pre);
    void flowStep();
    void tap(float x, float y);

    Args args_;
    GameEnv env_;
    Layout layout_;
    bool quit_ = false;
    int exitCode_ = 0;
    float lastTick_ = 0;
    float alpha_ = 1;

    sim::World world_;
    sim::Bot bot_;
    bool useBot_ = false;
    bool panScript_ = false;  // the pan_strip picture: fast swings across the lane while firing
    View view_;
    Renderer renderer_;
    Scene scene_;
    UiBatch labels_, uiBatch_;
    Ui ui_;
    DebugDraw debug_;
    bool graphicsReady_ = false;
    audio::Mixer mixer_;
    Feedback feedback_;
    SaveData save_;
    char savePath_[512] = "";
    bool saveEnabled_ = true;

    enum class Mode { Debug, Play, Picture, BenchFlood } mode_ = Mode::Play;
    Screen screen_ = Screen::Home;
    Screen settingsReturn_ = Screen::Home;
    float screenTime_ = 0;       // seconds in the current screen
    float resultTime_ = 0;       // seconds since the level ended
    bool worldTicks_ = false;
    float resumeRamp_ = 1;       // time ramps back up after RESUME
    float tickBudget_ = 0;
    int level_ = 1;
    bool coldStart_ = true;
    float titleFade_ = 1;
    int stars_ = 0;
    int starsLanded_ = 0;
    float levelTime_ = 0;
    float fadeOut_ = 0;          // > 0: fading out before entering `pendingLevel_`
    int pendingLevel_ = 0;
    float fadeIn_ = 1;
    int hint_ = -1;
    float hintTime_ = 0;
    float holdProgress_ = 0;
    bool confirmReset_ = false;
    float popWaveZ_ = 0;
    int firingTicks_ = 0, divPasses_ = 0, mulPasses_ = 0;
    char advice_[64] = "";
    bool pausePressed_ = false;
    int lintProblems_ = 0;
    bool lintRan_ = false;
    int pictureTick_ = 0;
    int stripFrames_ = 0, stripFailures_ = 0;
    static constexpr int kHapticRing = 24;
    double hapticRing_[kHapticRing] = {};
    int hapticsSent_ = 0, hapticPeak_ = 0;
    int levelFrames_ = 0, levelLate_ = 0;
    // The updater (docs/RELEASE_PIPELINE.md): asked once a launch; the picture home_update pretends GitHub has build 999.
    bool updateAsked_ = false;
    int updateOverride_ = 0;
    char pictureScreen_[16] = "";

    // Player input (GAME_DESIGN 4): the oldest finger still down owns the gesture.
    static constexpr int kFingers = 6;
    i64 fingers_[kFingers] = {};
    int fingerCount_ = 0;
    i64 uiFingers_[kFingers] = {};
    int uiFingerCount_ = 0;
    float target_ = 0;
    bool keyLeft_ = false, keyRight_ = false, keyFire_ = false;
    bool debugReadout_ = false;
    float frameMs_ = 16.7f;

    // The flow test (TECH_SPEC 8.4).
    int flowStep_ = 0;
    int flowWait_ = 0;
    int flowFailures_ = 0;
    int flowChecks_ = 0;
    bool flowTapPending_ = false;
    i64 flowProduced_ = 0;
    float flowLauncher_ = 0;

    // P0 test scenes (touch_test, bench_idle).
    static constexpr int kCloud = 1000;
    int cloudN_ = 0;
    float cx_[kCloud], cy_[kCloud], cvx_[kCloud], cvy_[kCloud], cdx_[kCloud], cdy_[kCloud];
    u32 ccol_[kCloud];
    float markerX_ = 0, fingerX_ = 0, fingerY_ = 0;
    float frameHist_[120] = {};
    int frameHistHead_ = 0;
};

}  // namespace gt

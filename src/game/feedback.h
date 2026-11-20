#pragma once
// Feedback (GAME_DESIGN 10, 12, 13): the simulation's events become sounds for the mixer and haptic pulses, with the
// rate limits the design asks for. No graphics: the headless tool renders audio through it too.

#include "audio/mixer.h"
#include "sim/world.h"

namespace gt {

enum HapticKind { kHapticLight = 0, kHapticMedium = 1, kHapticStrong = 2, kHapticWin = 3, kHapticLose = 4, kHapticDouble = 5 };

class Feedback {
public:
    void init(audio::Mixer* mixer) { mixer_ = mixer; }
    void setEnabled(bool sound, bool haptics) {
        sound_ = sound;
        haptics_ = haptics;
    }
    // After each tick; `time` is the view's clock in seconds.
    void onTick(const sim::World& w, float time);
    void star(int k);  // a star lands on the result screen (0..2)
    void uiTap();
    void setTide(i64 tide, bool hushed = false);  // HUSH: the music's pad alone

    // Haptic requests since the last call (the platform plays them).
    int takeHaptics(int* out, int max);

private:
    void play(int sound, float gain, float pan, float pitch = 1.0f, float delay = 0.0f);
    void haptic(int kind, float time);

    audio::Mixer* mixer_ = nullptr;
    bool sound_ = true, haptics_ = true;
    float gateNoteAt_[sim::kMaxGates] = {};
    float lastTick_ = -1, lastThump_ = -1, lastChip_ = -1, lastGateHaptic_ = -1, lastPulseHaptic_ = -1, lastTink_ = -1;
    double hapticTimes_[12] = {};  // wall clock, seconds
    int hapticHead_ = 0;
    int pending_[16] = {};
    int pendingCount_ = 0;
    int shots_ = 0;
    u32 rng_ = 12345;
};

int ladderNote(const sim::Gate& g);  // GAME_DESIGN 12: 2 x row + (k - 2), clamped to the scale

}  // namespace gt

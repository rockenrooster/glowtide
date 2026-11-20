#include "game/feedback.h"

#include <cmath>

#include "core/clock.h"

namespace gt {

using namespace audio;

int ladderNote(const sim::Gate& g) { return clampv(2 * int(g.row) + (int(g.k) - 2), 0, 10); }

void Feedback::play(int sound, float gain, float pan, float pitch, float delay) {
    if (!sound_ || !mixer_) return;
    mixer_->play(sound, gain, pan, pitch, delay);
}

void Feedback::haptic(int kind, float) {
    if (!haptics_) return;
    // At most twelve a second of wall-clock time (GAME_DESIGN 13), slow motion included: the oldest of the last
    // twelve must be a second old (1.05 s: the pulses go out with the next frame, a few ms later).
    const double now = nowSeconds();
    if (hapticTimes_[hapticHead_] != 0 && now - hapticTimes_[hapticHead_] < 1.05) return;
    hapticTimes_[hapticHead_] = now;
    hapticHead_ = (hapticHead_ + 1) % 12;
    if (pendingCount_ < 16) pending_[pendingCount_++] = kind;
}

int Feedback::takeHaptics(int* out, int max) {
    const int n = minv(max, pendingCount_);
    for (int i = 0; i < n; ++i) out[i] = pending_[i];
    pendingCount_ = 0;
    return n;
}

void Feedback::star(int k) { play(kSndStar0 + clampv(k, 0, 2), 0.7f, 0); }
void Feedback::uiTap() { play(kSndUiTap, 0.5f, 0); }

void Feedback::setTide(i64 tide) {
    if (!mixer_) return;
    const float x = tide > 1 ? float(std::log10(double(tide))) / 4.0f : 0.0f;
    mixer_->setIntensity(clampv(x, 0.0f, 1.0f));
}

void Feedback::onTick(const sim::World& w, float t) {
    const float hs = 1.0f / sim::kHW;
    auto rnd = [this]() {
        rng_ = rng_ * 1664525u + 1013904223u;
        return float(rng_ >> 8) / 16777216.0f;
    };
    for (int e = 0; e < w.eventCount; ++e) {
        const sim::Event& ev = w.events[e];
        const float pan = clampv(ev.x * hs, -1.0f, 1.0f) * 0.7f;
        switch (ev.type) {
            case sim::kEvFire:
                if (++shots_ % 2 == 0) play(kSndPip, 0.22f, pan, rnd() < 0.5f ? 1.0f : 1.0595f);
                break;
            case sim::kEvGateMul: {
                const int g = ev.a;
                if (t - gateNoteAt_[g] >= 0.05f) {  // at most twenty a second per gate
                    gateNoteAt_[g] = t;
                    play(kSndLadder0 + ladderNote(w.def.gates[g]), minv(1.0f, 0.35f + 0.1f * float(ev.b)), pan);
                }
                if (t - lastGateHaptic_ >= 0.1f) {
                    lastGateHaptic_ = t;
                    haptic(kHapticLight, t);
                }
                break;
            }
            case sim::kEvGateDiv:
                if (ev.b > 0) play(kSndPff, 0.5f, pan);
                break;
            case sim::kEvClashCount:
                if (t - lastTick_ >= 1.0f / 30.0f) {
                    lastTick_ = t;
                    play(kSndTick, minv(0.8f, 0.25f + 0.05f * float(ev.a)), (rnd() - 0.5f) * 0.6f, 0.8f + rnd() * 0.4f);
                }
                break;
            case sim::kEvBarrierHit:
                if (t - lastChip_ >= 0.1f) {
                    lastChip_ = t;
                    play(kSndChip, 0.5f, pan);
                }
                break;
            case sim::kEvBarrierBreak:
                play(kSndShatterSmall, 0.8f, pan);
                haptic(kHapticMedium, t);
                break;
            case sim::kEvSnuffed:
                play(kSndPff, 0.4f, pan);
                break;
            case sim::kEvBruteDown:
                play(kSndPopLow, 0.8f, pan);
                haptic(kHapticMedium, t);
                break;
            case sim::kEvSpireHit:
                if (t - lastThump_ >= 1.0f / 15.0f) {
                    lastThump_ = t;
                    play(kSndThump, minv(1.0f, 0.4f + 0.01f * float(ev.a)), pan * 0.5f);
                }
                if (t - lastPulseHaptic_ >= 0.125f) {
                    lastPulseHaptic_ = t;
                    haptic(kHapticLight, t);
                }
                break;
            case sim::kEvSpireStage:
                play(kSndCrack, 0.9f, 0);
                haptic(kHapticMedium, t);
                break;
            case sim::kEvCoreHit:
                play(kSndCoreHit, 1.0f, pan);
                haptic(kHapticStrong, t);
                break;
            case sim::kEvTideMark: {
                const int base = ev.a >= 2500 ? 5 : ev.a >= 1000 ? 4 : ev.a >= 500 ? 3 : 2;
                for (int k = 0; k < 5; ++k) play(kSndLadder0 + minv(10, base + k), 0.5f, 0, 1.0f, 0.04f * float(k));
                haptic(kHapticDouble, t);
                break;
            }
            case sim::kEvGeodeHit:
                if (t - lastTink_ >= 1.0f / 15.0f) {  // at most fifteen a second, stepping up the ladder as its health falls
                    lastTink_ = t;
                    const float full = float(maxv<i32>(1, w.def.geodes[ev.a].hp));
                    const int note = clampv(int(10.0f * (1.0f - float(w.geodeHp[ev.a]) / full)), 0, 10);
                    play(kSndTink, 0.4f, pan, midiHz(float(kLadderMidi[note])) / midiHz(float(kLadderMidi[0])));
                }
                break;
            case sim::kEvPlateHit:
                if (t - lastChip_ >= 0.1f) {
                    lastChip_ = t;
                    play(kSndChip, 0.5f, pan);
                }
                break;
            case sim::kEvPlateBreak:
                play(kSndShatterSmall, 0.8f, pan);
                haptic(kHapticMedium, t);
                break;
            case sim::kEvGeodeOpen:
                play(kSndGeodeOpen, 0.8f, pan);
                haptic(kHapticDouble, t);
                break;
            case sim::kEvGeodeLost:
                if (ev.b == 0) play(kSndPff, 0.45f, pan);
                break;
            case sim::kEvBoon: {
                // The boon's motif: three ladder notes, a shape each (lantern rises, quicken hurries, bright leaps, ward is a
                // soft chord).
                static const int notes[4][3] = {{5, 7, 9}, {6, 6, 8}, {3, 6, 10}, {2, 4, 7}};
                static const float gaps[4] = {0.09f, 0.05f, 0.12f, 0.0f};
                if (ev.a >= 0 && ev.a < 4) {
                    for (int k = 0; k < 3; ++k)
                        play(kSndLadder0 + notes[ev.a][k], ev.a == 3 ? 0.4f : 0.55f, 0, 1.0f, 0.18f + gaps[ev.a] * float(k));
                    if (ev.a == 3) haptic(kHapticMedium, t);
                }
                break;
            }
            case sim::kEvWin:
                play(kSndShatterBig, 1.0f, 0);
                if (w.def.boss) play(kSndShatterBig, 0.9f, 0, 0.94f, 0.12f);
                play(kSndWinChord, 0.9f, 0, 1.0f, 0.35f);
                if (mixer_ && sound_) mixer_->duck(1.5f);  // nothing to duck under with SOUND off
                haptic(kHapticWin, t);
                break;
            case sim::kEvLose:
                play(kSndLose, 0.9f, 0);
                if (mixer_ && sound_) mixer_->duck(1.5f);  // nothing to duck under with SOUND off
                haptic(kHapticLose, t);
                break;
            default:
                break;
        }
    }
    setTide(w.tide);
}

}  // namespace gt

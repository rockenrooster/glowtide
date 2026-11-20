#pragma once
// The camera (TECH_SPEC 5.2): a fixed rig scaled to fit the lane into the safe area, plus a follow and shake.

#include "core/mathx.h"
#include "game/input.h"

namespace gt {

struct Camera {
    // Reference rig: eye (0, 24, -11), target (0, 0, 14.4), 56 degrees vertical field of view.
    V3 eyeRef{0, 24, -11}, targetRef{0, 0, 14.4f};
    float fovY = 56.0f * kPi / 180.0f;
    float nearZ = 2.0f, farZ = 160.0f;

    int w = 1, h = 1;
    float scale = 1;     // the fit's s
    float len = 36;
    V3 eye, target;      // fitted, before follow and shake
    V3 eyeNow, targetNow;
    V3 fwd, right, up;
    M4 viewProj;

    // Fits the rig for a lane of length `len` into a frame; returns false if no s in [0.6, 1.4] satisfies the rules.
    bool fit(float laneLen, int width, int height, const SafeArea& safe, float dpScale);
    // Final placement: lateral follow (metres) and a shake offset.
    void place(float followX, V3 shake);
    // From straight above (a hair behind, so 'up' stays defined), the whole lane in the frame: level thumbnails.
    void topDown(float laneLen);
    // World -> frame pixels (top-left origin); returns the view depth (negative behind the camera).
    float project(V3 p, float& sx, float& sy) const;
    float pixelsPerMetre(V3 p) const;  // at that point, horizontally

private:
    void build(V3 e, V3 t);
};

}  // namespace gt

#pragma once
// The std140 "Frame" uniform block (shaders/prelude.glsl), field for field.

#include "core/mathx.h"

namespace gt {

struct FrameUniforms {
    float viewProj[16];
    V4 camRight, camUp, camPos;
    V4 time;      // x seconds, y alpha, z cheer, w seconds wrapped to 60
    V4 grade;     // x brighten, y desaturate, z fade, w vignette
    V4 screen;    // w, h, 1/w, 1/h
    V4 lane;      // half-width, length, u scale, v scale
    V4 fog;
    V4 backdrop[4];
    V4 laneFar, laneNear, rail, brighten;
    V4 wisp[3];
    V4 gloom[4];
    V4 good, bad;
    V4 spire;
    V4 params, params2;
    V4 spireScreen;
};
static_assert(sizeof(FrameUniforms) == 64 + 16 * 29, "std140 layout");

}  // namespace gt

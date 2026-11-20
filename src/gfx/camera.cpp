#include "gfx/camera.h"

#include <cmath>

#include "sim/level.h"

namespace gt {

void Camera::build(V3 e, V3 t) {
    eyeNow = e;
    targetNow = t;
    fwd = normalize(t - e);
    right = normalize(cross(V3(0, 1, 0), fwd));
    up = cross(fwd, right);
    const float aspect = float(w) / float(h);
    const float f = 1.0f / std::tan(fovY * 0.5f);
    // View: rows right, up, fwd (depth positive in front). Projection maps depth near..far to -1..1.
    M4 view;
    const V3 r = right, u = up, d = fwd;
    view.at(0, 0) = r.x; view.at(0, 1) = r.y; view.at(0, 2) = r.z; view.at(0, 3) = -dot(r, e);
    view.at(1, 0) = u.x; view.at(1, 1) = u.y; view.at(1, 2) = u.z; view.at(1, 3) = -dot(u, e);
    view.at(2, 0) = d.x; view.at(2, 1) = d.y; view.at(2, 2) = d.z; view.at(2, 3) = -dot(d, e);
    view.at(3, 0) = 0; view.at(3, 1) = 0; view.at(3, 2) = 0; view.at(3, 3) = 1;
    M4 proj;
    proj.at(0, 0) = f / aspect;
    proj.at(1, 1) = f;
    proj.at(2, 2) = (farZ + nearZ) / (farZ - nearZ);
    proj.at(2, 3) = -2.0f * farZ * nearZ / (farZ - nearZ);
    proj.at(3, 2) = 1.0f;
    proj.at(3, 3) = 0.0f;
    viewProj = proj * view;
}

float Camera::project(V3 p, float& sx, float& sy) const {
    const V4 c = mul(viewProj, V4(p, 1.0f));
    if (c.w <= 1e-4f) {
        sx = sy = -1e6f;
        return c.w;
    }
    sx = (c.x / c.w * 0.5f + 0.5f) * float(w);
    sy = (0.5f - c.y / c.w * 0.5f) * float(h);
    return c.w;
}

float Camera::pixelsPerMetre(V3 p) const {
    float x0, y0, x1, y1;
    project(p, x0, y0);
    project(p + V3(1, 0, 0), x1, y1);
    return std::fabs(x1 - x0);
}

bool Camera::fit(float laneLen, int width, int height, const SafeArea& safe, float dp) {
    w = width;
    h = height;
    len = laneLen;
    const float topLine = safe.top + 64.0f * dp;
    const float bottomLine = float(h) - safe.bottom - 24.0f * dp;
    const float leftLine = safe.left, rightLine = float(w) - safe.right;
    const V3 tgt(0, 0, 0.40f * laneLen);
    auto ok = [&](float s) {
        eye = tgt + (eyeRef - targetRef) * s;
        target = tgt;
        build(eye, target);
        float x, y;
        project(V3(0, 0, -1.5f), x, y);  // the launcher's base
        if (y > bottomLine) return false;
        project(V3(0, 6.3f, laneLen + 1.4f), x, y);  // the spire's top
        if (y < topLine) return false;
        const float edge = sim::kHW + 0.53f;  // the lane's edge at the launcher line, with the follow
        project(V3(-edge, 0, 0), x, y);
        if (x < leftLine) return false;
        project(V3(edge, 0, 0), x, y);
        if (x > rightLine) return false;
        return true;
    };
    float lo = 0.6f, hi = 1.4f;
    const bool any = ok(hi);
    if (!any) {
        scale = hi;
        ok(hi);
        return false;
    }
    if (ok(lo)) {
        scale = lo;
        return true;
    }
    for (int i = 0; i < 24; ++i) {
        const float mid = (lo + hi) * 0.5f;
        if (ok(mid))
            hi = mid;
        else
            lo = mid;
    }
    scale = hi;
    ok(hi);
    return true;
}

void Camera::place(float followX, V3 shake) { build(eye + V3(followX, 0, 0) + shake, target + V3(followX, 0, 0) + shake * 0.5f); }

}  // namespace gt

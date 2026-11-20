// Units (TECH_SPEC 5.4): one camera-facing quad each, placed between ticks, hopping, popping in at birth.
layout(location = 0) in vec2 aPos;    // x, z
layout(location = 1) in vec2 aStep;   // last tick's displacement x 4096
layout(location = 2) in vec4 aInfo;   // flags, seed, age (ticks), tier
out vec2 vLocal;
flat out int vFlags;
out float vSeed;
out float vPx;      // pixels per unit radius (edge antialiasing)
out float vFade;    // a side door's gloom fades in over its first second
void main() {
    int flags = int(aInfo.x + 0.5);
    float seed = aInfo.y / 255.0;
    int tier = int(aInfo.w + 0.5);
    bool gloom = (flags & 1) != 0;
    float r = unitRadius(gloom, tier);
    float age = (aInfo.z + uTime.y) / 60.0;
    // A clone pops 0.3 -> 1.15 -> 1 in 180 ms; a fired wisp (flag 4) grows 0.5 -> 1 in 90 ms (GAME_DESIGN 10).
    float pop = (flags & 4) != 0 ? (age < 0.09 ? mix(0.5, 1.0, age / 0.09) : 1.0)
                                 : (age < 0.09 ? mix(0.3, 1.15, age / 0.09) : (age < 0.18 ? mix(1.15, 1.0, (age - 0.09) / 0.09) : 1.0));
    float t = uTime.x;
    float hop;
    float sy = 1.0;
    if (!gloom) {
        float ph = fract(t * uParams2.x + seed);
        float s = sin(ph * 3.14159);
        hop = uParams.w * s * (r / 0.26) * 0.7 + max(uTime.z, 0.0) * 0.35 * abs(sin(t * 7.0 + seed * 6.0));
        sy = 1.0 - 0.16 * (1.0 - smoothstep(0.0, 0.3, s));
    } else {
        hop = 0.06 * sin(t * 4.0 + seed * 6.28) + max(-uTime.z, 0.0) * 0.3 * abs(sin(t * 6.0 + seed * 6.0));  // the lose
    }
    vec2 xz = aPos - aStep * (1.0 / 4096.0) * (1.0 - uTime.y);
    float rr = r * pop;
    vec3 centre = vec3(xz.x, rr * 1.02 + hop + (gloom ? 0.0 : muzzleLift(xz.y)), xz.y);
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    // Room for a runner's streak above it and a comet's tail below it.
    float top = gloom ? ((flags & 16) != 0 ? 2.6 : 1.7) : 2.15;
    float bottom = (flags & 8) != 0 ? -4.0 : -1.25;
    vec2 local = vec2(mix(-1.3, 1.3, c.x), mix(bottom, top, c.y));
    float sx = 1.0 + (1.0 - sy) * 0.6;
    vec3 wp = centre + uCamRight.xyz * (local.x * rr * sx) + uCamUp.xyz * (local.y * rr * sy);
    gl_Position = uViewProj * vec4(wp, 1.0);
    vLocal = local;
    vFlags = flags;
    vFade = (flags & 32) != 0 ? clamp(age, 0.15, 1.0) : 1.0;
    vSeed = seed;
    // Projected size of one radius in pixels (for the edge width).
    vec4 a = uViewProj * vec4(centre, 1.0);
    vec4 b = uViewProj * vec4(centre + uCamRight.xyz * rr, 1.0);
    vPx = max(0.5, abs(b.x / b.w - a.x / a.w) * 0.5 * uScreen.x);
}

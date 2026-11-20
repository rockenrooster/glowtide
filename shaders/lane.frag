// The causeway's top: a gradient far to near, faint seams and forward chevrons, lit by the crowd (TECH_SPEC 5.5).
in highp vec2 vXZ;
uniform sampler2D uLight;
layout(location = 0) out vec4 oColor;
void main() {
    highp float x = vXZ.x;
    highp float z = vXZ.y;
    float t = clamp((z + 2.4) / (uLane.y + 4.1), 0.0, 1.0);
    vec3 base = mix(uLaneNear.rgb, uLaneFar.rgb, t);
    // Seams: every 2 m along the lane and across it.
    highp float sz = (0.5 - abs(fract(z * 0.5) - 0.5)) * 2.0;
    highp float sx = (0.5 - abs(fract(x * 0.5) - 0.5)) * 2.0;
    float wz = 0.03 + fwidth(z);
    float wx = 0.03 + fwidth(x);
    float seam = max(1.0 - smoothstep(0.0, wz, sz), 1.0 - smoothstep(0.0, wx, sx));
    // Chevrons every 4 m, pointing up the lane.
    highp float cz = mod(z - 3.0, 4.0);
    float ax = abs(x);
    float vline = abs(cz - (0.9 - ax));
    float chev = (ax < 0.9 && z > 3.0 && z < uLane.y - 2.0) ? 1.0 - smoothstep(0.05, 0.12 + fwidth(cz), vline) : 0.0;
    vec3 c = base + vec3(seam * 0.055 + chev * 0.045);
    highp vec2 luv = vec2((x + uLane.x + 1.0) * uLane.z, (z + 3.0) * uLane.w);
    vec3 L = texture(uLight, luv).rgb;
    c = c * (0.9 + L * 1.6) + L * uParams2.z;
    oColor = vec4(grade(c) + dither(), 1.0);
}

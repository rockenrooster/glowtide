// The depth below the causeway (GAME_DESIGN 9.3): four-colour gradient, warm glow behind the spire, mist, motes.
in highp vec2 vUV;
layout(location = 0) out vec4 oColor;
highp float hash12(highp vec2 p) {
    highp vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
void main() {
    float y = 1.0 - vUV.y;  // 0 at the top
    vec3 c;
    if (y < 0.26) c = mix(uBackdrop[0].rgb, uBackdrop[1].rgb, y / 0.26);
    else if (y < 0.62) c = mix(uBackdrop[1].rgb, uBackdrop[2].rgb, (y - 0.26) / 0.36);
    else c = mix(uBackdrop[2].rgb, uBackdrop[3].rgb, (y - 0.62) / 0.38);
    // Warm glow behind the spire (screen blend).
    highp vec2 d = (vUV - uSpireScreen.xy) * vec2(uScreen.x / uScreen.y, 1.0);
    float g = exp(-dot(d, d) / (uSpireScreen.z * uSpireScreen.z));
    c = c + (vec3(1.0) - c) * vec3(0.59, 0.27, 0.47) * g * 0.7;
    // Mist: a few soft bands that drift.
    highp float t = uTime.w;
    float m = 0.0;
    for (int i = 0; i < 4; ++i) {
        float fi = float(i);
        highp float yy = fract(0.18 + fi * 0.27 + t * 0.0015 * (fi + 1.0));
        float band = exp(-pow((y - yy) * 16.0, 2.0));
        float wob = 0.5 + 0.5 * sin(vUV.x * (5.0 + fi) + fi * 2.1 + t * 0.07);
        m += band * wob;
    }
    c = mix(c, vec3(0.59, 0.48, 0.84), m * 0.07);
    // Motes: at most one per 46 px cell, drifting up, twinkling.
    highp vec2 px = gl_FragCoord.xy + vec2(0.0, -t * 9.0 * (uScreen.y / 2400.0));
    highp float cellSize = 46.0 * (uScreen.y / 2400.0);
    highp vec2 cell = floor(px / cellSize);
    float h = hash12(cell);
    if (h < 0.2) {
        highp vec2 centre = (cell + vec2(hash12(cell + 7.1), hash12(cell + 3.7)) * 0.8 + 0.1) * cellSize;
        float r = (1.5 + 5.0 * hash12(cell + 9.3)) * (uScreen.y / 2400.0);
        float dd = length(px - centre);
        float a = (1.0 - smoothstep(r * 0.4, r, dd)) * (0.15 + 0.32 * (0.5 + 0.5 * sin(t * (1.0 + h * 9.0) + h * 40.0)));
        float k = hash12(cell + 1.3);
        vec3 mc = k < 0.4 ? vec3(1.0, 0.84, 0.59) : (k < 0.7 ? vec3(0.65, 0.88, 1.0) : vec3(1.0, 0.65, 0.88));
        c = mix(c, mc, a);
    }
    oColor = vec4(grade(c) + dither(), 1.0);
}

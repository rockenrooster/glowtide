// The depth below the causeway (GAME_DESIGN 9.3): four-colour gradient, warm glow behind the spire, mist, motes; and the
// world's sky feature (VARIETY_DESIGN 15) by uBackdrop[0].w: 1 aurora ribbons, 2 a low moon, 3 falling motes, 4 a far
// second spire. Only where nothing else is drawn, so its cost is the top of the screen and the sides.
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
        float dy = (y - yy) * 16.0;  // a square, not pow(): pow of a negative is undefined, and dearer
        if (abs(dy) > 3.0) continue;  // exp(-9): nothing to see, so most pixels skip the band
        float band = exp(-dy * dy);
        float wob = 0.5 + 0.5 * sin(vUV.x * (5.0 + fi) + fi * 2.1 + t * 0.07);
        m += band * wob;
    }
    c = mix(c, vec3(0.59, 0.48, 0.84), m * 0.07);
    float feature = uBackdrop[0].w;
    if (feature > 0.5 && feature < 1.5) {
        // Aurora: two soft ribbons waving across the upper sky in the rail's colour (nothing of them below y 0.36).
        for (int i = 0; i < 2 && y < 0.36; ++i) {
            float fi = float(i);
            float yy = 0.09 + 0.08 * fi + 0.03 * sin(vUV.x * (4.0 + 2.5 * fi) + t * (0.17 + 0.06 * fi) + fi * 1.7);
            float dy = (y - yy) * (30.0 - 6.0 * fi);
            if (abs(dy) > 3.0) continue;
            float band = exp(-dy * dy) * (0.55 + 0.45 * sin(vUV.x * 13.0 - t * 0.35 + fi * 2.0));
            c += uRail.rgb * band * 0.14;
        }
    } else if (feature > 1.5 && feature < 2.5) {
        // A low moon over the left of the sky, a pale disc with a halo in the dawn's colour.
        highp vec2 md = (vUV - vec2(0.2, 0.86)) * vec2(uScreen.x / uScreen.y, 1.0);
        float r = length(md);
        float disc = 1.0 - smoothstep(0.040, 0.044, r);
        c = mix(c, mix(uBrighten.rgb, vec3(1.0), 0.5), disc * 0.8) + uBrighten.rgb * exp(-r * r / 0.010) * 0.10;
    } else if (feature > 2.5 && feature < 3.5) {
        // Falling motes: small cold flecks drifting down, at most one per 30 px cell.
        highp vec2 fp = gl_FragCoord.xy + vec2(t * 3.0, t * 14.0) * (uScreen.y / 2400.0);
        highp float fs = 30.0 * (uScreen.y / 2400.0);
        highp vec2 fc = floor(fp / fs);
        float fh = hash12(fc + 11.0);
        if (fh < 0.16) {
            highp vec2 fcen = (fc + vec2(hash12(fc + 5.3), hash12(fc + 2.9)) * 0.8 + 0.1) * fs;
            float fr = (1.0 + 2.0 * hash12(fc + 8.1)) * (uScreen.y / 2400.0);
            c = mix(c, mix(uRail.rgb, vec3(1.0), 0.6), (1.0 - smoothstep(fr * 0.3, fr, length(fp - fcen))) * 0.35);
        }
    } else if (feature > 3.5) {
        // A far second spire: a dark needle on the horizon to the left, its tip lit.
        float h = (vUV.y - 0.66) / 0.22;
        float w = 0.022 * (1.0 - h) * (uScreen.y / uScreen.x) * 0.5;
        if (h > 0.0 && h < 1.0 && abs(vUV.x - 0.16) < w) c = mix(c, uBackdrop[3].rgb, 0.75);
        highp vec2 td = (vUV - vec2(0.16, 0.88)) * vec2(uScreen.x / uScreen.y, 1.0);
        c += uSpire.rgb * exp(-dot(td, td) / 0.00012) * 0.35;
    }
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

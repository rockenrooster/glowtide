in highp vec2 vUV;
flat in float vKind;
flat in float vGhost;
in float vFlash;
in highp float vHeightPx;
uniform float uGlowMode;  // 1: only the bright edges, for the glow buffer
layout(location = 0) out vec4 oColor;
void main() {
    vec3 col = vKind < 0.5 ? uGood.rgb : (vKind < 1.5 ? uBad.rgb : vec3(1.0, 0.78, 0.3));  // 2: GOLDEN GATE
    highp float t = uTime.w;
    float shimmer = 0.5 + 0.5 * sin(vUV.x * 21.0 + vUV.y * 3.0 - t * 1.7) * sin(vUV.x * 7.0 + t * 0.9);
    float fill = 0.17 + 0.07 * shimmer + (vUV.y < 0.24 ? 0.07 : 0.0);
    // Bright top and bottom bars, 0.1 m thick, with a pixel of antialiasing.
    float bar = 0.10 / 2.88;
    float px = 1.0 / vHeightPx;
    float edge = max(1.0 - smoothstep(bar, bar + px, vUV.y), smoothstep(1.0 - bar - px, 1.0 - bar, vUV.y));
    float flash = vFlash;
    // A hopper's ghost frame: the edges faint, almost no sheet.
    float ghost = vGhost > 0.5 ? 1.0 : 0.0;
    edge *= 1.0 - 0.6 * ghost;
    fill *= 1.0 - 0.8 * ghost;
    if (uGlowMode > 0.5) {
        float a = edge * (0.55 + flash * 0.45) + fill * 0.05;
        oColor = vec4(col * a, 0.0);
        return;
    }
    // A pass brightens the panel by 40% and decays in 200 ms (GAME_DESIGN 10).
    vec3 c = min(col * (1.0 + 0.4 * flash), vec3(1.0));
    float a = clamp(fill * (1.0 + 0.5 * flash), 0.0, 0.6);
    vec3 outc = mix(c * a, c, edge);
    float outa = mix(a, 1.0, edge);
    oColor = vec4(grade(outc / max(outa, 0.001)) * outa, outa);
}

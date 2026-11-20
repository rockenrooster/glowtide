// A wisp (teardrop of light, seen from behind) or a gloom (round, two horn bumps, glowing eyes): signed distance
// fields in radius units, three-tone shading, antialiased edges; premultiplied output (TECH_SPEC 5.4).
in vec2 vLocal;
flat in int vFlags;
in float vSeed;
in float vPx;
layout(location = 0) out vec4 oColor;

float sdUnevenCapsule(vec2 p, float r1, float r2, float h) {
    p.x = abs(p.x);
    float b = (r1 - r2) / h;
    float a = sqrt(1.0 - b * b);
    float k = dot(p, vec2(-b, a));
    if (k < 0.0) return length(p) - r1;
    if (k > a * h) return length(p - vec2(0.0, h)) - r2;
    return dot(p, vec2(a, b)) - r1;
}
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}
float ellipse(vec2 p, vec2 c, vec2 r) {
    vec2 q = (p - c) / r;
    return (length(q) - 1.0) * min(r.x, r.y);
}

void main() {
    vec2 p = vLocal;
    bool gloom = (vFlags & 1) != 0;
    highp float t = uTime.w;
    float d;
    vec3 col;
    float aa = 1.0 / vPx;  // one pixel in radius units
    if (!gloom) {
        // The tip leans and wobbles; the body is a circle joined smoothly to it.
        float lean = (vSeed - 0.5) * 0.7 + 0.18 * sin(t * 6.0 + vSeed * 30.0);
        vec2 q = vec2(p.x - lean * clamp(p.y, 0.0, 2.0) / 1.85, p.y);
        d = sdUnevenCapsule(q, 1.0, 0.12, 1.75);
        float inner = -d;
        bool white = (vFlags & 8) != 0;
        vec3 body = white ? vec3(1.0, 0.96, 0.85) : uWisp[0].rgb;
        vec3 rim = white ? vec3(1.0, 0.8, 0.45) : uWisp[2].rgb;
        col = mix(rim, body, smoothstep(0.0, 0.16, inner));
        float lightD = length((p - vec2(-0.12, 0.18)) / vec2(0.5, 0.48));
        col = mix(col, uWisp[1].rgb, (1.0 - smoothstep(0.7, 1.0, lightD)) * 0.8);
        float core = length((p - vec2(-0.14, 0.26)) / vec2(0.22, 0.21));
        col = mix(col, vec3(1.0, 0.98, 0.9), (1.0 - smoothstep(0.6, 1.0, core)) * 0.9);
        // At the win the wisps turn round: two happy eyes.
        if (uTime.z > 0.01) {
            for (int s = 0; s < 2; ++s) {
                vec2 e = p - vec2(s == 0 ? -0.34 : 0.34, 0.02);
                float arc = abs(length(e - vec2(0.0, -0.1)) - 0.17);
                float m = (e.y > -0.08 ? 1.0 : 0.0) * (1.0 - smoothstep(0.035, 0.075 + aa, arc));
                col = mix(col, vec3(0.45, 0.22, 0.07), m * uTime.z);
            }
        }
    } else {
        // Body, slightly flattened; two horn bumps smoothly joined.
        float body = length(p * vec2(1.0, 1.06)) - 1.0;
        float h1 = sdUnevenCapsule((p - vec2(-0.52, 0.55)) * mat2(0.94, 0.34, -0.34, 0.94), 0.3, 0.1, 0.8);
        float h2 = sdUnevenCapsule((p - vec2(0.52, 0.55)) * mat2(0.94, -0.34, 0.34, 0.94), 0.3, 0.1, 0.8);
        d = smin(body, min(h1, h2), 0.14);
        float inner = -d;
        col = mix(uGloom[2].rgb, uGloom[0].rgb, smoothstep(0.0, 0.16, inner));
        float hl = length((p - vec2(-0.12, 0.42)) / vec2(0.56, 0.34));
        col = mix(col, uGloom[1].rgb, (1.0 - smoothstep(0.6, 1.0, hl)) * 0.8);
        // Eyes: glowing rose, with dark pupils; they blink now and then.
        float blink = step(0.97, fract(t * 0.23 + vSeed * 7.0)) * 0.8;
        for (int s = 0; s < 2; ++s) {
            vec2 c = vec2(s == 0 ? -0.36 : 0.36, -0.1);
            float e = ellipse(p, c, vec2(0.2, 0.27 * (1.0 - blink)));
            col = mix(col, uGloom[3].rgb, 1.0 - smoothstep(-aa, aa, e));
            float pd = ellipse(p, c + vec2(0.0, -0.06), vec2(0.085, 0.12 * (1.0 - blink)));
            col = mix(col, uGloom[2].rgb, (1.0 - smoothstep(-aa, aa, pd)) * 0.9);
        }
        if ((vFlags & 2) != 0) col *= 1.08;  // brutes are a touch brighter
    }
    float fw = clamp(aa * 1.2, 0.02, 0.35);
    float alpha = clamp(0.5 - d / fw, 0.0, 1.0);
    oColor = vec4(grade(col) * alpha, alpha);
}

// Sparks, motes and rings add (alpha 0); puffs and shards cover (alpha > 0). Premultiplied.
in vec2 vLocal;
in vec4 vColor;
flat in float vKind;
in float vLife;
uniform float uGlowMode;
layout(location = 0) out vec4 oColor;
void main() {
    vec2 p = vLocal;
    float a;
    float cover = 0.0;
    if (vKind < 0.5) {         // four-point star
        float d = min(abs(p.x) * 0.25 + abs(p.y), abs(p.y) * 0.25 + abs(p.x));
        a = clamp(1.0 - d * 1.6, 0.0, 1.0);
        a *= a;
    } else if (vKind < 1.5) {  // soft puff
        float d = clamp(1.0 - dot(p, p), 0.0, 1.0);
        a = d * d;
        cover = 0.55;
    } else if (vKind < 2.5) {  // shard: a sharp triangle
        float d = max(abs(p.x) * 1.7 + p.y * 0.9, -p.y);
        a = 1.0 - smoothstep(0.75, 0.9, d);
        cover = 1.0;
    } else if (vKind < 3.5) {  // mote
        float d = clamp(1.0 - dot(p, p), 0.0, 1.0);
        a = d * d * d;
    } else {                   // ring
        float r = length(p);
        a = 1.0 - smoothstep(0.08, 0.16, abs(r - 0.8));
    }
    float alpha = vColor.a * a;
    if (uGlowMode > 0.5) {
        oColor = vec4(vColor.rgb * alpha * (cover > 0.9 ? 0.4 : 1.0), 0.0);
        return;
    }
    oColor = vec4(grade(vColor.rgb) * alpha, alpha * cover);
}

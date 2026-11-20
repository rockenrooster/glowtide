in vec2 vLocal;
in vec4 vColor;
flat in float vShape;
layout(location = 0) out vec4 oColor;
void main() {
    float f;
    if (vShape < 0.5) {
        float d = clamp(1.0 - dot(vLocal, vLocal), 0.0, 1.0);
        f = d * d;
    } else {
        vec2 q = clamp(vec2(1.0) - vLocal * vLocal, 0.0, 1.0);
        f = q.x * q.x * q.y;
    }
    float a = vColor.a * f;
    oColor = vec4(vColor.rgb * a, a);
}

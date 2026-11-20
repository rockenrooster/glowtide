in vec2 vLocal;
in vec4 vColor;
layout(location = 0) out vec4 oColor;
void main() {
    float d = clamp(1.0 - dot(vLocal, vLocal), 0.0, 1.0);
    float a = vColor.a * d * d;
    oColor = vec4(vColor.rgb * a, a);
}

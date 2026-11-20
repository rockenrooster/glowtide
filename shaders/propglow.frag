// Emissive props into the glow buffer: alpha 0, so "over" blending adds them.
in vec3 vColor;
in float vFog;
uniform highp vec4 uTint;
layout(location = 0) out vec4 oColor;
void main() {
    oColor = vec4(vColor * uTint.a * (1.0 - vFog * 0.5), 0.0);
}

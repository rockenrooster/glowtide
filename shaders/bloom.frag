// Bloom: three blur levels with bounded weights, added with the screen blend (ONE, ONE_MINUS_SRC_COLOR), which
// brightens without clipping (TECH_SPEC 5.3, 5.9).
in highp vec2 vUV;
uniform sampler2D uSrc;   // quarter resolution
uniform sampler2D uSrc8;  // eighth
uniform sampler2D uSrc16; // sixteenth
layout(location = 0) out vec4 oColor;
void main() {
    vec3 g = texture(uSrc, vUV).rgb * 0.42 + texture(uSrc8, vUV).rgb * 0.36 + texture(uSrc16, vUV).rgb * 0.3;
    g *= uParams.x;
    float l = dot(g, vec3(0.299, 0.587, 0.114));
    g = mix(g, vec3(l), uGrade.y) * uGrade.z;
    oColor = vec4(clamp(g, 0.0, 1.0), 0.0);
}

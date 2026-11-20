// The glow chain (TECH_SPEC 5.9): a 4-tap box at half-texel offsets (16 texels with bilinear filtering).
in highp vec2 vUV;
uniform sampler2D uSrc;
uniform highp vec2 uTexel;  // source texel size
uniform float uGain;
layout(location = 0) out vec4 oColor;
void main() {
    vec4 s = texture(uSrc, vUV + uTexel * vec2(-1.0, -1.0));
    s += texture(uSrc, vUV + uTexel * vec2(1.0, -1.0));
    s += texture(uSrc, vUV + uTexel * vec2(-1.0, 1.0));
    s += texture(uSrc, vUV + uTexel * vec2(1.0, 1.0));
    oColor = s * (0.25 * uGain);
}

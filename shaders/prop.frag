in vec3 vColor;
in float vFog;
layout(location = 0) out vec4 oColor;
void main() {
    vec3 c = mix(vColor, uFog.rgb, vFog);
    oColor = vec4(grade(c) + dither(), 1.0);
}

// The crowd lights the ground: one soft disc a unit in the lane-space light map (TECH_SPEC 5.5).
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aStep;
layout(location = 2) in vec4 aInfo;
out vec2 vLocal;
out vec4 vColor;
flat out float vShape;
void main() {
    int flags = int(aInfo.x + 0.5);
    float tier = aInfo.w;
    bool gloom = (flags & 1) != 0;
    float r = uParams2.y + 0.3 * tier;
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    vec2 xz = aPos - aStep * (1.0 / 4096.0) * (1.0 - uTime.y) + c * r;
    vec2 uv = vec2((xz.x + uLane.x + 1.0) * uLane.z, (xz.y + 3.0) * uLane.w);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    vLocal = c;
    vColor = vec4(gloom ? vec3(0.59, 0.24, 1.0) : uWisp[0].rgb, uParams.z * (gloom ? 0.9 : 1.0));
    vShape = 0.0;
}

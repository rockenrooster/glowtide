// A soft disc per unit in the quarter-resolution glow buffer.
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aStep;
layout(location = 2) in vec4 aInfo;
out vec2 vLocal;
out vec4 vColor;
void main() {
    int flags = int(aInfo.x + 0.5);
    int tier = int(aInfo.w + 0.5);
    bool gloom = (flags & 1) != 0;
    float r = unitRadius(gloom, tier) * uParams2.w;
    vec2 xz = aPos - aStep * (1.0 / 4096.0) * (1.0 - uTime.y);
    vec3 centre = vec3(xz.x, r * 0.55 + (gloom ? 0.0 : muzzleLift(xz.y)), xz.y);
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    vec3 wp = centre + uCamRight.xyz * (c.x * r) + uCamUp.xyz * (c.y * r);
    gl_Position = uViewProj * vec4(wp, 1.0);
    vLocal = c;
    vec3 col = gloom ? vec3(0.5, 0.2, 0.9) : vec3(0.95, 0.52, 0.12);
    if ((flags & 8) != 0) col = vec3(1.0, 0.9, 0.7);
    vColor = vec4(col, uParams.y * (gloom ? 0.55 : 1.0));
}

// Slow lights in the light map: bars under gates, pools, rail strips, flashes.
layout(location = 0) in vec4 aRect;   // x, z, half size x, half size z
layout(location = 1) in vec4 aColor;  // rgb, alpha
layout(location = 2) in vec4 aShape;  // x: 0 disc, 1 box
out vec2 vLocal;
out vec4 vColor;
flat out float vShape;
void main() {
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    vec2 xz = aRect.xy + c * aRect.zw;
    vec2 uv = vec2((xz.x + uLane.x + 1.0) * uLane.z, (xz.y + 3.0) * uLane.w);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    vLocal = c;
    vColor = aColor;
    vShape = aShape.x;
}

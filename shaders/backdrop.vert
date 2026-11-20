// A full-screen triangle at the far plane (depth 1): it shades only pixels nothing else covered.
out highp vec2 vUV;
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 1.0, 1.0);
}

// Gate panels (GAME_DESIGN 9.3): a translucent sheet between the posts, drawn in the far-to-near unit stream.
layout(location = 0) in vec4 aGate;   // x0, x1, z, flash
layout(location = 1) in vec4 aKind;   // x: 0 good, 1 bad; y: sliding arrows; z: punch; w: 1 a hopper's ghost frame
out highp vec2 vUV;
flat out float vKind;
flat out float vGhost;
out float vFlash;
out highp float vHeightPx;
void main() {
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    vec3 wp = vec3(mix(aGate.x, aGate.y, c.x), mix(0.12, 3.0, c.y), aGate.z);
    gl_Position = uViewProj * vec4(wp, 1.0);
    vUV = c;
    vKind = aKind.x;
    vGhost = aKind.w;
    vFlash = aGate.w;
    vec4 lo = uViewProj * vec4(aGate.x, 0.12, aGate.z, 1.0);
    vec4 hi = uViewProj * vec4(aGate.x, 3.0, aGate.z, 1.0);
    vHeightPx = max(1.0, abs(hi.y / hi.w - lo.y / lo.w) * 0.5 * uScreen.y);
}

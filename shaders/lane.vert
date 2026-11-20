layout(location = 0) in vec2 aXZ;
out highp vec2 vXZ;
void main() {
    vXZ = aXZ;
    gl_Position = uViewProj * vec4(aXZ.x, 0.0, aXZ.y, 1.0);
}

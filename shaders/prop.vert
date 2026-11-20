// Props: generated meshes with baked vertex colours (TECH_SPEC 5.6). Alpha 1 marks emissive vertices (unlit).
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uModel;
uniform vec4 uTint;          // rgb multiplies the colour; a = gain in the glow pass
uniform sampler2D uLight;
out vec3 vColor;
out float vFog;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    gl_Position = uViewProj * wp;
    vec3 c = aColor.rgb * uTint.rgb;
    float emit = aColor.a;
    // The crowd's light at the prop's foot, fading with height (only over the lane).
    vec2 luv = vec2((wp.x + uLane.x + 1.0) * uLane.z, (wp.z + 3.0) * uLane.w);
    vec3 L = vec3(0.0);
    if (luv.x > 0.0 && luv.x < 1.0 && luv.y > 0.0 && luv.y < 1.0 && wp.y > -0.6)
        L = textureLod(uLight, luv, 0.0).rgb * (1.0 - clamp(wp.y / 3.5, 0.0, 1.0));
    vColor = mix(c * (1.0 + L * 1.3) + L * 0.18, c, emit);
    float dist = length(wp.xyz - uCamPos.xyz);
    vFog = clamp((dist - 28.0) / 55.0, 0.0, 1.0) * uFog.a * (1.0 - emit * 0.7);
}

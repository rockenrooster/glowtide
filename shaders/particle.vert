// Stateless particles (TECH_SPEC 5.8): written once at spawn; the shader moves them by their age.
layout(location = 0) in vec4 aOrigin;  // x, y, z, birth time
layout(location = 1) in vec4 aVel;     // vx, vy, vz, life
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec4 aMisc;    // size, kind, seed, spin
out vec2 vLocal;
out vec4 vColor;
flat out float vKind;
out float vLife;
void main() {
    float age = uTime.x - aOrigin.w;
    float life = aVel.w;
    vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    if (age < 0.0 || age > life) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        vLocal = c;
        vColor = vec4(0.0);
        vKind = 0.0;
        vLife = 1.0;
        return;
    }
    float kind = aMisc.y;
    float u = age / life;
    vec3 v = aVel.xyz;
    vec3 p;
    float size = aMisc.x;
    if (kind < 0.5) {         // spark: fast, slows, falls a little
        p = aOrigin.xyz + v * (age - 0.6 * age * age) + vec3(0.0, -2.5, 0.0) * age * age;
        size *= 1.0 - u * 0.6;
    } else if (kind < 1.5) {  // puff: drifts, grows, fades
        p = aOrigin.xyz + v * age * (1.0 - u * 0.5);
        size *= 0.6 + u * 0.9;
    } else if (kind < 2.5) {  // shard: thrown, falls
        p = aOrigin.xyz + v * age + vec3(0.0, -7.0, 0.0) * age * age;
        p.y = max(p.y, 0.05);
    } else if (kind < 3.5) {  // mote / confetti: rises slowly, sways
        p = aOrigin.xyz + v * age + vec3(0.35 * sin(age * 3.0 + aMisc.z), 0.0, 0.0);
    } else {                  // ring on the ground: expands
        p = aOrigin.xyz;
        size *= 0.3 + u * 1.0;
    }
    vec3 right = uCamRight.xyz, up = uCamUp.xyz;
    if (kind > 3.5) {
        right = vec3(1.0, 0.0, 0.0);
        up = vec3(0.0, 0.0, 1.0);
    }
    float spin = aMisc.w * age;
    vec2 rc = vec2(c.x * cos(spin) - c.y * sin(spin), c.x * sin(spin) + c.y * cos(spin));
    vec3 wp = p + right * (rc.x * size) + up * (rc.y * size);
    gl_Position = uViewProj * vec4(wp, 1.0);
    vLocal = c;
    float fade = kind > 3.5 ? 1.0 - u : (1.0 - smoothstep(0.55, 1.0, u)) * (kind > 2.5 ? smoothstep(0.0, 0.15, u) : 1.0);
    vColor = vec4(aColor.rgb, aColor.a * fade);
    vKind = kind;
    vLife = u;
}

#include "gfx/renderer.h"

#include <cstddef>
#include <cstring>

#include "core/log.h"
#include "gfx/shader.h"
#include "sim/world.h"

namespace gt {

namespace {

constexpr int kMaxUnits = sim::kMaxWisps + sim::kMaxGlooms;
constexpr int kMaxPanels = 32;
constexpr int kMaxLights = 512;
constexpr int kLightW = 128, kLightH = 512;

GLuint makeBuffer(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    GLuint b = 0;
    glGenBuffers(1, &b);
    glBindBuffer(target, b);
    glBufferData(target, size, data, usage);
    return b;
}

void attrib(GLuint loc, GLint n, GLenum type, GLboolean norm, GLsizei stride, size_t offset, GLuint divisor) {
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, n, type, norm, stride, reinterpret_cast<void*>(offset));
    glVertexAttribDivisor(loc, divisor);
}

void stream(GLuint vbo, GLsizeiptr capacity, const void* data, GLsizeiptr size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, capacity, nullptr, GL_STREAM_DRAW);  // orphan
    if (size > 0) glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

}  // namespace

bool Renderer::init(int w, int h) {
    static const char* lightSampler[] = {"uLight"};
    static const char* atlasSampler[] = {"uAtlas"};
    static const char* srcSampler[] = {"uSrc"};
    static const char* bloomSamplers[] = {"uSrc", "uSrc8", "uSrc16"};
    pProp_ = buildProgram("prop.vert", "prop.frag", lightSampler, 1);
    pPropGlow_ = buildProgram("prop.vert", "propglow.frag", lightSampler, 1);
    pLane_ = buildProgram("lane.vert", "lane.frag", lightSampler, 1);
    pBackdrop_ = buildProgram("backdrop.vert", "backdrop.frag");
    pUnit_ = buildProgram("unit.vert", "unit.frag");
    pLightUnit_ = buildProgram("lightunit.vert", "light.frag");
    pLightQuad_ = buildProgram("lightquad.vert", "light.frag");
    pGlowUnit_ = buildProgram("glowunit.vert", "glowdisc.frag");
    pPanel_ = buildProgram("panel.vert", "panel.frag");
    pParticle_ = buildProgram("particle.vert", "particle.frag");
    pText_ = buildProgram("text.vert", "text.frag", atlasSampler, 1);
    pBlur_ = buildProgram("fullscreen.vert", "blur.frag", srcSampler, 1);
    pBloom_ = buildProgram("fullscreen.vert", "bloom.frag", bloomSamplers, 3);
    for (GLuint p : {pProp_, pPropGlow_, pLane_, pBackdrop_, pUnit_, pLightUnit_, pLightQuad_, pGlowUnit_, pPanel_, pParticle_,
                     pText_, pBlur_, pBloom_})
        if (!p) return false;
    uPropModel_ = glGetUniformLocation(pProp_, "uModel");
    uPropTint_ = glGetUniformLocation(pProp_, "uTint");
    uPropGlowModel_ = glGetUniformLocation(pPropGlow_, "uModel");
    uPropGlowTint_ = glGetUniformLocation(pPropGlow_, "uTint");
    uPanelGlow_ = glGetUniformLocation(pPanel_, "uGlowMode");
    uParticleGlow_ = glGetUniformLocation(pParticle_, "uGlowMode");
    uTextSpread_ = glGetUniformLocation(pText_, "uSpread");
    uTextOpacity_ = glGetUniformLocation(pText_, "uOpacity");
    uBlurTexel_ = glGetUniformLocation(pBlur_, "uTexel");
    uBlurGain_ = glGetUniformLocation(pBlur_, "uGain");

    ubo_ = makeBuffer(GL_UNIFORM_BUFFER, sizeof(FrameUniforms), nullptr, GL_DYNAMIC_DRAW);

    // Units: x, z (float), step (short), info (4 bytes).
    unitCapacity_ = kMaxUnits;
    glGenVertexArrays(1, &vaoUnits_);
    glBindVertexArray(vaoUnits_);
    vboUnits_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(UnitInstance) * kMaxUnits, nullptr, GL_STREAM_DRAW);
    bindUnitAttribs(0);

    glGenVertexArrays(1, &vaoPanels_);
    glBindVertexArray(vaoPanels_);
    vboPanels_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(PanelInstance) * kMaxPanels, nullptr, GL_STREAM_DRAW);
    attrib(0, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), offsetof(PanelInstance, x0), 1);
    attrib(1, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), offsetof(PanelInstance, kind), 1);

    glGenVertexArrays(1, &vaoLights_);
    glBindVertexArray(vaoLights_);
    vboLights_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(LightInstance) * kMaxLights, nullptr, GL_STREAM_DRAW);
    attrib(0, 4, GL_FLOAT, GL_FALSE, sizeof(LightInstance), offsetof(LightInstance, x), 1);
    attrib(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(LightInstance), offsetof(LightInstance, r), 1);
    attrib(2, 4, GL_FLOAT, GL_FALSE, sizeof(LightInstance), offsetof(LightInstance, shape), 1);

    particleCapacity_ = 0;
    glGenVertexArrays(1, &vaoParticles_);
    glBindVertexArray(vaoParticles_);
    vboParticles_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(ParticleInstance) * 4096, nullptr, GL_STREAM_DRAW);
    attrib(0, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), offsetof(ParticleInstance, ox), 1);
    attrib(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), offsetof(ParticleInstance, vx), 1);
    attrib(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ParticleInstance), offsetof(ParticleInstance, r), 1);
    attrib(3, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), offsetof(ParticleInstance, size), 1);

    // Text and UI: per-vertex attributes, a shared quad index buffer.
    u16* quadIdx = new u16[UiBatch::kMaxQuads * 6];
    for (int q = 0; q < UiBatch::kMaxQuads; ++q) {
        const u16 b = u16(q * 4);
        const u16 idx[6] = {b, u16(b + 1), u16(b + 2), u16(b + 2), u16(b + 1), u16(b + 3)};
        std::memcpy(quadIdx + q * 6, idx, sizeof idx);
    }
    glGenVertexArrays(1, &vaoText_);
    glBindVertexArray(vaoText_);
    iboQuads_ = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * UiBatch::kMaxQuads * 6, quadIdx, GL_STATIC_DRAW);
    delete[] quadIdx;
    vboText_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(UiVertex) * UiBatch::kMaxQuads * 4, nullptr, GL_STREAM_DRAW);
    attrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), offsetof(UiVertex, x), 0);
    attrib(1, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), offsetof(UiVertex, u), 0);
    attrib(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UiVertex), offsetof(UiVertex, fill), 0);
    attrib(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UiVertex), offsetof(UiVertex, stroke), 0);
    attrib(4, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), offsetof(UiVertex, p0), 0);
    attrib(5, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), offsetof(UiVertex, p1), 0);

    glGenVertexArrays(1, &vaoLane_);
    glBindVertexArray(vaoLane_);
    vboLane_ = makeBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, nullptr, GL_STATIC_DRAW);
    attrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0, 0);

    glGenVertexArrays(1, &vaoEmpty_);
    glGenVertexArrays(1, &vaoLevel_);
    glGenVertexArrays(1, &vaoProps_);
    glBindVertexArray(0);

    int aw = 0, ah = 0;
    const unsigned char* pixels = fontAtlas(aw, ah);
    glGenTextures(1, &atlas_);
    glBindTexture(GL_TEXTURE_2D, atlas_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, aw, ah, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (!light_.create(kLightW, kLightH, false, true)) return false;
    resize(w, h);
    checkGl("Renderer::init");
    return true;
}

void Renderer::resize(int w, int h) {
    if (w == w_ && h == h_ && glow4_.fbo) return;
    w_ = w;
    h_ = h;
    glow4_.create(maxv(1, w / 4), maxv(1, h / 4), false, true);
    glow8_.create(maxv(1, w / 8), maxv(1, h / 8), false, true);
    glow16_.create(maxv(1, w / 16), maxv(1, h / 16), false, true);
}

void Renderer::destroy() {
    for (GLuint* p : {&pProp_, &pPropGlow_, &pLane_, &pBackdrop_, &pUnit_, &pLightUnit_, &pLightQuad_, &pGlowUnit_, &pPanel_,
                      &pParticle_, &pText_, &pBlur_, &pBloom_}) {
        if (*p) glDeleteProgram(*p);
        *p = 0;
    }
    for (GLuint* b : {&ubo_, &vboUnits_, &vboPanels_, &vboLights_, &vboParticles_, &vboText_, &iboQuads_, &vboLane_, &vboLevel_,
                      &iboLevel_, &iboLevelEmit_, &vboProps_, &iboProps_, &iboPropsEmit_}) {
        if (*b) glDeleteBuffers(1, b);
        *b = 0;
    }
    for (GLuint* v : {&vaoUnits_, &vaoPanels_, &vaoLights_, &vaoParticles_, &vaoText_, &vaoLane_, &vaoEmpty_, &vaoLevel_, &vaoProps_}) {
        if (*v) glDeleteVertexArrays(1, v);
        *v = 0;
    }
    if (atlas_) glDeleteTextures(1, &atlas_);
    atlas_ = 0;
    light_.destroy();
    glow4_.destroy();
    glow8_.destroy();
    glow16_.destroy();
    w_ = h_ = 0;
}

void Renderer::bindUnitAttribs(int first) {
    const size_t base = size_t(first) * sizeof(UnitInstance);
    glBindBuffer(GL_ARRAY_BUFFER, vboUnits_);
    attrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(UnitInstance), base + offsetof(UnitInstance, x), 1);
    attrib(1, 2, GL_SHORT, GL_FALSE, sizeof(UnitInstance), base + offsetof(UnitInstance, dx), 1);
    attrib(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(UnitInstance), base + offsetof(UnitInstance, flags), 1);
}

void Renderer::setLevel(const sim::LevelDef& def, const WorldPalette& wp, const GamePalette& gp) {
    laneLen_ = def.len;
    buildLevelMesh(level_, def, wp, gp);
    buildPropMeshes(props_, wp, gp);
    auto upload = [](GLuint vao, GLuint& vbo, GLuint& ibo, GLuint& iboEmit, const MeshBuilder& mb) {
        glBindVertexArray(vao);
        if (!vbo) glGenBuffers(1, &vbo);
        if (!ibo) glGenBuffers(1, &ibo);
        if (!iboEmit) glGenBuffers(1, &iboEmit);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(mb.v.size() * sizeof(PropVertex)), mb.v.data(), GL_STATIC_DRAW);
        attrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(PropVertex), offsetof(PropVertex, x), 0);
        attrib(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PropVertex), offsetof(PropVertex, r), 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboEmit);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(mb.emit.size() * sizeof(u32)), mb.emit.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(mb.idx.size() * sizeof(u32)), mb.idx.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    };
    upload(vaoLevel_, vboLevel_, iboLevel_, iboLevelEmit_, level_.mb);
    upload(vaoProps_, vboProps_, iboProps_, iboPropsEmit_, props_.mb);
    const float hw = sim::kHW, z0 = -2.4f, z1 = def.len + 1.7f;
    const float lane[8] = {-hw, z0, hw, z0, -hw, z1, hw, z1};
    glBindBuffer(GL_ARRAY_BUFFER, vboLane_);
    glBufferData(GL_ARRAY_BUFFER, sizeof lane, lane, GL_STATIC_DRAW);
    GT_LOG("level mesh: %d vertices, %d triangles (%d emissive)", level_.mb.vertexCount(), int(level_.mb.idx.size() / 3),
           int(level_.mb.emit.size() / 3));
    checkGl("Renderer::setLevel");
}

void Renderer::uploadInstances(const Scene& s) {
    stream(vboUnits_, GLsizeiptr(sizeof(UnitInstance) * size_t(unitCapacity_)), s.units,
           GLsizeiptr(sizeof(UnitInstance) * size_t(minv(s.unitCount, unitCapacity_))));
    stream(vboPanels_, sizeof(PanelInstance) * kMaxPanels, s.panels,
           GLsizeiptr(sizeof(PanelInstance) * size_t(minv(s.panelCount, kMaxPanels))));
    stream(vboLights_, sizeof(LightInstance) * kMaxLights, s.lights,
           GLsizeiptr(sizeof(LightInstance) * size_t(minv(s.lightCount, kMaxLights))));
    if (s.particleCapacity > 0) {
        const int n = minv(s.particleCapacity, 4096);
        stream(vboParticles_, sizeof(ParticleInstance) * 4096, s.particles, GLsizeiptr(sizeof(ParticleInstance) * size_t(n)));
        particleCapacity_ = n;
    }
}

void Renderer::drawUnitsWith(GLuint prog, int first, int count) {
    if (count <= 0) return;
    glUseProgram(prog);
    glBindVertexArray(vaoUnits_);
    bindUnitAttribs(first);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
    ++drawCalls_;
}

void Renderer::drawUnits(int first, int count) { drawUnitsWith(pUnit_, first, count); }

void Renderer::drawProps(const Scene& s, bool glow) {
    const GLuint prog = glow ? pPropGlow_ : pProp_;
    const GLint uModel = glow ? uPropGlowModel_ : uPropModel_;
    const GLint uTint = glow ? uPropGlowTint_ : uPropTint_;
    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, light_.color);
    const M4 ident;
    auto drawRange = [&](Range r) {
        if (r.count <= 0) return;
        glDrawElements(GL_TRIANGLES, r.count, GL_UNSIGNED_INT, reinterpret_cast<void*>(size_t(r.first) * sizeof(u32)));
        ++drawCalls_;
    };
    // The level's static geometry.
    glBindVertexArray(vaoLevel_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glow ? iboLevelEmit_ : iboLevel_);
    glUniformMatrix4fv(uModel, 1, GL_FALSE, ident.m);
    glUniform4f(uTint, 1, 1, 1, s.emissiveGain);
    drawRange(glow ? level_.emissive : level_.opaque);
    if (!s.spireGone) {  // after the win the spire is its shards
        drawRange(glow ? level_.spireEmit : level_.spire);
        for (int k = 0; k < minv(s.spireStage, 3); ++k) drawRange(glow ? level_.cracksEmit[k] : level_.cracks[k]);
        glUniform4f(uTint, 1, 1, 1, s.emissiveGain * s.heartGain);
        drawRange(glow ? level_.heartEmit : level_.heart);
        glUniform4f(uTint, 1, 1, 1, s.emissiveGain);
    }
    for (int b = 0; b < level_.barrierCount; ++b) {
        if (!s.barrierAlive[b]) continue;
        const M4 m = translation(s.barrierOffset[b]);
        glUniformMatrix4fv(uModel, 1, GL_FALSE, m.m);
        drawRange(glow ? level_.barrierEmit[b] : level_.barrier[b]);
    }
    // Props that move.
    glBindVertexArray(vaoProps_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glow ? iboPropsEmit_ : iboProps_);
    for (int i = 0; i < s.propCount; ++i) {
        const PropDraw& d = s.props[i];
        if (glow && d.glow <= 0) continue;
        glUniformMatrix4fv(uModel, 1, GL_FALSE, d.model.m);
        glUniform4f(uTint, d.tint.x, d.tint.y, d.tint.z, d.glow * s.emissiveGain);
        drawRange(glow ? props_.partEmit[d.mesh] : props_.part[d.mesh]);
    }
    glBindVertexArray(0);
}

void Renderer::drawBatch(const UiBatch& b, float opacity) {
    if (b.quads() == 0) return;
    stream(vboText_, GLsizeiptr(sizeof(UiVertex) * UiBatch::kMaxQuads * 4), b.vertices(),
           GLsizeiptr(sizeof(UiVertex) * size_t(b.quads()) * 4));
    glUseProgram(pText_);
    glUniform1f(uTextSpread_, fontSpread());
    glUniform1f(uTextOpacity_, opacity);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas_);
    glBindVertexArray(vaoText_);
    glDrawElements(GL_TRIANGLES, b.quads() * 6, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
    ++drawCalls_;
}

void Renderer::draw(const Scene& s, GLuint fbo, int w, int h) {
    drawCalls_ = 0;
    if (w != w_ || h != h_) resize(w, h);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUniforms), &s.fu, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kFrameBinding, ubo_);
    glDisable(GL_CULL_FACE);

    if (s.world) {
        uploadInstances(s);
        // 1. Light map (lane space): the crowd and the slow lights, blended "over".
        light_.bind();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        drawUnitsWith(pLightUnit_, 0, s.unitCount);
        if (s.lightCount > 0) {
            glUseProgram(pLightQuad_);
            glBindVertexArray(vaoLights_);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, minv(s.lightCount, kMaxLights));
            ++drawCalls_;
        }
        // 2. Glow buffer (quarter resolution) and its blur chain.
        glow4_.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        drawUnitsWith(pGlowUnit_, 0, s.unitCount);
        drawProps(s, true);
        if (s.panelCount > 0) {
            glUseProgram(pPanel_);
            glUniform1f(uPanelGlow_, 1.0f);
            glBindVertexArray(vaoPanels_);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, s.panelCount);
            ++drawCalls_;
        }
        if (particleCapacity_ > 0) {
            glUseProgram(pParticle_);
            glUniform1f(uParticleGlow_, 1.0f);
            glBindVertexArray(vaoParticles_);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particleCapacity_);
            ++drawCalls_;
        }
        glUseProgram(pBlur_);
        glBindVertexArray(vaoEmpty_);
        glActiveTexture(GL_TEXTURE0);
        auto blur = [&](const Target& src, const Target& dst, float gain) {
            dst.bind();
            glBindTexture(GL_TEXTURE_2D, src.color);
            glUniform2f(uBlurTexel_, 1.0f / float(src.w), 1.0f / float(src.h));
            glUniform1f(uBlurGain_, gain);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            ++drawCalls_;
        };
        glDisable(GL_BLEND);
        blur(glow4_, glow8_, 1.0f);
        blur(glow8_, glow16_, 1.0f);
    }

    // 3. The scene at native resolution.
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    glDepthMask(GL_TRUE);
    glClearColor(s.fu.backdrop[3].x, s.fu.backdrop[3].y, s.fu.backdrop[3].z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (s.world) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        glUseProgram(pLane_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, light_.color);
        glBindVertexArray(vaoLane_);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        ++drawCalls_;
        drawProps(s, false);
        // Backdrop: only where nothing was drawn (depth still 1).
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glUseProgram(pBackdrop_);
        glBindVertexArray(vaoEmpty_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ++drawCalls_;
        // Units far to near, cut at each gate row so its panels sit between the units behind and in front.
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        int start = 0;
        for (int c = 0; c < s.cutCount; ++c) {
            const RowCut& cut = s.cuts[c];
            drawUnits(start, cut.unitEnd - start);
            start = cut.unitEnd;
            if (cut.panelCount > 0) {
                glUseProgram(pPanel_);
                glUniform1f(uPanelGlow_, 0.0f);
                glBindVertexArray(vaoPanels_);
                const size_t base = size_t(cut.panelFirst) * sizeof(PanelInstance);
                glBindBuffer(GL_ARRAY_BUFFER, vboPanels_);
                attrib(0, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), base + offsetof(PanelInstance, x0), 1);
                attrib(1, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), base + offsetof(PanelInstance, kind), 1);
                glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, cut.panelCount);
                ++drawCalls_;
            }
        }
        drawUnits(start, s.unitCount - start);
        glBindVertexArray(vaoPanels_);  // restore the panel VAO's base for the glow pass next frame
        glBindBuffer(GL_ARRAY_BUFFER, vboPanels_);
        attrib(0, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), offsetof(PanelInstance, x0), 1);
        attrib(1, 4, GL_FLOAT, GL_FALSE, sizeof(PanelInstance), offsetof(PanelInstance, kind), 1);
        if (particleCapacity_ > 0) {
            glUseProgram(pParticle_);
            glUniform1f(uParticleGlow_, 0.0f);
            glBindVertexArray(vaoParticles_);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particleCapacity_);
            ++drawCalls_;
        }
        glDisable(GL_DEPTH_TEST);
        // 4. Bloom: the screen blend never clips.
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        glUseProgram(pBloom_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glow4_.color);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, glow8_.color);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, glow16_.color);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vaoEmpty_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ++drawCalls_;
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // Labels win: after every unit and after the bloom, so no crowd and no glow ever hides a number.
    if (s.world && s.labels) drawBatch(*s.labels, s.labelOpacity);
    if (s.ui) drawBatch(*s.ui, s.uiOpacity);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

}  // namespace gt

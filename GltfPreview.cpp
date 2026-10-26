// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "GltfPreview.h"
#include "GltfPlacementMath.h"
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPointer>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <tuple>

struct GltfPreview::Resources {
    struct Mesh {
        QOpenGLBuffer buffer;
        QOpenGLVertexArrayObject array;
        std::shared_ptr<QOpenGLTexture> texture;
        QVector3D centre;
    };
    QOpenGLShaderProgram shader;
    QPointer<QOpenGLContext> owner;
    std::vector<std::unique_ptr<Mesh>> meshes;
};

namespace {
// Preserve actual GL state rather than changing GLUU's cached legacy state.
class State {
public:
    explicit State(QOpenGLExtraFunctions *functions) : f(functions) {
        f->glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        f->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
        f->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &array);
        f->glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        f->glActiveTexture(GL_TEXTURE0);
        f->glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        f->glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
        f->glGetIntegerv(GL_FRONT_FACE, &frontFace);
        f->glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
        f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
        f->glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackRowLength);
        f->glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackSkipRows);
        f->glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackSkipPixels);
        f->glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        f->glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
        f->glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
        f->glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
        f->glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
        f->glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEquationRgb);
        f->glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEquationAlpha);
        f->glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        cull = f->glIsEnabled(GL_CULL_FACE);
        blend = f->glIsEnabled(GL_BLEND);
        dither = f->glIsEnabled(GL_DITHER);
        srgb = f->glIsEnabled(GL_FRAMEBUFFER_SRGB);
    }
    ~State() {
        f->glUseProgram(GLuint(program));
        f->glBindVertexArray(GLuint(array));
        f->glBindBuffer(GL_ARRAY_BUFFER, GLuint(buffer));
        f->glBindTexture(GL_TEXTURE_2D, GLuint(texture));
        f->glBindSampler(0, GLuint(sampler));
        f->glActiveTexture(GLenum(activeTexture));
        f->glFrontFace(GLenum(frontFace));
        f->glCullFace(GLenum(cullMode));
        f->glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        f->glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRowLength);
        f->glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackSkipRows);
        f->glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackSkipPixels);
        f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GLuint(unpackBuffer));
        cull ? f->glEnable(GL_CULL_FACE) : f->glDisable(GL_CULL_FACE);
        f->glBlendFuncSeparate(GLenum(blendSrcRgb), GLenum(blendDstRgb),
                              GLenum(blendSrcAlpha), GLenum(blendDstAlpha));
        f->glBlendEquationSeparate(GLenum(blendEquationRgb), GLenum(blendEquationAlpha));
        f->glDepthMask(depthWrite);
        blend ? f->glEnable(GL_BLEND) : f->glDisable(GL_BLEND);
        dither ? f->glEnable(GL_DITHER) : f->glDisable(GL_DITHER);
        srgb ? f->glEnable(GL_FRAMEBUFFER_SRGB) : f->glDisable(GL_FRAMEBUFFER_SRGB);
    }
private:
    QOpenGLExtraFunctions *f;
    GLint program, buffer, array, activeTexture, texture, sampler, frontFace, cullMode, unpackAlignment;
    GLint unpackRowLength, unpackSkipRows, unpackSkipPixels, unpackBuffer;
    GLint blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha;
    GLint blendEquationRgb, blendEquationAlpha;
    GLboolean depthWrite, cull, blend, dither, srgb;
};
}

GltfPreview::GltfPreview() = default;
GltfPreview::~GltfPreview() = default;

bool GltfPreview::load(const QString &path, QString &error) {
    if(!model.load(path, error)) { loadError = error; return false; }
    sourcePath = path;
    ++revision;
    loadError.clear();
    dirty = true; // GPU replacement is deferred until its context is current.
    return true;
}

void GltfPreview::release() {
    auto *context = QOpenGLContext::currentContext();
    if(resources && resources->owner && context != resources->owner) return;
    resources.reset();
    failedContext.clear();
    graphicsError.clear();
}

bool GltfPreview::draw(const QMatrix4x4 &matrix, QString &error,
        int selectionColor, bool mirroredInstance) {
    error.clear();
    auto *context = QOpenGLContext::currentContext();
    if(!context) { error = "No current preview graphics context."; return false; }
    if(failedContext == context && failedRevision == revision && !graphicsError.isEmpty()) {
        error = graphicsError;
        return false;
    }
    if(resources && resources->owner && resources->owner != context) {
        error = "Model graphics belong to another editor context."; return false;
    }
    if(resources && (!resources->owner || dirty)) release();
    auto *f = context->extraFunctions();
    State state(f);
    const auto fail = [&](const QString &message) {
        release();
        // Do not compile/upload again on every route frame after a GPU failure.
        // Reload, explicit cleanup or a new context permits another attempt.
        failedContext = context;
        failedRevision = revision;
        graphicsError = message;
        error = message;
        return false;
    };
    if(!resources) {
        resources = std::make_unique<Resources>();
        resources->owner = context;
        const bool modern = context->format().profile() == QSurfaceFormat::CoreProfile;
        const QByteArray version = modern ? "#version 330\n" : "#version 120\n";
        const QByteArray vertex = version + (modern
                ? "in vec3 position; in vec2 uv; out vec2 texcoord;\n"
                : "attribute vec3 position; attribute vec2 uv; varying vec2 texcoord;\n")
                + "uniform mat4 matrix; void main(){ texcoord=uv; gl_Position=matrix*vec4(position,1.0); }\n";
        const QByteArray fragment = version + (modern
                ? "in vec2 texcoord; out vec4 result;\n"
                : "varying vec2 texcoord;\n")
                + "uniform sampler2D baseTexture; uniform bool textured; uniform vec4 factor;\n"
                  "uniform bool masked; uniform bool blended; uniform float cutoff;\n"
                  "uniform bool picking; uniform vec3 pickingColor;\n"
                  "vec3 toLinear(vec3 c){return mix(c/12.92,pow((c+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),c));}\n"
                  "vec3 toSrgb(vec3 c){return mix(12.92*c,1.055*pow(max(c,vec3(0)),vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));}\n"
                  "void main(){ vec4 c=vec4(1); if(textured) c="
                + (modern ? "texture" : "texture2D")
                + "(baseTexture,texcoord); c.rgb=toLinear(c.rgb)*factor.rgb; c.a*=factor.a;\n"
                  "if((masked && c.a<cutoff) || (blended && c.a<=0.001)) discard; "
                  "c=picking ? vec4(pickingColor,1) : vec4(toSrgb(c.rgb),blended ? c.a : 1.0);\n"
                + (modern ? "result=c;" : "gl_FragColor=c;") + "}\n";
        auto &shader = resources->shader;
        if(!shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex)
                || !shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment)) {
            return fail("Cannot compile glTF preview shader: " + shader.log());
        }
        shader.bindAttributeLocation("position", 0);
        shader.bindAttributeLocation("uv", 1);
        if(!shader.link() || !shader.bind()) {
            return fail("Cannot link glTF preview shader: " + shader.log());
        }
        GLint maxTextureSize = 0;
        f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        std::map<std::tuple<qint64, int, int, int, int>, std::shared_ptr<QOpenGLTexture>> textures;
        for(const auto &primitive : model.primitives) {
            auto mesh = std::make_unique<Resources::Mesh>();
            resources->meshes.push_back(std::move(mesh));
            auto &gpu = *resources->meshes.back();
            // Cache a primitive centre once, rather than walking its vertices each frame.
            if(!primitive.vertices.empty()) {
                QVector3D minimum(primitive.vertices[0], primitive.vertices[1], primitive.vertices[2]);
                QVector3D maximum = minimum;
                for(size_t vertex = 0; vertex + 4 < primitive.vertices.size(); vertex += 5)
                    for(int axis = 0; axis < 3; ++axis) {
                        minimum[axis] = std::min(minimum[axis], primitive.vertices[vertex + axis]);
                        maximum[axis] = std::max(maximum[axis], primitive.vertices[vertex + axis]);
                    }
                gpu.centre = (minimum + maximum) * 0.5f;
            }
            if(!gpu.array.create() || !gpu.buffer.create()) {
                return fail("Cannot allocate glTF preview geometry.");
            }
            gpu.array.bind();
            const int bufferBytes = int(primitive.vertices.size() * sizeof(float));
            if(!gpu.buffer.bind()) {
                return fail("Cannot bind glTF preview geometry.");
            }
            gpu.buffer.allocate(primitive.vertices.data(), bufferBytes);
            if(gpu.buffer.size() != bufferBytes) {
                return fail("GPU could not allocate the glTF preview vertex buffer.");
            }
            shader.enableAttributeArray(0);
            shader.enableAttributeArray(1);
            shader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(float));
            shader.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
            if(!primitive.image.isNull()) {
                const auto key = std::make_tuple(primitive.image.cacheKey(), primitive.wrapS,
                        primitive.wrapT, primitive.minFilter, primitive.magFilter);
                const auto cached = textures.find(key);
                if(cached != textures.end()) { gpu.texture = cached->second; continue; }
                if(primitive.image.width() > maxTextureSize || primitive.image.height() > maxTextureSize) {
                    return fail("Model texture exceeds this GPU's maximum texture size.");
                }
                gpu.texture = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
                if(!gpu.texture->create()) {
                    return fail("Cannot allocate glTF texture.");
                }
                f->glBindTexture(GL_TEXTURE_2D, gpu.texture->textureId());
                f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                f->glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                f->glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                // glTF UV origin matches the first decoded image row; no flip.
                f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, primitive.image.width(),
                        primitive.image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, primitive.image.constBits());
                if(primitive.minFilter >= 9984) f->glGenerateMipmap(GL_TEXTURE_2D);
                int width = primitive.image.width(), height = primitive.image.height(), level = 0;
                do {
                    GLint allocatedWidth = 0, allocatedHeight = 0;
                    f->glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &allocatedWidth);
                    f->glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &allocatedHeight);
                    if(allocatedWidth != width || allocatedHeight != height)
                        return fail("GPU could not allocate the glTF preview texture.");
                    if(primitive.minFilter < 9984 || (width == 1 && height == 1)) break;
                    width = std::max(1, width / 2);
                    height = std::max(1, height / 2);
                    ++level;
                } while(true);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, primitive.minFilter);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, primitive.magFilter);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, primitive.wrapS);
                f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, primitive.wrapT);
                textures.emplace(key, gpu.texture);
            }
        }
        dirty = false;
    }
    auto &shader = resources->shader;
    if(!shader.bind()) return fail("Cannot bind glTF editor shader: " + shader.log());
    shader.setUniformValue("matrix", matrix);
    shader.setUniformValue("baseTexture", 0);
    shader.setUniformValue("picking", selectionColor != 0);
    shader.setUniformValue("pickingColor", GltfPlacementMath::selectionRgb(selectionColor));
    f->glDisable(GL_BLEND);
    f->glBindSampler(0, 0);
    // Shader output is already display-encoded. Picking must retain exact
    // byte IDs, even if another pass left framebuffer conversion/dither on.
    f->glDisable(GL_FRAMEBUFFER_SRGB);
    if(selectionColor != 0) f->glDisable(GL_DITHER);
    f->glFrontFace(mirroredInstance ? GL_CW : GL_CCW);
    f->glCullFace(GL_BACK);
    // Opaque/cutout surfaces establish depth first. Transparent primitives are
    // drawn back to front without depth writes. This remains an editor preview:
    // intersecting transparent surfaces are not sorted at triangle granularity.
    std::vector<size_t> order(model.primitives.size());
    std::vector<float> depths(model.primitives.size());
    for(size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
        const float depth = matrix.map(resources->meshes[i]->centre).z();
        depths[i] = std::isfinite(depth) ? depth : 0.0f;
    }
    if(selectionColor == 0)
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            if(model.primitives[a].blended != model.primitives[b].blended)
                return !model.primitives[a].blended;
            return model.primitives[a].blended && depths[a] > depths[b];
        });
    for(size_t i : order) {
        const auto &primitive = model.primitives[i];
        const auto &mesh = resources->meshes[i];
        primitive.doubleSided ? f->glDisable(GL_CULL_FACE) : f->glEnable(GL_CULL_FACE);
        shader.setUniformValue("factor", primitive.baseColor);
        shader.setUniformValue("textured", bool(mesh->texture));
        shader.setUniformValue("masked", primitive.masked);
        shader.setUniformValue("blended", primitive.blended);
        const bool blendSurface = primitive.blended && selectionColor == 0;
        if(blendSurface) {
            f->glEnable(GL_BLEND);
            f->glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            f->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                  GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        } else f->glDisable(GL_BLEND);
        f->glDepthMask(blendSurface ? GL_FALSE : GL_TRUE);
        shader.setUniformValue("cutoff", primitive.alphaCutoff);
        f->glBindTexture(GL_TEXTURE_2D, mesh->texture ? mesh->texture->textureId() : 0);
        mesh->array.bind();
        f->glDrawArrays(GL_TRIANGLES, 0, GLsizei(primitive.vertices.size() / 5));
    }
    return true;
}

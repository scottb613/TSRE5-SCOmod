// TSRE GenX modifications. Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "GltfPreview.h"
#include <QGuiApplication>
#include <QColor>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QTemporaryDir>
#include <QSurfaceFormat>
#include <array>
#include <iostream>

namespace {
int failures = 0;
void check(bool success, const char *message) {
    if(!success) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
constexpr std::array<GLenum, 13> stateNames{{GL_CURRENT_PROGRAM, GL_ARRAY_BUFFER_BINDING,
    GL_VERTEX_ARRAY_BINDING, GL_ACTIVE_TEXTURE, GL_TEXTURE_BINDING_2D,
    GL_FRONT_FACE, GL_CULL_FACE_MODE, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH,
    GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_PIXEL_UNPACK_BUFFER_BINDING,
    GL_FRAMEBUFFER_BINDING}};
constexpr std::array<GLenum, 4> capabilities{{GL_CULL_FACE, GL_BLEND, GL_DITHER, GL_FRAMEBUFFER_SRGB}};
struct SavedState {
    std::array<GLint, stateNames.size()> values{};
    std::array<GLboolean, capabilities.size()> enabled{};
    GLint texture0 = 0, sampler0 = 0;
    explicit SavedState(QOpenGLExtraFunctions *f) {
        for(size_t i = 0; i < values.size(); ++i) f->glGetIntegerv(stateNames[i], &values[i]);
        for(size_t i = 0; i < enabled.size(); ++i) enabled[i] = f->glIsEnabled(capabilities[i]);
        f->glActiveTexture(GL_TEXTURE0);
        f->glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
        f->glGetIntegerv(GL_SAMPLER_BINDING, &sampler0);
        f->glActiveTexture(GLenum(values[3]));
    }
    bool operator==(const SavedState &other) const {
        return values == other.values && enabled == other.enabled
                && texture0 == other.texture0 && sampler0 == other.sampler0;
    }
};
void run(QSurfaceFormat::OpenGLContextProfile profile) {
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(profile);
    QOpenGLContext context;
    context.setFormat(format);
    check(context.create(), "create test GL context");
    if(!context.isValid()) return;
    check(context.format().profile() == profile, "test requested core/compatibility profile");
    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    check(context.makeCurrent(&surface), "make test context current");
    if(QOpenGLContext::currentContext() != &context) return;
    auto *f = context.extraFunctions();
    QOpenGLFramebufferObjectFormat targetFormat;
    targetFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    targetFormat.setInternalTextureFormat(GL_SRGB8_ALPHA8);
    QOpenGLFramebufferObject target(64, 64, targetFormat);
    check(target.isValid() && target.bind(), "bind sRGB offscreen target");
    f->glViewport(0, 0, 64, 64);
    f->glEnable(GL_DEPTH_TEST);
    f->glClearColor(0, 0, 1, 1);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GltfPreview model;
    GltfPrimitive quad;
    quad.vertices = {-.8f,-.8f,0,0,0, .8f,-.8f,0,1,0, .8f,.8f,0,1,1,
                     -.8f,-.8f,0,0,0, .8f,.8f,0,1,1, -.8f,.8f,0,0,1};
    quad.masked = true;
    quad.image = QImage(4, 4, QImage::Format_RGBA8888);
    quad.image.fill(Qt::white);
    for(int y = 1; y <= 2; ++y) for(int x = 1; x <= 2; ++x)
        quad.image.setPixelColor(x, y, QColor(255, 255, 255, 0));
    model.model.primitives = {quad, quad}; // Exercise shared image/sampler reuse.

    GLuint foreignBuffer = 0, foreignArray = 0, foreignTexture = 0, foreignSampler = 0;
    f->glGenBuffers(1, &foreignBuffer);
    f->glGenVertexArrays(1, &foreignArray);
    f->glGenTextures(1, &foreignTexture);
    f->glGenSamplers(1, &foreignSampler);
    f->glBindSampler(0, foreignSampler);
    f->glBindVertexArray(foreignArray);
    f->glBindBuffer(GL_ARRAY_BUFFER, foreignBuffer);
    f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, foreignBuffer);
    f->glBufferData(GL_PIXEL_UNPACK_BUFFER, 4, nullptr, GL_STATIC_DRAW);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, foreignTexture);
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, foreignTexture);
    f->glFrontFace(GL_CW);
    f->glCullFace(GL_FRONT);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 17);
    f->glPixelStorei(GL_UNPACK_SKIP_ROWS, 2);
    f->glPixelStorei(GL_UNPACK_SKIP_PIXELS, 3);
    f->glEnable(GL_BLEND);
    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_DITHER);
    f->glEnable(GL_FRAMEBUFFER_SRGB);
    const SavedState before(f);
    QString error;
    QMatrix4x4 identity;
    check(model.draw(identity, error, 0x123450), "upload and draw masked picking geometry");
    if(!error.isEmpty()) std::cerr << error.toStdString() << '\n';
    check(SavedState(f) == before, "preserve foreign shader/bindings/unpack/cull/colour state");
    check(f->glGetError() == GL_NO_ERROR, "no upload error with foreign PBO and row settings");
    const QImage pixels = target.toImage();
    check(pixels.pixelColor(11, 32) == QColor(0x12, 0x34, 0x50), "exact selection bytes on sRGB target");
    check(pixels.pixelColor(32, 32) == QColor(Qt::blue), "masked hole keeps background and depth");

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 mirrored;
    mirrored.scale(-1, 1, 1);
    check(model.draw(mirrored, error, 0x234560, true), "mirrored instance adjusts winding");
    check(target.toImage().pixelColor(11, 32) == QColor(0x23, 0x45, 0x60), "mirrored surface remains selectable");
    QTemporaryDir directory;
    check(directory.isValid(), "temporary missing-reload path");
    check(!model.load(directory.filePath("missing.glb"), error), "failed reload rejected");
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    check(model.draw(identity, error, 0x123450), "failed reload retains usable GPU mesh");
    check(error.isEmpty() && target.toImage().pixelColor(11, 32) == QColor(0x12, 0x34, 0x50),
            "failed reload preserves pick output and clears old draw diagnostic");

    GltfPreview invalidTexture;
    GltfPrimitive oversized = quad;
    GLint maxTextureSize = 0;
    f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    oversized.image = QImage(maxTextureSize + 1, 1, QImage::Format_RGBA8888);
    invalidTexture.model.primitives = {oversized};
    check(!invalidTexture.draw(identity, error), "reject image over driver texture limit");
    check(SavedState(f) == before, "failed upload restores foreign state");
    invalidTexture.model.primitives = {quad};
    check(!invalidTexture.draw(identity, error), "failed upload stays cached until explicit retry");
    invalidTexture.release();
    check(invalidTexture.draw(identity, error), "explicit cleanup permits GPU retry");
    invalidTexture.release();

    QOpenGLContext other;
    other.setFormat(context.format());
    check(other.create(), "create independent context");
    QOffscreenSurface otherSurface;
    otherSurface.setFormat(other.format());
    otherSurface.create();
    check(other.makeCurrent(&otherSurface), "make independent context current");
    check(!model.draw(identity, error), "reject resources belonging to another context");
    check(context.makeCurrent(&surface), "return to owning context");
    model.release();
    target.bind();
    check(model.draw(identity, error, 0x123450), "recreate resources after explicit cleanup");
    check(SavedState(f) == before, "preserve foreign state after recreation");
    model.release();
    check(f->glGetError() == GL_NO_ERROR, "clean graphics teardown");
    f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    f->glBindVertexArray(0);
    f->glDeleteBuffers(1, &foreignBuffer);
    f->glDeleteVertexArrays(1, &foreignArray);
    f->glDeleteTextures(1, &foreignTexture);
    f->glBindSampler(0, 0);
    f->glDeleteSamplers(1, &foreignSampler);
}
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    run(QSurfaceFormat::CoreProfile);
    run(QSurfaceFormat::CompatibilityProfile);
    std::cout << "glTF graphics probe: " << failures << " failures\n";
    return failures ? 1 : 0;
}

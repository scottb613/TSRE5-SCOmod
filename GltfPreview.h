// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef TSRE_GLTF_PREVIEW_H
#define TSRE_GLTF_PREVIEW_H
#include "GltfModel.h"
#include <QMatrix4x4>
#include <QPointer>
#include <memory>

class QOpenGLContext;

// GPU ownership is restricted to one editor/viewer context. CPU model survives
// context recreation. release() must be called while that context is current.
class GltfPreview {
public:
    GltfPreview();
    ~GltfPreview();
    GltfModel model;
    QString sourcePath;
    QString loadError;
    unsigned int revision = 0;
    bool load(const QString &path, QString &error);
    bool draw(const QMatrix4x4 &matrix, QString &error,
              int selectionColor = 0, bool mirroredInstance = false);
    void release();
private:
    struct Resources;
    std::unique_ptr<Resources> resources;
    bool dirty = false;
    QPointer<QOpenGLContext> failedContext;
    QString graphicsError;
    unsigned int failedRevision = 0;
};
#endif

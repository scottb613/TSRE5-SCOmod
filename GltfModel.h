// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef TSRE_GLTF_MODEL_H
#define TSRE_GLTF_MODEL_H

#include <QImage>
#include <QStringList>
#include <QVector3D>
#include <QVector4D>
#include <vector>

// CPU-only, read-only static preview data. No MSTS shape/cache ownership.
struct GltfPrimitive {
    // Position and UV, with node transforms and OR's forward conversion applied.
    std::vector<float> vertices;
    QVector4D baseColor{1, 1, 1, 1};
    QImage image;
    int wrapS = 10497;
    int wrapT = 10497;
    int minFilter = 9987;
    int magFilter = 9729;
    bool doubleSided = false;
    bool masked = false;
    float alphaCutoff = 0.5f;
};

class GltfModel {
public:
    static bool accepts(const QString &path);
    bool load(const QString &path, QString &error);
    std::vector<GltfPrimitive> primitives;
    QVector3D minimum;
    QVector3D maximum;
    QStringList warnings;
};

#endif

// TSRE GenX terrain mesh preparation. Licensed under GNU GPL v3 or later.
#pragma once

#include "TerrainGridMath.h"
#include "Vector3f.h"
#include <QPair>
#include <QVector>
#include <QVector3D>
#include <array>
#include <memory>
#include <vector>

struct TerrainMeshInput {
    TerrainGridMath::RenderLayout layout;
    int sampleSize = 0;
    std::vector<float> heights;
    std::vector<unsigned char> flags;
    std::vector<std::array<float, 6>> textureTransforms;
};

struct TerrainMeshData {
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
    std::vector<float> patchVertices;
    QVector<QPair<QVector3D, QVector3D>> bounds;
};

std::unique_ptr<TerrainMeshData> buildTerrainMesh(const TerrainMeshInput &input);

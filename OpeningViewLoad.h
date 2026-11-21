// TSRE GenX route startup loading. Licensed under GNU GPL v3 or later.
#pragma once

#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QSet>
#include <QVector3D>
#include <QVector4D>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <functional>

struct OpeningViewLoad {
    using Jobs = std::deque<std::function<void()>>;
    Jobs discovery;
    std::deque<std::function<bool()>> assets;
    int discovered = 0;
    int discoveryCount = 0;
    int prepared = 0;
    int assetCount = 0;
    int queuedTerrainPatches = 0;
    bool profiling = false;
    QSet<int> terrainTextures;
    QSet<int> sceneryTextureIds;
    std::deque<int> sceneryTextures;
    QElapsedTimer sceneryDecodeGrace;
    int sceneryUploaded = 0;
    int sceneryDeferred = 0;

    void queueSceneryTexture(int id) {
        if (!sceneryTextureIds.contains(id)) {
            sceneryTextureIds.insert(id);
            sceneryTextures.push_back(id);
        }
    }
};

class TerrainViewFrustum {
public:
    explicit TerrainViewFrustum(const QMatrix4x4 &clip) {
        const QVector4D w = clip.row(3);
        for (int axis = 0; axis < 3; ++axis) {
            planes[2 * axis] = w + clip.row(axis);
            planes[2 * axis + 1] = w - clip.row(axis);
        }
    }

    bool intersects(const QVector3D &minimum, const QVector3D &maximum) const {
        for (const QVector4D &plane : planes) {
            const float x = plane.x() < 0 ? minimum.x() : maximum.x();
            const float y = plane.y() < 0 ? minimum.y() : maximum.y();
            const float z = plane.z() < 0 ? minimum.z() : maximum.z();
            const float margin = 0.01f * (std::abs(plane.x()) + std::abs(plane.y())
                                        + std::abs(plane.z()));
            if (plane.x() * x + plane.y() * y + plane.z() * z + plane.w() < -margin)
                return false;
        }
        return true;
    }

private:
    std::array<QVector4D, 6> planes;
};

inline QMatrix4x4 openingViewMatrix(const float *columnMajor) {
    QMatrix4x4 matrix;
    std::copy_n(columnMajor, 16, matrix.data());
    return matrix;
}

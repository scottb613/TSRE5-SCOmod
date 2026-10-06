// TSRE GenX modifications. Licensed under GNU GPL v3 or later. See LICENSE.md.
#ifndef TSRE_GLTF_PLACEMENT_MATH_H
#define TSRE_GLTF_PLACEMENT_MATH_H

#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>
#include <algorithm>

namespace GltfPlacementMath {
inline QMatrix4x4 matrix(const float *legacyMatrix) {
    QMatrix4x4 result;
    std::copy(legacyMatrix, legacyMatrix + 16, result.data());
    // WorldObj already includes its legacy Y half-turn. SFile's root adds
    // an X reflection; together these convert OR-local Z to editor-local Z.
    // Reproduce that same coordinate bridge for the OR-oriented CPU mesh.
    result.scale(-1, 1, 1);
    return result;
}
inline QVector3D selectionRgb(int encoded) {
    const unsigned int value = static_cast<unsigned int>(encoded);
    return QVector3D(float((value >> 16) & 255) / 255,
                     float((value >> 8) & 255) / 255, float(value & 255) / 255);
}
inline void boxPoints(const QVector3D &minimum, const QVector3D &maximum,
        QVector<float> &points) {
    // drawBox uses the WorldObj matrix, so include the same X reflection
    // used by the glTF mesh draw. Do not recenter the placement pivot.
    for(int axis = 0; axis < 3; ++axis) {
        const int a = (axis + 1) % 3, b = (axis + 2) % 3;
        for(int sideA = 0; sideA < 2; ++sideA)
            for(int sideB = 0; sideB < 2; ++sideB) {
                QVector3D start = minimum, end = minimum;
                start[a] = end[a] = sideA ? maximum[a] : minimum[a];
                start[b] = end[b] = sideB ? maximum[b] : minimum[b];
                end[axis] = maximum[axis];
                for(const auto &v : {start, end})
                    points << -v.x() << v.y() << v.z();
            }
    }
}
}
#endif

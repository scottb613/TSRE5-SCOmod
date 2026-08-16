#ifndef FORESTPATCHBAKER_H
#define FORESTPATCHBAKER_H

#include <QHash>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

struct ForestShapeVertex {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float nx = 0.0f, ny = 1.0f, nz = 0.0f;
    float u = 0.0f, v = 0.0f;
};

struct ForestShapeMesh {
    QString texture;
    QVector<ForestShapeVertex> vertices;
    QVector<int> indices;
};

struct ForestBakeInstance {
    QString shapePath;
    double x = 0.0, y = 0.0, z = 0.0;
    double yawDegrees = 0.0;
    double uniformScale = 1.0;
};

struct ForestPatchKey {
    int tileX = 0, tileZ = 0, patchX = 0, patchZ = 0;
    bool operator==(const ForestPatchKey &other) const {
        return tileX == other.tileX && tileZ == other.tileZ
            && patchX == other.patchX && patchZ == other.patchZ;
    }
};

inline size_t qHash(const ForestPatchKey &key, size_t seed = 0) {
    return qHashMulti(seed, key.tileX, key.tileZ, key.patchX, key.patchZ);
}

struct ForestBakedPatch {
    ForestPatchKey key;
    double originX = 0.0, originY = 0.0, originZ = 0.0;
    QVector<ForestShapeMesh> meshes;
    int sourceInstanceCount = 0;
};

struct ForestPatchBakeResult {
    QVector<ForestBakedPatch> patches;
    QStringList errors;
    bool isValid() const { return errors.isEmpty(); }
};

class ForestShapeTextIO {
public:
    static bool readCruciform(const QString &path, ForestShapeMesh &mesh,
                              QString &error);
    static bool writePatch(const QString &path, const ForestBakedPatch &patch,
                           QString &error);
    static bool writeDescriptor(const QString &shapePath, QString &error);
};

class ForestPatchBaker {
public:
    static ForestPatchBakeResult bake(const QVector<ForestBakeInstance> &instances,
                                       int patchSpan = 1);
};

#endif

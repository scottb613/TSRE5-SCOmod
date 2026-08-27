#ifndef FORESTBAKEMANIFEST_H
#define FORESTBAKEMANIFEST_H

#include "ForestPatchBaker.h"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

struct ForestBakeManifestEntry {
    QString id;
    QString shapeFile;
    int tileX = 0, tileZ = 0;
    int blockX = 0, blockZ = 0;
    double positionX = 0.0, positionY = 0.0, positionZ = 0.0;
    bool enabled = true;
    QVector<ForestBakeInstance> sources;
};

struct ForestBakePruneResult {
    int originalBlocks = 0;
    int remainingBlocks = 0;
    int removedBlocks = 0;
    int removedAssets = 0;
    bool removedManifest = false;
    QStringList retainedShapes;
    QStringList removedShapes;
};

class ForestBakeSession {
public:
    bool rememberFile(const QString &path, QString &error);
    bool rollback(QString &error);
    void commit();
    bool isEmpty() const;

private:
    struct FileState {
        bool existed = false;
        QByteArray contents;
    };
    QHash<QString, FileState> files;
};

class ForestBakeManifest {
public:
    static bool upsert(const QString &path, const ForestBakeManifestEntry &entry,
                       QString &error);
    static bool pruneUnreferenced(const QString &routePath,
                                  ForestBakePruneResult &result,
                                  QString &error);
};

#endif

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef FORESTOSMCACHE_H
#define FORESTOSMCACHE_H

#include "ForestGenerator.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct ForestOsmPolygon {
    QString featureId;
    QString category;
    int drawOrder = 0;
    QHash<QString, QString> tags;
    ForestPlantingBoundary boundary;
    double minimumX = 0.0;
    double maximumX = 0.0;
    double minimumZ = 0.0;
    double maximumZ = 0.0;
};

struct ForestOsmCacheLoadResult {
    QString filePath;
    int schemaVersion = 0;
    int sourceFeatureCount = 0;
    QVector<ForestOsmPolygon> polygons;
    QStringList errors;
    QStringList warnings;

    bool isValid() const { return errors.isEmpty(); }
};

class ForestOsmCache {
public:
    static ForestOsmCacheLoadResult loadRoute(const QString &routePath);
    static ForestOsmCacheLoadResult loadFile(const QString &filePath);
};

#endif

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "PolyVegObject.h"

#include "ForestDefinition.h"
#include "Game.h"

#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>

namespace {
QString shapeBaseName(QString name) {
    name = name.trimmed();
    name.replace('\\', '/');
    return name.section('/', -1);
}

const QSet<QString> &rawShapeNames() {
    static QString cachedPath;
    static QDateTime cachedModified;
    static QSet<QString> cachedNames;
    static QElapsedTimer lastCheck;
    const QString routePath = Game::root + "/routes/" + Game::route;
    const QString jsonPath = routePath + "/OpenRails/polyveg.json";
    // Hidden dense routes must not stat the schema for every object every frame.
    if(jsonPath == cachedPath && lastCheck.isValid() && lastCheck.elapsed() < 1000)
        return cachedNames;
    lastCheck.start();
    const QDateTime modified = QFileInfo(jsonPath).lastModified();
    if(jsonPath == cachedPath && modified == cachedModified) return cachedNames;

    cachedPath = jsonPath;
    cachedModified = modified;
    cachedNames.clear();
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(routePath);
    if(result.isValid())
        for(const ForestRecipeDefinition &recipe : result.catalog.polyVeg)
            for(const ForestVegetationDefinition &vegetation : recipe.vegetation)
                cachedNames.insert(shapeBaseName(vegetation.shape).toLower());
    return cachedNames;
}
}

bool PolyVegObject::isRawShape(const QString &fileName) {
    return rawShapeNames().contains(shapeBaseName(fileName).toLower());
}

bool PolyVegObject::isBakeShape(const QString &fileName) {
    static const QRegularExpression expression(
        QStringLiteral("^V[+-]\\d{5}[+-]\\d{5}-\\d{2}\\.s$"),
        QRegularExpression::CaseInsensitiveOption);
    return expression.match(shapeBaseName(fileName)).hasMatch();
}

bool PolyVegObject::isVegetationShape(const QString &fileName, bool generatedRaw) {
    return generatedRaw || isBakeShape(fileName) || isRawShape(fileName);
}

QString PolyVegObject::labelForShape(const QString &fileName, bool generatedRaw) {
    if(isBakeShape(fileName)) return QStringLiteral("PolyVeg - Bake");
    if(generatedRaw) return QStringLiteral("PolyVeg - Raw");
    return QStringLiteral("Static Object");
}

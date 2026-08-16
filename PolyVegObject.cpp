#include "PolyVegObject.h"

#include "ForestDefinition.h"
#include "Game.h"

#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace {
QSet<QString> rawShapeNames() {
    static QString cachedPath;
    static QDateTime cachedModified;
    static QSet<QString> cachedNames;
    const QString routePath = Game::root + "/routes/" + Game::route;
    const QString jsonPath = routePath + "/OpenRails/polyveg.json";
    const QDateTime modified = QFileInfo(jsonPath).lastModified();
    if(jsonPath == cachedPath && modified == cachedModified) return cachedNames;

    cachedPath = jsonPath;
    cachedModified = modified;
    cachedNames.clear();
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(routePath);
    if(result.isValid())
        for(const ForestRecipeDefinition &recipe : result.catalog.polyVeg)
            for(const ForestVegetationDefinition &vegetation : recipe.vegetation)
                cachedNames.insert(vegetation.shape.toLower());
    return cachedNames;
}
}

bool PolyVegObject::isRawShape(const QString &fileName) {
    return rawShapeNames().contains(fileName.toLower());
}

bool PolyVegObject::isBakeShape(const QString &fileName) {
    static const QRegularExpression expression(
        QStringLiteral("^V[+-]\\d{5}[+-]\\d{5}-\\d{2}\\.s$"),
        QRegularExpression::CaseInsensitiveOption);
    return expression.match(fileName).hasMatch();
}

QString PolyVegObject::labelForShape(const QString &fileName) {
    if(isBakeShape(fileName)) return QStringLiteral("PolyVeg - Bake");
    if(isRawShape(fileName)) return QStringLiteral("PolyVeg - Raw");
    return QStringLiteral("Static Object");
}

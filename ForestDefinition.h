#ifndef FORESTDEFINITION_H
#define FORESTDEFINITION_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct ForestNumberRange {
    double minimum = 0.0;
    double maximum = 0.0;
};

struct ForestOsmMatchRule {
    QHash<QString, QStringList> tags;
};

struct ForestVegetationDefinition {
    QString id;
    QString name;
    QString shape;
    double proportion = 0.0;
    double normalizedProportion = 0.0;
    ForestNumberRange yawDegrees;
    ForestNumberRange uniformScale;
    double plantingDepthMetres = 0.0;
    bool hasPlantingDepth = false;
    double footprintRadiusMetres = 0.0;
};

struct ForestRecipeDefinition {
    QString id;
    QString name;
    QString description;

    double defaultDensityPerSquareMetre = 0.0;
    double defaultTrackClearanceMetres = 0.0;
    double defaultRoadClearanceMetres = 0.0;

    ForestNumberRange densityLimitsPerSquareMetre;
    int defaultMaximumTrees = 0;
    int minimumMaximumTrees = 0;
    int maximumMaximumTrees = 0;

    double maximumSlopeDegrees = 0.0;
    double edgeFeatherMetres = 0.0;
    double minimumSeparationMetres = 0.0;

    int osmPriority = 0;
    QStringList osmCategories;
    QVector<ForestOsmMatchRule> osmMatchAny;
    QVector<ForestVegetationDefinition> vegetation;
};

struct ForestCatalog {
    int schemaVersion = 0;
    QVector<ForestRecipeDefinition> polyVeg;
};

struct ForestCatalogLoadResult {
    QString filePath;
    ForestCatalog catalog;
    QStringList errors;
    QStringList warnings;

    bool isValid() const { return errors.isEmpty(); }
};

class ForestDefinitionLoader {
public:
    static ForestCatalogLoadResult loadRoute(const QString &routePath);
    static ForestCatalogLoadResult loadFile(
        const QString &filePath,
        const QString &routeShapesPath);
};

#endif

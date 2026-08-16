#ifndef FORESTGENERATOR_H
#define FORESTGENERATOR_H

#include "ForestDefinition.h"

#include <QVector>

#include <cstdint>
#include <functional>

struct ForestPlanPoint {
    double x = 0.0;
    double z = 0.0;
};

using ForestPlanRing = QVector<ForestPlanPoint>;

struct ForestPlantingBoundary {
    ForestPlanRing outer;
    QVector<ForestPlanRing> holes;
};

struct ForestGenerationSettings {
    double densityPerSquareMetre = 0.0;
    int maximumTrees = 0;
    double variationScale = 1.0;
    std::uint64_t seed = 0;
    bool rowsEnabled = false;
    double rowWidthMetres = 0.0;
    double rowSpacingMetres = 0.0;
    double rowDirectionDegrees = 0.0;
    std::function<bool(double x, double z)> acceptsTerrain;
    std::function<void(int attempts, int maximumAttempts,
                       int accepted, int target)> progress;
    std::function<bool()> shouldCancel;
};

struct ForestCandidate {
    int vegetationIndex = -1;
    QString vegetationId;
    QString shape;
    QString stratum;
    double x = 0.0;
    double z = 0.0;
    double yawDegrees = 0.0;
    double uniformScale = 1.0;
    double scaledFootprintRadiusMetres = 0.0;
};

struct ForestGenerationResult {
    double usableAreaSquareMetres = 0.0;
    int requestedCount = 0;
    int targetCount = 0;
    int attempts = 0;
    int rejectedOutside = 0;
    int rejectedEdgeFeather = 0;
    int rejectedOccupied = 0;
    int rejectedTerrain = 0;
    bool objectLimitApplied = false;
    bool cancelled = false;
    QVector<ForestCandidate> candidates;
    QStringList errors;

    bool isValid() const { return errors.isEmpty(); }
};

class ForestGenerator {
public:
    static ForestGenerationResult generate(
        const ForestRecipeDefinition &recipe,
        const ForestPlantingBoundary &boundary,
        const ForestGenerationSettings &settings);
};

#endif

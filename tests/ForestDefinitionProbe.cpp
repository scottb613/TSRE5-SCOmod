#include "ForestDefinition.h"
#include "ForestGenerator.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>

#include <algorithm>
#include <algorithm>
#include <cmath>

namespace {

bool check(bool condition, const QString &message) {
    if(!condition)
        qCritical().noquote() << message;
    return condition;
}

bool writeJson(const QString &path, const QJsonDocument &document) {
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(document.toJson(QJsonDocument::Indented)) > 0;
}

bool prepareRoute(
    const QString &routePath,
    const QJsonDocument &document,
    bool createShapes) {
    if(!QDir().mkpath(routePath + "/OpenRails")
            || !QDir().mkpath(routePath + "/Shapes")
            || !writeJson(routePath + "/OpenRails/polyveg.json", document))
        return false;
    if(!createShapes)
        return true;

    const QJsonArray recipes = document.object().value("polyVeg").toArray();
    for(const QJsonValue &forestValue : recipes) {
        const QJsonArray vegetation =
            forestValue.toObject().value("vegetation").toArray();
        for(const QJsonValue &entryValue : vegetation) {
            const QString shape = entryValue.toObject().value("shape").toString();
            QFile shapeFile(routePath + "/Shapes/" + shape);
            if(!shapeFile.open(QIODevice::WriteOnly))
                return false;
            shapeFile.write("SIMISA");
        }
    }
    return true;
}

QJsonDocument readDocument(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return QJsonDocument();
    return QJsonDocument::fromJson(file.readAll());
}

bool containsError(
    const ForestCatalogLoadResult &result,
    const QString &needle) {
    for(const QString &error : result.errors) {
        if(error.contains(needle, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool sameCandidates(
    const QVector<ForestCandidate> &a,
    const QVector<ForestCandidate> &b) {
    if(a.size() != b.size())
        return false;
    for(int index = 0; index < a.size(); ++index) {
        const ForestCandidate &left = a.at(index);
        const ForestCandidate &right = b.at(index);
        if(left.vegetationId != right.vegetationId
                || left.x != right.x || left.z != right.z
                || left.yawDegrees != right.yawDegrees
                || left.uniformScale != right.uniformScale)
            return false;
    }
    return true;
}

bool insideAxisAlignedHole(const ForestCandidate &candidate) {
    return candidate.x > 40.0 && candidate.x < 60.0
        && candidate.z > 40.0 && candidate.z < 60.0;
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if(app.arguments().size() < 2) {
        qCritical() << "Expected the version-1 example JSON path.";
        return 1;
    }

    const QJsonDocument example = readDocument(app.arguments().at(1));
    if(!check(example.isObject(), "Example forest JSON did not parse."))
        return 2;

    QTemporaryDir temporary;
    if(!check(temporary.isValid(), "Unable to create temporary route."))
        return 3;
    const QString validRoute = temporary.path() + "/ValidRoute";
    if(!check(prepareRoute(validRoute, example, true),
            "Unable to prepare valid route fixture."))
        return 4;

    bool passed = true;
    const ForestCatalogLoadResult valid =
        ForestDefinitionLoader::loadRoute(validRoute);
    passed &= check(valid.isValid(),
        "Valid catalog was rejected:\n" + valid.errors.join('\n'));
    passed &= check(valid.catalog.schemaVersion == 1,
        "Schema version was not retained.");
    passed &= check(valid.catalog.polyVeg.size() == 1,
        "Expected one recipe.");
    if(valid.catalog.polyVeg.size() == 1) {
        const ForestRecipeDefinition &recipe = valid.catalog.polyVeg.first();
        passed &= check(recipe.vegetation.size() == 10,
            "Expected ten vegetation entries.");
        passed &= check(recipe.minimumSeparationMetres > 0.0,
            "Global minimum separation was not loaded.");
        passed &= check(recipe.preventFootprintOverlap,
            "Global footprint overlap prevention was not loaded.");
        passed &= check(recipe.osmMatchAny.size() == 2,
            "Expected natural=wood and landuse=forest OSM rules.");
        double normalizedTotal = 0.0;
        for(const ForestVegetationDefinition &entry : recipe.vegetation)
            normalizedTotal += entry.normalizedProportion;
        passed &= check(std::fabs(normalizedTotal - 1.0) < 0.000001,
            "Vegetation proportions were not normalized to one.");

        ForestPlantingBoundary boundary;
        boundary.outer = {{0.0, 0.0}, {100.0, 0.0},
            {100.0, 100.0}, {0.0, 100.0}};
        boundary.holes.append({{40.0, 40.0}, {60.0, 40.0},
            {60.0, 60.0}, {40.0, 60.0}});
        ForestGenerationSettings settings;
        settings.densityPerSquareMetre = recipe.defaultDensityPerSquareMetre;
        settings.maximumTrees = recipe.defaultMaximumTrees;
        settings.variationScale = recipe.defaultVariationScale;
        settings.seed = 0x123456789abcdef0ULL;

        const ForestGenerationResult first =
            ForestGenerator::generate(recipe, boundary, settings);
        const ForestGenerationResult repeated =
            ForestGenerator::generate(recipe, boundary, settings);
        passed &= check(first.isValid(),
            "Valid polygon generation failed: " + first.errors.join(';'));
        passed &= check(first.usableAreaSquareMetres == 9600.0,
            "Polygon hole was not subtracted from usable area.");
        passed &= check(first.targetCount == 173,
            "Area-density target count was calculated incorrectly.");
        passed &= check(first.candidates.size() == first.targetCount,
            "Generator did not fill the feasible target population.");
        passed &= check(sameCandidates(first.candidates, repeated.candidates),
            "Same recipe, polygon, settings, and seed were not deterministic.");

        ForestGenerationSettings differentSeed = settings;
        differentSeed.seed++;
        const ForestGenerationResult changed =
            ForestGenerator::generate(recipe, boundary, differentSeed);
        passed &= check(!sameCandidates(first.candidates, changed.candidates),
            "New seed did not produce a new layout.");

        ForestGenerationSettings rowSettings = settings;
        rowSettings.rowsEnabled = true;
        rowSettings.rowWidthMetres = 10.0;
        rowSettings.rowSpacingMetres = 10.0;
        rowSettings.rowDirectionDegrees = 0.0;
        rowSettings.densityPerSquareMetre = -1.0;
        const ForestGenerationResult rows =
            ForestGenerator::generate(recipe, boundary, rowSettings);
        const ForestGenerationResult repeatedRows =
            ForestGenerator::generate(recipe, boundary, rowSettings);
        passed &= check(rows.isValid() && !rows.candidates.isEmpty(),
            "Row generation failed: " + rows.errors.join(';'));
        passed &= check(rows.requestedCount == 96,
            "Row population did not use area divided by row width and spacing.");
        passed &= check(sameCandidates(rows.candidates, repeatedRows.candidates),
            "Row generation was not deterministic.");
        for(const ForestCandidate &candidate : rows.candidates) {
            passed &= check(std::fabs(candidate.x/10.0
                    - std::round(candidate.x/10.0)) < 0.0000001,
                "Zero-degree row candidate was not aligned to its row width.");
            passed &= check(std::fabs(candidate.z/10.0
                    - std::round(candidate.z/10.0)) < 0.0000001,
                "Zero-degree row candidate was not aligned to its spacing.");
        }

        ForestGenerationSettings eastWestRows = rowSettings;
        eastWestRows.rowDirectionDegrees = 90.0;
        const ForestGenerationResult rotatedRows =
            ForestGenerator::generate(recipe, boundary, eastWestRows);
        passed &= check(rotatedRows.isValid() && !rotatedRows.candidates.isEmpty(),
            "Rotated row generation failed: " + rotatedRows.errors.join(';'));
        for(const ForestCandidate &candidate : rotatedRows.candidates) {
            passed &= check(std::fabs(candidate.z/10.0
                    - std::round(candidate.z/10.0)) < 0.0000001,
                "Ninety-degree row candidate was not aligned to its row width.");
            passed &= check(std::fabs(candidate.x/10.0
                    - std::round(candidate.x/10.0)) < 0.0000001,
                "Ninety-degree row candidate was not aligned to its spacing.");
        }

        ForestGenerationSettings terrainFiltered = settings;
        terrainFiltered.acceptsTerrain = [](double x, double) {
            return x < 50.0;
        };
        const ForestGenerationResult filtered =
            ForestGenerator::generate(recipe, boundary, terrainFiltered);
        passed &= check(filtered.isValid() && !filtered.candidates.isEmpty()
                && filtered.rejectedTerrain > 0,
            "Terrain acceptance filter was not applied.");
        for(const ForestCandidate &candidate : filtered.candidates)
            passed &= check(candidate.x < 50.0,
                "Terrain acceptance filter admitted a rejected point.");

        ForestGenerationSettings cancelledSettings = settings;
        int cancellationChecks = 0;
        cancelledSettings.shouldCancel = [&cancellationChecks]() {
            return ++cancellationChecks >= 2;
        };
        const ForestGenerationResult cancelled =
            ForestGenerator::generate(recipe, boundary, cancelledSettings);
        passed &= check(cancelled.isValid() && cancelled.cancelled
                && cancelled.candidates.size() < cancelled.targetCount,
            "Generator cancellation did not stop an in-progress layout.");

        QSet<QString> selectedVegetation;
        for(int index = 0; index < first.candidates.size(); ++index) {
            const ForestCandidate &candidate = first.candidates.at(index);
            selectedVegetation.insert(candidate.vegetationId);
            const ForestVegetationDefinition &entry =
                recipe.vegetation.at(candidate.vegetationIndex);
            passed &= check(!insideAxisAlignedHole(candidate),
                "Candidate was accepted inside a polygon hole.");
            passed &= check(candidate.uniformScale >= entry.uniformScale.minimum
                    && candidate.uniformScale <= entry.uniformScale.maximum,
                "Candidate scale escaped its entry range.");
            passed &= check(candidate.yawDegrees >= entry.yawDegrees.minimum
                    && candidate.yawDegrees <= entry.yawDegrees.maximum,
                "Candidate yaw escaped its entry range.");
            for(int otherIndex = index + 1;
                    otherIndex < first.candidates.size(); ++otherIndex) {
                const ForestCandidate &other = first.candidates.at(otherIndex);
                const double dx = candidate.x - other.x;
                const double dz = candidate.z - other.z;
                const double required = std::max(recipe.minimumSeparationMetres,
                    candidate.scaledFootprintRadiusMetres
                        + other.scaledFootprintRadiusMetres);
                passed &= check(dx*dx + dz*dz + 0.0000001 >= required*required,
                    "Two accepted candidates violate global separation.");
            }
        }
        passed &= check(selectedVegetation.size() == recipe.vegetation.size(),
            "Weighted selection did not exercise every example vegetation entry.");

        ForestGenerationSettings noVariation = settings;
        noVariation.variationScale = 0.0;
        const ForestGenerationResult midpoint =
            ForestGenerator::generate(recipe, boundary, noVariation);
        for(const ForestCandidate &candidate : midpoint.candidates) {
            const ForestNumberRange range =
                recipe.vegetation.at(candidate.vegetationIndex).uniformScale;
            passed &= check(candidate.uniformScale
                    == (range.minimum + range.maximum) * 0.5,
                "Zero variation did not use each entry's scale midpoint.");
        }
    }

    QJsonObject contractCategoryRoot = example.object();
    QJsonArray contractRecipes = contractCategoryRoot.value("polyVeg").toArray();
    QJsonObject contractRecipe = contractRecipes.first().toObject();
    QJsonObject contractOsm = contractRecipe.value("osm").toObject();
    contractOsm.insert("categories", QJsonArray {
        "woodland", "scrub", "heath", "grassland", "wetland", "agriculture",
        "orchard", "parkland", "golf_course", "cemetery", "sports", "zoo"
    });
    contractRecipe.insert("osm", contractOsm);
    contractRecipes.replace(0, contractRecipe);
    contractCategoryRoot.insert("polyVeg", contractRecipes);
    const QString contractRoute = temporary.path() + "/ContractCategories";
    passed &= check(prepareRoute(contractRoute,
            QJsonDocument(contractCategoryRoot), true),
        "Unable to prepare the contract-category fixture.");
    const ForestCatalogLoadResult contractCategories =
        ForestDefinitionLoader::loadRoute(contractRoute);
    passed &= check(contractCategories.isValid()
            && !contractCategories.catalog.polyVeg.isEmpty()
            && contractCategories.catalog.polyVeg.first().osmCategories.size() == 12,
        "The twelve LIDEX v2 planting categories were not accepted.");

    QJsonObject missingShapeRoot = example.object();
    QJsonArray missingShapeForests = missingShapeRoot.value("polyVeg").toArray();
    QJsonObject missingShapeForest = missingShapeForests.at(0).toObject();
    QJsonArray missingShapeEntries = missingShapeForest.value("vegetation").toArray();
    QJsonObject missingShapeEntry = missingShapeEntries.at(0).toObject();
    missingShapeEntry.insert("shape", "missing-tree.s");
    missingShapeEntries.replace(0, missingShapeEntry);
    missingShapeForest.insert("vegetation", missingShapeEntries);
    missingShapeForests.replace(0, missingShapeForest);
    missingShapeRoot.insert("polyVeg", missingShapeForests);
    const QString missingRoute = temporary.path() + "/MissingRoute";
    prepareRoute(missingRoute, QJsonDocument(missingShapeRoot), false);
    const ForestCatalogLoadResult missing =
        ForestDefinitionLoader::loadRoute(missingRoute);
    passed &= check(!missing.isValid() && containsError(missing, "shape not found"),
        "Missing route shapes were not rejected.");

    QJsonObject unsafeRoot = example.object();
    QJsonArray unsafeForests = unsafeRoot.value("polyVeg").toArray();
    QJsonObject unsafeForest = unsafeForests.at(0).toObject();
    QJsonArray unsafeEntries = unsafeForest.value("vegetation").toArray();
    QJsonObject unsafeEntry = unsafeEntries.at(0).toObject();
    unsafeEntry.insert("shape", "../outside.s");
    unsafeEntries.replace(0, unsafeEntry);
    unsafeForest.insert("vegetation", unsafeEntries);
    unsafeForests.replace(0, unsafeForest);
    unsafeRoot.insert("polyVeg", unsafeForests);
    const QString unsafeRoute = temporary.path() + "/UnsafeRoute";
    prepareRoute(unsafeRoute, QJsonDocument(unsafeRoot), false);
    const ForestCatalogLoadResult unsafe =
        ForestDefinitionLoader::loadRoute(unsafeRoute);
    passed &= check(!unsafe.isValid() && containsError(unsafe, "safe .s basename"),
        "Unsafe shape traversal was not rejected.");

    QJsonObject overlapRoot = example.object();
    QJsonArray overlapForests = overlapRoot.value("polyVeg").toArray();
    QJsonObject overlapForest = overlapForests.at(0).toObject();
    QJsonObject distribution = overlapForest.value("distribution").toObject();
    distribution.insert("minimumSeparationMetres", 0.0);
    distribution.insert("preventFootprintOverlap", false);
    overlapForest.insert("distribution", distribution);
    overlapForests.replace(0, overlapForest);
    overlapRoot.insert("polyVeg", overlapForests);
    const QString overlapRoute = temporary.path() + "/OverlapRoute";
    prepareRoute(overlapRoute, QJsonDocument(overlapRoot), true);
    const ForestCatalogLoadResult overlap =
        ForestDefinitionLoader::loadRoute(overlapRoute);
    passed &= check(!overlap.isValid()
            && containsError(overlap, "global footprint-overlap prevention"),
        "A recipe permitting coincident cross-stratum placement was accepted.");

    if(app.arguments().size() >= 3) {
        const ForestCatalogLoadResult routeResult =
            ForestDefinitionLoader::loadRoute(app.arguments().at(2));
        passed &= check(routeResult.isValid(),
            "Requested route catalog was rejected:\n"
                + routeResult.errors.join('\n'));
        if(routeResult.isValid()) {
            qInfo().noquote() << QString("Validated route catalog: %1 recipe(s)")
                .arg(routeResult.catalog.polyVeg.size());
        }
    }

    return passed ? 0 : 5;
}

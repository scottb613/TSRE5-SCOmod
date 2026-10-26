// TSRE GenX forest replacement regression probe. GPL v3 or later.
#include "ForestReplacementPlan.h"
#include "PolyVegPlantingArea.h"
#include "PolyVegClearance.h"
#include "ForestBakeManifest.h"
#include "PolyVegCorridor.h"
#include <QCoreApplication>
#include <iostream>
#include <QFile>
#include <QTemporaryDir>

namespace {
bool check(bool value, const char *message) {
    if(!value) std::cerr << "FAILED: " << message << '\n';
    return value;
}
bool write(const QString &path, const QByteArray &contents) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}
QByteArray read(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    using namespace ForestReplacementPlan;
    bool ok = true;
    ok &= check(bakeShapeName(-3, 4, 1, 2) == "V-00003-00004-12.s",
        "Bake filenames must retain the existing tile and world-Z convention");
    QVector<QPainterPath> groups;
    merge(groups, footprint(0, 0, 100, 100, 0));
    merge(groups, footprint(150, 0, 100, 100, 0));
    merge(groups, footprint(75, 0, 100, 100, 0));
    ok &= check(groups.size() == 1 && std::abs(area(groups[0])-25000) < 0.01,
        "Transitive overlap must merge without double-counting area");
    merge(groups, footprint(0, 0, 100, 100, 0));
    ok &= check(groups.size() == 1 && std::abs(area(groups[0])-25000) < 0.01,
        "Nested and duplicate regions must not increase density");
    merge(groups, footprint(500, 500, 10, 10, 0));
    ok &= check(groups.size() == 2, "Disconnected forests must stay separate");
    const QPainterPath rotated = footprint(-2048, -2048, 100, 20, 3.141592653589793/2);
    ok &= check(std::abs(rotated.boundingRect().width()-20) < 0.01
            && std::abs(rotated.boundingRect().height()-100) < 0.01
            && std::abs(area(rotated)-2000) < 0.01,
        "Rotated forests on negative tiles must preserve their footprint");
    QPainterPath hole;
    hole.addRect(-50, -50, 100, 100);
    hole.addRect(-10, -10, 20, 20);
    hole.setFillRule(Qt::OddEvenFill);
    ok &= check(!hole.contains(QPointF(0, 0)) && std::abs(area(hole)-9600) < 0.01,
        "Union area must exclude holes");
    QPainterPath inner;
    inner.addRect(-10, -10, 20, 20);
    QPainterPath reverseHole;
    reverseHole.addRect(-50, -50, 100, 100);
    reverseHole.addPath(inner.toReversed());
    reverseHole.setFillRule(Qt::OddEvenFill);
    ok &= check(std::abs(area(reverseHole)-9600) < 0.01,
        "Hole area must not depend on contour winding");
    QPainterPath island = hole;
    island.addRect(-2, -2, 4, 4);
    ok &= check(std::abs(area(island)-9616) < 0.01,
        "Area must include islands nested inside holes");
    QPainterPath winding;
    winding.addRect(-50, -50, 100, 100);
    winding.addRect(-10, -10, 20, 20);
    winding.setFillRule(Qt::WindingFill);
    ok &= check(std::abs(area(winding)-10000) < 0.01,
        "Winding fill must not subtract filled nested contours");
    const QPainterPath crossing = footprint(1024, 0, 200, 100, 0);
    const auto left = crossing.intersected(tileRectangle(0, 0));
    const auto right = crossing.intersected(tileRectangle(1, 0));
    ok &= check(std::abs(area(left)+area(right)-area(crossing)) < 0.01,
        "Tile partition must preserve total area");
    ok &= check(std::abs(area(left)-10000) < 0.01,
        "Planting clipped to one covered tile must omit the uncovered area");
    ok &= check(std::abs(edgeDistance(QPointF(1024, 0), crossing.toSubpathPolygons())-50) < 0.01,
        "Artificial tile edge must not become a feather edge");

    ForestRecipeDefinition recipe;
    recipe.minimumSeparationMetres = 4;
    recipe.edgeFeatherMetres = 20;
    ForestVegetationDefinition vegetation;
    vegetation.normalizedProportion = 1;
    vegetation.uniformScale = {1, 1};
    vegetation.footprintRadiusMetres = 3;
    recipe.vegetation.append(vegetation);
    ForestPlantingBoundary box;
    box.outer = {{1024, -50}, {1124, -50}, {1124, 50}, {1024, 50}};
    ForestGenerationSettings settings;
    settings.densityPerSquareMetre = 0.1;
    settings.maximumTrees = 40;
    settings.seed = 123;
    settings.usableAreaOverride = area(right);
    settings.containsPlantingPoint = [right](double x, double z) { return right.contains({x, z}); };
    settings.plantingEdgeDistance = [crossing](double x, double z) {
        return edgeDistance({x, z}, crossing.toSubpathPolygons());
    };
    ForestCandidate neighbor;
    neighbor.x = 1023;
    neighbor.z = 0;
    neighbor.scaledFootprintRadiusMetres = 3;
    settings.occupiedCandidates.append(neighbor);
    const auto result = ForestGenerator::generate(recipe, box, settings);
    const auto repeat = ForestGenerator::generate(recipe, box, settings);
    ok &= check(result.isValid() && result.objectLimitApplied && result.candidates.size() == 40,
        "Each tile must honor its cap using union area");
    ok &= check(result.candidates.size() == repeat.candidates.size(), "Batch seeds must be repeatable");
    for(int i = 0; i < result.candidates.size(); ++i) {
        const auto &candidate = result.candidates[i];
        ok &= check(right.contains({candidate.x, candidate.z}), "Candidates must stay in the real footprint");
        ok &= check(std::hypot(candidate.x-neighbor.x, candidate.z-neighbor.z) >= 6,
            "Footprint spacing must continue across tile boundaries");
        ok &= check(candidate.x == repeat.candidates[i].x && candidate.z == repeat.candidates[i].z,
            "Repeat planting must produce the same positions");
    }
    settings.acceptsTerrain = [](double x, double z) {
        // Three distinct exclusion strips stand in for the live TDB, RDB and
        // water predicate; the editor wiring is verified on a copied route.
        return x >= 1030 && x <= 1110 && z >= 0;
    };
    const auto excluded = ForestGenerator::generate(recipe, box, settings);
    ok &= check(excluded.isValid() && excluded.rejectedTerrain > 0 && !excluded.candidates.isEmpty(),
        "Exclusion predicates must run during candidate generation");
    for(const auto &candidate : excluded.candidates)
        ok &= check(candidate.x >= 1030 && candidate.x <= 1110 && candidate.z >= 0,
            "No accepted plant may cross an exclusion predicate");

    ForestRecipeDefinition sparseRecipe = recipe;
    sparseRecipe.edgeFeatherMetres = 0;
    sparseRecipe.minimumSeparationMetres = 0.5;
    sparseRecipe.vegetation[0].footprintRadiusMetres = 0.2;
    const QPainterPath sliver = footprint(1000, 0, 0.25, 200, 0);
    ForestPlantingBoundary fullTile;
    fullTile.outer = {{-1024, -1024}, {1024, -1024}, {1024, 1024}, {-1024, 1024}};
    ForestGenerationSettings sparse;
    sparse.densityPerSquareMetre = 0.018;
    sparse.maximumTrees = 60000;
    sparse.seed = 1;
    sparse.usableAreaOverride = area(sliver);
    sparse.containsPlantingPoint = [sliver](double x, double z) { return sliver.contains(QPointF(x, z)); };
    sparse.samplingRectangles = samplingRectangles(sliver);
    const auto plantedSliver = ForestGenerator::generate(sparseRecipe, fullTile, sparse);
    ok &= check(plantedSliver.isValid() && plantedSliver.targetCount == 1
            && plantedSliver.candidates.size() == 1 && plantedSliver.rejectedOutside == 0,
        "Tiny plantable fragments must not exhaust attempts sampling an entire 2 km tile");
    ok &= check(tileOutcome(plantedSliver) == TileOutcome::Bake, "Nonempty tile must be baked");
    const double farX = -11000*2048.0, farZ = -14000*2048.0;
    const QPainterPath farSliver = footprint(farX+1000, farZ, 0.25, 200, 0);
    ForestPlantingBoundary farTile;
    farTile.outer = {{farX-1024, farZ-1024}, {farX+1024, farZ-1024},
                    {farX+1024, farZ+1024}, {farX-1024, farZ+1024}};
    ForestGenerationSettings far = sparse;
    far.usableAreaOverride = area(farSliver);
    far.samplingRectangles = samplingRectangles(farSliver);
    far.containsPlantingPoint = [farSliver](double x, double z) { return farSliver.contains(QPointF(x, z)); };
    const auto plantedFar = ForestGenerator::generate(sparseRecipe, farTile, far);
    ok &= check(std::abs(far.usableAreaOverride-50) < 0.01
            && plantedFar.isValid() && plantedFar.candidates.size() == 1,
        "Sparse sampling and area must work at large negative route coordinates");
    const QPainterPath separated = footprint(-1000, -1000, 10, 10, 0)
        .united(footprint(1000, 1000, 20, 20, 0));
    const auto proposals = samplingRectangles(separated);
    for(int i = 0; i < proposals.size(); ++i)
        for(int j = i+1; j < proposals.size(); ++j) {
            const auto &a = proposals[i], &b = proposals[j];
            ok &= check(!QRectF(a.minimumX, a.minimumZ, a.maximumX-a.minimumX, a.maximumZ-a.minimumZ)
                    .intersects(QRectF(b.minimumX, b.minimumZ, b.maximumX-b.minimumX, b.maximumZ-b.minimumZ)),
                "Proposal boxes must not overlap and multiply candidate density");
        }
    sparse.usableAreaOverride = area(separated);
    sparse.densityPerSquareMetre = 0.02;
    sparse.samplingRectangles = proposals;
    sparse.containsPlantingPoint = [separated](double x, double z) { return separated.contains(QPointF(x, z)); };
    const auto plantedSeparated = ForestGenerator::generate(sparseRecipe, fullTile, sparse);
    ok &= check(plantedSeparated.isValid() && plantedSeparated.candidates.size() == 10,
        "Distant disconnected fragments must retain their combined population");
    for(const auto &candidate : plantedSeparated.candidates)
        ok &= check(separated.contains(QPointF(candidate.x, candidate.z)),
            "Sparse sampling must not place vegetation between disconnected forests");
    sparse.acceptsTerrain = [](double, double) { return false; };
    const auto fullyExcluded = ForestGenerator::generate(sparseRecipe, fullTile, sparse);
    ok &= check(fullyExcluded.isValid() && fullyExcluded.candidates.isEmpty()
            && fullyExcluded.rejectedTerrain > 0 && tileOutcome(fullyExcluded) == TileOutcome::Empty,
        "A fully excluded tile must be counted as empty rather than treated as an error");
    sparse.usableAreaOverride = 0.01;
    const auto belowDensity = ForestGenerator::generate(sparseRecipe, fullTile, sparse);
    ok &= check(belowDensity.isValid() && belowDensity.targetCount == 0
            && tileOutcome(belowDensity) == TileOutcome::Empty,
        "Population rounded to zero must not abort the route batch");
    ok &= check(canCommitBatch(2, 2, 1) && !canCommitBatch(2, 2, 0)
            && !canCommitBatch(1, 2, 1),
        "Mixed empty/planted tiles may commit, but all-empty or incomplete batches must preserve originals");
    settings.shouldCancel = []() { return true; };
    const auto cancelled = ForestGenerator::generate(recipe, box, settings);
    ok &= check(cancelled.cancelled && cancelled.candidates.isEmpty(), "Generation must honor Cancel");
    ok &= check(tileOutcome(cancelled) == TileOutcome::Cancelled, "Cancel must never become an empty-tile success");
    const auto invalid = ForestGenerator::generate(ForestRecipeDefinition(), box, settings);
    ok &= check(tileOutcome(invalid) == TileOutcome::Error, "Invalid generation must still abort and roll back");

    // Corridor geometry and generation reuse the same boundary/cap/spacing contract.
    const PolyVegCorridor::Samples alignment {{900, 10, 0}, {1100, 30, 0}};
    const auto corridorBoth = PolyVegCorridor::boundary(alignment, 0, 10, 40);
    const auto corridorLeft = PolyVegCorridor::boundary(alignment, 1, 10, 40);
    const auto corridorRight = PolyVegCorridor::boundary(alignment, 2, 10, 40);
    ok &= check(std::abs(area(corridorBoth)-16000) < 0.01
            && std::abs(area(corridorLeft)-8000) < 0.01
            && std::abs(area(corridorRight)-8000) < 0.01,
        "Corridor width is per side; setback must not contribute to planted area");
    ok &= check(corridorLeft.contains({1000, -30}) && !corridorLeft.contains({1000, 30})
            && corridorRight.contains({1000, 30}) && !corridorRight.contains({1000, -30})
            && !corridorBoth.contains({1000, 0}) && !corridorBoth.contains({1000, 55})
            && !corridorBoth.contains({899, 30}) && !corridorBoth.contains({1101, 30}),
        "Left/right, centre setback, outer boundary and run endpoints must be respected");
    auto reversedAlignment = alignment;
    std::reverse(reversedAlignment.begin(), reversedAlignment.end());
    const auto reversedLeft = PolyVegCorridor::boundary(reversedAlignment, 1, 10, 40);
    ok &= check(reversedLeft.contains({1000, 30}) && !reversedLeft.contains({1000, -30}),
        "Backward runs must reverse the meaning of left/right");
    ok &= check(std::abs(PolyVegCorridor::alignmentHeight(alignment, 1000, -30)-20) < 1e-9
            && std::abs(PolyVegCorridor::alignmentHeight(alignment, 900, 30)-10) < 1e-9,
        "Terrain-off planting must interpolate alignment height at the closest station");
    const PolyVegCorridor::Samples bend {{-2048, 2, -2048}, {-1948, 4, -2048}, {-1948, 6, -1948}};
    const auto bent = PolyVegCorridor::boundary(bend, 0, 10, 40);
    ok &= check(bent.contains({-2000, -2078}) && bent.contains({-1918, -2000})
            && !bent.contains({-2000, -2048}) && !bent.contains({-1948, -2000}),
        "Curved corridors at negative world coordinates must follow the alignment and retain its clear zone");
    auto duplicated = alignment;
    duplicated.insert(1, duplicated.first());
    ok &= check(std::abs(area(PolyVegCorridor::boundary(duplicated, 0, 10, 40))-16000) < 0.01
            && PolyVegCorridor::boundary({}, 0, 10, 40).isEmpty()
            && PolyVegCorridor::boundary(alignment, 0, -1, 40).isEmpty()
            && PolyVegCorridor::boundary(alignment, 0, 10, 0).isEmpty(),
        "Duplicate samples must not change the corridor; invalid settings must produce no boundary");
    ForestPlantingBoundary corridorBox;
    const auto corridorBounds = corridorBoth.boundingRect();
    corridorBox.outer = {{corridorBounds.left(), corridorBounds.top()}, {corridorBounds.right(), corridorBounds.top()},
        {corridorBounds.right(), corridorBounds.bottom()}, {corridorBounds.left(), corridorBounds.bottom()}};
    ForestGenerationSettings corridorSettings;
    corridorSettings.densityPerSquareMetre = 0.01;
    corridorSettings.maximumTrees = 50;
    corridorSettings.seed = 19;
    corridorSettings.usableAreaOverride = area(corridorBoth);
    corridorSettings.samplingRectangles = samplingRectangles(corridorBoth);
    corridorSettings.containsPlantingPoint = [corridorBoth](double x, double z) { return corridorBoth.contains({x, z}); };
    corridorSettings.plantingEdgeDistance = [corridorBoth](double x, double z) {
        return edgeDistance({x, z}, corridorBoth.toSubpathPolygons());
    };
    const auto corridorPlants = ForestGenerator::generate(recipe, corridorBox, corridorSettings);
    const auto corridorRepeat = ForestGenerator::generate(recipe, corridorBox, corridorSettings);
    ok &= check(corridorPlants.isValid() && corridorPlants.objectLimitApplied
            && corridorPlants.candidates.size() == 50 && corridorRepeat.candidates.size() == 50,
        "The corridor cap must apply once across both sides and all tiles");
    bool onFirstTile = false, onSecondTile = false;
    for(int i = 0; i < corridorPlants.candidates.size(); ++i) {
        const auto &plant = corridorPlants.candidates[i];
        onFirstTile = onFirstTile || plant.x < 1024;
        onSecondTile = onSecondTile || plant.x >= 1024;
        ok &= check(corridorBoth.contains({plant.x, plant.z})
                && plant.x == corridorRepeat.candidates[i].x && plant.z == corridorRepeat.candidates[i].z,
            "Corridor plants must stay inside the bands and reproduce their seed");
    }
    ok &= check(onFirstTile && onSecondTile, "Corridor generation must span tile boundaries");
    const PolyVegClearance::Segment sourceTrack {0,100,0, 100,100,0, 17};
    const PolyVegClearance::Segment parallelTrack {0,100,25, 100,100,25, 18};
    ok &= check(sourceTrack.blocks(50,100,10,400)
            && !sourceTrack.blocks(50,70,-10,400),
        "Ordinary recipe clearance must retain 3D distance above and below the alignment");
    ok &= check(!sourceTrack.blocks(50,100,10,400,17)
            && !sourceTrack.blocks(50,70,-10,400,17)
            && sourceTrack.blocks(50,100,10,400,18),
        "Corridor setback must replace clearance only for its followed node on either side and elevation");
    ok &= check(parallelTrack.blocks(50,100,10,400,17)
            && !parallelTrack.blocks(50,100,-10,400,17),
        "Other nearby nodes must retain recipe clearance during corridor planting");
    int leftCount = 0, rightCount = 0;
    for(const auto &plant : corridorPlants.candidates) {
        if(PolyVegCorridor::sideOf(alignment, plant.x, plant.z) == 1) ++leftCount;
        else ++rightCount;
    }
    ok &= check(leftCount > 0 && rightCount > 0,
        "Both Sides must generate candidates on both sides of the alignment");
    auto reversed = alignment;
    std::reverse(reversed.begin(), reversed.end());
    ok &= check(PolyVegCorridor::sideOf(alignment, 1000, -30) == 1
            && PolyVegCorridor::sideOf(reversed, 1000, -30) == 2,
        "Backward runs must reverse Left and Right");
    ForestPlantingBoundary ownedBoundary;
    ownedBoundary.outer = {{0,0}, {100,0}, {100,100}, {0,100}};
    ownedBoundary.holes = {{{40,40}, {60,40}, {60,60}, {40,60}}};
    const auto owned = PolyVegPlantingArea::fromBoundary(ownedBoundary);
    QPainterPath repeatArea; repeatArea.addRect(10,10,20,20);
    QPainterPath overlapArea; overlapArea.addRect(90,10,20,20);
    QPainterPath adjacentArea; adjacentArea.addRect(100,0,100,100);
    QPainterPath holeArea; holeArea.addRect(45,45,10,10);
    ok &= check(PolyVegPlantingArea::overlaps(repeatArea, owned)
            && PolyVegPlantingArea::overlaps(overlapArea, owned)
            && !PolyVegPlantingArea::overlaps(adjacentArea, owned)
            && !PolyVegPlantingArea::overlaps(holeArea, owned),
        "Duplicate-area guard must reject contained and partial overlap, allow shared edges and holes");
    ok &= check(PolyVegPlantingArea::overlaps(corridorBoth, corridorLeft)
            && !PolyVegPlantingArea::overlaps(corridorLeft, corridorRight),
        "Both Sides must conflict with an existing one-sided run while disjoint sides remain plantable");
    corridorSettings.occupiedCandidates = corridorPlants.candidates;
    const auto neighboringPlants = ForestGenerator::generate(recipe, corridorBox, corridorSettings);
    for(const auto &plant : neighboringPlants.candidates)
        for(const auto &previous : corridorPlants.candidates)
            ok &= check(std::hypot(plant.x-previous.x, plant.z-previous.z) >= 6,
                "Repeated corridor planting must preserve footprint spacing against existing raw plants");
    corridorSettings.shouldCancel = []() { return true; };
    const auto cancelledCorridor = ForestGenerator::generate(recipe, corridorBox, corridorSettings);
    ok &= check(cancelledCorridor.cancelled && cancelledCorridor.candidates.isEmpty(),
        "Cancelled corridor generation must not return a partial placement batch");

    QTemporaryDir temporary;
    QString error;
    const QString oldFile = temporary.filePath("manifest.json");
    const QString newFile = temporary.filePath("new-bake.s");
    const QByteArray original("existing manifest bytes");
    ForestBakeSession batch;
    ok &= check(temporary.isValid() && write(oldFile, original)
        && batch.rememberFile(oldFile, error) && batch.rememberFile(newFile, error)
        && write(oldFile, "changed") && write(newFile, "tile one")
        && batch.rememberFile(oldFile, error) && write(oldFile, "tile two")
        && batch.rollback(error) && read(oldFile) == original && !QFile::exists(newFile),
        "Batch rollback must restore pre-batch files, rather than a previous tile's manifest");
    ForestBakeInstance instance;
    instance.shapePath = "unused.s";
    const auto bakeCancelled = ForestPatchBaker::bake({instance}, 4,
        [](int, int) { return false; });
    ok &= check(!bakeCancelled.isValid() && bakeCancelled.patches.isEmpty(),
        "Bake cancellation must not publish partial geometry");
    ForestBakedPatch patch;
    ForestShapeMesh mesh;
    mesh.vertices.resize(1024);
    mesh.indices = {0, 1, 2};
    patch.meshes.append(mesh);
    const QString interrupted = temporary.filePath("interrupted.s");
    int callbacks = 0;
    ok &= check(write(interrupted, "original shape bytes")
            && !ForestShapeTextIO::writePatch(interrupted, patch, error,
                [&callbacks]() { return ++callbacks < 2; })
            && callbacks == 2 && read(interrupted) == "original shape bytes",
        "Cancelled serialization after opening output must preserve the original shape");
    return ok ? 0 : 1;
}

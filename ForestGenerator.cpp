#include "ForestGenerator.h"

#include <QHash>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double Epsilon = 0.0000001;
constexpr double Pi = 3.14159265358979323846;

class StableRandom {
public:
    explicit StableRandom(std::uint64_t seed)
        : state(seed + 0x9e3779b97f4a7c15ULL) {
    }

    std::uint64_t nextU64() {
        state += 0x9e3779b97f4a7c15ULL;
        std::uint64_t value = state;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    double unit() {
        return static_cast<double>(nextU64() >> 11)
            * (1.0 / 9007199254740992.0);
    }

    double range(double minimum, double maximum) {
        return minimum + (maximum - minimum) * unit();
    }

private:
    std::uint64_t state;
};

double signedRingArea(const ForestPlanRing &ring) {
    if(ring.size() < 3)
        return 0.0;
    double twiceArea = 0.0;
    for(int index = 0; index < ring.size(); ++index) {
        const ForestPlanPoint &a = ring.at(index);
        const ForestPlanPoint &b = ring.at((index + 1) % ring.size());
        twiceArea += a.x * b.z - b.x * a.z;
    }
    return twiceArea * 0.5;
}

bool pointInRing(const ForestPlanPoint &point, const ForestPlanRing &ring) {
    bool inside = false;
    for(int i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const ForestPlanPoint &a = ring.at(i);
        const ForestPlanPoint &b = ring.at(j);
        const bool crosses = ((a.z > point.z) != (b.z > point.z))
            && point.x < (b.x - a.x) * (point.z - a.z)
                / (b.z - a.z) + a.x;
        if(crosses)
            inside = !inside;
    }
    return inside;
}

bool pointInBoundary(
    const ForestPlanPoint &point,
    const ForestPlantingBoundary &boundary) {
    if(!pointInRing(point, boundary.outer))
        return false;
    for(const ForestPlanRing &hole : boundary.holes) {
        if(pointInRing(point, hole))
            return false;
    }
    return true;
}

double pointSegmentDistanceSquared(
    const ForestPlanPoint &point,
    const ForestPlanPoint &a,
    const ForestPlanPoint &b) {
    const double dx = b.x - a.x;
    const double dz = b.z - a.z;
    const double lengthSquared = dx*dx + dz*dz;
    if(lengthSquared <= Epsilon) {
        const double px = point.x - a.x;
        const double pz = point.z - a.z;
        return px*px + pz*pz;
    }
    const double t = std::clamp(
        ((point.x - a.x)*dx + (point.z - a.z)*dz) / lengthSquared,
        0.0, 1.0);
    const double px = point.x - (a.x + t*dx);
    const double pz = point.z - (a.z + t*dz);
    return px*px + pz*pz;
}

double distanceToRing(const ForestPlanPoint &point, const ForestPlanRing &ring) {
    double minimumSquared = std::numeric_limits<double>::max();
    for(int index = 0; index < ring.size(); ++index) {
        minimumSquared = std::min(minimumSquared,
            pointSegmentDistanceSquared(point, ring.at(index),
                ring.at((index + 1) % ring.size())));
    }
    return std::sqrt(minimumSquared);
}

double distanceToBoundary(
    const ForestPlanPoint &point,
    const ForestPlantingBoundary &boundary) {
    double distance = distanceToRing(point, boundary.outer);
    for(const ForestPlanRing &hole : boundary.holes)
        distance = std::min(distance, distanceToRing(point, hole));
    return distance;
}

int selectVegetation(
    const ForestRecipeDefinition &recipe,
    StableRandom &random) {
    const double selected = random.unit();
    double cumulative = 0.0;
    for(int index = 0; index < recipe.vegetation.size(); ++index) {
        cumulative += recipe.vegetation.at(index).normalizedProportion;
        if(selected <= cumulative)
            return index;
    }
    return recipe.vegetation.size() - 1;
}

qint64 cellKey(int x, int z) {
    return static_cast<qint64>(
        (static_cast<quint64>(static_cast<quint32>(x)) << 32)
        | static_cast<quint32>(z));
}

bool finitePoint(const ForestPlanPoint &point) {
    return std::isfinite(point.x) && std::isfinite(point.z);
}

} // namespace

ForestGenerationResult ForestGenerator::generate(
    const ForestRecipeDefinition &recipe,
    const ForestPlantingBoundary &boundary,
    const ForestGenerationSettings &settings) {
    ForestGenerationResult result;
    if(boundary.outer.size() < 3) {
        result.errors.append("Planting boundary requires at least three outer points.");
        return result;
    }
    for(const ForestPlanPoint &point : boundary.outer) {
        if(!finitePoint(point)) {
            result.errors.append("Planting boundary contains a non-finite point.");
            return result;
        }
    }
    for(const ForestPlanRing &hole : boundary.holes) {
        if(hole.size() < 3) {
            result.errors.append("Every planting hole requires at least three points.");
            return result;
        }
        for(const ForestPlanPoint &point : hole) {
            if(!finitePoint(point)) {
                result.errors.append("Planting hole contains a non-finite point.");
                return result;
            }
        }
    }
    if(recipe.vegetation.isEmpty()) {
        result.errors.append("Forest recipe has no vegetation entries.");
        return result;
    }
    if(!settings.rowsEnabled
            && (!std::isfinite(settings.densityPerSquareMetre)
                || settings.densityPerSquareMetre <= 0.0)) {
        result.errors.append("Population density must be positive and finite.");
        return result;
    }
    if(!std::isfinite(settings.variationScale)
            || settings.variationScale < 0.0 || settings.variationScale > 1.0) {
        result.errors.append("Variation scale must be in 0..1.");
        return result;
    }
    if(settings.rowsEnabled
            && (!std::isfinite(settings.rowWidthMetres)
                || settings.rowWidthMetres < 0.0)) {
        result.errors.append("Row width must be zero or a positive finite value.");
        return result;
    }
    if(settings.rowsEnabled
            && (!std::isfinite(settings.rowSpacingMetres)
                || settings.rowSpacingMetres < 0.0)) {
        result.errors.append("Row spacing must be zero or a positive finite value.");
        return result;
    }
    if(settings.rowsEnabled
            && !std::isfinite(settings.rowDirectionDegrees)) {
        result.errors.append("Row direction must be finite.");
        return result;
    }
    if(recipe.minimumSeparationMetres <= 0.0
            || !recipe.preventFootprintOverlap) {
        result.errors.append(
            "Recipe must enforce positive global separation and footprint overlap prevention.");
        return result;
    }

    double minimumX = boundary.outer.first().x;
    double maximumX = minimumX;
    double minimumZ = boundary.outer.first().z;
    double maximumZ = minimumZ;
    for(const ForestPlanPoint &point : boundary.outer) {
        minimumX = std::min(minimumX, point.x);
        maximumX = std::max(maximumX, point.x);
        minimumZ = std::min(minimumZ, point.z);
        maximumZ = std::max(maximumZ, point.z);
    }
    if(maximumX - minimumX <= Epsilon || maximumZ - minimumZ <= Epsilon) {
        result.errors.append("Planting boundary has no usable bounding area.");
        return result;
    }

    result.usableAreaSquareMetres = std::fabs(signedRingArea(boundary.outer));
    for(const ForestPlanRing &hole : boundary.holes)
        result.usableAreaSquareMetres -= std::fabs(signedRingArea(hole));
    if(result.usableAreaSquareMetres <= Epsilon) {
        result.errors.append("Planting boundary has no usable polygon area.");
        return result;
    }

    const double resolvedRowWidth = settings.rowWidthMetres > Epsilon
        ? settings.rowWidthMetres : recipe.minimumSeparationMetres;
    const double resolvedRowSpacing = settings.rowSpacingMetres > Epsilon
        ? settings.rowSpacingMetres : recipe.minimumSeparationMetres;
    const double requestedCount = settings.rowsEnabled
        ? result.usableAreaSquareMetres
            / (resolvedRowWidth*resolvedRowSpacing)
        : result.usableAreaSquareMetres * settings.densityPerSquareMetre;
    result.requestedCount = std::max(0,
        static_cast<int>(std::llround(requestedCount)));
    result.targetCount = result.requestedCount;
    if(settings.maximumTrees > 0 && result.targetCount > settings.maximumTrees) {
        result.targetCount = settings.maximumTrees;
        result.objectLimitApplied = true;
    }
    if(result.targetCount == 0)
        return result;

    StableRandom random(settings.seed);
    const double cellSize = recipe.minimumSeparationMetres;
    const double normalizedRowDirection = std::fmod(
        settings.rowDirectionDegrees, 360.0);
    const double rowRadians = normalizedRowDirection*Pi/180.0;
    const double rowAlongX = std::sin(rowRadians);
    const double rowAlongZ = std::cos(rowRadians);
    const double rowNormalX = std::cos(rowRadians);
    const double rowNormalZ = -std::sin(rowRadians);
    QHash<qint64, QVector<int>> occupiedCells;
    double largestAcceptedRadius = 0.0;
    const int maximumAttempts = std::max(1000, result.targetCount * 100);

    while(result.candidates.size() < result.targetCount
            && result.attempts < maximumAttempts) {
        if(settings.shouldCancel && settings.shouldCancel()) {
            result.cancelled = true;
            break;
        }
        ++result.attempts;
        if(settings.progress && (result.attempts == 1
                || result.attempts % 1000 == 0))
            settings.progress(result.attempts, maximumAttempts,
                              result.candidates.size(), result.targetCount);
        ForestPlanPoint point {
            random.range(minimumX, maximumX),
            random.range(minimumZ, maximumZ)
        };
        if(settings.rowsEnabled) {
            // Use a route-global origin so rows remain continuous across
            // clipped polygon pieces and neighboring planting operations.
            const double rowCoordinate =
                point.x*rowNormalX + point.z*rowNormalZ;
            const double snappedCoordinate =
                std::round(rowCoordinate/resolvedRowWidth)*resolvedRowWidth;
            const double rowAdjustment = snappedCoordinate-rowCoordinate;
            const double alongCoordinate =
                point.x*rowAlongX + point.z*rowAlongZ;
            const double snappedAlongCoordinate =
                std::round(alongCoordinate/resolvedRowSpacing)
                    * resolvedRowSpacing;
            const double alongAdjustment =
                snappedAlongCoordinate-alongCoordinate;
            point.x += rowAdjustment*rowNormalX
                + alongAdjustment*rowAlongX;
            point.z += rowAdjustment*rowNormalZ
                + alongAdjustment*rowAlongZ;
        }
        if(!pointInBoundary(point, boundary)) {
            ++result.rejectedOutside;
            continue;
        }
        if(recipe.edgeFeatherMetres > Epsilon) {
            const double edgeAcceptance = std::clamp(
                distanceToBoundary(point, boundary) / recipe.edgeFeatherMetres,
                0.0, 1.0);
            if(random.unit() > edgeAcceptance) {
                ++result.rejectedEdgeFeather;
                continue;
            }
        }
        if(settings.acceptsTerrain && !settings.acceptsTerrain(point.x, point.z)) {
            ++result.rejectedTerrain;
            continue;
        }

        const int vegetationIndex = selectVegetation(recipe, random);
        const ForestVegetationDefinition &vegetation =
            recipe.vegetation.at(vegetationIndex);
        const double scaleMidpoint =
            (vegetation.uniformScale.minimum + vegetation.uniformScale.maximum) * 0.5;
        const double scaleMinimum = scaleMidpoint
            + (vegetation.uniformScale.minimum - scaleMidpoint)
                * settings.variationScale;
        const double scaleMaximum = scaleMidpoint
            + (vegetation.uniformScale.maximum - scaleMidpoint)
                * settings.variationScale;
        const double scale = random.range(scaleMinimum, scaleMaximum);
        const double radius = vegetation.footprintRadiusMetres * scale;

        const int cellX = static_cast<int>(std::floor(point.x / cellSize));
        const int cellZ = static_cast<int>(std::floor(point.z / cellSize));
        const int searchRadius = static_cast<int>(std::ceil(
            std::max(recipe.minimumSeparationMetres,
                radius + largestAcceptedRadius) / cellSize));
        bool occupied = false;
        for(int dz = -searchRadius; dz <= searchRadius && !occupied; ++dz) {
            for(int dx = -searchRadius; dx <= searchRadius && !occupied; ++dx) {
                const auto nearbyIt = occupiedCells.constFind(
                    cellKey(cellX + dx, cellZ + dz));
                if(nearbyIt == occupiedCells.constEnd())
                    continue;
                const QVector<int> &nearby = nearbyIt.value();
                for(int acceptedIndex : nearby) {
                    const ForestCandidate &accepted = result.candidates.at(acceptedIndex);
                    const double offsetX = point.x - accepted.x;
                    const double offsetZ = point.z - accepted.z;
                    const double requiredDistance = std::max(
                        recipe.minimumSeparationMetres,
                        radius + accepted.scaledFootprintRadiusMetres);
                    if(offsetX*offsetX + offsetZ*offsetZ
                            < requiredDistance*requiredDistance) {
                        occupied = true;
                        break;
                    }
                }
            }
        }
        if(occupied) {
            ++result.rejectedOccupied;
            continue;
        }

        ForestCandidate candidate;
        candidate.vegetationIndex = vegetationIndex;
        candidate.vegetationId = vegetation.id;
        candidate.shape = vegetation.shape;
        candidate.stratum = vegetation.stratum;
        candidate.x = point.x;
        candidate.z = point.z;
        candidate.yawDegrees = random.range(
            vegetation.yawDegrees.minimum, vegetation.yawDegrees.maximum);
        candidate.uniformScale = scale;
        candidate.scaledFootprintRadiusMetres = radius;
        const int candidateIndex = result.candidates.size();
        result.candidates.append(candidate);
        occupiedCells[cellKey(cellX, cellZ)].append(candidateIndex);
        largestAcceptedRadius = std::max(largestAcceptedRadius, radius);
    }
    if(settings.progress)
        settings.progress(result.attempts, maximumAttempts,
                          result.candidates.size(), result.targetCount);

    return result;
}

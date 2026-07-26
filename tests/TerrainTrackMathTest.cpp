#include "../TerrainTrackMath.h"

#include <cassert>
#include <cmath>
#include <iostream>

static bool almostEqual(float a, float b, float tolerance = 0.001f) {
    return std::fabs(a - b) <= tolerance;
}

int main() {
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(1), 8.0f));
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(2), 12.0f));

    const float track[] = {
        0.0f, 10.0f, 0.0f,
        80.0f, 20.0f, 0.0f
    };

    float distance = 0.0f;
    float height = 0.0f;
    bool found = TerrainTrackMath::nearestTrack(track, 6, 40.0f, 8.0f, distance, height);
    assert(found);
    assert(almostEqual(distance, 8.0f));
    assert(almostEqual(height, 15.0f));

    TerrainTrackMath::Bounds bounds = TerrainTrackMath::boundsForTrack(track, 6);
    assert(almostEqual(bounds.minX, 0.0f));
    assert(almostEqual(bounds.maxX, 80.0f));
    assert(almostEqual(bounds.minZ, 0.0f));
    assert(almostEqual(bounds.maxZ, 0.0f));

    float bed = TerrainTrackMath::bedHalfWidth(2);
    float radius = TerrainTrackMath::conformInfluenceRadius(bed, 10);
    float gentleShoulder = TerrainTrackMath::shoulderWidth(radius, bed, 1);
    float steepShoulder = TerrainTrackMath::shoulderWidth(radius, bed, 10);
    assert(gentleShoulder > steepShoulder);
    assert(steepShoulder >= TerrainTrackMath::MinShoulderWidth);

    float start = TerrainTrackMath::smoothStart(bed);
    float smoothRadius = TerrainTrackMath::smoothInfluenceRadius(bed, 2);
    assert(smoothRadius >= start + TerrainTrackMath::GridSize);

    std::cout << "TerrainTrackMathTest passed\n";
    return 0;
}

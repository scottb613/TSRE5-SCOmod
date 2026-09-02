// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

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
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(1, 4.0f), 4.0f));
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(2, 4.0f), 6.0f));
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(3, 4.0f), 8.0f));
    assert(almostEqual(TerrainTrackMath::bedHalfWidth(7, 4.0f), 16.0f));
    assert(almostEqual(TerrainTrackMath::conformInfluenceRadius(8.0f, 1, 4.0f), 12.0f));
    assert(almostEqual(TerrainTrackMath::conformInfluenceRadius(8.0f, 1, 8.0f), 16.0f));

    float cutHeight = TerrainTrackMath::conformEnvelopeHeight(
        150.0f, 100.0f, 12.0f, 8.0f, 4.0f, 10, 10);
    assert(almostEqual(cutHeight, 110.0f));
    assert(almostEqual(TerrainTrackMath::conformEnvelopeHeight(
        cutHeight, 100.0f, 12.0f, 8.0f, 4.0f, 10, 10), cutHeight));
    float embankmentHeight = TerrainTrackMath::conformEnvelopeHeight(
        50.0f, 100.0f, 12.0f, 8.0f, 4.0f, 10, 10);
    assert(almostEqual(embankmentHeight, 90.0f));
    assert(almostEqual(TerrainTrackMath::conformEnvelopeHeight(
        105.0f, 100.0f, 12.0f, 8.0f, 4.0f, 10, 10), 105.0f));
    assert(almostEqual(TerrainTrackMath::conformEnvelopeHeight(
        150.0f, 100.0f, 8.0f, 8.0f, 4.0f, 10, 10), 100.0f));

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

    assert(TerrainTrackMath::tileOffsetForCoordinate(-1024.0f) == 0);
    assert(TerrainTrackMath::tileOffsetForCoordinate(1024.0f) == 1);
    assert(TerrainTrackMath::tileOffsetForCoordinate(-1025.0f) == -1);

    int first4m = TerrainTrackMath::firstNativeSampleIndex(-16.0f, 0, 4);
    int last4m = TerrainTrackMath::lastNativeSampleIndex(16.0f, 0, 4, 512);
    assert(first4m == 252);
    assert(last4m == 260);
    assert(last4m - first4m + 1 == 9);
    assert(almostEqual(TerrainTrackMath::nativeSampleCoordinate(0, first4m, 4), -16.0f));

    int first8m = TerrainTrackMath::firstNativeSampleIndex(-16.0f, 0, 8);
    int last8m = TerrainTrackMath::lastNativeSampleIndex(16.0f, 0, 8, 256);
    assert(first8m == 126);
    assert(last8m == 130);
    assert(last8m - first8m + 1 == 5);
    assert(almostEqual(TerrainTrackMath::nativeSampleCoordinate(0, first8m, 8), -16.0f));

    assert(almostEqual(TerrainTrackMath::distanceBetweenTilePositionsXZ(
        10, 20, 1000.0f, 30.0f,
        10, 20, 1006.0f, 38.0f), 10.0f));
    assert(almostEqual(TerrainTrackMath::distanceBetweenTilePositionsXZ(
        10, 20, 1018.0f, 30.0f,
        11, 20, -1022.0f, 30.0f), 8.0f));
    assert(almostEqual(TerrainTrackMath::distanceBetweenTilePositionsXZ(
        10, 20, 30.0f, -1018.0f,
        10, 19, 30.0f, 1022.0f), 8.0f));

    std::cout << "TerrainTrackMathTest passed\n";
    return 0;
}

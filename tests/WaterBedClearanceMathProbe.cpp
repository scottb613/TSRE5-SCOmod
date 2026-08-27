#include "../TSREvcWIP/WaterBedClearanceMath.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool almostEqual(float first, float second, float tolerance = 0.0001f) {
    return std::fabs(first - second) <= tolerance;
}

}

int main() {
    const float flatLevels[4] = {50.0f, 50.0f, 50.0f, 50.0f};
    assert(almostEqual(
        WaterBedClearanceMath::bilinearWaterHeight(flatLevels, 0.0f, 0.0f),
        50.0f));

    const float slopedLevels[4] = {40.0f, 44.0f, 48.0f, 52.0f};
    assert(almostEqual(
        WaterBedClearanceMath::bilinearWaterHeight(
            slopedLevels, -1024.0f, -1024.0f), 40.0f));
    assert(almostEqual(
        WaterBedClearanceMath::bilinearWaterHeight(
            slopedLevels, 1024.0f, 1024.0f), 52.0f));
    assert(almostEqual(
        WaterBedClearanceMath::bilinearWaterHeight(slopedLevels, 0.0f, 0.0f),
        46.0f));

    assert(almostEqual(
        WaterBedClearanceMath::shoreTaperFactor(true), 0.5f));
    assert(almostEqual(
        WaterBedClearanceMath::shoreTaperFactor(false), 1.0f));

    // Definite island/bank cores and adequately deep terrain never move.
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        50.3f, 50.0f, 0.25f, 1.0f), 0.0f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        49.75f, 50.0f, 0.25f, 1.0f), 0.0f));

    // Near-level posts are always lowered. A water-mask edge only reduces the
    // correction; it cannot suppress it completely.
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        50.0f, 50.0f, 0.25f, 1.0f), 0.25f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        50.1f, 50.0f, 0.25f, 1.0f), 0.35f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        50.1f, 50.0f, 0.25f, 0.5f), 0.225f));

    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        49.95f, 50.0f, 0.25f, 1.0f), 0.20f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        49.95f, 50.0f, 0.25f, 0.5f), 0.075f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        49.875f, 50.0f, 0.25f, 0.5f), 0.0f));
    assert(almostEqual(WaterBedClearanceMath::shallowBedDrop(
        49.95f, 50.0f, 0.25f, 0.0f), 0.0f));

    assert(WaterBedClearanceMath::terrainIsSubmerged(true, 9.0f, 10.0f));
    assert(!WaterBedClearanceMath::terrainIsSubmerged(false, 9.0f, 10.0f));
    assert(!WaterBedClearanceMath::terrainIsSubmerged(true, 10.0f, 10.0f));

    std::cout << "WaterBedClearanceMathProbe passed\n";
    return 0;
}

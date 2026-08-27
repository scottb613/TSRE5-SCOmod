#ifndef WATERBEDCLEARANCEMATH_H
#define WATERBEDCLEARANCEMATH_H

#include <algorithm>
#include <cmath>

namespace WaterBedClearanceMath {

inline float bilinearWaterHeight(const float levels[4],
                                 float localX, float localZ) {
    const float x = localX + 1024.0f;
    const float z = localZ + 1024.0f;
    const float inv = 1.0f / (2048.0f * 2048.0f);
    return (x * z) * inv * levels[3]
         + ((2048.0f - x) * z) * inv * levels[2]
         + ((2048.0f - x) * (2048.0f - z)) * inv * levels[0]
         + (x * (2048.0f - z)) * inv * levels[1];
}

inline float shoreTaperFactor(bool waterMaskEdge) {
    // A mask edge receives a smaller but still useful clearance. Definite
    // land cores are rejected separately by elevation.
    return waterMaskEdge ? 0.5f : 1.0f;
}

inline float shallowBedDrop(float terrainHeight, float waterHeight,
                            float clearance,
                            float taperFactor) {
    const float depth = waterHeight - terrainHeight;
    if(depth <= -clearance || depth >= clearance)
        return 0.0f;
    return std::max(0.0f, clearance
        * std::clamp(taperFactor, 0.0f, 1.0f) - depth);
}

inline bool terrainIsSubmerged(bool waterPatchVisible,
                               float terrainHeight, float waterHeight) {
    return waterPatchVisible && std::isfinite(terrainHeight)
        && std::isfinite(waterHeight) && terrainHeight < waterHeight;
}

} // namespace WaterBedClearanceMath

#endif // WATERBEDCLEARANCEMATH_H

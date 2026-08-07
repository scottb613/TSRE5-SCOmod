#ifndef TERRAINTRACKMATH_H
#define TERRAINTRACKMATH_H

#include <algorithm>
#include <cmath>
#include <limits>

namespace TerrainTrackMath {

const float GridSize = 8.0f;
const float MinShoulderWidth = GridSize;
const float MaxShoulderFeather = GridSize * 1.5f;

struct Bounds {
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

inline float clamp01(float value) {
    if(value < 0.0f) return 0.0f;
    if(value > 1.0f) return 1.0f;
    return value;
}

inline float smoothStep(float value) {
    value = clamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

inline float bedHalfWidth(int eSize) {
    return std::max(8.0f, 4.0f + eSize * 4.0f);
}

inline float conformInfluenceRadius(float bedWidth, int eRadius) {
    return std::max(bedWidth + GridSize, eRadius * GridSize);
}

inline float smoothStart(float bedWidth) {
    return bedWidth + GridSize;
}

inline float smoothInfluenceRadius(float bedWidth, int eRadius) {
    float start = smoothStart(bedWidth);
    return std::max(start + GridSize, bedWidth + eRadius * GridSize);
}

inline float shoulderWidth(float influenceRadius, float bedWidth, int sliderValue) {
    float steepness = clamp01((float)sliderValue / 10.0f);
    float minimumWidth = MinShoulderWidth + (MaxShoulderFeather - MinShoulderWidth) * (1.0f - steepness);
    float fullWidth = std::max(minimumWidth, influenceRadius - bedWidth);
    return std::max(minimumWidth, fullWidth * (1.0f - steepness) * (1.0f - steepness));
}

inline Bounds boundsForTrack(const float* points, int length) {
    Bounds bounds;
    bounds.minX = points[0];
    bounds.maxX = points[0];
    bounds.minZ = points[2];
    bounds.maxZ = points[2];

    for(int i = 3; i < length; i += 3) {
        bounds.minX = std::min(bounds.minX, points[i]);
        bounds.maxX = std::max(bounds.maxX, points[i]);
        bounds.minZ = std::min(bounds.minZ, points[i + 2]);
        bounds.maxZ = std::max(bounds.maxZ, points[i + 2]);
    }

    return bounds;
}

inline int gridStart(float value, float radius) {
    return (int)std::floor((value - radius) / GridSize) * (int)GridSize;
}

inline int gridEnd(float value, float radius) {
    return (int)std::ceil((value + radius) / GridSize) * (int)GridSize;
}

inline bool nearestTrack(const float* points, int length, float x, float z, float& distance, float& height) {
    distance = std::numeric_limits<float>::max();
    height = 0.0f;

    if(points == 0 || length < 3)
        return false;

    if(length == 3) {
        float dx = x - points[0];
        float dz = z - points[2];
        distance = std::sqrt(dx * dx + dz * dz);
        height = points[1];
        return true;
    }

    for(int i = 0; i < length - 3; i += 3) {
        float ax = points[i];
        float ay = points[i + 1];
        float az = points[i + 2];
        float bx = points[i + 3];
        float by = points[i + 4];
        float bz = points[i + 5];
        float sx = bx - ax;
        float sz = bz - az;
        float len2 = sx * sx + sz * sz;
        if(len2 < 0.0001f)
            continue;

        float t = ((x - ax) * sx + (z - az) * sz) / len2;
        if(t < 0.0f) t = 0.0f;
        if(t > 1.0f) t = 1.0f;

        float px = ax + sx * t;
        float pz = az + sz * t;
        float dx = x - px;
        float dz = z - pz;
        float d = std::sqrt(dx * dx + dz * dz);
        if(d < distance) {
            distance = d;
            height = ay + (by - ay) * t;
        }
    }

    return distance < std::numeric_limits<float>::max();
}

}

#endif

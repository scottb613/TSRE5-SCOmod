#ifndef TERRAINTRACKMATH_H
#define TERRAINTRACKMATH_H

#include <algorithm>
#include <cmath>
#include <limits>

namespace TerrainTrackMath {

const float GridSize = 8.0f;
const float MinShoulderWidth = GridSize;
const float MaxShoulderFeather = GridSize * 1.5f;
const float TileSize = 2048.0f;
const float TileHalfSize = TileSize * 0.5f;

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

inline float nativeGridSize(float sampleSize) {
    return sampleSize > 0.0f ? sampleSize : GridSize;
}

inline float bedHalfWidth(int eSize, float sampleSize = GridSize) {
    const float gridSize = nativeGridSize(sampleSize);
    return std::max(gridSize,
        gridSize * 0.5f + std::max(1, eSize) * gridSize * 0.5f);
}

inline float conformInfluenceRadius(float bedWidth, int eRadius,
                                    float sampleSize = GridSize) {
    const float gridSize = nativeGridSize(sampleSize);
    return std::max(bedWidth + gridSize,
                    std::max(1, eRadius) * gridSize);
}

inline float smoothStart(float bedWidth, float sampleSize = GridSize) {
    return bedWidth + nativeGridSize(sampleSize);
}

inline float smoothInfluenceRadius(float bedWidth, int eRadius,
                                   float sampleSize = GridSize) {
    const float gridSize = nativeGridSize(sampleSize);
    float start = smoothStart(bedWidth, gridSize);
    return std::max(start + gridSize,
                    bedWidth + std::max(1, eRadius) * gridSize);
}

inline float shoulderWidth(float influenceRadius, float bedWidth,
                           int sliderValue, float sampleSize = GridSize) {
    const float gridSize = nativeGridSize(sampleSize);
    float steepness = clamp01((float)sliderValue / 10.0f);
    float minimumWidth = gridSize + gridSize * 0.5f * (1.0f - steepness);
    float fullWidth = std::max(minimumWidth, influenceRadius - bedWidth);
    return std::max(minimumWidth, fullWidth * (1.0f - steepness) * (1.0f - steepness));
}

inline float conformEnvelopeHeight(float originalHeight, float trackHeight,
                                   float distance, float bedWidth,
                                   float sampleSize, int cuttingPerPost,
                                   int embankmentPerPost) {
    if(distance <= bedWidth)
        return trackHeight;

    const float gridSize = nativeGridSize(sampleSize);
    const float postsOutsideBed = (distance - bedWidth) / gridSize;
    if(originalHeight > trackHeight){
        const float maximumHeight = trackHeight
            + postsOutsideBed * std::max(0, cuttingPerPost);
        return std::min(originalHeight, maximumHeight);
    }
    if(originalHeight < trackHeight){
        const float minimumHeight = trackHeight
            - postsOutsideBed * std::max(0, embankmentPerPost);
        return std::max(originalHeight, minimumHeight);
    }
    return originalHeight;
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

inline int tileOffsetForCoordinate(float value) {
    return static_cast<int>(std::floor((value + TileHalfSize) / TileSize));
}

inline float tileMinimumCoordinate(int tileOffset) {
    return tileOffset * TileSize - TileHalfSize;
}

inline int firstNativeSampleIndex(float minimum, int tileOffset, int sampleSize) {
    if(sampleSize <= 0)
        return 0;
    return std::max(0, static_cast<int>(std::ceil(
        (minimum - tileMinimumCoordinate(tileOffset)) / sampleSize)));
}

inline int lastNativeSampleIndex(float maximum, int tileOffset,
                                 int sampleSize, int samples) {
    if(sampleSize <= 0 || samples <= 0)
        return -1;
    return std::min(samples - 1, static_cast<int>(std::floor(
        (maximum - tileMinimumCoordinate(tileOffset)) / sampleSize)));
}

inline float nativeSampleCoordinate(int tileOffset, int sampleIndex,
                                    int sampleSize) {
    return tileMinimumCoordinate(tileOffset) + sampleIndex * sampleSize;
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

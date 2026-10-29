// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#ifndef CURSOR_TERRAIN_PICK_H
#define CURSOR_TERRAIN_PICK_H

#include <algorithm>
#include <cmath>
#include <limits>

namespace CursorTerrainPick {
struct Point { double x, y, z; };

// Normalize across any number of 2048m world tiles, not just one boundary.
inline bool normalizeAxis(int baseTile, double position, int &tile, float &local) {
    if(!std::isfinite(position))
        return false;
    const double shift = std::floor((position + 1024.0) / 2048.0);
    const double result = double(baseTile) + shift;
    if(result < std::numeric_limits<int>::min()
            || result > std::numeric_limits<int>::max())
        return false;
    tile = int(result);
    local = float(position - shift * 2048.0);
    // Float conversion can round a point just below a boundary onto it.
    if(local >= 1024.0f) {
        if(tile == std::numeric_limits<int>::max()) return false;
        ++tile;
        local -= 2048.0f;
    }
    return true;
}

// Sample only loaded terrain; never bridge an unknown-height interval.
// A one-metre ray step is below the supported 4m/8m terrain sample spacing.
template<class HeightSample>
bool trace(Point origin, Point direction, double limit,
           HeightSample sampleHeight, Point &hit) {
    const double length = std::sqrt(direction.x * direction.x
        + direction.y * direction.y + direction.z * direction.z);
    if(!std::isfinite(length) || length <= 0.0
            || !std::isfinite(limit) || limit <= 0.0 || limit > 65536.0
            || !std::isfinite(origin.x) || !std::isfinite(origin.y)
            || !std::isfinite(origin.z))
        return false;
    direction = {direction.x / length, direction.y / length, direction.z / length};
    auto pointAt = [&](double t) {
        return Point{origin.x + direction.x * t,
                     origin.y + direction.y * t, origin.z + direction.z * t};
    };
    bool previousValid = false;
    double previousT = 0.0, previousDelta = 0.0;
    const int count = static_cast<int>(std::ceil(limit));
    for(int i = 0; i <= count; ++i) {
        const double t = std::min(double(i), limit);
        const Point point = pointAt(t);
        double height;
        if(!sampleHeight(point.x, point.z, height) || !std::isfinite(height)) {
            previousValid = false;
            continue;
        }
        const double delta = point.y - height;
        if(previousValid && previousDelta >= 0.0 && delta <= 0.0) {
            double lo = previousT, hi = t;
            for(int j = 0; j < 20; ++j) {
                const double mid = (lo + hi) * 0.5;
                const Point candidate = pointAt(mid);
                if(!sampleHeight(candidate.x, candidate.z, height)
                        || !std::isfinite(height))
                    return false;
                if(candidate.y > height) lo = mid;
                else hi = mid;
            }
            hit = pointAt((lo + hi) * 0.5);
            return true;
        }
        previousValid = true;
        previousT = t;
        previousDelta = delta;
    }
    return false;
}
}
#endif

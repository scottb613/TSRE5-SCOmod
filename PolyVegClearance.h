// TSRE GenX PolyVeg database clearance. GPL v3 or later.
#ifndef POLYVEGCLEARANCE_H
#define POLYVEGCLEARANCE_H
#include <algorithm>

namespace PolyVegClearance {
struct Segment {
    double ax, ay, az;
    double bx, by, bz;
    int node;

    bool blocks(double x, double y, double z, double clearanceSquared,
                int corridorSourceNode = -1) const {
        // The followed alignment is governed by the explicit corridor setback.
        // Other database nodes still use the recipe's 3D clearance.
        if(corridorSourceNode >= 0 && node == corridorSourceNode) return false;
        const double vx = bx-ax, vy = by-ay, vz = bz-az;
        const double lengthSquared = vx*vx+vy*vy+vz*vz;
        const double t = lengthSquared > 0.000001
            ? std::clamp(((x-ax)*vx+(y-ay)*vy+(z-az)*vz)/lengthSquared, 0.0, 1.0) : 0.0;
        const double dx = x-(ax+vx*t), dy = y-(ay+vy*t), dz = z-(az+vz*t);
        return dx*dx+dy*dy+dz*dz < clearanceSquared;
    }
};
}
#endif

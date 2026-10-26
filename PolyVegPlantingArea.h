// TSRE GenX PolyVeg planting-area ownership. GPL v3 or later.
#ifndef POLYVEGPLANTINGAREA_H
#define POLYVEGPLANTINGAREA_H
#include "ForestReplacementPlan.h"

namespace PolyVegPlantingArea {
inline QPainterPath fromBoundary(const ForestPlantingBoundary &boundary) {
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    auto addRing = [&path](const ForestPlanRing &ring) {
        if(ring.size() < 3) return;
        path.moveTo(ring.first().x, ring.first().z);
        for(int i = 1; i < ring.size(); ++i) path.lineTo(ring[i].x, ring[i].z);
        path.closeSubpath();
    };
    addRing(boundary.outer);
    for(const auto &hole : boundary.holes) addRing(hole);
    return path;
}
inline bool overlaps(const QPainterPath &requested, const QPainterPath &occupied) {
    // Shared edges are allowed; only a positive-area overlap is a duplicate.
    return requested.boundingRect().intersects(occupied.boundingRect())
        && ForestReplacementPlan::area(requested.intersected(occupied)) > 0.0001;
}
}
#endif

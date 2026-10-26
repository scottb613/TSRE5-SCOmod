// TSRE GenX PolyVeg corridor geometry. GPL v3 or later.
#ifndef POLYVEGCORRIDOR_H
#define POLYVEGCORRIDOR_H

#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPolygonF>
#include <QLineF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>

namespace PolyVegCorridor {
// Samples use the editor's world X/Z convention and source alignment height.
struct Sample {
    double px = 0, py = 0, pz = 0;
    double x() const { return px; }
    double y() const { return py; }
    double z() const { return pz; }
};
using Samples = QVector<Sample>;

inline QPainterPath boundary(const Samples &samples, int side, double setback, double width) {
    QPainterPath result;
    result.setFillRule(Qt::WindingFill);
    if(samples.size() < 2 || side < 0 || side > 2 || !std::isfinite(setback)
            || !std::isfinite(width) || setback < 0 || width <= 0) return result;
    QVector<QPointF> normals;
    QVector<QPointF> points;
    for(const auto &sample : samples) {
        if(!std::isfinite(sample.x()) || !std::isfinite(sample.y()) || !std::isfinite(sample.z())) return {};
        const QPointF point(sample.x(), sample.z());
        if(points.isEmpty() || QLineF(points.last(), point).length() > 0.001) points.append(point);
    }
    if(points.size() < 2) return result;
    for(int i = 1; i < points.size(); ++i) {
        const QPointF delta = points[i]-points[i-1];
        const double length = std::hypot(delta.x(), delta.y());
        // Left in world X/Z (north is negative Z).
        normals.append(QPointF(delta.y()/length, -delta.x()/length));
    }
    auto addQuad = [&result](const QPointF &a, const QPointF &b,
                              const QPointF &c, const QPointF &d) {
        QPolygonF polygon{a, b, c, d};
        double twiceArea = 0;
        for(int i = 0; i < polygon.size(); ++i) {
            const QPointF p = polygon[i]-a, q = polygon[(i+1)%polygon.size()]-a;
            twiceArea += p.x()*q.y()-q.x()*p.y();
        }
        if(std::abs(twiceArea) < 1e-9) return;
        if(twiceArea < 0) std::reverse(polygon.begin(), polygon.end());
        result.addPolygon(polygon);
        result.closeSubpath();
    };
    for(int sign : {1, -1}) {
        if((side == 1 && sign < 0) || (side == 2 && sign > 0)) continue;
        const double inner = sign*setback, outer = sign*(setback+width);
        for(int i = 0; i < normals.size(); ++i) {
            const QPointF a = points[i], b = points[i+1], n = normals[i];
            addQuad(a+n*inner, b+n*inner, b+n*outer, a+n*outer);
            if(i > 0) {
                // Bevel joins avoid unbounded mitres on sharp track bends.
                const QPointF previous = normals[i-1];
                addQuad(a+previous*inner, a+n*inner, a+n*outer, a+previous*outer);
            }
        }
    }
    result = result.simplified();
    // A looping alignment's other legs must retain the same centre clearance.
    if(setback > 0) {
        QPainterPath centre;
        centre.moveTo(points.first());
        for(int i = 1; i < points.size(); ++i) centre.lineTo(points[i]);
        QPainterPathStroker stroker;
        stroker.setWidth(2*setback);
        stroker.setCapStyle(Qt::FlatCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        result = result.subtracted(stroker.createStroke(centre));
    }
    return result.simplified();
}

// Classify against the nearest sampled segment, using the same left normal
// as the corridor boundary. Reversing a run reverses the reported side.
inline int sideOf(const Samples &samples, double x, double z) {
    double nearest = std::numeric_limits<double>::infinity();
    int side = 0;
    for(int i = 1; i < samples.size(); ++i) {
        const auto &a = samples[i-1]; const auto &b = samples[i];
        const double dx = b.x()-a.x(), dz = b.z()-a.z();
        const double length2 = dx*dx+dz*dz;
        if(length2 <= 0.000001) continue;
        const double t = std::clamp(((x-a.x())*dx+(z-a.z())*dz)/length2, 0.0, 1.0);
        const double ex = x-a.x()-t*dx, ez = z-a.z()-t*dz;
        const double distance2 = ex*ex+ez*ez;
        if(distance2 < nearest) {
            nearest = distance2;
            side = ex*dz-ez*dx >= 0 ? 1 : 2;
        }
    }
    return side;
}

inline double alignmentHeight(const Samples &samples, double x, double z) {
    double nearest = std::numeric_limits<double>::infinity();
    double height = std::numeric_limits<double>::quiet_NaN();
    for(int i = 1; i < samples.size(); ++i) {
        const auto &a = samples[i-1]; const auto &b = samples[i];
        const double dx = b.x()-a.x(), dz = b.z()-a.z();
        const double length2 = dx*dx+dz*dz;
        if(length2 <= 1e-12) continue;
        const double t = std::clamp(((x-a.x())*dx+(z-a.z())*dz)/length2, 0.0, 1.0);
        const double ex = x-a.x()-t*dx, ez = z-a.z()-t*dz;
        const double distance2 = ex*ex+ez*ez;
        if(distance2 < nearest) {
            nearest = distance2;
            height = a.y()+t*(b.y()-a.y());
        }
    }
    return height;
}
}
#endif

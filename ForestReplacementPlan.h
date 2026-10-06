// TSRE GenX forest replacement geometry. Licensed under GPL v3 or later.
#ifndef FORESTREPLACEMENTPLAN_H
#define FORESTREPLACEMENTPLAN_H

#include "ForestGenerator.h"
#include <QPainterPath>
#include <QPolygonF>
#include <QPair>
#include <algorithm>
#include <cmath>
#include <limits>

namespace ForestReplacementPlan {
using TileCoordinate = QPair<int, int>;

inline QString bakeShapeName(int x, int z, int blockX, int blockZ) {
    auto signedTile = [](int value) {
        return QString("%1%2").arg(value < 0 ? '-' : '+')
            .arg(std::abs(value), 5, 10, QLatin1Char('0'));
    };
    return QString("V%1%2-%3%4.s").arg(signedTile(x), signedTile(-z))
        .arg(blockX).arg(blockZ);
}

inline QPainterPath footprint(double centreX, double centreZ,
                              double width, double depth, double yawRadians) {
    QPolygonF corners;
    const double c = std::cos(yawRadians), s = std::sin(yawRadians);
    for(const QPointF &corner : {QPointF(-width/2, -depth/2),
            QPointF(width/2, -depth/2), QPointF(width/2, depth/2),
            QPointF(-width/2, depth/2)})
        corners.append({centreX + c*corner.x() + s*corner.y(),
                        centreZ - s*corner.x() + c*corner.y()});
    QPainterPath path;
    path.addPolygon(corners);
    path.closeSubpath();
    path.setFillRule(Qt::WindingFill);
    return path;
}

inline double area(const QPainterPath &path) {
    double result = 0;
    // Fill polygons insert connecting edges and need not reverse hole winding.
    // Use simple contours and nesting depth instead of assuming their orientation.
    const auto rings = path.simplified().toSubpathPolygons();
    for(int ringIndex = 0; ringIndex < rings.size(); ++ringIndex) {
        const QPolygonF &polygon = rings[ringIndex];
        if(polygon.size() < 3) continue;
        double twiceArea = 0;
        const QPointF origin = polygon.first();
        QPointF probe = origin;
        double longestEdge = 0;
        for(int i = 0; i < polygon.size(); ++i) {
            const QPointF a = polygon[i]-origin;
            const QPointF b = polygon[(i+1)%polygon.size()]-origin;
            twiceArea += a.x()*b.y()-b.x()*a.y();
            const QPointF edge = b-a;
            const double lengthSquared = edge.x()*edge.x()+edge.y()*edge.y();
            if(lengthSquared > longestEdge) {
                longestEdge = lengthSquared;
                probe = origin+(a+b)/2;
            }
        }
        int depth = 0;
        for(int other = 0; other < rings.size(); ++other)
            if(other != ringIndex && rings[other].containsPoint(probe, Qt::OddEvenFill)) ++depth;
        result += (depth%2 ? -1 : 1)*std::abs(twiceArea)/2;
    }
    return result;
}

inline void merge(QVector<QPainterPath> &groups, QPainterPath path) {
    // Restart after a union: a new footprint can join multiple existing groups.
    for(int i = 0; i < groups.size();) {
        if(groups[i].boundingRect().intersects(path.boundingRect())
                && groups[i].intersects(path)) {
            path = path.united(groups[i]);
            groups.removeAt(i);
            i = 0;
        } else ++i;
    }
    groups.append(path);
}

inline QPainterPath tileRectangle(int x, int z) {
    QPainterPath path;
    path.addRect(x*2048.0-1024, z*2048.0-1024, 2048, 2048);
    return path;
}

inline QVector<ForestSamplingRectangle> samplingRectangles(const QPainterPath &path) {
    QVector<ForestSamplingRectangle> result;
    const QRectF bounds = path.boundingRect();
    // A 2 km tile has at most 8x8 proposal cells. Tight fragment bounds inside
    // disjoint cells avoid both sparse-tile starvation and double density.
    constexpr double cellSize = 256;
    const double firstX = std::floor(bounds.left()/cellSize)*cellSize;
    const double firstZ = std::floor(bounds.top()/cellSize)*cellSize;
    for(double x = firstX; x < bounds.right(); x += cellSize)
        for(double z = firstZ; z < bounds.bottom(); z += cellSize) {
            QPainterPath cell;
            cell.addRect(x, z, cellSize, cellSize);
            const QPainterPath fragment = path.intersected(cell);
            if(fragment.isEmpty()) continue;
            const QRectF box = fragment.boundingRect();
            if(box.width() > 0 && box.height() > 0)
                result.append({box.left(), box.top(), box.right(), box.bottom()});
        }
    return result;
}

enum class TileOutcome { Bake, Empty, Cancelled, Error };
inline TileOutcome tileOutcome(const ForestGenerationResult &generated) {
    if(generated.cancelled) return TileOutcome::Cancelled;
    if(!generated.isValid()) return TileOutcome::Error;
    return generated.candidates.isEmpty() ? TileOutcome::Empty : TileOutcome::Bake;
}

inline bool canCommitBatch(int processed, int total, qint64 plants) {
    return total > 0 && processed == total && plants > 0;
}

inline double edgeDistance(const QPointF &point,
                          const QList<QPolygonF> &rings) {
    double best = std::numeric_limits<double>::max();
    for(const QPolygonF &ring : rings)
        for(int i = 0; i < ring.size(); ++i) {
            const QPointF a = ring[i], b = ring[(i+1)%ring.size()];
            const QPointF d = b-a, p = point-a;
            const double length = d.x()*d.x()+d.y()*d.y();
            const double t = length > 0
                ? std::clamp((p.x()*d.x()+p.y()*d.y())/length, 0.0, 1.0) : 0;
            const QPointF offset = p-t*d;
            best = std::min(best, std::hypot(offset.x(), offset.y()));
        }
    return best;
}
} // namespace ForestReplacementPlan
#endif

// TSRE GenX texture brush sampling. Licensed under GNU GPL v3 or later.
#ifndef TSRE_TEXTURE_PAINT_MATH_H
#define TSRE_TEXTURE_PAINT_MATH_H

#include <QJsonObject>
#include <QtGlobal>

namespace TexturePaintMath {
inline int scalePercent(int percent) {
    return qBound(25, percent, 400);
}

inline int presetScalePercent(const QJsonObject &preset) {
    return scalePercent(preset["scale"].toInt(100));
}

// Scale the rotated source coordinate before rounding, then wrap negatives.
// At 100%, retain the legacy sampling exactly (including qRound at rotation).
inline int sourceCoordinate(float rotated, int percent, int limit) {
    const float scale = 100.0f / scalePercent(percent);
    const int coordinate = qRound(rotated * scale) % limit;
    return coordinate < 0 ? coordinate + limit : coordinate;
}
}

#endif

// TSRE GenX terrain display cache. Licensed under GNU GPL v3 or later.
#pragma once

#include <QString>
#include <QPointF>
#include <cstdint>

namespace TerrainTextureStream {
struct Result {
    int texture = -1;
    bool pending = false;
};

Result request(const QString &path, bool allowWork, bool detailed = false,
               QPoint tile = {}, QPointF patchCenter = {});
void setView(QPoint tile, QPointF camera);
bool wantsOffscreen(QPoint tile);
void queueOffscreen(const QString &path, QPoint tile, QPointF patchCenter);
void prefetchOffscreen(qint64 budgetNs);
std::uint64_t previewRevision();
void invalidate(const QString &path);
void clear();
}

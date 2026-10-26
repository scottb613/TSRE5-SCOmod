#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QPair>
#include <functional>

// Independent AP wire geometry/serialization. No PolyVeg data or ownership.
namespace AutoPlaceWire {
inline constexpr int MaximumSpanCount = 8192;
inline bool validWireType(const QString &type) { return type=="plain" || type=="barbed"; }
inline bool isBarbed(const QJsonObject &span) { return span["wireType"].toString("plain")=="barbed"; }
inline bool registryWithinLimit(const QJsonObject &spans, QString &error) {
    error.clear();
    if(spans.size() <= MaximumSpanCount) return true;
    error = QString("AP wire definitions exceed the %1-span limit. "
                    "Remove unused wire runs before committing another section.")
        .arg(MaximumSpanCount);
    return false;
}
// Match the selected support's name family, without its shape extension.
inline bool matchesSupportPrefix(QString candidate, QString selected) {
    if(candidate.endsWith(".s", Qt::CaseInsensitive)) candidate.chop(2);
    if(selected.endsWith(".s", Qt::CaseInsensitive)) selected.chop(2);
    return !selected.isEmpty() && candidate.startsWith(selected, Qt::CaseInsensitive);
}
inline bool isRaw(const QJsonObject &span) {
    return !span.isEmpty() && !span["baked"].toBool()
        && !span["external"].toBool() && !span["deleted"].toBool();
}
inline bool isPending(const QJsonObject &span) {
    return isRaw(span) && span["active"].toBool();
}
// Baking and its progress UI belong only to active raw work. A saved bake
// remains a duplicate reservation, not an instruction to revisit its tile.
inline QStringList pendingBakeKeys(const QJsonObject &spans) {
    QStringList keys;
    for(auto it = spans.constBegin(); it != spans.constEnd(); ++it)
        if(isPending(it.value().toObject())) keys.append(it.key());
    return keys;
}
inline bool hasPendingCleanup(const QJsonObject &spans, bool cleanupAll) {
    if(cleanupAll) return true;
    for(const auto &value : spans)
        if(value.toObject()["deleted"].toBool()) return true;
    return false;
}
struct Vertex { QVector3D point, normal; QVector2D uv{0.5f,0.5f}; };
using Mesh = QVector<Vertex>;
QVector3D vector(const QJsonValue &value);
QJsonArray json(const QVector3D &value);
Mesh mesh(const QJsonObject &span, bool distant = false);
Mesh previewMesh(const QJsonObject &span);
QString shapeName(const QString &key);
QJsonObject reconcileReservation(QJsonObject span, bool present);
bool isWireShape(const QString &name);
using CleanupProgress = std::function<void(const QString &, int, int)>;
bool findWireWorldTiles(const QString &routePath, QVector<QPair<int, int>> &tiles,
                        QString &error, const CleanupProgress &progress = {});
bool pruneAssets(const QString &routePath, QJsonObject &spans, bool all, QString &error,
                 bool allowSavedReferences = false, const CleanupProgress &progress = {});
bool discardBakes(const QString &routePath, QJsonObject &spans,
                  const QSet<QString> &keys, QString &error);
QSet<QString> connectedSpans(const QJsonObject &spans, const QString &support, int node, bool road);
bool writeShape(const QString &routePath, const QJsonObject &span, QString &error);
bool writeRegistry(const QString &path, const QJsonObject &registry, QString &error);
}

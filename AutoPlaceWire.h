#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QVector3D>
#include <QString>
#include <QSet>

// Independent AP wire geometry/serialization. No PolyVeg data or ownership.
namespace AutoPlaceWire {
inline constexpr int MaximumSpanCount = 8192;
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
struct Vertex { QVector3D point, normal; };
using Mesh = QVector<Vertex>;
QVector3D vector(const QJsonValue &value);
QJsonArray json(const QVector3D &value);
Mesh mesh(const QJsonObject &span, bool distant = false);
QString shapeName(const QString &key);
QJsonObject reconcileReservation(QJsonObject span, bool present);
bool isWireShape(const QString &name);
bool pruneAssets(const QString &routePath, QJsonObject &spans, bool all, QString &error,
                 bool allowSavedReferences = false);
bool discardBakes(const QString &routePath, QJsonObject &spans,
                  const QSet<QString> &keys, QString &error);
QSet<QString> connectedSpans(const QJsonObject &spans, const QString &support, int node, bool road);
bool writeShape(const QString &routePath, const QJsonObject &span, QString &error);
bool writeRegistry(const QString &path, const QJsonObject &registry, QString &error);
}

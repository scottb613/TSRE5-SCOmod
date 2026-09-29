#ifndef WIRE_ATTACHMENT_ORDER_H
#define WIRE_ATTACHMENT_ORDER_H

#include <QVector3D>
#include <QMap>
#include <QString>
#include <algorithm>
#include <vector>

namespace WireAttachmentOrder {
inline int attachmentId(const QString &name) {
    if(!name.startsWith("SNAP_", Qt::CaseInsensitive)) return 0;
    const QString suffix = name.mid(5);
    if(suffix.isEmpty() || suffix[0] < QLatin1Char('1') || suffix[0] > QLatin1Char('9')) return 0;
    for(QChar c : suffix)
        if(c < QLatin1Char('0') || c > QLatin1Char('9')) return 0;
    bool ok = false;
    const int id = suffix.toInt(&ok);
    return ok ? id : 0;
}

// Group in model space so wobble cannot move an insulator into another row.
// Reverse only complete matching rows, choosing the shorter total connection.
inline QMap<int, int> match(const QMap<int, QVector3D> &local, const QMap<int, QVector3D> &a,
                           const QMap<int, QVector3D> &b) {
    QMap<int, int> result;
    std::vector<int> remaining;
    for(auto it = a.cbegin(); it != a.cend(); ++it) {
        const int i = it.key();
        result[i] = i;
        remaining.push_back(i);
    }
    if(a.keys() != b.keys() || a.keys() != local.keys()) return result;
    std::sort(remaining.begin(), remaining.end(), [&local](int i, int j) {
        return local[i].y() == local[j].y() ? i < j : local[i].y() < local[j].y();
    });
    for(std::size_t begin = 0; begin < remaining.size();) {
        std::size_t end = begin + 1;
        while(end < remaining.size() && local[remaining[end]].y() - local[remaining[begin]].y() < 0.05f) ++end;
        std::vector<int> row(remaining.begin() + begin, remaining.begin() + end);
        std::sort(row.begin(), row.end(), [&local](int i, int j) {
            return local[i].x() == local[j].x() ? i < j : local[i].x() < local[j].x();
        });
        bool complete = true;
        double straight = 0, reversed = 0;
        for(std::size_t n = 0; n < row.size(); ++n) {
            complete &= b.contains(row[n]);
            straight += (a[row[n]] - b[row[n]]).lengthSquared();
            reversed += (a[row[n]] - b[row[row.size() - 1 - n]]).lengthSquared();
        }
        if(complete && reversed + 0.001 < straight)
            for(std::size_t n = 0; n < row.size(); ++n) result[row[n]] = row[row.size() - 1 - n];
        begin = end;
    }
    return result;
}
}
#endif

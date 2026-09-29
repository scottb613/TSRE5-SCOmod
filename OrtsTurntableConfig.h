// TSRE GenX. Licensed under GNU GPL v3 or later. See LICENSE.md.
#ifndef ORTSTURNTABLECONFIG_H
#define ORTSTURNTABLECONFIG_H

#include <QByteArray>
#include <QString>

namespace OrtsTurntableConfig {
// World track filenames may be relative to GLOBAL/SHAPES and lead into a route.
// This is only a candidate filter; the loaded animation/track profile is checked separately.
bool is42mShapeReference(QString fileName);
struct Entry {
    enum class Kind { Turntable, Transfer };
    Kind kind = Kind::Turntable;
    QString worldFile;
    unsigned int uid = 0;
    int shapeIndex = 0;
    QString animation;
    double x = 0, y = 0, z = 0, diameter = 0;
    double length = 0;
};
// Existing entries are never replaced, including custom rotation limits.
bool append(const QByteArray &original, const Entry &entry, QByteArray &result,
            bool &alreadyPresent, QString &error);
bool activate(const QString &routeDirectory, const Entry &entry,
              bool &alreadyPresent, QString &error);
}
#endif

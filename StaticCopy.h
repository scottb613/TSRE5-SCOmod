// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include <QByteArray>
#include <QStringList>
#include <QVector>
#include <functional>

namespace StaticCopy {
using Progress = std::function<bool(double,const QString &)>;
struct Destination {
    QString root, descriptor, refPath, name, workingRoot;
    QStringList categories, errors;
    QByteArray descriptorHash, refHash, refBytes;
    bool refExists=false, valid=false, workingFolder=false;
};
struct Source {
    QString routeRoot, shapeRoot, textureRoot;
    QStringList shapes;
    QString settings; // automatically serialized REF fields, not user input
    QString type="Static";
};
struct File {
    QString source, relative;
    QByteArray hash;
    qint64 bytes=0;
    bool reuse=false;
};
struct Plan {
    Source source;
    Destination destination;
    QVector<File> files;
    QStringList shapeNames, warnings, errors;
    qint64 bytes=0;
    bool valid=false, cancelled=false;
};
struct Result {
    QStringList copied, reused, errors;
    qint64 bytes=0;
    QString outputFolder;
    bool cancelled=false, refUpdated=false, completed=false;
};
Destination validateDestination(const QString &root, bool probeWrite=false);
Destination validateWorkingFolder(const QString &root, bool probeWrite=false);
Plan prepare(const Source &source,const Destination &destination,const Progress &progress={});
Result transfer(const Plan &plan,const QString &name,const QString &category,const Progress &progress={});
QString quote(const QString &value);
}

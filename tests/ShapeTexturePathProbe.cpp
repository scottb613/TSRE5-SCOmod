// TSRE GenX - shape texture lookup regression support.
// Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "ShapeTexturePath.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return 1;
    const QString snow = fixture.path() + "/snow";
    const QString main = fixture.path() + "/main";
    if (!QDir().mkpath(snow) || !QDir().mkpath(main))
        return 1;
    for (const QString &path : QStringList{
             main + "/tree.ace", main + "/tree.dds", snow + "/tree.ace"}) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return 1;
    }
    auto expect = [](const QString &actual, const QString &expected) {
        if (actual == expected)
            return true;
        qCritical() << "Texture lookup mismatch:" << actual << expected;
        return false;
    };
    if (!expect(ShapeTexturePath::resolve({main}, "tree.ace"), main + "/tree.dds")
        || !expect(ShapeTexturePath::resolve({snow, main}, "tree.ace"), snow + "/tree.ace")
        || !expect(ShapeTexturePath::resolve({snow, main}, "tree.dds"), snow + "/tree.ace"))
        return 1;
    QFile snowDds(snow + "/tree.dds");
    if (!snowDds.open(QIODevice::WriteOnly))
        return 1;
    snowDds.close();
    if (!expect(ShapeTexturePath::resolve({snow, main}, "tree.ace"), snow + "/tree.dds"))
        return 1;
    if (!QFile::remove(snow + "/tree.ace"))
        return 1;
    if (!expect(ShapeTexturePath::resolve({snow, main}, "tree.ace"), snow + "/tree.dds")
        || !expect(ShapeTexturePath::resolve({main}, "tree.ace", false), main + "/tree.ace")
        || !expect(ShapeTexturePath::resolve({snow, main}, "missing.ace"), snow + "/missing.ace"))
        return 1;
    if (!QFile::remove(snow + "/tree.dds"))
        return 1;
    if (!expect(ShapeTexturePath::resolve({snow, main}, "tree.ace"), main + "/tree.dds"))
        return 1;
    return 0;
}

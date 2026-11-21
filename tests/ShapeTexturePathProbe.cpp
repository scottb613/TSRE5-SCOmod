// TSRE GenX - shape texture lookup regression support.
// Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "ShapeTexturePath.h"
#include "FreightAnimPath.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

static bool expect(const QString &actual, const QString &expected) {
    if(actual == expected)
        return true;
    qCritical() << "Texture lookup mismatch:" << actual << "expected:" << expected;
    return false;
}

static bool freightAnimLookup(const QString &root) {
    const QString trainset = root + "/TRAINSET";
    const QString wagon = trainset + "/WellCar";
    const QString repaint = trainset + "/OtherRepaint";
    const QString shared = trainset + "/Amazon_Box";
    for(const QString &texture : QStringList{wagon + "/container.ace",
            wagon + "/wintersnow/container.ace", wagon + "/snow/container.dds",
            wagon + "/container.png", repaint + "/container.dds",
            shared + "/container.dds", shared + "/Subfolder/unique.ace",
            wagon + "/sub/local.ace"}) {
        if(!QDir().mkpath(QFileInfo(texture).path()))
            return false;
        QFile file(texture);
        if(!file.open(QIODevice::WriteOnly))
            return false;
    }
    const auto first = FreightAnimPath::resolve(wagon, "..\\Amazon_Box\\container.s");
    const auto second = FreightAnimPath::resolve(repaint, "../Amazon_Box/container.s");
    const auto local = FreightAnimPath::resolve(wagon, "Loads/container.s");
    const auto nested = FreightAnimPath::resolve(wagon + "/Variants", "../Loads/container.s");
    const auto container = FreightAnimPath::resolveLoad(trainset, "Amazon_Box\\Subfolder\\container.s");
    if(!expect(first.shape, shared + "/container.s")
        || !expect(second.shape, first.shape)
        || !expect(local.shape, wagon + "/Loads/container.s")
        || !expect(local.textures, wagon)
        || !expect(nested.shape, local.shape)
        || !expect(nested.textures, wagon + "/Variants")
        || !expect(container.shape, shared + "/Subfolder/container.s")
        || !expect(container.textures, shared)
        || !expect(FreightAnimPath::resolveLoad(trainset, "container.s").shape, {}))
        return false;

    using ShapeTexturePath::resolveRollingStock;
    const QString snow = ShapeTexturePath::rollingStockSeason(0x81, "snow");
    const QString winterSnow = ShapeTexturePath::rollingStockSeason(0x80, "snow");
    if(!expect(snow, "snow") || !expect(winterSnow, "wintersnow")
        || !expect(ShapeTexturePath::rollingStockSeason(0x05, "spring"), {})
        || !expect(ShapeTexturePath::rollingStockSeason(0x100, "night"), {})
        || !expect(resolveRollingStock(first.textures, {}, "container.dds"), wagon + "/container.ace")
        || !expect(resolveRollingStock(second.textures, {}, "container.ace"), repaint + "/container.dds")
        || !expect(resolveRollingStock(first.textures, snow, "container.ace"), wagon + "/snow/container.dds")
        || !expect(resolveRollingStock(first.textures, winterSnow, "container.ace"), wagon + "/wintersnow/container.ace")
        || !expect(resolveRollingStock(first.textures, {}, "..\\Amazon_Box\\container.ace"), shared + "/container.dds")
        || !expect(resolveRollingStock(first.textures, {}, "container.png"), {})
        || !expect(resolveRollingStock(first.textures, {}, "C:\\Other\\container.ace"), {})
        || !expect(resolveRollingStock(first.textures, snow, "sub/local.ace"), wagon + "/snow/sub/local.ace")
        || !expect(resolveRollingStock(container.textures, {}, "container.ace"), shared + "/container.dds")
        || !expect(resolveRollingStock(container.textures, {}, "unique.ace"), shared + "/unique.ace"))
        return false;

    // Missing seasonal textures may fall back to the wagon, never to the shared shape.
    if(!QFile::remove(wagon + "/snow/container.dds")
        || !expect(resolveRollingStock(first.textures, snow, "container.ace"), wagon + "/container.ace")
        || !QFile::remove(wagon + "/container.ace")
        || !expect(resolveRollingStock(first.textures, snow, "container.ace"), wagon + "/snow/container.ace"))
        return false;

    // Lookup stops at the selected DDS even if its contents fail to decode.
    QFile invalidDds(wagon + "/container.dds");
    QFile ace(wagon + "/container.ace");
    return invalidDds.open(QIODevice::WriteOnly) && ace.open(QIODevice::WriteOnly)
        && expect(resolveRollingStock(first.textures, {}, "container.ace"), invalidDds.fileName());
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return 1;
    if (!freightAnimLookup(fixture.path()))
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

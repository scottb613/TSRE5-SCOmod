// Ordinary saves must not claim loose vegetation assets. GPL v3 or later.
#include "ForestBakeManifest.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QDebug>

int main() {
    QTemporaryDir temporary;
    if(!temporary.isValid()) return 2;
    const QString route = temporary.path();
    int failures = 0;
    auto check = [&failures](bool ok, const char *message) {
        if(!ok) { qCritical() << message; ++failures; }
    };
    auto write = [](const QString &path, const QByteArray &contents) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    };
    QDir().mkpath(route + "/shapes");
    const QString loose = route + "/shapes/V+00001-00002-00.s";
    check(write(loose, "shared library shape"), "Create loose matching shape");
    ForestBakePruneResult result;
    QString error = "stale error";
    // No schema, manifest, OpenRails or world folder: an ordinary save has no
    // PolyVeg ownership and must not even attempt a world scan or manifest write.
    check(ForestBakeManifest::pruneUnreferenced(route, result, error, nullptr, false),
          "Save without PolyVeg must succeed");
    check(error.isEmpty() && result.removedAssets == 0, "No stale warning or cleanup");
    check(QFileInfo::exists(loose), "Ordinary save must preserve loose matching assets");
    check(!QFileInfo::exists(route + "/OpenRails"), "Do not create PolyVeg metadata");

    QDir().mkpath(route + "/world");
    const QString manifest = route + "/OpenRails/forest-bakes.json";
    QDir().mkpath(route + "/OpenRails");
    ForestBakeManifestEntry entry;
    entry.id = "tracked";
    entry.shapeFile = "V+00001-00002-01.s";
    check(ForestBakeManifest::upsert(manifest, entry, error), "Create tracked bake");
    check(write(route + "/shapes/" + entry.shapeFile, "tracked"), "Create tracked asset");
    check(ForestBakeManifest::pruneUnreferenced(route, result, error, nullptr, false),
          "Save still cleans unreferenced tracked bakes");
    check(result.removedBlocks == 1 && result.removedAssets == 1,
          "Remove only tracked bake");
    check(QFileInfo::exists(loose), "Loose asset survives with a manifest too");
    check(!QFileInfo::exists(manifest), "Empty tracked manifest removed");
    QDir().rmdir(route + "/OpenRails");
    // Explicit orphan cleanup may still remove the loose asset, without trying
    // to create the nonexistent manifest which caused the reported save error.
    check(ForestBakeManifest::pruneUnreferenced(route, result, error),
          "Explicit orphan cleanup needs no OpenRails directory");
    check(result.removedAssets == 1 && !QFileInfo::exists(loose), "Explicit cleanup removes orphan");
    check(!QFileInfo::exists(route + "/OpenRails"), "Orphan cleanup creates no metadata");
    return failures ? 1 : 0;
}

#include "ForestBakeManifest.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

#include <iostream>

namespace {
bool check(bool condition, const char *message) {
    if(condition) return true;
    std::cerr << "FAILED: " << message << '\n';
    return false;
}

bool touch(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return false;
    file.write("generated");
    return true;
}
}

int main(int argc, char **argv) {
    if(argc == 2) {
        ForestBakePruneResult result;
        QString error;
        if(!ForestBakeManifest::pruneUnreferenced(
                QString::fromLocal8Bit(argv[1]), result, error)) {
            std::cerr << error.toStdString() << '\n';
            return 1;
        }
        std::cout << "originalBlocks=" << result.originalBlocks
                  << " removedBlocks=" << result.removedBlocks
                  << " remainingBlocks=" << result.remainingBlocks
                  << " removedAssets=" << result.removedAssets << '\n';
        return 0;
    }

    QTemporaryDir temporary;
    if(!temporary.isValid()) return 2;
    const QString route = temporary.path();
    QDir().mkpath(route + "/OpenRails");
    QDir().mkpath(route + "/world");
    QDir().mkpath(route + "/shapes");

    ForestBakeManifestEntry kept;
    kept.id = "kept";
    kept.shapeFile = "V-00001+00002-00.s";
    ForestBakeManifestEntry removed;
    removed.id = "removed";
    removed.shapeFile = "V-00001+00002-01.s";
    QString error;
    const QString manifest = route + "/OpenRails/forest-bakes.json";
    bool passed = true;
    passed &= check(ForestBakeManifest::upsert(manifest, kept, error),
                    "kept entry should write");
    passed &= check(ForestBakeManifest::upsert(manifest, removed, error),
                    "removed entry should write");
    for(const QString &shape : {kept.shapeFile, removed.shapeFile}) {
        passed &= check(touch(route + "/shapes/" + shape), "shape should write");
        passed &= check(touch(route + "/shapes/"
            + QFileInfo(shape).completeBaseName() + ".sd"),
            "descriptor should write");
    }
    passed &= check(touch(route + "/shapes/V-99999+99999-99.s"),
                    "unowned shape should write");

    QFile world(route + "/world/w-000001+000002.w");
    passed &= check(world.open(QIODevice::WriteOnly | QIODevice::Text),
                    "world should open");
    QTextStream stream(&world);
    stream.setEncoding(QStringConverter::Utf16);
    stream.setGenerateByteOrderMark(true);
    stream << "Tr_Worldfile ( Static ( FileName ( " << kept.shapeFile << " ) ) )";
    world.close();

    ForestBakePruneResult result;
    passed &= check(ForestBakeManifest::pruneUnreferenced(route, result, error),
                    "prune should succeed");
    passed &= check(result.originalBlocks == 2 && result.remainingBlocks == 1
                    && result.removedBlocks == 1,
                    "only unreferenced manifest block should be pruned");
    passed &= check(result.removedAssets == 2,
                    "exact shape and descriptor should be removed");
    passed &= check(QFileInfo::exists(route + "/shapes/" + kept.shapeFile),
                    "referenced shape should remain");
    passed &= check(QFileInfo::exists(route + "/shapes/"
                    + QFileInfo(kept.shapeFile).completeBaseName() + ".sd"),
                    "referenced descriptor should remain");
    passed &= check(!QFileInfo::exists(route + "/shapes/" + removed.shapeFile),
                    "unreferenced shape should be removed");
    passed &= check(QFileInfo::exists(route + "/shapes/V-99999+99999-99.s"),
                    "unowned generated-looking asset should remain");
    if(passed) std::cout << "Forest bake manifest prune probe passed.\n";
    return passed ? 0 : 1;
}

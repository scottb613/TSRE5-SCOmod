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

    const QString replacedFile = route + "/shapes/session-existing.s";
    const QString newFile = route + "/shapes/session-new.sd";
    bool passed = true;
    passed &= check(touch(replacedFile), "session existing file should write");
    ForestBakeSession session;
    QString error;
    passed &= check(session.rememberFile(replacedFile, error),
                    "session should preserve existing file");
    passed &= check(session.rememberFile(newFile, error),
                    "session should remember absent file");
    QFile replaced(replacedFile);
    passed &= check(replaced.open(QIODevice::WriteOnly | QIODevice::Truncate),
                    "session existing file should reopen");
    replaced.write("replacement");
    replaced.close();
    passed &= check(touch(newFile), "session new file should write");
    passed &= check(session.rollback(error), "session rollback should succeed");
    QFile restored(replacedFile);
    passed &= check(restored.open(QIODevice::ReadOnly),
                    "session existing file should reopen after rollback");
    passed &= check(restored.readAll() == QByteArray("generated"),
                    "session rollback should restore existing contents");
    restored.close();
    passed &= check(!QFileInfo::exists(newFile),
                    "session rollback should remove newly created file");

    ForestBakeSession committedSession;
    passed &= check(committedSession.rememberFile(replacedFile, error),
                    "committed session should preserve existing file");
    replaced.setFileName(replacedFile);
    passed &= check(replaced.open(QIODevice::WriteOnly | QIODevice::Truncate),
                    "committed file should reopen");
    replaced.write("committed");
    replaced.close();
    committedSession.commit();
    passed &= check(committedSession.rollback(error),
                    "committed session rollback should be empty");
    restored.setFileName(replacedFile);
    passed &= check(restored.open(QIODevice::ReadOnly),
                    "committed file should reopen after empty rollback");
    passed &= check(restored.readAll() == QByteArray("committed"),
                    "session commit should retain replacement contents");
    restored.close();

    ForestBakeManifestEntry kept;
    kept.id = "kept";
    kept.shapeFile = "V-00001+00002-00.s";
    ForestBakeManifestEntry removed;
    removed.id = "removed";
    removed.shapeFile = "V-00001+00002-01.s";
    const QString manifest = route + "/OpenRails/forest-bakes.json";
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
    passed &= check(result.removedAssets == 3,
                    "manifest pair and orphaned generated shape should be removed");
    passed &= check(QFileInfo::exists(route + "/shapes/" + kept.shapeFile),
                    "referenced shape should remain");
    passed &= check(QFileInfo::exists(route + "/shapes/"
                    + QFileInfo(kept.shapeFile).completeBaseName() + ".sd"),
                    "referenced descriptor should remain");
    passed &= check(!QFileInfo::exists(route + "/shapes/" + removed.shapeFile),
                    "unreferenced shape should be removed");
    passed &= check(!QFileInfo::exists(route + "/shapes/V-99999+99999-99.s"),
                    "orphaned generated shape should be removed");

    const QString manifestlessOrphan = "V-88888+88888-88.s";
    passed &= check(touch(route + "/shapes/" + manifestlessOrphan),
                    "post-prune orphan shape should write");
    passed &= check(touch(route + "/shapes/"
                    + QFileInfo(manifestlessOrphan).completeBaseName() + ".sd"),
                    "post-prune orphan descriptor should write");
    ForestBakePruneResult orphanResult;
    passed &= check(ForestBakeManifest::pruneUnreferenced(
                    route, orphanResult, error),
                    "orphan-only prune should succeed with no removable blocks");
    passed &= check(orphanResult.removedBlocks == 0
                    && orphanResult.removedAssets == 2,
                    "orphan-only prune should remove the generated pair");
    passed &= check(!QFileInfo::exists(route + "/shapes/" + manifestlessOrphan),
                    "orphan-only prune should remove the shape");

    QTemporaryDir corruptTemporary;
    passed &= check(corruptTemporary.isValid(),
                    "corrupt-world temporary route should exist");
    const QString corruptRoute = corruptTemporary.path();
    QDir().mkpath(corruptRoute + "/OpenRails");
    QDir().mkpath(corruptRoute + "/world");
    QDir().mkpath(corruptRoute + "/shapes");
    ForestBakeManifestEntry protectedEntry;
    protectedEntry.id = "protected";
    protectedEntry.shapeFile = "V-77777+77777-77.s";
    const QString corruptManifest = corruptRoute
        + "/OpenRails/forest-bakes.json";
    passed &= check(ForestBakeManifest::upsert(
                    corruptManifest, protectedEntry, error),
                    "protected entry should write");
    const QString protectedShape = corruptRoute + "/shapes/"
        + protectedEntry.shapeFile;
    passed &= check(touch(protectedShape), "protected shape should write");
    QFile corruptWorld(corruptRoute + "/world/w-000001+000002.w");
    passed &= check(corruptWorld.open(QIODevice::WriteOnly),
                    "corrupt compressed world should open");
    QByteArray corruptData(40, '\0');
    corruptData.replace(0, 8, QByteArray("SIMISA@F", 8));
    corruptWorld.write(corruptData);
    corruptWorld.close();
    ForestBakePruneResult corruptResult;
    passed &= check(!ForestBakeManifest::pruneUnreferenced(
                    corruptRoute, corruptResult, error),
                    "corrupt compressed world should stop destructive prune");
    passed &= check(QFileInfo::exists(protectedShape),
                    "failed world decode should preserve generated assets");
    if(passed) std::cout << "Forest bake manifest prune probe passed.\n";
    return passed ? 0 : 1;
}

#include "ForestBakeManifest.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
#include <QStringConverter>

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

void appendLittleEndian16(QByteArray &data, quint16 value) {
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
}

void appendLittleEndian32(QByteArray &data, quint32 value) {
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
    data.append(static_cast<char>((value >> 16) & 0xff));
    data.append(static_cast<char>((value >> 24) & 0xff));
}

QByteArray compressedBinaryWorld(const QString &shapeFile,
                                 bool validRootLength = true) {
    // Representative MSTS binary token field: FileName token 0x0004005f,
    // empty label byte, UTF-16 character count, then UTF-16LE filename data.
    QStringEncoder encoder(QStringEncoder::Utf16LE);
    const QByteArray encodedName = encoder(shapeFile);
    QByteArray rootPayload;
    rootPayload.append('\0');
    appendLittleEndian32(rootPayload, 0x0004005f);
    appendLittleEndian32(rootPayload,
                         static_cast<quint32>(1 + 2 + encodedName.size()));
    rootPayload.append('\0');
    appendLittleEndian16(rootPayload, static_cast<quint16>(shapeFile.size()));
    rootPayload.append(encodedName);

    QByteArray payload("JINX0w0b______\r\n", 16);
    appendLittleEndian32(payload, 0x0004004b);
    appendLittleEndian32(payload, static_cast<quint32>(rootPayload.size()
                         + (validRootLength ? 0 : 1)));
    payload.append(rootPayload);

    const QByteArray compressed = qCompress(payload);
    QByteArray world("SIMISA@F", 8);
    appendLittleEndian32(world, static_cast<quint32>(payload.size()));
    world.append("@@@@", 4);
    world.append(compressed.sliced(4));
    return world;
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

    QTemporaryDir binaryTemporary;
    passed &= check(binaryTemporary.isValid(),
                    "binary-world temporary route should exist");
    const QString binaryRoute = binaryTemporary.path();
    QDir().mkpath(binaryRoute + "/OpenRails");
    QDir().mkpath(binaryRoute + "/world");
    QDir().mkpath(binaryRoute + "/shapes");
    ForestBakeManifestEntry binaryEntry;
    binaryEntry.id = "binary-protected";
    binaryEntry.shapeFile = "V-12345+23456-12.s";
    const QString binaryManifest = binaryRoute
        + "/OpenRails/forest-bakes.json";
    passed &= check(ForestBakeManifest::upsert(
                    binaryManifest, binaryEntry, error),
                    "binary-world entry should write");
    const QString binaryShape = binaryRoute + "/shapes/"
        + binaryEntry.shapeFile;
    passed &= check(touch(binaryShape), "binary-world shape should write");
    QFile binaryWorld(binaryRoute + "/world/w-000001+000002.w");
    passed &= check(binaryWorld.open(QIODevice::WriteOnly),
                    "binary world should open");
    const QByteArray binaryWorldData =
        compressedBinaryWorld(binaryEntry.shapeFile);
    passed &= check(binaryWorld.write(binaryWorldData) == binaryWorldData.size(),
                    "binary world should write completely");
    binaryWorld.close();
    ForestBakePruneResult binaryResult;
    passed &= check(ForestBakeManifest::pruneUnreferenced(
                    binaryRoute, binaryResult, error),
                    "valid compressed binary world should scan safely");
    passed &= check(binaryResult.remainingBlocks == 1
                    && binaryResult.removedBlocks == 0,
                    "binary world reference should retain its manifest block");
    passed &= check(QFileInfo::exists(binaryShape),
                    "binary world reference should retain its generated shape");

    QTemporaryDir malformedBinaryTemporary;
    passed &= check(malformedBinaryTemporary.isValid(),
                    "malformed-binary temporary route should exist");
    const QString malformedBinaryRoute = malformedBinaryTemporary.path();
    QDir().mkpath(malformedBinaryRoute + "/OpenRails");
    QDir().mkpath(malformedBinaryRoute + "/world");
    QDir().mkpath(malformedBinaryRoute + "/shapes");
    ForestBakeManifestEntry malformedBinaryEntry;
    malformedBinaryEntry.id = "malformed-binary-protected";
    malformedBinaryEntry.shapeFile = "V-23456+34567-23.s";
    passed &= check(ForestBakeManifest::upsert(
                    malformedBinaryRoute + "/OpenRails/forest-bakes.json",
                    malformedBinaryEntry, error),
                    "malformed-binary entry should write");
    const QString malformedBinaryShape = malformedBinaryRoute + "/shapes/"
        + malformedBinaryEntry.shapeFile;
    passed &= check(touch(malformedBinaryShape),
                    "malformed-binary shape should write");
    QFile malformedBinaryWorld(malformedBinaryRoute
        + "/world/w-000001+000002.w");
    passed &= check(malformedBinaryWorld.open(QIODevice::WriteOnly),
                    "malformed binary world should open");
    const QByteArray malformedBinaryData = compressedBinaryWorld(
        malformedBinaryEntry.shapeFile, false);
    passed &= check(malformedBinaryWorld.write(malformedBinaryData)
                    == malformedBinaryData.size(),
                    "malformed binary world should write completely");
    malformedBinaryWorld.close();
    ForestBakePruneResult malformedBinaryResult;
    passed &= check(!ForestBakeManifest::pruneUnreferenced(
                    malformedBinaryRoute, malformedBinaryResult, error),
                    "malformed binary root should stop destructive prune");
    passed &= check(QFileInfo::exists(malformedBinaryShape),
                    "malformed binary root should preserve generated assets");

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

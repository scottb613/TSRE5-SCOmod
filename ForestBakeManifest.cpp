// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "ForestBakeManifest.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStringConverter>
#include <QRegularExpression>
#include <QVector>
#include <QtEndian>

namespace {
constexpr quint32 MaximumExpandedWorldBytes = 256u * 1024u * 1024u;

bool uncompressWorldPayload(const QByteArray &data, int offset,
                            QByteArray &payload) {
    if(offset < 0 || data.size() - offset < 4)
        return false;
    const quint32 expandedSize = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(data.constData() + offset));
    if(expandedSize == 0 || expandedSize > MaximumExpandedWorldBytes)
        return false;
    payload = qUncompress(data.mid(offset));
    return !payload.isEmpty()
        && static_cast<quint32>(payload.size()) == expandedSize;
}

QJsonObject sourceObject(const ForestBakeInstance &source) {
    QJsonObject object;
    object["shape"] = source.shapePath;
    object["x"] = source.x;
    object["y"] = source.y;
    object["z"] = source.z;
    object["yawDegrees"] = source.yawDegrees;
    object["uniformScale"] = source.uniformScale;
    return object;
}

QJsonObject entryObject(const ForestBakeManifestEntry &entry) {
    QJsonObject object;
    object["id"] = entry.id;
    object["shapeFile"] = entry.shapeFile;
    object["tileX"] = entry.tileX;
    object["tileZ"] = entry.tileZ;
    object["blockX"] = entry.blockX;
    object["blockZ"] = entry.blockZ;
    object["positionX"] = entry.positionX;
    object["positionY"] = entry.positionY;
    object["positionZ"] = entry.positionZ;
    object["enabled"] = entry.enabled;
    QJsonArray sources;
    for(const ForestBakeInstance &source : entry.sources) sources.append(sourceObject(source));
    object["sources"] = sources;
    return object;
}

bool expandedWorldFile(QByteArray data, QByteArray &expanded) {
    expanded = data;
    if(data.size() < 18) return true;
    const bool hasBom = static_cast<unsigned char>(data[0]) == 0xFF
        && static_cast<unsigned char>(data[1]) == 0xFE;
    if(!hasBom && data.size() > 16 && data[7] == 'F') {
        data[12] = data[11];
        data[13] = data[10];
        data[14] = data[9];
        data[15] = data[8];
        QByteArray payload;
        if(!uncompressWorldPayload(data, 12, payload)) return false;
        expanded = data.left(16) + payload;
        return true;
    }
    if(hasBom && data.size() > 34 && data[16] == 'F') {
        data[30] = data[19];
        data[31] = data[18];
        data[32] = data[17];
        data[33] = data[13];
        QByteArray payload;
        if(!uncompressWorldPayload(data, 30, payload)) return false;
        expanded = data.left(34) + payload;
        return true;
    }
    return true;
}

bool decodedWorldFile(const QByteArray &input, QString &text,
                      QByteArray &binaryPayload) {
    QByteArray data;
    if(!expandedWorldFile(input, data)) return false;
    text.clear();
    binaryPayload.clear();

    // MSTS binary world files carry the JINX0w0b marker ahead of a binary
    // token stream whose string values are UTF-16LE. Treating that stream as
    // UTF-8 rejects otherwise valid files (for example on byte 0x95). Keep the
    // bytes intact and let the narrowly scoped generated-name scan below look
    // for the exact ASCII/UTF-16LE filename encodings instead.
    const QByteArray header = data.left(48).toLower();
    const QByteArray asciiBinaryMarker("jinx0w0b", 8);
    const qsizetype markerOffset = header.indexOf(asciiBinaryMarker);
    if(markerOffset >= 0) {
        constexpr quint32 BinaryWorldRootToken = 261844u + 375u;
        const qsizetype tokenOffset = markerOffset + 16;
        if(data.size() - tokenOffset < 8)
            return false;
        const quint32 rootToken = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(data.constData() + tokenOffset));
        const quint32 declaredBytes = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(data.constData() + tokenOffset + 4));
        const qsizetype availableBytes = data.size() - tokenOffset - 8;
        if(rootToken != BinaryWorldRootToken
                || declaredBytes != static_cast<quint32>(availableBytes))
            return false;
        binaryPayload = data.mid(tokenOffset + 8);
        return true;
    }

    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFF
            && static_cast<unsigned char>(data[1]) == 0xFE) {
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        text = decoder(data);
        return !decoder.hasError();
    }
    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFE
            && static_cast<unsigned char>(data[1]) == 0xFF) {
        QStringDecoder decoder(QStringDecoder::Utf16BE);
        text = decoder(data);
        return !decoder.hasError();
    }
    QStringDecoder decoder(QStringDecoder::Utf8);
    text = decoder(data);
    return !decoder.hasError();
}

bool isGeneratedBakeName(const QString &name) {
    static const QRegularExpression pattern(
        QStringLiteral("^V[+-]\\d{5}[+-]\\d{5}-\\d{2}\\.s$"),
        QRegularExpression::CaseInsensitiveOption);
    return QFileInfo(name).fileName() == name && pattern.match(name).hasMatch();
}
}

bool ForestBakeSession::rememberFile(const QString &path, QString &error) {
    error.clear();
    const QString cleanPath = QDir::cleanPath(path);
    if(files.contains(cleanPath))
        return true;

    FileState state;
    const QFileInfo info(cleanPath);
    state.existed = info.exists();
    if(state.existed) {
        if(!info.isFile()) {
            error = "PolyVeg bake output is not a file: " + cleanPath;
            return false;
        }
        QFile file(cleanPath);
        if(!file.open(QIODevice::ReadOnly)) {
            error = "Unable to preserve PolyVeg bake file before replacement: "
                    + cleanPath;
            return false;
        }
        state.contents = file.readAll();
        if(file.error() != QFileDevice::NoError) {
            error = "Unable to read PolyVeg bake file before replacement: "
                    + cleanPath;
            return false;
        }
    }
    files.insert(cleanPath, state);
    return true;
}

bool ForestBakeSession::rollback(QString &error) {
    error.clear();
    QStringList failures;
    for(auto it = files.constBegin(); it != files.constEnd(); ++it) {
        const QString path = it.key();
        const FileState &state = it.value();
        if(!state.existed) {
            if(QFileInfo::exists(path) && !QFile::remove(path))
                failures.append("Unable to remove unsaved PolyVeg bake file: " + path);
            continue;
        }

        QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly)
                || file.write(state.contents) != state.contents.size()
                || !file.commit()) {
            failures.append("Unable to restore saved PolyVeg bake file: " + path);
        }
    }
    if(!failures.isEmpty()) {
        error = failures.join('\n');
        return false;
    }
    files.clear();
    return true;
}

void ForestBakeSession::commit() {
    files.clear();
}

bool ForestBakeSession::isEmpty() const {
    return files.isEmpty();
}

bool ForestBakeManifest::upsert(const QString &path,
                                const ForestBakeManifestEntry &entry,
                                QString &error) {
    QJsonObject root;
    QFile existing(path);
    if(existing.exists()) {
        if(!existing.open(QIODevice::ReadOnly)) {
            error = "Unable to read forest bake manifest: " + path;
            return false;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(existing.readAll(), &parseError);
        if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
            error = "Invalid forest bake manifest: " + parseError.errorString();
            return false;
        }
        root = document.object();
        existing.close();
    } else {
        root["version"] = 1;
    }

    QJsonArray entries = root["blocks"].toArray();
    bool replaced = false;
    for(qsizetype i = 0; i < entries.size(); ++i) {
        if(entries[i].toObject()["id"].toString() == entry.id) {
            entries[i] = entryObject(entry);
            replaced = true;
            break;
        }
    }
    if(!replaced) entries.append(entryObject(entry));
    root["blocks"] = entries;

    QSaveFile output(path);
    if(!output.open(QIODevice::WriteOnly)) {
        error = "Unable to create forest bake manifest: " + path;
        return false;
    }
    const QByteArray document = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if(output.write(document) != document.size() || !output.commit()) {
        error = "Unable to publish forest bake manifest: " + path;
        return false;
    }
    return true;
}

bool ForestBakeManifest::pruneUnreferenced(const QString &routePath,
                                           ForestBakePruneResult &result,
                                           QString &error) {
    result = ForestBakePruneResult();
    const QString cleanRoutePath = QDir::cleanPath(routePath);
    const QString manifestPath = cleanRoutePath
        + "/OpenRails/forest-bakes.json";
    QFile input(manifestPath);
    QJsonObject root;
    if(input.exists()) {
        if(!input.open(QIODevice::ReadOnly)) {
            error = "Unable to read forest bake manifest: " + manifestPath;
            return false;
        }
        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(input.readAll(), &parseError);
        input.close();
        if(parseError.error != QJsonParseError::NoError
                || !document.isObject()) {
            error = "Invalid forest bake manifest: " + parseError.errorString();
            return false;
        }
        root = document.object();
    } else {
        root["version"] = 1;
    }
    const QJsonArray entries = root["blocks"].toArray();
    result.originalBlocks = entries.size();
    QSet<QString> manifestShapes;
    for(const QJsonValue &value : entries) {
        const QString shape = value.toObject()["shapeFile"].toString();
        if(!isGeneratedBakeName(shape)) {
            error = "Unsafe generated bake shape name in manifest: " + shape;
            return false;
        }
        manifestShapes.insert(shape.toLower());
    }

    // Older PolyVeg bakes can predate the manifest. Discover their narrowly
    // defined generated names directly so route cleanup does not depend on
    // bookkeeping that may never have existed.
    const QDir shapesDirectory(cleanRoutePath + "/shapes");
    const QStringList generatedShapeFiles = shapesDirectory.entryList(
        QStringList() << "V*.s", QDir::Files, QDir::Name);
    for(const QString &shape : generatedShapeFiles)
        if(isGeneratedBakeName(shape))
            manifestShapes.insert(shape.toLower());
    const QStringList generatedDescriptorFiles = shapesDirectory.entryList(
        QStringList() << "V*.sd", QDir::Files, QDir::Name);
    for(const QString &descriptor : generatedDescriptorFiles) {
        const QString shape =
            QFileInfo(descriptor).completeBaseName() + ".s";
        if(isGeneratedBakeName(shape))
            manifestShapes.insert(shape.toLower());
    }
    if(manifestShapes.isEmpty())
        return true;

    struct GeneratedNameEncoding {
        QString name;
        QByteArray ascii;
        QByteArray utf16Le;
    };
    QVector<GeneratedNameEncoding> generatedNames;
    generatedNames.reserve(manifestShapes.size());
    for(const QString &shape : manifestShapes) {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        generatedNames.append({shape, shape.toLatin1(), encoder(shape)});
    }

    QSet<QString> referencedShapes;
    const QDir worldDirectory(cleanRoutePath + "/world");
    const QStringList worldFiles = worldDirectory.entryList(
        QStringList() << "*.w", QDir::Files, QDir::Name);
    for(const QString &worldName : worldFiles) {
        QFile worldFile(worldDirectory.filePath(worldName));
        if(!worldFile.open(QIODevice::ReadOnly)) {
            error = "Unable to scan saved world file: " + worldFile.fileName();
            return false;
        }
        if(worldFile.size() < 0
                || worldFile.size() > MaximumExpandedWorldBytes) {
            error = "Saved world file exceeds the safe cleanup scan limit: "
                    + worldFile.fileName();
            return false;
        }
        QString worldText;
        QByteArray binaryPayload;
        if(!decodedWorldFile(worldFile.readAll(), worldText, binaryPayload)) {
            error = "Unable to decode saved world file safely: "
                    + worldFile.fileName();
            return false;
        }
        if(binaryPayload.isEmpty()) {
            worldText = worldText.toLower();
            for(const GeneratedNameEncoding &shape : generatedNames)
                if(worldText.contains(shape.name))
                    referencedShapes.insert(shape.name);
            continue;
        }

        const QByteArray foldedPayload = binaryPayload.toLower();
        for(const GeneratedNameEncoding &shape : generatedNames)
            if(foldedPayload.contains(shape.ascii)
                    || foldedPayload.contains(shape.utf16Le))
                referencedShapes.insert(shape.name);
    }

    QJsonArray retainedEntries;
    QStringList assetsToRemove;
    for(const QJsonValue &value : entries) {
        const QString shape = value.toObject()["shapeFile"].toString();
        if(referencedShapes.contains(shape.toLower())) {
            retainedEntries.append(value);
            result.retainedShapes.append(shape);
            continue;
        }
        result.removedShapes.append(shape);
        assetsToRemove.append(cleanRoutePath + "/shapes/" + shape);
        assetsToRemove.append(cleanRoutePath + "/shapes/"
            + QFileInfo(shape).completeBaseName() + ".sd");
    }
    result.remainingBlocks = retainedEntries.size();
    result.removedBlocks = result.originalBlocks-result.remainingBlocks;

    // Add generated assets discovered outside the manifest. Manifest-backed
    // entries are already present in these result lists, so de-duplicate by
    // case-folded shape name before staging the pair.
    QSet<QString> scheduledShapes;
    for(const QString &shape : result.removedShapes)
        scheduledShapes.insert(shape.toLower());
    for(const QString &shape : manifestShapes) {
        if(referencedShapes.contains(shape)
                || scheduledShapes.contains(shape))
            continue;
        const QString actualShape = [&shapesDirectory, shape]() {
            const QStringList matches = shapesDirectory.entryList(
                QStringList() << shape, QDir::Files, QDir::Name);
            return matches.isEmpty() ? shape : matches.first();
        }();
        assetsToRemove.append(shapesDirectory.filePath(actualShape));
        assetsToRemove.append(shapesDirectory.filePath(
            QFileInfo(actualShape).completeBaseName() + ".sd"));
        result.removedShapes.append(actualShape);
        scheduledShapes.insert(shape);
    }
    if(result.removedBlocks == 0 && assetsToRemove.isEmpty())
        return true;

    struct StagedAsset { QString original; QString staged; };
    QVector<StagedAsset> stagedAssets;
    auto restoreStagedAssets = [&stagedAssets]() {
        for(auto it = stagedAssets.crbegin(); it != stagedAssets.crend(); ++it)
            QFile::rename(it->staged, it->original);
    };
    for(const QString &assetPath : assetsToRemove) {
        if(!QFileInfo::exists(assetPath)) continue;
        const QString stagedPath = assetPath + ".tsre-polyveg-prune";
        QFile::remove(stagedPath);
        if(!QFile::rename(assetPath, stagedPath)) {
            restoreStagedAssets();
            error = "Unable to stage generated PolyVeg asset for committed "
                    "cleanup: " + assetPath;
            return false;
        }
        stagedAssets.append({assetPath, stagedPath});
    }

    root["blocks"] = retainedEntries;
    QSaveFile output(manifestPath);
    if(!output.open(QIODevice::WriteOnly)) {
        restoreStagedAssets();
        error = "Unable to update forest bake manifest: " + manifestPath;
        return false;
    }
    const QByteArray document = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if(output.write(document) != document.size() || !output.commit()) {
        restoreStagedAssets();
        error = "Unable to publish pruned forest bake manifest: " + manifestPath;
        return false;
    }

    QStringList removalFailures;
    for(const StagedAsset &asset : stagedAssets) {
        if(QFile::remove(asset.staged)) {
            ++result.removedAssets;
            continue;
        }
        if(!QFile::rename(asset.staged, asset.original))
            removalFailures.append("Unable to remove or restore staged PolyVeg asset: "
                                   + asset.staged);
        else
            removalFailures.append("Unable to remove generated PolyVeg asset: "
                                   + asset.original);
    }
    if(retainedEntries.isEmpty() && QFileInfo::exists(manifestPath)) {
        if(!QFile::remove(manifestPath)) {
            error = "Unable to remove empty forest bake manifest: "
                + manifestPath;
            return false;
        }
        result.removedManifest = true;
    }
    if(!removalFailures.isEmpty()) {
        error = removalFailures.join('\n');
        return false;
    }
    return true;
}

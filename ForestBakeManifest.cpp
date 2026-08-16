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

namespace {
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

QByteArray expandedWorldFile(QByteArray data) {
    if(data.size() < 18) return data;
    const bool hasBom = static_cast<unsigned char>(data[0]) == 0xFF
        && static_cast<unsigned char>(data[1]) == 0xFE;
    if(!hasBom && data.size() > 16 && data[7] == 'F') {
        data[12] = data[11];
        data[13] = data[10];
        data[14] = data[9];
        data[15] = data[8];
        return data.left(16) + qUncompress(data.mid(12));
    }
    if(hasBom && data.size() > 34 && data[16] == 'F') {
        data[30] = data[19];
        data[31] = data[18];
        data[32] = data[17];
        data[33] = data[13];
        return data.left(34) + qUncompress(data.mid(30));
    }
    return data;
}

QString decodedWorldFile(const QByteArray &input) {
    const QByteArray data = expandedWorldFile(input);
    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFF
            && static_cast<unsigned char>(data[1]) == 0xFE) {
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        return decoder(data);
    }
    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFE
            && static_cast<unsigned char>(data[1]) == 0xFF) {
        QStringDecoder decoder(QStringDecoder::Utf16BE);
        return decoder(data);
    }
    return QString::fromUtf8(data);
}

bool isGeneratedBakeName(const QString &name) {
    static const QRegularExpression pattern(
        QStringLiteral("^V[+-]\\d{5}[+-]\\d{5}-\\d{2}\\.s$"),
        QRegularExpression::CaseInsensitiveOption);
    return QFileInfo(name).fileName() == name && pattern.match(name).hasMatch();
}
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
    output.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if(!output.commit()) {
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
    if(!input.exists()) return true;
    if(!input.open(QIODevice::ReadOnly)) {
        error = "Unable to read forest bake manifest: " + manifestPath;
        return false;
    }
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
    input.close();
    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = "Invalid forest bake manifest: " + parseError.errorString();
        return false;
    }

    QJsonObject root = document.object();
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
        const QString worldText = decodedWorldFile(worldFile.readAll()).toLower();
        for(const QString &shape : manifestShapes)
            if(worldText.contains(shape)) referencedShapes.insert(shape);
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
    if(result.removedBlocks == 0) return true;

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
    output.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if(!output.commit()) {
        restoreStagedAssets();
        error = "Unable to publish pruned forest bake manifest: " + manifestPath;
        return false;
    }

    for(const StagedAsset &asset : stagedAssets) {
        if(!QFile::remove(asset.staged))
            qWarning() << "Unable to remove staged PolyVeg asset" << asset.staged;
        ++result.removedAssets;
    }
    return true;
}

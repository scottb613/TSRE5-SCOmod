#include "ForestPatchBaker.h"
#include "ForestBakeManifest.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include <cmath>
#include <iostream>

namespace {
constexpr double TileSize = 2048.0;
constexpr double PatchSize = 128.0;

QVector<double> captures(const QRegularExpressionMatch &match, int first, int count) {
    QVector<double> values;
    for(int i = 0; i < count; ++i) values.append(match.captured(first + i).toDouble());
    return values;
}

int patchForLocal(double coordinate) {
    return qBound(0, static_cast<int>(std::floor((coordinate + 1024.0)/PatchSize)), 15);
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if(args.size() != 6) {
        std::cerr << "Usage: tsre_bake_forest_patch <source.w> <shapes-dir> "
                     "<4x4-block-x> <4x4-block-z> <output.s>\n";
        return 2;
    }

    const QString worldPath = args[1];
    const QString shapesDir = args[2];
    const int wantedBlockX = args[3].toInt();
    const int wantedBlockZ = args[4].toInt();
    const QString outputPath = args[5];
    if(wantedBlockX < 0 || wantedBlockX > 3 || wantedBlockZ < 0 || wantedBlockZ > 3) {
        std::cerr << "4x4 block coordinates must be between 0 and 3.\n";
        return 2;
    }

    const QRegularExpression tileExpression("w([+-]\\d{6})([+-]\\d{6})\\.w$",
                                             QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch tileMatch = tileExpression.match(QFileInfo(worldPath).fileName());
    if(!tileMatch.hasMatch()) {
        std::cerr << "Unable to read tile coordinates from world filename.\n";
        return 1;
    }
    const int tileX = tileMatch.captured(1).toInt();
    const int tileZ = tileMatch.captured(2).toInt();

    QFile worldFile(worldPath);
    if(!worldFile.open(QIODevice::ReadOnly)) {
        std::cerr << "Unable to open source world file.\n";
        return 1;
    }
    const QByteArray worldBytes = worldFile.readAll();
    if(worldBytes.size() < 4 || static_cast<unsigned char>(worldBytes[0]) != 0xff
            || static_cast<unsigned char>(worldBytes[1]) != 0xfe) {
        std::cerr << "Source world file is not UTF-16LE.\n";
        return 1;
    }
    const QString worldText = QString::fromUtf16(
        reinterpret_cast<const char16_t*>(worldBytes.constData() + 2),
        (worldBytes.size() - 2)/2);

    const QString number = "([-+0-9.eE]+)";
    const QRegularExpression blockExpression(
        "^[ \\t]*Static[ \\t]*\\(.*?^[ \\t]*\\)[ \\t]*(?:\\r?\\n)?",
        QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression fileExpression("FileName\\s*\\(\\s*([^\\s\\)]+)\\s*\\)",
                                             QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression positionExpression(
        "Position\\s*\\(\\s*" + number + "\\s+" + number + "\\s+" + number + "\\s*\\)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression matrixExpression(
        "Matrix3x3\\s*\\(\\s*" + number + "\\s+" + number + "\\s+" + number
        + "\\s+" + number + "\\s+" + number + "\\s+" + number
        + "\\s+" + number + "\\s+" + number + "\\s+" + number + "\\s*\\)",
        QRegularExpression::CaseInsensitiveOption);

    QVector<ForestBakeInstance> instances;
    auto blocks = blockExpression.globalMatch(worldText);
    while(blocks.hasNext()) {
        const QString block = blocks.next().captured();
        const QRegularExpressionMatch fileMatch = fileExpression.match(block);
        const QRegularExpressionMatch positionMatch = positionExpression.match(block);
        const QRegularExpressionMatch matrixMatch = matrixExpression.match(block);
        if(!fileMatch.hasMatch() || !positionMatch.hasMatch() || !matrixMatch.hasMatch()) continue;
        const QVector<double> position = captures(positionMatch, 1, 3);
        if(patchForLocal(position[0])/4 != wantedBlockX
                || patchForLocal(position[2])/4 != wantedBlockZ) continue;
        const QVector<double> matrix = captures(matrixMatch, 1, 9);
        const double scaleX = std::hypot(matrix[0], matrix[2]);
        const double scaleY = std::abs(matrix[4]);
        const double scaleZ = std::hypot(matrix[6], matrix[8]);
        ForestBakeInstance instance;
        instance.shapePath = QFileInfo(shapesDir + '/' + fileMatch.captured(1)).absoluteFilePath();
        instance.x = tileX*TileSize + position[0];
        instance.y = position[1];
        instance.z = tileZ*TileSize + position[2];
        instance.yawDegrees = std::atan2(matrix[6], matrix[0])*180.0/M_PI;
        instance.uniformScale = (scaleX + scaleY + scaleZ)/3.0;
        instances.append(instance);
    }
    if(instances.isEmpty()) {
        std::cerr << "No Static objects found in the requested patch.\n";
        return 1;
    }

    const ForestPatchBakeResult result = ForestPatchBaker::bake(instances, 4);
    if(!result.isValid() || result.patches.size() != 1) {
        std::cerr << "Patch bake failed: " << result.errors.join("; ").toStdString() << '\n';
        return 1;
    }
    QString error;
    if(!ForestShapeTextIO::writePatch(outputPath, result.patches.front(), error)) {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }
    if(!ForestShapeTextIO::writeDescriptor(outputPath, error)) {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }
    const ForestBakedPatch &patch = result.patches.front();
    ForestBakeManifestEntry manifestEntry;
    manifestEntry.id = QString("%1,%2:%3,%4")
        .arg(tileX).arg(tileZ).arg(wantedBlockX).arg(wantedBlockZ);
    manifestEntry.shapeFile = QFileInfo(outputPath).fileName();
    manifestEntry.tileX = tileX;
    manifestEntry.tileZ = tileZ;
    manifestEntry.blockX = wantedBlockX;
    manifestEntry.blockZ = wantedBlockZ;
    manifestEntry.positionX = patch.originX - tileX*TileSize;
    manifestEntry.positionY = patch.originY;
    manifestEntry.positionZ = patch.originZ - tileZ*TileSize;
    manifestEntry.sources = instances;
    const QString manifestPath = QFileInfo(outputPath).absolutePath() + "/forest-bakes.json";
    if(!ForestBakeManifest::upsert(manifestPath, manifestEntry, error)) {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }
    std::cout << "Baked " << patch.sourceInstanceCount << " trees into "
              << QFileInfo(outputPath).absoluteFilePath().toStdString() << '\n'
              << "tile=" << tileX << ',' << tileZ
              << " block=" << wantedBlockX << ',' << wantedBlockZ
              << " position=" << patch.originX - tileX*TileSize << ',' << patch.originY << ','
              << patch.originZ - tileZ*TileSize << '\n'
              << "descriptor=" << QFileInfo(outputPath).completeBaseName().toStdString() << ".sd\n"
              << "manifest=" << manifestPath.toStdString() << '\n';
    return 0;
}

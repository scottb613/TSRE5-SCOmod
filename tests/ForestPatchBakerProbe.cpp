#include "ForestPatchBaker.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QStringConverter>

#include <cmath>
#include <iostream>
#include <limits>

namespace {
bool check(bool condition, const char *message) {
    if(condition) return true;
    std::cerr << "FAILED: " << message << '\n';
    return false;
}

bool writeCruciformFixture(const QString &path, bool invalidIndex = false) {
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return false;
    file.write("\xff\xfe", 2);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf16LE);
    out.setGenerateByteOrderMark(false);
    out << "SIMISA@@@@@@@@@@JINX0s1t______\r\n\r\nshape (\r\n"
        << " points ( 8\r\n"
        << "  point ( -1 0 0 ) point ( 1 0 0 )"
        << " point ( 1 2 0 ) point ( -1 2 0 )\r\n"
        << "  point ( 0 0 -1 ) point ( 0 0 1 )"
        << " point ( 0 2 1 ) point ( 0 2 -1 )\r\n )\r\n"
        << " normals ( 1 vector ( 0 1 0 ) )\r\n"
        << " uv_points ( 8\r\n";
    for(int i = 0; i < 8; ++i)
        out << "  uv_point ( " << (i & 1) << ' ' << ((i >> 1) & 1) << " )\r\n";
    out << " )\r\n images ( 1 image ( tree.ace ) )\r\n"
        << " sub_objects ( 1 sub_object ( vertices ( 8\r\n";
    for(int i = 0; i < 8; ++i)
        out << "  vertex ( 00000000 " << i
            << " 0 FFFFFFFF FF000000 vertex_uvs ( 1 " << i << " ) )\r\n";
    const int indices[24] = {
        0,1,2, 0,2,3, 2,1,0, 3,2,0,
        4,5,6, 4,6,7, 6,5,4, 7,6,4
    };
    out << " ) primitives ( 1 indexed_trilist ( vertex_idxs ( 24";
    for(int i = 0; i < 24; ++i)
        out << ' ' << (invalidIndex && i == 23 ? 99 : indices[i]);
    out << " ) ) ) ) )\r\n)\r\n";
    out.flush();
    return file.error() == QFileDevice::NoError;
}
}

int main(int argc, char **argv) {
    if(argc > 2) {
        std::cerr << "Usage: ForestPatchBakerProbe [UTF-16 cruciform .s]\n";
        return 2;
    }
    QTemporaryDir temporary;
    if(!temporary.isValid()) return 2;
    const QString sourcePath = argc == 2
        ? QString::fromLocal8Bit(argv[1])
        : temporary.filePath("cruciform.s");
    if(argc == 1 && !writeCruciformFixture(sourcePath)) return 2;
    ForestShapeMesh source;
    QString error;
    bool passed = check(ForestShapeTextIO::readCruciform(sourcePath, source, error),
                        "actual vegetation shape should load");
    if(!passed) {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }
    passed &= check(source.vertices.size() == 8, "cruciform vertex count");
    passed &= check(source.indices.size() == 24, "cruciform index count");
    passed &= check(!source.texture.isEmpty(), "source texture retained");

    QVector<ForestBakeInstance> instances;
    instances.append({sourcePath, -150.0, 100.0, -20.0, 0.0, 1.0});
    instances.append({sourcePath, -20.0, 102.0, -10.0, 45.0, 1.5});
    instances.append({sourcePath, -5.0, 98.0, -15.0, 90.0, 0.75});
    const ForestPatchBakeResult baked = ForestPatchBaker::bake(instances);
    passed &= check(baked.isValid(), "bake should succeed");
    passed &= check(baked.patches.size() == 2,
                    "instances split across adjacent 128 m terrain patches");
    int instanceTotal = 0;
    for(const ForestBakedPatch &patch : baked.patches)
        instanceTotal += patch.sourceInstanceCount;
    passed &= check(instanceTotal == 3, "all instances retained");

    const ForestPatchBakeResult blockBaked = ForestPatchBaker::bake(instances, 4);
    passed &= check(blockBaked.isValid(), "4x4 patch block bake should succeed");
    passed &= check(blockBaked.patches.size() == 1,
                    "adjacent terrain patches merge into one 4x4 block");
    passed &= check(blockBaked.patches.front().sourceInstanceCount == 3,
                    "4x4 block retains every source instance");

    QVector<ForestBakeInstance> invalidInstances = instances;
    invalidInstances.append({sourcePath,
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0, 1.0});
    passed &= check(!ForestPatchBaker::bake(invalidInstances).isValid(),
                    "non-finite instance coordinates should be rejected");

    const QString invalidShapePath = temporary.filePath("invalid-index.s");
    passed &= check(writeCruciformFixture(invalidShapePath, true),
                    "invalid-index fixture should write");
    ForestShapeMesh invalidShape;
    error.clear();
    passed &= check(!ForestShapeTextIO::readCruciform(
                    invalidShapePath, invalidShape, error),
                    "out-of-range triangle index should be rejected");

    for(int i = 0; i < baked.patches.size(); ++i) {
        const QString path = temporary.filePath(QString("patch-%1.s").arg(i));
        error.clear();
        passed &= check(ForestShapeTextIO::writePatch(path, baked.patches[i], error),
                        "baked shape should write");
        QFile file(path);
        passed &= check(file.open(QIODevice::ReadOnly), "baked shape should reopen");
        const QByteArray header = file.read(34);
        passed &= check(header.size() >= 2
            && static_cast<unsigned char>(header[0]) == 0xff
            && static_cast<unsigned char>(header[1]) == 0xfe,
            "baked shape should be UTF-16LE");
        ForestShapeMesh roundTrip;
        error.clear();
        passed &= check(ForestShapeTextIO::readCruciform(path, roundTrip, error),
                        "baked shape should round-trip through CPU reader");
        int expectedVertices = 0, expectedIndices = 0;
        for(const ForestShapeMesh &mesh : baked.patches[i].meshes) {
            expectedVertices += mesh.vertices.size();
            expectedIndices += mesh.indices.size();
        }
        passed &= check(roundTrip.vertices.size() == expectedVertices,
                        "round-trip vertex count");
        passed &= check(roundTrip.indices.size() == expectedIndices,
                        "round-trip index count");
        error.clear();
        passed &= check(ForestShapeTextIO::writeDescriptor(path, error),
                        "baked shape descriptor should write");
        QFile descriptor(temporary.filePath(QString("patch-%1.sd").arg(i)));
        passed &= check(descriptor.open(QIODevice::ReadOnly | QIODevice::Text),
                        "baked shape descriptor should reopen");
        const QByteArray descriptorText = descriptor.readAll();
        passed &= check(descriptorText.contains("ESD_Detail_Level ( 0 )"),
                        "descriptor uses always-visible detail level");
        passed &= check(descriptorText.contains("ESD_Alternative_Texture ( 252 )"),
                        "descriptor enables seasonal vegetation textures");
        passed &= check(!descriptorText.contains("ESD_Bounding_Box"),
                        "descriptor leaves bounds to shape geometry");
    }
    if(passed) std::cout << "Forest patch baker probe passed.\n";
    return passed ? 0 : 1;
}

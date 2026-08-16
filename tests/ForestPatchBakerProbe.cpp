#include "ForestPatchBaker.h"

#include <QFile>
#include <QTemporaryDir>

#include <cmath>
#include <iostream>

namespace {
bool check(bool condition, const char *message) {
    if(condition) return true;
    std::cerr << "FAILED: " << message << '\n';
    return false;
}
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cerr << "Usage: ForestPatchBakerProbe <UTF-16 cruciform .s>\n";
        return 2;
    }
    const QString sourcePath = QString::fromLocal8Bit(argv[1]);
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

    QTemporaryDir temporary;
    passed &= check(temporary.isValid(), "temporary output directory");
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

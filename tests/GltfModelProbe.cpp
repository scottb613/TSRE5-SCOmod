// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "GltfModel.h"
#include "GltfPlacementMath.h"
#include "GltfTextureBudget.h"
#include <QDir>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtEndian>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void check(bool success, const char *message) {
    if(!success) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
void append32(QByteArray &bytes, quint32 value) {
    const quint32 little = qToLittleEndian(value);
    bytes.append(reinterpret_cast<const char *>(&little), 4);
}
QByteArray geometry() {
    QByteArray bytes;
    for(float value : {1.f, 0.f, 0.f, 2.f, 0.f, 0.f, 1.f, 1.f, 0.f}) {
        quint32 bits;
        std::memcpy(&bits, &value, 4);
        append32(bytes, bits);
    }
    bytes.append(char(0)); bytes.append(char(1)); bytes.append(char(2));
    return bytes;
}
QJsonObject fixture(const QByteArray &bytes) {
    return QJsonObject{
        {"asset", QJsonObject{{"version", "2.0"}}},
        {"buffers", QJsonArray{QJsonObject{{"byteLength", bytes.size()},
            {"uri", "data:application/octet-stream;base64," + QString::fromLatin1(bytes.toBase64())}}}},
        {"bufferViews", QJsonArray{QJsonObject{{"buffer", 0}, {"byteOffset", 0}, {"byteLength", 36}},
            QJsonObject{{"buffer", 0}, {"byteOffset", 36}, {"byteLength", 3}}}},
        {"accessors", QJsonArray{QJsonObject{{"bufferView", 0}, {"componentType", 5126},
            {"count", 3}, {"type", "VEC3"}}, QJsonObject{{"bufferView", 1},
            {"componentType", 5121}, {"count", 3}, {"type", "SCALAR"}}}},
        {"meshes", QJsonArray{QJsonObject{{"primitives", QJsonArray{QJsonObject{
            {"attributes", QJsonObject{{"POSITION", 0}}}, {"indices", 1}}}}}}},
        {"nodes", QJsonArray{QJsonObject{{"mesh", 0}, {"translation", QJsonArray{3, 2, 4}}}}},
        {"scenes", QJsonArray{QJsonObject{{"nodes", QJsonArray{0}}}}}, {"scene", 0}
    };
}
bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
bool load(GltfModel &model, const QString &path, const QJsonObject &json, QString &error) {
    check(write(path, QJsonDocument(json).toJson(QJsonDocument::Compact)), "write fixture");
    return model.load(path, error);
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }
QByteArray fileHash(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) { check(false, "open asset for read-only verification"); return {}; }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    check(hash.addData(&file), "hash asset");
    return hash.result();
}
QByteArray glb(QJsonObject json, QByteArray bin) {
    QJsonArray buffers = json["buffers"].toArray();
    QJsonObject buffer = buffers[0].toObject(); buffer.remove("uri"); buffers[0] = buffer;
    json["buffers"] = buffers;
    QByteArray text = QJsonDocument(json).toJson(QJsonDocument::Compact);
    while(text.size() % 4) text.append(' ');
    while(bin.size() % 4) bin.append(char(0));
    QByteArray result;
    append32(result, 0x46546c67); append32(result, 2);
    append32(result, quint32(12 + 8 + text.size() + 8 + bin.size()));
    append32(result, quint32(text.size())); append32(result, 0x4e4f534a); result += text;
    append32(result, quint32(bin.size())); append32(result, 0x004e4942); result += bin;
    return result;
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    check(gltfTextureBytes(QSize(1, 8), true) == 60, "narrow texture full mip-chain budget");
    check(gltfTextureBytes(QSize(3, 5), true) == 72, "non-power-of-two mip-chain budget");
    check(gltfTextureBytes(QSize(3, 5), false) == 60, "non-mipmapped texture budget");
    check(argc == 2, "CTest supplies Blender fixture directory");
    if(argc != 2) return 1;
    const QDir realFixtures(QString::fromLocal8Bit(argv[1]));
    const QStringList preservedFiles{"orts_static_probe.glb", "orts_static_probe.gltf",
            "orts_static_probe.bin", "probe_quadrants.png"};
    QVector<QByteArray> originalHashes;
    for(const QString &filename : preservedFiles) originalHashes << fileHash(realFixtures.filePath(filename));
    for(const QString &filename : {QString("orts_static_probe.glb"), QString("orts_static_probe.gltf")}) {
        GltfModel exported;
        QString exportedError;
        check(exported.load(realFixtures.filePath(filename), exportedError), "load actual Blender fixture");
        if(exported.primitives.empty()) { std::cerr << exportedError.toStdString() << '\n'; continue; }
        check(exported.primitives.size() == 5, "Blender fixture has five material primitives");
        check((exported.minimum - QVector3D(-2, 0, -3)).length() < .0001f
                && (exported.maximum - QVector3D(2, 2.85f, 3)).length() < .0001f,
                "Blender metre scale and OR bounds");
        int masked = 0;
        for(const auto &primitive : exported.primitives) if(primitive.masked) {
            ++masked;
            check(!primitive.image.isNull() && primitive.doubleSided && near(primitive.alphaCutoff, .5),
                    "Blender double-sided cutout material");
            check(primitive.image.pixelColor(primitive.image.width()/2, primitive.image.height()/2).alpha() == 0,
                    "Blender texture contains transparent picking hole");
        }
        check(masked == 1 && exported.warnings.isEmpty(), "fixture uses implemented ORTS unlit subset");
    }
    for(qsizetype i = 0; i < preservedFiles.size(); ++i)
        check(originalHashes[i] == fileHash(realFixtures.filePath(preservedFiles[i])),
                "model loading preserves source asset and resources byte-for-byte");
    QMatrix4x4 legacy;
    legacy.translate(10, 20, -30);
    legacy.rotate(180, 0, -1, 0);
    const QMatrix4x4 placed = GltfPlacementMath::matrix(legacy.constData());
    check((placed.map(QVector3D(1, 2, 3)) - QVector3D(11, 22, -33)).length() < .0001f,
            "OR local position maps to editor world without recentering pivot");
    legacy.setToIdentity();
    legacy.translate(10, 20, -30);
    legacy.rotate(90, 0, 1, 0);
    legacy.rotate(180, 0, -1, 0);
    legacy.scale(2, 3, 4);
    check((GltfPlacementMath::matrix(legacy.constData()).map(QVector3D(1, 2, 3))
                - QVector3D(-2, 26, -32)).length() < .0001f, "rotated scaled route placement");
    const int pickId = (7 << 20) | (2345 << 4);
    const QVector3D pickRgb = GltfPlacementMath::selectionRgb(pickId);
    check((qRound(pickRgb.x()*255) << 16 | qRound(pickRgb.y()*255) << 8 | qRound(pickRgb.z()*255)) == pickId,
            "selection identifier survives RGB encoding");
    QVector<float> outline;
    GltfPlacementMath::boxPoints(QVector3D(-2, 0, -3), QVector3D(2, 2.85f, 3), outline);
    check(outline.size() == 72, "selection outline has twelve edges");
    QTemporaryDir directory;
    check(directory.isValid(), "temporary fixture directory");
    if(!directory.isValid()) return 1;
    const QString path = directory.filePath(QString::fromUtf8("model-\xc3\xa9.GLTF"));
    const QByteArray bytes = geometry();
    const QJsonObject original = fixture(bytes);
    QString error;
    GltfModel model;
    const QString diagnosticPath = qEnvironmentVariable("TSRE_GLTF_DIAGNOSTIC_MODEL");
    if(!diagnosticPath.isEmpty()) {
        GltfModel diagnostic;
        check(diagnostic.load(diagnosticPath, error), "operator model loads in authored rest pose");
        if(diagnostic.primitives.empty()) std::cerr << error.toStdString() << '\n';
        else std::cout << "Operator model: " << diagnostic.primitives.size() << " primitives; "
                       << diagnostic.warnings.join("; ").toStdString() << '\n';
    }
    check(GltfModel::accepts("scenery.GLB") && GltfModel::accepts(path)
            && !GltfModel::accepts("legacy.s"), "format dispatch preserves .s ownership");
    check(load(model, path, original, error), "load embedded indexed triangle");
    check(model.primitives.size() == 1, "one primitive");
    if(model.primitives.empty()) { std::cerr << error.toStdString(); return 1; }
    check(near(model.minimum.x(), -5) && near(model.maximum.x(), -4)
            && near(model.minimum.y(), 2) && near(model.maximum.y(), 3)
            && near(model.minimum.z(), -4), "node translation and OR forward conversion bounds");
    check(model.primitives[0].vertices.size() == 15, "indexed mesh expands three vertices");
    const QString binaryPath = directory.filePath("model.glb");
    check(write(binaryPath, glb(original, bytes)), "write GLB");
    check(model.load(binaryPath, error), "load GLB binary chunk");
    check(near(model.minimum.x(), -5), "GLB and glTF geometry match");

    QJsonObject json = original;
    QByteArray animatedBytes = bytes;
    while(animatedBytes.size() % 4) animatedBytes.append(char(0));
    const int inputOffset = int(animatedBytes.size());
    append32(animatedBytes, 0);
    const int outputOffset = int(animatedBytes.size());
    quint32 ninetyNine;
    const float animatedTranslation = 99.f;
    std::memcpy(&ninetyNine, &animatedTranslation, 4);
    for(int axis = 0; axis < 3; ++axis) append32(animatedBytes, ninetyNine);
    json["buffers"] = QJsonArray{QJsonObject{{"byteLength", animatedBytes.size()},
        {"uri", "data:application/octet-stream;base64," + QString::fromLatin1(animatedBytes.toBase64())}}};
    auto animationViews = json["bufferViews"].toArray();
    animationViews.append(QJsonObject{{"buffer", 0}, {"byteOffset", inputOffset}, {"byteLength", 4}});
    animationViews.append(QJsonObject{{"buffer", 0}, {"byteOffset", outputOffset}, {"byteLength", 12}});
    json["bufferViews"] = animationViews;
    auto animationAccessors = json["accessors"].toArray();
    animationAccessors.append(QJsonObject{{"bufferView", 2}, {"componentType", 5126}, {"count", 1}, {"type", "SCALAR"}});
    animationAccessors.append(QJsonObject{{"bufferView", 3}, {"componentType", 5126}, {"count", 1}, {"type", "VEC3"}});
    json["accessors"] = animationAccessors;
    json["animations"] = QJsonArray{QJsonObject{
        {"samplers", QJsonArray{QJsonObject{{"input", 2}, {"output", 3}, {"interpolation", "LINEAR"}}}},
        {"channels", QJsonArray{QJsonObject{{"sampler", 0}, {"target", QJsonObject{{"node", 0}, {"path", "translation"}}}}}}}};
    check(load(model, path, json, error) && near(model.minimum.x(), -5)
            && model.warnings.join(" ").contains("rest pose"), "animated node accepted without applying animation keyframes");
    json = original;
    json["extensionsRequired"] = QJsonArray{"MSFT_lod"};
    json["extensionsUsed"] = QJsonArray{"MSFT_lod"};
    check(!load(model, path, json, error) && error.contains("MSFT_lod"), "reject unsupported required extension");
    check(model.primitives.size() == 1 && near(model.minimum.x(), -5), "failed reload preserves loaded model");
    json = original; json["extensionsRequired"] = QJsonArray{"KHR_materials_unlit"};
    check(!load(model, path, json, error), "match OR rejection of required extension missing from extensionsUsed");
    json["extensionsUsed"] = QJsonArray{"KHR_materials_unlit"};
    check(load(model, path, json, error), "accept supported required unlit extension");
    json = original; json["extensionsRequired"] = QJsonArray{"KHR_materials_transmission"};
    json["extensionsUsed"] = QJsonArray{"KHR_materials_transmission"};
    check(!load(model, path, json, error) && error.contains("outside the Open Rails"), "exclude parser features outside ORTS profile");
    for(const QString &extension : {QString("KHR_materials_clearcoat"), QString("KHR_materials_emissive_strength"),
            QString("KHR_materials_ior"), QString("KHR_materials_specular")}) {
        json = original;
        json["extensionsRequired"] = QJsonArray{extension};
        json["extensionsUsed"] = QJsonArray{extension};
        json["materials"] = QJsonArray{QJsonObject{{"extensions", QJsonObject{{extension, QJsonObject{}}}},
                {"pbrMetallicRoughness", QJsonObject{{"baseColorFactor", QJsonArray{.25, .5, .75, 1}}}}}};
        auto effectMeshes = json["meshes"].toArray();
        auto effectMesh = effectMeshes[0].toObject();
        auto effectPrimitives = effectMesh["primitives"].toArray();
        auto effectPrimitive = effectPrimitives[0].toObject(); effectPrimitive["material"] = 0;
        effectPrimitives[0] = effectPrimitive; effectMesh["primitives"] = effectPrimitives;
        effectMeshes[0] = effectMesh; json["meshes"] = effectMeshes;
        check(load(model, path, json, error) && near(model.primitives[0].baseColor.x(), .25f)
                && model.warnings.join(" ").contains(extension), "required ORTS shading effect permits labelled core-material preview");
    }
    json = original;
    json["images"] = QJsonArray{QJsonObject{{"uri", "not-read.dds"}, {"mimeType", "image/vnd-ms.dds"}}};
    check(load(model, path, json, error) && model.warnings.join(" ").contains("ORTS image schema"),
            "warn about ORTS-incompatible MIME on unused optional DDS image");

    json = original;
    QJsonArray nodes = json["nodes"].toArray();
    QJsonObject node = nodes[0].toObject(); node["scale"] = QJsonArray{-1, 2, 1};
    nodes[0] = node; json["nodes"] = nodes;
    check(load(model, path, json, error), "mirrored nonuniform transform");
    check(near(model.minimum.x(), -2) && near(model.maximum.x(), -1)
            && near(model.maximum.y(), 4), "mirrored scaled bounds");
    check(near(model.primitives[0].vertices[6], 4), "mirrored transform reverses winding");

    json = original;
    nodes = json["nodes"].toArray();
    nodes.append(QJsonObject{{"children", QJsonArray{0}}, {"translation", QJsonArray{0, 5, 0}}});
    json["nodes"] = nodes;
    json["scenes"] = QJsonArray{QJsonObject{{"nodes", QJsonArray{1}}}};
    check(load(model, path, json, error) && near(model.minimum.y(), 7), "hierarchical transforms");

    json = original;
    nodes = json["nodes"].toArray();
    nodes.append(QJsonObject{{"mesh", 0}, {"translation", QJsonArray{100, 0, 0}}});
    json["nodes"] = nodes;
    check(load(model, path, json, error) && model.primitives.size() == 1, "only active scene is loaded");

    json = original;
    QJsonArray buffers = json["buffers"].toArray();
    QJsonObject buffer = buffers[0].toObject(); buffer["uri"] = "geometry.bin";
    buffers[0] = buffer; json["buffers"] = buffers;
    check(write(directory.filePath("geometry.bin"), bytes), "write local buffer");
    check(load(model, path, json, error), "local external buffer with Unicode model path");
    for(const QString &uri : {QString("missing.bin"), QString("../outside.bin"),
            QString("https://example.invalid/mesh.bin"), QString("file:///C:/mesh.bin")}) {
        buffer["uri"] = uri; buffers[0] = buffer; json["buffers"] = buffers;
        check(!load(model, path, json, error), "reject missing, escaped, or remote resource");
    }
    QByteArray badIndices = bytes; badIndices[38] = char(99);
    check(!load(model, path, fixture(badIndices), error), "reject out-of-range index");
    QByteArray badFloat = bytes;
    const quint32 nan = qToLittleEndian(quint32(0x7fc00000));
    std::memcpy(badFloat.data(), &nan, 4);
    check(!load(model, path, fixture(badFloat), error), "reject non-finite geometry");
    json = original;
    QJsonArray accessors = json["accessors"].toArray();
    QJsonObject accessor = accessors[0].toObject(); accessor["count"] = 1000000000;
    accessors[0] = accessor; json["accessors"] = accessors;
    check(!load(model, path, json, error), "reject huge accessor before range arithmetic");
    check(write(binaryPath, glb(original, bytes).left(20)), "write truncated GLB");
    check(!model.load(binaryPath, error), "reject truncated GLB");
    check(write(path, "not JSON"), "write malformed JSON");
    check(!model.load(path, error), "reject malformed JSON");

    // Texture fixture: UVs, embedded PNG, linear factor, alpha mask.
    QByteArray texturedBytes = bytes;
    texturedBytes.append(char(0)); // Float accessor offset must be four-byte aligned.
    for(float value : {0.f, 0.f, 1.f, 0.f, 0.f, 1.f}) {
        quint32 bits; std::memcpy(&bits, &value, 4); append32(texturedBytes, bits);
    }
    json = fixture(texturedBytes);
    QJsonArray views = json["bufferViews"].toArray();
    views.append(QJsonObject{{"buffer", 0}, {"byteOffset", 40}, {"byteLength", 24}});
    json["bufferViews"] = views;
    accessors = json["accessors"].toArray();
    accessors.append(QJsonObject{{"bufferView", 2}, {"componentType", 5126}, {"count", 3}, {"type", "VEC2"}});
    json["accessors"] = accessors;
    QJsonArray meshes = json["meshes"].toArray();
    QJsonObject mesh = meshes[0].toObject();
    QJsonArray primitives = mesh["primitives"].toArray();
    QJsonObject primitive = primitives[0].toObject();
    primitive["attributes"] = QJsonObject{{"POSITION", 0}, {"TEXCOORD_0", 2}};
    primitive["material"] = 0; primitives[0] = primitive; mesh["primitives"] = primitives;
    meshes[0] = mesh; json["meshes"] = meshes;
    QImage image(2, 2, QImage::Format_RGBA8888); image.fill(Qt::red);
    check(image.save(directory.filePath("color.png"), "PNG"), "write PNG texture");
    json["images"] = QJsonArray{QJsonObject{{"uri", "color.png"}}};
    json["textures"] = QJsonArray{QJsonObject{{"source", 0}}};
    QJsonObject material{{"alphaMode", "MASK"}, {"doubleSided", true},
        {"pbrMetallicRoughness", QJsonObject{{"baseColorTexture", QJsonObject{{"index", 0}}},
            {"baseColorFactor", QJsonArray{0.5, 1, 1, 1}}}}};
    json["materials"] = QJsonArray{material};
    check(load(model, path, json, error), "load textured masked material");
    check(model.primitives[0].image.size() == QSize(2, 2) && model.primitives[0].masked
            && model.primitives[0].doubleSided && near(model.primitives[0].baseColor.x(), 0.5), "texture and material data");
    QByteArray png;
    QBuffer pngBuffer(&png); pngBuffer.open(QIODevice::WriteOnly);
    check(image.save(&pngBuffer, "PNG"), "encode embedded PNG");
    json["images"] = QJsonArray{QJsonObject{{"uri", "data:image/png;base64," + QString::fromLatin1(png.toBase64())}}};
    check(load(model, path, json, error), "base64 embedded PNG");
    const int pngOffset = int(texturedBytes.size());
    texturedBytes += png;
    buffers = json["buffers"].toArray();
    buffer = buffers[0].toObject(); buffer["byteLength"] = texturedBytes.size();
    buffer["uri"] = "data:application/octet-stream;base64," + QString::fromLatin1(texturedBytes.toBase64());
    buffers[0] = buffer; json["buffers"] = buffers;
    views.append(QJsonObject{{"buffer", 0}, {"byteOffset", pngOffset}, {"byteLength", png.size()}});
    json["bufferViews"] = views;
    json["images"] = QJsonArray{QJsonObject{{"bufferView", 3}, {"mimeType", "image/png"}}};
    check(write(binaryPath, glb(json, texturedBytes)), "write GLB with embedded PNG");
    check(model.load(binaryPath, error) && model.primitives[0].image.size() == QSize(2, 2), "GLB buffer-view PNG");
    material["alphaMode"] = "BLEND"; json["materials"] = QJsonArray{material};
    check(!load(model, path, json, error), "reject unsupported blend rather than rendering opaque");
    std::cout << "glTF model probe: " << failures << " failures\n";
    return failures ? 1 : 0;
}

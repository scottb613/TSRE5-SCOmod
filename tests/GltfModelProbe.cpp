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
    QMatrix4x4 roster;
    roster.translate(0, 0, 20);
    const QMatrix4x4 vehicle = GltfPlacementMath::rollingStockMatrix(roster.constData());
    check(vehicle.determinant() > 0
            && (vehicle.map(QVector3D(1, 2, 3)) - QVector3D(-1, 2, 17)).length() < .0001f,
            "rolling stock aligns forward direction while retaining handedness and offset");
    roster.rotate(180, 0, 1, 0);
    const QMatrix4x4 flippedVehicle = GltfPlacementMath::rollingStockMatrix(roster.constData());
    check(flippedVehicle.determinant() > 0
            && (flippedVehicle.map(QVector3D(1, 2, 3)) - QVector3D(1, 2, 23)).length() < .0001f,
            "consist flip rotates rolling stock without mirroring its artwork");
    QMatrix4x4 loaderOrientation;
    loaderOrientation.rotate(180, 0, 1, 0);
    check(((vehicle * loaderOrientation).map(QVector3D(0, 0, 3))
                - QVector3D(0, 0, 23)).length() < .0001f,
            "loaded glTF forward axis matches legacy rolling-stock local Z");
    check(((flippedVehicle * loaderOrientation).map(QVector3D(0, 0, 3))
                - QVector3D(0, 0, 17)).length() < .0001f,
            "normal consist flip reverses loaded glTF forward axis once");
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
    check(load(model, path, json, error) && !model.warnings.join(" ").contains("ORTS image schema"),
            "recognize DDS MIME metadata supported by the current Open Rails loader");

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
    image.setPixelColor(1, 1, QColor(0, 0, 255, 0));
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
    check(load(model, path, json, error) && model.primitives[0].image.isNull()
            && model.warnings.join(" ").contains("direct image data URIs"),
            "match Open Rails when a material uses a direct image data URI");
    const auto embedImage = [&](const QByteArray &payload, const QString &mime) {
        const QByteArray bin = texturedBytes + payload;
        auto embedded = json;
        embedded["buffers"] = QJsonArray{QJsonObject{{"byteLength", bin.size()},
            {"uri", "data:application/octet-stream;base64," + QString::fromLatin1(bin.toBase64())}}};
        auto imageViews = views;
        imageViews.append(QJsonObject{{"buffer", 0}, {"byteOffset", texturedBytes.size()},
            {"byteLength", payload.size()}});
        embedded["bufferViews"] = imageViews;
        embedded["images"] = QJsonArray{QJsonObject{{"bufferView", 3}, {"mimeType", mime}}};
        return embedded;
    };
    json = embedImage(png, "image/png");
    check(QFile::remove(directory.filePath("color.png")), "remove texture sidecar before embedded tests");
    check(load(model, path, json, error) && model.primitives[0].image.size() == QSize(2, 2)
            && model.primitives[0].image.pixelColor(0, 0) == QColor(Qt::red)
            && model.primitives[0].image.pixelColor(1, 1).alpha() == 0,
            "self-contained glTF buffer-view PNG preserves pixels and alpha");
    const QImage decodedPng = model.primitives[0].image;
    check(write(binaryPath, glb(json, texturedBytes + png)), "write GLB with embedded PNG");
    check(model.load(binaryPath, error) && model.primitives[0].image == decodedPng,
            "GLB buffer-view PNG preserves pixels and alpha");
    check(!load(model, path, embedImage(png, "image/jpeg"), error)
            && error.contains("declared PNG/JPEG MIME type") && model.primitives[0].image == decodedPng,
            "reject mismatched embedded MIME type and preserve previously loaded texture");
    check(!load(model, path, embedImage(png, {}), error) && error.contains("MIME type"),
            "reject absent MIME type for an image buffer view");
    auto dualSource = json;
    dualSource["images"] = QJsonArray{QJsonObject{{"uri", "color.png"},
        {"bufferView", 3}, {"mimeType", "image/png"}}};
    check(!load(model, path, dualSource, error) && error.contains("no URI"),
            "reject competing URI and buffer-view image sources");

    QImage opaqueImage(2, 2, QImage::Format_RGB32); opaqueImage.fill(Qt::red);
    QByteArray jpeg;
    QBuffer jpegBuffer(&jpeg); jpegBuffer.open(QIODevice::WriteOnly);
    check(opaqueImage.save(&jpegBuffer, "JPEG", 100), "encode embedded JPEG");
    const auto jpegModel = embedImage(jpeg, "image/jpeg");
    check(load(model, path, jpegModel, error) && model.primitives[0].image.size() == QSize(2, 2)
            && model.primitives[0].image.pixelColor(0, 0).red() > 240,
            "self-contained glTF buffer-view JPEG decodes actual pixels");
    check(write(binaryPath, glb(jpegModel, texturedBytes + jpeg)), "write GLB with embedded JPEG");
    check(model.load(binaryPath, error) && model.primitives[0].image.size() == QSize(2, 2)
            && model.primitives[0].image.pixelColor(0, 0).red() > 240,
            "GLB buffer-view JPEG decodes actual pixels");

    // A BC1/DXT1 block whose first RGB565 endpoint is pure red.
    QByteArray dds(136, char(0));
    std::memcpy(dds.data(), "DDS ", 4);
    qToLittleEndian<quint32>(124, dds.data() + 4);
    qToLittleEndian<quint32>(0x81007, dds.data() + 8);
    qToLittleEndian<quint32>(4, dds.data() + 12);
    qToLittleEndian<quint32>(4, dds.data() + 16);
    qToLittleEndian<quint32>(8, dds.data() + 20);
    qToLittleEndian<quint32>(32, dds.data() + 76);
    qToLittleEndian<quint32>(4, dds.data() + 80);
    std::memcpy(dds.data() + 84, "DXT1", 4);
    qToLittleEndian<quint32>(0x1000, dds.data() + 108);
    qToLittleEndian<quint16>(0xf800, dds.data() + 128);
    auto ddsModel = embedImage(dds, "image/vnd-ms.dds");
    ddsModel["extensionsUsed"] = QJsonArray{"MSFT_texture_dds"};
    ddsModel["extensionsRequired"] = QJsonArray{"MSFT_texture_dds"};
    const QJsonObject ddsTexture{{"extensions", QJsonObject{
        {"MSFT_texture_dds", QJsonObject{{"source", 0}}}}}};
    ddsModel["textures"] = QJsonArray{ddsTexture};
    check(load(model, path, ddsModel, error) && model.primitives[0].image.size() == QSize(4, 4)
            && model.primitives[0].image.pixelColor(0, 0) == QColor(Qt::red),
            "self-contained glTF loads the required MSFT_texture_dds source");
    check(write(binaryPath, glb(ddsModel, texturedBytes + dds)), "write GLB with embedded DDS");
    check(model.load(binaryPath, error) && model.primitives[0].image.size() == QSize(4, 4)
            && model.primitives[0].image.pixelColor(0, 0) == QColor(Qt::red),
            "GLB buffer-view DDS decodes actual pixels");
    check(!model.warnings.join(' ').contains("Optional ORTS extension is not previewed yet: MSFT_texture_dds"),
            "supported DDS extension does not produce an unsupported-preview warning");
    {
        // Put the DDS above both former GLB size limits without allocating the
        // intervening padding in the test process.
        constexpr quint32 imageOffset = 340 * 1024 * 1024;
        const quint32 binSize = imageOffset + quint32(dds.size());
        auto largeJson = ddsModel;
        largeJson["buffers"] = QJsonArray{QJsonObject{{"byteLength", qint64(binSize)}}};
        auto largeViews = largeJson["bufferViews"].toArray();
        auto imageView = largeViews[3].toObject();
        imageView["byteOffset"] = qint64(imageOffset);
        largeViews[3] = imageView;
        largeJson["bufferViews"] = largeViews;
        QByteArray prefix = glb(largeJson, texturedBytes);
        const quint32 binOffset = 28 + qFromLittleEndian<quint32>(prefix.constData() + 12);
        qToLittleEndian<quint32>(binOffset + binSize, prefix.data() + 8);
        qToLittleEndian<quint32>(binSize, prefix.data() + binOffset - 8);

        const QString largePath = directory.filePath("large-embedded-dds.glb");
        QFile largeFile(largePath);
        const bool prepared = largeFile.open(QIODevice::WriteOnly)
            && largeFile.write(prefix) == prefix.size()
            && largeFile.seek(qint64(binOffset) + imageOffset)
            && largeFile.write(dds) == dds.size();
        largeFile.close();
        check(prepared, "write GLB with an embedded DDS above 320 MiB");
        GltfModel largeModel;
        check(prepared && largeModel.load(largePath, error) && !largeModel.primitives.empty()
                && largeModel.primitives[0].image.pixelColor(0, 0) == QColor(Qt::red),
                "decode embedded DDS from a large file-backed GLB");
        check(QFile::remove(largePath), "release the file mapping after loading");
    }
    auto largeExternal = json;
    largeExternal["buffers"] = QJsonArray{QJsonObject{{"byteLength", 257 * 1024 * 1024},
        {"uri", "large.bin"}}};
    check(!load(model, path, largeExternal, error) && error.contains("buffer budget"),
            "separately loaded buffers retain the allocation limit");
    auto invalidDds = ddsModel;
    invalidDds["images"] = QJsonArray{QJsonObject{{"bufferView", 3}, {"mimeType", "image/png"}}};
    check(!load(model, path, invalidDds, error) && error.contains("MIME type"),
            "reject an MSFT_texture_dds buffer view without the DDS MIME type");
    QByteArray oversizedDds = dds;
    qToLittleEndian<quint32>(8192, oversizedDds.data() + 12);
    qToLittleEndian<quint32>(8192, oversizedDds.data() + 16);
    check(write(binaryPath, glb(ddsModel, texturedBytes + oversizedDds)), "write DDS over the preview budget");
    check(!model.load(binaryPath, error) && error.contains("DDS image is too large"),
            "reject DDS dimensions over the preview budget before decoding");
    QByteArray cubeDds = dds;
    qToLittleEndian<quint32>(0x1008, cubeDds.data() + 108);
    qToLittleEndian<quint32>(0x200, cubeDds.data() + 112);
    check(write(binaryPath, glb(ddsModel, texturedBytes + cubeDds)), "write DDS with cubemap flags");
    check(!model.load(binaryPath, error) && error.contains("2D texture"),
            "reject cubemaps as glTF DDS material textures");
    check(image.save(directory.filePath("color.png"), "PNG"), "write the optional core fallback texture");
    check(write(directory.filePath("color.dds"), dds), "write DDS sidecar");
    auto optionalDds = json;
    optionalDds["extensionsUsed"] = QJsonArray{"MSFT_texture_dds"};
    optionalDds["images"] = QJsonArray{QJsonObject{{"uri", "color.dds"}}, QJsonObject{{"uri", "color.png"}}};
    auto optionalTexture = ddsTexture;
    optionalTexture["source"] = 1;
    optionalDds["textures"] = QJsonArray{optionalTexture};
    check(load(model, path, optionalDds, error) && model.primitives[0].image.size() == QSize(4, 4),
            "select the declared DDS extension image ahead of the optional core image");
    check(write(directory.filePath("color.dds"), dds.left(129)), "truncate selected DDS file");
    check(!load(model, path, optionalDds, error) && error.contains("truncated"),
            "report corrupt selected DDS instead of displaying the core fallback");
    // A conventional 4K texture exceeds decimal 16 million pixels, but fits
    // the bounded preview budget. Exercise the actual GLB image decode path.
    QImage image4k(4096, 4096, QImage::Format_RGBA8888);
    image4k.fill(Qt::green);
    QByteArray png4k;
    QBuffer png4kBuffer(&png4k); png4kBuffer.open(QIODevice::WriteOnly);
    check(image4k.save(&png4kBuffer, "PNG"), "encode 4096-square PNG");
    image4k = QImage();
    check(write(binaryPath, glb(embedImage(png4k, "image/png"), texturedBytes + png4k)),
            "write GLB with embedded 4K PNG");
    check(model.load(binaryPath, error) && !model.primitives.empty()
            && model.primitives[0].image.size() == QSize(4096, 4096),
            "accept standard 4K embedded base-colour texture");

    QImage imageOverBudget(4097, 4096, QImage::Format_RGBA8888);
    imageOverBudget.fill(Qt::green);
    QByteArray pngOverBudget;
    QBuffer pngOverBuffer(&pngOverBudget); pngOverBuffer.open(QIODevice::WriteOnly);
    check(imageOverBudget.save(&pngOverBuffer, "PNG"), "encode over-budget PNG");
    imageOverBudget = QImage();
    check(!load(model, path, embedImage(pngOverBudget, "image/png"), error)
            && error.contains("pixel budget"), "reject image beyond 4K pixel budget");

    material["alphaMode"] = "BLEND"; json["materials"] = QJsonArray{material};
    check(load(model, path, json, error) && !model.primitives.empty()
            && model.primitives[0].blended && !model.primitives[0].masked
            && model.warnings.join(" ").contains("Transparent materials"),
            "accept blended base-colour material with labelled preview limitation");
    {
        // Exercise the source limit without allocating a huge test-side buffer.
        const QString oversizedPath = directory.path() + "/oversized.glb";
        QFile oversized(oversizedPath);
        const bool prepared = oversized.open(QIODevice::WriteOnly)
            && oversized.resize(320ll*1024*1024 + 1);
        oversized.close();
        check(prepared, "prepare oversized source fixture");
        if(prepared)
            check(!model.load(oversizedPath, error) && error.contains("memory budget"),
                "source budget checked before parser or allocation");
    }
    std::cout << "glTF model probe: " << failures << " failures\n";
    return failures ? 1 : 0;
}

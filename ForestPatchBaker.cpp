#include "ForestPatchBaker.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>

namespace {
constexpr double TileSize = 2048.0;
constexpr double PatchSize = 128.0;

QString block(const QString &text, const QString &name, int start = 0) {
    const QRegularExpression re("\\b" + QRegularExpression::escape(name) + "\\s*\\(");
    const QRegularExpressionMatch match = re.match(text, start);
    if(!match.hasMatch()) return {};
    const int open = text.indexOf('(', match.capturedStart());
    int depth = 0;
    for(int i = open; i < text.size(); ++i) {
        if(text.at(i) == '(') ++depth;
        else if(text.at(i) == ')' && --depth == 0)
            return text.mid(open + 1, i - open - 1);
    }
    return {};
}

QVector<double> numbers(const QString &text) {
    static const QRegularExpression re(
        "[-+]?(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][-+]?\\d+)?");
    QVector<double> result;
    auto it = re.globalMatch(text);
    while(it.hasNext()) result.append(it.next().captured().toDouble());
    return result;
}

QVector<QString> repeatedBlocks(const QString &text, const QString &name) {
    QVector<QString> result;
    int position = 0;
    while(true) {
        const QRegularExpression re("\\b" + QRegularExpression::escape(name) + "\\s*\\(");
        const QRegularExpressionMatch match = re.match(text, position);
        if(!match.hasMatch()) break;
        const int open = text.indexOf('(', match.capturedStart());
        int depth = 0, close = -1;
        for(int i = open; i < text.size(); ++i) {
            if(text.at(i) == '(') ++depth;
            else if(text.at(i) == ')' && --depth == 0) { close = i; break; }
        }
        if(close < 0) break;
        result.append(text.mid(open + 1, close - open - 1));
        position = close + 1;
    }
    return result;
}

int tileFor(double coordinate) {
    return static_cast<int>(std::floor((coordinate + 1024.0) / TileSize));
}

ForestPatchKey patchFor(double x, double z, int patchSpan) {
    ForestPatchKey key;
    key.tileX = tileFor(x);
    key.tileZ = tileFor(z);
    const double localX = x - key.tileX*TileSize;
    const double localZ = z - key.tileZ*TileSize;
    const int terrainPatchX = std::clamp(
        static_cast<int>(std::floor((localX + 1024.0)/PatchSize)), 0, 15);
    const int terrainPatchZ = std::clamp(
        static_cast<int>(std::floor((localZ + 1024.0)/PatchSize)), 0, 15);
    key.patchX = terrainPatchX/patchSpan;
    key.patchZ = terrainPatchZ/patchSpan;
    return key;
}

double patchCenter(int tile, int patch, int patchSpan) {
    return tile*TileSize - 1024.0 + (patch*patchSpan + patchSpan*0.5)*PatchSize;
}
}

bool ForestShapeTextIO::readCruciform(const QString &path, ForestShapeMesh &mesh,
                                      QString &error) {
    mesh = {};
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        error = "Unable to open shape: " + path;
        return false;
    }
    const QByteArray bytes = file.readAll();
    if(bytes.size() < 4 || !(static_cast<unsigned char>(bytes[0]) == 0xff
            && static_cast<unsigned char>(bytes[1]) == 0xfe)) {
        error = "Baker currently requires an uncompressed UTF-16 MSTS shape: " + path;
        return false;
    }
    const QString text = QString::fromUtf16(
        reinterpret_cast<const char16_t*>(bytes.constData() + 2), (bytes.size()-2)/2);
    if(!text.startsWith("SIMISA@@@@@@@@@@JINX0s1t______")) {
        error = "Unsupported MSTS shape header: " + path;
        return false;
    }

    QVector<ForestShapeVertex> sourceVertices;
    QVector<QVector3D> points, normals;
    QVector<QPointF> uvs;
    for(const QString &entry : repeatedBlocks(block(text, "points"), "point")) {
        const QVector<double> n = numbers(entry);
        if(n.size() >= 3) points.append(QVector3D(n[0], n[1], n[2]));
    }
    for(const QString &entry : repeatedBlocks(block(text, "normals"), "vector")) {
        const QVector<double> n = numbers(entry);
        if(n.size() >= 3) normals.append(QVector3D(n[0], n[1], n[2]));
    }
    for(const QString &entry : repeatedBlocks(block(text, "uv_points"), "uv_point")) {
        const QVector<double> n = numbers(entry);
        if(n.size() >= 2) uvs.append(QPointF(n[0], n[1]));
    }
    const QString verticesText = block(text, "vertices", text.indexOf("sub_objects"));
    for(const QString &entry : repeatedBlocks(verticesText, "vertex")) {
        static const QRegularExpression vertexHeader(
            "^\\s*[0-9A-Fa-f]+\\s+(-?\\d+)\\s+(-?\\d+)\\s+"
            "[0-9A-Fa-f]+\\s+[0-9A-Fa-f]+");
        const QRegularExpressionMatch header = vertexHeader.match(entry);
        if(!header.hasMatch()) continue;
        const int pointIndex = header.captured(1).toInt();
        const int normalIndex = header.captured(2).toInt();
        const QVector<double> uvn = numbers(block(entry, "vertex_uvs"));
        if(pointIndex < 0 || pointIndex >= points.size() || normalIndex < 0
                || normalIndex >= normals.size() || uvn.size() < 2) continue;
        const int uvIndex = static_cast<int>(uvn[1]);
        if(uvIndex < 0 || uvIndex >= uvs.size()) continue;
        ForestShapeVertex vertex;
        vertex.x = points[pointIndex].x(); vertex.y = points[pointIndex].y();
        vertex.z = points[pointIndex].z(); vertex.nx = normals[normalIndex].x();
        vertex.ny = normals[normalIndex].y(); vertex.nz = normals[normalIndex].z();
        vertex.u = uvs[uvIndex].x(); vertex.v = uvs[uvIndex].y();
        sourceVertices.append(vertex);
    }
    const QVector<double> indexNumbers = numbers(block(block(text, "indexed_trilist"), "vertex_idxs"));
    for(int i = 1; i < indexNumbers.size(); ++i)
        mesh.indices.append(static_cast<int>(indexNumbers[i]));
    mesh.vertices = sourceVertices;
    const QString imageEntry = block(block(text, "images"), "image").trimmed();
    mesh.texture = imageEntry.section(QRegularExpression("\\s+"), 0, 0);
    if(mesh.vertices.isEmpty() || mesh.indices.isEmpty() || mesh.texture.isEmpty()) {
        error = "Shape contains no supported cruciform mesh: " + path;
        return false;
    }
    return true;
}

ForestPatchBakeResult ForestPatchBaker::bake(const QVector<ForestBakeInstance> &instances,
                                             int patchSpan) {
    ForestPatchBakeResult result;
    if(patchSpan < 1 || patchSpan > 16 || 16 % patchSpan != 0) {
        result.errors.append("Patch span must divide the tile's 16-patch grid.");
        return result;
    }
    QHash<QString, ForestShapeMesh> templates;
    QHash<ForestPatchKey, QVector<ForestBakeInstance>> groups;
    for(const ForestBakeInstance &instance : instances)
        groups[patchFor(instance.x, instance.z, patchSpan)].append(instance);

    for(auto group = groups.constBegin(); group != groups.constEnd(); ++group) {
        ForestBakedPatch patch;
        patch.key = group.key();
        patch.originX = patchCenter(patch.key.tileX, patch.key.patchX, patchSpan);
        patch.originZ = patchCenter(patch.key.tileZ, patch.key.patchZ, patchSpan);
        for(const ForestBakeInstance &instance : group.value()) patch.originY += instance.y;
        patch.originY /= static_cast<double>(group.value().size());
        QHash<QString, int> meshByTexture;
        for(const ForestBakeInstance &instance : group.value()) {
            if(!templates.contains(instance.shapePath)) {
                ForestShapeMesh source; QString error;
                if(!ForestShapeTextIO::readCruciform(instance.shapePath, source, error)) {
                    result.errors.append(error); continue;
                }
                templates.insert(instance.shapePath, source);
            }
            const ForestShapeMesh &source = templates[instance.shapePath];
            int meshIndex = meshByTexture.value(source.texture, -1);
            if(meshIndex < 0) {
                meshIndex = patch.meshes.size(); meshByTexture.insert(source.texture, meshIndex);
                ForestShapeMesh mesh; mesh.texture = source.texture; patch.meshes.append(mesh);
            }
            ForestShapeMesh &target = patch.meshes[meshIndex];
            const int offset = target.vertices.size();
            const double yaw = instance.yawDegrees*M_PI/180.0;
            const double c = std::cos(yaw), s = std::sin(yaw);
            for(const ForestShapeVertex &v : source.vertices) {
                ForestShapeVertex out = v;
                out.x = static_cast<float>(instance.x - patch.originX
                    + instance.uniformScale*(c*v.x + s*v.z));
                out.y = static_cast<float>(instance.y - patch.originY + instance.uniformScale*v.y);
                out.z = static_cast<float>(instance.z - patch.originZ
                    + instance.uniformScale*(-s*v.x + c*v.z));
                out.nx = static_cast<float>(c*v.nx + s*v.nz);
                out.nz = static_cast<float>(-s*v.nx + c*v.nz);
                target.vertices.append(out);
            }
            for(int index : source.indices) target.indices.append(offset + index);
            ++patch.sourceInstanceCount;
        }
        if(!patch.meshes.isEmpty()) result.patches.append(patch);
    }
    return result;
}

bool ForestShapeTextIO::writePatch(const QString &path,
                                   const ForestBakedPatch &patch,
                                   QString &error) {
    int vertexCount = 0, indexCount = 0;
    double radius = 1.0;
    for(const ForestShapeMesh &mesh : patch.meshes) {
        vertexCount += mesh.vertices.size();
        indexCount += mesh.indices.size();
        for(const ForestShapeVertex &v : mesh.vertices)
            radius = std::max(radius, std::sqrt(static_cast<double>(v.x)*v.x
                                                + static_cast<double>(v.y)*v.y
                                                + static_cast<double>(v.z)*v.z));
    }
    if(vertexCount == 0 || indexCount == 0 || indexCount % 3 != 0) {
        error = "Baked patch contains no valid triangles.";
        return false;
    }
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)) {
        error = "Unable to create baked shape: " + path;
        return false;
    }
    file.write("\xff\xfe", 2);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf16LE);
    out.setGenerateByteOrderMark(false);
    const int triangles = indexCount/3;
    out << "SIMISA@@@@@@@@@@JINX0s1t______\r\n\r\nshape (\r\n"
        << " shape_header ( 00000000 00000000 )\r\n volumes ( 1 vol_sphere ( vector ( 0 0 0 ) "
        << radius*1.05 << " ) )\r\n shader_names ( 1 named_shader ( BlendATexDiff ) )\r\n"
        << " texture_filter_names ( 1 named_filter_mode ( MipLinear ) )\r\n"
        << " points ( " << vertexCount << "\r\n";
    for(const ForestShapeMesh &mesh : patch.meshes)
        for(const ForestShapeVertex &v : mesh.vertices)
            out << "  point ( " << v.x << ' ' << v.y << ' ' << v.z << " )\r\n";
    out << " )\r\n uv_points ( " << vertexCount << "\r\n";
    for(const ForestShapeMesh &mesh : patch.meshes)
        for(const ForestShapeVertex &v : mesh.vertices)
            out << "  uv_point ( " << v.u << ' ' << v.v << " )\r\n";
    out << " )\r\n normals ( " << vertexCount << "\r\n";
    for(const ForestShapeMesh &mesh : patch.meshes)
        for(const ForestShapeVertex &v : mesh.vertices)
            out << "  vector ( " << v.nx << ' ' << v.ny << ' ' << v.nz << " )\r\n";
    out << " )\r\n sort_vectors ( 1 vector ( 0 0 0 ) )\r\n colours ( 0 )\r\n"
        << " matrices ( 1 matrix MAIN ( 1 0 0 0 1 0 0 0 1 0 0 0 ) )\r\n"
        << " images ( " << patch.meshes.size() << "\r\n";
    for(const ForestShapeMesh &mesh : patch.meshes)
        out << "  image ( " << mesh.texture << " )\r\n";
    out << " )\r\n textures ( " << patch.meshes.size() << "\r\n";
    for(int i = 0; i < patch.meshes.size(); ++i)
        out << "  texture ( " << i << " 0 0 ff000000 )\r\n";
    out << " )\r\n light_materials ( 0 )\r\n"
        << " light_model_cfgs ( 1 light_model_cfg ( 00000000 uv_ops ( 1 uv_op_copy ( 1 0 ) ) ) )\r\n"
        << " vtx_states ( 1 vtx_state ( 00000000 0 -9 0 00000002 ) )\r\n"
        << " prim_states ( " << patch.meshes.size() << "\r\n";
    for(int i = 0; i < patch.meshes.size(); ++i)
        out << "  prim_state forest_patch_" << i
            << " ( 00000000 0 tex_idxs ( 1 " << i << " ) 0 0 1 0 1 )\r\n";
    out << " )\r\n lod_controls ( 1 lod_control ( distance_levels_header ( 0 ) distance_levels ( 1\r\n"
        << "  distance_level ( distance_level_header ( dlevel_selection ( 2000 ) hierarchy ( 1 -1 ) )\r\n"
        << "   sub_objects ( 1 sub_object (\r\n"
        << "    sub_object_header ( 00000400 -1 -1 000001d2 000001c4\r\n"
        << "     geometry_info ( " << triangles << " 1 0 " << indexCount
        << " 0 0 " << patch.meshes.size() << " 0 0 0\r\n"
        << "      geometry_nodes ( 1 geometry_node ( 1 0 0 0 0 cullable_prims ( "
        << patch.meshes.size() << ' ' << triangles << ' ' << indexCount << " ) ) )\r\n"
        << "      geometry_node_map ( 1 0 ) )\r\n"
        << "     subobject_shaders ( 1 0 ) subobject_light_cfgs ( 1 0 ) 0 )\r\n"
        << "    vertices ( " << vertexCount << "\r\n";
    int globalVertex = 0;
    for(const ForestShapeMesh &mesh : patch.meshes) {
        for(const ForestShapeVertex &v : mesh.vertices) {
            out << "     vertex ( 00000000 " << globalVertex << ' ' << globalVertex
                << " FFFFFFFF FF000000 vertex_uvs ( 1 " << globalVertex << " ) )\r\n";
            ++globalVertex;
        }
    }
    out << "    )\r\n vertex_sets ( 1 vertex_set ( 0 0 " << vertexCount << " ) )\r\n"
        << "    primitives ( " << patch.meshes.size()*2 << "\r\n";
    int meshOffset = 0;
    for(int meshIndex = 0; meshIndex < patch.meshes.size(); ++meshIndex) {
        const ForestShapeMesh &mesh = patch.meshes[meshIndex];
        out << "     prim_state_idx ( " << meshIndex << " )\r\n indexed_trilist ( vertex_idxs ( "
            << mesh.indices.size();
        for(int index : mesh.indices) out << ' ' << meshOffset + index;
        const int meshTriangles = mesh.indices.size()/3;
        out << " ) normal_idxs ( " << meshTriangles;
        for(int i = 0; i < meshTriangles; ++i) out << " 0 3";
        out << " ) flags ( " << meshTriangles;
        for(int i = 0; i < meshTriangles; ++i) out << " 00000000";
        out << " ) )\r\n";
        meshOffset += mesh.vertices.size();
    }
    out << "    ) ) ) ) ) ) )\r\n)\r\n";
    out.flush();
    if(!file.commit()) {
        error = "Unable to publish baked shape: " + path;
        return false;
    }
    return true;
}

bool ForestShapeTextIO::writeDescriptor(const QString &shapePath, QString &error) {
    const QFileInfo shapeInfo(shapePath);
    const QString descriptorPath = shapeInfo.absolutePath() + '/'
        + shapeInfo.completeBaseName() + ".sd";
    QSaveFile file(descriptorPath);
    if(!file.open(QIODevice::WriteOnly)) {
        error = "Unable to create baked shape descriptor: " + descriptorPath;
        return false;
    }
    QTextStream out(&file);
    out << "SIMISA@@@@@@@@@@JINX0t1t______\r\n\r\n"
        << "shape ( " << shapeInfo.fileName() << "\r\n"
        << "\tESD_Detail_Level ( 0 )\r\n"
        << "\tESD_Alternative_Texture ( 252 )\r\n"
        << ")\r\n";
    out.flush();
    if(!file.commit()) {
        error = "Unable to publish baked shape descriptor: " + descriptorPath;
        return false;
    }
    return true;
}

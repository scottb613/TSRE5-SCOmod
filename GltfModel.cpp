// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "GltfModel.h"
#include "GltfTextureBudget.h"
#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf/cgltf.h"
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMatrix4x4>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <tuple>

namespace {
constexpr qint64 maxFileBytes = 64 * 1024 * 1024;
constexpr size_t maxResourceBytes = 256 * 1024 * 1024;
constexpr size_t maxVertices = 3000000;
struct ParseBudget { size_t remaining = maxFileBytes; };
void *allocate(void *user, cgltf_size size) {
    auto &budget = *static_cast<ParseBudget *>(user);
    if(size > budget.remaining) return nullptr;
    void *ptr = std::malloc(size);
    if(ptr) budget.remaining -= size;
    return ptr;
}
void deallocate(void *, void *ptr) { std::free(ptr); }

bool readFile(const QString &path, QByteArray &bytes, QString &error) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly) || file.size() < 1 || file.size() > maxFileBytes) {
        error = "Missing, empty, or oversized resource: " + QFileInfo(path).fileName();
        return false;
    }
    bytes = file.read(maxFileBytes + 1);
    if(bytes.size() != file.size() || bytes.size() > maxFileBytes) {
        error = "Cannot read complete resource: " + QFileInfo(path).fileName();
        return false;
    }
    return true;
}

bool readUri(const char *uri, const QDir &directory, QByteArray &bytes, QString &error) {
    const QByteArray encoded(uri ? uri : "");
    if(encoded.startsWith("data:")) {
        const int comma = encoded.indexOf(',');
        if(comma < 0 || !encoded.left(comma).endsWith(";base64")) {
            error = "Only base64 data resources are supported.";
            return false;
        }
        const auto decoded = QByteArray::fromBase64Encoding(encoded.mid(comma + 1),
                QByteArray::AbortOnBase64DecodingErrors);
        if(!decoded || decoded.decoded.size() > maxFileBytes) {
            error = "Invalid or oversized base64 resource.";
            return false;
        }
        bytes = decoded.decoded;
        return true;
    }
    const QUrl url = QUrl::fromEncoded(encoded, QUrl::StrictMode);
    const QString relative = url.path(QUrl::FullyDecoded);
    if(!url.isValid() || !url.isRelative() || !url.host().isEmpty()
            || url.hasQuery() || url.hasFragment() || relative.contains(QChar(0))
            || relative.contains('\\') || relative.contains(':')
            || QDir::isAbsolutePath(relative)) {
        error = "External resources must use relative local paths.";
        return false;
    }
    const QString root = directory.canonicalPath() + '/';
    const QString resolved = QFileInfo(directory.filePath(relative)).canonicalFilePath();
    if(resolved.isEmpty() || !resolved.startsWith(root, Qt::CaseInsensitive)) {
        error = "Missing resource or resource outside the model directory: " + relative;
        return false;
    }
    return readFile(resolved, bytes, error);
}

bool finite(const QVector3D &v) {
    return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
}

bool decodeImage(const cgltf_image *image, const QDir &directory,
        QImage &result, size_t &resourceBytes, QString &error) {
    QByteArray bytes;
    if(image->buffer_view) {
        const auto *view = image->buffer_view;
        if(view->size > maxFileBytes || !cgltf_buffer_view_data(view)) {
            error = "Invalid embedded image.";
            return false;
        }
        bytes = QByteArray(reinterpret_cast<const char *>(cgltf_buffer_view_data(view)),
                static_cast<qsizetype>(view->size));
    } else if(!readUri(image->uri, directory, bytes, error)) return false;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QByteArray format = reader.format().toLower();
    const QSize size = reader.size();
    if((format != "png" && format != "jpeg" && format != "jpg")
            || !size.isValid() || size.width() > 8192 || size.height() > 8192
            || size_t(size.width()) * size_t(size.height()) > 16000000) {
        error = "Preview requires PNG/JPEG images within the 16 megapixel limit.";
        return false;
    }
    const size_t decodedBytes = size_t(size.width()) * size_t(size.height()) * 4;
    if(decodedBytes > maxResourceBytes - resourceBytes) {
        error = "Model exceeds the preview resource budget.";
        return false;
    }
    result = reader.read().convertToFormat(QImage::Format_RGBA8888);
    if(result.isNull()) { error = "Cannot decode model image."; return false; }
    resourceBytes += decodedBytes;
    return true;
}
}

bool GltfModel::accepts(const QString &path) {
    const QString suffix = QFileInfo(path).suffix();
    return suffix.compare("gltf", Qt::CaseInsensitive) == 0
            || suffix.compare("glb", Qt::CaseInsensitive) == 0;
}

bool GltfModel::load(const QString &path, QString &error) {
    error.clear();
    // Commit only a complete load: failed reloads preserve the previous model.
    GltfModel pending;
    if(!accepts(path)) { error = "Expected a .gltf or .glb model."; return false; }
    QByteArray source;
    if(!readFile(path, source, error)) return false;
    ParseBudget budget;
    cgltf_options options{};
    options.memory = {allocate, deallocate, &budget};
    cgltf_data *raw = nullptr;
    if(cgltf_parse(&options, source.constData(), size_t(source.size()), &raw)
            != cgltf_result_success) {
        error = "Invalid glTF/GLB file or parser memory limit exceeded.";
        return false;
    }
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, cgltf_free);
    if(!data->asset.version || QString::fromUtf8(data->asset.version) != "2.0") {
        error = "Only glTF 2.0 is supported."; return false;
    }
    if(data->asset.min_version && QString::fromUtf8(data->asset.min_version) != "2.0") {
        error = "Model requires an unsupported minimum glTF version."; return false;
    }
    if(!data->scenes_count) {
        error = "The Open Rails asset profile requires a scene."; return false;
    }
    // ORTS baseline: 38615b19f75ae2c0742b890f241aa1dd820e8b8c,
    // GltfShape.ExtensionsSupported. Parser capabilities do not expand this list.
    const QStringList ortsExtensions{
        "KHR_animation_pointer", "KHR_materials_unlit", "KHR_materials_clearcoat",
        "KHR_materials_emissive_strength", "KHR_materials_ior", "KHR_materials_specular",
        "KHR_materials_variants", "KHR_mesh_quantization", "KHR_node_visibility",
        "KHR_texture_transform", "KHR_lights_punctual", "EXT_lights_image_based",
        "MSFT_lod", "MSFT_texture_dds", "MSFT_packing_normalRoughnessMetallic",
        "MSFT_packing_occlusionRoughnessMetallic", "ASOBO_asset_optimized",
        "ASOBO_material_draw_order", "ASOBO_normal_map_convention"
    };
    // These ORTS effects change shading, not editable geometry or base-colour
    // coverage. Their authored data stays in the read-only asset. TSRE shows
    // the core material as an explicitly simplified editor preview.
    const QStringList previewMaterialExtensions{
        "KHR_materials_clearcoat", "KHR_materials_emissive_strength",
        "KHR_materials_ior", "KHR_materials_specular"
    };
    QStringList usedExtensions;
    for(size_t i = 0; i < data->extensions_used_count; ++i) {
        const QString extension = QString::fromUtf8(data->extensions_used[i]);
        usedExtensions << extension;
        if(!ortsExtensions.contains(extension))
            pending.warnings << "Ignored optional extension outside the ORTS profile: " + extension;
    }
    for(size_t i = 0; i < data->extensions_required_count; ++i) {
        const QString extension = QString::fromUtf8(data->extensions_required[i]);
        if(!ortsExtensions.contains(extension)) {
            error = "Required extension is outside the Open Rails profile: " + extension; return false;
        }
        if(!usedExtensions.contains(extension)) {
            error = "Required extension is not listed in extensionsUsed: " + extension; return false;
        }
        if(extension != "KHR_materials_unlit" && !previewMaterialExtensions.contains(extension)) {
            error = "Open Rails extension is pending in this preview: " + extension; return false;
        }
    }
    if(data->nodes_count > 10000 || data->buffers_count > 1024
            || data->images_count > 4096) {
        error = "Model exceeds the preview scene limits."; return false;
    }
    // The verified ORTS U2026.10.03 schema rejects non-core MIME enum values
    // even on unused extension images. A PNG preview can otherwise hide this
    // parser limitation. DDS's extension MIME value is valid; warn without
    // rewriting any image or model metadata.
    for(size_t i = 0; i < data->images_count; ++i) {
        const QString mime = QString::fromUtf8(data->images[i].mime_type);
        if(mime == "image/vnd-ms.dds")
            pending.warnings << "Open Rails compatibility: valid MSFT_texture_dds MIME metadata is rejected by the verified ORTS image schema. A PNG/JPEG test fallback avoids this parser limitation.";
        else if(!mime.isEmpty() && mime != "image/png" && mime != "image/jpeg")
            pending.warnings << "Open Rails compatibility: image MIME type '" + mime
                + "' is rejected by the verified ORTS image schema; verify simulator compatibility before testing.";
    }
    // Check arithmetic before cgltf_validate computes accessor byte ranges.
    for(size_t i = 0; i < data->buffers_count; ++i) {
        if(data->buffers[i].size > maxFileBytes) {
            error = "Declared buffer exceeds the preview limit."; return false;
        }
    }
    for(size_t i = 0; i < data->buffer_views_count; ++i) {
        const auto &view = data->buffer_views[i];
        if(!view.buffer || view.offset > view.buffer->size
                || view.size > view.buffer->size - view.offset || view.stride > 256
                || view.has_meshopt_compression) {
            error = "Invalid buffer view or unsupported geometry compression."; return false;
        }
    }
    for(size_t i = 0; i < data->accessors_count; ++i) {
        const auto &accessor = data->accessors[i];
        if(accessor.is_sparse || !accessor.buffer_view || !accessor.count
                || accessor.count > maxVertices || accessor.stride > 256
                || accessor.offset > accessor.buffer_view->size) {
            error = "Invalid accessor or unsupported sparse data."; return false;
        }
    }
    if(cgltf_validate(data.get()) != cgltf_result_success) {
        error = "Invalid model structure or accessor range."; return false;
    }
    const QDir directory = QFileInfo(path).absoluteDir();
    std::vector<QByteArray> buffers(data->buffers_count);
    size_t resourceBytes = 0;
    for(size_t i = 0; i < data->buffers_count; ++i) {
        auto &buffer = data->buffers[i];
        if(buffer.size > maxFileBytes || buffer.size > maxResourceBytes - resourceBytes) {
            error = "Model exceeds the preview buffer budget."; return false;
        }
        resourceBytes += buffer.size;
        if(buffer.uri) {
            if(!readUri(buffer.uri, directory, buffers[i], error)) return false;
            if(size_t(buffers[i].size()) < buffer.size) {
                error = "External buffer is shorter than its declared size."; return false;
            }
            const size_t extraBytes = size_t(buffers[i].size()) - buffer.size;
            if(extraBytes > maxResourceBytes - resourceBytes) {
                error = "Loaded buffers exceed the preview resource budget."; return false;
            }
            resourceBytes += extraBytes;
            buffer.data = buffers[i].data();
        } else if(i == 0 && data->bin && data->bin_size >= buffer.size) {
            buffer.data = const_cast<void *>(data->bin);
        } else { error = "Missing model buffer."; return false; }
        buffer.data_free_method = cgltf_data_free_method_none;
    }
    if(cgltf_validate(data.get()) != cgltf_result_success) {
        error = "Invalid model indices or loaded buffer ranges."; return false;
    }
    std::vector<QImage> images(data->images_count);
    std::vector<bool> imageLoaded(data->images_count, false);
    size_t vertexCount = 0;
    size_t gpuImageBytes = 0;
    std::set<std::tuple<size_t, int, int, int, int>> gpuTextures;
    QMatrix4x4 orientation;
    orientation.rotate(180.0f, 0, 1, 0);
    // Iterative scene traversal, bounded against cycles and excessive depth.
    struct Visit { cgltf_node *node; QMatrix4x4 parent; size_t depth; };
    std::vector<Visit> visits;
    const cgltf_scene *scene = data->scene;
    if(!scene && data->scenes_count) scene = &data->scenes[0];
    for(size_t i = 0; i < scene->nodes_count; ++i)
        visits.push_back({scene->nodes[i], orientation, 0});
    std::vector<bool> seen(data->nodes_count, false);
    bool firstPoint = true;
    while(!visits.empty()) {
        const Visit visit = visits.back(); visits.pop_back();
        const size_t nodeIndex = size_t(visit.node - data->nodes);
        if(visit.depth > 128 || seen[nodeIndex]) {
            error = "Cyclic, repeated, or excessively deep scene node."; return false;
        }
        seen[nodeIndex] = true;
        const auto &node = *visit.node;
        if(node.skin) { error = "Skinned models require the later animation stage."; return false; }
        float localValues[16];
        cgltf_node_transform_local(&node, localValues);
        // cgltf stores matrices column-major; Qt's pointer constructor is row-major.
        QMatrix4x4 local;
        std::copy(localValues, localValues + 16, local.data());
        const QMatrix4x4 world = visit.parent * local;
        for(float value : localValues) {
            if(!std::isfinite(value)) { error = "Non-finite node transform."; return false; }
        }
        for(size_t i = 0; i < node.children_count; ++i)
            visits.push_back({node.children[i], world, visit.depth + 1});
        if(!node.mesh) continue;
        for(size_t p = 0; p < node.mesh->primitives_count; ++p) {
            if(pending.primitives.size() >= 4096) {
                error = "Model exceeds the preview primitive limit."; return false;
            }
            const auto &primitive = node.mesh->primitives[p];
            if(cgltf_find_accessor(&primitive, cgltf_attribute_type_color, 0))
                pending.warnings << "Vertex colors are not previewed yet.";
            if(primitive.type != cgltf_primitive_type_triangles || primitive.targets_count
                    || primitive.has_draco_mesh_compression) {
                error = "Preview requires static, uncompressed triangle meshes."; return false;
            }
            const auto *positions = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
            if(!positions || positions->type != cgltf_type_vec3 || !positions->buffer_view) {
                error = "Mesh has no readable VEC3 positions."; return false;
            }
            const size_t count = primitive.indices ? primitive.indices->count : positions->count;
            if(count == 0 || count % 3 || count > maxVertices - vertexCount) {
                error = "Invalid triangle count or preview vertex limit exceeded."; return false;
            }
            vertexCount += count;
            GltfPrimitive output;
            const cgltf_texture_view *texture = nullptr;
            const auto *material = primitive.material;
            if(material) {
                if(material->alpha_mode == cgltf_alpha_mode_blend) {
                    error = "Blended materials require the later transparency stage."; return false;
                }
                output.doubleSided = material->double_sided;
                output.masked = material->alpha_mode == cgltf_alpha_mode_mask;
                output.alphaCutoff = material->alpha_cutoff;
                const auto &pbr = material->pbr_metallic_roughness;
                output.baseColor = QVector4D(pbr.base_color_factor[0], pbr.base_color_factor[1],
                        pbr.base_color_factor[2], pbr.base_color_factor[3]);
                for(int component = 0; component < 4; ++component) {
                    if(!std::isfinite(output.baseColor[component])
                            || output.baseColor[component] < 0 || output.baseColor[component] > 1) {
                        error = "Invalid base-color factor."; return false;
                    }
                }
                if(!std::isfinite(output.alphaCutoff) || output.alphaCutoff < 0) {
                    error = "Invalid alpha cutoff."; return false;
                }
                texture = &pbr.base_color_texture;
                if(!material->unlit)
                    pending.warnings << "Editor preview uses base colour; final material shading is rendered by Open Rails.";
            }
            const cgltf_accessor *uv = nullptr;
            if(texture && texture->texture) {
                if(texture->has_transform) {
                    error = "Texture transforms require a later preview stage."; return false;
                }
                uv = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, texture->texcoord);
                const auto *image = texture->texture->image;
                if(!image || !uv || uv->type != cgltf_type_vec2 || !uv->buffer_view) {
                    error = "Base-color texture has no supported image or UV set."; return false;
                }
                const size_t imageIndex = size_t(image - data->images);
                if(!imageLoaded[imageIndex]) {
                    if(!decodeImage(image, directory, images[imageIndex], resourceBytes, error)) return false;
                    imageLoaded[imageIndex] = true;
                }
                output.image = images[imageIndex];
                if(texture->texture->sampler) {
                    output.wrapS = texture->texture->sampler->wrap_s;
                    output.wrapT = texture->texture->sampler->wrap_t;
                    const auto *sampler = texture->texture->sampler;
                    if(sampler->min_filter) output.minFilter = sampler->min_filter;
                    if(sampler->mag_filter) output.magFilter = sampler->mag_filter;
                }
                const auto validWrap = [](int value){ return value == 10497 || value == 33071 || value == 33648; };
                if(!validWrap(output.wrapS) || !validWrap(output.wrapT)) {
                    error = "Invalid texture sampler wrap mode."; return false;
                }
                if((output.magFilter != 9728 && output.magFilter != 9729)
                        || (output.minFilter != 9728 && output.minFilter != 9729
                            && (output.minFilter < 9984 || output.minFilter > 9987))) {
                    error = "Invalid texture sampler filter."; return false;
                }
                if(gpuTextures.emplace(imageIndex, output.wrapS, output.wrapT,
                        output.minFilter, output.magFilter).second) {
                    const size_t imageBytes = gltfTextureBytes(output.image.size(), output.minFilter >= 9984);
                    if(imageBytes > maxResourceBytes - gpuImageBytes) {
                        error = "Model exceeds the preview GPU image budget."; return false;
                    }
                    gpuImageBytes += imageBytes;
                }
            }
            output.vertices.reserve(count * 5);
            const bool mirrored = world.determinant() < 0;
            for(size_t v = 0; v < count; ++v) {
                const size_t corner = v % 3;
                const size_t sourceVertex = mirrored && corner ? v - corner + (3 - corner) : v;
                const size_t index = primitive.indices
                        ? cgltf_accessor_read_index(primitive.indices, sourceVertex) : sourceVertex;
                float point[3]; float texcoord[2]{};
                if(index >= positions->count || !cgltf_accessor_read_float(positions, index, point, 3)
                        || (uv && !cgltf_accessor_read_float(uv, index, texcoord, 2))) {
                    error = "Cannot read mesh vertex or index."; return false;
                }
                const QVector3D transformed = world.map(QVector3D(point[0], point[1], point[2]));
                if(!finite(transformed) || !std::isfinite(texcoord[0]) || !std::isfinite(texcoord[1])) {
                    error = "Non-finite mesh vertex."; return false;
                }
                if(firstPoint) { pending.minimum = pending.maximum = transformed; firstPoint = false; }
                for(int axis = 0; axis < 3; ++axis) {
                    pending.minimum[axis] = std::min(pending.minimum[axis], transformed[axis]);
                    pending.maximum[axis] = std::max(pending.maximum[axis], transformed[axis]);
                }
                output.vertices.insert(output.vertices.end(), {transformed.x(), transformed.y(),
                        transformed.z(), texcoord[0], texcoord[1]});
            }
            pending.primitives.push_back(std::move(output));
        }
    }
    if(firstPoint) { error = "Model has no supported mesh in its active scene."; return false; }
    if(data->animations_count) pending.warnings << "Animations are shown in their authored rest pose.";
    for(const QString &extension : usedExtensions)
        if(previewMaterialExtensions.contains(extension))
            pending.warnings << "Simplified editor material: " + extension + " shading is not displayed.";
        else if(extension != "KHR_materials_unlit" && ortsExtensions.contains(extension))
            pending.warnings << "Optional ORTS extension is not previewed yet: " + extension;
    pending.warnings.removeDuplicates();
    *this = std::move(pending);
    return true;
}

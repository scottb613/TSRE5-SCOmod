// TSRE GenX terrain mesh preparation. Licensed under GNU GPL v3 or later.
#include "TerrainMesh.h"

#include <algorithm>

std::unique_ptr<TerrainMeshData> buildTerrainMesh(const TerrainMeshInput &input) {
    const auto &layout = input.layout;
    const int samples = layout.samples;
    const int row = samples + 1;
    const auto index = [row](int x, int z) { return std::size_t(x) * row + z; };
    auto mesh = std::make_unique<TerrainMeshData>();
    mesh->positions.resize(input.heights.size());
    mesh->normals.resize(input.heights.size());
    for (int x = 0; x <= samples; ++x) {
        for (int z = 0; z <= samples; ++z)
            mesh->positions[index(x,z)].set(x * input.sampleSize, input.heights[index(z,x)],
                                           z * input.sampleSize);
    }

    for (int x = 0; x < samples; ++x) {
        for (int z = 0; z < samples; ++z) {
            const auto a = index(x,z);
            const auto b = index(x,z+1);
            const auto c = index(x+1,z+1);
            const auto d = index(x+1,z);
            Vector3f u, v, n;
            u.setFromSub(mesh->positions[a], mesh->positions[d]);
            v.setFromSub(mesh->positions[a], mesh->positions[b]);
            n.setFromCross(v,u);
            mesh->normals[a].add(n);
            mesh->normals[d].add(n);
            mesh->normals[b].add(n);
            u.setFromSub(mesh->positions[c], mesh->positions[d]);
            v.setFromSub(mesh->positions[c], mesh->positions[b]);
            n.setFromCross(u,v);
            mesh->normals[c].add(n);
            mesh->normals[d].add(n);
            mesh->normals[b].add(n);
        }
    }
    for (auto &normal : mesh->normals)
        normal.normalize();

    mesh->patchVertices.resize(layout.totalFloatCount, 0.0f);
    mesh->bounds.resize(qsizetype(layout.patchCount));
    const int patches = layout.patches;
    const int resolution = layout.patchResolution;
    for (int patchX = 0; patchX < patches; ++patchX) {
        for (int patchZ = 0; patchZ < patches; ++patchZ) {
            const int patch = patchZ * patches + patchX;
            const auto &uv = input.textureTransforms[patch];
            auto &bounds = mesh->bounds[patch];
            const std::size_t start = std::size_t(patchX * patches + patchZ) * layout.patchFloatCount;
            std::size_t write = start;
            const auto append = [&](int x, int z) {
                const auto vertex = index(patchX * resolution + x, patchZ * resolution + z);
                const auto &p = mesh->positions[vertex];
                const auto &n = mesh->normals[vertex];
                const QVector3D position(p.x, p.y, p.z);
                if (write == start) {
                    bounds = {position, position};
                } else {
                    for (int axis = 0; axis < 3; ++axis) {
                        bounds.first[axis] = std::min(bounds.first[axis], position[axis]);
                        bounds.second[axis] = std::max(bounds.second[axis], position[axis]);
                    }
                }
                const std::array<float, 8> values{p.x, p.y, p.z, n.x, n.y, n.z,
                    x * uv[2] + z * uv[3] + uv[0], x * uv[4] + z * uv[5] + uv[1]};
                std::copy(values.begin(), values.end(), mesh->patchVertices.data() + write);
                write += values.size();
            };
            for (int z = 0; z < resolution; ++z) {
                for (int x = 0; x < resolution; ++x) {
                    const auto open = [&](int dx, int dz) {
                        return input.flags.empty() || !(input.flags[index(
                            patchZ * resolution + z + dz, patchX * resolution + x + dx)] & 4);
                    };
                    const bool a = open(0, 0), b = open(0, 1), c = open(1, 1), d = open(1, 0);
                    if ((x + z) % 2 == 0) {
                        if (a && b && c) {
                            append(x,z); append(x,z+1); append(x+1,z+1);
                        }
                        if (a && c && d) {
                            append(x,z); append(x+1,z+1); append(x+1,z);
                        }
                    } else {
                        if (b && c && d) {
                            append(x,z+1); append(x+1,z+1); append(x+1,z);
                        }
                        if (a && b && d) {
                            append(x,z); append(x,z+1); append(x+1,z);
                        }
                    }
                }
            }
        }
    }
    return mesh;
}

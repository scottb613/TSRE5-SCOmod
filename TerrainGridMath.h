// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef TERRAINGRIDMATH_H
#define TERRAINGRIDMATH_H

#include <cstddef>
#include <cmath>
#include <limits>

namespace TerrainGridMath {

// MSTS/TSRE terrain descriptors use a fixed 16x16 patch-record grid. The
// elevation resolution within those patches may vary (16 or 32 samples here).
inline constexpr int SupportedPatchDimension = 16;
inline constexpr int SupportedPatchRecordCount =
        SupportedPatchDimension * SupportedPatchDimension;

// Detailed-terrain patch matrices consume raw patch-local sample coordinates.
// Keep the established R=16 values bit-for-bit while deriving equivalent
// values for other sample counts per patch.
inline constexpr float TextureInset = 0.001f;
inline constexpr float LegacyExactTextureScale = 0.0625f;
inline constexpr float LegacyInsetTextureScale = 0.062375f;

inline int patchSampleCount(const int samples, const int patches) {
    if(samples <= 0 || patches <= 0 || samples % patches != 0)
        return 0;
    return samples / patches;
}

inline float exactPatchTextureScale(const int samplesPerPatch) {
    if(samplesPerPatch <= 0)
        return 0.0f;
    if(samplesPerPatch == SupportedPatchDimension)
        return LegacyExactTextureScale;
    return 1.0f / static_cast<float>(samplesPerPatch);
}

inline float insetPatchTextureScale(const int samplesPerPatch) {
    if(samplesPerPatch <= 0)
        return 0.0f;
    if(samplesPerPatch == SupportedPatchDimension)
        return LegacyInsetTextureScale;
    return (1.0f - 2.0f * TextureInset)
            / static_cast<float>(samplesPerPatch);
}

inline float wholeTerrainTextureScale(const int samples) {
    return samples > 0 ? 1.0f / static_cast<float>(samples) : 0.0f;
}

inline float patchTextureCenter(const int samplesPerPatch,
                                const float start,
                                const float scale) {
    return start + 0.5f * static_cast<float>(samplesPerPatch) * scale;
}

inline float textureAxisScale(const float firstComponent,
                              const float secondComponent,
                              const float baseScale) {
    if(baseScale <= 0.0f)
        return 0.0f;
    return std::hypot(firstComponent, secondComponent) / baseScale;
}

inline bool textureValueNear(const float value, const float expected,
                             const float tolerance = 0.000001f) {
    return std::fabs(value - expected) <= tolerance;
}

inline bool isAxisAlignedTextureScale(const float *record,
                                      const float expectedScale) {
    if(record == NULL || expectedScale <= 0.0f)
        return false;
    const float w = std::fabs(record[9]);
    const float b = std::fabs(record[10]);
    const float c = std::fabs(record[11]);
    const float h = std::fabs(record[12]);
    const bool ordinary = textureValueNear(w, expectedScale)
            && textureValueNear(b, 0.0f)
            && textureValueNear(c, 0.0f)
            && textureValueNear(h, expectedScale);
    const bool quarterTurn = textureValueNear(w, 0.0f)
            && textureValueNear(b, expectedScale)
            && textureValueNear(c, expectedScale)
            && textureValueNear(h, 0.0f);
    return ordinary || quarterTurn;
}

inline bool usesLegacyFixed16TextureDomain(const int samples,
                                           const int patches,
                                           const float *patchRecords) {
    const int samplesPerPatch = patchSampleCount(samples, patches);
    if(samplesPerPatch <= 0
            || samplesPerPatch == SupportedPatchDimension
            || patchRecords == NULL)
        return false;

    const int patchCount = patches * patches;
    const float nativeExact = exactPatchTextureScale(samplesPerPatch);
    const float nativeInset = insetPatchTextureScale(samplesPerPatch);
    const float legacyMap = 1.0f / static_cast<float>(patchCount);
    const float nativeMap = wholeTerrainTextureScale(samples);
    int legacyVotes = 0;
    int nativeVotes = 0;
    int legacyMapVotes = 0;
    int nativeMapVotes = 0;

    for(int patch = 0; patch < patchCount; ++patch){
        const float *record = patchRecords + patch * 13;
        if(isAxisAlignedTextureScale(record, LegacyExactTextureScale)
                || isAxisAlignedTextureScale(record,
                        LegacyInsetTextureScale))
            ++legacyVotes;
        if(isAxisAlignedTextureScale(record, nativeExact)
                || isAxisAlignedTextureScale(record, nativeInset))
            ++nativeVotes;
        if(isAxisAlignedTextureScale(record, legacyMap))
            ++legacyMapVotes;
        if(isAxisAlignedTextureScale(record, nativeMap))
            ++nativeMapVotes;
    }

    // A whole-tile generated map has a unique fixed-16 signature. For ordinary
    // patch textures require a majority so intentional repeats on a few native
    // patches are never mistaken for a legacy tile-wide coordinate domain.
    if(legacyMapVotes == patchCount && nativeMapVotes == 0)
        return true;
    return legacyVotes >= (patchCount + 1) / 2 && nativeVotes == 0;
}

inline void convertLegacyFixed16TextureDomain(const int samples,
                                              const int patches,
                                              float *patchRecords) {
    const int samplesPerPatch = patchSampleCount(samples, patches);
    if(samplesPerPatch <= 0 || patchRecords == NULL)
        return;
    const float factor = static_cast<float>(SupportedPatchDimension)
            / static_cast<float>(samplesPerPatch);
    const int patchCount = patches * patches;
    for(int patch = 0; patch < patchCount; ++patch){
        float *record = patchRecords + patch * 13;
        record[9] *= factor;
        record[10] *= factor;
        record[11] *= factor;
        record[12] *= factor;
    }
}

enum class RenderLayoutError {
    None,
    NonPositiveSamples,
    NonPositivePatches,
    UnevenPatchDivision,
    NonPositivePatchResolution,
    SizeOverflow,
    OpenGLBufferTooLarge
};

struct RenderLayout {
    int samples = 0;
    int patches = 0;
    int patchResolution = 0;
    std::size_t patchCount = 0;
    std::size_t patchFloatCount = 0;
    std::size_t patchByteCount = 0;
    std::size_t totalFloatCount = 0;
    std::size_t totalByteCount = 0;
};

inline bool checkedMultiply(const std::size_t left, const std::size_t right,
                            std::size_t &result) {
    if(left != 0 && right > std::numeric_limits<std::size_t>::max() / left)
        return false;
    result = left * right;
    return true;
}

inline bool calculateSamplePayloadSize(const int samples,
                                       const std::size_t bytesPerSample,
                                       std::size_t &byteCount) {
    byteCount = 0;
    if(samples <= 0 || bytesPerSample == 0)
        return false;

    std::size_t sampleCount = 0;
    if(!checkedMultiply(static_cast<std::size_t>(samples),
                        static_cast<std::size_t>(samples), sampleCount)
            || !checkedMultiply(sampleCount, bytesPerSample, byteCount))
        return false;

    return byteCount <= static_cast<std::size_t>(
            std::numeric_limits<int>::max());
}

inline bool calculateRenderLayout(const int samples, const int patches,
                                  RenderLayout &layout,
                                  RenderLayoutError &error) {
    layout = RenderLayout{};
    error = RenderLayoutError::None;

    if(samples <= 0){
        error = RenderLayoutError::NonPositiveSamples;
        return false;
    }
    if(patches <= 0){
        error = RenderLayoutError::NonPositivePatches;
        return false;
    }
    if(samples % patches != 0){
        error = RenderLayoutError::UnevenPatchDivision;
        return false;
    }

    const int patchResolution = samples / patches;
    if(patchResolution <= 0){
        error = RenderLayoutError::NonPositivePatchResolution;
        return false;
    }

    constexpr std::size_t trianglesVertexCountPerCell = 6;
    constexpr std::size_t floatsPerVertex = 8;
    std::size_t patchCellCount = 0;
    std::size_t patchVertexCount = 0;
    std::size_t patchFloatCount = 0;
    std::size_t patchByteCount = 0;
    std::size_t patchCount = 0;
    std::size_t totalFloatCount = 0;
    std::size_t totalByteCount = 0;
    const std::size_t resolution = static_cast<std::size_t>(patchResolution);
    const std::size_t patchDimension = static_cast<std::size_t>(patches);

    if(!checkedMultiply(resolution, resolution, patchCellCount)
            || !checkedMultiply(patchCellCount, trianglesVertexCountPerCell,
                                patchVertexCount)
            || !checkedMultiply(patchVertexCount, floatsPerVertex,
                                patchFloatCount)
            || !checkedMultiply(patchFloatCount, sizeof(float), patchByteCount)
            || !checkedMultiply(patchDimension, patchDimension, patchCount)
            || !checkedMultiply(patchCount, patchFloatCount, totalFloatCount)
            || !checkedMultiply(totalFloatCount, sizeof(float), totalByteCount)){
        error = RenderLayoutError::SizeOverflow;
        return false;
    }

    // QOpenGLBuffer::allocate() and write() accept signed-int byte counts and
    // offsets, even on 64-bit builds.
    const std::size_t maxOpenGLBufferBytes =
            static_cast<std::size_t>(std::numeric_limits<int>::max());
    if(patchByteCount > maxOpenGLBufferBytes
            || totalByteCount > maxOpenGLBufferBytes){
        error = RenderLayoutError::OpenGLBufferTooLarge;
        return false;
    }

    layout.samples = samples;
    layout.patches = patches;
    layout.patchResolution = patchResolution;
    layout.patchCount = patchCount;
    layout.patchFloatCount = patchFloatCount;
    layout.patchByteCount = patchByteCount;
    layout.totalFloatCount = totalFloatCount;
    layout.totalByteCount = totalByteCount;
    return true;
}

inline const char *renderLayoutErrorText(const RenderLayoutError error) {
    switch(error){
    case RenderLayoutError::None:
        return "none";
    case RenderLayoutError::NonPositiveSamples:
        return "terrain sample count must be positive";
    case RenderLayoutError::NonPositivePatches:
        return "terrain patch count must be positive";
    case RenderLayoutError::UnevenPatchDivision:
        return "terrain samples must divide evenly into patches";
    case RenderLayoutError::NonPositivePatchResolution:
        return "terrain patch resolution must be positive";
    case RenderLayoutError::SizeOverflow:
        return "terrain render-buffer size overflow";
    case RenderLayoutError::OpenGLBufferTooLarge:
        return "terrain render buffer exceeds QOpenGLBuffer limits";
    }
    return "unknown terrain render-layout error";
}

} // namespace TerrainGridMath

#endif // TERRAINGRIDMATH_H

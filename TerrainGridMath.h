#ifndef TERRAINGRIDMATH_H
#define TERRAINGRIDMATH_H

#include <cstddef>
#include <limits>

namespace TerrainGridMath {

// MSTS/TSRE terrain descriptors use a fixed 16x16 patch-record grid. The
// elevation resolution within those patches may vary (16 or 32 samples here).
inline constexpr int SupportedPatchDimension = 16;
inline constexpr int SupportedPatchRecordCount =
        SupportedPatchDimension * SupportedPatchDimension;

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

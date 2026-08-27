#include "TerrainGridMath.h"

#include <iostream>

namespace {
bool expectLayout(const int samples, const int patches,
                  const int expectedPatchResolution,
                  const std::size_t expectedPatchFloatCount,
                  const std::size_t expectedTotalByteCount) {
    TerrainGridMath::RenderLayout layout;
    TerrainGridMath::RenderLayoutError error;
    if(!TerrainGridMath::calculateRenderLayout(samples, patches, layout, error)){
        std::cerr << "Unexpected layout rejection: "
                  << TerrainGridMath::renderLayoutErrorText(error) << '\n';
        return false;
    }
    if(layout.patchResolution != expectedPatchResolution
            || layout.patchFloatCount != expectedPatchFloatCount
            || layout.totalByteCount != expectedTotalByteCount){
        std::cerr << "Unexpected layout for " << samples << " samples and "
                  << patches << " patches\n";
        return false;
    }
    return true;
}

bool expectRejected(const int samples, const int patches,
                    const TerrainGridMath::RenderLayoutError expectedError) {
    TerrainGridMath::RenderLayout layout;
    TerrainGridMath::RenderLayoutError error;
    if(TerrainGridMath::calculateRenderLayout(samples, patches, layout, error)){
        std::cerr << "Unexpectedly accepted invalid terrain layout\n";
        return false;
    }
    if(error != expectedError){
        std::cerr << "Unexpected rejection reason: "
                  << TerrainGridMath::renderLayoutErrorText(error) << '\n';
        return false;
    }
    return true;
}
}

int main() {
    bool ok = true;
    ok &= TerrainGridMath::SupportedPatchDimension == 16;
    ok &= TerrainGridMath::SupportedPatchRecordCount == 256;
    ok &= expectLayout(256, 16, 16, 12288, 12582912);
    ok &= expectLayout(512, 16, 32, 49152, 50331648);
    ok &= expectRejected(0, 16,
            TerrainGridMath::RenderLayoutError::NonPositiveSamples);
    ok &= expectRejected(512, 0,
            TerrainGridMath::RenderLayoutError::NonPositivePatches);
    ok &= expectRejected(511, 16,
            TerrainGridMath::RenderLayoutError::UnevenPatchDivision);
    ok &= expectRejected(65536, 1,
            TerrainGridMath::RenderLayoutError::OpenGLBufferTooLarge);

    std::size_t byteCount = 0;
    ok &= TerrainGridMath::calculateSamplePayloadSize(256, 2, byteCount)
            && byteCount == 131072;
    ok &= TerrainGridMath::calculateSamplePayloadSize(512, 2, byteCount)
            && byteCount == 524288;
    ok &= TerrainGridMath::calculateSamplePayloadSize(256, 1, byteCount)
            && byteCount == 65536;
    ok &= TerrainGridMath::calculateSamplePayloadSize(512, 1, byteCount)
            && byteCount == 262144;
    ok &= !TerrainGridMath::calculateSamplePayloadSize(0, 2, byteCount);
    return ok ? 0 : 1;
}

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "TerrainGridMath.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

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

bool expectNear(const float value, const float expected,
                const char *description) {
    if(std::fabs(value - expected) <= 0.000001f)
        return true;
    std::cerr << description << ": expected " << expected
              << ", got " << value << '\n';
    return false;
}

bool expectTextureDomain(const int samples, const int patches,
                         const int expectedSamplesPerPatch) {
    const int samplesPerPatch =
            TerrainGridMath::patchSampleCount(samples, patches);
    if(samplesPerPatch != expectedSamplesPerPatch)
        return false;
    const float exact =
            TerrainGridMath::exactPatchTextureScale(samplesPerPatch);
    const float inset =
            TerrainGridMath::insetPatchTextureScale(samplesPerPatch);
    bool ok = true;
    ok &= expectNear(samplesPerPatch * exact, 1.0f,
                     "exact patch texture span");
    ok &= expectNear(TerrainGridMath::TextureInset
                     + samplesPerPatch * inset,
                     1.0f - TerrainGridMath::TextureInset,
                     "inset patch texture end");
    ok &= expectNear(TerrainGridMath::wholeTerrainTextureScale(samples),
                     1.0f / samples, "whole-terrain texture step");
    return ok;
}

void setAxisAlignedScale(std::vector<float> &records, const int patch,
                         const float scale) {
    records[patch * 13 + 9] = scale;
    records[patch * 13 + 10] = 0.0f;
    records[patch * 13 + 11] = 0.0f;
    records[patch * 13 + 12] = scale;
}

bool expectLegacyTextureConversion() {
    constexpr int samples = 512;
    constexpr int patches = 16;
    constexpr int patchCount = patches * patches;
    std::vector<float> records(patchCount * 13, 0.0f);
    for(int patch = 0; patch < patchCount; ++patch){
        records[patch * 13 + 7] = TerrainGridMath::TextureInset;
        records[patch * 13 + 8] = TerrainGridMath::TextureInset;
        setAxisAlignedScale(records, patch,
                TerrainGridMath::LegacyExactTextureScale);
    }
    if(!TerrainGridMath::usesLegacyFixed16TextureDomain(
            samples, patches, records.data()))
        return false;

    TerrainGridMath::convertLegacyFixed16TextureDomain(
            samples, patches, records.data());
    bool ok = true;
    ok &= !TerrainGridMath::usesLegacyFixed16TextureDomain(
            samples, patches, records.data());
    ok &= expectNear(records[9], 1.0f / 32.0f,
                     "converted 4 m patch scale");
    ok &= expectNear(records[7], TerrainGridMath::TextureInset,
                     "converted patch offset");

    std::fill(records.begin(), records.end(), 0.0f);
    const float legacyMapScale = 1.0f / patchCount;
    for(int patch = 0; patch < patchCount; ++patch)
        setAxisAlignedScale(records, patch, legacyMapScale);
    ok &= TerrainGridMath::usesLegacyFixed16TextureDomain(
            samples, patches, records.data());
    TerrainGridMath::convertLegacyFixed16TextureDomain(
            samples, patches, records.data());
    ok &= expectNear(records[9], 1.0f / samples,
                     "converted whole-terrain map step");

    std::fill(records.begin(), records.end(), 0.0f);
    for(int patch = 0; patch < patchCount; ++patch)
        setAxisAlignedScale(records, patch, 1.0f / 32.0f);
    for(int patch = 0; patch < 10; ++patch)
        setAxisAlignedScale(records, patch,
                TerrainGridMath::LegacyExactTextureScale);
    ok &= !TerrainGridMath::usesLegacyFixed16TextureDomain(
            samples, patches, records.data());
    ok &= !TerrainGridMath::usesLegacyFixed16TextureDomain(
            256, patches, records.data());
    return ok;
}

bool expectRotationIndependentScale() {
    const float base = TerrainGridMath::insetPatchTextureScale(32);
    const float diagonal = base * std::sqrt(0.5f);
    bool ok = true;
    ok &= expectNear(TerrainGridMath::textureAxisScale(base, 0.0f, base),
                     1.0f, "zero-degree texture scale");
    ok &= expectNear(TerrainGridMath::textureAxisScale(
                         diagonal, -diagonal, base),
                     1.0f, "45-degree texture scale");
    ok &= expectNear(TerrainGridMath::textureAxisScale(0.0f, -base, base),
                     1.0f, "90-degree texture scale");
    ok &= TerrainGridMath::textureAxisScale(base, 0.0f, 0.0f) == 0.0f;
    return ok;
}
}

int main() {
    bool ok = true;
    ok &= TerrainGridMath::SupportedPatchDimension == 16;
    ok &= TerrainGridMath::SupportedPatchRecordCount == 256;
    ok &= expectLayout(256, 16, 16, 12288, 12582912);
    ok &= expectLayout(512, 16, 32, 49152, 50331648);
    ok &= expectTextureDomain(128, 16, 8);
    ok &= expectTextureDomain(256, 16, 16);
    ok &= expectTextureDomain(512, 16, 32);
    ok &= expectLegacyTextureConversion();
    ok &= expectRotationIndependentScale();

    const float legacyExact = TerrainGridMath::LegacyExactTextureScale;
    const float legacyInset = TerrainGridMath::LegacyInsetTextureScale;
    const float calculatedExact =
            TerrainGridMath::exactPatchTextureScale(16);
    const float calculatedInset =
            TerrainGridMath::insetPatchTextureScale(16);
    ok &= std::memcmp(&legacyExact, &calculatedExact, sizeof(float)) == 0;
    ok &= std::memcmp(&legacyInset, &calculatedInset, sizeof(float)) == 0;
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

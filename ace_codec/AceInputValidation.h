// TSRE GenX ACE input preflight. GNU GPL v3 or later. See ../LICENSE.md.
#pragma once
#include "AceDocument.h"

namespace AceInputValidation {
enum class Profile { RoutePaint, TerrainConversion };

inline AceReadOptions readOptions(Profile profile) {
    AceReadOptions options;
    options.maxDimension = 8192;
    options.maxPixels = 64LL * 1024 * 1024;
    if (profile == Profile::RoutePaint) {
        // Preserve both 128 MiB encoded/inflated and conservative RGBA budgets.
        // The parser checks maxPixels before copying image payloads, not after
        // allocating an editable RGB/RGBA image.
        options.maxBytes = 128LL * 1024 * 1024;
        options.maxPixels = (64LL * 1024 * 1024) / 4;
    }
    return options;
}

inline bool preflight(const QString &path, Profile profile, QString &error) {
    AceDocument document;
    if (!AceDocument::read(path, document, error, readOptions(profile)))
        return false;
    // Retain the editor's existing minimum dimensions. The parser itself also
    // supports tiny images for independent content/thumbnail consumers.
    const auto &base = document.levels[0];
    if (base.width <= 1 || base.height <= 1) {
        error = "ACE dimensions are outside the supported 2..8192 range.";
        return false;
    }
    // No editable pixel allocation or write occurs here. The owning document
    // is released before the existing AceLib decode path is invoked.
    return true;
}
} // namespace AceInputValidation

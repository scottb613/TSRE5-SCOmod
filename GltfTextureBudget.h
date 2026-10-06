// TSRE GenX modifications. Licensed under GNU GPL v3 or later. See LICENSE.md.
#ifndef TSRE_GLTF_TEXTURE_BUDGET_H
#define TSRE_GLTF_TEXTURE_BUDGET_H

#include <QSize>
#include <algorithm>
#include <cstddef>

// Call only after the loader has validated the image dimensions. Include the
// complete RGBA8 mip chain: a 1-by-N texture approaches twice its base size.
inline size_t gltfTextureBytes(QSize size, bool mipmaps) {
    size_t bytes = 0;
    do {
        bytes += size_t(size.width()) * size_t(size.height()) * 4;
        if(!mipmaps || (size.width() == 1 && size.height() == 1)) break;
        size = QSize(std::max(1, size.width() / 2), std::max(1, size.height() / 2));
    } while(true);
    return bytes;
}
#endif

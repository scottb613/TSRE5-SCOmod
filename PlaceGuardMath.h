// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef PLACEGUARDMATH_H
#define PLACEGUARDMATH_H

#include <cmath>

namespace PlaceGuardMath {

inline bool acceptsSceneryHeight(bool stickPointerToTerrain,
                                 bool terrainLoaded,
                                 float heightAboveTerrain) {
    // Stick to All deliberately targets rendered world geometry, including
    // elevated track. Its height must not be mistaken for an off-terrain cast.
    if(!stickPointerToTerrain)
        return true;

    return terrainLoaded && std::isfinite(heightAboveTerrain)
        && heightAboveTerrain >= -1.0f && heightAboveTerrain <= 1.0f;
}

} // namespace PlaceGuardMath

#endif // PLACEGUARDMATH_H

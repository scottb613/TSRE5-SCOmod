// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "../TSREvcWIP/PlaceGuardMath.h"

#include <cassert>
#include <iostream>
#include <limits>

int main() {
    // Terrain-snapped scenery retains the original one-metre guardrail.
    assert(PlaceGuardMath::acceptsSceneryHeight(true, true, 1.0f));
    assert(PlaceGuardMath::acceptsSceneryHeight(true, true, -1.0f));
    assert(!PlaceGuardMath::acceptsSceneryHeight(true, true, 1.01f));
    assert(!PlaceGuardMath::acceptsSceneryHeight(true, true, -1.01f));
    assert(!PlaceGuardMath::acceptsSceneryHeight(true, false, 0.0f));
    assert(!PlaceGuardMath::acceptsSceneryHeight(
        true, true, std::numeric_limits<float>::quiet_NaN()));

    // All-surface placement intentionally accepts clicked elevated geometry.
    assert(PlaceGuardMath::acceptsSceneryHeight(false, true, 25.0f));
    assert(PlaceGuardMath::acceptsSceneryHeight(false, true, -25.0f));
    assert(PlaceGuardMath::acceptsSceneryHeight(false, false, 25.0f));

    std::cout << "PlaceGuardMathProbe passed\n";
    return 0;
}

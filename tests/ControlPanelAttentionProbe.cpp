// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "../TSREvcWIP/ControlPanelAttention.h"

#include <cassert>
#include <iostream>

int main() {
    assert(!ControlPanelAttention::saveTimeRequiresFlash(0));
    assert(!ControlPanelAttention::saveTimeRequiresFlash(59));
    assert(!ControlPanelAttention::saveTimeRequiresFlash(60));
    assert(ControlPanelAttention::saveTimeRequiresFlash(61));
    assert(ControlPanelAttention::saveTimeRequiresFlash(120));

    std::cout << "ControlPanelAttentionProbe passed\n";
    return 0;
}

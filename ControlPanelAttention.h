// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef CONTROLPANELATTENTION_H
#define CONTROLPANELATTENTION_H

namespace ControlPanelAttention {

inline bool saveTimeRequiresFlash(int minutesSinceSave) {
    return minutesSinceSave > 60;
}

} // namespace ControlPanelAttention

#endif // CONTROLPANELATTENTION_H

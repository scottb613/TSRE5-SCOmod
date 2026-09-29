// Auto Place click modifiers. GPL v3 or later.
#ifndef AUTO_PLACE_INPUT_H
#define AUTO_PLACE_INPUT_H
#include <QMouseEvent>
namespace AutoPlaceInput {
inline int mode(const QMouseEvent &event) {
    // The key press may have gone to a panel field before this click restores
    // viewport focus. Use this event, never the viewport's cached key state.
    if(event.modifiers().testFlag(Qt::ShiftModifier)) return 2;
    if(event.modifiers().testFlag(Qt::ControlModifier)) return 1;
    return 0;
}
}
#endif

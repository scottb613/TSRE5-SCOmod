// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef POLYVEGOBJECT_H
#define POLYVEGOBJECT_H

#include <QString>

namespace PolyVegObject {
QString labelForShape(const QString &fileName);
bool isRawShape(const QString &fileName);
bool isBakeShape(const QString &fileName);
}

#endif

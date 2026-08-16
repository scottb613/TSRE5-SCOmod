#ifndef POLYVEGOBJECT_H
#define POLYVEGOBJECT_H

#include <QString>

namespace PolyVegObject {
QString labelForShape(const QString &fileName);
bool isRawShape(const QString &fileName);
bool isBakeShape(const QString &fileName);
}

#endif

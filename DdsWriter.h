// TSRE GenX - Licensed under GNU GPL v3 or later. See LICENSE.md.
// Native RGB24 and explicit opaque DXT1 TERRTEX output. GNU GPL v3 or later.
#pragma once
#include <QImage>
#include <QByteArray>

namespace DdsWriter {
bool serialize24(const QImage &base, bool mipmaps, QByteArray &data, QString *error = nullptr);
// Explicit production conversion: discard alpha and encode opaque BC1 blocks.
bool serializeDxt1(const QImage &base, bool mipmaps, QByteArray &data, QString *error = nullptr);
bool isOpaqueDxt1(const QByteArray &data);
}

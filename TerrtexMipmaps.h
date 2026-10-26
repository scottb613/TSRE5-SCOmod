// High quality uncompressed terrain ACE mip chains. GNU GPL v3 or later.
#pragma once
#include <QImage>
#include <QByteArray>
#include <QVector>

namespace TerrtexMipmaps {
bool generate(const QImage &base, QVector<QImage> &levels, QString *error = nullptr);
bool generateDds(const QImage &base, QVector<QImage> &levels, QString *error = nullptr);
bool serializeAce(const QImage &base, bool alpha, QByteArray &data, QString *error = nullptr);
}

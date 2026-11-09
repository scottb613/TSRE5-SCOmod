// TSRE GenX TERRTEX processing. GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace TerrtexProcessing {
enum class Mipmaps { Keep = 0, Remove = 1, AddMissing = 2 };
enum class Output { Keep = 0, Ace = 1, Dds = 2 };
struct Options { bool compress = true; Mipmaps mipmaps = Mipmaps::Keep; Output output = Output::Keep; };
struct Change {
    QByteArray bytes;
    QString suffix;
    bool changed = false, compressed = false, added = false, removed = false, reformatted = false;
};
QJsonObject toJson(const Options &options);
bool fromJson(const QJsonObject &json, Options &options);
QString destination(const QString &path, Output output);
bool plan(const QString &path, const Options &options, QString &output, QString &error);
bool process(const QString &path, const QByteArray &input, const Options &options,
             Change &change, QString &error);
}

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

/*
 * Native DDS decoder used by the Qt6 port.
 *
 * Qt does not build its DDS image plugin by default.  TSRE therefore decodes
 * the legacy rolling-stock formats itself instead of passing DDS files to
 * QImage.
 */

#ifndef DDSDECODER_H
#define DDSDECODER_H

#include <QByteArray>
#include <QString>
#include <limits>

struct DdsImage {
    int width = 0;
    int height = 0;
    QByteArray rgba;
    bool opaqueRgb24 = false;
    bool opaqueDxt1 = false;
};

struct DdsBc1Level {
    int width = 0;
    int height = 0;
    QByteArray blocks;
};

class DdsDecoder {
public:
    // Reads only the requested level. Transparent DDS blocks stay on the
    // pixel-decoding path so the renderer can apply its existing alpha rules.
    static bool readOpaqueBc1(const QString &path, int maximumSide, DdsBc1Level &level);
    static qint64 minimumUploadBytes(const QString &path, bool nativeDxt1);
    static bool decodeStoredPreview(const QString &path, int maximumSide, DdsImage &image);
    static bool decodeFile(
        const QString &path, DdsImage &image, QString *error = nullptr);
    static bool decode(
        const QByteArray &fileData, DdsImage &image, QString *error = nullptr,
        quint64 maximumDecodedBytes = std::numeric_limits<quint64>::max());
};

#endif

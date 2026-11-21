/*
 * This file is part of TSRE5.
 *
 * Licensed under GNU General Public License 3.0 or later.
 */

#ifndef DDSLIB_H
#define DDSLIB_H

#include <QString>
#include <QByteArray>
#include <memory>

class Texture;

struct DdsTextureData {
    std::unique_ptr<unsigned char[]> pixels;
    QByteArray bc1Blocks;
    QString error;
    int width = 0;
    int height = 0;
    int components = 0;
    qint64 minimumFullUploadBytes = 0;
    bool missing = false;
};

class DdsLib {
public:
    Texture *texture = nullptr;
    void run();

    // Main-thread entry points. Workers only receive a path and return
    // owned CPU data, with no access to Texture, TexLib, or OpenGL.
    static bool takeStreamed(const QString &path, DdsTextureData &result, int previewSide = 0,
                             bool nativeDxt1 = false, bool allowOddAce = false,
                             bool prefetch = false);
    static void discardStreamed(const QString &path);
    static void publish(Texture &target, DdsTextureData result);
};

#endif

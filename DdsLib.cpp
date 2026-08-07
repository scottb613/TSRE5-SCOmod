/*
 * This file is part of TSRE5.
 *
 * Licensed under GNU General Public License 3.0 or later.
 */

#include "DdsLib.h"

#include "DdsDecoder.h"
#include "Texture.h"

#include <QDebug>
#include <QFileInfo>
#include <QOpenGLFunctions>

#include <cstring>

bool DdsLib::IsThread = true;

DdsLib::DdsLib() {
}

void DdsLib::run() {
    if(texture == nullptr)
        return;

    DdsImage image;
    QString error;
    if(!DdsDecoder::decodeFile(texture->pathid, image, &error)) {
        texture->loaded = false;
        texture->missing = !QFileInfo::exists(texture->pathid);
        texture->error = !texture->missing;
        qWarning() << "DDS decode failed:" << texture->pathid << "-" << error;
        return;
    }

    texture->width = image.width;
    texture->height = image.height;
    texture->bytesPerPixel = 4;
    texture->type = GL_RGBA;
    texture->imageData = new unsigned char[image.rgba.size()];
    std::memcpy(
        texture->imageData, image.rgba.constData(), image.rgba.size());
    texture->loaded = true;
    texture->editable = true;
}

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

// Texture is a legacy shared object whose decode fields are consumed and
// released by Texture::GLTextures() on the OpenGL thread.  Publishing those
// raw fields from a worker can race the upload/free path and corrupt the heap.
// Keep native DDS decode synchronous until Texture has an owned, queued
// decode-result handoff.
bool DdsLib::IsThread = false;

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
    texture->saveDds24 = true;
    texture->height = image.height;
    texture->sourceWidth = image.width;
    texture->sourceHeight = image.height;
    const bool opaque=image.opaqueRgb24 || image.opaqueDxt1;
    texture->bytesPerPixel = opaque ? 3 : 4;
    texture->bpp = texture->bytesPerPixel * 8;
    texture->imageSize = image.width * image.height * texture->bytesPerPixel;
    texture->type = opaque ? GL_RGB : GL_RGBA;
    texture->imageData = new unsigned char[texture->imageSize];
    if(opaque) {
        // Keep opaque RGB24 and BC1 data in RGB storage. Inventing an alpha
        // channel lets the legacy AA border-removal option make it transparent
        // and would prevent a subsequent RGB24 paint/save cycle.
        for(int i=0;i<image.width*image.height;++i)
            std::memcpy(texture->imageData+i*3,image.rgba.constData()+i*4,3);
    } else std::memcpy(texture->imageData,image.rgba.constData(),image.rgba.size());
    texture->loaded = true;
    texture->editable = true;
}

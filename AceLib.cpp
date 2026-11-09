/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "AceLib.h"
#include "ace_codec/AceDocument.h"
#include "Texture.h"
#include "TerrtexMipmaps.h"
#include "SaveProgress.h"
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <QString>
#include <QBuffer>
#include <QDataStream>
#include <QSaveFile>
#include <QFileInfo>
#include <limits>
#include <memory>
#include <cstring>
#include "Game.h"

bool AceLib::IsThread = true;

AceLib::AceLib(){
    
}

AceLib::~AceLib(){
    
}

// CPU-only adapter: preserve Texture's existing renderer/edit/save contract.
void AceLib::run() {
    if (!texture)
        return;
    texture->loaded = false;
    auto reject = [&](const QString &error) {
        texture->missing = !QFileInfo::exists(texture->pathid);
        texture->error = !texture->missing;
        texture->editable = false;
        qWarning() << "ACE decode failed:" << texture->pathid << "-" << error;
    };

    AceReadOptions options;
    options.maxDimension = 8192;
    options.maxPixels = 64LL * 1024 * 1024;
    options.legacyDxt1Colors = true;
    options.retainOriginal = false;
    AceDocument document;
    QString error;
    if (!AceDocument::read(texture->pathid, document, error, options)) {
        reject(error);
        return;
    }
    const int sourceWidth = document.levels[0].width;
    const int sourceHeight = document.levels[0].height;
    // Keep the editor's existing dimension/Unsafe Mode policy. The standalone
    // codec can decode smaller/odd inputs, but this renderer contract is unchanged.
    if (sourceWidth <= 1 || sourceHeight <= 1
            || ((!Game::UnsafeMode) && (sourceWidth % 2 || sourceHeight % 2))) {
        reject(QString("Unsupported editor texture dimensions: %1 x %2")
                   .arg(sourceWidth).arg(sourceHeight));
        return;
    }
    if (sourceWidth % 2 || sourceHeight % 2)
        qWarning() << "Check Texture Dimensions (may cause crash):" << texture->pathid
                   << "=" << sourceWidth << "x" << sourceHeight;
    const int components = document.hasAlpha() ? 4 : 3;
    const qint64 decodedBytes = qint64(sourceWidth) * sourceHeight * components;
    constexpr qint64 maximumDecodedBytes = 256LL * 1024 * 1024;
    if (decodedBytes <= 0 || decodedBytes > maximumDecodedBytes
            || decodedBytes > std::numeric_limits<int>::max()) {
        reject("Decoded image size exceeds the supported limit.");
        return;
    }
    std::unique_ptr<unsigned char[]> pixels(new unsigned char[static_cast<int>(decodedBytes)]);
    int decodedComponents = 0;
    if (!document.decodeInto(0, pixels.get(), static_cast<qsizetype>(decodedBytes),
                             decodedComponents, error) || decodedComponents != components) {
        reject(error.isEmpty() ? QString("ACE component count changed during decode.") : error);
        return;
    }

    int width = sourceWidth, height = sourceHeight;
    // Preserve legacy nearest-neighbour viewing quality. Editing always gets
    // full source pixels, regardless of rendering quality, before paint/save.
    if (Game::textureQuality > 1 && !Game::writeEnabled
            && width >= Game::textureQuality && height >= Game::textureQuality) {
        width /= Game::textureQuality;
        height /= Game::textureQuality;
        std::unique_ptr<unsigned char[]> reduced(new unsigned char[width * height * components]);
        const float scaleWidth = float(sourceWidth) / width;
        const float scaleHeight = float(sourceHeight) / height;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                const int sy = int(scaleHeight * y), sx = int(scaleWidth * x);
                std::memcpy(reduced.get() + (y * width + x) * components,
                            pixels.get() + (sy * sourceWidth + sx) * components, components);
            }
        pixels = std::move(reduced);
    }
    int channelType = components == 3 ? 0 : 2;
    for (const auto &channel : document.metadata.channels)
        if (channel.id == 6)
            channelType = 1;
    if ((document.options() & 16)
            && (document.surface() == 17 || document.surface() >= 19))
        channelType = 1;

    // Publish only a fully decoded result. No OpenGL calls, compressed upload,
    // retained source document or authored-mip staging is introduced here.
    delete[] texture->imageData;
    texture->imageData = pixels.release();
    texture->width = width;
    texture->height = height;
    texture->sourceWidth = sourceWidth;
    texture->sourceHeight = sourceHeight;
    texture->bytesPerPixel = components;
    texture->bpp = components * 8;
    texture->imageSize = width * height * components;
    texture->type = components == 4 ? GL_RGBA : GL_RGB;
    texture->typk = channelType;
    texture->compressed = static_cast<int>(document.surface());
    texture->saveMipmaps = (document.options() & 1) != 0;
    texture->saveDds24 = false;
    texture->missing = false;
    texture->error = false;
    texture->glLoaded = false;
    texture->editable = true;
    texture->loaded = true;
}

bool AceLib::serialize(Texture *t, QByteArray &data, QString *error){
    data.clear();
    if(error) error->clear();
    if(t == NULL || t->imageData == NULL || t->width <= 0 || t->height <= 0
            || t->width > 8192 || t->height > 8192
            || (t->bytesPerPixel != 3 && t->bytesPerPixel != 4)){
        if(error)
            *error = "Texture has no valid editable image data.";
        return false;
    }
    const qint64 pixels = qint64(t->width) * qint64(t->height);
    if(pixels <= 0 || pixels > std::numeric_limits<int>::max() / t->bytesPerPixel){
        if(error)
            *error = "Texture dimensions exceed the ACE encoder limit.";
        return false;
    }

    if(t->saveMipmaps) {
        const QImage image(t->imageData, t->width, t->height,
            t->width*t->bytesPerPixel, t->bytesPerPixel == 4
                ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
        return TerrtexMipmaps::serializeAce(image, t->bytesPerPixel == 4, data, error);
    }

    QBuffer buffer(&data);
    if(!buffer.open(QIODevice::WriteOnly)){
        if(error)
            *error = buffer.errorString();
        return false;
    }
    QDataStream write(&buffer);
    write.setByteOrder(QDataStream::LittleEndian);
    write.setFloatingPointPrecision(QDataStream::SinglePrecision);
    
    const char header[] = {
        0x53,0x49,0x4D,0x49,0x53,0x41,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40
    };
    //header
    write.writeRawData(header, 16);
    write << (qint32)1;
    //options
    write << (qint32)0;
    //width
    write << (qint32)t->width;
    //height
    write << (qint32)t->height;
    //pixel format
    write << (qint32)14;
    //channels
    write << (qint32)3;
    //0
    write << (qint32)0;
    //empty strings
    for(int i = 0; i < 31; i++)
        write << (qint32)0;
    
    //channels
    write << (qint32)8;
    write << (qint32)0;
    write << (qint32)3;
    write << (qint32)0;
    write << (qint32)8;
    write << (qint32)0;
    write << (qint32)4;
    write << (qint32)0;
    write << (qint32)8;
    write << (qint32)0;
    write << (qint32)5;
    write << (qint32)0;
    //200
    int offset = t->height*4 + 200;
    for(int i = 0; i < t->height; i++){
        write << (qint32)offset + i*t->width*3*4;
    }
    //data
    QByteArray encodedRow(t->width*3, '\0');
    for(int i = 0; i < t->height; i++){
        SaveProgress::set(double(i)/t->height);
        if(i%32 == 0) SaveProgress::report(QString("Encoding TERRTEX %1: row %2 of %3")
            .arg(QFileInfo(t->pathid).fileName()).arg(i+1).arg(t->height));
        const unsigned char *pixels = t->imageData + t->bytesPerPixel*t->width*i;
        char *encoded = encodedRow.data();
        for(int c = 0; c < 3; ++c)
            for(int j = 0; j < t->width; ++j)
                encoded[c*t->width+j] = char(pixels[j*t->bytesPerPixel+c]);
        write.writeRawData(encodedRow.constData(), int(encodedRow.size()));
    }
    
    const bool ok = write.status() == QDataStream::Ok;
    write.setDevice(nullptr);
    buffer.close();
    if(!ok || data.isEmpty()){
        data.clear();
        if(error)
            *error = "ACE serialization failed.";
        return false;
    }
    return true;
}

bool AceLib::save(QString path, Texture* t, QString *error){
    path.replace("//", "/");
    qDebug() << "zapis .ace " << path;
    QByteArray data;
    if(!serialize(t, data, error))
        return false;
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly)){
        if(error)
            *error = file.errorString();
        return false;
    }
    if(file.write(data) != data.size()){
        if(error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    if(!file.commit()){
        if(error)
            *error = file.errorString();
        return false;
    }
    return true;
}

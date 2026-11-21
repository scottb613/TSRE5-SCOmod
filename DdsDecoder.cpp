// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

/*
 * Native DDS decoder used by the Qt6 port.
 *
 * The block layout follows the Microsoft DDS/BC1-BC3 specification.
 */

#include "DdsDecoder.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>

namespace {

constexpr qsizetype DdsFileHeaderSize = 128;
constexpr quint32 DdsHeaderPitch = 0x00000008;
constexpr quint32 DdsPixelFormatFourCc = 0x00000004;
constexpr quint32 DdsPixelFormatRgb = 0x00000040;
constexpr quint32 FourCcDxt1 = 0x31545844;
constexpr quint32 FourCcDxt3 = 0x33545844;
constexpr quint32 FourCcDxt5 = 0x35545844;
constexpr quint32 FourCcDx10 = 0x30315844;
constexpr int MaximumDimension = 32768;
constexpr quint64 MaximumDecodedBytes = 1024ULL * 1024ULL * 1024ULL;

quint32 readU32(const char *data) {
    return qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(data));
}

quint16 readU16(const uchar *data) {
    return qFromLittleEndian<quint16>(data);
}

void setError(QString *error, const QString &message) {
    if(error != nullptr)
        *error = message;
}

uchar expand5(quint16 value) {
    return static_cast<uchar>((value << 3) | (value >> 2));
}

uchar expand6(quint16 value) {
    return static_cast<uchar>((value << 2) | (value >> 4));
}

std::array<uchar, 3> decode565(quint16 value) {
    return {
        expand5((value >> 11) & 0x1f),
        expand6((value >> 5) & 0x3f),
        expand5(value & 0x1f)
    };
}

void decodeColorBlock(
    const uchar *block,
    QByteArray &rgba,
    int blockX,
    int blockY,
    int width,
    int height,
    bool forceFourColor,
    bool preserveAlpha) {
    const quint16 color0 = readU16(block);
    const quint16 color1 = readU16(block + 2);
    std::array<std::array<uchar, 4>, 4> colors{};

    const auto rgb0 = decode565(color0);
    const auto rgb1 = decode565(color1);
    for(int channel = 0; channel < 3; ++channel) {
        colors[0][channel] = rgb0[channel];
        colors[1][channel] = rgb1[channel];
    }
    colors[0][3] = 255;
    colors[1][3] = 255;

    if(color0 > color1 || forceFourColor) {
        for(int channel = 0; channel < 3; ++channel) {
            colors[2][channel] = static_cast<uchar>(
                (2 * colors[0][channel] + colors[1][channel]) / 3);
            colors[3][channel] = static_cast<uchar>(
                (colors[0][channel] + 2 * colors[1][channel]) / 3);
        }
        colors[2][3] = 255;
        colors[3][3] = 255;
    } else {
        for(int channel = 0; channel < 3; ++channel)
            colors[2][channel] = static_cast<uchar>(
                (colors[0][channel] + colors[1][channel]) / 2);
        colors[2][3] = 255;
        colors[3] = {0, 0, 0, 0};
    }

    const quint32 indices = readU32(
        reinterpret_cast<const char *>(block + 4));
    uchar *pixels = reinterpret_cast<uchar *>(rgba.data());
    for(int y = 0; y < 4; ++y) {
        const int pixelY = blockY * 4 + y;
        if(pixelY >= height)
            continue;
        for(int x = 0; x < 4; ++x) {
            const int pixelX = blockX * 4 + x;
            if(pixelX >= width)
                continue;
            const int index = (indices >> (2 * (4 * y + x))) & 0x03;
            uchar *destination =
                pixels + (pixelY * width + pixelX) * 4;
            std::memcpy(destination, colors[index].data(), 3);
            if(!preserveAlpha)
                destination[3] = colors[index][3];
        }
    }
}

void decodeDxt3Alpha(
    const uchar *block,
    QByteArray &rgba,
    int blockX,
    int blockY,
    int width,
    int height) {
    uchar *pixels = reinterpret_cast<uchar *>(rgba.data());
    for(int y = 0; y < 4; ++y) {
        const quint16 alphaRow = readU16(block + y * 2);
        const int pixelY = blockY * 4 + y;
        if(pixelY >= height)
            continue;
        for(int x = 0; x < 4; ++x) {
            const int pixelX = blockX * 4 + x;
            if(pixelX >= width)
                continue;
            pixels[(pixelY * width + pixelX) * 4 + 3] =
                static_cast<uchar>(((alphaRow >> (x * 4)) & 0x0f) * 17);
        }
    }
}

void decodeDxt5Alpha(
    const uchar *block,
    QByteArray &rgba,
    int blockX,
    int blockY,
    int width,
    int height) {
    std::array<uchar, 8> alpha{};
    alpha[0] = block[0];
    alpha[1] = block[1];
    if(alpha[0] > alpha[1]) {
        for(int index = 2; index < 8; ++index)
            alpha[index] = static_cast<uchar>(
                ((8 - index) * alpha[0] + (index - 1) * alpha[1]) / 7);
    } else {
        for(int index = 2; index < 6; ++index)
            alpha[index] = static_cast<uchar>(
                ((6 - index) * alpha[0] + (index - 1) * alpha[1]) / 5);
        alpha[6] = 0;
        alpha[7] = 255;
    }

    quint64 indices = 0;
    for(int byte = 0; byte < 6; ++byte)
        indices |= static_cast<quint64>(block[2 + byte]) << (8 * byte);

    uchar *pixels = reinterpret_cast<uchar *>(rgba.data());
    for(int pixel = 0; pixel < 16; ++pixel) {
        const int pixelX = blockX * 4 + pixel % 4;
        const int pixelY = blockY * 4 + pixel / 4;
        if(pixelX < width && pixelY < height)
            pixels[(pixelY * width + pixelX) * 4 + 3] =
                alpha[(indices >> (pixel * 3)) & 0x07];
    }
}

uchar extractChannel(quint32 pixel, quint32 mask, uchar defaultValue) {
    if(mask == 0)
        return defaultValue;

    int shift = 0;
    while(((mask >> shift) & 1U) == 0U)
        ++shift;
    const quint32 shiftedMask = mask >> shift;
    const quint32 value = (pixel & mask) >> shift;
    return static_cast<uchar>(
        (static_cast<quint64>(value) * 255 + shiftedMask / 2)
        / shiftedMask);
}

} // namespace

qint64 DdsDecoder::minimumUploadBytes(const QString &path, bool nativeDxt1) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return 0;
    const auto header = file.read(DdsFileHeaderSize);
    const char *data = header.constData();
    if (header.size() != DdsFileHeaderSize || !header.startsWith("DDS ")
            || readU32(data + 4) != 124 || readU32(data + 76) != 32)
        return 0;
    const quint32 width = readU32(data + 16);
    const quint32 height = readU32(data + 12);
    if (!width || !height || width > MaximumDimension || height > MaximumDimension)
        return 0;
    const qint64 decodedBytes = qint64(width) * height * 4;
    if (quint64(decodedBytes) > MaximumDecodedBytes)
        return 0;
    // Header information can establish a lower bound. The full mip must
    // still pass the opacity and layout checks before compressed upload.
    if (nativeDxt1 && width % 4 == 0 && height % 4 == 0
            && (readU32(data + 80) & DdsPixelFormatFourCc)
            && readU32(data + 84) == FourCcDxt1 && readU32(data + 24) <= 1)
        return decodedBytes / 8;
    return decodedBytes;
}

bool DdsDecoder::readOpaqueBc1(const QString &path, int maximumSide, DdsBc1Level &level) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const auto header = file.read(DdsFileHeaderSize);
    const char *data = header.constData();
    if (header.size() != DdsFileHeaderSize || !header.startsWith("DDS ")
            || readU32(data + 4) != 124 || readU32(data + 76) != 32
            || !(readU32(data + 80) & DdsPixelFormatFourCc) || readU32(data + 84) != FourCcDxt1
            || readU32(data + 24) > 1)
        return false;

    quint32 width = readU32(data + 16);
    quint32 height = readU32(data + 12);
    if (!width || !height || width > MaximumDimension || height > MaximumDimension
            || quint64(width) * height * 4 > MaximumDecodedBytes)
        return false;
    const quint32 flags = readU32(data + 8);
    const quint32 mipCount = flags & 0x20000 ? readU32(data + 28) : 1;
    const auto bytesFor = [](quint32 w, quint32 h) {
        return qint64((w + 3) / 4) * ((h + 3) / 4) * 8;
    };
    qint64 offset = DdsFileHeaderSize;
    for (quint32 mip = 0; maximumSide > 0 && std::max(width, height) > quint32(maximumSide); ++mip) {
        if (mip + 1 >= mipCount || mip >= 15)
            return false;
        offset += bytesFor(width, height);
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    // Standard terrain dimensions have whole blocks. Edge blocks and
    // transparent selectors use the existing decoder for exact pixel rules.
    if (width % 4 || height % 4 || !file.seek(offset))
        return false;
    const auto size = bytesFor(width, height);
    auto blocks = file.read(size);
    if (blocks.size() != size)
        return false;
    for (qsizetype i = 0; i < blocks.size(); i += 8) {
        const char *block = blocks.constData() + i;
        const quint32 selectors = readU32(block + 4);
        if (readU16(reinterpret_cast<const uchar *>(block))
                    <= readU16(reinterpret_cast<const uchar *>(block + 2))
                && (selectors & (selectors >> 1) & 0x55555555u))
            return false;
    }
    level = {int(width), int(height), std::move(blocks)};
    return true;
}

bool DdsDecoder::decodeStoredPreview(const QString &path, int maximumSide, DdsImage &image) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray header = file.read(DdsFileHeaderSize);
    if (header.size() != DdsFileHeaderSize || !header.startsWith("DDS ")
            || readU32(header.constData() + 4) != 124 || readU32(header.constData() + 76) != 32
            || maximumSide < 1)
        return false;

    const quint32 flags = readU32(header.constData() + 8);
    quint32 height = readU32(header.constData() + 12);
    quint32 width = readU32(header.constData() + 16);
    const quint32 mipCount = readU32(header.constData() + 28);
    if (!width || !height || width > MaximumDimension || height > MaximumDimension
            || !(flags & 0x20000) || mipCount < 2 || mipCount > 16
            || readU32(header.constData() + 24) > 1)
        return false;

    const quint32 format = readU32(header.constData() + 80);
    const quint32 fourCc = readU32(header.constData() + 84);
    const quint32 bits = readU32(header.constData() + 88);
    const bool compressed = format & DdsPixelFormatFourCc;
    int unitSize = 0;
    if (compressed) {
        if (fourCc == FourCcDxt1)
            unitSize = 8;
        else if (fourCc == FourCcDxt3 || fourCc == FourCcDxt5)
            unitSize = 16;
    } else if ((format & DdsPixelFormatRgb) && (bits == 24 || bits == 32)) {
        unitSize = bits / 8;
        // A padded top pitch does not specify the layout of the lower mips.
        if ((flags & DdsHeaderPitch) && readU32(header.constData() + 20) != width * unitSize)
            return false;
    }
    if (!unitSize || std::max(width, height) <= quint32(maximumSide))
        return false;

    const auto levelBytes = [compressed, unitSize](quint32 w, quint32 h) -> qint64 {
        if (compressed)
            return qint64((w + 3) / 4) * ((h + 3) / 4) * unitSize;
        return qint64(w) * h * unitSize;
    };
    qint64 offset = DdsFileHeaderSize;
    for (quint32 level = 1; level < mipCount; ++level) {
        if (!compressed && (width * unitSize) % 4)
            return false;
        offset += levelBytes(width, height);
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
        if (std::max(width, height) > quint32(maximumSide))
            continue;
        if (!compressed && (width * unitSize) % 4)
            return false;
        const qint64 bytes = levelBytes(width, height);
        if (offset + bytes > file.size() || !file.seek(offset))
            return false;
        auto levelPixels = file.read(bytes);
        if (levelPixels.size() != bytes)
            return false;
        qToLittleEndian(height, header.data() + 12);
        qToLittleEndian(width, header.data() + 16);
        if (!compressed)
            qToLittleEndian(quint32(width * unitSize), header.data() + 20);
        header.append(levelPixels);
        return decode(header, image);
    }
    return false;
}

bool DdsDecoder::decodeFile(
    const QString &path, DdsImage &image, QString *error) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        setError(error, QStringLiteral("could not open the file"));
        return false;
    }
    return decode(file.readAll(), image, error);
}

bool DdsDecoder::decode(
    const QByteArray &fileData, DdsImage &image, QString *error,
    quint64 maximumDecodedBytes) {
    image = DdsImage();
    if(fileData.size() < DdsFileHeaderSize) {
        setError(error, QStringLiteral("file is shorter than the DDS header"));
        return false;
    }
    const char *header = fileData.constData();
    if(std::memcmp(header, "DDS ", 4) != 0) {
        setError(error, QStringLiteral("DDS signature is missing"));
        return false;
    }
    if(readU32(header + 4) != 124 || readU32(header + 76) != 32) {
        setError(error, QStringLiteral("DDS header size is invalid"));
        return false;
    }

    const quint32 headerFlags = readU32(header + 8);
    const quint32 heightValue = readU32(header + 12);
    const quint32 widthValue = readU32(header + 16);
    if(widthValue == 0 || heightValue == 0
            || widthValue > MaximumDimension
            || heightValue > MaximumDimension) {
        setError(error, QStringLiteral("DDS dimensions are invalid"));
        return false;
    }
    const quint64 decodedSize =
        static_cast<quint64>(widthValue) * heightValue * 4;
    if(decodedSize > MaximumDecodedBytes || decodedSize > maximumDecodedBytes
            || decodedSize > static_cast<quint64>(
                std::numeric_limits<qsizetype>::max())) {
        setError(error, QStringLiteral("DDS image is too large"));
        return false;
    }

    const int width = static_cast<int>(widthValue);
    const int height = static_cast<int>(heightValue);
    const quint32 pixelFlags = readU32(header + 80);
    const quint32 fourCc = readU32(header + 84);
    const quint32 rgbBits = readU32(header + 88);
    const quint32 redMask = readU32(header + 92);
    const quint32 greenMask = readU32(header + 96);
    const quint32 blueMask = readU32(header + 100);
    const quint32 alphaMask = readU32(header + 104);
    const uchar *source = reinterpret_cast<const uchar *>(
        fileData.constData() + DdsFileHeaderSize);
    const qsizetype available = fileData.size() - DdsFileHeaderSize;

    QByteArray rgba(static_cast<qsizetype>(decodedSize), '\0');
    if((pixelFlags & DdsPixelFormatFourCc) != 0) {
        if(fourCc == FourCcDx10) {
            setError(error, QStringLiteral("DX10 DDS textures are unsupported"));
            return false;
        }
        int blockBytes = 0;
        if(fourCc == FourCcDxt1)
            blockBytes = 8;
        else if(fourCc == FourCcDxt3 || fourCc == FourCcDxt5)
            blockBytes = 16;
        else {
            setError(error, QStringLiteral("DDS compression format is unsupported"));
            return false;
        }

        const int blockColumns = (width + 3) / 4;
        const int blockRows = (height + 3) / 4;
        const quint64 required =
            static_cast<quint64>(blockColumns) * blockRows * blockBytes;
        if(required > static_cast<quint64>(available)) {
            setError(error, QStringLiteral("DDS pixel data is truncated"));
            return false;
        }

        for(int blockY = 0; blockY < blockRows; ++blockY) {
            for(int blockX = 0; blockX < blockColumns; ++blockX) {
                const uchar *block = source
                    + (blockY * blockColumns + blockX) * blockBytes;
                if(fourCc == FourCcDxt1) {
                    decodeColorBlock(
                        block, rgba, blockX, blockY, width, height,
                        false, false);
                } else if(fourCc == FourCcDxt3) {
                    decodeDxt3Alpha(
                        block, rgba, blockX, blockY, width, height);
                    decodeColorBlock(
                        block + 8, rgba, blockX, blockY, width, height,
                        true, true);
                } else {
                    decodeDxt5Alpha(
                        block, rgba, blockX, blockY, width, height);
                    decodeColorBlock(
                        block + 8, rgba, blockX, blockY, width, height,
                        true, true);
                }
            }
        }
    } else if((pixelFlags & DdsPixelFormatRgb) != 0
            && (rgbBits == 24 || rgbBits == 32)) {
        const quint32 bytesPerPixel = rgbBits / 8;
        const quint64 tightPitch =
            static_cast<quint64>(widthValue) * bytesPerPixel;
        quint64 sourcePitch = tightPitch;
        const quint32 declaredPitch = readU32(header + 20);
        if((headerFlags & DdsHeaderPitch) != 0
                && declaredPitch >= tightPitch)
            sourcePitch = declaredPitch;
        const quint64 required = sourcePitch * heightValue;
        if(required > static_cast<quint64>(available)) {
            setError(error, QStringLiteral("DDS pixel data is truncated"));
            return false;
        }

        // Standard byte-channel DDS files need no mask search or scaling per
        // pixel. Keep the generic path for packed/nonstandard channel masks.
        const bool bgr = redMask == 0x00ff0000 && greenMask == 0x0000ff00
            && blueMask == 0x000000ff;
        const bool rgb = redMask == 0x000000ff && greenMask == 0x0000ff00
            && blueMask == 0x00ff0000;
        const bool byteChannels = (bgr || rgb)
            && (alphaMask == 0 || (rgbBits == 32 && alphaMask == 0xff000000));
        uchar *destination = reinterpret_cast<uchar *>(rgba.data());
        for(int y = 0; y < height; ++y) {
            const uchar *row = source + y * sourcePitch;
            if(byteChannels) {
                uchar *output = destination + y * width * 4;
                const int red = bgr ? 2 : 0;
                const int blue = bgr ? 0 : 2;
                for(int x = 0; x < width; ++x, row += bytesPerPixel, output += 4) {
                    output[0] = row[red];
                    output[1] = row[1];
                    output[2] = row[blue];
                    output[3] = alphaMask ? row[3] : 255;
                }
                continue;
            }
            for(int x = 0; x < width; ++x) {
                quint32 pixel = 0;
                std::memcpy(&pixel, row + x * bytesPerPixel, bytesPerPixel);
                pixel = qFromLittleEndian(pixel);
                uchar *output = destination + (y * width + x) * 4;
                output[0] = extractChannel(pixel, redMask, 0);
                output[1] = extractChannel(pixel, greenMask, 0);
                output[2] = extractChannel(pixel, blueMask, 0);
                output[3] = extractChannel(pixel, alphaMask, 255);
            }
        }
    } else {
        setError(error, QStringLiteral("DDS pixel format is unsupported"));
        return false;
    }

    image.width = width;
    image.height = height;
    image.rgba = std::move(rgba);
    image.opaqueDxt1 = (pixelFlags & DdsPixelFormatFourCc) != 0 && fourCc == FourCcDxt1;
    if(image.opaqueDxt1) for(qsizetype i=3;i<image.rgba.size();i+=4) {
        if(static_cast<uchar>(image.rgba[i])!=255) { image.opaqueDxt1=false; break; }
    }
    image.opaqueRgb24 = (pixelFlags & DdsPixelFormatFourCc) == 0
        && (pixelFlags & DdsPixelFormatRgb) != 0 && rgbBits == 24
        && alphaMask == 0 && (pixelFlags & 1) == 0;
    return true;
}

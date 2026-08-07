/*
 * Native DDS decoder used by the Qt6 port.
 *
 * The block layout follows the Microsoft DDS/BC1-BC3 specification.  Only the
 * top mip level is expanded because TSRE generates its OpenGL mipmaps after
 * upload.
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
    const QByteArray &fileData, DdsImage &image, QString *error) {
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
    if(decodedSize > MaximumDecodedBytes
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

        uchar *destination = reinterpret_cast<uchar *>(rgba.data());
        for(int y = 0; y < height; ++y) {
            const uchar *row = source + y * sourcePitch;
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
    return true;
}

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "DdsDecoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QtEndian>

#include <cstring>

namespace {

void writeU32(QByteArray &data, int offset, quint32 value) {
    qToLittleEndian(
        value, reinterpret_cast<uchar *>(data.data() + offset));
}

void writeU16(QByteArray &data, int offset, quint16 value) {
    qToLittleEndian(
        value, reinterpret_cast<uchar *>(data.data() + offset));
}

QByteArray makeDds(quint32 fourCc, const QByteArray &block) {
    QByteArray data(128, '\0');
    std::memcpy(data.data(), "DDS ", 4);
    writeU32(data, 4, 124);
    writeU32(data, 8, 0x00081007);
    writeU32(data, 12, 4);
    writeU32(data, 16, 4);
    writeU32(data, 20, block.size());
    writeU32(data, 76, 32);
    writeU32(data, 80, 0x00000004);
    writeU32(data, 84, fourCc);
    data.append(block);
    return data;
}

QByteArray makeRgbDds(
    quint32 bitsPerPixel,
    quint32 redMask,
    quint32 greenMask,
    quint32 blueMask,
    quint32 alphaMask,
    const QByteArray &pixels) {
    QByteArray data(128, '\0');
    std::memcpy(data.data(), "DDS ", 4);
    writeU32(data, 4, 124);
    writeU32(data, 8, 0x0000100f);
    writeU32(data, 12, 1);
    writeU32(data, 16, 1);
    writeU32(data, 20, bitsPerPixel / 8);
    writeU32(data, 76, 32);
    writeU32(data, 80, alphaMask == 0 ? 0x00000040 : 0x00000041);
    writeU32(data, 88, bitsPerPixel);
    writeU32(data, 92, redMask);
    writeU32(data, 96, greenMask);
    writeU32(data, 100, blueMask);
    writeU32(data, 104, alphaMask);
    data.append(pixels);
    return data;
}

bool pixelEquals(
    const DdsImage &image,
    int pixel,
    uchar red,
    uchar green,
    uchar blue,
    uchar alpha) {
    const uchar *rgba =
        reinterpret_cast<const uchar *>(image.rgba.constData()) + pixel * 4;
    return rgba[0] == red && rgba[1] == green
        && rgba[2] == blue && rgba[3] == alpha;
}

bool check(bool condition, const QString &message) {
    if(!condition)
        qCritical().noquote() << message;
    return condition;
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    bool passed = true;
    DdsImage image;
    QString error;

    QByteArray dxt1(8, '\0');
    writeU16(dxt1, 0, 0xf800);
    writeU16(dxt1, 2, 0x07e0);
    passed &= check(
        DdsDecoder::decode(makeDds(0x31545844, dxt1), image, &error),
        "DXT1 decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 255, 0, 0, 255),
        "DXT1 opaque pixel was decoded incorrectly");

    QByteArray dxt1Transparent(8, '\0');
    writeU16(dxt1Transparent, 0, 0x0000);
    writeU16(dxt1Transparent, 2, 0xffff);
    writeU32(dxt1Transparent, 4, 0x00000003);
    passed &= check(
        DdsDecoder::decode(
            makeDds(0x31545844, dxt1Transparent), image, &error),
        "DXT1 transparency decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 0, 0, 0, 0),
        "DXT1 transparent selector did not produce zero alpha");

    QByteArray dxt3(16, static_cast<char>(0xff));
    dxt3[0] = 0xf0;
    writeU16(dxt3, 8, 0xf800);
    writeU16(dxt3, 10, 0x07e0);
    writeU32(dxt3, 12, 0);
    passed &= check(
        DdsDecoder::decode(makeDds(0x33545844, dxt3), image, &error),
        "DXT3 decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 255, 0, 0, 0)
            && pixelEquals(image, 1, 255, 0, 0, 255),
        "DXT3 explicit alpha was decoded incorrectly");

    QByteArray dxt5(16, '\0');
    dxt5[0] = 10;
    dxt5[1] = 20;
    dxt5[2] = static_cast<char>(0x3e);
    writeU16(dxt5, 8, 0xf800);
    writeU16(dxt5, 10, 0x07e0);
    passed &= check(
        DdsDecoder::decode(makeDds(0x35545844, dxt5), image, &error),
        "DXT5 decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 255, 0, 0, 0)
            && pixelEquals(image, 1, 255, 0, 0, 255),
        "DXT5 special alpha selectors were decoded incorrectly");

    const QByteArray rgb24("\x00\x00\xff", 3);
    passed &= check(
        DdsDecoder::decode(
            makeRgbDds(
                24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0, rgb24),
            image,
            &error),
        "24-bit DDS decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 255, 0, 0, 255),
        "24-bit DDS pixel was decoded incorrectly");

    const QByteArray rgba32("\x00\xff\x00\x80", 4);
    passed &= check(
        DdsDecoder::decode(
            makeRgbDds(
                32,
                0x00ff0000,
                0x0000ff00,
                0x000000ff,
                0xff000000,
                rgba32),
            image,
            &error),
        "32-bit DDS decode failed: " + error);
    passed &= check(
        pixelEquals(image, 0, 0, 255, 0, 128),
        "32-bit DDS pixel was decoded incorrectly");

    QByteArray rgb24LinearPixels(
        "\x00\x00\xff\x00\xff\x00\xff\x00\x00\xff\xff\xff",
        12);
    QByteArray rgb24Linear = makeRgbDds(
        24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0,
        rgb24LinearPixels);
    writeU32(rgb24Linear, 8, 0x00081007);
    writeU32(rgb24Linear, 12, 2);
    writeU32(rgb24Linear, 16, 2);
    writeU32(rgb24Linear, 20, rgb24LinearPixels.size());
    passed &= check(
        DdsDecoder::decode(rgb24Linear, image, &error),
        "24-bit DDS with DDSD_LINEARSIZE failed: " + error);
    passed &= check(
        image.width == 2 && image.height == 2
            && pixelEquals(image, 0, 255, 0, 0, 255)
            && pixelEquals(image, 3, 255, 255, 255, 255),
        "DDSD_LINEARSIZE was incorrectly used as the per-row pitch");

    QByteArray rgba32LinearPixels(
        "\x00\x00\xff\x20\x00\xff\x00\x40"
        "\xff\x00\x00\x80\xff\xff\xff\xff",
        16);
    QByteArray rgba32Linear = makeRgbDds(
        32,
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0xff000000,
        rgba32LinearPixels);
    writeU32(rgba32Linear, 8, 0x00081007);
    writeU32(rgba32Linear, 12, 2);
    writeU32(rgba32Linear, 16, 2);
    writeU32(rgba32Linear, 20, rgba32LinearPixels.size());
    passed &= check(
        DdsDecoder::decode(rgba32Linear, image, &error),
        "32-bit DDS with DDSD_LINEARSIZE failed: " + error);
    passed &= check(
        image.width == 2 && image.height == 2
            && pixelEquals(image, 0, 255, 0, 0, 32)
            && pixelEquals(image, 3, 255, 255, 255, 255),
        "32-bit DDSD_LINEARSIZE or alpha was decoded incorrectly");

    QByteArray truncated = makeDds(0x33545844, dxt3);
    truncated.chop(1);
    passed &= check(
        !DdsDecoder::decode(truncated, image, &error),
        "Truncated DDS data was incorrectly accepted");

    // Independent byte oracle: exercise every component value, both channel
    // orders, padding, stored/absent alpha and the generic packed-mask path.
    for(int bits : {24, 32}) for(bool bgr : {false, true})
        for(bool alpha : {false, true}) {
            if(bits == 24 && alpha) continue;
            const int width = 257, height = 2, stride = bits/8;
            const int pitch = width*stride+7;
            QByteArray pixels(pitch*height, char(0xcd));
            QByteArray expected(width*height*4, '\0');
            for(int i=0; i<width*height; ++i) {
                const uchar red = uchar(i), green = uchar(i*37), blue = uchar(255-i);
                char *source = pixels.data()+(i/width)*pitch+(i%width)*stride;
                source[bgr ? 2 : 0] = char(red);
                source[1] = char(green);
                source[bgr ? 0 : 2] = char(blue);
                if(bits == 32) source[3] = char(i*13);
                char *output = expected.data()+i*4;
                output[0] = char(red); output[1] = char(green); output[2] = char(blue);
                output[3] = alpha ? char(i*13) : char(255);
            }
            QByteArray data = makeRgbDds(bits, bgr ? 0xff0000 : 0xff,
                0xff00, bgr ? 0xff : 0xff0000, alpha ? 0xff000000 : 0, pixels);
            writeU32(data, 12, height); writeU32(data, 16, width); writeU32(data, 20, pitch);
            passed &= check(DdsDecoder::decode(data,image,&error)
                && image.rgba == expected
                && image.opaqueRgb24 == (bits == 24),
                QString("Byte-channel DDS parity failed: %1-bit BGR=%2 alpha=%3: %4")
                    .arg(bits).arg(bgr).arg(alpha).arg(error));
            data.chop(8); // Remove padding and one pixel byte from the last row.
            passed &= check(!DdsDecoder::decode(data,image,&error),
                "Truncated padded DDS was accepted");
        }
    const QByteArray packed = makeRgbDds(24, 0xf800, 0x07e0, 0x001f, 0,
        QByteArray("\x00\xfc\x00",3));
    passed &= check(DdsDecoder::decode(packed,image,&error)
        && pixelEquals(image,0,255,130,0,255), "Packed-mask DDS fallback changed");

    // Opt-in throughput evidence, never a machine-dependent pass threshold.
    if(qEnvironmentVariableIntValue("TSRE_PROFILE_ROUTE_LOAD") != 0) {
        const int size = 2048;
        QByteArray data = makeRgbDds(24,0xff0000,0xff00,0xff,0,
            QByteArray(size*size*3,char(0x5a)));
        writeU32(data,12,size); writeU32(data,16,size); writeU32(data,20,size*3);
        QElapsedTimer timer; timer.start();
        for(int i=0;i<8;++i)
            passed &= check(DdsDecoder::decode(data,image,&error),"DDS benchmark decode failed");
        qInfo() << "DDS RGB24 benchmark: 8 x 2048x2048, elapsed_ms=" << timer.elapsed();
    }

    const QStringList paths = app.arguments().mid(1);
    for(const QString &path : paths) {
        error.clear();
        passed &= check(
            DdsDecoder::decodeFile(path, image, &error),
            path + ": " + error);
        if(!image.rgba.isEmpty())
            qInfo().noquote()
                << QStringLiteral("Decoded %1: %2x%3 RGBA")
                       .arg(path)
                       .arg(image.width)
                       .arg(image.height);
    }

    if(passed)
        qInfo() << "DDS decoder probe passed";
    return passed ? 0 : 1;
}

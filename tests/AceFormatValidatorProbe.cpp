#include "TSREvcWIP/AceFormatValidator.h"

#include <QByteArray>
#include <QDebug>
#include <algorithm>

namespace {
QByteArray makeAce(int width, int height, int channel, int compression,
                   qsizetype payloadLength){
    QByteArray data(qMax<qsizetype>(payloadLength, 37), '\0');
    const QByteArray signature("SIMISA@@@@@@@@@@", 16);
    std::copy(signature.cbegin(), signature.cend(), data.begin());
    data[20] = 0;
    data[24] = char(width & 0xff);
    data[25] = char((width >> 8) & 0xff);
    data[28] = char(height & 0xff);
    data[29] = char((height >> 8) & 0xff);
    data[32] = char(compression);
    data[36] = char(channel);
    return data;
}

void useCompressedWrapper(QByteArray &data){
    const QByteArray prefix("SIMISA@F", 8);
    std::copy(prefix.cbegin(), prefix.cend(), data.begin());
    data[8] = char(0x78);
    data[9] = char(0x56);
    data[10] = char(0x34);
    data[11] = char(0x12);
    data[12] = '@';
    data[13] = '@';
    data[14] = '@';
    data[15] = '@';
}

bool expect(bool condition, const char *message){
    if(!condition)
        qCritical() << message;
    return condition;
}
}

int main(){
    AceFormatLayout layout;
    QString error;
    bool ok = true;

    QByteArray validRgb = makeAce(4, 4, 3, 0, 296);
    ok &= expect(AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(validRgb.constData()),
        validRgb.size(), layout, &error), "valid RGB ACE rejected");
    ok &= expect(layout.dataOffset == 232 && layout.encodedBytes == 48,
                 "RGB ACE layout mismatch");
    QByteArray validCompressedWrapper = validRgb;
    useCompressedWrapper(validCompressedWrapper);
    ok &= expect(AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(validCompressedWrapper.constData()),
        validCompressedWrapper.size(), layout, &error),
        "valid SIMISA@F compressed-wrapper ACE rejected");
    QByteArray poisonedOffsets = validRgb;
    for(int index = 216; index < 232; ++index)
        poisonedOffsets[index] = char(0xff);
    ok &= expect(AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(poisonedOffsets.constData()),
        poisonedOffsets.size(), layout, &error)
        && layout.dataOffset == 232,
        "untrusted row offsets redirected the checked sequential layout");

    QByteArray validDxt = makeAce(5, 5, 4, 18, 296);
    ok &= expect(AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(validDxt.constData()),
        validDxt.size(), layout, &error), "valid clipped-edge DXT ACE rejected");

    QByteArray shortHeader(36, '\0');
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(shortHeader.constData()),
        shortHeader.size(), layout, &error), "truncated header accepted");

    QByteArray badSignature = validRgb;
    badSignature[0] = 'X';
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(badSignature.constData()),
        badSignature.size(), layout, &error), "invalid signature accepted");

    QByteArray tooWide = makeAce(8193, 4, 3, 0, 40000);
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(tooWide.constData()),
        tooWide.size(), layout, &error), "single excessive dimension accepted");

    QByteArray badChannel = makeAce(4, 4, 9, 0, 400);
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(badChannel.constData()),
        badChannel.size(), layout, &error), "unsupported channel layout accepted");

    QByteArray truncatedRgb = makeAce(4, 4, 3, 0, 279);
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(truncatedRgb.constData()),
        truncatedRgb.size(), layout, &error), "truncated RGB payload accepted");

    QByteArray truncatedDxt = makeAce(8, 8, 4, 18, 280);
    ok &= expect(!AceFormatValidator::inspect(
        reinterpret_cast<const unsigned char*>(truncatedDxt.constData()),
        truncatedDxt.size(), layout, &error), "truncated DXT payload accepted");

    return ok ? 0 : 1;
}

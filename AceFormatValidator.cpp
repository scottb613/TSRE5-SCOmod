#include "AceFormatValidator.h"

#include <limits>
#include <cstring>

namespace {
constexpr int MaximumDimension = 8192;
constexpr qint64 MaximumDecodedBytes = 256ll * 1024ll * 1024ll;

bool checkedProduct(qint64 first, qint64 second, qint64 limit, qint64 &result){
    if(first < 0 || second < 0 || (first != 0 && second > limit / first))
        return false;
    result = first * second;
    return true;
}
}

bool AceFormatValidator::inspect(const unsigned char *data, qsizetype length,
                                 AceFormatLayout &layout, QString *error){
    layout = AceFormatLayout();
    auto reject = [&](const QString &message){
        if(error)
            *error = message;
        return false;
    };
    if(data == NULL || length < 37)
        return reject("ACE header is truncated.");
    static const unsigned char uncompressedSignature[16] = {
        0x53,0x49,0x4d,0x49,0x53,0x41,0x40,0x40,
        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40
    };
    static const unsigned char compressedPrefix[8] = {
        0x53,0x49,0x4d,0x49,0x53,0x41,0x40,0x46
    };
    static const unsigned char wrapperSuffix[4] = {
        0x40,0x40,0x40,0x40
    };
    const bool uncompressed = std::memcmp(
        data, uncompressedSignature, sizeof(uncompressedSignature)) == 0;
    // ReadFile decompresses SIMISA@F files but deliberately retains their
    // 16-byte wrapper: prefix, four-byte decoded length, then "@@@@".
    const bool compressedWrapper = std::memcmp(
        data, compressedPrefix, sizeof(compressedPrefix)) == 0
        && std::memcmp(data + 12, wrapperSuffix, sizeof(wrapperSuffix)) == 0;
    if(!uncompressed && !compressedWrapper)
        return reject("ACE signature is invalid.");

    layout.width = int(data[24]) | (int(data[25]) << 8);
    layout.height = int(data[28]) | (int(data[29]) << 8);
    layout.compression = int(data[32]);
    const int channel = int(data[36]);
    if(channel == 3){
        layout.bitsPerPixel = 24;
        layout.channelType = 0;
    } else if(channel == 4){
        layout.bitsPerPixel = 32;
        layout.channelType = 2;
    } else if(channel == 5){
        layout.bitsPerPixel = 32;
        layout.channelType = 1;
    } else {
        return reject("ACE channel layout is unsupported.");
    }
    if(layout.width <= 1 || layout.height <= 1
            || layout.width > MaximumDimension
            || layout.height > MaximumDimension)
        return reject("ACE dimensions are outside the supported 2..8192 range.");

    const qint64 bytesPerPixel = layout.bitsPerPixel / 8;
    qint64 pixels = 0;
    qint64 decoded = 0;
    if(!checkedProduct(layout.width, layout.height,
                       MaximumDecodedBytes, pixels)
            || !checkedProduct(pixels, bytesPerPixel,
                               MaximumDecodedBytes, decoded)
            || decoded > std::numeric_limits<int>::max())
        return reject("ACE decoded image size is excessive or overflows.");
    layout.decodedBytes = qsizetype(decoded);

    qint64 start = 0;
    qint64 encoded = 0;
    if(layout.compression == 18){
        start = 216 + (layout.bitsPerPixel == 24 ? 4 : 20);
        for(int mipHeight = layout.height; mipHeight >= 1; mipHeight /= 2)
            start += 4;
        const qint64 blocksWide = (qint64(layout.width) + 3) / 4;
        const qint64 blocksHigh = (qint64(layout.height) + 3) / 4;
        qint64 blockCount = 0;
        if(!checkedProduct(blocksWide, blocksHigh,
                           std::numeric_limits<qint64>::max() / 8,
                           blockCount))
            return reject("ACE compressed block count overflows.");
        encoded = blockCount * 8;
    } else {
        start = layout.channelType == 0 ? 216
              : layout.channelType == 1 ? 248 : 232;
        const int tableKind = int(data[20]);
        if(tableKind == 0 || tableKind == 4)
            start += qint64(layout.height) * 4;
        else if(tableKind == 1 || tableKind == 5)
            start += qint64(layout.height) * 8 - 4;

        qint64 bytesPerRow = qint64(layout.width) * 3;
        if(layout.channelType == 1)
            bytesPerRow += qint64(layout.width) + layout.height / 8;
        else if(layout.channelType == 2)
            bytesPerRow += (qint64(layout.width) + 7) / 8;
        if(!checkedProduct(bytesPerRow, layout.height,
                           std::numeric_limits<qint64>::max(), encoded))
            return reject("ACE pixel payload size overflows.");
    }

    if(start < 0 || encoded <= 0 || start > length
            || encoded > qint64(length) - start)
        return reject("ACE pixel payload is truncated or has an invalid offset.");
    layout.dataOffset = qsizetype(start);
    layout.encodedBytes = qsizetype(encoded);
    return true;
}

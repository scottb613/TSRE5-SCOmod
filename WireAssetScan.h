#pragma once
// Bounded MSTS world decoding adapted from the existing asset cleanup scanner.
// Wire ownership remains independent of PolyVeg.
#include <QByteArray>
#include <QStringConverter>
#include "WorldCleanupValidation.h"
#include <QtEndian>
namespace WireAssetScan {
constexpr quint32 MaximumExpandedWorldBytes = 256u * 1024u * 1024u;

inline bool uncompressWorldPayload(const QByteArray &data, int offset,
                            QByteArray &payload) {
    if(offset < 0 || data.size() - offset < 4)
        return false;
    const quint32 expandedSize = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(data.constData() + offset));
    if(expandedSize == 0 || expandedSize > MaximumExpandedWorldBytes)
        return false;
    payload = qUncompress(data.mid(offset));
    return !payload.isEmpty()
        && static_cast<quint32>(payload.size()) == expandedSize;
}

inline bool expandedWorldFile(QByteArray data, QByteArray &expanded) {
    expanded = data;
    if(data.size() < 18) return true;
    const bool hasBom = static_cast<unsigned char>(data[0]) == 0xFF
        && static_cast<unsigned char>(data[1]) == 0xFE;
    if(!hasBom && data.size() > 16 && data[7] == 'F') {
        data[12] = data[11];
        data[13] = data[10];
        data[14] = data[9];
        data[15] = data[8];
        QByteArray payload;
        if(!uncompressWorldPayload(data, 12, payload)) return false;
        expanded = data.left(16) + payload;
        return true;
    }
    if(hasBom && data.size() > 34 && data[16] == 'F') {
        data[30] = data[19];
        data[31] = data[18];
        data[32] = data[17];
        data[33] = data[13];
        QByteArray payload;
        if(!uncompressWorldPayload(data, 30, payload)) return false;
        expanded = data.left(34) + payload;
        return true;
    }
    return true;
}

inline bool decodedWorldFile(const QByteArray &input, QString &text,
                      QByteArray &binaryPayload) {
    QByteArray data;
    if(!expandedWorldFile(input, data)) return false;
    // Expansion retains the compressed wrapper, whose size bytes are not text.
    // Restore its ordinary signature before decoding a textual world payload.
    if(input.size() > 16 && input.startsWith("SIMISA@F"))
        data.replace(0, 16, "SIMISA@@@@@@@@@@");
    else if(input.size() > 34 && input.startsWith(QByteArray::fromHex("fffe"))
            && input[16] == 'F') {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        data.replace(0, 34, QByteArray::fromHex("fffe") + encoder(u"SIMISA@@@@@@@@@@"));
    }
    text.clear();
    binaryPayload.clear();

    // MSTS binary world files carry the JINX0w0b marker ahead of a binary
    // token stream whose string values are UTF-16LE. Treating that stream as
    // UTF-8 rejects otherwise valid files (for example on byte 0x95). Keep the
    // bytes intact and let the narrowly scoped generated-name scan below look
    // for the exact ASCII/UTF-16LE filename encodings instead.
    const QByteArray header = data.left(48).toLower();
    const QByteArray asciiBinaryMarker("jinx0w0b", 8);
    const qsizetype markerOffset = header.indexOf(asciiBinaryMarker);
    if(markerOffset >= 0) {
        constexpr quint32 BinaryWorldRootToken = 261844u + 375u;
        const qsizetype tokenOffset = markerOffset + 16;
        if(data.size() - tokenOffset < 8)
            return false;
        const quint32 rootToken = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(data.constData() + tokenOffset));
        const quint32 declaredBytes = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(data.constData() + tokenOffset + 4));
        const qsizetype availableBytes = data.size() - tokenOffset - 8;
        if(rootToken != BinaryWorldRootToken
                || declaredBytes != static_cast<quint32>(availableBytes))
            return false;
        binaryPayload = data.mid(tokenOffset + 8);
        return true;
    }

    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFF
            && static_cast<unsigned char>(data[1]) == 0xFE) {
        QStringDecoder decoder(QStringDecoder::Utf16LE, QStringConverter::Flag::Stateless);
        text = decoder(data);
        return !decoder.hasError() && WorldCleanupValidation::completeTextWorld(text);
    }
    if(data.size() >= 2
            && static_cast<unsigned char>(data[0]) == 0xFE
            && static_cast<unsigned char>(data[1]) == 0xFF) {
        QStringDecoder decoder(QStringDecoder::Utf16BE, QStringConverter::Flag::Stateless);
        text = decoder(data);
        return !decoder.hasError() && WorldCleanupValidation::completeTextWorld(text);
    }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    text = decoder(data);
    return !decoder.hasError() && WorldCleanupValidation::completeTextWorld(text);
}

}

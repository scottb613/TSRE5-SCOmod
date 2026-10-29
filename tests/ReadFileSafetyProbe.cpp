// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "FileBuffer.h"
#include "ReadFile.h"
#include "AceFormatValidator.h"

#include <QByteArray>
#include <QDebug>
#include <QTemporaryFile>
#include <QtEndian>

#include <memory>

FileBuffer::FileBuffer(unsigned char *source, int sourceLength)
    : off(0), length(sourceLength), data(source) {
}

FileBuffer::~FileBuffer(){
    delete[] data;
}

static std::unique_ptr<FileBuffer> readBytes(const QByteArray &bytes){
    QTemporaryFile file;
    if(!file.open()){
        qCritical() << "Unable to create temporary input" << file.errorString();
        return nullptr;
    }
    if(file.write(bytes) != bytes.size() || !file.seek(0)){
        qCritical() << "Unable to prepare temporary input" << file.errorString();
        return nullptr;
    }
    return std::unique_ptr<FileBuffer>(ReadFile::read(&file));
}

static QByteArray makeAsciiCompressed(const QByteArray &payload){
    const QByteArray compressed = qCompress(payload);
    QByteArray input(16, '\0');
    input[7] = 'F';
    input[8] = compressed[3];
    input[9] = compressed[2];
    input[10] = compressed[1];
    input[11] = compressed[0];
    input.append(compressed.sliced(4));
    return input;
}

static QByteArray makeUtf16Compressed(const QByteArray &payload){
    const QByteArray compressed = qCompress(payload);
    QByteArray input(34, '\0');
    input[0] = static_cast<char>(0xFF);
    input[1] = static_cast<char>(0xFE);
    input[16] = 'F';
    input[19] = compressed[0];
    input[18] = compressed[1];
    input[17] = compressed[2];
    input[13] = compressed[3];
    input.append(compressed.sliced(4));
    return input;
}

static bool hasPayloadSuffix(const FileBuffer *buffer, const QByteArray &payload){
    if(buffer == nullptr || buffer->length < payload.size())
        return false;
    const int offset = buffer->length - payload.size();
    return QByteArray(reinterpret_cast<const char*>(buffer->data + offset), payload.size())
            == payload;
}

int main(){
    if(readBytes(QByteArray())){
        qCritical() << "Zero-byte input was accepted";
        return 1;
    }
    if(readBytes(QByteArray(1, 'x'))){
        qCritical() << "One-byte input was accepted";
        return 2;
    }
    if(readBytes(QByteArray(7, 'x'))){
        qCritical() << "Seven-byte input was accepted";
        return 3;
    }

    QByteArray truncatedAscii(8, '\0');
    truncatedAscii[7] = 'F';
    if(readBytes(truncatedAscii)){
        qCritical() << "Truncated ASCII compressed header was accepted";
        return 4;
    }

    QByteArray truncatedUtf16(17, '\0');
    truncatedUtf16[0] = static_cast<char>(0xFF);
    truncatedUtf16[1] = static_cast<char>(0xFE);
    truncatedUtf16[16] = 'F';
    if(readBytes(truncatedUtf16)){
        qCritical() << "Truncated UTF-16 compressed header was accepted";
        return 5;
    }

    QByteArray invalidCompressed(16, '\0');
    invalidCompressed[7] = 'F';
    if(readBytes(invalidCompressed)){
        qCritical() << "Invalid compressed payload was accepted";
        return 6;
    }

    const QByteArray plain("SIMISA plain fixture");
    const std::unique_ptr<FileBuffer> plainResult = readBytes(plain);
    if(!plainResult || plainResult->length != plain.size()
            || QByteArray(reinterpret_cast<const char*>(plainResult->data), plainResult->length)
                != plain){
        qCritical() << "Plain input did not round-trip";
        return 7;
    }

    const QByteArray payload("compressed route fixture");
    const std::unique_ptr<FileBuffer> asciiResult = readBytes(makeAsciiCompressed(payload));
    if(!hasPayloadSuffix(asciiResult.get(), payload)){
        qCritical() << "Valid ASCII compressed input did not decode";
        return 8;
    }

    const std::unique_ptr<FileBuffer> utf16Result = readBytes(makeUtf16Compressed(payload));
    if(!hasPayloadSuffix(utf16Result.get(), payload)){
        qCritical() << "Valid UTF-16 compressed input did not decode";
        return 9;
    }

    // Exercise the actual reader-to-validator boundary, not a fabricated
    // expanded wrapper. qCompress covers the ordinary zlib-wrapped ACE input.
    QByteArray ace(280, '\0');
    ace.replace(0, 16, QByteArray("SIMISA@@@@@@@@@@", 16));
    qToLittleEndian<quint32>(4, ace.data() + 24);
    qToLittleEndian<quint32>(4, ace.data() + 28);
    ace[36] = 3;
    QByteArray compressedAce = makeAsciiCompressed(ace.mid(16));
    compressedAce.replace(0, 8, QByteArray("SIMISA@F", 8));
    const auto aceResult = readBytes(compressedAce);
    AceFormatLayout layout;
    QString error;
    if(!aceResult || !AceFormatValidator::inspect(aceResult->data,
            aceResult->length, layout, &error) || layout.width != 4 || layout.height != 4) {
        qCritical() << "Compressed ACE reader/validator integration failed" << error;
        return 10;
    }
    // Optional local evidence is read only, and is never embedded in fixtures.
    const QString samplePath = qEnvironmentVariable("TSRE_TEST_ACE_FILE");
    if(!samplePath.isEmpty()) {
        QFile sample(samplePath);
        if(!sample.open(QIODevice::ReadOnly)) return 11;
        const std::unique_ptr<FileBuffer> actual(ReadFile::read(&sample));
        if(!actual || !AceFormatValidator::inspect(actual->data, actual->length, layout, &error)) {
            qCritical() << "Local ACE reader/validator failed" << error;
            return 12;
        }
        qInfo() << "Local ACE reader/validator passed:" << layout.width << layout.height;
    }
    return 0;
}

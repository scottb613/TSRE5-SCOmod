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
#include <QVector>

#include <memory>

FileBuffer::FileBuffer(unsigned char *source, int sourceLength)
    : off(0), length(sourceLength), data(source) {
}

FileBuffer::~FileBuffer(){
    delete[] data;
}

// This probe supplies the primitive buffer operations without linking the
// editor's Game dependencies. getBlockEnd() itself is the production method.
int FileBuffer::getInt(){
    const int value = qFromLittleEndian<qint32>(data + off);
    off += 4;
    return value;
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

static QByteArray binaryRecord(int token, const QByteArray &payload){
    QByteArray bytes(8, '\0');
    qToLittleEndian<quint32>(token + 261844, bytes.data());
    qToLittleEndian<quint32>(payload.size(), bytes.data() + 4);
    return bytes + payload;
}

static bool walkBinaryRecords(FileBuffer &buffer, int parentEnd,
        QVector<int> &tokens){
    while(buffer.off < parentEnd){
        if(parentEnd - buffer.off < 8) return false;
        const int token = buffer.getInt() - 261844;
        const int recordEnd = buffer.getBlockEnd();
        if(recordEnd <= buffer.off || recordEnd > parentEnd) return false;
        tokens.append(token);
        // Fixtures have empty labels, as emitted by binary MSTS world files.
        if(buffer.data[buffer.off++] != 0) return false;
        if(token == 375 || token == 579 || token == 303 || token == 305){
            if(!walkBinaryRecords(buffer, recordEnd, tokens)) return false;
        }
        // Unknown records and scalar fields must also land on the next token.
        buffer.off = recordEnd;
    }
    return buffer.off == parentEnd;
}

static bool binaryWorldBoundaries(){
    const QByteArray label(1, '\0');
    const QByteArray position = binaryRecord(397, label + QByteArray(12, '\0'));
    const QByteArray sphere = binaryRecord(579,
            label + binaryRecord(579, label + position));
    const QByteArray contents = binaryRecord(584, label + QByteArray(4, '\0'))
            + sphere + binaryRecord(303, label + position)
            + binaryRecord(999, label + QByteArray(7, 'x'))
            + binaryRecord(305, label + position);
    const QByteArray root = binaryRecord(375, label + contents);
    // Exercise both world and world-sound envelopes and compressed/plain input.
    for(const QByteArray &marker : {QByteArray("JINX0w0b______\r\n"),
                                   QByteArray("JINX0W0b______\r\n")}){
        const QByteArray payload = marker + root;
        for(const QByteArray &input : {
                QByteArray("SIMISA@@@@@@@@@@", 16) + payload,
                makeAsciiCompressed(payload)}){
            auto buffer = readBytes(input);
            if(!buffer) return false;
            buffer->off = 32;
            QVector<int> tokens;
            if(!walkBinaryRecords(*buffer, buffer->length, tokens)
                    || tokens != QVector<int>({375, 584, 579, 579, 397,
                                              303, 397, 999, 305, 397}))
                return false;
        }
    }
    return true;
}

int main(){
    if(!binaryWorldBoundaries()){
        qCritical() << "Binary world/world-sound record boundaries drifted";
        return 13;
    }
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
    const QString worldPath = qEnvironmentVariable("TSRE_TEST_WORLD_FILE");
    if(!worldPath.isEmpty()){
        QFile sample(worldPath);
        if(!sample.open(QIODevice::ReadOnly)) return 14;
        const std::unique_ptr<FileBuffer> actual(ReadFile::read(&sample));
        if(!actual || actual->length < 41) return 15;
        actual->off = 32;
        QVector<int> tokens;
        if(!walkBinaryRecords(*actual, actual->length, tokens)
                || tokens.isEmpty() || tokens.front() != 375){
            qCritical() << "Local binary world record walk failed";
            return 16;
        }
        qInfo() << "Local binary world record walk passed:" << tokens.size();
    }
    return 0;
}

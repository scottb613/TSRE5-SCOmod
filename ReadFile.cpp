/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ReadFile.h"
#include <limits>

FileBuffer* ReadFile::read(QFile* file) {
    if (file == nullptr || !file->isOpen() || !file->isReadable()) {
        qWarning() << "ReadFile: file is not open for reading";
        return nullptr;
    }

    QByteArray input = file->readAll();
    if (file->error() != QFileDevice::NoError) {
        qWarning() << "ReadFile: unable to read complete file" << file->fileName()
                   << file->errorString();
        return nullptr;
    }
    if (input.size() < 8) {
        qWarning() << "ReadFile: file is too short" << file->fileName() << input.size();
        return nullptr;
    }
    if (input.size() > std::numeric_limits<int>::max()) {
        qWarning() << "ReadFile: file is too large" << file->fileName() << input.size();
        return nullptr;
    }

    const bool utf16Bom = static_cast<unsigned char>(input[0]) == 0xFF
            && static_cast<unsigned char>(input[1]) == 0xFE;
    const bool compressedAscii = !utf16Bom && input.size() > 7 && input[7] == 'F';
    const bool compressedUtf16 = utf16Bom && input.size() > 16 && input[16] == 'F';

    QByteArray decoded;
    qsizetype headerSize = 0;
    if (compressedAscii) {
        if (input.size() < 16) {
            qWarning() << "ReadFile: truncated compressed header" << file->fileName();
            return nullptr;
        }
        input[12] = input[11];
        input[13] = input[10];
        input[14] = input[9];
        input[15] = input[8];
        decoded = qUncompress(
            reinterpret_cast<const uchar*>(input.constData() + 12), input.size() - 12);
        headerSize = 16;
    } else if (compressedUtf16) {
        if (input.size() < 34) {
            qWarning() << "ReadFile: truncated UTF-16 compressed header" << file->fileName();
            return nullptr;
        }
        input[30] = input[19];
        input[31] = input[18];
        input[32] = input[17];
        input[33] = input[13];
        decoded = qUncompress(
            reinterpret_cast<const uchar*>(input.constData() + 30), input.size() - 30);
        headerSize = 34;
    }

    QByteArray output;
    if (headerSize > 0) {
        if (decoded.isEmpty()) {
            qWarning() << "ReadFile: compressed payload is invalid" << file->fileName();
            return nullptr;
        }
        if (decoded.size() > std::numeric_limits<int>::max() - headerSize) {
            qWarning() << "ReadFile: decompressed file is too large" << file->fileName();
            return nullptr;
        }
        output.reserve(headerSize + decoded.size());
        output.append(input.constData(), headerSize);
        output.append(decoded);
    } else {
        output = input;
    }

    const int outputSize = static_cast<int>(output.size());
    unsigned char* data = new unsigned char[outputSize];
    std::copy(output.cbegin(), output.cend(), data);
    return new FileBuffer(data, outputSize);
}

FileBuffer* ReadFile::readRAW(QFile* file) {
    if (file == nullptr || !file->isOpen() || !file->isReadable()) {
        qWarning() << "ReadFile: raw file is not open for reading";
        return nullptr;
    }
    QByteArray fileData = file->readAll();
    if (file->error() != QFileDevice::NoError) {
        qWarning() << "ReadFile: unable to read complete raw file" << file->fileName()
                   << file->errorString();
        return nullptr;
    }
    if (fileData.isEmpty()) {
        qWarning() << "ReadFile: raw file is empty" << file->fileName();
        return nullptr;
    }
    if (fileData.size() > std::numeric_limits<int>::max()) {
        qWarning() << "ReadFile: raw file is too large" << file->fileName() << fileData.size();
        return nullptr;
    }
    const int nLength = static_cast<int>(fileData.size());
    unsigned char* data = new unsigned char[nLength + 1];
    data[nLength] = 0;
    std::copy(fileData.cbegin(), fileData.cend(), data);
    return new FileBuffer(data, nLength);
}

/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "FileBuffer.h"
#include "ReadFile.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <string> 
#include <algorithm>
#include <memory>
#include "Game.h"

FileBuffer::FileBuffer() {
    this->off = 0;
}

FileBuffer::FileBuffer(unsigned char * data, int nLength) {
    this->data = data;
    this->length = nLength;
    this->off = 0;
}

FileBuffer::FileBuffer(const FileBuffer* orig) {
    length = orig->length;
    data = new unsigned char[length];
    std::copy ( orig->data, orig->data+length, data );
}

FileBuffer::~FileBuffer() {
    delete[] this->data;
}

int FileBuffer::getInt() {
    this->off += 4;
    return *((int*) & this->data[this->off - 4]);
}

int FileBuffer::getToken(){
    this->off += 4;
    return (*((int*) & this->data[this->off - 4]) - this->tokenOffset);
}

void FileBuffer::setTokenOffset(int val){
    this->tokenOffset = val;
}

unsigned int FileBuffer::getUint() {
    this->off += 4;
    return *((unsigned int*) & this->data[this->off - 4]);
}

unsigned short int FileBuffer::getShort() {
    this->off += 2;
    //return this->data[this->off - 2]*256 + 0;
    return *((unsigned short int*) & this->data[this->off - 2]);
}

void FileBuffer::skipBOM(){
    if(this->getShort() == 65279)
        return;
    off -= 2;
    return;
}

bool FileBuffer::isBOM(){
    if(this->getShort() == 65279){
        off -= 2;
        return true;
    }
    off -= 2;
    return false;
}

short int FileBuffer::getSignedShort() {
    this->off += 2;
    //return this->data[this->off - 2]*256 + 0;
    return *((short int*) & this->data[this->off - 2]);
}

float FileBuffer::getFloat() {
    this->off += 4;
    return *(float*) & this->data[this->off - 4];
}

unsigned char FileBuffer::get() {
    return this->data[this->off++];
}

QString* FileBuffer::getString(int start, int end) {
    QString* s = new QString();

    for (int i = start; i < end; i += 2) {
        if (data[i] == 13) continue;
        *s += QChar(data[i], data[i + 1]);
    }
    return s;

}

void FileBuffer::findToken(int id) {
    int s;
    while (length > off) {
        s = (int) getInt();
        if (s == id)
            return;
        s = getInt();
        off += s;
    }
    return;
}

void FileBuffer::toUtf16(){
    if(isBOM()) return;
    // if(Game::debugOutput) qDebug() << __FILE__ << __LINE__ << "converting to UTF16";
    // 
    unsigned char * newData = new unsigned char[length * 2];
    for(int i = 0; i < length; i++){
        newData[i*2] = data[i];
        newData[i*2+1] = 0;
    }
    length = length * 2;
    delete[] data;
    data = newData;
}

bool FileBuffer::insertFile(QString incPath, QString alternativePath, QString* loaded){
    // Eric-from-Trainsim's 8.006m work highlighted ORTS stock whose
    // Include paths mix slash styles or contain legitimate ".." segments.
    // Normalize each complete candidate path instead of rewriting fragments.
    QStringList candidates;
    const auto addCandidate = [&candidates](QString candidate){
        candidate.replace("\\", "/");
        candidate = QDir::cleanPath(candidate);
        if(!candidate.isEmpty() && candidate != "."
                && !candidates.contains(candidate, Qt::CaseInsensitive))
            candidates.push_back(candidate);
    };
    addCandidate(incPath);
    addCandidate(alternativePath);

    QFile file;
    QString openedPath;
    for(const QString& candidate : candidates){
        file.setFileName(candidate);
        if(file.open(QIODevice::ReadOnly)){
            openedPath = candidate;
            break;
        }
    }
    if(openedPath.isEmpty()){
        if(Game::debugOutput)
            qDebug() << __FILE__ << __LINE__
                     << "Include file not found. Tried:" << candidates;
        return false;
    }
    incPath = openedPath;
    if(Game::debugOutput)
        qDebug() << __FILE__ << __LINE__
                 << "Include file is found at this location: \t" << incPath;
    if(loaded != NULL){
        *loaded = incPath;
    }
    std::unique_ptr<FileBuffer> incData(ReadFile::readRAW(&file));
    if (!incData)
        return false;
    incData->toUtf16();
    int remaining = length-off;
    unsigned char * newData = new unsigned char[incData->length + remaining ];
    memcpy(newData, incData->data, incData->length);
    memcpy(newData+incData->length, data+off, remaining);
    delete[] data;
    data = newData;
    length = incData->length + remaining;
    off = 0;
    skipBOM();
    return true;
}

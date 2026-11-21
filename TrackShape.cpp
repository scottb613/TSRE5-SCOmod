/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TrackShape.h"
#include "ParserX.h"
#include <QTextStream>
#include "FileBuffer.h"
#include <cmath>
#include <limits>

namespace {
QString pathToken(FileBuffer *data) {
    QString token;
    while(data->off <= data->length - 2) {
        const QChar ch(data->getShort());
        if(ch.isSpace() && token.isEmpty()) continue;
        if(ch.isSpace() || ch == '(' || ch == ')') {
            data->off -= 2;
            return token;
        }
        token.append(ch);
    }
    return {}; // Every value must be followed by a delimiter.
}
}

bool TrackShape::readIndex(FileBuffer *data, int &index) {
    bool ok = false;
    index = pathToken(data).toInt(&ok);
    return ok && index >= 0;
}

bool TrackShape::atBlockEnd(FileBuffer *data) {
    while(data->off <= data->length - 2) {
        const QChar ch(data->getShort());
        if(ch.isSpace()) continue;
        data->off -= 2;
        return ch == ')';
    }
    return false;
}

bool TrackShape::readPath(FileBuffer *data, SectionIdx &result, bool positioned) {
    SectionIdx parsed;
    if(!readIndex(data, parsed.n) || !RouteSectionPolicy::validTrackPathCount(parsed.n))
        return false;
    if(positioned) {
        for(float *field : {&parsed.pos[0], &parsed.pos[1], &parsed.pos[2], &parsed.rotDeg}) {
            bool ok = false;
            *field = pathToken(data).toFloat(&ok);
            if(!ok || !std::isfinite(*field)) return false;
        }
    }
    for(int i = 0; i < parsed.n; ++i) {
        int sectionId;
        if(!readIndex(data, sectionId)) return false;
        parsed.sect[i] = unsigned(sectionId);
    }
    if(!atBlockEnd(data)) return false;
    result = parsed;
    return true;
}

TrackShape::TrackShape() {
    
}

TrackShape::TrackShape(int t) {
    this->id = t;
}

QString TrackShape::getHashString(){
    if(filename.length() > 0)
        return filename;
    
    QString hash;
    for(int i = 0; i < numpaths; i++){
        for(int j = 0; j < path[i].n; j++)
        hash += QString::number(path[i].sect[j], 16);
    }
    return hash;
}

TrackShape::~TrackShape() {
}

void TrackShape::saveToStream(QTextStream &out){
    out << "TrackShape (\n";
            out << "	TrackPath ( " << id << " " << path[0].n;
            for (int j = 0; j < path[0].n; j++)
                out << " " << path[0].sect[j];
            out << " ) \n";
}

bool TrackShape::loadUtf16Data(FileBuffer *data){
    SectionIdx parsed;
    int pathId = -1;
    QString token;
    while(!(token = ParserX::NextTokenInside(data)).isEmpty()) {
        if(token.compare("trackpath", Qt::CaseInsensitive) == 0) {
            if(pathId >= 0 || !readIndex(data, pathId)
                    || pathId == std::numeric_limits<int>::max() || !readPath(data, parsed, false))
                return false;
        }
        ParserX::SkipToken(data);
    }
    if(pathId < 0) return false;
    id = pathId;
    dyntrack = true;
    numpaths = 1;
    path = new SectionIdx[1]{parsed};
    return true;
}

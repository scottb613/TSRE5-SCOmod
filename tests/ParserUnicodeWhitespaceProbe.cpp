// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "FileBuffer.h"
#include "ParserX.h"

#include <QDebug>
#include <QString>

FileBuffer::FileBuffer() = default;

FileBuffer::FileBuffer(unsigned char *source, int sourceLength)
    : off(0), length(sourceLength), data(source) {
}

FileBuffer::~FileBuffer(){
    delete[] data;
}

unsigned short int FileBuffer::getShort(){
    off += 2;
    return static_cast<unsigned short int>(data[off - 2])
            | (static_cast<unsigned short int>(data[off - 1]) << 8);
}

static FileBuffer makeBuffer(const QString &text){
    const int byteCount = text.size() * 2;
    auto *bytes = new unsigned char[byteCount];
    for(int i = 0; i < text.size(); ++i){
        const ushort value = text.at(i).unicode();
        bytes[i * 2] = static_cast<unsigned char>(value & 0xff);
        bytes[i * 2 + 1] = static_cast<unsigned char>(value >> 8);
    }
    return FileBuffer(bytes, byteCount);
}

static bool expectToken(FileBuffer *buffer, const QString &expected){
    const QString actual = ParserX::NextTokenInside(buffer);
    if(actual == expected)
        return true;
    qCritical() << "Expected token" << expected << "but received" << actual;
    return false;
}

int main(){
    const QChar nbsp(0x00a0);
    const QString source =
        "Wagon ( TestEngine\n"
        "  Type ( Engine )\n"
        "  " + QString(2, nbsp) + " " + nbsp + "\n"
        "  ORTSBrakeShoeFriction ( 0 0.31 )" + nbsp + "\n"
        ")\n"
        "Engine ( TestEngine\n"
        "  Type ( Steam" + nbsp + " )\n"
        "  Name ( \"Steam" + nbsp + "Name\" )\n"
        ")\n";
    FileBuffer buffer = makeBuffer(source);

    if(!expectToken(&buffer, "Wagon"))
        return 1;
    if(ParserX::GetString(&buffer) != "TestEngine")
        return 2;

    if(!expectToken(&buffer, "Type"))
        return 3;
    if(ParserX::GetString(&buffer) != "Engine")
        return 4;
    ParserX::SkipToken(&buffer);

    if(!expectToken(&buffer, "ORTSBrakeShoeFriction"))
        return 5;
    ParserX::SkipToken(&buffer);

    if(!ParserX::NextTokenInside(&buffer).isEmpty()){
        qCritical() << "Unicode whitespace escaped the Wagon section as a token";
        return 6;
    }
    ParserX::SkipToken(&buffer);

    if(!expectToken(&buffer, "Engine"))
        return 7;
    if(ParserX::GetString(&buffer) != "TestEngine")
        return 8;

    if(!expectToken(&buffer, "Type"))
        return 9;
    const QString engineType = ParserX::GetString(&buffer);
    if(engineType != "Steam"){
        qCritical() << "Expected Steam classification but received" << engineType;
        return 10;
    }
    ParserX::SkipToken(&buffer);

    if(!expectToken(&buffer, "Name"))
        return 11;
    const QString expectedQuotedName = QStringLiteral("Steam") + nbsp
            + QStringLiteral("Name");
    const QString quotedName = ParserX::GetString(&buffer);
    if(quotedName != expectedQuotedName){
        qCritical() << "Quoted Unicode whitespace was not preserved" << quotedName;
        return 12;
    }

    return 0;
}

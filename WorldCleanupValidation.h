#pragma once
#include <QStringView>

namespace WorldCleanupValidation {
// A reference scan may authorize deletion only after seeing a complete world
// envelope. This deliberately does not interpret object data or change files.
inline bool completeTextWorld(QStringView text) {
    qsizetype pos = 0;
    if(!text.isEmpty() && text.front() == QChar(0xfeff)) ++pos;
    auto skipSpace = [&]() {
        while(pos < text.size() && text[pos].isSpace()) ++pos;
    };
    skipSpace();
    if(text.mid(pos, 6).compare(u"SIMISA", Qt::CaseInsensitive) == 0) {
        const qsizetype start = pos;
        while(pos < text.size() && !text[pos].isSpace()) ++pos;
        if(text.mid(start, pos - start).compare(
                u"SIMISA@@@@@@@@@@JINX0w0t______", Qt::CaseInsensitive) != 0)
            return false;
        skipSpace();
    }
    constexpr qsizetype rootLength = 12;
    if(text.mid(pos, rootLength).compare(u"Tr_Worldfile", Qt::CaseInsensitive) != 0)
        return false;
    pos += rootLength;
    skipSpace();
    if(pos == text.size() || text[pos++] != QChar('(')) return false;
    qsizetype depth = 1;
    bool quoted = false, escaped = false;
    while(pos < text.size()) {
        const QChar ch = text[pos++];
        if(ch.isNull() || (ch.unicode() < 32 && !ch.isSpace())) return false;
        if(quoted) {
            if(escaped) escaped = false;
            else if(ch == QChar('\\')) escaped = true;
            else if(ch == QChar('"')) quoted = false;
        } else if(ch == QChar('"')) quoted = true;
        else if(ch == QChar('(')) ++depth;
        else if(ch == QChar(')') && --depth == 0) {
            skipSpace();
            return pos == text.size();
        }
    }
    return false;
}
}

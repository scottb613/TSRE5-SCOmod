// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include "WireAssetScan.h"
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QVector>
#include <QtEndian>

namespace TerrainTextureRewrite {
using Names = QMap<QString,QString>; // case-folded filename -> replacement filename
inline quint32 u32(const QByteArray &data, qsizetype p) {
    return qFromLittleEndian<quint32>(data.constData()+p);
}
inline bool stringEnd(const QByteArray &data, qsizetype p, qsizetype &end) {
    if(p+2>data.size()) return false;
    end=p+2+2*qsizetype(qFromLittleEndian<quint16>(data.constData()+p));
    return end<=data.size();
}
inline QByteArray encodedString(const QString &text) {
    QByteArray out(2+text.size()*2,'\0');
    qToLittleEndian<quint16>(quint16(text.size()),out.data());
    for(qsizetype i=0;i<text.size();++i) qToLittleEndian<quint16>(text[i].unicode(),out.data()+2+i*2);
    return out;
}
// Rebuild only the terrain/shader/texture-slot containers. Every other block,
// including patch matrices, sample references and unknown fields, stays opaque.
inline bool block(const QByteArray &input, qsizetype &position, quint32 expected,
                  const Names &names, QByteArray &out, QSet<QString> &changed, int depth=0) {
    if(depth>8 || position+9>input.size()) return false;
    const quint32 token=u32(input,position), length=u32(input,position+4);
    if(token!=expected || length<1 || length>quint64(input.size()-position-8)) return false;
    const QByteArray payload=input.mid(position+8,length);
    const qsizetype label=1+2*quint8(payload[0]);
    if(label>payload.size()) return false;
    QByteArray body=payload;
    if(token==154) {
        qsizetype end=0;
        if(!stringEnd(payload,label,end) || payload.size()-end!=8) return false;
        QString value;
        for(qsizetype p=label+2;p<end;p+=2) value.append(QChar(qFromLittleEndian<quint16>(payload.constData()+p)));
        const auto replacement=names.constFind(value.toCaseFolded());
        if(replacement!=names.cend()) {
            if(replacement->size()>65535) return false;
            body=payload.left(label)+encodedString(*replacement)+payload.mid(end);
            changed.insert(value.toCaseFolded());
        }
    } else if(token==136 || token==151 || token==152 || token==153) {
        qsizetype p=label; quint32 count=0, child=0;
        if(token==151 || token==153) {
            if(p+4>payload.size()) return false;
            count=u32(payload,p); p+=4; child=token==151?152:154;
            if(count>quint64(payload.size()-p)/9) return false;
        } else if(token==152) {
            if(!stringEnd(payload,p,p)) return false;
        }
        body=payload.left(p);
        quint32 seen=0;
        while(p<payload.size()) {
            if(p+9>payload.size()) return false;
            const quint32 next=u32(payload,p), size=u32(payload,p+4);
            if(size<1 || size>quint64(payload.size()-p-8)) return false;
            if(child && next!=child) return false;
            if(next==151 || next==152 || next==153 || next==154) {
                if(!block(payload,p,next,names,body,changed,depth+1)) return false;
            } else { body+=payload.mid(p,8+size); p+=8+size; }
            if(body.size()>WireAssetScan::MaximumExpandedWorldBytes) return false;
            ++seen;
        }
        if(child && seen!=count) return false;
    }
    QByteArray header=input.mid(position,8);
    if(out.size()+body.size()+8>WireAssetScan::MaximumExpandedWorldBytes) return false;
    qToLittleEndian<quint32>(quint32(body.size()),header.data()+4);
    out+=header+body; position+=8+length; return true;
}
inline bool rewrite(const QByteArray &input, const Names &names,
                    QByteArray &output, QSet<QString> &changed) {
    changed.clear(); output=input;
    QByteArray expanded;
    if(!WireAssetScan::expandedWorldFile(input,expanded)) return false;
    const bool compressed=input.startsWith("SIMISA@F");
    if(compressed) expanded.replace(0,16,"SIMISA@@@@@@@@@@");
    // UTF-16 compressed wrappers are intentionally retained, not rewritten.
    if(input.startsWith(QByteArray::fromHex("fffe")) && input.size()>34 && input[16]=='F') return false;
    if(expanded.mid(16,8).toLower()=="jinx0t6b") {
        QByteArray rewritten=expanded.left(32); qsizetype position=32;
        if(!block(expanded,position,136,names,rewritten,changed) || position!=expanded.size()) return false;
        expanded=rewritten;
    } else {
        const bool le=expanded.startsWith(QByteArray::fromHex("fffe"));
        const bool be=expanded.startsWith(QByteArray::fromHex("feff"));
        const bool bom=expanded.startsWith(QByteArray::fromHex("efbbbf"));
        if((le || be) && expanded.size()%2) return false;
        QStringDecoder decoder(le?QStringDecoder::Utf16LE:(be?QStringDecoder::Utf16BE:QStringDecoder::Utf8),QStringConverter::Flag::Stateless);
        QString text=decoder(expanded);
        const bool latin=decoder.hasError() && !le && !be;
        if(decoder.hasError() && !latin) return false;
        if(latin) text=QString::fromLatin1(expanded);
        if(!text.isValidUtf16() || text.contains(QChar(0))) return false;
        static const QRegularExpression slotPattern(R"tsre(\bterrain_texslot\s*\(\s*(?:"([^"]+)"|([^\s()]+)))tsre",
            QRegularExpression::CaseInsensitiveOption);
        auto matches=slotPattern.globalMatch(text);
        struct Edit { qsizetype start,length; QString value; };
        QVector<Edit> edits;
        qsizetype projected=text.size();
        while(matches.hasNext()) {
            const auto match=matches.next(); const int capture=match.capturedStart(1)>=0?1:2;
            const QString old=match.captured(capture).toCaseFolded();
            const auto replacement=names.constFind(old);
            if(replacement==names.cend()) continue;
            // Quote newly spaced tokens without changing an existing quoted span.
            QString value=*replacement;
            if(capture==2 && value.contains(QRegularExpression("[\\s()]"))) value='"'+value+'"';
            projected+=value.size()-match.capturedLength(capture);
            if(projected>WireAssetScan::MaximumExpandedWorldBytes/4) return false;
            edits.append({match.capturedStart(capture),match.capturedLength(capture),value}); changed.insert(old);
        }
        for(auto it=edits.crbegin();it!=edits.crend();++it) text.replace(it->start,it->length,it->value);
        if(changed.isEmpty()) return true;
        if(le || be) {
            QStringEncoder encoder(le?QStringEncoder::Utf16LE:QStringEncoder::Utf16BE);
            expanded=QByteArray::fromHex(le?"fffe":"feff")+encoder(text);
        } else {
            if(latin && QString::fromLatin1(text.toLatin1())!=text) return false;
            expanded=(bom?QByteArray::fromHex("efbbbf"):QByteArray())+(latin?text.toLatin1():text.toUtf8());
        }
    }
    if(changed.isEmpty()) return true;
    if(compressed) {
        output=input.left(16);
        qToLittleEndian<quint32>(quint32(expanded.size()-16),output.data()+8);
        output+=qCompress(expanded.mid(16)).mid(4);
    } else output=expanded;
    return true;
}
}

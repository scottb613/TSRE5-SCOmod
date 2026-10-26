// TSRE GenX - Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "DdsWriter.h"
#include "TerrtexMipmaps.h"
#include "SaveProgress.h"
#include <QDataStream>
#include <QIODevice>
#include <QtEndian>
#include <limits>
#include <algorithm>

namespace {
quint16 rgb565(const int *p) {
    return quint16(((p[0]*31+127)/255)<<11 | ((p[1]*63+127)/255)<<5 | (p[2]*31+127)/255);
}
void unpack565(quint16 c, int *p) {
    const int r=(c>>11)&31, g=(c>>5)&63, b=c&31;
    p[0]=(r<<3)|(r>>2); p[1]=(g<<2)|(g>>4); p[2]=(b<<3)|(b>>2);
}
int colorError(const int *a, const int *b) {
    const int r=a[0]-b[0], g=a[1]-b[1], blue=a[2]-b[2];
    return r*r+2*g*g+blue*blue;
}
void writeBc1(QDataStream &out, const QImage &image, int bx, int by) {
    int pixels[16][3];
    for(int y=0;y<4;++y) for(int x=0;x<4;++x) {
        const uchar *p=image.constScanLine(qMin(by+y,image.height()-1))
            +4*qMin(bx+x,image.width()-1);
        for(int c=0;c<3;++c) pixels[y*4+x][c]=p[c];
    }
    int first=0, last=0, furthest=-1;
    for(int i=0;i<16;++i) for(int j=0;j<i;++j) {
        const int distance=colorError(pixels[i],pixels[j]);
        if(distance>furthest) { furthest=distance; first=i; last=j; }
    }
    quint16 c0=rgb565(pixels[first]), c1=rgb565(pixels[last]);
    quint16 best0=0, best1=0;
    quint32 bestBits=0;
    qint64 bestError=std::numeric_limits<qint64>::max();
    // Fit endpoints to the selected palette twice after the initial fit.
    for(int iteration=0;iteration<3;++iteration) {
        if(c0<c1) std::swap(c0,c1);
        if(c0==c1) { if(c0<65535) ++c0; else --c1; }
        int palette[4][3]; unpack565(c0,palette[0]); unpack565(c1,palette[1]);
        for(int c=0;c<3;++c) {
            palette[2][c]=(2*palette[0][c]+palette[1][c])/3;
            palette[3][c]=(palette[0][c]+2*palette[1][c])/3;
        }
        quint32 bits=0; qint64 error=0;
        double aa=0, ab=0, bb=0, ap[3]={}, bp[3]={};
        const double weights[4]={1,0,2.0/3,1.0/3};
        for(int i=0;i<16;++i) {
            int selected=0, distance=std::numeric_limits<int>::max();
            for(int k=0;k<4;++k) {
                const int candidate=colorError(pixels[i],palette[k]);
                if(candidate<distance) { distance=candidate; selected=k; }
            }
            bits|=quint32(selected)<<(2*i); error+=distance;
            const double a=weights[selected], b=1-a;
            aa+=a*a; ab+=a*b; bb+=b*b;
            for(int c=0;c<3;++c) { ap[c]+=a*pixels[i][c]; bp[c]+=b*pixels[i][c]; }
        }
        if(error<bestError) { bestError=error; best0=c0; best1=c1; bestBits=bits; }
        const double det=aa*bb-ab*ab;
        if(det<1.e-8) break;
        int a[3], b[3];
        for(int c=0;c<3;++c) {
            a[c]=qBound(0,qRound((ap[c]*bb-bp[c]*ab)/det),255);
            b[c]=qBound(0,qRound((bp[c]*aa-ap[c]*ab)/det),255);
        }
        c0=rgb565(a); c1=rgb565(b);
    }
    out<<best0<<best1<<bestBits;
}
}

bool DdsWriter::isOpaqueDxt1(const QByteArray &data) {
    if(data.size()<128 || data.left(4)!="DDS ") return false;
    auto word=[&](int n){ return qFromLittleEndian<quint32>(data.constData()+n); };
    if(word(4)!=124 || word(76)!=32 || word(80)!=4 || word(84)!=0x31545844
            || (word(112)&0x20fe00)!=0 || word(24)!=0) return false;
    quint32 w=word(16), h=word(12), levels=qMax(quint32(1),word(28));
    if(!w || !h || w>8192 || h>8192 || levels>14) return false;
    qsizetype offset=128;
    for(quint32 mip=0;mip<levels;++mip) {
        const qsizetype size=qsizetype((w+3)/4)*((h+3)/4)*8;
        if(size>data.size()-offset) return false;
        for(qsizetype p=offset;p<offset+size;p+=8)
            if(qFromLittleEndian<quint16>(data.constData()+p)
                    <=qFromLittleEndian<quint16>(data.constData()+p+2)) return false;
        offset+=size;
        if(mip+1<levels && w==1 && h==1) return false;
        w=qMax(quint32(1),w/2); h=qMax(quint32(1),h/2);
    }
    return offset==data.size();
}

bool DdsWriter::serializeDxt1(const QImage &base, bool mipmaps, QByteArray &data, QString *error) {
    data.clear(); if(error) error->clear();
    if(base.isNull() || base.width()>8192 || base.height()>8192) {
        if(error) *error="Invalid dimensions for DXT1 production output.";
        return false;
    }
    // Opaque RGB conversion deliberately strips alpha before filtering mips.
    const QImage opaque=base.convertToFormat(QImage::Format_RGB888).convertToFormat(QImage::Format_RGBA8888);
    if(opaque.isNull()) { if(error) *error="Could not allocate DXT1 source pixels."; return false; }
    QVector<QImage> levels;
    if(mipmaps) {
        SaveProgress::Range generation(0,0.25);
        if(!TerrtexMipmaps::generateDds(opaque,levels,error)) return false;
    } else levels.append(opaque);
    qsizetype size=128;
    for(const QImage &level:levels) size+=qsizetype((level.width()+3)/4)*((level.height()+3)/4)*8;
    data.reserve(size);
    QDataStream out(&data,QIODevice::WriteOnly); out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("DDS ",4);
    out<<quint32(124)<<quint32(0x81007 | (levels.size()>1 ? 0x20000 : 0))
        <<quint32(base.height())<<quint32(base.width())
        <<quint32(((base.width()+3)/4)*((base.height()+3)/4)*8)
        <<quint32(0)<<quint32(levels.size());
    for(int i=0;i<11;++i) out<<quint32(0);
    out<<quint32(32)<<quint32(4)<<quint32(0x31545844);
    for(int i=0;i<5;++i) out<<quint32(0);
    out<<quint32(0x1000 | (levels.size()>1 ? 0x400008 : 0));
    for(int i=0;i<4;++i) out<<quint32(0);
    SaveProgress::Range encoding(mipmaps ? 0.25 : 0,1);
    double done=0, total=double(size-128);
    for(const QImage &level:levels) for(int y=0;y<level.height();y+=4) {
        SaveProgress::set(done/total);
        if(y%32==0) SaveProgress::report(QString("Compressing DXT1 %1 x %2: row %3 of %4")
            .arg(level.width()).arg(level.height()).arg(y+1).arg(level.height()));
        for(int x=0;x<level.width();x+=4) writeBc1(out,level,x,y);
        done+=((level.width()+3)/4)*8;
    }
    if(out.status()!=QDataStream::Ok || data.size()!=size || !isOpaqueDxt1(data)) {
        data.clear(); if(error) *error="DXT1 output validation failed."; return false;
    }
    return true;
}

bool DdsWriter::serialize24(const QImage &base, bool mipmaps, QByteArray &data, QString *error) {
    data.clear();
    if(error) error->clear();
    if(base.isNull() || base.width() < 1 || base.height() < 1
            || base.width() > 8192 || base.height() > 8192) {
        if(error) *error = "Invalid TERRTEX dimensions for DDS output.";
        return false;
    }
    const QImage rgba = base.convertToFormat(QImage::Format_RGBA8888);
    if(rgba.isNull()) {
        if(error) *error = "Could not allocate DDS source pixels.";
        return false;
    }
    if(base.hasAlphaChannel()) {
        for(int y=0;y<rgba.height();++y) {
            const uchar *row = rgba.constScanLine(y);
            for(int x=0;x<rgba.width();++x) if(row[x*4+3] != 255) {
                if(error) *error = "24-bit DDS requires opaque TERRTEX. Use ACE for textures with transparency.";
                return false;
            }
        }
    }
    QVector<QImage> levels;
    if(mipmaps) {
        SaveProgress::Range generation(0, 0.75);
        if(!TerrtexMipmaps::generate(rgba,levels,error)) return false;
    } else levels.append(rgba);
    SaveProgress::Range encoding(mipmaps ? 0.75 : 0, 1);
    qsizetype bytes = 128;
    for(const QImage &level : levels) bytes += qsizetype(level.width())*level.height()*3;
    data.reserve(bytes);
    QDataStream out(&data,QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("DDS ",4);
    out << quint32(124) << quint32(0x100f | (mipmaps ? 0x20000 : 0))
        << quint32(base.height()) << quint32(base.width()) << quint32(base.width()*3)
        << quint32(0) << quint32(levels.size());
    for(int i=0;i<11;++i) out << quint32(0);
    // Legacy RGB24, tightly packed BGR bytes. No FourCC or compression.
    out << quint32(32) << quint32(0x40) << quint32(0) << quint32(24)
        << quint32(0x00ff0000) << quint32(0x0000ff00) << quint32(0x000000ff) << quint32(0);
    out << quint32(0x1000 | (mipmaps ? 0x400008 : 0));
    for(int i=0;i<4;++i) out << quint32(0);
    double pixelsDone = 0, pixelsTotal = 0;
    for(const QImage &level : levels) pixelsTotal += double(level.width())*level.height();
    for(const QImage &level : levels) {
        SaveProgress::Range mip(pixelsDone/pixelsTotal,
            (pixelsDone+double(level.width())*level.height())/pixelsTotal);
        pixelsDone += double(level.width())*level.height();
        QByteArray encodedRow(level.width()*3,'\0');
        for(int y=0;y<level.height();++y) {
            SaveProgress::set(double(y)/level.height());
            if(y%32==0) SaveProgress::report(QString("Encoding DDS RGB24 %1 x %2: row %3 of %4")
                .arg(level.width()).arg(level.height()).arg(y+1).arg(level.height()));
            const uchar *pixels = level.constScanLine(y);
            char *row = encodedRow.data();
            for(int x=0;x<level.width();++x) {
                row[x*3] = char(pixels[x*4+2]);
                row[x*3+1] = char(pixels[x*4+1]);
                row[x*3+2] = char(pixels[x*4]);
            }
            out.writeRawData(encodedRow.constData(),int(encodedRow.size()));
        }
    }
    if(out.status()!=QDataStream::Ok || data.size()!=bytes) {
        data.clear();
        if(error) *error = "Could not encode complete DDS RGB24 output.";
        return false;
    }
    return true;
}

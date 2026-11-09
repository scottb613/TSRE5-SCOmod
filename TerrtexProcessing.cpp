// TSRE GenX TERRTEX processing. GNU GPL v3 or later. See LICENSE.md.
#include "TerrtexProcessing.h"
#include "ace_codec/AceDocument.h"
#include "ace_codec/DxtCodec.h"
#include "DdsWriter.h"
#include "DdsDecoder.h"
#include "TerrtexMipmaps.h"
#include "SaveProgress.h"
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QtEndian>
#include <algorithm>
#include <cstring>
#include <utility>

namespace {
constexpr qint64 InputLimit = 256LL*1024*1024;
quint32 word(const QByteArray &b,int p) { return qFromLittleEndian<quint32>(b.constData()+p); }
void put(QByteArray &b,int p,quint32 v) { qToLittleEndian(v,b.data()+p); }
bool fail(QString &error,const QString &message) { error=message; return false; }
int fullCount(int w,int h) { int n=1; while(w>1 || h>1) { w=std::max(1,w/2); h=std::max(1,h/2); ++n; } return n; }
struct Level { int width=0,height=0,pitch=0; QByteArray data; };
struct Dds {
    QByteArray header,trailing; QVector<Level> levels;
    bool compressed=false,padded=false; int bits=0;
    DxtCodec::Format format=DxtCodec::Format::Dxt1;
    quint32 masks[4]={};
};
bool ddsRead(const QByteArray &bytes,Dds &out,QString &error) {
    if(bytes.size()<128 || bytes.left(4)!="DDS " || word(bytes,4)!=124 || word(bytes,76)!=32)
        return fail(error,"Invalid DDS header.");
    const quint32 w=word(bytes,16),h=word(bytes,12),flags=word(bytes,80);
    if(!w || !h || w>8192 || h>8192 || word(bytes,24)!=0 || word(bytes,112)!=0)
        return fail(error,"Only supported 2D TERRTEX dimensions/formats can be processed.");
    const quint32 count=std::max(1u,word(bytes,28));
    if(count>quint32(fullCount(w,h))) return fail(error,"DDS has excessive mip levels.");
    Dds doc; doc.header=bytes.left(128); doc.compressed=(flags&4)!=0; int basePitch=0;
    if(doc.compressed) {
        const quint32 four=word(bytes,84);
        if(four<0x31545844 || four>0x35545844 || (four&0x00ffffff)!=0x00545844)
            return fail(error,"DDS processing supports DXT1 through DXT5, not DX10 or other compression.");
        doc.format=static_cast<DxtCodec::Format>((four>>24)-'1');
    } else {
        doc.bits=int(word(bytes,88));
        if(!(flags&0x40) || (doc.bits!=24 && doc.bits!=32))
            return fail(error,"DDS processing requires RGB24/RGBA32 or supported DXT blocks.");
        quint32 used=0;
        for(int i=0;i<4;++i) {
            const quint32 mask=word(bytes,92+i*4); doc.masks[i]=mask;
            if(i<3 && !mask) return fail(error,"DDS RGB channel mask is missing.");
            if(mask) {
                quint32 value=mask; while(!(value&1)) value>>=1;
                if((value&(value+1)) || (used&mask) || (doc.bits==24 && (mask&0xff000000)))
                    return fail(error,"DDS channel masks are unsupported or overlap.");
                used|=mask;
            }
        }
        const quint32 tight=w*(doc.bits/8),pitch=(word(bytes,8)&8)?word(bytes,20):tight;
        if(pitch!=tight && pitch!=((tight+3)&~3u))
            return fail(error,"Unsupported DDS row padding.");
        doc.padded=pitch>tight;
        basePitch=int(pitch);
        // A base row can already be DWORD-aligned while tiny RGB24 mips still
        // need padding. Select only an exact complete-layout size; never shift
        // authored lower rows by assuming the base reveals their alignment.
        if(doc.bits==24 && count>1) {
            qint64 tightTotal=0,paddedTotal=0; int mw=int(w),mh=int(h);
            for(quint32 m=0;m<count;++m) {
                tightTotal+=qint64(m==0?basePitch:mw*3)*mh;
                paddedTotal+=qint64(m==0?basePitch:((mw*3+3)&~3))*mh;
                mw=std::max(1,mw/2); mh=std::max(1,mh/2);
            }
            if(tightTotal!=paddedTotal) {
                if(bytes.size()-128==tightTotal) doc.padded=false;
                else if(bytes.size()-128==paddedTotal) doc.padded=true;
                else return fail(error,"Ambiguous RGB24 mip row alignment or trailing data.");
            }
        }
    }
    qint64 at=128; int lw=int(w),lh=int(h);
    for(quint32 m=0;m<count;++m) {
        const int tight=lw*(doc.bits/8),pitch=m==0?basePitch:doc.padded?((tight+3)&~3):tight;
        const qint64 size=doc.compressed?DxtCodec::byteSize(lw,lh,doc.format):qint64(pitch)*lh;
        if(size<=0 || at>bytes.size() || size>bytes.size()-at)
            return fail(error,"DDS mip payload is truncated.");
        doc.levels.append({lw,lh,pitch,bytes.mid(at,size)}); at+=size;
        lw=std::max(1,lw/2); lh=std::max(1,lh/2);
    }
    doc.trailing=bytes.mid(at); out=std::move(doc); return true;
}
bool ddsSerialize(const Dds &doc,QByteArray &bytes,QString &error) {
    qint64 total=128+doc.trailing.size(); for(const auto &level:doc.levels) total+=level.data.size();
    if(total>InputLimit) return fail(error,"Processed DDS exceeds the 256 MiB file budget.");
    bytes=doc.header;
    put(bytes,28,quint32(doc.levels.size()));
    put(bytes,8,(word(bytes,8)&~0x20000u)|(doc.levels.size()>1?0x20000u:0));
    put(bytes,108,(word(bytes,108)&~0x400008u)|(doc.levels.size()>1?0x400008u:0));
    for(const auto &level:doc.levels) bytes+=level.data;
    bytes+=doc.trailing; return true;
}
bool ddsImage(const Dds &doc,int index,QImage &image,QString &error) {
    const auto &level=doc.levels[index]; QByteArray pixels;
    if(doc.compressed) {
        if(!DxtCodec::decode(level.data,level.width,level.height,doc.format,true,pixels,error,
                            DxtCodec::ColorExpansion::LegacyBitReplication)) return false;
    } else {
        QByteArray one=doc.header; put(one,16,level.width); put(one,12,level.height);
        put(one,8,word(one,8)|8u); put(one,20,level.pitch); put(one,28,1); one+=level.data;
        DdsImage decoded; if(!DdsDecoder::decode(one,decoded,&error)) return false;
        pixels=std::move(decoded.rgba);
    }
    image=QImage(reinterpret_cast<const uchar *>(pixels.constData()),level.width,level.height,
                 level.width*4,QImage::Format_RGBA8888).copy();
    return !image.isNull() || fail(error,"Cannot allocate TERRTEX mip source.");
}
bool ddsEncode(const Dds &doc,const QImage &image,Level &level,QString &error) {
    level.width=image.width(); level.height=image.height();
    QByteArray tight;
    for(int y=0;y<image.height();++y) tight.append(reinterpret_cast<const char *>(image.constScanLine(y)),image.width()*4);
    if(doc.compressed)
        return DxtCodec::encode(reinterpret_cast<const unsigned char *>(tight.constData()),tight.size(),
            image.width(),image.height(),4,doc.format,true,level.data,error);
    const int row=image.width()*(doc.bits/8); level.pitch=doc.padded?((row+3)&~3):row;
    level.data=QByteArray(level.pitch*image.height(),'\0');
    for(int y=0;y<image.height();++y) for(int x=0;x<image.width();++x) {
        const uchar *pixel=image.constScanLine(y)+x*4; quint32 packed=0;
        for(int c=0;c<4;++c) if(doc.masks[c]) {
            quint32 mask=doc.masks[c]; int shift=0; while(!(mask&1)) {mask>>=1; ++shift;}
            packed|=quint32((quint64(pixel[c])*mask+127)/255)<<shift;
        }
        char *dst=level.data.data()+y*level.pitch+x*(doc.bits/8);
        for(int c=0;c<doc.bits/8;++c) dst[c]=char(packed>>(8*c));
    }
    return true;
}
AceReadOptions aceOptions() { AceReadOptions o; o.maxBytes=InputLimit; o.maxDimension=8192; o.legacyDxt1Colors=true; return o; }
bool isAce(const QString &path) { return QFileInfo(path).suffix().compare("ace",Qt::CaseInsensitive)==0; }
bool blockAce(const AceDocument &doc) { return (doc.options()&16) && doc.surface()>=18 && doc.surface()<=22; }
bool aceImage(const AceDocument &doc,int index,QImage &image,QString &error) {
    QByteArray pixels; int c=0; if(!doc.decode(index,pixels,c,error)) return false;
    const auto &level=doc.levels[index];
    image=QImage(reinterpret_cast<const uchar *>(pixels.constData()),level.width,level.height,level.width*c,
        c==4?QImage::Format_RGBA8888:QImage::Format_RGB888).convertToFormat(QImage::Format_RGBA8888).copy();
    return !image.isNull() || fail(error,"Cannot allocate ACE mip source.");
}
bool aceEncode(const AceDocument &doc,const QImage &image,AceLevel &level,QString &error) {
    level.width=image.width(); level.height=image.height();
    level.raw=(doc.options()&16) && !(blockAce(doc) && level.width<4);
    QByteArray tight; for(int y=0;y<image.height();++y)
        tight.append(reinterpret_cast<const char *>(image.constScanLine(y)),image.width()*4);
    if(level.raw && blockAce(doc))
        return DxtCodec::encode(reinterpret_cast<const unsigned char *>(tight.constData()),tight.size(),
            image.width(),image.height(),4,static_cast<DxtCodec::Format>(doc.surface()-18),doc.hasAlpha(),level.data,error);
    if(level.raw) {
        AceWriteOptions options;
        if(doc.surface()==14) options.encoding=AceEncoding::Rgb565;
        else if(doc.surface()==16) options.encoding=AceEncoding::Argb1555;
        else if(doc.surface()==17) options.encoding=AceEncoding::Argb4444;
        else return fail(error,"Unsupported packed ACE mip encoding.");
        AceDocument encoded;
        if(!AceDocument::fromPixels(reinterpret_cast<const unsigned char *>(tight.constData()),tight.size(),
            image.width(),image.height(),4,options,encoded,error)) return false;
        level.data=encoded.levels[0].data; return true;
    }
    if(!doc.metadata.palettes.isEmpty())
        return fail(error,"Adding indexed ACE mipmaps is unsupported; existing data has not been modified.");
    // Use the original descriptor order; never replace the source header/palette/base.
    for(int y=0;y<image.height();++y) for(const auto &channel:doc.metadata.channels) {
        if(channel.id>=3 && channel.id<=6 && channel.bits==8) {
            const int c=channel.id==6?3:int(channel.id)-3;
            for(int x=0;x<image.width();++x) level.data.append(char(image.constScanLine(y)[x*4+c]));
        } else if(channel.id==2 && channel.bits==1) {
            for(int x=0;x<image.width();x+=8) {
                quint8 mask=0; for(int b=0;b<8 && x+b<image.width();++b)
                    if(image.constScanLine(y)[(x+b)*4+3]>=128) mask|=quint8(1<<(7-b));
                level.data.append(char(mask));
            }
        } else return fail(error,"Unsupported ACE channel layout for new mipmaps.");
    }
    return true;
}

Dds ddsTemplate(int width,int height,bool compressed,DxtCodec::Format format,bool alpha) {
    Dds doc; doc.compressed=compressed; doc.format=format; doc.bits=alpha?32:24;
    doc.header=QByteArray(128,'\0'); std::memcpy(doc.header.data(),"DDS ",4);
    put(doc.header,4,124); put(doc.header,8,0x1007u|(compressed?0x80000u:8u));
    put(doc.header,12,height); put(doc.header,16,width); put(doc.header,28,1);
    put(doc.header,20,compressed?quint32(DxtCodec::byteSize(width,height,format)):quint32(width*(doc.bits/8)));
    put(doc.header,76,32); put(doc.header,80,compressed?4u:(alpha?0x41u:0x40u));
    put(doc.header,108,0x1000);
    if(compressed) put(doc.header,84,0x31545844u+(quint32(format)<<24));
    else {
        put(doc.header,88,doc.bits);
        const quint32 masks[]={0xffu,0xff00u,0xff0000u,alpha?0xff000000u:0u};
        for(int c=0;c<4;++c) { doc.masks[c]=masks[c]; put(doc.header,92+c*4,masks[c]); }
    }
    return doc;
}

// Build container metadata from a tiny fixture, never recompress the real base
// merely to obtain a header. ACE's serializer derives dimensions from levels.
bool aceTemplate(bool compressed,DxtCodec::Format format,bool alpha,AceDocument &doc,QString &error,
        const QByteArray &headerTemplate) {
    AceWriteOptions options;
    options.headerTemplate=headerTemplate;
    if(compressed) {
        const AceEncoding encodings[]={alpha?AceEncoding::Dxt1Mask:AceEncoding::Dxt1,
            AceEncoding::Dxt2,AceEncoding::Dxt3,AceEncoding::Dxt4,AceEncoding::Dxt5};
        options.encoding=encodings[int(format)];
    } else options.encoding=alpha?AceEncoding::Rgba:AceEncoding::Rgb;
    const QByteArray pixels(4*4*4,char(255));
    if(!AceDocument::fromPixels(reinterpret_cast<const uchar *>(pixels.constData()),pixels.size(),
            4,4,4,options,doc,error)) return false;
    doc.levels.clear(); return true;
}

bool aceToDds(const AceDocument &source,Dds &target,QString &error) {
    target=ddsTemplate(source.levels[0].width,source.levels[0].height,blockAce(source),
        blockAce(source)?static_cast<DxtCodec::Format>(source.surface()-18):DxtCodec::Format::Dxt1,
        source.hasAlpha());
    // Cross-container conversion carries image levels, not container-specific
    // authoring resources. Blindly appending ACE resources to RGB24 DDS would
    // make its tiny-mip row layout ambiguous to readers.
    for(int m=0;m<source.levels.size();++m) {
        const auto &level=source.levels[m]; Level result;
        if(target.compressed && level.raw) {
            // Opaque ACE BC1 can use selector 3 as black where DDS means alpha
            // zero. Refuse that incompatible representation rather than alter
            // compressed blocks or silently make solid ground transparent.
            if(target.format==DxtCodec::Format::Dxt1 && !source.hasAlpha())
                for(qsizetype p=0;p<level.data.size();p+=8) {
                    const auto *block=level.data.constData()+p;
                    if(qFromLittleEndian<quint16>(block)<=qFromLittleEndian<quint16>(block+2)) {
                        const quint32 selectors=qFromLittleEndian<quint32>(block+4);
                        for(int i=0;i<16;++i) if(((selectors>>(i*2))&3)==3)
                            return fail(error,"Opaque ACE DXT1 black selectors cannot be transferred losslessly to DDS.");
                    }
                }
            result={level.width,level.height,0,level.data};
        } else {
            QImage image; if(!aceImage(source,m,image,error) || !ddsEncode(target,image,result,error)) return false;
        }
        target.levels.append(std::move(result));
    }
    return true;
}

bool ddsToAce(const Dds &source,AceDocument &target,QString &error,const QByteArray &headerTemplate={}) {
    bool alpha=source.compressed?source.format!=DxtCodec::Format::Dxt1:source.masks[3]!=0;
    if(source.compressed && source.format==DxtCodec::Format::Dxt1)
        for(const auto &level:source.levels) for(qsizetype p=0;p<level.data.size();p+=8) {
            const auto *block=level.data.constData()+p;
            if(qFromLittleEndian<quint16>(block)>qFromLittleEndian<quint16>(block+2)) continue;
            const quint32 selectors=qFromLittleEndian<quint32>(block+4);
            for(int i=0;i<16;++i) alpha|=((selectors>>(i*2))&3)==3;
        }
    if(!aceTemplate(source.compressed,source.format,alpha,target,error,headerTemplate)) return false;
    for(int m=0;m<source.levels.size();++m) {
        const auto &level=source.levels[m]; AceLevel result;
        if(source.compressed && level.width>=4)
            result={level.width,level.height,true,level.data};
        else {
            QImage image; if(!ddsImage(source,m,image,error) || !aceEncode(target,image,result,error)) return false;
        }
        target.levels.append(std::move(result));
    }
    return true;
}

bool validOptions(const TerrtexProcessing::Options &options,QString &error) {
    return (int(options.mipmaps)>=0 && int(options.mipmaps)<=2
            && int(options.output)>=0 && int(options.output)<=2)
        || fail(error,"Invalid TERRTEX processing options.");
}

bool aceLayout(int width,int height,int count,const TerrtexProcessing::Options &options,QString &error) {
    const int finalCount=options.mipmaps==TerrtexProcessing::Mipmaps::Remove?1:
        options.mipmaps==TerrtexProcessing::Mipmaps::AddMissing?fullCount(width,height):count;
    if(finalCount>1 && (width!=height || (width&(width-1)) || finalCount!=fullCount(width,height)))
        return fail(error,"ACE requires a complete square power-of-two mip chain. Choose Remove mipmaps or Add missing mipmaps for a partial chain.");
    return true;
}
}

namespace TerrtexProcessing {
QJsonObject toJson(const Options &o) { return {{"compress",o.compress},{"mipmaps",int(o.mipmaps)},{"output",int(o.output)}}; }
bool fromJson(const QJsonObject &json,Options &o) {
    if(!json["compress"].isBool() || !json["mipmaps"].isDouble() || !json["output"].isDouble()) return false;
    const int m=json["mipmaps"].toInt(-1); if(m<0 || m>2 || json["mipmaps"].toDouble()!=m) return false;
    const int f=json["output"].toInt(-1); if(f<0 || f>2 || json["output"].toDouble()!=f) return false;
    o.compress=json["compress"].toBool(); o.mipmaps=static_cast<Mipmaps>(m); o.output=static_cast<Output>(f); return true;
}
QString destination(const QString &path,Output output) {
    if(output==Output::Keep || isAce(path)==(output==Output::Ace)) return path;
    return path.left(path.size()-4)+(output==Output::Ace?".ace":".dds");
}
bool plan(const QString &path,const Options &options,QString &output,QString &error) {
    if(!validOptions(options,error)) return false;
    output=destination(path,options.output);
    QFile file(path); if(!file.open(QIODevice::ReadOnly) || file.size()>InputLimit)
        return fail(error,"Cannot read TERRTEX within the 256 MiB input budget: "+path);
    const QByteArray bytes=file.read(InputLimit+1);
    if(file.error()!=QFileDevice::NoError || bytes.size()!=file.size()) return fail(error,"Incomplete TERRTEX read.");
    if(isAce(path)) {
        AceDocument doc; if(!AceDocument::parse(bytes,doc,error,aceOptions())) return false;
        if(isAce(output) && !aceLayout(doc.levels[0].width,doc.levels[0].height,doc.levels.size(),options,error)) return false;
        if(options.mipmaps==Mipmaps::AddMissing && isAce(output)
            && (doc.levels[0].width!=doc.levels[0].height || (doc.levels[0].width&(doc.levels[0].width-1))))
            return fail(error,"ACE mipmaps require square power-of-two dimensions.");
        if(options.mipmaps==Mipmaps::AddMissing && isAce(output) && !options.compress
            && doc.levels.size()<fullCount(doc.levels[0].width,doc.levels[0].height)
            && !doc.metadata.palettes.isEmpty())
            return fail(error,"Adding indexed ACE mipmaps is unsupported; no textures have been modified.");
    } else {
        Dds doc; if(!ddsRead(bytes,doc,error)) return false;
        if(isAce(output) && !aceLayout(doc.levels[0].width,doc.levels[0].height,doc.levels.size(),options,error)) return false;
    }
    return true;
}
bool process(const QString &path,const QByteArray &input,const Options &options,Change &change,QString &error) {
    change={}; error.clear();
    if(!validOptions(options,error)) return false;
    if(input.size()>InputLimit) return fail(error,"TERRTEX input exceeds 256 MiB.");
    AceDocument ace; Dds dds; const bool sourceAce=isAce(path);
    if(sourceAce) { if(!AceDocument::parse(input,ace,error,aceOptions())) return false; }
    else if(!ddsRead(input,dds,error)) return false;
    const bool compressed=sourceAce?blockAce(ace):dds.compressed;
    const bool compress=options.compress && !compressed;
    const bool targetAce=isAce(destination(path,options.output));
    const QByteArray aceHeader=sourceAce?ace.metadata.header:QByteArray();
    const QByteArray aceResources=sourceAce?ace.metadata.trailing:QByteArray();
    const bool aceEnvelope=sourceAce && ace.compressedEnvelope;
    change.suffix=targetAce?"ace":"dds";
    const int count=sourceAce?int(ace.levels.size()):int(dds.levels.size());
    const int width=sourceAce?ace.levels[0].width:dds.levels[0].width;
    const int height=sourceAce?ace.levels[0].height:dds.levels[0].height;
    if(targetAce && !aceLayout(width,height,count,options,error)) return false;
    const bool remove=options.mipmaps==Mipmaps::Remove && count>1;
    const bool add=options.mipmaps==Mipmaps::AddMissing && count<fullCount(width,height);
    if(!compress && !remove && !add && sourceAce==targetAce) return true; // Whole file stays byte-identical.
    change.changed=true; change.compressed=compress; change.added=add; change.removed=remove;
    change.reformatted=sourceAce!=targetAce;
    if(remove) { if(sourceAce) ace.levels.resize(1); else dds.levels.resize(1); }
    if(compress) {
        Dds converted;
        for(int m=0;m<(remove?1:count);++m) {
            SaveProgress::Range encoding(0.55*double(m)/(remove?1:count),0.55*double(m+1)/(remove?1:count));
            QImage image;
            if(sourceAce ? !aceImage(ace,m,image,error) : !ddsImage(dds,m,image,error)) return false;
            QByteArray encoded;
            if(!DdsWriter::serializeDxt1(image,false,encoded,&error)) return false;
            if(m==0) {
                converted.header=encoded.left(128); converted.compressed=true;
                converted.format=DxtCodec::Format::Dxt1;
            }
            converted.levels.append({image.width(),image.height(),0,encoded.mid(128)});
        }
        dds=std::move(converted);
    }
    if(targetAce && (compress || !sourceAce)) {
        if(!ddsToAce(dds,ace,error,sourceAce?aceHeader:QByteArray())) return false;
        if(sourceAce) { ace.metadata.trailing=aceResources; ace.compressedEnvelope=aceEnvelope; }
    } else if(!targetAce && sourceAce && !compress) {
        if(!aceToDds(ace,dds,error)) return false;
    }
    QVector<QByteArray> retained;
    if(targetAce) for(const auto &level:ace.levels) retained.append(level.data);
    else for(const auto &level:dds.levels) retained.append(level.data);
    if(add) {
        SaveProgress::Range mipWork(compress?0.55:0.0,0.95);
        QImage last;
        if(targetAce) {
            if(width!=height || (width&(width-1))) return fail(error,"ACE mipmaps require square power-of-two dimensions.");
            if(!aceImage(ace,int(ace.levels.size())-1,last,error)) return false;
        } else if(!ddsImage(dds,int(dds.levels.size())-1,last,error)) return false;
        QVector<QImage> generated;
        {
            SaveProgress::Range generate(0,0.5);
            if(!TerrtexMipmaps::generateDds(last,generated,&error)) return false;
        }
        for(int m=1;m<generated.size();++m) {
            SaveProgress::Range encode(0.5+0.5*double(m-1)/(generated.size()-1),
                                      0.5+0.5*double(m)/(generated.size()-1));
            if(targetAce) {
                AceLevel level; if(!aceEncode(ace,generated[m],level,error)) return false;
                ace.levels.append(std::move(level));
            } else {
                Level level; if(!ddsEncode(dds,generated[m],level,error)) return false;
                dds.levels.append(std::move(level));
            }
        }
    }
    if(targetAce) {
        put(ace.metadata.header,4,(ace.options()&~1u)|(ace.levels.size()>1?1u:0u));
        if(!ace.serialize(change.bytes,ace.compressedEnvelope,error)) return false;
        AceDocument verified;
        if(!AceDocument::parse(change.bytes,verified,error,aceOptions())) return false;
        for(int m=0;m<retained.size();++m)
            if(m>=verified.levels.size() || verified.levels[m].data!=retained[m])
                return fail(error,"ACE base/authored-mip preservation verification failed.");
    } else {
        if(!ddsSerialize(dds,change.bytes,error)) return false;
        Dds verified;
        if(!ddsRead(change.bytes,verified,error)) return false;
        if(verified.levels.size()!=dds.levels.size()) return fail(error,"DDS mip count verification failed.");
        for(int m=0;m<dds.levels.size();++m)
            if(verified.levels[m].data!=dds.levels[m].data) return fail(error,"DDS mip layout verification failed.");
        for(int m=0;m<retained.size();++m)
            if(m>=verified.levels.size() || verified.levels[m].data!=retained[m])
                return fail(error,"DDS base/authored-mip preservation verification failed.");
    }
    if(change.bytes.size()>InputLimit) return fail(error,"Processed texture exceeds the 256 MiB file budget.");
    return true;
}
}

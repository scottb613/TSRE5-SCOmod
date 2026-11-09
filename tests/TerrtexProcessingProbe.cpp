// TSRE GenX TERRTEX processing regressions. GNU GPL v3 or later.
#include "TerrtexProcessing.h"
#include "TerrtexProduction.h"
#include "ace_codec/AceDocument.h"
#include "DdsWriter.h"
#include "DdsDecoder.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QColor>
#include <QtEndian>
#include <iostream>
#include <algorithm>

namespace {
using namespace TerrtexProcessing;
int failures=0,checks=0;
void check(bool ok,const char *message) { ++checks; if(!ok) { ++failures; std::cerr<<message<<'\n'; } }
quint32 word(const QByteArray &b,int p) { return qFromLittleEndian<quint32>(b.constData()+p); }
void put(QByteArray &b,int p,quint32 v) { qToLittleEndian(v,b.data()+p); }
QImage image(int w=8,int h=8) {
    QImage value(w,h,QImage::Format_RGBA8888);
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        uchar *p=value.scanLine(y)+x*4; p[0]=uchar(31*x); p[1]=uchar(27*y);
        p[2]=uchar(17*(x+y)); p[3]=uchar((x+y)%3?255:0);
    }
    return value;
}
QByteArray dds(int format,int count,int w=8,int h=8) {
    QByteArray b(128,'\0'); std::copy_n("DDS ",4,b.data());
    put(b,4,124); put(b,8,0x81007|(count>1?0x20000:0)); put(b,12,h); put(b,16,w);
    put(b,20,quint32(((w+3)/4)*((h+3)/4)*(format==1?8:16))); put(b,28,count);
    put(b,76,32); put(b,80,4); put(b,84,0x00545844u|(quint32('0'+format)<<24));
    put(b,108,0x1000|(count>1?0x400008:0));
    for(int m=0;m<count;++m) {
        for(int n=0;n<((w+3)/4)*((h+3)/4);++n) {
            QByteArray block(format==1?8:16,char(0x73+m)); const int c=format==1?0:8;
            qToLittleEndian<quint16>(quint16(0x1234+m*31+n),block.data()+c);
            qToLittleEndian<quint16>(quint16(0xabcd-m*19-n),block.data()+c+2);
            qToLittleEndian<quint32>(0xe4b12936u+quint32(n),block.data()+c+4); b+=block;
        }
        w=qMax(1,w/2); h=qMax(1,h/2);
    }
    return b;
}
QByteArray ddsLevel(const QByteArray &b,int wanted) {
    int w=int(word(b,16)),h=int(word(b,12)); qsizetype at=128;
    const bool compressed=(word(b,80)&4)!=0;
    const int bytes=word(b,84)==0x31545844?8:16;
    for(int m=0;m<=wanted;++m) {
        const qsizetype size=compressed?qsizetype((w+3)/4)*((h+3)/4)*bytes:qsizetype(w)*h*(word(b,88)/8);
        if(m==wanted) return b.mid(at,size);
        at+=size; w=qMax(1,w/2); h=qMax(1,h/2);
    }
    return {};
}
QByteArray ace(AceEncoding format,bool mips,bool zipped=false) {
    const QImage src=image(); AceWriteOptions options; options.encoding=format; options.mipmaps=mips;
    AceDocument doc; QString error;
    check(AceDocument::fromPixels(src.constBits(),src.sizeInBytes(),8,8,4,options,doc,error),"ACE source fixture");
    doc.metadata.header[40]=char(0x5a); doc.metadata.trailing="opaque resource fixture";
    QByteArray b; check(doc.serialize(b,zipped,error),"ACE source serialize"); return b;
}
bool write(const QString &path,const QByteArray &bytes) {
    QFile f(path); return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size();
}
QByteArray read(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray(); }
TerrtexProduction::Processor processor(const QString &root,const Options &options,int *calls=nullptr) {
    TerrtexProduction::Processor p; p.options=toJson(options);
    p.destination=[options](const QString &path) { return destination(path,options.output); };
    p.plan=[root,options](const QString &relative,QString &output,QString &error) {
        QString absoluteOutput; if(!plan(root+'/'+relative,options,absoluteOutput,error)) return false;
        output=destination(relative,options.output); return true;
    };
    p.transform=[options,calls](const QString &path,const QByteArray &bytes,TerrtexProduction::Processed &out,QString &error) {
        if(calls) ++*calls; Change change;
        if(!process(path,bytes,options,change,error)) return false;
        out.bytes=std::move(change.bytes); out.changed=change.changed; out.compressed=change.compressed;
        out.added=change.added; out.removed=change.removed; out.suffix=change.suffix; out.reformatted=change.reformatted; return true;
    }; return p;
}
void ddsTests() {
    QString error;
    for(int format : {1,2,3,4,5}) for(int count : {1,2,4}) {
        const QByteArray original=dds(format,count); Change change;
        check(process("sample.dds",original,{true,Mipmaps::Keep},change,error)&&!change.changed,
            "every already-compressed DDS skips compression, including alpha formats");
        check(process("sample.dds",original,{false,Mipmaps::Remove},change,error),"remove DDS mips");
        if(count>1) {
            check(change.removed&&!change.compressed&&word(change.bytes,28)==1,"DDS removal header/count");
            check(ddsLevel(change.bytes,0)==ddsLevel(original,0),"DDS remove retains exact base block bytes");
            check(word(change.bytes,84)==word(original,84),"DDS remove retains compression format");
        } else check(!change.changed,"base-only remove is byte-preserving no-op");
        check(process("sample.dds",original,{true,Mipmaps::AddMissing},change,error),"add missing DDS mip levels");
        if(count<4) {
            check(change.added&&!change.compressed&&word(change.bytes,28)==4,"DDS missing mip count/operation");
            for(int m=0;m<count;++m) check(ddsLevel(change.bytes,m)==ddsLevel(original,m),"DDS add preserves all authored level bytes");
            check(word(change.bytes,84)==word(original,84),"DDS add retains alpha compression format");
            if(format==3 || format==5) {
                QByteArray lower=change.bytes.left(128); put(lower,16,4); put(lower,12,4); put(lower,28,1);
                lower+=ddsLevel(change.bytes,1); DdsImage decoded;
                check(DdsDecoder::decode(lower,decoded,&error),"new alpha DDS mip decodes");
                bool alpha=false; for(qsizetype i=3;i<decoded.rgba.size();i+=4) alpha|=quint8(decoded.rgba[i])!=255;
                check(alpha,"new DXT3/5 mip alpha is not discarded");
            }
            Change again;
            check(process("sample.dds",change.bytes,{true,Mipmaps::AddMissing},again,error)&&!again.changed,
                "repeated add does not recompress any DDS level");
        } else check(!change.changed,"complete authored DDS chain left unchanged");
    }
    const QByteArray odd=dds(1,1,7,5); Change changed;
    check(process("odd.dds",odd,{false,Mipmaps::AddMissing},changed,error)
        && word(changed.bytes,28)==3 && ddsLevel(changed.bytes,0)==ddsLevel(odd,0),"odd rectangular DDS chain preserves base");
    for(bool mips : {false,true}) {
        QImage opaque=image().convertToFormat(QImage::Format_RGB888); QByteArray raw;
        check(DdsWriter::serialize24(opaque,mips,raw,&error),"uncompressed DDS source fixture");
        check(process("rgb.dds",raw,{false,Mipmaps::Keep},changed,error)&&!changed.changed,"uncompressed DDS keep no-op");
        check(process("rgb.dds",raw,{false,mips?Mipmaps::Remove:Mipmaps::AddMissing},changed,error),"uncompressed DDS mip-only processing");
        check(ddsLevel(changed.bytes,0)==ddsLevel(raw,0)&&word(changed.bytes,88)==24,"uncompressed DDS base and layout retained");
        check(process("rgb.dds",raw,{true,Mipmaps::Keep},changed,error)&&changed.compressed
            && DdsWriter::isOpaqueDxt1(changed.bytes)&&word(changed.bytes,28)==(mips?4u:1u),"compress only uncompressed DDS, preserve mip presence");
    }
    QByteArray bad=dds(1,2); bad.chop(1);
    check(!process("bad.dds",bad,{true,Mipmaps::Keep},changed,error),"truncated compressed chain rejected before skip");
    bad=dds(1,1); put(bad,84,0x30315844);
    check(!process("dx10.dds",bad,{true,Mipmaps::Keep},changed,error),"unsupported DDS profile never silently recompressed");
    // RGB24 rows with DWORD padding: base padding bytes must survive too.
    QByteArray padded(128,'\0'); std::copy_n("DDS ",4,padded.data());
    put(padded,4,124); put(padded,8,0x100f); put(padded,12,2); put(padded,16,3); put(padded,20,12);
    put(padded,28,1); put(padded,76,32); put(padded,80,0x40); put(padded,88,24);
    put(padded,92,0xff); put(padded,96,0xff00); put(padded,100,0xff0000); put(padded,108,0x1000);
    padded+=QByteArray(24,char(0x59));
    check(process("padded.dds",padded,{false,Mipmaps::AddMissing},changed,error)
        && changed.bytes.mid(128,24)==padded.mid(128,24)&&changed.bytes.size()==128+24+4,
        "padded RGB24 base rows and padding retained when adding mips");
    QByteArray rgba=padded.left(128); put(rgba,12,8); put(rgba,16,8); put(rgba,20,32);
    put(rgba,80,0x41); put(rgba,88,32); put(rgba,104,0xff000000);
    const QImage src=image(); rgba.append(reinterpret_cast<const char *>(src.constBits()),src.sizeInBytes());
    check(process("rgba.dds",rgba,{false,Mipmaps::AddMissing},changed,error)
        && ddsLevel(changed.bytes,0)==ddsLevel(rgba,0)&&word(changed.bytes,104)==0xff000000,
        "uncompressed RGBA32 base and alpha mask retained");
    QImage constant(8,8,QImage::Format_RGB888); constant.fill(QColor(35,70,105)); QByteArray tight;
    check(DdsWriter::serialize24(constant,true,tight,&error),"aligned-base RGB fixture");
    QByteArray aligned=tight.left(128);
    for(int m=0,w=8,h=8;m<4;++m,w=qMax(1,w/2),h=qMax(1,h/2)) {
        const QByteArray level=ddsLevel(tight,m); const int row=w*3,pitch=(row+3)&~3;
        for(int y=0;y<h;++y) {aligned+=level.mid(y*row,row); aligned+=QByteArray(pitch-row,char(0x5a));}
    }
    check(process("aligned.dds",aligned,{true,Mipmaps::Keep},changed,error)
        && ddsLevel(changed.bytes,2)==ddsLevel(changed.bytes,0).left(8),
        "RGB24 tiny-mip padding inferred even when base row was already aligned");
}
void aceTests() {
    QString error;
    for(auto format : {AceEncoding::Dxt1,AceEncoding::Dxt1Mask,AceEncoding::Dxt2,AceEncoding::Dxt3,AceEncoding::Dxt4,AceEncoding::Dxt5,
                       AceEncoding::Rgb,AceEncoding::Mask,AceEncoding::Rgba,AceEncoding::Rgb565,AceEncoding::Argb1555,AceEncoding::Argb4444})
      for(bool mips : {false,true}) for(bool zipped : {false,true}) {
        const QByteArray input=ace(format,mips,zipped); AceDocument before; Change change;
        check(AceDocument::parse(input,before,error),"ACE baseline parse");
        const bool compressed=before.surface()>=18;
        check(process("tile.ace",input,{false,mips?Mipmaps::Remove:Mipmaps::AddMissing},change,error),"ACE mip-only processing");
        AceDocument after;
        check(AceDocument::parse(change.bytes,after,error),"ACE mip result parse");
        if(!after.levels.isEmpty()) {
            check(after.levels[0].data==before.levels[0].data,"ACE exact base payload retained");
            check(after.surface()==before.surface()&&after.compressedEnvelope==zipped,"ACE storage/envelope retained");
            check(after.metadata.header[40]==char(0x5a)&&after.metadata.trailing==before.metadata.trailing,"ACE unknown fields/resources retained");
            check(after.levels.size()==(mips?1:4),"ACE requested mip count");
        }
        if(compressed) check(process("tile.ace",input,{true,Mipmaps::Keep},change,error)
            && !change.changed&&!change.reformatted,"already compressed ACE keeps filename and exact bytes");
        else {
            check(process("tile.ace",input,{true,Mipmaps::Keep,Output::Dds},change,error)&&change.reformatted&&change.compressed
                && DdsWriter::isOpaqueDxt1(change.bytes),"uncompressed ACE converts to explicit opaque DXT1 DDS output");
            check(word(change.bytes,28)==(mips?4u:1u),"compression keep preserves authored mip count");
            check(process("tile.ace",input,{true,Mipmaps::Keep,Output::Keep},change,error),"compress ACE in place with Keep format");
            AceDocument inPlace;
            check(AceDocument::parse(change.bytes,inPlace,error)&&inPlace.surface()==18
                &&inPlace.compressedEnvelope==zipped&&inPlace.metadata.header[40]==char(0x5a)
                &&inPlace.metadata.trailing==before.metadata.trailing&&inPlace.levels.size()==before.levels.size(),
                "same-container compression preserves envelope, unknown fields/resources and mip count");
        }
    }
    const QByteArray full=ace(AceEncoding::Dxt3,true);
    Change c; check(process("full.ace",full,{true,Mipmaps::AddMissing},c,error)&&!c.changed,"complete ACE authored chain untouched");
    Options decoded;
    check(fromJson(toJson({false,Mipmaps::Remove}),decoded)&&!decoded.compress&&decoded.mipmaps==Mipmaps::Remove,"operation settings round trip");
    check(!fromJson(QJsonObject{{"compress",true},{"mipmaps",7}},decoded),"invalid persisted operation refused");
    const QByteArray indexed=ace(AceEncoding::IndexedRgb,false);
    check(!process("indexed.ace",indexed,{false,Mipmaps::AddMissing},c,error)
        && error.contains("indexed ACE"),"unsupported new indexed mips fail instead of changing palettes/base");
}
void formatTests() {
    QString error; Change change;
    for(auto format : {AceEncoding::Rgb,AceEncoding::Mask,AceEncoding::Rgba,
            AceEncoding::Rgb565,AceEncoding::Argb1555,AceEncoding::Argb4444,
            AceEncoding::Dxt1,AceEncoding::Dxt1Mask,AceEncoding::Dxt2,AceEncoding::Dxt3,
            AceEncoding::Dxt4,AceEncoding::Dxt5}) for(bool mips : {false,true}) {
        const QByteArray original=ace(format,mips); AceDocument source;
        check(AceDocument::parse(original,source,error),"format ACE fixture parse");
        check(process("patch.ace",original,{false,Mipmaps::Keep,Output::Dds},change,error)
            && change.reformatted && !change.compressed && change.suffix=="dds","explicit ACE to DDS without compression");
        if(change.bytes.isEmpty()) continue;
        const QByteArray converted=change.bytes;
        for(int m=0;m<source.levels.size();++m) {
            const auto &level=source.levels[m];
            if(level.raw && source.surface()>=18)
                check(ddsLevel(converted,m)==level.data,"ACE to DDS retains compressed blocks exactly");
        }
        // Base pixels for uncompressed conversion must be exact, including alpha.
        if(source.surface()<18) {
            QByteArray pixels; int components=0; DdsImage decoded;
            check(source.decode(0,pixels,components,error)&&DdsDecoder::decode(converted,decoded,&error),"uncompressed conversion pixel decode");
            if(pixels.size()!=64*components || decoded.rgba.size()!=256) { check(false,"conversion pixel bounds"); continue; }
            bool same=true;
            for(int i=0;i<64;++i) for(int c=0;c<4;++c)
                same &= uchar(decoded.rgba[i*4+c])==(c<components?uchar(pixels[i*components+c]):255);
            check(same,"uncompressed ACE to DDS preserves every base pixel and alpha");
        }
        check(process("patch.dds",converted,{false,Mipmaps::Keep,Output::Ace},change,error)
            && change.reformatted && !change.compressed && change.suffix=="ace","explicit DDS back to ACE");
        AceDocument roundtrip;
        check(AceDocument::parse(change.bytes,roundtrip,error),"roundtrip ACE reparses");
        if(roundtrip.levels.size()==source.levels.size()) {
            for(int m=0;m<source.levels.size();++m) {
                if(source.levels[m].raw && source.surface()>=18)
                    check(roundtrip.levels[m].data==source.levels[m].data,"compressed roundtrip retains every block mip");
                if(source.surface()<18) {
                    QByteArray before,after; int beforeComponents=0,afterComponents=0;
                    check(source.decode(m,before,beforeComponents,error)&&roundtrip.decode(m,after,afterComponents,error)
                        &&beforeComponents==afterComponents&&before==after,"uncompressed roundtrip retains authored mip pixels and alpha");
                }
            }
        } else check(false,"roundtrip retains requested mip count");
        check(process("patch.ace",original,{true,Mipmaps::Remove,Output::Ace},change,error),"compression can target ACE");
        AceDocument compressed;
        check(AceDocument::parse(change.bytes.isEmpty()?original:change.bytes,compressed,error)
            && compressed.surface()>=18 && compressed.levels.size()==1 && !change.reformatted,
            "Keep/ACE compression never silently creates DDS");
    }
    for(int format : {1,2,3,4,5}) for(Mipmaps operation : {Mipmaps::Keep,Mipmaps::Remove,Mipmaps::AddMissing}) {
        const QByteArray original=dds(format,2); // Partial authored DDS chain.
        const bool ok=process("partial.dds",original,{false,operation,Output::Ace},change,error);
        if(operation==Mipmaps::Keep) check(!ok,"partial ACE chain cannot be silently completed by Keep");
        else {
            check(ok,"partial DDS converts with explicit mip operation"); AceDocument result;
            check(AceDocument::parse(change.bytes,result,error)&&result.levels.size()==(operation==Mipmaps::Remove?1:4),
                "ACE output has exactly requested supported mip chain");
            if(!result.levels.isEmpty()) check(result.levels[0].data==ddsLevel(original,0),"DDS to ACE preserves compressed base");
        }
    }
    QImage rgb=image().convertToFormat(QImage::Format_RGB888); QByteArray raw;
    check(DdsWriter::serialize24(rgb,false,raw,&error),"raw DDS conversion fixture");
    for(Output output : {Output::Keep,Output::Ace,Output::Dds}) for(bool compress : {false,true})
      for(Mipmaps operation : {Mipmaps::Keep,Mipmaps::Remove,Mipmaps::AddMissing}) {
        check(process("rgb.dds",raw,{compress,operation,output},change,error),"independent format/compression/mipmap matrix");
        check(change.compressed==compress && change.reformatted==(output==Output::Ace),"independent processing operations reported");
        if(output==Output::Ace) {
            AceDocument result; check(AceDocument::parse(change.bytes,result,error)
                && (result.surface()>=18)==compress,"ACE output respects compression choice");
        } else if(compress) check(DdsWriter::isOpaqueDxt1(change.bytes),"DDS output respects compression choice");
    }
    Options options;
    check(fromJson(toJson({false,Mipmaps::AddMissing,Output::Ace}),options)
        && options.output==Output::Ace,"format settings round trip");
    check(!fromJson(QJsonObject{{"compress",false},{"mipmaps",0}},options),"older incomplete options are not silently reinterpreted");
    check(!fromJson(QJsonObject{{"compress",false},{"mipmaps",0},{"output",8}},options),"invalid output format refused");
    const QByteArray rawAce=ace(AceEncoding::Rgb,false);
    check(process("keep.ace",rawAce,{true,Mipmaps::Keep,Output::Keep},change,error)
        && change.suffix=="ace" && change.compressed && !change.reformatted,"Keep format permits ACE compression in place");
}
void reverseRouteTests() {
    QTemporaryDir temp; check(temp.isValid(),"reverse route temporary folder");
    const QString root=temp.path()+"/terrtex",journal=temp.path()+"/process.json";
    check(QDir().mkpath(root+"/snow"),"reverse seasonal folder");
    const QByteArray original=dds(5,4);
    check(write(root+"/tile.dds",original)&&write(root+"/snow/tile.dds",original),"reverse seasonal fixtures");
    auto policy=processor(root,{true,Mipmaps::Keep,Output::Ace});
    TerrtexProduction::Result result; QString error; int calls=0;
    auto references=[&](const QMap<QString,QString> &names,QString &failure) {
        check(names.value("tile.dds")=="tile.ace" && names.value("snow/tile.dds")=="snow/tile.ace",
            "DDS to ACE remaps main and seasonal names");
        check(QFile::exists(root+"/tile.dds")&&QFile::exists(root+"/snow/tile.dds"),"DDS originals retained before reference publication");
        if(++calls==1) {failure="injected reference stop"; return false;} return true;
    };
    check(!TerrtexProduction::run(root,journal,{},references,result,error,&policy),"reverse reference failure preserves originals");
    auto wrong=processor(root,{true,Mipmaps::Keep,Output::Dds});
    check(!TerrtexProduction::run(root,journal,{},references,result,error,&wrong),"resume cannot change output format");
    check(TerrtexProduction::run(root,journal,{},references,result,error,&policy),"reverse processing resumes");
    check(!QFile::exists(root+"/tile.dds")&&!QFile::exists(root+"/snow/tile.dds")
        &&QFile::exists(root+"/tile.ace")&&QFile::exists(root+"/snow/tile.ace"),"successful reverse conversion leaves no old DDS originals");
    AceDocument resultAce;
    check(AceDocument::parse(read(root+"/tile.ace"),resultAce,error)
        &&resultAce.levels[0].data==ddsLevel(original,0),"reverse production does not recompress DXT5 base");
    check(TerrtexProduction::run(root,journal,{}, {},result,error,&policy)&&result.converted==0,"repeat ACE conversion is a no-op");
    const QString collide=temp.path()+"/collision";
    check(QDir().mkpath(collide)&&write(collide+"/tile.dds",original)
        &&write(collide+"/tile.ace",ace(AceEncoding::Rgb,false)),"reverse collision fixture");
    auto collision=processor(collide,{false,Mipmaps::Keep,Output::Ace});
    check(!TerrtexProduction::run(collide,temp.path()+"/collision.json",{}, {},result,error,&collision)
        &&read(collide+"/tile.dds")==original,"reverse collision refuses overwrite and deletion");

    auto record=QJsonDocument::fromJson(read(journal)).object(); record["version"]=2;
    check(write(journal,QJsonDocument(record).toJson()),"older completed checkpoint fixture");
    check(TerrtexProduction::run(root,journal,{}, {},result,error,&policy),"completed version-2 processing can start a new version-3 pass");
    record=QJsonDocument::fromJson(read(journal)).object(); record["version"]=2; record["complete"]=false;
    check(write(journal,QJsonDocument(record).toJson()),"older incomplete checkpoint fixture");
    const QByteArray before=read(root+"/tile.ace");
    check(!TerrtexProduction::run(root,journal,{}, {},result,error,&policy)
        &&read(root+"/tile.ace")==before,"unfinished older operation is refused without reinterpretation");

    const QString verifyRoot=temp.path()+"/verify";
    check(QDir().mkpath(verifyRoot)&&write(verifyRoot+"/a.dds",original)
        &&write(verifyRoot+"/b.dds",original),"cleanup verification fixtures");
    auto verifyPolicy=processor(verifyRoot,{false,Mipmaps::Keep,Output::Ace});
    const auto tamper=[&](const QMap<QString,QString> &,QString &) {
        return write(verifyRoot+"/b.ace","external mutation during reference update");
    };
    check(!TerrtexProduction::run(verifyRoot,temp.path()+"/verify.json",{},tamper,result,error,&verifyPolicy)
        &&read(verifyRoot+"/a.dds")==original&&read(verifyRoot+"/b.dds")==original,
        "all originals retained if any replacement fails final verification");
}
void changedSourceResumeTests() {
    QTemporaryDir temp; check(temp.isValid(),"changed source resume temporary folder");
    for(Output output : {Output::Ace,Output::Dds}) {
        const QString sourceSuffix=output==Output::Ace?".dds":".ace";
        const QString outputSuffix=output==Output::Ace?".ace":".dds";
        const QString root=temp.path()+"/case"+sourceSuffix+"/terrtex";
        const QString journal=temp.path()+"/case"+sourceSuffix+"/process.json";
        const QString refs=temp.path()+"/case"+sourceSuffix+"/terrain-refs.txt";
        const QByteArray original=output==Output::Ace?dds(5,4):ace(AceEncoding::Rgb,true);
        const QByteArray edited=output==Output::Ace?dds(3,4):ace(AceEncoding::Rgba,true);
        check(original!=edited,"edited source fixture differs");
        check(QDir().mkpath(root+"/snow")&&write(root+"/a"+sourceSuffix,original)
            &&write(root+"/snow/b"+sourceSuffix,original)&&write(refs,"original references"),
            "main/season changed-source fixtures");
        int references=0,transforms=0;
        auto policy=processor(root,{false,Mipmaps::Keep,output},&transforms);
        auto publish=[&](const QMap<QString,QString> &,QString &error) {
            if(++references==1) { error="injected reference interruption"; return false; }
            return write(refs,"updated references");
        };
        TerrtexProduction::Result result; QString error;
        check(!TerrtexProduction::run(root,journal,{},publish,result,error,&policy)
            &&references==1,"interrupted conversion leaves original references");
        const QByteArray firstOutput=read(root+"/a"+outputSuffix);
        const QByteArray seasonalOutput=read(root+"/snow/b"+outputSuffix);
        check(!firstOutput.isEmpty()&&!seasonalOutput.isEmpty(),"converted outputs ready before interruption");
        check(write(root+"/snow/b"+sourceSuffix,edited),"external seasonal source edit");
        const int previousTransforms=transforms;
        check(!TerrtexProduction::run(root,journal,{},publish,result,error,&policy)
            &&error.contains("original changed before reference publication"),"resume rejects edited original before publication");
        check(references==1&&transforms==previousTransforms
            &&read(refs)=="original references","blocked resume never calls reference updater or re-encodes outputs");
        check(read(root+"/a"+sourceSuffix)==original&&read(root+"/snow/b"+sourceSuffix)==edited,
            "blocked resume preserves all originals including external edits");
        check(read(root+"/a"+outputSuffix)==firstOutput&&read(root+"/snow/b"+outputSuffix)==seasonalOutput,
            "blocked resume leaves verified converted outputs unchanged");
        check(write(root+"/snow/b"+sourceSuffix,original),"restore original fixture for valid resume");
        check(TerrtexProduction::run(root,journal,{},publish,result,error,&policy)
            &&references==2&&read(refs)=="updated references","valid restored originals still allow resume");
        check(!QFile::exists(root+"/a"+sourceSuffix)&&!QFile::exists(root+"/snow/b"+sourceSuffix),
            "valid resume cleans originals only after successful references");
    }
}
void routeTests() {
    QTemporaryDir temp; check(temp.isValid(),"temporary route available");
    const QString root=temp.path()+"/terrtex",journal=temp.path()+"/process.json";
    check(QDir().mkpath(root+"/snow"),"seasonal fixture folder");
    const QByteArray raw=ace(AceEncoding::Rgb,false),bc3=dds(3,1),aceBlocks=ace(AceEncoding::Dxt1Mask,false);
    check(write(root+"/tile.ace",raw)&&write(root+"/snow/tile.ace",raw)&&write(root+"/alpha.dds",bc3)
        &&write(root+"/keep.dds",dds(1,1)),"mixed main/season/compressed route fixtures");
    Options options{true,Mipmaps::AddMissing,Output::Dds}; int transforms=0,referenceCalls=0;
    auto policy=processor(root,options,&transforms);
    auto references=[&](const QMap<QString,QString> &names,QString &error) {
        check(names.value("tile.ace")=="tile.dds"&&!names.contains("keep.ace"),"only actually converted ACE references remap");
        check(QFile::exists(root+"/tile.ace")&&QFile::exists(root+"/snow/tile.ace"),"ACE originals retained until all references publish");
        if(++referenceCalls==1) {error="injected descriptor stop";return false;} return true;
    };
    TerrtexProduction::Result result; QString error;
    check(!TerrtexProduction::run(root,journal,{},references,result,error,&policy),"interrupted processing checkpoint retained");
    const QByteArray alpha=read(root+"/alpha.dds"),converted=read(root+"/tile.dds");
    check(ddsLevel(alpha,0)==ddsLevel(bc3,0),"route operation preserves compressed alpha base bytes");
    auto wrong=processor(root,{true,Mipmaps::Remove});
    check(!TerrtexProduction::run(root,journal,{},references,result,error,&wrong)
        &&error.contains("original processing options"),"interrupted options cannot change");
    const int beforeResume=transforms;
    QFile tail(journal+".log"); check(tail.open(QIODevice::Append)&&tail.write("torn")==4,"torn checkpoint fixture"); tail.close();
    check(TerrtexProduction::run(root,journal,{},references,result,error,&policy)&&transforms==beforeResume,"resume never re-encodes completed textures");
    check(read(root+"/tile.dds")==converted&&!QFile::exists(root+"/tile.ace")
        &&QFile::exists(root+"/keep.dds"),"cleanup deletes only converted uncompressed ACE");
    auto keepPolicy=processor(root,{true,Mipmaps::Keep});
    check(TerrtexProduction::run(root,journal,{}, {},result,error,&keepPolicy)
        &&result.converted==0&&result.skipped==4,"completed processing is safely repeatable without recompression");
    auto removePolicy=processor(root,{false,Mipmaps::Remove});
    check(TerrtexProduction::run(root,journal,{}, {},result,error,&removePolicy)
        &&result.compressed==0&&result.removed==4,"new mip operation allowed after completed run");
    check(ddsLevel(read(root+"/alpha.dds"),0)==ddsLevel(bc3,0),"repeated operations preserve original BC3 base");
    auto addPolicy=processor(root,{false,Mipmaps::AddMissing});
    check(TerrtexProduction::run(root,journal,{}, {},result,error,&addPolicy)&&result.added==4,"mip regeneration does not require compression");
    check(ddsLevel(read(root+"/alpha.dds"),0)==ddsLevel(bc3,0),"add/remove/add never accumulates base loss");
    const QString collision=temp.path()+"/collision"; check(QDir().mkpath(collision),"collision folder");
    check(write(collision+"/tile.ace",raw)&&write(collision+"/tile.dds",bc3),"collision files");
    auto collide=processor(collision,options);
    check(!TerrtexProduction::run(collision,temp.path()+"/collision.json",{}, {},result,error,&collide)
        &&read(collision+"/tile.ace")==raw&&read(collision+"/tile.dds")==bc3,"destination collision fails before any texture changes");
    const QString coexist=temp.path()+"/coexist"; check(QDir().mkpath(coexist),"compressed ACE sibling folder");
    check(write(coexist+"/tile.ace",aceBlocks)&&write(coexist+"/tile.dds",bc3),"compressed ACE sibling fixtures");
    auto coexistPolicy=processor(coexist,{true,Mipmaps::Keep});
    check(TerrtexProduction::run(coexist,temp.path()+"/coexist.json",{}, {},result,error,&coexistPolicy)
        &&result.converted==0&&read(coexist+"/tile.ace")==aceBlocks&&read(coexist+"/tile.dds")==bc3,
        "already compressed ACE is not renamed or deleted when DDS sibling exists");
    const QString tampered=temp.path()+"/tampered",tamperedJournal=temp.path()+"/tampered.json";
    check(QDir().mkpath(tampered)&&write(tampered+"/rgb.dds",dds(3,1))&&write(tampered+"/rename.ace",raw),"tamper fixtures");
    auto tamperPolicy=processor(tampered,options);
    const auto stop=[](const QMap<QString,QString> &,QString &failure){failure="injected reference stop";return false;};
    check(!TerrtexProduction::run(tampered,tamperedJournal,{},stop,result,error,&tamperPolicy),"tamper checkpoint ready");
    check(write(tampered+"/rgb.dds","external change"),"external DDS edit");
    check(!TerrtexProduction::run(tampered,tamperedJournal,{},stop,result,error,&tamperPolicy)
        &&read(tampered+"/rgb.dds")=="external change","resume does not overwrite externally changed completed output");
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv); ddsTests(); aceTests(); formatTests(); routeTests(); reverseRouteTests(); changedSourceResumeTests();
    std::cout<<"TERRTEX processing checks="<<checks<<" failures="<<failures<<'\n'; return failures?1:0;
}

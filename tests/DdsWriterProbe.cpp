// Native RGB24 DDS layout, round-trip and atomic-save coverage. GPL v3 or later.
#include "DdsWriter.h"
#include "DdsDecoder.h"
#include "TerrtexMipmaps.h"
#include "TerrainTextureFormat.h"
#include "RouteSaveTransaction.h"
#include "TerrtexProduction.h"
#include <QtEndian>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

int main() {
    int failures=0;
    auto check=[&](bool ok,const char *message){
        if(!ok) { ++failures; std::cerr << message << '\n'; }
    };
    // Native DDS output preserves each season's own full-resolution source.
    for(int size : {1024,2048}) {
        QImage source(size,size,QImage::Format_RGB888);
        source.fill(QColor(35,70,105));
        QByteArray bytes; QString error;
        check(DdsWriter::serialize24(source,false,bytes,&error),"full-size RGB24 save");
        if(bytes.size()>=128) {
            check(qFromLittleEndian<quint32>(bytes.constData()+12)==quint32(size)
                && qFromLittleEndian<quint32>(bytes.constData()+16)==quint32(size),
                "DDS base dimensions equal original source");
            check(bytes.size()==128+size*size*3,"full-resolution RGB24 byte count");
        } else check(false,"full-size DDS payload exists");
    }
    QByteArray lastDds;
    for(bool mips : {false,true}) {
        for(int size : {2,8,32}) {
            QImage image(size,size,QImage::Format_RGBA8888);
            for(int y=0;y<size;++y) for(int x=0;x<size;++x)
                image.setPixelColor(x,y,QColor((x*19+y*3)%256,(x*7+y*11)%256,(x*13+y*17)%256,255));
            QByteArray dds; QString error;
            check(DdsWriter::serialize24(image,mips,dds,&error),"serialize RGB24 DDS");
            if(dds.size()<128) continue;
            auto word=[&](int offset){ return qFromLittleEndian<quint32>(dds.constData()+offset); };
            check(dds.left(4)=="DDS " && word(4)==124 && word(76)==32,"DDS magic and header sizes");
            check(word(12)==quint32(size) && word(16)==quint32(size) && word(20)==quint32(size*3),"dimensions and tight RGB24 pitch");
            check(word(80)==0x40 && word(84)==0 && word(88)==24,"RGB24 without FourCC/compression");
            check(word(92)==0xff0000 && word(96)==0xff00 && word(100)==0xff && word(104)==0,"BGR byte order and RGB masks");
            QVector<QImage> levels;
            if(mips) check(TerrtexMipmaps::generate(image,levels),"generate expected mip pixels");
            else levels.append(image);
            check(word(28)==quint32(levels.size()),"complete mip count");
            check(bool(word(8)&0x20000)==mips && bool(word(108)&0x400000)==mips,"mipmap header and caps flags");
            int offset=128;
            for(const QImage &level : levels) {
                for(int y=0;y<level.height();++y) for(int x=0;x<level.width();++x) {
                    const QColor pixel=level.pixelColor(x,y);
                    if(offset+3>dds.size()) { check(false,"truncated mip payload"); break; }
                    check(uchar(dds[offset])==pixel.blue() && uchar(dds[offset+1])==pixel.green()
                        && uchar(dds[offset+2])==pixel.red(),"exact RGB pixels in every mip");
                    offset+=3;
                }
            }
            check(offset==dds.size(),"exact payload byte count including tiny mips");
            DdsImage decoded;
            check(DdsDecoder::decode(dds,decoded,&error),"existing native DDS reader round trip");
            check(decoded.opaqueRgb24,"RGB24 reload remains opaque without synthetic alpha borders");
            if(decoded.rgba.size()==size*size*4) {
                for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
                    const QColor pixel=image.pixelColor(x,y); const int at=(y*size+x)*4;
                    check(uchar(decoded.rgba[at])==pixel.red() && uchar(decoded.rgba[at+1])==pixel.green()
                        && uchar(decoded.rgba[at+2])==pixel.blue() && uchar(decoded.rgba[at+3])==255,"reader preserves base RGB exactly");
                }
            } else check(false,"decoded base byte count");
            lastDds=dds;
        }
    }
    QImage odd(3,2,QImage::Format_RGB888); odd.fill(QColor(30,60,90));
    QByteArray dds; QString error;
    check(DdsWriter::serialize24(odd,false,dds,&error) && dds.size()==146,"nonmip odd-width tight rows");
    DdsImage decoded;
    check(DdsDecoder::decode(dds,decoded,&error) && decoded.width==3 && decoded.height==2,"odd-width native decode");
    check(!DdsWriter::serialize24(odd,true,dds,&error) && dds.isEmpty(),"unsupported mip dimensions do not resize");
    QImage alpha(2,2,QImage::Format_RGBA8888); alpha.fill(Qt::transparent);
    check(!DdsWriter::serialize24(alpha,false,dds,&error) && dds.isEmpty(),"RGB24 does not silently discard transparency");

    QTemporaryDir temporary;
    check(temporary.isValid(),"temporary route fixture");
    if(!temporary.isValid()) return 1;
    const QString root=temporary.path();
    const QString oldAce=root+"/patch.ace";
    const QString newDds=TerrainTextureFormat::outputPath(oldAce,true);
    check(newDds==root+"/patch_dds24.dds","distinct DDS basename avoids ACE shadowing");
    check(TerrainTextureFormat::outputPath(newDds,false)==root+"/patch_ace.ace","return to ACE without stale DDS shadow");
    check(TerrainTextureFormat::isPaintedPatchName("tile_2_3_dds24.dds","tile_2_3.ace"),"DDS patch repaint recognition");
    auto write=[](const QString &path,const QByteArray &data){ QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(data)==data.size(); };
    auto read=[](const QString &path){ QFile f(path); if(!f.open(QIODevice::ReadOnly)) return QByteArray(); return f.readAll(); };
    // Production BC1: independent header/payload checks, opaque decode and tiny mips.
    for(bool mips:{false,true}) for(int size:{2,8,32}) {
        QImage source(size,size,QImage::Format_RGBA8888); source.fill(QColor(40,100,180,0));
        QByteArray compressed;
        check(DdsWriter::serializeDxt1(source,mips,compressed,&error),"opaque DXT1 production encoding");
        if(compressed.size()<128) { check(false,"DXT1 header exists"); continue; }
        auto word=[&](int n){ return qFromLittleEndian<quint32>(compressed.constData()+n); };
        check(word(80)==4 && word(84)==0x31545844 && word(104)==0,"DXT1 FourCC without alpha flag/mask");
        check(word(12)==quint32(size) && word(16)==quint32(size),"DXT1 never resizes base pixels");
        check(word(20)==quint32(qMax(1,(size+3)/4)*qMax(1,(size+3)/4)*8),"DXT1 base linear size");
        int at=128, width=size, height=size;
        for(quint32 level=0;level<word(28);++level) {
            const int length=qMax(1,(width+3)/4)*qMax(1,(height+3)/4)*8;
            check(at+length<=compressed.size(),"complete block payload for every mip");
            for(int block=at;block+8<=at+length && block+8<=compressed.size();block+=8)
                check(qFromLittleEndian<quint16>(compressed.constData()+block)
                    >qFromLittleEndian<quint16>(compressed.constData()+block+2),"every BC1 block forces four opaque colors");
            at+=length; width=qMax(1,width/2); height=qMax(1,height/2);
        }
        check(at==compressed.size() && DdsWriter::isOpaqueDxt1(compressed),"exact DXT1 chain size including 1x1 mip");
        DdsImage roundTrip;
        check(DdsDecoder::decode(compressed,roundTrip,&error) && roundTrip.opaqueDxt1,"DXT1 decoder identifies truly opaque data");
        for(int i=0;i+3<roundTrip.rgba.size();i+=4) {
            check(uchar(roundTrip.rgba[i+3])==255,"production conversion removes alpha");
            check(qAbs(int(uchar(roundTrip.rgba[i]))-40)<=8
                && qAbs(int(uchar(roundTrip.rgba[i+1]))-100)<=4
                && qAbs(int(uchar(roundTrip.rgba[i+2]))-180)<=8,"constant color stays within RGB565 quantization bounds");
        }
        // A valid transparent BC1 block must still retain RGBA decode storage.
        QByteArray transparent=compressed;
        for(int i=128;i<transparent.size();i+=8) {
            for(int j=0;j<4;++j) transparent[i+j]=0;
            for(int j=4;j<8;++j) transparent[i+j]=char(255);
        }
        check(DdsDecoder::decode(transparent,roundTrip,&error) && !roundTrip.opaqueDxt1,
            "transparent DXT1 remains distinguishable from opaque DXT1");
    }
    for(QColor color:{QColor(Qt::black),QColor(Qt::white)}) {
        QImage solid(3,2,QImage::Format_RGB888); solid.fill(color);
        QByteArray compressed; DdsImage roundTrip;
        check(DdsWriter::serializeDxt1(solid,false,compressed,&error) && compressed.size()==136
            && DdsDecoder::decode(compressed,roundTrip,&error),"odd-size constant BC1 block round trip");
        for(int i=0;i+3<roundTrip.rgba.size();i+=4)
            check(uchar(roundTrip.rgba[i])==color.red() && uchar(roundTrip.rgba[i+1])==color.green()
                && uchar(roundTrip.rgba[i+2])==color.blue() && uchar(roundTrip.rgba[i+3])==255,
                "black/white endpoint degeneracy does not introduce transparency");
    }
    for(const QSize dimensions:{QSize(3,7),QSize(1,8),QSize(8,1)}) {
        QImage source(dimensions,QImage::Format_RGB888); source.fill(QColor(40,100,180));
        QByteArray compressed; DdsImage roundTrip;
        check(DdsWriter::serializeDxt1(source,true,compressed,&error)
            && DdsWriter::isOpaqueDxt1(compressed)
            && DdsDecoder::decode(compressed,roundTrip,&error)
            && roundTrip.width==dimensions.width() && roundTrip.height==dimensions.height(),
            "rectangular/odd DDS mip chains retain full base dimensions");
        check(compressed.size()>=128 && qFromLittleEndian<quint32>(compressed.constData()+28)>1,
            "rectangular DDS includes a full mip chain");
    }
    {
        const QString textures=root+"/production/TERRTEX", journal=root+"/production/state.json";
        check(QDir().mkpath(textures+"/snow"),"production seasonal fixture");
        check(write(textures+"/tile.ace","opaque ACE fixture")
            && write(textures+"/snow/tile.ace","snow ACE fixture")
            && write(textures+"/tile.dds",lastDds),"ACE/DDS basename collision fixture");
        int aceReads=0, referenceCalls=0;
        const auto ace=[&](const QString &,QImage &image,bool &mipmaps,QString &) {
            ++aceReads; image=QImage(8,8,QImage::Format_RGB888); image.fill(QColor(30,90,150)); mipmaps=true; return true;
        };
        const auto references=[&](const QMap<QString,QString> &names,QString &failure) {
            ++referenceCalls;
            check(names.value("tile.ace")=="tile_ace_dxt1.dds"
                && names.value("snow/tile.ace")=="snow/tile_ace_dxt1.dds","root and seasonal references remap consistently");
            check(QFile::exists(textures+"/tile.ace") && QFile::exists(textures+"/snow/tile.ace"),
                "ACE originals remain until every descriptor is published");
            if(referenceCalls==1) { failure="injected descriptor write failure"; return false; }
            return true;
        };
        TerrtexProduction::Result result;
        check(!TerrtexProduction::run(textures,journal,ace,references,result,error),"failed reference publication stops production cleanup");
        const QByteArray mainHash=TerrtexProduction::hashFile(textures+"/tile.dds");
        const int readsBeforeResume=aceReads;
        QFile tail(journal+".log");
        check(tail.open(QIODevice::Append) && tail.write("interrupted checkpoint")==22,"interrupted checkpoint fixture"); tail.close();
        double last=0; int progressReports=0;
        {
            SaveProgress::Range all(0,1);
            SaveProgress::Scope progress([&](const QString &){
                check(SaveProgress::fraction()>=last && SaveProgress::fraction()<1,"production progress monotonic and reserves full completion");
                last=SaveProgress::fraction(); ++progressReports;
            });
            check(TerrtexProduction::run(textures,journal,ace,references,result,error),"resume after descriptor failure and torn checkpoint tail");
        }
        check(progressReports>0 && aceReads==readsBeforeResume,"resume verifies hashes without recompressing lossy outputs");
        check(mainHash==TerrtexProduction::hashFile(textures+"/tile.dds"),"resumed DDS bytes are unchanged");
        check(!QFile::exists(textures+"/tile.ace") && !QFile::exists(textures+"/snow/tile.ace")
            && QFile::exists(textures+"/tile_ace_dxt1.dds") && QFile::exists(textures+"/snow/tile_ace_dxt1.dds"),
            "successful production leaves no ACE originals or backups");
        check(!TerrtexProduction::run(textures,journal,ace,references,result,error)
            && error.contains("only be run once"),"completed conversion cannot be run a second time");
        check(mainHash==TerrtexProduction::hashFile(textures+"/tile.dds"),"rerun refusal preserves production bytes");
    }
    {
        const QString textures=root+"/collision";
        check(QDir().mkpath(textures) && write(textures+"/source.ace","ACE")
            && write(textures+"/source_ace_dxt1.dds",lastDds),"reserved-name collision fixture");
        TerrtexProduction::Result result;
        check(!TerrtexProduction::run(textures,root+"/collision.json",{}, {},result,error)
            && read(textures+"/source.ace")=="ACE"
            && read(textures+"/source_ace_dxt1.dds")==lastDds,"destination collision is refused before any texture changes");
    }
    {
        const QString textures=root+"/tampered", journal=root+"/tampered.json";
        check(QDir().mkpath(textures) && write(textures+"/tile.dds",lastDds),"tampered output fixture");
        TerrtexProduction::Result result;
        const auto stop=[](const QMap<QString,QString> &,QString &failure) { failure="injected stop"; return false; };
        check(!TerrtexProduction::run(textures,journal,{},stop,result,error),"checkpoint after in-place DDS conversion");
        check(write(textures+"/tile.dds","external change"),"replace converted DDS externally");
        check(!TerrtexProduction::run(textures,journal,{},stop,result,error)
            && error.contains("has changed") && read(textures+"/tile.dds")=="external change",
            "resume refuses to overwrite an externally changed completed output");
    }
    check(write(root+"/patch.dds","existing DDS"),"shadow fixture");
    check(TerrainTextureFormat::outputPath(oldAce,false)==root+"/patch_ace.ace","external DDS cannot shadow requested ACE output");
    check(write(root+"/tile.t","old ACE reference"),"old terrain descriptor");
    {
        RouteSaveTransaction transaction(root,root+"/backups");
        check(transaction.addFile(newDds,lastDds,&error)
            && transaction.addFile(root+"/tile.t","new DDS reference",&error)
            && transaction.commit(&error),"publish DDS and terrain reference together");
    }
    check(read(newDds)==lastDds && read(root+"/tile.t")=="new DDS reference","published bytes and reference");
    check(write(root+"/blocked","not a directory"),"failure fixture");
    {
        RouteSaveTransaction transaction(root,root+"/backups");
        check(transaction.addFile(newDds,"replacement",&error)
            && transaction.addFile(root+"/tile.t","replacement reference",&error)
            && transaction.addFile(root+"/blocked/fail.dds","fail",&error),"stage failure case");
        check(!transaction.commit(&error),"blocked output stops publication");
    }
    check(read(newDds)==lastDds && read(root+"/tile.t")=="new DDS reference","failure restores texture and descriptor");
    return failures ? 1 : 0;
}

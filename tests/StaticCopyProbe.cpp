// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "StaticCopy.h"
#include "RouteDistro.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QTemporaryDir>
#include <QStringConverter>
#include <QtEndian>
#include <cstdlib>
#include <iostream>

static void require(bool condition,const char *message) {
    if(!condition) { std::cerr<<message<<'\n'; std::exit(1); }
}
static void put(const QString &root,const QString &relative,const QByteArray &bytes) {
    const QString path=QDir(root).filePath(relative);
    require(QDir().mkpath(QFileInfo(path).absolutePath()),"fixture directory");
    QFile file(path); require(file.open(QIODevice::WriteOnly),"fixture open"); require(file.write(bytes)==bytes.size(),"fixture write");
}
static QByteArray get(const QString &root,const QString &relative) {
    QFile file(QDir(root).filePath(relative)); require(file.open(QIODevice::ReadOnly),"fixture read"); return file.readAll();
}
static QByteArray unicode(const QString &text) {
    QStringEncoder encoder(QStringEncoder::Utf16LE); return QByteArray::fromHex("fffe")+encoder(text);
}
static void route(const QString &root,const QString &stem,bool ref=true) {
    put(root,stem+".trk",unicode("SIMISA@@@@@@@@@@JINX0r1t______\r\nTr_RouteFile ( FileName ( "+StaticCopy::quote(stem)+" ) Name ( \"Test route\" ) )"));
    if(ref) put(root,stem+".ref",unicode("SIMISA@@@@@@@@@@JINX0r1t______\r\nComment ( \"preserve this text exactly\" )\r\nStatic ( Class ( Existing ) Filename ( existing.s ) Description ( Existing ) )\r\n"));
}
static StaticCopy::Source fixture(const QString &root) {
    route(root,"Source");
    put(root,"SHAPES/Fence.s",unicode("SIMISA@@@@@@@@@@JINX0s1t______\r\nshape ( images ( 1 image ( \"Rusty metal.ace\" ) ) )"));
    put(root,"SHAPES/Fence.sd",unicode("Shape ( OldFilename.s ESD_Detail_Level ( 2 ) ESD_Alternative_Texture ( 257 ) ESD_Snapable ( ) )"));
    put(root,"TEXTURES/Rusty metal.ace","summer ACE"); put(root,"TEXTURES/Rusty metal.dds","summer DDS");
    put(root,"TEXTURES/SNOW/Rusty metal.ace","snow ACE"); put(root,"TEXTURES/NIGHT/Rusty metal.dds","night DDS");
    StaticCopy::Source source; source.routeRoot=root; source.shapeRoot=root+"/SHAPES"; source.textureRoot=root+"/TEXTURES";
    source.shapes={"Fence.s"}; source.settings="Align ( \"None\" )\r\nRandomRotY ( -2 2 )\r\nTSREStaticFlags ( \"4294967295\" )\r\nTSREStaticDetailLevel ( \"-1\" )\r\nTSRECollideFlags ( \"32\" )\r\nTSRECollideFunction ( \"1\" )\r\n";
    return source;
}
static QByteArray binaryShape(const QString &texture) {
    QByteArray payload(1,'\0'); QDataStream data(&payload,QIODevice::Append); data.setByteOrder(QDataStream::LittleEndian);
    data<<quint16(texture.size()); for(QChar c:texture) data<<quint16(c.unicode());
    QByteArray file("SIMISA@@@@@@@@@@JINX0s1b______\r\n",32); QDataStream out(&file,QIODevice::Append); out.setByteOrder(QDataStream::LittleEndian);
    out<<quint32(71)<<quint32(payload.size()); out.writeRawData(payload.constData(),payload.size()); return file;
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination");
        const auto source=fixture(src); route(dst,"Destination");
        put(dst,"TEXTURES/Rusty metal.ace","summer ACE");
        const QByteArray originalRef=get(dst,"Destination.ref");
        const auto destination=StaticCopy::validateDestination(dst,true);
        require(destination.valid && destination.categories.contains("Existing"),"destination validation/category discovery");
        const auto plan=StaticCopy::prepare(source,destination);
        if(!plan.valid) std::cerr<<plan.errors.join('\n').toStdString()<<'\n';
        require(plan.valid && plan.files.size()==6,"complete model/SD/base/season/night/format resource graph");
        double last=0; bool ended=false;
        const auto result=StaticCopy::transfer(plan,QStringLiteral("Zaun \u00e4"),"Fences",[&](double value,const QString &){
            require(value+1e-9>=last,"transfer progress moved backward"); last=value;
            if(value>=1) { require(get(dst,"Destination.ref").size()>originalRef.size(),"100 percent preceded REF commit"); ended=true; }
            return true;
        });
        if(!result.refUpdated) std::cerr<<result.errors.join('\n').toStdString()<<'\n';
        require(result.refUpdated && result.copied.size()==5 && result.reused.size()==1 && ended,"successful copy/reuse/ref/progress result");
        for(const auto &file:plan.files) require(get(dst,file.relative)==get(src,QDir(src).relativeFilePath(file.source)),"copied resource bytes changed");
        const QByteArray copiedRef=get(dst,"Destination.ref"); require(copiedRef.startsWith(originalRef),"existing REF bytes were rewritten");
        QStringDecoder decoder(QStringDecoder::Utf16LE); const QString text=decoder(copiedRef);
        require(text.contains(QStringLiteral("Description ( \"Zaun \u00e4\" )")) && text.contains(source.settings),"name/category/settings or Unicode were lost");
        require(!StaticCopy::transfer(plan,"Another","Fences").refUpdated,"stale destination REF review was reused");
        const auto again=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(!StaticCopy::transfer(again,QStringLiteral("Zaun \u00e4"),"Fences").refUpdated,"duplicate REF name/category was silently added");
        const auto reused=StaticCopy::transfer(again,"Second fence","Fences");
        require(reused.refUpdated && reused.copied.isEmpty() && reused.reused.size()==6,"identical resources were not reused on a second transfer");
    }
    {
        QTemporaryDir dir; const auto source=fixture(dir.filePath("Source")); const QString dst=dir.filePath("Destination"); route(dst,"Destination");
        put(dst,"TEXTURES/Rusty metal.ace","different destination texture"); const QByteArray ref=get(dst,"Destination.ref");
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(!plan.valid && get(dst,"Destination.ref")==ref && !QFile::exists(dst+"/SHAPES/Fence.s"),"conflicting resources were overwritten or partially copied during review");
        require(!StaticCopy::validateDestination(dir.filePath("NotARoute")).valid,"invalid destination accepted");
        require(!StaticCopy::prepare(source,StaticCopy::validateDestination(source.routeRoot)).valid,"source route accepted as destination");
    }
    {
        QTemporaryDir dir; const auto source=fixture(dir.filePath("Source")); const QString dst=dir.filePath("Destination"); route(dst,"Destination");
        const QByteArray ref=get(dst,"Destination.ref"); const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        const auto result=StaticCopy::transfer(plan,"Fence","Fences",[&](double,const QString &message){
            return !message.startsWith("Copying:") || !QFile::exists(dst+"/SHAPES/Fence.s");
        });
        require(result.cancelled && result.copied.isEmpty() && get(dst,"Destination.ref")==ref
            && !QFile::exists(dst+"/SHAPES/Fence.s"),"cancel after first commit failed to roll back new resources");
    }
    {
        QTemporaryDir dir; const auto source=fixture(dir.filePath("Source")); const QString dst=dir.filePath("Destination"); route(dst,"Destination");
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        const QByteArray external=unicode("SIMISA@@@@@@@@@@JINX0r1t______\r\nComment ( \"external REF change\" )\r\n");
        const auto result=StaticCopy::transfer(plan,"Fence","Fences",[&](double,const QString &message){
            if(message=="Updating destination REF") put(dst,"Destination.ref",external); return true;
        });
        require(!result.refUpdated && !result.errors.isEmpty() && result.copied.isEmpty() && get(dst,"Destination.ref")==external
            && !QFile::exists(dst+"/SHAPES/Fence.s"),"changed REF was overwritten or newly copied resources were not rolled back");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination",false);
        put(src,"SHAPES/Fence.s","shape ( image ( missing.ace ) )");
        require(!StaticCopy::prepare(source,StaticCopy::validateDestination(dst)).valid,"missing dependency was accepted");
        put(src,"SHAPES/Fence.s","shape ( image ( C:/absolute.ace ) )");
        require(!StaticCopy::prepare(source,StaticCopy::validateDestination(dst)).valid,"absolute resource reference accepted without rewriting");
        source=fixture(src);
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(StaticCopy::transfer(plan,QStringLiteral("Fence \u96ea"),"Fences").refUpdated
            && get(dst,"Destination.ref").startsWith(QByteArray::fromHex("fffe")),"new Unicode REF was not created as UTF-16LE");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination");
        const QByteArray legacy="SIMISA@@@@@@@@@@JINX0r1t______\r\nComment ( \"legacy bytes\" )\r\n"; put(dst,"Destination.ref",legacy);
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(StaticCopy::transfer(plan,QStringLiteral("Zaun \u00e4"),"Fences").refUpdated
            && get(dst,"Destination.ref").startsWith(legacy),"legacy REF bytes/encoding were not preserved");
        const auto reviewed=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        const QByteArray before=get(dst,"Destination.ref");
        require(!StaticCopy::transfer(reviewed,QStringLiteral("Fence \u96ea"),"Fences").refUpdated
            && get(dst,"Destination.ref")==before,"unrepresentable name corrupted legacy REF");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination");
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        put(src,"TEXTURES/Rusty metal.ace","changed source pixels");
        require(!StaticCopy::transfer(plan,"Fence","Fences").refUpdated && !QFile::exists(dst+"/SHAPES/Fence.s"),"source mutation was not detected before destination publication");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination");
        put(src,"SHAPES/models/Tree.gltf",R"json({"asset":{"version":"2.0"},"buffers":[{"uri":"mesh.bin"}],"images":[{"uri":"../../TEXTURES/My%20image.png"},{"uri":"data:image/png;base64,AAAA"}]})json");
        put(src,"SHAPES/models/mesh.bin","mesh bytes"); put(src,"TEXTURES/My image.png","image bytes"); source.shapes={"models/Tree.gltf"};
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(plan.valid && plan.files.size()==3,"glTF URI buffer/image dependencies not collected");
        require(StaticCopy::transfer(plan,"Tree","Scenery").refUpdated && get(dst,"SHAPES/models/mesh.bin")=="mesh bytes"
            && get(dst,"TEXTURES/My image.png")=="image bytes","glTF relative resource layout not preserved");
        put(src,"SHAPES/models/Tree.gltf",R"json({"asset":{"version":"2.0"},"images":[{"uri":"../../../escape.png"}]})json");
        put(dir.path(),"escape.png","outside");
        const QString other=dir.filePath("OtherDestination"); route(other,"Other");
        require(!StaticCopy::prepare(source,StaticCopy::validateDestination(other)).valid,"escaping glTF destination path accepted");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination");
        const QByteArray model=binaryShape("Rusty metal.ace"); QByteArray compressed=model.left(16); compressed[7]='F';
        qToLittleEndian<quint32>(quint32(model.size()-16),compressed.data()+8); compressed+=qCompress(model.mid(16)).mid(4);
        put(src,"SHAPES/Binary.s",compressed); source.shapes={"Binary.s"};
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(plan.valid && StaticCopy::transfer(plan,"Binary","Scenery").refUpdated
            && get(dst,"SHAPES/Binary.s")==compressed && get(dst,"TEXTURES/NIGHT/Rusty metal.dds")=="night DDS",
            "compressed binary shape dependencies or original encoding were lost");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); auto source=fixture(src); route(dst,"Destination");
        QByteArray json=R"json({"asset":{"version":"2.0"},"images":[{"uri":"../TEXTURES/External.png"}]})json";
        while(json.size()%4) json+=' ';
        QByteArray glb("glTF"); QDataStream out(&glb,QIODevice::Append); out.setByteOrder(QDataStream::LittleEndian);
        out<<quint32(2)<<quint32(20+json.size())<<quint32(json.size()); out.writeRawData("JSON",4); out.writeRawData(json.constData(),json.size());
        put(src,"SHAPES/Tree.glb",glb); put(src,"TEXTURES/External.png","GLB external image"); source.shapes={"Tree.glb"};
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(plan.valid && plan.files.size()==2 && StaticCopy::transfer(plan,"GLB tree","Scenery").refUpdated
            && get(dst,"SHAPES/Tree.glb")==glb && get(dst,"TEXTURES/External.png")=="GLB external image","GLB external image or original container was lost");
    }
    {
        QTemporaryDir dir; const QString dst=dir.filePath("Destination"); route(dst,"Destination");
        put(dst,"Destination.trk",unicode("Comment ( \"Tr_RouteFile ( FileName ( Fake ) )\" )"));
        require(!StaticCopy::validateDestination(dst).valid,"quoted route block was accepted as an actual route descriptor");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), work=dir.filePath("Working"); auto source=fixture(src);
        require(QDir().mkdir(work),"working folder fixture");
        const auto destination=StaticCopy::validateWorkingFolder(work,true);
        require(destination.valid && destination.workingFolder && !StaticCopy::validateDestination(work).valid,
            "ordinary working folder incorrectly needs or passes route validation");
        // Working exports require no catalog fields, even if REF settings are unavailable.
        source.settings="not a REF (";
        const auto plan=StaticCopy::prepare(source,destination);
        require(plan.valid && plan.destination.root==work+"/Fence" && !QFileInfo::exists(plan.destination.root),
            "working review did not choose a fresh object folder without creating it");
        double last=0; bool complete=false;
        const auto result=StaticCopy::transfer(plan,{},{},[&](double value,const QString &){
            require(value+1e-9>=last,"working progress moved backward"); last=value;
            if(value>=1) { require(get(plan.destination.root,"SHAPES/Fence.s")==get(src,"SHAPES/Fence.s"),"working progress completed before resources"); complete=true; }
            return true;
        });
        require(result.completed && !result.refUpdated && complete && result.copied.size()==6
            && result.outputFolder==plan.destination.root,"working copy result");
        for(const auto &file:plan.files) require(get(result.outputFolder,file.relative)==get(src,QDir(src).relativeFilePath(file.source)),"working resource bytes changed");
        require(QDir(work).entryList({"*.ref"},QDir::Files).isEmpty()
            && QDir(result.outputFolder).entryList({"*.ref"},QDir::Files).isEmpty(),"working export created a REF");
        const auto second=StaticCopy::prepare(source,destination);
        require(second.valid && second.destination.root==work+"/Fence_2"
            && StaticCopy::transfer(second,{},{}).completed,"repeat working export reused an existing object folder");
        require(!StaticCopy::transfer(plan,{},{}).completed,"stale working review overwrote an existing folder");
        require(!StaticCopy::prepare(source,StaticCopy::validateWorkingFolder(src)).valid,"working export accepted source route");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), work=dir.filePath("Working"); const auto source=fixture(src);
        require(QDir().mkdir(work),"working cancellation fixture"); put(work,"Keep.txt","existing working file");
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateWorkingFolder(work));
        const auto result=StaticCopy::transfer(plan,{},{},[&](double,const QString &message){
            return !message.startsWith("Copying:") || !QFile::exists(plan.destination.root+"/SHAPES/Fence.s");
        });
        require(result.cancelled && !result.completed && result.copied.isEmpty() && !QFileInfo::exists(plan.destination.root)
            && get(work,"Keep.txt")=="existing working file","cancelled working export left owned resources or removed existing data");
        const auto reserved=StaticCopy::prepare(source,StaticCopy::validateWorkingFolder(work));
        require(QDir().mkdir(reserved.destination.root),"external object folder fixture"); put(reserved.destination.root,"Keep.txt","external");
        require(!StaticCopy::transfer(reserved,{},{}).completed && get(reserved.destination.root,"Keep.txt")=="external",
            "working transfer touched an object folder created after review");
        const auto finalCancel=StaticCopy::prepare(source,StaticCopy::validateWorkingFolder(work));
        const auto cancelled=StaticCopy::transfer(finalCancel,{},{},[](double,const QString &message){ return message!="Finishing object folder"; });
        require(cancelled.cancelled && !cancelled.completed && !QFileInfo::exists(finalCancel.destination.root),
            "working cancellation at final validation left a completed export");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); const auto source=fixture(src); route(dst,"Destination");
        put(dst,"Destination.trk",unicode("SIMISA@@@@@@@@@@JINX0r1t______\r\nTr_RouteFile ( Name ( \"FileName ( Wrong )\" ) Comment ( FileName ( AlsoWrong ) ) FileName ( Destination ) )"));
        put(dst,"Destination.ref",unicode("SIMISA@@@@@@@@@@JINX0r1t______\r\nStatic ( Comment ( Class ( Wrong ) Description ( Wrong ) ) Class ( Fences ) Filename ( old.s ) Description ( Existing ) )"));
        const auto checked=StaticCopy::validateDestination(dst);
        require(checked.valid && checked.refPath==dst+"/Destination.ref" && checked.categories==QStringList{"Fences"},
            "quoted or commented field text overrode actual route/catalog fields");
        const auto plan=StaticCopy::prepare(source,checked);
        require(!StaticCopy::transfer(plan,"Existing","Fences").completed && !QFileInfo::exists(dst+"/SHAPES/Fence.s"),
            "commented description bypassed duplicate name protection");
        const auto ref=get(dst,"Destination.ref"); put(dst,"Destination.ref",ref+QByteArray(1,'x'));
        require(!StaticCopy::validateDestination(dst).valid,"odd-byte UTF-16 REF accepted for appending");
        put(dst,"Destination.ref",ref+QByteArray::fromHex("00d8"));
        require(!StaticCopy::validateDestination(dst).valid,"incomplete UTF-16 surrogate accepted for appending");
        put(dst,"Destination.ref",ref+QByteArray::fromHex("00dc"));
        require(!StaticCopy::validateDestination(dst).valid,"isolated UTF-16 low surrogate accepted for appending");
        put(dst,"Destination.ref",ref);
        const auto validUnicode=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(StaticCopy::transfer(validUnicode,QStringLiteral("Fence \U0001f682"),"Fences").completed,
            "valid supplementary Unicode character was rejected");
        put(dst,"Destination.trk",get(dst,"Destination.trk")+QByteArray(1,'x'));
        require(!StaticCopy::validateDestination(dst).valid,"odd-byte UTF-16 route descriptor accepted");
    }
    {
        QTemporaryDir dir; const QString src=dir.filePath("Source"), dst=dir.filePath("Destination"); const auto source=fixture(src); route(dst,"Destination");
        const auto plan=StaticCopy::prepare(source,StaticCopy::validateDestination(dst));
        require(plan.valid,"shared lock fixture plan");
        const QByteArray before=get(dst,"Destination.ref");
        QLockFile lock(RouteDistro::operationLockPath(plan.destination.root)); require(lock.tryLock(0),"shared route lock fixture");
        const auto stopped=StaticCopy::transfer(plan,"Fence","Fences");
        require(!stopped.completed && !stopped.errors.isEmpty() && get(dst,"Destination.ref")==before
            && !QFileInfo::exists(dst+"/SHAPES/Fence.s"),"copy ignored cleanup/shared destination lock");
        require(RouteDistro::operationLockPath(plan.destination.root.toUpper())==RouteDistro::operationLockPath(plan.destination.root),
            "case-only route aliases used different operation locks");
    }
    std::cout<<"Static Copy To dependency/encoding/transaction probes passed\n";
}

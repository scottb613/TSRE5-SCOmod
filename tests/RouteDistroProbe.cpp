// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "RouteDistro.h"
#include "TerrainTextureRewrite.h"
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

static void require(bool value,const char *message) {
    if(!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
static void put(const QString &root,const QString &relative,const QByteArray &bytes) {
    const QString path=QDir(root).filePath(relative);
    require(QDir().mkpath(QFileInfo(path).absolutePath()),"fixture directory");
    QFile file(path); require(file.open(QIODevice::WriteOnly),"fixture open");
    require(file.write(bytes)==bytes.size(),"fixture write");
}
static bool candidate(const RouteDistro::Report &report,const QString &path) {
    for(const auto &entry:report.removals) if(entry.path==path) return true;
    return false;
}
static void base(const QString &root) {
    put(root,"WORLD/tile.w","Tr_Worldfile ( Static ( FileName ( \"Fence.s\" ) ) )");
    put(root,"SHAPES/Fence.s","shape ( textures ( 1 image ( \"Brown metal.ace\" 0 ) ) )");
    put(root,"SHAPES/Fence.sd","shape ( \"Fence.s\" ESD_Alternative_Texture ( 256 ) )");
    put(root,"TEXTURES/Brown metal.ace","required texture");
    put(root,"TEXTURES/SNOW/Brown metal.dds","seasonal required texture");
    put(root,"SHAPES/Unused.s","shape ( )");
    put(root,"TEXTURES/Unused.ace","unused texture");
    put(root,"notes.tmp","unreferenced temporary file");
    put(root,"TEXTURES/backups/Brown metal.ace","old texture backup");
}
static QByteArray binary(quint32 token,const QString &name) {
    QByteArray payload;
    QDataStream strings(&payload,QIODevice::WriteOnly); strings.setByteOrder(QDataStream::LittleEndian);
    strings<<quint8(0)<<quint16(name.size());
    for(QChar c:name) strings<<quint16(c.unicode());
    QByteArray bytes=token==136?QByteArray("SIMISA@@@@@@@@@@JINX0t6b______\r\n",32):
        (token==71?QByteArray("SIMISA@@@@@@@@@@JINX0s1b______\r\n",32):QByteArray("SIMISA@@@@@@@@@@JINX0w0b______\r\n",32));
    QDataStream stream(&bytes,QIODevice::Append); stream.setByteOrder(QDataStream::LittleEndian);
    stream<<token<<quint32(payload.size()); stream.writeRawData(payload.constData(),payload.size());
    return bytes;
}
static QByteArray terrainBlock(quint32 token,const QByteArray &payload) {
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream<<token<<quint32(payload.size()); stream.writeRawData(payload.constData(),payload.size());
    return bytes;
}
static QByteArray terrainCount(quint32 count) {
    QByteArray bytes(1,'\0'); QDataStream stream(&bytes,QIODevice::Append);
    stream.setByteOrder(QDataStream::LittleEndian); stream<<count; return bytes;
}
static QByteArray terrainString(const QString &value) {
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian); stream<<quint16(value.size());
    for(QChar c:value) stream<<quint16(c.unicode()); return bytes;
}
static QByteArray terrainWith(const QStringList &names) {
    QByteArray textureSlots=terrainCount(quint32(names.size()));
    for(const QString &name:names)
        textureSlots+=terrainBlock(154,QByteArray(1,'\0')+terrainString(name)+QByteArray::fromHex("0100000000000000"));
    const QByteArray shader=terrainBlock(152,QByteArray(1,'\0')+terrainString("shader")+terrainBlock(153,textureSlots));
    const QByteArray untouched=terrainBlock(139,QByteArray::fromHex("0001020304050607"))
        +terrainBlock(157,QByteArray::fromHex("00deadbeef110022003300"));
    return QByteArray("SIMISA@@@@@@@@@@JINX0t6b______\r\n",32)
        +terrainBlock(136,QByteArray(1,'\0')+untouched+terrainBlock(151,terrainCount(1)+shader));
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    {
        QTemporaryDir dir; require(dir.isValid(),"fixture"); base(dir.path());
        auto report=RouteDistro::scan(dir.path(),dir.path());
        if(!report.complete) std::cerr<<report.problems.join('\n').toStdString()<<'\n';
        require(report.complete,"basic full-route scan");
        require(candidate(report,"SHAPES/Unused.s") && candidate(report,"TEXTURES/Unused.ace")
            && candidate(report,"notes.tmp") && candidate(report,"TEXTURES/backups/Brown metal.ace"),
            "unused/development candidates missing");
        require(!candidate(report,"SHAPES/Fence.s") && !candidate(report,"SHAPES/Fence.sd")
            && !candidate(report,"TEXTURES/Brown metal.ace")
            && !candidate(report,"TEXTURES/SNOW/Brown metal.dds"),"required/descriptor/space/season/DDS fallback asset offered for deletion");
        const auto result=RouteDistro::clean(report,{"TEXTURES/Unused.ace"});
        require(result.errors.isEmpty() && result.removed==QStringList{"TEXTURES/Unused.ace"},"selected cleanup failed");
        require(result.sizeMeasured && result.beforeBytes==report.bytes && result.netBytes==result.bytes
            && result.beforeBytes-result.afterBytes==result.netBytes,"ordinary deletion net savings were not measured");
        require(QFile::exists(dir.filePath("SHAPES/Unused.s"))
            && QFile::exists(dir.filePath("SHAPES/Fence.s")),"cleanup crossed selection/dependency boundary");
        require(!RouteDistro::clean(report,{"notes.tmp"}).errors.isEmpty(),"stale review reused after deletion");
    }
    {
        QTemporaryDir dir; base(dir.path());
        const QString unicodeName=QStringLiteral("Gr\u00fcn%20.ace");
        put(dir.path(),"TEXTURES/"+unicodeName,"required Unicode literal-percent texture");
        put(dir.path(),"SHAPES/Fence.s",binary(71,unicodeName));
        const QByteArray world=binary(261844u+375u,"Fence.s");
        QByteArray compressed=world.left(16); compressed[7]='F';
        qToLittleEndian<quint32>(world.size()-16,compressed.data()+8);
        compressed+=qCompress(world.mid(16)).mid(4);
        put(dir.path(),"WORLD/tile.w",compressed);
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !candidate(report,"TEXTURES/"+unicodeName),
            "compressed binary world and Unicode/literal-percent binary shape dependencies");
        QByteArray malformed=world; qToLittleEndian<quint32>(1,malformed.data()+36);
        put(dir.path(),"WORLD/tile.w",malformed);
        const auto uncertain=RouteDistro::scan(dir.path(),dir.path());
        require(uncertain.complete && !uncertain.warnings.isEmpty()
            && !candidate(uncertain,"SHAPES/Unused.s"),"truncated binary world did not protect uncertain assets");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"tiles/tile.t",binary(136,"paint.ace"));
        put(dir.path(),"terrtex/paint.ace","identical painted pixels");
        put(dir.path(),"terrtex/other.ace","identical painted pixels");
        put(dir.path(),"terrtex/SNOW/paint.dds","snow pixels");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && report.duplicateGroups==1,"binary terrain reference / TERRTEX grouping");
        require(candidate(report,"terrtex/other.ace") && !candidate(report,"terrtex/paint.ace")
            && !candidate(report,"terrtex/SNOW/paint.dds"),"used duplicate/seasonal TEX was not protected");
        put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static ( FileName ( \"Unused.s\" ) ) )");
        const auto result=RouteDistro::clean(report,{"SHAPES/Unused.s"});
        require(!result.errors.isEmpty() && QFile::exists(dir.filePath("SHAPES/Unused.s")),"changed world dependency did not stop cleanup");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static (");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !report.warnings.isEmpty()
            && !candidate(report,"TEXTURES/Unused.ace"),"malformed world authorized orphan inference");
        require(!RouteDistro::clean(report,{"TEXTURES/Unused.ace"}).errors.isEmpty()
            && QFile::exists(dir.filePath("TEXTURES/Unused.ace")),"incomplete scan allowed deletion");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"SHAPES/Fence.s","not_a_shape ( )");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !report.warnings.isEmpty() && !candidate(report,"TEXTURES/Unused.ace"),
            "unrecognized required shape did not protect uncertain assets");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"SHAPES/Fence.s","shape ( image ( missing/Unused.ace ) )");
        const auto missing=RouteDistro::scan(dir.path(),dir.path());
        require(missing.complete && !missing.warnings.isEmpty(),
            "same basename in another folder hid a missing explicit dependency warning");
        put(dir.path(),"SHAPES/Fence.s","shape ( image ( \"../TEXTURES/backups/Brown metal.ace\" ) )");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !candidate(report,"TEXTURES/backups/Brown metal.ace"),
            "explicitly referenced backup-folder asset was offered for removal");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"catalog.ref","Static ( FileName ( \"Unused.s\" ) )");
        const auto editable=RouteDistro::scan(dir.path(),dir.path());
        const auto playable=RouteDistro::scan(dir.path(),dir.path(),{false});
        require(editable.complete && playable.complete && !candidate(editable,"SHAPES/Unused.s")
            && candidate(playable,"SHAPES/Unused.s"),"catalog preservation option");
        require(!RouteDistro::clean(editable,{"../outside.ace"}).errors.isEmpty(),"unreviewed traversal selected");
        require(!RouteDistro::clean(editable,{"SHAPES/Fence.s"}).errors.isEmpty(),"required file selected through forged request");
    }
    {
        QTemporaryDir dir; base(dir.path());
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        auto result=RouteDistro::clean(report,{"notes.tmp"},[](double,const QString &){ return false; });
        require(result.cancelled && result.removed.isEmpty() && QFile::exists(dir.filePath("notes.tmp")),"preflight cancellation changed route");
        int attempts=0;
        result=RouteDistro::clean(report,{"notes.tmp","SHAPES/Unused.s"},[&](double,const QString &message){
            return !message.startsWith("Removing:") || attempts++==0;
        });
        require(result.cancelled && result.removed.size()==1 && QFile::exists(dir.filePath("SHAPES/Unused.s")),
            "mid-clean cancellation did not preserve remaining selection");
    }
    {
        QTemporaryDir dir; base(dir.path());
        const auto report=RouteDistro::scan(dir.path(),dir.path()); bool changed=false;
        const auto result=RouteDistro::clean(report,{"SHAPES/Unused.s"},[&](double,const QString &message){
            if(!changed && message.startsWith("Verifying removal:")) {
                changed=true;
                put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static ( FileName ( Unused.s ) ) )");
            }
            return true;
        });
        require(!result.errors.isEmpty() && result.removed.isEmpty() && QFile::exists(dir.filePath("SHAPES/Unused.s")),
            "dependency changed during selected-file verification did not stop cleanup");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static ( FileName ( \"Model.gltf\" ) ) )");
        put(dir.path(),"SHAPES/Model.gltf",R"({"asset":{"version":"2.0"},"buffers":[{"uri":"mesh.bin"}],"images":[{"uri":"../TEXTURES/My%20image.png"}]})");
        put(dir.path(),"SHAPES/mesh.bin","binary mesh");
        put(dir.path(),"TEXTURES/My image.png","image bytes");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !candidate(report,"SHAPES/mesh.bin")
            && !candidate(report,"TEXTURES/My image.png"),"glTF buffer/image URI dependencies");
        put(dir.path(),"SHAPES/Model.gltf","{ incomplete JSON");
        const auto uncertain=RouteDistro::scan(dir.path(),dir.path());
        require(uncertain.complete && !uncertain.warnings.isEmpty()
            && !candidate(uncertain,"SHAPES/mesh.bin") && !candidate(uncertain,"TEXTURES/My image.png"),
            "malformed required glTF did not protect possible dependencies");
    }
    {
        QTemporaryDir dir; const QString route=dir.filePath("routes/Test"); base(route);
        put(route,"WORLD/tile.w","Tr_Worldfile ( TrackObj ( FileName ( \"GlobalTrack.s\" ) ) )");
        put(dir.path(),"GLOBAL/SHAPES/GlobalTrack.s","shape ( textures ( 1 image ( \"rail.ace\" 0 ) ) )");
        put(dir.path(),"GLOBAL/TEXTURES/rail.dds","global DDS fallback");
        const auto report=RouteDistro::scan(route,dir.path());
        if(!report.complete) std::cerr<<report.problems.join('\n').toStdString()<<'\n';
        require(report.complete && !report.notices.isEmpty(),"GLOBAL shape/DDS dependency graph");
        const auto result=RouteDistro::clean(report,{"SHAPES/Unused.s"});
        require(result.errors.isEmpty() && QFile::exists(dir.filePath("GLOBAL/SHAPES/GlobalTrack.s")),"external dependency cleanup boundary");
    }
    {
        QTemporaryDir dir; const QString route=dir.filePath("routes/Test"); base(route);
        put(route,"ACTIVITIES/Trip.act","Activity ( Player_Service_Definition ( Trip ) )");
        put(route,"SERVICES/Trip.srv","Service_Definition ( Trip Train_Config ( TestConsist ) PathID ( RoutePath ) )");
        put(route,"PATHS/RoutePath.pat","TrackPDPs ( )");
        put(dir.path(),"TRAINS/CONSISTS/TestConsist.con","Train ( TrainCfg ( TestConsist Engine ( EngineData ( Unit Set ) ) ) )");
        put(dir.path(),"TRAINS/TRAINSET/Set/Unit.eng","Wagon ( Unit WagonShape ( Body.s ) )");
        put(dir.path(),"TRAINS/TRAINSET/Set/Body.s","shape ( textures ( 1 image ( \"../../../routes/Test/TEXTURES/Unused.ace\" 0 ) ) )");
        const auto report=RouteDistro::scan(route,dir.path());
        if(!report.complete) std::cerr<<report.problems.join('\n').toStdString()<<'\n';
        require(report.complete && !candidate(report,"TEXTURES/Unused.ace"),
            "activity/service/path/consist/rolling-stock dependency chain must protect route textures");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"TILES/tile.t","terrain ( terrain_sample_ybuffer ( tile_y.raw ) terrain_sample_fbuffer ( tile_f.raw ) terrain_sample_ebuffer ( tile_e.raw ) terrain_sample_nbuffer ( tile_n.raw ) )");
        put(dir.path(),"TILES/tile_y.raw","elevation bytes");
        put(dir.path(),"TILES/tile_f.raw","terrain flag bytes");
        auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && report.warnings.isEmpty(),"absent generated terrain buffers produced warnings");
        put(dir.path(),"TILES/tile_e.raw","generated e buffer");
        put(dir.path(),"TILES/tile_n.raw","generated n buffer");
        put(dir.path(),"LO_TILES/distant_n.raw","generated distant buffer");
        put(dir.path(),"TILES/unknown.raw","unknown terrain data");
        put(dir.path(),"TILES/backups/tile_y.raw","backup elevation data");
        put(dir.path(),"TILES/backups/tile_f.raw","backup terrain flags");
        put(dir.path(),"TILES/tile_y.raw.bak","named elevation backup");
        put(dir.path(),"TILES/tile_f.raw.backup","named terrain flag backup");
        put(dir.path(),"TEXTURES/tile_e.raw","outside terrain folder");
        report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && candidate(report,"TILES/tile_e.raw") && candidate(report,"TILES/tile_n.raw")
            && candidate(report,"LO_TILES/distant_n.raw"),"generated buffers were not optional candidates");
        for(const QString &path:QStringList{"TILES/tile_y.raw","TILES/tile_f.raw","TILES/unknown.raw",
                "TILES/backups/tile_y.raw","TILES/backups/tile_f.raw","TILES/tile_y.raw.bak",
                "TILES/tile_f.raw.backup","TEXTURES/tile_e.raw"}) {
            require(!candidate(report,path),"non-optional RAW data offered for deletion");
            require(!RouteDistro::clean(report,{path}).errors.isEmpty(),"forged RAW deletion was authorized");
        }
        const auto result=RouteDistro::clean(report,{"TILES/tile_e.raw"});
        require(result.errors.isEmpty() && result.removed==QStringList{"TILES/tile_e.raw"}
            && QFile::exists(dir.filePath("TILES/tile_n.raw")) && QFile::exists(dir.filePath("TILES/tile_y.raw"))
            && QFile::exists(dir.filePath("TILES/tile_f.raw")),"optional buffer removal crossed selection/elevation boundary");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static ( FileName ( Span'BridgeA.s ) ) )");
        put(dir.path(),"SHAPES/Span'BridgeA.s","shape ( image ( \"Brown metal.ace\" ) )");
        put(dir.path(),"SHAPES/Span'BridgeA.sd","shape ( OldName.s )");
        put(dir.path(),"SERVICES/missing.srv","Service_Definition ( Train_Config ( MissingTrain ) )");
        put(dir.path(),"osm_data/catalog.json",R"({"path":"geodata.gpkg"})");
        put(dir.path(),"osm_data/geodata.gpkg","opaque geodata");
        put(dir.path(),"SOUND/test.sms","Tr_SMS ( File ( missing.wav -1 )");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !report.warnings.isEmpty() && candidate(report,"TEXTURES/Unused.ace")
            && !candidate(report,"SHAPES/Span'BridgeA.s"),"warnings blocked unrelated cleanup or apostrophe shape was not retained");
        require(!report.warnings.join('\n').contains("-> BridgeA.s")
            && !report.warnings.join('\n').contains("OldName.s"),"apostrophe filename or descriptor label produced a false warning");
        const auto result=RouteDistro::clean(report,{"TEXTURES/Unused.ace"});
        require(result.errors.isEmpty() && result.removed.size()==1,"warning-only scan could not clean unrelated selection");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"WORLD/tile.w","Tr_Worldfile ( Static (");
        put(dir.path(),"TILES/tile_n.raw","generated buffer");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !report.warnings.isEmpty() && candidate(report,"TILES/tile_n.raw")
            && !candidate(report,"SHAPES/Unused.s") && !candidate(report,"TEXTURES/Unused.ace"),
            "uncertain world did not separate protected assets from unrelated buffers");
        require(RouteDistro::clean(report,{"TILES/tile_n.raw"}).errors.isEmpty(),"uncertain world blocked unrelated buffer cleanup");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"TILES/paint.t",terrainWith({"a.ace","paint_long_b.ace"}));
        for(const QString &name:QStringList{"a","paint_long_b"}) {
            put(dir.path(),"TERRTEX/"+name+".ace","same summer bytes");
            put(dir.path(),"TERRTEX/SNOW/"+name+".dds","same snow bytes");
        }
        put(dir.path(),"TILES/paint_y.raw","elevation remains unchanged");
        put(dir.path(),"TILES/paint_f.raw","flags remain unchanged");
        require(!candidate(RouteDistro::scan(dir.path(),dir.path(),{true,false,false}),"TERRTEX/paint_long_b.ace"),
            "referenced consolidation was offered with its option disabled");
        const auto report=RouteDistro::scan(dir.path(),dir.path(),{true,true,false});
        require(report.complete && candidate(report,"TERRTEX/paint_long_b.ace")
            && !candidate(report,"TERRTEX/a.ace"),"identical whole families were not offered with a protected keeper");
        require(RouteDistro::summary(report).contains("Consolidate TERRTEX"),"category savings summary missing");
        const auto result=RouteDistro::clean(report,{"TERRTEX/paint_long_b.ace"});
        if(!result.errors.isEmpty()) std::cerr<<result.errors.join('\n').toStdString()<<'\n';
        require(result.errors.isEmpty() && result.removed.size()==2 && result.rewritten==QStringList{"TILES/paint.t"},
            "binary terrain consolidation failed");
        QFile terrain(dir.filePath("TILES/paint.t")); require(terrain.open(QIODevice::ReadOnly),"packed terrain open");
        const QByteArray output=terrain.readAll();
        require(output==terrainWith({"a.ace","a.ace"}),"binary rewrite altered unrelated terrain fields or lengths");
        require(result.sizeMeasured && result.beforeBytes==report.bytes
            && result.netBytes==result.bytes+terrainWith({"a.ace","paint_long_b.ace"}).size()-output.size(),
            "net route savings ignored terrain reference length changes");
        const QString audit=RouteDistro::auditReport(report,{"TERRTEX/paint_long_b.ace"},&result);
        require(audit.contains("[SELECTED] TERRTEX/paint_long_b.ace")
            && audit.contains("retained replacement: TERRTEX/a.ace")
            && audit.contains("DELETED FILES\nTERRTEX/paint_long_b.ace")
            && audit.contains("TERRTEX/SNOW/paint_long_b.dds")
            && audit.contains("UPDATED TERRAIN DESCRIPTORS\nTILES/paint.t")
            && audit.contains("Measured route bytes before:"),"audit report lost selections, replacements, actual deletions or net savings");
        require(QFile::exists(dir.filePath("TERRTEX/a.ace")) && QFile::exists(dir.filePath("TERRTEX/SNOW/a.dds"))
            && QFile::exists(dir.filePath("TILES/paint_y.raw")) && QFile::exists(dir.filePath("TILES/paint_f.raw")),
            "packing removed canonical/terrain data");
        QFile elevations(dir.filePath("TILES/paint_y.raw")), flags(dir.filePath("TILES/paint_f.raw"));
        require(elevations.open(QIODevice::ReadOnly) && elevations.readAll()=="elevation remains unchanged"
            && flags.open(QIODevice::ReadOnly) && flags.readAll()=="flags remain unchanged",
            "packing changed elevation or terrain flag bytes");
        const auto after=RouteDistro::scan(dir.path(),dir.path());
        require(after.complete && after.warnings.isEmpty(),"packing left broken terrain dependencies");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"TILES/paint.t",terrainWith({"a.ace","b.ace"}));
        put(dir.path(),"TERRTEX/a.ace","same summer"); put(dir.path(),"TERRTEX/b.ace","same summer");
        put(dir.path(),"TERRTEX/SNOW/a.ace","snow one"); put(dir.path(),"TERRTEX/SNOW/b.ace","snow two");
        require(!candidate(RouteDistro::scan(dir.path(),dir.path(),{true,true,false}),"TERRTEX/b.ace"),
            "matching summer alone authorized consolidation of different snow textures");
        put(dir.path(),"TERRTEX/SNOW/b.ace","snow one");
        put(dir.path(),"SHAPES/Fence.s","shape ( image ( b.ace ) )");
        require(!candidate(RouteDistro::scan(dir.path(),dir.path(),{true,true,false}),"TERRTEX/b.ace"),
            "non-terrain texture usage was consolidated without rewriting its references");
    }
    {
        const TerrainTextureRewrite::Names names{{"old_long.ace","a.ace"}};
        const QByteArray original=terrainWith({"old_long.ace"});
        QByteArray compressed=original.left(16); compressed[7]='F';
        qToLittleEndian<quint32>(quint32(original.size()-16),compressed.data()+8);
        compressed+=qCompress(original.mid(16)).mid(4);
        QByteArray output; QSet<QString> changed;
        require(TerrainTextureRewrite::rewrite(compressed,names,output,changed) && output.startsWith("SIMISA@F"),
            "compressed terrain rewrite failed or lost compression");
        QByteArray expanded; require(WireAssetScan::expandedWorldFile(output,expanded),"packed terrain decompression failed");
        expanded.replace(0,16,"SIMISA@@@@@@@@@@");
        require(expanded==terrainWith({"a.ace"}),"compressed rewrite corrupted terrain data");
        const QString text="terrain ( terrain_texslot ( \"old_long.ace\" 1 0 ) terrain_sample_ybuffer ( tile_y.raw ) )";
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        const QByteArray utf16=QByteArray::fromHex("fffe")+encoder(text);
        require(TerrainTextureRewrite::rewrite(utf16,names,output,changed) && output.startsWith(QByteArray::fromHex("fffe")),
            "UTF-16 terrain rewrite failed");
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        require(QString(decoder(output))==QString(text).replace("old_long.ace","a.ace"),"UTF-16 rewrite changed unrelated text");
        QByteArray truncated=original; truncated.chop(1);
        require(!TerrainTextureRewrite::rewrite(truncated,names,output,changed),"truncated terrain accepted for rewriting");
    }
    {
        QTemporaryDir dir; base(dir.path());
        for(const QString &tile:QStringList{"one","two"}) put(dir.path(),"TILES/"+tile+".t",terrainWith({"a.ace","long_b.ace"}));
        put(dir.path(),"TERRTEX/a.ace","identical bytes"); put(dir.path(),"TERRTEX/long_b.ace","identical bytes");
        const auto report=RouteDistro::scan(dir.path(),dir.path(),{true,true,false});
        int commits=0;
        const auto cancelled=RouteDistro::clean(report,{"TERRTEX/long_b.ace"},[&](double,const QString &message){
            return !message.startsWith("Updating terrain references:") || commits++==0;
        });
        require(cancelled.cancelled && cancelled.rewritten.size()==1 && cancelled.removed.isEmpty()
            && QFile::exists(dir.filePath("TERRTEX/a.ace")) && QFile::exists(dir.filePath("TERRTEX/long_b.ace")),
            "partial reference-commit cancellation removed a still-needed texture");
        require(cancelled.sizeMeasured && cancelled.netBytes==terrainWith({"a.ace","long_b.ace"}).size()-terrainWith({"a.ace","a.ace"}).size(),
            "partial commit net savings did not reflect the actual descriptor change");
        require(RouteDistro::scan(dir.path(),dir.path()).warnings.isEmpty(),"partial reference commit left dangling dependencies");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"TILES/one.t",terrainWith({"a.ace","long_b.ace"}));
        put(dir.path(),"TERRTEX/a.ace","identical bytes"); put(dir.path(),"TERRTEX/long_b.ace","identical bytes");
        const auto report=RouteDistro::scan(dir.path(),dir.path(),{true,true,false}); bool changed=false;
        const auto result=RouteDistro::clean(report,{"TERRTEX/long_b.ace"},[&](double,const QString &message){
            if(!changed && message.startsWith("Staging terrain references:")) {
                put(dir.path(),"TERRTEX/a.ace","keeper changed externally"); changed=true;
            }
            return true;
        });
        require(!result.errors.isEmpty() && result.removed.isEmpty() && QFile::exists(dir.filePath("TERRTEX/long_b.ace")),
            "changed canonical texture did not prevent redundant texture deletion");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"osm_data/index.json",R"({"path":"data.gpkg"})");
        put(dir.path(),"osm_data/data.gpkg","editing data");
        require(!candidate(RouteDistro::scan(dir.path(),dir.path(),{true,false,false}),"osm_data/data.gpkg"),"geodata offered with its option disabled");
        const auto report=RouteDistro::scan(dir.path(),dir.path(),{true,false,true});
        require(candidate(report,"osm_data/index.json") && candidate(report,"osm_data/data.gpkg"),"opted-in unreferenced geodata not offered");
        put(dir.path(),"catalog.ref","Static ( Filename ( osm_data/index.json ) )");
        require(!candidate(RouteDistro::scan(dir.path(),dir.path(),{true,false,true}),"osm_data/data.gpkg"),
            "referenced editing geodata was offered for removal");
    }
    {
        const auto groups=RouteDistro::warningGroups({
            "Missing dependency: ACTIVITIES/CONSISTS/Trip.con -> C:/Sim/TRAINS/TRAINSET/Unit/Unit.eng",
            "Missing dependency: SERVICES/Trip.srv -> Stock.con",
            "Missing dependency: WORLD/tile.w -> Bridge.s",
            "Missing dependency: ssource.dat -> forest.sms",
            "Missing dependency: Route.trk -> rain.env",
            "Referenced file retained without format inspection: osm_data/index.json -> osm_data/data.gpkg",
            "Missing dependency: TILES/tile.t -> tile_y.raw"});
        require(groups.value("Rolling stock / activities").size()==2
            && groups.value("Scenery / shapes").size()==1 && groups.value("Sound").size()==1
            && groups.value("Environment / weather").size()==1 && groups.value("Editing geodata / caches").size()==1
            && groups.value("Terrain data").size()==1,"dependency warnings were not grouped by their user-facing roles");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"WORLD/tile.w.bk","old world backup");
        put(dir.path(),"source.psd","source artwork");
        put(dir.path(),"TILES/tile_y.raw.bk","elevation backup");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        QMap<QString,QString> categories;
        for(const auto &removal:report.removals) categories.insert(removal.path,removal.category);
        require(categories.value("WORLD/tile.w.bk")=="Backups" && categories.value("source.psd")=="Source artwork"
            && categories.value("notes.tmp")=="Logs / temporary files"
            && categories.value("TEXTURES/Unused.ace")=="Unused textures"
            && categories.value("SHAPES/Unused.s")=="Unused shapes / models"
            && !candidate(report,"TILES/tile_y.raw.bk"),"cleanup category separation lost a file or offered RAW backup data");
    }
    {
        QTemporaryDir dir; base(dir.path());
        auto report=RouteDistro::scan(dir.path(),dir.path());
        report.warnings<<"Missing dependency: WORLD/tile.w -> Missing.s";
        const QString review=RouteDistro::auditReport(report,{"notes.tmp"});
        require(review.contains("Scenery / shapes (1)") && review.contains("[SELECTED] notes.tmp")
            && review.contains("[not selected] SHAPES/Unused.s") && review.contains("Review only.")
            && !review.contains("ACTUAL DELETION RESULT"),"review export confused candidates with actual deletions");
        const auto result=RouteDistro::clean(report,{"notes.tmp"},[](double,const QString &message){
            return !message.startsWith("Measuring route size:");
        });
        require(result.cancelled && result.removed==QStringList{"notes.tmp"} && !result.sizeMeasured
            && RouteDistro::sizeSummary(result).contains("unavailable"),"cancelled measurement invented net savings or lost actual deletions");
        const QString actual=RouteDistro::auditReport(report,{"notes.tmp"},&result);
        require(actual.contains("DELETED FILES\nnotes.tmp") && actual.contains("Route-size measurement cancelled."),
            "partial/cancelled operation export lost actual outcomes");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"TILES/one.t",terrainWith({"a_very_long_keeper.ace","b.ace"}));
        put(dir.path(),"TERRTEX/a_very_long_keeper.ace","x"); put(dir.path(),"TERRTEX/b.ace","x");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        const auto result=RouteDistro::clean(report,{"TERRTEX/b.ace"});
        require(result.errors.isEmpty() && result.sizeMeasured && result.netBytes<0
            && RouteDistro::sizeSummary(result).contains("net growth"),"descriptor growth was incorrectly presented as net savings");
    }
    {
        QTemporaryDir dir; base(dir.path());
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        const QByteArray malformed=QByteArray::fromHex("fffe")+encoder(u"shape ( images ( image ( \"Brown metal.ace\" ) ) )")+QByteArray(1,'x');
        put(dir.path(),"SHAPES/Fence.s",malformed);
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && !report.warnings.isEmpty() && !candidate(report,"TEXTURES/Unused.ace")
            && candidate(report,"notes.tmp"),"truncated UTF-16 model failed to protect possible dependencies");
        QByteArray output; QSet<QString> changed;
        const QByteArray terrain=QByteArray::fromHex("fffe")+encoder(u"terrain ( terrain_texslot ( old.ace 0 0 ) )")+QByteArray(1,'x');
        require(!TerrainTextureRewrite::rewrite(terrain,{{"old.ace","new.ace"}},output,changed),
            "odd-byte UTF-16 terrain was rewritten with data loss");
        const QByteArray surrogate=QByteArray::fromHex("fffe")+encoder(u"terrain ( terrain_texslot ( old.ace 0 0 ) )")+QByteArray::fromHex("00d8");
        require(!TerrainTextureRewrite::rewrite(surrogate,{{"old.ace","new.ace"}},output,changed),
            "incomplete UTF-16 surrogate terrain was rewritten with data loss");
        put(dir.path(),"SHAPES/Fence.s",malformed.left(malformed.size()-1)+QByteArray::fromHex("00dc"));
        const auto invalidSurrogate=RouteDistro::scan(dir.path(),dir.path());
        require(invalidSurrogate.complete && !invalidSurrogate.warnings.isEmpty() && !candidate(invalidSurrogate,"TEXTURES/Unused.ace"),
            "isolated UTF-16 surrogate model failed to protect possible dependencies");
    }
    {
        QTemporaryDir dir; base(dir.path()); const auto report=RouteDistro::scan(dir.path(),dir.path());
        QLockFile lock(RouteDistro::operationLockPath(report.root)); require(lock.tryLock(0),"shared cleanup lock fixture");
        const auto result=RouteDistro::clean(report,{"notes.tmp"});
        require(result.removed.isEmpty() && !result.errors.isEmpty() && QFileInfo::exists(dir.filePath("notes.tmp")),
            "cleanup ignored Copy To/shared destination lock");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"terrain_maps/123.png","saved map overlay");
        put(dir.path(),"terrain_maps/sub/overlay.tif","saved reference image");
        put(dir.path(),"terrain_maps/keep.raw","protected raw data");
        put(dir.path(),"other_terrain_maps/keep.png","unclassified image");
        put(dir.path(),"TILES/map.png","unclassified terrain folder file");
        const auto disabled=RouteDistro::scan(dir.path(),dir.path(),{true,false,false});
        require(!candidate(disabled,"terrain_maps/123.png") && !candidate(disabled,"terrain_maps/sub/overlay.tif"),
            "saved maps offered with editing-data option disabled");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(report.complete && candidate(report,"terrain_maps/123.png") && candidate(report,"terrain_maps/sub/overlay.tif")
            && !candidate(report,"terrain_maps/keep.raw") && !candidate(report,"other_terrain_maps/keep.png")
            && !candidate(report,"TILES/map.png"),"map cleanup missed known overlays or selected protected/unclassified files");
        for(const auto &item:report.removals) if(item.path=="terrain_maps/123.png")
            require(item.category=="Editing maps / overlays" && item.bytes==17,"map category or bytes");
        const auto result=RouteDistro::clean(report,{"terrain_maps/123.png","terrain_maps/sub/overlay.tif"});
        require(result.errors.isEmpty() && result.removed.size()==2 && result.sizeMeasured && result.netBytes==38
            && QFileInfo::exists(dir.filePath("terrain_maps/keep.raw")) && QFileInfo::exists(dir.filePath("TILES/map.png")),
            "selected map cleanup failed or altered protected files/measured savings");
    }
    {
        QTemporaryDir dir; base(dir.path());
        put(dir.path(),"terrain_maps/used.png","runtime image"); put(dir.path(),"terrain_maps/unused.png","editor map");
        put(dir.path(),"SHAPES/Fence.s","shape ( images ( image ( \"../terrain_maps/used.png\" ) ) )");
        const auto report=RouteDistro::scan(dir.path(),dir.path());
        require(!candidate(report,"terrain_maps/used.png") && candidate(report,"terrain_maps/unused.png"),
            "runtime-referenced map image offered for deletion");
        put(dir.path(),"SHAPES/Fence.s","shape ( incomplete");
        const auto uncertain=RouteDistro::scan(dir.path(),dir.path());
        require(uncertain.complete && !uncertain.warnings.isEmpty() && !candidate(uncertain,"terrain_maps/used.png")
            && !candidate(uncertain,"terrain_maps/unused.png") && candidate(uncertain,"notes.tmp"),
            "uncertain dependencies failed to protect possible map resources or blocked unrelated cleanup");
    }
    std::cout<<"Route distribution scan/selection/safety probes passed\n";
}

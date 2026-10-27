// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "RouteDistro.h"
#include "WireAssetScan.h"
#include "TerrainTextureRewrite.h"
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QQueue>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QSet>
#include <QStringConverter>
#include <QUrl>
#include <QXmlStreamReader>
#include <QtEndian>
#include <algorithm>
#include <limits>

namespace RouteDistro {
namespace {
constexpr qint64 MaximumReferenceBytes = 256ll*1024*1024;
constexpr int MaximumFiles = 1000000;
QString key(QString path) { return QDir::cleanPath(path.replace('\\','/')).toCaseFolded(); }
bool inside(const QString &root, const QString &path) {
    return path.startsWith(root+'/',Qt::CaseInsensitive);
}
bool safeFile(const QString &root, const QString &relative) {
    if(relative.isEmpty() || QDir::isAbsolutePath(relative) || relative.contains(':')
            || relative.contains('\\') || QDir::cleanPath(relative)!=relative) return false;
    const QFileInfo info(QDir(root).filePath(relative));
    return info.isFile() && !info.isSymLink() && !info.isJunction()
        && inside(root,info.canonicalFilePath());
}
bool notify(const Progress &progress, double value, const QString &message) {
    return !progress || progress(std::clamp(value,0.0,1.0),message);
}
bool referenceType(const QString &suffix) {
    static const QSet<QString> types{"w","ws","t","s","sd","gltf","glb",
        "trk","tdb","rdb","tit","rit","dat","sms","env","haz","act",
        "srv","trf","pat","con","eng","wag","ref","json","xml","ini",
        "cfg","csv","inc","js","cs","lua","cvf","timetable_or","timetable-or"};
    return types.contains(suffix);
}
bool textureType(const QString &suffix) {
    static const QSet<QString> types{"ace","dds","png","jpg","jpeg","bmp",
        "tga","gif","webp","tif","tiff"};
    return types.contains(suffix);
}
bool optionalTerrainBuffer(const QString &path) {
    // Only direct TILES/LO_TILES companions qualify; never general *.raw files.
    static const QRegularExpression pattern("^(tiles|lo_tiles)/[^/]+_[en]\\.raw$",
        QRegularExpression::CaseInsensitiveOption);
    return pattern.match(path).hasMatch();
}
bool terrainBufferReference(const QString &path) {
    static const QRegularExpression pattern("_[en]\\.raw$",QRegularExpression::CaseInsensitiveOption);
    return pattern.match(path).hasMatch();
}
bool rawDataFile(const QString &path) {
    static const QRegularExpression pattern("\\.raw(?:\\.|$)",QRegularExpression::CaseInsensitiveOption);
    return pattern.match(QFileInfo(path).fileName()).hasMatch();
}
bool developmentFile(const QString &path) {
    const QFileInfo file(path);
    const QString name=file.fileName().toLower(), suffix=file.suffix().toLower();
    static const QRegularExpression backupFolder("(^|/)(backups?|\\.backup)(/)",QRegularExpression::CaseInsensitiveOption);
    static const QSet<QString> suffixes{"bk","bak","bkp","backup","tmp","temp","old","blend","blend1","blend2","psd","xcf","fbx","max"};
    return backupFolder.match(path).hasMatch()
        || suffixes.contains(suffix)
        || name.startsWith("tsre-log-") || name=="openrailslog.txt"
        || name=="thumbs.db" || name=="desktop.ini" || name==".ds_store" || name.contains(".backup.");
}
bool assetFile(const QString &path) {
    const QString lower=path.toLower(), suffix=QFileInfo(path).suffix().toLower();
    const bool folder=lower.startsWith("shapes/") || lower.startsWith("textures/")
        || lower.startsWith("terrtex/");
    static const QSet<QString> models{"s","sd","gltf","glb","bin"};
    return folder && (textureType(suffix) || models.contains(suffix));
}
bool editorCache(const QString &path) {
    return path.startsWith("osm_data/",Qt::CaseInsensitive)
        || path.startsWith("terrain_maps/",Qt::CaseInsensitive);
}
struct File {
    QString path, suffix, error;
    qint64 size=0;
    QByteArray hash;
    QSet<QString> references;
    bool asset=false, development=false, external=false;
};
QString removalCategory(const File &file) {
    if(file.development) {
        static const QSet<QString> backups{"bk","bak","bkp","backup","old"};
        static const QSet<QString> artwork{"blend","blend1","blend2","psd","xcf","fbx","max"};
        static const QRegularExpression folder("(^|/)(backups?|\\.backup)/",QRegularExpression::CaseInsensitiveOption);
        if(backups.contains(file.suffix) || folder.match(file.path).hasMatch()
                || QFileInfo(file.path).fileName().contains(".backup.",Qt::CaseInsensitive)) return "Backups";
        if(artwork.contains(file.suffix)) return "Source artwork";
        return "Logs / temporary files";
    }
    if(file.path.startsWith("terrtex/",Qt::CaseInsensitive)) return "TERRTEX";
    return textureType(file.suffix)?QString("Unused textures"):QString("Unused shapes / models");
}
// Accept quoted filenames (including spaces) and ordinary unquoted MSTS tokens.
// Binary strings are also enumerated from their length-prefixed UTF-16 storage.
void textReferences(const QString &text, QSet<QString> &result) {
    // MSTS quotes strings with double quotes. Apostrophes are filename characters.
    static const QRegularExpression quoted(QStringLiteral("\"([^\"\\r\\n]+)\""));
    static const QRegularExpression token(QStringLiteral(
        R"((?:[\p{L}\p{M}\p{N}_~!@#$%&+'.,:/\\-]+)\.(?:ace|dds|png|jpg|jpeg|bmp|tga|gif|webp|tiff?|s|sd|gltf|glb|bin|wav|sms|env|haz|dat|con|eng|wag|pat|srv|trf|act|trk|raw|inc|json|xml|js|cs|lua|cvf)(?![\p{L}\p{N}_]))"),
        QRegularExpression::CaseInsensitiveOption);
    auto matches=quoted.globalMatch(text);
    while(matches.hasNext()) {
        const auto match=matches.next();
        result.insert(match.captured(1).replace("\\\\","\\"));
    }
    QString unquoted=text; unquoted.replace(quoted," ");
    matches=token.globalMatch(unquoted);
    while(matches.hasNext()) result.insert(matches.next().captured());
}
void jsonReferences(const QJsonValue &value, QSet<QString> &result) {
    if(value.isString()) result.insert(value.toString());
    else if(value.isArray()) for(const auto &entry:value.toArray()) jsonReferences(entry,result);
    else if(value.isObject()) for(const auto &entry:value.toObject()) jsonReferences(entry,result);
}
void implicitReferences(const QString &text,const QString &suffix,QSet<QString> &result) {
    const QString atom=QStringLiteral("(?:\"([^\"\\r\\n]+)\"|([^\\s()]+))");
    const auto field=[&](const QString &name,const QString &prefix,const QString &extension){
        const QRegularExpression pattern("\\b"+name+"\\s*\\(\\s*"+atom,QRegularExpression::CaseInsensitiveOption);
        auto matches=pattern.globalMatch(text);
        while(matches.hasNext()) {
            const auto match=matches.next();
            QString value=match.captured(1).isEmpty()?match.captured(2):match.captured(1);
            if(!value.endsWith(extension,Qt::CaseInsensitive)) value+=extension;
            result.insert(prefix+value);
        }
    };
    if(suffix=="srv" || suffix=="act") field("Train_Config","@consist:",".con");
    if(suffix=="srv") field("PathID","",".pat");
    if(suffix=="act") {
        field("Player_Service_Definition","",".srv");
        field("Traffic_Definition","",".trf");
    }
    if(suffix=="trf" || suffix=="act") field("Service_Definition","",".srv");
    if(suffix=="con") {
        const QRegularExpression pattern("\\b(EngineData|WagonData)\\s*\\(\\s*"+atom+"\\s+"+atom,
            QRegularExpression::CaseInsensitiveOption);
        auto matches=pattern.globalMatch(text);
        while(matches.hasNext()) {
            const auto match=matches.next();
            QString file=match.captured(2).isEmpty()?match.captured(3):match.captured(2);
            const QString folder=match.captured(4).isEmpty()?match.captured(5):match.captured(4);
            const QString extension=match.captured(1).compare("EngineData",Qt::CaseInsensitive)==0?".eng":".wag";
            if(!file.endsWith(extension,Qt::CaseInsensitive)) file+=extension;
            result.insert("@train:"+folder+'/'+file);
        }
    }
}
bool balanced(const QString &text) {
    int depth=0; bool quote=false, escape=false;
    for(QChar c:text) {
        if(c.isNull()) return false;
        if(quote) {
            if(escape) escape=false;
            else if(c=='\\') escape=true;
            else if(c=='"') quote=false;
        } else if(c=='"') quote=true;
        else if(c=='(') ++depth;
        else if(c==')' && --depth<0) return false;
    }
    return !quote && depth==0;
}
bool references(const QByteArray &input, const QString &suffix, QSet<QString> &result,
                const std::function<bool()> &poll = {}) {
    QByteArray data=input;
    if(suffix=="glb") {
        if(data.size()<20 || data.left(4)!="glTF"
                || qFromLittleEndian<quint32>(data.constData()+4)!=2
                || qFromLittleEndian<quint32>(data.constData()+8)!=quint32(data.size())) return false;
        const quint32 length=qFromLittleEndian<quint32>(data.constData()+12);
        if(length>quint32(data.size()-20) || data.mid(16,4)!="JSON") return false;
        data=data.mid(20,length);
    }
    if(suffix=="json" || suffix=="gltf" || suffix=="glb") {
        QJsonParseError error;
        const auto document=QJsonDocument::fromJson(data,&error);
        if(error.error!=QJsonParseError::NoError || document.isNull()) return false;
        if(document.isObject()) jsonReferences(document.object(),result);
        else jsonReferences(document.array(),result);
        return true;
    }
    if(!WireAssetScan::expandedWorldFile(data,data)) return false;
    if(input.startsWith("SIMISA@F")) data.replace(0,16,"SIMISA@@@@@@@@@@");
    else if(input.startsWith(QByteArray::fromHex("fffe")) && input.size()>34 && input[16]=='F') {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        data.replace(0,34,QByteArray::fromHex("fffe")+encoder(u"SIMISA@@@@@@@@@@"));
    }
    const bool binary=data.left(48).toLower().contains("jinx")
        && data.left(48).toLower().contains('b') && data.size()>40
        && data.contains('\0');
    if(binary) {
        if(suffix=="act" || suffix=="srv" || suffix=="trf" || suffix=="con" || suffix=="eng" || suffix=="wag") return false;
        // Supported legacy binary S/W/T envelopes begin at byte 32. World
        // token IDs have a different base, but their block lengths are alike.
        const quint32 length=qFromLittleEndian<quint32>(data.constData()+36);
        if(length!=quint32(data.size()-40)) return false;
        const quint32 token=qFromLittleEndian<quint32>(data.constData()+32);
        if((suffix=="s" && token!=71) || (suffix=="t" && token!=136)
                || (suffix=="w" && token!=261844u+375u)) return false;
        for(qsizetype p=40;p+4<data.size();++p) {
            if((p&65535)==0 && poll && !poll()) return false;
            const quint16 count=qFromLittleEndian<quint16>(data.constData()+p);
            if(!count || count>4096 || qsizetype(count)*2>data.size()-p-2) continue;
            QString value; bool valid=true;
            for(int i=0;i<count;++i) {
                const QChar c(qFromLittleEndian<quint16>(data.constData()+p+2+i*2));
                if(!c.isPrint() && !c.isSurrogate()) { valid=false; break; }
                value.append(c);
            }
            if(valid && value.contains('.')) result.insert(value);
        }
        return true;
    }
    QString text;
    if(data.startsWith(QByteArray::fromHex("fffe"))) {
        if(data.size()%2) return false;
        QStringDecoder decoder(QStringDecoder::Utf16LE,QStringConverter::Flag::Stateless);
        text=decoder(data); if(decoder.hasError()) return false;
    } else if(data.startsWith(QByteArray::fromHex("feff"))) {
        if(data.size()%2) return false;
        QStringDecoder decoder(QStringDecoder::Utf16BE,QStringConverter::Flag::Stateless);
        text=decoder(data); if(decoder.hasError()) return false;
    } else {
        QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
        text=decoder(data);
        if(decoder.hasError()) text=QString::fromLatin1(data);
    }
    if(!text.isValidUtf16()) return false;
    if(suffix=="xml") {
        QXmlStreamReader reader(text);
        while(!reader.atEnd()) reader.readNext();
        if(reader.hasError()) return false;
    } else {
        // Inclusion scanning must not enforce one grammar on scripts, catalogs
        // and sound definitions. Keep strict envelopes for world/model/terrain.
        static const QSet<QString> strict{"w","ws","t","s","sd"};
        if(text.contains(QChar(0)) || (strict.contains(suffix) && !balanced(text))) return false;
    }
    if(suffix=="w" && !WorldCleanupValidation::completeTextWorld(text)) return false;
    if(suffix=="s" || suffix=="sd") {
        QString envelope=text.trimmed();
        if(envelope.startsWith(QChar(0xfeff))) envelope.remove(0,1);
        if(envelope.startsWith("SIMISA",Qt::CaseInsensitive)) {
            const qsizetype end=envelope.indexOf(QRegularExpression("\\s"));
            if(end<0) return false;
            envelope=envelope.mid(end).trimmed();
        }
        static const QRegularExpression shapeRoot("^shape\\s*\\(",QRegularExpression::CaseInsensitiveOption);
        if(!shapeRoot.match(envelope).hasMatch()) return false;
    }
    if(suffix=="sd") {
        // The first Shape(...) value is a descriptive name, often stale after
        // a rename. The actual sidecar is resolved from the used shape's name.
        static const QRegularExpression descriptorName(
            R"(\bshape\s*\(\s*(?:"[^"]*"|[^\s()]+))",QRegularExpression::CaseInsensitiveOption);
        text.replace(descriptorName,"shape (");
    }
    textReferences(text,result);
    implicitReferences(text,suffix,result);
    return true;
}
QByteArray readFile(const QString &path, QByteArray *contents, const Progress &progress,
        double start, double share, bool &cancelled, QString &error) {
    QFile file(path); QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!file.open(QIODevice::ReadOnly)) { error=file.errorString(); return {}; }
    const qint64 size=file.size(); qint64 done=0;
    if(contents && size>MaximumReferenceBytes) { error="Dependency file exceeds the supported size limit."; return {}; }
    while(!file.atEnd()) {
        const QByteArray chunk=file.read(1024*1024);
        if(chunk.isEmpty()) { error="File could not be read completely: "+path; return {}; }
        if(contents && chunk.size()>MaximumReferenceBytes-contents->size()) {
            error="Dependency file grew beyond the supported size limit."; return {};
        }
        hash.addData(chunk); done+=chunk.size();
        if(contents) contents->append(chunk);
        if(!notify(progress,start+share*double(done)/qMax(qint64(1),size),QFileInfo(path).fileName())) {
            cancelled=true; return {};
        }
    }
    if(file.error()!=QFileDevice::NoError || done!=size) { error="File changed or could not be read completely."; return {}; }
    return hash.result();
}
}

bool inspectAssetReferences(const QString &path, QStringList &found, QString &error, const Progress &progress) {
    found.clear(); error.clear();
    if(QFileInfo(path).size()>MaximumReferenceBytes) { error="Dependency file exceeds the supported size limit: "+path; return false; }
    QByteArray data; bool cancelled=false;
    if(readFile(path,&data,progress,0,0,cancelled,error).isEmpty()) {
        if(cancelled) error="Dependency inspection cancelled.";
        return false;
    }
    QSet<QString> values;
    if(!references(data,QFileInfo(path).suffix().toLower(),values,[&](){ return notify(progress,0,"Inspecting: "+QFileInfo(path).fileName()); })) {
        error="Unsupported or incomplete dependency data: "+path; return false;
    }
    found=values.values(); found.sort(); return true;
}

Report scan(const QString &routeRoot, const QString &installationRoot, Options options,
            const Progress &progress) {
    Report report; report.root=QFileInfo(routeRoot).canonicalFilePath();
    report.scannedAtUtc=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    report.installationRoot=QFileInfo(installationRoot).canonicalFilePath(); report.options=options;
    if(report.root.isEmpty() || QFileInfo(routeRoot).isSymLink() || QFileInfo(routeRoot).isJunction()) {
        report.problems<<"Choose an ordinary existing route folder."; return report;
    }
    QMap<QString,File> files;
    QHash<QString,QStringList> names;
    QDirIterator inventory(report.root,QDir::Files|QDir::Dirs|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot,
                           QDirIterator::Subdirectories);
    while(inventory.hasNext()) {
        const QFileInfo info(inventory.next());
        if(info.isSymLink() || info.isJunction()) { report.problems<<"Linked path requires review: "+info.filePath(); continue; }
        if(info.isDir()) {
            if(!QDir(info.absoluteFilePath()).isReadable()) report.problems<<"Unreadable directory: "+info.filePath();
            continue;
        }
        const QString path=QDir(report.root).relativeFilePath(info.absoluteFilePath());
        if(!safeFile(report.root,path)) { report.problems<<"Unsafe route path: "+path; continue; }
        if(files.contains(key(path))) { report.problems<<"Case-colliding route paths: "+path; continue; }
        File entry; entry.path=path; entry.suffix=info.suffix().toLower(); entry.size=info.size();
        entry.asset=assetFile(path); entry.development=developmentFile(path);
        files.insert(key(path),entry); names[info.fileName().toCaseFolded()].append(key(path));
        if(entry.size<0 || report.bytes>std::numeric_limits<qint64>::max()-entry.size) {
            report.problems<<"Route size exceeds the supported measurement range."; return report;
        }
        report.bytes+=entry.size;
        if(files.size()>MaximumFiles) { report.problems<<"Route exceeds the one-million-file scan limit."; return report; }
        if(!notify(progress,0.03,"Inventory: "+QString::number(files.size())+" route files")) {
            report.cancelled=true; return report;
        }
    }
    report.files=files.size();
    bool hasRouteData=false;
    for(const auto &entry:files) if(!entry.development && (entry.suffix=="w" || entry.suffix=="t" || entry.suffix=="trk")) hasRouteData=true;
    if(!hasRouteData) report.problems<<"No route/world/terrain descriptors were found; cleanup is not authorized.";
    qint64 done=0;
    for(auto it=files.begin();it!=files.end();++it) {
        File &entry=it.value(); QByteArray data;
        const bool parse=referenceType(entry.suffix);
        const bool oversized=parse && entry.size>MaximumReferenceBytes;
        entry.hash=readFile(QDir(report.root).filePath(entry.path),parse&&!oversized?&data:nullptr,
            progress,0.05+0.83*double(done)/qMax(qint64(1),report.bytes),
            0.83*double(entry.size)/qMax(qint64(1),report.bytes),report.cancelled,entry.error);
        if(report.cancelled) return report;
        if(entry.hash.isEmpty()) report.problems<<"Unreadable file: "+entry.path+": "+entry.error;
        else if(oversized || (parse && !references(data,entry.suffix,entry.references,[&](){
            const bool keepGoing=notify(progress,0.05+0.83*double(done+entry.size)/qMax(qint64(1),report.bytes),
                "Inspecting dependencies: "+entry.path);
            report.cancelled=!keepGoing; return keepGoing;
        })))
            entry.error="Unsupported, incomplete or malformed dependency data.";
        if(report.cancelled) return report;
        done+=entry.size;
    }
    QSet<QString> required;
    QSet<QString> nonTerrainNames;
    bool uncertainTerrtex=false;
    QQueue<QString> queue;
    const auto retain=[&](const QString &path) { if(!required.contains(path)) { required.insert(path); queue.enqueue(path); } };
    const auto protectUncertain=[&](const File &source) {
        // Incomplete dependency data protects its possible target assets, rather
        // than disabling unrelated buffer/temporary-file cleanup across a route.
        QStringList folders;
        if(source.suffix=="sms" || source.suffix=="cvf") folders={"SOUND/","TEXTURES/"};
        else if(source.suffix=="t") folders={"TERRTEX/"};
        else folders={"SHAPES/","TEXTURES/","TERRTEX/","SOUND/"};
        // A retained malformed source may refer outside the usual asset
        // folders. Do not delete map/cache files whose use cannot be ruled out.
        folders<<"terrain_maps/"<<"osm_data/";
        if(folders.contains("TERRTEX/")) uncertainTerrtex=true;
        for(auto it=files.cbegin();it!=files.cend();++it) {
            if(it->external || optionalTerrainBuffer(it->path)) continue;
            for(const QString &folder:folders) if(it->path.startsWith(folder,Qt::CaseInsensitive)) {
                retain(it.key()); break;
            }
        }
        report.warnings<<source.path+": "+source.error+" Potentially affected assets are protected ("+folders.join(", ")+").";
    };
    const auto externalFile=[&](const QString &path) {
        const QFileInfo info(path); const QString canonical=info.canonicalFilePath();
        const QString id="@"+key(canonical);
        if(files.contains(id)) { retain(id); return; }
        if(canonical.isEmpty() || report.installationRoot.isEmpty()
                || !inside(report.installationRoot,canonical) || info.isSymLink() || info.isJunction()) {
            report.warnings<<"External dependency cannot be safely inspected: "+path;
            File unknown; unknown.path=path; unknown.suffix=info.suffix().toLower();
            unknown.error="Dependency inspection unavailable."; protectUncertain(unknown); return;
        }
        if(files.size()>=MaximumFiles) { report.problems<<"Dependency graph exceeds the file limit."; return; }
        File entry; entry.path=canonical; entry.external=true; entry.suffix=info.suffix().toLower(); entry.size=info.size();
        QByteArray data;
        const bool parse=referenceType(entry.suffix);
        const bool oversized=parse && entry.size>MaximumReferenceBytes;
        entry.hash=readFile(canonical,parse&&!oversized?&data:nullptr,progress,0.89,0.005,report.cancelled,entry.error);
        if(entry.hash.isEmpty() || oversized || (parse && !references(data,entry.suffix,entry.references,[&](){
            const bool keepGoing=notify(progress,0.895,"Inspecting external dependency: "+canonical);
            report.cancelled=!keepGoing; return keepGoing;
        })))
            entry.error="External dependency could not be read completely.";
        files.insert(id,entry); retain(id);
        report.notices<<"External prerequisite: "+canonical;
    };
    for(auto it=files.cbegin();it!=files.cend();++it) {
        const auto &entry=it.value();
        const bool editor=entry.suffix=="ref" || entry.path.startsWith("ADDONS/",Qt::CaseInsensitive)
            || entry.path.endsWith("/polyveg.json",Qt::CaseInsensitive);
        if(!entry.asset && !entry.development && !optionalTerrainBuffer(entry.path)
                && !(options.includeEditorCaches && editorCache(entry.path))
                && (options.keepEditorAssets || !editor)) retain(it.key());
        const QString base=QFileInfo(entry.path).fileName().toLower();
        if(!entry.development && (base=="terrain.ace" || base=="terraindm.ace" || base=="terraindm.dds"
                || base=="ortsdefaultsnow.ace" || base=="ortsdefaultdmsnow.ace")) retain(it.key());
    }
    QSet<QString> unresolved;
    while(!queue.isEmpty()) {
        const QString current=queue.dequeue(); const File entry=files.value(current);
        if(!entry.error.isEmpty()) protectUncertain(entry);
        if(entry.suffix=="s") {
            for(const QString &sidecar:names.value((QFileInfo(entry.path).completeBaseName()+".sd").toCaseFolded()))
                if(!files[sidecar].development) retain(sidecar);
            if(entry.external) {
                const QString sidecar=QFileInfo(entry.path).absolutePath()+'/'+QFileInfo(entry.path).completeBaseName()+".sd";
                if(QFileInfo(sidecar).isFile()) externalFile(sidecar);
            }
        }
        for(QString reference:entry.references) {
            reference=reference.trimmed().replace('\\','/');
            if(reference.isEmpty() || reference.startsWith("data:",Qt::CaseInsensitive)) continue;
            // T descriptors retain legacy generated-buffer names even when the
            // companions are absent. Elevation/flag references are not skipped.
            if(entry.suffix=="t" && terrainBufferReference(reference)) continue;
            if(reference.startsWith("@consist:")) reference=report.installationRoot+"/TRAINS/CONSISTS/"+reference.mid(9);
            else if(reference.startsWith("@train:")) reference=report.installationRoot+"/TRAINS/TRAINSET/"+reference.mid(7);
            if(entry.suffix=="gltf" || entry.suffix=="glb")
                reference=QUrl::fromPercentEncoding(reference.toUtf8());
            const QFileInfo referenceInfo(reference);
            const QString suffix=referenceInfo.suffix().toLower();
            if(entry.suffix!="t") nonTerrainNames.insert(referenceInfo.completeBaseName().toCaseFolded());
            QStringList variants{referenceInfo.fileName().toCaseFolded()};
            if(suffix=="ace" || suffix=="dds") variants<<
                (referenceInfo.completeBaseName()+(suffix=="ace"?".dds":".ace")).toCaseFolded();
            // Keep every same-name seasonal/fallback variant conservatively.
            for(const QString &variant:variants) for(const QString &path:names.value(variant)) {
                if(files[path].development) continue; // Explicit path resolution below still protects referenced backups.
                retain(path);
                const QString targetSuffix=files[path].suffix;
                if(!referenceType(targetSuffix) && !textureType(targetSuffix)
                        && targetSuffix!="wav" && targetSuffix!="bin" && targetSuffix!="raw")
                    report.warnings<<"Referenced file retained without format inspection: "+entry.path+" -> "+files[path].path;
            }
            if(!referenceType(suffix) && !textureType(suffix)
                    && suffix!="wav" && suffix!="bin" && suffix!="raw") continue;
            const QString source=entry.external?entry.path:QDir(report.root).filePath(entry.path);
            const QString absolute=QDir::cleanPath(QFileInfo(reference).isAbsolute()?reference:
                QDir(QFileInfo(source).absolutePath()).absoluteFilePath(reference));
            QStringList external{absolute};
            for(const QString &folder:{QString("SHAPES"),QString("TEXTURES"),QString("SOUND"),QString("CABVIEW")})
                external<<QDir(QFileInfo(source).absolutePath()+'/'+folder).filePath(reference);
            for(const QString &folder:{QString(),QString("SHAPES"),QString("TEXTURES"),QString("TERRTEX"),
                    QString("ENVFILES"),QString("ENVFILES/TEXTURES"),QString("SOUND"),
                    QString("SERVICES"),QString("PATHS"),QString("TRAFFIC"),QString("ACTIVITIES")})
                external<<QDir(report.root+'/'+folder).filePath(reference);
            if(!report.installationRoot.isEmpty()) for(const QString &folder:{QString("GLOBAL/SHAPES"),QString("GLOBAL/TEXTURES"),QString("GLOBAL"),QString("SOUND"),QString()})
                external<<QDir(report.installationRoot+'/'+folder).filePath(reference);
            if(suffix=="ace" || suffix=="dds") {
                const QString alternate=suffix=="ace"?"dds":"ace";
                const QStringList initial=external;
                for(const QString &path:initial) external<<path.left(path.size()-3)+alternate;
            }
            bool exists=false;
            QSet<QString> checked;
            for(QString path:external) {
                path=QDir::cleanPath(path);
                if(checked.contains(key(path))) continue;
                checked.insert(key(path));
                const QString local=key(QDir(report.root).relativeFilePath(path));
                if(files.contains(local)) { retain(local); exists=true; continue; }
                if(!inside(report.root,path) && QFileInfo(path).isFile()) {
                    exists=true; externalFile(path);
                }
            }
            if(!exists) unresolved.insert(entry.path+" -> "+reference);
        }
        if(report.cancelled) return report;
        if(!notify(progress,0.90,"Checking dependencies: "+entry.path)) { report.cancelled=true; return report; }
    }
    for(const QString &missing:unresolved) report.warnings<<"Missing dependency: "+missing;
    QCryptographicHash signature(QCryptographicHash::Sha256);
    signature.addData(key(report.root).toUtf8()); signature.addData("\0",1);
    signature.addData(key(report.installationRoot).toUtf8()); signature.addData(options.keepEditorAssets?"1":"0",1);
    signature.addData(options.consolidateTerrtex?"1":"0",1);
    signature.addData(options.includeEditorCaches?"1":"0",1);
    for(auto it=files.cbegin();it!=files.cend();++it) {
        signature.addData(it.key().toUtf8()); signature.addData("\0",1); signature.addData(it->hash);
        if(required.contains(it.key()) && referenceType(it->suffix) && !it->hash.isEmpty())
            report.referenceHashes.insert(it->external?it->path:QDir(report.root).filePath(it->path),it->hash);
    }
    report.signature=signature.result();
    report.required=required.size();
    QHash<QByteArray,QStringList> duplicates;
    for(auto it=files.cbegin();it!=files.cend();++it)
        if(!it->external && it->path.startsWith("terrtex/",Qt::CaseInsensitive) && textureType(it->suffix) && !it->hash.isEmpty())
            duplicates[it->hash].append(it.key());
    for(const auto &group:duplicates) if(group.size()>1) {
        ++report.duplicateGroups;
        QStringList used; for(const QString &path:group) if(required.contains(path)) used<<files[path].path;
        if(used.size()>1) report.notices<<"Referenced identical TERRTEX retained: "+used.join(", ");
    }
    QSet<QString> consolidationPaths;
    if(options.consolidateTerrtex && !uncertainTerrtex) {
        // Match whole filename families, including every season/night/format.
        // Matching one snow file alone cannot prove two painted patches equal.
        QMap<QString,QMap<QString,QString>> families;
        QSet<QString> roots;
        for(auto it=files.cbegin();it!=files.cend();++it) {
            const File &file=it.value();
            if(file.external || file.development || !file.path.startsWith("TERRTEX/",Qt::CaseInsensitive)
                    || !textureType(file.suffix) || file.hash.isEmpty()) continue;
            const QString relative=file.path.mid(8), base=QFileInfo(relative).completeBaseName().toCaseFolded();
            const QString folder=QFileInfo(relative).path().toCaseFolded();
            families[base].insert(folder+'/'+file.suffix,it.key());
            if(folder==".") roots.insert(base);
        }
        QMap<QByteArray,QStringList> matching;
        for(auto it=families.cbegin();it!=families.cend();++it) {
            if(!roots.contains(it.key()) || nonTerrainNames.contains(it.key())
                    || it.key()=="terrain" || it.key()=="terraindm" || it.key().startsWith("ortsdefault")) continue;
            QCryptographicHash hash(QCryptographicHash::Sha256);
            for(auto part=it->cbegin();part!=it->cend();++part) {
                hash.addData(part.key().toUtf8()); hash.addData("\0",1); hash.addData(files[part.value()].hash);
            }
            matching[hash.result()].append(it.key());
        }
        QVector<Removal> planned;
        TerrainTextureRewrite::Names replacements;
        for(auto group:matching) {
            if(group.size()<2) continue;
            group.sort();
            // Prefer a currently used family; it will never be a removal row.
            for(const QString base:QStringList(group)) {
                bool used=false;
                for(const QString &part:families[base]) if(required.contains(part)) used=true;
                if(used) { group.removeAll(base); group.prepend(base); break; }
            }
            const QString canonical=group.front();
            for(qsizetype i=1;i<group.size();++i) {
                const QString base=group[i]; bool used=false;
                for(const QString &part:families[base]) if(required.contains(part)) used=true;
                if(!used) continue; // Already handled by ordinary orphan removal.
                if(families[base].keys()!=families[canonical].keys()) continue;
                bool identical=true;
                for(auto part=families[base].cbegin();part!=families[base].cend();++part)
                    if(files[part.value()].hash!=files[families[canonical].value(part.key())].hash) identical=false;
                if(!identical) continue;
                Removal removal; removal.category="Consolidate TERRTEX";
                for(auto part=families[base].cbegin();part!=families[base].cend();++part) {
                    const File &old=files[part.value()]; const File &keeper=files[families[canonical][part.key()]];
                    removal.related<<old.path; removal.keepers<<keeper.path; removal.bytes+=old.size;
                    removal.memberHashes<<old.hash;
                    if(QFileInfo(old.path).path().compare("TERRTEX",Qt::CaseInsensitive)==0 && removal.path.isEmpty()) {
                        removal.path=old.path; removal.hash=old.hash;
                    }
                    replacements.insert(QFileInfo(old.path).fileName().toCaseFolded(),QFileInfo(keeper.path).fileName());
                }
                removal.reason="Rewrite terrain texture slots, then remove identical family: "+removal.related.join(", ")
                    +"; keep: "+removal.keepers.join(", ");
                if(!removal.path.isEmpty()) planned.append(removal);
            }
        }
        bool supported=true; QSet<QString> remaining;
        for(auto it=files.cbegin();it!=files.cend() && !planned.isEmpty();++it) {
            if(it->external || it->development || it->suffix!="t") continue;
            QFile terrain(QDir(report.root).filePath(it->path));
            QByteArray rewritten; QSet<QString> changed, refs;
            if(!terrain.open(QIODevice::ReadOnly) || terrain.size()>MaximumReferenceBytes
                    || !TerrainTextureRewrite::rewrite(terrain.readAll(),replacements,rewritten,changed)
                    || !references(rewritten,"t",refs)) { supported=false; break; }
            for(QString ref:refs) {
                const QString name=QFileInfo(ref.replace('\\','/')).fileName().toCaseFolded();
                if(replacements.contains(name)) remaining.insert(name);
            }
            if(!notify(progress,0.95,"Reviewing TERRTEX consolidation: "+it->path)) { report.cancelled=true; return report; }
        }
        if(supported) for(const Removal &removal:planned) {
            bool safe=true;
            for(const QString &path:removal.related) if(remaining.contains(QFileInfo(path).fileName().toCaseFolded())) safe=false;
            if(!safe) continue;
            for(const QString &path:removal.related) consolidationPaths.insert(key(path));
            for(const QString &path:removal.keepers) required.insert(key(path));
            report.removals.append(removal); report.removableBytes+=removal.bytes;
        }
        else if(!planned.isEmpty()) report.notices<<"TERRTEX consolidation retained: a terrain descriptor cannot be safely rewritten.";
    } else if(options.consolidateTerrtex && uncertainTerrtex)
        report.notices<<"TERRTEX consolidation retained: uncertain terrain/asset dependency data.";
    for(auto it=files.cbegin();it!=files.cend();++it) {
        const auto &entry=it.value();
        const bool buffer=optionalTerrainBuffer(entry.path);
        // Never offer elevation, flag or unknown RAW data, even in backup folders.
        if(rawDataFile(entry.path) && !buffer) continue;
        const bool cache=options.includeEditorCaches && editorCache(entry.path);
        if(entry.external || consolidationPaths.contains(it.key()) || required.contains(it.key())
                || (!entry.asset && !entry.development && !buffer && !cache) || entry.hash.isEmpty()) continue;
        Removal removal; removal.path=entry.path; removal.bytes=entry.size; removal.hash=entry.hash;
        removal.category=removalCategory(entry);
        removal.reason=entry.development?"Unreferenced development/temporary file":"No dependency from the complete retained route graph";
        if(buffer) {
            removal.category="Optional terrain buffer";
            removal.reason="Regenerable _e.raw/_n.raw companion; elevation and flag data are preserved";
        }
        if(cache) {
            const bool map=entry.path.startsWith("terrain_maps/",Qt::CaseInsensitive);
            removal.category=map?"Editing maps / overlays":"Editing geodata/cache";
            removal.reason=map?"Saved editor map tile/overlay; not referenced by retained route/catalog files":
                "Optional editing data, not referenced by retained route/catalog files";
        }
        if(entry.path.startsWith("terrtex/",Qt::CaseInsensitive)
                && duplicates.contains(entry.hash) && duplicates[entry.hash].size()>1)
            removal.reason="Unreferenced byte-identical TERRTEX duplicate";
        report.removals.append(removal); report.removableBytes+=entry.size;
    }
    report.problems.removeDuplicates(); report.warnings.removeDuplicates(); report.notices.removeDuplicates();
    report.problems.sort(); report.warnings.sort(); report.notices.sort();
    report.complete=report.problems.isEmpty();
    report.required=required.size();
    notify(progress,1,"Full-route scan complete");
    return report;
}

static Result cleanImpl(const Report &reviewed, const QStringList &selected, const Progress &progress) {
    Result result;
    if(!reviewed.complete || selected.isEmpty()) { result.errors<<"A complete scan and selected removal items are required."; return result; }
    const Report fresh=scan(reviewed.root,reviewed.installationRoot,reviewed.options,
        [&](double value,const QString &message){ return notify(progress,0.60*value,"Revalidating: "+message); });
    if(fresh.cancelled) { result.cancelled=true; return result; }
    if(!fresh.complete || fresh.signature!=reviewed.signature) {
        result.errors<<"The route changed or the dependency scan is incomplete. Rescan and review before cleaning."; return result;
    }
    result.beforeBytes=fresh.bytes; result.beforeMeasured=true;
    QMap<QString,Removal> allowed; for(const auto &entry:fresh.removals) allowed.insert(entry.path,entry);
    QSet<QString> reviewedPaths; for(const auto &entry:reviewed.removals) reviewedPaths.insert(entry.path);
    QStringList paths=selected; paths.removeDuplicates();
    TerrainTextureRewrite::Names replacements;
    QStringList deletions;
    QMap<QString,QByteArray> familyProofs;
    QMap<QString,QByteArray> removalHashes;
    QMap<QString,qint64> removalSizes;
    for(int i=0;i<paths.size();++i) {
        const QString &path=paths[i];
        if(!reviewedPaths.contains(path) || !allowed.contains(path) || !safeFile(fresh.root,path)) {
            result.errors<<"Selected path is not an authorized removal: "+path; return result;
        }
        bool cancelled=false; QString error;
        const QByteArray hash=readFile(QDir(fresh.root).filePath(path),nullptr,
            [&](double value,const QString &message){ return notify(progress,0.60+0.05*(i+value)/paths.size(),"Verifying removal: "+message); },
            0,1,cancelled,error);
        if(cancelled) { result.cancelled=true; return result; }
        if(hash!=allowed[path].hash) { result.errors<<"Selected file changed: "+path; return result; }
        const Removal &removal=allowed[path];
        if(removal.related.isEmpty()) {
            deletions<<path; removalSizes.insert(path,removal.bytes); removalHashes.insert(path,removal.hash);
        }
        else for(qsizetype j=0;j<removal.related.size();++j) {
            const QString &old=removal.related[j], &keeper=removal.keepers[j];
            if(!safeFile(fresh.root,old) || !safeFile(fresh.root,keeper)) {
                result.errors<<"Unsafe consolidation path. Rescan before packing."; return result;
            }
            replacements.insert(QFileInfo(old).fileName().toCaseFolded(),QFileInfo(keeper).fileName());
            familyProofs.insert(old,removal.memberHashes[j]); familyProofs.insert(keeper,removal.memberHashes[j]);
            removalHashes.insert(old,removal.memberHashes[j]);
            removalSizes.insert(old,QFileInfo(QDir(fresh.root).filePath(old)).size());
            deletions<<old;
        }
    }
    deletions.removeDuplicates();
    const auto verifyFamilies=[&](double fraction) {
        for(auto it=familyProofs.cbegin();it!=familyProofs.cend();++it) {
            bool cancelled=false; QString error;
            if(!safeFile(fresh.root,it.key())) { result.errors<<"Unsafe texture family path: "+it.key(); return false; }
            const QByteArray hash=readFile(QDir(fresh.root).filePath(it.key()),nullptr,
                [&](double,const QString &message){ return notify(progress,fraction,"Verifying texture family: "+message); },
                0,1,cancelled,error);
            if(cancelled) { result.cancelled=true; return false; }
            if(hash!=it.value()) { result.errors<<"Texture family changed; nothing further removed: "+it.key(); return false; }
        }
        return true;
    };
    if(!verifyFamilies(0.65)) return result;
    // Selected payload validation can take time on a large route. Recheck the
    // retained dependency sources immediately afterward, before deleting files.
    int checked=0;
    for(auto it=fresh.referenceHashes.cbegin();it!=fresh.referenceHashes.cend();++it) {
        bool cancelled=false; QString error;
        const QFileInfo info(it.key());
        if(info.isSymLink() || info.isJunction()) { result.errors<<"A dependency source became linked. Rescan before cleaning."; return result; }
        const QByteArray hash=readFile(it.key(),nullptr,[&](double value,const QString &message){
            return notify(progress,0.65+0.05*(checked+value)/qMax(1,fresh.referenceHashes.size()),"Rechecking dependencies: "+message);
        },0,1,cancelled,error);
        if(cancelled) { result.cancelled=true; return result; }
        if(hash!=it.value()) { result.errors<<"A dependency source changed during validation: "+it.key(); return result; }
        ++checked;
    }
    if(!replacements.isEmpty()) {
        // Stage each descriptor on disk, bounding memory to one terrain file.
        // Old textures remain until ALL descriptors commit and a fresh graph
        // confirms they are unreferenced. A crash/cancel before that is safe:
        // both old and canonical families still exist.
        QTemporaryDir staging(QDir::tempPath()+"/tsre-pack-XXXXXX");
        if(!staging.isValid()) { result.errors<<"Cannot stage terrain reference updates."; return result; }
        QMap<QString,QString> staged;
        int index=0;
        for(auto it=fresh.referenceHashes.cbegin();it!=fresh.referenceHashes.cend();++it) {
            if(QFileInfo(it.key()).suffix().compare("t",Qt::CaseInsensitive)!=0) continue;
            if(!notify(progress,0.70,"Staging terrain references: "+QFileInfo(it.key()).fileName())) { result.cancelled=true; return result; }
            QFile input(it.key()); QByteArray output; QSet<QString> changed;
            if(!input.open(QIODevice::ReadOnly) || input.size()>MaximumReferenceBytes) {
                result.errors<<"Cannot read terrain descriptor: "+it.key(); return result;
            }
            const QByteArray original=input.readAll();
            if(QCryptographicHash::hash(original,QCryptographicHash::Sha256)!=it.value()
                    || !TerrainTextureRewrite::rewrite(original,replacements,output,changed)) {
                result.errors<<"Terrain descriptor changed or cannot be safely rewritten: "+it.key(); return result;
            }
            if(changed.isEmpty()) continue;
            const QString stage=staging.filePath(QString::number(index++)); QFile file(stage);
            if(!file.open(QIODevice::WriteOnly) || file.write(output)!=output.size() || !file.flush()) {
                result.errors<<"Cannot stage terrain descriptor: "+it.key(); return result;
            }
            staged.insert(it.key(),stage);
        }
        index=0;
        if(!verifyFamilies(0.75)) return result;
        for(auto it=staged.cbegin();it!=staged.cend();++it) {
            if(!notify(progress,0.75+0.07*index/qMax(1.0,double(staged.size())),"Updating terrain references: "+QFileInfo(it.key()).fileName())) {
                result.cancelled=true; return result;
            }
            bool cancelled=false; QString error;
            if(!safeFile(fresh.root,QDir(fresh.root).relativeFilePath(it.key()))
                    || readFile(it.key(),nullptr,{},0,0,cancelled,error)!=fresh.referenceHashes.value(it.key())) {
                result.errors<<"Terrain descriptor changed before commit: "+it.key(); return result;
            }
            QFile stage(it.value()); QSaveFile output(it.key()); output.setDirectWriteFallback(false);
            if(!stage.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
                result.errors<<"Cannot commit terrain descriptor: "+it.key(); return result;
            }
            while(!stage.atEnd()) {
                const QByteArray bytes=stage.read(1024*1024);
                if(bytes.isEmpty() || output.write(bytes)!=bytes.size()) {
                    result.errors<<"Failed writing terrain descriptor: "+it.key(); return result;
                }
            }
            if(!output.commit()) { result.errors<<"Failed committing terrain descriptor: "+it.key(); return result; }
            result.rewritten<<QDir(fresh.root).relativeFilePath(it.key()); ++index;
        }
        const Report after=scan(fresh.root,fresh.installationRoot,fresh.options,
            [&](double value,const QString &message){ return notify(progress,0.82+0.10*value,"Verifying packed dependencies: "+message); });
        if(after.cancelled) { result.cancelled=true; return result; }
        QSet<QString> orphaned;
        for(const Removal &removal:after.removals) if(removal.related.isEmpty()) orphaned.insert(removal.path);
        if(!after.complete) { result.errors<<"Packed dependency scan is incomplete; no textures removed."; return result; }
        for(const QString &path:deletions) if(!orphaned.contains(path)) {
            result.errors<<"File remains referenced after packing; no selected files removed: "+path; return result;
        }
        if(!verifyFamilies(0.92)) return result;
    }
    for(int i=0;i<deletions.size();++i) {
        const QString &path=deletions[i];
        if(!notify(progress,0.92+0.07*double(i)/deletions.size(),"Removing: "+path)) { result.cancelled=true; break; }
        if(!safeFile(fresh.root,path)) { result.errors<<"Unsafe removal path: "+path; break; }
        bool cancelled=false; QString error;
        const QByteArray hash=readFile(QDir(fresh.root).filePath(path),nullptr,
            [&](double,const QString &message){ return notify(progress,0.92+0.07*double(i)/deletions.size(),"Verifying selected payload: "+message); },
            0,0,cancelled,error);
        if(cancelled) { result.cancelled=true; break; }
        if(hash!=removalHashes.value(path)) { result.errors<<"Selected file changed before removal: "+path; break; }
        if(!safeFile(fresh.root,path) || !QFile::remove(QDir(fresh.root).filePath(path))) {
            result.errors<<"Could not remove: "+path; break;
        }
        result.removed<<path;
        result.bytes+=removalSizes.value(path);
    }
    if(!result.cancelled && result.errors.isEmpty()) notify(progress,1,"Selected cleanup complete");
    return result;
}

QString operationLockPath(const QString &root) {
    const QString id=QString::fromLatin1(QCryptographicHash::hash(key(root).toUtf8(),QCryptographicHash::Sha256).toHex());
    return QDir(QDir::tempPath()).filePath("tsre-route-operation-"+id+".lock");
}

Result clean(const Report &reviewed, const QStringList &selected, const Progress &progress) {
    Result result;
    result.completedAtUtc=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    if(!reviewed.complete || selected.isEmpty()) {
        result.errors<<"A complete scan and selected removal items are required."; return result;
    }
    QLockFile lock(operationLockPath(reviewed.root));
    if(!lock.tryLock(0)) { result.errors<<"Another route file operation is already running for this route."; return result; }
    result=cleanImpl(reviewed,selected,[&](double value,const QString &message){ return notify(progress,0.94*value,message); });
    // Measure the whole route after success, cancellation or a partial failure.
    // Counting metadata avoids another expensive content/hash scan, and includes
    // descriptor growth/shrinkage instead of equating deletion bytes with savings.
    if(result.beforeMeasured) {
        qint64 total=0; int count=0;
        const QFileInfo rootInfo(reviewed.root);
        if(!rootInfo.isDir() || rootInfo.isSymLink() || rootInfo.isJunction()
                || rootInfo.canonicalFilePath().compare(reviewed.root,Qt::CaseInsensitive)!=0 || !QDir(reviewed.root).isReadable())
            result.measurementError="Route folder changed or cannot be read for size measurement.";
        QDirIterator inventory(reviewed.root,QDir::Files|QDir::Dirs|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot,
                               QDirIterator::Subdirectories);
        while(result.measurementError.isEmpty() && inventory.hasNext()) {
            const QFileInfo info(inventory.next());
            if(info.isSymLink() || info.isJunction() || !inside(reviewed.root,info.canonicalFilePath())) {
                result.measurementError="Linked or unsafe path prevents a complete route-size measurement."; break;
            }
            if(info.isDir()) {
                if(!QDir(info.absoluteFilePath()).isReadable()) { result.measurementError="An unreadable folder prevents complete size measurement."; break; }
            } else if(info.isFile()) {
                if(info.size()<0 || total>std::numeric_limits<qint64>::max()-info.size()) {
                    result.measurementError="Route size exceeds the supported measurement range."; break;
                }
                total+=info.size();
            } else { result.measurementError="An unrecognized route entry prevents complete size measurement."; break; }
            ++count;
            if(!notify(progress,0.94+0.05*qMin(1.0,double(count)/qMax(1,reviewed.files)),"Measuring route size: "+info.fileName())) {
                result.measurementError="Route-size measurement cancelled."; result.cancelled=true; break;
            }
            if(count>MaximumFiles) { result.measurementError="Route-size measurement exceeds the entry limit."; break; }
        }
        if(!QFileInfo(reviewed.root).isDir()) result.measurementError="Route folder disappeared during size measurement.";
        if(result.measurementError.isEmpty()) {
            result.afterBytes=total; result.netBytes=result.beforeBytes-result.afterBytes; result.sizeMeasured=true;
        }
    }
    result.completedAtUtc=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    if(result.sizeMeasured && !result.cancelled && result.errors.isEmpty()) notify(progress,1,"Deletion and route-size measurement complete");
    return result;
}

QString summary(const Report &report) {
    QMap<QString,qint64> sizes;
    QMap<QString,int> counts;
    for(const Removal &removal:report.removals) {
        sizes[removal.category]+=removal.bytes;
        counts[removal.category]+=qMax(1,removal.related.size());
    }
    QStringList categories;
    for(auto it=sizes.cbegin();it!=sizes.cend();++it)
        categories<<QString("%1: %2 files, %3 MiB").arg(it.key()).arg(counts[it.key()]).arg(double(it.value())/(1024*1024),0,'f',2);
    return QString("Files scanned: %1\nRequired/protected by dependency graph: %2\nRemoval entries: %3 (%4 MiB)\nIdentical TERRTEX groups: %5\nWarnings: %6\n\n%7\n\n%8")
        .arg(report.files).arg(report.required).arg(report.removals.size())
        .arg(double(report.removableBytes)/(1024*1024),0,'f',2).arg(report.duplicateGroups)
        .arg(report.warnings.size())
        .arg(categories.join('\n'))
        .arg(report.cancelled?"Scan cancelled; cleanup disabled.":(report.complete?"Scan complete. Review and select files to remove.":"Scan incomplete; all cleanup disabled until the problems are resolved."));
}

QMap<QString,QStringList> warningGroups(const QStringList &warnings) {
    QMap<QString,QStringList> groups;
    for(const QString &warning:warnings) {
        const QString text=QString(warning).replace('\\','/').toLower();
        const QString target=text.section(" -> ",-1).trimmed();
        const QString suffix=QFileInfo(target).suffix();
        QString category="Other dependencies";
        if(text.contains("potentially affected assets") || text.contains("cannot be safely inspected"))
            category="Dependency inspection / protected assets";
        else if(text.contains("/trains/") || text.contains("activities/") || text.contains("services/")
                || text.contains("traffic/") || text.contains("paths/")
                || suffix=="eng" || suffix=="wag" || suffix=="con" || suffix=="srv" || suffix=="trf" || suffix=="act" || suffix=="pat" || suffix=="cvf")
            category="Rolling stock / activities";
        else if(text.contains("envfiles/") || suffix=="env") category="Environment / weather";
        else if(text.contains("sound/") || text.contains("ssource.dat") || suffix=="sms" || suffix=="wav") category="Sound";
        else if(text.contains("osm_data/") || text.contains("terrain_maps/") || suffix=="gpkg" || suffix=="geojson" || suffix=="pbf") category="Editing geodata / caches";
        else if(text.contains("tiles/") || suffix=="raw") category="Terrain data";
        else if(text.contains("shapes/") || text.contains("world/") || suffix=="s" || suffix=="sd" || suffix=="gltf" || suffix=="glb")
            category="Scenery / shapes";
        else if(textureType(suffix)) category="Textures";
        groups[category].append(warning);
    }
    return groups;
}

QString sizeSummary(const Result &result) {
    if(!result.sizeMeasured)
        return "Net route savings unavailable. "+(result.measurementError.isEmpty()?QString("Route totals were not measured."):result.measurementError);
    return QString("Route before: %1 MiB; after: %2 MiB; net %3: %4 MiB (%5 bytes).")
        .arg(double(result.beforeBytes)/(1024*1024),0,'f',3)
        .arg(double(result.afterBytes)/(1024*1024),0,'f',3)
        .arg(result.netBytes>=0?"reduction":"growth")
        .arg(qAbs(double(result.netBytes))/(1024*1024),0,'f',3)
        .arg(result.netBytes>=0?result.netBytes:-result.netBytes);
}

QString auditReport(const Report &report, const QStringList &selected, const Result *result) {
    QStringList lines;
    lines<<"PACK FOR DISTRO — REVIEW / DELETION REPORT"
        <<"Generated (UTC): "+QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
        <<"Scan (UTC): "+report.scannedAtUtc<<"Route folder: "+report.root
        <<"Installation folder: "+report.installationRoot
        <<"Scan signature: "+QString::fromLatin1(report.signature.toHex())
        <<QString("Options: preserve catalogs=%1; consolidate TERRTEX=%2; include editing caches=%3")
            .arg(report.options.keepEditorAssets?"on":"off").arg(report.options.consolidateTerrtex?"on":"off")
            .arg(report.options.includeEditorCaches?"on":"off")
        <<""<<"SCAN SUMMARY"<<summary(report)<<""<<"BLOCKING PROBLEMS";
    lines<<(report.problems.isEmpty()?QString("None"):report.problems.join('\n'));
    lines<<""<<"WARNINGS — UNRELATED CLEANUP ALLOWED";
    const auto grouped=warningGroups(report.warnings);
    if(grouped.isEmpty()) lines<<"None";
    for(auto it=grouped.cbegin();it!=grouped.cend();++it)
        lines<<QString("%1 (%2)").arg(it.key()).arg(it->size())<<it->join('\n')<<"";
    lines<<"NOTICES / TERRTEX GROUPS"<<(report.notices.isEmpty()?QString("None"):report.notices.join('\n'))
        <<""<<"REVIEWED CANDIDATES / SELECTION";
    QSet<QString> checked(selected.cbegin(),selected.cend());
    for(const Removal &removal:report.removals) {
        if(result && !checked.contains(removal.path)) continue;
        lines<<QString("[%1] %2 — %3 — %4 bytes")
            .arg(checked.contains(removal.path)?"SELECTED":"not selected").arg(removal.path).arg(removal.category).arg(removal.bytes)
            <<"Reason: "+removal.reason;
        if(removal.related.isEmpty()) lines<<"Candidate file: "+removal.path;
        else for(qsizetype i=0;i<removal.related.size();++i)
            lines<<"Candidate duplicate: "+removal.related[i]+"; retained replacement: "+removal.keepers[i];
    }
    if(result) {
        lines<<""<<"ACTUAL DELETION RESULT"
            <<"Finished (UTC): "+result->completedAtUtc
            <<QString("Status: %1").arg(result->cancelled?"Cancelled":(result->errors.isEmpty()?"Completed":"Stopped with errors"))
            <<QString("Deleted files: %1; deleted file bytes: %2").arg(result->removed.size()).arg(result->bytes)
            <<sizeSummary(*result);
        if(result->sizeMeasured)
            lines<<QString("Measured route bytes before: %1; after: %2; net reduction (signed): %3")
                .arg(result->beforeBytes).arg(result->afterBytes).arg(result->netBytes);
        lines<<"Sizes are logical file bytes, measured before report export; archive compression and allocated disk space can differ."
            <<""<<"DELETED FILES"<<(result->removed.isEmpty()?QString("None"):result->removed.join('\n'))
            <<""<<"UPDATED TERRAIN DESCRIPTORS"<<(result->rewritten.isEmpty()?QString("None"):result->rewritten.join('\n'))
            <<""<<"ERRORS"<<(result->errors.isEmpty()?QString("None"):result->errors.join('\n'));
    } else lines<<""<<"Review only. This section does not record a deletion operation.";
    return lines.join('\n')+'\n';
}
}

// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "StaticCopy.h"
#include "RouteDistro.h"
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QQueue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStringConverter>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>
#include <QtEndian>
#include <algorithm>

namespace StaticCopy {
namespace {
constexpr qint64 MaxText=256ll*1024*1024;
bool report(const Progress &progress,double fraction,const QString &message) {
    return !progress || progress(std::clamp(fraction,0.0,1.0),message);
}
QString norm(QString path) { return QDir::cleanPath(path.replace('\\','/')); }
bool inside(const QString &root,const QString &path) { return path.startsWith(root+'/',Qt::CaseInsensitive); }
bool safeRelative(const QString &path) {
    return !path.isEmpty() && !QDir::isAbsolutePath(path) && !path.contains(':') && !path.contains('\\')
        && !path.contains(QChar(0)) && norm(path)==path && path!=".." && !path.startsWith("../");
}
bool ordinary(const QString &path) {
    QFileInfo info(path);
    if(!info.isFile() || info.isSymLink() || info.isJunction()) return false;
    QDir parent=info.absoluteDir();
    for(;;) {
        const QFileInfo folder(parent.absolutePath());
        if(folder.isSymLink() || folder.isJunction()) return false;
        if(!parent.cdUp()) break;
    }
    return true;
}
bool safeTarget(const QString &root,const QString &relative) {
    if(!safeRelative(relative)) return false;
    const QString path=QDir(root).filePath(relative);
    QFileInfo current(path);
    if(current.exists() && (!current.isFile() || current.isSymLink() || current.isJunction())) return false;
    QString parent=QFileInfo(path).absolutePath();
    while(inside(root,parent)) {
        const QFileInfo folder(parent);
        if(folder.exists() && (!folder.isDir() || folder.isSymLink() || folder.isJunction())) return false;
        const QString next=QFileInfo(parent).absolutePath();
        if(next==parent) return false;
        parent=next;
    }
    return parent.compare(root,Qt::CaseInsensitive)==0;
}
QByteArray hashFile(const QString &path,const Progress &progress,double fraction,bool &cancelled,QString &error,double share=0) {
    QFile input(path); QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!ordinary(path) || !input.open(QIODevice::ReadOnly)) { error="Cannot safely read: "+path; return {}; }
    const qint64 size=input.size(); qint64 done=0;
    while(!input.atEnd()) {
        const QByteArray bytes=input.read(1024*1024);
        if(bytes.isEmpty()) { error="Read failed: "+path; return {}; }
        hash.addData(bytes); done+=bytes.size();
        if(!report(progress,fraction+share*double(done)/qMax(qint64(1),size),"Checking: "+QFileInfo(path).fileName())) { cancelled=true; return {}; }
    }
    if(done!=size || input.error()!=QFileDevice::NoError) { error="File changed or could not be read: "+path; return {}; }
    return hash.result();
}
bool textFile(const QByteArray &bytes,QString &text) {
    if(bytes.startsWith(QByteArray::fromHex("fffe"))) {
        if(bytes.size()%2) return false;
        QStringDecoder decoder(QStringDecoder::Utf16LE,QStringConverter::Flag::Stateless);
        text=decoder(bytes); return !decoder.hasError() && text.isValidUtf16();
    }
    if(bytes.startsWith(QByteArray::fromHex("feff")) || bytes.startsWith(QByteArray::fromHex("efbbbf"))) return false;
    text=QString::fromLatin1(bytes); return !text.contains(QChar(0));
}
bool complete(const QString &text) {
    int depth=0; bool quoted=false,escaped=false;
    for(QChar c:text) {
        if(c.isNull()) return false;
        if(quoted) {
            if(escaped) escaped=false;
            else if(c=='\\') escaped=true;
            else if(c=='"') quoted=false;
        } else if(c=='"') quoted=true;
        else if(c=='(') ++depth;
        else if(c==')' && --depth<0) return false;
    }
    return !quoted && depth==0;
}
struct RefBlock { QString type,text; };
QVector<RefBlock> refBlocks(const QString &text) {
    QVector<RefBlock> out; int depth=0; bool quoted=false,escaped=false;
    QString token; qsizetype tokenStart=0, start=0; QString type;
    for(qsizetype i=0;i<text.size();++i) {
        const QChar c=text[i];
        if(quoted) { if(escaped) escaped=false; else if(c=='\\') escaped=true; else if(c=='"') quoted=false; continue; }
        if(c=='"') { quoted=true; continue; }
        if(depth==0 && (c.isLetter() || c=='_')) {
            tokenStart=i;
            while(i+1<text.size() && (text[i+1].isLetterOrNumber() || text[i+1]=='_')) ++i;
            token=text.mid(tokenStart,i-tokenStart+1).toLower(); continue;
        }
        if(c=='(') { if(depth==0) { start=tokenStart; type=token; } ++depth; }
        else if(c==')' && --depth==0) {
            if(type!="comment" && type!="skip") out.append({type,text.mid(start,i-start+1)});
            token.clear();
        } else if(depth==0 && !c.isSpace()) token.clear();
    }
    return out;
}
QString field(const QString &text,const QString &name) {
    // Read immediate fields only; quoted descriptions and Comment/Skip blocks
    // can contain text that looks like a Filename/Class/Description field.
    const qsizetype open=text.indexOf('('), close=text.lastIndexOf(')');
    if(open<0 || close<=open) return {};
    for(const auto &block:refBlocks(text.mid(open+1,close-open-1))) {
        if(block.type.compare(name,Qt::CaseInsensitive)!=0) continue;
        const qsizetype valueStart=block.text.indexOf('(')+1;
        const QString value=block.text.mid(valueStart,block.text.lastIndexOf(')')-valueStart).trimmed();
        if(!value.startsWith('"')) return value.section(QRegularExpression("\\s"),0,0);
        bool escaped=false;
        for(qsizetype i=1;i<value.size();++i) {
            if(!escaped && value[i]=='"') return value.mid(1,i-1);
            if(!escaped && value[i]=='\\') escaped=true; else escaped=false;
        }
        return {};
    }
    return {};
}
bool texture(const QString &suffix) {
    static const QSet<QString> values{"ace","dds","png","jpg","jpeg","bmp","tga","gif","webp","tif","tiff"};
    return values.contains(suffix);
}
void uriValues(const QJsonValue &value,QStringList &uris) {
    if(value.isObject()) {
        const auto object=value.toObject();
        for(auto it=object.constBegin();it!=object.constEnd();++it) {
            if(it.key()=="uri" && it->isString()) uris<<it->toString(); else uriValues(it.value(),uris);
        }
    } else if(value.isArray()) for(const auto &entry:value.toArray()) uriValues(entry,uris);
}
bool gltfUris(const QString &path,QStringList &uris,QString &error) {
    QFile input(path);
    if(!input.open(QIODevice::ReadOnly) || input.size()>MaxText) { error="Cannot inspect glTF: "+path; return false; }
    QByteArray bytes=input.readAll();
    if(input.error()!=QFileDevice::NoError) { error="Could not read complete glTF data: "+path; return false; }
    if(QFileInfo(path).suffix().compare("glb",Qt::CaseInsensitive)==0) {
        if(bytes.size()<20 || bytes.left(4)!="glTF" || qFromLittleEndian<quint32>(bytes.constData()+4)!=2
                || qFromLittleEndian<quint32>(bytes.constData()+8)!=quint32(bytes.size())
                || bytes.mid(16,4)!="JSON" || qFromLittleEndian<quint32>(bytes.constData()+12)>quint32(bytes.size()-20)) {
            error="Invalid GLB: "+path; return false;
        }
        bytes=bytes.mid(20,qFromLittleEndian<quint32>(bytes.constData()+12));
    }
    QJsonParseError parse; const auto document=QJsonDocument::fromJson(bytes,&parse);
    if(parse.error!=QJsonParseError::NoError || !document.isObject()) { error="Invalid glTF JSON: "+path; return false; }
    uriValues(document.object(),uris); return true;
}
bool refOutput(const Destination &destination,const Plan &plan,const QString &name,const QString &category,QByteArray &output,QString &error) {
    for(const QString &value:QStringList{name,category})
        if(value.trimmed().isEmpty() || value.size()>512 || value.contains(QRegularExpression("[\\x00-\\x1f\"\\\\]"))) {
            error="Name and category must be nonempty and cannot contain quotes, backslashes or control characters."; return false;
        }
    if(category.trimmed().startsWith('#')) { error="Categories beginning with # are reserved for editor tools."; return false; }
    if(!complete(plan.source.settings)) { error="Source REF settings are incomplete."; return false; }
    QString existing;
    if(destination.refExists && textFile(destination.refBytes,existing)) for(const auto &block:refBlocks(existing)) {
        if((block.type=="static" || block.type=="collideobject" || block.type=="gantry")
                && field(block.text,"Class").compare(category.trimmed(),Qt::CaseInsensitive)==0
                && field(block.text,"Description").compare(name.trimmed(),Qt::CaseInsensitive)==0) {
            error="This name already exists in the destination category. Choose a different name."; return false;
        }
    }
    QString entry="\r\n"+plan.source.type+" (\r\n\tClass ( "+quote(category.trimmed())+" )\r\n";
    for(const QString &shape:plan.shapeNames) entry+="\tFilename ( "+quote(shape)+" )\r\n";
    entry+="\tDescription ( "+quote(name.trimmed())+" )\r\n"+plan.source.settings+"\r\n)\r\n";
    if(destination.refBytes.startsWith(QByteArray::fromHex("fffe")) || !destination.refExists) {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        output=destination.refExists?destination.refBytes:QByteArray::fromHex("fffe")+encoder(u"SIMISA@@@@@@@@@@JINX0r1t______\r\n");
        output+=encoder(entry);
    } else {
        if(QString::fromLatin1(entry.toLatin1())!=entry) { error="The destination REF uses legacy 8-bit encoding and cannot represent this name/category."; return false; }
        output=destination.refBytes+entry.toLatin1();
    }
    return true;
}
}
QString quote(const QString &value) { QString copy=value; copy.replace('\\',"\\\\").replace('"',"\\\""); return '"'+copy+'"'; }

Destination validateWorkingFolder(const QString &root,bool probeWrite) {
    Destination out; const QFileInfo info(root); out.root=info.canonicalFilePath();
    if(out.root.isEmpty() || !info.isDir() || info.isSymLink() || info.isJunction() || !info.isWritable()) {
        out.errors<<"Choose an existing writable folder without links."; return out;
    }
    // Validate ancestors as well as the chosen directory, including junctions.
    QDir parent=info.absoluteDir();
    do {
        const QFileInfo folder(parent.absolutePath());
        if(folder.isSymLink() || folder.isJunction()) { out.errors<<"Destination folders cannot use links or junctions."; return out; }
    } while(parent.cdUp());
    if(probeWrite) {
        QTemporaryFile probe(QDir(out.root).filePath(".tsre-copy-write-XXXXXX"));
        if(!probe.open()) { out.errors<<"The destination folder is not writable."; return out; }
    }
    out.name=info.fileName(); out.workingFolder=true; out.valid=true; return out;
}

Destination validateDestination(const QString &root,bool probeWrite) {
    Destination out=validateWorkingFolder(root,probeWrite);
    out.workingFolder=false;
    if(!out.valid) return out;
    out.valid=false;
    const QFileInfo info(out.root);
    const auto descriptors=QDir(out.root).entryInfoList({"*.trk"},QDir::Files);
    if(descriptors.size()!=1 || !ordinary(descriptors.front().filePath())) { out.errors<<"The route folder must contain one readable .trk descriptor."; return out; }
    out.descriptor=descriptors.front().filePath(); QFile trk(out.descriptor); QString text;
    if(!trk.open(QIODevice::ReadOnly) || trk.size()>MaxText) { out.errors<<"Cannot read route descriptor."; return out; }
    const QByteArray bytes=trk.readAll();
    if(trk.error()!=QFileDevice::NoError) { out.errors<<"Cannot read complete route descriptor."; return out; }
    if(!textFile(bytes,text) || !text.startsWith("SIMISA") || !complete(text)) {
        out.errors<<"Invalid or unsupported route descriptor."; return out;
    }
    QString routeText; int routeCount=0;
    for(const auto &block:refBlocks(text)) if(block.type=="tr_routefile") { routeText=block.text; ++routeCount; }
    if(routeCount!=1) { out.errors<<"The descriptor must contain one Tr_RouteFile block."; return out; }
    out.descriptorHash=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
    QString stem=field(routeText,"FileName"); if(stem.isEmpty()) stem=descriptors.front().completeBaseName();
    if(!safeRelative(stem) || QFileInfo(stem).fileName()!=stem) { out.errors<<"Route descriptor has an unsafe REF filename."; return out; }
    out.name=field(routeText,"Name"); if(out.name.isEmpty()) out.name=info.fileName();
    out.refPath=QDir(out.root).filePath(stem+".ref");
    if(!safeTarget(out.root,stem+".ref")) { out.errors<<"Destination REF path is linked or unsafe."; return out; }
    out.refExists=QFileInfo::exists(out.refPath);
    if(out.refExists) {
        if(!QFileInfo(out.refPath).isWritable()) { out.errors<<"Destination REF is not writable."; return out; }
        QFile ref(out.refPath);
        if(!ref.open(QIODevice::ReadOnly) || ref.size()>MaxText) { out.errors<<"Cannot read destination REF."; return out; }
        out.refBytes=ref.readAll();
        if(ref.error()!=QFileDevice::NoError) { out.errors<<"Cannot read complete destination REF."; return out; }
        if(!textFile(out.refBytes,text) || !complete(text)) { out.errors<<"Destination REF has incomplete data or unsupported encoding."; return out; }
        out.refHash=QCryptographicHash::hash(out.refBytes,QCryptographicHash::Sha256);
        for(const auto &block:refBlocks(text)) {
            const QString category=field(block.text,"Class");
            if(!category.isEmpty() && !category.startsWith('#')) out.categories<<category;
        }
        out.categories.removeDuplicates(); out.categories.sort();
    }
    out.valid=true; return out;
}

Plan prepare(const Source &source,const Destination &destination,const Progress &progress) {
    Plan plan; plan.source=source;
    plan.destination=destination.workingFolder?validateWorkingFolder(destination.root):validateDestination(destination.root);
    if(!plan.destination.valid) { plan.errors=plan.destination.errors; return plan; }
    const QString sourceRoot=QFileInfo(source.routeRoot).canonicalFilePath();
    if(sourceRoot.isEmpty() || sourceRoot.compare(plan.destination.root,Qt::CaseInsensitive)==0
            || inside(sourceRoot,plan.destination.root) || inside(plan.destination.root,sourceRoot)) {
        plan.errors<<"Choose a separate, nonoverlapping destination folder."; return plan;
    }
    if(plan.destination.workingFolder) {
        QString stem=source.shapes.isEmpty()?QString("Object"):QFileInfo(norm(source.shapes.first())).completeBaseName();
        stem.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");
        stem=stem.left(100).trimmed();
        while(stem.endsWith('.') || stem.endsWith(' ')) stem.chop(1);
        if(stem.isEmpty()) stem="Object";
        if(QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)",QRegularExpression::CaseInsensitiveOption).match(stem).hasMatch()) stem="Object_"+stem;
        plan.destination.workingRoot=plan.destination.root;
        QString leaf=stem; int suffix=2;
        while(QFileInfo::exists(QDir(plan.destination.workingRoot).filePath(leaf)) && suffix<=10000) leaf=stem+'_'+QString::number(suffix++);
        if(!safeTarget(plan.destination.workingRoot,leaf) || QFileInfo::exists(QDir(plan.destination.workingRoot).filePath(leaf))) {
            plan.errors<<"Cannot choose a new object folder in the working folder."; return plan;
        }
        plan.destination.root=QDir(plan.destination.workingRoot).filePath(leaf); plan.destination.name=leaf;
    }
    if(source.type.compare("Static",Qt::CaseInsensitive)!=0 && source.type.compare("CollideObject",Qt::CaseInsensitive)!=0
            && source.type.compare("Gantry",Qt::CaseInsensitive)!=0) { plan.errors<<"Copy To supports Static Objects only."; return plan; }
    QQueue<int> queue; QMap<QString,int> targets;
    const auto add=[&](const QString &path,const QString &relative) {
        if(plan.cancelled || !plan.errors.isEmpty()) return;
        const QString canonical=QFileInfo(path).canonicalFilePath();
        if(!ordinary(path) || !safeTarget(plan.destination.root,relative)) { plan.errors<<"Unsafe or missing resource: "+path+" -> "+relative; return; }
        const QString id=relative.toCaseFolded();
        if(targets.contains(id) && plan.files[targets[id]].source==canonical) return;
        bool cancelled=false; QString error;
        const QByteArray hash=hashFile(path,progress,0.05,cancelled,error);
        if(cancelled) { plan.cancelled=true; return; }
        if(hash.isEmpty()) { plan.errors<<error; return; }
        if(targets.contains(id)) {
            if(plan.files[targets[id]].hash!=hash) plan.errors<<"Two source resources require different content at: "+relative;
            return;
        }
        File file; file.source=canonical; file.relative=relative; file.hash=hash; file.bytes=QFileInfo(path).size();
        const QString target=QDir(plan.destination.root).filePath(relative);
        if(QFileInfo::exists(target)) {
            const QByteArray existing=hashFile(target,progress,0.08,cancelled,error);
            if(cancelled) { plan.cancelled=true; return; }
            if(existing!=hash) { plan.errors<<"Destination filename conflict (not overwritten): "+relative; return; }
            file.reuse=true;
        } else {
            QString parent=QFileInfo(target).absolutePath();
            while(inside(plan.destination.root,parent) && !QFileInfo::exists(parent)) parent=QFileInfo(parent).absolutePath();
            if(parent==plan.destination.root && plan.destination.workingFolder) parent=plan.destination.workingRoot;
            if(!QFileInfo(parent).isWritable()) { plan.errors<<"Destination resource folder is not writable: "+relative; return; }
        }
        if(plan.files.size()>=100000) { plan.errors<<"Resource dependency limit exceeded."; return; }
        targets.insert(id,plan.files.size()); queue.enqueue(plan.files.size()); plan.files.append(file); plan.bytes+=file.bytes;
    };
    for(QString shape:source.shapes) {
        shape=norm(shape);
        const QString suffix=QFileInfo(shape).suffix().toLower();
        if(suffix!="s" && suffix!="gltf" && suffix!="glb") { plan.errors<<"Unsupported Static model format: "+shape; break; }
        if((suffix=="gltf" || suffix=="glb") && source.type.compare("Static",Qt::CaseInsensitive)!=0) {
            plan.errors<<"glTF Copy To currently requires the Static object type; collision/gantry glTF placement is not supported."; break;
        }
        const QString path=QDir(source.shapeRoot).filePath(shape);
        const QString target=safeRelative(shape)?shape:QFileInfo(shape).fileName();
        if(!safeRelative(target)) { plan.errors<<"Unsafe model filename: "+shape; continue; }
        add(path,"SHAPES/"+target); plan.shapeNames<<target;
    }
    if(plan.shapeNames.isEmpty()) { plan.errors<<"The object has no model filename."; return plan; }
    if(plan.cancelled || !plan.errors.isEmpty()) return plan;
    QStringList textureFolders{""};
    static const QSet<QString> seasons{"summer","spring","autumn","fall","winter","snow","wintersnow","springsnow","autumnsnow","fallsnow","night"};
    if(QFileInfo(source.textureRoot).isDir()) {
        QDirIterator dirs(source.textureRoot,QDir::Dirs|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
        while(dirs.hasNext()) {
            const QFileInfo folder(dirs.next());
            const QString relative=QDir(source.textureRoot).relativeFilePath(folder.filePath()); bool seasonal=true;
            for(const QString &part:relative.split('/')) if(!seasons.contains(part.toLower())) seasonal=false;
            if(seasonal && !folder.isSymLink() && !folder.isJunction()) textureFolders<<relative;
            if(!report(progress,0.08,"Finding seasonal textures: "+relative)) { plan.cancelled=true; return plan; }
        }
    }
    while(!queue.isEmpty() && plan.errors.isEmpty() && !plan.cancelled) {
        const File file=plan.files[queue.dequeue()]; const QString suffix=QFileInfo(file.source).suffix().toLower();
        const bool model=suffix=="s" || suffix=="gltf" || suffix=="glb";
        if(model) {
            const QString sd=QFileInfo(file.source).absolutePath()+'/'+QFileInfo(file.source).completeBaseName()+".sd";
            if(QFileInfo::exists(sd)) add(sd,QFileInfo(file.relative).path()+'/'+QFileInfo(sd).fileName());
            else if(suffix=="s") plan.warnings<<"No .sd companion; using model defaults: "+file.relative;
        }
        const bool gltf=suffix=="gltf" || suffix=="glb";
        // Native shapes load image resources; SD settings are copied opaquely.
        // Descriptive matrix/shape labels ending in .s are not dependencies.
        if(!model && suffix!="sms") continue;
        QStringList references; QString error;
        bool ok=gltf?gltfUris(file.source,references,error):RouteDistro::inspectAssetReferences(file.source,references,error,
            [&](double,const QString &message){ const bool keep=report(progress,0.10,message); plan.cancelled=!keep; return keep; });
        if(!ok) { if(!plan.cancelled) plan.errors<<error; break; }
        for(QString reference:references) {
            if(reference.startsWith("data:",Qt::CaseInsensitive)) continue;
            if(gltf) reference=QUrl::fromPercentEncoding(reference.toUtf8());
            reference.replace('\\','/');
            const QString ext=QFileInfo(reference).suffix().toLower();
            if(suffix=="s" && !texture(ext)) continue;
            if(!gltf && !texture(ext) && ext!="s" && ext!="sms" && ext!="wav" && ext!="bin") continue;
            if(QDir::isAbsolutePath(reference) || reference.contains(':')) { plan.errors<<"Absolute or remote resource paths cannot be transferred safely: "+reference; break; }
            if(texture(ext) && !gltf && suffix=="s") {
                bool found=false, baseFound=false;
                QStringList names{reference};
                if(ext=="ace" || ext=="dds") names<<reference.left(reference.size()-3)+(ext=="ace"?"dds":"ace");
                for(const QString &folder:textureFolders) for(const QString &name:names) {
                    const QString relative=norm("TEXTURES/"+(folder.isEmpty()?QString():folder+'/')+name);
                    const QString path=QDir(QDir(source.textureRoot).filePath(folder)).filePath(name);
                    if(QFileInfo::exists(path)) { add(path,relative); found=true; if(folder.isEmpty()) baseFound=true; }
                }
                if(!found) plan.errors<<"Missing texture: "+reference+" (model "+file.relative+")";
                else if(!baseFound) plan.warnings<<"Only seasonal variants exist for texture: "+reference;
            } else {
                const QString path=QDir(QFileInfo(file.source).absolutePath()).filePath(reference);
                const QString relative=norm(QFileInfo(file.relative).path()+'/'+reference);
                add(path,relative);
            }
            if(plan.cancelled || !plan.errors.isEmpty()) break;
        }
        if(!report(progress,0.12,"Resources gathered: "+QString::number(plan.files.size()))) { plan.cancelled=true; break; }
    }
    plan.errors.removeDuplicates(); plan.warnings.removeDuplicates(); plan.valid=plan.errors.isEmpty() && !plan.cancelled;
    return plan;
}

Result transfer(const Plan &plan,const QString &name,const QString &category,const Progress &progress) {
    Result result;
    if(!plan.valid) { result.errors<<"A complete resource review is required."; return result; }
    result.outputFolder=plan.destination.root;
    const bool working=plan.destination.workingFolder;
    QLockFile lock(RouteDistro::operationLockPath(working?plan.destination.workingRoot:plan.destination.root));
    if(!lock.tryLock(0)) { result.errors<<"Another route file operation is using this destination."; return result; }
    Destination current=working?plan.destination:validateDestination(plan.destination.root,true);
    if(working) {
        const auto parent=validateWorkingFolder(current.workingRoot,true);
        if(!parent.valid || parent.root!=current.workingRoot || current.root!=QDir(parent.root).filePath(current.name)
                || !safeTarget(parent.root,current.name) || QFileInfo::exists(current.root)) {
            result.errors<<"Working folder changed or the object folder already exists. Try Copy To again."; return result;
        }
    }
    if(!current.valid || current.root.compare(plan.destination.root,Qt::CaseInsensitive)!=0 || current.descriptorHash!=plan.destination.descriptorHash
            || current.refExists!=plan.destination.refExists || current.refHash!=plan.destination.refHash) {
        result.errors<<"Destination descriptor or REF changed. Review the transfer again."; return result;
    }
    QByteArray refBytes; QString error;
    if(!working && !refOutput(current,plan,name,category,refBytes,error)) { result.errors<<error; return result; }
    QTemporaryDir staging;
    if(!staging.isValid()) { result.errors<<"Cannot stage transfer resources."; return result; }
    qint64 done=0;
    for(int i=0;i<plan.files.size();++i) {
        const File &file=plan.files[i]; QFile input(file.source), staged(staging.filePath(QString::number(i)));
        if(!ordinary(file.source) || !input.open(QIODevice::ReadOnly) || !staged.open(QIODevice::WriteOnly)) { result.errors<<"Cannot stage: "+file.relative; return result; }
        QCryptographicHash hash(QCryptographicHash::Sha256); qint64 size=0;
        while(!input.atEnd()) {
            const QByteArray bytes=input.read(1024*1024);
            if(bytes.isEmpty() || staged.write(bytes)!=bytes.size()) { result.errors<<"Staging failed: "+file.relative; return result; }
            hash.addData(bytes); size+=bytes.size(); done+=bytes.size();
            if(!report(progress,0.15+0.45*double(done)/qMax(qint64(1),plan.bytes),"Staging and verifying: "+file.relative)) { result.cancelled=true; return result; }
        }
        if(input.error()!=QFileDevice::NoError || hash.result()!=file.hash || size!=file.bytes || !staged.flush()) { result.errors<<"Source changed or staging failed: "+file.relative; return result; }
    }
    QStringList created, folders;
    const auto rollback=[&](){
        QStringList remaining; qint64 retainedBytes=0;
        for(auto it=created.crbegin();it!=created.crend();++it) {
            QByteArray expected;
            for(const File &file:plan.files) if(file.relative==*it) { expected=file.hash; break; }
            bool cancelled=false; QString checkError;
            const QString path=QDir(current.root).filePath(*it);
            if(!safeTarget(current.root,*it) || hashFile(path,[&](double,const QString &message){ report(progress,0.98,"Rolling back transfer: "+message); return true; },0,cancelled,checkError)!=expected || !QFile::remove(path)) {
                result.errors<<"Rollback retained a changed or inaccessible newly copied file: "+*it;
                remaining<<*it; retainedBytes+=QFileInfo(path).size();
            }
        }
        result.copied=remaining; result.bytes=retainedBytes;
        for(auto it=folders.crbegin();it!=folders.crend();++it)
            if(inside(current.root,*it) || (working && *it==current.root)) QDir().rmdir(*it);
    };
    if(working) {
        if(!report(progress,0.60,"Creating object folder")) { result.cancelled=true; return result; }
        if(!QDir(current.workingRoot).mkdir(current.name)) { result.errors<<"Cannot create object folder: "+current.root; return result; }
        folders<<current.root;
    }
    done=0;
    for(int i=0;i<plan.files.size();++i) {
        const File &file=plan.files[i]; const QString target=QDir(current.root).filePath(file.relative);
        if(!report(progress,0.60+0.32*double(done)/qMax(qint64(1),plan.bytes),"Copying: "+file.relative)) { result.cancelled=true; rollback(); return result; }
        if(QFileInfo(current.root).canonicalFilePath()!=current.root || QFileInfo(current.root).isSymLink()
                || QFileInfo(current.root).isJunction() || !safeTarget(current.root,file.relative)) {
            result.errors<<"Destination path became unsafe: "+file.relative; rollback(); return result;
        }
        if(QFileInfo::exists(target)) {
            bool cancelled=false;
            const QByteArray hash=hashFile(target,progress,0.60+0.32*double(done)/qMax(qint64(1),plan.bytes),cancelled,error,
                0.32*double(file.bytes)/qMax(qint64(1),plan.bytes));
            if(cancelled || hash!=file.hash) { result.cancelled=cancelled; if(!cancelled) result.errors<<"Destination conflict: "+file.relative; rollback(); return result; }
            result.reused<<file.relative; done+=file.bytes; continue;
        }
        QString parent=QFileInfo(target).absolutePath(); QStringList missing;
        while(inside(current.root,parent) && !QFileInfo::exists(parent)) { missing.prepend(parent); parent=QFileInfo(parent).absolutePath(); }
        for(const QString &folder:missing) {
            if(!QDir().mkdir(folder)) { result.errors<<"Cannot create destination folder: "+folder; rollback(); return result; }
            folders<<folder;
        }
        QFile staged(staging.filePath(QString::number(i)));
        QTemporaryFile output(QDir(QFileInfo(target).absolutePath()).filePath(".tsre-copy-XXXXXX"));
        if(!staged.open(QIODevice::ReadOnly) || !output.open()) { result.errors<<"Cannot copy: "+file.relative; rollback(); return result; }
        qint64 written=0;
        while(!staged.atEnd()) {
            const QByteArray bytes=staged.read(1024*1024);
            if(bytes.isEmpty() || output.write(bytes)!=bytes.size()) { result.errors<<"Copy failed: "+file.relative; output.remove(); rollback(); return result; }
            written+=bytes.size();
            if(!report(progress,0.60+0.32*double(done+written)/qMax(qint64(1),plan.bytes),"Copying: "+file.relative)) { result.cancelled=true; output.remove(); rollback(); return result; }
        }
        if(!output.flush()) { result.errors<<"Destination flush failed: "+file.relative; output.remove(); rollback(); return result; }
        output.close();
        // QFile rename refuses an existing destination; never replace a resource
        // that another process created after our preflight check.
        if(!output.rename(target)) { result.errors<<"Destination changed or rename failed: "+file.relative; output.remove(); rollback(); return result; }
        output.setAutoRemove(false);
        created<<file.relative; result.copied<<file.relative; result.bytes+=file.bytes; done+=file.bytes;
    }
    done=0;
    for(const File &file:plan.files) {
        bool cancelled=false;
        if(hashFile(QDir(current.root).filePath(file.relative),progress,0.92+0.06*double(done)/qMax(qint64(1),plan.bytes),cancelled,error,
                0.06*double(file.bytes)/qMax(qint64(1),plan.bytes))!=file.hash) {
            result.cancelled=cancelled; if(!cancelled) result.errors<<"Destination verification failed: "+file.relative; rollback(); return result;
        }
        done+=file.bytes;
    }
    if(!report(progress,0.98,working?"Finishing object folder":"Updating destination REF")) { result.cancelled=true; rollback(); return result; }
    if(working) {
        const auto finalCheck=validateWorkingFolder(current.root);
        if(!finalCheck.valid || finalCheck.root!=current.root) { result.errors<<"Object folder changed during transfer."; rollback(); return result; }
        result.completed=true; result.outputFolder=current.root;
        report(progress,1,"Object resources copied to working folder"); return result;
    }
    const Destination finalCheck=validateDestination(current.root);
    if(!finalCheck.valid || finalCheck.root.compare(current.root,Qt::CaseInsensitive)!=0 || finalCheck.descriptorHash!=current.descriptorHash || finalCheck.refHash!=current.refHash || finalCheck.refExists!=current.refExists) {
        result.errors<<"Destination REF or descriptor changed; transfer cancelled."; rollback(); return result;
    }
    QSaveFile ref(current.refPath); ref.setDirectWriteFallback(false);
    if(!ref.open(QIODevice::WriteOnly) || ref.write(refBytes)!=refBytes.size() || !ref.commit()) { result.errors<<"Cannot update destination REF: "+ref.errorString(); rollback(); return result; }
    result.refUpdated=true; result.completed=true; result.outputFolder=current.root;
    report(progress,1,"Object resources and REF copied successfully"); return result;
}
}

// Explicit TERRTEX processing with atomic publication/checkpoints; no texture backups. GPL v3 or later.
#pragma once
#include "DdsWriter.h"
#include "DdsDecoder.h"
#include "SaveProgress.h"
#include <QCryptographicHash>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QLockFile>
#include <QMap>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QtEndian>
#include <functional>
#include <utility>

namespace TerrtexProduction {
using AceDecoder=std::function<bool(const QString &,QImage &,bool &,QString &)>;
using References=std::function<bool(const QMap<QString,QString> &,QString &)>;
struct Result { int converted=0, skipped=0, compressed=0, added=0, removed=0, resumed=0, reformatted=0; qint64 before=0, after=0; };
// Optional processing policy; the original overload remains a regression reference.
struct Processed {
    QByteArray bytes;
    QString suffix;
    bool changed=false, compressed=false, added=false, removed=false, reformatted=false;
};
struct Processor {
    QJsonObject options;
    std::function<QString(const QString &)> destination;
    std::function<bool(const QString &,QString &,QString &)> plan;
    std::function<bool(const QString &,const QByteArray &,Processed &,QString &)> transform;
};
inline bool write(const QString &path,const QByteArray &bytes,QString &error) {
    QSaveFile file(path); file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
        error="Cannot publish "+QFileInfo(path).fileName()+": "+file.errorString(); return false;
    }
    return true;
}
inline QByteArray hashFile(const QString &path) {
    QFile file(path); QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!file.open(QIODevice::ReadOnly) || !hash.addData(&file) || file.error()!=QFileDevice::NoError) return {};
    return hash.result().toHex();
}
inline QString destination(const QString &source) {
    return source.endsWith(".ace",Qt::CaseInsensitive)
        ? source.left(source.size()-4)+"_ace_dxt1.dds" : source;
}
inline QString formatDestination(const QString &source,const QString &suffix) {
    if(source.endsWith('.'+suffix,Qt::CaseInsensitive)) return source;
    return source.left(source.size()-4)+'.'+suffix;
}
inline bool safePath(const QString &root,const QString &relative) {
    if(relative.isEmpty() || QDir::isAbsolutePath(relative) || relative.contains(':')
            || relative.contains('\\') || relative!=QDir::cleanPath(relative)
            || relative==".." || relative.startsWith("../")) return false;
    const QFileInfo info(QDir(root).filePath(relative));
    const QString parent=QFileInfo(info.absolutePath()).canonicalFilePath();
    const QString canonical=QFileInfo(root).canonicalFilePath();
    return !canonical.isEmpty() && !info.isSymLink()
        && (parent.compare(canonical,Qt::CaseInsensitive)==0
            || parent.startsWith(canonical+'/',Qt::CaseInsensitive));
}
inline bool run(const QString &textureRoot,const QString &journalPath,
        const AceDecoder &decodeAce,const References &updateReferences,Result &result,QString &error,
        const Processor *processor=nullptr) {
    result={}; error.clear();
    const QString root=QFileInfo(textureRoot).canonicalFilePath();
    if(root.isEmpty()) { error="TERRTEX folder was not found."; return false; }
    if(!QDir().mkpath(QFileInfo(journalPath).absolutePath())) { error="Cannot create conversion journal folder."; return false; }
    QLockFile lock(journalPath+".lock");
    if(!lock.tryLock(0)) { error="A TERRTEX conversion is already active."; return false; }
    QJsonObject journal;
    QJsonArray files;
    int journalIndex=-1;
    const QString logPath=journalPath+".log";
    const auto saveJournal=[&]() {
        if(journalIndex>=0) {
            // Append a small checkpoint rather than rewriting a route-sized
            // manifest for each texture (which would make large routes quadratic).
            QFile log(logPath);
            const QByteArray line=QJsonDocument(QJsonObject{{"index",journalIndex},
                {"entry",files[journalIndex]}}).toJson(QJsonDocument::Compact)+'\n';
            if(!log.open(QIODevice::WriteOnly|QIODevice::Append) || log.write(line)!=line.size() || !log.flush()) {
                error="Cannot write conversion checkpoint."; return false;
            }
            return true;
        }
        return write(journalPath,QJsonDocument(journal).toJson(QJsonDocument::Compact),error);
    };
    bool resume=QFile::exists(journalPath);
    if(resume) {
        QFile file(journalPath); QJsonParseError parse;
        if(!file.open(QIODevice::ReadOnly)) { error="Cannot read conversion journal."; return false; }
        const auto document=QJsonDocument::fromJson(file.readAll(),&parse);
        journal=document.object();
        const int version=journal["version"].toInt();
        const bool acceptedVersion=version==(processor?3:1)
            || (processor && version==2 && journal["complete"].toBool());
        if(parse.error!=QJsonParseError::NoError || !acceptedVersion
                || journal["root"].toString().compare(root,Qt::CaseInsensitive)!=0
                || !journal["files"].isArray()) { error="Conversion journal is invalid or belongs to another route."; return false; }
        if(journal["complete"].toBool()) {
            if(!processor) {
                error="This route has already completed production DXT1 conversion. DXT1 is lossy and should only be run once.";
                return false;
            }
            file.close();
            if(QFile::exists(logPath) && !QFile::remove(logPath)) {
                error="Cannot clear completed processing checkpoints."; return false;
            }
            resume=false;
        }
        if(resume) {
            if(processor && journal["options"].toObject()!=processor->options) {
                error="Resume the interrupted operation with its original processing options."; return false;
            }
            files=journal["files"].toArray();
            if(QFile::exists(logPath)) {
                QFile log(logPath);
                if(!log.open(QIODevice::ReadWrite)) { error="Cannot read conversion checkpoints."; return false; }
                const QByteArray records=log.readAll();
                const qsizetype end=records.lastIndexOf('\n')+1;
                if(end<records.size() && !log.resize(end)) { error="Cannot repair interrupted checkpoint tail."; return false; }
                for(const QByteArray &line:records.left(end).split('\n')) {
                    if(line.isEmpty()) continue;
                    QJsonParseError recordError;
                    const auto record=QJsonDocument::fromJson(line,&recordError).object();
                    const int index=record["index"].toInt(-1);
                    const auto entry=record["entry"].toObject();
                    if(recordError.error!=QJsonParseError::NoError || index<0 || index>=files.size()
                            || entry["source"]!=files[index].toObject()["source"]
                            || entry["output"]!=files[index].toObject()["output"]) {
                        error="Invalid conversion checkpoint."; return false;
                    }
                    files[index]=entry;
                }
            }
        }
    }
    if(!resume) {
        if(QFile::exists(logPath)) { error="Orphan conversion checkpoints require inspection before starting."; return false; }
        QSet<QString> outputs;
        QDirIterator scan(root,QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
            | QDir::Hidden | QDir::System,QDirIterator::Subdirectories);
        while(scan.hasNext()) {
            const QFileInfo info(scan.next());
            if(info.isDir()) {
                if(info.isSymLink() || info.isJunction()) { error="Linked TERRTEX folders are not supported by production conversion."; return false; }
                continue;
            }
            const QString suffix=info.suffix().toLower();
            if(suffix!="ace" && suffix!="dds") continue;
            const QString source=QDir(root).relativeFilePath(info.absoluteFilePath());
            QString output=destination(source);
            if(processor && (!processor->plan || !processor->plan(source,output,error))) return false;
            if(!safePath(root,source) || !safePath(root,output) || outputs.contains(output.toLower())) {
                error="Unsafe or conflicting TERRTEX destination: "+source; return false;
            }
            outputs.insert(output.toLower());
            files.append(QJsonObject{{"source",source},{"output",output},{"size",double(info.size())}});
            SaveProgress::report(QString("Scanning TERRTEX: %1 textures found...").arg(files.size()));
        }
        for(const auto &value:files) {
            const auto entry=value.toObject();
            if(entry["source"]!=entry["output"] && QFile::exists(QDir(root).filePath(entry["output"].toString()))) {
                error="A texture conversion target already exists: "+entry["output"].toString(); return false;
            }
        }
        if(files.isEmpty()) { error="No ACE or DDS textures were found in TERRTEX."; return false; }
        journal=QJsonObject{{"version",processor?3:1},{"root",root},{"complete",false},{"files",files}};
        if(processor) journal["options"]=processor->options;
        if(!saveJournal()) return false;
    }
    // Treat persisted paths as data: validate again before any replacement/deletion.
    QSet<QString> outputs;
    for(const auto &value:files) {
        const auto entry=value.toObject(); const QString src=entry["source"].toString(), dst=entry["output"].toString();
        if((!src.endsWith(".ace",Qt::CaseInsensitive) && !src.endsWith(".dds",Qt::CaseInsensitive))
                || (processor ? (dst!=src && dst!=formatDestination(src,"ace") && dst!=formatDestination(src,"dds"))
                              : dst!=destination(src)) || !safePath(root,src) || !safePath(root,dst)
                || (processor && (!processor->destination || dst!=processor->destination(src)))
                || outputs.contains(dst.toLower())) { error="Unsafe conversion journal entry."; return false; }
        outputs.insert(dst.toLower());
    }
    for(int i=0;i<files.size();++i) {
        journalIndex=i;
        SaveProgress::Range work(0.80*double(i)/files.size(),0.80*double(i+1)/files.size());
        auto entry=files[i].toObject();
        const QString source=QDir(root).filePath(entry["source"].toString());
        const QString output=QDir(root).filePath(entry["output"].toString());
        SaveProgress::report(QString("Processing TERRTEX %1 of %2: %3")
            .arg(i+1).arg(files.size()).arg(entry["source"].toString()));
        const QByteArray expected=entry["sha256"].toString().toLatin1();
        result.before+=qint64(entry["size"].toDouble());
        if(!expected.isEmpty() && hashFile(output)==expected) {
            ++result.skipped; ++result.resumed; result.after+=QFileInfo(output).size(); continue;
        }
        if(entry["done"].toBool()) { error="A previously converted texture has changed: "+output; return false; }
        if(source!=output && expected.isEmpty() && QFile::exists(output)) {
            error="An unexpected conversion output already exists: "+output; return false;
        }
        QFile input(source);
        if(!input.open(QIODevice::ReadOnly) || input.size()>256ll*1024*1024) { error="Cannot read supported TERRTEX input: "+source; return false; }
        QByteArray bytes=input.read(256ll*1024*1024+1);
        if(input.error()!=QFileDevice::NoError || bytes.size()!=input.size()
            || bytes.size()>256ll*1024*1024) { error="Incomplete or oversized TERRTEX read: "+source; return false; }
        input.close();
        const QByteArray sourceHash=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();
        if(!entry["sourceSha256"].toString().isEmpty()
                && entry["sourceSha256"].toString().toLatin1()!=sourceHash) { error="Conversion input changed since interruption: "+source; return false; }
        if(!processor && source==output && DdsWriter::isOpaqueDxt1(bytes)) {
            entry["sha256"]=QString::fromLatin1(sourceHash); entry["done"]=true;
            files[i]=entry; if(!saveJournal()) return false;
            ++result.skipped; result.after+=bytes.size(); continue;
        }
        Processed processed;
        if(processor) {
            if(!processor->transform || !processor->transform(source,bytes,processed,error)) return false;
            if(processed.suffix!="ace" && processed.suffix!="dds") {
                error="Invalid processed texture format."; return false;
            }
            const QString expectedOutput=formatDestination(entry["source"].toString(),processed.suffix);
            if(expectedOutput!=entry["output"].toString()) {
                error="Texture storage changed after planning; processing stopped before publication."; return false;
            }
            if(!processed.changed) {
                if(hashFile(source)!=sourceHash) { error="Unchanged source was modified during processing."; return false; }
                entry["sourceSha256"]=QString::fromLatin1(sourceHash);
                entry["sha256"]=QString::fromLatin1(sourceHash); entry["done"]=true;
                files[i]=entry; if(!saveJournal()) return false;
                ++result.skipped; result.after+=bytes.size(); continue;
            }
            bytes=std::move(processed.bytes);
        } else {
            QImage image; bool mipmaps=false;
            if(source.endsWith(".ace",Qt::CaseInsensitive)) {
                if(!decodeAce) { error="No ACE decoder is available."; return false; }
                if(!decodeAce(source,image,mipmaps,error)) return false;
            } else {
                DdsImage decoded;
                if(!DdsDecoder::decode(bytes,decoded,&error)) { error=source+": "+error; return false; }
                mipmaps=bytes.size()>=128 && qFromLittleEndian<quint32>(bytes.constData()+28)>1;
                image=QImage(reinterpret_cast<const uchar*>(decoded.rgba.constData()),decoded.width,
                    decoded.height,decoded.width*4,QImage::Format_RGBA8888).copy();
            }
            bytes.clear();
            if(!DdsWriter::serializeDxt1(image,mipmaps,bytes,&error)) { error=source+": "+error; return false; }
            image=QImage();
            DdsImage verified;
            if(!DdsDecoder::decode(bytes,verified,&error)) return false;
            verified.rgba.clear();
        }
        if(hashFile(source)!=sourceHash) { error="Source changed while compressing: "+source; return false; }
        entry["sourceSha256"]=QString::fromLatin1(sourceHash);
        entry["sha256"]=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
        // Durable intent before publication handles interruption between file and journal commits.
        files[i]=entry; if(!saveJournal() || !write(output,bytes,error)) return false;
        entry["done"]=true; files[i]=entry; if(!saveJournal()) return false;
        ++result.converted; result.after+=bytes.size();
        result.compressed+=processed.compressed; result.added+=processed.added; result.removed+=processed.removed;
        result.reformatted+=processed.reformatted;
    }
    QMap<QString,QString> names;
    for(const auto &value:files) {
        const auto entry=value.toObject(); const QString src=entry["source"].toString(), dst=entry["output"].toString();
        if(src==dst) continue;
        names.insert(src.toLower(),dst);
        names.insert(QFileInfo(src).fileName().toLower(),QFileInfo(dst).fileName());
    }
    // Verify replacements and still-present originals before publishing names.
    // No additional pre-reference hash pass is needed for in-place mip work.
    if(!names.isEmpty() || !processor) {
        SaveProgress::Range verification(0.80,0.85);
        for(int i=0;i<files.size();++i) {
            SaveProgress::set(double(i)/files.size());
            SaveProgress::report(QString("Verifying replacement %1 of %2 before terrain references...").arg(i+1).arg(files.size()));
            const auto entry=files[i].toObject();
            const QString output=QDir(root).filePath(entry["output"].toString());
            if(hashFile(output)!=entry["sha256"].toString().toLatin1()) {
                error="Converted texture verification failed before reference publication: "+output; return false;
            }
            const QString source=QDir(root).filePath(entry["source"].toString());
            if(source!=output && QFile::exists(source)
                && hashFile(source)!=entry["sourceSha256"].toString().toLatin1()) {
                error="Texture original changed before reference publication: "+source; return false;
            }
        }
    }
    {
        SaveProgress::Range references(0.85,0.94);
        if(!names.isEmpty() || !processor) {
            if(!updateReferences) { error="No terrain reference updater is available."; return false; }
            if(!updateReferences(names,error)) return false;
        }
    }
    // No superseded input is removed until all terrain references are published.
    // Keep only one decoded/encoded texture in memory, never the full route.
    for(int i=0;i<files.size();++i) {
        const auto entry=files[i].toObject();
        SaveProgress::set(0.94+0.04*double(i)/files.size());
        SaveProgress::report(QString("Verifying processed texture %1 of %2...").arg(i+1).arg(files.size()));
        const QString source=QDir(root).filePath(entry["source"].toString());
        const QString output=QDir(root).filePath(entry["output"].toString());
        if(hashFile(output)!=entry["sha256"].toString().toLatin1()) { error="Converted texture verification failed: "+output; return false; }
        if(source!=output && QFile::exists(source)) {
            if(hashFile(source)!=entry["sourceSha256"].toString().toLatin1()) { error="Texture original changed during conversion: "+source; return false; }
        }
    }
    // All references and all replacements are verified before cleanup begins.
    for(int i=0;i<files.size();++i) {
        const auto entry=files[i].toObject();
        if(entry["source"]==entry["output"]) continue;
        SaveProgress::set(0.98+0.019*double(i)/files.size());
        SaveProgress::report(QString("Removing superseded texture %1 of %2...").arg(i+1).arg(files.size()));
        const QString source=QDir(root).filePath(entry["source"].toString());
        if(QFile::exists(source)) {
            const QString output=QDir(root).filePath(entry["output"].toString());
            if(hashFile(source)!=entry["sourceSha256"].toString().toLatin1()
                || hashFile(output)!=entry["sha256"].toString().toLatin1()) {
                error="Texture changed before cleanup: "+source; return false;
            }
            if(!QFile::remove(source)) { error="Cannot remove superseded texture: "+source; return false; }
        }
    }
    journal["complete"]=true;
    journalIndex=-1;
    return saveJournal();
}
}

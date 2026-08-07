#include "RouteSaveTransaction.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QUuid>

namespace {

const int BackupGenerationsToKeep = 5;

QString normalizedPath(const QString &path) {
    return QDir::fromNativeSeparators(
                QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
}

bool isInsideRoute(const QString &path, const QString &routeRoot) {
    const QString cleanPath = normalizedPath(path);
    QString cleanRoot = normalizedPath(routeRoot);
    if(!cleanRoot.endsWith('/'))
        cleanRoot += '/';
    return cleanPath.startsWith(cleanRoot, Qt::CaseInsensitive);
}

bool writeBytesAtomically(const QString &path, const QByteArray &data, QString *error) {
    QSaveFile output(path);
    output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly)){
        if(error)
            *error = QString("Could not open %1: %2").arg(path, output.errorString());
        return false;
    }
    if(output.write(data) != data.size()){
        if(error)
            *error = QString("Could not write %1: %2").arg(path, output.errorString());
        output.cancelWriting();
        return false;
    }
    if(!output.commit()){
        if(error)
            *error = QString("Could not commit %1: %2").arg(path, output.errorString());
        return false;
    }
    return true;
}

bool copyAtomically(const QString &source, const QString &destination, QString *error) {
    QFile input(source);
    if(!input.open(QIODevice::ReadOnly)){
        if(error)
            *error = QString("Could not read %1: %2").arg(source, input.errorString());
        return false;
    }

    QSaveFile output(destination);
    output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly)){
        if(error)
            *error = QString("Could not create %1: %2").arg(destination, output.errorString());
        return false;
    }

    while(!input.atEnd()){
        const QByteArray chunk = input.read(1024 * 1024);
        if(chunk.isEmpty() && input.error() != QFile::NoError){
            if(error)
                *error = QString("Could not read %1: %2").arg(source, input.errorString());
            output.cancelWriting();
            return false;
        }
        if(output.write(chunk) != chunk.size()){
            if(error)
                *error = QString("Could not write %1: %2").arg(destination, output.errorString());
            output.cancelWriting();
            return false;
        }
    }

    if(!output.commit()){
        if(error)
            *error = QString("Could not commit %1: %2").arg(destination, output.errorString());
        return false;
    }
    return true;
}

QByteArray fileHash(const QString &path, QString *error) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        if(error)
            *error = QString("Could not read %1: %2").arg(path, file.errorString());
        return QByteArray();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!hash.addData(&file)){
        if(error)
            *error = QString("Could not hash %1").arg(path);
        return QByteArray();
    }
    return hash.result();
}

bool writeManifest(const QString &generationDir, const QJsonObject &manifest, QString *error) {
    return writeBytesAtomically(generationDir + "/manifest.json",
                                QJsonDocument(manifest).toJson(QJsonDocument::Indented),
                                error);
}

QJsonObject readManifest(const QString &generationDir, QString *error) {
    QFile file(generationDir + "/manifest.json");
    if(!file.open(QIODevice::ReadOnly)){
        if(error)
            *error = QString("Could not read recovery manifest %1: %2")
                    .arg(file.fileName(), file.errorString());
        return QJsonObject();
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !document.isObject()){
        if(error)
            *error = QString("Invalid recovery manifest %1").arg(file.fileName());
        return QJsonObject();
    }
    return document.object();
}

bool restoreManifest(const QString &routeRoot,
                     const QString &generationDir,
                     QJsonObject &manifest,
                     QString *error) {
    const QJsonArray entries = manifest.value("files").toArray();
    for(const QJsonValue &value : entries){
        const QJsonObject entry = value.toObject();
        const QString destination = entry.value("destination").toString();
        if(!isInsideRoute(destination, routeRoot)){
            if(error)
                *error = QString("Recovery destination is outside the active route: %1")
                        .arg(destination);
            return false;
        }

        if(entry.value("existed").toBool()){
            const QString backup = generationDir + "/" + entry.value("backup").toString();
            if(!QFileInfo::exists(backup)){
                if(error)
                    *error = QString("Recovery backup is missing: %1").arg(backup);
                return false;
            }
            if(!copyAtomically(backup, destination, error))
                return false;
        } else if(QFileInfo::exists(destination) && !QFile::remove(destination)){
            if(error)
                *error = QString("Could not remove partially-created file %1").arg(destination);
            return false;
        }
    }
    manifest.insert("state", "rolled_back");
    manifest.insert("recoveredUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return writeManifest(generationDir, manifest, error);
}

bool acquireLock(QLockFile &lock, QString *error) {
    lock.setStaleLockTime(5000);
    if(lock.tryLock(0))
        return true;
    if(lock.removeStaleLockFile() && lock.tryLock(0))
        return true;
    if(error)
        *error = "Another route save or recovery operation is already active.";
    return false;
}

void pruneCompleteBackups(const QString &backupRoot) {
    QDir root(backupRoot);
    const QStringList generations = root.entryList(
                QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
    int completeCount = 0;
    for(const QString &name : generations){
        const QString generationDir = root.filePath(name);
        QString ignored;
        const QJsonObject manifest = readManifest(generationDir, &ignored);
        if(manifest.value("state").toString() != "complete")
            continue;
        ++completeCount;
        if(completeCount > BackupGenerationsToKeep)
            QDir(generationDir).removeRecursively();
    }
}

}

RouteSaveTransaction::RouteSaveTransaction(const QString &routeRoot,
                                           const QString &backupRoot)
    : routeRoot(normalizedPath(routeRoot)),
      backupRoot(normalizedPath(backupRoot)) {
}

bool RouteSaveTransaction::addFile(const QString &destination,
                                   const QByteArray &data,
                                   QString *error) {
    const QString cleanDestination = normalizedPath(destination);
    if(!isInsideRoute(cleanDestination, routeRoot)){
        if(error)
            *error = QString("Save destination is outside the active route: %1")
                    .arg(cleanDestination);
        return false;
    }
    if(data.isEmpty()){
        if(error)
            *error = QString("Refusing to replace %1 with an empty file.").arg(cleanDestination);
        return false;
    }
    for(const FileEntry &entry : files){
        if(entry.destination.compare(cleanDestination, Qt::CaseInsensitive) == 0){
            if(error)
                *error = QString("Duplicate save destination: %1").arg(cleanDestination);
            return false;
        }
    }

    FileEntry entry;
    entry.destination = cleanDestination;
    entry.data = data;
    files.append(entry);
    return true;
}

bool RouteSaveTransaction::commit(QString *error) {
    if(files.isEmpty())
        return true;
    if(!QDir().mkpath(backupRoot)){
        if(error)
            *error = QString("Could not create route backup directory %1").arg(backupRoot);
        return false;
    }

    QLockFile lock(backupRoot + "/route-save.lock");
    if(!acquireLock(lock, error))
        return false;

    const QString generationName =
            QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz-")
            + QUuid::createUuid().toString().remove('{').remove('}');
    const QString generationDir = backupRoot + "/" + generationName;
    if(!QDir().mkpath(generationDir)){
        if(error)
            *error = QString("Could not create route backup generation %1").arg(generationDir);
        return false;
    }

    QJsonObject manifest;
    manifest.insert("version", 1);
    manifest.insert("state", "preparing");
    manifest.insert("createdUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    manifest.insert("routeRoot", routeRoot);
    manifest.insert("files", QJsonArray());
    if(!writeManifest(generationDir, manifest, error))
        return false;

    QJsonArray manifestFiles;
    for(int i = 0; i < files.size(); ++i){
        FileEntry &entry = files[i];
        entry.existed = QFileInfo::exists(entry.destination);
        entry.backupName = QString("%1-%2")
                .arg(i, 4, 10, QChar('0'))
                .arg(QFileInfo(entry.destination).fileName());

        if(entry.existed){
            QString hashError;
            entry.originalHash = fileHash(entry.destination, &hashError);
            if(entry.originalHash.isEmpty()){
                if(error)
                    *error = hashError;
                manifest.insert("state", "backup_failed");
                writeManifest(generationDir, manifest, NULL);
                return false;
            }
            if(!copyAtomically(entry.destination,
                               generationDir + "/" + entry.backupName,
                               error)){
                manifest.insert("state", "backup_failed");
                writeManifest(generationDir, manifest, NULL);
                return false;
            }
        }

        QJsonObject fileObject;
        fileObject.insert("destination", entry.destination);
        fileObject.insert("existed", entry.existed);
        fileObject.insert("backup", entry.backupName);
        fileObject.insert("sha256", QString::fromLatin1(entry.originalHash.toHex()));
        manifestFiles.append(fileObject);
    }

    manifest.insert("files", manifestFiles);
    manifest.insert("state", "prepared");
    if(!writeManifest(generationDir, manifest, error))
        return false;

    manifest.insert("state", "committing");
    if(!writeManifest(generationDir, manifest, error))
        return false;

    for(const FileEntry &entry : files){
        if(!writeBytesAtomically(entry.destination, entry.data, error)){
            QString restoreError;
            if(!restoreManifest(routeRoot, generationDir, manifest, &restoreError) && error)
                *error += QString("\nRollback also failed: %1").arg(restoreError);
            return false;
        }
    }

    manifest.insert("state", "complete");
    manifest.insert("completedUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if(!writeManifest(generationDir, manifest, error)){
        QString restoreError;
        if(!restoreManifest(routeRoot, generationDir, manifest, &restoreError) && error)
            *error += QString("\nRollback also failed: %1").arg(restoreError);
        return false;
    }

    pruneCompleteBackups(backupRoot);
    return true;
}

bool RouteSaveTransaction::recoverInterrupted(const QString &routeRoot,
                                              const QString &backupRoot,
                                              QString *message) {
    const QString cleanRouteRoot = normalizedPath(routeRoot);
    const QString cleanBackupRoot = normalizedPath(backupRoot);
    QDir root(cleanBackupRoot);
    if(!root.exists())
        return true;

    QLockFile lock(cleanBackupRoot + "/route-save.lock");
    QString lockError;
    if(!acquireLock(lock, &lockError)){
        if(message)
            *message = lockError;
        return false;
    }

    QStringList recovered;
    const QStringList generations = root.entryList(
                QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for(const QString &name : generations){
        const QString generationDir = root.filePath(name);
        QString manifestError;
        QJsonObject manifest = readManifest(generationDir, &manifestError);
        if(manifest.isEmpty()){
            // A process can stop after creating the generation directory but
            // before its first atomic manifest commit. No route file has been
            // touched at that point, so this orphan is safe to remove.
            QDir(generationDir).removeRecursively();
            continue;
        }

        const QString state = manifest.value("state").toString();
        if(state == "committing"){
            if(!restoreManifest(cleanRouteRoot, generationDir, manifest, &manifestError)){
                if(message)
                    *message = manifestError;
                return false;
            }
            recovered.append(name);
        } else if(state == "preparing" || state == "prepared"){
            QDir(generationDir).removeRecursively();
        }
    }

    if(message && !recovered.isEmpty())
        *message = QString("Recovered %1 interrupted route save(s).").arg(recovered.size());
    pruneCompleteBackups(cleanBackupRoot);
    return true;
}

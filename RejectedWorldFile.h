// Recovery-preserving removal of a scan-identified malformed world filename.
#ifndef REJECTEDWORLDFILE_H
#define REJECTEDWORLDFILE_H
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
namespace RejectedWorldFile {
inline bool malformedName(const QString &name){
    static const QRegularExpression normal("^w([+-][0-9]{6})([+-][0-9]{6})\\.w$",
                                            QRegularExpression::CaseInsensitiveOption);
    return !normal.match(name).hasMatch();
}
inline QByteArray digest(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return hash.addData(&file) && file.error() == QFileDevice::NoError ? hash.result() : QByteArray();
}
inline bool remove(const QString &activeWorld, const QString &scannedWorld,
                   const QString &name, const QByteArray &expectedHash,
                   QString &recoveryPath, QString &error){
    const QString root = QFileInfo(activeWorld).canonicalFilePath();
    const QFileInfo source(QDir(root).filePath(name));
    if(root.isEmpty() || root != scannedWorld || name != QFileInfo(name).fileName()
            || name.contains('/') || name.contains('\\') || !name.endsWith(".w", Qt::CaseInsensitive)
            || !malformedName(name) || !source.isFile() || source.isSymLink()
            || source.canonicalPath() != root || expectedHash.isEmpty()
            || digest(source.absoluteFilePath()) != expectedHash){
        error = "The file, route or scan evidence changed. Scan again before deleting.";
        return false;
    }
    recoveryPath = source.absoluteFilePath() + ".rejected-"
            + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".bak";
    if(!QFile::rename(source.absoluteFilePath(), recoveryPath)){
        error = "Could not remove the file from the active world list. It was left in place.";
        return false;
    }
    return true;
}
}
#endif

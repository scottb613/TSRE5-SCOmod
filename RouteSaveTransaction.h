#ifndef ROUTESAVETRANSACTION_H
#define ROUTESAVETRANSACTION_H

#include <QByteArray>
#include <QList>
#include <QString>

class RouteSaveTransaction {
public:
    RouteSaveTransaction(const QString &routeRoot, const QString &backupRoot);

    bool addFile(const QString &destination, const QByteArray &data, QString *error = NULL);
    bool commit(QString *error = NULL);

    static bool recoverInterrupted(const QString &routeRoot,
                                   const QString &backupRoot,
                                   QString *message = NULL);

private:
    struct FileEntry {
        QString destination;
        QByteArray data;
        bool existed = false;
        QString backupName;
        QByteArray originalHash;
    };

    QString routeRoot;
    QString backupRoot;
    QList<FileEntry> files;
};

#endif

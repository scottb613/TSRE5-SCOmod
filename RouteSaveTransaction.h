// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

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

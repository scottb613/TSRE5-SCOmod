// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "RouteSaveTransaction.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cstdio>

namespace {

bool writeFile(const QString &path, const QByteArray &data) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly)
            && file.write(data) == data.size();
}

QByteArray readFile(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.readAll();
}

int fail(const QString &message) {
    std::fprintf(stderr, "FAIL: %s\n", qPrintable(message));
    return 1;
}

}

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    QTemporaryDir temp;
    if(!temp.isValid())
        return fail("Could not create the temporary test directory.");

    const QString routeRoot = temp.path() + "/route";
    const QString backupRoot = temp.path() + "/appdata/backups";
    QDir().mkpath(routeRoot);
    const QString tdb = routeRoot + "/sample.tdb";
    const QString tit = routeRoot + "/sample.tit";
    if(!writeFile(tdb, "old-tdb") || !writeFile(tit, "old-tit"))
        return fail("Could not create source files.");

    RouteSaveTransaction transaction(routeRoot, backupRoot);
    QString error;
    if(!transaction.addFile(tdb, "new-tdb", &error)
            || !transaction.addFile(tit, "new-tit", &error)
            || !transaction.commit(&error))
        return fail(error);
    if(readFile(tdb) != "new-tdb" || readFile(tit) != "new-tit")
        return fail("Atomic commit did not install all new files.");

    RouteSaveTransaction invalid(routeRoot, backupRoot);
    if(invalid.addFile(temp.path() + "/outside.tdb", "bad", &error))
        return fail("A destination outside the route was accepted.");

    // Model multiple dirty patches sharing a main or paired-season ACE.
    const QString terrtex = routeRoot + "/Terrtex";
    QDir().mkpath(terrtex + "/snow");
    const QString mainAce = terrtex + "/shared.ace";
    const QString snowAce = terrtex + "/snow/shared.ace";
    const QString sharedBackups = temp.path() + "/shared-backups";
    if(!writeFile(mainAce, "old-main") || !writeFile(snowAce, "old-snow"))
        return fail("Could not create shared texture fixtures.");
    RouteSaveTransaction shared(routeRoot, sharedBackups);
    if(!shared.addFile(mainAce, "new-main", &error)
            || !shared.addFile(terrtex + "/./shared.ace", "new-main", &error)
            || !shared.addFile(terrtex + "/SHARED.ACE", "new-main", &error)
            || !shared.addFile(snowAce, "new-snow", &error)
            || !shared.addFile(snowAce, "new-snow", &error))
        return fail("Identical shared texture staging failed: " + error);
    if(shared.addFile(mainAce, "conflicting-main", &error)
            || shared.addFile(terrtex + "/SHARED.ACE", "conflicting-main", &error)
            || shared.addFile(mainAce, QByteArray(), &error))
        return fail("Conflicting or empty shared texture data was accepted.");
    if(readFile(mainAce) != "old-main" || readFile(snowAce) != "old-snow")
        return fail("Staging shared textures changed the original files.");
    if(!shared.commit(&error))
        return fail(error);
    if(readFile(mainAce) != "new-main" || readFile(snowAce) != "new-snow")
        return fail("Shared texture commit did not preserve the accepted data.");
    const QStringList generations = QDir(sharedBackups).entryList(
        QDir::Dirs | QDir::NoDotAndDotDot);
    if(generations.size() != 1)
        return fail("Expected one shared texture backup generation.");
    const QJsonObject sharedManifest = QJsonDocument::fromJson(readFile(
        sharedBackups + "/" + generations.first() + "/manifest.json")).object();
    if(sharedManifest.value("files").toArray().size() != 2)
        return fail("Shared textures were not staged exactly once per destination.");

    RouteSaveTransaction sharedFailure(routeRoot, sharedBackups);
    if(!sharedFailure.addFile(mainAce, "partial-main", &error)
            || !sharedFailure.addFile(mainAce, "partial-main", &error)
            || !sharedFailure.addFile(routeRoot + "/absent/later.ace", "bad", &error))
        return fail("Could not stage shared texture rollback test.");
    if(sharedFailure.commit(&error) || readFile(mainAce) != "new-main")
        return fail("Shared texture rollback failed after a later write error.");

    RouteSaveTransaction laterFailure(routeRoot, backupRoot);
    const QString missingParent = routeRoot + "/missing/sample_f.raw";
    if(!laterFailure.addFile(tdb, "partial-write", &error)
            || !laterFailure.addFile(missingParent, "cannot-write", &error))
        return fail("Could not prepare later-component failure test.");
    if(laterFailure.commit(&error))
        return fail("Transaction unexpectedly committed through a missing directory.");
    if(readFile(tdb) != "new-tdb")
        return fail("Earlier component was not restored after a later failure.");
    if(QFileInfo::exists(missingParent))
        return fail("Failed later component was unexpectedly published.");

    const QString interruptedDir = backupRoot + "/99999999-interrupted";
    QDir().mkpath(interruptedDir);
    if(!writeFile(interruptedDir + "/0000-sample.tdb", "known-good")
            || !writeFile(tdb, "partial-write"))
        return fail("Could not prepare interrupted-save fixtures.");

    QJsonObject fileEntry;
    fileEntry.insert("destination", QDir::cleanPath(tdb));
    fileEntry.insert("existed", true);
    fileEntry.insert("backup", "0000-sample.tdb");
    QJsonArray files;
    files.append(fileEntry);
    QJsonObject manifest;
    manifest.insert("version", 1);
    manifest.insert("state", "committing");
    manifest.insert("routeRoot", QDir::cleanPath(routeRoot));
    manifest.insert("files", files);
    if(!writeFile(interruptedDir + "/manifest.json",
                  QJsonDocument(manifest).toJson(QJsonDocument::Indented)))
        return fail("Could not create interrupted-save manifest.");

    QString recoveryMessage;
    if(!RouteSaveTransaction::recoverInterrupted(
                routeRoot, backupRoot, &recoveryMessage))
        return fail(recoveryMessage);
    if(readFile(tdb) != "known-good")
        return fail("Interrupted save was not rolled back.");
    if(recoveryMessage.isEmpty())
        return fail("Successful recovery was not reported.");

    std::fprintf(stdout, "RouteSaveTransaction tests passed.\n");
    return 0;
}

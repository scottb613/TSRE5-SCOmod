#include "RejectedWorldFile.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>

int main(int argc, char **argv){
    QCoreApplication app(argc,argv);
    QTemporaryDir fixture;
    if(!fixture.isValid()) return 1;
    const QString root = QFileInfo(fixture.path()).canonicalFilePath();
    const QString name = "w+1234567890-1234567890.w";
    const auto write = [&](const QString &fileName, const QByteArray &bytes){
        QFile file(QDir(root).filePath(fileName));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    if(!write(name,"test world data")) return 1;
    const QByteArray original = RejectedWorldFile::digest(QDir(root).filePath(name));
    int failures = 0;
    const auto require = [&](bool ok, const char *text){
        if(!ok){ ++failures; std::cerr << text << '\n'; }
    };
    QString recovery, error;
    require(!RejectedWorldFile::remove(root,root+"/other",name,original,recovery,error),"Reject another route");
    require(!RejectedWorldFile::remove(root,root,"../"+name,original,recovery,error),"Reject path traversal");
    require(write(name,"changed data"),"Change fixture");
    require(!RejectedWorldFile::remove(root,root,name,original,recovery,error),"Reject changed file");
    const QByteArray changed = RejectedWorldFile::digest(QDir(root).filePath(name));
    require(RejectedWorldFile::remove(root,root,name,changed,recovery,error),"Remove rejected file");
    require(!QFile::exists(QDir(root).filePath(name)),"Active file gone");
    require(RejectedWorldFile::digest(recovery) == changed,"Recovery preserves exact contents");
    require(QDir(root).entryList({"*.w"},QDir::Files).isEmpty(),"Recovery excluded from scan");
    const QString normal = "w+000123-000456.w";
    require(write(normal,"normal world"),"Create valid filename");
    require(!RejectedWorldFile::remove(root,root,normal,
            RejectedWorldFile::digest(QDir(root).filePath(normal)),recovery,error),"Refuse normally named tile");
    return failures ? 1 : 0;
}

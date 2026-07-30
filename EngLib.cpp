/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "EngLib.h"
#include "Eng.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QProgressDialog>
#include <QCoreApplication>
#include "Game.h"

//int EngLib::jesteng = 0;
//std::unordered_map<int, Eng*> EngLib::eng;
    
EngLib::EngLib() {
}

EngLib::~EngLib() {
}

int EngLib::getEngByPointer(Eng* pointer){
    for ( auto it = eng.begin(); it != eng.end(); ++it ){
        if(it->second == pointer) 
            return it->first;
    }
    return -1;
}

int EngLib::addEng(QString path, QString name) {
    QString pathid = (path + "/" + name).toLower();
    pathid.replace("\\", "/");
    pathid.replace("//", "/");
    //qDebug() << pathid;
    for ( auto it = eng.begin(); it != eng.end(); ++it ){
        if(it->second == NULL) continue;
        if (((Eng*) it->second)->pathid.length() == pathid.length())
            if (((Eng*) it->second)->pathid == pathid) {
                ((Eng*) it->second)->ref++;
                //qDebug() <<"engid "<< pathid;
                return (int)it->first;
            }
    }
    //qDebug() << "Nowy " << jesteng << " eng: " << pathid;

    eng[jesteng] = new Eng(pathid, path, name);
    // qDebug() << "EngLib 49 : "<< pathid << " ~ emass: " << eng[jesteng]->mass ;

    return jesteng++;
}

int EngLib::removeBroken() {
    for (int i = 0; i < jesteng; i++){
        if(eng[i] == NULL) continue;
        if (eng[i]->loaded != 1)
            eng[i] = NULL;
    }
    return 0;
}

void EngLib::removeAll(){
    eng.clear();
    jesteng = 0;
}

int EngLib::getEngByPathid(QString pathid) {
    for ( auto it = eng.begin(); it != eng.end(); ++it ){
        if(it->second == NULL) continue;
        if (((Eng*) it->second)->pathid.length() == pathid.length())
            if (((Eng*) it->second)->pathid == pathid) {
                return (int)it->first;
            }
    }
    return -1;
}

int EngLib::loadAll(QString gameRoot, bool gui){
    QString path;
    path = gameRoot + "/trains/trainset/";
    QDir dir(path);
    QDir trainDir;
    trainDir.setFilter(QDir::Files);
    // Include files are shared definition fragments, not selectable vehicles.
    // Eng::load() still resolves and parses them when an ENG/WAG/EOT references
    // one, but the Consist Editor should not list them as rolling stock.
    trainDir.setNameFilters(QStringList()<<"*.eng"<<"*.wag"<<"*.eot");
    if(Game::debugOutput) qDebug() << path;
    if(!dir.exists())
        if(Game::debugOutput) qDebug() << "Path not exist" << path;
    dir.setFilter(QDir::Dirs);
    if(Game::debugOutput) qDebug() << dir.count() <<" dirs";
    unsigned long long timeNow = QDateTime::currentMSecsSinceEpoch();
    QStringList dirPaths;
    QStringList engPaths;
    foreach(QString dirFile, dir.entryList()){
        //qDebug() << dirFile;
        trainDir.setPath(path+dirFile);
        foreach(QString engfile, trainDir.entryList()){
            //qDebug() << path << dirFile <<"/"<< engfile;
            //addEng(path+dirFile,engfile);
            dirPaths.push_back(path+dirFile);
            engPaths.push_back(engfile);
        }
    }
    
    //// EFO --- this is a second run at a separate directory  EOT Support
    QString path2;
    path2 = gameRoot + "/trains/orts_eot/";    
    QDir dir2(path2);
    trainDir.setFilter(QDir::Files);
    if(Game::debugOutput) qDebug() << path2;
    if(!dir2.exists())
        if(Game::debugOutput) qDebug() << "Path not exist" << path2;
    dir2.setFilter(QDir::Dirs);
    if(Game::debugOutput) qDebug() << dir2.count() <<" dirs2";
    foreach(QString dirFile, dir2.entryList()){
        //qDebug() << dirFile;
        trainDir.setPath(path2+dirFile);
        foreach(QString engfile, trainDir.entryList()){
            if(Game::debugOutput) qDebug() << path2 << dirFile <<"/"<< engfile;
            //addEng(path+dirFile,engfile);
            dirPaths.push_back(path2+dirFile);
            engPaths.push_back(engfile);
        }
    }
    
    QProgressDialog *progress = NULL;
    if(gui){
        progress = new QProgressDialog("Loading TRAINS...", "", 0, dirPaths.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
    }
    for(int i = 0; i < dirPaths.size(); i++){
        //qDebug() << path << dirFile <<"/"<< engfile;
        addEng(dirPaths[i],engPaths[i]);
        if(progress != NULL){
            progress->setValue(i+1);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }
    if(Game::debugOutput) qDebug() << "loaded" << (QDateTime::currentMSecsSinceEpoch() - timeNow)/1000<< "s";
    delete progress;
    return 0;
}

/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TexLib.h"
#include "AceLib.h"
#include "DdsLib.h"
#include "ImageLib.h"
#include "PaintTexLib.h"
#include "MapLib.h"
#include "Texture.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "Game.h"
#include "Route.h"

int TexLib::jesttextur = 0;
std::unordered_map<int, Texture*> TexLib::mtex;
QHash<int, int> TexLib::disabledTextures;

QString TexLib::resolveTexturePath(const QStringList &basePaths, const QString &fileName, bool tryAceDdsAlternative) {
    QStringList candidateNames;
    candidateNames.append(fileName);

    if (tryAceDdsAlternative) {
        const QFileInfo requested(fileName);
        const QString suffix = requested.suffix().toLower();
        if (suffix == "ace" || suffix == "dds") {
            const QString alternativeSuffix = (suffix == "ace") ? "dds" : "ace";
            QString alternativeName = requested.path() == "."
                    ? requested.completeBaseName() + "." + alternativeSuffix
                    : requested.path() + "/" + requested.completeBaseName() + "." + alternativeSuffix;
            candidateNames.append(alternativeName);
        }
    }

    for (const QString &basePath : basePaths) {
        for (const QString &candidateName : candidateNames) {
            const QString candidatePath = QDir::cleanPath(basePath + "/" + candidateName).replace("\\", "/");
            if (QFileInfo::exists(candidatePath))
                return candidatePath;
        }
    }

    if (basePaths.isEmpty())
        return fileName;
    return QDir::cleanPath(basePaths.first() + "/" + fileName).replace("\\", "/");
}

void TexLib::reset() {
    jesttextur = 0;
    mtex.clear();
}

void TexLib::enableTexture(int id){
    Texture* tex = mtex[id];
    if(tex != NULL)
        disabledTextures[tex->tex[0]] = 0;
}

void TexLib::disableTexture(int id){
    Texture* tex = mtex[id];
    if(tex != NULL)
        disabledTextures[tex->tex[0]] = 1;
}

void TexLib::delRef(int texx) {
    try {
        Texture* t = mtex.at(texx);
        t->ref--;
        if (t->ref <= 0) {
            //System.out.println("--refs: "+mtex.get(texx).ref);
            if (t->glLoaded) {
                t->delVBO();
                mtex.erase(texx);
            }
        }
    } catch (const std::out_of_range& oor) {
            
    }
}

void TexLib::addRef(int texx) {
    try {
        Texture* t = mtex.at(texx);
        t->ref++;
    } catch (const std::out_of_range& oor) {
            
    }    
}

int TexLib::addTex(QString path, QString name, bool reload) {
    QString pathid = (path+"/"+name).toLower();
    pathid.replace("\\", "/");
    pathid.replace("//", "/");
    return addTex(pathid, reload);
}

int TexLib::getTex(QString pathid) {
    for ( auto it = mtex.begin(); it != mtex.end(); ++it ){
        if(it->second == NULL) continue;
        for(int i = 0; i < ((Texture*) it->second)->hashid.size(); i++)
            if (((Texture*) it->second)->hashid[i].length() == pathid.length()) 
                if (((Texture*) it->second)->hashid[i] == pathid) {
                    ((Texture*) it->second)->ref++;
                    return (int)it->first;
                }
    }
    return -1;
}

int TexLib::addTex(QString pathid, bool reload) {
    const QString requestedPathid = pathid;
    Texture* newFile = NULL;
    int texId = -1;
    for ( auto it = mtex.begin(); it != mtex.end(); ++it ){
        if(it->second == NULL) continue;
        for(int i = 0; i < ((Texture*) it->second)->hashid.size(); i++)
            if (((Texture*) it->second)->hashid[i].length() == pathid.length()) 
                if (((Texture*) it->second)->hashid[i] == pathid) {
                    if(!reload){
                        ((Texture*) it->second)->ref++;
                        if(((Texture*) it->second)->missing
                                && !Route::missingTextureList.contains(
                                    pathid, Qt::CaseInsensitive))
                            Route::missingTextureList.append(pathid.toLower());
                        return (int)it->first;
                    } else {
                        newFile = ((Texture*) it->second);
                        texId = (int)it->first;
                        break;
                    }
                }
        if(newFile != NULL)
            break;
    }
    //qDebug() << "Nowa " << jesttextur << " textura: " << pathid;
    
    QString tType = pathid.toLower().split(".").last();
    
    /// EFO there's no good reason to swap DDS and ACE files... 
    /// This allows it to be done, default is NOT to
    if(Game::imageSubstitution)
    {
        /// If DDS doesn't exist but ACE does
        if(tType == "dds"){   
            QFile file(pathid);
            if (!file.exists()){  
                tType = "ace";
                pathid = pathid.left(pathid.length() - 3)+"ace";
            }
        }

        /// if ACE doesn't exist but DDS does
        if(tType == "ace"){            
            QFile file(pathid);
            if (!file.exists()){
                tType = "dds";
                pathid = pathid.left(pathid.length() - 3)+"dds";
            }
        }
    }
    
    if(Game::imageUpgrade)
    {
            if(tType == "ace"){            
            /// check to see if DDS exists for ACE
            QFile file(pathid.left(pathid.length() - 3)+"dds");            
            if  ((file.exists()) and (pathid.contains(Game::route +  "/terrtex") == false) ){
            // if  (file.exists()) {                
                qDebug () << "DDS upgraded for " << pathid;                 
                tType = "dds";
                pathid = pathid.left(pathid.length() - 3)+"dds";
            }
        }    
    }        

    // A shape normally names an ACE texture even when Image Upgrade resolves
    // that request to DDS. Cache the requested ACE name on the resolved DDS
    // texture so later references reuse it instead of decoding another copy.
    // Keep this resolution-aware: direct DDS files and excluded TERRTEX ACE
    // files remain distinct unless substitution/upgrade actually changed the
    // requested path.
    if(pathid != requestedPathid){
        for(auto it = mtex.begin(); it != mtex.end(); ++it){
            Texture *texture = it->second;
            if(texture == NULL)
                continue;
            if(!texture->hashid.contains(pathid))
                continue;

            if(!texture->hashid.contains(requestedPathid))
                texture->hashid.push_back(requestedPathid);
            if(!reload){
                texture->ref++;
                if(texture->missing
                        && !Route::missingTextureList.contains(
                            requestedPathid, Qt::CaseInsensitive))
                    Route::missingTextureList.append(requestedPathid.toLower());
                return (int)it->first;
            }
            newFile = texture;
            texId = (int)it->first;
            break;
        }
    }
    
    if(tType == "ace" || tType == "dds"){
        if(!QFile::exists(pathid)){
            qWarning() << "Missing texture:" << pathid.toLower();
            if(!Route::missingTextureList.contains(pathid, Qt::CaseInsensitive))
                Route::missingTextureList.append(pathid.toLower());
        }
    }

    if(Game::listFiles)
    {
        if(Route::texturesList.contains(pathid) == false)
        {
            if((pathid.endsWith(".ace")) | (pathid.endsWith(".dds")))
            Route::texturesList.append(pathid);
        }
    }
    
    if(newFile == NULL){
        newFile = new Texture(pathid);
        newFile->ref++;
        mtex[jesttextur] = newFile;
        texId = jesttextur;
        jesttextur++;
    } else {
        newFile->delVBO();
    }

    if(pathid != requestedPathid
            && !newFile->hashid.contains(requestedPathid))
        newFile->hashid.push_back(requestedPathid);

    //qDebug() << pathid.toLower();
    //qDebug() << tType;
        
    if(tType == "ace"){
        AceLib* t = new AceLib();
        t->texture = newFile;
        if(AceLib::IsThread && !reload){
            QObject::connect(t, &QThread::finished, t, &QObject::deleteLater);
            t->start();
        } else {
            t->run();
            delete t;
        }
    } else if(tType == "png"||tType == "bmp"||tType == "jpg"||tType == "tga"){
        ImageLib* t = new ImageLib();
        t->texture = newFile;
        if(ImageLib::IsThread && !reload){
            QObject::connect(t, &QThread::finished, t, &QObject::deleteLater);
            t->start();
        } else {
            t->run();
            delete t;
        }
    } else if(tType == "dds"){
        DdsLib* t = new DdsLib();
        t->texture = newFile;
        if(DdsLib::IsThread && !reload){
            QObject::connect(t, &QThread::finished, t, &QObject::deleteLater);
            t->start();
        } else {
            t->run();
            delete t;
        }
    } else if(tType == ":painttex"){
        PaintTexLib* t = new PaintTexLib();
        t->texture = newFile;
        //t->start();
        t->run();
        delete t;
    } else if(tType == ":maptex"){
        MapLib* t = new MapLib();
        t->texture = newFile;
        QObject::connect(t, &QThread::finished, t, &QObject::deleteLater);
        t->start();
    }
    //AceLib::LoadACE(newFile);
    //tConcurrent::run();
    return texId;
}

int TexLib::cloneTex(int id) {
    Texture* t = mtex[id];
    if(t == NULL) {
        qDebug() << "null texture " << id;
        return -2;
    }
    Texture* newFile = new Texture(t);
    newFile->ref++;
    mtex[jesttextur] = newFile;
 
    return jesttextur++;
}

void TexLib::save(QString type, QString path, int id){
    Texture* t = mtex.at(id);
    if(t == NULL) 
        return;
    if(!t->editable)
        t->setEditable();
    AceLib::save(path, t);
}

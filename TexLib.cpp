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
#include "RouteFrameProfile.h"
#include "ShapeTexturePath.h"
#include "AceLib.h"
#include "DdsLib.h"
#include "TerrainTextureStream.h"
#include "ImageLib.h"
#include "PaintTexLib.h"
#include "MapLib.h"
#include "Texture.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <utility>
#include "Game.h"
#include "Route.h"

int TexLib::jesttextur = 0;
std::unordered_map<int, Texture*> TexLib::mtex;
QHash<int, int> TexLib::disabledTextures;

namespace {
void startTextureDecode(QThread *loader, int id, Texture *texture) {
    texture->decodePending = true;
    QObject::connect(loader, &QThread::finished, QCoreApplication::instance(),
        [id, texture] {
            const auto found = TexLib::mtex.find(id);
            if (found != TexLib::mtex.end() && found->second == texture)
                texture->decodePending = false;
        }, Qt::QueuedConnection);
    QObject::connect(loader, &QThread::finished, loader, &QObject::deleteLater);
    loader->start();
}
}

QString TexLib::resolveTexturePath(const QStringList &basePaths, const QString &fileName, bool tryAceDdsAlternative) {
    return ShapeTexturePath::resolve(basePaths, fileName, tryAceDdsAlternative);
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

void TexLib::releaseMapTexture(int texx) {
    auto entry = mtex.find(texx);
    if(entry == mtex.end() || entry->second == NULL)
        return;

    Texture *texture = entry->second;
    if(!texture->pathid.endsWith(":maptex", Qt::CaseInsensitive))
        return;

    texture->ref--;
    if(texture->ref > 0)
        return;

    if(texture->glLoaded && texture->tex != NULL){
        QOpenGLContext *context = QOpenGLContext::currentContext();
        if(context != NULL)
            context->functions()->glDeleteTextures(1, texture->tex);
    }
    delete[] texture->tex;
    texture->tex = NULL;
    delete[] texture->imageData;
    texture->imageData = NULL;
    mtex.erase(entry);
    delete texture;
}

bool TexLib::releaseAllMapTextures() {
    QOpenGLContext *context = QOpenGLContext::currentContext();
    bool allReleased = true;

    for(auto entry = mtex.begin(); entry != mtex.end(); ){
        Texture *texture = entry->second;
        if(texture == NULL
                || !texture->pathid.endsWith(":maptex", Qt::CaseInsensitive)){
            ++entry;
            continue;
        }

        // A live GL texture can only be destroyed while a context is current.
        // Leave it registered so the render-time purge can retry next frame.
        if(texture->glLoaded && texture->tex != NULL && context == NULL){
            allReleased = false;
            ++entry;
            continue;
        }

        if(texture->glLoaded && texture->tex != NULL)
            context->functions()->glDeleteTextures(1, texture->tex);
        delete[] texture->tex;
        texture->tex = NULL;
        delete[] texture->imageData;
        texture->imageData = NULL;
        delete texture;
        entry = mtex.erase(entry);
    }

    return allReleased;
}

void TexLib::addRef(int texx) {
    try {
        Texture* t = mtex.at(texx);
        t->ref++;
    } catch (const std::out_of_range& oor) {
            
    }    
}

namespace {
QString texturePathKey(const QString &path, const QString &name) {
    QString key = (path + "/" + name).toLower();
    key.replace("\\", "/");
    key.replace("//", "/");
    return key;
}
}

int TexLib::addTex(QString path, QString name, bool reload) {
    return addTex(texturePathKey(path, name), reload);
}

int TexLib::getTex(QString pathid) {
    return getTex(pathid, false);
}

int TexLib::getResidentTex(const QString &path, const QString &name) {
    return getTex(texturePathKey(path, name), true);
}

int TexLib::getTex(const QString &pathid, bool residentOnly) {
    for (const auto &[id, texture] : mtex) {
        if (!texture || (residentOnly && !texture->glLoaded))
            continue;
        if (texture->hashid.contains(pathid)) {
            ++texture->ref;
            return id;
        }
    }
    return -1;
}

int TexLib::addTex(QString pathid, bool reload, bool exactPath) {
    return addTexInternal(std::move(pathid), reload, exactPath, false);
}

int TexLib::addStreamingTerrainTex(const QString &path, const QString &name) {
    return addTexInternal(texturePathKey(path, name), false, false, true);
}

int TexLib::addTexInternal(QString pathid, bool reload, bool exactPath, bool streamDds) {
    const QString requestedPathid = pathid;
    if (reload)
        TerrainTextureStream::invalidate(requestedPathid);
    RouteFrameProfile::Scope request(RouteFrameProfile::Stage::TextureRequest, &requestedPathid);
    RouteFrameProfile::Scope cache(RouteFrameProfile::Stage::TextureCacheLookup);
    Texture* newFile = NULL;
    int texId = -1;
    for ( auto it = mtex.begin(); it != mtex.end(); ++it ){
        if(it->second == NULL) continue;
        if(exactPath && it->second->pathid != pathid)
            continue;
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
    cache.finish();
    
    QString tType = pathid.toLower().split(".").last();
    
    /// EFO there's no good reason to swap DDS and ACE files... 
    /// This allows it to be done, default is NOT to
    if(!exactPath && Game::imageSubstitution)
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
    
    if(!exactPath && Game::imageUpgrade)
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
        if (reload)
            TerrainTextureStream::invalidate(pathid);
        RouteFrameProfile::Scope resolvedCache(RouteFrameProfile::Stage::TextureCacheLookup);
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
    
    DdsTextureData streamed;
    const bool useStreamed = streamDds && tType == "dds";
    if (useStreamed) {
        // Keep pending assets out of the shared cache. Edits, reloads,
        // and saves can continue to use the synchronous texture path.
        if (!DdsLib::takeStreamed(pathid, streamed))
            return -1;
    } else if (tType == "dds") {
        DdsLib::discardStreamed(pathid);
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
            startTextureDecode(t, texId, newFile);
        } else {
            t->run();
            delete t;
        }
    } else if(tType == "png"||tType == "bmp"||tType == "jpg"||tType == "tga"){
        ImageLib* t = new ImageLib();
        t->texture = newFile;
        if(ImageLib::IsThread && !reload){
            startTextureDecode(t, texId, newFile);
        } else {
            t->run();
            delete t;
        }
    } else if(tType == "dds"){
        if (useStreamed) {
            DdsLib::publish(*newFile, std::move(streamed));
        } else {
            DdsLib decoder;
            decoder.texture = newFile;
            decoder.run();
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
        // Overlay residency can change on the next camera tile. Load the map
        // synchronously so eviction cannot race a detached MapLib thread.
        t->load();
        delete t;
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

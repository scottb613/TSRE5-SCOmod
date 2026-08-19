/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ShapeLib.h"
#include <QDir>
#include "Game.h"
#include <QDebug>
#include "SFile.h"

//int ShapeLib::jestshape;
//std::unordered_map<int, SFile*> ShapeLib::shape;

ShapeLib::ShapeLib() {
}

ShapeLib::ShapeLib(const ShapeLib& orig) {
}

ShapeLib::~ShapeLib() {
    for(auto entry = shape.begin(); entry != shape.end(); ++entry)
        delete entry->second;
    shape.clear();
    jestshape = 0;
}

void ShapeLib::reset() {
    for(auto entry = shape.begin(); entry != shape.end(); ++entry)
        delete entry->second;
    jestshape = 0;
    shape.clear();
}
        
void ShapeLib::delRef(int texx) {
    /*if(!mtex.containsKey(texx)) return;
    mtex.get(texx).ref--;
    if(mtex.get(texx).ref<=0){

        if(mtex.get(texx).glLoaded){
            mtex.get(texx).delVBO(gl);
            mtex.remove(texx);
        }
    }*/
}
        
void ShapeLib::addRef(int texx) {
    //if(!mtex.containsKey(texx)) return;
    //mtex.get(texx).ref++;
}

int ShapeLib::addShape(QString path){
    return addShape(path, Game::root+"/routes/"+Game::route+"/textures");
}       

int ShapeLib::addShape(QString path, QString texPath) {
    QString pathid = path;//(path + "/" + name).toLower();
    pathid.replace("\\", "/");
    pathid.replace("//", "/");
    //console.log(pathid);
    for ( auto it = shape.begin(); it != shape.end(); ++it ){
        if(it->second == NULL) continue;
        if (((SFile*) it->second)->pathid.length() == pathid.length())
            if (((SFile*) it->second)->pathid == pathid) {
                ((SFile*) it->second)->ref++;
                return (int)it->first;
            }
    }
    if(Game::debugOutput) qDebug() << "Nowy " << jestshape << " shape: " << pathid;

    shape[jestshape] = new SFile(pathid, path.split("/").last(), texPath);
    shape[jestshape]->pathid = pathid;
    
   
    
    return jestshape++;      
}

bool ShapeLib::reloadShapeIfCached(QString path) {
    path = QDir::cleanPath(path);
    path.replace("\\", "/");

    for(auto it = shape.begin(); it != shape.end(); ++it) {
        SFile *cachedShape = it->second;
        if(cachedShape == NULL)
            continue;

        QString cachedPath = QDir::cleanPath(cachedShape->pathid);
        cachedPath.replace("\\", "/");
        if(cachedPath.compare(path, Qt::CaseInsensitive) != 0)
            continue;

        if(cachedShape->loaded == 1)
            cachedShape->reload();
        return true;
    }
    return false;
}

void ShapeLib::refreshSeasonTextures() {
    for (auto it = shape.begin(); it != shape.end(); ++it) {
        if (it->second == NULL)
            continue;
        it->second->refreshSeasonTextures();
    }
}

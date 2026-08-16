/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TerrainLibQt.h"
#include "Terrain.h"
#include "GLMatrix.h"
#include <QOpenGLShaderProgram>
#include <algorithm>
#include <set>
#include <limits>
#include <math.h>
#include "Game.h"
#include "Brush.h"
#include "HeightWindow.h"
#include "QuadTree.h"
#include "Undo.h"
#include "Route.h"
#include "Environment.h"
#include "TerrainInfo.h"
#include "Renderer.h"
#include "TexLib.h"
#include "TerrainTrackMath.h"
#include "MapWindow.h"

TerrainLibQt::TerrainLibQt() {
}

TerrainLibQt::TerrainLibQt(const TerrainLibQt&) : TerrainLib() {
}

TerrainLibQt::~TerrainLibQt() {
}

void TerrainLibQt::setDetailedAsCurrent(){
    currentQt = &terrainQt;
    currentQuadTree = quadTree;
}

void TerrainLibQt::setDistantAsCurrent(){
    currentQt = &terrainQtLo;
    currentQuadTree = quadTreeLo;
}

Terrain* TerrainLibQt::getTerrainByXY(int x, int y, bool load) {
    if(currentQuadTree == NULL)
        currentQuadTree = quadTree;
    if(currentQt == NULL)
        currentQt = &terrainQt;
    
    
    //QString terrainName = currentQuadTree->getMyName((int) x, -y);
    unsigned int terrainNameId = currentQuadTree->getMyNameId((int) x, -y);

    if (terrainNameId == 0)
        return NULL;
    if ((*currentQt)[terrainNameId] != NULL) {
        if((*currentQt)[terrainNameId]->t != NULL)
            return (*currentQt)[terrainNameId]->t;
    }
    if (load) {
        (*currentQt)[terrainNameId] = new TerrainInfo();
        currentQuadTree->fillTerrainInfo(x, -y, (*currentQt)[terrainNameId]);
        //qDebug() << terrainNameId;
        (*currentQt)[terrainNameId]->t = new Terrain((*currentQt)[terrainNameId]);
        mapOverlayResidencyValid = false;
        return (*currentQt)[terrainNameId]->t;
    }

    return NULL;
}

QuadTree* TerrainLibQt::getQuadTreeDetailed(){
    return quadTree;
}

QuadTree* TerrainLibQt::getQuadTreeDistant(){
    return quadTreeLo;
}

void TerrainLibQt::saveQtToStream(QTextStream &out){
    quadTree->save(out);
}

void TerrainLibQt::saveQtLoToStream(QTextStream &out){
    quadTreeLo->save(out);
}

void TerrainLibQt::loadQuadTree() {
    quadTree = new QuadTree();
    quadTree->load();

    quadTreeLo = new QuadTree(true);
    quadTreeLo->load();

    //currentQt = &terrainQtLo;
    //currentQuadTree = quadTreeLo;
    //quadTree->listNames();
}

void TerrainLibQt::loadQuadTreeDetailed(FileBuffer *data) {
    quadTree = new QuadTree();
    quadTree->load(data, false);
}

void TerrainLibQt::loadQuadTreeDistant(FileBuffer *data) {
    quadTreeLo = new QuadTree();
    quadTreeLo->load(data, false);
}

void TerrainLibQt::createNewRouteTerrain(int x, int z) {
    currentQuadTree = new QuadTree();
    currentQuadTree->createNew(x, z);
    QString name = currentQuadTree->getMyName(x, z);
    Terrain::SaveEmpty(name);
}

void TerrainLibQt::saveEmpty(int x, int z) {
    if(Game::debugOutput) qDebug() << "#new tile add to QT ";
    currentQuadTree->addTile(x, z);
    if(Game::debugOutput) qDebug() << "#new tile get name ";
    QString name = currentQuadTree->getMyName(x, z);
    if(Game::debugOutput) qDebug() << "#new tile Gen "<<name;
    if(currentQuadTree->isLow())
        Terrain::SaveEmpty(name, 256, 128, 16, true);
    else
        Terrain::SaveEmpty(name);
}

bool TerrainLibQt::isLoaded(int x, int z) {
    unsigned int terrainNameId = quadTree->getMyNameId((int) x, -z);
    if (terrainNameId == 0)
        return false;
    if ((*currentQt)[terrainNameId] == NULL)
        return false;
    if ((*currentQt)[terrainNameId]->t == NULL)
        return false;
    if ((*currentQt)[terrainNameId]->t->loaded == false)
        return false;
    return true;
}

bool TerrainLibQt::load(int x, int z) {
    Terrain *t = getTerrainByXY(x, z, true);
    if (t == NULL)
        return false;
    if (t->loaded == false)
        return false;
    return true;
}

void TerrainLibQt::getUnsavedInfo(QVector<QString> &items) {
    if (!Game::writeEnabled) return;
    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i.value()->t;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            items.push_back("[T] "+QString::number(tTile->mojex)+" "+QString::number(-tTile->mojez));
        }
    }
    QHashIterator<unsigned int, TerrainInfo*> i2(terrainQtLo);
    while (i2.hasNext()) {
        i2.next();
        if (i2.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i2.value()->t;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            items.push_back("[T] "+QString::number(tTile->mojex)+" "+QString::number(-tTile->mojez));
        }
    }
}

void TerrainLibQt::save() {
    if (!Game::writeEnabled) return;
    qDebug() << "save terrain";
    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i.value()->t;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            tTile->save();
            tTile->setModified(false);
        }
    }
    qDebug() << "save lo terrain";
    QHashIterator<unsigned int, TerrainInfo*> i2(terrainQtLo);
    while (i2.hasNext()) {
        i2.next();
        if (i2.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i2.value()->t;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            tTile->save();
            tTile->setModified(false);
        }
    }
}

bool TerrainLibQt::reload(int x, int z) {
    unsigned int terrainNameId = currentQuadTree->getMyNameId((int) x, -z);
    if (terrainNameId == 0)
        return false;

    (*currentQt)[terrainNameId] = new TerrainInfo();
    currentQuadTree->fillTerrainInfo(x, -z, (*currentQt)[terrainNameId]);
    (*currentQt)[terrainNameId]->t = new Terrain((*currentQt)[terrainNameId]);
    mapOverlayResidencyValid = false;
    if ((*currentQt)[terrainNameId]->t->loaded)
        return true;
    return false;
}

int TerrainLibQt::reloadLoaded() {
    std::set<std::pair<int, int>> detailedTiles;
    std::set<std::pair<int, int>> lowTiles;

    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i.value()->t;
        if (tTile == NULL || !tTile->loaded) continue;
        detailedTiles.insert(std::make_pair((int)tTile->mojex, (int)tTile->mojez));
    }

    QHashIterator<unsigned int, TerrainInfo*> i2(terrainQtLo);
    while (i2.hasNext()) {
        i2.next();
        if (i2.value() == NULL) continue;
        Terrain* tTile = (Terrain*) i2.value()->t;
        if (tTile == NULL || !tTile->loaded) continue;
        lowTiles.insert(std::make_pair((int)tTile->mojex, (int)tTile->mojez));
    }

    int reloaded = 0;
    for (std::set<std::pair<int, int>>::iterator it = detailedTiles.begin(); it != detailedTiles.end(); ++it) {
        if (quadTree == NULL)
            continue;
        unsigned int terrainNameId = quadTree->getMyNameId(it->first, -it->second);
        if (terrainNameId == 0)
            continue;

        terrainQt[terrainNameId] = new TerrainInfo();
        quadTree->fillTerrainInfo(it->first, -it->second, terrainQt[terrainNameId]);
        terrainQt[terrainNameId]->t = new Terrain(terrainQt[terrainNameId]);
        mapOverlayResidencyValid = false;
        if (terrainQt[terrainNameId]->t->loaded)
            reloaded++;
    }

    for (std::set<std::pair<int, int>>::iterator it = lowTiles.begin(); it != lowTiles.end(); ++it) {
        if (quadTreeLo == NULL)
            continue;
        unsigned int terrainNameId = quadTreeLo->getMyNameId(it->first, -it->second);
        if (terrainNameId == 0)
            continue;

        terrainQtLo[terrainNameId] = new TerrainInfo();
        quadTreeLo->fillTerrainInfo(it->first, -it->second, terrainQtLo[terrainNameId]);
        terrainQtLo[terrainNameId]->t = new Terrain(terrainQtLo[terrainNameId]);
        if (terrainQtLo[terrainNameId]->t->loaded)
            reloaded++;
    }

    return reloaded;
}

float TerrainLibQt::getHeight(int x, int z, float posx, float posz) {
    return TerrainLibQt::getHeight(x, z, posx, posz, false);
}

void TerrainLibQt::refresh(int x, int z) {
    Terrain *terr = this->getTerrainByXY(x, z);

    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->refresh();
}

void TerrainLibQt::setHeight(int x, int z, float posx, float posz, float h) {
    Game::check_coords(x, z, posx, posz);
    Terrain *terr = this->getTerrainByXY(x, z);

    if (terr == NULL) return;
    if (terr->loaded == false) return;
    
    terr->setHeight(x, z, posx, posz, h);
}

Terrain* TerrainLibQt::setHeight256(int x, int z, int posx, int posz, float h) {
    return setHeight256(x, z, posx, posz, h, 0, 0);
}

Terrain* TerrainLibQt::setHeight256(int x, int z, int posx, int posz, float h, float diffC, float diffE) {
    Game::check_coords(x, z, posx, posz);
    Terrain *terr = getTerrainByXY(x, z);

    if (terr == NULL) return NULL;
    if (terr->loaded == false) return NULL;

    float lx = posx, lz = posz;
    terr->getLocalCoords(x, z, lx, lz);
    int sampleSize = terr->getSampleSize();
    posx = lx / sampleSize;
    posz = lz / sampleSize;
    
    if(diffC == 0 && diffE == 0){
        terr->terrainData[(posz)][(posx)] = h;
    } else {
        if(terr->terrainData[(posz)][(posx)] < h)
            if(terr->terrainData[(posz)][(posx)] < h - diffE) 
                terr->terrainData[(posz)][(posx)] = h - diffE;
        if(terr->terrainData[(posz)][(posx)] > h)
            if(terr->terrainData[(posz)][(posx)] > h + diffC) 
                terr->terrainData[(posz)][(posx)] = h + diffC;
    }
    terr->setErrorBias(x, z, posx, posz, 0);
    terr->setModified(true);
    
    return terr;
}

float TerrainLibQt::getHeight(int x, int z, float posx, float posz, bool addR) {
    Game::check_coords(x, z, posx, posz);
    
    Terrain *terr = getTerrainByXY(x, z, false);

    if (terr == NULL) return -1;
    if (terr->loaded == false) return -1;

    return terr->getHeight(x, z, posx, posz, addR);
}

void TerrainLibQt::fillHeightMap(int x, int z, float* data) {
    Terrain *terr = getTerrainByXY(x, z, false);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->fillHeightMap(data);
}

void TerrainLibQt::getRotation(float* rot, int x, int z, float posx, float posz) {
    Game::check_coords(x, z, posx, posz);
    rot[0] = 0;
    rot[1] = 0;
    
    Terrain *terr = getTerrainByXY(x, z, false);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->getRotation(rot, x, z, posx, posz);
    return;
}

void TerrainLibQt::setHeightFromGeoGui(int x, int z, float* p) {
    if(heightWindow == NULL)
        heightWindow = new HeightWindow();
    
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;

    int X, Y;
    terr->getLowCornerTileXY(X, Y);
    heightWindow->tileX = X;
    heightWindow->tileZ = -Y;
    heightWindow->ok = false;
    int samples = terr->getSampleCount();
    heightWindow->terrainResolution = samples;
    heightWindow->terrainSize = terr->getSampleCount()*terr->getSampleSize();
    heightWindow->exec();
    if(heightWindow->ok){
        if(Game::debugOutput) qDebug() << "ok";
        for (int i = 0; i < samples; i++) {
            for (int j = 0; j < samples; j++) {
                terr->terrainData[i][j] = heightWindow->terrainData[j][i];
            }
        }
        terr->setModified(true);
        int X, Y;
        for(int i = -1; i <= 1; i++)
            for(int j = -1; j<= 1; j++){
                terr->getCornerCoordsXY(X, Y, i, j);
                Terrain* tterr  = getTerrainByXY(X, Y);
                if (tterr != NULL) 
                    tterr->refresh();
            }
        updateTerrainHeightmap(terr);
    }
}

void TerrainLibQt::setHeightFromGeo(int x, int z, float* p) {
    if(heightWindow == NULL)
        heightWindow = new HeightWindow();
    
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    int X, Y;
    terr->getLowCornerTileXY(X, Y);
    heightWindow->tileX = X;
    heightWindow->tileZ = -Y;
    heightWindow->ok = false;
    int samples = terr->getSampleCount();
    heightWindow->terrainResolution = samples;
    heightWindow->terrainSize = terr->getSampleCount()*terr->getSampleSize();
    heightWindow->load(false);
    if(heightWindow->ok){
        if(Game::debugOutput) qDebug() << "ok";
        for (int i = 0; i < samples; i++) {
            for (int j = 0; j < samples; j++) {
                terr->terrainData[i][j] = heightWindow->terrainData[j][i];
            }
        }
        terr->setModified(true);
        //terr->refresh();
        int X, Y;
        terr->getCornerCoordsXY(X, Y, 0, 1);
        Terrain* tterr;
        tterr = getTerrainByXY(X, Y);
        if (tterr != NULL) 
            tterr->refresh();
        terr->getCornerCoordsXY(X, Y, 0, -1);
        tterr = getTerrainByXY(X, Y);
        if (tterr != NULL) 
            tterr->refresh();
        terr->getCornerCoordsXY(X, Y, 1, 0);
        tterr = getTerrainByXY(X, Y);
        if (tterr != NULL) 
            tterr->refresh();
        terr->getCornerCoordsXY(X, Y, -1, 0);
        tterr = getTerrainByXY(X, Y);
        if (tterr != NULL) 
            tterr->refresh();
        updateTerrainHeightmap(terr);
    }
}

void TerrainLibQt::setTextureToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz) {
    QSet<Terrain*> touchedTerrains;
    float posx, posz;
    int ttx, ttz;
    for(int i = 0; i < length; i+=3 ){
        posx = punkty[i];
        posz = punkty[i+2];
        ttx = tx;
        ttz = tz;
        Game::check_coords(ttx, ttz, posx, posz);
        Terrain *terr = this->getTerrainByXY(ttx, ttz);
        if (terr == NULL)
            continue;
        if (terr->loaded == false) continue;
        terr->paintTexture(brush, ttx, ttz, posx, posz);
        touchedTerrains.insert(terr);
    }

    foreach (Terrain *terr, touchedTerrains) {
        updateTerrainTFile(terr);
    }
}

void TerrainLibQt::setTerrainToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz, float* matrix, float offsetY) {
    QSet<Terrain*> uterr;
    if(brush == NULL || punkty == NULL || length < 3)
        return;
    (void)matrix;

    // calculating plane equation
    float p1[3];
    float p2[3];
    
    p1[0] = punkty[0];
    p1[1] = punkty[1];
    p1[2] = punkty[2];
    p2[0] = punkty[length-3];
    p2[1] = punkty[length-2];
    p2[2] = punkty[length-1];
    if(Game::debugOutput) qDebug() << p1[0] << " " << p1[1] <<" " << p1[2];
    if(Game::debugOutput) qDebug() << p2[0] << " " << p2[1] <<" " << p2[2];

    const float gridSize = TerrainTrackMath::GridSize;
    const float bedHalfWidth = TerrainTrackMath::bedHalfWidth(brush->eSize);
    const float influenceRadius = TerrainTrackMath::conformInfluenceRadius(bedHalfWidth, brush->eRadius);
    TerrainTrackMath::Bounds bounds = TerrainTrackMath::boundsForTrack(punkty, length);
    const int startX = TerrainTrackMath::gridStart(bounds.minX, influenceRadius);
    const int endX = TerrainTrackMath::gridEnd(bounds.maxX, influenceRadius);
    const int startZ = TerrainTrackMath::gridStart(bounds.minZ, influenceRadius);
    const int endZ = TerrainTrackMath::gridEnd(bounds.maxZ, influenceRadius);

    QSet<Terrain*> undoTerrains;

    for(int gx = startX; gx <= endX; gx += (int)gridSize){
        for(int gz = startZ; gz <= endZ; gz += (int)gridSize){
            float distance;
            float trackHeight;
            if(!TerrainTrackMath::nearestTrack(punkty, length, (float)gx, (float)gz, distance, trackHeight))
                continue;
            if(distance > influenceRadius)
                continue;

            int ttx = tx;
            int ttz = tz;
            int posx = gx;
            int posz = gz;
            Game::check_coords(ttx, ttz, posx, posz);
            Terrain *terr = getTerrainByXY(ttx, ttz);
            if(terr == NULL || !terr->loaded)
                continue;

            if(!undoTerrains.contains(terr)){
                Undo::PushTerrainHeightMap(terr->mojex, terr->mojez, terr->terrainData, terr->getSampleCount());
                undoTerrains.insert(terr);
            }

            float lx = posx;
            float lz = posz;
            terr->getLocalCoords(ttx, ttz, lx, lz);
            int sampleSize = terr->getSampleSize();
            int sx = lx / sampleSize;
            int sz = lz / sampleSize;
            float originalHeight = terr->terrainData[sz][sx];
            float targetHeight = trackHeight + offsetY;

            if(distance > bedHalfWidth){
                float shoulderWidth = originalHeight > targetHeight
                    ? TerrainTrackMath::shoulderWidth(influenceRadius, bedHalfWidth, brush->eCut)
                    : TerrainTrackMath::shoulderWidth(influenceRadius, bedHalfWidth, brush->eEmb);
                if(distance > bedHalfWidth + shoulderWidth)
                    continue;

                float blend = TerrainTrackMath::smoothStep((distance - bedHalfWidth) / shoulderWidth);
                targetHeight = targetHeight + (originalHeight - targetHeight) * blend;
            }

            uterr.insert(setHeight256(tx, tz, gx, gz, targetHeight));
        }
    }

    foreach (Terrain *value, uterr){
        if(value == NULL)
            continue;
        value->setModified(true);
        value->refresh();
        updateTerrainHeightmap(value);
    }
}

void TerrainLibQt::smoothTerrainToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz, float* matrix) {
    QSet<Terrain*> uterr;
    if(brush == NULL || punkty == NULL || length < 3)
        return;
    (void)matrix;

    const float gridSize = TerrainTrackMath::GridSize;
    const float bedHalfWidth = TerrainTrackMath::bedHalfWidth(brush->eSize);
    const float smoothStart = TerrainTrackMath::smoothStart(bedHalfWidth);
    const float influenceRadius = TerrainTrackMath::smoothInfluenceRadius(bedHalfWidth, brush->eRadius);
    const float smoothWidth = std::max(gridSize, influenceRadius - smoothStart);
    float strength = std::max(0.35f, brush->alpha);
    if(strength < 0.0f) strength = 0.0f;
    if(strength > 1.0f) strength = 1.0f;

    TerrainTrackMath::Bounds bounds = TerrainTrackMath::boundsForTrack(punkty, length);
    const int startX = TerrainTrackMath::gridStart(bounds.minX, influenceRadius);
    const int endX = TerrainTrackMath::gridEnd(bounds.maxX, influenceRadius);
    const int startZ = TerrainTrackMath::gridStart(bounds.minZ, influenceRadius);
    const int endZ = TerrainTrackMath::gridEnd(bounds.maxZ, influenceRadius);

    struct SmoothTarget {
        int gx;
        int gz;
        float height;
        Terrain* terrain;
    };
    QVector<SmoothTarget> targets;

    for(int gx = startX; gx <= endX; gx += (int)gridSize){
        for(int gz = startZ; gz <= endZ; gz += (int)gridSize){
            float distance;
            float trackHeight;
            if(!TerrainTrackMath::nearestTrack(punkty, length, (float)gx, (float)gz, distance, trackHeight))
                continue;
            if(distance > influenceRadius)
                continue;

            int ttx = tx;
            int ttz = tz;
            int posx = gx;
            int posz = gz;
            Game::check_coords(ttx, ttz, posx, posz);
            Terrain *terr = getTerrainByXY(ttx, ttz);
            if(terr == NULL || !terr->loaded)
                continue;

            float originalHeight = getHeight(tx, tz, gx, gz, false);
            if(distance <= bedHalfWidth){
                if(fabs(trackHeight - originalHeight) < 0.001f)
                    continue;

                SmoothTarget target;
                target.gx = gx;
                target.gz = gz;
                target.height = trackHeight;
                target.terrain = terr;
                targets.push_back(target);
                continue;
            }

            if(distance < smoothStart)
                continue;

            float weightedHeight = 0.0f;
            float weightTotal = 0.0f;

            for(int ox = -2; ox <= 2; ox++){
                for(int oz = -2; oz <= 2; oz++){
                    float sampleX = gx + ox * gridSize;
                    float sampleZ = gz + oz * gridSize;
                    float sampleHeight = getHeight(tx, tz, sampleX, sampleZ, false);
                    if(sampleHeight < -999.0f)
                        continue;
                    float sampleDistance;
                    float sampleTrackHeight;
                    if(!TerrainTrackMath::nearestTrack(punkty, length, sampleX, sampleZ, sampleDistance, sampleTrackHeight))
                        continue;
                    if(sampleDistance < smoothStart)
                        continue;
                    float kernelDistance = sqrt((float)(ox*ox + oz*oz));
                    float weight = 1.0f / (1.0f + kernelDistance);
                    if(ox == 0 && oz == 0)
                        weight = 2.0f;
                    weightedHeight += sampleHeight * weight;
                    weightTotal += weight;
                }
            }

            if(weightTotal <= 0.0f)
                continue;

            float averageHeight = weightedHeight / weightTotal;
            float fade = 1.0f - TerrainTrackMath::smoothStep((distance - smoothStart) / smoothWidth);
            float targetHeight = originalHeight + (averageHeight - originalHeight) * strength * fade;
            if(fabs(targetHeight - originalHeight) < 0.001f)
                continue;

            SmoothTarget target;
            target.gx = gx;
            target.gz = gz;
            target.height = targetHeight;
            target.terrain = terr;
            targets.push_back(target);
        }
    }

    QSet<Terrain*> undoTerrains;
    for(int i = 0; i < targets.size(); i++){
        Terrain* terr = targets[i].terrain;
        if(terr == NULL || !terr->loaded)
            continue;
        if(!undoTerrains.contains(terr)){
            Undo::PushTerrainHeightMap(terr->mojex, terr->mojez, terr->terrainData, terr->getSampleCount());
            undoTerrains.insert(terr);
        }
        uterr.insert(setHeight256(tx, tz, targets[i].gx, targets[i].gz, targets[i].height));
    }

    foreach (Terrain *value, uterr){
        if(value == NULL)
            continue;
        value->setModified(true);
        value->refresh();
        updateTerrainHeightmap(value);
    }
}

void TerrainLibQt::setTerrainTexture(Brush* brush, int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setTexture(brush, x, z, posx, posz);
    updateTerrainTFile(terr);
}

void TerrainLibQt::toggleWaterDraw(int x, int z, float* p, float direction) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleWaterDraw(x, z, posx, posz, direction);
}

void TerrainLibQt::makeTextureFromMap(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->makeTextureFromMap();
}

void TerrainLibQt::removeTileTextureFromMap(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->removeTextureFromMap();
}

void TerrainLibQt::setTileBlob(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setTileBlob();
}

void TerrainLibQt::setRouteMapOverlayVisible(bool visible) {
    MapWindow::setRouteMapOverlaysVisible(visible);
    mapOverlayResidencyValid = false;
    mapOverlayUnavailableTiles.clear();
}

void TerrainLibQt::updateMapOverlayResidency(float *playerT) {
    const int centerX = (int)playerT[0];
    const int centerZ = (int)playerT[1];
    const int radius = qMax(0, Game::tileLod);
    const bool viewChanged = centerX != mapOverlayCenterX
            || centerZ != mapOverlayCenterZ || radius != mapOverlayRadius;
    if(mapOverlayResidencyValid && centerX == mapOverlayCenterX
            && centerZ == mapOverlayCenterZ && radius == mapOverlayRadius)
        return;
    if(viewChanged)
        mapOverlayUnavailableTiles.clear();

    struct OverlayCandidate {
        Terrain *terrain = NULL;
        int tileX = 0;
        int tileZ = 0;
        int distanceSquared = 0;
    };
    QVector<OverlayCandidate> candidates;

    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    while(i.hasNext()){
        i.next();
        TerrainInfo *info = i.value();
        if(info == NULL || info->t == NULL)
            continue;
        Terrain *terrain = (Terrain*)info->t;
        if(!terrain->loaded || terrain->lowTile)
            continue;
        int tileX, tileZ;
        terrain->getLowCornerTileXY(tileX, tileZ);
        const bool inRadius = qAbs(tileX-centerX) <= radius
                && qAbs(tileZ-centerZ) <= radius;
        const bool shouldShow = inRadius
                && MapWindow::mapOverlayVisibleForTile(tileX, tileZ);
        if(!shouldShow) {
            terrain->setMapOverlayVisible(false);
            continue;
        }
        const int hash = tileX*10000+tileZ;
        if(!terrain->showBlob && !mapOverlayUnavailableTiles.contains(hash)) {
            const int dx = tileX-centerX;
            const int dz = tileZ-centerZ;
            candidates.append({terrain, tileX, tileZ, dx*dx+dz*dz});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const OverlayCandidate &a, const OverlayCandidate &b) {
            return a.distanceSquared < b.distanceSquared;
        });
    constexpr int overlayLoadsPerPass = 1;
    const int loadCount = std::min(
        overlayLoadsPerPass, static_cast<int>(candidates.size()));
    for(int index = 0; index < loadCount; ++index) {
        OverlayCandidate &candidate = candidates[index];
        candidate.terrain->setMapOverlayVisible(true);
        if(!candidate.terrain->showBlob)
            mapOverlayUnavailableTiles.insert(
                candidate.tileX*10000+candidate.tileZ);
    }

    mapOverlayCenterX = centerX;
    mapOverlayCenterZ = centerZ;
    mapOverlayRadius = radius;
    mapOverlayResidencyValid = candidates.size() <= overlayLoadsPerPass;
}

void TerrainLibQt::setWaterLevelGui(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setWaterLevelGui();
}

void TerrainLibQt::toggleDraw(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleDraw(x, z, posx, posz);
    updateTerrainTFile(terr);
}

int TerrainLibQt::getTexture(int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return -1;
    if (terr->loaded == false) return -1;
    return terr->getTexture(x, z, posx, posz);
}

void TerrainLibQt::paintTexture(Brush* brush, int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->paintTexture(brush, x, z, posx, posz);
}

int TerrainLibQt::paintWaterEdges(Brush* brush, int x, int z) {
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return 0;
    if (terr->loaded == false) return 0;
    int paintedEdges = terr->paintWaterEdges(brush);
    if (paintedEdges > 0)
        updateTerrainTFile(terr);
    return paintedEdges;
}

void TerrainLibQt::lockTexture(Brush* brush, int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->lockTexture(brush, x, z, posx, posz);
}

void TerrainLibQt::toggleGaps(int x, int z, float* p, float direction) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleGaps(x, z, posx, posz, direction);
}

void TerrainLibQt::setFixedTileHeight(Brush* brush, int x, int z, float* p) {
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    
    Terrain *terr = this->getTerrainByXY(x, z);
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    Undo::PushTerrainHeightMap(terr->mojex, terr->mojez, terr->terrainData, terr->getSampleCount());
    terr->setFixedHeight(brush->hFixed);
    updateTerrainHeightmap(terr);
}

QSet<Terrain*> TerrainLibQt::paintHeightMap(Brush* brush, int x, int z, float* p) {

    QSet<Terrain*> uterr;
    
    float posx = round(p[0]/8.0)*8.0;
    float posz = round(p[2]/8.0)*8.0;
    
    Game::check_coords(x, z, posx, posz);
    if(Game::debugOutput) qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = getTerrainByXY(x, z);
    if (terr == NULL) return uterr;
    if (terr->loaded == false) return uterr;

    if(brush->hType == 5){
        if(brush->hFixed >= 0 || brush->size <= 0)
            return uterr;

        const int radius = brush->size;
        const int radiusSquared = radius * radius;
        const float inverseRadius = 1.0f / radius;
        QSet<Terrain*> undoTerrains;
        Terrain *cachedTerrain = NULL;
        int cachedX = std::numeric_limits<int>::min();
        int cachedZ = std::numeric_limits<int>::min();

        for(int i = -radius; i < radius; i++){
            const int iSquared = i * i;
            for(int j = -radius; j < radius; j++){
                const int distanceSquared = iSquared + j * j;
                if(distanceSquared > radiusSquared)
                    continue;

                int tx = x;
                int tz = z;
                int samplePosX = (int)posx + i * 8;
                int samplePosZ = (int)posz + j * 8;
                Game::check_coords(tx, tz, samplePosX, samplePosZ);

                if(tx != cachedX || tz != cachedZ){
                    cachedX = tx;
                    cachedZ = tz;
                    cachedTerrain = getTerrainByXY(tx, tz);
                }
                if(cachedTerrain == NULL || !cachedTerrain->loaded)
                    continue;

                if(!undoTerrains.contains(cachedTerrain)){
                    Undo::PushTerrainHeightMap(
                        cachedTerrain->mojex, cachedTerrain->mojez,
                        cachedTerrain->terrainData, cachedTerrain->getSampleCount());
                    undoTerrains.insert(cachedTerrain);
                }

                const float distance = std::sqrt((float)distanceSquared);
                const float strength = std::max(0.0f, std::min(1.0f,
                    (radius - distance) * inverseRadius * brush->alpha));
                if(cachedTerrain->paintWaterbedOffset(
                        tx, tz, samplePosX, samplePosZ,
                        brush->hFixed, strength))
                    uterr.insert(cachedTerrain);
            }
        }

        foreach(Terrain *value, uterr){
            value->setModified(true);
            value->refresh();
            updateTerrainHeightmap(value);
        }
        return uterr;
    }

    int px = posx;
    int pz = posz;
    float size = brush->size;
    float h = 0;
    float rd = 0;
    float hAvg = 0;
    int tx, tz;
    int tpx, tpz;
    int count = 0;
    
    // The brush is bounded to less than one terrain tile. Determine the small
    // set of touched tiles directly instead of scanning every brush sample once
    // for Undo and then scanning all samples again for painting.
    const int minTileX = x + (px - (int)size * 8 < -1024 ? -1 : 0);
    const int maxTileX = x + (px + ((int)size - 1) * 8 >= 1024 ? 1 : 0);
    const int minTileZ = z + (pz - (int)size * 8 < -1024 ? -1 : 0);
    const int maxTileZ = z + (pz + ((int)size - 1) * 8 >= 1024 ? 1 : 0);
    for(int undoX = minTileX; undoX <= maxTileX; undoX++){
        for(int undoZ = minTileZ; undoZ <= maxTileZ; undoZ++){
            Terrain *undoTerrain = getTerrainByXY(undoX, undoZ);
            if(undoTerrain == NULL || !undoTerrain->loaded)
                continue;
            Undo::PushTerrainHeightMap(
                undoTerrain->mojex, undoTerrain->mojez,
                undoTerrain->terrainData, undoTerrain->getSampleCount());
        }
    }
    
    terr = getTerrainByXY(x, z);
    h = brush->alpha*brush->direction*10.0;
    if(brush->hType == 1){
        rd = terr->setHeight(x, z, posx, posz, h, true);
    }
    if(brush->hType == 2){
        hAvg = brush->hFixed;
    }
    if(brush->hType == 3){
        for(int i = -size; i < size; i++)
            for(int j = -size; j < size; j++){
                tpx = px+i*8;
                tpz = pz+j*8;
                tx = x;
                tz = z;
                Game::check_coords(tx, tz, tpx, tpz);
                terr = getTerrainByXY(tx, tz);
                if (terr == NULL) continue;
                if (!terr->loaded) continue;
                float lx = tpx, lz = tpz;
                terr->getLocalCoords(tx, tz, lx, lz);
                int sampleSize = terr->getSampleSize();
                tpx = lx / sampleSize;
                tpz = lz / sampleSize;
                hAvg += terr->terrainData[tpz][tpx];
                count++;
            }
        hAvg /= count;
    }
    
    for(int i = -size; i < size; i++)
        for(int j = -size; j < size; j++){
            if(brush->hType == 1)
                if(i == 0 && j == 0) continue;
            tx = x;
            tz = z;
            tpx = px+i*8;
            tpz = pz+j*8;
            Game::check_coords(tx, tz, tpx, tpz);
            terr = getTerrainByXY(tx, tz);
            if (terr == NULL) continue;
            if (!terr->loaded) continue;
            uterr.insert(terr);
            
            const float distance = std::sqrt((float)(i*i + j*j));
            if(distance > size) continue;
            
            int sampleSize = terr->getSampleSize();
            float tileSizeMultipler = (8.0*8.0)/(sampleSize*sampleSize);
            h = (size - distance)/size;
            h = h*brush->alpha*brush->direction*10.0*tileSizeMultipler;;
            
            terr->setErrorBias(tx, tz, tpx, tpz, 0);
            float samplePosX = tpx;
            float samplePosZ = tpz;
            float lx = tpx, lz = tpz;
            terr->getLocalCoords(tx, tz, lx, lz);
            //qDebug() << tpx << lx << tpz << lz;
            
            tpx = lx / sampleSize;
            tpz = lz / sampleSize;
            if(brush->hType == 0){
                    terr->terrainData[tpz][tpx] += h;
            } else if(brush->hType == 1){
                if(h < 0){
                    if(terr->terrainData[tpz][tpx] > rd)
                        terr->terrainData[tpz][tpx] += h;
                }
                if(h > 0){
                    if(terr->terrainData[tpz][tpx] < rd)
                        terr->terrainData[tpz][tpx] += h;
                }
            } else if(brush->hType == 2 || brush->hType == 3){
                if(terr->terrainData[tpz][tpx] >hAvg){
                    terr->terrainData[tpz][tpx] -= h*brush->direction;
                    if(terr->terrainData[tpz][tpx] < hAvg)
                        terr->terrainData[tpz][tpx] = hAvg;
                }
                if(terr->terrainData[tpz][tpx] < hAvg){
                    terr->terrainData[tpz][tpx] += h*brush->direction;
                    if(terr->terrainData[tpz][tpx] > hAvg)
                        terr->terrainData[tpz][tpx] = hAvg;
                }
            } else if(brush->hType == 4 && Game::currentRoute != NULL){
                float samplePos[3] = {samplePosX, terr->terrainData[tpz][tpx], samplePosZ};
                float targetHeight = 0;
                float maxDbDistance = std::min(24.0f, std::max(8.0f, size * 8.0f));
                if(Game::currentRoute->findNearestDbHeight(tx, tz, samplePos, maxDbDistance, targetHeight)){
                    float strength = std::max(0.0f, std::min(1.0f,
                            ((size - distance) / size) * brush->alpha));
                    terr->terrainData[tpz][tpx] += (targetHeight - terr->terrainData[tpz][tpx]) * strength;
                }
            }
        }
    
    foreach (Terrain *value, uterr){
        value->setModified(true);
        value->refresh();
        updateTerrainHeightmap(value);
    }
    return uterr;
}

void TerrainLibQt::fillWaterLevels(float *w, int mojex, int mojez) {
    Terrain *cTile = getTerrainByXY(mojex, mojez);
    Terrain *tTile;
    int X, Y;
    
    cTile->getCornerCoordsXY(X, Y, -1, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[0] = tTile->getWaterLevelSE();
        }
    cTile->getCornerCoordsXY(X, Y, 0, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[1] = tTile->getWaterLevelSW();
            w[2] = tTile->getWaterLevelSE();
        }
    cTile->getCornerCoordsXY(X, Y, 1, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[3] = tTile->getWaterLevelSW();
        }
    cTile->getCornerCoordsXY(X, Y, -1, 0);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL) 
        if(tTile->loaded){
            w[4] = tTile->getWaterLevelNE();
            w[6] = tTile->getWaterLevelSE();
        }
    cTile->getCornerCoordsXY(X, Y, 1, 0);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[5] = tTile->getWaterLevelNW();
            w[7] = tTile->getWaterLevelSW();
        }
    cTile->getCornerCoordsXY(X, Y, -1, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[8] = tTile->getWaterLevelNE();
        }
    cTile->getCornerCoordsXY(X, Y, 0, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL) 
        if(tTile->loaded){
            w[9] = tTile->getWaterLevelNW();
            w[10] = tTile->getWaterLevelNE();
        }
    cTile->getCornerCoordsXY(X, Y, 1, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            w[11] = tTile->getWaterLevelNW();
        }
}

void TerrainLibQt::setWaterLevels(float *w, int mojex, int mojez) {
    Terrain *cTile = getTerrainByXY(mojex, mojez);
    if(!cTile->loaded)
        return;
    Terrain *tTile;
    int X, Y;
    
    cTile->getCornerCoordsXY(X, Y, -1, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSE(w[0]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, 0, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSW(w[1]);
            tTile->setWaterLevelSE(w[2]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, 1, -1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSW(w[3]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, -1, 0);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL) 
        if(tTile->loaded){
            tTile->setWaterLevelNE(w[4]);
            tTile->setWaterLevelSE(w[6]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, 1, 0);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[5]);
            tTile->setWaterLevelSW(w[7]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, -1, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNE(w[8]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, 0, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL) 
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[9]);
            tTile->setWaterLevelNE(w[10]);
            tTile->refreshWaterShapes();
        }
    cTile->getCornerCoordsXY(X, Y, 1, 1);
    tTile = getTerrainByXY(X, Y);
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[11]);
            tTile->refreshWaterShapes();
        }
}


void TerrainLibQt::setDetailedTerrainAsCurrent(){
    currentQuadTree = quadTree;
    currentQt = &terrainQt;
}

void TerrainLibQt::setLowTerrainAsCurrent(){
    currentQuadTree = quadTreeLo;
    currentQt = &terrainQtLo;
}

void TerrainLibQt::fillTerrainData(Terrain* tTile, float* offsetXYZ){
    ///QuadTree* tQuadTree = currentQuadTree;
    //tTile->mojex;
    //tTile->mojez;
    //QHash<int, Terrain*> tt;
    //Terrain *t;
    int x, z;
    float position[3];
    position[1] = offsetXYZ[1];
    
    float h = 0;
    tTile->setFixedHeight(200);
    for(int i = -1024; i < 1024; i+= 8)
        for(int j = -1024; j < 1024; j+= 8){
            x = tTile->mojex;
            z = tTile->mojez;
            position[0] = i - offsetXYZ[0];
            position[2] = j + offsetXYZ[2];
            while(position[0] > 1024 || position[0] < -1024 || position[2] > 1024 || position[2] < -1024 ){
                Game::check_coords(x, z, position);
            }
            if(position[0] == 1024){
                x++;
                position[0] = -1024;
            }
            if(position[2] == 1024){
                z++;
                position[2] = -1024;
            }
            //qDebug() << tTile->mojex << tTile->mojez << x << z << i << j << position[0] << position[2];
            Terrain *terr = getTerrainByXY(x, z, false);
            if (terr == NULL){
                //qDebug() << "NULL" << tTile->mojex << tTile->mojez << x << z;
            } else if(terr->loaded){
                //qDebug() << "fail not loaded" << x << z;
                h = terr->getHeight(x, z, position[0], position[2], false);
            }
            tTile->setHeight(tTile->mojex, tTile->mojez, i, j, h + offsetXYZ[1], false);
        }
    
    for(int i = -1023; i < 1024; i+= 128)
        for(int j = -1023; j < 1024; j+= 128){
            x = tTile->mojex;
            z = tTile->mojez;
            position[0] = i - offsetXYZ[0];
            position[2] = j + offsetXYZ[2];
            while(position[0] > 1024 || position[0] < -1024 || position[2] > 1024 || position[2] < -1024 ){
                Game::check_coords(x, z, position);
            }
            if(position[0] == 1024){
                x++;
                position[0] = -1024;
            }
            if(position[2] == 1024){
                z++;
                position[2] = -1024;
            }
            Terrain *terr = getTerrainByXY(x, z, false);
            if (terr == NULL){
            } else if(terr->loaded){
                QString tex = terr->getPatchMainTextureName(x, z, position[0], position[2]);
                tTile->setTexture(tex, tTile->mojex, tTile->mojez, i, j, terr->getPatchTexTransformString(x, z, position[0], position[2]));
                int flags = terr->getPatchFlags(x, z, position[0], position[2]);
                tTile->setPatchFlags(tTile->mojex, tTile->mojez, i, j, flags);
                if ((flags & 0xc0) != 0){
                    tTile->setAvgWaterLevel(terr->getAvgVaterLevel() + offsetXYZ[1]);
                }
            }
        }

}

void TerrainLibQt::fillRaw(Terrain *cTerr, int mojex, int mojez) {
    QuadTree* tQuadTree = currentQuadTree;
    QHash<unsigned int, TerrainInfo*> *tterrainQt = currentQt;
    
    if(cTerr->lowTile){
        currentQuadTree = quadTreeLo;
        currentQt = &terrainQtLo;
    } else {
        currentQuadTree = quadTree;
        currentQt = &terrainQt;
    }
    
    Terrain *tTile;
    int X, Y;
    cTerr->getCornerCoordsXY(X, Y, 1, 0);
    tTile = getTerrainByXY(X, Y, true);
    if(tTile != NULL)
        if (tTile->loaded) {
            cTerr->fillTerrainDataX(tTile);
        }

    cTerr->getCornerCoordsXY(X, Y, 0, 1);
    tTile = getTerrainByXY(X, Y, true);
    if(tTile != NULL)
        if (tTile->loaded) {
            cTerr->fillTerrainDataY(tTile);
        }

    cTerr->getCornerCoordsXY(X, Y, 1, 1);
    tTile = getTerrainByXY(X, Y, true);
    if(tTile != NULL)
        if (tTile->loaded) {
            cTerr->fillTerrainDataXY(tTile);
        }
    
    currentQuadTree = tQuadTree;
    currentQt = tterrainQt;
}

void TerrainLibQt::renderWater(GLUU* gluu, float* playerT, float* playerW, float* target, float fov, int renderMode, int layer) {
    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    int i = 0, j = 0;
    QHash<QString, bool> rendered;
    for (int n = -1; n < (Game::tileLod * 2 + 1)*(Game::tileLod * 2 + 1) - 1; n++) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

        tTile = getTerrainByXY((int) playerT[0] + i, (int) playerT[1] + j, true);
        if(tTile == NULL)
            continue;
        if (tTile->loaded == false)
            continue;
        if (rendered[tTile->name])
            continue;
        rendered[tTile->name] = true;

        if (tTile->loaded) {
            float lodx = 2048 * i - playerW[0];
            float lodz = 2048 * j - playerW[2];
            gluu->mvPushMatrix();
            Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, Game::currentRoute->env->water[layer].height, 2048 * j);
            gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
            if (renderMode == gluu->RENDER_SELECTION) {
                selectionColor = 10 << 20;
                selectionColor |= ((i + 1) << 10);
                selectionColor |= ((j + 1) << 8);
            }
            tTile->renderWater(lodx, lodz, playerT[0] + i, playerT[1] + j, playerW, target, fov, layer, selectionColor);
            gluu->mvPopMatrix();
        }
    }
}

void TerrainLibQt::renderWaterLo(GLUU* gluu, float* playerT, float* playerW, float* target, float fov, int renderMode, int layer) {
    int renderCount = 90*90 ;
    if (renderMode == gluu->RENDER_SELECTION) {
        renderCount = 9;
    }

    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    unsigned int terrainNameId;
    for (int n = -1, i = 0, j = 0; n < renderCount; n+=16) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

            terrainNameId = quadTreeLo->getMyNameId((int) playerT[0] + i, -(int) playerT[1] - j);
            if (terrainNameId == 0)
                continue;
            if (terrainQtLo[terrainNameId] == NULL) {
                terrainQtLo[terrainNameId] = new TerrainInfo();
                quadTreeLo->fillTerrainInfo((int) playerT[0] + i, -(int) playerT[1] - j, terrainQtLo[terrainNameId]);
                if(Game::debugOutput) qDebug() << "Terrain Name ID: " << terrainNameId;
                terrainQtLo[terrainNameId]->t = new Terrain(terrainQtLo[terrainNameId]);
            }
            if (terrainQtLo[terrainNameId]->rendered)
                continue;
            terrainQtLo[terrainNameId]->rendered = true;
            tTile = terrainQtLo[terrainNameId]->t;

            if (tTile->loaded == false)
                continue;

            if (tTile->loaded) {
                float lodx = 2048 * i - playerW[0];
                float lodz = 2048 * j - playerW[2];
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, Game::currentRoute->env->water[layer].height, 2048 * j);
                gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                if (renderMode == gluu->RENDER_SELECTION) {
                    selectionColor = 10 << 20;
                    selectionColor |= ((i + 1) << 10);
                    selectionColor |= ((j + 1) << 8);
                }
                tTile->renderWater(lodx, lodz, playerT[0] + i, playerT[1] + j, playerW, target, fov, layer, selectionColor);
                gluu->mvPopMatrix();
            }
        }

    QHashIterator<unsigned int, TerrainInfo*> i(terrainQtLo);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        i.value()->rendered = false;
    }
}

void TerrainLibQt::renderShadowMap(GLUU *gluu, float * playerT, float* playerW, float* target, float fov) {
    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int i = 0, j = 0;
    QHash<QString, bool> rendered;
    for (int n = -1; n < 9 - 1; n++) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

        tTile = getTerrainByXY((int) playerT[0] + i, (int) playerT[1] + j, true);
        if(tTile == NULL)
            continue;
        if (tTile->loaded == false)
            continue;
        if (rendered[tTile->name])
            continue;
        rendered[tTile->name] = true;
        
        if (tTile->loaded) {
            float lodx = 2048 * i - playerW[0];
            float lodz = 2048 * j - playerW[2];
            gluu->mvPushMatrix();
            Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
            gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
            tTile->render(lodx, lodz, playerT[0]+i, playerT[1]+j, playerW, target, fov, 1);
            gluu->mvPopMatrix();
        }
    }
}

void TerrainLibQt::renderEmpty(GLUU *gluu, float * playerT, float* playerW, float* target, float fov) {
    int i = 0, j = 0;
    for (int n = -1; n < 9 - 1; n++) {
        if (n != -1){
            spiralLoop(n, i, j);
        }
        getTerrainByXY((int) playerT[0] + i, (int) playerT[1] + j, true);
    }
}

void TerrainLibQt::pushRenderItems(float * playerT, float* playerW, float* target, float fov, int renderMode) {
    updateMapOverlayResidency(playerT);
    int renderCount = (Game::tileLod * 2 + 1)*(Game::tileLod * 2 + 1);
    if (renderMode == Game::currentRenderer->RENDER_SELECTION)
        renderCount = 9;

    //gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    //gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    QHash<QString, bool> rendered;
    
    for (int n = -1, i = 0, j = 0; n < renderCount - 1; n++) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

        tTile = getTerrainByXY((int) playerT[0] + i, (int) playerT[1] + j, true);
        if(tTile == NULL)
            continue;
        
        tTile->inUse = true;
        if (tTile->loaded == false)
            continue;
        if (rendered[tTile->name])
            continue;
        rendered[tTile->name] = true;

        if (tTile->loaded) {
            float lodx = 2048 * i - playerW[0];
            float lodz = 2048 * j - playerW[2];
            Game::currentRenderer->mvPushMatrix();
            Mat4::translate(Game::currentRenderer->mvMatrix, Game::currentRenderer->mvMatrix, 2048 * i, 0, 2048 * j);
            if (renderMode == Game::currentRenderer->RENDER_SELECTION) {
                selectionColor = 10 << 20;
                selectionColor |= ((i + 1) << 10);
                selectionColor |= ((j + 1) << 8);
            }
            tTile->pushRenderItem(lodx, lodz, playerT[0] + i, playerT[1] + j, playerW, target, fov, selectionColor);
            Game::currentRenderer->mvPopMatrix();
        }
    }

    updateMapOverlayResidency(playerT);
    
    if(renderMode == Game::currentRenderer->RENDER_SELECTION)
        return;
    
    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    /*while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        Terrain* obj = (Terrain*) i.value()->t;
        if(obj == NULL) continue;
        if(!obj->inUse && obj->loaded && !obj->isModified() && !obj->isSelected()){
           delete obj;
           i.value()->t = NULL;
       } else {
           obj->inUse = false;
       }
    }*/
}

void TerrainLibQt::render(GLUU *gluu, float * playerT, float* playerW, float* target, float fov, int renderMode) {
    updateMapOverlayResidency(playerT);
    int renderCount = (Game::tileLod * 2 + 1)*(Game::tileLod * 2 + 1);
    if (renderMode == gluu->RENDER_SELECTION)
        renderCount = 9;

    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    QHash<QString, bool> rendered;
    
    for (int n = -1, i = 0, j = 0; n < renderCount - 1; n++) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

        tTile = getTerrainByXY((int) playerT[0] + i, (int) playerT[1] + j, true);
        if(tTile == NULL)
            continue;
        
        tTile->inUse = true;
        if (tTile->loaded == false)
            continue;
        if (rendered[tTile->name])
            continue;
        rendered[tTile->name] = true;

        if (tTile->loaded) {
            float lodx = 2048 * i - playerW[0];
            float lodz = 2048 * j - playerW[2];
            gluu->mvPushMatrix();
            Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
            if (renderMode == gluu->RENDER_SELECTION) {
                selectionColor = 10 << 20;
                selectionColor |= ((i + 1) << 10);
                selectionColor |= ((j + 1) << 8);
            }
            tTile->render(lodx, lodz, playerT[0] + i, playerT[1] + j, playerW, target, fov, selectionColor);
            gluu->mvPopMatrix();
        }
    }

    updateMapOverlayResidency(playerT);
    
    if(renderMode == gluu->RENDER_SELECTION)
        return;
    
    QHashIterator<unsigned int, TerrainInfo*> i(terrainQt);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        Terrain* obj = (Terrain*) i.value()->t;
        if(obj == NULL) continue;
        if(!obj->inUse && obj->loaded && !obj->isModified() && !obj->isSelected()){
           delete obj;
           i.value()->t = NULL;
       } else {
           obj->inUse = false;
       }
    }

}

void TerrainLibQt::renderLo(GLUU *gluu, float * playerT, float* playerW, float* target, float fov, int renderMode) {
    int distantCount = Game::distantLod/1000 - 10;
    int renderCount = distantCount*distantCount ;
    if (renderMode == gluu->RENDER_SELECTION) {
        renderCount = 9;
    }

    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    unsigned int terrainNameId;
    for (int n = -1, i = 0, j = 0; n < renderCount; n+=16) {
        if (n != -1){
            spiralLoop(n, i, j);
        }

            terrainNameId = quadTreeLo->getMyNameId((int) playerT[0] + i, -(int) playerT[1] - j);
            if (terrainNameId == 0)
                continue;
            if (terrainQtLo[terrainNameId] == NULL) {
                terrainQtLo[terrainNameId] = new TerrainInfo();
                quadTreeLo->fillTerrainInfo((int) playerT[0] + i, -(int) playerT[1] - j, terrainQtLo[terrainNameId]);
                if(Game::debugOutput) qDebug() << "TID: " << terrainNameId;
                terrainQtLo[terrainNameId]->t = new Terrain(terrainQtLo[terrainNameId]);
            }
            if (terrainQtLo[terrainNameId]->rendered)
                continue;
            terrainQtLo[terrainNameId]->rendered = true;
            tTile = terrainQtLo[terrainNameId]->t;

            if (tTile->loaded == false)
                continue;

            if (tTile->loaded) {
                float lodx = 2048 * i - playerW[0];
                float lodz = 2048 * j - playerW[2];
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                if (renderMode == gluu->RENDER_SELECTION) {
                    selectionColor = 10 << 20;
                    selectionColor |= ((i + 1) << 10);
                    selectionColor |= ((j + 1) << 8);
                }
                tTile->render(lodx, lodz, playerT[0] + i, playerT[1] + j, playerW, target, fov, selectionColor);
                gluu->mvPopMatrix();
            }
        }

    QHashIterator<unsigned int, TerrainInfo*> i(terrainQtLo);
    while (i.hasNext()) {
        i.next();
        if (i.value() == NULL) continue;
        i.value()->rendered = false;
    }
}

void TerrainLibQt::spiralLoop(int n, int &x, int &y) {
    int r = floor((sqrt(n + 1) - 1) / 2) + 1;
    int p = (8 * r * (r - 1)) / 2;
    int en = r * 2;
    int a = (1 + n - p) % (r * 8);

    switch ((int) (floor((float) a / (r * 2)))) {
        case 0:
            x = a - r;
            y = -r;
            break;
        case 1:
            x = r;
            y = (a % en) - r;
            break;
        case 2:
            x = r - (a % en);
            y = r;
            break;
        case 3:
            x = -r;
            y = r - (a % en);
            break;
    }
}

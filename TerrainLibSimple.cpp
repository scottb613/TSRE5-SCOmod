/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TerrainLibSimple.h"
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
#include "MapWindow.h"
#include "TerrainTrackMath.h"

namespace {

template<typename TerrainGetter, typename Visitor>
void forEachNativeSimpleTerrainSample(TerrainGetter terrainGetter,
                                      int originTileX, int originTileZ,
                                      const TerrainTrackMath::Bounds &bounds,
                                      float radius,
                                      Visitor visitor) {
    const float minWorldX = bounds.minX - radius;
    const float maxWorldX = bounds.maxX + radius;
    const float minWorldZ = bounds.minZ - radius;
    const float maxWorldZ = bounds.maxZ + radius;
    const int minTileOffsetX = TerrainTrackMath::tileOffsetForCoordinate(minWorldX);
    const int maxTileOffsetX = TerrainTrackMath::tileOffsetForCoordinate(maxWorldX);
    const int minTileOffsetZ = TerrainTrackMath::tileOffsetForCoordinate(minWorldZ);
    const int maxTileOffsetZ = TerrainTrackMath::tileOffsetForCoordinate(maxWorldZ);

    for(int tileOffsetX = minTileOffsetX; tileOffsetX <= maxTileOffsetX; ++tileOffsetX){
        for(int tileOffsetZ = minTileOffsetZ; tileOffsetZ <= maxTileOffsetZ; ++tileOffsetZ){
            const int tileX = originTileX + tileOffsetX;
            const int tileZ = originTileZ + tileOffsetZ;
            Terrain *sampleTerrain = terrainGetter(tileX, tileZ);
            if(sampleTerrain == NULL || !sampleTerrain->loaded)
                continue;

            const int samples = sampleTerrain->getSampleCount();
            const int sampleSize = sampleTerrain->getSampleSize();
            if(samples <= 0 || sampleSize <= 0)
                continue;

            const int firstSampleX = TerrainTrackMath::firstNativeSampleIndex(
                minWorldX, tileOffsetX, sampleSize);
            const int lastSampleX = TerrainTrackMath::lastNativeSampleIndex(
                maxWorldX, tileOffsetX, sampleSize, samples);
            const int firstSampleZ = TerrainTrackMath::firstNativeSampleIndex(
                minWorldZ, tileOffsetZ, sampleSize);
            const int lastSampleZ = TerrainTrackMath::lastNativeSampleIndex(
                maxWorldZ, tileOffsetZ, sampleSize, samples);

            for(int sampleX = firstSampleX; sampleX <= lastSampleX; ++sampleX){
                const float worldX = TerrainTrackMath::nativeSampleCoordinate(
                    tileOffsetX, sampleX, sampleSize);
                for(int sampleZ = firstSampleZ; sampleZ <= lastSampleZ; ++sampleZ){
                    const float worldZ = TerrainTrackMath::nativeSampleCoordinate(
                        tileOffsetZ, sampleZ, sampleSize);
                    visitor(sampleTerrain, tileX, tileZ, sampleX, sampleZ,
                            sampleSize, worldX, worldZ);
                }
            }
        }
    }
}

void setNativeSimpleTerrainSampleHeight(Terrain *terrain,
                                        int tileX, int tileZ,
                                        int sampleX, int sampleZ,
                                        int sampleSize, float height) {
    terrain->terrainData[sampleZ][sampleX] = height;
    const float localX = sampleX * sampleSize - TerrainTrackMath::TileHalfSize;
    const float localZ = sampleZ * sampleSize - TerrainTrackMath::TileHalfSize;
    terrain->setErrorBias(tileX, tileZ, localX, localZ, 0);
    terrain->setModified(true);
}

}

TerrainLibSimple::TerrainLibSimple() {
}

TerrainLibSimple::TerrainLibSimple(const TerrainLibSimple& orig) {
}

TerrainLibSimple::~TerrainLibSimple() {
}

Terrain* TerrainLibSimple::getTerrainByXY(int x, int y, bool load){
    return terrain[x*10000+y];
}

void TerrainLibSimple::loadQuadTree(){
    quadTree = new QuadTree();
    quadTree->load();
}

void TerrainLibSimple::createNewRouteTerrain(int x, int z){
    quadTree = new QuadTree();
    quadTree->createNew(x, z);
    QString name = Terrain::getTileName(x, z);
    Terrain::SaveEmpty(name);
}

void TerrainLibSimple::saveEmpty(int x, int z){
    quadTree->addTile(x, z);
    QString name = Terrain::getTileName(x, z);
    Terrain::SaveEmpty(name);
}

bool TerrainLibSimple::isLoaded(int x, int z) {
    Terrain *tTile;

    tTile = terrain[((x)*10000 + z)];

    if (tTile == NULL)
        return false;

    if (tTile->loaded) {
        return true;
    }
    return false;
}

bool TerrainLibSimple::load(int x, int z) {
    Terrain* tTile = terrain[x*10000 + z];
    if (tTile == NULL) {
        terrain[x*10000 + z] = new Terrain(x, z);
        mapOverlayResidencyValid = false;
    }
    tTile = terrain[x*10000 + z];
    if (tTile == NULL)
        return false;
    if (tTile->loaded) {
        return true;
    }
    return false;
}

void TerrainLibSimple::getUnsavedInfo(QVector<QString> &items){
    if (!Game::writeEnabled) return;
    for (auto it = terrain.begin(); it != terrain.end(); ++it) {
        //console.log(obj.type);
        Terrain* tTile = (Terrain*) it->second;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            items.push_back("[T] "+QString::number(tTile->mojex)+" "+QString::number(-tTile->mojez));
        }
    }
}

void TerrainLibSimple::save(){
    if (!Game::writeEnabled) return;
    qDebug() << "save terrain";
    for (auto it = terrain.begin(); it != terrain.end(); ++it) {
        //console.log(obj.type);
        Terrain* tTile = (Terrain*) it->second;
        if (tTile == NULL) continue;
        if (tTile->loaded && tTile->isModified()) {
            tTile->save();
            tTile->setModified(false);
        }
    }
}

bool TerrainLibSimple::reload(int x, int z) {
    Terrain* tTile;// = terrain[x*10000 + z];
    //if (tTile == NULL) {
    terrain[x*10000 + z] = new Terrain(x, z);
    mapOverlayResidencyValid = false;
    //}
    tTile = terrain[x*10000 + z];
    if (tTile == NULL)
        return false;
    if (tTile->loaded) {
        return true;
    }
    return false;
}

float TerrainLibSimple::getHeight(int x, int z, float posx, float posz) {
    return TerrainLibSimple::getHeight(x, z, posx, posz, false);
}

void TerrainLibSimple::refresh(int x, int z) {
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];

    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->refresh();
}

void TerrainLibSimple::setHeight(int x, int z, float posx, float posz, float h) {
    Game::check_coords(x, z, posx, posz);
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];

    if (terr == NULL) return;
    if (terr->loaded == false) return;
    
    terr->setHeight(x, z, posx, posz, h);
}

Terrain* TerrainLibSimple::setHeight256(int x, int z, int posx, int posz, float h){
    return setHeight256(x, z, posx, posz, h, 0, 0);
}

Terrain* TerrainLibSimple::setHeight256(int x, int z, int posx, int posz, float h, float diffC, float diffE) {
    Game::check_coords(x, z, posx, posz);
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];

    if (terr == NULL) return NULL;
    if (terr->loaded == false) return NULL;
    
    float localX = posx;
    float localZ = posz;
    terr->getLocalCoords(x, z, localX, localZ);
    const int sampleSize = terr->getSampleSize();
    if(sampleSize <= 0)
        return NULL;
    const int sampleX = localX / sampleSize;
    const int sampleZ = localZ / sampleSize;

    if(diffC == 0 && diffE == 0){
        terr->terrainData[sampleZ][sampleX] = h;
    } else {
        if(terr->terrainData[sampleZ][sampleX] < h)
            if(terr->terrainData[sampleZ][sampleX] < h - diffE)
                terr->terrainData[sampleZ][sampleX] = h - diffE;
        if(terr->terrainData[sampleZ][sampleX] > h)
            if(terr->terrainData[sampleZ][sampleX] > h + diffC)
                terr->terrainData[sampleZ][sampleX] = h + diffC;
    }
    terr->setErrorBias(x, z, posx, posz, 0);
    terr->setModified(true);
    
    return terr;
}

float TerrainLibSimple::getHeight(int x, int z, float posx, float posz, bool addR) {
    Game::check_coords(x, z, posx, posz);

    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return -1;
    if (terr->loaded == false) return -1;

    return terr->getHeight(x, z, posx, posz, addR);
}

void TerrainLibSimple::fillHeightMap(int x, int z, float* data){
    Terrain *terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->fillHeightMap(data);
}

void TerrainLibSimple::getRotation(float* rot, int x, int z, float posx, float posz){
    Game::check_coords(x, z, posx, posz);
    rot[0] = 0;
    rot[1] = 0;
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    
    terr->getRotation(rot, x, z, posx, posz);
}

void TerrainLibSimple::setHeightFromGeoGui(int x, int z, float* p){
    if(heightWindow == NULL)
        heightWindow = new HeightWindow();
    
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;

    heightWindow->tileX = x;
    heightWindow->tileZ = -z;
    heightWindow->ok = false;
    const int samples = terr->getSampleCount();
    heightWindow->terrainResolution = samples;
    heightWindow->terrainSize = samples * terr->getSampleSize();
    heightWindow->exec();
    if(heightWindow->ok){
        qDebug() << "ok";
        for (int i = 0; i < samples; i++) {
            for (int j = 0; j < samples; j++) {
                terr->terrainData[i][j] = heightWindow->terrainData[j][i];
            }
        }
        terr->setModified(true);
        terr->refresh();
        terr = terrain[(x * 10000 + z + 1)];
        if (terr != NULL) terr->refresh();
        terr = terrain[(x * 10000 + z - 1)];
        if (terr != NULL) terr->refresh();
        terr = terrain[((x+1) * 10000 + z)];
        if (terr != NULL) terr->refresh();
        terr = terrain[((x-1) * 10000 + z)];
        if (terr != NULL) terr->refresh();
    }
}

void TerrainLibSimple::setHeightFromGeo(int x, int z, float* p){
    if(heightWindow == NULL)
        heightWindow = new HeightWindow();
    
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;

    heightWindow->tileX = x;
    heightWindow->tileZ = -z;
    heightWindow->ok = false;
    const int samples = terr->getSampleCount();
    heightWindow->terrainResolution = samples;
    heightWindow->terrainSize = samples * terr->getSampleSize();
    heightWindow->load(false);
    if(heightWindow->ok){
        qDebug() << "ok";
        for (int i = 0; i < samples; i++) {
            for (int j = 0; j < samples; j++) {
                terr->terrainData[i][j] = heightWindow->terrainData[j][i];
            }
        }
        terr->setModified(true);
        //terr->refresh();
        terr = terrain[(x * 10000 + z + 1)];
        if (terr != NULL) terr->refresh();
        terr = terrain[(x * 10000 + z - 1)];
        if (terr != NULL) terr->refresh();
        terr = terrain[((x+1) * 10000 + z)];
        if (terr != NULL) terr->refresh();
        terr = terrain[((x-1) * 10000 + z)];
        if (terr != NULL) terr->refresh();
    }
}

void TerrainLibSimple::setTextureToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz){
    float posx, posz;
    int ttx, ttz;
    for(int i = 0; i < length; i+=3 ){
        posx = punkty[i];
        posz = punkty[i+2];
        ttx = tx;
        ttz = tz;
        Game::check_coords(ttx, ttz, posx, posz);
        
        Terrain *terr;
        terr = terrain[(ttx * 10000 + ttz)];
        if (terr == NULL) continue;
        if (terr->loaded == false) continue;
        terr->paintTexture(brush, ttx, ttz, posx, posz);
    }
}

void TerrainLibSimple::setTerrainToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz, float* matrix, float offsetY){
    QSet<Terrain*> uterr;
    if(brush == NULL || punkty == NULL || length < 3)
        return;
    (void)matrix;

    auto terrainGetter = [&](int tileX, int tileZ) {
        return terrain[(tileX * 10000 + tileZ)];
    };
    int referenceTileX = tx;
    int referenceTileZ = tz;
    float referenceX = punkty[0];
    float referenceZ = punkty[2];
    Game::check_coords(referenceTileX, referenceTileZ, referenceX, referenceZ);
    Terrain *referenceTerrain = terrainGetter(referenceTileX, referenceTileZ);
    const float gridSize = referenceTerrain != NULL && referenceTerrain->loaded
        ? referenceTerrain->getSampleSize()
        : TerrainTrackMath::GridSize;
    const float bedHalfWidth = TerrainTrackMath::bedHalfWidth(
        brush->eSize, gridSize);
    const float influenceRadius = TerrainTrackMath::conformInfluenceRadius(
        bedHalfWidth, brush->eRadius, gridSize);
    const TerrainTrackMath::Bounds bounds =
        TerrainTrackMath::boundsForTrack(punkty, length);

    QSet<Terrain*> undoTerrains;
    forEachNativeSimpleTerrainSample(terrainGetter, tx, tz, bounds,
        influenceRadius,
        [&](Terrain *terr, int tileX, int tileZ,
            int sampleX, int sampleZ, int sampleSize,
            float worldX, float worldZ) {
            float distance;
            float trackHeight;
            if(!TerrainTrackMath::nearestTrack(
                    punkty, length, worldX, worldZ, distance, trackHeight))
                return;
            if(distance > influenceRadius)
                return;

            const float originalHeight = terr->terrainData[sampleZ][sampleX];
            const float targetHeight = TerrainTrackMath::conformEnvelopeHeight(
                originalHeight, trackHeight + offsetY, distance,
                bedHalfWidth, sampleSize, brush->eCut, brush->eEmb);
            if(fabs(targetHeight - originalHeight) < 0.001f)
                return;

            if(!undoTerrains.contains(terr)){
                Undo::PushTerrainHeightMap(
                    terr->mojex, terr->mojez,
                    terr->terrainData, terr->getSampleCount());
                undoTerrains.insert(terr);
            }

            setNativeSimpleTerrainSampleHeight(
                terr, tileX, tileZ, sampleX, sampleZ,
                sampleSize, targetHeight);
            uterr.insert(terr);
        });

    foreach (Terrain *value, uterr){
        if(value == NULL)
            continue;
        value->setModified(true);
        value->refresh();
    }
}

void TerrainLibSimple::smoothTerrainToTrackObj(Brush* brush, float* punkty, int length, int tx, int tz, float* matrix){
    QSet<Terrain*> uterr;
    if(brush == NULL || punkty == NULL || length < 3)
        return;
    (void)matrix;

    auto terrainGetter = [&](int tileX, int tileZ) {
        return terrain[(tileX * 10000 + tileZ)];
    };
    int referenceTileX = tx;
    int referenceTileZ = tz;
    float referenceX = punkty[0];
    float referenceZ = punkty[2];
    Game::check_coords(referenceTileX, referenceTileZ, referenceX, referenceZ);
    Terrain *referenceTerrain = terrainGetter(referenceTileX, referenceTileZ);
    const float gridSize = referenceTerrain != NULL && referenceTerrain->loaded
        ? referenceTerrain->getSampleSize()
        : TerrainTrackMath::GridSize;
    const float bedHalfWidth = TerrainTrackMath::bedHalfWidth(
        brush->eSize, gridSize);
    const float smoothStart = TerrainTrackMath::smoothStart(
        bedHalfWidth, gridSize);
    const float influenceRadius = TerrainTrackMath::smoothInfluenceRadius(
        bedHalfWidth, brush->eRadius, gridSize);
    const float smoothWidth = std::max(gridSize, influenceRadius - smoothStart);
    float strength = std::max(0.35f, brush->alpha);
    if(strength < 0.0f) strength = 0.0f;
    if(strength > 1.0f) strength = 1.0f;

    const TerrainTrackMath::Bounds bounds =
        TerrainTrackMath::boundsForTrack(punkty, length);
    struct SmoothTarget {
        int tileX;
        int tileZ;
        int sampleX;
        int sampleZ;
        int sampleSize;
        float height;
        Terrain *terrain;
    };
    QVector<SmoothTarget> targets;

    forEachNativeSimpleTerrainSample(terrainGetter, tx, tz, bounds,
        influenceRadius,
        [&](Terrain *terr, int tileX, int tileZ,
            int sampleX, int sampleZ, int sampleSize,
            float worldX, float worldZ) {
            float distance;
            float trackHeight;
            if(!TerrainTrackMath::nearestTrack(
                    punkty, length, worldX, worldZ, distance, trackHeight))
                return;
            if(distance > influenceRadius)
                return;

            const float originalHeight = terr->terrainData[sampleZ][sampleX];
            if(distance <= bedHalfWidth){
                if(fabs(trackHeight - originalHeight) < 0.001f)
                    return;
                targets.push_back({tileX, tileZ, sampleX, sampleZ,
                                   sampleSize, trackHeight, terr});
                return;
            }
            if(distance < smoothStart)
                return;

            float weightedHeight = 0.0f;
            float weightTotal = 0.0f;
            const float localX = sampleX * sampleSize - TerrainTrackMath::TileHalfSize;
            const float localZ = sampleZ * sampleSize - TerrainTrackMath::TileHalfSize;
            for(int ox = -2; ox <= 2; ++ox){
                for(int oz = -2; oz <= 2; ++oz){
                    const float neighborWorldX = worldX + ox * sampleSize;
                    const float neighborWorldZ = worldZ + oz * sampleSize;
                    const float sampleHeight = getHeight(
                        tileX, tileZ,
                        localX + ox * sampleSize,
                        localZ + oz * sampleSize, false);
                    if(sampleHeight < -999.0f)
                        continue;
                    float sampleDistance;
                    float sampleTrackHeight;
                    if(!TerrainTrackMath::nearestTrack(
                            punkty, length, neighborWorldX, neighborWorldZ,
                            sampleDistance, sampleTrackHeight))
                        continue;
                    if(sampleDistance < smoothStart)
                        continue;
                    const float kernelDistance =
                        sqrt((float)(ox * ox + oz * oz));
                    float weight = 1.0f / (1.0f + kernelDistance);
                    if(ox == 0 && oz == 0)
                        weight = 2.0f;
                    weightedHeight += sampleHeight * weight;
                    weightTotal += weight;
                }
            }
            if(weightTotal <= 0.0f)
                return;

            const float averageHeight = weightedHeight / weightTotal;
            const float fade = 1.0f - TerrainTrackMath::smoothStep(
                (distance - smoothStart) / smoothWidth);
            const float targetHeight = originalHeight
                + (averageHeight - originalHeight) * strength * fade;
            if(fabs(targetHeight - originalHeight) < 0.001f)
                return;
            targets.push_back({tileX, tileZ, sampleX, sampleZ,
                               sampleSize, targetHeight, terr});
        });

    QSet<Terrain*> undoTerrains;
    for(int i = 0; i < targets.size(); ++i){
        Terrain *terr = targets[i].terrain;
        if(terr == NULL || !terr->loaded)
            continue;
        if(!undoTerrains.contains(terr)){
            Undo::PushTerrainHeightMap(
                terr->mojex, terr->mojez,
                terr->terrainData, terr->getSampleCount());
            undoTerrains.insert(terr);
        }
        setNativeSimpleTerrainSampleHeight(
            terr, targets[i].tileX, targets[i].tileZ,
            targets[i].sampleX, targets[i].sampleZ,
            targets[i].sampleSize, targets[i].height);
        uterr.insert(terr);
    }

    foreach (Terrain *value, uterr){
        if(value == NULL)
            continue;
        value->setModified(true);
        value->refresh();
    }
}

void TerrainLibSimple::setTerrainTexture(Brush* brush, int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setTexture(brush, x, z, posx, posz);
}

void TerrainLibSimple::toggleWaterDraw(int x, int z, float* p, float direction){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleWaterDraw(x, z, posx, posz, direction);
}

void TerrainLibSimple::makeTextureFromMap(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->makeTextureFromMap();
}

void TerrainLibSimple::removeTileTextureFromMap(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->removeTextureFromMap();
}

void TerrainLibSimple::setTileBlob(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setTileBlob();
}

void TerrainLibSimple::setRouteMapOverlayVisible(bool visible){
    MapWindow::setRouteMapOverlaysVisible(visible);
    mapOverlayResidencyValid = false;
}

void TerrainLibSimple::updateMapOverlayResidency(float *playerT){
    const int centerX = (int)playerT[0];
    const int centerZ = (int)playerT[1];
    const int radius = qMax(0, Game::tileLod);
    if(mapOverlayResidencyValid && centerX == mapOverlayCenterX
            && centerZ == mapOverlayCenterZ && radius == mapOverlayRadius)
        return;

    for(auto &entry : terrain){
        Terrain *terr = entry.second;
        if(terr == NULL || !terr->loaded || terr->lowTile)
            continue;
        int tileX, tileZ;
        terr->getLowCornerTileXY(tileX, tileZ);
        const bool inRadius = qAbs(tileX-centerX) <= radius
                && qAbs(tileZ-centerZ) <= radius;
        terr->setMapOverlayVisible(inRadius
                && MapWindow::mapOverlayVisibleForTile(tileX, tileZ));
    }

    mapOverlayCenterX = centerX;
    mapOverlayCenterZ = centerZ;
    mapOverlayRadius = radius;
    mapOverlayResidencyValid = true;
}

void TerrainLibSimple::setWaterLevelGui(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->setWaterLevelGui();
}

void TerrainLibSimple::toggleDraw(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleDraw(x, z, posx, posz);
}

int TerrainLibSimple::getTexture(int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return -1;
    if (terr->loaded == false) return -1;
    return terr->getTexture(x, z, posx, posz);
}

void TerrainLibSimple::paintTexture(Brush* brush, int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->paintTexture(brush, x, z, posx, posz);
}

void TerrainLibSimple::lockTexture(Brush* brush, int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->lockTexture(brush, x, z, posx, posz);
}

void TerrainLibSimple::toggleGaps(int x, int z, float* p, float direction){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    qDebug() << x << " " << z << " " << posx << " " << posz;
    
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];
    if (terr == NULL) return;
    if (terr->loaded == false) return;
    terr->toggleGaps(x, z, posx, posz, direction);
}

void TerrainLibSimple::setFixedTileHeight(Brush* brush, int x, int z, float* p){
    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);
    Terrain *terr;
    terr = terrain[(x * 10000 + z)];

    if (terr == NULL) return;
    if (terr->loaded == false) return;
    Undo::PushTerrainHeightMap(terr->mojex, terr->mojez, terr->terrainData, terr->getSampleCount());
    terr->setFixedHeight(brush->hFixed);
}

QSet<Terrain*> TerrainLibSimple::paintHeightMap(Brush* brush, int x, int z, float* p){
    QSet<Terrain*> uterr;

    if(brush == NULL || p == NULL || brush->size <= 0)
        return uterr;

    float posx = p[0];
    float posz = p[2];
    Game::check_coords(x, z, posx, posz);

    auto terrainGetter = [&](int tileX, int tileZ) {
        return terrain[(tileX * 10000 + tileZ)];
    };

    Terrain *terr = terrainGetter(x, z);
    if(terr == NULL || !terr->loaded)
        return uterr;

    const int centerSampleSize = terr->getSampleSize();
    if(centerSampleSize <= 0)
        return uterr;
    posx = std::round(posx / centerSampleSize) * centerSampleSize;
    posz = std::round(posz / centerSampleSize) * centerSampleSize;
    Game::check_coords(x, z, posx, posz);
    terr = terrainGetter(x, z);
    if(terr == NULL || !terr->loaded)
        return uterr;

    if(Game::debugOutput)
        qDebug() << x << " " << z << " " << posx << " " << posz;

    const float legacyGridSize = TerrainTrackMath::GridSize;
    const float brushRadius = brush->size * legacyGridSize;
    TerrainTrackMath::Bounds brushBounds;
    brushBounds.minX = posx - brushRadius;
    brushBounds.maxX = posx + brushRadius;
    brushBounds.minZ = posz - brushRadius;
    brushBounds.maxZ = posz + brushRadius;

    if(brush->hType == 5 && brush->hFixed >= 0)
        return uterr;

    QSet<Terrain*> undoTerrains;
    forEachNativeSimpleTerrainSample(terrainGetter, x, z, brushBounds, 0.0f,
        [&](Terrain *sampleTerrain, int, int, int, int, int, float, float) {
            if(undoTerrains.contains(sampleTerrain))
                return;
            Undo::PushTerrainHeightMap(
                sampleTerrain->mojex, sampleTerrain->mojez,
                sampleTerrain->terrainData, sampleTerrain->getSampleCount());
            undoTerrains.insert(sampleTerrain);
        });

    if(undoTerrains.isEmpty())
        return uterr;

    if(brush->hType == 5){
        forEachNativeSimpleTerrainSample(terrainGetter, x, z, brushBounds, 0.0f,
            [&](Terrain *sampleTerrain, int tileX, int tileZ,
                int sampleX, int sampleZ, int sampleSize,
                float worldX, float worldZ) {
                const float dx = worldX - posx;
                const float dz = worldZ - posz;
                const float distance = std::sqrt(dx * dx + dz * dz);
                if(distance > brushRadius)
                    return;

                const float strength = std::max(0.0f, std::min(1.0f,
                    ((brushRadius - distance) / brushRadius) * brush->alpha));
                const float localX = sampleX * sampleSize - TerrainTrackMath::TileHalfSize;
                const float localZ = sampleZ * sampleSize - TerrainTrackMath::TileHalfSize;
                if(sampleTerrain->paintWaterbedOffset(
                        tileX, tileZ, localX, localZ,
                        brush->hFixed, strength))
                    uterr.insert(sampleTerrain);
            });
    } else {
        float referenceHeight = brush->hFixed;
        if(brush->hType == 3){
            referenceHeight = 0.0f;
            int sampleCount = 0;
            forEachNativeSimpleTerrainSample(terrainGetter, x, z, brushBounds, 0.0f,
                [&](Terrain *sampleTerrain, int, int,
                    int sampleX, int sampleZ, int,
                    float, float) {
                    referenceHeight += sampleTerrain->terrainData[sampleZ][sampleX];
                    ++sampleCount;
                });
            if(sampleCount == 0)
                return uterr;
            referenceHeight /= sampleCount;
        }

        float radiusHeight = 0.0f;
        if(brush->hType == 1){
            radiusHeight = terr->setHeight(
                x, z, posx, posz,
                brush->alpha * brush->direction * 10.0f, true);
            uterr.insert(terr);
        }

        forEachNativeSimpleTerrainSample(terrainGetter, x, z, brushBounds, 0.0f,
            [&](Terrain *sampleTerrain, int tileX, int tileZ,
                int sampleX, int sampleZ, int sampleSize,
                float worldX, float worldZ) {
                const float dx = worldX - posx;
                const float dz = worldZ - posz;
                const float distance = std::sqrt(dx * dx + dz * dz);
                if(distance > brushRadius)
                    return;
                if(brush->hType == 1 && distance < 0.001f)
                    return;

                uterr.insert(sampleTerrain);
                const float distanceInLegacySamples = distance / legacyGridSize;
                float heightDelta = (brush->size - distanceInLegacySamples) / brush->size;
                heightDelta *= brush->alpha * brush->direction * 10.0f;

                const float localX = sampleX * sampleSize - TerrainTrackMath::TileHalfSize;
                const float localZ = sampleZ * sampleSize - TerrainTrackMath::TileHalfSize;
                sampleTerrain->setErrorBias(tileX, tileZ, localX, localZ, 0);
                float &sampleHeight = sampleTerrain->terrainData[sampleZ][sampleX];

                if(brush->hType == 0){
                    sampleHeight += heightDelta;
                } else if(brush->hType == 1){
                    if(heightDelta < 0 && sampleHeight > radiusHeight)
                        sampleHeight += heightDelta;
                    if(heightDelta > 0 && sampleHeight < radiusHeight)
                        sampleHeight += heightDelta;
                } else if(brush->hType == 2 || brush->hType == 3){
                    if(sampleHeight > referenceHeight){
                        sampleHeight -= heightDelta * brush->direction;
                        if(sampleHeight < referenceHeight)
                            sampleHeight = referenceHeight;
                    }
                    if(sampleHeight < referenceHeight){
                        sampleHeight += heightDelta * brush->direction;
                        if(sampleHeight > referenceHeight)
                            sampleHeight = referenceHeight;
                    }
                } else if(brush->hType == 4 && Game::currentRoute != NULL){
                    float samplePos[3] = {localX, sampleHeight, localZ};
                    float targetHeight = 0.0f;
                    const float maxDbDistance = std::min(
                        24.0f, std::max(8.0f, brushRadius));
                    if(Game::currentRoute->findNearestDbHeight(
                            tileX, tileZ, samplePos, maxDbDistance, targetHeight)){
                        const float strength = std::max(0.0f, std::min(1.0f,
                            ((brushRadius - distance) / brushRadius) * brush->alpha));
                        sampleHeight += (targetHeight - sampleHeight) * strength;
                    }
                }
            });
    }

    foreach (Terrain *value, uterr){
        value->setModified(true);
        value->refresh();
    }
    return uterr;
}

void TerrainLibSimple::fillWaterLevels(float *w, int mojex, int mojez){
    Terrain *tTile;
    
    tTile = terrain[((mojex - 1)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[0] = tTile->getWaterLevelSE();
        }
    tTile = terrain[((mojex)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[1] = tTile->getWaterLevelSW();
            w[2] = tTile->getWaterLevelSE();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[3] = tTile->getWaterLevelSW();
        }
    tTile = terrain[((mojex - 1)*10000 + mojez)];
    if (tTile != NULL) 
        if(tTile->loaded){
            w[4] = tTile->getWaterLevelNE();
            w[6] = tTile->getWaterLevelSE();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[5] = tTile->getWaterLevelNW();
            w[7] = tTile->getWaterLevelSW();
        }
    tTile = terrain[((mojex - 1)*10000 + mojez + 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[8] = tTile->getWaterLevelNE();
        }
    tTile = terrain[((mojex)*10000 + mojez + 1)];
    if (tTile != NULL) 
        if(tTile->loaded){
            w[9] = tTile->getWaterLevelNW();
            w[10] = tTile->getWaterLevelNE();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez + 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            w[11] = tTile->getWaterLevelNW();
        }
}

void TerrainLibSimple::setWaterLevels(float *w, int mojex, int mojez){
    Terrain *tTile;
    
    tTile = terrain[((mojex - 1)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSE(w[0]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSW(w[1]);
            tTile->setWaterLevelSE(w[2]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez - 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelSW(w[3]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex - 1)*10000 + mojez)];
    if (tTile != NULL) 
        if(tTile->loaded){
            tTile->setWaterLevelNE(w[4]);
            tTile->setWaterLevelSE(w[6]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[5]);
            tTile->setWaterLevelSW(w[7]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex - 1)*10000 + mojez + 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNE(w[8]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex)*10000 + mojez + 1)];
    if (tTile != NULL) 
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[9]);
            tTile->setWaterLevelNE(w[10]);
            tTile->refreshWaterShapes();
        }
    tTile = terrain[((mojex + 1)*10000 + mojez + 1)];
    if (tTile != NULL)
        if(tTile->loaded){
            tTile->setWaterLevelNW(w[11]);
            tTile->refreshWaterShapes();
        }
}

void TerrainLibSimple::fillTerrainData(Terrain* tTile, float* offsetXYZ){
    // ToDo
}

void TerrainLibSimple::fillRaw(Terrain *cTerr, int mojex, int mojez) {
    Terrain *tTile;

    tTile = terrain[((mojex + 1)*10000 + mojez)];

    if (tTile == NULL) {
        terrain[(mojex + 1)*10000 + mojez] = new Terrain(mojex + 1, mojez);
        mapOverlayResidencyValid = false;
    }
    tTile = terrain[(mojex + 1)*10000 + mojez];
    if (tTile->loaded)
        cTerr->fillTerrainDataX(tTile);

    
    tTile = terrain[((mojex)*10000 + mojez + 1)];

    if (tTile == NULL) {
        terrain[(mojex)*10000 + mojez + 1] = new Terrain(mojex, mojez + 1);
        mapOverlayResidencyValid = false;
    }
    tTile = terrain[(mojex)*10000 + mojez + 1];
    if (tTile->loaded)
        cTerr->fillTerrainDataY(tTile);


    tTile = terrain[((mojex + 1)*10000 + (mojez + 1))];

    if (tTile == NULL) {
        terrain[(mojex + 1)*10000 + mojez + 1] = new Terrain(mojex + 1, mojez + 1);
        mapOverlayResidencyValid = false;
    }
    tTile = terrain[(mojex + 1)*10000 + mojez + 1];
    if (tTile->loaded)
        cTerr->fillTerrainDataXY(tTile);
}

void TerrainLibSimple::render(GLUU *gluu, float * playerT, float* playerW, float* target, float fov, int renderMode) {
    updateMapOverlayResidency(playerT);
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;
    
    if(renderMode == gluu->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }
    //TerrainLibSimple.render(playerT, playerW); 

    /*for (auto it = terrain.begin(); it != terrain.end(); ++it) {
        //console.log(obj.type);
        Terrain* obj = (Terrain*) it->second;
        if(obj == NULL) continue;
        obj->inUse = false;
    }*/
    
    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = terrain[(((int)playerT[0] + i)*10000 + (int)playerT[1] + j)];
            
            if (tTile == NULL) {
                terrain[((int)playerT[0] + i)*10000 + (int)playerT[1] + j] = new Terrain((int)playerT[0] + i, (int)playerT[1] + j);
                mapOverlayResidencyValid = false;
            }

            tTile = terrain[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            tTile->inUse = true;
            if (tTile->loaded == false) continue;
            //tTile->inUse = true;
            
            if (tTile->loaded) {
                float lodx = 2048 * i - playerW[0];
                float lodz = 2048 * j - playerW[2];
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                if (renderMode == gluu->RENDER_SELECTION) {
                    selectionColor = 10 << 20;
                    selectionColor |= ((i+1) << 10);
                    selectionColor |= ((j+1) << 8);
                }
                tTile->render(lodx, lodz, playerT[0]+i, playerT[1]+j, playerW, target, fov, selectionColor);
                gluu->mvPopMatrix();
            }
        }
    }

    updateMapOverlayResidency(playerT);
    
    if(renderMode == gluu->RENDER_SELECTION)
        return;
    
    mintile = -3;
    maxtile = 3;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = terrain[(((int)playerT[0] + i)*10000 + (int)playerT[1] + j)];
            if (tTile != NULL)
                tTile->inUse = true;
        }
    }
    
    for (auto it = terrain.begin(); it != terrain.end(); ++it) {
        //console.log(obj.type);
        Terrain* obj = (Terrain*) it->second;
        if(obj == NULL) continue;
        if(!obj->inUse && obj->loaded && !obj->isModified() && !obj->isSelected()){
           //console.log("a"+this.tile[key]);
           delete obj;
           terrain[(int)it->first] = NULL;
       } else {
           obj->inUse = false;
       }
    }
}

void TerrainLibSimple::renderWater(GLUU *gluu, float* playerT, float* playerW, float* target, float fov, int renderMode, int layer) {
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;
    
    if(renderMode == gluu->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }
    
    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    int selectionColor = 0;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = terrain[(((int)playerT[0] + i)*10000 + (int)playerT[1] + j)];
            if (tTile == NULL) 
                continue;
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
                    selectionColor |= ((i+1) << 10);
                    selectionColor |= ((j+1) << 8);
                }
                tTile->renderWater(lodx, lodz, playerT[0]+i, playerT[1]+j, playerW, target, fov, layer, selectionColor);
                gluu->mvPopMatrix();
            }
        }
    }
}

void TerrainLibSimple::renderShadowMap(GLUU *gluu, float * playerT, float* playerW, float* target, float fov) {
    int mintile = -1;
    int maxtile = 1;
    
    gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, 0.0f);
    gluu->enableNormals();

    Terrain *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = terrain[(((int)playerT[0] + i)*10000 + (int)playerT[1] + j)];
            
            if (tTile == NULL) {
                terrain[((int)playerT[0] + i)*10000 + (int)playerT[1] + j] = new Terrain((int)playerT[0] + i, (int)playerT[1] + j);
            }

            tTile = terrain[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            tTile->inUse = true;
            if (tTile->loaded == false) continue;
            //tTile->inUse = true;
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
}

void TerrainLibSimple::renderEmpty(GLUU *gluu, float * playerT, float* playerW, float* target, float fov) {
    int mintile = -1;
    int maxtile = 1;

    Terrain *tTile;
    for (int i = mintile; i <= maxtile; i++)
        for (int j = maxtile; j >= mintile; j--) {
            tTile = terrain[(((int)playerT[0] + i)*10000 + (int)playerT[1] + j)];
            if (tTile == NULL) {
                terrain[((int)playerT[0] + i)*10000 + (int)playerT[1] + j] = new Terrain((int)playerT[0] + i, (int)playerT[1] + j);
            }
     }
}

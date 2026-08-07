/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include <QDebug>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QStringConverter>
#include <QTextStream>
#include <functional>
#include "Route.h"
#include "TSectionDAT.h"
#include "GLUU.h"
#include "Tile.h"
#include "GLMatrix.h"
#include "TerrainLib.h"
#include "TerrainLibSimple.h"
#include "TerrainLibQt.h"
#include "TFile.h"
#include "Game.h"
#include "GuiFunct.h"
#include "TrackObj.h"
#include "TrWatermarkObj.h"
#include "RulerObj.h"
#include "Path.h"
#include "Terrain.h"
#include "FileFunctions.h"
#include "ParserX.h"
#include "ReadFile.h"
#include "DynTrackObj.h"
#include "PlatformObj.h"
#include "CarSpawnerObj.h"
#include "ForestObj.h"
#include "Coords.h"
#include "CoordsMkr.h"
#include "CoordsKml.h"
#include "CoordsGpx.h"
#include "CoordsRoutePlaces.h"
#include "SoundList.h"
#include "ActLib.h"
#include "Trk.h"
#include "AboutWindow.h"
#include "TrkWindow.h"
#include "PlatformObj.h"
#include "GroupObj.h"
#include "Undo.h"
#include "Activity.h"
#include "Service.h"
#include "Traffic.h"

#include "Path.h"
#include "Environment.h"
#include "OrtsWeatherChange.h"
#include "GeoCoordinates.h"
#include "Consist.h"
#include "Skydome.h"
#include "TRitem.h"
#include "ActionChooseDialog.h"
#include "ErrorMessagesWindow.h"
#include "ErrorMessagesLib.h"
#include "ErrorMessage.h"
#include "AceLib.h"
#include "Renderer.h"
#include "RenderItem.h"
#include "SpeedPostDAT.h"
#include "SigCfg.h"
#include "TDBClient.h"
#include "RouteEditorWindow.h"
#include "ShapeLib.h"
#include "SFile.h"
#include "RouteMergeDialog.h"
#include "RouteSaveTransaction.h"
#include "UnsafeModeDialog.h"
#include "TerrainTools.h" // EFO
#include <math.h>
#include <iostream>

namespace {
bool tdbTerrainBiasConfirmedThisSession = false;
bool rdbTerrainBiasConfirmedThisSession = false;

void clearRouteHealthInventories() {
    Route::fileList.clear();
    Route::trackList.clear();
    Route::shapesList.clear();
    Route::texturesList.clear();
    Route::missingList.clear();
    Route::staticFlagList.clear();
    Route::missingTextureList.clear();
}

QString activeRouteRoot() {
    return QDir::cleanPath(Game::root + "/routes/" + Game::route);
}

QString activeRouteBackupRoot() {
    return Game::appDataDir() + "/atomicSaves/" + Game::routeAppDataKey();
}

bool serializeUtf16Text(int precision,
                        const QString &signature,
                        const std::function<void(QTextStream &)> &writer,
                        QByteArray &data,
                        QString &error) {
    data.clear();
    QBuffer buffer(&data);
    if(!buffer.open(QIODevice::WriteOnly)){
        error = "Could not allocate the route save buffer.";
        return false;
    }
    QTextStream out(&buffer);
    out.setRealNumberPrecision(precision);
    out.setEncoding(QStringConverter::Utf16);
    out.setGenerateByteOrderMark(true);
    out << signature;
    writer(out);
    out.flush();
    if(out.status() != QTextStream::Ok){
        error = "Could not serialize one of the route key files.";
        return false;
    }
    buffer.close();
    return !data.isEmpty();
}

bool recoverRouteSaveIfRequired(QString &message) {
    return RouteSaveTransaction::recoverInterrupted(
                activeRouteRoot(), activeRouteBackupRoot(), &message);
}
}



Route::Route() {

}

/// These are static members that need to be instantiated outside of a function
    QStringList Route::fileList ;
    QStringList Route::trackList ;    
    QStringList Route::shapesList ;    
    QStringList Route::texturesList ;    
    QStringList Route::missingList;
    QStringList Route::staticFlagList;
    QStringList Route::missingTextureList;

void Route::load(){

    Game::currentRoute = this;
    trkName = Game::trkName;
    routeDir = Game::route;
    clearRouteHealthInventories();

    ///  Check for Unsafe early
    if(Game::UnsafeMode){
        this->confirmUnsafe();
    }

    
     if(Game::debugOutput) qDebug() << "# Load Route";
    
    if(!Game::useQuadTree)
        terrainLib = new TerrainLibSimple();
    else
        terrainLib = new TerrainLibQt();
    
    Game::terrainLib = terrainLib;
    
    QFile file(Game::root + "/routes");
    if (!file.exists()){ 
        if(Game::debugOutput)  qDebug() << "Route dir not exist " << file.fileName();
        return;
    }
    file.setFileName(Game::root + "/global");
    if (!file.exists()){ 
        if(Game::debugOutput) qDebug() << "Global dir not exist " << file.fileName();
        return;
    }

    file.setFileName(Game::root + "/routes/" + Game::route);
    if (!file.exists()) {
        if(Game::debugOutput) qDebug() << "Route does not exist.";
        if (Game::createNewRoutes) {
            if(Game::debugOutput) qDebug() << "new Route";
            Route::createNew();
        }
    }

    QString recoveryMessage;
    if(!recoverRouteSaveIfRequired(recoveryMessage)){
        qWarning() << "Route save recovery failed:" << recoveryMessage;
        if(Game::gui)
            QMessageBox::critical(NULL, QObject::tr("Route recovery failed"), recoveryMessage);
        return;
    }
    if(!recoveryMessage.isEmpty()){
        qWarning() << recoveryMessage;
        if(Game::gui)
            QMessageBox::information(NULL, QObject::tr("Route save recovered"), recoveryMessage);
    }

    trk = new Trk();
    trk->load();
    Game::useSuperelevation = trk->tsreSuperelevation;
    
    if(trk->tsreProjection != NULL){
        if(Game::debugOutput) qDebug() << "TSRE Geo Projection";
        Game::GeoCoordConverter = new GeoTsreCoordinateConverter(trk->tsreProjection);
    } else {
        if(Game::debugOutput) qDebug() << "MSTS Geo Projection";
        Game::GeoCoordConverter = new GeoMstsCoordinateConverter();
    }
    env = new Environment(Game::root + "/routes/" + Game::route + "/ENVFILES/editor.env");
    Game::routeName = trk->routeName.toLower();
    routeName = Game::routeName;
    if(Game::debugOutput) qDebug() << Game::routeName;

    this->tsection = new TSectionDAT();
    // Check Track Section Databaase
    if(!checkTrackSectionDatabase())
        return;
    
    if(Game::loadAllWFiles){
        preloadWFiles(Game::gui);
    }
    
    if((Game::UnsafeMode) && (Game::routeRebuildTDB)){
        TDB::saveEmpty(true);  /// true = road
        TDB::saveEmpty(false);  /// false = track
        qDebug() << "TDB/RDB backed up";
    }

    
    this->trackDB = new TDB(tsection, false); 
    this->trackDB->loadTdb(); 
    this->roadDB = new TDB(tsection, true); 
    this->roadDB->loadTdb(); 
    Game::trackDB = this->trackDB;
    Game::roadDB = this->roadDB;  
    loadAddons();    
    
    loadMkrList();
    loadServices();
    loadTraffic();
    loadPaths();
    if(Game::loadActivities)
        loadActivities();

    soundList = new SoundList();
    soundList->loadSoundSources(Game::root + "/routes/" + Game::route + "/ssource.dat");
    soundList->loadSoundRegions(Game::root + "/routes/" + Game::route + "/ttype.dat");
    Game::soundList = soundList;
    
    Game::terrainLib->loadQuadTree();
    OrtsWeatherChange::LoadList();
    qDebug() << "184";
    ForestObj::LoadForestList();
    ForestObj::ForestClearDistance = trk->forestClearDistance;
    CarSpawnerObj::LoadCarSpawnerList();
    //qDebug() << "188";
    if(Game::loadAllWFiles){        
        //qDebug() << "190";                 
        preloadWFilesInit();        
    }
    //qDebug() << "198";
    checkRouteDatabase();
    loaded = true;
    
    Vec3::set(placementAutoTranslationOffset, 0, 0, 0);
    Vec3::set(placementAutoRotationOffset, 0, 0, 0);
    
    skydome = new Skydome();
 
    // Route Merge. 
    if(Game::routeMergeString.length() > 0){
 
        confirmMerge();
    }
    
    if((Game::UnsafeMode) && (Game::routeRebuildTDB)){
        RebuildTDB();
    }
    
    
}


void Route::load(QString name){
    clearRouteHealthInventories();
    if(!Game::useQuadTree)
        terrainLib = new TerrainLibSimple();
    else
        terrainLib = new TerrainLibQt();
    
    QFile file(Game::root + "/routes");
    if (!file.exists()){ 
        if(Game::debugOutput) qDebug() << "Route dir not exist " << file.fileName();
        return;
    }
    file.setFileName(Game::root + "/global");
    if (!file.exists()){ 
        if(Game::debugOutput) qDebug() << "Global dir not exist " << file.fileName();
        return;
    }

    file.setFileName(Game::root + "/routes/" + name);
    if (!file.exists()) {
        if(Game::debugOutput) qDebug() << "Route does not exist.";
        return;
    }
    Game::route = name;
    Game::checkRoute(Game::route);
    routeDir = Game::route;
    trkName = Game::trkName;

    QString recoveryMessage;
    if(!recoverRouteSaveIfRequired(recoveryMessage)){
        qWarning() << "Route save recovery failed:" << recoveryMessage;
        return;
    }
    if(!recoveryMessage.isEmpty())
        qWarning() << recoveryMessage;

    trk = new Trk();
    trk->load();
    //Game::useSuperelevation = trk->tsreSuperelevation;
    
    
    /*if(trk->tsreProjection != NULL){
        qDebug() << "TSRE Geo Projection";
        Game::GeoCoordConverter = new GeoTsreCoordinateConverter(trk->tsreProjection);
    } else {
        qDebug() << "MSTS Geo Projection";
        Game::GeoCoordConverter = new GeoMstsCoordinateConverter();
    }
    env = new Environment(Game::root + "/routes/" + Game::route + "/ENVFILES/editor.env");*/
    Game::routeName = trk->routeName.toLower();
    routeName = Game::routeName;
    // qDebug() << Game::routeName;

    this->tsection = new TSectionDAT();
    
    if(Game::loadAllWFiles){
        preloadWFiles(true);
    }

    this->trackDB = new TDB(tsection, false);
    this->trackDB->loadTdb();
    this->roadDB = new TDB(tsection, true);
    this->roadDB->loadTdb();
    //Game::trackDB = this->trackDB;
    //Game::roadDB = this->roadDB;
    
    //loadAddons();

    //loadMkrList();
    //createMkrPlaces();
    //loadServices();
    //loadTraffic();
    //loadPaths();
    //loadActivities();

    //soundList = new SoundList();
    //soundList->loadSoundSources(Game::root + "/routes/" + Game::route + "/ssource.dat");
    //soundList->loadSoundRegions(Game::root + "/routes/" + Game::route + "/ttype.dat");
    //Game::soundList = soundList;
    
    terrainLib->loadQuadTree();
    //OrtsWeatherChange::LoadList();
    //ForestObj::LoadForestList();
    //ForestObj::ForestClearDistance = trk->forestClearDistance;
    //CarSpawnerObj::LoadCarSpawnerList();

    //if(Game::loadAllWFiles){
    //    preloadWFilesInit();
    //}
    
    //checkRouteDatabase();
    
    loaded = true;
    
    //Vec3::set(placementAutoTranslationOffset, 0, 0, 0);
    //Vec3::set(placementAutoRotationOffset, 0, 0, 0);
    
    //skydome = new Skydome();
    
}

void Route::setAsCurrentGameRoute(){
    Game::route = routeDir;
    Game::routeName = routeName;
    Game::trkName = trkName;
    Game::currentRoute = this;
}

void Route::mergeRoute(QString route2Name, float offsetX, float offsetY, float offsetZ){
    QProgressDialog *progress = NULL;
    bool gui = true;
    
    Route *route2 = new Route();
    route2->load(route2Name);
    float mOffset[3];
    mOffset[0] = offsetX;// (-4*2048) - 256;
    mOffset[1] = offsetY;//81.47992;
    mOffset[2] = offsetZ;//(-5*2048) + 640;


    unsigned int trackNodeOffset = 0; 
    unsigned int trackItemOffset = 0;
    unsigned int roadNodeOffset = 0; 
    unsigned int roadItemOffset = 0;
    QHash<unsigned int, unsigned int> fixedSectionIds;
    QHash<unsigned int, unsigned int> fixedShapeIds;
    unsigned int oldTrackNodeCount = this->trackDB->iTRnodes; 
    unsigned int oldRoadNodeCount = this->roadDB->iTRnodes;    
    
    
    if(Game::routeMergeTDB)
    {
        // Merge TDB
        qDebug() << "## Merge TDB";
        this->trackDB->mergeTDB(route2->trackDB, mOffset, trackNodeOffset, trackItemOffset, fixedSectionIds, fixedShapeIds);
        this->roadDB->mergeTDB(route2->roadDB, mOffset, roadNodeOffset, roadItemOffset, fixedSectionIds, fixedShapeIds);
    }
    else
        qDebug() << "## Merge TDB Skipped";
    
    // Merge world objects
    if(gui){
        progress = new QProgressDialog("Merging World Objects ...", "", 0, route2->tile.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    qDebug() << "## Merge World Objects";
    QHash<int, Tile*> modifiedWorldTiles;
    Tile *t2Tile;
    QVector<int*> trackObjUpdates;
    int pi = 0;

    foreach (Tile* tTile, route2->tile){
        if(progress != NULL){
            progress->setValue((++pi));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        if (tTile == NULL) 
            continue;
        if (tTile->loaded == 1) {
            tTile->updateTrackSectionInfo(fixedShapeIds, fixedSectionIds);
        }
        
        for(int i = 0; i < tTile->jestObiektow; i++){
            WorldObj *wObj = tTile->obiekty[i];
            if(wObj == NULL) continue;
            //if(wObj->isTrackItem()) continue;
            
            if(Game::routeMergeTDB)  wObj->addTrackItemIdOffset(trackItemOffset, roadItemOffset);                        
            
            int x, z, uid, oldx, oldz, olduid;
            oldx = wObj->x;
            oldz = wObj->y;
            olduid = wObj->UiD;
            wObj->position[0] += mOffset[0];
            wObj->position[1] += mOffset[1];
            wObj->position[2] -= mOffset[2];
            //qDebug() << "old tile" << wObj->x << wObj->y;
            while(wObj->position[0] > 1024 || wObj->position[0] < -1024 || wObj->position[2] > 1024 || wObj->position[2] < -1024 ){
                Game::check_coords(wObj->x, wObj->y, wObj->position);
            }
            //qDebug() << "new tile" << wObj->x << wObj->y;
            //qDebug() << "";
            x = wObj->x;
            z = wObj->y;
            
            t2Tile = tile[((x)*10000 + z)];
            if (t2Tile == NULL){
                tile[(x)*10000 + z] = new Tile(x, z);
                t2Tile = tile[((x)*10000 + z)];
                t2Tile->initNew();
            }
            if (modifiedWorldTiles[((x)*10000 + z)] == NULL)
                modifiedWorldTiles[((x)*10000 + z)] = t2Tile;
            //
            t2Tile->placeObject(wObj);
            if(Game::routeMergeTDB == true)
            {                
                if(wObj->typeID == wObj->trackobj || wObj->typeID == wObj->dyntrack){
                    int *u = new int[6];
                    u[0] = oldx;
                    u[1] = oldz;
                    u[2] = olduid;
                    u[3] = x;
                    u[4] = z;
                    u[5] = wObj->UiD;
                    trackObjUpdates.push_back(u);
                }
            }
        }
        
    }
    
    /// EFO this is now settings driven
    if(Game::routeMergeTDB == true)
    {        
        this->trackDB->updateUiDs(trackObjUpdates, oldTrackNodeCount);
        this->roadDB->updateUiDs(trackObjUpdates, oldRoadNodeCount);
    }            
    
    if(progress != NULL)
        delete progress;

    
    if(Game::debugOutput) qDebug() << "## Create MKR Places";
    createMkrPlaces();
    
    // Merge terrain
    /// EFO this is now settings driven    
    if(Game::routeMergeTerrain){        
        if(gui){
            progress = new QProgressDialog("Merging Terrain ...", "", 0, route2->tile.size() + modifiedWorldTiles.size());
            progress->setWindowModality(Qt::WindowModal);
            progress->setCancelButton(NULL);
            progress->setWindowFlags(Qt::CustomizeWindowHint);
            progress->show();
        }
        pi = 0;
        qDebug() << "Load all route2 terrain tiles";

        foreach (Tile* wTile, route2->tile){                  
            if(progress != NULL){
                progress->setValue((++pi));
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            }
           if (wTile == NULL) 
               continue;
           Terrain *t = route2->terrainLib->getTerrainByXY(wTile->x, wTile->z, true);
           if (t == NULL)
               qDebug() << "FAIL terrain NULL";
           else if (!t->loaded)
               qDebug() << "FAIL terrain not loaded";
           else
               if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << t->mojex << t->mojez;
        }
    }
        else
        qDebug() << "## Merge Terrain Skipped";

    
    if(Game::routeMergeTerrtex){
        qDebug() << "copying Terrtex: ";
        QString route2path = Game::root + "/routes/" + route2->routeName  + "/terrtex";
        QString route1path = Game::root + "/routes/" + this->routeName + "/terrtex/";
            FileFunctions::copyFiles(route2path, route1path  );
           // qDebug() << "copying Terrtex: " << route2path << " to " << route1path;
    }
        else
        qDebug() << "## Merge TerrTex Skipped";

    
    setAsCurrentGameRoute();

    /// EFO this is now settings driven    
    if(Game::routeMergeTerrain){        
        qDebug() << "Fill Terrain data";
        Terrain *tTile;
        foreach (Tile* wTile, modifiedWorldTiles){
            if(progress != NULL){
                progress->setValue((++pi));
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            }
            if (wTile == NULL) 
                continue;
            tTile = terrainLib->getTerrainByXY(wTile->x, wTile->z, false);
            if(tTile == NULL){
                terrainLib->saveEmpty(wTile->x, -wTile->z);
                if(!terrainLib->reload(wTile->x, wTile->z)){
                    qDebug() << "reload terrain fail";
                }
                tTile = terrainLib->getTerrainByXY(wTile->x, wTile->z, false);

            }
            if (!tTile->loaded)
               qDebug() << "FAIL main terrain not loaded";

            qDebug() << "fill";
            route2->terrainLib->fillTerrainData(tTile, mOffset);

        }
        if(progress != NULL)
           delete progress;

    } 
    
    // Other
    
}

void Route::selectObjectsByXYRange(int mojex, int mojez, int minx, int maxx, int minz, int maxz){
    Tile *tTile = tile[mojex*10000 + mojez];
    if (tTile == NULL)
        return;
    QVector<GameObj*> objs;
    tTile->selectObjectsByXYRange(objs, minx, maxx, minz, maxz);
    this->objectSelected(objs);
}

Route::Route(const Route& orig) {
}

Route::~Route() {
    clearMkrList();
}

void Route::loadAddons(){
    this->ref = new Ref((Game::root + "/routes/" + Game::route + "/" + Game::routeName + ".ref"));
    
    QString dirFile = Game::root + "/routes/" + Game::route + "/addons";
    QDir aDir(dirFile);
    if(!aDir.exists()){
        if(Game::debugOutput) if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << dirFile;
        if(Game::debugOutput) qDebug() << "# No Addons";
        return;
    }
        
    aDir.setFilter(QDir::Files);
    aDir.setNameFilters(QStringList()<<"*.ref");
    foreach(QString file, aDir.entryList()){
        //qDebug()<< dirFile + "/" + file;
        this->ref->loadFile(dirFile + "/" + file);
    }

}

void Route::checkRouteDatabase(){
    trackDB->checkDatabase();
    roadDB->checkDatabase();
    
}

bool Route::checkTrackSectionDatabase(){
    if(!this->tsection->dataOutOfSync)
        return true;
    
    qDebug() << "tsection out of sync !!!";
    if(Game::playerMode)
        return true;
    if(!Game::writeEnabled)
        return true;
    if(!Game::writeTDB)
        return true;
    
    // Edit mode. Make an action regarding not synced tsection data
    ActionChooseDialog dialog(4);
    dialog.setWindowTitle("TDB Error");
    dialog.setInfoText("Route Track Section database is out of sync with your Global database.\n"
                       "Choose action:");
    dialog.pushAction("FIX", "Convert route database to current Global now");
    dialog.pushAction("VIEW", "Disable writing to TDB - avoid editing tracks and interactives");
    dialog.pushAction("IGNORE", "Ignore and continue - saving route may destroy your route");
    dialog.pushAction("EXIT", "Quit TSRE now");
    dialog.exec();
    if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << dialog.actionChoosen;
    
    if(dialog.actionChoosen == "FIX"){
        Game::loadAllWFiles = true;
        preloadWFiles(true);
        // load tsection with autofix
        this->tsection = new TSectionDAT(true);
        // update ids inside W files
        foreach (Tile* tTile, tile){
            if (tTile == NULL) continue;
            if (tTile->loaded == 1) {
                tTile->updateTrackSectionInfo(tsection->autoFixedShapeIds, tsection->autoFixedSectionIds);
             }
        }
        ErrorMessage *e = new ErrorMessage(
            ErrorMessage::Type_Info, 
            ErrorMessage::Source_Editor, 
            QString("Route Track Section synced by TSRE. "),
                    "TSRE made automatic conversion of route database to current Global."
                    );
        ErrorMessagesLib::PushErrorMessage(e);
        
        
        return true;
    } 
    ErrorMessage *e = new ErrorMessage(
    ErrorMessage::Type_Error, 
    ErrorMessage::Source_TDB, 
    QString("Route Track Section database is out of sync with your Global database. "),
            "Route Track Section isn't compatibile with your current Global database. \n"
            "Check route installation for custom Global or convert Route using TSRE. \n"
            "Editing route may cause fatal errors. Make sure that writing to TDB is disabled."
        );
    ErrorMessagesLib::PushErrorMessage(e);
    if(dialog.actionChoosen == "VIEW"){
        Game::writeTDB = false;
        return true;
    }
    if(dialog.actionChoosen == "IGNORE"){
        // just do nothing
        return true;
    } 
    if(dialog.actionChoosen == "EXIT"){
        loaded = false;
        return false;
    }

    return true;
}

void Route::activitySelected(Activity* selected){
    currentActivity = selected;
}

Trk *Route::getTrk(){
    return this->trk;
}

int Route::getStartTileX(){
    return this->trk->startTileX;
}

int Route::getStartTileZ(){
    return this->trk->startTileZ;
}

float Route::getStartpX(){
    return this->trk->startpX;
}

float Route::getStartpZ(){
    return this->trk->startpZ;
}

void Route::createMkrPlaces(){
    QString key;
    key = "| Route: Stations";
    mkrList[key] = new CoordsRoutePlaces(trackDB, "stations");
    
    key = "| Route: Sidings";
    mkrList[key] = new CoordsRoutePlaces(trackDB, "sidings");
    
    key = "| Route: Speedposts";
    mkrList[key] = new CoordsRoutePlaces(trackDB, "speedposts");

}

void Route::clearMkrList(){
    qDeleteAll(mkrList);
    mkrList.clear();
    mkr = NULL;
    Game::markerFiles.clear();
}

void Route::loadMkrList(){
    /// Step one of Markers -- pulls the files in the files in the route directory
    //this->mkr = new CoordsMkr(Game::root + "/routes/" + Game::route + "/" + Game::routeName +".mkr");
    if(!mkrList.isEmpty() && Game::debugOutput)
        qDebug() << "Clearing Marker List" << mkrList.size();
    clearMkrList();

    QDir dir(Game::root + "/routes/" + Game::route);
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList()){
        if(dirFile.endsWith(".mkr", Qt::CaseInsensitive)){
            mkrList[(dirFile).toLower()] = new CoordsMkr(Game::root + "/routes/" + Game::route + "/" + dirFile);
            Game::markerFiles.append(dirFile);
        } else if(dirFile.endsWith(".kml", Qt::CaseInsensitive)){
            mkrList[(dirFile).toLower()] = new CoordsKml(Game::root + "/routes/" + Game::route + "/" + dirFile);
            Game::markerFiles.append(dirFile);
        } else if(dirFile.endsWith(".gpx", Qt::CaseInsensitive)){
            mkrList[(dirFile).toLower()] = new CoordsGpx(Game::root + "/routes/" + Game::route + "/" + dirFile);
            Game::markerFiles.append(dirFile);
        }
    }
    if(mkrList.size() > 0){
        if(mkrList[(Game::routeName+".mkr").toLower()] != NULL){
            if(mkrList[(Game::routeName+".mkr").toLower()]->loaded)
                mkr = mkrList[(Game::routeName+".mkr").toLower()];
            else
                mkr = mkrList.begin().value();
                //mkr = mkrList[(Game::routeName+".mkr").toLower().toStdString()];
        } else {
            mkr = mkrList.begin().value();
        }
    }
    /// step two of markers -- make pseudo markers for stations and sidings
    createMkrPlaces();
    
}

void Route::setMkrFile(QString name){
    if(mkrList[name] != NULL)
        this->mkr = mkrList[name];
}

void Route::loadActivities(){
    QDir dir(Game::root + "/routes/" + Game::route + "/activities");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.act");

    foreach(QString actfile, dir.entryList()){ 
        // Create a QFileInfo object for the specific file
        QFileInfo checkFile(dir.filePath(actfile));

        // Only add if the file size exceeds 100 bytes   /// EFO
        if(checkFile.size() > 100) {
            activityId.push_back(ActLib::AddAct(dir.path(), actfile)); 
            if(Game::debugOutput) qDebug() << "activity loaded";            
        }
        else
        {
            qDebug() << actfile << " is too small, not loaded" ;
        }
    } 
/*

    foreach(QString actfile, dir.entryList()){
        
        
        activityId.push_back(ActLib::AddAct(dir.path(), actfile));
    }
  */      


    return;
}

void Route::loadServices(){
    QDir dir(Game::root + "/routes/" + Game::route + "/services");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.srv");

    foreach(QString actfile, dir.entryList()){
        QFileInfo checkFile(dir.filePath(actfile));

        if(checkFile.size() > 100) {
            int id = ActLib::AddService(dir.path(), actfile);
            if(id >= 0 && !serviceId.contains(id))
                serviceId.push_back(id);
            if(Game::debugOutput) qDebug() << "service loaded" ;
        } else {
            qDebug() << actfile << " is too small, not loaded" ;
        }
    }    
    
    /*
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddService(dir.path(), actfile);
        //service.push_back(ActLib::Services[id]);
    }
    if(Game::debugOutput) qDebug() << "service loaded";
     */
    return;
    
}

void Route::loadTraffic(){
    QDir dir(Game::root + "/routes/" + Game::route + "/traffic");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.trf");

    foreach(QString actfile, dir.entryList()){ 
        // Create a QFileInfo object for the specific file
        QFileInfo checkFile(dir.filePath(actfile));

        // Only add if the file size exceeds 100 bytes   /// EFO
        if(checkFile.size() > 100) {
            int id = ActLib::AddTraffic(dir.path(), actfile);
            if(id >= 0 && !trafficId.contains(id))
                trafficId.push_back(id);
            if(Game::debugOutput) qDebug() << "traffic loaded";
        }
        else
        {
            qDebug() << actfile << " is too small, not loaded" ;
        }
    } 

    
/*
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddTraffic(dir.path(), actfile);
        //traffic.push_back(ActLib::Traffics[id]);
    }
*/

    return;
 
}

void Route::loadPaths(){
    QDir dir(Game::root + "/routes/" + Game::route + "/paths");
    if(!dir.exists()) 
        return;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.pat");
    foreach(QString actfile, dir.entryList()){
        int id = ActLib::AddPath(dir.path(), actfile);
        if(!path.contains(ActLib::Paths[id]))
            path.push_back(ActLib::Paths[id]);
    }

    if(Game::debugOutput) qDebug() << "paths loaded";
    return;
}

WorldObj* Route::getObj(int x, int z, int id) {
    Tile *tTile;

    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return NULL;
    return tTile->getObj(id);

}

WorldObj* Route::findWorldObjByUid(int x, int z, unsigned int uid) {
    Tile *tTile = tile.value(x * 10000 + z, NULL);
    if(tTile == NULL || tTile->loaded != 1)
        return NULL;
    for(auto it = tTile->obiekty.begin(); it != tTile->obiekty.end(); ++it){
        WorldObj *obj = it->second;
        if(obj != NULL && obj->loaded && obj->UiD == uid)
            return obj;
    }
    return NULL;
}

WorldObj* Route::findNearestObj(int x, int z, float* pos){
    Game::check_coords(x, z, pos);
    Tile *tTile;
    //try {
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return NULL;
    return tTile->findNearestObj(pos);
}

void Route::transalteObj(int x, int z, float px, float py, float pz, int uid) {
    Tile *tTile;

    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return;
    tTile->transalteObj(px, py, pz, uid);

}

void Route::updateSim(float *playerT, float deltaTime){
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            if (tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                tTile->updateSim(deltaTime);
            }
        }
    }
    
    if(currentActivity != NULL){
        currentActivity->updateSim(playerT, deltaTime);
    }
}

WorldObj* Route::updateWorldObjData(FileBuffer *data){
    QString sh;
    int x = 0;
    int z = 0;
    WorldObj *nowy = NULL;
    bool objloaded = true;
    
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        //qDebug() << sh;
        if (sh == ("x")) {
            x = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("z")) {
            z = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("remove")) {
            objloaded = false;
            ParserX::SkipToken(data);
            continue;
        }
        if ((nowy = WorldObj::createObj(sh)) != NULL) {
            //qDebug() << nowy->type;
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                nowy->set(sh, data);
                ParserX::SkipToken(data);
            }
            if(tile[x*10000+z] != NULL){
                tile[x*10000+z]->replaceWorldObj(nowy);
            }
            nowy->loaded = objloaded;
            //qDebug() << nowy->loaded;
            //obiekty[jestObiektow++] = nowy;
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
        continue;
    }
    return nowy;
}

void Route::loadTSectionData(FileBuffer *data){
    tsection->loadRouteUtf16Data(data, false);
    tsection->routeMaxIdx += 2 - tsection->routeMaxIdx % 2;
    tsection->routeShapes++;
    if(Game::serverClient != NULL){
        loadingProgress++;
        load();
    }
}

void Route::loadQuadTreeDetailed(FileBuffer *data){
    terrainLib->loadQuadTreeDetailed(data);
    if(Game::serverClient != NULL){
        loadingProgress++;
        load();
    }
}

void Route::loadQuadTreeDistant(FileBuffer *data){
    terrainLib->loadQuadTreeDistant(data);
    if(Game::serverClient != NULL){
        loadingProgress++;
        load();
    }
}

void Route::loadTrkData(FileBuffer *data){
    trk = new Trk();
    trk->loadUtf16Data(data);
    if(Game::serverClient != NULL){
        loadingProgress++;
        load();
    }
}
    
void Route::loadTdbData(FileBuffer *data, QString type){
    bool road = false;
    if(type == "rdb")
        road = true;
    
    QString sh;
    int x = 0;
    int z = 0;
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
       if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << sh;
        if (sh == "trackdb") {
            TDB *t = NULL;
            if(Game::serverClient == NULL){
                t = new TDB(tsection, road);
            } else {
                t = new TDBClient(tsection, road);
            }
            t->loadUtf16Data(data);
            if(!t->isRoad()){
                t->speedPostDAT = new SpeedPostDAT();
                t->sigCfg = new SigCfg();
                this->trackDB = t;
                Game::trackDB = t;
            } else {
                this->roadDB = t;
                Game::roadDB = t;
            }
            t->loaded = true;
            
            if(Game::serverClient != NULL){
                loadingProgress++;
                load();
            }
            ParserX::SkipToken(data);
            continue;
            
        }
        ParserX::SkipToken(data);
    }
}

void Route::updateTileData(FileBuffer *data){
    QString sh;
    int x = 0;
    int z = 0;
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
      if(Game::debugOutput)  if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << sh;
        if (sh == ("x")) {
            x = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("z")) {
            z = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("tr_worldfile")) {
            tile[x*10000+z] = new Tile(x, z, data);
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
        continue;
    }
}

void Route::preloadWFiles(bool gui){
    Tile *tTile;
    
    QString path = Game::root + "/routes/" + Game::route + "/world";
    QDir dir(path);
    //qDebug() << path;
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.w");
    
    if (!dir.exists()) {
        qDebug() << "route W dir not exist - aborting";
        return;
    }
        
    int WX = 0, WZ = 0;
    unsigned long long timeNow = QDateTime::currentMSecsSinceEpoch();
    QProgressDialog *progress = NULL;
    if(gui){
        progress = new QProgressDialog("Loading All World Files ...", "", 0, dir.entryList().size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    
    int i = 0;
    foreach(QString wfile, dir.entryList()){
        if(wfile.length() != 17){
           if(Game::debugOutput) qDebug() << "# W File undefined name " << wfile;
        }
        QString wxString = wfile.mid(1, 7);
        QString wzString = wfile.mid(8, 7);
        WX = wxString.toInt();
        WZ = -wzString.toInt();
        tTile = tile[(WX)*10000 + WZ];

        if (tTile == NULL){
           if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << wxString << wzString << "-" << WX << WZ;
            tile[(WX)*10000 + WZ] = new Tile(WX, WZ);
        }
        if(progress != NULL){
            progress->setValue((++i));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }

   if(Game::debugOutput) qDebug() << "#W Files preloaded: " << (QDateTime::currentMSecsSinceEpoch() - timeNow)/1000<< "s";
    delete progress;
    
}

// Use this function to init W files if loaded before route data.
void Route::preloadWFilesInit(){
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->loadInit();
        }
    }
    /*
    // Do some stats
    QHash<QString, int> names;
    
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded != 1) continue;
        for (auto it = tTile->obiekty.begin(); it != tTile->obiekty.end(); ++it) {
            WorldObj* obj = (WorldObj*) it->second;
            if(obj == NULL) 
                continue;
            if(obj->typeID == obj->trackobj)
                names[obj->fileName] += 1;
        }
    }
    
    QHashIterator<QString, int> i(names);
    qDebug() << "Track Shapes";
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << " : " << i.value();
    }*/
}

void Route::pushRenderItems(float * playerT, float* playerW, float* target, float playerRot, float fov, int renderMode) {
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    if(renderMode == Game::currentRenderer->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }
    
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            
            if(Game::autoNewTiles)
                if (i == 0 && j == 0)
                    if (tTile->loaded == -2) {
                        Route::newTile((int)playerT[0] + i, (int)playerT[1] + j);
                        tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
                    }
            if (tTile->loaded == 1) {
                Game::currentRenderer->mvPushMatrix();
                Mat4::translate(Game::currentRenderer->mvMatrix, Game::currentRenderer->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->pushRenderItems(playerT, playerW, target, fov, renderMode);
                Game::currentRenderer->mvPopMatrix();
            }
        }
    }
    
    /*if (renderMode == gluu->RENDER_DEFAULT) {
        if(Game::viewTrackDbLines)
            trackDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines)
            trackDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewTrackDbLines)
            roadDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines)
            roadDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewMarkers)
            if(this->mkr != NULL)
                this->mkr->render(gluu, playerT, playerW, playerRot);
    }
    if(Game::renderTrItems){
        trackDB->renderItems(gluu, playerT, playerRot, renderMode);
        roadDB->renderItems(gluu, playerT, playerRot, renderMode);
    }
    
    if(currentActivity != NULL){
        currentActivity->render(gluu, playerT, playerRot, renderMode);
    }
    
    for(int i = 0; i < path.size(); i++){
        if(path[i]->isSelected())
            path[i]->render(gluu, playerT, renderMode);
    }*/

    Game::ignoreLoadLimits = false;
}

void Route::render(GLUU *gluu, float * playerT, float* playerW, float* target, float playerRot, float fov, int renderMode) {
    if(!loaded) return;
    
    int mintile = -Game::tileLod;
    int maxtile = Game::tileLod;

    if(renderMode == gluu->RENDER_SELECTION){
        mintile = -1;
        maxtile = 1;
    }

    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            
            if(Game::autoNewTiles)
                if (i == 0 && j == 0)
                    if (tTile->loaded == -2) {
                        Route::newTile((int)playerT[0] + i, (int)playerT[1] + j);
                        tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
                    }
            if (tTile->loaded == 1) {
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->render(playerT, playerW, target, fov, renderMode);
                gluu->mvPopMatrix();
            }
        }
    }

    // A multi-tile water ruler belongs to the world tile containing its first
    // point. Normal object/tile LOD would therefore hide the entire guide when
    // the camera follows a long river away from that first tile. Render this
    // one editor guide explicitly whenever normal tile rendering culled it.
    if(waterRulerObj != NULL && waterRulerObj->loaded
            && renderMode != gluu->RENDER_SELECTION){
        int offsetX = waterRulerObj->x - (int)playerT[0];
        int offsetZ = waterRulerObj->y - (int)playerT[1];
        float lodx = offsetX * 2048.0f
                + waterRulerObj->position[0] - playerW[0];
        float lodz = offsetZ * 2048.0f
                + waterRulerObj->position[2] - playerW[2];
        float lod = std::sqrt(lodx * lodx + lodz * lodz);
        bool homeTileRendered = offsetX >= mintile && offsetX <= maxtile
                && offsetZ >= mintile && offsetZ <= maxtile;
        bool normalObjectRendered = homeTileRendered
                && (lod < Game::objectLod || waterRulerObj->isInternalLodControl());
        if(!normalObjectRendered){
            gluu->mvPushMatrix();
            Mat4::translate(gluu->mvMatrix, gluu->mvMatrix,
                            2048.0f * offsetX, 0, 2048.0f * offsetZ);
            waterRulerObj->render(gluu, lod, lodx, lodz,
                                  playerW, target, fov, 0, renderMode);
            gluu->mvPopMatrix();
        }
    }

    // Keep the route-wide vegetation guide visible when its owning world tile
    // is outside the normal render window. Selection remains on the ordinary
    // world-tile pass so its control-point color encoding stays unchanged.
    if(vegetationRulerObj != NULL && vegetationRulerObj->loaded
            && renderMode != gluu->RENDER_SELECTION){
        int offsetX = vegetationRulerObj->x - (int)playerT[0];
        int offsetZ = vegetationRulerObj->y - (int)playerT[1];
        float lodx = offsetX * 2048.0f
                + vegetationRulerObj->position[0] - playerW[0];
        float lodz = offsetZ * 2048.0f
                + vegetationRulerObj->position[2] - playerW[2];
        float lod = std::sqrt(lodx * lodx + lodz * lodz);
        bool homeTileRendered = offsetX >= mintile && offsetX <= maxtile
                && offsetZ >= mintile && offsetZ <= maxtile;
        bool normalObjectRendered = homeTileRendered
                && (lod < Game::objectLod || vegetationRulerObj->isInternalLodControl());
        if(!normalObjectRendered){
            gluu->mvPushMatrix();
            Mat4::translate(gluu->mvMatrix, gluu->mvMatrix,
                            2048.0f * offsetX, 0, 2048.0f * offsetZ);
            vegetationRulerObj->render(gluu, lod, lodx, lodz,
                                       playerW, target, fov, 0, renderMode);
            gluu->mvPopMatrix();
        }
    }

    if (renderMode == gluu->RENDER_DEFAULT) {
        if(Game::viewTrackDbLines && trackDB != NULL)
            trackDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines && trackDB != NULL)
            trackDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewTrackDbLines && roadDB != NULL)
            roadDB->renderAll(gluu, playerT, playerRot);
        if(Game::viewTsectionLines && roadDB != NULL)
            roadDB->renderLines(gluu, playerT, playerRot);
        if(Game::viewMarkers)
            if(this->mkr != NULL)
                this->mkr->render(gluu, playerT, playerW, playerRot);
    }
    if(Game::renderTrItems){
        trackDB->renderItems(gluu, playerT, playerRot, renderMode);
        roadDB->renderItems(gluu, playerT, playerRot, renderMode);
    }
    
    if(currentActivity != NULL){
        currentActivity->render(gluu, playerT, playerRot, renderMode);
    }
    
    for(int i = 0; i < path.size(); i++){
        if(path[i]->isSelected())
            path[i]->render(gluu, playerT, renderMode);
    }
    //trackDB->renderItems(gluu, playerT, playerRot);
    /*
    for (var key in this.tile){
       if(this.tile[key] === undefined) continue;
       if(this.tile[key] === null) continue;
       //console.log(this.tile[key].inUse);
       if(!this.tile[key].inUse){
           //console.log("a"+this.tile[key]);
           this.tile[key] = undefined;
       } else {
           this.tile[key].inUse = false;
       }
    }*/
    Game::ignoreLoadLimits = false;
}

void Route::renderShadowMap(GLUU *gluu, float * playerT, float* playerW, float* target, float playerRot, float fov, bool selection) {
    if(!loaded) return;
    
    int mintile = -1;
    int maxtile = 1;
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = requestTile((int)playerT[0] + i, (int)playerT[1] + j, false);
            if(tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                gluu->mvPushMatrix();
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * i, 0, 2048 * j);
                tTile->render(playerT, playerW, target, fov, GLUU::RENDER_SHADOWMAP);
                gluu->mvPopMatrix();
            }
        }
    }
    if(currentActivity != NULL)
        currentActivity->render(gluu, playerT, playerRot, 0);
}

void Route::setTerrainTextureToObj(int x, int y, float *pos, Brush* brush, WorldObj* obj){
    if(obj == NULL)
        obj = Route::findNearestObj(x, y, pos);
    if(obj == NULL)
        return;
    if(obj->typeID == WorldObj::trackobj){
        setTerrainTextureToTrack(x, y, pos, brush, 0);
        return;
    }
    if(!obj->hasLinePoints())
        return;

    float* punkty = new float[10000];
    float* ptr = punkty;
    obj->getLinePoints(ptr);
    int length = ptr - punkty;
   if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "lo "<<length;
    Game::terrainLib->setTextureToTrackObj(brush, punkty, length, obj->x, obj->y);
    delete[] punkty;
}

void Route::setTerrainTextureToTrack(int x, int y, float* pos, Brush* brush, int mode){
    Game::check_coords(x, y, pos);
    float playerT[2];
    playerT[0] = x;
    playerT[1] = y;
    float tp[3];
    Vec3::copy(tp,pos);
    int ok1, ok2;
    QVector<float> punkty;
    punkty.reserve(10000);
    if(placementAutoTargetType == 0) {
        this->trackDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    } else if(placementAutoTargetType == 1) {
        this->roadDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    } else if(placementAutoTargetType == 2) {
        bool road = false;
        ok1 = this->trackDB->findNearestPositionOnTDB(playerT, tp, NULL, NULL);
        ok2 = this->roadDB->findNearestPositionOnTDB(playerT, tp, NULL, NULL);
        if(ok2 >= 0)
            if(ok1 < 0 || ok2 < ok1){
                road = true;
        }
        if(road)
            this->roadDB->getVectorSectionPoints(x, y, pos, punkty, mode);
        else
            this->trackDB->getVectorSectionPoints(x, y, pos, punkty, mode);
    }
    int length = punkty.length();
   if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
    Game::terrainLib->setTextureToTrackObj(brush, punkty.data(), length, x, y);
}

static int setTerrainTextureToTileFromDb(TDB *db, int x, int y, Brush* brush){
    if (db == NULL)
        return 0;

    QVector<float> punkty;
    punkty.reserve(10000);
    int sections = db->getVectorSectionPointsForTile(x, y, punkty);
    int length = punkty.length();
    if(length > 0)
        Game::terrainLib->setTextureToTrackObj(brush, punkty.data(), length, x, y);
    return sections;
}

int Route::setTerrainTextureToTileTrack(int x, int y, Brush* brush){
    return setTerrainTextureToTileFromDb(this->trackDB, x, y, brush);
}

int Route::setTerrainTextureToTileRoad(int x, int y, Brush* brush){
    return setTerrainTextureToTileFromDb(this->roadDB, x, y, brush);
}

bool Route::findNearestDbHeight(int x, int y, float *pos, float maxDistance, float &height){
    if (pos == NULL || maxDistance <= 0)
        return false;

    Game::check_coords(x, y, pos);

    float bestDistance = maxDistance;
    float bestHeight = 0;
    bool found = false;
    float playerT[2] = {(float)x, (float)y};

    TDB *dbs[2] = {this->trackDB, this->roadDB};
    for (int i = 0; i < 2; i++) {
        if (dbs[i] == NULL)
            continue;

        float sample[3];
        Vec3::copy(sample, pos);
        float tpos[3];
        int ok = dbs[i]->findNearestPositionOnTDB(playerT, sample, NULL, tpos);
        if (ok < 0)
            continue;

        float dx = sample[0] - pos[0];
        float dz = sample[2] - pos[2];
        float distance = sqrt(dx * dx + dz * dz);
        if (distance <= bestDistance) {
            bestDistance = distance;
            bestHeight = sample[1];
            found = true;
        }
    }

    if (!found)
        return false;

    height = bestHeight;
    return true;
}

bool Route::resetTerrainTextureOnTile(int x, int y, int &filesDeleted, int &filesFailed){
    filesDeleted = 0;
    filesFailed = 0;

    QString routePath = Game::root + "/routes/" + Game::route;
    QString tileName = Terrain::getTileName(x, -y);
    QString tilePath = routePath + "/tiles/" + tileName + ".t";

    TFile tfile;
    if (!tfile.readT(tilePath))
        return false;

    int defaultMat = tfile.getMatByTexture("terrain.ace");
    if (defaultMat < 0)
        defaultMat = tfile.newMat();

    int patches = tfile.patchsetNpatches;
    for (int patch = 0; patch < patches * patches; patch++) {
        tfile.tdata[patch * 13 + 6] = defaultMat;
        tfile.tdata[patch * 13 + 7] = 0.001f;
        tfile.tdata[patch * 13 + 8] = 0.001f;
        tfile.tdata[patch * 13 + 9] = 0.062375f;
        tfile.tdata[patch * 13 + 10] = 0.0f;
        tfile.tdata[patch * 13 + 11] = 0.0f;
        tfile.tdata[patch * 13 + 12] = 0.062375f;
    }

    tfile.save(tilePath);

    QDir terrtexDir(routePath + "/terrtex");
    if (terrtexDir.exists()) {
        QString tilePrefix = tileName.toLower() + "_";
        QFileInfoList texFiles = terrtexDir.entryInfoList(QStringList() << "*.ace" << "*.ACE" << "*.dds" << "*.DDS", QDir::Files, QDir::Name);
        for (int i = 0; i < texFiles.size(); i++) {
            QFileInfo texInfo = texFiles[i];
            if (!texInfo.completeBaseName().toLower().startsWith(tilePrefix))
                continue;

            if (QFile::remove(texInfo.absoluteFilePath()))
                filesDeleted++;
            else
                filesFailed++;
        }
    }

    if (Game::terrainLib != NULL) {
        Game::terrainLib->setDetailedTerrainAsCurrent();
        Game::terrainLib->reload(x, y);
    }
    reloadTile(x, y);
    if (tile[x * 10000 + y] != NULL)
        tile[x * 10000 + y]->setModified(false);

    return true;
}

bool Route::confirmTerrainConformBias(bool roadDatabase){
    const float bias = roadDatabase
            ? Game::terrainConformRdbBias
            : Game::terrainConformTdbBias;
    if(qAbs(bias) < 0.0001f)
        return true;

    bool &confirmed = roadDatabase
            ? rdbTerrainBiasConfirmedThisSession
            : tdbTerrainBiasConfirmedThisSession;
    if(confirmed)
        return true;

    const QString databaseName = roadDatabase ? "RDB" : "TDB";
    const QString message =
        QString("%1 terrain conform height bias is set to %2 m.\n\n"
                "This non-standard setting will offset terrain height whenever "
                "terrain is conformed to the %1 during this session.\n\n"
                "Continue with this terrain conform operation?")
            .arg(databaseName)
            .arg(bias, 0, 'f', 2);
    if(!GuiFunct::confirmDestructiveAction(
            NULL,
            QString("Non-standard %1 Terrain Bias").arg(databaseName),
            message))
        return false;

    confirmed = true;
    return true;
}

bool Route::setTerrainToTrackObj(WorldObj* obj, Brush* brush){
    if(obj == NULL) return true;
    if(obj->typeObj != WorldObj::worldobj)
        return true;
    
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            if(!setTerrainToTrackObj(gobj->objects[i], brush))
                return false;
        }
        return true;
    }
    
    if(obj->type == "trackobj" || obj->type == "dyntrack" ){
        //TrackObj* tobj = (TrackObj*)obj;
        //TrackObj* track = (TrackObj*)obj;
        QVector<float> punkty;
        punkty.reserve(10000);
        const bool roadShape = this->tsection->isRoadShape(obj->sectionIdx);
        if(!confirmTerrainConformBias(roadShape))
            return false;
        if(roadShape)
            this->roadDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        else
            this->trackDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        int length = punkty.length();
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        if(length == 0){
            if(obj->sectionIdx >= 0){
                float matrix[16];
                for(int i = 0; i < tsection->shape[obj->sectionIdx]->path[0].n; i++){
                    memcpy(matrix, obj->matrix, sizeof(float)*16);
                    int sidx = tsection->shape[obj->sectionIdx]->path[0].sect[i];
                    tsection->sekcja[sidx]->getPoints(punkty, matrix);
                }
            } else {
                //obj->getLinePoints(ptr);
            }
            length = punkty.length();
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        }
        if(length > 0)
            Game::terrainLib->setTerrainToTrackObj(
                brush, punkty.data(), length, obj->x, obj->y, obj->matrix,
                roadShape ? Game::terrainConformRdbBias : Game::terrainConformTdbBias);
    } else if(obj->hasLinePoints()) {
        float* punkty = new float[10000];
        float* ptr = punkty;
        obj->getLinePoints(ptr);
        int length = ptr - punkty;
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        Game::terrainLib->setTerrainToTrackObj(brush, punkty, length, obj->x, obj->y, obj->matrix);
        delete[] punkty;
    }
    return true;
}

void Route::smoothTerrainToTrackObj(WorldObj* obj, Brush* brush){
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;

    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            smoothTerrainToTrackObj(gobj->objects[i], brush);
        }
        return;
    }

    if(obj->type == "trackobj" || obj->type == "dyntrack" ){
        QVector<float> punkty;
        punkty.reserve(10000);
        if(this->tsection->isRoadShape(obj->sectionIdx))
            this->roadDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        else
            this->trackDB->getVectorSectionPoints(obj->x, obj->y, obj->UiD, punkty);
        int length = punkty.length();
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        if(length == 0){
            if(obj->sectionIdx >= 0){
                float matrix[16];
                for(int i = 0; i < tsection->shape[obj->sectionIdx]->path[0].n; i++){
                    memcpy(matrix, obj->matrix, sizeof(float)*16);
                    int sidx = tsection->shape[obj->sectionIdx]->path[0].sect[i];
                    tsection->sekcja[sidx]->getPoints(punkty, matrix);
                }
            }
            length = punkty.length();
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        }
        if(length > 0)
            Game::terrainLib->smoothTerrainToTrackObj(brush, punkty.data(), length, obj->x, obj->y, obj->matrix);
    } else if(obj->hasLinePoints()) {
        float* punkty = new float[10000];
        float* ptr = punkty;
        obj->getLinePoints(ptr);
        int length = ptr - punkty;
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "l "<<length;
        Game::terrainLib->smoothTerrainToTrackObj(brush, punkty, length, obj->x, obj->y, obj->matrix);
        delete[] punkty;
    }
}

int Route::setTerrainToNearestDbTile(int x, int y, float *pos, Brush* brush){
    if(pos == NULL || brush == NULL)
        return 0;

    TDB *bestDb = NULL;
    float bestTpos[3];
    float bestDist = 99999;

    TDB *dbs[2] = { this->trackDB, this->roadDB };
    for(int i = 0; i < 2; i++){
        if(dbs[i] == NULL)
            continue;

        float playerT[2] = { (float)x, (float)y };
        float sample[3];
        Vec3::copy(sample, pos);
        float tpos[3];
        int dist = dbs[i]->findNearestPositionOnTDB(playerT, sample, NULL, tpos);
        if(dist >= 0 && dist < bestDist){
            bestDist = dist;
            bestDb = dbs[i];
            bestTpos[0] = tpos[0];
            bestTpos[1] = tpos[1];
            bestTpos[2] = tpos[2];
        }
    }

    if(bestDb == NULL || bestDist > Game::snapableRadius)
        return 0;
    if(!confirmTerrainConformBias(bestDb == this->roadDB))
        return 0;

    QVector<float> points;
    points.reserve(8192);
    int sections = bestDb->getVectorSectionPointsForTile(x, y, (int)bestTpos[0], points);
    if(sections <= 0 || points.length() < 6)
        return 0;

    Game::terrainLib->setTerrainToTrackObj(
        brush, points.data(), points.length(), x, y, NULL,
        bestDb == this->roadDB
            ? Game::terrainConformRdbBias
            : Game::terrainConformTdbBias);
    return sections;
}

int Route::setTerrainToTrackObjTile(WorldObj* obj, Brush* brush, int tileX, int tileZ){
    if(obj == NULL || brush == NULL)
        return 0;
    if(obj->typeObj != WorldObj::worldobj)
        return 0;
    if(obj->type != "trackobj" && obj->type != "dyntrack")
        return 0;

    TDB *db = this->trackDB;
    if(this->tsection->isRoadShape(obj->sectionIdx))
        db = this->roadDB;
    if(db == NULL)
        return 0;
    if(!confirmTerrainConformBias(db == this->roadDB))
        return 0;

    float pos[3] = { obj->position[0], obj->position[1], obj->position[2] };
    float playerT[2] = { (float)obj->x, (float)obj->y };
    float tpos[3];
    int dist = db->findNearestPositionOnTDB(playerT, pos, NULL, tpos);
    if(dist < 0 || dist > Game::snapableRadius)
        return 0;

    QVector<float> points;
    points.reserve(8192);
    int sections = db->getVectorSectionPointsForTile(tileX, tileZ, (int)tpos[0], points);
    if(sections <= 0 || points.length() < 6)
        return 0;

    Game::terrainLib->setTerrainToTrackObj(
        brush, points.data(), points.length(), tileX, tileZ, NULL,
        db == this->roadDB
            ? Game::terrainConformRdbBias
            : Game::terrainConformTdbBias);
    return sections;
}

ActivityObject* Route::getActivityObject(int id){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity->getObjectById(id);
    return NULL;
}

Consist* Route::getActivityConsist(int id){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity->getServiceConsistById(id);
}

Activity* Route::getCurrentActivity(){
    if(currentActivity == NULL)
        return NULL;
    return currentActivity;
}

float Route::getDistantTerrainYOffset(){
    return trk->distantTerrainYOffset;
}

WorldObj* Route::placeObject(int x, int z, float* p) {
    float* q = new float[4];
    Quat::fill((float*)q); 
    return placeObject(x, z, p, (float*) q, 0, ref->selected); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
}

WorldObj* Route::placeObject(int x, int z, float* p, float* q, float elev) {
    return placeObject(x, z, p, q, elev, ref->selected); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
}

WorldObj* Route::placeObject(int x, int z, float* p, float* q, float elev, Ref::RefItem* r) {
    if(r == NULL) 
        return NULL;
    Game::check_coords(x, z, p);
    // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
    // pozycja wzgledem TDB:
    int itemTrackType = WorldObj::isTrackObj(r->type);
    
    if(itemTrackType != 0){
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
    }
    
    float* tpos = NULL;
    if(placementStickToTarget){
            tpos = new float[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, p);
            Quat::copy(tq, q);
            Vec3::copy(tp2, p);
            Quat::copy(tq2, q);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, tpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, tpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, tpos);  if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, tpos);
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                Quat::copy(q, tq);
                if(!snapableOnlyRotation){
                    Vec3::copy(p, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    if(itemTrackType == 1){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->trackDB->findNearestPositionOnTDB(playerT, p, q, tpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
        if(ok < 0) return NULL;      
        x = playerT[0];
        z = playerT[1];
    }
    if(itemTrackType == 2){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->roadDB->findNearestPositionOnTDB(playerT, p, q, tpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
        if(ok < 0) return NULL;
        x = playerT[0];
        z = playerT[1];
    } 
    if(itemTrackType == 3){
        tpos = new float[3];
        float* playerT = Vec2::fromValues(x, z);
        int ok = this->roadDB->findNearestPositionOnTDB(playerT, p, q, tpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
        if(ok < 0) return NULL;
        float* buffer;
        int len;
        this->roadDB->getVectorSectionLine(buffer, len, playerT[0], playerT[1], tpos[0]);
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "len "<<len;
        bool ok1 = this->trackDB->getSegmentIntersectionPositionOnTDB(playerT, buffer, len, p, q, tpos);
        if(!ok1) return NULL;
        x = playerT[0];
        z = playerT[1];
        //return NULL;
    }

    Tile *tTile = requestTile(x, z);
    if(tTile == NULL) return NULL;
    if(tTile->loaded != 1) return NULL;
    
    int snapableSide = -1;
    if(placementStickToTarget && placementAutoTargetType == 3){
        snapableSide = tTile->getNearestSnapablePosition(p, q);  
    }
        
    float endp[5];
    memset(endp, 0, sizeof(endp));
    endp[3] = 1;
    float firstPos[3];
    int placementSnapNodeId = -1;
    bool placementSnapRoad = false;
    if ((r->type == "trackobj" || r->type == "dyntrack" )) {
        if(r->type == "dyntrack"){   if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
            this->roadDB->setDefaultEnd(0);
            this->trackDB->setDefaultEnd(0);
        }
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"1: "<< x <<" "<<z<<" P "<<p[0]<<" "<<p[1]<<" "<<p[2]<<" Q" <<q[1]<<" "<<q[3] ;
        int oldx = x;
        int oldz = z;
        Vec3::copy(firstPos, p);
        placementSnapRoad = this->tsection->isRoadShape(r->value);
        if(placementSnapRoad)
            this->roadDB->findPosition(x, z, p, q, endp, r->value, &placementSnapNodeId);
        else
            this->trackDB->findPosition(x, z, p, q, endp, r->value, &placementSnapNodeId); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
        Game::check_coords(x, z, p);
        firstPos[0] -= (x-oldx)*2048;
        firstPos[2] -= (z-oldz)*2048;
        if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"2: "<< x <<" "<<z<<" P "<<p[0]<<" "<<p[1]<<" "<<p[2]<<" Q" <<q[1]<<" "<<q[3] ; 
        tTile = requestTile(x, z);
        if(tTile == NULL) return NULL;
        if(tTile->loaded != 1) return NULL;
    }
          
    WorldObj* nowy = tTile->placeObject(p, q, r, tpos);   if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << nowy->typeObj << "/" << nowy->type;  
    if ((r->type == "trackobj" || r->type == "dyntrack" )&& nowy != NULL) {
        if(nowy->endp == 0) nowy->endp = new float[5];
        memcpy(nowy->endp, endp, sizeof(float)*5);
        Vec3::copy(nowy->firstPosition,firstPos);
        nowy->placementSnapNodeId = placementSnapNodeId;
        nowy->placementSnapRoad = placementSnapRoad;
    }   
    nowy->snapped(snapableSide);
    if(nowy->typeID == nowy->sstatic){        
        moveWorldObjToTile(nowy->x, nowy->y, nowy);
    }
    if(elev !=0)
        nowy->rotate(elev, 0, 0);
    
    if((r->type == "signal") || (r->type == "speedpost")) {        
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"1: "<< x <<" "<<z<<" P "<<nowy->position[0]<<" "<<nowy->position[1]<<" "<<nowy->position[2]<<" Q" <<nowy->qDirection[1]<<" "<<nowy->qDirection[3] ;            
            float pos[3]; pos[0] = 0;  pos[1] = 0;  pos[2] = 0;
            if(r->currentFilename.toLower().contains("gantry") == false)  pos[0] = Game::sigOffset;
            Vec3::transformQuat((float*)pos, (float*)pos, (float*)nowy->qDirection);
            nowy->translate(pos[0], pos[1], pos[2]);  nowy->modified = true;  nowy->setMartix();
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"1: "<< x <<" "<<z<<" P "<<nowy->position[0]<<" "<<nowy->position[1]<<" "<<nowy->position[2]<<" Q" <<nowy->qDirection[1]<<" "<<nowy->qDirection[3] ;            
    }
    
    Undo::PushWorldObjPlaced(nowy);  if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
    return nowy;
}

float* Route::getPointerPosition(float* out, int &x, int &z, float* pos){
    if(out == NULL)
        return NULL;
    Vec3::copy(out, pos);
    if(placementStickToTarget){
            float ttpos[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, pos);
            Vec3::copy(tp2, pos);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);   if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, ttpos); // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                if(!snapableOnlyRotation){
                    Vec3::copy(out, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    return out;
}

void Route::dragWorldObject(WorldObj* obj, int x, int z, float* pos){
    if(obj->typeObj != WorldObj::worldobj)
        return;
    
    float tpos[3];
    Vec3::copy(tpos, pos);
    Game::check_coords(x, z, tpos);
    Tile *tTile = requestTile(x, z);
    if(tTile == NULL) return;
    if(tTile->loaded != 1) return;
    int snapableSide = -1;
    float q[4];
    Quat::copy(q, obj->qDirection);
    
    if(placementStickToTarget){
            float ttpos[3];
            float* playerT = Vec2::fromValues(x, z);
            float* playerT2 = Vec2::fromValues(x, z);
            float tp[3], tp2[3];
            float tq[4], tq2[3];
            Vec3::copy(tp, pos);
            Quat::copy(tq, q);
            Vec3::copy(tp2, pos);
            Quat::copy(tq2, q);
            int ok = -1;
            if(placementAutoTargetType == 0) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 1) {
                ok = this->roadDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
            } else if(placementAutoTargetType == 2) {
                ok = this->trackDB->findNearestPositionOnTDB(playerT, tp, tq, ttpos);
                int ok2 = this->roadDB->findNearestPositionOnTDB(playerT2, tp2, tq2, ttpos);
                if(ok2 >= 0)
                    if(ok < 0 || ok2 < ok){
                        ok = ok2;
                        Vec3::copy(tp, tp2);
                        Quat::copy(tq, tq2);
                        Vec2::copy(playerT, playerT2);
                    }
            }
            if(ok >= 0 && ok <= Game::snapableRadius) {
                Quat::copy(q, tq);
                if(!snapableOnlyRotation){
                    Vec3::copy(tpos, tp);
                    x = playerT[0];
                    z = playerT[1];
                }
            }
    }
    
    
    if(obj->isTrackItem() || obj->typeID == obj->groupobject || obj->typeID == obj->ruler ){
        obj->setPosition(x, z, tpos);
        obj->setMartix();
        return;
    }
    
    if (obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack)
        if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
            obj->setPosition(x, z, pos);
            obj->setMartix();
            return;
    }
    
    if(placementStickToTarget && placementAutoTargetType == 3){
        snapableSide = tTile->getNearestSnapablePosition(tpos, q, obj->UiD);
    }

    if ((obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack )) {
        if(this->tsection->isRoadShape(obj->sectionIdx)){
            this->roadDB->setDefaultEnd(0);
            this->roadDB->findPosition(x, z, tpos, q, obj->endp, obj->sectionIdx);
        }else{
            this->trackDB->setDefaultEnd(0);
            this->trackDB->findPosition(x, z, tpos, q, obj->endp, obj->sectionIdx);
        }
        tTile = requestTile(x, z);
        if(tTile == NULL) return;
        if(tTile->loaded != 1) return;
    }

    /// 
    
    obj->setPosition(tpos);
    Vec3::copy(obj->firstPosition, obj->position);
    obj->setQdirection(q);
    obj->snapped(snapableSide);
    moveWorldObjToTile(x, z, obj);
    obj->setMartix();
    obj->setModified();
}

TRitem *Route::getTrackItem(int TID, int UID){
    if(TID == 0)
        return trackDB->trackItems[UID];  if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
    if(TID == 1)
        return roadDB->trackItems[UID]; // if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
    return NULL;
}

void Route::deleteTrackItem(TRitem * item){
    if(item == NULL)
        return;
    unsigned int TID = item->tdbId;
    unsigned int UID = item->trItemId;
    if(TID == 0)
        return trackDB->deleteTrItem(UID);
    if(TID == 1)
        return roadDB->deleteTrItem(UID);
    return;
}

void Route::actPickNewEventLocation(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->pickNewEventLocation(tpos);
    }
}

void Route::actNewLooseConsist(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->newLooseConsist(tpos);
    }
}

void Route::actNewFailedSignal(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    
    
}

void Route::actNewNewSpeedZone(int x, int z, float* p){
    if(currentActivity == NULL)
        return;
    float tp[3];
    float tpos[3];
    float posT[2];
    
    Vec3::copy(tp, p);
    Game::check_coords(x, z, tp);
    posT[0] = x;
    posT[1] = z;            
    int ok = this->trackDB->findNearestPositionOnTDB(posT, tp, NULL, tpos);
    if(ok >= 0){
        currentActivity->newSpeedZone(tpos);
    }
}

Tile * Route::requestTile(int x, int z, bool allowNew){
    Tile *tTile = tile[((x)*10000 + z)];
    if (tTile == NULL){
        tile[(x)*10000 + z] = new Tile(x, z);
        tTile = tile[(x)*10000 + z];
    }

    if(!allowNew)
        return tTile;

    if (tTile->loaded == -2) {
        if (Game::terrainLib->isLoaded(x, z)) {
            tTile->initNew();
        } else {
            return NULL;
        }
    }
    return tTile;
}

void Route::linkSignal(int x, int z, float* p, WorldObj* obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID != obj->signal)
        return;
    SignalObj* sobj = (SignalObj*)obj;
    float *tpos = new float[3];
    float* playerT = Vec2::fromValues(x, z);
    int ok = this->trackDB->findNearestPositionOnTDB(playerT, p, NULL, tpos);
    if(ok < 0) return;
    sobj->linkSignal(tpos[0], tpos[1]);
}

float *fromtwovectors(float* out, float* u, float* v){
    //float m = sqrt(2.f + 2.f * Vec3::dot(u, v));
    float w[3];
    /*Vec3::cross((float*)w, u, v);
    Vec3::scale((float*)w, (float*)w, (1.f / m));
    out[0] = 0.5f * m;
    out[1] = w[0];
    out[2] = w[1];
    out[3] = w[2];*/
    float cos_theta = Vec3::dot(u, v);
    float angle = acos(cos_theta);
    Vec3::cross(w, u, v);
    Vec3::normalize(w, w);
    Quat::setAxisAngle(out, w, angle);
    return out;
}

WorldObj* Route::autoPlaceObject(int x, int z, float* p, int mode) {
    if(ref->selected == NULL) return NULL;
    Game::check_coords(x, z, p);
    
    autoPlacementLastPlaced.clear();
    
    TDB * tdb = NULL;
    if(placementAutoTargetType == 0)
        tdb = this->trackDB;
    else if(placementAutoTargetType == 1)
        tdb = this->roadDB;
    else if(placementAutoTargetType == 2)
        tdb = this->trackDB;
    else
        return NULL;
    
    // pozycja wzgledem TDB:
    float* tpos = new float[3];
    float* playerT = Vec2::fromValues(x, z);
    int ok = tdb->findNearestPositionOnTDB(playerT, p, NULL, tpos);
    if(ok < 0) return NULL;
    
    x = playerT[0];
    z = playerT[1];
    int trackNodeIdx = tpos[0];
    int length = tdb->getVectorSectionLength(trackNodeIdx);
    int metry = 0;
    float drawPosition1[7];
    float drawPosition2[7];
    float xyz[3];
    float *quat = Quat::create();
    float *vec1 = Vec3::create();
    vec1[2] = -1.0;
    float *vec2 = Vec3::create();
    float step = placementAutoLength;
    float startPos = 0;
    float endPos = length;
    float rot = 0;
    if(mode == 1){
        startPos = tpos[1];
    }
    if(mode == 2){
        startPos = 0;
        endPos = tpos[1];
        rot = M_PI;
    }    
    float i1, i2;
    for(float i = startPos; i < endPos; i+=step ){
        if(mode == 2){
           i1 = endPos - i;
           i2 = i1-step;
           if(i2 < 0)
                i2 = 0 + 0.1;
        } else {
            i1 = i;
            i2 = i1+step;
            if(i2 > length)
                i2 = length - 0.1;
        }
        if(!tdb->getDrawPositionOnTrNode((float*)drawPosition1, trackNodeIdx, i1))
            return NULL;
        if(!tdb->getDrawPositionOnTrNode((float*)drawPosition2, trackNodeIdx, i2))
            return NULL;
        x = drawPosition1[5];
        z = -drawPosition1[6];
        
        /*x = currentPosition[5];
        z = -currentPosition[6];
        //vec1[0] = currentPosition[0];
        //vec1[1] = currentPosition[1];
        //vec1[2] = -currentPosition[2];
        vec2[0] = currentPosition1[0] - (currentPosition[0]-(currentPosition1[5]-currentPosition[5])*2048);
        vec2[1] = currentPosition1[1] - currentPosition[1];
        vec2[2] = -currentPosition1[2] + (currentPosition[2]-(currentPosition1[6]-currentPosition[6])*2048);
        Vec3::normalize(vec2, vec2);
        //Vec3::normalize(vec1, vec1);
        if(placementAutoTwoPointRot){
            fromtwovectors(quat, vec1, vec2);
            Quat::rotateY(quat, quat, rot);
        }else {
            Quat::fill(quat);
            Quat::rotateY(quat, quat, currentPosition[3]);
            Quat::rotateX(quat, quat, -currentPosition[4]);
        }
        */
        drawPosition2[0] += 2048*(drawPosition2[5]-drawPosition1[5]);
        drawPosition2[2] += 2048*(drawPosition2[6]-drawPosition1[6]);
        float dlugosc = Vec3::distance(drawPosition1, drawPosition2);

        int someval = (((drawPosition2[2]-drawPosition1[2])+0.00001f)/fabs((drawPosition2[2]-drawPosition1[2])+0.00001f));
        float rotY = ((float)someval+1.0)*(M_PI/2)+(float)(atan((drawPosition1[0]-drawPosition2[0])/(drawPosition1[2]-drawPosition2[2]))); 
        float sinv = (drawPosition1[1]-drawPosition2[1])/(dlugosc);
        if(sinv > 1.0f)
            sinv = 1.0f;
        if(sinv < -1.0f)
            sinv = -1.0f;
        float rotX = -(float)asin(sinv); 

        if(placementAutoTwoPointRot){
            Quat::fill(quat);
            Quat::rotateY(quat, quat, -rotY+M_PI);
            Quat::rotateX(quat, quat, rotX);
        }else {
            Quat::fill(quat);
            Quat::rotateY(quat, quat, drawPosition1[3]+rot);
            Quat::rotateX(quat, quat, -drawPosition1[4]);
        }
        
        float offset[3];
        Vec3::copy(offset, placementAutoTranslationOffset);
        float offsetq[4];
        Quat::fill(offsetq);
        Quat::rotateY(offsetq,offsetq,(placementAutoRotationOffset[1]*M_PI)/180);
        Quat::rotateX(offsetq,offsetq,(placementAutoRotationOffset[0]*M_PI)/180);
        
        Vec3::transformQuat(offset, offset, quat);
        Quat::multiply(quat, quat, offsetq);
        
        xyz[0] = drawPosition1[0] + offset[0];
        xyz[1] = drawPosition1[1] + offset[1];
        xyz[2] = -drawPosition1[2] + offset[2];      
        
        autoPlacementLastPlaced.push_back(placeObject(x, z, (float*) xyz, quat, 0, ref->selected));
    }

    return NULL;
    
}

void Route::fillWorldObjectsByTrackItemIds(QHash<int,QVector<WorldObj*>> &objects, int tdbId){
    foreach (Tile* tTile, tile){ 
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->fillWorldObjectsByTrackItemIds(objects, tdbId);
        }
    }
}

void Route::fillWorldObjectsByTrackItemId(QVector<WorldObj*> &objects, int tdbId, int id){
    foreach (Tile* tTile, tile){  
        if (tTile == NULL) continue;
        if (tTile->loaded == 1) {
            tTile->fillWorldObjectsByTrackItemId(objects, tdbId, id);
        }
    }
}

void Route::findSimilar(WorldObj* obj, GroupObj* group, float *playerT, int tileRadius){
    if(obj->typeID == WorldObj::groupobject)
        return;
    int mintile = -tileRadius;
    int maxtile = tileRadius;
    
    Tile *tTile;
    for (int i = mintile; i <= maxtile; i++) {
        for (int j = maxtile; j >= mintile; j--) {
            tTile = tile[((int)playerT[0] + i)*10000 + (int)playerT[1] + j];
            if (tTile == NULL)
                continue;
            if (tTile->loaded == 1) {
                tTile->findSimilar(obj, group);
            }
        }
    }
}

void Route::autoPlacementDeleteLast(){
    for(int i = 0; i < autoPlacementLastPlaced.length(); i++){
        deleteObj(autoPlacementLastPlaced[i]);
    }
    autoPlacementLastPlaced.clear();
}


void Route::replaceWorldObjPointer(WorldObj* o, WorldObj* n){
    if(o->typeObj != WorldObj::worldobj)
        return;
    if(n->typeObj != WorldObj::worldobj)
        return;
    int x = o->x;
    int z = o->y;
    if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":";
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return;
    
    for(int i = 0; i < tTile->jestObiektow; i++){
        if(tTile->obiekty[i] == NULL) continue;
        if(tTile->obiekty[i]->UiD == o->UiD){
            tTile->obiekty[i] = n;
            if(waterRulerObj == o){
                RulerObj *replacementRuler = dynamic_cast<RulerObj*>(n);
                waterRulerObj = replacementRuler != NULL
                        && replacementRuler->isWaterRuler()
                        ? replacementRuler : NULL;
            }
            if(vegetationRulerObj == o){
                RulerObj *replacementRuler = dynamic_cast<RulerObj*>(n);
                vegetationRulerObj = replacementRuler != NULL
                        && replacementRuler->isVegetationRuler()
                        ? replacementRuler : NULL;
            }
            emit objectSelected((GameObj*)n);
            return;
        }
    }
}

void Route::addToTDB(WorldObj* obj) {
    //qDebug() << "1971";
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    //qDebug() << "A2TDB 1981";    
    int x = obj->x;//post[0];
    int z = obj->y;//post[1];
    float p[3];
    //p[0] = pos[0];
    //p[1] = pos[1];
    //p[2] = pos[2];
    p[0] = obj->position[0];
    p[1] = obj->position[1];
    p[2] = obj->position[2];
    //Game::check_coords(x, z, (float*) &p);
    float q[4];
    //q[0] = obj->tRotation[0]; //track->qDirection[0];
    //q[1] = obj->tRotation[1]; //qDirection[1];
    //q[2] = 0; //track->qDirection[2];
    //q[3] = 1; //track->qDirection[3];
    q[0] = obj->qDirection[0];
    q[1] = obj->qDirection[1];
    q[2] = obj->qDirection[2];
    q[3] = obj->qDirection[3];
    //qDebug() << "A2TDB 2001";
    
    if (obj->type == "trackobj") {
        //qDebug() << "A2TDB 2003";
        TrackObj* track = (TrackObj*) obj;
        //this->trackDB->placeTrack(x, z, p, q, r, nowy->UiD);
        //float scale = (float) sqrt(track->qDirection[0] * track->qDirection[0] + track->qDirection[1] * track->qDirection[1] + track->qDirection[2] * track->qDirection[2]);
        //float elevation = ((track->qDirection[0] + 0.0000001f) / fabs(scale + 0.0000001f))*(float) -acos(track->qDirection[3])*2;
        //float elevation = -3.14/16.0;
        //q[0] = elevation;
                //qDebug() << "A2TDB 2010";
        
//        if(track->sectionIdx > trackDB->tsection->tsectionMaxIdx )
//        {
//            qDebug() << "Section IDX out of range for TrackObj";
//            return;
//        }
        
        if(this->tsection->isRoadShape(track->sectionIdx))
            this->roadDB->placeTrack(x, z, (float*) &p, (float*) &q, track->sectionIdx, obj->UiD);
        else
            this->trackDB->placeTrack(x, z, (float*) &p, (float*) &q, track->sectionIdx, obj->UiD, &track->jNodePosn);
        //qDebug() << "A2TDB 2015";                
        //obj->setPosition(p);
        //obj->setQdirection(q);
        //obj->setMartix();
        //track->setJNodePosN();
    } else if(obj->type == "dyntrack"){
        //qDebug() << "A2TDB 2021";        
        Undo::Clear();
        DynTrackObj* dynTrack = (DynTrackObj*) obj;
        
                
        /// EFO If the sectionIdx is out of range
        if(dynTrack->sectionIdx > trackDB->tsection->routeMaxIdx)
        {
            int prevSectionIdx = dynTrack->sectionIdx;
            dynTrack->sectionIdx = -1;
            qWarning() << "DT SectionIDX " << prevSectionIdx << " is greater than max in local TSection -- " << trackDB->tsection->routeMaxIdx << " -- resetting to be safe." ;
        } 
        if(dynTrack->sectionIdx == -1){
            this->trackDB->fillDynTrack(dynTrack);
            if(Game::debugOutput) qDebug() << "DT SectionIDX " << dynTrack->sectionIdx << " being written to tSection... ";
        }
        //qDebug() << "A2TDB 2038";
        // Mesh, TDB sections and the yellow database line must use the same
        // planar transform.  A separate average-grade quaternion introduces
        // a vertical break at every curve/straight section boundary.
        this->trackDB->placeTrack(x, z, (float*) &p, (float*) &q, dynTrack->sectionIdx, obj->UiD);
        obj->setPosition(p);
        obj->setQdirection(q);
        obj->setModified();
        obj->setMartix();
    } 
}

void Route::setTDB(TDB* tdb, bool road){
    if(tdb == NULL)
        return;
    if(road){
        delete this->roadDB;
        this->roadDB = tdb;
        Game::roadDB = tdb;
    } else {
        delete this->trackDB;
        this->trackDB = tdb;
        Game::trackDB = tdb;
    }
}

void Route::toggleToTDB(WorldObj* obj) {
    if(obj == NULL) return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            toggleToTDB(gobj->objects[i]);
        }
        return;
    }
    
    if (obj->type != "trackobj" && obj->type != "dyntrack") {
            return;
    }
    if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
        removeTrackFromTDB(obj);
        obj->setModified();   //// EFO added to account for unsaved on TDB edits only
        return;
    }
    addToTDB(obj);
    obj->setModified();   //// EFO added to account for unsaved on TDB edits only
}

void Route::addToTDBIfNotExist(WorldObj* obj) {
    //qDebug() << "A2TDB 2086";
    if(obj == NULL) return;
    //qDebug() << "A2TDB 2088";
    if(obj->typeObj != WorldObj::worldobj)
        return;
    //qDebug() << "A2TDB 2089";
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            addToTDBIfNotExist(gobj->objects[i]);   //qDebug() << "A2TDB 2095 Exit";
        }
        return;
    }
    
    if (obj->type != "trackobj" && obj->type != "dyntrack") {
            //qDebug() << "A2TDB 2100 Exit";
            return;
    }
       
    if(roadDB->ifTrackExist(obj->x, obj->y, obj->UiD) || trackDB->ifTrackExist(obj->x, obj->y, obj->UiD)){
            //qDebug() << "A2TDB 2105 Exit";
            return;
        }
    
    //qDebug() << "A2TDB 2110";    
    Undo::StateBegin();
    Undo::PushTrackDB(trackDB, false);
    Undo::PushTrackDB(roadDB, true);
    Undo::StateEnd();
    //qDebug() << "A2TDB 2115";    
    addToTDB(obj);
}

RulerObj* Route::placeWaterRuler(int x, int z, float *p) {
    if(p == NULL)
        return NULL;
    // There may be only one route-wide water ruler. Check saved world files
    // as well as loaded tiles before creating another marker.
    RulerObj *existingRuler = findWaterRuler(true);
    if(existingRuler != NULL)
        return existingRuler;
    Game::check_coords(x, z, p);
    Tile *worldTile = requestTile(x, z);
    if(worldTile == NULL || worldTile->loaded != 1)
        return NULL;

    WorldObj *base = WorldObj::createObj("ruler");
    RulerObj *rulerObj = dynamic_cast<RulerObj*>(base);
    if(rulerObj == NULL){
        delete base;
        return NULL;
    }
    float q[4];
    Quat::fill(q);
    rulerObj->initPQ(p, q);
    rulerObj->setWaterRuler(true);
    rulerObj->load(x, z);
    worldTile->placeObject(rulerObj);
    Undo::PushWorldObjPlaced(rulerObj);
    waterRulerObj = rulerObj;
    return rulerObj;
}

RulerObj* Route::findWaterRuler(bool loadWorldTiles) {
    if(waterRulerObj != NULL && waterRulerObj->loaded
            && waterRulerObj->isWaterRuler())
        return waterRulerObj;

    auto findInLoadedTiles = [this]() -> RulerObj* {
        QList<int> keys = tile.keys();
        for(int key : keys){
            Tile *worldTile = tile.value(key, NULL);
            if(worldTile == NULL || worldTile->loaded != 1)
                continue;
            for(int i = 0; i < worldTile->jestObiektow; i++){
                RulerObj *rulerObj = dynamic_cast<RulerObj*>(worldTile->obiekty[i]);
                if(rulerObj != NULL && rulerObj->loaded && rulerObj->isWaterRuler())
                    return rulerObj;
            }
        }
        return NULL;
    };

    RulerObj *loadedRuler = findInLoadedTiles();
    if(loadedRuler != NULL){
        waterRulerObj = loadedRuler;
        return loadedRuler;
    }
    if(!loadWorldTiles)
        return loadedRuler;

    // Do not preload every world object merely to recover one helper ruler.
    // Large routes can exhaust address space when that is followed immediately
    // by loading a terrain corridor. Newly saved WaterRuler objects live in
    // uncompressed UTF-16 world files, so locate the owning file cheaply and
    // load only that one tile.
    QString worldPath = Game::root + "/routes/" + Game::route + "/world";
    QDir worldDir(worldPath);
    worldDir.setFilter(QDir::Files);
    worldDir.setNameFilters(QStringList() << "*.w");
    QStringList worldFiles = worldDir.entryList();

    QByteArray utf16Marker;
    const QString marker = "WaterRuler";
    for(QChar c : marker){
        ushort value = c.unicode();
        utf16Marker.append((char)(value & 0xff));
        utf16Marker.append((char)((value >> 8) & 0xff));
    }

    for(int i = 0; i < worldFiles.size(); i++){
        const QString &worldFile = worldFiles[i];
        QFile file(worldDir.filePath(worldFile));
        if(!file.open(QIODevice::ReadOnly))
            continue;
        QByteArray raw = file.readAll();
        if(!raw.contains(utf16Marker) && !raw.contains("WaterRuler"))
            continue;
        if(worldFile.length() != 17)
            continue;
        bool xOk = false;
        bool zOk = false;
        int worldX = worldFile.mid(1, 7).toInt(&xOk);
        int worldZ = -worldFile.mid(8, 7).toInt(&zOk);
        if(!xOk || !zOk)
            continue;
        Tile *worldTile = requestTile(worldX, worldZ);
        if(worldTile == NULL || worldTile->loaded != 1)
            continue;
        for(int i = 0; i < worldTile->jestObiektow; i++){
            RulerObj *rulerObj = dynamic_cast<RulerObj*>(worldTile->obiekty[i]);
            if(rulerObj != NULL && rulerObj->loaded && rulerObj->isWaterRuler()){
                waterRulerObj = rulerObj;
                return rulerObj;
            }
        }
    }
    return NULL;
}

RulerObj* Route::placeVegetationRuler(int x, int z, float *p) {
    if(p == NULL)
        return NULL;
    RulerObj *existingRuler = findVegetationRuler(true);
    if(existingRuler != NULL)
        return existingRuler;
    Game::check_coords(x, z, p);
    Tile *worldTile = requestTile(x, z);
    if(worldTile == NULL || worldTile->loaded != 1)
        return NULL;

    WorldObj *base = WorldObj::createObj("ruler");
    RulerObj *rulerObj = dynamic_cast<RulerObj*>(base);
    if(rulerObj == NULL){
        delete base;
        return NULL;
    }
    float q[4];
    Quat::fill(q);
    rulerObj->initPQ(p, q);
    rulerObj->setVegetationRuler(true);
    rulerObj->load(x, z);
    worldTile->placeObject(rulerObj);
    Undo::PushWorldObjPlaced(rulerObj);
    vegetationRulerObj = rulerObj;
    return rulerObj;
}

RulerObj* Route::findVegetationRuler(bool loadWorldTiles) {
    if(vegetationRulerObj != NULL && vegetationRulerObj->loaded
            && vegetationRulerObj->isVegetationRuler())
        return vegetationRulerObj;

    auto findInLoadedTiles = [this]() -> RulerObj* {
        QList<int> keys = tile.keys();
        for(int key : keys){
            Tile *worldTile = tile.value(key, NULL);
            if(worldTile == NULL || worldTile->loaded != 1)
                continue;
            for(int i = 0; i < worldTile->jestObiektow; i++){
                RulerObj *rulerObj = dynamic_cast<RulerObj*>(worldTile->obiekty[i]);
                if(rulerObj != NULL && rulerObj->loaded && rulerObj->isVegetationRuler())
                    return rulerObj;
            }
        }
        return NULL;
    };

    RulerObj *loadedRuler = findInLoadedTiles();
    if(loadedRuler != NULL){
        vegetationRulerObj = loadedRuler;
        return loadedRuler;
    }
    if(!loadWorldTiles)
        return NULL;

    QString worldPath = Game::root + "/routes/" + Game::route + "/world";
    QDir worldDir(worldPath);
    worldDir.setFilter(QDir::Files);
    worldDir.setNameFilters(QStringList() << "*.w");
    QStringList worldFiles = worldDir.entryList();
    QByteArray utf16Marker;
    const QString marker = "VegetationRuler";
    for(QChar c : marker){
        ushort value = c.unicode();
        utf16Marker.append((char)(value & 0xff));
        utf16Marker.append((char)((value >> 8) & 0xff));
    }

    for(const QString &worldFile : worldFiles){
        QFile file(worldDir.filePath(worldFile));
        if(!file.open(QIODevice::ReadOnly))
            continue;
        QByteArray raw = file.readAll();
        if(!raw.contains(utf16Marker) && !raw.contains("VegetationRuler"))
            continue;
        if(worldFile.length() != 17)
            continue;
        bool xOk = false;
        bool zOk = false;
        int worldX = worldFile.mid(1, 7).toInt(&xOk);
        int worldZ = -worldFile.mid(8, 7).toInt(&zOk);
        if(!xOk || !zOk)
            continue;
        Tile *worldTile = requestTile(worldX, worldZ);
        if(worldTile == NULL || worldTile->loaded != 1)
            continue;
        RulerObj *rulerObj = findInLoadedTiles();
        if(rulerObj != NULL){
            vegetationRulerObj = rulerObj;
            return rulerObj;
        }
    }
    return NULL;
}

bool Route::placementEndpointBelongsToTrack(const WorldObj *placed, int x, int y, unsigned int uid) const {
    if(placed == NULL || placed->placementSnapNodeId < 0)
        return false;
    TDB *database = placed->placementSnapRoad ? roadDB : trackDB;
    return database != NULL
            && database->endpointBelongsToTrack(placed->placementSnapNodeId, x, y, uid);
}

void Route::newPositionTDB(WorldObj* obj) {
    if(obj->typeObj != WorldObj::worldobj)
        return;
    int x = obj->x;//post[0];
    int z = obj->y;//post[1];
    float p[3]; 
    p[0] = obj->firstPosition[0];
    p[1] = obj->firstPosition[1];
    p[2] = obj->firstPosition[2];
    Game::check_coords(x, z, (float*) &p);

    if (obj->type == "trackobj") {
        float q[4];
        q[0] = 0;
        q[1] = 0;
        q[2] = 0;
        q[3] = 1;
        TrackObj* track = (TrackObj*) obj;
        //this->trackDB->placeTrack(x, z, p, q, r, nowy->UiD);
        if(this->tsection->isRoadShape(track->sectionIdx))
            this->roadDB->findPosition(x, z, (float*) &p, (float*) &q, track->endp, track->sectionIdx);
        else
            this->trackDB->findPosition(x, z, (float*) &p, (float*) &q, track->endp, track->sectionIdx);

        //Vec3::copy(obj->position, p);
        obj->setPosition(p);
        obj->setQdirection(q);
        obj->setMartix();
        obj->setModified();
        moveWorldObjToTile(x, z, obj);
    }
}

void Route::moveWorldObjToTile(int x, int z, WorldObj* obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    //qDebug() << "new tile" << obj->x <<" "<< obj->y<<" "<< obj->position[0]<<" "<< -obj->position[2];
    float oldPos[3];
    int xx = x, zz = z;
    Vec3::copy(oldPos, obj->position);
    Game::check_coords(xx, zz, oldPos);
    if(xx == obj->x && zz == obj->y)
        return;
    Vec3::copy(obj->position, oldPos);
    x = xx;
    z = zz;
    
    if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "obj outside tile border !!!";
    //qDebug() << "new tile" << x <<" "<< z;
    if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "new tile" << xx <<" "<< zz <<" "<< obj->position[0]<<" "<< -obj->position[2];

    
    Undo::Clear();
    
    Tile *tTile = tile[((obj->x)*10000 + obj->y)];
    tTile->deleteObject(obj);
    
    tTile = requestTile(x, z);
    if(tTile == NULL) return;
    if(tTile->loaded != 1) return;
    
    if (tTile->loaded == 1) {
        obj->firstPosition[0] -= (x-obj->x)*2048;
        obj->firstPosition[2] -= (z-obj->y)*2048;
        obj->placedAtPosition[0] = obj->position[0];
        obj->placedAtPosition[2] = obj->position[2];
        tTile->placeObject(obj);
    }
    if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" << "--" << obj->x <<" "<< obj->y<<" "<< obj->position[0]<<" "<< -obj->position[2];
}

void Route::deleteTDBTree(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        this->roadDB->deleteTree(obj->x, obj->y, obj->UiD);
        this->trackDB->deleteTree(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::fixTDBVectorElevation(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        //this->roadDB->fixTDBVectorElevation(obj->x, obj->y, obj->UiD);
        this->trackDB->fixTDBVectorElevation(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::deleteTDBVector(WorldObj* obj){
    Undo::StateBegin();
    Undo::PushTrackDB(this->trackDB, false);
    Undo::PushTrackDB(this->roadDB, true);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        this->roadDB->deleteVectorSection(obj->x, obj->y, obj->UiD);
        this->trackDB->deleteVectorSection(obj->x, obj->y, obj->UiD);
    }
    Undo::StateEnd();
}

void Route::undoPlaceObj(int x, int y, int UiD){
    Tile *tTile;
    tTile = tile[((x)*10000 + y)];
    if (tTile == NULL)
        return;
    
    for(int i = 0; i < tTile->jestObiektow; i++){
        if(tTile->obiekty[i] == NULL) continue;
        if(tTile->obiekty[i]->UiD == UiD){
            tTile->obiekty[i]->loaded = false;
            tTile->obiekty[i]->modified = false;
            tTile->obiekty[i]->UiD = -1;
            emit sendMsg("unselect");
            return;
        }
    }
}

void Route::deleteObj(WorldObj* obj) {
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj == waterRulerObj)
        waterRulerObj = NULL;
    if(obj == vegetationRulerObj)
        vegetationRulerObj = NULL;
    if(obj->typeID == obj->groupobject) {
        GroupObj *gobj = (GroupObj*)obj;
        for(int i = 0; i < gobj->objects.size(); i++ ){
            deleteObj(gobj->objects[i]);
        }
        return;
    }
    
    Undo::PushWorldObjRemoved(obj);
    
    if (obj->type == "trackobj" || obj->type == "dyntrack") {
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
        removeTrackFromTDB(obj);
        if(Game::leaveTrackShapeAfterDelete)
            return;
    }
    
    obj->loaded = false;
    obj->setModified();
    if (obj->isTrackItem()) {
        Undo::PushTrackDB(trackDB, false);
        Undo::PushTrackDB(roadDB, true);
        obj->deleteTrItems();
    }
    Tile *tTile;
    tTile = tile[((obj->x)*10000 + obj->y)];
    if (tTile != NULL)
        tTile->jestHiddenObj++;
}

int Route::removeAllInteractives(bool gui) {
    // A route-wide cleanup must not depend on which tiles happen to be visible.
    // preloadWFiles creates and loads every world tile that is not already in
    // the route tile map.
    preloadWFiles(gui);

    QVector<WorldObj*> interactives;
    foreach (Tile *tTile, tile) {
        if (tTile == NULL || tTile->loaded != 1)
            continue;
        for (auto it = tTile->obiekty.begin(); it != tTile->obiekty.end(); ++it) {
            WorldObj *obj = it->second;
            if (obj != NULL && obj->loaded && obj->isTrackItem())
                interactives.push_back(obj);
        }
    }

    if (interactives.isEmpty())
        return 0;

    QProgressDialog *progress = NULL;
    if (gui) {
        progress = new QProgressDialog(
            "Removing All Interactives ...", "", 0, interactives.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }

    Undo::StateBegin();
    Undo::PushTrackDB(trackDB, false);
    Undo::PushTrackDB(roadDB, true);
    for (int i = 0; i < interactives.size(); ++i) {
        WorldObj *obj = interactives[i];
        Undo::PushWorldObjRemoved(obj);
        obj->loaded = false;
        obj->setModified();
        obj->deleteTrItems();

        Tile *tTile = tile.value(obj->x * 10000 + obj->y, NULL);
        if (tTile != NULL)
            ++tTile->jestHiddenObj;

        if (progress != NULL) {
            progress->setValue(i + 1);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }
    }
    Undo::StateEnd();

    delete progress;
    emit sendMsg("unselect");
    return interactives.size();
}

int Route::deleteAllInstances(WorldObj *selected, bool gui) {
    if(selected == NULL
    || (selected->typeID != WorldObj::sstatic
        && selected->typeID != WorldObj::gantry
        && selected->typeID != WorldObj::collideobject))
        return 0;

    const WorldObj::TypeID selectedType = selected->typeID;
    const QString selectedFileName = selected->fileName.trimmed();
    if(selectedFileName.isEmpty())
        return 0;

    // Instance cleanup must inspect the route, not merely the tiles currently
    // around the camera. Constructing missing Tile objects loads their world
    // files before the matching pass begins.
    preloadWFiles(gui);

    QVector<WorldObj*> matches;
    foreach (Tile *tTile, tile) {
        if(tTile == NULL || tTile->loaded != 1)
            continue;
        for(auto it = tTile->obiekty.begin(); it != tTile->obiekty.end(); ++it) {
            WorldObj *obj = it->second;
            if(obj == NULL || !obj->loaded || obj->typeID != selectedType)
                continue;
            if(QString::compare(
                    obj->fileName.trimmed(), selectedFileName,
                    Qt::CaseInsensitive) == 0)
                matches.push_back(obj);
        }
    }

    if(matches.isEmpty())
        return 0;

    QProgressDialog *progress = NULL;
    if(gui) {
        progress = new QProgressDialog(
            "Deleting Matching Object Instances ...", "", 0, matches.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }

    Undo::StateBegin();
    for(int i = 0; i < matches.size(); ++i) {
        deleteObj(matches[i]);
        if(progress != NULL) {
            progress->setValue(i + 1);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }
    }
    Undo::StateEnd();

    delete progress;
    emit sendMsg("unselect");
    return matches.size();
}

void Route::removeTrackFromTDB(WorldObj* obj) {
    if(obj->typeObj != WorldObj::worldobj)
        return;
    bool ok;
    ok = this->roadDB->removeTrackFromTDB(obj->x, obj->y, obj->UiD);
    ok |= this->trackDB->removeTrackFromTDB(obj->x, obj->y, obj->UiD);
    if(ok)
        obj->removedFromTDB();
}

int Route::getTileObjCount(int x, int z) {
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return 0;
    return tTile->jestObiektow;
}

int Route::getTileHiddenObjCount(int x, int z) {
    Tile *tTile;
    tTile = tile[((x)*10000 + z)];
    if (tTile == NULL)
        return 0;
    return tTile->jestHiddenObj;
}

void Route::getUnsavedInfo(QVector<QString> &items){
    if (!Game::writeEnabled) return;
    
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1 && tTile->isModified()) {
            items.push_back("[W] "+QString::number(tTile->x)+" "+QString::number(-tTile->z));
        }
    }
    Game::terrainLib->getUnsavedInfo(items);
    if(this->trk->isModified())
        items.push_back("[S] Route Settings - TRK File");
    
    ActLib::GetUnsavedInfo(items);
    
    /*foreach(Service *s, service){
        if(s == NULL)
            continue;
        if(s->isModified())
            items.push_back("[S] "+s->name);
    }
    foreach(Path *p, path){
        if(p == NULL)
            continue;
        if(p->isModified())
            items.push_back("[P] "+p->name);
    }*/
    //this->trackDB->save();
    //this->roadDB->save();
}


void Route::save() {
    lastSaveResult = false;
    if (!Game::writeEnabled) return;
    qDebug() << __FILE__ << " " << __LINE__ << ":" << "save";
    foreach (Tile* tTile, tile){
        if (tTile == NULL) continue;
        if (tTile->loaded == 1 && tTile->isModified()) {
            tTile->save();
            tTile->setModified(false);
        }
    }
    Game::terrainLib->save();

    QString keySaveError;
    if(!saveKeyRouteFiles(keySaveError)){
        qWarning() << "Route key-file save failed:" << keySaveError;
        emit sendMsg("saveError");
        if(Game::gui){
            QMessageBox::critical(NULL, QObject::tr("Route save failed"),
                    QObject::tr("The route key files were not saved.\n\n%1\n\n"
                                "The previous files were left intact or restored from "
                                "the managed backup.")
                    .arg(keySaveError));
        }
        return;
    }

    ActLib::SaveAll();
    
    /// EFO this is a hack to trick the updated timestamp on the folder.  It could be used as a stub for 
    /// moving the TSRE log out of the TSRE folder and placing into the route folder, which might be a better place for it
    QString filePath;
    filePath = Game::root + "/routes/" + Game::route + "/" + Game::routeName + "tsreupd.txt";
    QFile file(filePath);    
    file.open(QIODevice::WriteOnly);
    file.close(); 
    QFile::remove(filePath);
    lastSaveResult = true;
    
    
    /*foreach(Service *s, service){
        if(s == NULL)
            continue;
        if(s->isModified())
            s->save();
    }
    foreach(Path *p, path){
        if(p == NULL)
            continue;
        if(p->isModified())
            p->save();
    }*/
}

bool Route::lastSaveSucceeded() const {
    return lastSaveResult;
}

bool Route::saveKeyRouteFiles(QString &error) {
    if(!Game::writeTDB && (trk == NULL || !trk->isModified()))
        return true;
    if(trackDB == NULL || roadDB == NULL || tsection == NULL || trk == NULL){
        error = "The route databases are not fully loaded.";
        return false;
    }

    if(Game::writeTDB){
        trackDB->prepareSave();
        roadDB->prepareSave();
    }

    RouteSaveTransaction transaction(activeRouteRoot(), activeRouteBackupRoot());
    const QString routeBase = activeRouteRoot() + "/" + Game::routeName;
    QByteArray data;

    if(Game::writeTDB){
        if(!serializeUtf16Text(Game::rnp, "SIMISA@@@@@@@@@@JINX0T0t______\n\n",
                              [this](QTextStream &out){ trackDB->saveToStream(out); },
                              data, error)
                || !transaction.addFile(routeBase + ".tdb", data, &error))
            return false;

        if(!serializeUtf16Text(Game::rnp, "SIMISA@@@@@@@@@@JINX0T0t______\n\n",
                              [this](QTextStream &out){ trackDB->saveTitToStream(out); },
                              data, error)
                || !transaction.addFile(routeBase + ".tit", data, &error))
            return false;

        if(tsection->routeMaxIdx >= 3){
            if(!serializeUtf16Text(6, "SIMISA@@@@@@@@@@JINX0T0t______\n\n",
                                  [this](QTextStream &out){ tsection->saveRouteToStream(out); },
                                  data, error)
                    || !transaction.addFile(activeRouteRoot() + "/tsection.dat", data, &error))
                return false;
        }

        if(!serializeUtf16Text(Game::rnp, "SIMISA@@@@@@@@@@JINX0T0t______\n\n",
                              [this](QTextStream &out){ roadDB->saveToStream(out); },
                              data, error)
                || !transaction.addFile(routeBase + ".rdb", data, &error))
            return false;

        if(!serializeUtf16Text(Game::rnp, "SIMISA@@@@@@@@@@JINX0T0t______\n\n",
                              [this](QTextStream &out){ roadDB->saveTitToStream(out); },
                              data, error)
                || !transaction.addFile(routeBase + ".rit", data, &error))
            return false;
    }

    const bool saveTrk = trk->isModified();
    if(saveTrk){
        if(!serializeUtf16Text(8, "SIMISA@@@@@@@@@@JINX0r1t______\n\n",
                              [this](QTextStream &out){ trk->saveToStream(out); },
                              data, error)
                || !transaction.addFile(activeRouteRoot() + "/" + Game::trkName + ".trk",
                                        data, &error))
            return false;
    }

    if(!transaction.commit(&error))
        return false;

    if(saveTrk)
        trk->setModified(false);
    qDebug() << "Route key files saved atomically";
    return true;
}

void Route::createNewPaths() {
    if (!Game::writeEnabled) return;
    Path::CreatePaths(this->trackDB);
}

QMap<QString, Coords*> Route::getMkrList(){
    return this->mkrList;
}

void Route::nextDefaultEnd(){
    this->trackDB->nextDefaultEnd();
    this->roadDB->nextDefaultEnd();
}

void Route::flipObject(WorldObj *obj){
    if(obj == NULL)
        return;
    if(obj->typeObj != WorldObj::worldobj)
        return;
    if(obj->typeID == obj->trackobj || obj->typeID == obj->dyntrack ){
        nextDefaultEnd();
        newPositionTDB(obj);                
    } else {
        obj->flip();
    }
                
}

void Route::paintHeightMap(Brush* brush, int x, int z, float* p){
    Game::ignoreLoadLimits = true;
    QSet<Terrain*> modifiedTiles = Game::terrainLib->paintHeightMap(brush, x, z, p);
    Tile *ttile;

    QSet<int> tileIds;
    foreach (Terrain *value, modifiedTiles){
        value->getWTileIds(tileIds);
    }
    foreach (int value, tileIds){
        ttile = tile[value];
        if(ttile != NULL){
            ttile->updateTerrainObjects();
        }
    }
}

void Route::createNew() {
    if (!Game::writeEnabled) return;

    QString path;

    path = Game::root + "/routes/" + Game::route;
    if (QDir(path).exists()) {
        if(Game::debugOutput) qDebug() << "route folder exist - aborting";
        return;
    }
    QDir().mkdir(path);
    QDir().mkdir(path + "/envfiles");
    QDir().mkdir(path + "/envfiles/textures");
    QDir().mkdir(path + "/paths");
    QDir().mkdir(path + "/shapes");
    QDir().mkdir(path + "/sound");
    QDir().mkdir(path + "/textures");
    QDir().mkdir(path + "/terrtex");
    QDir().mkdir(path + "/tiles");
    QDir().mkdir(path + "/td");
    QDir().mkdir(path + "/world");

    int x = Game::newRouteX;
    int z = Game::newRouteZ;
    
    Trk * newTrk = new Trk();
    newTrk->idName = Game::route;
    newTrk->routeName = Game::route;
    newTrk->displayName = Game::route;
    newTrk->startTileX = Game::newRouteX;
    newTrk->startTileZ = Game::newRouteZ;
    showTrkEditr(newTrk);
    newTrk->save();
    
    TDB::saveEmpty(false);
    TDB::saveEmpty(true);
    Game::terrainLib->createNewRouteTerrain(x, z);
    Tile::saveEmpty(x, z);
    //Terrain::saveEmpty(x, z);

    QString templateDir = "templateroute_0.6/";
    QString res = QString("tsre_assets/templateroute_0.6/");//+templateDir;
    path += "/";

    QFile::copy(res + "sigcfg.dat", path + "sigcfg.dat");
    QFile::copy(res + "sigscr.dat", path + "sigscr.dat");
    QFile::copy(res + "ttype.dat", path + "ttype.dat");
    QFile::copy(res + "template.ref", path + Game::route + ".ref");
    QFile::copy(res + "carspawn.dat", path + "carspawn.dat");
    QFile::copy(res + "deer.haz", path + "deer.haz");
    QFile::copy(res + "forests.dat", path + "forests.dat");
    QFile::copy(res + "speedpost.dat", path + "speedpost.dat");
    QFile::copy(res + "spotter.haz", path + "spotter.haz");
    QFile::copy(res + "ssource.dat", path + "ssource.dat");
    QFile::copy(res + "telepole.dat", path + "telepole.dat");

    FileFunctions::copyFiles(res + "envfiles", path + "envfiles");
    FileFunctions::copyFiles(res + "envfiles/textures", path + "envfiles/textures");
    FileFunctions::copyFiles(res + "shapes", path + "shapes");
    FileFunctions::copyFiles(res + "sound", path + "sound");
    FileFunctions::copyFiles(res + "terrtex", path + "terrtex");
    FileFunctions::copyFiles(res + "textures", path + "textures");
    
    Texture *graphicTexture = new Texture(200,150,24);
    AceLib::save(path + "graphic.ace", graphicTexture);
}

void Route::reloadTile(int x, int z) {
    tile[x * 10000 + z] = new Tile(x, z);
    return;
}

void Route::reloadLoadedWorldObjects() {
    foreach (Tile* tTile, tile) {
        if (tTile == NULL || tTile->loaded != 1)
            continue;

        for (int i = 0; i < tTile->jestObiektow; i++) {
            WorldObj *obj = tTile->obiekty[i];
            if (obj == NULL || !obj->loaded)
                continue;
            if (obj->typeID != WorldObj::transfer)
                continue;
            obj->reload();
        }
    }
}

int Route::newTile(int x, int z, bool forced) {
    if (!Game::writeEnabled) return 0;
    
    if (tile[x*10000 + z] == NULL)
        tile[x*10000 + z] = new Tile(x, z);
    
    if(!forced)
        if (tile[x*10000 + z]->loaded == 1)
            return 1;
            
    Tile::saveEmpty(x, -z);
    //Terrain::saveEmpty(x, -z);
    Game::terrainLib->saveEmpty(x, -z);
    Game::terrainLib->reload(x, z);
    reloadTile(x, z);

    if(Game::autoGeoTerrain){
        float pos[3];
        Vec3::set(pos, 0, 0, 0);
        Game::terrainLib->setHeightFromGeo(x, z, (float*)&pos);
    }
    
    return 2;
}

void Route::showTrkEditr(Trk * val){
    TrkWindow trkWindow;
    if(val == NULL)
        val = this->trk;
    trkWindow.trk = val;
    trkWindow.exec();
}

bool Route::prepareHealthReportData(int &worldFileCount,
                                    QStringList &unloadedWorldFiles,
                                    QString &error) {
    const QString worldPath =
            QDir::cleanPath(Game::root + "/routes/" + Game::route + "/world");
    QDir worldDir(worldPath);
    worldDir.setFilter(QDir::Files);
    worldDir.setNameFilters(QStringList() << "*.w");
    if(!worldDir.exists()){
        error = QObject::tr("The route world folder could not be found:\n%1")
                .arg(QDir::toNativeSeparators(worldPath));
        return false;
    }

    const QStringList worldFiles = worldDir.entryList();
    worldFileCount = worldFiles.size();
    unloadedWorldFiles.clear();

    // A report must not run the editor's optional auto-repair while it loads
    // every world tile for inspection.
    const bool originalAutoFix = Game::autoFix;
    Game::autoFix = false;
    preloadWFiles(true);
    Game::autoFix = originalAutoFix;

    for(const QString& worldFile : worldFiles){
        bool xOk = false;
        bool zOk = false;
        if(worldFile.length() != 17){
            unloadedWorldFiles.push_back(worldFile + " (unexpected filename)");
            continue;
        }

        const int worldX = worldFile.mid(1, 7).toInt(&xOk);
        const int worldZ = -worldFile.mid(8, 7).toInt(&zOk);
        const Tile *worldTile = (xOk && zOk)
                ? tile.value(worldX * 10000 + worldZ, NULL)
                : NULL;
        if(worldTile == NULL || worldTile->loaded != 1)
            unloadedWorldFiles.push_back(worldFile);
    }
    return true;
}

void Route::collectHealthReportData(QStringList &usedObjects,
                                    QStringList &usedTrack,
                                    QStringList &staticFlags,
                                    QStringList &uidIssues,
                                    QStringList &trackSectionIssues) const {
    for(auto tileIt = tile.constBegin(); tileIt != tile.constEnd(); ++tileIt){
        const Tile *routeTile = tileIt.value();
        if(routeTile == NULL)
            continue;

        const QString worldFileName =
                "w" + Tile::getNameXY(routeTile->x)
                + Tile::getNameXY(-routeTile->z) + ".w";
        QHash<unsigned int, int> worldUidCounts;
        QHash<unsigned int, int> soundUidCounts;

        for(auto objectIt = routeTile->obiekty.cbegin();
                objectIt != routeTile->obiekty.cend(); ++objectIt){
            WorldObj *object = objectIt->second;
            if(object == NULL || !object->loaded)
                continue;
            // Tr_Watermark is world-file metadata. Its UID is intentionally
            // zero and must not be diagnosed as a route object UID failure.
            if(dynamic_cast<TrWatermarkObj*>(object) != NULL)
                continue;

            QHash<unsigned int, int> &uidCounts =
                    object->isSoundItem() ? soundUidCounts : worldUidCounts;
            uidCounts[object->UiD] = uidCounts.value(object->UiD, 0) + 1;

            QString fileName = object->fileName;
            fileName.replace("\\", "/");
            const int shapesIndex =
                    fileName.indexOf("/shapes/", 0, Qt::CaseInsensitive);
            if(shapesIndex >= 0)
                fileName = fileName.mid(shapesIndex + 8);
            else if(fileName.startsWith("shapes/", Qt::CaseInsensitive))
                fileName = fileName.mid(7);

            if(!fileName.trimmed().isEmpty()){
                if(object->type.compare("trackobj", Qt::CaseInsensitive) == 0)
                    usedTrack.push_back(fileName.toLower());
                else
                    usedObjects.push_back(fileName.toLower());
            }

            staticFlags.push_back(
                    QString("0x%1 | %2 | %3")
                    .arg(QString::number(object->staticFlags, 16).toUpper())
                    .arg(ParserX::MakeFlagsString(object->staticFlags))
                    .arg(object->type));

            if((object->typeID == WorldObj::trackobj
                    || object->typeID == WorldObj::dyntrack)
                    && object->sectionIdx >= 0
                    && (tsection == NULL
                        || tsection->shape.find(object->sectionIdx)
                           == tsection->shape.end()
                        || tsection->shape.at(object->sectionIdx) == NULL)){
                trackSectionIssues.push_back(
                        QString("%1 | %2 | %3 | %4 | %5")
                        .arg(worldFileName, -17)
                        .arg("Missing TrackShape entry in active tsection.dat", -48)
                        .arg(object->UiD, 8)
                        .arg(object->sectionIdx, 10)
                        .arg(fileName.isEmpty() ? object->type : fileName));
            }
        }

        const auto appendDuplicateUids =
                [&uidIssues, &worldFileName](
                    const QHash<unsigned int, int> &uidCounts,
                    const QString &uidType){
            for(auto uidIt = uidCounts.constBegin();
                    uidIt != uidCounts.constEnd(); ++uidIt){
                if(uidIt.value() > 1){
                    uidIssues.push_back(
                            QString("%1 | duplicate %2 UID %3 | %4 objects")
                            .arg(worldFileName)
                            .arg(uidType)
                            .arg(uidIt.key())
                            .arg(uidIt.value()));
                }
            }
        };
        appendDuplicateUids(worldUidCounts, "world");
        appendDuplicateUids(soundUidCounts, "sound");
    }
}


void Route::confirmMerge() {

    RouteMergeDialog dialog;
    int result = dialog.exec(); // Shows the dialog, waits for user interaction
    qDebug() << "Dialog result: " << result;
    if (result == QDialog::Accepted) {
        qDebug() << "Merge Started" ;
        
        QStringList args = Game::routeMergeString.split(":");
        if(args.size() == 4)
            {
             // execute with offset
                float offsetX = args[1].toFloat();
                float offsetY = args[2].toFloat();
                float offsetZ = args[3].toFloat();
                mergeRoute(args[0], offsetX, offsetY, offsetZ);
            }
        else  // execute without offset
                mergeRoute(args[0], 0,0,0);
 
 
            setAsCurrentGameRoute();

        } 
    else
        qDebug() << "Merge Canceled" ;

}

/// EFO hail mary, stealing from ToggleToTDB and making it do both a delete and an add
void Route::RebuildTDB(){
    QProgressDialog *progress = NULL;
    bool gui = true;
    int maxuid = 0;
    
    qDebug() << "TDB gap tolerance: " << Game::trackGap ;   
        
    if(gui){
        progress = new QProgressDialog("Rebuilding TDB ...", "", 0, this->tile.size());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(NULL);
        progress->setWindowFlags(Qt::CustomizeWindowHint);
        progress->show();
    }
    int pi = 0;    
    qDebug() << "SectionIDX Max " << tsection->tsectionMaxIdx;
    qDebug() << "Read Tiles for TDB Rebuild";
    foreach (Tile* tTile, this->tile){

        if(progress != NULL){
            progress->setValue((++pi));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        
        if (tTile == NULL) 
            continue;

        
        for(int bi = 0; bi < tTile->jestObiektow; bi++){            
            WorldObj *wObj = tTile->obiekty[bi];
                       
            if(wObj == NULL) continue;
            //if(wObj->isTrackItem()) continue;
            
            /// Can't handle Dynatrax automatically
//            if((wObj->sectionIdx >= tsection->tsectionMaxIdx) && (wObj->type == "trackobj"))
//            {
//               qDebug() << "SectionIDX " << wObj->sectionIdx << " greater than MaxIDS " << tsection->tsectionMaxIdx;
//               continue;                
//            }

            maxuid++;
            wObj->UiD = maxuid;
            wObj->setModified();
            
            if((Game::routeRebuildTDB == true) && (Game::UnsafeMode == true))
            {                
                if(wObj->type == "trackobj" || wObj->type == "dyntrack"){
                    
                                        
                    if( (std::isnan(wObj->position[0])) || (std::isnan(wObj->position[1])) || (std::isnan(wObj->position[2])))
                    {
                        qDebug() << "Tile object skipped due to missing position values " << bi << " " << wObj->fileName << " " << wObj->type;
                        continue;
                    }
                    
                    //qDebug() << "Tile object " << bi << " UID " << wObj->UiD << " " << wObj->fileName << " " << wObj->type << " " << "(next UID should be " << maxuid << ")";
                    try{
                    addToTDBIfNotExist(wObj);
                    } 
                    catch(...)
                        { qDebug() << "failed to add to TDB"; }
                   
                }
            }
        }
            
    }

}

void Route::confirmUnsafe() {
    UnsafeModeDialog dialog;
    int result = dialog.exec(); // Shows the dialog, waits for user interaction
    qDebug() << "Unsafe Mode Dialog result: " << result;
    if (result == QDialog::Accepted) {
        qDebug() << "Unsafe Mode Confirmed" ;
        } 
    else
        {
            Game::UnsafeMode = false;
            qDebug() << "Unsafe Mode Disabled" ;
        }
}


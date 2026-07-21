/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "Game.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QString>
#include <QEventLoop>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
//#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
//#include <QUrl>
//#include <QUrlQuery>
//#include "RouteEditorWindow.h"ad
//#include "LoadWindow.h"
//#include "CELoadWindow.h"
#include "SoundList.h"
#include "ShapeLib.h"
#include "EngLib.h"
#include <QtWidgets>
#include <QColor>
#include "TarFile.h"
#include "Renderer.h"
#include "RouteEditorWindow.h"
#include "StatusWindow.h"
#include "Camera.h"
#include "SettingsDialog.h"

//////////////////////////////////
//////// Version
//////////////////////////////////

QString Game::AppVersion = "v0.6";  // over-ride from main.cpp


bool Game::ServerMode = false;
QString Game::serverLogin = "";
QString Game::serverAuth = "";
RouteEditorClient *Game::serverClient = NULL;
GeoWorldCoordinateConverter *Game::GeoCoordConverter = NULL;
TDB *Game::trackDB = NULL;
TDB *Game::roadDB = NULL;    
SoundList *Game::soundList = NULL;    
TerrainLib *Game::terrainLib = NULL;   
bool Game::LocalTSectionOnly = false;
bool Game::UseWorkingDir = false;
QString Game::AppName = "TSRE GenX";

/// EFO New Setting for default startup option
QString Game::startapp = "r";

bool Game::showSDL = false;

QString Game::AppDataVersion = "0.697";
QString Game::root = "";
QString Game::route = "";
QString Game::routeName = "";
QString Game::trkName = "";
QString Game::season = "";
bool Game::loadActivities = true;
bool Game::loadConsists = true;
//QString Game::route = "traska";
//QString Game::route = "cmk";
QString Game::mainWindowLayout = "PWTC";
QString Game::ceWindowLayout = "C1";
QString Game::ActivityToPlay = "";
Renderer *Game::currentRenderer = NULL;
bool Game::playerMode = false;
bool Game::useNetworkEng = false;
bool Game::useQuadTree = true;
bool Game::useTdbEmptyItems = true;
int Game::allowObjLag = 1000;
int Game::maxObjLag = 10;
bool Game::ignoreLoadLimits = false;
int Game::startTileX = 0;
int Game::startTileY = 0;
float Game::objectLod = 3000;
float Game::distantLod = 100000;
int Game::tileLod = 1;
int Game::start = 0;
bool Game::ignoreMissingGlobalShapes = false;
bool Game::deleteTrWatermarks = false;
bool Game::deleteViewDbSpheres = false;
bool Game::createNewRoutes = false;
bool Game::writeEnabled = false;
bool Game::writeTDB = false;
bool Game::systemTheme = false;
bool Game::toolsHidden = false;
bool Game::usenNumPad = false;
float Game::cameraFov = 55.0f;
float Game::cameraSpeedMin = 1.0;
float Game::cameraSpeedStd = 3.0;
float Game::cameraSpeedMax = 40.0;
float Game::mouseSpeed = 0.1;
bool Game::cameraStickToTerrain = false;
bool Game::mstsShadows = false;

bool Game::viewWorldGrid = true;
bool Game::viewTileGrid = true;
bool Game::viewTerrainGrid = false;
bool Game::viewTerrainShape = true;
bool Game::viewInteractives = true;
bool Game::viewForestRegions = true;
bool Game::viewTrackDbLines = true;
bool Game::viewTsectionLines = true;
bool Game::viewPointer3d = true;
bool Game::viewMarkers = false;
bool Game::viewSnapable = false;
bool Game::viewCompass = false;
bool Game::warningBox = false;
bool Game::leaveTrackShapeAfterDelete = false;
bool Game::renderTrItems = false;
int Game::newRouteX = -5000;
int Game::newRouteZ = 15000;

bool Game::consoleOutput = false;
bool Game::flexLogEnabled = false;
bool Game::flexLogCandidates = false;
QString Game::flexLogFile = "";
int Game::fpsLimit = 0;
bool Game::ortsEngEnable = true;
bool Game::sortTileObjects = true;
int Game::oglDefaultLineWidth = 1;
bool Game::showWorldObjPivotPoints = false;
int Game::shadowMapSize = 2048;
int Game::shadowLowMapSize = 1024;
int Game::shadowsEnabled = 1;
float Game::sunLightDirection[] = {-1.0,2.0,1.0};
int Game::textureQuality = 1;
float Game::snapableRadius = 20;
bool Game::snapableOnlyRot = false;
float Game::trackElevationMaxPm = 700.0;
bool Game::proceduralTracks = false;
bool Game::fullscreen = false;
bool Game::hudEnabled = false;
float Game::hudScale = 1.0;
float Game::uiScale = 1.20f;
bool Game::markerLines = false;

bool Game::loadAllWFiles = false;
bool Game::autoFix = false;
bool Game::gui = true;
bool Game::listFiles = false;
bool Game::objSelected = false;

QString Game::geoPath = "hgst";

//RouteEditorWindow* Game::window = NULL;
//LodWindow* Game::loadWindow = NULL;
ShapeLib *Game::currentShapeLib = NULL;
EngLib *Game::currentEngLib = NULL;
Route *Game::currentRoute = NULL;
GameObj *Game::currentSelectedGameObj = NULL;
QColor *Game::colorConView = NULL;
QColor *Game::colorShapeView = NULL;

QString Game::StyleMainLabel = "#770000";
QString Game::StyleGreenButton = "#55FF55";
QString Game::StyleRedButton = "#FF5555";
QString Game::StyleYellowButton = "#FFFF55";
QString Game::StyleGreenText = "#009900";
QString Game::StyleRedText = "#990000";

QString Game::imageMapsUrl;
int Game::mapImageResolution = 4096;

bool Game::autoNewTiles = false;
bool Game::autoGeoTerrain = false;

bool Game::useSuperelevation = false;

bool Game::soundEnabled = false;
bool Game::scoSoundEnabled = true;

int Game::AASamples = 0;
bool Game::AARemoveBorder = false;
float Game::PixelRatio = 1.0;

float Game::fogDensity = 0.7;
float Game::shadow1Res = 2000.0;
float Game::shadow1Bias = 0.0025;
float Game::shadow2Res = 4000.0;
float Game::shadow2Bias = 0.002;
//float fogColor[4]{0.5, 0.75, 1.0, 1.0};
float Game::fogColor[4] = {230.0/255.0,248.0/255,255.0/255.0, 1.0};
float Game::skyColor[4] = {230.0/255.0,248.0/255,255.0/255.0, 1.0};

int Game::DefaultElevationBox = 1;
float Game::DefaultMoveStep = 0.25;
bool Game::gradeOverlayEnabled = true;
unsigned int Game::gradeOverlayRevision = 1;
bool Game::gradeLockEnabled = false;
float Game::gradeLockedPercent = 0.0f;
bool Game::gradeAssistInitialized = false;
bool Game::gradeAssistEnabled = false;
bool Game::gradeAssistTargetReached = false;
float Game::gradeAssistCurrentPercent = 0.0f;
float Game::gradeAssistTargetPercent = 0.0f;
float Game::gradeAssistStepPercent = 0.25f;
float Game::gradeAssistNextPercent = 0.0f;

bool Game::seasonalEditing = false;
int Game::numRecentItems = 30;
bool Game::useOnlyPositiveQuaternions = false;

QStringList Game::objectsToRemove;
QString Game::routeMergeString;

// EFO added 
float Game::wireLineHeight = 3;

float Game::sectionLineHeight = 2.8;
float Game::terrainTools[] = {1,5,5,9,1,10};
int   Game::selectedWidth = 2;
int   Game::selectedTerrWidth = 2;
bool  Game::lockCamera = false;
QColor *Game::selectedColor = new QColor("#FF0000");       /// EFO default red
QColor *Game::selectedTerrColor = new QColor("#FF0000");   /// EFO default red
QColor *Game::wireLineColor = new QColor("#FFFF00");       /// EFO default yellow
QColor *Game::terrBrushColor = new QColor("#000000");   // Default black

QString Game::mainPos;   /// EFO Null handling exists
QString Game::statusPos;  /// EFO Null handling exists
bool Game::restoreLastSessionWindowGeometry = false;
int Game::restoreMainX = 0;
int Game::restoreMainY = 0;
int Game::restoreMainW = 0;
int Game::restoreMainH = 0;
bool Game::restoreMainMaximized = false;
bool Game::restoreStatusGeometry = false;
int Game::restoreStatusX = 0;
int Game::restoreStatusY = 0;
int Game::restoreStatusW = 0;
int Game::restoreStatusH = 0;
bool Game::restoreLastSessionCamera = false;
int Game::restoreCameraTileX = 0;
int Game::restoreCameraTileZ = 0;
float Game::restoreCameraX = 0;
float Game::restoreCameraY = 0;
float Game::restoreCameraZ = 0;
float Game::restoreCameraRotX = 0;
float Game::restoreCameraRotY = 0;

bool  Game::debugOutput = false;
bool  Game::legacySupport = false; 
bool  Game::newSymbols = true;
int   Game::pointerIn = 4;
int   Game::pointerOut = 3;
int   Game::pyramid = 5;
int   Game::maxAutoPlacement = 999;
int   Game::imageMapsZoomOffset = 0;
float Game::railProfile[] = {0.7175, 0.7895};
bool Game::flexDebugWindow = false;

float Game::convertDistance = 1;  /// EFO will set to feet = 3.28084 if useImperial is set to true;
QString Game::convertUnitD = " m";
float Game::convertMass = 1;  /// EFO will set to pounds = 2.20462 if useImperial is set to true;
QString Game::convertUnitM = " t";
float Game::convertSpeed = 1;  /// EFO will set to mph = 0.621371 if useImperial is set to true;
QString Game::convertUnitS = " kmph";
float  Game::deepUnderground = -100;
bool Game::viewTRLabels = false;
float Game::trackGap = 0.19;   /// EFO this is the gap allowable for auto-joining tracks and vectors

int   Game::markerHeight = 30;
int   Game::markerText = 16;
float Game::lastElev = 0.0;
float Game::sigOffset = 0;
QStringList Game::markerFiles;
bool Game::reload;
QString Game::MapAPIKey = "";
bool Game::imageSubstitution = true;
bool Game::imageUpgrade = true;
QString Game::includeFolder = "openrails";

int Game::logfileMax = 99999;
int Game::logfileDays = 99999;

QStringList Game::preloadTextures;

bool Game::resetTools = false;

QString Game::appDataDir(){
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if(base.isEmpty())
        base = QDir::homePath()+"/AppData/Local";

    QString path = base + "/TSRE";
    QDir().mkpath(path);
    return path;
}

QString Game::sessionSplashImagePath(){
    static QString sessionSplashPath;
    if(!sessionSplashPath.isEmpty())
        return sessionSplashPath;

    const QString splashDirPath = QString("tsre_appdata/") + AppDataVersion;
    QDir splashDir(splashDirPath);
    const QStringList nameFilters = QStringList()
            << "Splash_*.png"
            << "Splash_*.jpg"
            << "Splash_*.jpeg"
            << "Splash_*.bmp"
            << "Splash_*.webp";
    const QStringList splashFiles = splashDir.entryList(
            nameFilters, QDir::Files | QDir::Readable, QDir::Name);

    if(splashFiles.isEmpty()){
        qWarning() << "No splash image matching Splash_* in" << splashDirPath;
        return QString();
    }

    const auto shuffled = [](QStringList values){
        for(int i = values.size() - 1; i > 0; --i){
            const int j = QRandomGenerator::global()->bounded(i + 1);
            qSwap(values[i], values[j]);
        }
        return values;
    };

    QStringList remaining;
    const QString cycleStatePath = appDataDir() + "/splash-cycle.json";
    QFile cycleStateFile(cycleStatePath);
    if(cycleStateFile.open(QIODevice::ReadOnly)){
        const QJsonObject state = QJsonDocument::fromJson(cycleStateFile.readAll()).object();
        cycleStateFile.close();
        QStringList storedFiles;
        for(const QJsonValue &value : state.value("files").toArray())
            storedFiles.append(value.toString());
        for(const QJsonValue &value : state.value("remaining").toArray())
            remaining.append(value.toString());

        QStringList sortedStoredFiles = storedFiles;
        sortedStoredFiles.sort();
        QStringList sortedSplashFiles = splashFiles;
        sortedSplashFiles.sort();
        if(sortedStoredFiles != sortedSplashFiles)
            remaining.clear();
        else {
            QStringList sortedRemaining = remaining;
            sortedRemaining.sort();
            sortedRemaining.removeDuplicates();
            for(const QString &fileName : sortedRemaining){
                if(!splashFiles.contains(fileName)){
                    remaining.clear();
                    break;
                }
            }
        }
    }

    if(remaining.isEmpty())
        remaining = shuffled(splashFiles);

    const QString selectedFile = remaining.takeFirst();
    if(remaining.isEmpty()){
        remaining = shuffled(splashFiles);
        if(remaining.size() > 1 && remaining.first() == selectedFile){
            const int swapIndex = 1 + QRandomGenerator::global()->bounded(remaining.size() - 1);
            qSwap(remaining[0], remaining[swapIndex]);
        }
    }

    QJsonArray filesJson;
    for(const QString &fileName : splashFiles)
        filesJson.append(fileName);
    QJsonArray remainingJson;
    for(const QString &fileName : remaining)
        remainingJson.append(fileName);
    QJsonObject state;
    state.insert("files", filesJson);
    state.insert("remaining", remainingJson);

    QSaveFile savedCycleState(cycleStatePath);
    if(savedCycleState.open(QIODevice::WriteOnly)){
        savedCycleState.write(QJsonDocument(state).toJson(QJsonDocument::Compact));
        if(!savedCycleState.commit())
            qWarning() << "Unable to save splash cycle state to" << cycleStatePath
                       << savedCycleState.errorString();
    } else {
        qWarning() << "Unable to open splash cycle state" << cycleStatePath
                   << savedCycleState.errorString();
    }

    sessionSplashPath = splashDir.filePath(selectedFile);
    qDebug() << "Using session splash image" << sessionSplashPath;
    return sessionSplashPath;
}

QString Game::routeAppDataDir(){
    QString cleanRoute = route.trimmed();
    if(cleanRoute.isEmpty())
        cleanRoute = "unknown_route";
    cleanRoute.replace(QRegExp("[^A-Za-z0-9_\\-]+"), "_");
    cleanRoute = cleanRoute.left(80);

    QString rootHash = QString(QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Md5).toHex().left(8));
    QString path = appDataDir()+"/routes/"+cleanRoute+"_"+rootHash;
    QDir().mkpath(path);
    return path;
}

QString Game::lastSessionFilePath(){
    return appDataDir()+"/lastSession.json";
}

QString Game::terrainPaintPresetFilePath(){
    return routeAppDataDir()+"/tsre_terrain_paint_presets.json";
}

bool Game::CheckBraces = false;
bool Game::UnsafeMode = false;
bool Game::extendedDebug = false;
bool Game::routeMergeTerrain = false;
bool Game::routeMergeTDB = false;
bool Game::routeMergeTerrtex = false;
bool Game::routeRebuildTDB = false;

int Game::rnp = 7;  //// can be 8

QHash<QString, int> Game::TextureFlags {
        {"none", 0x0},
        {"snow", 0x1},
        {"snowtrack", 0x2},
        {"spring", 0x4},
        {"autumn", 0x8},
        {"winter", 0x10},
        {"springsnow", 0x20},
        {"autumnsnow", 0x40},
        {"wintersnow", 0x80},
        {"night", 0x100},
        {"underground", 0x40000000}
    };

    
QStringList getFilesInDirectory(const QString& directoryPath) {
    QDir directory(directoryPath);

    // Filter for files only (exclude directories)
    QStringList filters;
    filters << "tsre-log-*.txt"; // filter

    // Apply filters and sorting
    directory.setSorting(QDir::Time);
    
    // Get the list of files
    QStringList files = directory.entryList(filters);
            
    // Remove "." and ".." entries
    files.removeOne(".");
    files.removeOne("..");     
    return files;
}    
    
void Game::InitAssets() {
    QString path;
    bool newInstallation = false;
    path = "./tsre_assets/";
    QFile appFile3(path);
    if (!appFile3.exists()){
        QDir().mkdir(path);
    }
    
    path = "./tsre_appdata/";
    
    QFile appFile1(path);
    if (!appFile1.exists()){
        newInstallation = true;
        QMessageBox msgBox;
        msgBox.setWindowTitle("TSRE");
        msgBox.setText("Welcome in TSRE!\n\nThis is experimental version.\nUsing it may seriously damage your data."
                       "\nMake backup first!\n\n\nNow TSRE will download app data.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        QDir().mkdir(path);
    }
    
    path += Game::AppDataVersion;
    
    QFile appFile2(path);
    if (!appFile2.exists()){
        qDebug() << "no appdata";
        DownloadAppData(path);
    }
    if (!appFile2.exists()){
        qDebug() << "appdata failed to load";
        return;
    }
}

void Game::load() {
    
    QString sh;
    QString path;
    
    path = "settings.txt";
    QFile file(path);
    
    if (!file.exists()){
        qDebug() << "creating new settings file";
        ////  CreateNewSettingsFile();   /// EFO replacing this with SettingsDialog's new create 
        SettingsDialog* diag = new SettingsDialog(nullptr); // Create the object
        diag->loadSettings();                               // Populate the maps from current settings.txt
        diag->save(path);                                   // Execute the save to your new path
        delete diag;                                        // Clean up memory
    }
    
    if (!file.open(QIODevice::ReadOnly)){
        qDebug() << "settings file fails to open";
        return;
    }
    
    if(Game::debugOutput) qDebug() << path;

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString line;
    QStringList args;
    QString setval;
    QString setname; 
    QString skipped;   // save what's being skipped for output later

    qDebug() << "Debug: " << Game::debugOutput;
    
    if(Game::reload) qDebug() << "Limited Reload In Progress";
    
    while (!in.atEnd()) {
        line = in.readLine();
        
       //qDebug() << line;
       
        // EFO  Partial comment stripper if "//" is found after arguments
        if (line.indexOf("//") >= 0 && line.indexOf("://") <= 0) {
            skipped = line.mid(line.indexOf("//"),99);
            line = line.left(line.indexOf("//"));
        }
       
       // EFO Main comment stripper
       if(line.startsWith("#", Qt::CaseInsensitive)) {  if(Game::debugOutput) qDebug() << "Skip#   : " << skipped;  ; continue;}
       if(line.startsWith("//", Qt::CaseInsensitive)){  if(Game::debugOutput) qDebug() << "Skip  //: " << skipped; ; continue;}
        
        //args = line.split("=");
        args.clear();
        args.push_back(line.section('=', 0, 0));
        args.push_back(line.section('=', 1));
        
        //if(Game::debugOutput) qDebug() << args[0] << args[1];
        
        
        
        if(args.count() < 2) continue;
        setname = args[0].trimmed().toLower();       
        setval = args[1].trimmed().toLower();
        setval = setval.replace("\"","");
        setval = setval.replace(";","");

        
        if(setname.length()==0) continue;
        
        if(Game::debugOutput) qDebug() << "Args    : " << args[0].trimmed() << " "<< args[1].trimmed();
        if(Game::debugOutput) qDebug() << "Sets    : " << setname << "=" << setval;        

/*
 
 
 *      When evaluating "setname" always be sure to check against lowercase 
 *          Tokens get set to lowercase and don't care if mixed case was used in the settings.txt file
 
 
 
 */
        
        //// These are protected and should never be reloaded
        if(Game::reload == false)
        {
            if(setname =="gameroot") 
                root = setval;

            if(setname =="routename")
                route = setval;

            if(setname =="starttilex"){
                Game::start++;
                startTileX = setval.toInt();
            }
            if(setname =="starttiley"){
                Game::start++;
                startTileY = setval.toInt();
            }                        

            if(setname =="writeenabled"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    writeEnabled = true;
                else
                    writeEnabled = false;
            }
            if(setname =="writetdb"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    writeTDB = true;
                else
                    writeTDB = false;
            }

            if(setname =="tilelod"){
                tileLod = setval.toInt();
            }
            if(setname =="objectlod"){
                objectLod = setval.toInt();
            }
            if(setname =="maxobjlag"){
                maxObjLag = setval.toInt();
            }
            if(setname =="allowobjlag"){
                allowObjLag = setval.toInt();
            }
            if(setname =="fpslimit"){
                fpsLimit = setval.toInt();
            }
            if(setname =="camerafov"){
                cameraFov = setval.toFloat();
            }
            if(setname =="warningbox"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    warningBox = true;
                else
                    warningBox = false;
            }
            if(setname =="leavetrackshapeafterdelete"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    leaveTrackShapeAfterDelete = true;
                else
                    leaveTrackShapeAfterDelete = false;
            }

            // EFO configure yellow line display height
            if(setname =="wirelineheight"){
                wireLineHeight = setval.toFloat();
            }

            // EFO configure grey line display height
            if(setname =="sectionlineheight"){
                sectionLineHeight = setval.toFloat();
            }

            if(setname =="ogldefaultlinewidth"){
                oglDefaultLineWidth = setval.toInt();
            }        

            if(setname =="aasamples"){
                AASamples = setval.toInt();
            }

            if(setname =="proceduraltracks"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    proceduralTracks = true;
                else
                    proceduralTracks = false; 
            }
            if(setname =="fullscreen"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    fullscreen = true;
                else
                    fullscreen = false; 
            }

            if(setname =="usetdbemptyitems"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    useTdbEmptyItems = true;
                else
                    useTdbEmptyItems = false; 
            }
            if(setname =="useworkingdir"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    UseWorkingDir = true;
                else
                    UseWorkingDir = false; 
            }
            if(setname =="numrecentitems"){
                numRecentItems = setval.toInt();
            }

            if(setname =="loadallwfiles"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    loadAllWFiles = true;
                else
                    loadAllWFiles = false; 
            }
            if(setname =="autofix"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    autoFix = true;
                else
                    autoFix = false; 
            }
            if(setname =="useonlypositivequaternions"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    useOnlyPositiveQuaternions = true;
                else
                    useOnlyPositiveQuaternions = false; 
            }
            if(setname =="routemergestring")
                routeMergeString = args[1]; 
            
            
            if(setname =="serverlogin"){
                serverLogin = args[1].trimmed();
            }
            if(setname =="serverauth"){
                serverAuth = args[1].trimmed();
            }

            // EFO Configure WindowPos
            if(setname =="mainwindow") {
                mainPos = setval;
            }

            if(setname =="controlpanel" || setname =="statuswindow") {
                statusPos = setval;
            }            

            if(setname == "markerheight"){
                    markerHeight = setval.toInt();
            }       

            if(setname == "markertext"){
                    markerText = setval.toFloat();
            }       

            if(setname == "sigoffset"){
                sigOffset = setval.toFloat();
            }

            if(setname =="routemergeterrain"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    routeMergeTerrain = true;
                else
                    routeMergeTerrain = false; 
            }

            if(setname =="routemergetdb"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    routeMergeTDB = true;
                else
                    routeMergeTDB = false; 
            }

            if(setname =="routemergeterrtex"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    routeMergeTerrtex = true;
                else
                    routeMergeTerrtex = false; 
            }
            
            if(setname =="routerebuildtdb"){
                if((setval == "true") or (setval == "1") or (setval == "on"))
                    routeRebuildTDB = true;
                else
                    routeRebuildTDB = false; 
            }

   

            
            
        }
        
        //// These should be reload safe
        
        if(setname =="consoleoutput"){
            if((setval == "true") or (setval == "1") or (setval == "on")) 
                Game::consoleOutput = true;
            else
                Game::consoleOutput = false;
        }
        
        if(setname =="debugoutput"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                Game::debugOutput = true;
            else if(setval == "ext")
            {
                qSetMessagePattern("%{file}:%{function}:%{line}: \t%{message}");
                Game::debugOutput = true;
                qDebug() << "Expanded Debugging Enabled";
                
            }
            else
                Game::debugOutput = false;
        }        
        
        if(setname =="legacysupport"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                Game::legacySupport = true;
        }
        
        if(setname =="deletetrwatermarks"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                deleteTrWatermarks = true;
            else
                deleteTrWatermarks = false;
        }
        if(setname =="deleteviewdbspheres"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                deleteViewDbSpheres = true;
            else
                deleteViewDbSpheres = false;
        }
        if(setname =="createnewifnotexist"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                createNewRoutes = true;
            else
                createNewRoutes = false;
        }
        
        if(setname =="systemtheme"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                systemTheme = true;
            else
                systemTheme = false;
        }
        if(setname =="toolshidden"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                toolsHidden = true;
            else
                toolsHidden = false;
        }
        if(setname =="usennumpad"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                usenNumPad = true;
            else
                usenNumPad = false;
        }

        if(setname =="rendertritems"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                renderTrItems = true;
            else
                renderTrItems = false;
        }


        
        // EFO configure selectedTerrWidth
        if(setname =="selectedterrwidth"){
            selectedTerrWidth = setval.toInt();
        }

                // EFO configure selectedTerrWidth
        if(setname =="selectedwidth"){
            selectedWidth = setval.toInt();
        }

        
        if(setname =="selectedcolor"){
            selectedColor = new QColor(setval);
        }
        
        if(setname =="selectedterrcolor"){
            selectedTerrColor = new QColor(setval);
        }
        
        // EFO Configure Terrain Tools
        if(setname =="terrainsize") {
            terrainTools[0] = setval.toInt();
        }
        if(setname =="terrainembankment") {
            terrainTools[1] = setval.toInt();
        }
        if(setname =="terraincut") {
            terrainTools[2] = setval.toInt();
        }
        if(setname =="terrainradius") {
            terrainTools[3] = setval.toInt();
        }
        if(setname =="terrainbrushsize") {
            terrainTools[4] = setval.toInt();
        }
        if(setname =="terrainbrushintensity") {
            terrainTools[5] = setval.toInt();
        }
        if(setname=="terrainbrushcolor") {
            terrBrushColor = new QColor(setval);
        }
        
        
        // END EFO Configure Terrain Tools
        
        
        if(setname =="geopath")
            geoPath = setval;
        
        if(setname =="colorconview")
            colorConView = new QColor(setval);
        if(setname =="colorshapeview")
            colorShapeView = new QColor(setval);
        
        if(setname =="ortsengenable"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                ortsEngEnable = true;
            else
                ortsEngEnable = false;
        }
        
        
        if(setname =="sorttileobjects"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                sortTileObjects = true;
            else
                sortTileObjects = false;
        }
        
        if(setname =="ignoremissingglobalshapes"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                ignoreMissingGlobalShapes = true;
            else
                ignoreMissingGlobalShapes = false;
        }
        if(setname =="snapableonlyrot"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                snapableOnlyRot = true;
            else
                snapableOnlyRot = false; 
        }
                
        if(setname =="shadowsenabled"){
          if((setval == "true") or (setval == "1") or (setval == "on"))  
            shadowsEnabled = 1;
          else 
              shadowsEnabled = 0;
        }
          
        if(setname =="shadowmapsize"){
            shadowMapSize = setval.toInt();
            if(shadowMapSize == 8192){
                shadow1Res = 3000.0;
                shadow1Bias = 0.0004;
            }
            if(shadowMapSize == 4096){
                shadow1Res = 2500.0;
                shadow1Bias = 0.0007;
            }
        }
        if(setname =="shadowlowmapsize"){
            shadowLowMapSize = setval.toInt();
            if(shadowLowMapSize >= 2048){
                shadow2Res = 4000.0;
                shadow2Bias = 0.001;
            }
        }
        
        if(setname =="texturequality"){
            textureQuality = setval.toInt();
        }
        if(setname =="imagemapsurl"){
            imageMapsUrl = args[1].trimmed();
        }        
        
        if(setname =="imagemapszoomoffset"){
            imageMapsZoomOffset = setval.toInt();
        }
        
        
        
        if(setname =="mapimageresolution"){
            mapImageResolution = setval.toInt();
        }
 
        if(setname =="camerasticktoterrain"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                cameraStickToTerrain = true;
            else
                cameraStickToTerrain = false; 
        }

        if(setname =="soundenabled"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                soundEnabled = true;
            else
                soundEnabled = false; 
        }

        if(setname =="scosoundenabled"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                scoSoundEnabled = true;
            else
                scoSoundEnabled = false;
        }
        
        if(setname =="cameraspeedmin"){
            cameraSpeedMin = setval.toFloat();
        }
        if(setname =="cameraspeedstd"){
            cameraSpeedStd = setval.toFloat();
        }
        if(setname =="cameraspeedmax"){
            cameraSpeedMax = setval.toFloat();
        }
        if(setname =="mousespeed"){
            mouseSpeed = setval.toFloat();
        }
        if(setname =="trackelevationmaxpm"){
            trackElevationMaxPm = setval.toFloat();
        }
        if(setname =="mainwindowlayout"){
            mainWindowLayout = setval;
        }
        if(setname =="cewindowlayout"){
            ceWindowLayout = setval;
        }
        if(setname =="usequadtree"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                useQuadTree = true;
            else
                useQuadTree = false; 
        }
        if(setname =="playermode"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                playerMode = true;
            else
                playerMode = false; 
        }
        if(setname =="usenetworkeng"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                useNetworkEng = true;
            else
                useNetworkEng = false; 
        }
        if(setname =="fogdensity"){
            fogDensity = setval.toFloat();
        }
        if(setname =="fogcolor"){
            QColor tcolor(setval);
            fogColor[0] = tcolor.redF();
            fogColor[1] = tcolor.greenF();
            fogColor[2] = tcolor.blueF();
        }
        if(setname =="skycolor"){
            QColor tcolor(setval);
            skyColor[0] = tcolor.redF();
            skyColor[1] = tcolor.greenF();
            skyColor[2] = tcolor.blueF();
        }
        if(setname =="defaultelevationbox"){
            DefaultElevationBox = setval.toInt();
        }
        if(setname =="defaultmovestep"){
            DefaultMoveStep = setval.toFloat();
        }
        if(setname =="hudenabled"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                hudEnabled = true;
            else
                hudEnabled = false; 
        }
        if(setname =="hudscale"){
            hudScale = setval.toFloat();
        }
        if(setname =="uiscale"){
            uiScale = setval.toFloat();
            if(uiScale < 0.75f)
                uiScale = 0.75f;
            if(uiScale > 1.50f)
                uiScale = 1.50f;
        }

        if(setname =="season"){
            // Disabled for GenX: seasonal display is controlled live from the
            // F2 terrain texture selector so old settings files cannot override it.
        }
        
        if(setname =="startapp"){
            startapp = setval;
        }
/*
        if(setname =="markerlines"){
            if((setval == "true") or (setval == "1") or (setval == "on"))
                markerLines = true;
        }
  */      
        if(setname =="seasonalediting"){
            // Disabled for GenX. The old global seasonal-editing mode caused
            // terrain/shape confusion; seasonal preview now uses the F2 selector.
            seasonalEditing = false;
        }

        
        if(setname =="maxautoplacement") {
            maxAutoPlacement = setval.toInt();
        }

        if(setname =="lockcamera") {
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 lockCamera = true;
            else
                 lockCamera = false;
        }       

        if(setname =="cameralock") {
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 lockCamera = true;
            else
                 lockCamera = false;
        }       

        
        
        if(setname =="newsymbols") {
             if((setval == "true") or (setval == "1") or (setval == "on"))
             {
                 newSymbols = true;
                 pointerIn = 4;
                 pointerOut = 3;
                 pyramid = 5;
                 qDebug() << "Symbol = true";
             }
            else
             {   
                 newSymbols = false;
                 pointerIn = 2;
                 pointerOut = 2;
                 pyramid = 0;
                 qDebug() << "Symbol = false";
             }
        }       
        if(setname =="flexdebugwindow")
            flexDebugWindow = ((setval == "true") or (setval == "1") or (setval == "on"));
        if(setname =="railprofile")
        {             
                QStringList railList = setval.split(",");
                if(railList.length() >= 2){
                    railProfile[0] = railList[0].toDouble();
                    railProfile[1] = railList[1].toDouble();
                }
        }
        
        if(setname =="useimperial"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
             {            
                convertDistance = 3.28084;
                convertMass = 1.102;
                convertSpeed = 0.621371;
                convertUnitD = " ft";
                convertUnitM = " T";               
                convertUnitS = " mph";
             }                
        }

        if(setname == "deepunderground")
        {
            deepUnderground = setval.toInt();        
        }


        
        if(setname == "viewtrlabels"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 viewTRLabels = true;
            else
                 viewTRLabels = false;
        }       

        
        if(setname == "viewcompass"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 viewCompass = true;
            else
                 viewCompass = false;
        }       
/*
        if(setname == "viewmarkers"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 viewMarkers = true;
            else
                 viewMarkers = false;
        }       
*/
        if(setname == "listfiles"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 listFiles = true;
            else
                 listFiles = false;
        }       

        if(setname =="mapapikey"){
                MapAPIKey = args[1].trimmed();
                
        }

        if(setname =="includefolder"){
                includeFolder = args[1].trimmed();
        }
        
        if(setname =="logfiledays"){
                logfileDays = setval.toInt();
        }
        
        if(setname =="logfilemax"){
                logfileMax = setval.toInt();
        }
        

        
        if(setname == "imagesubstitution"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 imageSubstitution = true;
            else
                 imageSubstitution = false;
        }       
        
        if(setname == "imageupgrade"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 imageUpgrade = true;
            else
                 imageUpgrade = false;
        }       

        if(setname == "loadconsists"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 loadConsists = true;
            else
                 loadConsists = false;
        }

        if(setname == "loadactivities"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 loadActivities = true;
            else
                 loadActivities = false;
        }
        
        if(setname == "localtsectiononly"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 LocalTSectionOnly = true;
            else
                 LocalTSectionOnly = false;
        }
                       
        if(setname == "objectstoremove" ) 
            {
              // qDebug() << "Removal Found";
              objectsToRemove = setval.split(",") ;
              qDebug() << "Removal objects found: " << objectsToRemove.size();              
            }

        if(setname == "preloadtextures" ) 
            {
              // qDebug() << "Removal Found";
              preloadTextures = setval.split(",") ;
            }
                
        if(setname == "checkbraces" ) 
        {
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 CheckBraces = true;
            else
                 CheckBraces = false;
        }           
                    
        if(setname == "unsafemode")
        {
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 UnsafeMode = true;
            else
                 UnsafeMode = false;
        }           
        
        
        ///////// These are unpublished settings ///////////////

        if(setname == "extendeddebug"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 extendedDebug = true;
            else
                 extendedDebug = false;
        }       

        
        if(setname == "wfhuser"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 showSDL = true;
            else
                 showSDL = false;
        }       

        if(setname == "realnumberprecision"){
             rnp = setval.toInt();
        }       

        if(setname == "trackgap")
        {
            trackGap = setval.toFloat();        
        }

        
        if(Game::debugOutput) qDebug() << "Skip: " << skipped;
        skipped.clear();               
        
    // qDebug() << setname << " --> " << setval;        
    
        
    }
    
    qDebug() <<  Game::AppVersion ;

    if(maxObjLag < 2)
        maxObjLag = 2;
    if(allowObjLag > maxObjLag)
        allowObjLag = maxObjLag;
    if(allowObjLag < 0)
        allowObjLag = 0;
    
    cleanupLogs();

    Game::seasonalEditing = false;
}
/*
bool Game::loadRouteEditor(){
    
    window = new RouteEditorWindow();
    if(Game::fullscreen){
        window->setWindowFlags(Qt::CustomizeWindowHint);
        window->setWindowState(Qt::WindowMaximized);
    } else {
        window->resize(1280, 800);
    }
    
    loadWindow = new LoadWindow();
    QObject::connect(window, SIGNAL(exitNow()),
                      loadWindow, SLOT(exitNow()));
    
    QObject::connect(loadWindow, SIGNAL(showMainWindow()),
                      window, SLOT(show()));
    
    if(Game::checkRoot(Game::root) && (Game::checkRoute(Game::route) || Game::createNewRoutes)){
        Game::window->show();
    } else {
        Game::loadWindow->show();
    }
}

bool Game::loadConEditor(){
    //if(Game::warningBox){
    //    QMessageBox msgBox;
    //    msgBox.setText("This is experimental version.\nUsing it may seriously damage your consists."
    //                   "\nMake backup first!\n\nTo disable this window, set 'warningBox = false' in settings.txt.");
    //    msgBox.setIcon(QMessageBox::Warning);
    //    msgBox.exec();
    //}
    CELoadWindow* ceLoadWindow = new CELoadWindow();
    ceLoadWindow->show();
    //ConEditorWindow* cwindow = new ConEditorWindow();
    //cwindow->resize(1280, 720);
    //cwindow->show();
}
*/
bool Game::checkRoot(QString dir){
    QString path;
    path = dir + "/routes";
    path.replace("//", "/");
    QFile file(path);
    if (!file.exists()) {
        qDebug () << "/routes not exist: "<< dir <<  file.fileName();
        return false;
    }
    path = dir + "/global";
    path.replace("//", "/");
    file.setFileName(path);
    if (!file.exists()) {
        qDebug () << "/global not exist: "<< file.fileName();
        return false;
    }
    path = dir + "/global/tsection.dat";
    path.replace("//", "/");
    file.setFileName(path);
    if (!file.exists()) {
        qDebug () << "/global/tsection.dat not exist: "<< file.fileName();
        return false;
    }
    
    return true;
}

bool Game::checkCERoot(QString dir){
    QString path;
    path = dir + "/trains";
    path.replace("//", "/");
    QFile file(path);
    if (!file.exists()) {
        qDebug () << "/trains not exist: "<< file.fileName();
        return false;
    }
    path = dir + "/trains/trainset";
    path.replace("//", "/");
    file.setFileName(path);
    if (!file.exists()) {
        qDebug () << "/trains/trainset not exist: " << file.fileName();
        return false;
    }
    path = dir + "/trains/consists";
    path.replace("//", "/");
    file.setFileName(path);
    if (!file.exists()) {
        qDebug () << "/trains/consists not exist: " << file.fileName();
        return false;
    }
    
    return true;
}

bool Game::checkRoute(QString dir){
    QFile file;
    file.setFileName(Game::root+"/routes/"+dir+"/"+dir+".trk");
    if(file.exists()){
        Game::trkName = dir;
        return true;
    }
    QDir folder(Game::root+"/routes/"+dir+"/");
    folder.setNameFilters(QStringList() << "*.trk");
    folder.setFilter(QDir::Files);
    foreach(QString dirFile, folder.entryList()){
        Game::trkName = dirFile.split(".")[0];
        //qDebug() << Game::trkName;
        return true;
    }
    
    //qDebug() << file.fileName();
    //qDebug() << file.exists();
    return false;
}

bool Game::checkRemoteRoute(QString dir){
    QFile file;
    file.setFileName(Game::root+"/routes/"+dir);
    if(file.exists()){
        //Game::trkName = dir;
        return true;
    }
    return false;
}

template<class T>
void Game::check_coords(T&& x, T&& z, float* p) {
    if (p[0] >= 1024) {
        p[0] -= 2048;
        x++;
    }
    if (p[0] < -1024) {
        p[0] += 2048;
        x--;
    }
    
    if (p[2] >= 1024) {
        p[2] -= 2048;
        z++;
    }
    if (p[2] < -1024) {
        p[2] += 2048;
        z--;
    }
}
template void Game::check_coords(int& x, int& z, float* p);
template void Game::check_coords(float& x, float& z, float* p);

template<class T, class K>
void Game::check_coords(T&& x, T&& z, K&& px, K&& pz) {
    if (px >= 1024) {
        px -= 2048;
        x++;
    }
    if (px < -1024) {
        px += 2048;
        x--;
    }
    if (pz >= 1024) {
        pz -= 2048;
        z++;
    }
    if (pz < -1024) {
        pz += 2048;
        z--;
    }
}
template void Game::check_coords(int& x, int& z, int& px, int& pz);
template void Game::check_coords(int& x, int& z, float& px, float& pz);
template void Game::check_coords(float& x, float& z, float& px, float& pz);

void Game::CreateNewSettingsFile(){
    QFile file;
    QTextStream out;
    QString filepath;

    filepath = "./settings.txt";
    file.setFileName(filepath);
    //qDebug() << filepath;
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    out.setDevice(&file);

    
    out << "// TSRE " << Game::AppVersion << "\n";
    out << "// In this file, there are two comment marks in use -- # and // -- Both work the same  \n";    
    out << "\n";
    out << "\n " ; 
    out << "\n " ; 
    out << "//// System Tokens (affects all modules) \n " ; 
    out << "\n " ; 
    out << "consoleOutput = false          // displays log output in realtime in command window \n " ; 
    out << "debugOutput = false            // enables extended logging detail \n " ; 
    out << "fullscreen = false             // Prevents screen from being maximized \n " ; 
    out << "imageSubstitution = true       // allow for ACE or DDS to be shown if missing DDS or ACE \n " ; 
    out << "imageUpgrade = true            // show DDS if available, false uses shapefile defined texture only \n " ; 
    out << "logfiledays = 20               // delete files older than X days \n " ; 
    out << "logfilemax = 50000             // keep only X logs \n " ; 
    out << "//mainWindow = 100, 100          // X, Y of main windows and load window \n " ; 
    out << "mainWindowLayout = \"PWTC\"      // Order of windows: P = Properties, T = Tools, W = World, C = Control Panel \n " ; 
    out << "maxObjLag = 10  \n " ; 
    out << "mouseSpeed = 0.1  \n " ; 
    out << "shadowLowMapSize = 1024  \n " ; 
    out << "shadowMapSize = 2048  \n " ; 
    out << "shadowsEnabled = false          // affects performance if true  \n " ; 
    out << "soundEnabled = false          \n " ; 
    out << "scoSoundEnabled = true         // GenX editor UI sounds: click, buzz, chirp  \n " ;
    out << "startapp = r                    // r=Route Edit   c=Consist Edit   s=Shapeviewer    \n " ; 
    out << "systemTheme = false             // setting to true uses your Windows pallete  \n " ; 
    out << "unsafemode = false              // Only set to true for risky features  \n " ; 
    out << "useImperial = on                // Converts some display values from metric  \n " ; 
    out << "usenNumPad = true           \n " ; 
    out << "useWorkingDir = false           // false saves logs to the TSRE folder  \n " ; 
    out << "warningBox = true               // Warn before exiting without a save  \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor Startup Tokens  \n " ; 
    out << "// gameRoot = \"\"             // your ORTS Content drive/folder  \n " ; 
    out << "// routeName = \"\"            // add route name to skip route selection menu  \n " ; 
    out << "createNewIfNotExist = true       // Create routeName if not already present  \n " ; 
    out << "// startTileX = \"\"                  // optional start location  \n " ; 
    out << "// startTileY = \"\"                  // optional start location  \n " ; 
    out << "// geoPath = {path}                // Drive and folder housing HGT files  \n " ; 
    out << "loadActivities = true            // Check route activities for errors  \n " ; 
    out << "loadAllWFiles = false            // true loads the entire route for error checking; slower on large routes  \n " ; 
    out << "// routeMergeString = \"\"          // For merging a second route with offsets X Y Z applied  e.g. \"IRM:0:0:0\"  \n " ; 
    out << "// routeMergeTDB = false            // set to true to merge TDBs  \n " ; 
    out << "// routeMergeTerrain = false        // set to true to overwrite overlapping terrain heights  \n " ; 
    out << "// routeMergeTerrtex = false        // set to true to overwrite overlapping terrain textures  \n " ; 
    out << "#season = \"Spring\"               // Requires seasonalEditing, winter/autumn/spring/summer/night  \n " ; 
    out << "#seasonalEditing = on            //   \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor File Management Tokens  \n " ; 
    out << "autoFix = false                   // repair TDB anomalies  \n " ; 
    out << "deepunderground = -100           // flag pieces that aren't on the terrain  \n " ; 
    out << "deleteTrWatermarks = true        // removes detail not used by ORTS  \n " ; 
    out << "deleteViewDbSpheres = true       // removes detail not used by ORTS  \n " ; 
    out << "legacySupport = false            // enable retention of ViewDBSphere and VDBID when true   \n " ; 
    out << "listfiles = true                 // create lists of files used/unused on exit  \n " ; 
    out << "objectsToRemove = \"\"             // Requires listfiles and LoadAllWFiles, comma separated list of shapes  \n " ; 
    out << "routeRebuildTDB = false           // Requires unsafemode  \n " ; 
    out << "sortTileObjects = true           // Orders items by detail level on save   \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor Editing Preset Tokens  \n " ; 
    out << "defaultElevationBox = 0  \n " ; 
    out << "defaultMoveStep = 0.25  \n " ; 
    out << "ignoreMissingGlobalShapes = true  // false shows only track/road shapes present in Global\\Shapes  \n " ; 
    out << "leaveTrackShapeAfterDelete = false // Use only when deleting track/road but keeping TDB lines  \n " ; 
    out << "maxAutoPlacement = 999            // max distance in meters for auto-placement  \n " ; 
    out << "mapImageResolution = 4096         // Image resolution for downloaded map imagery  \n " ;
    out << "numRecentItems = 30               // Length of recently used items list  \n " ;
    out << "preloadTextures = \"rock.ace\"      // supports ace, bmp, dds, png files in TERRTEX folder  \n " ; 
    out << "sigOffset = 2.5                   // offset for signal object placement  \n " ; 
    out << "snapableRadius = 20               // max distance to snap to nearest object  \n " ; 
    out << "snapableOnlyRot = false           // false allows free rotation  \n " ; 
    out << "terrainBrushColor = \"#1B2E29\"  \n " ; 
    out << "terrainBrushIntensity = 70  \n " ; 
    out << "terrainBrushSize = 25  \n " ; 
    out << "terrainCut = 2  \n " ; 
    out << "terrainEmbankment = 2   \n " ; 
    out << "terrainRadius = 9   \n " ; 
    out << "terrainSize = 1   \n " ; 
    out << "textureQuality = 1  \n " ; 
    out << "trackElevationMaxPm = 100         // maximum grade permille   \n " ; 
    out << "useOnlyPositiveQuaternions = false   \n " ; 
    out << "useQuadTree = true  \n " ; 
    out << "useTdbEmptyItems = true           // Preserves node numbering when deleting items from TDB  \n " ; 
    out << "writeEnabled = true               // set to false for read-only  \n " ; 
    out << "writeTDB = true                   // set to false to manually decide what to add to TDB via Z key  \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor MapBox Tokens  \n " ; 
    out << "// imageMapsUrl = https://api.mapbox.com/styles/v1/mapbox/satellite-v9/static/{lon},{lat},{zoom}/{res}x{res}?access_token=  \n " ; 
    out << "// imageMapsZoomOffset = -1         /// required for MapBox  \n " ; 
    out << "// MapAPIKey = {your API key}  \n " ; 
    out << "\n " ; 
    out << "//// Route Editor Google Maps Tokens  \n " ; 
    out << "// imageMapsUrl = http://maps.googleapis.com/maps/api/staticmap?center={lat},{lon}&zoom={zoom}&size={res}x{res}&maptype=satellite&key=  \n " ; 
    out << "// MapAPIKey = {your API key}  \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor Viewport Tokens  \n " ; 
    out << "AASamples = 0  \n " ; 
    out << "allowObjLag = 10          \n " ; 
    out << "cameraFov = 35   \n " ; 
    out << "cameraSpeedMax = 40                // Camera movement with SHIFT  \n " ; 
    out << "cameraSpeedMin = 1                 // Camera movement with CTRL  \n " ; 
    out << "cameraSpeedStd = 3                 // Camera movement normal  \n " ; 
    out << "cameraStickToTerrain = true        // Stop camera from going underground, toggled with \"/\" key  \n " ; 
    out << "// fogColor =   \n " ; 
    out << "// fogDensity =  \n " ; 
    out << "hudEnabled = false                //  \n " ; 
    out << "hudScale = 1                      // HUD text scale  \n " ; 
    out << "uiScale = 1.20                    // Global editor UI font/panel scale; 1.00 to 1.25 recommended  \n " ; 
    out << "lockCamera = false                // same as hitting the . when moving about with the camera, can be unset  \n " ; 
    out << "markerHeight = 10                 // Height of the marker stick  \n " ; 
    out << "markerLines = true                // Show markers when route loads  \n " ; 
    out << "markerText = 2.5                  // Text size for marker text  \n " ; 
    out << "MSTSshadows = false               // dumb down shadows when true  \n " ; 
    out << "newSymbols = true                 // default is true, false uses the older TSRE pyramids   \n " ; 
    out << "objectLod = 3000                  // 2000 is plenty for most purposes   \n " ; 
    out << "oglDefaultLineWidth = 1           // width of standard lines  \n " ; 
    out << "railProfile = 0.7175, 0.7895      // rail edges for dynamic track  \n " ; 
    out << "flexDebugWindow = false           // show the old Flex solver debug window  \n " ; 
    out << "renderTrItems = false             // Show the black markers for TrItems  \n " ; 
    out << "sectionLineHeight = 5.0           // grey section line height   \n " ; 
    out << "selectedColor = #B612FF           // object selection line color  \n " ; 
    out << "selectedWidth = 2                 // object selection line width  \n " ; 
    out << "selectedTerrColor = #FFB612       // terrain selection line color  \n " ; 
    out << "selectedTerrWidth = 4             // terrain selection line width  \n " ; 
    out << "skyColor =  #E0FFFF  \n " ; 
    out << "controlPanel=10,400                // X, Y of Control Panel  \n " ; 
    out << "tileLod = 1                       // tiles in each direction to load  \n " ; 
    out << "toolsHidden = false               // only show the viewport  \n " ; 
    out << "useSuperelevation = false         // apply superelevation when rendering curves  \n " ; 
    out << "viewCompass = false               // show compass at top center  \n " ; 
    out << "viewMarkers = true                // view markers selected in Control Panel  \n " ; 
    out << "viewTRLabels == true              // view data labels for interactives  \n " ;
    
    out << "wireLineHeight = 6.8              // yellow TDB line height  \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Route Editor Multi-User Tokens  \n " ; 
    out << "# fpsLimit = 59  \n " ; 
    out << "# playerMode =   \n " ; 
    out << "# proceduralTracks = true        \n " ; 
    out << "# serverAuth = yes               \n " ; 
    out << "# serverLogin = yes@yes.com  \n " ; 
    out << "# useNetworkEng = false      \n " ; 
    out << "\n " ; 
    out << "\n " ; 
    out << "//// Consist Editor Tokens  \n " ; 
    out << "ceWindowLayout = \"cu1\"               // C - Consists, 1 = Main List,  2 = Second List,   \n " ; 
    out << "colorConView = #a2a2a2               \n " ; 
    out << "colorShapeView = #a2a2a2            \n " ; 
    out << "includeFolder = \"openrails\"          // Optional override for OpenRailsCZSK Project  \n " ; 
    out << "loadConsists = true                  // set to false to skip loading consists   \n " ; 
    out << "ortsEngEnable = true                 // Give precedence to settings in /OpenRails folders  \n " ; 
    out << "\n " ; 
           
    out.flush();
    file.close();
}

void Game::CheckForOpenAl(){
    
    QString openalfilename;
    QString Url;
    
    QFile file("./"+openalfilename);
    if(file.exists())
        return;
    
#ifdef Q_PROCESSOR_X86_64
    openalfilename = "OpenAL32.dll";
    Url = "http://koniec.org/tsre5/data/openal/Win64/soft_oal.dll";
#endif

#ifdef Q_PROCESSOR_X86_32
    openalfilename = "openal32.dll";
    Url = "http://koniec.org/tsre5/data/openal/Win32/soft_oal.dll";
#endif
    
    QNetworkAccessManager* mgr = new QNetworkAccessManager();
    //connect(mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(isData(QNetworkReply*)));
    qDebug() << "Wait ..";

    QNetworkRequest req;
    req.setUrl(QUrl(Url));
    qDebug() << req.url();
    QNetworkReply* r = mgr->get(req);
    QEventLoop loop;
    QObject::connect(r, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    qDebug() << "Network Reply Loop End";
    QByteArray data = r->readAll();

    file.open(QIODevice::WriteOnly);
    file.write(data);
    file.close();
}

void Game::DownloadAppData(QString path){
    QDir().mkdir(path);
    
    // Download and extract AppData
    QNetworkAccessManager* mgr = new QNetworkAccessManager();
    qDebug() << "Wait ..";
    QString Url = "http://koniec.org/tsre5/data/appdata/"+ Game::AppDataVersion + ".tar";
    qDebug() << Url;
    QNetworkRequest req;//(QUrl(Url));
    req.setUrl(QUrl(Url));
    qDebug() << req.url();
    QNetworkReply* r = mgr->get(req);
    QEventLoop loop;
    QObject::connect(r, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    
    qDebug() << "Network Reply Loop End";
    QByteArray data = r->readAll();
    FileBuffer *fileData = new FileBuffer((unsigned char*)data.data(), data.length());
    TarFile tarFile(fileData);
    tarFile.extractTo("./tsre_appdata/");
    
    // Create bat file for Consist Editor.
    QString conBatFile = QFileInfo(QCoreApplication::applicationFilePath()).fileName()+" --conedit";
    QFile file1("./ConsistEditor.bat");
    file1.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out;
    out.setDevice(&file1);
    out << conBatFile;
    out.flush();
    file1.close();
    
    // Create bat file for Shape Viewer.
    conBatFile = QFileInfo(QCoreApplication::applicationFilePath()).fileName()+" --shapeview";
    QFile file2("./ShapeViewer.bat");
    file2.open(QIODevice::WriteOnly | QIODevice::Text);
    out.setDevice(&file2);
    out << conBatFile;
    out.flush();
    file2.close();
}

void Game::cleanupLogs(){
    qDebug() << "";
    
    // Get the application directory
     QString appDir = QDir::currentPath();

     // Define the search pattern for log files
     QString pattern = "tsre-*.txt";

     // Set the maximum number of files to keep
     int maxFiles = Game::logfileMax ;
     int deletedFiles = 0;
     
     qDebug() << "Logfile Max Nbr:" << Game::logfileMax;
     qDebug() << "Logfile Max days:" << Game::logfileDays;
    
     // Create a QDir object for the application directory
     QDir dir(appDir);     
     QStringList fileList = getFilesInDirectory(appDir);
     
     qDebug() << "Directory files found:" << fileList.size();
     
     // Delete files older than 12 days, keeping only the newest ones
     QDateTime daysToKeep = QDateTime::currentDateTime().addDays(-Game::logfileDays);          
     qDebug() << "Last Date Kept:" << daysToKeep;
     
     for (int i = 0; i < fileList.size(); ++i) {
       const QString& filePath = dir.filePath(fileList[i]);
       QFileInfo fileInfo(filePath);
       if (fileInfo.isFile() && fileInfo.lastModified() < daysToKeep ) {
                QFile::remove(filePath);
                deletedFiles++;
                continue;
                }
                if(i > maxFiles)
                {
                    QFile::remove(filePath);
                    deletedFiles++;
                }
                            
        }
     qDebug() << "Logs Deleted:"  << deletedFiles;
     }  
      
       

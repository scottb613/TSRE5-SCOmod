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
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QScreen>
//#include <QCoreApplication>
//#include <QUrl>
//#include <QUrlQuery>
//#include "RouteEditorWindow.h"ad
//#include "LoadWindow.h"
#include "SoundList.h"
#include "ShapeLib.h"
#include "EngLib.h"
#include <QtWidgets>
#include <QColor>
#include "Renderer.h"
#include "RouteEditorWindow.h"
#include "StatusWindow.h"
#include "Camera.h"
#include "SettingsDialog.h"

//////////////////////////////////
//////// Version
//////////////////////////////////

QString Game::AppVersion = "v0.14";  // over-ride from main.cpp


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
QString Game::ceWindowLayout = "C1";
Renderer *Game::currentRenderer = NULL;
bool Game::useQuadTree = true;
bool Game::useTdbEmptyItems = true;
int Game::allowObjLag = 1000;
int Game::maxObjLag = 10;
bool Game::ignoreLoadLimits = false;
int Game::startTileX = 0;
int Game::startTileY = 0;
float Game::objectLod = 2000;
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
float Game::cameraSpeedMin = 0.25;
float Game::cameraSpeedStd = 3.0;
float Game::cameraSpeedMax = 35.0;
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
bool Game::viewCompass = true;
bool Game::warningBox = true;
bool Game::instanceProtection = false;
bool Game::leaveTrackShapeAfterDelete = false;
bool Game::renderTrItems = false;
int Game::newRouteX = -5000;
int Game::newRouteZ = 15000;

bool Game::consoleOutput = false;
bool Game::flexLogEnabled = false;
bool Game::flexLogCandidates = false;
QString Game::flexLogFile = "";
int Game::fpsLimit = 59;
bool Game::ortsEngEnable = true;
bool Game::sortTileObjects = false;
int Game::oglDefaultLineWidth = 1;
bool Game::showWorldObjPivotPoints = false;
int Game::shadowMapSize = 2048;
int Game::shadowLowMapSize = 1024;
int Game::shadowsEnabled = 0;
float Game::sunLightDirection[] = {-1.0,2.0,1.0};
int Game::textureQuality = 1;
float Game::snapableRadius = 20;
bool Game::snapableOnlyRot = false;
float Game::trackElevationMaxPm = 700.0;
bool Game::fullscreen = false;
float Game::uiScale = 1.00f;
bool Game::markerLines = false;

bool Game::loadAllWFiles = false;
bool Game::autoFix = false;
bool Game::gui = true;
bool Game::listFiles = false;
bool Game::objSelected = false;

QString Game::geoPath = "";

//RouteEditorWindow* Game::window = NULL;
//LodWindow* Game::loadWindow = NULL;
ShapeLib *Game::currentShapeLib = NULL;
EngLib *Game::currentEngLib = NULL;
Route *Game::currentRoute = NULL;
GameObj *Game::currentSelectedGameObj = NULL;
QColor *Game::colorConView = NULL;
QColor *Game::colorShapeView = NULL;

QString Game::StyleMainLabel = "#770000";
QString Game::StyleGreenButton = "#4b9b5d";
QString Game::StyleGreenButtonHover = "#65b778";
QString Game::StyleBlueButton = "#4788b5";
QString Game::StyleBlueButtonHover = "#61a2cf";
QString Game::StyleOrangeButton = "#b47a3b";
QString Game::StyleOrangeButtonHover = "#ce9454";
QString Game::StyleRedButton = "#a95050";
QString Game::StyleRedButtonHover = "#c46868";
QString Game::StyleYellowButton = "#b59b4c";
QString Game::StyleYellowButtonHover = "#cfb765";
QString Game::StyleGreenText = "#009900";
QString Game::StyleRedText = "#990000";

QString Game::imageMapsUrl;
int Game::mapImageResolution = 4096;

bool Game::autoNewTiles = false;
bool Game::autoGeoTerrain = false;

bool Game::useSuperelevation = false;

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
float Game::terrainConformTdbBias = 0.0f;
float Game::terrainConformRdbBias = 0.0f;
int   Game::selectedWidth = 2;
int   Game::selectedTerrWidth = 2;
bool  Game::lockCamera = false;
QColor *Game::selectedColor = new QColor("#B612FF");
QColor *Game::selectedTerrColor = new QColor("#FFB612");
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
bool  Game::newSymbols = false;
int   Game::pointerIn = 2;
int   Game::pointerOut = 2;
int   Game::pyramid = 0;
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

int   Game::markerHeight = 20;
int   Game::markerText = 5;
float Game::lastElev = 0.0;
float Game::sigOffset = 0;
QStringList Game::markerFiles;
QString Game::MapAPIKey = "";
QString Game::mapEngine = "";
QString Game::googleImageMapsUrl = "https://maps.googleapis.com/maps/api/staticmap?center={lat},{lon}&zoom={zoom}&size={res}x{res}&maptype=satellite&key=";
QString Game::googleMapAPIKey = "";
int Game::googleImageMapsZoomOffset = 0;
QString Game::mapboxImageMapsUrl = "https://api.mapbox.com/styles/v1/mapbox/satellite-v9/static/{lon},{lat},{zoom}/{res}x{res}?access_token=";
QString Game::mapboxMapAPIKey = "";
int Game::mapboxImageMapsZoomOffset = -1;
QString Game::customImageMapsUrl = "";
QString Game::customMapAPIKey = "";
int Game::customImageMapsZoomOffset = 0;
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

void Game::cleanupAppData(){
    QDir appDir(appDataDir());

    const auto pruneDatedFiles = [&appDir](const QRegularExpression &namePattern, int keepCount){
        const QStringList names = appDir.entryList(
                    QDir::Files | QDir::Readable, QDir::Time);
        int retained = 0;
        for(const QString &name : names){
            if(!namePattern.match(name).hasMatch())
                continue;
            if(retained++ < keepCount)
                continue;
            if(!QFile::remove(appDir.filePath(name)))
                qWarning() << "Unable to remove old AppData file" << appDir.filePath(name);
        }
    };

    // Settings backups are diagnostic rollback files, not permanent history.
    pruneDatedFiles(
        QRegularExpression(
            "^settings\\d{8}-\\d{6}(?:-\\d{3})?\\.json$",
            QRegularExpression::CaseInsensitiveOption), 5);
    pruneDatedFiles(
        QRegularExpression(
            "^settings\\.corrupt-\\d{8}-\\d{6}-\\d{3}\\.json$",
            QRegularExpression::CaseInsensitiveOption), 3);

    // Route-specific folders are useful only when they contain persisted
    // presets or other route state. Old code created them merely to derive the
    // atomic-save key, leaving an empty directory for every opened route.
    QDir routesDir(appDir.filePath("routes"));
    if(routesDir.exists()){
        const QStringList routeDirs = routesDir.entryList(
                    QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for(const QString &routeDirName : routeDirs){
            QDir routeDir(routesDir.filePath(routeDirName));
            if(routeDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()
                    && !routesDir.rmdir(routeDirName))
                qWarning() << "Unable to remove empty route AppData folder"
                           << routeDir.absolutePath();
        }
        if(routesDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())
            appDir.rmdir("routes");
    }
}

QString Game::settingsFilePath(){
    return appDataDir()+"/settings.json";
}

void Game::configureMapProvider(){
    QString selectedMapEngine = mapEngine.trimmed().toLower();
    if(selectedMapEngine.isEmpty()){
        // Migrate an existing single-provider configuration without making the
        // user re-enter a URL or API key.
        if(!customImageMapsUrl.trimmed().isEmpty()){
            if(customImageMapsUrl.contains("google", Qt::CaseInsensitive)){
                mapEngine = "Google";
                googleImageMapsUrl = customImageMapsUrl;
                googleMapAPIKey = customMapAPIKey;
                googleImageMapsZoomOffset = customImageMapsZoomOffset;
            } else if(customImageMapsUrl.contains("mapbox", Qt::CaseInsensitive)){
                mapEngine = "Mapbox";
                mapboxImageMapsUrl = customImageMapsUrl;
                mapboxMapAPIKey = customMapAPIKey;
                mapboxImageMapsZoomOffset = customImageMapsZoomOffset;
            } else {
                mapEngine = "Custom";
            }
            selectedMapEngine = mapEngine.toLower();
        }
    }

    if(selectedMapEngine == "google"){
        imageMapsUrl = googleImageMapsUrl;
        MapAPIKey = googleMapAPIKey;
        imageMapsZoomOffset = googleImageMapsZoomOffset;
        mapEngine = "Google";
    } else if(selectedMapEngine == "mapbox"){
        imageMapsUrl = mapboxImageMapsUrl;
        MapAPIKey = mapboxMapAPIKey;
        imageMapsZoomOffset = mapboxImageMapsZoomOffset;
        mapEngine = "Mapbox";
    } else if(selectedMapEngine == "custom"){
        imageMapsUrl = customImageMapsUrl;
        MapAPIKey = customMapAPIKey;
        imageMapsZoomOffset = customImageMapsZoomOffset;
        mapEngine = "Custom";
    } else {
        imageMapsUrl.clear();
        MapAPIKey.clear();
        imageMapsZoomOffset = 0;
        mapEngine = "None";
    }
}

bool Game::saveMapProviderSettings(){
    QFile existingFile(settingsFilePath());
    QJsonObject settings;
    if(existingFile.exists()){
        if(!existingFile.open(QIODevice::ReadOnly))
            return false;
        QJsonParseError parseError;
        const QJsonDocument existingDocument =
                QJsonDocument::fromJson(existingFile.readAll(), &parseError);
        existingFile.close();
        if(parseError.error != QJsonParseError::NoError || !existingDocument.isObject())
            return false;
        settings = existingDocument.object();
    }

    settings.insert("mapengine", mapEngine);
    settings.insert("mapImageResolution", mapImageResolution);
    settings.insert("googleImageMapsUrl", googleImageMapsUrl);
    settings.insert("googleMapAPIKey", googleMapAPIKey);
    settings.insert("googleImageMapsZoomOffset", googleImageMapsZoomOffset);
    settings.insert("mapboxImageMapsUrl", mapboxImageMapsUrl);
    settings.insert("mapboxMapAPIKey", mapboxMapAPIKey);
    settings.insert("mapboxImageMapsZoomOffset", mapboxImageMapsZoomOffset);
    settings.insert("imageMapsUrl", customImageMapsUrl);
    settings.insert("MapAPIKey", customMapAPIKey);
    settings.insert("imageMapsZoomOffset", customImageMapsZoomOffset);

    QSaveFile outputFile(settingsFilePath());
    if(!outputFile.open(QIODevice::WriteOnly))
        return false;
    outputFile.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
    return outputFile.commit();
}

bool Game::saveViewCompassState(){
    QFile existingFile(settingsFilePath());
    QJsonObject settings;
    if(existingFile.exists()){
        if(!existingFile.open(QIODevice::ReadOnly))
            return false;
        QJsonParseError parseError;
        const QJsonDocument existingDocument =
                QJsonDocument::fromJson(existingFile.readAll(), &parseError);
        existingFile.close();
        if(parseError.error != QJsonParseError::NoError || !existingDocument.isObject())
            return false;
        settings = existingDocument.object();
    }

    settings.insert("viewCompass", viewCompass);

    QSaveFile outputFile(settingsFilePath());
    if(!outputFile.open(QIODevice::WriteOnly))
        return false;
    outputFile.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
    return outputFile.commit();
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

QString Game::routeAppDataKey(){
    QString cleanRoute = route.trimmed();
    if(cleanRoute.isEmpty())
        cleanRoute = "unknown_route";
    cleanRoute.replace(QRegularExpression("[^A-Za-z0-9_\\-]+"), "_");
    cleanRoute = cleanRoute.left(80);

    const QString rootHash = QString(
                QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Md5)
                .toHex().left(8));
    return cleanRoute+"_"+rootHash;
}

QString Game::routeAppDataDir(){
    const QString path = appDataDir()+"/routes/"+routeAppDataKey();
    QDir().mkpath(path);
    return path;
}

QString Game::lastSessionFilePath(){
    return appDataDir()+"/lastSession.json";
}

QString Game::windowPinsFilePath(){
    return appDataDir()+"/windowPins.json";
}

static QJsonObject readWindowPins(){
    QFile file(Game::windowPinsFilePath());
    if(!file.open(QIODevice::ReadOnly))
        return QJsonObject();

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    return document.isObject() ? document.object() : QJsonObject();
}

static void writeWindowPins(const QJsonObject &pins){
    QSaveFile file(Game::windowPinsFilePath());
    if(!file.open(QIODevice::WriteOnly)){
        qWarning() << "Unable to open window pin state" << file.fileName()
                   << file.errorString();
        return;
    }
    file.write(QJsonDocument(pins).toJson(QJsonDocument::Indented));
    if(!file.commit())
        qWarning() << "Unable to save window pin state" << file.fileName()
                   << file.errorString();
}

bool Game::pinnedWindowPosition(const QString &windowName, QPoint *position){
    QRect geometry;
    if(!pinnedWindowGeometry(windowName, &geometry))
        return false;
    if(position != NULL)
        *position = geometry.topLeft();
    return true;
}

bool Game::pinnedWindowGeometry(const QString &windowName, QRect *geometry, bool *maximized){
    const QJsonObject entry = readWindowPins().value(windowName).toObject();
    if(!entry.value("pinned").toBool(false))
        return false;
    if(geometry != NULL){
        const int width = entry.value("w").toInt();
        const int height = entry.value("h").toInt();
        *geometry = QRect(entry.value("x").toInt(), entry.value("y").toInt(), width, height);
    }
    if(maximized != NULL)
        *maximized = entry.value("maximized").toBool(false);
    return true;
}

void Game::savePinnedWindowPosition(const QString &windowName, const QPoint &position){
    QJsonObject pins = readWindowPins();
    QJsonObject entry;
    entry["pinned"] = true;
    entry["x"] = position.x();
    entry["y"] = position.y();
    pins[windowName] = entry;
    writeWindowPins(pins);
}

void Game::savePinnedWindowGeometry(const QString &windowName, const QRect &geometry, bool maximized){
    QJsonObject pins = readWindowPins();
    QJsonObject entry;
    entry["pinned"] = true;
    entry["x"] = geometry.x();
    entry["y"] = geometry.y();
    entry["w"] = geometry.width();
    entry["h"] = geometry.height();
    entry["maximized"] = maximized;
    pins[windowName] = entry;
    writeWindowPins(pins);
}

void Game::clearPinnedWindowPosition(const QString &windowName){
    QJsonObject pins = readWindowPins();
    pins.remove(windowName);
    writeWindowPins(pins);
}

QPoint Game::visibleWindowPosition(const QPoint &position, const QSize &windowSize){
    QScreen *targetScreen = NULL;
    const QRect requested(position, windowSize);
    const QList<QScreen*> screens = QGuiApplication::screens();
    for(QScreen *screen : screens){
        if(screen->availableGeometry().intersects(requested)){
            targetScreen = screen;
            break;
        }
    }
    if(targetScreen == NULL)
        targetScreen = QGuiApplication::primaryScreen();
    if(targetScreen == NULL)
        return position;

    const QRect available = targetScreen->availableGeometry();
    const int maxX = qMax(available.left(), available.right() - qMin(windowSize.width(), available.width()) + 1);
    const int maxY = qMax(available.top(), available.bottom() - qMin(windowSize.height(), available.height()) + 1);
    return QPoint(qBound(available.left(), position.x(), maxX),
                  qBound(available.top(), position.y(), maxY));
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
    const QString assetsPath = "./tsre_assets/";
    if (!QFileInfo::exists(assetsPath)) {
        QDir().mkdir(assetsPath);
    }

    const QString appDataPath = "./tsre_appdata/" + Game::AppDataVersion;
    if (!QFileInfo(appDataPath).isDir()) {
        qCritical() << "Required bundled app data is missing:" << appDataPath;
        QMessageBox::critical(
            nullptr,
            "TSRE - Missing App Data",
            "The required bundled application data is missing:\n\n"
                + QDir::toNativeSeparators(QFileInfo(appDataPath).absoluteFilePath())
                + "\n\nReinstall or restore the matching TSRE application-data folder."
                  " TSRE no longer downloads executable data from the Internet.");
        return;
    }
}

void Game::load() {
    
    QString sh;
    QString path;
    
    cleanupAppData();
    path = settingsFilePath();
    QFile file(path);
    
    if (!file.exists()){
        qDebug() << "creating new JSON settings file" << path;
        SettingsDialog diag(nullptr);
        if(!diag.saveDefaults(path)){
            qWarning() << "unable to create settings file" << path;
            return;
        }
    }
    
    if (!file.open(QIODevice::ReadOnly)){
        qDebug() << "settings file fails to open";
        return;
    }
    
    if(Game::debugOutput) qDebug() << path;

    QJsonParseError jsonError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll(), &jsonError);
    file.close();
    if(jsonError.error != QJsonParseError::NoError || !jsonDocument.isObject()){
        qWarning() << "settings JSON fails to parse:" << jsonError.errorString();
        const QString corruptPath = appDataDir()
                + "/settings.corrupt-"
                + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz")
                + ".json";
        bool preserved = QFile::rename(path, corruptPath);
        if(!preserved)
            preserved = QFile::copy(path, corruptPath);
        if(!preserved){
            QMessageBox::warning(
                nullptr, "Settings Recovery Failed",
                "settings.json is damaged and could not be backed up.\n\n"
                "TSRE will continue with its built-in settings for this session.");
            return;
        }

        SettingsDialog diag(nullptr);
        if(!diag.saveDefaults(path)){
            QMessageBox::warning(
                nullptr, "Settings Recovery Failed",
                QString("The damaged settings were preserved at:\n%1\n\n"
                        "TSRE could not create a replacement settings.json.")
                .arg(corruptPath));
            return;
        }

        QFile repairedFile(path);
        if(!repairedFile.open(QIODevice::ReadOnly)){
            qWarning() << "repaired settings file fails to open";
            return;
        }
        jsonDocument = QJsonDocument::fromJson(repairedFile.readAll(), &jsonError);
        repairedFile.close();
        if(jsonError.error != QJsonParseError::NoError || !jsonDocument.isObject()){
            qWarning() << "repaired settings JSON fails to parse:" << jsonError.errorString();
            return;
        }

        QMessageBox::warning(
            nullptr, "Settings Recovered",
            QString("settings.json was damaged.\n\n"
                    "TSRE preserved it as:\n%1\n\n"
                    "A clean default settings.json was created.")
            .arg(corruptPath));
    }

    QString settingsText;
    QTextStream settingsWriter(&settingsText, QIODevice::WriteOnly);
    const QJsonObject settingsObject = jsonDocument.object();
    for(auto it = settingsObject.constBegin(); it != settingsObject.constEnd(); ++it){
        QString value;
        if(it.value().isBool())
            value = it.value().toBool() ? "true" : "false";
        else if(it.value().isDouble())
            value = QString::number(it.value().toDouble(), 'g', 15);
        else if(it.value().isString())
            value = it.value().toString();
        else
            continue;
        settingsWriter << it.key() << "=" << value << "\n";
    }
    settingsWriter.flush();
    QTextStream in(&settingsText, QIODevice::ReadOnly);
    QString line;
    QStringList args;
    QString setval;
    QString setname; 
    QString skipped;   // save what's being skipped for output later

    qDebug() << "Debug: " << Game::debugOutput;
    
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
        args.push_back(line.section('=', 1, -1));
        
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
 *          Tokens get set to lowercase and don't care if mixed case was used in settings.json
 
 
 
 */
        
        //// Startup settings
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
            if(setname =="instanceprotection"){
                instanceProtection = (setval == "true") || (setval == "1") || (setval == "on");
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

            if(setname =="mstsshadows"){
                mstsShadows = (setval == "true") || (setval == "1") || (setval == "on");
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
                const QString authMode = args[1].trimmed();
                const QString normalizedAuthMode = authMode.toLower();
                if(normalizedAuthMode == "false" || normalizedAuthMode == "0"
                        || normalizedAuthMode == "off")
                    serverAuth.clear();
                else if(normalizedAuthMode == "true" || normalizedAuthMode == "1"
                        || normalizedAuthMode == "on")
                    serverAuth = "file";
                else
                    serverAuth = authMode;
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

   

            
            
        //// Remaining settings
        
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
            const QColor configuredColor(setval);
            if(configuredColor.isValid()){
                if(selectedColor == NULL)
                    selectedColor = new QColor(configuredColor);
                else
                    *selectedColor = configuredColor;
            } else {
                qWarning() << "Ignoring invalid selectedColor setting:" << setval;
            }
        }
        
        if(setname =="selectedterrcolor"){
            const QColor configuredColor(setval);
            if(configuredColor.isValid()){
                if(selectedTerrColor == NULL)
                    selectedTerrColor = new QColor(configuredColor);
                else
                    *selectedTerrColor = configuredColor;
            } else {
                qWarning() << "Ignoring invalid selectedTerrColor setting:" << setval;
            }
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
        if(setname =="terrainconformtdbbias") {
            terrainConformTdbBias = setval.toFloat();
        }
        if(setname =="terrainconformrdbbias") {
            terrainConformRdbBias = setval.toFloat();
        }
        if(setname=="terrainbrushcolor") {
            if(terrBrushColor == NULL)
                terrBrushColor = new QColor(setval);
            else
                *terrBrushColor = QColor(setval);
        }
        
        
        // END EFO Configure Terrain Tools
        
        
        if(setname =="geopath")
            geoPath = setval;
        
        if(setname =="colorconview") {
            if(colorConView == NULL)
                colorConView = new QColor(setval);
            else
                *colorConView = QColor(setval);
        }
        if(setname =="colorshapeview") {
            if(colorShapeView == NULL)
                colorShapeView = new QColor(setval);
            else
                *colorShapeView = QColor(setval);
        }
        
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
            customImageMapsUrl = args[1].trimmed();
        }

        if(setname =="imagemapszoomoffset"){
            customImageMapsZoomOffset = setval.toInt();
        }

        if(setname =="mapengine"){
            mapEngine = args[1].trimmed();
        }

        if(setname =="googleimagemapsurl"){
            googleImageMapsUrl = args[1].trimmed();
        }

        if(setname =="googlemapapikey"){
            googleMapAPIKey = args[1].trimmed();
        }

        if(setname =="googleimagemapszoomoffset"){
            googleImageMapsZoomOffset = setval.toInt();
        }

        if(setname =="mapboximagemapsurl"){
            mapboxImageMapsUrl = args[1].trimmed();
        }

        if(setname =="mapboxmapapikey"){
            mapboxMapAPIKey = args[1].trimmed();
        }

        if(setname =="mapboximagemapszoomoffset"){
            mapboxImageMapsZoomOffset = setval.toInt();
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
        if(setname =="snapableradius"){
            snapableRadius = qMax(0.0f, setval.toFloat());
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
        if(setname =="fogdensity"){
            fogDensity = setval.toFloat();
        }
        if(setname =="fogcolor"){
            QColor tcolor(setval);
            if(tcolor.isValid()){
                fogColor[0] = tcolor.redF();
                fogColor[1] = tcolor.greenF();
                fogColor[2] = tcolor.blueF();
            }
        }
        if(setname =="skycolor"){
            QColor tcolor(setval);
            if(tcolor.isValid()){
                skyColor[0] = tcolor.redF();
                skyColor[1] = tcolor.greenF();
                skyColor[2] = tcolor.blueF();
            }
        }
        if(setname =="defaultelevationbox"){
            DefaultElevationBox = setval.toInt();
        }
        if(setname =="defaultmovestep"){
            DefaultMoveStep = setval.toFloat();
        }
        if(setname =="uiscale"){
            uiScale = setval.toFloat();
            if(uiScale < 0.75f)
                uiScale = 0.75f;
            if(uiScale > 1.25f)
                uiScale = 1.25f;
        }

        if(setname =="season"){
            // Disabled for GenX: seasonal display is controlled live from the
            // F2 terrain texture selector so old settings files cannot override it.
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
        if(setname == "viewmarkers"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 viewMarkers = true;
            else
                 viewMarkers = false;
        }       
        if(setname == "listfiles"){
             if((setval == "true") or (setval == "1") or (setval == "on"))
                 listFiles = true;
            else
                 listFiles = false;
        }       

        if(setname =="mapapikey"){
                customMapAPIKey = args[1].trimmed();

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
    
    configureMapProvider();
    qDebug() << "F3 imagery provider:" << mapEngine;
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
      
       

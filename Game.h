/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef GAMESETTINGS_H
#define	GAMESETTINGS_H

#include <QString>
#include <QHash>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//class RouteEditorWindow;
//class LoadWindow;
class TDB;
class SoundList;
class ShapeLib;
class EngLib;
class QColor;
class Route;
class GameObj;
class TerrainLib;
class GeoWorldCoordinateConverter;
class Renderer;
class RouteEditorClient;

class Game {
public:
    static bool ServerMode;
    static QString serverLogin;
    static QString serverAuth;
    static RouteEditorClient* serverClient;
    
    static GeoWorldCoordinateConverter *GeoCoordConverter;
    
    static TDB *trackDB;
    static TDB *roadDB;
    static SoundList *soundList;
    static TerrainLib *terrainLib;
    static Renderer *currentRenderer;
    
    static bool UseWorkingDir;
    static QString AppVersion;
    static QString AppDataVersion;
    static QString AppName;
    static QString root;
    static QString route;
    static QString routeName;
    static QString trkName;
    static QString season;
    static QString mainWindowLayout;
    static QString ceWindowLayout;
    static QString ActivityToPlay;
    static bool useQuadTree;
    static bool useTdbEmptyItems;
    static bool playerMode;
    static bool useNetworkEng;
    static float objectLod;
    static float distantLod;
    static int tileLod;
    static int allowObjLag;
    static int maxObjLag;
    static bool ignoreLoadLimits;
    static void load();
    static void InitAssets();
    static QString appDataDir();
    static QString routeAppDataDir();
    static QString lastSessionFilePath();
    static QString terrainPaintPresetFilePath();
    //static bool loadRouteEditor();
    //static bool loadConEditor();
    static bool checkRoot(QString dir);
    static bool checkCERoot(QString dir);
    static bool checkRoute(QString dir);
    static bool checkRemoteRoute(QString dir);
    template<class T>
    static void check_coords(T&& x, T&& z, float* p);
    template<class T, class K>
    static void check_coords(T&& x, T&& z, K&& px, K&& pz);
    static int start;
    static int startTileX;
    static int startTileY;
    static bool ignoreMissingGlobalShapes;
    static bool deleteTrWatermarks;
    static bool deleteViewDbSpheres;
    static bool createNewRoutes;
    static bool writeEnabled;
    static bool writeTDB;
    static bool systemTheme;
    static bool toolsHidden;
    static bool usenNumPad;
    static float cameraFov;
    static float cameraSpeedMin;
    static float cameraSpeedStd;
    static float cameraSpeedMax;
    static float mouseSpeed;
    static bool cameraStickToTerrain;
    static bool mstsShadows;
    static bool viewWorldGrid;
    static bool viewTileGrid;
    static bool viewTerrainGrid;
    static bool viewTerrainShape;
    static bool viewInteractives;
    static bool viewForestRegions;
    static bool viewTrackDbLines;
    static bool viewTsectionLines;
    static bool viewMarkers;
    static bool viewSnapable;
    static bool viewCompass;
    static bool warningBox;
    static bool viewPointer3d;
    static bool leaveTrackShapeAfterDelete;
    static bool renderTrItems;
    static int newRouteX;
    static int newRouteZ;
    static QString geoPath;
    static ShapeLib *currentShapeLib;
    static EngLib *currentEngLib;
    static Route *currentRoute;
    static GameObj *currentSelectedGameObj;
    static QColor *colorConView;
    static QColor *colorShapeView;
    static bool consoleOutput;
    static bool flexLogEnabled;
    static bool flexLogCandidates;
    static QString flexLogFile;
    static int fpsLimit;
    static bool ortsEngEnable;
    static bool sortTileObjects;
    static int oglDefaultLineWidth;
    static bool showWorldObjPivotPoints;
    static int shadowMapSize;
    static int shadowLowMapSize;
    static int shadowsEnabled;
    static float sunLightDirection[];
    static int textureQuality;
    static float snapableRadius;
    static bool snapableOnlyRot;
    static float trackElevationMaxPm;    
    static bool proceduralTracks;
    static QString StyleMainLabel;
    static QString StyleGreenButton;
    static QString StyleRedButton;
    static QString StyleYellowButton;
    static QString StyleGreenText;
    static QString StyleRedText;
    static QString imageMapsUrl;
    static int mapImageResolution;
    static bool autoNewTiles;
    static bool autoGeoTerrain;
    static bool useSuperelevation;
    static bool soundEnabled;
    static bool scoSoundEnabled;
    static bool fullscreen;
            
    static float fogDensity;// = 0.7;
    static float shadow1Res;
    static float shadow1Bias;
    static float shadow2Res;
    static float shadow2Bias;
    //float fogColor[4]{0.5, 0.75, 1.0, 1.0};
    static float fogColor[4];//{230.0/255.0,248.0/255,255.0/255.0, 1.0};
    static float skyColor[4];//{230.0/255.0,248.0/255,255.0/255.0, 1.0};
    
    static int AASamples;
    static bool AARemoveBorder;
    static float PixelRatio;
    
    static int DefaultElevationBox;
    static float DefaultMoveStep;
    
    static bool hudEnabled;
    static float hudScale;
    static float uiScale;
    
    static bool markerLines;
    static bool seasonalEditing;
    static int numRecentItems;
    
    static bool loadAllWFiles;
    static bool autoFix;
    static bool gui;
    
    static QHash<QString, int> TextureFlags;
    
    static bool useOnlyPositiveQuaternions;
    
    static QString routeMergeString;
    static QStringList objectsToRemove;
    
    // EFO Adds
    static bool LocalTSectionOnly;
    static float wireLineHeight;
    static float sectionLineHeight;
    static float terrainTools[];
    static int selectedTerrWidth;
    static int selectedWidth;    
    static QColor *selectedColor;
    static QColor *selectedTerrColor;        
    static QColor *wireLineColor;
    static QColor *terrBrushColor;
    
    static bool loadConsists;
    static bool loadActivities;    
    static QString mainPos;
    static QString statusPos;
    static QString naviPos;
    static bool restoreLastSessionWindowGeometry;
    static int restoreMainX;
    static int restoreMainY;
    static int restoreMainW;
    static int restoreMainH;
    static bool restoreMainMaximized;
    static bool restoreNaviGeometry;
    static int restoreNaviX;
    static int restoreNaviY;
    static int restoreNaviW;
    static int restoreNaviH;
    static bool restoreStatusGeometry;
    static int restoreStatusX;
    static int restoreStatusY;
    static int restoreStatusW;
    static int restoreStatusH;
    static bool restoreLastSessionCamera;
    static int restoreCameraTileX;
    static int restoreCameraTileZ;
    static float restoreCameraX;
    static float restoreCameraY;
    static float restoreCameraZ;
    static float restoreCameraRotX;
    static float restoreCameraRotY;
    static bool debugOutput;
    static bool legacySupport;
    static bool lockCamera;
    static bool newSymbols;
    static int pointerIn;
    static int pointerOut;
    static int pyramid;
    static int maxAutoPlacement;
    static int imageMapsZoomOffset;
    static float railProfile[];
    static bool flexDebugWindow;
    static QString convertUnitS;
    static float convertSpeed;

    static QString convertUnitM;
    static float convertMass;
    static QString convertUnitD;
    static float convertDistance;
    static int markerHeight;
    static int markerText;
    static float lastElev;
    static float sigOffset;
    static QStringList markerFiles;
    static bool reload;
    static QString MapAPIKey;    
    static bool imageSubstitution;
    static bool imageUpgrade;
    static QString includeFolder;
    static float  deepUnderground;
    static int logfileMax;
    static int logfileDays;
    static QString startapp;
    static bool listFiles;
    static bool showSDL;
    static int rnp;
    static bool CheckBraces;
    static bool UnsafeMode;
    static bool extendedDebug;
    static bool routeMergeTerrain;
    static bool routeMergeTDB;
    static bool routeMergeTerrtex;    
    static bool routeRebuildTDB;    
    static QStringList preloadTextures;
    static bool objSelected;
    static bool resetTools;
    static bool viewTRLabels;
    static float trackGap;

private:
    //static RouteEditorWindow* window;
    //static LoadWindow* loadWindow;
    static void CreateNewSettingsFile();
    static void DownloadAppData(QString path);
    static void CheckForOpenAl();
    static void cleanupLogs();
    
    
};

#endif	/* GAMESETTINGS_H */


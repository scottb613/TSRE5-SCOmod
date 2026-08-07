/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QMap>
#include <QTimer>

class QSlider;
class QLabel;
class RouteEditorGLWidget;
class ObjTools;
class GeoTools;
class TerrainTools;
class ActivityTools;
class ActivityBuilderWindow;
class NaviBox;
class AboutWindow;
class StatusWindow;
class SettingsDialog;
class ShapeViewWindow;
class PropertiesAbstract;
class GameObj;
class PreciseTileCoordinate;
class ActivityEventWindow;
class ActivityServiceWindow;
class ActivityTrafficWindow;
class ActivityTimetableWindow;
class ErrorMessagesWindow;
class ClientUsersWindow;

class RouteEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    RouteEditorWindow();
    void applyPinnedMainWindowPosition();
    
public slots:
    void save();
    void openRouteFolder();
    void createRouteHealthReport();
    void showRoute();
    void show();
    void createPaths();
    void reloadRef();
    void reloadMkr();
    void restartAndRestore();
    void toggleMainWindowPositionPin(bool pinned);
    void savePinnedMainWindowPosition();
    void refreshErrors();
    void about();
    void terrainCamera(bool val);
    void mstsShadows(bool val);
    void detailedTerrainEnabled();
    void distantTerrainEnabled();
    void setToolbox(QString name);
    void showProperties(GameObj* obj);
    void updateProperties(GameObj* obj);
    void hideShowToolWidget(bool show);
    void hideShowPropertiesWidget(bool show);
    void hideShowStatWidget(bool);    
    void hideShowSettingsDialog(bool);
    void hideShowShapeViewWidget(bool);
    void hideShowErrorMsgWidget(bool);
    void viewWorldGrid(bool show);
    void viewTileGrid(bool show);
    void viewTerrainShape(bool show);
    void viewTerrainGrid(bool show);
    void viewInteractives(bool show);
    void viewForestRegions(bool show);
    void viewTrackDbLines(bool show);
    void viewTsectionLines(bool show);
    void viewGradeSymbols(bool show);
    void viewTrackItems(bool show);
    void viewPointer3d(bool show);
    void viewMarkers(bool show);
    void viewSnapable(bool show);
    void viewCompass(bool show);
    void showToolsObjectAndTerrain(bool show);
    void showToolsObject(bool show);
    void showToolsTerrain(bool show);
    void showToolsGeo(bool show);
    void showToolsActivity(bool show);
    
    void showTerrainTreeEditr();
    void showWorldObjPivotPointsEnabled(bool show);
    void statusWindowClosed();    
    void errorMessagesWindowClosed();
    void shapeVeiwWindowClosed();
    void viewToggleAll();
    void showActivityEventEditor();
    void showActivityServiceEditor();
    void showActivityTrafficEditor();
    void showActivityTimetableEditor();
    void exitToLoadWindow();

    
signals:
    void exitNow();
    void returnToLoadWindow();
    void sendMsg(QString text);
    void reloadRefFile();
    void reloadMkrFile();    
    void reloadMkrLists();
    void refreshErrorList();
   /// EFO Status updates
    void updStatus(QString statName, QString statValue);    
    
protected:
    void keyPressEvent(QKeyEvent *event);
    virtual void closeEvent(QCloseEvent * event );
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);
    void changeEvent(QEvent *event);
    void hideAllTools();

private:
    void hideRouteSessionWindows();
    void completeEditorClose(QCloseEvent *event);
    void saveLastSession();
    void applyRestoredSessionGeometry();
    QWidget* box;
    QWidget* box2;
    QLabel* propertiesPanelTitle = NULL;
    
    RouteEditorGLWidget *glWidget;
    ObjTools *objTools;
    GeoTools *geoTools;
    TerrainTools *terrainTools;
    ActivityTools *activityTools;
    ActivityBuilderWindow *activityBuilderWindow;
    //NaviBox *naviBox;
    
    QMap<QString, PropertiesAbstract*> objProperties;
    
    QMenu *routeMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QMenu *toolsMenu;
    QMenu *settingsMenu;
    
    QAction *saveAction;
    QAction *openRouteFolderAction;
    QAction *routeHealthReportAction;
    QAction *createPathsAction;
    QAction *reloadRefAction;
    QAction *reloadMkrAction;    
    QAction *closeAction;
    QAction *exitAction;
    QAction *rebuildAction;
    
    QAction *copyAction;
    QAction *undoAction;
    QAction *pasteAction;
    QAction *selectAction;
    QAction *aboutAction;
    QAction *terrainCameraAction;
    QAction *mstsShadowsAction;
    QAction *pinMainWindowAction = NULL;
    QAction *propertiesAction;
    QAction *statAction;
    QAction *settingsAction;    
    QAction *shapeViewAction;
    QAction *errorViewAction;
    QAction *toolsAction;
    QAction *objectsAction;
    QAction *objectsAndTerrainAction;
    QAction *terrainAction;
    QAction *terrainTreeEditr;
    QAction *trkEditr;
    QAction *geoAction;
    QAction *activityAction;
    
    QAction* vViewWorldGrid;
    QAction* vViewTileGrid;
    QAction* vViewTerrainGrid;
    QAction* vViewTerrainShape;
    QAction* vShowWorldObjPivotPoints;
    QAction* vViewInteractives;
    QAction* vViewForestRegions;
    QAction* vViewTrackDbLines;
    QAction* vViewTsectionLines;
    QAction* vViewGradeSymbols;
    QAction* vViewTrackItems;
    QAction* vViewPointer3d;
    QAction* vViewMarkers;
    QAction* vViewSnapable;
    QAction* vViewCompass;
    QHash<QAction*, bool> savedViewSelections;
    bool viewAllOff = false;
    
    AboutWindow* aboutWindow;
    StatusWindow* statusWindow;
    SettingsDialog* settingsDialog;    
    ErrorMessagesWindow* errorMessagesWindow;
    ClientUsersWindow* clientUsersWindow;
    ActivityEventWindow* activityEventWindow;
    ActivityServiceWindow* activityServiceWindow;
    ActivityTrafficWindow* activityTrafficWindow;
    ActivityTimetableWindow* activityTimetableWindow;
    ShapeViewWindow* shapeViewWindow;
    QTimer pinPositionTimer;
    bool applyingWindowPosition = false;
};
//! [0]

#endif

/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include <QtWidgets>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include "RouteEditorGLWidget.h"
#include "RouteEditorWindow.h"
#include "Game.h"
#include "AceLib.h"
#include <QDebug>
#include "GuiFunct.h"
#include "ObjTools.h"
#include "TerrainTools.h"
#include "GeoTools.h"
#include "ActivityTools.h"
#include "ActivityBuilderWindow.h"
#include "NaviBox.h"
#include "ShapeViewWindow.h"
#include "AboutWindow.h"
#include "SoundManager.h"
#include "PropertiesAbstract.h"
#include "PropertiesUndefined.h"
#include "PropertiesStatic.h"
#include "PropertiesTransfer.h"
#include "PropertiesPlatform.h"
#include "PropertiesSiding.h"
#include "PropertiesCarspawner.h"
#include "PropertiesDyntrack.h"
#include "PropertiesSignal.h"
#include "PropertiesPickup.h"
#include "PropertiesForest.h"
#include "PropertiesSoundSource.h"
#include "PropertiesSpeedpost.h"
#include "PropertiesTrackObj.h"
#include "PropertiesGroup.h"
#include "PropertiesRuler.h"
#include "PropertiesLevelCr.h"
#include "PropertiesSoundRegion.h"
#include "PropertiesTerrain.h"
#include "PropertiesActivityObject.h"
#include "PropertiesTrackItem.h"
#include "PropertiesActivityPath.h"
#include "PropertiesConsist.h"
#include "Ref.h"
#include "StatusWindow.h"
#include "SettingsDialog.h"
#include "ErrorMessagesWindow.h"
#include "ClientUsersWindow.h"
#include "ErrorMessagesLib.h"
#include "UnsavedDialog.h"
#include "ActivityEventWindow.h"
#include "ActivityEventProperties.h"
#include "ActivityServiceWindow.h"
#include "ActivityServiceProperties.h"
#include "ActivityTrafficWindow.h"
#include "ActivityTrafficProperties.h"
#include "ActivityTimetableWindow.h"
#include "ActivityTimetableProperties.h"
#include "RouteEditorClient.h"
#include "Route.h"
#include "LoadWindow.h"
#include "CELoadWindow.h"
#include "TexLib.h"
#include "PropertiesPolyForest.h"
#include "PropertiesHazard.h"
#include "SettingsDialog.h"

static int scaledUiSize(int base){
    return qRound(base * qMax(1.0f, Game::uiScale));
}

static void tuneScaledPanel(QWidget *panel){
    QFont panelFont = panel->font();
    if(panelFont.pointSizeF() > 0)
        panelFont.setPointSizeF(panelFont.pointSizeF() * 1.08);
    panel->setFont(panelFont);

    QList<QWidget*> widgets = panel->findChildren<QWidget*>();
    for(int i = 0; i < widgets.size(); i++){
        QWidget *widget = widgets[i];
        widget->setFont(panelFont);
        if(qobject_cast<QPushButton*>(widget) != NULL ||
           qobject_cast<QLineEdit*>(widget) != NULL ||
           qobject_cast<QComboBox*>(widget) != NULL ||
           qobject_cast<QCheckBox*>(widget) != NULL)
            widget->setMinimumHeight(scaledUiSize(20));
    }
}

static void restoreEditorFocusAfterButtons(QWidget *panel, RouteEditorGLWidget *editor){
    if(panel == NULL || editor == NULL)
        return;
    const QList<QPushButton*> buttons = panel->findChildren<QPushButton*>();
    for(QPushButton *button : buttons){
        if(button == NULL)
            continue;
        QObject::connect(button, &QPushButton::clicked, editor, [editor](){
            QTimer::singleShot(0, editor, &RouteEditorGLWidget::focusEditor);
        });
    }
}

static void restoreEditorFocusAfterToolButtons(QWidget *panel, RouteEditorGLWidget *editor){
    if(panel == NULL || editor == NULL)
        return;
    const QList<QToolButton*> buttons = panel->findChildren<QToolButton*>();
    for(QToolButton *button : buttons){
        if(button == NULL)
            continue;
        QObject::connect(button, &QToolButton::clicked, editor, [editor](){
            QTimer::singleShot(0, editor, &RouteEditorGLWidget::focusEditor);
        });
    }
}

static void addUserClickSoundToButtons(QWidget *panel, RouteEditorGLWidget *editor){
    if(panel == NULL || editor == NULL)
        return;
    const QList<QPushButton*> buttons = panel->findChildren<QPushButton*>();
    for(QPushButton *button : buttons){
        if(button == NULL)
            continue;
        if(button->property("scoSuppressClickSound").toBool())
            continue;
        if(button->property("scoSoundOnPress").toBool())
            QObject::connect(button, &QPushButton::pressed,
                             editor, &RouteEditorGLWidget::userModeChangeSound);
        else
            QObject::connect(button, &QPushButton::clicked,
                             editor, &RouteEditorGLWidget::userModeChangeSound);
    }
}

static void addUserClickSoundToToolButtons(QWidget *panel, RouteEditorGLWidget *editor){
    if(panel == NULL || editor == NULL)
        return;
    const QList<QToolButton*> buttons = panel->findChildren<QToolButton*>();
    for(QToolButton *button : buttons){
        if(button != NULL)
            QObject::connect(button, &QToolButton::clicked,
                             editor, &RouteEditorGLWidget::userModeChangeSound);
    }
}


RouteEditorWindow::RouteEditorWindow() {

    objTools = new ObjTools("ObjTools");
    terrainTools = new TerrainTools("TerrainTools");
    geoTools = new GeoTools("GeoTools");
    activityTools = new ActivityTools("ActivityTools");
    activityBuilderWindow = new ActivityBuilderWindow(activityTools, this);
    //naviBox = new NaviBox();
    glWidget = new RouteEditorGLWidget(this);
    
    shapeViewWindow = new ShapeViewWindow(this);
    aboutWindow = new AboutWindow(this);
    statusWindow = new StatusWindow(this);
    settingsDialog = new SettingsDialog(this);
    QObject::connect(settingsDialog, SIGNAL(restartAndRestoreRequested()),
                     this, SLOT(restartAndRestore()));
    
    
    errorMessagesWindow = ErrorMessagesLib::GetWindow(this);
    clientUsersWindow = new ClientUsersWindow(this);
    activityEventWindow = new ActivityEventWindow(this);
    activityServiceWindow = new ActivityServiceWindow(this);
    activityTrafficWindow = new ActivityTrafficWindow(this);
    activityTimetableWindow = new ActivityTimetableWindow(this);
    
    objProperties["Static"] = new PropertiesStatic;
    objProperties["Transfer"] = new PropertiesTransfer;
    objProperties["Platform"] = new PropertiesPlatform;
    objProperties["Siding"] = new PropertiesSiding;
    objProperties["Carspawner"] = new PropertiesCarspawner;
    objProperties["Dyntrack"] = new PropertiesDyntrack;
    objProperties["Signal"] = new PropertiesSignal;
    objProperties["Pickup"] = new PropertiesPickup;
    objProperties["Forest"] = new PropertiesForest;
    objProperties["Speedpost"] = new PropertiesSpeedpost;
    objProperties["SoundSource"] = new PropertiesSoundSource;
    objProperties["TrackObj"] = new PropertiesTrackObj;
    objProperties["Group"] = new PropertiesGroup;
    objProperties["Ruler"] = new PropertiesRuler;
    objProperties["SoundRegion"] = new PropertiesSoundRegion;
    objProperties["LevelCr"] = new PropertiesLevelCr;
    objProperties["Terrain"] = new PropertiesTerrain;
    objProperties["ActivityObject"] = new PropertiesActivityObject;
    objProperties["TrackItem"] = new PropertiesTrackItem;
    objProperties["ActivityPath"] = new PropertiesActivityPath;
    objProperties["ActivityConsist"] = new PropertiesConsist;
    objProperties["Hazard"] = new PropertiesHazard;
    
    // last 
    objProperties["Undefined"] = new PropertiesUndefined;
    
    QWidget* remain = new QWidget();
    
    box = new QWidget(this);
    box2 = new QWidget(this);
    box->setFixedWidth(scaledUiSize(250));
    box->setWindowTitle("Tools Window");
    box2->setWindowFilePath("Properties Window");
    box2->setMaximumWidth(scaledUiSize(190));
    box2->setMinimumWidth(scaledUiSize(190));
    //box2->setMaximumWidth(250);
    //box2->setMinimumWidth(250);
    QHBoxLayout *mainLayout2 = new QHBoxLayout; 
    mainLayout2->setMargin(0);
    mainLayout2->setSpacing(0);
    mainLayout2->setContentsMargins(0,0,0,0);
    mainLayout2->addWidget(objTools);
    mainLayout2->addWidget(terrainTools);
    mainLayout2->addWidget(geoTools);
    //mainLayout2->addWidget(naviBox);
    //mainLayout2->setAlignment(naviBox, Qt::AlignBottom);
    box->setLayout(mainLayout2);
    
    
    QVBoxLayout *mainLayout3 = new QVBoxLayout;
    mainLayout3->setContentsMargins(3,3,3,3);
    mainLayout3->setSpacing(3);
    mainLayout2->setMargin(0);
    mainLayout2->setSpacing(0);
    propertiesPanelTitle = new QLabel("OBJECT PROPERTIES", box2);
    GuiFunct::styleEditorTitle(propertiesPanelTitle);
    propertiesPanelTitle->hide();
    mainLayout3->addWidget(propertiesPanelTitle);
    mainLayout3->addSpacing(scaledUiSize(3));
    //mainLayout3->addWidget(propertiesUndefined);
    
    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        it->hide();
        //console.log(obj.type);
        mainLayout3->addWidget(it);
    }
    
    //mainLayout3->addWidget(terrainTools);
    //mainLayout3->setAlignment(naviBox, Qt::AlignBottom);
    box2->setLayout(mainLayout3);
    tuneScaledPanel(box2);

    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setMargin(3);
    mainLayout->setSpacing(3);
    
    QString mainWindowLayout = Game::mainWindowLayout;
    if(!mainWindowLayout.toUpper().contains('W')){
        mainWindowLayout += 'W';
    }
    for(int i = 0; i < mainWindowLayout.length(); i++){
        if(mainWindowLayout[i].toUpper() == 'P')
            mainLayout->addWidget(box2);
        if(mainWindowLayout[i].toUpper() == 'T')
            mainLayout->addWidget(box);
        if(mainWindowLayout[i].toUpper() == 'W')
            mainLayout->addWidget(glWidget);
    }
    if(!mainWindowLayout.toUpper().contains('T')){
        box->move(this->pos());
        box->setWindowFlags(Qt::WindowType::Tool);
    }
    if(!mainWindowLayout.toUpper().contains('P')){
        box2->move(this->pos());
        box2->setWindowFlags(Qt::WindowType::Tool);
    }
    
    remain->setLayout(mainLayout);
    mainLayout->setContentsMargins(0,0,0,0);
    
    this->setCentralWidget(remain);
    setWindowTitle(Game::AppName+" "+Game::AppVersion+" Route Editor");
    QFont menuFont = menuBar()->font();
    if(menuFont.pointSizeF() > 0)
        menuFont.setPointSizeF(menuFont.pointSizeF() * qMax(1.0f, Game::uiScale));
    menuBar()->setFont(menuFont);
    menuBar()->setStyleSheet(QString("QMenuBar::item { padding: %1px %2px; } "
                                     "QMenu::item { padding: %1px %3px; min-height: %4px; } "
                                     "QMenu::item:selected { background-color: #f08200; color: black; }")
                             .arg(scaledUiSize(3))
                             .arg(scaledUiSize(5))
                             .arg(scaledUiSize(18))
                             .arg(scaledUiSize(18)));
    
    // MENUBAR
    // EFO -- modifying many keystroke shortcuts to eliminate overlaps    
    // Route
    saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcut(QKeySequence("Shift+Ctrl+S"));
    QObject::connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

    openRouteFolderAction = new QAction(tr("&Open Route Folder"), this);
    QObject::connect(openRouteFolderAction, SIGNAL(triggered()), this, SLOT(openRouteFolder()));

    createPathsAction = new QAction(tr("&Create Debug Paths"), this);
    QObject::connect(createPathsAction, SIGNAL(triggered()), this, SLOT(createPaths()));
    
    reloadRefAction = new QAction(tr("&Reload Ref File"), this);
    QObject::connect(reloadRefAction, SIGNAL(triggered()), this, SLOT(reloadRef()));
    
    reloadMkrAction = new QAction(tr("&Reload Mkr Files"), this);
    QObject::connect(reloadMkrAction, SIGNAL(triggered()), this, SLOT(reloadMkr()));
    
    closeAction = new QAction(tr("&Close"), this);
    QObject::connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));
      
    exitAction = new QAction(tr("&Exit"), this);
    exitAction->setShortcut(QKeySequence("Alt+F4"));
    QObject::connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    
    trkEditr = new QAction(tr("E&dit route settings"), this);
    QObject::connect(trkEditr, SIGNAL(triggered()), glWidget, SLOT(showTrkEditr()));
    
    rebuildAction = new QAction(tr("Re&build TDB (experimental)"), this);
    QObject::connect(rebuildAction, SIGNAL(triggered()), glWidget, SLOT(rebuildTDB()));
    
    
    if(Game::serverClient == NULL){
        routeMenu = menuBar()->addMenu(tr("&Route"));
        routeMenu->addAction(saveAction);
        routeMenu->addAction(openRouteFolderAction);
        routeMenu->addSeparator();
        routeMenu->addAction(reloadRefAction);
        routeMenu->addAction(reloadMkrAction);   
        routeMenu->addAction(createPathsAction);
        routeMenu->addAction(trkEditr);
        //routeMenu->addAction(rebuildAction);    // Not yet ready
        routeMenu->addAction(exitAction);
    } else {
        routeMenu = menuBar()->addMenu(tr("&Server"));
        routeMenu->addAction(exitAction);
    }
    // Edit
    editMenu = menuBar()->addMenu(tr("&Edit"));
    if(Undo::UndoEnabled){
        undoAction = new QAction(tr("&Undo"), this); 
        undoAction->setShortcut(QKeySequence("Ctrl+Z"));
        QObject::connect(undoAction, SIGNAL(triggered()), glWidget, SLOT(editUndo()));
        editMenu->addAction(undoAction);
    }
    copyAction = new QAction(tr("&Copy"), this); 
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    QObject::connect(copyAction, SIGNAL(triggered()), glWidget, SLOT(editCopy()));
    editMenu->addAction(copyAction);
    pasteAction = new QAction(tr("&Paste"), this); 
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    QObject::connect(pasteAction, SIGNAL(triggered()), glWidget, SLOT(editPaste()));
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    selectAction = new QAction(tr("&Select Tool"), this); 
    QObject::connect(selectAction, SIGNAL(triggered()), glWidget, SLOT(editSelect()));
    editMenu->addAction(selectAction);
    // View
    viewMenu = menuBar()->addMenu(tr("&View"));
    //toolsAction = GuiFunct::newMenuCheckAction(tr("&Tools"), this); 
    //viewMenu->addAction(toolsAction);
    //QObject::connect(toolsAction, SIGNAL(triggered(bool)), this, SLOT(hideShowToolWidget(bool)));

    QAction* viewToggleAll = new QAction(tr("&Toggle All"), this);
    viewMenu->addAction(viewToggleAll);
    QObject::connect(viewToggleAll, SIGNAL(triggered()), this, SLOT(viewToggleAll()));
    viewMenu->addSeparator();
    vViewWorldGrid = GuiFunct::newMenuCheckAction(tr("&World Grid"), this); 
    viewMenu->addAction(vViewWorldGrid);
    QObject::connect(vViewWorldGrid, SIGNAL(triggered(bool)), this, SLOT(viewWorldGrid(bool)));
    vViewTileGrid = GuiFunct::newMenuCheckAction(tr("Tile &Grid"), this); 
    viewMenu->addAction(vViewTileGrid);
    QObject::connect(vViewTileGrid, SIGNAL(triggered(bool)), this, SLOT(viewTileGrid(bool)));    
    vViewTerrainGrid = GuiFunct::newMenuCheckAction(tr("Te&rrain Grid"), this, false); 
    viewMenu->addAction(vViewTerrainGrid);
    QObject::connect(vViewTerrainGrid, SIGNAL(triggered(bool)), this, SLOT(viewTerrainGrid(bool)));   
    vViewTerrainShape = GuiFunct::newMenuCheckAction(tr("&Hide Terrain Shape"), this, false); 
    viewMenu->addAction(vViewTerrainShape);
    QObject::connect(vViewTerrainShape, SIGNAL(triggered(bool)), this, SLOT(viewTerrainShape(bool)));   
    vShowWorldObjPivotPoints = GuiFunct::newMenuCheckAction(tr("World&Obj Markers"), this, false); 
    viewMenu->addAction(vShowWorldObjPivotPoints);
    QObject::connect(vShowWorldObjPivotPoints, SIGNAL(triggered(bool)), this, SLOT(showWorldObjPivotPointsEnabled(bool)));
    vViewInteractives = GuiFunct::newMenuCheckAction(tr("&Interactives"), this); 
    viewMenu->addAction(vViewInteractives);
    QObject::connect(vViewInteractives, SIGNAL(triggered(bool)), this, SLOT(viewInteractives(bool)));
    vViewForestRegions = GuiFunct::newMenuCheckAction(tr("&Forest Region"), this, Game::viewForestRegions);
    viewMenu->addAction(vViewForestRegions);
    QObject::connect(vViewForestRegions, SIGNAL(triggered(bool)), this, SLOT(viewForestRegions(bool)));
    vViewTrackDbLines = GuiFunct::newMenuCheckAction(tr("Track&DB Lines"), this); 
    viewMenu->addAction(vViewTrackDbLines);
    QObject::connect(vViewTrackDbLines, SIGNAL(triggered(bool)), this, SLOT(viewTrackDbLines(bool)));    
    vViewTsectionLines = GuiFunct::newMenuCheckAction(tr("T&section Lines"), this); 
    viewMenu->addAction(vViewTsectionLines);
    QObject::connect(vViewTsectionLines, SIGNAL(triggered(bool)), this, SLOT(viewTsectionLines(bool)));
    vViewGradeSymbols = GuiFunct::newMenuCheckAction(tr("&Grade Symbols"), this, false);
    viewMenu->addAction(vViewGradeSymbols);
    QObject::connect(vViewGradeSymbols, SIGNAL(triggered(bool)), this, SLOT(viewGradeSymbols(bool)));
    QObject::connect(viewMenu, &QMenu::aboutToShow, this, [this](){
        vViewGradeSymbols->setChecked(Game::gradeOverlayEnabled);
    });
    vViewTrackItems = GuiFunct::newMenuCheckAction(tr("&TrackDB Items"), this, Game::renderTrItems); 
    viewMenu->addAction(vViewTrackItems);
    QObject::connect(vViewTrackItems, SIGNAL(triggered(bool)), this, SLOT(viewTrackItems(bool)));
    
    vViewPointer3d = GuiFunct::newMenuCheckAction(tr("&3D Pointer"), this); 
    viewMenu->addAction(vViewPointer3d);
    QObject::connect(vViewPointer3d, SIGNAL(triggered(bool)), this, SLOT(viewPointer3d(bool)));
    vViewMarkers = GuiFunct::newMenuCheckAction(tr("&Markers"), this, Game::viewMarkers); 
    viewMenu->addAction(vViewMarkers);
    QObject::connect(vViewMarkers, SIGNAL(triggered(bool)), this, SLOT(viewMarkers(bool)));
    vViewSnapable = GuiFunct::newMenuCheckAction(tr("S&napable Points"), this, Game::viewSnapable); 
    viewMenu->addAction(vViewSnapable);
    QObject::connect(vViewSnapable, SIGNAL(triggered(bool)), this, SLOT(viewSnapable(bool)));
    vViewCompass = GuiFunct::newMenuCheckAction(tr("&Compass"), this, Game::viewCompass);
    viewMenu->addAction(vViewCompass);
    QObject::connect(vViewCompass, SIGNAL(triggered(bool)), this, SLOT(viewCompass(bool)));

    // Tools
    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    propertiesAction = GuiFunct::newMenuCheckAction(tr("&Properties"), this); 
    propertiesAction->setShortcut(QKeySequence("F5"));            
    toolsMenu->addAction(propertiesAction);
    QObject::connect(propertiesAction, SIGNAL(triggered(bool)), this, SLOT(hideShowPropertiesWidget(bool)));

    statAction = GuiFunct::newMenuCheckAction(tr("Control &Panel"), this, false); 
    statAction->setShortcut(QKeySequence("F7"));
    toolsMenu->addAction(statAction);
    QObject::connect(statAction, SIGNAL(triggered(bool)), this, SLOT(hideShowStatWidget(bool)));
    
    settingsAction = GuiFunct::newMenuCheckAction(tr("S&ettings Window"), this, false); 
    settingsAction->setShortcut(QKeySequence("F12"));
    toolsMenu->addAction(settingsAction);
    QObject::connect(settingsAction, SIGNAL(triggered(bool)), this, SLOT(hideShowSettingsDialog(bool)));
    
    
    
    shapeViewAction = GuiFunct::newMenuCheckAction(tr("&Shape View Window"), this, false); 
    toolsMenu->addAction(shapeViewAction);
    QObject::connect(shapeViewAction, SIGNAL(triggered(bool)), this, SLOT(hideShowShapeViewWidget(bool)));
    errorViewAction = GuiFunct::newMenuCheckAction(tr("&Errors and Messages"), this, false); 
    errorViewAction->setShortcut(QKeySequence("F8"));    
    toolsMenu->addAction(errorViewAction);
    QObject::connect(errorViewAction, SIGNAL(triggered(bool)), this, SLOT(hideShowErrorMsgWidget(bool)));
    toolsMenu->addSeparator();
    objectsAndTerrainAction = GuiFunct::newMenuCheckAction(tr("O&bjects and Terrain"), this); 
    toolsMenu->addAction(objectsAndTerrainAction);
    QObject::connect(objectsAndTerrainAction, SIGNAL(triggered(bool)), this, SLOT(showToolsObjectAndTerrain(bool)));
    objectsAction = GuiFunct::newMenuCheckAction(tr("&Objects"), this); 
    objectsAction->setShortcut(QKeySequence("F1"));
    toolsMenu->addAction(objectsAction);
    QObject::connect(objectsAction, SIGNAL(triggered(bool)), this, SLOT(showToolsObject(bool)));
    terrainAction = GuiFunct::newMenuCheckAction(tr("&Terrain"), this); 
    terrainAction->setChecked(false);    
    terrainAction->setShortcut(QKeySequence("F2"));
    toolsMenu->addAction(terrainAction);
    QObject::connect(terrainAction, SIGNAL(triggered(bool)), this, SLOT(showToolsTerrain(bool)));
    geoAction = GuiFunct::newMenuCheckAction(tr("&Geo"), this); 
    geoAction->setChecked(false);    
    geoAction->setShortcut(QKeySequence("F3"));
    toolsMenu->addAction(geoAction);
    QObject::connect(geoAction, SIGNAL(triggered(bool)), this, SLOT(showToolsGeo(bool)));
    activityAction = GuiFunct::newMenuCheckAction(tr("&Activity"), this); 
    activityAction->setChecked(false);    
    activityAction->setShortcut(QKeySequence("F4"));
    toolsMenu->addAction(activityAction);
    QObject::connect(activityAction, SIGNAL(triggered(bool)), this, SLOT(showToolsActivity(bool)));
    QObject::connect(activityBuilderWindow, SIGNAL(visibilityChanged(bool)),
                     activityAction, SLOT(setChecked(bool)));
    QObject::connect(activityBuilderWindow, SIGNAL(userToggleSoundRequested()),
                     glWidget, SLOT(userPanelToggleSound()));
    // Settings
    terrainCameraAction = GuiFunct::newMenuCheckAction(tr("&Stick Camera To Terrain"), this); 
    terrainCameraAction->setChecked(Game::cameraStickToTerrain);
    terrainCameraAction->setShortcut(QKeySequence("/"));
    QObject::connect(terrainCameraAction, SIGNAL(triggered(bool)), this, SLOT(terrainCamera(bool)));
    mstsShadowsAction = GuiFunct::newMenuCheckAction(tr("&MSTS Shadows"), this); 
    mstsShadowsAction->setChecked(Game::mstsShadows);
    QObject::connect(mstsShadowsAction, SIGNAL(triggered(bool)), this, SLOT(mstsShadows(bool)));
    QPoint pinnedMainPosition;
    const bool mainPositionPinned = Game::pinnedWindowPosition("mainWindow", &pinnedMainPosition);
    pinMainWindowAction = GuiFunct::newMenuCheckAction(tr("Pin Main"), this, mainPositionPinned);
    pinMainWindowAction->setToolTip(mainPositionPinned
        ? tr("The Main Window position is saved between sessions. Unpin to return to centered placement.")
        : tr("Pin the current Main Window position between sessions."));
    QObject::connect(pinMainWindowAction, SIGNAL(triggered(bool)), this, SLOT(toggleMainWindowPositionPin(bool)));
    pinPositionTimer.setSingleShot(true);
    QObject::connect(&pinPositionTimer, SIGNAL(timeout()), this, SLOT(savePinnedMainWindowPosition()));
    QMenu *terrainMenu = new QMenu("&Terrain Editing:");
    QAction *detailTerrainAction = new QAction(tr("&Detailed Terrain"), this);
    QObject::connect(detailTerrainAction, SIGNAL(triggered()), this, SLOT(detailedTerrainEnabled()));
    QAction *distantTerrainAction = new QAction(tr("&Distant Terrain"), this);
    QObject::connect(distantTerrainAction, SIGNAL(triggered()), this, SLOT(distantTerrainEnabled()));
    terrainMenu->addAction(detailTerrainAction);
    terrainMenu->addAction(distantTerrainAction);
    settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(terrainCameraAction);
    settingsMenu->addAction(mstsShadowsAction);
    settingsMenu->addAction(pinMainWindowAction);
    settingsMenu->addMenu(terrainMenu);
    // Help
    aboutAction = new QAction(tr("&About"), this);
    QObject::connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    
    hideAllTools();
    objTools->show();
    ///// EFO  End MENUBAR 
 
    if(Game::toolsHidden){
        box->hide();
        box2->hide();
        menuBar()->hide();
    } else {
        //box->show();
        //box2->show();
    }
    
    if(Game::playerMode){
        statusWindow->hide();
        errorMessagesWindow->hide();
        box->hide();
        box2->hide();
        menuBar()->hide();
        this->viewToggleAll();
    }
    
    if(Game::serverClient != NULL){
        clientUsersWindow->show();
    }
    
    QObject::connect(this, SIGNAL(sendMsg(QString)),
                      glWidget, SLOT(msg(QString)));
    
    //ObjTools <-> qlWidget
    QObject::connect(objTools, SIGNAL(sendMsg(QString)), glWidget, SLOT(msg(QString)));
    QObject::connect(objTools, SIGNAL(sendMsg(QString, bool)), glWidget, SLOT(msg(QString, bool)));
    QObject::connect(objTools, SIGNAL(sendMsg(QString, int)), glWidget, SLOT(msg(QString, int)));
    QObject::connect(objTools, SIGNAL(sendMsg(QString, float)), glWidget, SLOT(msg(QString, float)));
    QObject::connect(objTools, SIGNAL(sendMsg(QString, QString)), glWidget, SLOT(msg(QString, QString)));
    
    QObject::connect(glWidget, SIGNAL(sendMsg(QString)), objTools, SLOT(msg(QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, bool)), objTools, SLOT(msg(QString, bool)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, int)), objTools, SLOT(msg(QString, int)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, float)), objTools, SLOT(msg(QString, float)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), objTools, SLOT(msg(QString, QString)));
    
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), terrainTools, SLOT(msg(QString, QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), geoTools, SLOT(msg(QString, QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), activityTools, SLOT(msg(QString, QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString)), activityTools, SLOT(msg(QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), activityEventWindow->eventProperties, SLOT(msg(QString, QString)));
    
    QObject::connect(statusWindow, SIGNAL(sendMsg(QString, QString)), glWidget, SLOT(msg(QString, QString)));


    QObject::connect(glWidget, SIGNAL(sendMsg(QString)), shapeViewWindow, SLOT(msg(QString)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, bool)), shapeViewWindow, SLOT(msg(QString, bool)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, int)), shapeViewWindow, SLOT(msg(QString, int)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, float)), shapeViewWindow, SLOT(msg(QString, float)));
    QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), shapeViewWindow, SLOT(msg(QString, QString)));
    
    QObject::connect(glWidget, SIGNAL(naviInfo(int, int)),
                      statusWindow, SLOT(naviInfo(int, int)));
    
    QObject::connect(glWidget, SIGNAL(posInfo(PreciseTileCoordinate*)),
                      statusWindow, SLOT(posInfo(PreciseTileCoordinate*)));
    
    QObject::connect(glWidget, SIGNAL(pointerInfo(float*)),
                      statusWindow, SLOT(pointerInfo(float*)));
    
    QObject::connect(glWidget, SIGNAL(mkrList(QMap<QString, Coords*>)),
                      statusWindow, SLOT(mkrList(QMap<QString, Coords*>)));
    
    QObject::connect(glWidget, SIGNAL(mkrList(QMap<QString, Coords*>)),
                      geoTools, SLOT(mkrList(QMap<QString, Coords*>)));
    
    QObject::connect(geoTools, SIGNAL(createNewTiles(QMap<int, QPair<int, int>*>)),
                      glWidget, SLOT(createNewTiles(QMap<int, QPair<int, int>*>)));
    
    QObject::connect(geoTools, SIGNAL(createNewLoTiles(QMap<int, QPair<int, int>*>)),
                      glWidget, SLOT(createNewLoTiles(QMap<int, QPair<int, int>*>)));
    
    QObject::connect(glWidget, SIGNAL(routeLoaded(Route*)),
                      objTools, SLOT(routeLoaded(Route*)));

    QObject::connect(glWidget, SIGNAL(routeLoaded(Route*)),
                      activityTools, SLOT(routeLoaded(Route*)));
    QObject::connect(glWidget, SIGNAL(routeLoaded(Route*)),
                      activityBuilderWindow, SLOT(routeLoaded(Route*)));
    QObject::connect(activityTools, SIGNAL(pathSelectionChanged(Path*)),
                      activityBuilderWindow, SLOT(setSelectedPath(Path*)));
    
    QObject::connect(objTools, SIGNAL(enableTool(QString)),
                      glWidget, SLOT(enableTool(QString)));

    QObject::connect(objTools, SIGNAL(userModeChanged()),
                      glWidget, SLOT(userModeChangeSound()));

    QObject::connect(objTools, SIGNAL(requestMainFocus()),
                      glWidget, SLOT(focusEditor()));
    
    QObject::connect(terrainTools, SIGNAL(enableTool(QString)),
                      glWidget, SLOT(enableTool(QString)));   
    
    QObject::connect(geoTools, SIGNAL(enableTool(QString)),
                      glWidget, SLOT(enableTool(QString)));   
    
    QObject::connect(activityTools, SIGNAL(enableTool(QString)),
                      glWidget, SLOT(enableTool(QString)));   
    
    QObject::connect(activityEventWindow->eventProperties, SIGNAL(enableTool(QString)),
                      glWidget, SLOT(enableTool(QString)));   
    
    
    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        QObject::connect(it, SIGNAL(enableTool(QString)),
            glWidget, SLOT(enableTool(QString)));   
        QObject::connect(it, SIGNAL(userButtonPressed()),
            glWidget, SLOT(userModeChangeSound()));
        QObject::connect(glWidget, SIGNAL(sendMsg(QString, QString)), 
                it, SLOT(msg(QString, QString)));
    }
    
    QObject::connect(glWidget, SIGNAL(flexData(int, int, float*)),
                      objProperties["Dyntrack"], SLOT(flexData(int, int, float*)));

    QObject::connect(objProperties["Dyntrack"], SIGNAL(flexResult(bool)),
                      glWidget, SLOT(flexResult(bool)));

    QObject::connect(objProperties["ActivityObject"], SIGNAL(sendMsg(QString)),
                      glWidget, SLOT(msg(QString)));
    
    QObject::connect(objProperties["ActivityConsist"], SIGNAL(cameraObject(GameObj*)),
                      glWidget, SLOT(setCameraObject(GameObj*)));
    
    QObject::connect(objProperties["TrackObj"], SIGNAL(setMoveStep(float)),
                      glWidget, SLOT(setMoveStep(float)));
    QObject::connect(objProperties["TrackObj"], SIGNAL(requestMainFocus()),
                      glWidget, SLOT(focusEditor()));
    QObject::connect(glWidget, SIGNAL(resetGradeHelperRequested()),
                      objProperties["TrackObj"], SLOT(resetGradeHelper()));
    
    QObject::connect(objProperties["Dyntrack"], SIGNAL(setMoveStep(float)),
                      glWidget, SLOT(setMoveStep(float)));
    
    QObject::connect(terrainTools, SIGNAL(setPaintBrush(Brush*)),
                      glWidget, SLOT(setPaintBrush(Brush*)));   
    QObject::connect(terrainTools, SIGNAL(mirrorSeasonAccepted()),
                      glWidget, SLOT(userModeChangeSound()));
    QObject::connect(terrainTools, SIGNAL(mirrorSeasonError()),
                      glWidget, SLOT(userErrorSound()));
    
    QObject::connect(glWidget, SIGNAL(setBrushTextureId(int)),
                      terrainTools, SLOT(setBrushTextureId(int)));   
    
    QObject::connect(statusWindow, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      glWidget, SLOT(jumpTo(PreciseTileCoordinate*)));
    QObject::connect(statusWindow, SIGNAL(jumpSoundRequested()),
                      glWidget, SLOT(userJumpSound()));
    QObject::connect(statusWindow, SIGNAL(requestMainFocus()),
                      glWidget, SLOT(focusEditor()));

    // Command buttons should never strand keyboard focus in an editor panel.
    // Input widgets are intentionally excluded so typing and dropdown use are
    // unaffected.
    restoreEditorFocusAfterButtons(objTools, glWidget);
    restoreEditorFocusAfterButtons(terrainTools, glWidget);
    restoreEditorFocusAfterButtons(geoTools, glWidget);
    restoreEditorFocusAfterButtons(box2, glWidget);
    restoreEditorFocusAfterButtons(statusWindow, glWidget);
    restoreEditorFocusAfterToolButtons(objTools, glWidget);
    restoreEditorFocusAfterToolButtons(statusWindow, glWidget);

    // Object Selection, Object Placement, and Control Panel push buttons
    // already have purpose-specific sound paths. Supply the same user-click
    // sound to panels which historically lacked one, plus the pin buttons.
    addUserClickSoundToButtons(terrainTools, glWidget);
    addUserClickSoundToButtons(geoTools, glWidget);
    addUserClickSoundToButtons(activityBuilderWindow, glWidget);
    addUserClickSoundToToolButtons(objTools, glWidget);
    addUserClickSoundToToolButtons(statusWindow, glWidget);
    
    QObject::connect(glWidget, SIGNAL(itemSelected(Ref::RefItem*)),
                      objTools, SLOT(itemSelected(Ref::RefItem*)));

    QObject::connect(glWidget, SIGNAL(showProperties(GameObj*)),
                      this, SLOT(showProperties(GameObj*)));
    
    QObject::connect(glWidget, SIGNAL(updateProperties(GameObj*)),
                      this, SLOT(updateProperties(GameObj*)));
    
    QObject::connect(this, SIGNAL(exitNow()),
                      aboutWindow, SLOT(exitNow())); 
    
    QObject::connect(statusWindow, SIGNAL(windowClosed()),
                      this, SLOT(statusWindowClosed()));     
    QObject::connect(statusWindow, SIGNAL(statusCommand(QString)),
                      glWidget, SLOT(statusPanelCommand(QString)));

    QObject::connect(errorMessagesWindow, SIGNAL(windowClosed()),
                      this, SLOT(errorMessagesWindowClosed())); 

    QObject::connect(shapeViewWindow, SIGNAL(windowClosed()),
                      this, SLOT(shapeVeiwWindowClosed())); 
    
    QObject::connect(glWidget, SIGNAL(setToolbox(QString)),
                      this, SLOT(setToolbox(QString)));
    
    QObject::connect(activityTools, SIGNAL(objectSelected(GameObj*)),
                      glWidget, SLOT(objectSelected(GameObj*)));
    
    QObject::connect(activityTools, SIGNAL(showActivityEventEditor()),
                      this, SLOT(showActivityEventEditor()));
    
    QObject::connect(activityTools, SIGNAL(showActivityServiceEditor()),
                      this, SLOT(showActivityServiceEditor()));
    
    QObject::connect(activityTools, SIGNAL(showActivityTrafficEditor()),
                      this, SLOT(showActivityTrafficEditor()));
    
    QObject::connect(activityTools, SIGNAL(showActivityTimetableEditor()),
                      this, SLOT(showActivityTimetableEditor()));
    
    QObject::connect(activityTools, SIGNAL(showEvents(Activity*)),
                      activityEventWindow, SLOT(showEvents(Activity*)));
    
    QObject::connect(activityTools, SIGNAL(showServices(Route*)),
                      activityServiceWindow, SLOT(showServices(Route*)));
    
    QObject::connect(activityTools, SIGNAL(showTraffic(Route*)),
                      activityTrafficWindow, SLOT(showTraffic(Route*)));
    
    QObject::connect(activityServiceWindow, SIGNAL(reloadServicesList()),
                      activityTools, SLOT(reloadServicesList()));
    
    QObject::connect(activityTrafficWindow, SIGNAL(reloadTrafficsList()),
                       activityTools, SLOT(reloadTrafficsList()));

    QObject::connect(activityTools, SIGNAL(showTimetable(Activity*)),
                      activityTimetableWindow, SLOT(showTimetable(Activity*)));
    
    QObject::connect(activityEventWindow->eventProperties, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      glWidget, SLOT(jumpTo(PreciseTileCoordinate*)));
    
    QObject::connect(activityTools, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      glWidget, SLOT(jumpTo(PreciseTileCoordinate*)));
    
    QObject::connect(errorMessagesWindow, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      glWidget, SLOT(jumpTo(PreciseTileCoordinate*)));
    
    QObject::connect(errorMessagesWindow, SIGNAL(selectObject(GameObj*)),
                      glWidget, SLOT(objectSelected(GameObj*)));
    
    QObject::connect(activityTools, SIGNAL(sendMsg(QString)), glWidget, SLOT(msg(QString)));
    
    QObject::connect(this, SIGNAL(reloadRefFile()),
                      glWidget, SLOT(reloadRefFile()));

    /// This connects the menu to the GLWidget where the work happens
    QObject::connect(this, SIGNAL(reloadMkrFile()),
                      glWidget, SLOT(reloadMkrFiles()));
    
    QObject::connect(glWidget, SIGNAL(refreshObjLists()),
                      objTools, SLOT(refreshObjLists()));
    
        if(Game::serverClient != NULL)
        QObject::connect(Game::serverClient, SIGNAL(refreshObjLists()),
                      objTools, SLOT(refreshObjLists()));
    
      /// Control Panel status updates
      QObject::connect(this, SIGNAL(updStatus(QString, QString)),     statusWindow, SLOT(recStatus(QString, QString)));   
      QObject::connect(glWidget, SIGNAL(updStatus(QString, QString)), statusWindow, SLOT(recStatus(QString, QString)));   
      QObject::connect(objTools, SIGNAL(updStatus(QString, QString)), statusWindow, SLOT(recStatus(QString, QString)));         
 
      /// EFO connect the status buttons to the other windows
      
      QObject::connect(glWidget, SIGNAL(preloadTexturesSignal()), terrainTools, SLOT(preloadTextures()));

      
}

void RouteEditorWindow::keyPressEvent(QKeyEvent *e) {

    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}

void RouteEditorWindow::exitToLoadWindow(){
        LoadWindow *loadWindow = new LoadWindow();

        QStringList winPos = Game::mainPos.split(","); 
        if(winPos.count() > 1) loadWindow->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());        
        loadWindow->show();        
}

void RouteEditorWindow::completeApplicationClose(QCloseEvent *event){
    saveLastSession();
    emit exitNow();
    event->accept();
    SoundManager::CloseAl();

    // Several editor helpers are independent Qt tool windows. Closing Main
    // must end the application even if one of those windows ignores close in
    // order to implement its normal hide-on-close behavior.
    QTimer::singleShot(0, qApp, [](){
        const QWidgetList windows = QApplication::topLevelWidgets();
        for(QWidget *window : windows){
            if(window != NULL)
                window->hide();
        }
        QCoreApplication::quit();
    });
}

void RouteEditorWindow::closeEvent(QCloseEvent * event ){
    QVector<QString> unsavedItems;
    glWidget->getUnsavedInfo(unsavedItems);
    
    /// EFO List missing shapes
    QFile file("./" + Game::route + "_missingShapes.txt");    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        QStringList sortedFileList = Route::missingList;
        sortedFileList.sort();
        for (const QString& fileName : sortedFileList) {
            out << fileName << " \n";
        }
        file.close();        
    }      
/*
    QFile file2("./" + Game::route + "_texturesUsed.txt");    
    if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file2);
        QStringList sortedFileList = Route::texturesList;
        sortedFileList.sort();
        for (const QString& fileName : sortedFileList) {
            out << fileName << " \n";
        }
        file.close();        
    }
 * */          
        
    if(unsavedItems.size() == 0){
        if(Game::debugOutput) qDebug() << "Nothing to Save";
        completeApplicationClose(event);
        return;
    }
   
    UnsavedDialog unsavedDialog;   /// EFO need to add the stwqc here when terrain and world are split
    unsavedDialog.setWindowTitle("Save changes?");
    unsavedDialog.setMsg("Save changes in route?");
    for(int i = 0; i < unsavedItems.size(); i++){
        unsavedDialog.items.addItem(unsavedItems[i]);
    }
    unsavedDialog.exec();
    if(unsavedDialog.changed == 0){
        event->ignore();
        return;
    }
    if(unsavedDialog.changed == 2){
        completeApplicationClose(event);
        return;
    }

    //// EFO  need to flesh this out for saving terrain and world separately
    save();


    completeApplicationClose(event);
    
}

void RouteEditorWindow::moveEvent(QMoveEvent *event){
    QMainWindow::moveEvent(event);
    if(pinMainWindowAction != NULL && pinMainWindowAction->isChecked() && !applyingWindowPosition)
        pinPositionTimer.start(240);
}

void RouteEditorWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    if(pinMainWindowAction != NULL && pinMainWindowAction->isChecked() && !applyingWindowPosition)
        pinPositionTimer.start(240);
}

void RouteEditorWindow::changeEvent(QEvent *event){
    QMainWindow::changeEvent(event);
    if(event->type() == QEvent::WindowStateChange && pinMainWindowAction != NULL
            && pinMainWindowAction->isChecked() && !applyingWindowPosition)
        pinPositionTimer.start(240);
}

void RouteEditorWindow::applyPinnedMainWindowPosition(){
    QRect pinnedGeometry;
    bool pinnedMaximized = false;
    if(!Game::pinnedWindowGeometry("mainWindow", &pinnedGeometry, &pinnedMaximized))
        return;
    applyingWindowPosition = true;
    setWindowState(windowState() & ~Qt::WindowMaximized);
    if(pinnedGeometry.width() > 0 && pinnedGeometry.height() > 0)
        resize(pinnedGeometry.size());
    move(Game::visibleWindowPosition(pinnedGeometry.topLeft(), size()));
    if(pinnedMaximized)
        setWindowState(windowState() | Qt::WindowMaximized);
    applyingWindowPosition = false;
}

void RouteEditorWindow::toggleMainWindowPositionPin(bool pinned){
    pinMainWindowAction->setToolTip(pinned
        ? tr("The Main Window position is saved between sessions. Unpin to return to centered placement.")
        : tr("Pin the current Main Window position between sessions."));
    if(pinned){
        Game::clearPinnedWindowPosition("mainWindowUseDefault");
        Game::savePinnedWindowGeometry("mainWindow", isMaximized() ? normalGeometry() : geometry(), isMaximized());
        return;
    }

    pinPositionTimer.stop();
    Game::clearPinnedWindowPosition("mainWindow");
    Game::savePinnedWindowPosition("mainWindowUseDefault", QPoint(0, 0));
    QScreen *screen = QApplication::primaryScreen();
    if(screen != NULL){
        applyingWindowPosition = true;
        setWindowState(windowState() & ~Qt::WindowMaximized);
        resize(1280, 800);
        const QRect available = screen->availableGeometry();
        const QPoint centered(available.left() + (available.width() - width()) / 2,
                              available.top() + (available.height() - height()) / 2);
        move(Game::visibleWindowPosition(centered, size()));
        applyingWindowPosition = false;
    }
}

void RouteEditorWindow::savePinnedMainWindowPosition(){
    if(pinMainWindowAction != NULL && pinMainWindowAction->isChecked() && !applyingWindowPosition)
        Game::savePinnedWindowGeometry("mainWindow", isMaximized() ? normalGeometry() : geometry(), isMaximized());
}

void RouteEditorWindow::saveLastSession(){
    QJsonObject root;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["root"] = Game::root;
    root["route"] = Game::route;

    QJsonObject mainWindow;
    QRect mainGeom = geometry();
    mainWindow["x"] = mainGeom.x();
    mainWindow["y"] = mainGeom.y();
    mainWindow["w"] = mainGeom.width();
    mainWindow["h"] = mainGeom.height();
    mainWindow["maximized"] = isMaximized();
    root["mainWindow"] = mainWindow;

    QJsonObject status;
    QRect statusGeom = statusWindow->geometry();
    status["x"] = statusGeom.x();
    status["y"] = statusGeom.y();
    status["w"] = statusGeom.width();
    status["h"] = statusGeom.height();
    status["visible"] = statusWindow->isVisible();
    root["controlPanel"] = status;

    root["camera"] = glWidget->getSessionCameraState();

    QFile file(Game::lastSessionFilePath());
    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void RouteEditorWindow::applyRestoredSessionGeometry(){
    pinPositionTimer.stop();
    applyingWindowPosition = true;
    statusWindow->setPositionPersistenceSuspended(true);
    QRect pinnedMainGeometry;
    bool pinnedMainMaximized = false;
    const bool mainWindowPinned =
        Game::pinnedWindowGeometry("mainWindow", &pinnedMainGeometry, &pinnedMainMaximized);
    if(mainWindowPinned){
        setWindowState(windowState() & ~Qt::WindowMaximized);
        if(pinnedMainGeometry.width() > 0 && pinnedMainGeometry.height() > 0){
            resize(pinnedMainGeometry.size());
            move(Game::visibleWindowPosition(pinnedMainGeometry.topLeft(), size()));
        }
        if(pinnedMainMaximized)
            setWindowState(windowState() | Qt::WindowMaximized);
    } else {
        showMaximized();
    }
    Game::restoreLastSessionWindowGeometry = false;

    if(Game::restoreStatusGeometry && Game::restoreStatusW > 0 && Game::restoreStatusH > 0){
        statusWindow->setGeometry(Game::restoreStatusX, Game::restoreStatusY, Game::restoreStatusW, Game::restoreStatusH);
        Game::restoreStatusGeometry = false;
    }
    statusWindow->setPositionPersistenceSuspended(false);
    applyingWindowPosition = false;
}

void RouteEditorWindow::save(){
    emit sendMsg(QString("save"));
    emit updStatus(QString("stat0"),QString("Saved"));    
}

void RouteEditorWindow::openRouteFolder(){
    QDir routeFolder(Game::root + "/routes/" + Game::route);
    if(!routeFolder.exists()){
        QMessageBox::warning(this, tr("Open Route Folder"),
                             tr("The active route folder could not be found:\n%1")
                             .arg(QDir::toNativeSeparators(routeFolder.absolutePath())));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(routeFolder.absolutePath()));
}

void RouteEditorWindow::reloadRef(){
    emit reloadRefFile();
}

void RouteEditorWindow::reloadMkr(){
    /// emit signal to GLW which fires off Route->something
    emit reloadMkrFile();    
    if(Game::debugOutput) qDebug() << "Menu triggered ->REW->emit reloadMkrFile";    

    
}

void RouteEditorWindow::restartAndRestore(){
    QVector<QString> unsavedItems;
    glWidget->getUnsavedInfo(unsavedItems);
    if(!unsavedItems.isEmpty()){
        QStringList pendingItems;
        for(const QString &item : unsavedItems)
            pendingItems.append(item);
        QMessageBox::warning(this, tr("Restart and Restore"),
                             tr("TSRE was not restarted because route changes are still pending.\n\n"
                                "Save the route changes first, then try Restart and Restore again.\n\n%1")
                             .arg(pendingItems.join("\n")));
        return;
    }

    if(!settingsDialog->save())
        return;

    saveLastSession();
    qApp->exit(Game::RestartAndRestoreExitCode);
}



void RouteEditorWindow::refreshErrors(){
    emit refreshErrorList(); 
}

void RouteEditorWindow::createPaths(){
    QMessageBox msgBox;
    msgBox.setText("This action will delete all your existing activity paths and create new simple paths! Continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    switch (msgBox.exec()) {
      case QMessageBox::Yes:
          emit sendMsg(QString("createPaths"));
          break;
      case QMessageBox::No:
          break;
      default:
          break;
    }
}

void RouteEditorWindow::terrainCamera(bool val){
    Game::cameraStickToTerrain = val;
}

void RouteEditorWindow::detailedTerrainEnabled(){
    emit this->sendMsg("editDetailedTerrain");
}

void RouteEditorWindow::distantTerrainEnabled(){
    emit this->sendMsg("editDistantTerrain");
}
    
void RouteEditorWindow::mstsShadows(bool val){
    Game::mstsShadows = val;
}

void RouteEditorWindow::about(){
    aboutWindow->show();
}

void RouteEditorWindow::showTerrainTreeEditr(){
    emit sendMsg(QString("showTerrainTreeEditr"));
}

void RouteEditorWindow::showToolsObject(bool show){
    if(show){
        hideShowToolWidget(true);
        setToolbox("objTools");
    } else {
        hideShowToolWidget(false);
    }
}

void RouteEditorWindow::showToolsObjectAndTerrain(bool show){
    if(show){
        hideShowToolWidget(true);
        hideAllTools();
        objTools->show();
        objectsAndTerrainAction->setChecked(true);
        terrainTools->show();
        box->setFixedWidth(scaledUiSize(500));
    } else {
        hideShowToolWidget(false);
    }
}

void RouteEditorWindow::showToolsTerrain(bool show){
    if(show){
        hideShowToolWidget(true);
        setToolbox("terrainTools");
    } else {
        hideShowToolWidget(false);
    }
}

void RouteEditorWindow::showToolsGeo(bool show){
    if(show){
        hideShowToolWidget(true);
        setToolbox("geoTools");
    } else {
        hideShowToolWidget(false);
    }
}

void RouteEditorWindow::showToolsActivity(bool show){
    if(show){
        if(Game::serverClient == NULL){
            hideShowToolWidget(false);
            activityBuilderWindow->showMaximized();
            activityBuilderWindow->raise();
            activityBuilderWindow->activateWindow();
            activityAction->setChecked(true);
        }
    } else {
        activityBuilderWindow->hide();
        activityAction->setChecked(false);
    }
}

void RouteEditorWindow::showActivityEventEditor(){
    activityEventWindow->show();
}

void RouteEditorWindow::showActivityServiceEditor(){
    activityServiceWindow->show();
}

void RouteEditorWindow::showActivityTrafficEditor(){
    activityTrafficWindow->show();
}

void RouteEditorWindow::showActivityTimetableEditor(){
    activityTimetableWindow->show();
}

void RouteEditorWindow::setToolbox(QString name){
    if(name == "objTools"){
        hideAllTools();
        objTools->show();
        objectsAction->setChecked(true);
    }
    if(name == "terrainTools"){
        hideAllTools();
        terrainTools->show();
        terrainAction->setChecked(true);       
    }
    if(name == "geoTools"){
        hideAllTools();
        geoTools->show();
        geoAction->setChecked(true);
    }
    if(name == "activityTools"){
        showToolsActivity(true);
    }
}

void RouteEditorWindow::hideAllTools(){
    objTools->hide();
    terrainTools->hide();
    geoTools->hide();
    objectsAction->setChecked(false);
    terrainAction->setChecked(false);     
    geoAction->setChecked(false);
    objectsAndTerrainAction->setChecked(false);
    box->setFixedWidth(scaledUiSize(250));
}

void RouteEditorWindow::showProperties(GameObj* obj){
    // hide all
    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        it->hide();
    }
    if(obj == NULL){
        propertiesPanelTitle->hide();
        return;
    }
    propertiesPanelTitle->show();
    // show 
    //qDebug() << obj->typeObj;

    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        if(!it->support(obj)) continue;
        it->showObj(obj);
        it->show();
        return;
    }
}

void RouteEditorWindow::updateProperties(GameObj* obj){
    if(obj == NULL) return;
    // show 

    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        if(it->isVisible() && it->support(obj)){
            it->updateObj(obj);
            return;
        }
    }
}

void RouteEditorWindow::hideShowPropertiesWidget(bool show){
    if(show) 
        { box2->show(); propertiesAction->setChecked(true); }
    else 
        { box2->hide(); propertiesAction->setChecked(false); }
}



void RouteEditorWindow::hideShowShapeViewWidget(bool show){
    if(show) shapeViewWindow->show();
    else shapeViewWindow->hide();
}

void RouteEditorWindow::hideShowErrorMsgWidget(bool show){
    if(show) {
        errorMessagesWindow->show();
    }
    else errorMessagesWindow->hide();
}

void RouteEditorWindow::hideShowStatWidget(bool show){
    if(show) { statusWindow->show();  }
    else { statusWindow->hide();  }
}

void RouteEditorWindow::hideShowSettingsDialog(bool show){
    if(show) { settingsDialog->show();  }
    else { settingsDialog->hide();  }
}


void RouteEditorWindow::hideShowToolWidget(bool show){
    if(show) { box->show();     }
    else     { box->hide();    }
}

void RouteEditorWindow::viewWorldGrid(bool show){
    Game::viewWorldGrid = show;
}
void RouteEditorWindow::viewTileGrid(bool show){
    Game::viewTileGrid = show;
}
void RouteEditorWindow::viewTerrainShape(bool show){
    Game::viewTerrainShape = !show;
}
void RouteEditorWindow::viewTerrainGrid(bool show){
    Game::viewTerrainGrid = show;
}
void RouteEditorWindow::showWorldObjPivotPointsEnabled(bool show){
    Game::showWorldObjPivotPoints = show;
}
void RouteEditorWindow::viewInteractives(bool show){
    Game::viewInteractives = show;
}
void RouteEditorWindow::viewForestRegions(bool show){
    Game::viewForestRegions = show;
}
void RouteEditorWindow::viewTrackDbLines(bool show){
    Game::viewTrackDbLines = show;
}
void RouteEditorWindow::viewTsectionLines(bool show){
    Game::viewTsectionLines = show;
}

void RouteEditorWindow::viewGradeSymbols(bool show){
    Game::gradeOverlayEnabled = show;
    ++Game::gradeOverlayRevision;
}

void RouteEditorWindow::viewTrackItems(bool show){
    Game::renderTrItems = show;
}

void RouteEditorWindow::viewPointer3d(bool show){
    Game::viewPointer3d = show;
}

void RouteEditorWindow::viewMarkers(bool show){
    Game::viewMarkers = show;
}

void RouteEditorWindow::viewSnapable(bool show){
    Game::viewSnapable = show;
}

void RouteEditorWindow::viewCompass(bool show){
    Game::viewCompass = show;
}

void RouteEditorWindow::showRoute(){
    if(Game::serverClient == NULL){
        if(!glWidget->initRoute()){
            emit exitNow();
            SoundManager::CloseAl();
            qApp->quit();
            return;
        }
        show();
    } else {
        QObject::connect(glWidget, SIGNAL(showWindow()), this, SLOT(show()));
        glWidget->initRoute();
    }
}

// EFO Move windows
void RouteEditorWindow::show(){
    if(!Game::playerMode){
        /// Control Panel is enabled by C; S remains accepted for older settings.
        if(Game::mainWindowLayout.toLower().contains("c") || Game::mainWindowLayout.toLower().contains("s"))
         {
             statusWindow->show();
             statAction->setChecked(true);
         }
         QStringList winPos = Game::statusPos.split(",");
         QPoint pinnedControlPosition;
         const bool controlPositionPinned = Game::pinnedWindowPosition("controlPanel", &pinnedControlPosition);
         const bool controlDefaultRequested = Game::pinnedWindowPosition("controlPanelUseDefault", NULL);
        
          if(!controlPositionPinned && (controlDefaultRequested || winPos.count() < 2))
          {            
            const int naviTemp1 = this->x() - 300;  // left of window 
            const int naviTemp2 = this->y() + 200;  // 200 from the top corner
            statusWindow->move(std::max(0,naviTemp1) , std::min(naviTemp2,QApplication::primaryScreen()->geometry().bottom()-200));
          }            
    }
    
    if(Game::lockCamera == true) emit updStatus(QString("camera"),QString("Camera Locked")); else emit updStatus(QString("camera"),QString("Camera Unlocked"));
    
    QMainWindow::show();
    applyRestoredSessionGeometry();
}

void RouteEditorWindow::statusWindowClosed(){
    statAction->blockSignals(true);
    statAction->setChecked(false);
    statAction->blockSignals(false);
}


void RouteEditorWindow::errorMessagesWindowClosed(){
    errorViewAction->blockSignals(true);
    errorViewAction->setChecked(false);
    errorViewAction->blockSignals(false);
}

void RouteEditorWindow::shapeVeiwWindowClosed(){
    shapeViewAction->blockSignals(true);
    shapeViewAction->setChecked(false);
    shapeViewAction->blockSignals(false);
}

void RouteEditorWindow::viewToggleAll(){
    if(!viewAllOff){
        savedViewSelections.clear();
        foreach(QAction *action, viewMenu->actions()){
            if(!action->isCheckable() || action == vViewCompass)
                continue;
            savedViewSelections.insert(action, action->isChecked());
            action->setChecked(false);
            action->triggered(false);
        }
        viewAllOff = true;
        return;
    }

    for(auto it = savedViewSelections.constBegin(); it != savedViewSelections.constEnd(); ++it){
        QAction *action = it.key();
        if(action == NULL)
            continue;
        action->setChecked(it.value());
        action->triggered(it.value());
    }
    viewAllOff = false;
}
//void Window::exitNow(){
//    this->hide();
//}

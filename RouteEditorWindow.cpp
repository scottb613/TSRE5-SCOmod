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
#include <QSaveFile>
#include <QSet>
#include <QDateTime>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QTextCursor>
#include <QUrl>
#include "RouteEditorGLWidget.h"
#include "RouteEditorWindow.h"
#include "Game.h"
#include "MapWindow.h"
#include "TerrainLib.h"
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
#include "PropertiesAbstract.h"
#include "PropertiesUndefined.h"
#include "PropertiesStatic.h"
#include "PropertiesPolyVegBake.h"
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
#include "TexLib.h"
#include "PropertiesHazard.h"
#include "PolyVegHelper.h"
#include "PolyVegSchemaEditor.h"
#include "TerrainWaterWindow2.h"

static int scaledUiSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
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
    terrainTextureTools = terrainTools->texturePanel();
    geoTools = new GeoTools("GeoTools");
    activityTools = new ActivityTools("ActivityTools");
    activityBuilderWindow = new ActivityBuilderWindow(activityTools, this);
    //naviBox = new NaviBox();
    glWidget = new RouteEditorGLWidget(this);
    waterHelper = new TerrainWaterWindow2(this);
    polyVegHelper = new PolyVegHelper(this);
    polyVegSchemaEditor = new PolyVegSchemaEditor(this);
    QObject::connect(polyVegHelper, &PolyVegHelper::settingsChanged,
                     glWidget, &RouteEditorGLWidget::setPolyVegSettings);
    QObject::connect(polyVegHelper, &PolyVegHelper::bakeRequested,
                     glWidget, &RouteEditorGLWidget::bakeVegetationCurrentTile);
    QObject::connect(polyVegHelper, &PolyVegHelper::bakeAllRequested,
                     glWidget, &RouteEditorGLWidget::bakeAllVegetation);
    QObject::connect(polyVegHelper, &PolyVegHelper::countsRequested,
                     glWidget, &RouteEditorGLWidget::refreshPolyVegTileCounts);
    QObject::connect(polyVegHelper, &PolyVegHelper::placeRulerRequested,
                     glWidget, &RouteEditorGLWidget::placePolyVegRuler);
    QObject::connect(polyVegHelper, &PolyVegHelper::removeRulerRequested,
                     glWidget, &RouteEditorGLWidget::removePolyVegRuler);
    QObject::connect(polyVegHelper, &PolyVegHelper::addRulerPointsRequested,
                     glWidget, &RouteEditorGLWidget::addPolyVegRulerPoints);
    QObject::connect(polyVegHelper, &PolyVegHelper::editRulerPointsRequested,
                     glWidget, &RouteEditorGLWidget::editPolyVegRulerPoints);
    QObject::connect(polyVegHelper, &PolyVegHelper::rulerAreaChanged,
                     glWidget, &RouteEditorGLWidget::setPolyVegRulerArea);
    QObject::connect(polyVegHelper, &PolyVegHelper::rulerWidthChanged,
                     glWidget, &RouteEditorGLWidget::setPolyVegRulerWidth);
    QObject::connect(polyVegHelper, &PolyVegHelper::plantRulerRequested,
                     glWidget, &RouteEditorGLWidget::plantPolyVegRuler);
    QObject::connect(polyVegHelper, &PolyVegHelper::jumpRawRequested,
                     glWidget, &RouteEditorGLWidget::jumpNextPolyVegRawTile);
    QObject::connect(polyVegHelper, &PolyVegHelper::resetRawJumpRequested,
                     glWidget, &RouteEditorGLWidget::resetPolyVegRawJump);
    QObject::connect(polyVegHelper, &PolyVegHelper::jumpBakeRequested,
                     glWidget, &RouteEditorGLWidget::jumpNextPolyVegBakeTile);
    QObject::connect(polyVegHelper, &PolyVegHelper::resetBakeJumpRequested,
                     glWidget, &RouteEditorGLWidget::resetPolyVegBakeJump);
    QObject::connect(glWidget, &RouteEditorGLWidget::polyVegHelperRequested,
                     this, [this](){ showPolyVegHelper(true); });
    QObject::connect(glWidget, &RouteEditorGLWidget::polyVegTileCounts,
                     polyVegHelper, &PolyVegHelper::setTileCounts);
    QObject::connect(glWidget, &RouteEditorGLWidget::polyVegPanelStatus,
                     this, [this](const QString &text){
        if(polyVegDock != NULL && polyVegDock->isVisible())
            polyVegHelper->setStatus(text);
    });
    QObject::connect(polyVegHelper, &PolyVegHelper::schemaEditorRequested,
                     this, [this](){ showPolyVegSchemaEditor(true); });
    QObject::connect(polyVegSchemaEditor, &PolyVegSchemaEditor::schemaSaved,
                     polyVegHelper, &PolyVegHelper::openForCurrentRoute);
    QObject::connect(polyVegSchemaEditor,
                     &PolyVegSchemaEditor::exitToPlanterRequested,
                     this, [this]() {
        showPolyVegSchemaEditor(false);
        if(polyVegHelperAction != NULL)
            polyVegHelperAction->setChecked(true);
        else
            showPolyVegHelper(true);
    });
    QObject::connect(polyVegSchemaEditor, &PolyVegSchemaEditor::userToggleSoundRequested,
                     glWidget, &RouteEditorGLWidget::userPanelToggleSound);
    QObject::connect(polyVegSchemaEditor, &PolyVegSchemaEditor::userButtonSoundRequested,
                     glWidget, &RouteEditorGLWidget::userModeChangeSound);
    QObject::connect(waterHelper, &TerrainWaterWindow2::placeRulerRequested,
                     glWidget, &RouteEditorGLWidget::placeWaterRuler);
    QObject::connect(waterHelper, &TerrainWaterWindow2::removeRulerRequested,
                     glWidget, &RouteEditorGLWidget::removeWaterRuler);
    QObject::connect(waterHelper, &TerrainWaterWindow2::addPointsRequested,
                     glWidget, &RouteEditorGLWidget::addWaterRulerPoints);
    QObject::connect(waterHelper, &TerrainWaterWindow2::editPointsRequested,
                     glWidget, &RouteEditorGLWidget::editWaterRulerPoints);
    QObject::connect(waterHelper, &TerrainWaterWindow2::scanRequested,
                     glWidget, &RouteEditorGLWidget::scanWaterRuler);
    QObject::connect(waterHelper, &TerrainWaterWindow2::adjustTerrainRequested,
                     glWidget, &RouteEditorGLWidget::adjustWaterTerrain);
    QObject::connect(waterHelper, &TerrainWaterWindow2::undoScanRequested,
                     glWidget, &RouteEditorGLWidget::undoWaterScan);
    QObject::connect(waterHelper, &TerrainWaterWindow2::userButtonPressed,
                     glWidget, &RouteEditorGLWidget::userModeChangeSound);
    QObject::connect(glWidget, &RouteEditorGLWidget::waterPanelStatus,
                     this, [this](const QString &text){
        if(waterHelperDock != NULL && waterHelperDock->isVisible())
            waterHelper->setStatus(text);
    });
    QObject::connect(glWidget, &RouteEditorGLWidget::waterPanelProgress,
                     this, [this](int value, int maximum, const QString &text){
        if(waterHelperDock != NULL && waterHelperDock->isVisible())
            waterHelper->setProgress(value, maximum, text);
    });
    QObject::connect(glWidget, &RouteEditorGLWidget::waterRulerPlacementRequested,
                     this, [this](){
        showWaterHelper(true);
        waterHelper->activateRuler();
    });
    QObject::connect(glWidget, &RouteEditorGLWidget::polyVegRulerPlacementRequested,
                     this, [this](){
        showPolyVegHelper(true);
        polyVegHelper->activateRuler();
    });
    
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
    
    objProperties["PolyVegBake"] = new PropertiesPolyVegBake;
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
    box2->setMaximumWidth(scaledUiSize(220));
    box2->setMinimumWidth(scaledUiSize(220));

    // Make the embedded-panel ownership and Qt::Widget type explicit before
    // layout. This prevents future window-style changes from treating these
    // panels as independent native surfaces.
    objTools->setParent(box, Qt::Widget);
    terrainTools->setParent(box, Qt::Widget);
    terrainTextureTools->setParent(box, Qt::Widget);
    geoTools->setParent(box, Qt::Widget);
    foreach(PropertiesAbstract *propertyPanel, objProperties){
        if(propertyPanel != NULL)
            propertyPanel->setParent(box2, Qt::Widget);
    }

    //box2->setMaximumWidth(250);
    //box2->setMinimumWidth(250);
    QHBoxLayout *mainLayout2 = new QHBoxLayout; 
    mainLayout2->setContentsMargins(0, 0, 0, 0);
    mainLayout2->setSpacing(0);
    mainLayout2->setContentsMargins(0,0,0,0);
    mainLayout2->addWidget(objTools);
    mainLayout2->addWidget(terrainTools);
    mainLayout2->addWidget(terrainTextureTools);
    mainLayout2->addWidget(geoTools);
    //mainLayout2->addWidget(naviBox);
    //mainLayout2->setAlignment(naviBox, Qt::AlignBottom);
    box->setLayout(mainLayout2);
    
    
    QVBoxLayout *mainLayout3 = new QVBoxLayout;
    mainLayout3->setContentsMargins(3,3,3,3);
    mainLayout3->setSpacing(3);
    mainLayout2->setContentsMargins(0, 0, 0, 0);
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

    PropertiesTerrain *terrainProperties =
        qobject_cast<PropertiesTerrain*>(objProperties["Terrain"]);
    PropertiesTrackObj *trackProperties =
        qobject_cast<PropertiesTrackObj*>(objProperties["TrackObj"]);
    if(terrainProperties != NULL){
        QObject::connect(glWidget, &RouteEditorGLWidget::naviInfo,
                         terrainProperties, &PropertiesTerrain::naviInfo);
        if(trackProperties != NULL){
            QObject::connect(terrainProperties, &PropertiesTerrain::hacksToggled,
                             trackProperties, &PropertiesTrackObj::toggleHacksForSelection);
        }
    }
    PropertiesStatic *staticProperties =
        qobject_cast<PropertiesStatic*>(objProperties["Static"]);
    if(staticProperties != NULL && trackProperties != NULL){
        QObject::connect(staticProperties, &PropertiesStatic::hacksToggled,
                         trackProperties, &PropertiesTrackObj::toggleHacksForSelection);
    }
    PropertiesPolyVegBake *polyVegBakeProperties =
        qobject_cast<PropertiesPolyVegBake*>(objProperties["PolyVegBake"]);
    if(polyVegBakeProperties != NULL && trackProperties != NULL){
        QObject::connect(polyVegBakeProperties,
                         &PropertiesPolyVegBake::hacksToggled,
                         trackProperties,
                         &PropertiesTrackObj::toggleHacksForSelection);
    }
    PropertiesSignal *signalProperties =
        qobject_cast<PropertiesSignal*>(objProperties["Signal"]);
    if(signalProperties != NULL && trackProperties != NULL){
        QObject::connect(signalProperties, &PropertiesSignal::hacksToggled,
                         trackProperties, &PropertiesTrackObj::toggleHacksForSelection);
    }
    if(trackProperties != NULL){
        QObject::connect(trackProperties,
                         &PropertiesTrackObj::resetRouteTerrtexRequested,
                         terrainTools, &TerrainTools::resetRouteTerrtexPaint);
        QObject::connect(trackProperties,
                         &PropertiesTrackObj::disableRouteWaterRequested,
                         terrainTools, &TerrainTools::disableRouteWaterTiles);
        QObject::connect(trackProperties,
                         &PropertiesTrackObj::deleteAllPolyVegBakesRequested,
                         glWidget, &RouteEditorGLWidget::deleteAllPolyVegBakes);
    }
    
    //mainLayout3->addWidget(terrainTools);
    //mainLayout3->setAlignment(naviBox, Qt::AlignBottom);
    box2->setLayout(mainLayout3);
    tuneScaledPanel(box2);

    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(3, 3, 3, 3);
    mainLayout->setSpacing(3);
    
    mainLayout->addWidget(box2);
    mainLayout->addWidget(glWidget);
    mainLayout->addWidget(box);
    
    remain->setLayout(mainLayout);
    mainLayout->setContentsMargins(0,0,0,0);
    
    this->setCentralWidget(remain);
    autoPlacementDock = new QDockWidget(tr("AUTO PLACE"), this);
    autoPlacementDock->setObjectName("autoPlacementDock");
    autoPlacementDock->setAllowedAreas(Qt::RightDockWidgetArea);
    autoPlacementDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *autoPlacementDockTitle = new QWidget(autoPlacementDock);
    autoPlacementDockTitle->setFixedHeight(0);
    autoPlacementDock->setTitleBarWidget(autoPlacementDockTitle);
    autoPlacementDock->setFixedWidth(scaledUiSize(350));
    QScrollArea *autoPlacementScroll = new QScrollArea(autoPlacementDock);
    autoPlacementScroll->setObjectName("autoPlacementScroll");
    autoPlacementScroll->setWidgetResizable(true);
    autoPlacementScroll->setFrameShape(QFrame::NoFrame);
    autoPlacementScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    autoPlacementScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QWidget *autoPlacementPanel = objTools->autoPlacementPanel();
    autoPlacementPanel->setParent(autoPlacementScroll, Qt::Widget);
    autoPlacementScroll->setWidget(autoPlacementPanel);
    autoPlacementDock->setWidget(autoPlacementScroll);
    addDockWidget(Qt::RightDockWidgetArea, autoPlacementDock);
    autoPlacementDock->hide();
    QObject::connect(autoPlacementDock, &QDockWidget::visibilityChanged,
                     this, [this](bool visible){
        if(visible){
            if(polyVegDock != NULL && polyVegDock->isVisible())
                polyVegDock->hide();
            if(waterHelperDock != NULL && waterHelperDock->isVisible())
                waterHelperDock->hide();
            if(box != NULL)
                box->hide();
            if(activityBuilderWindow != NULL && activityBuilderWindow->isVisible())
                activityBuilderWindow->hide();
        }
        if(autoPlacementAction != NULL){
            const QSignalBlocker blocker(autoPlacementAction);
            autoPlacementAction->setChecked(visible);
        }
        objTools->autoPlacementPanelVisibilityChanged(visible);
    });

    polyVegDock = new QDockWidget(tr("POLYVEG PLANTER"), this);
    polyVegDock->setObjectName("polyVegDock");
    polyVegDock->setAllowedAreas(Qt::RightDockWidgetArea);
    polyVegDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *polyVegDockTitle = new QWidget(polyVegDock);
    polyVegDockTitle->setFixedHeight(0);
    polyVegDock->setTitleBarWidget(polyVegDockTitle);
    polyVegDock->setFixedWidth(scaledUiSize(350));
    QScrollArea *polyVegScroll = new QScrollArea(polyVegDock);
    polyVegScroll->setObjectName("polyVegScroll");
    polyVegScroll->setWidgetResizable(true);
    polyVegScroll->setFrameShape(QFrame::NoFrame);
    polyVegScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    polyVegScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    polyVegHelper->setParent(polyVegScroll, Qt::Widget);
    polyVegScroll->setWidget(polyVegHelper);
    polyVegDock->setWidget(polyVegScroll);
    addDockWidget(Qt::RightDockWidgetArea, polyVegDock);
    polyVegDock->hide();
    QObject::connect(polyVegDock, &QDockWidget::visibilityChanged,
                     glWidget, &RouteEditorGLWidget::setPolyVegHelperVisible);
    QObject::connect(polyVegDock, &QDockWidget::visibilityChanged,
                     this, [this](bool visible){
        if(visible){
            if(autoPlacementDock != NULL && autoPlacementDock->isVisible())
                autoPlacementDock->hide();
            if(waterHelperDock != NULL && waterHelperDock->isVisible())
                waterHelperDock->hide();
            if(box != NULL)
                box->hide();
            if(activityBuilderWindow != NULL && activityBuilderWindow->isVisible())
                activityBuilderWindow->hide();
        }
        if(polyVegHelperAction != NULL){
            const QSignalBlocker blocker(polyVegHelperAction);
            polyVegHelperAction->setChecked(visible);
        }
        if(!visible) {
            polyVegHelper->clearPlacementTools();
            glWidget->removePolyVegRuler();
        }
    });

    waterHelperDock = new QDockWidget(tr("WATER TOOLS"), this);
    waterHelperDock->setObjectName("waterHelperDock");
    waterHelperDock->setAllowedAreas(Qt::RightDockWidgetArea);
    waterHelperDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *waterHelperDockTitle = new QWidget(waterHelperDock);
    waterHelperDockTitle->setFixedHeight(0);
    waterHelperDock->setTitleBarWidget(waterHelperDockTitle);
    waterHelperDock->setFixedWidth(scaledUiSize(250));
    QScrollArea *waterHelperScroll = new QScrollArea(waterHelperDock);
    waterHelperScroll->setObjectName("waterHelperScroll");
    waterHelperScroll->setWidgetResizable(true);
    waterHelperScroll->setFrameShape(QFrame::NoFrame);
    waterHelperScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    waterHelperScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    waterHelper->setParent(waterHelperScroll, Qt::Widget);
    waterHelperScroll->setWidget(waterHelper);
    waterHelperDock->setWidget(waterHelperScroll);
    addDockWidget(Qt::RightDockWidgetArea, waterHelperDock);
    waterHelperDock->hide();
    QObject::connect(waterHelperDock, &QDockWidget::visibilityChanged,
                     this, [this](bool visible){
        if(visible){
            if(autoPlacementDock != NULL && autoPlacementDock->isVisible())
                autoPlacementDock->hide();
            if(polyVegDock != NULL && polyVegDock->isVisible())
                polyVegDock->hide();
            if(box != NULL)
                box->hide();
            if(activityBuilderWindow != NULL && activityBuilderWindow->isVisible())
                activityBuilderWindow->hide();
            waterHelper->setStatus("Ready.");
        } else {
            glWidget->removeWaterRuler();
            waterHelper->resetSession();
        }
        if(waterHelperAction != NULL){
            const QSignalBlocker blocker(waterHelperAction);
            waterHelperAction->setChecked(visible);
        }
    });
    setWindowTitle(Game::AppName+" "+Game::AppVersion+" Route Editor");
    QFont menuFont = menuBar()->font();
    if(menuFont.pointSizeF() > 0)
        menuFont.setPointSizeF(menuFont.pointSizeF() * qBound(0.75f, Game::uiScale, 1.25f));
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

    routeHealthReportAction = new QAction(tr("Create Route &Health Report"), this);
    routeHealthReportAction->setToolTip(
            tr("Save and immediately display missing-content and route-usage information."));
    QObject::connect(routeHealthReportAction, SIGNAL(triggered()),
                     this, SLOT(createRouteHealthReport()));

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
    // This experimental action is not exposed in the menu and its legacy
    // rebuildTDB slot no longer exists. Keep the placeholder disconnected
    // until a reviewed GenX rebuild workflow is restored.
    
    
    if(Game::serverClient == NULL){
        routeMenu = menuBar()->addMenu(tr("&Route"));
        routeMenu->addAction(saveAction);
        routeMenu->addAction(openRouteFolderAction);
        routeMenu->addAction(routeHealthReportAction);
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
    QObject::connect(glWidget, &RouteEditorGLWidget::terrainShapeToggleRequested,
                     vViewTerrainShape, &QAction::trigger);
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
    propertiesAction->setShortcut(QKeySequence("Shift+F1"));
    QObject::connect(propertiesAction, SIGNAL(triggered(bool)), this, SLOT(hideShowPropertiesWidget(bool)));

    statAction = GuiFunct::newMenuCheckAction(tr("Control &Panel"), this, false); 
    statAction->setShortcut(QKeySequence("Ctrl+F1"));
    QObject::connect(statAction, SIGNAL(triggered(bool)), this, SLOT(hideShowStatWidget(bool)));
    
    settingsAction = GuiFunct::newMenuCheckAction(tr("S&ettings Window"), this, false); 
    settingsAction->setShortcut(QKeySequence("F12"));
    settingsAction->setShortcutContext(Qt::ApplicationShortcut);
    settingsAction->setToolTip(tr("Settings (F12)"));
    QObject::connect(settingsAction, SIGNAL(triggered(bool)), this, SLOT(hideShowSettingsDialog(bool)));
    QObject::connect(settingsDialog, &QDialog::finished, this, [this](int){
        settingsAction->setChecked(false);
    });

    errorViewAction = GuiFunct::newMenuCheckAction(tr("Errors && Messages"), this, false);
    errorViewAction->setShortcut(QKeySequence("F11"));
    errorViewAction->setToolTip(tr("Errors & Messages (F11)"));
    QObject::connect(errorViewAction, SIGNAL(triggered(bool)), this, SLOT(hideShowErrorMsgWidget(bool)));

    shapeViewAction = GuiFunct::newMenuCheckAction(tr("&Shape View Window"), this, false); 
    QObject::connect(shapeViewAction, SIGNAL(triggered(bool)), this, SLOT(hideShowShapeViewWidget(bool)));
    objectsAction = GuiFunct::newMenuCheckAction(tr("&Objects"), this); 
    objectsAction->setShortcut(QKeySequence("F1"));
    QObject::connect(objectsAction, SIGNAL(triggered(bool)), this, SLOT(showToolsObject(bool)));
    terrainAction = GuiFunct::newMenuCheckAction(tr("Terrain &Mesh"), this);
    terrainAction->setChecked(false);    
    terrainAction->setShortcut(QKeySequence("F2"));
    QObject::connect(terrainAction, SIGNAL(triggered(bool)), this, SLOT(showToolsTerrain(bool)));
    terrainTextureAction = GuiFunct::newMenuCheckAction(tr("Terrain &Texture"), this);
    terrainTextureAction->setChecked(false);
    terrainTextureAction->setShortcut(QKeySequence("F3"));
    QObject::connect(terrainTextureAction, SIGNAL(triggered(bool)),
                     this, SLOT(showToolsTerrainTexture(bool)));
    geoAction = GuiFunct::newMenuCheckAction(tr("&Geo"), this); 
    geoAction->setChecked(false);    
    geoAction->setShortcut(QKeySequence("F4"));
    QObject::connect(geoAction, SIGNAL(triggered(bool)), this, SLOT(showToolsGeo(bool)));
    activityAction = GuiFunct::newMenuCheckAction(tr("&Activity"), this); 
    activityAction->setChecked(false);
    activityAction->setShortcut(QKeySequence("F10"));
    QObject::connect(activityAction, SIGNAL(triggered(bool)), this, SLOT(showToolsActivity(bool)));
    QObject::connect(activityBuilderWindow, SIGNAL(visibilityChanged(bool)),
                     activityAction, SLOT(setChecked(bool)));
    QObject::connect(activityBuilderWindow, SIGNAL(userToggleSoundRequested()),
                     glWidget, SLOT(userPanelToggleSound()));
    QObject::connect(activityBuilderWindow, SIGNAL(userPlacementSoundRequested()),
                     glWidget, SLOT(userPlacementSound()));
    QObject::connect(activityBuilderWindow, SIGNAL(userErrorSoundRequested()),
                     glWidget, SLOT(userErrorSound()));
    autoPlacementAction = GuiFunct::newMenuCheckAction(tr("Auto &Place"), this, false);
    autoPlacementAction->setShortcut(QKeySequence("F5"));
    QObject::connect(autoPlacementAction, &QAction::toggled,
                     this, [this](bool visible){
        if(autoPlacementDock == NULL)
            return;
        if(visible){
            autoPlacementDock->show();
            autoPlacementDock->raise();
        } else {
            autoPlacementDock->hide();
        }
    });
    QObject::connect(autoPlacementAction, &QAction::triggered,
                     glWidget, &RouteEditorGLWidget::userPanelToggleSound);
    polyVegHelperAction = GuiFunct::newMenuCheckAction(tr("PolyVeg Planter"), this, false);
    polyVegHelperAction->setShortcut(QKeySequence("F6"));
    QObject::connect(polyVegHelperAction, &QAction::toggled,
                     this, [this](bool visible){
        if(visible)
            glWidget->requestPolyVegHelper();
        else
            showPolyVegHelper(false);
    });
    polyVegSchemaEditorAction = GuiFunct::newMenuCheckAction(
        tr("PolyVeg Schema"), this, false);
    polyVegSchemaEditorAction->setShortcut(QKeySequence("Shift+F6"));
    QObject::connect(polyVegSchemaEditorAction, &QAction::toggled,
                     this, &RouteEditorWindow::showPolyVegSchemaEditor);
    QObject::connect(polyVegSchemaEditor, &PolyVegSchemaEditor::visibilityChanged,
                     polyVegSchemaEditorAction, &QAction::setChecked);
    waterHelperAction = GuiFunct::newMenuCheckAction(
        tr("Water Tools"), this, false);
    waterHelperAction->setShortcut(QKeySequence("F7"));
    QObject::connect(waterHelperAction, &QAction::toggled,
                     this, &RouteEditorWindow::showWaterHelper);
    QObject::connect(waterHelperAction, &QAction::triggered,
                     glWidget, &RouteEditorGLWidget::userPanelToggleSound);

    toolsMenu->addAction(objectsAction);
    toolsMenu->addAction(propertiesAction);
    toolsMenu->addAction(statAction);
    toolsMenu->addAction(terrainAction);
    toolsMenu->addAction(terrainTextureAction);
    toolsMenu->addAction(geoAction);
    toolsMenu->addAction(autoPlacementAction);
    toolsMenu->addAction(polyVegHelperAction);
    toolsMenu->addAction(polyVegSchemaEditorAction);
    toolsMenu->addAction(waterHelperAction);
    toolsMenu->addAction(activityAction);
    toolsMenu->addAction(errorViewAction);
    toolsMenu->addAction(settingsAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(shapeViewAction);

    QActionGroup *toolModeActions = new QActionGroup(this);
    toolModeActions->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    toolModeActions->addAction(objectsAction);
    toolModeActions->addAction(terrainAction);
    toolModeActions->addAction(terrainTextureAction);
    toolModeActions->addAction(geoAction);
    toolModeActions->addAction(activityAction);
    toolModeActions->addAction(autoPlacementAction);
    toolModeActions->addAction(polyVegHelperAction);
    toolModeActions->addAction(polyVegSchemaEditorAction);
    toolModeActions->addAction(waterHelperAction);

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

    routeNameLabel = new QLabel(menuBar());
    routeNameLabel->setText(Game::routeName.trimmed().isEmpty()
                            ? Game::route : Game::routeName);
    routeNameLabel->setAlignment(Qt::AlignCenter);
    routeNameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    routeNameLabel->setStyleSheet(
        "QLabel { color: #707477; background: transparent; font-weight: normal; }");
    glWidget->installEventFilter(this);
    menuBar()->installEventFilter(this);
    QTimer::singleShot(0, this, [this](){ positionRouteNameLabel(); });
    
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
    restoreEditorFocusAfterButtons(terrainTextureTools, glWidget);
    restoreEditorFocusAfterButtons(geoTools, glWidget);
    restoreEditorFocusAfterButtons(box2, glWidget);
    restoreEditorFocusAfterButtons(statusWindow, glWidget);
    restoreEditorFocusAfterButtons(polyVegHelper, glWidget);
    restoreEditorFocusAfterToolButtons(objTools, glWidget);
    restoreEditorFocusAfterToolButtons(statusWindow, glWidget);

    // Object Selection, Object Placement, and Control Panel push buttons
    // already have purpose-specific sound paths. Supply the same user-click
    // sound to panels which historically lacked one, plus the pin buttons.
    addUserClickSoundToButtons(terrainTools, glWidget);
    addUserClickSoundToButtons(terrainTextureTools, glWidget);
    addUserClickSoundToButtons(geoTools, glWidget);
    addUserClickSoundToButtons(activityBuilderWindow, glWidget);
    addUserClickSoundToButtons(polyVegHelper, glWidget);
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
    QObject::connect(glWidget, &RouteEditorGLWidget::primaryEditorToolsEnabled,
                     statusWindow, &StatusWindow::setPrimaryEditorToolsEnabled);

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
    saveLastSession();
    hideRouteSessionWindows();
    hide();
    // The legacy editor contains reparented tool windows and OpenGL resources
    // that cannot yet be destroyed safely as one in-process QObject tree.
    // Restarting lets the OS reclaim that tree and guarantees Main Load begins
    // with no route, renderer, texture, or compass state from this process.
    qApp->exit(Game::RestartToMainLoadExitCode);
}

void RouteEditorWindow::hideRouteSessionWindows(){
    if(polyVegDock != NULL)
        polyVegDock->hide();
    if(waterHelperDock != NULL)
        waterHelperDock->hide();
    const QWidgetList windows = QApplication::topLevelWidgets();
    for(QWidget *window : windows){
        if(window == NULL || window == this)
            continue;

        // Qt::Tool windows can be top-level visually while retaining a nested
        // QObject owner (for example RouteEditor -> ObjTools -> Auto Place).
        // Follow that full ownership chain instead of checking only the direct
        // QWidget parent so no editor helper can remain beside Main Load.
        bool belongsToEditor = false;
        for(QObject *owner = window->parent(); owner != NULL; owner = owner->parent()){
            if(owner == this){
                belongsToEditor = true;
                break;
            }
        }

        if(belongsToEditor || window->parentWidget() == this || isAncestorOf(window))
            window->hide();
    }
}

void RouteEditorWindow::completeEditorClose(QCloseEvent *event){
    saveLastSession();
    hideRouteSessionWindows();
    hide();
    event->ignore();
    qApp->exit(Game::RestartToMainLoadExitCode);
}

void RouteEditorWindow::closeEvent(QCloseEvent * event ){
    QVector<QString> unsavedItems;
    glWidget->getUnsavedInfo(unsavedItems);
    auto discardUnsavedBakeFiles = [this, event]() {
        QString cleanupError;
        if(glWidget->discardUnsavedPolyVegBakeFiles(cleanupError))
            return true;
        GuiFunct::showEditorStopped(this, tr("Discard Route Changes Failed"),
            tr("TSRE could not restore the pre-bake generated files, so the "
               "editor will remain open. No additional route save was "
               "attempted.\n\n%1").arg(cleanupError));
        event->ignore();
        return false;
    };
    
    if(unsavedItems.size() == 0){
        if(Game::debugOutput) qDebug() << "Nothing to Save";
        if(!discardUnsavedBakeFiles())
            return;
        completeEditorClose(event);
        return;
    }
   
    UnsavedDialog unsavedDialog;   /// EFO need to add the stwqc here when terrain and world are split
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
        if(!discardUnsavedBakeFiles())
            return;
        completeEditorClose(event);
        return;
    }

    //// EFO  need to flesh this out for saving terrain and world separately
    if(!glWidget->saveRoute()){
        event->ignore();
        return;
    }
    completeEditorClose(event);
    
}

void RouteEditorWindow::moveEvent(QMoveEvent *event){
    QMainWindow::moveEvent(event);
    if(pinMainWindowAction != NULL && pinMainWindowAction->isChecked() && !applyingWindowPosition)
        pinPositionTimer.start(240);
}

bool RouteEditorWindow::eventFilter(QObject *watched, QEvent *event){
    if((watched == glWidget || watched == menuBar())
    && (event->type() == QEvent::Resize
     || event->type() == QEvent::Move
     || event->type() == QEvent::Show)){
        QTimer::singleShot(0, this, [this](){ positionRouteNameLabel(); });
    }
    return QMainWindow::eventFilter(watched, event);
}

void RouteEditorWindow::positionRouteNameLabel(){
    if(routeNameLabel == NULL || glWidget == NULL || menuBar() == NULL)
        return;
    const int minimumWidth = scaledUiSize(180);
    const int preferredWidth = routeNameLabel->sizeHint().width() + scaledUiSize(36);
    const int labelWidth = qMin(menuBar()->width(),
                                qMax(minimumWidth,
                                     qMin(scaledUiSize(520), preferredWidth)));
    const QPoint viewportCenter = glWidget->mapTo(
        menuBar(), QPoint(glWidget->width() / 2, 0));
    const int left = qBound(0, viewportCenter.x() - labelWidth / 2,
                            qMax(0, menuBar()->width() - labelWidth));
    routeNameLabel->setGeometry(left, 0, labelWidth, menuBar()->height());
    routeNameLabel->raise();
}

void RouteEditorWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    positionRouteNameLabel();
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
        setWindowState(windowState() | Qt::WindowMaximized);
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
    glWidget->saveRoute();
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

void RouteEditorWindow::createRouteHealthReport(){
    QDir routeFolder(Game::root + "/routes/" + Game::route);
    if(!routeFolder.exists()){
        QMessageBox::warning(this, tr("Route Health Report"),
                             tr("The active route folder could not be found:\n%1")
                             .arg(QDir::toNativeSeparators(routeFolder.absolutePath())));
        return;
    }

    if(Game::currentRoute == NULL){
        QMessageBox::warning(this, tr("Route Health Report"),
                             tr("No active route is available to inspect."));
        return;
    }

    int worldFileCount = 0;
    QStringList unloadedWorldFilesRaw;
    QString preparationError;
    if(!Game::currentRoute->prepareHealthReportData(
                worldFileCount, unloadedWorldFilesRaw, preparationError)){
        QMessageBox::warning(this, tr("Route Health Report"), preparationError);
        return;
    }

    const auto sortedUnique = [](const QStringList& source){
        QStringList result;
        QSet<QString> seen;
        for(QString value : source){
            value = value.trimmed();
            const QString key = value.toCaseFolded();
            if(!value.isEmpty() && !seen.contains(key)){
                seen.insert(key);
                result.push_back(value);
            }
        }
        result.sort(Qt::CaseInsensitive);
        return result;
    };

    QStringList usedObjectsRaw;
    QStringList usedTrackRaw;
    QStringList staticFlagsRaw;
    QStringList uidIssuesRaw;
    QStringList trackSectionIssuesRaw;
    Game::currentRoute->collectHealthReportData(
                usedObjectsRaw, usedTrackRaw, staticFlagsRaw,
                uidIssuesRaw, trackSectionIssuesRaw);

    const QStringList unloadedWorldFiles = sortedUnique(unloadedWorldFilesRaw);
    const QStringList missingTextures = sortedUnique(Route::missingTextureList);
    const QStringList missingShapes = sortedUnique(Route::missingList);
    const QStringList usedObjects = sortedUnique(usedObjectsRaw);
    const QStringList usedTrack = sortedUnique(usedTrackRaw);
    const QStringList staticFlags = sortedUnique(staticFlagsRaw);
    const QStringList uidIssues = sortedUnique(uidIssuesRaw);
    const QStringList trackSectionIssues =
            sortedUnique(trackSectionIssuesRaw);

    QString report;
    QTextStream out(&report);
    out << "TSRE GenX Route Health Report\n";
    out << "=============================\n\n";
    out << "Route: " << Game::route << "\n";
    out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "World-file scope: complete route scan\n";
    out << "World files found: " << worldFileCount << "\n";
    out << "World files loaded: "
        << qMax(0, worldFileCount - unloadedWorldFiles.size()) << "\n\n";
    out << "This report is diagnostic only. Creating it does not modify the route.\n";
    out << "Missing entries are those observed while the current route session loaded.\n\n";

    const auto writeSection = [&out](const QString& title, const QStringList& values){
        out << title << " (" << values.size() << ")\n";
        out << QString(title.size() + QString::number(values.size()).size() + 3, '-') << "\n";
        if(values.isEmpty()){
            out << "(none recorded)\n\n";
            return;
        }
        for(const QString& value : values)
            out << value << "\n";
        out << "\n";
    };

    const auto writeTrackDefinitionSection =
            [&out](const QStringList& values){
        const QString title = "Track definition problems";
        out << title << " (" << values.size() << ")\n";
        out << QString(title.size() + QString::number(values.size()).size() + 3, '-')
            << "\n";
        if(values.isEmpty()){
            out << "(none recorded)\n\n";
            return;
        }
        out << QString("%1 | %2 | %3 | %4 | %5")
               .arg("World file", -17)
               .arg("Failure", -48)
               .arg("UID", 8)
               .arg("SectionIdx", 10)
               .arg("Shape")
            << "\n";
        out << QString(17, '-') << "-+-"
            << QString(48, '-') << "-+-"
            << QString(8, '-') << "-+-"
            << QString(10, '-') << "-+-"
            << QString(20, '-') << "\n";
        for(const QString& value : values)
            out << value << "\n";
        out << "\n";
    };

    writeSection("World files not loaded", unloadedWorldFiles);
    writeSection("Missing textures", missingTextures);
    writeSection("Missing shapes", missingShapes);
    writeSection("Duplicate world/sound UIDs", uidIssues);
    writeTrackDefinitionSection(trackSectionIssues);
    writeSection("Used scenery/object files", usedObjects);
    writeSection("Used track shape files", usedTrack);
    writeSection("StaticFlags and object types", staticFlags);
    out.flush();

    const QString reportPath = routeFolder.filePath("TSRE-Route-Health-Report.txt");
    QSaveFile reportFile(reportPath);
    bool reportSaved = false;
    if(reportFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        reportFile.write(report.toUtf8());
        reportSaved = reportFile.commit();
    }

    QDialog dialog(this);
    GuiFunct::applyEditorPanelStyle(&dialog);
    GuiFunct::setEditorToolWindowTitle(&dialog);
    dialog.resize(scaledUiSize(900), scaledUiSize(700));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel(tr("ROUTE HEALTH REPORT"));
    GuiFunct::styleEditorTitle(title);
    layout->addWidget(title);

    QLabel *savedLocation = new QLabel(
            reportSaved
            ? tr("Saved to: %1").arg(QDir::toNativeSeparators(reportPath))
            : tr("The report is shown below, but it could not be saved to the route folder."));
    savedLocation->setWordWrap(true);
    layout->addWidget(savedLocation);

    QPlainTextEdit *reportView = new QPlainTextEdit;
    reportView->setReadOnly(true);
    reportView->setLineWrapMode(QPlainTextEdit::NoWrap);
    reportView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    reportView->setPlainText(report);
    reportView->moveCursor(QTextCursor::Start);
    layout->addWidget(reportView, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *copyButton = buttons->addButton(
            tr("Copy All"), QDialogButtonBox::ActionRole);
    QPushButton *openFolderButton = buttons->addButton(
            tr("Open Route Folder"), QDialogButtonBox::ActionRole);
    QObject::connect(copyButton, &QPushButton::released, &dialog, [report](){
        QApplication::clipboard()->setText(report);
    });
    QObject::connect(openFolderButton, &QPushButton::released, &dialog, [reportPath](){
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(reportPath).absolutePath()));
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    dialog.exec();
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
    if(!GuiFunct::confirmDestructiveAction(
            this, "Recreate Activity Paths",
            "This will delete all existing activity paths and create new "
            "simple paths.\n\nContinue?"))
        return;
    emit sendMsg(QString("createPaths"));
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
            if(polyVegSchemaEditor != NULL && polyVegSchemaEditor->isVisible())
                polyVegSchemaEditor->hide();
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
    if(name == "terrainTextureTools"){
        hideAllTools();
        terrainTextureTools->show();
        terrainTextureAction->setChecked(true);
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
    if(autoPlacementDock != NULL)
        autoPlacementDock->hide();
    if(polyVegDock != NULL)
        polyVegDock->hide();
    if(waterHelperDock != NULL)
        waterHelperDock->hide();
    objTools->hide();
    terrainTools->hide();
    terrainTextureTools->hide();
    geoTools->hide();
    objectsAction->setChecked(false);
    terrainAction->setChecked(false);     
    terrainTextureAction->setChecked(false);
    geoAction->setChecked(false);
    box->setFixedWidth(scaledUiSize(250));
}

void RouteEditorWindow::showToolsTerrainTexture(bool show){
    if(show){
        hideShowToolWidget(true);
        setToolbox("terrainTextureTools");
    } else {
        hideShowToolWidget(false);
    }
}

void RouteEditorWindow::showPolyVegHelper(bool show){
    if(polyVegDock == NULL || polyVegHelper == NULL)
        return;
    if(!show){
        polyVegDock->hide();
        return;
    }
    polyVegHelper->openForCurrentRoute();
    polyVegDock->show();
    polyVegDock->raise();
}

void RouteEditorWindow::showWaterHelper(bool show){
    if(waterHelperDock == NULL || waterHelper == NULL)
        return;
    if(!show){
        waterHelperDock->hide();
        return;
    }
    hideAllTools();
    waterHelperDock->show();
    waterHelperDock->raise();
}

void RouteEditorWindow::showPolyVegSchemaEditor(bool show){
    if(polyVegSchemaEditor == NULL)
        return;
    if(!show){
        polyVegSchemaEditor->hide();
        if(polyVegSchemaEditorAction != NULL)
            polyVegSchemaEditorAction->setChecked(false);
        return;
    }
    if(Game::serverClient != NULL){
        if(polyVegSchemaEditorAction != NULL)
            polyVegSchemaEditorAction->setChecked(false);
        return;
    }
    if(activityBuilderWindow != NULL && activityBuilderWindow->isVisible())
        activityBuilderWindow->hide();
    hideShowToolWidget(false);
    if(polyVegDock != NULL)
        polyVegDock->hide();
    if(waterHelperDock != NULL)
        waterHelperDock->hide();
    polyVegSchemaEditor->showForCurrentRoute();
    if(polyVegSchemaEditorAction != NULL)
        polyVegSchemaEditorAction->setChecked(true);
}

void RouteEditorWindow::showProperties(GameObj* obj){
    // hide all
    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        it->hide();
    }
    PropertiesTrackObj *trackProperties =
        qobject_cast<PropertiesTrackObj*>(objProperties["TrackObj"]);
    if(trackProperties != NULL){
        trackProperties->setHacksSelection(obj);
        QPushButton *visibleHacksButton = NULL;
        if(obj != NULL && trackProperties->support(obj)){
            visibleHacksButton = trackProperties->hacksButton();
        } else if(obj != NULL){
            PropertiesPolyVegBake *polyVegBakeProperties =
                qobject_cast<PropertiesPolyVegBake*>(objProperties["PolyVegBake"]);
            PropertiesStatic *staticProperties =
                qobject_cast<PropertiesStatic*>(objProperties["Static"]);
            PropertiesTerrain *terrainProperties =
                qobject_cast<PropertiesTerrain*>(objProperties["Terrain"]);
            PropertiesSignal *signalProperties =
                qobject_cast<PropertiesSignal*>(objProperties["Signal"]);
            if(polyVegBakeProperties != NULL && polyVegBakeProperties->support(obj))
                visibleHacksButton = polyVegBakeProperties->hacksButton();
            else if(staticProperties != NULL && staticProperties->support(obj))
                visibleHacksButton = staticProperties->hacksButton();
            else if(terrainProperties != NULL && terrainProperties->support(obj))
                visibleHacksButton = terrainProperties->hacksButton();
            else if(signalProperties != NULL && signalProperties->support(obj))
                visibleHacksButton = signalProperties->hacksButton();
        }
        trackProperties->adoptHacksButton(visibleHacksButton);
    }
    if(obj == NULL){
        EditorPopupWindow::closeActiveUnlessSupportedBy(NULL);
        propertiesPanelTitle->hide();
        return;
    }
    propertiesPanelTitle->show();
    // show 
    //qDebug() << obj->typeObj;

    // Generated PolyVeg blocks have a deliberately restricted panel. Dispatch
    // it before the generic QHash walk (whose Undefined fallback supports all
    // objects) so they can never inherit editable Static-object commands.
    PropertiesPolyVegBake *polyVegBakeProperties =
        qobject_cast<PropertiesPolyVegBake*>(objProperties["PolyVegBake"]);
    if(polyVegBakeProperties != NULL && polyVegBakeProperties->support(obj)){
        polyVegBakeProperties->showObj(obj);
        polyVegBakeProperties->show();
        EditorPopupWindow::closeActiveUnlessSupportedBy(polyVegBakeProperties);
        return;
    }

    //for (std::vector<PropertiesAbstract*>::iterator it = objProperties.begin(); it != objProperties.end(); ++it) {
    foreach (PropertiesAbstract *it, objProperties){
        if(it == NULL) continue;
        if(!it->support(obj)) continue;
        it->showObj(obj);
        it->show();
        EditorPopupWindow::closeActiveUnlessSupportedBy(it);
        return;
    }
    EditorPopupWindow::closeActiveUnlessSupportedBy(NULL);
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
    if(show) {
        settingsDialog->loadSettings();
        settingsDialog->show();
        settingsDialog->raise();
        settingsDialog->activateWindow();
    } else {
        settingsDialog->hide();
    }
}


void RouteEditorWindow::hideShowToolWidget(bool show){
    if(show) {
        if(activityBuilderWindow != NULL && activityBuilderWindow->isVisible())
            activityBuilderWindow->hide();
        if(autoPlacementDock != NULL && autoPlacementDock->isVisible())
            autoPlacementDock->hide();
        if(polyVegDock != NULL && polyVegDock->isVisible())
            polyVegDock->hide();
        if(waterHelperDock != NULL && waterHelperDock->isVisible())
            waterHelperDock->hide();
        box->show();
    }
    else     { box->hide();    }
}

void RouteEditorWindow::viewWorldGrid(bool show){
    Game::viewWorldGrid = show;
}
void RouteEditorWindow::viewTileGrid(bool show){
    Game::viewTileGrid = show;
}
void RouteEditorWindow::viewTerrainShape(bool show){
    if(show){
        if(Game::viewTerrainShape){
            routeMapVisibleBeforeTerrainHide = MapWindow::routeMapOverlaysVisible;
            terrainShapeMapStateCaptured = true;
        }
        if(MapWindow::routeMapOverlaysVisible && Game::terrainLib != NULL)
            Game::terrainLib->setRouteMapOverlayVisible(false);
        Game::viewTerrainShape = false;
        return;
    }

    Game::viewTerrainShape = true;
    if(!terrainShapeMapStateCaptured)
        return;

    const bool restoreMap = routeMapVisibleBeforeTerrainHide;
    terrainShapeMapStateCaptured = false;
    if(Game::terrainLib != NULL
            && MapWindow::routeMapOverlaysVisible != restoreMap)
        Game::terrainLib->setRouteMapOverlayVisible(restoreMap);
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
    if(!Game::saveViewCompassState())
        qWarning() << "Unable to save Compass visibility to" << Game::settingsFilePath();
}

void RouteEditorWindow::showRoute(){
    if(Game::serverClient == NULL){
        if(!glWidget->initRoute()){
            emit exitNow();
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
    if(Game::lockCamera == true) emit updStatus(QString("camera"),QString("Camera Locked")); else emit updStatus(QString("camera"),QString("Camera Unlocked"));
    
    QMainWindow::show();
    applyRestoredSessionGeometry();

    QStringList winPos = Game::statusPos.split(",");
    QPoint pinnedControlPosition;
    const bool controlPositionPinned =
            Game::pinnedWindowPosition("controlPanel", &pinnedControlPosition);
    const bool controlDefaultRequested =
            Game::pinnedWindowPosition("controlPanelUseDefault", NULL);

    if(!controlPositionPinned && (controlDefaultRequested || winPos.count() < 2)){
        const int naviTemp1 = this->x() - statusWindow->width();
        const int naviTemp2 = this->y() + 200;
        statusWindow->move(
            std::max(0, naviTemp1),
            std::min(naviTemp2,
                     QApplication::primaryScreen()->geometry().bottom()
                     - statusWindow->height()));
    }

    // A Qt::Tool child shown before its parent can remain logically checked
    // but invisible. Show it only after Main is live.
    statusWindow->show();
    statusWindow->raise();
    statAction->setChecked(true);
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

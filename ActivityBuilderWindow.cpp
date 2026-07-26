/*  This file is part of TSRE5.
 *
 *  Phase 1 Activity Builder workspace.
 */

#include "ActivityBuilderWindow.h"

#include "ActivityTools.h"
#include "ActivityTrackViewer.h"
#include "Game.h"
#include "GuiFunct.h"
#include "Path.h"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QGridLayout>
#include <QKeySequence>
#include <QMoveEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QStackedWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <functional>

ActivityBuilderWindow::ActivityBuilderWindow(ActivityTools *tools, QWidget *parent)
    : QMainWindow(parent, Qt::Window), defaultSize(qRound(1200 * qMax(1.0f, Game::uiScale)),
                                                   qRound(800 * qMax(1.0f, Game::uiScale))) {
    setWindowTitle(tr("TSRE Activity Builder"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks);
    setMinimumSize(850, 560);

    viewer = new ActivityTrackViewer(this);
    setCentralWidget(viewer);
    QObject::connect(viewer, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));
    QObject::connect(viewer, SIGNAL(junctionSelected(QString)), this, SLOT(showJunctionDetails(QString)));
    QObject::connect(viewer, SIGNAL(pathDraftStatus(QString)), this, SLOT(showJunctionDetails(QString)));
    QObject::connect(tools, SIGNAL(pathCreationStarted(Path*)), this, SLOT(beginPathCreation(Path*)));
    QObject::connect(tools, SIGNAL(pathEditStarted(Path*)), viewer, SLOT(beginPathEdit(Path*)));

    QDockWidget *activityDock = new QDockWidget(tr("Activity"), this);
    activityDock->setObjectName("activityBuilderActivityDock");
    QWidget *activityDockTitle = new QWidget(activityDock);
    activityDockTitle->setFixedHeight(0);
    activityDock->setTitleBarWidget(activityDockTitle);
    activityDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    activityDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    activityDock->setMinimumWidth(qRound(260 * qMax(1.0f, Game::uiScale)));
    QScrollArea *activityScroll = new QScrollArea(activityDock);
    activityScroll->setWidgetResizable(true);
    activityScroll->setFrameShape(QFrame::NoFrame);
    activityScroll->setWidget(tools);
    activityDock->setWidget(activityScroll);
    addDockWidget(Qt::LeftDockWidgetArea, activityDock);

    QDockWidget *detailsDock = new QDockWidget(tr("Selection & Legend"), this);
    detailsDock->setObjectName("activityBuilderDetailsDock");
    detailsDock->setAllowedAreas(Qt::RightDockWidgetArea);
    detailsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *detailsDockTitle = new QWidget(detailsDock);
    detailsDockTitle->setFixedHeight(0);
    detailsDock->setTitleBarWidget(detailsDockTitle);
    QWidget *details = new QWidget(detailsDock);
    GuiFunct::applyEditorPanelStyle(details);
    QVBoxLayout *detailsLayout = new QVBoxLayout(details);
    QLabel *detailsHeading = new QLabel(tr("SELECTION & LEGEND"), details);
    GuiFunct::styleEditorTitle(detailsHeading);
    detailsLayout->addWidget(detailsHeading);
    pathInfo = new QLabel(tr("No path selected"), details);
    pathInfo->setWordWrap(true);
    detailsLayout->addWidget(pathInfo);
    junctionInfo = new QLabel(tr("Click an orange switch to inspect its default route and TrackDB pins."), details);
    junctionInfo->setWordWrap(true);
    junctionInfo->setStyleSheet("margin-top: 8px; padding: 6px; background: #292d31;");
    detailsLayout->addWidget(junctionInfo);
    QLabel *legend = new QLabel(
        tr("<b>TrackDB</b><br>"
           "<span style='color:#b8bdc2'>■</span>&nbsp; Track<br>"
           "<span style='color:#ffae38'>■</span>&nbsp; Switch at default<br>"
           "<span style='color:#00ebff'>■</span>&nbsp; Switch changed<br>"
           "<span style='color:#69aef5'>■</span>&nbsp; Endpoint<br>"
           "<span style='color:#ffe140'>■</span>&nbsp; Selected path<br>"
           "<span style='color:#41dc69'>■</span>&nbsp; Path start<br>"
           "<span style='color:#eb4b4b'>■</span>&nbsp; Path end"), details);
    legend->setWordWrap(true);
    QLabel *interactiveLegend = new QLabel(
        tr("<b>Interactives</b><br>"
           "<span style='color:#5ae17d'>■</span>&nbsp; Signal<br>"
           "<span style='color:#55aff5'>■</span>&nbsp; Station/platform<br>"
           "<span style='color:#73a5eb'>■</span>&nbsp; Siding<br>"
           "<span style='color:#f5b941'>■</span>&nbsp; Service point<br>"
           "<span style='color:#cd78f0'>■</span>&nbsp; Route marker"), details);
    interactiveLegend->setWordWrap(true);
    QLabel *phase = new QLabel(tr("Path Builder: magenta is the unsaved path preview. "
                                  "Completed paths remain independent of activities."), details);
    phase->setWordWrap(true);
    phase->setStyleSheet("color: #aeb5bb;");
    detailsLayout->addWidget(phase);
    detailsLayout->addStretch(1);
    detailsLayout->addWidget(legend);
    detailsLayout->addWidget(interactiveLegend);
    detailsDock->setWidget(details);
    detailsDock->setMinimumWidth(qRound(190 * qMax(1.0f, Game::uiScale)));
    addDockWidget(Qt::RightDockWidgetArea, detailsDock);

    QDockWidget *pathDock = new QDockWidget(tr("Path Controls"), this);
    pathDock->setObjectName("activityBuilderPathControlsDock");
    pathDock->setAllowedAreas(Qt::RightDockWidgetArea);
    pathDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *pathDockTitle = new QWidget(pathDock);
    pathDockTitle->setFixedHeight(0);
    pathDock->setTitleBarWidget(pathDockTitle);
    QWidget *pathControls = new QWidget(pathDock);
    GuiFunct::applyEditorPanelStyle(pathControls);
    QVBoxLayout *pathLayout = new QVBoxLayout(pathControls);
    QLabel *pathHeading = new QLabel(tr("PATH CONTROLS"), pathControls);
    GuiFunct::styleEditorTitle(pathHeading);
    pathLayout->addWidget(pathHeading);
    QLabel *pathHelp = new QLabel(tr("Build the path like flowing water. Orange switches are always live: click one directly at any time. Use a button before placing a start, endpoint, or reverse point."), pathControls);
    pathHelp->setWordWrap(true);
    pathLayout->addWidget(pathHelp);
    QGridLayout *pathButtons = new QGridLayout();
    QPushButton *setStart = new QPushButton(tr("Place Start"), pathControls);
    QPushButton *reverseDirection = new QPushButton(tr("Reverse Start"), pathControls);
    QPushButton *placeEnd = new QPushButton(tr("Place Endpoint"), pathControls);
    QPushButton *addReverse = new QPushButton(tr("Reverse Point"), pathControls);
    QPushButton *addPassing = new QPushButton(tr("Add Passing Siding"), pathControls);
    QPushButton *undoEdit = new QPushButton(tr("Undo"), pathControls);
    QPushButton *redoEdit = new QPushButton(tr("Redo"), pathControls);
    QPushButton *validate = new QPushButton(tr("Check Path"), pathControls);
    validate->setToolTip(tr("Check that the start, endpoint, reverse legs, wait points, switches, and passing paths form a saveable route."));
    QPushButton *savePath = new QPushButton(tr("Save Path"), pathControls);
    pathButtons->addWidget(setStart, 0, 0);
    pathButtons->addWidget(reverseDirection, 0, 1);
    pathButtons->addWidget(placeEnd, 1, 0);
    pathButtons->addWidget(addReverse, 1, 1);
    pathButtons->addWidget(addPassing, 2, 0, 1, 2);
    pathButtons->addWidget(undoEdit, 3, 0);
    pathButtons->addWidget(redoEdit, 3, 1);
    pathButtons->addWidget(validate, 4, 0, 1, 2);
    pathButtons->addWidget(savePath, 5, 0, 1, 2);
    undoEdit->setShortcut(QKeySequence::Undo);
    redoEdit->setShortcut(QKeySequence::Redo);
    pathLayout->addLayout(pathButtons);
    pathLayout->addSpacing(qRound(5 * qMax(1.0f, Game::uiScale)));
    QLabel *waitHeading = new QLabel(tr("• Wait Point"), pathControls);
    waitHeading->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel
                               + "; font-weight: bold; }");
    waitHeading->setContentsMargins(8,0,0,0);
    pathLayout->addWidget(waitHeading);
    QGridLayout *waitControls = new QGridLayout();
    QCheckBox *waitForDuration = new QCheckBox(tr("Wait for duration"), pathControls);
    QCheckBox *waitUntilTime = new QCheckBox(tr("Wait until clock time"), pathControls);
    QButtonGroup *waitModeGroup = new QButtonGroup(pathControls);
    waitModeGroup->setExclusive(true);
    waitModeGroup->addButton(waitForDuration);
    waitModeGroup->addButton(waitUntilTime);
    waitForDuration->setChecked(true);
    QSpinBox *waitMinutes = new QSpinBox(pathControls);
    waitMinutes->setRange(0, 1092);
    waitMinutes->setSuffix(tr(" min"));
    waitMinutes->setValue(1);
    QSpinBox *waitSeconds = new QSpinBox(pathControls);
    waitSeconds->setRange(0, 59);
    waitSeconds->setSuffix(tr(" sec"));
    QSpinBox *waitUntilHour = new QSpinBox(pathControls);
    waitUntilHour->setRange(0, 23);
    waitUntilHour->setSuffix(tr(" hr"));
    waitUntilHour->setValue(12);
    QSpinBox *waitUntilMinute = new QSpinBox(pathControls);
    waitUntilMinute->setRange(0, 59);
    waitUntilMinute->setSuffix(tr(" min"));
    waitUntilHour->setEnabled(false);
    waitUntilMinute->setEnabled(false);
    QPushButton *addWait = new QPushButton(tr("Add Wait Point"), pathControls);
    setStart->setCheckable(true);
    placeEnd->setCheckable(true);
    addReverse->setCheckable(true);
    addPassing->setCheckable(true);
    addWait->setCheckable(true);
    waitControls->addWidget(waitForDuration, 0, 0, 1, 2);
    waitControls->addWidget(waitMinutes, 1, 0);
    waitControls->addWidget(waitSeconds, 1, 1);
    waitControls->addWidget(waitUntilTime, 2, 0, 1, 2);
    waitControls->addWidget(waitUntilHour, 3, 0);
    waitControls->addWidget(waitUntilMinute, 3, 1);
    waitControls->addWidget(addWait, 4, 0, 1, 2);
    pathLayout->addLayout(waitControls);
    pathLayout->addSpacing(qRound(5 * qMax(1.0f, Game::uiScale)));
    QLabel *advancedHeading = new QLabel(tr("• Advanced"), pathControls);
    advancedHeading->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel
                                   + "; font-weight: bold; }");
    advancedHeading->setContentsMargins(8,0,0,0);
    pathLayout->addWidget(advancedHeading);
    QComboBox *advancedOperation = new QComboBox(pathControls);
    advancedOperation->addItem(tr("Blow Horn"));
    advancedOperation->addItem(tr("Uncouple Cars"));
    advancedOperation->addItem(tr("Join / Split"));
    advancedOperation->addItem(tr("Request Pass Red"));
    pathLayout->addWidget(advancedOperation);
    QStackedWidget *advancedOptions = new QStackedWidget(pathControls);

    QWidget *hornPage = new QWidget(advancedOptions);
    QGridLayout *hornLayout = new QGridLayout(hornPage);
    hornLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *hornLabel = new QLabel(tr("Horn duration"), hornPage);
    QSpinBox *hornSeconds = new QSpinBox(hornPage);
    hornSeconds->setRange(1, 10);
    hornSeconds->setValue(2);
    hornSeconds->setSuffix(tr(" sec"));
    hornLayout->addWidget(hornLabel, 0, 0);
    hornLayout->addWidget(hornSeconds, 0, 1);
    advancedOptions->addWidget(hornPage);

    QWidget *uncouplePage = new QWidget(advancedOptions);
    QGridLayout *uncoupleLayout = new QGridLayout(uncouplePage);
    uncoupleLayout->setContentsMargins(0, 0, 0, 0);
    QComboBox *uncoupleEnd = new QComboBox(uncouplePage);
    uncoupleEnd->addItem(tr("Keep front of train"));
    uncoupleEnd->addItem(tr("Keep rear of train"));
    QSpinBox *uncoupleCars = new QSpinBox(uncouplePage);
    uncoupleCars->setRange(1, 99);
    uncoupleCars->setValue(1);
    uncoupleCars->setSuffix(tr(" cars"));
    uncoupleCars->setToolTip(tr("Number of cars to keep, including locomotives."));
    QSpinBox *uncouplePause = new QSpinBox(uncouplePage);
    uncouplePause->setRange(0, 99);
    uncouplePause->setValue(1);
    uncouplePause->setSuffix(tr(" sec"));
    uncoupleLayout->addWidget(uncoupleEnd, 0, 0, 1, 2);
    uncoupleLayout->addWidget(new QLabel(tr("Cars to keep"), uncouplePage), 1, 0);
    uncoupleLayout->addWidget(uncoupleCars, 1, 1);
    uncoupleLayout->addWidget(new QLabel(tr("Pause after"), uncouplePage), 2, 0);
    uncoupleLayout->addWidget(uncouplePause, 2, 1);
    advancedOptions->addWidget(uncouplePage);

    QWidget *joinPage = new QWidget(advancedOptions);
    QVBoxLayout *joinLayout = new QVBoxLayout(joinPage);
    joinLayout->setContentsMargins(0, 2, 0, 2);
    QLabel *joinHelp = new QLabel(tr("Join the nearby train and continue the shunting move."), joinPage);
    joinHelp->setWordWrap(true);
    joinLayout->addWidget(joinHelp);
    advancedOptions->addWidget(joinPage);

    QWidget *passRedPage = new QWidget(advancedOptions);
    QVBoxLayout *passRedLayout = new QVBoxLayout(passRedPage);
    passRedLayout->setContentsMargins(0, 2, 0, 2);
    QLabel *passRedHelp = new QLabel(tr("Request permission for the AI train to pass the next red signal."), passRedPage);
    passRedHelp->setWordWrap(true);
    passRedLayout->addWidget(passRedHelp);
    advancedOptions->addWidget(passRedPage);
    pathLayout->addWidget(advancedOptions);

    QLabel *advancedNote = new QLabel(tr("Requires ORTS Extended AI train shunting."), pathControls);
    advancedNote->setWordWrap(true);
    advancedNote->setStyleSheet("color: #aeb5bb;");
    pathLayout->addWidget(advancedNote);
    QPushButton *addAdvanced = new QPushButton(tr("Place Horn Point"), pathControls);
    addAdvanced->setCheckable(true);
    pathLayout->addWidget(addAdvanced);
    const std::function<int()> advancedValue =
        [advancedOperation, hornSeconds, uncoupleEnd, uncoupleCars, uncouplePause]() {
        switch(advancedOperation->currentIndex()){
        case 1:
            return (uncoupleEnd->currentIndex() == 1 ? 50000 : 40000)
                 + uncoupleCars->value() * 100 + uncouplePause->value();
        case 2:
            return 60001;
        case 3:
            return 60002;
        default:
            return 60010 + hornSeconds->value();
        }
    };
    pathLayout->addStretch(1);
    QLabel *keyHeading = new QLabel(tr("• Path Key"), pathControls);
    keyHeading->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel
                              + "; font-weight: bold; }");
    keyHeading->setContentsMargins(8,0,0,0);
    pathLayout->addWidget(keyHeading);
    QLabel *colorKey = new QLabel(
        tr("<span style='color:#ff00d2'>■</span>&nbsp; Path: Main<br>"
           "<span style='color:#00ebff'>■</span>&nbsp; Path: Overlap<br>"
           "<span style='color:#ff9119'>■</span>&nbsp; Path: Passing Siding<br>"
           "<span style='color:#ffe140'>■</span>&nbsp; Path: Saved"),
        pathControls);
    colorKey->setWordWrap(true);
    pathLayout->addWidget(colorKey);
    pathDock->setWidget(pathControls);
    pathDock->setMinimumWidth(qRound(260 * qMax(1.0f, Game::uiScale)));
    addDockWidget(Qt::RightDockWidgetArea, pathDock);
    tabifyDockWidget(detailsDock, pathDock);
    pathDock->raise();

    QObject::connect(setStart, SIGNAL(clicked()), viewer, SLOT(choosePathStart()));
    QObject::connect(reverseDirection, SIGNAL(clicked()), viewer, SLOT(reverseStartDirection()));
    QObject::connect(placeEnd, SIGNAL(clicked()), viewer, SLOT(choosePathEnd()));
    QObject::connect(addReverse, SIGNAL(clicked()), viewer, SLOT(choosePathReverse()));
    QObject::connect(addPassing, SIGNAL(clicked()), viewer, SLOT(choosePassingSiding()));
    QObject::connect(undoEdit, SIGNAL(clicked()), viewer, SLOT(undoDraftEdit()));
    QObject::connect(redoEdit, SIGNAL(clicked()), viewer, SLOT(redoDraftEdit()));
    QObject::connect(validate, SIGNAL(clicked()), viewer, SLOT(validateDraftPath()));
    QObject::connect(savePath, SIGNAL(clicked()), viewer, SLOT(saveDraftPath()));
    QObject::connect(waitForDuration, &QCheckBox::toggled, this,
                     [waitMinutes, waitSeconds, waitUntilHour, waitUntilMinute](bool duration) {
        waitMinutes->setEnabled(duration);
        waitSeconds->setEnabled(duration);
        waitUntilHour->setEnabled(!duration);
        waitUntilMinute->setEnabled(!duration);
    });
    QObject::connect(addWait, &QPushButton::clicked, this,
                     [this, addAdvanced, waitUntilTime, waitMinutes, waitSeconds,
                      waitUntilHour, waitUntilMinute]() {
        advancedWaitPlacement = false;
        addAdvanced->setChecked(false);
        const int encodedWait = waitUntilTime->isChecked()
            ? 30000 + waitUntilHour->value() * 100 + waitUntilMinute->value()
            : qMin(65535, waitMinutes->value() * 60 + waitSeconds->value());
        viewer->choosePathWait(encodedWait);
    });
    QObject::connect(advancedOperation, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, [this, advancedOptions, addAdvanced, advancedValue](int index) {
        advancedOptions->setCurrentIndex(index);
        const QString labels[] = {
            tr("Place Horn Point"),
            tr("Place Uncouple Point"),
            tr("Place Join / Split Point"),
            tr("Place Pass-Red Point")
        };
        addAdvanced->setText(labels[qBound(0, index, 3)]);
        if(addAdvanced->isChecked()){
            advancedWaitPlacement = true;
            viewer->choosePathWait(advancedValue());
        }
    });
    QObject::connect(addAdvanced, &QPushButton::clicked, this,
                     [this, addWait, advancedValue]() {
        advancedWaitPlacement = true;
        addWait->setChecked(false);
        viewer->choosePathWait(advancedValue());
    });
    QObject::connect(viewer, &ActivityTrackViewer::pathPlacementModeChanged, this,
                     [this, setStart, placeEnd, addReverse, addPassing, addWait, addAdvanced](int mode) {
        setStart->setChecked(mode == ActivityTrackViewer::PlaceStart);
        placeEnd->setChecked(mode == ActivityTrackViewer::PlaceEnd);
        addReverse->setChecked(mode == ActivityTrackViewer::PlaceReverse);
        addPassing->setChecked(mode == ActivityTrackViewer::PlacePassingSiding);
        addWait->setChecked(mode == ActivityTrackViewer::PlaceWait && !advancedWaitPlacement);
        addAdvanced->setChecked(mode == ActivityTrackViewer::PlaceWait && advancedWaitPlacement);
        if(mode != ActivityTrackViewer::PlaceWait)
            advancedWaitPlacement = false;
    });

    QAction *toggleActivityBuilder = new QAction(this);
    toggleActivityBuilder->setShortcut(QKeySequence(Qt::Key_F4));
    toggleActivityBuilder->setShortcutContext(Qt::WindowShortcut);
    addAction(toggleActivityBuilder);
    QObject::connect(toggleActivityBuilder, &QAction::triggered, this, [this]() {
        emit userToggleSoundRequested();
        close();
    });

    QToolBar *mapTools = addToolBar(tr("Track Viewer"));
    mapTools->setMovable(false);
    QAction *fitRouteAction = mapTools->addAction(tr("Fit Route"));
    fitRouteAction->setToolTip(tr("Fit the complete TrackDB in the viewer. Double-clicking the map does the same."));
    QObject::connect(fitRouteAction, SIGNAL(triggered()), viewer, SLOT(fitRoute()));
    QAction *fitPathAction = mapTools->addAction(tr("Fit Path"));
    fitPathAction->setToolTip(tr("Fit the currently selected path."));
    QObject::connect(fitPathAction, SIGNAL(triggered()), viewer, SLOT(fitSelectedPath()));
    QAction *rotateAction = mapTools->addAction(tr("Rotate 90°"));
    rotateAction->setToolTip(tr("Rotate the map clockwise for screenshots. The N compass continues to indicate geographic north."));
    QObject::connect(rotateAction, SIGNAL(triggered()), viewer, SLOT(rotateView90()));
    mapTools->addSeparator();
    QAction *junctionAction = mapTools->addAction(tr("Junctions"));
    junctionAction->setCheckable(true);
    junctionAction->setChecked(true);
    QObject::connect(junctionAction, SIGNAL(toggled(bool)), viewer, SLOT(setShowJunctions(bool)));
    QAction *interactivesAction = mapTools->addAction(tr("Interactives"));
    interactivesAction->setCheckable(true);
    interactivesAction->setChecked(true);
    interactivesAction->setToolTip(tr("Show signals, stations, sidings, and service points."));
    QObject::connect(interactivesAction, SIGNAL(toggled(bool)), viewer, SLOT(setShowInteractives(bool)));
    QAction *markersAction = mapTools->addAction(tr("Markers"));
    markersAction->setCheckable(true);
    markersAction->setChecked(true);
    markersAction->setToolTip(tr("Show markers from the .mkr file whose name matches this route."));
    QObject::connect(markersAction, SIGNAL(toggled(bool)), viewer, SLOT(setShowMarkers(bool)));
    QAction *labelsAction = mapTools->addAction(tr("Labels"));
    labelsAction->setCheckable(true);
    labelsAction->setChecked(true);
    labelsAction->setToolTip(tr("Show collision-managed station, siding, signal, and marker text."));
    QObject::connect(labelsAction, SIGNAL(toggled(bool)), viewer, SLOT(setShowMapLabels(bool)));
    QAction *gridAction = mapTools->addAction(tr("Tile Grid"));
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    QObject::connect(gridAction, SIGNAL(toggled(bool)), viewer, SLOT(setShowTileGrid(bool)));
    // F4 is a full workspace, not a floating utility window. It always opens
    // maximized, so the former pin/geometry controls are intentionally omitted.
    restoreInitialGeometry();
    hide();
}

void ActivityBuilderWindow::restoreInitialGeometry(){
    resize(defaultSize);
    moveToDefaultPosition();
    initialGeometryApplied = true;
}

void ActivityBuilderWindow::moveToDefaultPosition(){
    QWidget *owner = parentWidget();
    if(owner != NULL){
        const QPoint centered = owner->frameGeometry().center() - QPoint(width() / 2, height() / 2);
        move(Game::visibleWindowPosition(centered, size()));
    }
}

void ActivityBuilderWindow::routeLoaded(Route *route){
    viewer->setRoute(route);
}

void ActivityBuilderWindow::setSelectedPath(Path *path){
    viewer->setSelectedPath(path);
    if(path == NULL){
        pathInfo->setText(tr("No path selected"));
        return;
    }
    pathInfo->setText(tr("<b>%1</b><br>%2 → %3<br>%4 path control points")
                      .arg(path->displayName.toHtmlEscaped())
                      .arg(path->trPathStart.toHtmlEscaped())
                      .arg(path->trPathEnd.toHtmlEscaped())
                      .arg(path->node.size()));
}

void ActivityBuilderWindow::showJunctionDetails(QString text){
    junctionInfo->setText(text);
}

void ActivityBuilderWindow::beginPathCreation(Path *path){
    setSelectedPath(path);
    viewer->beginPathCreation(path);
}

void ActivityBuilderWindow::setPinned(bool pinned){
    if(pinned){
        savePinnedGeometry();
    } else {
        geometrySaveTimer.stop();
        Game::clearPinnedWindowPosition("activityBuilder");
        resize(defaultSize);
        moveToDefaultPosition();
    }
}

void ActivityBuilderWindow::savePinnedGeometry(){
    if(!initialGeometryApplied || pinAction == NULL || !pinAction->isChecked())
        return;
    Game::savePinnedWindowGeometry("activityBuilder", normalGeometry(), isMaximized());
}

void ActivityBuilderWindow::closeEvent(QCloseEvent *event){
    savePinnedGeometry();
    hide();
    emit visibilityChanged(false);
    event->ignore();
}

void ActivityBuilderWindow::moveEvent(QMoveEvent *event){
    QMainWindow::moveEvent(event);
    if(initialGeometryApplied && pinAction != NULL && pinAction->isChecked())
        geometrySaveTimer.start();
}

void ActivityBuilderWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    if(initialGeometryApplied && pinAction != NULL && pinAction->isChecked())
        geometrySaveTimer.start();
}

void ActivityBuilderWindow::showEvent(QShowEvent *event){
    QMainWindow::showEvent(event);
    if(!isMaximized())
        QTimer::singleShot(0, this, SLOT(showMaximized()));
    if(!dockWidthsInitialized){
        dockWidthsInitialized = true;
        QTimer::singleShot(0, this, [this](){
            QDockWidget *activityDock = findChild<QDockWidget*>("activityBuilderActivityDock");
            QDockWidget *pathDock = findChild<QDockWidget*>("activityBuilderPathControlsDock");
            if(activityDock != NULL)
                resizeDocks(QList<QDockWidget*>() << activityDock,
                            QList<int>() << activityDock->minimumWidth(),
                            Qt::Horizontal);
            if(pathDock != NULL)
                resizeDocks(QList<QDockWidget*>() << pathDock,
                            QList<int>() << pathDock->minimumWidth(),
                            Qt::Horizontal);
        });
    }
    viewer->ensureCache();
    emit visibilityChanged(true);
}

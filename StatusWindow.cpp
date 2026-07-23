/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "StatusWindow.h"
#include "GeoCoordinates.h"
#include <QtWidgets>
#include <QDebug>
#include "Game.h"
#include "Route.h"
#include "RouteEditorGLWidget.h"
#include "ShapeLib.h"
#include "Coords.h"
#include "GuiFunct.h"

static int scaledUiSize(int base){
    return qRound(base * qMax(1.0f, Game::uiScale));
}

static QString statusButtonStyle(const QString& background, const QString& hoverBackground,
                                 const QString& textColor, const QString& border,
                                 const QString& pressedBackground){
    return QString(
        "QPushButton { color: %1;"
        " background-color: %2;"
        " border: 1px solid %4; border-radius: 1px; padding: 1px 4px; }"
        "QPushButton:hover { background-color: %3; border-color: #f08200; }"
        "QPushButton:pressed {"
        " background-color: %5; border-color: %4;"
        " padding-top: 2px; padding-bottom: 0px; }"
    ).arg(textColor, background, hoverBackground, border, pressedBackground);
}

static QString statusReadoutStyle(const QString& background, const QString& textColor,
                                  const QString& border){
    return QString(
        "QPushButton, QPushButton:hover, QPushButton:pressed { color: %1;"
        " background-color: %2; border: 1px solid %3; border-radius: 1px;"
        " padding: 1px 4px; }"
    ).arg(textColor, background, border);
}

static QPoint snapWindowPosition(QWidget *window){
    const int snapDistance = 10;
    QRect moving = window->frameGeometry();
    QPoint snappedFramePos = moving.topLeft();
    int bestX = snapDistance + 1;
    int bestY = snapDistance + 1;

    QList<QWidget*> targets;
    QWidget *parent = window->parentWidget();
    if(parent != NULL)
        targets.append(parent);
    if(parent != NULL){
        QList<QWidget*> siblings = parent->findChildren<QWidget*>();
        for(int i = 0; i < siblings.size(); i++){
            QWidget *candidate = siblings[i];
            if(candidate == window || !candidate->isWindow() || !candidate->isVisible())
                continue;
            targets.append(candidate);
        }
    }

    for(int i = 0; i < targets.size(); i++){
        QRect target = targets[i]->frameGeometry();
        bool verticalNear = moving.bottom() >= target.top() - snapDistance && moving.top() <= target.bottom() + snapDistance;
        bool horizontalNear = moving.right() >= target.left() - snapDistance && moving.left() <= target.right() + snapDistance;
        int xCandidates[4] = {
            target.left() - moving.left(),
            target.right() - moving.right(),
            target.right() + 1 - moving.left(),
            target.left() - 1 - moving.right()
        };
        int yCandidates[4] = {
            target.top() - moving.top(),
            target.bottom() - moving.bottom(),
            target.bottom() + 1 - moving.top(),
            target.top() - 1 - moving.bottom()
        };

        for(int j = 0; j < 4; j++){
            int dist = qAbs(xCandidates[j]);
            if(verticalNear && dist <= snapDistance && dist < bestX){
                bestX = dist;
                snappedFramePos.setX(moving.left() + xCandidates[j]);
            }
            dist = qAbs(yCandidates[j]);
            if(horizontalNear && dist <= snapDistance && dist < bestY){
                bestY = dist;
                snappedFramePos.setY(moving.top() + yCandidates[j]);
            }
        }
    }

    return window->pos() + (snappedFramePos - moving.topLeft());
}

StatusWindow::StatusWindow(QWidget* parent) : QWidget(parent) {
    this->setWindowFlags(Qt::WindowType::Tool);
    //this->setWindowFlags(Qt::WindowStaysOnTopHint);
    this->setFixedWidth(scaledUiSize(300));
    this->setFixedHeight(scaledUiSize(463));
    this->setWindowTitle(QString());
    const bool defaultPositionRequested = Game::pinnedWindowPosition("controlPanelUseDefault", NULL);
    QStringList winPos = Game::statusPos.split(",");
    if(!defaultPositionRequested && winPos.count() > 1)
        this->move(winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
    QPoint pinnedPosition;
    positionPinned = Game::pinnedWindowPosition("controlPanel", &pinnedPosition);
    if(positionPinned)
        this->move(Game::visibleWindowPosition(pinnedPosition, size()));
    snapTimer.setSingleShot(true);
    QObject::connect(&snapTimer, SIGNAL(timeout()), this, SLOT(applyWindowSnap()));
    pinSaveTimer.setSingleShot(true);
    QObject::connect(&pinSaveTimer, SIGNAL(timeout()), this, SLOT(savePinnedPosition()));
    guardErrorTimer.setSingleShot(true);
    QObject::connect(&guardErrorTimer, SIGNAL(timeout()), this, SLOT(clearGuardError()));


    /// EFO New




    QVBoxLayout *v = new QVBoxLayout;
    v->setSpacing(2);
    v->setContentsMargins(0,1,1,1);

    QHBoxLayout *pinRow = new QHBoxLayout;
    pinRow->setContentsMargins(4,3,4,0);
    QLabel *panelTitle = new QLabel("CONTROL PANEL");
    GuiFunct::styleEditorTitle(panelTitle);
    pinRow->addWidget(panelTitle);
    pinRow->addStretch();
    pinPositionButton.setCheckable(true);
    pinPositionButton.setChecked(positionPinned);
    pinPositionButton.setFocusPolicy(Qt::NoFocus);
    pinPositionButton.setText(tr("Pin"));
    QFont pinFont = pinPositionButton.font();
    pinFont.setBold(false);
    if(pinFont.pointSizeF() > 0)
        pinFont.setPointSizeF(qMax(7.0, pinFont.pointSizeF() * 0.85));
    pinPositionButton.setFont(pinFont);
    pinPositionButton.setFixedSize(scaledUiSize(30), scaledUiSize(17));
    pinRow->addWidget(&pinPositionButton);
    v->addLayout(pinRow);

    QGridLayout *vbox = new QGridLayout;


/// EFO
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,0,1,0);
    QList<QPushButton*> buttons;
    buttons << &status0 << &status1 << &status2 << &status3 << &status4 << &status5
            << &status6 << &status7 << &status8 << &status9 << &status10 << &status11
            << &status12 << &moveFast << &moveSlow;
    for(int i = 0; i < buttons.size(); i++){
        buttons[i]->setFlat(false);
        buttons[i]->setFocusPolicy(Qt::NoFocus);
        buttons[i]->setFixedHeight(scaledUiSize(21));
        buttons[i]->setContentsMargins(0,0,0,0);
    }
    status10.setFlat(true);

    moveFast.setText("Move: Fast");
    moveSlow.setText("Move: Slow");

    vbox->addWidget(&status4,0,0);
    vbox->addWidget(&status9,0,1);
    vbox->addWidget(&status7,1,0);
    vbox->addWidget(&status8,1,1);
    vbox->addWidget(&status3,2,0);
    vbox->addWidget(&status6,2,1);
    vbox->addWidget(&status1,3,0);
    vbox->addWidget(&status2,3,1);
    vbox->addWidget(&status0,4,0);
    vbox->addWidget(&status5,4,1);
    vbox->addWidget(&moveFast,5,0);
    vbox->addWidget(&moveSlow,5,1);
    vbox->addWidget(&status10,6,0);
    vbox->addWidget(&status11,6,1);
    vbox->addWidget(&status12,7,0,1,2);

    v->addItem(vbox);

    v->addSpacing(scaledUiSize(5));

    QLabel *markerFileLabel = new QLabel("• Marker File");
    markerFileLabel->setContentsMargins(12,0,0,0);
    markerFileLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    QLabel *markerLocationLabel = new QLabel("• Marker Location");
    markerLocationLabel->setContentsMargins(12,0,0,0);
    markerLocationLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    QLabel *positionLabel = new QLabel("• Position");
    positionLabel->setContentsMargins(12,0,0,0);
    positionLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    v->addWidget(markerFileLabel);
    v->addWidget(&markerFiles);
    v->addWidget(markerLocationLabel);
    v->addWidget(&markerList);
    v->addWidget(positionLabel);

    markerFiles.setStyleSheet("combobox-popup: 0;");
    markerList.setStyleSheet("combobox-popup: 0;");
    markerFiles.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    markerList.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QLabel *cameraPosLabel = new QLabel("Camera:");
    QLabel *pointerPosLabel = new QLabel("Pointer:");
    QGridLayout *positionGrid = new QGridLayout;
    positionGrid->setSpacing(2);
    positionGrid->setContentsMargins(3,0,1,0);
    positionGrid->addWidget(pointerPosLabel,0,0);
    positionGrid->addWidget(new QLabel("x"),0,1);
    positionGrid->addWidget(&pxBox,0,2);
    positionGrid->addWidget(new QLabel("y"),0,3);
    positionGrid->addWidget(&pyBox,0,4);
    positionGrid->addWidget(new QLabel("z"),0,5);
    positionGrid->addWidget(&pzBox,0,6);
    pxBox.setReadOnly(true);
    pyBox.setReadOnly(true);
    pzBox.setReadOnly(true);
    if(Game::convertUnitD != 'm'){
        pyBoxx.setReadOnly(true);
        positionGrid->addWidget(&pyBoxx,0,7);
    }
    positionGrid->addWidget(cameraPosLabel,1,0);
    positionGrid->addWidget(new QLabel("x"),1,1);
    positionGrid->addWidget(&xBox,1,2);
    positionGrid->addWidget(new QLabel("y"),1,3);
    positionGrid->addWidget(&yBox,1,4);
    positionGrid->addWidget(new QLabel("z"),1,5);
    positionGrid->addWidget(&zBox,1,6);
    v->addItem(positionGrid);

    QGridLayout *coordinateGrid = new QGridLayout;
    coordinateGrid->setSpacing(2);
    coordinateGrid->setContentsMargins(3,0,1,0);
    coordinateGrid->addWidget(new QLabel("X"),0,0);
    coordinateGrid->addWidget(&txBox,0,1);
    coordinateGrid->addWidget(new QLabel("Y"),0,2);
    coordinateGrid->addWidget(&tyBox,0,3);
    coordinateGrid->addWidget(new QLabel("lat"),1,0);
    coordinateGrid->addWidget(&latBox,1,1);
    coordinateGrid->addWidget(new QLabel("lon"),1,2);
    coordinateGrid->addWidget(&lonBox,1,3);
    v->addItem(coordinateGrid);
    v->addWidget(&tileInfo);
    QPushButton *jumpButton = new QPushButton("Jump", this);
    jumpButton->setFocusPolicy(Qt::NoFocus);
    jumpButton->setMinimumHeight(scaledUiSize(21));
    v->addWidget(jumpButton);


    /// EFO end

    this->setLayout(v);
    this->setStyleSheet(GuiFunct::scoPanelStyle());
    updatePositionPinAppearance();
    markerFileLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    markerLocationLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    positionLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");

    statS = statusButtonStyle("#26292c", "#303438", "#e7eaec", "#383d41", "#191b1d");
    statG = statusButtonStyle("#176c25", "#1e8430", "#f2fff4", "#319344", "#104b1a");
    statY = statusButtonStyle("#b3b300", "#d0d020", "#232323", "#e0e03a", "#707000");
    statReadout = statusReadoutStyle("#26292c", "#e7eaec", "#383d41");
    statReadoutY = statusReadoutStyle("#b3b300", "#232323", "#e0e03a");
    statR = statusButtonStyle("#8d3030", "#a63b3b", "#fff0f0", "#bd5151", "#602020");

    for(int i = 0; i < buttons.size(); i++)
        buttons[i]->setStyleSheet(statS);
    status10.setStyleSheet(statReadout);
    status10.setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QObject::connect(&status4, SIGNAL(released()), this, SLOT(selectButtonAction()));
    QObject::connect(&status9, SIGNAL(released()), this, SLOT(placeButtonAction()));
    QObject::connect(&status7, SIGNAL(released()), this, SLOT(rotateButtonAction()));
    QObject::connect(&status8, SIGNAL(released()), this, SLOT(translateButtonAction()));
    QObject::connect(&status3, SIGNAL(released()), this, SLOT(resizeButtonAction()));
    QObject::connect(&status6, SIGNAL(released()), this, SLOT(stickTerrainButtonAction()));
    QObject::connect(&status1, SIGNAL(released()), this, SLOT(autoTdbButtonAction()));
    QObject::connect(&status2, SIGNAL(released()), this, SLOT(brushDirectionButtonAction()));
    QObject::connect(&status0, SIGNAL(released()), this, SLOT(cameraLockButtonAction()));
    QObject::connect(&status5, SIGNAL(released()), this, SLOT(cameraTerrainButtonAction()));
    QObject::connect(&status11, SIGNAL(released()), this, SLOT(objectSelectedButtonAction()));
    QObject::connect(&status12, SIGNAL(released()), this, SLOT(placeGuardButtonAction()));
    QObject::connect(&moveFast, SIGNAL(released()), this, SLOT(moveFastButtonAction()));
    QObject::connect(&moveSlow, SIGNAL(released()), this, SLOT(moveSlowButtonAction()));
    QObject::connect(&pinPositionButton, SIGNAL(toggled(bool)), this, SLOT(togglePositionPin(bool)));
    QObject::connect(jumpButton, SIGNAL(released()), this, SLOT(jumpTileSelected()));
    QObject::connect(&markerFiles, SIGNAL(activated(QString)), this, SLOT(mkrFilesSelected(QString)));
    QObject::connect(&markerList, SIGNAL(activated(QString)), this, SLOT(mkrListSelected(QString)));
    QObject::connect(&txBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&tyBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&xBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&yBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&zBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&latBox, SIGNAL(textEdited(QString)), this, SLOT(latLonChanged(QString)));
    QObject::connect(&lonBox, SIGNAL(textEdited(QString)), this, SLOT(latLonChanged(QString)));
    tileInfo.setText(" ");

}


StatusWindow::~StatusWindow() {
    qDeleteAll(mkrPlaces);
}

void StatusWindow::hideEvent(QHideEvent *e){
    emit windowClosed();
}

void StatusWindow::moveEvent(QMoveEvent *e){
    QWidget::moveEvent(e);
    if(snapping)
        return;

    snapTimer.start(120);
    if(positionPinned && !positionPersistenceSuspended)
        pinSaveTimer.start(240);
}

void StatusWindow::applyWindowSnap(){
    if(snapping)
        return;

    QPoint snapped = snapWindowPosition(this);
    if(snapped == pos())
        return;

    snapping = true;
    move(snapped);
    snapping = false;
}

void StatusWindow::togglePositionPin(bool pinned){
    positionPinned = pinned;
    updatePositionPinAppearance();

    if(pinned){
        Game::clearPinnedWindowPosition("controlPanelUseDefault");
        Game::savePinnedWindowPosition("controlPanel", pos());
        return;
    }

    pinSaveTimer.stop();
    Game::clearPinnedWindowPosition("controlPanel");
    Game::savePinnedWindowPosition("controlPanelUseDefault", QPoint(0, 0));
    QWidget *mainWindow = parentWidget();
    if(mainWindow != NULL){
        const QPoint defaultPosition(mainWindow->x() - width(), mainWindow->y() + 200);
        positionPersistenceSuspended = true;
        move(Game::visibleWindowPosition(defaultPosition, size()));
        positionPersistenceSuspended = false;
    }
}

void StatusWindow::savePinnedPosition(){
    if(positionPinned && !positionPersistenceSuspended)
        Game::savePinnedWindowPosition("controlPanel", pos());
}

void StatusWindow::setPositionPersistenceSuspended(bool suspended){
    positionPersistenceSuspended = suspended;
    if(suspended)
        pinSaveTimer.stop();
}

void StatusWindow::updatePositionPinAppearance(){
    pinPositionButton.setText(tr("Pin"));
    pinPositionButton.setToolTip(positionPinned
        ? tr("The Control Panel position is saved between sessions. Click to unpin and return to default placement.")
        : tr("Save the current Control Panel position between sessions."));
    if(positionPinned){
        pinPositionButton.setStyleSheet(QString(
            "QToolButton { color: #232323; background-color: %1;"
            " border: 1px solid %1; padding: 0px 3px; font-weight: normal; }"
            "QToolButton:hover { border-color: #e4c5a3; }"
            "QToolButton:pressed { background-color: #a98a69; }").arg(Game::StyleMainLabel));
    } else {
        pinPositionButton.setStyleSheet(
            "QToolButton { color: #e7eaec; background-color: #26292c;"
            " border: 1px solid #383d41; padding: 0px 3px; font-weight: normal; }"
            "QToolButton:hover { background-color: #303438; border-color: #f08200; }"
            "QToolButton:pressed { background-color: #191b1d; }");
    }
}


void StatusWindow::cameraButtonAction(bool val){
    qDebug() << "Camera Clicked: " << val;
    if(val)
        emit enableTool("selectTool");
    else
        emit enableTool("selectTool");
}

void StatusWindow::selectButtonAction(){
    emit statusCommand("select");
}

void StatusWindow::placeButtonAction(){
    emit statusCommand("place");
}

void StatusWindow::rotateButtonAction(){
    emit statusCommand("rotate");
}

void StatusWindow::translateButtonAction(){
    emit statusCommand("translate");
}

void StatusWindow::resizeButtonAction(){
    emit statusCommand("resize");
}

void StatusWindow::autoTdbButtonAction(){
    emit statusCommand("autotdb");
}

void StatusWindow::stickTerrainButtonAction(){
    emit statusCommand("stickterr");
}

void StatusWindow::brushDirectionButtonAction(){
    emit statusCommand("brushdir");
}

void StatusWindow::cameraLockButtonAction(){
    emit statusCommand("camera");
}

void StatusWindow::cameraTerrainButtonAction(){
    emit statusCommand("camterr");
}

void StatusWindow::objectSelectedButtonAction(){
    emit statusCommand("clearselect");
}

void StatusWindow::placeGuardButtonAction(){
    emit statusCommand("guard");
}

void StatusWindow::moveFastButtonAction(){
    emit statusCommand("movefast");
}

void StatusWindow::moveSlowButtonAction(){
    emit statusCommand("moveslow");
}

void StatusWindow::clearGuardError(){
    guardErrorActive = false;
    status12.setText(lastGuardStatus);
    if(lastGuardStatus.endsWith("ON"))
        status12.setStyleSheet(statS);
    else
        status12.setStyleSheet(statY);
}

void StatusWindow::recStatus(QString statName, QString statVal ){
    // These get emitted from REGLW triggers and update here
    if(statName.contains("guarderror")) { guardErrorActive = true; status12.setText("ERROR"); status12.setStyleSheet(statR); guardErrorTimer.start(3000); return; }
    if(statName.contains("camera"))    { statVal.replace("Camera Unlocked", "Camera: FREE"); statVal.replace("Camera Locked", "Camera: LOCK"); statVal.replace("Camera FREE", "Camera: FREE"); statVal.replace("Camera LOCK", "Camera: LOCK"); status0.setText(statVal); if(statVal.endsWith("LOCK")) status0.setStyleSheet(statY); else status0.setStyleSheet(statS);  }
    if(statName.contains("autotdb"))   { status1.setText(statVal); if(statVal.endsWith("ON")) status1.setStyleSheet(statS); else status1.setStyleSheet(statY);  }
    if(statName.contains("brush"))     { status2.setText(statVal); if(statVal.endsWith("+")) status2.setStyleSheet(statG); else if(statVal.endsWith("-")) status2.setStyleSheet(statY); else status2.setStyleSheet(statS); }
    if(statName.contains("resize"))    { status3.setText(statVal); if(statVal.endsWith("ON")) status3.setStyleSheet(statY); else status3.setStyleSheet(statS);  }
    if(statName.contains("select"))    { status4.setText(statVal); if(statVal.endsWith("ON")) status4.setStyleSheet(statG); else status4.setStyleSheet(statS);  }

    if(statName.contains("camterr"))   { statVal.replace("Cam Terrain Unlocked", "Camera Terrain: FREE"); statVal.replace("Cam Terrain Locked", "Camera Terrain: LOCK"); statVal.replace("Cam Terrain FREE", "Camera Terrain: FREE"); statVal.replace("Cam Terrain LOCK", "Camera Terrain: LOCK"); status5.setText(statVal); if(statVal.endsWith("LOCK")) status5.setStyleSheet(statS); else status5.setStyleSheet(statY);  }
    if(statName.contains("stickterr")) { statVal.replace("StickToTerrain", "Stick To Terrain"); status6.setText(statVal); if(statVal.endsWith("ON")) status6.setStyleSheet(statS); else status6.setStyleSheet(statY);  }
    if(statName.contains("rotate"))    { status7.setText(statVal); if(statVal.endsWith("ON")) status7.setStyleSheet(statY); else status7.setStyleSheet(statS);  }
    if(statName.contains("translate")) { status8.setText(statVal); if(statVal.endsWith("ON")) status8.setStyleSheet(statY); else status8.setStyleSheet(statS);  }
    if(statName.contains("place"))     { statVal.replace("Place:", "Place New:"); status9.setText(statVal); if(statVal.endsWith("ON")) status9.setStyleSheet(statG); else status9.setStyleSheet(statS);  }
    if(statName.contains("timer"))     { status10.setText(statVal + "m Since Save"); if(statVal.toInt() > 10) status10.setStyleSheet(statReadoutY); else status10.setStyleSheet(statReadout);  }
    if(statName.contains("object"))    { if(statVal.size() > 0) {status11.setText(statVal + " Selected"); status11.setStyleSheet(statY); } else {status11.setText(""); status11.setStyleSheet(statS);}  }
    if(statName.contains("guard"))     { lastGuardStatus = statVal; if(guardErrorActive) return; status12.setText(statVal); if(statVal.endsWith("ON")) status12.setStyleSheet(statS); else status12.setStyleSheet(statY);  }
    if(statName.contains("movefast"))  { moveFast.setStyleSheet(statVal.endsWith("ON") ? statY : statS); }
    if(statName.contains("moveslow"))  { moveSlow.setStyleSheet(statVal.endsWith("ON") ? statY : statS); }
}

void StatusWindow::latLonChanged(QString){
    jumpType = "latlon";
}

void StatusWindow::xyChanged(QString){
    jumpType = "xy";
}

void StatusWindow::jumpTileSelected(){
    if(aCoords == NULL)
        aCoords = new PreciseTileCoordinate();

    bool jumped = false;
    if(jumpType == "xy"){
        aCoords->setWxyz(xBox.text().toInt(), yBox.text().toInt(), zBox.text().toInt());
        aCoords->TileX = txBox.text().toInt();
        aCoords->TileZ = tyBox.text().toInt();
        emit jumpTo(aCoords);
        jumped = true;
    } else if(jumpType == "latlon"){
        igh = Game::GeoCoordConverter->ConvertToInternal(latBox.text().toDouble(), lonBox.text().toDouble(), igh);
        aCoords = Game::GeoCoordConverter->ConvertToTile(igh, aCoords);
        aCoords->setWxyz();
        aCoords->wZ = -aCoords->wZ;
        emit jumpTo(aCoords);
        jumped = true;
    } else if(jumpType == "marker"){
        LatitudeLongitudeCoordinate *place = mkrPlaces.value(markerList.currentText(), NULL);
        if(place == NULL)
            return;
        igh = Game::GeoCoordConverter->ConvertToInternal(place->Latitude, place->Longitude, igh);
        aCoords = Game::GeoCoordConverter->ConvertToTile(igh, aCoords);
        aCoords->setWxyz();
        aCoords->wZ = -aCoords->wZ;
        emit jumpTo(aCoords);
        jumped = true;
    }

    if(jumped){
        emit jumpSoundRequested();
        emit requestMainFocus();
    }
}

void StatusWindow::naviInfo(int all, int hidden){
    if(all == objCount && hidden == objHidden)
        return;
    objCount = all;
    objHidden = hidden;
    tileInfo.setText("Objects: " + QString::number(all) + " (including " + QString::number(hidden) + " hidden)");
}

void StatusWindow::pointerInfo(float* coords){
    if(pointerInfoValid && lastPX == coords[0] && lastPY == coords[1] && lastPZ == coords[2])
        return;
    lastPX = coords[0];
    lastPY = coords[1];
    lastPZ = coords[2];
    pointerInfoValid = true;
    pxBox.setText(QString::number(coords[0]));
    pyBox.setText(QString::number(coords[1]));
    pyBoxx.setText(QString::number(coords[1] * Game::convertDistance, 'f', 0) + " " + Game::convertUnitD);
    pzBox.setText(QString::number(-coords[2]));
}

void StatusWindow::posInfo(PreciseTileCoordinate* coords){
    if(posInfoValid && lastX == coords->wX && lastY == coords->wY && lastZ == coords->wZ &&
       lastTX == coords->TileX && lastTZ == coords->TileZ)
        return;

    lastX = coords->wX;
    lastY = coords->wY;
    lastZ = coords->wZ;
    lastTX = coords->TileX;
    lastTZ = coords->TileZ;
    posInfoValid = true;
    txBox.setText(QString::number(lastTX));
    tyBox.setText(QString::number(lastTZ));
    xBox.setText(QString::number(lastX));
    yBox.setText(QString::number(lastY));
    zBox.setText(QString::number(-lastZ));
    igh = Game::GeoCoordConverter->ConvertToInternal(coords);
    latlon = Game::GeoCoordConverter->ConvertToLatLon(igh);
    latBox.setText(QString::number(latlon->Latitude));
    lonBox.setText(QString::number(latlon->Longitude));
}

void StatusWindow::reloadMkrLists(){
    if(Game::debugOutput)
        qDebug() << "Control Panel marker list" << Game::markerFiles;
}

void StatusWindow::mkrList(QMap<QString, Coords*> list){
    markerFiles.clear();
    mkrFiles = list;
    const QString routeId = Game::route.toLower() + ".mkr";

    for(auto it = list.begin(); it != list.end(); ++it){
        if(it.value() != NULL && it.value()->loaded)
            markerFiles.addItem(it.key());
    }

    if(markerFiles.count() == 0)
        return;
    int routeIndex = markerFiles.findText(routeId);
    if(routeIndex < 0)
        routeIndex = 0;
    markerFiles.setCurrentIndex(routeIndex);
    mkrFilesSelected(markerFiles.itemText(routeIndex));
}

void StatusWindow::mkrFilesSelected(QString item){
    Coords *coords = mkrFiles.value(item, NULL);
    if(coords == NULL)
        return;

    emit sendMsg("mkrFile", item);
    qDeleteAll(mkrPlaces);
    mkrPlaces.clear();
    QStringList names;
    for(int i = 0; i < coords->markerList.size(); i++){
        LatitudeLongitudeCoordinate *place = new LatitudeLongitudeCoordinate();
        place->Latitude = coords->markerList[i].lat;
        place->Longitude = coords->markerList[i].lon;
        mkrPlaces.insert(coords->markerList[i].name, place);
        names.append(coords->markerList[i].name);
    }
    names.sort(Qt::CaseInsensitive);
    names.removeDuplicates();
    markerList.clear();
    markerList.addItems(names);
    markerList.setMaxVisibleItems(25);
}

void StatusWindow::mkrListSelected(QString){
    jumpType = "marker";
}


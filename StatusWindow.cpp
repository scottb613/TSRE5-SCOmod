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
#include <functional>

static int scaledUiSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
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

class PairedCoordinateReadouts : public QObject {
public:
    PairedCoordinateReadouts(QLineEdit *first, QLineEdit *second,
                             const std::function<void()> &copiedCallback,
                             QObject *parent)
        : QObject(parent), first(first), second(second),
          copiedCallback(copiedCallback) {
        if(first != NULL)
            first->installEventFilter(this);
        if(second != NULL)
            second->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if(watched != first && watched != second)
            return QObject::eventFilter(watched, event);

        if(event->type() == QEvent::Enter)
            setPairHovered(true);
        else if(event->type() == QEvent::Leave)
            setPairHovered(false);
        else if(event->type() == QEvent::MouseButtonRelease){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton
            && first != NULL && second != NULL){
                QApplication::clipboard()->setText(
                    first->text() + '\t' + second->text(),
                    QClipboard::Clipboard);
                if(copiedCallback)
                    copiedCallback();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void setPairHovered(bool hovered){
        const QList<QLineEdit*> fields = { first, second };
        for(QLineEdit *field : fields){
            if(field == NULL)
                continue;
            field->setProperty("coordinatePairHover", hovered);
            field->style()->unpolish(field);
            field->style()->polish(field);
            field->update();
        }
    }

    QLineEdit *first = NULL;
    QLineEdit *second = NULL;
    std::function<void()> copiedCallback;
};

StatusWindow::StatusWindow(QWidget* parent) : QWidget(parent) {
    this->setWindowFlags(Qt::WindowType::Tool);
    this->setFixedWidth(scaledUiSize(300));
    this->setFixedHeight(scaledUiSize(442));
    GuiFunct::setEditorToolWindowTitle(this);
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
    GuiFunct::setupWindowPinButton(&pinPositionButton);
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
    vbox->setColumnStretch(0, 1);
    vbox->setColumnStretch(1, 1);
    QList<QPushButton*> buttons;
    buttons << &status0 << &status1 << &status2 << &status3 << &status4 << &status5
            << &status6 << &status7 << &status8 << &status9 << &status10 << &status11
            << &status12 << &moveFast << &moveSlow;
    for(int i = 0; i < buttons.size(); i++){
        buttons[i]->setFlat(false);
        buttons[i]->setFocusPolicy(Qt::NoFocus);
        // The status text changes at runtime.  Ignore its size hint so the
        // two grid columns remain equal instead of Select shrinking beside
        // the longer "Place New" label.
        buttons[i]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
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

    const int cardHorizontalPadding = scaledUiSize(5);
    const int cardVerticalPadding = scaledUiSize(3);
    positionLabel = new QLabel(QString(QChar(0x2022)) + " Position");
    QLabel *jumpLabel = new QLabel(QString(QChar(0x2022)) + " Jump");
    GuiFunct::styleEditorSubtitle(positionLabel);
    GuiFunct::styleEditorSubtitle(jumpLabel);
    positionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    jumpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    markerFiles.setStyleSheet("combobox-popup: 0;");
    markerList.setStyleSheet("combobox-popup: 0;");
    markerFiles.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    markerList.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QGridLayout *positionGrid = new QGridLayout;
    positionGrid->setSpacing(2);
    positionGrid->setContentsMargins(3,0,1,0);
    positionGrid->setColumnStretch(1,1);
    positionGrid->setColumnStretch(3,1);
    positionGrid->addWidget(new QLabel("Cam X:"),0,0);
    positionGrid->addWidget(&txBox,0,1);
    positionGrid->addWidget(new QLabel("Cam Y:"),0,2);
    positionGrid->addWidget(&tyBox,0,3);
    positionGrid->addWidget(new QLabel("Cam Lat:"),1,0);
    positionGrid->addWidget(&latBox,1,1);
    positionGrid->addWidget(new QLabel("Cam Lon:"),1,2);
    positionGrid->addWidget(&lonBox,1,3);
    positionGrid->addWidget(new QLabel("Pnt X:"),2,0);
    positionGrid->addWidget(&pxBox,2,1);
    positionGrid->addWidget(new QLabel("Pnt Y:"),2,2);
    positionGrid->addWidget(&pyBox,2,3);
    positionGrid->addWidget(new QLabel("Pnt Elev:"),3,0);
    positionGrid->addWidget(&pyBoxx,3,1);
    positionGrid->addWidget(new QLabel("Pnt Z:"),3,2);
    positionGrid->addWidget(&pzBox,3,3);
    pxBox.setReadOnly(true);
    pyBox.setReadOnly(true);
    pzBox.setReadOnly(true);
    txBox.setReadOnly(true);
    tyBox.setReadOnly(true);
    latBox.setReadOnly(true);
    lonBox.setReadOnly(true);
    txBox.setFocusPolicy(Qt::NoFocus);
    tyBox.setFocusPolicy(Qt::NoFocus);
    latBox.setFocusPolicy(Qt::NoFocus);
    lonBox.setFocusPolicy(Qt::NoFocus);
    if(Game::convertUnitD != 'm'){
        pyBoxx.setReadOnly(true);
    }
    const QList<QLineEdit*> numericFields = {
        &pxBox, &pyBox, &pzBox, &pyBoxx,
        &txBox, &tyBox, &latBox, &lonBox
    };
    const QString readOnlyStatusStyle =
        "QLineEdit { background-color: #252525; border: 1px solid #444444;"
        " color: #f0f0f0; padding: 1px 4px; selection-background-color: #555555;"
        " selection-color: #ffffff; }"
        "QLineEdit:focus { border: 1px solid #444444; }"
        "QLineEdit[coordinatePairHover=\"true\"] {"
        " background-color: #303438; border: 1px solid #f08200;"
        " color: #ffffff; }";
    for(QLineEdit *field : numericFields){
        field->setAlignment(Qt::AlignRight);
        field->setStyleSheet(readOnlyStatusStyle);
    }
    copiedStatusTimer.setSingleShot(true);
    QObject::connect(&copiedStatusTimer, &QTimer::timeout, this, [this](){
        if(positionLabel == NULL)
            return;
        positionLabel->setText(QString(QChar(0x2022)) + " Position");
        positionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        GuiFunct::styleEditorSubtitle(positionLabel);
    });
    const std::function<void()> showCopiedStatus = [this](){
        if(positionLabel == NULL)
            return;
        positionLabel->setText(QString(QChar(0x2022)) + " COPIED "
                               + QString(QChar(0x2022)));
        positionLabel->setAlignment(Qt::AlignCenter);
        positionLabel->setStyleSheet(
            GuiFunct::editorSubtitleStyle()
            + " QLabel { color: #f08200; }");
        copiedStatusTimer.start(2000);
    };
    new PairedCoordinateReadouts(&txBox, &tyBox, showCopiedStatus, this);
    new PairedCoordinateReadouts(&latBox, &lonBox, showCopiedStatus, this);

    QFrame *positionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(positionCard);
    positionCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout *positionCardLayout = new QVBoxLayout(positionCard);
    positionCardLayout->setContentsMargins(
        cardHorizontalPadding, cardVerticalPadding,
        cardHorizontalPadding, cardVerticalPadding);
    positionCardLayout->setSpacing(scaledUiSize(2));
    positionCardLayout->addLayout(positionGrid);
    v->addWidget(positionLabel);
    v->addWidget(positionCard);

    QPushButton *jumpButton = new QPushButton("Engage", this);
    jumpButton->setFocusPolicy(Qt::NoFocus);
    GuiFunct::styleEditorActionButton(jumpButton);
    QFrame *engageCell = new QFrame(this);
    GuiFunct::styleEditorPanelCard(engageCell);
    QVBoxLayout *engageLayout = new QVBoxLayout(engageCell);
    engageLayout->setContentsMargins(
        cardHorizontalPadding, cardVerticalPadding,
        cardHorizontalPadding, cardVerticalPadding);
    engageLayout->addWidget(jumpButton);

    QFrame *jumpCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(jumpCard);
    jumpCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QGridLayout *jumpLayout = new QGridLayout(jumpCard);
    jumpLayout->setContentsMargins(
        cardHorizontalPadding, cardVerticalPadding,
        cardHorizontalPadding, cardVerticalPadding);
    jumpLayout->setHorizontalSpacing(scaledUiSize(4));
    jumpLayout->setVerticalSpacing(scaledUiSize(3));
    QLabel *markerFileFieldLabel = new QLabel("Marker File:");
    QLabel *markerLocationFieldLabel = new QLabel("Location:");
    jumpLayout->addWidget(markerFileFieldLabel, 0, 0);
    jumpLayout->addWidget(&markerFiles, 0, 1);
    jumpLayout->addWidget(markerLocationFieldLabel, 1, 0);
    jumpLayout->addWidget(&markerList, 1, 1);
    jumpLayout->addWidget(engageCell, 2, 0, 1, 2);
    jumpLayout->setColumnStretch(1, 1);
    v->addWidget(jumpLabel);
    v->addWidget(jumpCard);


    /// EFO end

    this->setLayout(v);
    this->setStyleSheet(GuiFunct::scoEditorPanelStyle());
    GuiFunct::styleEditorActionButton(jumpButton);
    updatePositionPinAppearance();

    statS = statusButtonStyle("#26292c", "#303438", "#e7eaec", "#383d41", "#191b1d");
    statG = statusButtonStyle(Game::StyleGreenButton, Game::StyleGreenButtonHover,
                              "#232323", Game::StyleGreenButtonHover, "#356f43");
    statY = statusButtonStyle(Game::StyleYellowButton, Game::StyleYellowButtonHover,
                              "#232323", Game::StyleYellowButtonHover, "#75632f");
    statC = statusButtonStyle(Game::StyleBlueButton, Game::StyleBlueButtonHover,
                              "#232323", Game::StyleBlueButtonHover, "#315f80");
    statReadout = statusReadoutStyle("#26292c", "#e7eaec", "#383d41");
    statReadoutY = statusReadoutStyle(
        Game::StyleYellowButton, "#232323", Game::StyleYellowButtonHover);
    statR = statusButtonStyle(Game::StyleRedButton, Game::StyleRedButtonHover,
                              "#232323", Game::StyleRedButtonHover, "#743737");

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
    QObject::connect(&markerFiles, SIGNAL(textActivated(QString)), this, SLOT(mkrFilesSelected(QString)));
    QObject::connect(&markerList, SIGNAL(textActivated(QString)), this, SLOT(mkrListSelected(QString)));
    QObject::connect(&txBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&tyBox, SIGNAL(textEdited(QString)), this, SLOT(xyChanged(QString)));
    QObject::connect(&latBox, SIGNAL(textEdited(QString)), this, SLOT(latLonChanged(QString)));
    QObject::connect(&lonBox, SIGNAL(textEdited(QString)), this, SLOT(latLonChanged(QString)));
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

    QPoint snapped = GuiFunct::snappedWindowPosition(this);
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
    GuiFunct::setupWindowPinButton(&pinPositionButton);
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

void StatusWindow::setPrimaryEditorToolsEnabled(bool enabled){
    status4.setEnabled(enabled);
    status9.setEnabled(enabled);
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
    if(statName.contains("movefast"))  { moveFast.setStyleSheet(statVal.endsWith("ON") ? statC : statS); }
    if(statName.contains("moveslow"))  { moveSlow.setStyleSheet(statVal.endsWith("ON") ? statC : statS); }
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
        aCoords->setWxyz(lastX, lastY, -lastZ);
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

void StatusWindow::pointerInfo(float* coords){
    if(pointerInfoValid && lastPX == coords[0] && lastPY == coords[1] && lastPZ == coords[2])
        return;
    lastPX = coords[0];
    lastPY = coords[1];
    lastPZ = coords[2];
    pointerInfoValid = true;
    pxBox.setText(QString::number(coords[0]));
    pyBox.setText(QString::number(coords[1]));
    pyBoxx.setText(QString::number(coords[1] * Game::convertDistance, 'f', 2) + " " + Game::convertUnitD);
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
    markerList.clear();
    qDeleteAll(mkrPlaces);
    mkrPlaces.clear();
    mkrFiles = list;
    const QString routeId = Game::routeName.toLower() + ".mkr";

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


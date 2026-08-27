/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesTrackObj.h"
#include "WorldObj.h"
#include "TrackObj.h"
#include "SignalObj.h"
#include <math.h>
#include "GLMatrix.h"
#include "ParserX.h"
#include "EditFileNameDialog.h"
#include "Undo.h"
#include "Game.h"
#include "Route.h"
#include "GuiFunct.h"
#include "TextEditDialog.h"
#include <QScreen>

static int gradeHelperScaledSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
}

static QString gradeHelperStateStyle(const QString &background, const QString &border, const QString &text){
    return QString(
        "QPushButton { color: %3; background-color: %1; border: 1px solid %2;"
        " border-radius: 2px; padding: 3px 5px; }"
        "QPushButton:hover { border-color: #f08200; }"
        "QPushButton:pressed { background-color: #202020; }"
    ).arg(background, border, text);
}

class GradeHelperWindow : public EditorPopupWindow {
public:
    explicit GradeHelperWindow(PropertiesTrackObj *owner)
        : EditorPopupWindow(owner, "GRADE HELPER", "gradeHelper"),
          owner(owner) {
        QVBoxLayout *layout = popupLayout();
        setPopupPinToolTips(
            "Save the current Grade Helper position between sessions.",
            "The Grade Helper position is saved between sessions. Click to return to default placement.");
        addPopupSubtitle(QString::fromUtf8("• Transition"));
        QFrame *transitionCard = new QFrame;
        GuiFunct::styleEditorPanelCard(transitionCard);
        QFormLayout *form = new QFormLayout(transitionCard);
        form->setSpacing(3);
        form->setContentsMargins(4,3,4,3);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        currentGrade.setReadOnly(true);
        nextGrade.setReadOnly(true);
        currentGrade.setAlignment(Qt::AlignCenter);
        nextGrade.setAlignment(Qt::AlignCenter);
        const QString gradeInputStyle =
            "QDoubleSpinBox { border: 1px solid #70590e; }"
            "QDoubleSpinBox:hover { border: 1px solid #f08200; }"
            "QDoubleSpinBox:focus { border: 1px solid #8a7116; }"
            "QDoubleSpinBox:focus:hover { border: 1px solid #f08200; }";
        targetGrade.setStyleSheet(gradeInputStyle);
        stepGrade.setStyleSheet(gradeInputStyle);
        targetGrade.setToolTip("Enter the grade you want the transition to reach.");
        stepGrade.setToolTip("Enter the grade change to apply to each placed piece.");
        form->addRow("Current Grade:", &currentGrade);
        form->addRow(&targetGradeLabel, &targetGrade);
        form->addRow(&stepGradeLabel, &stepGrade);
        form->addRow("Next Grade:", &nextGrade);
        layout->addWidget(transitionCard);

        QFont fieldFont = font();
        fieldFont.setBold(false);
        const int fieldHeight = gradeHelperScaledSize(22);
        currentGrade.setFont(fieldFont);
        targetGrade.setFont(fieldFont);
        stepGrade.setFont(fieldFont);
        nextGrade.setFont(fieldFont);
        currentGrade.setMinimumHeight(fieldHeight);
        targetGrade.setMinimumHeight(fieldHeight);
        stepGrade.setMinimumHeight(fieldHeight);
        nextGrade.setMinimumHeight(fieldHeight);

        addPopupSubtitle(QString::fromUtf8("• Grade Assist"));
        QFrame *stateCard = new QFrame;
        GuiFunct::styleEditorPanelCard(stateCard);
        QVBoxLayout *stateLayout = new QVBoxLayout(stateCard);
        stateLayout->setContentsMargins(4,3,4,3);
        stateButton.setFocusPolicy(Qt::NoFocus);
        GuiFunct::styleEditorActionButton(&stateButton);
        stateLayout->addWidget(&stateButton);
        layout->addWidget(stateCard);

        QObject::connect(&refreshTimer, &QTimer::timeout, this, [this](){ syncUi(); });
        refreshTimer.start(120);

        QObject::connect(&targetGrade, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value){
            if(!Game::gradeAssistEnabled && !Game::gradeAssistTargetReached)
                Game::gradeAssistTargetPercent = displayGradeToPercent(value);
        });
        QObject::connect(&stepGrade, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value){
            if(!Game::gradeAssistEnabled && !Game::gradeAssistTargetReached)
                Game::gradeAssistStepPercent = displayStepToPercent(value);
        });
        QObject::connect(&stateButton, &QPushButton::clicked, this, [this](){
            this->owner->userButtonPressed();
            if(Game::gradeAssistEnabled){
                Game::gradeAssistEnabled = false;
                Game::gradeAssistTargetReached = false;
                Game::gradeLockEnabled = false;
                Game::gradeAssistNextPercent = Game::gradeAssistCurrentPercent;
            } else if(Game::gradeAssistTargetReached){
                Game::gradeAssistTargetReached = false;
                Game::gradeLockEnabled = false;
                Game::gradeAssistNextPercent = Game::gradeAssistCurrentPercent;
            } else {
                Game::gradeAssistTargetPercent = displayGradeToPercent(targetGrade.value());
                Game::gradeAssistStepPercent = displayStepToPercent(stepGrade.value());
                Game::gradeAssistEnabled = true;
                Game::gradeAssistTargetReached = false;
                Game::gradeLockEnabled = false;
                prepareNextGrade();
                if(qAbs(Game::gradeAssistTargetPercent - Game::gradeAssistCurrentPercent) <= 0.0005f){
                    Game::gradeAssistEnabled = false;
                    Game::gradeAssistTargetReached = true;
                    Game::gradeAssistCurrentPercent = Game::gradeAssistTargetPercent;
                    Game::gradeAssistNextPercent = Game::gradeAssistTargetPercent;
                    Game::gradeLockEnabled = true;
                    Game::gradeLockedPercent = Game::gradeAssistTargetPercent;
                } else {
                    this->owner->activateGradeAssistPlacement();
                }
            }
            syncUi();
            this->owner->requestMainFocus();
        });
        syncUi();
        finalizePopup();
    }

    void showForGrade(float grade){
        if(!Game::gradeAssistEnabled){
            Game::gradeAssistCurrentPercent = grade;
            Game::gradeAssistNextPercent = grade;
            Game::gradeAssistTargetReached = false;
            if(!Game::gradeAssistInitialized){
                Game::gradeAssistTargetPercent = grade;
                Game::gradeAssistInitialized = true;
            }
        }
        syncUi();
        if(!everShown && !isPopupPositionPinned()){
            moveToDefaultPosition();
            everShown = true;
        }
        showExclusive();
        targetGrade.setFocus();
        targetGrade.selectAll();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        owner->gradeHelperWindowClosed();
        owner->requestMainFocus();
    }

private:
    void popupPinChanged(bool pinned) override {
        if(pinned)
            Game::clearPinnedWindowPosition("gradeHelperUseDefault");
        EditorPopupWindow::popupPinChanged(pinned);
        if(!pinned){
            Game::savePinnedWindowPosition(
                "gradeHelperUseDefault", QPoint(0, 0));
            moveToDefaultPosition();
        }
    }

    void popupPinClicked() override {
        owner->userButtonPressed();
        owner->requestMainFocus();
    }

    void moveToDefaultPosition(){
        for(QWidget *candidate : QApplication::topLevelWidgets()){
            if(candidate != this && candidate->isVisible() && candidate->windowTitle() == "Control Panel"){
                QRect control = candidate->frameGeometry();
                setFixedWidth(candidate->width());
                move(Game::visibleWindowPosition(QPoint(control.right() + 1, control.top()), size()));
                return;
            }
        }
        QWidget *mainWindow = owner->window();
        if(mainWindow != NULL){
            const QPoint fallback(mainWindow->x() + mainWindow->width() - width(),
                                  mainWindow->y() + 80);
            move(Game::visibleWindowPosition(fallback, size()));
        }
    }

    float percentToDisplayGrade(float percent) const{
        switch(owner->gradeUnitsIndex()){
        case 0: return percent * 10.0f;
        case 2: return qFuzzyIsNull(percent) ? 0.0f : 100.0f / percent;
        case 3: return qRadiansToDegrees(qAtan(percent / 100.0f));
        default: return percent;
        }
    }

    float displayGradeToPercent(double value) const{
        float percent = (float)value;
        switch(owner->gradeUnitsIndex()){
        case 0: percent /= 10.0f; break;
        case 2: percent = qFuzzyIsNull(percent) ? 0.0f : 100.0f / percent; break;
        case 3: percent = qTan(qDegreesToRadians(percent)) * 100.0f; break;
        default: break;
        }
        const float maximum = Game::trackElevationMaxPm / 10.0f;
        return qBound(-maximum, percent, maximum);
    }

    float displayStepToPercent(double value) const{
        return qAbs(displayGradeToPercent(value));
    }

    void updateTargetGradeUnits(){
        const int units = owner->gradeUnitsIndex();
        if(units == displayedUnits)
            return;
        displayedUnits = units;
        targetGrade.blockSignals(true);
        stepGrade.blockSignals(true);
        targetGrade.setDecimals(5);
        stepGrade.setDecimals(5);
        if(units == 0){
            targetGradeLabel.setText("Target Grade (‰):");
            stepGradeLabel.setText("Step Per Piece (‰):");
            targetGrade.setRange(-Game::trackElevationMaxPm, Game::trackElevationMaxPm);
            targetGrade.setSingleStep(0.5);
            stepGrade.setRange(0.01, Game::trackElevationMaxPm);
            stepGrade.setSingleStep(0.5);
        } else if(units == 2){
            targetGradeLabel.setText("Target Grade (m):");
            stepGradeLabel.setText("Step Per Piece (m):");
            targetGrade.setRange(-100000.0, 100000.0);
            targetGrade.setSingleStep(1.0);
            stepGrade.setRange(1000.0 / Game::trackElevationMaxPm, 100000.0);
            stepGrade.setSingleStep(1.0);
        } else if(units == 3){
            targetGradeLabel.setText("Target Grade (°):");
            stepGradeLabel.setText("Step Per Piece (°):");
            const double maximumAngle = qRadiansToDegrees(qAtan(Game::trackElevationMaxPm / 1000.0));
            targetGrade.setRange(-maximumAngle, maximumAngle);
            targetGrade.setSingleStep(0.05);
            const double minimumAngle = qRadiansToDegrees(qAtan(0.001 / 100.0));
            stepGrade.setRange(minimumAngle, maximumAngle);
            stepGrade.setSingleStep(0.01);
        } else {
            targetGradeLabel.setText("Target Grade (%):");
            stepGradeLabel.setText("Step Per Piece (%):");
            const double maximumPercent = Game::trackElevationMaxPm / 10.0;
            targetGrade.setRange(-maximumPercent, maximumPercent);
            targetGrade.setSingleStep(0.05);
            stepGrade.setRange(0.001, maximumPercent);
            stepGrade.setSingleStep(0.05);
        }
        targetGrade.setValue(percentToDisplayGrade(Game::gradeAssistTargetPercent));
        stepGrade.setValue(percentToDisplayGrade(Game::gradeAssistStepPercent));
        targetGrade.blockSignals(false);
        stepGrade.blockSignals(false);
    }

    void prepareNextGrade(){
        const float difference = Game::gradeAssistTargetPercent - Game::gradeAssistCurrentPercent;
        const float amount = qMin(qAbs(difference), Game::gradeAssistStepPercent);
        Game::gradeAssistNextPercent = Game::gradeAssistCurrentPercent + (difference >= 0.0f ? amount : -amount);
    }

    void syncUi(){
        updateTargetGradeUnits();
        currentGrade.setText(QString::number(Game::gradeAssistCurrentPercent, 'f', 5) + " %");
        nextGrade.setText(QString::number(Game::gradeAssistNextPercent, 'f', 5) + " %");
        if(!targetGrade.hasFocus()){
            targetGrade.blockSignals(true);
            targetGrade.setValue(percentToDisplayGrade(Game::gradeAssistTargetPercent));
            targetGrade.blockSignals(false);
        }
        if(!stepGrade.hasFocus()){
            stepGrade.blockSignals(true);
            stepGrade.setValue(percentToDisplayGrade(Game::gradeAssistStepPercent));
            stepGrade.blockSignals(false);
        }
        const bool editingEnabled = !Game::gradeAssistEnabled && !Game::gradeAssistTargetReached;
        targetGrade.setEnabled(editingEnabled);
        stepGrade.setEnabled(editingEnabled);
        if(Game::gradeAssistEnabled){
            stateButton.setText("Grade Assist Active - Click to Stop");
            stateButton.setStyleSheet(
                gradeHelperStateStyle(Game::StyleYellowButton,
                                      Game::StyleYellowButtonHover, "#232323"));
        } else if(Game::gradeAssistTargetReached){
            stateButton.setText("Grade Achieved - Holding Target");
            stateButton.setStyleSheet(
                gradeHelperStateStyle(Game::StyleGreenButton,
                                      Game::StyleGreenButtonHover, "#232323"));
        } else {
            stateButton.setText("Start Grade Assist");
            GuiFunct::styleEditorActionButton(&stateButton);
        }
    }

    PropertiesTrackObj *owner;
    QLineEdit currentGrade;
    QLabel targetGradeLabel;
    QLabel stepGradeLabel;
    QDoubleSpinBox targetGrade;
    QDoubleSpinBox stepGrade;
    QLineEdit nextGrade;
    QPushButton stateButton;
    QTimer refreshTimer;
    bool everShown = false;
    int displayedUnits = -1;
};

class HacksWindow : public EditorPopupWindow {
public:
    explicit HacksWindow(PropertiesTrackObj *owner)
        : EditorPopupWindow(owner, "HACKS", "hacksHelper"),
          owner(owner) {
        QVBoxLayout *layout = popupLayout();
        setPopupPinToolTips(
            "Save the current Hacks position between sessions.",
            "The Hacks position is saved between sessions. Click to return to default placement.");

        QLabel *warning = new QLabel(
            "Specialized repair and route-cleanup tools. Make a full backup first.");
        warning->setWordWrap(true);
        warning->setStyleSheet("QLabel { color: #c7c7c7; padding: 1px 3px; }");
        layout->addWidget(warning);

        addPopupSubtitle(QString::fromUtf8("• Selected Track"));

        fixJNodePosn = new QPushButton("Repair Junction");
        removeTdbVector = new QPushButton("Remove Vector");
        removeTdbTree = new QPushButton("Remove Branch");
        fixElevation = new QPushButton("Repair Elevation");
        fixJNodePosn->setToolTip("Repairs the selected track object's JNodePosn data.");
        removeTdbVector->setToolTip(
            "Removes the selected TrackDB vector. Remove its interactive items first.");
        removeTdbTree->setToolTip(
            "Removes the connected TrackDB tree after its interactive items are removed. "
            "Limited to 1000 nodes.");
        fixElevation->setToolTip(
            "Repairs the selected TrackDB vector's stored section elevations (sElev).");

        QFrame *trackCard = new QFrame;
        GuiFunct::styleEditorPanelCard(trackCard);
        QGridLayout *trackActions = new QGridLayout(trackCard);
        trackActions->setSpacing(2);
        trackActions->setContentsMargins(4, 3, 4, 3);
        GuiFunct::styleEditorActionButton(fixJNodePosn);
        GuiFunct::styleEditorActionButton(fixElevation);
        GuiFunct::styleEditorActionButton(removeTdbVector);
        GuiFunct::styleEditorActionButton(removeTdbTree);
        trackActions->addWidget(fixJNodePosn, 0, 0);
        trackActions->addWidget(fixElevation, 0, 1);
        trackActions->addWidget(removeTdbVector, 1, 0);
        trackActions->addWidget(removeTdbTree, 1, 1);
        layout->addWidget(trackCard);

        addPopupSubtitle(QString::fromUtf8("• Selected Signal"));

        QFrame *signalCard = new QFrame;
        GuiFunct::styleEditorPanelCard(signalCard);
        QVBoxLayout *signalActions = new QVBoxLayout(signalCard);
        signalActions->setSpacing(2);
        signalActions->setContentsMargins(4, 3, 4, 3);
        fixSignalFlags = new QPushButton("Fix Signal Flags");
        fixSignalFlags->setToolTip(
            "Checks the selected signal's TrSignalType flags, previews each old/new "
            "change, and applies the repairs only after confirmation.");
        GuiFunct::styleEditorActionButton(fixSignalFlags);
        signalActions->addWidget(fixSignalFlags);
        layout->addWidget(signalCard);

        addPopupSubtitle(QString::fromUtf8("• Route Cleanup"));

        QFrame *cleanupCard = new QFrame;
        GuiFunct::styleEditorPanelCard(cleanupCard);
        QVBoxLayout *cleanupLayout = new QVBoxLayout(cleanupCard);
        cleanupLayout->setSpacing(2);
        cleanupLayout->setContentsMargins(4, 3, 4, 3);

        QLabel *routeHelp = new QLabel(
            "Full-route cleanup tools load every world tile before making changes.");
        routeHelp->setWordWrap(true);
        routeHelp->setStyleSheet("QLabel { color: #c7c7c7; padding: 0px 3px 2px 3px; }");
        cleanupLayout->addWidget(routeHelp);

        QPushButton *removeInteractives = new QPushButton("Remove All Interactives");
        removeInteractives->setToolTip(
            "Loads every world tile, then removes all TrackDB/RoadDB-linked interactive "
            "world objects. Track and road geometry are not removed.");
        GuiFunct::styleEditorActionButton(removeInteractives);
        cleanupLayout->addWidget(removeInteractives);

        deleteInstances = new QPushButton("Delete Instances");
        deleteInstances->setToolTip(
            "Deletes every route instance of the selected static object. "
            "The object type and filename must both match.");
        GuiFunct::styleEditorActionButton(deleteInstances);
        cleanupLayout->addWidget(deleteInstances);

        QPushButton *resetTerrtex = new QPushButton("Reset All TERRTEX");
        resetTerrtex->setToolTip(
            "Resets every route terrain patch to terrain.ace and removes "
            "matching per-tile TERRTEX paint files.");
        GuiFunct::styleEditorActionButton(resetTerrtex);
        cleanupLayout->addWidget(resetTerrtex);

        QPushButton *waterTilesOff = new QPushButton("Water Tiles Off");
        waterTilesOff->setToolTip(
            "Turns off every water patch in every terrain tile across the route.");
        GuiFunct::styleEditorActionButton(waterTilesOff);
        cleanupLayout->addWidget(waterTilesOff);

        QPushButton *deletePolyVegBakes =
            new QPushButton("Delete All PolyVeg Bakes");
        deletePolyVegBakes->setToolTip(
            "Loads every world tile and deletes every PolyVeg bake object. "
            "The deletion remains Undo-friendly until the route is saved.");
        GuiFunct::styleEditorActionButton(deletePolyVegBakes);
        cleanupLayout->addWidget(deletePolyVegBakes);
        layout->addWidget(cleanupCard);

        QObject::connect(fixJNodePosn, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->fixJNodePosnEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(removeTdbVector, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->haxRemoveTDBVectorEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(removeTdbTree, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->haxRemoveTDBTreeEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(fixElevation, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->haxElevTDBVectorEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(removeInteractives, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->removeAllInteractivesEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(deleteInstances, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->deleteSelectedInstancesEnabled();
            this->owner->requestMainFocus();
        });
        QObject::connect(resetTerrtex, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            emit this->owner->resetRouteTerrtexRequested();
            this->owner->requestMainFocus();
        });
        QObject::connect(waterTilesOff, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            emit this->owner->disableRouteWaterRequested();
            this->owner->requestMainFocus();
        });
        QObject::connect(deletePolyVegBakes, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            emit this->owner->deleteAllPolyVegBakesRequested();
            this->owner->requestMainFocus();
        });
        QObject::connect(fixSignalFlags, &QPushButton::clicked, owner, [this](){
            this->owner->userButtonPressed();
            this->owner->fixSignalFlagsEnabled();
            this->owner->requestMainFocus();
        });

        syncSelection();
        finalizePopup();
    }

    void showForOwner(){
        syncSelection();
        if(!everShown && !isPopupPositionPinned()){
            moveToDefaultPosition();
            everShown = true;
        }
        showExclusive();
    }

    void refreshSelection(){
        syncSelection();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        owner->hacksWindowClosed();
        owner->requestMainFocus();
    }

private:
    void syncSelection(){
        const bool trackSelected = owner->hasSelectedTrackForHacks();
        fixJNodePosn->setEnabled(trackSelected);
        removeTdbVector->setEnabled(trackSelected);
        removeTdbTree->setEnabled(trackSelected);
        fixElevation->setEnabled(trackSelected);
        deleteInstances->setEnabled(owner->hasSelectedStaticForHacks());
        fixSignalFlags->setEnabled(owner->hasSelectedSignalForHacks());
    }

    void popupPinChanged(bool pinned) override {
        if(pinned)
            Game::clearPinnedWindowPosition("hacksHelperUseDefault");
        EditorPopupWindow::popupPinChanged(pinned);
        if(!pinned){
            Game::savePinnedWindowPosition(
                "hacksHelperUseDefault", QPoint(0, 0));
            moveToDefaultPosition();
        }
    }

    void popupPinClicked() override {
        owner->userButtonPressed();
        owner->requestMainFocus();
    }

    void moveToDefaultPosition(){
        for(QWidget *candidate : QApplication::topLevelWidgets()){
            if(candidate != this && candidate->isVisible()
                    && candidate->windowTitle() == "Control Panel"){
                const QRect control = candidate->frameGeometry();
                move(Game::visibleWindowPosition(
                    QPoint(control.right() + 1, control.bottom() - height()), size()));
                return;
            }
        }
        QWidget *mainWindow = owner->window();
        if(mainWindow != NULL){
            move(Game::visibleWindowPosition(
                QPoint(mainWindow->x() + mainWindow->width() - width(),
                       mainWindow->y() + 80), size()));
        }
    }

    PropertiesTrackObj *owner;
    QPushButton *fixJNodePosn = NULL;
    QPushButton *removeTdbVector = NULL;
    QPushButton *removeTdbTree = NULL;
    QPushButton *fixElevation = NULL;
    QPushButton *deleteInstances = NULL;
    QPushButton *fixSignalFlags = NULL;
    bool everShown = false;
};

PropertiesTrackObj::PropertiesTrackObj(){
    const int alignedLabelWidth = qRound(66.0f * qBound(0.75f, Game::uiScale, 1.25f));
    const QString detailLevelHelp = "Controls this placed track object's StaticDetailLevel. Default uses the ESD_Detail_Level from the shape's .sd file; enable Custom to write an override into the world file. The value is limited by the route's TsreMaxStaticDetailLevel.";
    const QString collisionHelp = "Controls Open Rails collision behavior for this track object.";
    setStyleSheet(GuiFunct::scoEditorPanelStyle());

    QLabel *label;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    auto addRule = [vbox]() {
        vbox->addSpacing(qRound(5.0f * qBound(0.75f, Game::uiScale, 1.25f)));
    };
    infoLabel = new QLabel("Object: Track");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(3);
    vlist->setContentsMargins(3,0,3,0);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    this->eSectionIdx.setDisabled(true);
    vlist->addRow("UID:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    vlist->addRow("Id:",&this->eSectionIdx);

    vlist->labelForField(&this->uid)->setMinimumWidth(alignedLabelWidth);
    vbox->addItem(vlist);
    addRule();
    label = new QLabel(QString(QChar(0x2022)) + " Shape");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    this->fileName.setDisabled(true);
    this->fileName.setAlignment(Qt::AlignLeft);
    QGridLayout *shapeNameGrid = new QGridLayout;
    shapeNameGrid->setContentsMargins(3,0,3,0);
    shapeNameGrid->setColumnMinimumWidth(0, alignedLabelWidth);
    shapeNameGrid->setColumnStretch(1, 1);
    shapeNameGrid->addWidget(new QLabel("Name:"), 0, 0);
    shapeNameGrid->addWidget(&this->fileName, 0, 1);
    vbox->addLayout(shapeNameGrid);
    QFrame *filenameActionsCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(filenameActionsCard);
    QGridLayout *filenameList = new QGridLayout(filenameActionsCard);
    filenameList->setSpacing(2);
    filenameList->setContentsMargins(4,3,4,3);
    QPushButton *copyF = new QPushButton("Copy Name", this);
    GuiFunct::styleEditorActionButton(copyF);
    QObject::connect(copyF, SIGNAL(released()),
                      this, SLOT(copyFileNameEnabled()));
    QPushButton *editF = new QPushButton("Edit", this);
    GuiFunct::styleEditorActionButton(editF);
    QObject::connect(editF, SIGNAL(released()),
                      this, SLOT(editFileNameEnabled()));
    filenameList->addWidget(copyF, 0, 0);
    filenameList->addWidget(editF, 0, 1);
    vbox->addWidget(filenameActionsCard);
    
    addRule();
    label = new QLabel("Position & Rotation");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(3);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("X:",&this->posX);
    vlist->addRow("Y:",&this->posY);
    vlist->addRow("Z:",&this->posZ);
    this->posX.setReadOnly(true);
    this->posY.setReadOnly(true);
    this->posZ.setReadOnly(true);
    this->quat.setReadOnly(true);
    this->quat.setAlignment(Qt::AlignCenter);
    vlist->addRow("Rotation:",&this->quat);
    vlist->labelForField(&this->posX)->setMinimumWidth(alignedLabelWidth);
    vbox->addItem(vlist);
    QFrame *positionActionsCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(positionActionsCard);
    QGridLayout *posRotList = new QGridLayout(positionActionsCard);
    posRotList->setSpacing(2);
    posRotList->setContentsMargins(4,3,4,3);

    QPushButton *copyPos = new QPushButton("Copy Pos", positionActionsCard);
    GuiFunct::styleEditorActionButton(copyPos);
    QObject::connect(copyPos, SIGNAL(released()),
                      this, SLOT(copyPEnabled()));
    QPushButton *pastePos = new QPushButton("Paste Pos", positionActionsCard);
    GuiFunct::styleEditorActionButton(pastePos);
    QObject::connect(pastePos, SIGNAL(released()),
                      this, SLOT(pastePEnabled()));
    QPushButton *copyQrot = new QPushButton("Copy Rot", positionActionsCard);
    GuiFunct::styleEditorActionButton(copyQrot);
    QObject::connect(copyQrot, SIGNAL(released()),
                      this, SLOT(copyREnabled()));
    QPushButton *pasteQrot = new QPushButton("Paste Rot", positionActionsCard);
    GuiFunct::styleEditorActionButton(pasteQrot);
    QObject::connect(pasteQrot, SIGNAL(released()),
                      this, SLOT(pasteREnabled()));
    QPushButton *copyPosRot = new QPushButton("Copy Both", positionActionsCard);
    GuiFunct::styleEditorActionButton(copyPos);
    GuiFunct::styleEditorActionButton(copyPosRot);
    QObject::connect(copyPosRot, SIGNAL(released()),
                      this, SLOT(copyPREnabled()));
    QPushButton *pastePosRot = new QPushButton("Paste Both", positionActionsCard);
    GuiFunct::styleEditorActionButton(pastePosRot);
    QObject::connect(pastePosRot, SIGNAL(released()),
                      this, SLOT(pastePREnabled()));
    QPushButton *resetQrot = new QPushButton("Reset Rot", positionActionsCard);
    GuiFunct::styleEditorActionButton(resetQrot);
    QObject::connect(resetQrot, SIGNAL(released()),
                      this, SLOT(resetRotEnabled()));
    QPushButton *qRot90 = new QPushButton("Rot Y 90°", positionActionsCard);
    GuiFunct::styleEditorActionButton(qRot90);
    QObject::connect(qRot90, SIGNAL(released()),
                      this, SLOT(rotYEnabled()));
    QPushButton *transform = new QPushButton("Transform...", positionActionsCard);
    configureTransformButton(transform);
    
    posRotList->addWidget(copyPos, 0, 0);
    posRotList->addWidget(pastePos, 0, 1);
    posRotList->addWidget(copyQrot, 1, 0);
    posRotList->addWidget(pasteQrot, 1, 1);
    posRotList->addWidget(copyPosRot, 2, 0);
    posRotList->addWidget(pastePosRot, 2, 1);
    posRotList->addWidget(resetQrot, 3, 0);
    posRotList->addWidget(qRot90, 3, 1);
    posRotList->addWidget(transform, 4, 0, 1, 2);
    vbox->addWidget(positionActionsCard);
    addRule();
    
    label = new QLabel("Detail Level");
    label->setToolTip(detailLevelHelp);
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    this->defaultDetailLevel.setDisabled(true);
    this->defaultDetailLevel.setToolTip(detailLevelHelp);
    this->defaultDetailLevel.setAlignment(Qt::AlignCenter);
    this->enableCustomDetailLevel.setText("Custom");
    this->enableCustomDetailLevel.setToolTip(detailLevelHelp);
    QCheckBox* defaultDetailLevelLabel = new QCheckBox("Default", this);
    defaultDetailLevelLabel->setToolTip(detailLevelHelp);
    defaultDetailLevelLabel->setDisabled(true);
    defaultDetailLevelLabel->setChecked(true);
    QObject::connect(&enableCustomDetailLevel, SIGNAL(stateChanged(int)),
                      this, SLOT(enableCustomDetailLevelEnabled(int)));
    this->customDetailLevel.setDisabled(true);
    this->customDetailLevel.setToolTip(detailLevelHelp);
    this->customDetailLevel.setAlignment(Qt::AlignCenter);
    QObject::connect(&customDetailLevel, SIGNAL(textEdited(QString)),
                      this, SLOT(customDetailLevelEdited(QString)));
    QFrame *detailCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(detailCard);
    QGridLayout *detailLevelView = new QGridLayout(detailCard);
    detailLevelView->setSpacing(2);
    detailLevelView->setContentsMargins(4,3,4,3);
    detailLevelView->setColumnMinimumWidth(0, alignedLabelWidth);
    detailLevelView->addWidget(defaultDetailLevelLabel, 0, 0);
    detailLevelView->addWidget(&defaultDetailLevel, 0, 1);
    detailLevelView->addWidget(&enableCustomDetailLevel, 1, 0);
    detailLevelView->addWidget(&customDetailLevel, 1, 1);
    vbox->addWidget(detailCard);
    addRule();
    
    label = new QLabel("Grade");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(3);
    vlist->setContentsMargins(3,0,3,0);
    QDoubleValidator* doubleValidator = new QDoubleValidator(-10000, 10000, 6, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator* doubleValidator1 = new QDoubleValidator(-1000, 1000, 6, this); 
    doubleValidator1->setNotation(QDoubleValidator::StandardNotation);
    
    //‰
    vlist->addRow("Units: ",&this->elevType);
    elevType.addItem("Permille ‰");
    elevType.addItem("Percent %");
    elevType.addItem("1 in 'X' m");
    elevType.addItem("Angle º");
    elevType.setStyleSheet("combobox-popup: 0;");
    QObject::connect(&elevType, SIGNAL(currentTextChanged(QString)),
                      this, SLOT(elevTypeEdited(QString)));
    
    elevProm.setValidator(doubleValidator1);
    QObject::connect(&elevProm, SIGNAL(textEdited(QString)), this, SLOT(elevPromEnabled(QString)));
    //oneInXm
    elev1inXm.setValidator(doubleValidator);
    QObject::connect(&elev1inXm, SIGNAL(textEdited(QString)), this, SLOT(elev1inXmEnabled(QString)));
    //º
    elevProg.setValidator(doubleValidator1);
    QObject::connect(&elevProg, SIGNAL(textEdited(QString)), this, SLOT(elevProgEnabled(QString)));
    //%
    elevProp.setValidator(doubleValidator1);
    QObject::connect(&elevProp, SIGNAL(textEdited(QString)), this, SLOT(elevPropEnabled(QString)));
    elevValueStack.addWidget(&elevProm);
    elevValueStack.addWidget(&elevProp);
    elevValueStack.addWidget(&elev1inXm);
    elevValueStack.addWidget(&elevProg);
    elevValueStack.setContentsMargins(0,0,0,0);
    vlist->addRow(&elevValueLabel, &elevValueStack);
    elevStep.setToolTip("General object movement/rotation adjustment sensitivity; this is not a grade-transition increment.");
    elevStep.setValidator(doubleValidator);
    QObject::connect(&elevStep, SIGNAL(textEdited(QString)), this, SLOT(elevStepEnabled(QString)));
    elevType.setCurrentIndex(Game::DefaultElevationBox);
    showElevBox(elevType.currentText());
    vlist->labelForField(&this->elevType)->setMinimumWidth(alignedLabelWidth);
    vbox->addItem(vlist);

    gradeHelper.setText("Grade Helper...");
    gradeHelper.setCheckable(true);
    gradeHelper.setProperty("editorPopupKey", "gradeHelper");
    GuiFunct::styleEditorActionButton(&gradeHelper);
    gradeHelper.setFocusPolicy(Qt::NoFocus);
    gradeLock.setText("Lock Grade");
    gradeLock.setCheckable(true);
    GuiFunct::styleEditorActionButton(&gradeLock);
    gradeLock.setChecked(Game::gradeLockEnabled);
    gradeLock.setFocusPolicy(Qt::NoFocus);

    QFrame *gradeActionsCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(gradeActionsCard);
    QVBoxLayout *gradeActions = new QVBoxLayout(gradeActionsCard);
    gradeActions->setContentsMargins(4,3,4,3);
    gradeActions->setSpacing(2);
    gradeActions->addWidget(&gradeHelper);
    gradeActions->addWidget(&gradeLock);
    vbox->addWidget(gradeActionsCard);

    QObject::connect(&gradeLock, &QPushButton::toggled, [this](bool checked){
        if(checked && trackObj == NULL){
            gradeLock.setChecked(false);
            return;
        }
        Game::gradeLockEnabled = checked;
        if(checked){
            Game::gradeAssistEnabled = false;
            Game::gradeAssistTargetReached = false;
            Game::gradeLockedPercent = currentGradePercent();
        } else if(Game::gradeAssistTargetReached){
            Game::gradeAssistTargetReached = false;
        }
        refreshGradeLockUi();
        refreshGradeHelperUi();
    });
    QObject::connect(&gradeHelper, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&gradeHelper, checked);
        if(checked){
            openGradeHelper();
        } else if(gradeHelperWindow != NULL && gradeHelperWindow->isVisible()){
            resetGradeHelper();
            requestMainFocus();
        }
    });
    QObject::connect(&gradeHelperUiTimer, &QTimer::timeout, this, [this](){
        refreshGradeLockUi();
        refreshGradeHelperUi();
    });
    gradeHelperUiTimer.start(120);
    refreshGradeLockUi();
    refreshGradeHelperUi();
    addRule();
    
    
    label = new QLabel("Collision");
    label->setToolTip(collisionHelp);
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);

    eCollisionFlags.setToolTip(collisionHelp);
    eCollisionFlags.setDisabled(true);
    eCollisionFlags.setAlignment(Qt::AlignCenter);
    cCollisionType.addItem("Disabled");
    cCollisionType.addItem("Immovable");
    cCollisionType.addItem("Buffer");
    cCollisionType.setStyleSheet("combobox-popup: 0;");
    cCollisionType.setToolTip(collisionHelp);
    QGridLayout *collisionGrid = new QGridLayout;
    collisionGrid->setContentsMargins(3,0,3,0);
    collisionGrid->setColumnMinimumWidth(0, alignedLabelWidth);
    collisionGrid->setColumnStretch(1, 1);
    collisionGrid->addWidget(new QLabel("Type:"), 0, 0);
    collisionGrid->addWidget(&cCollisionType, 0, 1);
    vbox->addLayout(collisionGrid);
    QObject::connect(&cCollisionType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(cCollisionTypeEdited(int)));
    //QPushButton *resetFlags = new QPushButton("Reset Flags", this);
    //QObject::connect(resetFlags, SIGNAL(released()),
    //                  this, SLOT(copyFEnabled()));
    //vbox->addWidget(resetFlags);
    addRule();
    label = new QLabel(QString(QChar(0x2022)) + " Advanced");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    
    QFrame *advancedCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(advancedCard);
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedCard);
    advancedLayout->setContentsMargins(4,3,4,3);
    hacks.setText("Hacks...");
    hacks.setCheckable(true);
    hacks.setProperty("editorPopupKey", "hacksHelper");
    GuiFunct::styleEditorActionButton(&hacks);
    hacks.setFocusPolicy(Qt::NoFocus);
    QObject::connect(&hacks, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&hacks, checked);
        toggleHacksForSelection(trackObj, &hacks, checked);
    });
    advancedLayout->addWidget(&hacks);
    vbox->addWidget(advancedCard);
    
    vbox->addStretch(1);
    this->setLayout(vbox);

    QFont fieldFont = font();
    fieldFont.setBold(false);
    const int fieldHeight = gradeHelperScaledSize(22);
    foreach(QLineEdit *field, findChildren<QLineEdit*>()){
        field->setFont(fieldFont);
        field->setMinimumHeight(fieldHeight);
    }
    foreach(QSpinBox *field, findChildren<QSpinBox*>()){
        field->setFont(fieldFont);
        field->setMinimumHeight(fieldHeight);
    }
    foreach(QDoubleSpinBox *field, findChildren<QDoubleSpinBox*>()){
        field->setFont(fieldFont);
        field->setMinimumHeight(fieldHeight);
    }
    foreach(QComboBox *field, findChildren<QComboBox*>()){
        field->setFont(fieldFont);
        field->setMinimumHeight(fieldHeight);
    }
    
    GuiFunct::styleEditorActionButton(copyF);
    QObject::connect(copyF, SIGNAL(released()),
                      this, SLOT(copyFileNameEnabled()));
}

void PropertiesTrackObj::elevTypeEdited(QString val){
    showElevBox(val);
    ElevTypeName = val;
}

void PropertiesTrackObj::showElevBox(QString val){
    Q_UNUSED(val);
    const int units = elevType.currentIndex();
    elevValueStack.setCurrentIndex(units);
    if(units == 0)
        elevValueLabel.setText("‰");
    else if(units == 1)
        elevValueLabel.setText("%");
    else if(units == 2)
        elevValueLabel.setText("m");
    else
        elevValueLabel.setText("°");
    setStepValue(Game::DefaultMoveStep);
}

PropertiesTrackObj::~PropertiesTrackObj() {
    delete gradeHelperWindow;
    gradeHelperWindow = NULL;
    delete hacksWindow;
    hacksWindow = NULL;
}

QPushButton *PropertiesTrackObj::hacksButton(){
    return &hacks;
}

void PropertiesTrackObj::fixJNodePosnEnabled(){
    if(!hasSelectedTrackForHacks()){
        return;
    }
    TrackObj *selectedTrack = static_cast<TrackObj*>(hacksSelection);
    Undo::SinglePushWorldObjData(selectedTrack);
    selectedTrack->fillJNodePosn();
    Undo::StateEnd();
}

void PropertiesTrackObj::toggleHacksForSelection(
        GameObj *obj, QPushButton *button, bool checked){
    if(!checked){
        if(activeHacksButton == button){
            activeHacksButton = NULL;
            if(hacksWindow != NULL && hacksWindow->isVisible()){
                hacksWindow->hide();
                requestMainFocus();
            }
        }
        return;
    }

    activeHacksButton = button;
    setHacksSelection(obj);
    if(hacksWindow == NULL)
        hacksWindow = new HacksWindow(this);
    hacksWindow->showForOwner();
}

void PropertiesTrackObj::setHacksSelection(GameObj *obj){
    hacksSelection =
        obj != NULL && obj->typeObj == GameObj::worldobj
            ? static_cast<WorldObj*>(obj)
            : NULL;
    if(hacksWindow != NULL)
        hacksWindow->refreshSelection();
}

void PropertiesTrackObj::adoptHacksButton(QPushButton *button){
    if(hacksWindow == NULL || !hacksWindow->isVisible())
        return;
    if(activeHacksButton == button)
        return;

    if(activeHacksButton != NULL){
        GuiFunct::setEditorPopupButtonActive(activeHacksButton, false);
        activeHacksButton->blockSignals(true);
        activeHacksButton->setChecked(false);
        activeHacksButton->blockSignals(false);
    }
    activeHacksButton = button;

    if(activeHacksButton == NULL){
        hacksWindow->hide();
        return;
    }

    activeHacksButton->blockSignals(true);
    activeHacksButton->setChecked(true);
    activeHacksButton->blockSignals(false);
    GuiFunct::setEditorPopupButtonActive(activeHacksButton, true);
}

bool PropertiesTrackObj::hasSelectedTrackForHacks() const{
    return hacksSelection != NULL
        && hacksSelection->loaded
        && hacksSelection->type == "trackobj";
}

bool PropertiesTrackObj::hasSelectedStaticForHacks() const{
    return hacksSelection != NULL
        && hacksSelection->loaded
        && !hacksSelection->fileName.trimmed().isEmpty()
        && (hacksSelection->typeID == WorldObj::sstatic
            || hacksSelection->typeID == WorldObj::gantry
            || hacksSelection->typeID == WorldObj::collideobject);
}

bool PropertiesTrackObj::hasSelectedSignalForHacks() const{
    return hacksSelection != NULL
        && hacksSelection->loaded
        && hacksSelection->type == "signal";
}

void PropertiesTrackObj::fixSignalFlagsEnabled(){
    if(!hasSelectedSignalForHacks())
        return;

    SignalObj *selectedSignal = static_cast<SignalObj*>(hacksSelection);
    QStringList changes;
    selectedSignal->checkFlags(changes);

    TextEditDialog dialog;
    QString text = "___old_____new___\n";
    for(const QString &change : changes)
        text += change + "\n";
    dialog.textBox.setPlainText(text);
    dialog.setWindowTitle("Signal Flags");
    dialog.exec();
    if(dialog.changed == 1)
        selectedSignal->fixFlags();
}

void PropertiesTrackObj::hacksWindowClosed(){
    if(activeHacksButton != NULL){
        GuiFunct::setEditorPopupButtonActive(activeHacksButton, false);
        activeHacksButton->blockSignals(true);
        activeHacksButton->setChecked(false);
        activeHacksButton->blockSignals(false);
        activeHacksButton = NULL;
    }
    requestMainFocus();
}

void PropertiesTrackObj::removeAllInteractivesEnabled(){
    if(Game::currentRoute == NULL)
        return;

    const QString warning =
        "This will load every world tile and remove ALL TrackDB/RoadDB-linked "
        "interactive objects from the route:\n\n"
        "signals, speedposts and mileposts, platform and siding markers, pickups, "
        "level crossings, car spawners, hazards, and linked sound regions.\n\n"
        "Track and road geometry will not be removed.\n\n"
        "Make a full route backup first. Continue?";
    if(!GuiFunct::confirmDestructiveAction(
            hacksWindow, "Remove All Interactives", warning))
        return;

    const int removed = Game::currentRoute->removeAllInteractives(true);
    if(removed == 0){
        QMessageBox::information(
            hacksWindow, "Remove All Interactives",
            "No TrackDB/RoadDB-linked interactive objects were found.");
    } else {
        QMessageBox::information(
            hacksWindow, "Remove All Interactives",
            QString::number(removed)
                + " interactive object(s) removed.\n\n"
                  "Recommended: save the route immediately, close TSRE, and restart "
                  "before doing any further route work.\n\n"
                  "If this removal was accidental, use Undo before saving or closing TSRE.");
    }
}

void PropertiesTrackObj::deleteSelectedInstancesEnabled(){
    if(Game::currentRoute == NULL || !hasSelectedStaticForHacks())
        return;

    const QString selectedFile = hacksSelection->fileName;
    const QString warning =
        "This will load every world tile and delete every instance of the selected "
        "static object from the full route:\n\n"
        + selectedFile
        + "\n\nOnly objects with the same type and filename will be deleted.\n\n"
          "Make a full route backup first. Continue?";
    if(!GuiFunct::confirmDestructiveAction(
            hacksWindow, "Delete Instances", warning))
        return;

    const int removed =
        Game::currentRoute->deleteAllInstances(hacksSelection, true);
    hacksSelection = NULL;
    if(hacksWindow != NULL)
        hacksWindow->refreshSelection();

    if(removed == 0){
        QMessageBox::information(
            hacksWindow, "Delete Instances",
            "No matching static-object instances were found.");
    } else {
        QMessageBox::information(
            hacksWindow, "Delete Instances",
            QString::number(removed)
                + " matching object instance(s) deleted from the route.\n\n"
                  "If this removal was accidental, use Undo before saving or closing TSRE.");
    }
}

void PropertiesTrackObj::haxElevTDBVectorEnabled(){
    if(!hasSelectedTrackForHacks())
        return;
    if(Game::currentRoute == NULL)
        return;
    Game::currentRoute->fixTDBVectorElevation(hacksSelection);
}

void PropertiesTrackObj::haxRemoveTDBVectorEnabled(){
    if(!hasSelectedTrackForHacks())
        return;
    if(Game::currentRoute == NULL)
        return;
    Game::currentRoute->deleteTDBVector(hacksSelection);
}

void PropertiesTrackObj::haxRemoveTDBTreeEnabled(){
    if(!hasSelectedTrackForHacks())
        return;
    if(Game::currentRoute == NULL)
        return;
    
    Game::currentRoute->deleteTDBTree(hacksSelection);
}

void PropertiesTrackObj::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    trackObj = (TrackObj*) obj;
    
    this->infoLabel->setText("Object: Track");
    this->fileName.setText(trackObj->fileName);
    
    this->uid.setText(QString::number(trackObj->UiD, 10));
    this->tX.setText(QString::number(trackObj->x, 10));
    this->tY.setText(QString::number(-trackObj->y, 10));
    this->eSectionIdx.setText(QString::number(trackObj->sectionIdx, 10));
    this->posX.setText(QString::number(trackObj->position[0], 'G', 6));
    this->posY.setText(QString::number(trackObj->position[1], 'G', 6));
    this->posZ.setText(QString::number(-trackObj->position[2], 'G', 6));
    this->quat.setText(
            QString::number(trackObj->qDirection[0], 'G', 4) + " " +
            QString::number(trackObj->qDirection[1], 'G', 4) + " " +
            QString::number(-trackObj->qDirection[2], 'G', 4) + " " +
            QString::number(trackObj->qDirection[3], 'G', 4)
            );
    
    defaultDetailLevel.setText(QString::number(trackObj->getDefaultDetailLevel()));
    enableCustomDetailLevel.blockSignals(true);
    if(trackObj->customDetailLevelEnabled()){
        enableCustomDetailLevel.setChecked(true);
        customDetailLevel.setText(QString::number(trackObj->getCustomDetailLevel()));
        customDetailLevel.setEnabled(true);
    } else {
        enableCustomDetailLevel.setChecked(false);
        customDetailLevel.setText("");
        customDetailLevel.setEnabled(false);
    }
    enableCustomDetailLevel.blockSignals(false);
    
    this->flags.setText(ParserX::MakeFlagsString(trackObj->staticFlags));
    
    ///////////
    elevType.setCurrentText(ElevTypeName);
    
    TrackObj* track = (TrackObj*)obj;
    float prom = qTan(track->getElevation()) * 1000.0;
     
    float oneInXm = 0.0;
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    float prop = prom/10.0;

    //if(vect[1] > 0)
        oneInXm = qFuzzyIsNull(prom) ? 0.0 : 1000.0/prom;
    this->elevProm.setText(QString::number(prom));
    this->elevProg.setText(QString::number(prog));
    this->elevProp.setText(QString::number(prop));
    this->elev1inXm.setText(QString::number(oneInXm));
    setStepValue(Game::DefaultMoveStep);
    refreshGradeLockUi();
    refreshGradeHelperUi();
    
    /*float pitch = asin(2*(q[0]*q[2] - q[1]*q[3]));
    
    if(vect[2] < 0)
        pitch = M_PI - pitch;
    if(vect[2] == 0 && vect[0] < 0)
        pitch = M_PI/2;
    if(vect[2] == 0 && vect[0] > 0)
        pitch = -M_PI/2;*/

    //float elev = tan((vect[1]/10.0));
    //qe[1] = pitch;
    //qe[2] = 0;
    eCollisionFlags.setText(QString::number(worldObj->getCollisionFlags()));
    int collisionType = worldObj->getCollisionType();
    this->cCollisionType.blockSignals(true);
    this->cCollisionType.setCurrentIndex(collisionType);
    this->cCollisionType.blockSignals(false);
    
}

void PropertiesTrackObj::setStepValue(float step){
    if(elevType.currentIndex() == 0)
        step = step * 100;
    if(elevType.currentIndex() == 1)
        step = step * 10;
    if(elevType.currentIndex() == 2)
        step = 10.0/step;
    if(elevType.currentIndex() == 3)
        step = qRadiansToDegrees(qAtan(step/10.0));
    
    elevStep.setText(QString::number(step));
}

float PropertiesTrackObj::getStepValue(float step){
    if(elevType.currentIndex() == 0)
        return step / 100;
    if(elevType.currentIndex() == 1)
        return step / 10;
    if(elevType.currentIndex() == 2)
        return 10.0/step;
    if(elevType.currentIndex() == 3)
        return qTan(qDegreesToRadians(step))*10.0;
    return 0;
}

void PropertiesTrackObj::updateObj(GameObj* obj){
    if(obj == NULL){
        return;
    }
    TrackObj* track = (TrackObj*)obj;
    float prom = qTan(track->getElevation()) * 1000.0;
     
    float oneInXm = 0.0;
    oneInXm = qFuzzyIsNull(prom) ? 0.0 : 1000.0/prom;
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    float prop = prom/10.0;
       
    if(!posX.hasFocus() && !posY.hasFocus() && !posZ.hasFocus() && !quat.hasFocus()){
        this->uid.setText(QString::number(trackObj->UiD, 10));
        this->tX.setText(QString::number(trackObj->x, 10));
        this->tY.setText(QString::number(-trackObj->y, 10));
        this->posX.setText(QString::number(trackObj->position[0], 'G', 6));
        this->posY.setText(QString::number(trackObj->position[1], 'G', 6));
        this->posZ.setText(QString::number(-trackObj->position[2], 'G', 6));
        this->quat.setText(
                QString::number(trackObj->qDirection[0], 'G', 4) + " " +
                QString::number(trackObj->qDirection[1], 'G', 4) + " " +
                QString::number(-trackObj->qDirection[2], 'G', 4) + " " +
                QString::number(trackObj->qDirection[3], 'G', 4)
                );
    }
    if(!this->elevProm.hasFocus() && !this->elev1inXm.hasFocus() && !this->elevProg.hasFocus() && !this->elevProp.hasFocus()){
        this->elevProm.setText(QString::number(prom));
        this->elevProg.setText(QString::number(prog));
        this->elevProp.setText(QString::number(prop));
        this->elev1inXm.setText(QString::number(oneInXm));
     }
    refreshGradeLockUi();
    refreshGradeHelperUi();
    eCollisionFlags.setText(QString::number(worldObj->getCollisionFlags()));
}

float PropertiesTrackObj::currentGradePercent() const{
    if(trackObj == NULL)
        return 0.0f;
    return qTan(trackObj->getElevation()) * 100.0f;
}

int PropertiesTrackObj::gradeUnitsIndex() const{
    return elevType.currentIndex();
}

void PropertiesTrackObj::activateGradeAssistPlacement(){
    emit enableTool("placeTool");
}

void PropertiesTrackObj::refreshGradeLockUi(){
    gradeLock.blockSignals(true);
    gradeLock.setChecked(Game::gradeLockEnabled);
    gradeLock.blockSignals(false);
    if(Game::gradeLockEnabled){
        gradeLock.setText(QString("Lock Grade: %1%").arg(Game::gradeLockedPercent, 0, 'f', 5));
        gradeLock.setToolTip(QString("New track and road pieces will use a physical grade of %1%.").arg(Game::gradeLockedPercent, 0, 'f', 5));
    } else {
        gradeLock.setText("Lock Grade");
        gradeLock.setToolTip("Capture the selected physical grade and apply it to newly placed track and road pieces.");
    }
}

void PropertiesTrackObj::openGradeHelper(){
    if(trackObj == NULL)
        return;
    Game::gradeLockEnabled = false;
    refreshGradeLockUi();
    if(gradeHelperWindow == NULL){
        gradeHelperWindow = new GradeHelperWindow(this);
        QObject::connect(gradeHelperWindow, &QObject::destroyed, this, [this](){
            gradeHelperWindow = NULL;
        });
    }
    gradeHelperWindow->showForGrade(currentGradePercent());
    refreshGradeHelperUi();
}

void PropertiesTrackObj::gradeHelperWindowClosed(){
    Game::gradeAssistEnabled = false;
    Game::gradeAssistTargetReached = false;
    gradeHelper.blockSignals(true);
    gradeHelper.setChecked(false);
    gradeHelper.blockSignals(false);
    refreshGradeHelperUi();
}

void PropertiesTrackObj::resetGradeHelper(){
    Game::gradeAssistEnabled = false;
    Game::gradeAssistTargetReached = false;
    if(gradeHelperWindow != NULL && gradeHelperWindow->isVisible())
        gradeHelperWindow->hide();
    gradeHelper.blockSignals(true);
    gradeHelper.setChecked(false);
    gradeHelper.blockSignals(false);
    refreshGradeHelperUi();
}

void PropertiesTrackObj::refreshGradeHelperUi(){
    gradeHelper.blockSignals(true);
    gradeHelper.setChecked(gradeHelperWindow != NULL && gradeHelperWindow->isVisible());
    gradeHelper.blockSignals(false);
    gradeHelper.setText("Grade Helper...");
    gradeHelper.setStyleSheet(QString());
}

void PropertiesTrackObj::elevPromEnabled(QString val){
    if(trackObj == NULL){
        return;
    }
    bool ok = false;
    //prom
    float prom = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prom) > Game::trackElevationMaxPm + 0.000001) {   
        this->elevProm.setText(QString::number(Game::trackElevationMaxPm));
        return;
    }
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    if(Game::debugOutput) qDebug () << "Game::trackElevationMaxPm" << Game::trackElevationMaxPm;
    this->elev1inXm.setText(QString::number(oneInXm));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(QString::number(prog));
    //prop 
    float prop = prom/10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    this->elevProp.setText(QString::number(prop));  
    
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->setElevation(prom);
}

void PropertiesTrackObj::elevStepEnabled(QString val){
    if(trackObj == NULL)
        return;

    bool ok = false;
    float f = val.toFloat(&ok);
    if(!ok)
        return;
    
    f = getStepValue(f);
    
    Game::DefaultMoveStep = f;
    emit setMoveStep(f);
}

void PropertiesTrackObj::elev1inXmEnabled(QString val){
    if(trackObj == NULL){
        return;
    }
    bool ok = false;
    //oneInXm
    float oneInXm = val.toFloat(&ok);
    if(!ok) return;
    //qDebug () << "oneInXm" << oneInXm;
    //prom
    float prom = 1000.0/oneInXm;
    if(fabs(prom) > Game::trackElevationMaxPm + 0.000001) { 
        return;
    }
    
    if(Game::debugOutput) qDebug () << "Game::trackElevationMaxPm: " << Game::trackElevationMaxPm;
    this->elevProm.setText(QString::number(prom));  
    //prop 
    float prop = prom/10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    this->elevProp.setText(QString::number(prop));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(QString::number(prog));
    
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->setElevation(prom);
}

void PropertiesTrackObj::elevProgEnabled(QString val){
    if(trackObj == NULL){
         return;
    }
    bool ok = false;
    //prog
    float prog = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prog) > qRadiansToDegrees(qAtan(Game::trackElevationMaxPm/1000.0))+ 0.000001) {   
        this->elevProg.setText(QString::number(qRadiansToDegrees(qAtan(Game::trackElevationMaxPm/1000.0))));
        return;
    }
    //prop 
    float prop = qTan(qDegreesToRadians(prog))*100.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProp.setText(QString::number(prop));
    //prom
    float prom = prop*10.0;
    if(Game::debugOutput) qDebug () << "prom" << prom;
    this->elevProm.setText(QString::number(prom));
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    this->elev1inXm.setText(QString::number(oneInXm));
     
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->setElevation(prom);
}
 
void PropertiesTrackObj::elevPropEnabled(QString val){
    if(trackObj == NULL){
        return;
    }
    bool ok = false;
    //prop
    float prop = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prop) > (Game::trackElevationMaxPm/10.0)+ 0.000001)
    {    this->elevProp.setText(QString::number(Game::trackElevationMaxPm/10.0));
        return;}
    //prom    
    float prom = prop*10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    if(Game::debugOutput) qDebug () << "prom" << prom;
    if(Game::debugOutput) qDebug () << "Game::trackElevationMaxPm/10.0: " << Game::trackElevationMaxPm/10.0;
    this->elevProm.setText(QString::number(prom));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(QString::number(prog));
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    this->elev1inXm.setText(QString::number(oneInXm));
    
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->setElevation(prom);
}

bool PropertiesTrackObj::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "trackobj")
        return true;
    return false;
}

void PropertiesTrackObj::cCollisionTypeEdited(int val){
    if(worldObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    worldObj->setCollisionType(val-1);
}

void PropertiesTrackObj::editFileNameEnabled(){
    if(worldObj == NULL)
        return;
    EditFileNameDialog eWindow;
    eWindow.name.setText(worldObj->fileName);
    eWindow.exec();
    //qDebug() << waterWindow->changed;
    if(eWindow.isOk){
        
        //QString filename = Game::root + "/routes/" + Game::route + "/shapes/" + eWindow.name.text();
        QString filename = Game::root + "/global/shapes/" + eWindow.name.text();            
        QFile file(filename);            
        if (!file.exists()) 
        { 
            qWarning() << "Rename failed - " << eWindow.name.text() << " does not exist" ;            
            QMessageBox msgBox;
            msgBox.setWindowTitle("Shape Not Found");
            msgBox.setText("Rename Failed");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();                    
            return;
        }
        
        Undo::SinglePushWorldObjData(worldObj);
        worldObj->fileName = eWindow.name.text();
        worldObj->position[2] = -worldObj->position[2];
        worldObj->qDirection[2] = -worldObj->qDirection[2];
        worldObj->load(worldObj->x, worldObj->y);
        worldObj->modified = true;
    }
}

void PropertiesTrackObj::enableCustomDetailLevelEnabled(int val){
    if(worldObj == NULL)
        return;
    TrackObj* tObj = (TrackObj*) worldObj;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        customDetailLevel.setEnabled(true);
        customDetailLevel.setText("0");
        tObj->setCustomDetailLevel(0);
    } else {
        customDetailLevel.setEnabled(false);
        customDetailLevel.setText("");
        tObj->setCustomDetailLevel(-1);
    }
}

void PropertiesTrackObj::customDetailLevelEdited(QString val){
    if(worldObj == NULL)
        return;
    TrackObj* tObj = (TrackObj*) worldObj;
    bool ok = false;
    int level = val.toInt(&ok);
    //qDebug() << "aaaaaaaaaa";
    if(ok){
        Undo::SinglePushWorldObjData(worldObj);
        tObj->setCustomDetailLevel(level);
    }
}

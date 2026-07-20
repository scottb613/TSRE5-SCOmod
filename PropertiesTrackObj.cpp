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
#include <math.h>
#include "GLMatrix.h"
#include "ParserX.h"
#include "EditFileNameDialog.h"
#include "Undo.h"
#include "Game.h"
#include "Route.h"
#include "ProceduralShape.h"
#include "ShapeTemplates.h"
#include "GuiFunct.h"
#include <QScreen>

static int gradeHelperScaledSize(int base){
    return qRound(base * qMax(1.0f, Game::uiScale));
}

static QString gradeHelperStateStyle(const QString &background, const QString &border, const QString &text){
    return QString(
        "QPushButton { color: %3; background-color: %1; border: 1px solid %2;"
        " border-radius: 2px; padding: 3px 5px; }"
        "QPushButton:hover { border-color: #f08200; }"
        "QPushButton:pressed { background-color: #202020; }"
    ).arg(background, border, text);
}

static QPoint gradeHelperSnapPosition(QWidget *window){
    const int snapDistance = 10;
    QRect moving = window->frameGeometry();
    QPoint snappedFramePos = moving.topLeft();
    int bestX = snapDistance + 1;
    int bestY = snapDistance + 1;

    QList<QWidget*> targets = QApplication::topLevelWidgets();
    QScreen *screen = QGuiApplication::screenAt(moving.center());
    if(screen != NULL){
        QRect available = screen->availableGeometry();
        int xCandidates[2] = { available.left() - moving.left(), available.right() - moving.right() };
        int yCandidates[2] = { available.top() - moving.top(), available.bottom() - moving.bottom() };
        for(int i = 0; i < 2; ++i){
            if(qAbs(xCandidates[i]) <= snapDistance && qAbs(xCandidates[i]) < bestX){
                bestX = qAbs(xCandidates[i]);
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            if(qAbs(yCandidates[i]) <= snapDistance && qAbs(yCandidates[i]) < bestY){
                bestY = qAbs(yCandidates[i]);
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }

    for(QWidget *targetWidget : targets){
        if(targetWidget == window || !targetWidget->isVisible())
            continue;
        QRect target = targetWidget->frameGeometry();
        bool verticalNear = moving.bottom() >= target.top() - snapDistance && moving.top() <= target.bottom() + snapDistance;
        bool horizontalNear = moving.right() >= target.left() - snapDistance && moving.left() <= target.right() + snapDistance;
        int xCandidates[4] = { target.left() - moving.left(), target.right() - moving.right(),
                               target.right() + 1 - moving.left(), target.left() - 1 - moving.right() };
        int yCandidates[4] = { target.top() - moving.top(), target.bottom() - moving.bottom(),
                               target.bottom() + 1 - moving.top(), target.top() - 1 - moving.bottom() };
        for(int i = 0; i < 4; ++i){
            int distance = qAbs(xCandidates[i]);
            if(verticalNear && distance <= snapDistance && distance < bestX){
                bestX = distance;
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            distance = qAbs(yCandidates[i]);
            if(horizontalNear && distance <= snapDistance && distance < bestY){
                bestY = distance;
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }
    return window->pos() + (snappedFramePos - moving.topLeft());
}

class GradeHelperWindow : public QWidget {
public:
    explicit GradeHelperWindow(PropertiesTrackObj *owner)
        : QWidget(owner->window(), Qt::Tool), owner(owner) {
        setWindowTitle("Grade Helper");
        setFixedWidth(gradeHelperScaledSize(300));
        setStyleSheet(GuiFunct::scoPanelStyle());

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(3);
        layout->setContentsMargins(4,4,4,4);
        QLabel *heading = new QLabel("Grade Transition:");
        heading->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel + "; font-weight: bold; }");
        layout->addWidget(heading);

        QFormLayout *form = new QFormLayout;
        form->setSpacing(3);
        form->setContentsMargins(0,0,0,0);
        currentGrade.setReadOnly(true);
        nextGrade.setReadOnly(true);
        currentGrade.setAlignment(Qt::AlignCenter);
        nextGrade.setAlignment(Qt::AlignCenter);
        const double maximumGrade = Game::trackElevationMaxPm / 10.0;
        targetGrade.setRange(-maximumGrade, maximumGrade);
        targetGrade.setDecimals(5);
        targetGrade.setSingleStep(0.05);
        targetGrade.setSuffix(" %");
        stepGrade.setRange(0.001, maximumGrade);
        stepGrade.setDecimals(5);
        stepGrade.setSingleStep(0.05);
        stepGrade.setSuffix(" %");
        form->addRow("Current Grade:", &currentGrade);
        form->addRow("Target Grade:", &targetGrade);
        form->addRow("Step Per Piece:", &stepGrade);
        form->addRow("Next Grade:", &nextGrade);
        layout->addLayout(form);

        QWidget *ruleRow = new QWidget;
        ruleRow->setFixedHeight(7);
        QHBoxLayout *ruleLayout = new QHBoxLayout(ruleRow);
        ruleLayout->setContentsMargins(3,3,3,3);
        QFrame *rule = new QFrame;
        rule->setFixedHeight(1);
        rule->setStyleSheet("background-color: #484848; border: none;");
        ruleLayout->addWidget(rule);
        layout->addWidget(ruleRow);

        stateButton.setFocusPolicy(Qt::NoFocus);
        layout->addWidget(&stateButton);

        snapTimer.setSingleShot(true);
        QObject::connect(&snapTimer, &QTimer::timeout, this, [this](){
            if(snapping)
                return;
            QPoint snapped = gradeHelperSnapPosition(this);
            if(snapped != pos()){
                snapping = true;
                move(snapped);
                snapping = false;
            }
        });
        QObject::connect(&refreshTimer, &QTimer::timeout, this, [this](){ syncUi(); });
        refreshTimer.start(120);

        QObject::connect(&targetGrade, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [](double value){
            if(!Game::gradeAssistEnabled && !Game::gradeAssistTargetReached)
                Game::gradeAssistTargetPercent = (float)value;
        });
        QObject::connect(&stepGrade, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [](double value){
            if(!Game::gradeAssistEnabled && !Game::gradeAssistTargetReached)
                Game::gradeAssistStepPercent = (float)value;
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
                Game::gradeAssistTargetPercent = (float)targetGrade.value();
                Game::gradeAssistStepPercent = (float)stepGrade.value();
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
        layout->activate();
        setFixedHeight(layout->sizeHint().height());
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
        if(!everShown){
            for(QWidget *candidate : QApplication::topLevelWidgets()){
                if(candidate != this && candidate->isVisible() && candidate->windowTitle() == "Control Panel"){
                    QRect control = candidate->frameGeometry();
                    setFixedWidth(candidate->width());
                    move(control.right() + 1, control.top());
                    break;
                }
            }
            everShown = true;
        }
        show();
        raise();
        activateWindow();
        targetGrade.setFocus();
        targetGrade.selectAll();
    }

protected:
    void moveEvent(QMoveEvent *event) override {
        QWidget::moveEvent(event);
        if(!snapping)
            snapTimer.start(120);
    }

    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        owner->gradeHelperWindowClosed();
        owner->requestMainFocus();
    }

private:
    void prepareNextGrade(){
        const float difference = Game::gradeAssistTargetPercent - Game::gradeAssistCurrentPercent;
        const float amount = qMin(qAbs(difference), Game::gradeAssistStepPercent);
        Game::gradeAssistNextPercent = Game::gradeAssistCurrentPercent + (difference >= 0.0f ? amount : -amount);
    }

    void syncUi(){
        currentGrade.setText(QString::number(Game::gradeAssistCurrentPercent, 'f', 5) + " %");
        nextGrade.setText(QString::number(Game::gradeAssistNextPercent, 'f', 5) + " %");
        if(!targetGrade.hasFocus()){
            targetGrade.blockSignals(true);
            targetGrade.setValue(Game::gradeAssistTargetPercent);
            targetGrade.blockSignals(false);
        }
        if(!stepGrade.hasFocus()){
            stepGrade.blockSignals(true);
            stepGrade.setValue(Game::gradeAssistStepPercent);
            stepGrade.blockSignals(false);
        }
        const bool editingEnabled = !Game::gradeAssistEnabled && !Game::gradeAssistTargetReached;
        targetGrade.setEnabled(editingEnabled);
        stepGrade.setEnabled(editingEnabled);
        if(Game::gradeAssistEnabled){
            stateButton.setText("Grade Assist Active - Click to Stop");
            stateButton.setStyleSheet(gradeHelperStateStyle("#a88718", "#d0ad32", "#fff9d8"));
        } else if(Game::gradeAssistTargetReached){
            stateButton.setText("Grade Achieved - Holding Target");
            stateButton.setStyleSheet(gradeHelperStateStyle("#176c25", "#319344", "#f2fff4"));
        } else {
            stateButton.setText("Start Grade Assist");
            stateButton.setStyleSheet(QString());
        }
    }

    PropertiesTrackObj *owner;
    QLineEdit currentGrade;
    QDoubleSpinBox targetGrade;
    QDoubleSpinBox stepGrade;
    QLineEdit nextGrade;
    QPushButton stateButton;
    QTimer refreshTimer;
    QTimer snapTimer;
    bool snapping = false;
    bool everShown = false;
};

PropertiesTrackObj::PropertiesTrackObj(){
    QLabel *label;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    auto addRule = [vbox]() {
        QWidget *ruleRow = new QWidget;
        ruleRow->setFixedHeight(7);
        QHBoxLayout *ruleLayout = new QHBoxLayout(ruleRow);
        ruleLayout->setContentsMargins(6,3,6,3);
        QFrame *rule = new QFrame;
        rule->setFixedHeight(1);
        rule->setStyleSheet("background-color: #484848; border: none;");
        ruleLayout->addWidget(rule);
        vbox->addWidget(ruleRow);
    };
    infoLabel = new QLabel("Object: Track");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    this->eSectionIdx.setDisabled(true);
    vlist->addRow("UiD:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    vlist->addRow("Id:",&this->eSectionIdx);
    vlist->addRow("Name:",&this->fileName);
    vbox->addItem(vlist);
    addRule();
    this->fileName.setDisabled(true);
    this->fileName.setAlignment(Qt::AlignCenter);
    QGridLayout *filenameList = new QGridLayout;
    filenameList->setSpacing(2);
    filenameList->setContentsMargins(0,0,0,0);    
    QPushButton *copyF = new QPushButton("Copy Name", this);
    QObject::connect(copyF, SIGNAL(released()),
                      this, SLOT(copyFileNameEnabled()));
    QPushButton *editF = new QPushButton("Edit", this);
    QObject::connect(editF, SIGNAL(released()),
                      this, SLOT(editFileNameEnabled()));
    filenameList->addWidget(copyF, 0, 0);
    filenameList->addWidget(editF, 0, 1);
    vbox->addItem(filenameList);
    
    label = new QLabel("Shape Template:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vbox->addWidget(&eTemplate);
    eTemplate.setStyleSheet("combobox-popup: 0;");
    eTemplate.addItem("DEFAULT");
    eTemplate.addItem("DISABLED");
    
    ProceduralShape::Load();
    if(ProceduralShape::ShapeTemplateFile != NULL){
        QMapIterator<QString, ShapeTemplate*> i(ProceduralShape::ShapeTemplateFile->templates);
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            eTemplate.addItem(i.value()->name);
        }
    }
    QObject::connect(&eTemplate, SIGNAL(currentTextChanged(QString)),
                      this, SLOT(eTemplateEdited(QString)));

    addRule();
    label = new QLabel("Position & Rotation:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("X:",&this->posX);
    vlist->addRow("Y:",&this->posY);
    vlist->addRow("Z:",&this->posZ);
    this->quat.setDisabled(true);
    this->quat.setAlignment(Qt::AlignCenter);
    vlist->addRow("Rot:",&this->quat);
    vbox->addItem(vlist);
    QGridLayout *posRotList = new QGridLayout;
    posRotList->setSpacing(2);
    posRotList->setContentsMargins(0,0,0,0);    

    QPushButton *copyPos = new QPushButton("Copy Pos", this);
    QObject::connect(copyPos, SIGNAL(released()),
                      this, SLOT(copyPEnabled()));
    QPushButton *pastePos = new QPushButton("Paste", this);
    QObject::connect(pastePos, SIGNAL(released()),
                      this, SLOT(pastePEnabled()));
    QPushButton *copyQrot = new QPushButton("Copy Rot", this);
    QObject::connect(copyQrot, SIGNAL(released()),
                      this, SLOT(copyREnabled()));
    QPushButton *pasteQrot = new QPushButton("Paste", this);
    QObject::connect(pasteQrot, SIGNAL(released()),
                      this, SLOT(pasteREnabled()));
    QPushButton *copyPosRot = new QPushButton("Copy Pos+Rot", this);
    QObject::connect(copyPosRot, SIGNAL(released()),
                      this, SLOT(copyPREnabled()));
    QPushButton *pastePosRot = new QPushButton("Paste", this);
    QObject::connect(pastePosRot, SIGNAL(released()),
                      this, SLOT(pastePREnabled()));
    QPushButton *resetQrot = new QPushButton("Reset Rot", this);
    QObject::connect(resetQrot, SIGNAL(released()),
                      this, SLOT(resetRotEnabled()));
    QPushButton *qRot90 = new QPushButton("Rot Y 90°", this);
    QObject::connect(qRot90, SIGNAL(released()),
                      this, SLOT(rotYEnabled()));
    QPushButton *transform = new QPushButton("Transform ...", this);
    QObject::connect(transform, SIGNAL(released()),
                      this, SLOT(transformEnabled()));
    
    posRotList->addWidget(copyPos, 0, 0);
    posRotList->addWidget(pastePos, 0, 1);
    posRotList->addWidget(copyQrot, 1, 0);
    posRotList->addWidget(pasteQrot, 1, 1);
    posRotList->addWidget(copyPosRot, 2, 0);
    posRotList->addWidget(pastePosRot, 2, 1);
    posRotList->addWidget(resetQrot, 3, 0);
    posRotList->addWidget(qRot90, 3, 1);
    posRotList->addWidget(transform, 4, 0, 1, 2);
    vbox->addItem(posRotList);
    addRule();
    
    label = new QLabel("Detail Level:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    this->defaultDetailLevel.setDisabled(true);
    this->defaultDetailLevel.setAlignment(Qt::AlignCenter);
    this->enableCustomDetailLevel.setText("Custom");
    QCheckBox* defaultDetailLevelLabel = new QCheckBox("Default", this);
    defaultDetailLevelLabel->setDisabled(true);
    defaultDetailLevelLabel->setChecked(true);
    QObject::connect(&enableCustomDetailLevel, SIGNAL(stateChanged(int)),
                      this, SLOT(enableCustomDetailLevelEnabled(int)));
    this->customDetailLevel.setDisabled(true);
    this->customDetailLevel.setAlignment(Qt::AlignCenter);
    QObject::connect(&customDetailLevel, SIGNAL(textEdited(QString)),
                      this, SLOT(customDetailLevelEdited(QString)));
    QGridLayout *detailLevelView = new QGridLayout;
    detailLevelView->setSpacing(2);
    detailLevelView->setContentsMargins(0,0,0,0);    
    detailLevelView->addWidget(defaultDetailLevelLabel, 0, 0);
    detailLevelView->addWidget(&defaultDetailLevel, 0, 1);
    detailLevelView->addWidget(&enableCustomDetailLevel, 1, 0);
    detailLevelView->addWidget(&customDetailLevel, 1, 1);
    vbox->addItem(detailLevelView);
    addRule();
    
    label = new QLabel("Flags:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    this->flags.setDisabled(true);
    this->flags.setAlignment(Qt::AlignCenter);
    vbox->addWidget(&this->flags);
    QGridLayout *flagslView = new QGridLayout;
    flagslView->setSpacing(2);
    flagslView->setContentsMargins(0,0,0,0);    
    QPushButton *copyFlags = new QPushButton("Copy Flags", this);
    QObject::connect(copyFlags, SIGNAL(released()),
                      this, SLOT(copyFEnabled()));
    QPushButton *pasteFlags = new QPushButton("Paste", this);
    QObject::connect(pasteFlags, SIGNAL(released()),
                      this, SLOT(pasteFEnabled()));
    flagslView->addWidget(copyFlags,0,0);
    flagslView->addWidget(pasteFlags,0,1);
    vbox->addItem(flagslView);
    addRule();
    
    label = new QLabel("Grade:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(0);
    vlist->setContentsMargins(0,0,0,0);
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
    
    elevPromLabel.setText("‰");
    vlist->addRow(&elevPromLabel,&elevProm);
    elevProm.setValidator(doubleValidator1);
    QObject::connect(&elevProm, SIGNAL(textEdited(QString)), this, SLOT(elevPromEnabled(QString)));
    //oneInXm
    elev1inXmLabel.setText("1 in 'x' m");
    vlist->addRow(&elev1inXmLabel,&elev1inXm);
    elev1inXm.setValidator(doubleValidator);
    QObject::connect(&elev1inXm, SIGNAL(textEdited(QString)), this, SLOT(elev1inXmEnabled(QString)));
    //º
    elevProgLabel.setText("º");
    vlist->addRow(&elevProgLabel,&elevProg);
    elevProg.setValidator(doubleValidator1);
    QObject::connect(&elevProg, SIGNAL(textEdited(QString)), this, SLOT(elevProgEnabled(QString)));
    //%
    elevPropLabel.setText("%");
    vlist->addRow(&elevPropLabel,&elevProp);
    elevProp.setValidator(doubleValidator1);
    QObject::connect(&elevProp, SIGNAL(textEdited(QString)), this, SLOT(elevPropEnabled(QString)));
    elevStep.setToolTip("General object movement/rotation adjustment sensitivity; this is not a grade-transition increment.");
    elevStep.setValidator(doubleValidator);
    QObject::connect(&elevStep, SIGNAL(textEdited(QString)), this, SLOT(elevStepEnabled(QString)));
    hideElevBoxes();
    elevType.setCurrentIndex(Game::DefaultElevationBox);
    showElevBox(elevType.currentText());
    vbox->addItem(vlist);

    gradeOverlay.setText("Grade Overlay");
    gradeOverlay.setChecked(Game::gradeOverlayEnabled);
    gradeHelper.setText("Grade Helper...");
    gradeHelper.setCheckable(true);
    gradeHelper.setFocusPolicy(Qt::NoFocus);
    gradeLock.setText("Lock Grade");
    gradeLock.setCheckable(true);
    gradeLock.setChecked(Game::gradeLockEnabled);
    gradeLock.setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *gradeOverlayRow = new QHBoxLayout;
    gradeOverlayRow->setContentsMargins(3,1,3,0);
    gradeOverlayRow->addWidget(&gradeOverlay);
    gradeOverlayRow->addStretch(1);
    vbox->addLayout(gradeOverlayRow);

    QHBoxLayout *gradeHelperRow = new QHBoxLayout;
    gradeHelperRow->setContentsMargins(3,0,3,0);
    gradeHelperRow->addWidget(&gradeHelper);
    vbox->addLayout(gradeHelperRow);

    QHBoxLayout *gradeLockRow = new QHBoxLayout;
    gradeLockRow->setContentsMargins(3,0,3,1);
    gradeLockRow->addWidget(&gradeLock);
    vbox->addLayout(gradeLockRow);

    QObject::connect(&gradeOverlay, &QCheckBox::toggled, [this](bool checked){
        Game::gradeOverlayEnabled = checked;
        ++Game::gradeOverlayRevision;
    });
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
        if(checked){
            openGradeHelper();
        } else if(gradeHelperWindow != NULL && gradeHelperWindow->isVisible()){
            resetGradeHelper();
            requestMainFocus();
        }
    });
    QObject::connect(&gradeHelperUiTimer, &QTimer::timeout, this, [this](){
        gradeOverlay.blockSignals(true);
        gradeOverlay.setChecked(Game::gradeOverlayEnabled);
        gradeOverlay.blockSignals(false);
        refreshGradeLockUi();
        refreshGradeHelperUi();
    });
    gradeHelperUiTimer.start(120);
    refreshGradeLockUi();
    refreshGradeHelperUi();
    addRule();
    
    
    label = new QLabel("MSTS Collision:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vbox->addWidget(&eCollisionFlags);
    eCollisionFlags.setDisabled(true);
    eCollisionFlags.setAlignment(Qt::AlignCenter);
    cCollisionType.addItem("Disabled");
    cCollisionType.addItem("Immovable");
    cCollisionType.addItem("Buffer");
    cCollisionType.setStyleSheet("combobox-popup: 0;");
    vbox->addWidget(&cCollisionType);
    QObject::connect(&cCollisionType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(cCollisionTypeEdited(int)));
    //QPushButton *resetFlags = new QPushButton("Reset Flags", this);
    //QObject::connect(resetFlags, SIGNAL(released()),
    //                  this, SLOT(copyFEnabled()));
    //vbox->addWidget(resetFlags);
    addRule();
    label = new QLabel("Advanced:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    
    QPushButton *hacks = new QPushButton("Hacks", this);
    QObject::connect(hacks, SIGNAL(released()),
                      this, SLOT(hacksButtonEnabled()));
    vbox->addWidget(hacks);
    
    vbox->addStretch(1);
    this->setLayout(vbox);
    
    QObject::connect(copyF, SIGNAL(released()),
                      this, SLOT(copyFileNameEnabled()));
}

void PropertiesTrackObj::eTemplateEdited(QString val){
    if(trackObj == NULL){
        return;
    }
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->setTemplate(val);
    Undo::StateEnd();
}

void PropertiesTrackObj::elevTypeEdited(QString val){
    hideElevBoxes();
    showElevBox(val);
    ElevTypeName = val;
}

void PropertiesTrackObj::showElevBox(QString val){
    if(val == "Permille ‰"){
        elevProm.show();
        elevPromLabel.show();
    }
    if(val == "Percent %"){
        elevProp.show();
        elevPropLabel.show();
    }
    if(val == "1 in 'X' m"){
        elev1inXm.show();
        elev1inXmLabel.show();
    }
    if(val == "Angle º"){
        elevProg.show();
        elevProgLabel.show();
    }    
    setStepValue(Game::DefaultMoveStep);
}

void PropertiesTrackObj::hideElevBoxes(){
    elevProm.hide();
    elevProg.hide();
    elevProp.hide();
    elev1inXm.hide();
    elevPromLabel.hide();
    elevProgLabel.hide();
    elevPropLabel.hide();
    elev1inXmLabel.hide();
}

PropertiesTrackObj::~PropertiesTrackObj() {
    delete gradeHelperWindow;
    gradeHelperWindow = NULL;
}

void PropertiesTrackObj::fixJNodePosnEnabled(){
    if(trackObj == NULL){
        return;
    }
    Undo::SinglePushWorldObjData(worldObj);
    trackObj->fillJNodePosn();
    Undo::StateEnd();
}

void PropertiesTrackObj::hacksButtonEnabled(){
    if(trackObj == NULL){
        return;
    }
    
    QDialog d;
    d.setMinimumWidth(400);
    d.setWindowTitle("TrackObj Hacks");
    QVBoxLayout *vbox = new QVBoxLayout;
    QLabel *label = new QLabel("These functions will damage your route if you don't know what you are doing.");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    label->setWordWrap(true);
    
    QPushButton *fixJNodePosn = new QPushButton("Fix JNodePosn", this);
    QObject::connect(fixJNodePosn, SIGNAL(released()),
                      this, SLOT(fixJNodePosnEnabled()));
    vbox->addWidget(fixJNodePosn);
    
    QPushButton *haxRemoveTDBVector = new QPushButton("Remove TDB Vector ( remove TrItems first )", this);
    QObject::connect(haxRemoveTDBVector, SIGNAL(released()),
                      this, SLOT(haxRemoveTDBVectorEnabled()));
    vbox->addWidget(haxRemoveTDBVector);
    
    QPushButton *haxRemoveTDBTree = new QPushButton("Remove TDB Tree ( remove TrItems first; max 1000 nodes )", this);
    QObject::connect(haxRemoveTDBTree, SIGNAL(released()),
                      this, SLOT(haxRemoveTDBTreeEnabled()));
    vbox->addWidget(haxRemoveTDBTree);
    
    QPushButton *haxElevTDBVector = new QPushButton("[Fix sElev] Don't click me!", this);
    QObject::connect(haxElevTDBVector, SIGNAL(released()),
                      this, SLOT(haxElevTDBVectorEnabled()));
    vbox->addWidget(haxElevTDBVector);

    vbox->setSpacing(2);
    vbox->setContentsMargins(3,3,3,3);
    vbox->addStretch(1);
    d.setLayout(vbox);
    d.exec();
}

void PropertiesTrackObj::haxElevTDBVectorEnabled(){
    if(trackObj == NULL)
        return;
    if(Game::currentRoute == NULL)
        return;
    Game::currentRoute->fixTDBVectorElevation(trackObj);
}

void PropertiesTrackObj::haxRemoveTDBVectorEnabled(){
    if(trackObj == NULL)
        return;
    if(Game::currentRoute == NULL)
        return;
    Game::currentRoute->deleteTDBVector(trackObj);
}

void PropertiesTrackObj::haxRemoveTDBTreeEnabled(){
    if(trackObj == NULL)
        return;
    if(Game::currentRoute == NULL)
        return;
    
    Game::currentRoute->deleteTDBTree(trackObj);
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
    
    QString templateName = worldObj->getTemplate();
    if(templateName.length() == 0)
        eTemplate.setCurrentText("DEFAULT");
    else
        eTemplate.setCurrentText(templateName);
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

void PropertiesTrackObj::activateGradeAssistPlacement(){
    emit enableTool("placeTool");
}

void PropertiesTrackObj::refreshGradeLockUi(){
    gradeLock.blockSignals(true);
    gradeLock.setChecked(Game::gradeLockEnabled);
    gradeLock.blockSignals(false);
    if(Game::gradeLockEnabled){
        gradeLock.setText(QString("Locked Grade: %1%").arg(Game::gradeLockedPercent, 0, 'f', 5));
        gradeLock.setToolTip(QString("New track and road pieces will use a physical grade of %1%.").arg(Game::gradeLockedPercent, 0, 'f', 5));
    } else {
        gradeLock.setText("Lock Grade");
        gradeLock.setToolTip("Capture the selected physical grade and apply it to newly placed track and road pieces.");
    }
}

void PropertiesTrackObj::openGradeHelper(){
    if(trackObj == NULL)
        return;
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

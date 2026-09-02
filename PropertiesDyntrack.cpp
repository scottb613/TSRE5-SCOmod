/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesDyntrack.h"
#include <cmath>
#include "DynTrackObj.h"
#include "Flex.h"
#include "Game.h"
#include "GLMatrix.h"
#include "GuiFunct.h"
#include "TDB.h"

namespace {
int gradeDisplayDecimals(const int units){
    switch(units){
    case 0: return 1; // per mille: 0.1 per mille = 0.01%
    case 1: return 2; // percent
    case 2: return 2; // 1 in X
    case 3: return 3; // degrees: approximately equivalent to 0.01%
    default: return 2;
    }
}

QString gradeDisplayText(const double value, const int units){
    return QString::number(value, 'f', gradeDisplayDecimals(units));
}
}

PropertiesDyntrack::PropertiesDyntrack() {
    buttonTools["FlexTool"] = new QPushButton("Auto-Flex", this);
    GuiFunct::styleEditorActionButton(buttonTools["FlexTool"]);
    QMapIterator<QString, QPushButton*> i(buttonTools);
    while (i.hasNext()) {
        i.next();
        i.value()->setCheckable(true);
    }
    
    QVBoxLayout *vbox = new QVBoxLayout;
    const qreal panelScale = qBound(0.75f, Game::uiScale, 1.25f);
    const int panelMargin = qRound(4.0f * panelScale);
    const int cardMargin = qRound(6.0f * panelScale);
    const int panelSpacing = qRound(4.0f * panelScale);
    const int fieldLabelWidth = qRound(66.0f * panelScale);
    auto alignFormFields = [fieldLabelWidth, panelSpacing](QFormLayout *form) {
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setRowWrapPolicy(QFormLayout::DontWrapRows);
        form->setHorizontalSpacing(panelSpacing);
        for(int row = 0; row < form->rowCount(); row++){
            QLayoutItem *labelItem = form->itemAt(row, QFormLayout::LabelRole);
            if(labelItem != NULL && labelItem->widget() != NULL)
                labelItem->widget()->setMinimumWidth(fieldLabelWidth);
        }
    };
    vbox->setSpacing(panelSpacing);
    vbox->setContentsMargins(panelMargin, panelMargin,
                            panelMargin, panelMargin);
    GuiFunct::applyEditorPanelStyle(this);

    infoLabel = new QLabel("DYNAMIC TRACK");
    GuiFunct::styleEditorTitle(infoLabel);
    vbox->addWidget(infoLabel);

    QFrame *objectCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(objectCard);
    QFormLayout *vlist = new QFormLayout(objectCard);
    vlist->setSpacing(2);
    vlist->setContentsMargins(cardMargin, cardMargin,
                              cardMargin, cardMargin);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    this->eSectionIdx.setDisabled(true);
    this->eLength.setDisabled(true);
    this->eCurveCount.setDisabled(true);
    vlist->addRow("UiD:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    vlist->addRow("Index:",&this->eSectionIdx);
    vlist->addRow("Length:",&this->eLength);
    vlist->addRow("Curves:",&this->eCurveCount);
    alignFormFields(vlist);
    vbox->addWidget(objectCard);

    QLabel *flexModeLabel = new QLabel("FLEX MODE");
    GuiFunct::styleEditorSubtitle(flexModeLabel);
    vbox->addWidget(flexModeLabel);
    QFrame *flexCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(flexCard);
    QVBoxLayout *flexCardLayout = new QVBoxLayout(flexCard);
    flexCardLayout->setContentsMargins(cardMargin, cardMargin,
                                       cardMargin, cardMargin);
    flexCardLayout->setSpacing(panelSpacing);
    QLabel *nextGenFlexLabel = new QLabel("NextGen Flex S-C-S-C-S", flexCard);
    nextGenFlexLabel->setToolTip("Allows the solver to use up to two curve sections for compound and S-curve connections.");
    flexCardLayout->addWidget(nextGenFlexLabel);
    QWidget *flexButtonRow = new QWidget(flexCard);
    QHBoxLayout *flexButtonLayout = new QHBoxLayout(flexButtonRow);
    flexButtonLayout->setSpacing(0);
    flexButtonLayout->setContentsMargins(0,0,0,0);
    flexButtonLayout->addStretch(1);
    buttonTools["FlexTool"]->setMinimumWidth(125);
    flexButtonLayout->addWidget(buttonTools["FlexTool"]);
    flexButtonLayout->addStretch(1);
    flexCardLayout->addWidget(flexButtonRow);
    vbox->addWidget(flexCard);

    QLabel * label2 = new QLabel("SECTIONS");
    GuiFunct::styleEditorSubtitle(label2);
    vbox->addWidget(label2);
    QFrame *sectionsCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(sectionsCard);
    QVBoxLayout *sectionsLayout = new QVBoxLayout(sectionsCard);
    sectionsLayout->setContentsMargins(cardMargin, cardMargin,
                                       cardMargin, cardMargin);
    sectionsLayout->setSpacing(2);
    
    
    this->chSect[0].setText("First Straight:");
    this->chSect[0].setChecked(true);
    //this->chSect[0].setEnabled(false);// .setCheckable(false);
    sectionsLayout->addWidget(&chSect[0]);
    vSect[0].setSpacing(2);
    vSect[0].setContentsMargins(3,0,3,0);
    vSect[0].addRow("Length:",&this->sSectA[0]);
    alignFormFields(&vSect[0]);
    wSect[0].setLayout(&vSect[0]);
    sectionsLayout->addWidget(&wSect[0]);
    
    this->chSect[1].setText("First Curve:");
    sectionsLayout->addWidget(&chSect[1]);
    vSect[1].setSpacing(2);
    vSect[1].setContentsMargins(3,0,3,0);
    vSect[1].addRow("Angle:",&this->sSectA[1]);
    vSect[1].addRow("Radius:",&this->sSectR[1]);
    alignFormFields(&vSect[1]);
    wSect[1].setLayout(&vSect[1]);
    sectionsLayout->addWidget(&wSect[1]);
    
    
    this->chSect[2].setText("Second Straight:");
    sectionsLayout->addWidget(&chSect[2]);
    vSect[2].setSpacing(2);
    vSect[2].setContentsMargins(3,0,3,0);
    vSect[2].addRow("Length:",&this->sSectA[2]);
    alignFormFields(&vSect[2]);
    wSect[2].setLayout(&vSect[2]);
    sectionsLayout->addWidget(&wSect[2]);
    
    this->chSect[3].setText("Second Curve:");
    sectionsLayout->addWidget(&chSect[3]);
    vSect[3].setSpacing(2);
    vSect[3].setContentsMargins(3,0,3,0);
    vSect[3].addRow("Angle:",&this->sSectA[3]);
    vSect[3].addRow("Radius:",&this->sSectR[3]);
    alignFormFields(&vSect[3]);
    wSect[3].setLayout(&vSect[3]);
    sectionsLayout->addWidget(&wSect[3]);
    
    
    this->chSect[4].setText("Third Straight:");
    sectionsLayout->addWidget(&chSect[4]);
    vSect[4].setSpacing(2);
    vSect[4].setContentsMargins(3,0,3,0);
    vSect[4].addRow("Length:",&this->sSectA[4]);
    alignFormFields(&vSect[4]);
    wSect[4].setLayout(&vSect[4]);
    sectionsLayout->addWidget(&wSect[4]);
    vbox->addWidget(sectionsCard);
    
    QLabel *label = new QLabel("GRADE");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    QFrame *gradeCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(gradeCard);
    vlist = new QFormLayout(gradeCard);
    vlist->setSpacing(qRound(3.0f * qBound(0.75f, Game::uiScale, 1.25f)));
    vlist->setContentsMargins(cardMargin, cardMargin,
                              cardMargin, cardMargin);
    QDoubleValidator* doubleValidator = new QDoubleValidator(-10000, 10000, 6, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator* permilleValidator = new QDoubleValidator(-1000, 1000, 1, this);
    permilleValidator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator* percentValidator = new QDoubleValidator(-1000, 1000, 2, this);
    percentValidator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator* ratioValidator = new QDoubleValidator(-10000, 10000, 2, this);
    ratioValidator->setNotation(QDoubleValidator::StandardNotation);
    QDoubleValidator* angleValidator = new QDoubleValidator(-1000, 1000, 3, this);
    angleValidator->setNotation(QDoubleValidator::StandardNotation);
    
    //‰
    const int gradeFieldHeight = qRound(22.0f * qBound(0.75f, Game::uiScale, 1.25f));
    elevType.setMinimumHeight(gradeFieldHeight);
    elevProm.setMinimumHeight(gradeFieldHeight);
    elev1inXm.setMinimumHeight(gradeFieldHeight);
    elevProg.setMinimumHeight(gradeFieldHeight);
    elevProp.setMinimumHeight(gradeFieldHeight);
    elevStep.setMinimumHeight(gradeFieldHeight);

    vlist->addRow("Units: ",&this->elevType);
    elevType.addItem("Permille ‰");
    elevType.addItem("Percent %");
    elevType.addItem("1 in 'X' m");
    elevType.addItem("Angle º");
    elevType.setStyleSheet("combobox-popup: 0;");
    QObject::connect(&elevType, SIGNAL(currentTextChanged(QString)),
                      this, SLOT(elevTypeEdited(QString)));
    
    elevProm.setValidator(permilleValidator);
    QObject::connect(&elevProm, SIGNAL(textEdited(QString)), this, SLOT(elevPromEnabled(QString)));
    //oneInXm
    elev1inXm.setValidator(ratioValidator);
    QObject::connect(&elev1inXm, SIGNAL(textEdited(QString)), this, SLOT(elev1inXmEnabled(QString)));
    //º
    elevProg.setValidator(angleValidator);
    QObject::connect(&elevProg, SIGNAL(textEdited(QString)), this, SLOT(elevProgEnabled(QString)));
    //%
    elevProp.setValidator(percentValidator);
    QObject::connect(&elevProp, SIGNAL(textEdited(QString)), this, SLOT(elevPropEnabled(QString)));
    elevValueStack.addWidget(&elevProm);
    elevValueStack.addWidget(&elevProp);
    elevValueStack.addWidget(&elev1inXm);
    elevValueStack.addWidget(&elevProg);
    elevValueStack.setContentsMargins(0,0,0,0);
    vlist->addRow(&elevValueLabel, &elevValueStack);
    alignFormFields(vlist);
    elevStep.setValidator(doubleValidator);
    QObject::connect(&elevStep, SIGNAL(textEdited(QString)), this, SLOT(elevStepEnabled(QString)));
    elevType.setCurrentIndex(Game::DefaultElevationBox);
    showElevBox(elevType.currentText());
    vbox->addWidget(gradeCard);
    
    vbox->addStretch(1);
    this->setLayout(vbox);
    
    for(int i=0; i<5; i++){
        if(i%2 == 0){
            sSectA[i].setDecimals(2);
            sSectA[i].setMinimum(0);
            sSectA[i].setMaximum(5000);
            sSectA[i].setSingleStep(1.0);
        } else {
            sSectA[i].setDecimals(5);      /// EFO increasing precision from decimals(3)
            sSectA[i].setMinimum(-3.14);
            sSectA[i].setMaximum(3.14);
            sSectA[i].setSingleStep(0.01);
        }
        sSectR[i].setDecimals(3);  /// EFO increasing precision from decimals(2)
        sSectR[i].setMinimum(15);
        sSectR[i].setMaximum(15000);   //EFO increasing max radius to 15000 from 5000
        sSectR[i].setSingleStep(1.0);
    }
    
    for(int i = 0; i < 5; i++){
        dyntrackChSect.setMapping(&chSect[i], i);
        connect(&chSect[i], SIGNAL(clicked()), &dyntrackChSect, SLOT(map()));
    }
    
    QObject::connect(&dyntrackChSect, SIGNAL(mappedInt(int)),
            this, SLOT(chSectEnabled(int)));
    
    for(int i = 0; i < 5; i++){
        dyntrackSect.setMapping(&sSectR[i], i);
        connect(&sSectR[i], SIGNAL(valueChanged(double)), &dyntrackSect, SLOT(map()));
        dyntrackSect.setMapping(&sSectA[i], i);
        connect(&sSectA[i], SIGNAL(valueChanged(double)), &dyntrackSect, SLOT(map()));
    }
    
    QObject::connect(&dyntrackSect, SIGNAL(mappedInt(int)),
            this, SLOT(sSectEnabled(int)));
    
    QObject::connect(buttonTools["FlexTool"], SIGNAL(released()),
                      this, SLOT(flexEnabled()));
    
}

PropertiesDyntrack::~PropertiesDyntrack() {
}

void PropertiesDyntrack::msg(QString name, QString val){
    if(name == "toolEnabled"){
        QMapIterator<QString, QPushButton*> i(buttonTools);
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(true);
            i.value()->setChecked(false);
        }
        if(buttonTools[val] != NULL){
            buttonTools[val]->setChecked(true);
        }
        i.toFront();
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(false);
        }
    }
}

void PropertiesDyntrack::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    dobj = (DynTrackObj*)obj;
    this->infoLabel->setText("DYNAMIC TRACK");
    
    this->uid.setText(QString::number(dobj->UiD, 10));
    this->tX.setText(QString::number(dobj->x, 10));
    this->tY.setText(QString::number(-dobj->y, 10));
    if(dobj->sectionIdx < 0)
        this->eSectionIdx.setText("");
    else
        this->eSectionIdx.setText(QString::number(dobj->sectionIdx, 10));

    float totalLength = 0.0f;
    int curveCount = 0;
    for (int i = 0; i < 5; i++) {
        if (dobj->sections[i].sectIdx > 100000000)
            continue;
        if (dobj->sections[i].type == 1) {
            totalLength += fabs(dobj->sections[i].a * dobj->sections[i].r);
            curveCount++;
        } else {
            totalLength += fabs(dobj->sections[i].a);
        }
    }
    this->eLength.setText(QString::number(totalLength, 'f', 2) + " m");
    this->eCurveCount.setText(QString::number(curveCount));
    
    for (int i = 0; i < 5; i++) {
        if(dobj->sections[i].sectIdx > 1000000){
            this->chSect[i].setChecked(false);
            this->wSect[i].hide();
            continue;
        }
        this->wSect[i].show();
        this->chSect[i].setChecked(true);
        this->sSectA[i].blockSignals(true);
        this->sSectR[i].blockSignals(true);
        this->sSectA[i].setValue(dobj->sections[i].a);
        this->sSectR[i].setValue(dobj->sections[i].r);
        this->sSectA[i].blockSignals(false);
        this->sSectR[i].blockSignals(false);
    }
    
    ///////////
    elevType.setCurrentText(ElevTypeName);
    
    const float gradePermille = dobj->getAverageGradePermille();
     
    float oneInXm = 0.0;
    float prog = qRadiansToDegrees(qAtan(gradePermille/1000.0));
    float prop = gradePermille/10.0;

    //if(gradePermille > 0)
        oneInXm = qFuzzyIsNull(gradePermille) ? 0.0f : 1000.0f/gradePermille;
    this->elevProm.setText(gradeDisplayText(gradePermille, 0));
    this->elevProg.setText(gradeDisplayText(prog, 3));
    this->elevProp.setText(gradeDisplayText(prop, 1));
    this->elev1inXm.setText(gradeDisplayText(oneInXm, 2));
    setStepValue(Game::DefaultMoveStep);
    
    //this->carNumber.setText(QString::number(pobj->getCarNumber(),10));
    //this->carSpeed.setText(QString::number(pobj->getCarSpeed(),10));
}


void PropertiesDyntrack::updateObj(GameObj* obj){
    if(obj == NULL){
        return;
    }
    dobj = (DynTrackObj*)obj;
    const float gradePermille = dobj->getAverageGradePermille();
     
    float oneInXm = 0.0;
    oneInXm = qFuzzyIsNull(gradePermille) ? 0.0f : 1000.0f/gradePermille;
    float prog = qRadiansToDegrees(qAtan(gradePermille/1000.0));
    float prop = gradePermille/10.0;
       
    if(!this->elevProm.hasFocus() && !this->elev1inXm.hasFocus() && !this->elevProg.hasFocus() && !this->elevProp.hasFocus()){
        this->elevProm.setText(gradeDisplayText(gradePermille, 0));
        this->elevProg.setText(gradeDisplayText(prog, 3));
        this->elevProp.setText(gradeDisplayText(prop, 1));
        this->elev1inXm.setText(gradeDisplayText(oneInXm, 2));
    }

}

void PropertiesDyntrack::chSectEnabled(int idx){
    if(Game::debugOutput) qDebug() << "chSectEnabled";
    if(dobj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    
    bool state = this->chSect[idx].isChecked();
    
    if(state){
        this->wSect[idx].show();
        dobj->sections[idx].sectIdx = 0;
        if(idx%2 == 0){
            dobj->sections[idx].a = 2;
            dobj->sections[idx].r = 0;
        } else {
            dobj->sections[idx].a = 0.1;
            dobj->sections[idx].r = 100;
        }
        this->sSectA[idx].blockSignals(true);
        this->sSectR[idx].blockSignals(true);
        this->sSectA[idx].setValue(dobj->sections[idx].a);
        this->sSectR[idx].setValue(dobj->sections[idx].r);
        this->sSectA[idx].blockSignals(false);
        this->sSectR[idx].blockSignals(false);
    } else {
        this->wSect[idx].hide();
        dobj->sections[idx].sectIdx = 4294967295;
    }
    dobj->modified = true;
    dobj->deleteVBO();
}

void PropertiesDyntrack::sSectEnabled(int idx){
    if(Game::debugOutput) qDebug() << "sSectEnabled";
    if(dobj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    if(idx%2 == 1 && fabs(this->sSectA[idx].value()) < 0.0001 )
        return;

    dobj->sections[idx].a = this->sSectA[idx].value();
    if(idx%2 == 1)
        dobj->sections[idx].r = this->sSectR[idx].value();
    dobj->modified = true;
    dobj->box.loaded = false;
    dobj->deleteVBO();
}

bool PropertiesDyntrack::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "dyntrack")
        return true;
    return false;
}

void PropertiesDyntrack::flexEnabled(){
    emit enableTool("FlexTool");
}

void PropertiesDyntrack::flexData(int x, int z, float* p){
    emit enableTool("");
    if(dobj == NULL || p == NULL){
        qWarning() << "Auto-Flex rejected: no selected Dynamic Track or pointer position";
        emit flexResult(false);
        emit enableTool("selectTool");
        return;
    }
    
    if(Game::debugOutput) qDebug() << "flex1";
    
    float p1[3];
    float p2[3];
    p1[0] = dobj->position[0];
    p1[1] = dobj->position[1];
    p1[2] = dobj->position[2];
    p2[0] = p[0];
    p2[1] = p[1];
    p2[2] = p[2];

    int sourceX = dobj->x;
    int sourceZ = dobj->y;
    float sourceQ[4] = {0, 0, 0, 1};
    const int sourceConnectionId = Game::trackDB != NULL
            ? Game::trackDB->findNearestTrackConnection(
                    sourceX, sourceZ, p1, sourceQ,
                    dobj->x, dobj->y, dobj->UiD)
            : -1;
    if(sourceConnectionId < 0){
        qWarning() << "Auto-Flex rejected: no external source connection";
        emit flexResult(false);
        emit enableTool("selectTool");
        return;
    }

    const float sourceDx = (sourceX - dobj->x) * 2048.0f + p1[0] - dobj->position[0];
    const float sourceDy = p1[1] - dobj->position[1];
    const float sourceDz = (sourceZ - dobj->y) * 2048.0f + p1[2] - dobj->position[2];
    if(std::sqrt(sourceDx * sourceDx + sourceDy * sourceDy + sourceDz * sourceDz) > 4.0f){
        qWarning() << "Auto-Flex rejected: source connection is more than"
                << "4 metres from the dynamic-track origin";
        emit flexResult(false);
        emit enableTool("selectTool");
        return;
    }
    
    float dyntrackData[10];
    float visualElev = 0;
    float averageElev = 0;
    float resolvedSourceYaw = sourceQ[1];

    int destinationX = x;
    int destinationZ = z;
    float destinationQ[4] = {0, 0, 0, 1};
    const int destinationConnectionId = Game::trackDB != NULL
            ? Game::trackDB->findNearestTrackConnection(
                    destinationX, destinationZ, p2, destinationQ,
                    dobj->x, dobj->y, dobj->UiD)
            : -1;
    if(destinationConnectionId < 0){
        qWarning() << "Auto-Flex rejected: no destination track connection";
        emit flexResult(false);
        emit enableTool("selectTool");
        return;
    }
    if(destinationConnectionId == sourceConnectionId){
        qWarning() << "Auto-Flex rejected: source and destination belong"
                << "to the same TrackDB vector" << sourceConnectionId;
        emit flexResult(false);
        emit enableTool("selectTool");
        return;
    }
    
    bool success = Flex::AutoFlex(sourceX, sourceZ, (float*)p1,
            destinationX, destinationZ, (float*)p2,
            (float*)dyntrackData, visualElev, averageElev, 0.0f,
            sourceQ, destinationQ,
            &resolvedSourceYaw);
    if(Game::debugOutput) qDebug() << "flex2" << visualElev << averageElev;
    if(success){
        // The solver always uses the exact database source.  Move the world
        // object to that same point regardless of how the source was found.
        dobj->setPosition(sourceX, sourceZ, p1);
        float sourceOrientation[4];
        Quat::fill(sourceOrientation);
        Quat::rotateY(sourceOrientation, sourceOrientation,
                -resolvedSourceYaw);
        dobj->setQdirection(sourceOrientation);
        dobj->set("dyntrackdata", (float*)dyntrackData);
        dobj->setElevationFromSource(visualElev, averageElev);
        this->showObj(dobj);
    }
    emit flexResult(success);
    emit enableTool("selectTool");
}

void PropertiesDyntrack::elevTypeEdited(QString val){
    showElevBox(val);
    ElevTypeName = val;
}

void PropertiesDyntrack::showElevBox(QString val){
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

void PropertiesDyntrack::setStepValue(float step){
    if(elevType.currentIndex() == 0)
        step = step * 100;
    if(elevType.currentIndex() == 1)
        step = step * 10;
    if(elevType.currentIndex() == 2)
        step = 10.0/step;
    if(elevType.currentIndex() == 3)
        step = qRadiansToDegrees(qAtan(step/10.0));
    
    elevStep.setText(gradeDisplayText(step, elevType.currentIndex()));
}

float PropertiesDyntrack::getStepValue(float step){
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

void PropertiesDyntrack::elevPromEnabled(QString val){
    if(dobj == NULL){
        return;
    }
    bool ok = false;
    //prom
    float prom = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prom) > Game::trackElevationMaxPm + 0.000001) {   
        this->elevProm.setText(gradeDisplayText(Game::trackElevationMaxPm, 0));
        return;
    }
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    if(Game::debugOutput) qDebug () << "Game::trackElevationMaxPm" << Game::trackElevationMaxPm;
    this->elev1inXm.setText(gradeDisplayText(oneInXm, 2));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(gradeDisplayText(prog, 3));
    //prop 
    float prop = prom/10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    this->elevProp.setText(gradeDisplayText(prop, 1));
    
    Undo::SinglePushWorldObjData(worldObj);
    dobj->setElevation(prom);
}

void PropertiesDyntrack::elevStepEnabled(QString val){
    if(dobj == NULL)
        return;

    bool ok = false;
    float f = val.toFloat(&ok);
    if(!ok)
        return;
    
    f = getStepValue(f);
    
    Game::DefaultMoveStep = f;
    emit setMoveStep(f);
}

void PropertiesDyntrack::elev1inXmEnabled(QString val){
    if(dobj == NULL){
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
    this->elevProm.setText(gradeDisplayText(prom, 0));
    //prop 
    float prop = prom/10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    this->elevProp.setText(gradeDisplayText(prop, 1));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(gradeDisplayText(prog, 3));
    
    Undo::SinglePushWorldObjData(worldObj);
    dobj->setElevation(prom);
}

void PropertiesDyntrack::elevProgEnabled(QString val){
    if(dobj == NULL){
         return;
    }
    bool ok = false;
    //prog
    float prog = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prog) > qRadiansToDegrees(qAtan(Game::trackElevationMaxPm/1000.0))+ 0.000001) {   
        this->elevProg.setText(gradeDisplayText(
            qRadiansToDegrees(qAtan(Game::trackElevationMaxPm/1000.0)), 3));
        return;
    }
    //prop 
    float prop = qTan(qDegreesToRadians(prog))*100.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProp.setText(gradeDisplayText(prop, 1));
    //prom
    float prom = prop*10.0;
    if(Game::debugOutput) qDebug () << "prom" << prom;
    this->elevProm.setText(gradeDisplayText(prom, 0));
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    this->elev1inXm.setText(gradeDisplayText(oneInXm, 2));
     
    Undo::SinglePushWorldObjData(worldObj);
    dobj->setElevation(prom);
}
 
void PropertiesDyntrack::elevPropEnabled(QString val){
    if(dobj == NULL){
        return;
    }
    bool ok = false;
    //prop
    float prop = val.toFloat(&ok);
    if(!ok) return;
    if(fabs(prop) > (Game::trackElevationMaxPm/10.0)+ 0.000001)
    {    this->elevProp.setText(gradeDisplayText(Game::trackElevationMaxPm/10.0, 1));
        return;}
    //prom    
    float prom = prop*10.0;
    if(Game::debugOutput) qDebug () << "prop" << prop;
    if(Game::debugOutput) qDebug () << "prom" << prom;
    if(Game::debugOutput) qDebug () << "Game::trackElevationMaxPm/10.0: " << Game::trackElevationMaxPm/10.0;
    this->elevProm.setText(gradeDisplayText(prom, 0));
    //prog 
    float prog = qRadiansToDegrees(qAtan(prom/1000.0));
    if(Game::debugOutput) qDebug () << "prog" << prog;
    this->elevProg.setText(gradeDisplayText(prog, 3));
    //oneInXm
    float oneInXm = 1000.0/prom;
    if(Game::debugOutput) qDebug () << "oneInXm" << oneInXm;
    this->elev1inXm.setText(gradeDisplayText(oneInXm, 2));
    
    Undo::SinglePushWorldObjData(worldObj);
    dobj->setElevation(prom);
}

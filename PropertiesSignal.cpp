/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesSignal.h"
#include "SignalWindow.h"
#include "SignalObj.h"
#include "Game.h"
#include "TDB.h"
#include "SigCfg.h"
#include "SignalShape.h"
#include "ParserX.h"
#include "GLMatrix.h"
#include "GuiFunct.h"

PropertiesSignal::PropertiesSignal() {
    GuiFunct::applyEditorPanelStyle(this);
    signalWindow = new SignalWindow(this);
    
    QVBoxLayout *vbox = new QVBoxLayout;
    const int panelMargin = qRound(4.0f * qBound(0.75f, Game::uiScale, 1.25f));
    const int cardMargin = qRound(6.0f * qBound(0.75f, Game::uiScale, 1.25f));
    vbox->setSpacing(3);
    vbox->setContentsMargins(panelMargin,panelMargin,panelMargin,panelMargin);
    auto addSubtitle = [this, vbox](const QString &text){
        QLabel *label = new QLabel(QString(QChar(0x2022)) + ' ' + text, this);
        GuiFunct::styleEditorSubtitle(label);
        vbox->addWidget(label);
    };
    auto makeCard = [this, cardMargin](){
        QFrame *card = new QFrame(this);
        GuiFunct::styleEditorPanelCard(card);
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(cardMargin,cardMargin,cardMargin,cardMargin);
        layout->setSpacing(3);
        return qMakePair(card, layout);
    };
    
    infoLabel = new QLabel("Signal:");
    GuiFunct::styleEditorSubtitle(infoLabel);
    vbox->addWidget(infoLabel);
    auto identityCard = makeCard();
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    vlist->addRow("UiD:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    GuiFunct::alignEditorForm(vlist);
    identityCard.second->addLayout(vlist);
    vbox->addWidget(identityCard.first);
    auto shapeCard = makeCard();
    QFormLayout *shapeForm = new QFormLayout;
    shapeForm->setContentsMargins(0,0,0,0);
    shapeForm->addRow("Shape:", &name);
    shapeForm->addRow("Desc:", &description);
    GuiFunct::alignEditorForm(shapeForm);
    shapeCard.second->addLayout(shapeForm);
    subObjectsButton = new QPushButton("Subobjects...", this);
    subObjectsButton->setCheckable(true);
    subObjectsButton->setFocusPolicy(Qt::NoFocus);
    subObjectsButton->setProperty("editorPopupKey", "signalSubObjects");
    GuiFunct::styleEditorActionButton(subObjectsButton);
    shapeCard.second->addWidget(subObjectsButton);
    connect(subObjectsButton, &QPushButton::clicked, this, [this](){
        emit userButtonPressed();
    });
    connect(subObjectsButton, &QPushButton::toggled,
            this, &PropertiesSignal::showSubObjList);
    vbox->addWidget(shapeCard.first);
    /// EFO shift signal by negative signal offset
    addSubtitle("Signal Actions");
    auto signalActionsCard = makeCard();
    QPushButton *button = new QPushButton("Shift -Offset", this);
    GuiFunct::styleEditorActionButton(button);
    signalActionsCard.second->addWidget(button);
    connect(button, SIGNAL(released()), this, SLOT(shiftSignal()));    
    button = new QPushButton("Shift +Offset", this);
    GuiFunct::styleEditorActionButton(button);
    signalActionsCard.second->addWidget(button);
    connect(button, SIGNAL(released()), this, SLOT(flipSignal()));
    chFlipShape.setText("Flip Shape");
    chFlipShape.setChecked(true);
    signalActionsCard.second->addWidget(&chFlipShape);
    vbox->addWidget(signalActionsCard.first);
    addSubtitle("Position & Rotation");
    auto positionCard = makeCard();
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("X:",&this->posX);
    QDoubleValidator* doubleValidator = new QDoubleValidator(-1500, 1500, 6, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    this->posX.setValidator(doubleValidator);
    QObject::connect(&this->posX, SIGNAL(textEdited(QString)), this, SLOT(editPositionEnabled(QString)));
    vlist->addRow("Y:",&this->posY);
    this->posY.setValidator(doubleValidator);
    QObject::connect(&this->posY, SIGNAL(textEdited(QString)), this, SLOT(editPositionEnabled(QString)));
    vlist->addRow("Z:",&this->posZ);
    this->posZ.setValidator(doubleValidator);
    QObject::connect(&this->posZ, SIGNAL(textEdited(QString)), this, SLOT(editPositionEnabled(QString)));
    this->quat.setDisabled(true);
    this->quat.setAlignment(Qt::AlignCenter);
    vlist->addRow("Rot:",&this->quat);
    GuiFunct::alignEditorForm(vlist);
    positionCard.second->addLayout(vlist);
    QGridLayout *posRotList = new QGridLayout;
    posRotList->setSpacing(2);
    posRotList->setContentsMargins(0,0,0,0);    

    QPushButton *copyPos = new QPushButton("Copy Pos", this);
    GuiFunct::styleEditorActionButton(copyPos);
    QObject::connect(copyPos, SIGNAL(released()),
                      this, SLOT(copyPEnabled()));
    QPushButton *pastePos = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pastePos);
    QObject::connect(pastePos, SIGNAL(released()),
                      this, SLOT(pastePEnabled()));
    QPushButton *copyQrot = new QPushButton("Copy Rot", this);
    GuiFunct::styleEditorActionButton(copyQrot);
    QObject::connect(copyQrot, SIGNAL(released()),
                      this, SLOT(copyREnabled()));
    QPushButton *pasteQrot = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pasteQrot);
    QObject::connect(pasteQrot, SIGNAL(released()),
                      this, SLOT(pasteREnabled()));
    QPushButton *copyPosRot = new QPushButton("Copy Pos+Rot", this);
    GuiFunct::styleEditorActionButton(copyPosRot);
    QObject::connect(copyPosRot, SIGNAL(released()),
                      this, SLOT(copyPREnabled()));
    QPushButton *pastePosRot = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pastePosRot);
    QObject::connect(pastePosRot, SIGNAL(released()),
                      this, SLOT(pastePREnabled()));
    QPushButton *resetQrot = new QPushButton("Reset Rot", this);
    GuiFunct::styleEditorActionButton(resetQrot);
    QObject::connect(resetQrot, SIGNAL(released()),
                      this, SLOT(resetRotEnabled()));
    QPushButton *qRot90 = new QPushButton("Rot Y 90°", this);
    GuiFunct::styleEditorActionButton(qRot90);
    QObject::connect(qRot90, SIGNAL(released()),
                      this, SLOT(rotYEnabled()));
    QPushButton *transform = new QPushButton("Transform...", this);
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
    positionCard.second->addLayout(posRotList);
    vbox->addWidget(positionCard.first);
    
    // EFO adding StaticDetailLevel to Signals
    
    addSubtitle("Detail Level");
    auto detailCard = makeCard();
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
    detailCard.second->addLayout(detailLevelView);
    vbox->addWidget(detailCard.first);
    
    /// EFO End adding StaticDetailLevel to Signal
    
    addSubtitle("Rendering & Flags");
    auto flagsCard = makeCard();
    this->flags.setDisabled(true);
    this->flags.setAlignment(Qt::AlignCenter);
    flagsCard.second->addWidget(&this->flags);
    QGridLayout *flagslView = new QGridLayout;
    flagslView->setSpacing(2);
    flagslView->setContentsMargins(0,0,0,0);    
    QPushButton *copyFlags = new QPushButton("Copy Flags", this);
    GuiFunct::styleEditorActionButton(copyFlags);
    QObject::connect(copyFlags, SIGNAL(released()),
                      this, SLOT(copyFEnabled()));
    QPushButton *pasteFlags = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pasteFlags);
    QObject::connect(pasteFlags, SIGNAL(released()),
                      this, SLOT(pasteFEnabled()));
    flagslView->addWidget(copyFlags,0,0);
    flagslView->addWidget(pasteFlags,0,1);
    flagsCard.second->addLayout(flagslView);
    checkboxAnim.setText("Animate Object");
    checkboxTerrain.setText("Terrain Object");
    flagsCard.second->addWidget(&checkboxAnim);
    QObject::connect(&checkboxAnim, SIGNAL(stateChanged(int)),
                      this, SLOT(checkboxAnimEdited(int)));
    flagsCard.second->addWidget(&checkboxTerrain);
    QObject::connect(&checkboxTerrain, SIGNAL(stateChanged(int)),
                      this, SLOT(checkboxTerrainEdited(int)));
    cShadowType.addItem("No Shadow");
    cShadowType.addItem("Round Shadow");
    cShadowType.addItem("Rect. Shadow");
    cShadowType.addItem("Treeline Shadow");
    cShadowType.addItem("Dynamic Shadow");
    cShadowType.setStyleSheet("combobox-popup: 0;");
    flagsCard.second->addWidget(&cShadowType);
    vbox->addWidget(flagsCard.first);
    QObject::connect(&cShadowType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(cShadowTypeEdited(int)));
    
    addSubtitle("Advanced");
    auto advancedCard = makeCard();
    hacks.setText("HACKS...");
    hacks.setCheckable(true);
    hacks.setFocusPolicy(Qt::NoFocus);
    hacks.setProperty("editorPopupKey", "hacksHelper");
    hacks.setToolTip("Open context-sensitive repair and cleanup tools for the selected signal and route.");
    GuiFunct::styleEditorActionButton(&hacks);
    QObject::connect(&hacks, &QPushButton::clicked, this, [this](){
        emit userButtonPressed();
    });
    QObject::connect(&hacks, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&hacks, checked);
        emit hacksToggled(sobj, &hacks, checked);
    });
    advancedCard.second->addWidget(&hacks);
    vbox->addWidget(advancedCard.first);
    
    QObject::connect(signalWindow, SIGNAL(sendMsg(QString,QString)),
        this, SLOT(msg(QString,QString)));
    QObject::connect(signalWindow, &SignalWindow::helperClosed,
                     this, &PropertiesSignal::subObjectsWindowClosed);
    QObject::connect(signalWindow, &SignalWindow::userButtonPressed,
                     this, &PropertiesSignal::userButtonPressed);

    
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesSignal::~PropertiesSignal() {
    delete signalWindow;
    signalWindow = NULL;
}

void PropertiesSignal::msg(QString name, QString val){
    if(name == "enableTool"){
        emit enableTool(val);
    }
}

void PropertiesSignal::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    sobj = (SignalObj*)obj;
    this->infoLabel->setText("Object: "+sobj->type);
    this->uid.setText(QString::number(sobj->UiD, 10));
    this->tX.setText(QString::number(sobj->x, 10));
    this->tY.setText(QString::number(-sobj->y, 10));

    this->posX.setText(QString::number(worldObj->position[0], 'G', 6));
    this->posY.setText(QString::number(worldObj->position[1], 'G', 6));
    this->posZ.setText(QString::number(-worldObj->position[2], 'G', 6));
    this->quat.setText(
            QString::number(worldObj->qDirection[0], 'G', 4) + " " +
            QString::number(worldObj->qDirection[1], 'G', 4) + " " +
            QString::number(-worldObj->qDirection[2], 'G', 4) + " " +
            QString::number(worldObj->qDirection[3], 'G', 4)
            );
    
    /*for (int i = 0; i < maxSubObj; i++) {
        this->wSub[i].hide();
        this->chSub[i].setChecked(false);
        this->bSub[i].hide();
        this->bSub[i].setEnabled(false);
    }*/
    
    TDB* tdb = Game::trackDB;
    SignalShape* signalShape = tdb->sigCfg->signalShape[sobj->fileName];
    /*for(auto kv : tdb->sigCfg->signalShape) {
        qDebug() << "shape "<< QString::fromStdString(kv.first);
    } 
    qDebug() << "req " << sobj->fileName;*/
    
    if(signalShape == NULL){ 
        infoLabel->setText("NULL");
        return;
    }
    
    this->name.setText(sobj->fileName);
    this->description.setText(signalShape->desc);
    this->name.setToolTip(this->name.text());
    this->description.setToolTip(this->description.text());

    signalWindow->showObj(sobj);
    
    this->flags.setText(ParserX::MakeFlagsString(sobj->staticFlags));
    this->checkboxAnim.blockSignals(true);
    this->checkboxTerrain.blockSignals(true);
    this->cShadowType.blockSignals(true);
    this->checkboxAnim.setChecked(sobj->isAnimated());
    this->checkboxTerrain.setChecked(sobj->isTerrainObj());
    this->cShadowType.setCurrentIndex((int)sobj->getShadowType());
    this->checkboxAnim.blockSignals(false);
    this->checkboxTerrain.blockSignals(false);
    this->cShadowType.blockSignals(false);
}

void PropertiesSignal::updateObj(GameObj* obj){
    if(sobj == NULL){
        return;
    }
    
    if(!posX.hasFocus() && !posY.hasFocus() && !posZ.hasFocus() && !quat.hasFocus()){
        this->uid.setText(QString::number(worldObj->UiD, 10));
        this->tX.setText(QString::number(worldObj->x, 10));
        this->tY.setText(QString::number(-worldObj->y, 10));
        this->posX.setText(QString::number(worldObj->position[0], 'G', 6));
        this->posY.setText(QString::number(worldObj->position[1], 'G', 6));
        this->posZ.setText(QString::number(-worldObj->position[2], 'G', 6));
        this->quat.setText(
                QString::number(worldObj->qDirection[0], 'G', 4) + " " +
                QString::number(worldObj->qDirection[1], 'G', 4) + " " +
                QString::number(-worldObj->qDirection[2], 'G', 4) + " " +
                QString::number(worldObj->qDirection[3], 'G', 4)
                );
    }
    
    this->signalWindow->updateObj(sobj);
    
}

void PropertiesSignal::shiftSignal()
{
    if(sobj == NULL)
        return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    sobj->flip(chFlipShape.isChecked());

    if(Game::sigOffset)
    {       
            if(Game::debugOutput)  qDebug() << "Filename: " << sobj->fileName.toLower() ;

            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"1:P "<<sobj->firstPosition[0]<<" "<<sobj->firstPosition[1]<<" "<<sobj->firstPosition[2]<<" Q" <<sobj->qDirection[1]<<" "<<sobj->qDirection[3] ;            
            float thisOffset = 0;
            if(sobj->fileName.toLower().contains("gantry") == false) thisOffset = Game::sigOffset;
                
            sobj->position[0] = sobj->firstPosition[0];   /// (reset to the TrItemObj coordinates)
            // sobj->position[1] = sobj->firstPosition[1];
            sobj->position[2] = sobj->firstPosition[2];
            float pos[3];
            
            pos[0] = -thisOffset;
            pos[1] = 0;
            pos[2] = 0;
            /// comment this out for the rotation
            Vec3::transformQuat((float*)pos, (float*)pos, (float*)sobj->qDirection);
            sobj->translate(pos[0], pos[1], pos[2]);
            sobj->modified = true;
            sobj->setMartix(); 
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"2:P "<<sobj->position[0]<<" "<<sobj->position[1]<<" "<<sobj->position[2]<<" Q" <<sobj->qDirection[1]<<" "<<sobj->qDirection[3] ;                        
    }

    Undo::StateEnd();
}

void PropertiesSignal::flipSignal(){    
    if(sobj == NULL)
        return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    sobj->flip(chFlipShape.isChecked());

    if(Game::sigOffset)
    {       
            if(Game::debugOutput)  qDebug() << "Filename: " << sobj->fileName.toLower() ;

            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"1:P "<<sobj->firstPosition[0]<<" "<<sobj->firstPosition[1]<<" "<<sobj->firstPosition[2]<<" Q" <<sobj->qDirection[1]<<" "<<sobj->qDirection[3] ;            
            float thisOffset = 0;
            if(sobj->fileName.toLower().contains("gantry") == false) thisOffset = Game::sigOffset;
                
            sobj->position[0] = sobj->firstPosition[0];   /// (reset to the TrItemObj coordinates)
            // sobj->position[1] = sobj->firstPosition[1];
            sobj->position[2] = sobj->firstPosition[2];
            float pos[3];
            
            pos[0] = thisOffset;
            pos[1] = 0;
            pos[2] = 0;
            Vec3::transformQuat((float*)pos, (float*)pos, (float*)sobj->qDirection);
            sobj->translate(pos[0], pos[1], pos[2]);
            sobj->modified = true;
            sobj->setMartix(); 
            if(Game::debugOutput)  qDebug() << __FILE__ << " " << __LINE__ << ":" <<"2:P "<<sobj->position[0]<<" "<<sobj->position[1]<<" "<<sobj->position[2]<<" Q" <<sobj->qDirection[1]<<" "<<sobj->qDirection[3] ;                        
    }

    Undo::StateEnd();
}

void PropertiesSignal::showSubObjList(bool checked){
    GuiFunct::setEditorPopupButtonActive(subObjectsButton, checked);
    if(checked)
        signalWindow->showForOwner();
    else
        signalWindow->close();
}

QPushButton *PropertiesSignal::hacksButton(){
    return &hacks;
}

void PropertiesSignal::subObjectsWindowClosed(){
    subObjectsButton->blockSignals(true);
    subObjectsButton->setChecked(false);
    subObjectsButton->blockSignals(false);
    GuiFunct::setEditorPopupButtonActive(subObjectsButton, false);
}

bool PropertiesSignal::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "signal")
        return true;
    return false;
}

void PropertiesSignal::checkboxAnimEdited(int val){
    if(worldObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        worldObj->setAnimated(true);
    } else {
        worldObj->setAnimated(false);
    }
    this->flags.setText(ParserX::MakeFlagsString(worldObj->staticFlags));
}

void PropertiesSignal::checkboxTerrainEdited(int val){
    if(worldObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        worldObj->setTerrainObj(true);
    } else {
        worldObj->setTerrainObj(false);
    }
    this->flags.setText(ParserX::MakeFlagsString(worldObj->staticFlags));
}

void PropertiesSignal::cShadowTypeEdited(int val){
    if(worldObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    worldObj->setShadowType((WorldObj::ShadowType)val);
    this->flags.setText(ParserX::MakeFlagsString(worldObj->staticFlags));
}

//// SDL for Signals EFO

void PropertiesSignal::enableCustomDetailLevelEnabled(int val){
    if(worldObj == NULL)
        return;
    SignalObj* signalObj = (SignalObj*) worldObj;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        customDetailLevel.setEnabled(true);
        customDetailLevel.setText("0");
        signalObj->setCustomDetailLevel(0);
    } else {
        customDetailLevel.setEnabled(false);
        customDetailLevel.setText("");
        signalObj->setCustomDetailLevel(-1);
    }
}

void PropertiesSignal::customDetailLevelEdited(QString val){
    if(worldObj == NULL)
        return;
    SignalObj* signalObj = (SignalObj*) worldObj;
    bool ok = false;
    int level = val.toInt(&ok);
    //qDebug() << "aaaaaaaaaa";
    if(ok){
        Undo::SinglePushWorldObjData(worldObj);
        signalObj->setCustomDetailLevel(level);
    }
}


/// END SDL for Signals EFO


void PropertiesSignal::editPositionEnabled(QString val){
    if(worldObj == NULL)
        return;
    SignalObj* signalObj = (SignalObj*) worldObj;
    float pos[3];
    bool ok = false;
    pos[0] = this->posX.text().toFloat(&ok);
    if(!ok) return;
    pos[1] = this->posY.text().toFloat(&ok);
    if(!ok) return;
    pos[2] = -this->posZ.text().toFloat(&ok);
    if(!ok) return;
    
    Undo::SinglePushWorldObjData(worldObj);
    signalObj->setPosition((float*)pos);
    signalObj->modified = true;
    signalObj->setMartix();
}

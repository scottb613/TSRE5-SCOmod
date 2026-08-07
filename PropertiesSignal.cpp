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
#include "Game.h"
#include "TextEditDialog.h"
#include "GLMatrix.h"
#include <QGuiApplication>
#include <QScreen>

PropertiesSignal::PropertiesSignal() {
    signalWindow = new SignalWindow(this);
    
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    
    infoLabel = new QLabel("Signal:");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    vlist->addRow("UiD:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    vbox->addItem(vlist);
    QLabel* label = new QLabel("Name:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vbox->addWidget(&name);
    label = new QLabel("Description:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vbox->addWidget(&description);
    /// EFO shift signal by negative signal offset
    QPushButton *button = new QPushButton("Shift", this);
    vbox->addWidget(button);    
    connect(button, SIGNAL(released()), this, SLOT(shiftSignal()));    
    button = new QPushButton("Flip", this);
    vbox->addWidget(button);
    connect(button, SIGNAL(released()), this, SLOT(flipSignal()));
    chFlipShape.setText("Flip Shape");
    chFlipShape.setChecked(true);
    vbox->addWidget(&chFlipShape);
    label = new QLabel("SubObjects:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    button = new QPushButton("Show list", this);
    vbox->addWidget(button);
    connect(button, SIGNAL(released()), this, SLOT(showSubObjList()));
    
    label = new QLabel("Position & Rotation:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
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
    
    // EFO adding StaticDetailLevel to Signals
    
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
    
    /// EFO End adding StaticDetailLevel to Signal
    
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
    checkboxAnim.setText("Animate Object");
    checkboxTerrain.setText("Terrain Object");
    vbox->addWidget(&checkboxAnim);
    QObject::connect(&checkboxAnim, SIGNAL(stateChanged(int)),
                      this, SLOT(checkboxAnimEdited(int)));
    vbox->addWidget(&checkboxTerrain);
    QObject::connect(&checkboxTerrain, SIGNAL(stateChanged(int)),
                      this, SLOT(checkboxTerrainEdited(int)));
    cShadowType.addItem("No Shadow");
    cShadowType.addItem("Round Shadow");
    cShadowType.addItem("Rect. Shadow");
    cShadowType.addItem("Treeline Shadow");
    cShadowType.addItem("Dynamic Shadow");
    cShadowType.setStyleSheet("combobox-popup: 0;");
    vbox->addWidget(&cShadowType);
    QObject::connect(&cShadowType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(cShadowTypeEdited(int)));
    
    label = new QLabel("Advanced:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    QPushButton *hacks = new QPushButton("Hacks", this);
    QObject::connect(hacks, SIGNAL(released()),
                      this, SLOT(hacksButtonEnabled()));
    vbox->addWidget(hacks);
    
    QObject::connect(signalWindow, SIGNAL(sendMsg(QString,QString)),
        this, SLOT(msg(QString,QString)));   


    label = new QLabel("To link signals: click \"Link\" followed by \"Set Head\" before clicking on the track vector to protect");
    label->setContentsMargins(3,20,0,0);
    label->setWordWrap(true);
    vbox->addWidget(label);

    
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesSignal::~PropertiesSignal() {
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

    signalWindow->showObj(sobj);
    
    const QRect rec = QGuiApplication::primaryScreen()->geometry();
    signalWindow->move(rec.width()/2-signalWindow->width()/2 ,rec.height()/2-signalWindow->height()/2);
    
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

void PropertiesSignal::showSubObjList(){
    this->signalWindow->show();
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

void PropertiesSignal::hacksButtonEnabled(){
    if(sobj == NULL){
        return;
    }
    
    QDialog d;
    d.setMinimumWidth(400);
    d.setWindowTitle("SignalObj Hacks");
    QVBoxLayout *vbox = new QVBoxLayout;
    QLabel *label = new QLabel("Use only if you know what you are doing.");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    label->setWordWrap(true);
    QPushButton *haxRemoveTDBVector = new QPushButton("Fix TrSignalType Flags", this);
    QObject::connect(haxRemoveTDBVector, SIGNAL(released()),
                      this, SLOT(haxFixFlagsEnabled()));
    vbox->addWidget(haxRemoveTDBVector);
    
    /*QPushButton *haxRemoveTDBTree = new QPushButton("Remove TDB Tree ( remove TrItems first; max 1000 nodes )", this);
    QObject::connect(haxRemoveTDBTree, SIGNAL(released()),
                      this, SLOT(haxRemoveTDBTreeEnabled()));
    vbox->addWidget(haxRemoveTDBTree);*/
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,3,3,3);
    vbox->addStretch(1);
    d.setLayout(vbox);
    d.exec();
}

void PropertiesSignal::haxFixFlagsEnabled(){
    if(sobj == NULL){
        return;
    }
    QStringList list;

    sobj->checkFlags(list);
    TextEditDialog dialog;
    QString txt;
    txt += "___old_____new___\n";
    for(int i = 0; i < list.size(); i++)
        txt += list[i]+"\n";
    dialog.textBox.setPlainText(txt);
    dialog.setWindowTitle("Signal Flags");
    dialog.exec();
    if(dialog.changed == 1){
        sobj->fixFlags();
    }

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

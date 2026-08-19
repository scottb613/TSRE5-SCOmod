/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesCarspawner.h"
#include "WorldObj.h"
#include "CarSpawnerObj.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesCarspawner::PropertiesCarspawner() {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(4,4,4,4);
    
    infoLabel = new QLabel("Carspawner:");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    this->uid.setDisabled(true);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    this->lengthPlatform.setDisabled(true);
    vlist->addRow("UiD:",&this->uid);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    vlist->addRow("Length:",&this->lengthPlatform);
    GuiFunct::alignEditorForm(vlist);
    QFrame *identityCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(identityCard);
    QVBoxLayout *identityLayout = new QVBoxLayout(identityCard);
    identityLayout->setContentsMargins(6,4,6,4);
    identityLayout->addLayout(vlist);
    vbox->addWidget(identityCard);
    QLabel *label = new QLabel(QString::fromUtf8("• Traffic"));
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    QFrame *trafficCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(trafficCard);
    QFormLayout *trafficForm = new QFormLayout(trafficCard);
    trafficForm->setContentsMargins(6,4,6,4);
    trafficForm->addRow("Cars:", &carNumber);
    trafficForm->addRow("Speed:", &carSpeed);
    //vbox->addWidget(&this->useCustomList);
    //useCustomList.setText("Use custom car list.");
    trafficForm->addRow("List:", &carspawnList);
    GuiFunct::alignEditorForm(trafficForm);
    vbox->addWidget(trafficCard);
    carspawnList.setStyleSheet("combobox-popup: 0;");
    label = new QLabel("Track Items:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    QPushButton *bExpandSelected = new QPushButton("Expand");
    GuiFunct::styleEditorActionButton(bExpandSelected);
    QFrame *actionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(actionCard);
    QVBoxLayout *actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(4,3,4,3);
    actionLayout->addWidget(bExpandSelected);
    vbox->addWidget(actionCard);
    QObject::connect(bExpandSelected, SIGNAL(released()),
                      this, SLOT(bExpandEnabled()));
    vbox->addStretch(1);
    this->setLayout(vbox);
    QDoubleValidator* doubleValidator = new QDoubleValidator(0, 100, 1, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    carNumber.setValidator(doubleValidator);
    carSpeed.setValidator(doubleValidator);
    
    QObject::connect(&carNumber, SIGNAL(textEdited(QString)),
                      this, SLOT(carNumberEnabled(QString)));
    QObject::connect(&carSpeed, SIGNAL(textEdited(QString)),
                      this, SLOT(carSpeedEnabled(QString)));
    //QObject::connect(&useCustomList, SIGNAL(stateChanged(int)),
    //                  this, SLOT(useCustomListEnabled(int)));
    QObject::connect(&carspawnList, SIGNAL(textActivated(QString)),
                      this, SLOT(carspawnListSelected(QString)));
}

PropertiesCarspawner::~PropertiesCarspawner() {
}

void PropertiesCarspawner::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    this->infoLabel->setText("Object: "+worldObj->type);
    this->uid.setText(QString::number(worldObj->UiD, 10));
    this->tX.setText(QString::number(worldObj->x, 10));
    this->tY.setText(QString::number(-worldObj->y, 10));
    
    cobj = (CarSpawnerObj*)obj;
    this->lengthPlatform.setText(QString::number(cobj->getLength())+" m");
    this->carNumber.setText(QString::number(cobj->getCarNumber()));
    this->carSpeed.setText(QString::number(cobj->getCarSpeed()));
    
    carspawnList.clear();
    //carspawnList.setDisabled(true);
    //useCustomList.blockSignals(true);
    //useCustomList.setChecked(false);
    QString clist = cobj->getCarListName();
    carspawnList.setCurrentIndex(0);
    //useCustomList.setChecked(true);
    //carspawnList.setEnabled(true);
    for(int i = 0; i < cobj->carSpawnerList.length(); i++ ){
        carspawnList.addItem(cobj->carSpawnerList[i].name, i);
        if(cobj->carSpawnerList[i].name.toLower() == clist.toLower() )
            carspawnList.setCurrentIndex(i);
    }
    //useCustomList.blockSignals(false);
}

void PropertiesCarspawner::carNumberEnabled(QString val){
    if(cobj == NULL) return;
    Undo::SinglePushWorldObjData(worldObj);
    cobj->setCarNumber(val.toFloat());
}

void PropertiesCarspawner::carSpeedEnabled(QString val){
    if(cobj == NULL) return;
    Undo::SinglePushWorldObjData(worldObj);
    cobj->setCarSpeed(val.toFloat());
}

bool PropertiesCarspawner::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "carspawner")
        return true;
    return false;
}

void PropertiesCarspawner::carspawnListSelected(QString val){
    if(cobj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    if(carspawnList.currentIndex() == 0)
        cobj->setCarListName("");
    else
        cobj->setCarListName(val);
}

void PropertiesCarspawner::bExpandEnabled(){
    if(cobj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    cobj->expand();
}

void PropertiesCarspawner::useCustomListEnabled(int val){
    if(cobj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        if(cobj->carSpawnerList.length() == 0)
            return;
        for(int i = 0; i < cobj->carSpawnerList.length(); i++ ){
            carspawnList.addItem(cobj->carSpawnerList[i].name, i);
        }
        cobj->setCarListName(carspawnList.currentText());
        carspawnList.setEnabled(true);
    } else {
        carspawnList.clear();
        carspawnList.setDisabled(true);
        cobj->setCarListName("");
    }
}

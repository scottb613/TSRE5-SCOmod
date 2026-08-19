/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesActivityObject.h"
#include "Game.h"
#include "ActivityObject.h"
#include "Activity.h"
#include "GuiFunct.h"

PropertiesActivityObject::PropertiesActivityObject() {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(4,4,4,4);
    infoLabel = new QLabel("ActivityObject:");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Type:",&eObjectType);
    vlist->addRow("Id:",&eId);
    vlist->addRow("eId:",&eEid);
    eObjectType.setDisabled(true);
    eId.setDisabled(true);
    eEid.setDisabled(true);
    GuiFunct::alignEditorForm(vlist);
    QFrame *identityCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(identityCard);
    QVBoxLayout *identityLayout = new QVBoxLayout(identityCard);
    identityLayout->setContentsMargins(6,4,6,4);
    identityLayout->addLayout(vlist);
    vbox->addWidget(identityCard);
    
    QPushButton *bDelete = new QPushButton("Delete");
    GuiFunct::styleEditorActionButton(bDelete);
    QObject::connect(bDelete, SIGNAL(released()), this, SLOT(bDeleteEnabled()));
    QFrame *actionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(actionCard);
    QVBoxLayout *actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(4,3,4,3);
    actionLayout->addWidget(bDelete);
    vbox->addWidget(actionCard);
    
    QLabel *label = new QLabel("Owned by:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    QFrame *ownerCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(ownerCard);
    QVBoxLayout *ownerLayout = new QVBoxLayout(ownerCard);
    ownerLayout->setContentsMargins(6,4,6,4);
    ownerLayout->addWidget(&eActivityName);
    vbox->addWidget(ownerCard);
    
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesActivityObject::~PropertiesActivityObject() {
}

void PropertiesActivityObject::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    actObj = (ActivityObject*)obj;
    
    infoLabel->setText("Object: ActivityObject");
    eObjectType.setText(actObj->objectType);
    eId.setText(QString::number(actObj->getId()));
    eEid.setText(QString::number(actObj->getSelectedElementId()));
    eActivityName.setText(actObj->getParentName());

}

void PropertiesActivityObject::updateObj(GameObj* obj){
    if(obj == NULL){
        return;
    }
    actObj = (ActivityObject*)obj;

}

void PropertiesActivityObject::bDeleteEnabled(){
    if(actObj == NULL){
        return;
    }
    actObj->remove();
    emit sendMsg(QString("unselect"));
}

bool PropertiesActivityObject::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj == GameObj::activityobj)
        return true;
    return false;
}

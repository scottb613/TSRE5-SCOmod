/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesHazard.h"
#include "HazardObj.h"
#include "WorldObj.h"
#include "Game.h"
#include "TDB.h"
#include "TRitem.h"
#include "TrackItemObj.h"
#include "ParserX.h"

PropertiesHazard::PropertiesHazard() {
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    
    infoLabel = new QLabel("Hazard:");
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
    vlist->addRow("Pos X:",&this->posX);
    vlist->addRow("Pos Y:",&this->posY);    
    vlist->addRow("Pos Z:",&this->posZ);    


    vbox->addItem(vlist);
    // name
    QLabel* label = new QLabel("Hazard Name:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vbox->addWidget(&this->nameHazard);

    label = new QLabel("TrItem:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);    
    vbox->addWidget(&this->trItemRef);    

/*
    label = new QLabel("TrItemSData:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);    
    vbox->addWidget(&this->trItemSData);    

    label = new QLabel("TrItemPData:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);    
    vbox->addWidget(&this->trItemPData);    

    label = new QLabel("TrItemRData:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);    
    vbox->addWidget(&this->trItemRData);    
*/
    
    // misc
    label = new QLabel("Misc:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
           
    vbox->addStretch(1);
    this->setLayout(vbox);
    
}

PropertiesHazard::~PropertiesHazard() {
}

void PropertiesHazard::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*) obj;
    hazardObj = (HazardObj*) obj;
    
    
    this->uid.setText(QString::number(worldObj->UiD, 10));
    this->tX.setText(QString::number(worldObj->x, 10));
    this->tY.setText(QString::number(-worldObj->y, 10));
    this->posX.setText(QString::number(worldObj->position[0],'G',6));
    this->posY.setText(QString::number(worldObj->position[1],'G',6));    
    this->posZ.setText(QString::number(-worldObj->position[2],'G',6));

    TDB* tdb = Game::trackDB;
    
    
    this->infoLabel->setText("Object: "+worldObj->type);
    this->nameHazard.setText(hazardObj->fileName);
    this->trItemRef.setText(QString::number(hazardObj->trItemId[1]));
    
    
    
}


bool PropertiesHazard::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "hazzard")
        return true;
    if(((WorldObj*)obj)->type == "hazard")
        return true;
    return false;
}


/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */


#include "PropertiesUndefined.h"
#include "WorldObj.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesUndefined::PropertiesUndefined(){
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(4,4,4,4);
    infoLabel = new QLabel("Select to see properties.");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    QFrame *messageCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(messageCard);
    QVBoxLayout *messageLayout = new QVBoxLayout(messageCard);
    messageLayout->setContentsMargins(6,4,6,4);
    messageLayout->addWidget(infoLabel);
    vbox->addWidget(messageCard);
    
    
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesUndefined::~PropertiesUndefined() {
}

void PropertiesUndefined::showObj(GameObj* obj){
    if(obj == NULL)
        infoLabel->setText("Select to see properties.");
    else if(obj->typeObj == GameObj::worldobj)
        infoLabel->setText("Unsupported: "+((WorldObj*)obj)->type);
    else
        infoLabel->setText("Unsupported");
}

bool PropertiesUndefined::support(GameObj* obj){
    return true;
}

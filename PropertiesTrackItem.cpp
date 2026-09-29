/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesTrackItem.h"
#include "TRitem.h"
#include "Game.h"
#include "GuiFunct.h"
#include <cmath>

PropertiesTrackItem::PropertiesTrackItem() {
    GuiFunct::applyEditorPanelStyle(this);
   QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(4,4,4,4);
    infoLabel = new QLabel("Terrain:");
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
    vlist->addRow("Tile Y:",&this->tY);

    vlist->addRow("Pos X:",&posX);
    vlist->addRow("Pos Z:",&posZ);
    
    
    GuiFunct::alignEditorForm(vlist);
    vlist->addRow("Type:", &eItemType);
    eItemType.setDisabled(true);
    vlist->addRow("Id:", &eItemId);
    eItemId.setDisabled(true);
    QFrame *itemCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(itemCard);
    QVBoxLayout *itemLayout = new QVBoxLayout(itemCard);
    itemLayout->setContentsMargins(6,4,6,4);
    itemLayout->addLayout(vlist);
    vbox->addWidget(itemCard);
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesTrackItem::~PropertiesTrackItem() {
}


void PropertiesTrackItem::showObj(GameObj* obj){
    itemObj = nullptr;
    uid.clear();
    posX.clear();
    posZ.clear();
    tX.clear();
    tY.clear();
    eItemType.clear();
    eItemId.clear();
    if(!support(obj)){
        infoLabel->setText(tr("No Track Item selected"));
        return;
    }
    itemObj = static_cast<TRitem*>(obj);
    infoLabel->setText(tr("Object: TrackItem"));
    uid.setText(QString::number(itemObj->trItemId));
    eItemType.setText(itemObj->type);
    eItemId.setText(QString::number(itemObj->trItemId));

    // F11 deliberately exposes incomplete items. RData is optional; selecting
    // an item with no map position must still display its identity safely.
    const float* position = itemObj->trItemRData;
    const auto coordinate = [this,position](int index) {
        return position != nullptr && std::isfinite(position[index])
            ? QString::number(position[index]) : tr("Unavailable");
    };
    posX.setText(coordinate(0));
    posZ.setText(coordinate(2));
    tX.setText(coordinate(3));
    tY.setText(coordinate(4));
}

void PropertiesTrackItem::updateObj(GameObj* obj){
    showObj(obj);
}

bool PropertiesTrackItem::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj == GameObj::tritemobj)
        return true;
    return false;
}

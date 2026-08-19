/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ErrorMessageProperties.h"
#include <QDebug>
#include "Game.h"
#include "ErrorMessage.h"
#include "GeoCoordinates.h"
#include "GameObj.h"
#include "GuiFunct.h"
#include "TDB.h"
#include "TRitem.h"

ErrorMessageProperties::ErrorMessageProperties(QWidget* parent) : QWidget(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    setFixedHeight(qRound(150 * qBound(0.75f, Game::uiScale, 1.25f)));
    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(2,2,2,2);
    
    QLabel *label = new QLabel(QString(QChar(0x2022)) + " Selected Message");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);

    QFrame *detailsCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(detailsCard);
    QGridLayout *vlist = new QGridLayout(detailsCard);
    vlist->setSpacing(2);
    vlist->setContentsMargins(4,4,4,4);
    vlist->setAlignment(Qt::AlignTop);
    vlist->setColumnStretch(1,1);
    int row = 0;
    
    vlist->addWidget(&lMessage,row,0);
    vlist->addWidget(&eMessage,row,1);
    vlist->addWidget(&bSelect,row++,2);
    vlist->addWidget(&lAction,row,0);
    vlist->addWidget(&eAction,row++,1,1,3);
    vlist->addWidget(&lLocation,row,0);
    vlist->addWidget(&eLocation,row,1);
    vlist->addWidget(&bLocation,row++,2);
    vlist->addWidget(&bDelete,0,3);
    lMessage.setText("Message:");
    lMessage.hide();
    eMessage.hide();
    eMessage.setReadOnly(true);
    lAction.hide();
    lAction.setText("Description:");
    lAction.setAlignment(Qt::AlignTop);
    eAction.hide();
    eAction.setReadOnly(true);
    lLocation.setText("Location:");
    lLocation.hide();
    eLocation.hide();
    eLocation.setReadOnly(true);
    bLocation.hide();
    bSelect.hide();
    bDelete.hide();
    
    GuiFunct::styleEditorActionButton(&bLocation);
    GuiFunct::styleEditorActionButton(&bSelect);
    GuiFunct::styleEditorActionButton(&bDelete);
    vbox->addWidget(detailsCard);
    QObject::connect(&bLocation, SIGNAL(released()), this, SLOT(jumpToLocation()));
    QObject::connect(&bSelect, SIGNAL(released()), this, SLOT(bSelectReleased()));
    QObject::connect(&bDelete, SIGNAL(released()), this, SLOT(deleteCurrentItem()));
    
    this->setLayout(vbox);
}

ErrorMessageProperties::~ErrorMessageProperties() {
}

void ErrorMessageProperties::showMessage(ErrorMessage* msg){
    currentMessage = msg;
    lMessage.hide();
    lAction.hide();
    eAction.hide();
    lLocation.hide();
    bLocation.hide();
    eLocation.hide();
    bSelect.hide();
    bDelete.hide();
    if(currentMessage == NULL){
        return;
    }

    lMessage.show();
    eMessage.show();
    eMessage.setText(currentMessage->description);
    if(currentMessage->action.length() > 0){
        lAction.show();
        eAction.show();
        eAction.setPlainText(currentMessage->action);
    }
    
    if(currentMessage->obj != NULL){
        bSelect.show();
        bSelect.setEnabled(true);
        if(currentMessage->obj->typeObj == GameObj::tritemobj){
            bSelect.setText("Select Item");
            if(currentMessage->source == ErrorMessage::Source_TDB
            || currentMessage->source == ErrorMessage::Source_RDB){
                bDelete.setText("Delete Item");
                bDelete.setToolTip(
                    "Remove this invalid TrackDB/RoadDB item after confirmation.");
                bDelete.show();
            }
        } else {
            bSelect.setText("Select Object");
        }
    }
    
    if(currentMessage->coords != NULL){
        lLocation.show();
        eLocation.show();
        eLocation.setText(QString("Tile: ") + QString::number(currentMessage->coords->TileX) + " "+ QString::number(currentMessage->coords->TileZ) + " " + 
        ". Coordinates: " + QString::number(currentMessage->coords->wX) + " "+ QString::number(currentMessage->coords->wY) + " "+ QString::number(currentMessage->coords->wZ) + " ");
        bLocation.setText("Jump");
        bLocation.show();
    }
    
    
}

void ErrorMessageProperties::jumpToLocation(){
    emit jumpTo(currentMessage->coords);
}

void ErrorMessageProperties::bSelectReleased(){
    emit selectObject(currentMessage->obj);
}

void ErrorMessageProperties::deleteCurrentItem(){
    if(currentMessage == NULL || currentMessage->obj == NULL
    || currentMessage->obj->typeObj != GameObj::tritemobj)
        return;

    TDB *database = NULL;
    QString databaseName;
    if(currentMessage->source == ErrorMessage::Source_TDB){
        database = Game::trackDB;
        databaseName = "TrackDB";
    } else if(currentMessage->source == ErrorMessage::Source_RDB){
        database = Game::roadDB;
        databaseName = "RoadDB";
    }
    if(database == NULL)
        return;

    TRitem *item = static_cast<TRitem*>(currentMessage->obj);
    const int itemId = item->trItemId;
    if(itemId < 0 || itemId >= database->iTRitems
    || database->trackItems[itemId] != item){
        bDelete.setEnabled(false);
        bDelete.setToolTip("This logged item is no longer a live database target.");
        return;
    }

    if(!GuiFunct::confirmDestructiveAction(
            this, "DELETE DATABASE ITEM",
            QString("Delete %1 item %2?\n\n"
                    "This item has no usable map position. The operation uses "
                    "the same removal path as AutoFix.")
                .arg(databaseName).arg(itemId)))
        return;

    database->deleteTrItem(itemId);
    currentMessage->type = ErrorMessage::Type_AutoFix;
    if(!currentMessage->action.isEmpty())
        currentMessage->action += "\n";
    currentMessage->action +=
        QString("Resolved manually: %1 item %2 removed.")
            .arg(databaseName).arg(itemId);
    currentMessage->obj = NULL;
    eAction.setPlainText(currentMessage->action);
    lAction.show();
    eAction.show();
    bSelect.hide();
    bDelete.hide();
    emit messageUpdated();
}

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
#include "ErrorMessagesLib.h"
#include "GeoCoordinates.h"
#include "GameObj.h"
#include "GuiFunct.h"
#include "TDB.h"
#include "TRitem.h"
#include "RejectedWorldFile.h"

ErrorMessageProperties::ErrorMessageProperties(QWidget* parent) : QWidget(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    
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
    vlist->addWidget(&eMessage,row++,1,1,3);
    vlist->addWidget(&lAction,row,0);
    vlist->addWidget(&eAction,row++,1,1,3);
    vlist->addWidget(&lLocation,row,0);
    vlist->addWidget(&eLocation,row++,1,1,3);
    QFrame *messageSeparator = new QFrame(detailsCard);
    messageSeparator->setFrameShape(QFrame::HLine);
    messageSeparator->setFrameShadow(QFrame::Sunken);
    vlist->addWidget(messageSeparator,row++,0,1,4);
    QHBoxLayout *actions = new QHBoxLayout;
    actions->setSpacing(2);
    for(QPushButton *button : {&bLocation,&bSelect,&bDelete,&bNoFactor}){
        QFrame *cell = new QFrame(detailsCard);
        GuiFunct::styleEditorPanelCard(cell);
        QVBoxLayout *contents = new QVBoxLayout(cell);
        contents->setContentsMargins(4,3,4,3);
        contents->setSpacing(0);
        contents->addWidget(button);
        actions->addWidget(cell,1);
    }
    vlist->addLayout(actions,row,0,1,4);
    lMessage.setText("Message:");
    lMessage.setAlignment(Qt::AlignTop);
    lMessage.hide();
    eMessage.hide();
    eMessage.setReadOnly(true);
    eMessage.setLineWrapMode(QPlainTextEdit::WidgetWidth);
    eMessage.setFixedHeight(eMessage.fontMetrics().lineSpacing() * 2 + 12);
    eMessage.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lAction.hide();
    lAction.setText("Description:");
    lAction.setAlignment(Qt::AlignTop);
    eAction.hide();
    eAction.setReadOnly(true);
    eAction.setFixedHeight(eAction.fontMetrics().lineSpacing() * 4 + 12);
    lLocation.setText("Location:");
    lLocation.hide();
    eLocation.hide();
    eLocation.setReadOnly(true);
    // Selection must not resize the log viewport underneath a mouse click.
    // Reserve the same detail rows even when a record has no optional data.
    QWidget *detailFields[] = {&lMessage, &eMessage, &lAction, &eAction,
                              &lLocation, &eLocation};
    for(QWidget *field : detailFields){
        QSizePolicy policy = field->sizePolicy();
        policy.setRetainSizeWhenHidden(true);
        field->setSizePolicy(policy);
    }
    bSelect.setText("Select");
    bSelect.setToolTip("Select the object or database item associated with this record.");
    bLocation.setText("Jump");
    bDelete.setText("Delete");
    bNoFactor.setText("No Factor");
    bNoFactor.setToolTip("Hide this diagnostic for this route, including future sessions. Reset Status restores hidden records.");
    for(QPushButton *button : {&bSelect, &bLocation, &bDelete, &bNoFactor}){
        button->setEnabled(false);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    
    GuiFunct::styleEditorActionButton(&bLocation);
    GuiFunct::styleEditorActionButton(&bSelect);
    GuiFunct::styleEditorActionButton(&bDelete);
    GuiFunct::styleEditorActionButton(&bNoFactor);
    vbox->addWidget(detailsCard);
    QObject::connect(&bLocation, SIGNAL(released()), this, SLOT(jumpToLocation()));
    QObject::connect(&bSelect, SIGNAL(released()), this, SLOT(bSelectReleased()));
    QObject::connect(&bDelete, SIGNAL(released()), this, SLOT(deleteCurrentItem()));
    connect(&bNoFactor, &QPushButton::clicked, this, [this](){
        if(currentMessage != nullptr)
            emit noFactorRequested(currentMessage);
    });
    
    this->setLayout(vbox);
}

ErrorMessageProperties::~ErrorMessageProperties() {
}

void ErrorMessageProperties::showMessage(ErrorMessage* msg){
    currentMessage = msg;
    eMessage.clear();
    eMessage.hide();
    eAction.clear();
    eLocation.clear();
    lMessage.hide();
    lAction.hide();
    eAction.hide();
    lLocation.hide();
    bLocation.setEnabled(false);
    eLocation.hide();
    bSelect.setEnabled(false);
    bDelete.setEnabled(false);
    bNoFactor.setEnabled(false);
    if(currentMessage == NULL){
        return;
    }

    lMessage.show();
    eMessage.show();
    eMessage.setPlainText(currentMessage->description);
    eMessage.setToolTip(currentMessage->description);
    bNoFactor.setEnabled(Game::currentRoute != nullptr);
    if(!currentMessage->rejectedWorldHash.isEmpty()){
        const QString activeWorld = QFileInfo(Game::root + "/routes/" + Game::route + "/world").canonicalFilePath();
        bDelete.setEnabled(Game::writeEnabled && activeWorld == currentMessage->rejectedWorldRoot);
        bDelete.setToolTip("Remove the rejected file from the active world list after confirmation; preserve its exact contents as a recovery .bak file.");
    }
    if(currentMessage->action.length() > 0){
        lAction.show();
        eAction.show();
        eAction.setPlainText(currentMessage->action);
    }
    
    if(currentMessage->obj != NULL){
        bSelect.setEnabled(true);
        if(currentMessage->obj->typeObj == GameObj::tritemobj){
            if(currentMessage->source == ErrorMessage::Source_TDB
            || currentMessage->source == ErrorMessage::Source_RDB){
                bDelete.setToolTip(
                    "Remove this invalid TrackDB/RoadDB item after confirmation.");
                TDB *database = currentMessage->source == ErrorMessage::Source_TDB
                        ? Game::trackDB : Game::roadDB;
                TRitem *item = static_cast<TRitem*>(currentMessage->obj);
                bDelete.setEnabled(database != nullptr && item->trItemId >= 0
                        && item->trItemId < database->iTRitems
                        && database->trackItems[item->trItemId] == item);
                bSelect.setEnabled(bDelete.isEnabled());
            }
        }
    }
    
    if(currentMessage->coords != NULL){
        lLocation.show();
        eLocation.show();
        eLocation.setText(QString("Tile: ") + QString::number(currentMessage->coords->TileX) + " "+ QString::number(currentMessage->coords->TileZ) + " " + 
        ". Coordinates: " + QString::number(currentMessage->coords->wX) + " "+ QString::number(currentMessage->coords->wY) + " "+ QString::number(currentMessage->coords->wZ) + " ");
        bLocation.setText("Jump");
        eLocation.setToolTip(eLocation.text());
        bLocation.setEnabled(qIsFinite(currentMessage->coords->wX)
                && qIsFinite(currentMessage->coords->wY)
                && qIsFinite(currentMessage->coords->wZ));
    }
    
    
}

void ErrorMessageProperties::jumpToLocation(){
    if(currentMessage != nullptr && bLocation.isEnabled())
        emit jumpTo(currentMessage->coords);
}

void ErrorMessageProperties::bSelectReleased(){
    if(currentMessage != nullptr && currentMessage->obj != nullptr)
        emit selectObject(currentMessage->obj);
}

void ErrorMessageProperties::deleteCurrentItem(){
    if(currentMessage != nullptr && !currentMessage->rejectedWorldHash.isEmpty()){
        if(!Game::writeEnabled) return;
        const QString root = currentMessage->rejectedWorldRoot;
        const QString name = currentMessage->rejectedWorldName;
        const QByteArray hash = currentMessage->rejectedWorldHash;
        if(!GuiFunct::confirmDestructiveAction(this, "DELETE REJECTED WORLD FILE",
                QString("Remove %1 from the active world files?\n\n"
                        "The original will be preserved beside it with a unique .bak suffix. "
                        "It will no longer be loaded or scanned as a world tile. "
                        "Scan again afterward to refresh coverage.").arg(name))) return;
        QString recovery, error;
        if(!Game::writeEnabled || !RejectedWorldFile::remove(
                Game::root + "/routes/" + Game::route + "/world", root, name, hash, recovery, error)){
            GuiFunct::showEditorStopped(this, "File Not Deleted",
                error.isEmpty() ? "Route writing is disabled." : error);
            return;
        }
        for(ErrorMessage *message : ErrorMessagesLib::ErrorMessages){
            if(message != nullptr && message->rejectedWorldRoot == root
                    && message->rejectedWorldName == name){
                message->rejectedWorldHash.clear();
                message->type = ErrorMessage::Type_AutoFix;
                message->action = "Removed from the active world list. Recovery file: "
                        + recovery + "\nScan again to refresh coverage.";
            }
        }
        emit messageUpdated();
        return;
    }
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

    QVector<ErrorMessage*> matchingMessages;
    for(ErrorMessage *message : ErrorMessagesLib::ErrorMessages)
        if(message != NULL && message->obj == item)
            matchingMessages.append(message);

    database->deleteTrItem(itemId);
    const QString resolution = QString("Resolved manually: %1 item %2 removed.")
        .arg(databaseName).arg(itemId);
    for(ErrorMessage *message : matchingMessages){
        message->type = ErrorMessage::Type_AutoFix;
        if(!message->action.isEmpty())
            message->action += "\n";
        message->action += resolution;
        message->obj = NULL;
    }
    emit messageUpdated();
}

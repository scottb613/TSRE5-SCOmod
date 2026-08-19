/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesSiding.h"
#include "WorldObj.h"
#include "PlatformObj.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesSiding::PropertiesSiding() {
    GuiFunct::applyEditorPanelStyle(this);
    const int panelMargin = qRound(4.0f * qBound(0.75f, Game::uiScale, 1.25f));
    const int cardMargin = qRound(6.0f * qBound(0.75f, Game::uiScale, 1.25f));
    QVBoxLayout *vbox = new QVBoxLayout;
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
    
    infoLabel = new QLabel("Platform:");
    GuiFunct::styleEditorSubtitle(infoLabel);
    vbox->addWidget(infoLabel);
    auto identityCard = makeCard();
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
    identityCard.second->addLayout(vlist);
    vbox->addWidget(identityCard.first);
    // name
    addSubtitle("Name");
    auto nameCard = makeCard();
    QFormLayout *nameForm = new QFormLayout;
    nameForm->setContentsMargins(0,0,0,0);
    nameForm->addRow("Siding:", &this->namePlatform);
    GuiFunct::alignEditorForm(nameForm);
    nameCard.second->addLayout(nameForm);
    vbox->addWidget(nameCard.first);
    // misc
    addSubtitle("Options");
    auto optionsCard = makeCard();
    disablePlatform.setText("Disable Platform");
    optionsCard.second->addWidget(&disablePlatform);
    vbox->addWidget(optionsCard.first);
    vbox->addStretch(1);
    this->setLayout(vbox);
    
    QObject::connect(&disablePlatform, SIGNAL(stateChanged(int)),
                      this, SLOT(disablePlatformEnabled(int)));
    QObject::connect(&namePlatform, SIGNAL(textEdited(QString)),
                      this, SLOT(namePlatformEnabled(QString)));
}

PropertiesSiding::~PropertiesSiding() {
}

void PropertiesSiding::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    this->uid.setText(QString::number(worldObj->UiD, 10));
    this->tX.setText(QString::number(worldObj->x, 10));
    this->tY.setText(QString::number(-worldObj->y, 10));
    this->infoLabel->setText("Object: "+worldObj->type);
    pobj = (PlatformObj*)obj;
    this->lengthPlatform.setText(QString::number(pobj->getLength())+" m");
    this->namePlatform.setText(pobj->getPlatformName());
    this->disablePlatform.setChecked(pobj->getDisabled());
}

void PropertiesSiding:: disablePlatformEnabled(int state){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    if(state == Qt::Checked)
        pobj->setDisabled(true);
    else
        pobj->setDisabled(false);
    Undo::StateEnd();
}

void PropertiesSiding::namePlatformEnabled(QString val){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setPlatformName(val);
    Undo::StateEnd();
}

bool PropertiesSiding::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "siding")
        return true;
    return false;
}

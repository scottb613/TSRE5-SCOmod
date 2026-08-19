/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesPlatform.h"
#include "PlatformObj.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesPlatform::PropertiesPlatform() {
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
    // names
    addSubtitle("Names");
    auto namesCard = makeCard();
    QFormLayout *namesForm = new QFormLayout;
    namesForm->setContentsMargins(0,0,0,0);
    namesForm->setSpacing(2);
    namesForm->addRow("Station:", &this->nameStation);
    namesForm->addRow("Platform:", &this->namePlatform);
    GuiFunct::alignEditorForm(namesForm);
    namesCard.second->addLayout(namesForm);
    vbox->addWidget(namesCard.first);
    // side
    addSubtitle("Side");
    auto sideCard = makeCard();
    leftSide.setText("Left");
    rightSide.setText("Right");
    QHBoxLayout *sideLayout = new QHBoxLayout;
    sideLayout->setContentsMargins(0,0,0,0);
    sideLayout->addWidget(&leftSide);
    sideLayout->addWidget(&rightSide);
    sideCard.second->addLayout(sideLayout);
    vbox->addWidget(sideCard.first);
    // wait
    addSubtitle("Platform Wait");
    auto waitCard = makeCard();
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Minutes:",&this->waitMin);
    vlist->addRow("Seconds:",&this->waitSec);
    vlist->addRow("Passengers:",&this->waitPas);
    GuiFunct::alignEditorForm(vlist);
    waitCard.second->addLayout(vlist);
    vbox->addWidget(waitCard.first);
    // misc
    addSubtitle("Options");
    auto optionsCard = makeCard();
    disablePlatform.setText("Disable Platform");
    optionsCard.second->addWidget(&disablePlatform);
    vbox->addWidget(optionsCard.first);
    vbox->addStretch(1);
    this->setLayout(vbox);

    waitMin.setValidator( new QIntValidator(0, 100, this) );
    waitSec.setValidator( new QIntValidator(0, 60, this) );
    waitPas.setValidator( new QIntValidator(0, 999, this) );
    
    QObject::connect(&leftSide, SIGNAL(stateChanged(int)),
                      this, SLOT(leftSideEnabled(int)));
    QObject::connect(&rightSide, SIGNAL(stateChanged(int)),
                      this, SLOT(rightSideEnabled(int)));
    QObject::connect(&disablePlatform, SIGNAL(stateChanged(int)),
                      this, SLOT(disablePlatformEnabled(int)));
    QObject::connect(&nameStation, SIGNAL(textEdited(QString)),
                      this, SLOT(nameStationEnabled(QString)));
    QObject::connect(&namePlatform, SIGNAL(textEdited(QString)),
                      this, SLOT(namePlatformEnabled(QString)));
    QObject::connect(&waitMin, SIGNAL(textEdited(QString)),
                      this, SLOT(waitMinEnabled(QString)));
    QObject::connect(&waitSec, SIGNAL(textEdited(QString)),
                      this, SLOT(waitSecEnabled(QString)));
    QObject::connect(&waitPas, SIGNAL(textEdited(QString)),
                      this, SLOT(waitPasEnabled(QString)));
}

PropertiesPlatform::~PropertiesPlatform() {
}

void PropertiesPlatform::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    pobj = (PlatformObj*)obj;
    this->infoLabel->setText("Object: "+pobj->type);
    this->uid.setText(QString::number(pobj->UiD, 10));
    this->tX.setText(QString::number(pobj->x, 10));
    this->tY.setText(QString::number(-pobj->y, 10));
    this->lengthPlatform.setText(QString::number(pobj->getLength())+" m");
    this->nameStation.setText(pobj->getStationName());
    this->namePlatform.setText(pobj->getPlatformName());
    int sec = pobj->getPlatformMinWaitingTime();
    int min = sec/60;
    sec = sec - min*60;
    this->waitMin.setText(QString::number(min, 10));
    this->waitSec.setText(QString::number(sec, 10));
    this->waitPas.setText(QString::number(pobj->getPlatformNumPassengersWaiting(), 10));
    this->leftSide.blockSignals(true);
    this->leftSide.setChecked(pobj->getSideLeft());
    this->leftSide.blockSignals(false);
    this->rightSide.blockSignals(true);
    this->rightSide.setChecked(pobj->getSideRight());
    this->rightSide.blockSignals(false);
    this->disablePlatform.blockSignals(true);
    this->disablePlatform.setChecked(pobj->getDisabled());
    this->disablePlatform.blockSignals(false);
}

bool PropertiesPlatform::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "platform")
        return true;
    return false;
}

void PropertiesPlatform::leftSideEnabled(int state){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    if(state == Qt::Checked)
        pobj->setSideLeft(true);
    else
        pobj->setSideLeft(false);
    Undo::StateEnd();
}
void PropertiesPlatform::rightSideEnabled(int state){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    if(state == Qt::Checked)
        pobj->setSideRight(true);
    else
        pobj->setSideRight(false);
    Undo::StateEnd();
}
void PropertiesPlatform:: disablePlatformEnabled(int state){
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
void PropertiesPlatform::nameStationEnabled(QString val){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setStationName(val);
    Undo::StateEnd();
}
void PropertiesPlatform::namePlatformEnabled(QString val){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setPlatformName(val);
    Undo::StateEnd();
}
void PropertiesPlatform::waitMinEnabled(QString val){
    if(pobj == NULL) return;
    int min = this->waitMin.text().toInt(0,10);
    int sec = this->waitSec.text().toInt(0,10);
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setPlatformMinWaitingTime(min*60+sec);
    Undo::StateEnd();
}
void PropertiesPlatform::waitSecEnabled(QString val){
    if(pobj == NULL) return;
    int min = this->waitMin.text().toInt(0,10);
    int sec = this->waitSec.text().toInt(0,10);
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setPlatformMinWaitingTime(min*60+sec);
    Undo::StateEnd();
}
void PropertiesPlatform::waitPasEnabled(QString val){
    if(pobj == NULL) return;
    Undo::StateBegin();
    Undo::PushGameObjData(worldObj);
    Undo::PushTrackDB(Game::trackDB);
    pobj->setPlatformNumPassengersWaiting(val.toInt(0,10));
    Undo::StateEnd();
}

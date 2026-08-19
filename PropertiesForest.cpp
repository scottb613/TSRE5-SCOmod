/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesForest.h"
#include "WorldObj.h"
#include "ForestObj.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesForest::PropertiesForest() {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(3);
    vbox->setContentsMargins(4,4,4,4);
    auto addSubtitle = [this, vbox](const QString &text){
        QLabel *label = new QLabel(QString::fromUtf8("• ") + text, this);
        GuiFunct::styleEditorSubtitle(label);
        vbox->addWidget(label);
    };
    auto makeCard = [this](){
        QFrame *card = new QFrame(this);
        GuiFunct::styleEditorPanelCard(card);
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(6,4,6,4);
        layout->setSpacing(2);
        return qMakePair(card, layout);
    };
    infoLabel = new QLabel("Forest:");
    GuiFunct::styleEditorSubtitle(infoLabel);
    vbox->addWidget(infoLabel);

    auto identityCard = makeCard();
    QFormLayout *vlistt = new QFormLayout;
    vlistt->setContentsMargins(0,0,0,0);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    vlistt->addRow("Tile X:",&this->tX);
    vlistt->addRow("Tile Z:",&this->tY);
    GuiFunct::alignEditorForm(vlistt);
    identityCard.second->addLayout(vlistt);
    vbox->addWidget(identityCard.first);

    addSubtitle("Texture");
    auto textureCard = makeCard();
    this->fileName.setDisabled(true);
    QFormLayout *textureForm = new QFormLayout;
    textureForm->setContentsMargins(0,0,0,0);
    textureForm->addRow("File:", &fileName);
    GuiFunct::alignEditorForm(textureForm);
    textureCard.second->addLayout(textureForm);
    QPushButton *copyF = new QPushButton("Copy FileName", this);
    GuiFunct::styleEditorActionButton(copyF);
    QObject::connect(copyF, SIGNAL(released()), this, SLOT(copyFileNameEnabled()));
    textureCard.second->addWidget(copyF);
    vbox->addWidget(textureCard.first);

    addSubtitle("Forest Region");
    auto regionCard = makeCard();
    QFormLayout *vlist = new QFormLayout;
    vlist->setContentsMargins(0,0,0,0);
    vlist->addRow("Width:",&this->sizeX);
    vlist->addRow("Height:",&this->sizeY);
    QDoubleValidator* doubleValidator = new QDoubleValidator(0, 1000, 2, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    sizeX.setValidator(doubleValidator);
    QObject::connect(&sizeX, SIGNAL(textEdited(QString)),
                      this, SLOT(sizeEnabled(QString)));
    sizeY.setValidator(doubleValidator);
    QObject::connect(&sizeY, SIGNAL(textEdited(QString)),
                      this, SLOT(sizeEnabled(QString)));
    
    vlist->addRow("Population:",&this->population);
    population.setValidator( new QIntValidator(0, 1000000, this) );
    QObject::connect(&population, SIGNAL(textEdited(QString)),
                      this, SLOT(populationEnabled(QString)));
    
    vlist->addRow("Density/KM:",&this->densitykm);
    densitykm.setValidator( new QIntValidator(0, 1000000, this) );
    QObject::connect(&densitykm, SIGNAL(textEdited(QString)),
                      this, SLOT(densitykmEnabled(QString)));
    GuiFunct::alignEditorForm(vlist);
    regionCard.second->addLayout(vlist);
    vbox->addWidget(regionCard.first);

    addSubtitle("Position & Rotation");
    auto positionCard = makeCard();
    vlist = new QFormLayout;
    vlist->setContentsMargins(0,0,0,0);
    vlist->addRow("X:",&this->posX);
    vlist->addRow("Y:",&this->posY);
    vlist->addRow("Z:",&this->posZ);
    this->quat.setDisabled(true);
    this->quat.setAlignment(Qt::AlignCenter);
    vlist->addRow("Rot:",&this->quat);
    GuiFunct::alignEditorForm(vlist);
    positionCard.second->addLayout(vlist);
    QGridLayout *posRotList = new QGridLayout;
    posRotList->setSpacing(2);
    posRotList->setContentsMargins(0,0,0,0);    

    QPushButton *copyPos = new QPushButton("Copy Pos", this);
    GuiFunct::styleEditorActionButton(copyPos);
    QObject::connect(copyPos, SIGNAL(released()),
                      this, SLOT(copyPEnabled()));
    QPushButton *pastePos = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pastePos);
    QObject::connect(pastePos, SIGNAL(released()),
                      this, SLOT(pastePEnabled()));
    QPushButton *copyQrot = new QPushButton("Copy Rot", this);
    GuiFunct::styleEditorActionButton(copyQrot);
    QObject::connect(copyQrot, SIGNAL(released()),
                      this, SLOT(copyREnabled()));
    QPushButton *pasteQrot = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pasteQrot);
    QObject::connect(pasteQrot, SIGNAL(released()),
                      this, SLOT(pasteREnabled()));
    QPushButton *copyPosRot = new QPushButton("Copy Pos+Rot", this);
    GuiFunct::styleEditorActionButton(copyPosRot);
    QObject::connect(copyPosRot, SIGNAL(released()),
                      this, SLOT(copyPREnabled()));
    QPushButton *pastePosRot = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pastePosRot);
    QObject::connect(pastePosRot, SIGNAL(released()),
                      this, SLOT(pastePREnabled()));
    QPushButton *resetQrot = new QPushButton("Reset Rot", this);
    GuiFunct::styleEditorActionButton(resetQrot);
    QObject::connect(resetQrot, SIGNAL(released()),
                      this, SLOT(resetRotEnabled()));
    QPushButton *qRot90 = new QPushButton("Rot Y 90°", this);
    GuiFunct::styleEditorActionButton(qRot90);
    QObject::connect(qRot90, SIGNAL(released()),
                      this, SLOT(rotYEnabled()));
    QPushButton *transform = new QPushButton("Transform...", this);
    configureTransformButton(transform);
    
    posRotList->addWidget(copyPos, 0, 0);
    posRotList->addWidget(pastePos, 0, 1);
    posRotList->addWidget(copyQrot, 1, 0);
    posRotList->addWidget(pasteQrot, 1, 1);
    posRotList->addWidget(copyPosRot, 2, 0);
    posRotList->addWidget(pastePosRot, 2, 1);
    posRotList->addWidget(resetQrot, 3, 0);
    posRotList->addWidget(qRot90, 3, 1);
    posRotList->addWidget(transform, 4, 0, 1, 2);
    positionCard.second->addLayout(posRotList);
    vbox->addWidget(positionCard.first);

    addSubtitle("Detail Level");
    auto detailCard = makeCard();
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
    detailCard.second->addLayout(detailLevelView);
    vbox->addWidget(detailCard.first);

    addSubtitle("Flags");
    auto flagsCard = makeCard();
    this->flags.setDisabled(true);
    this->flags.setAlignment(Qt::AlignCenter);
    flagsCard.second->addWidget(&this->flags);
    QGridLayout *flagslView = new QGridLayout;
    flagslView->setSpacing(2);
    flagslView->setContentsMargins(0,0,0,0);    
    QPushButton *copyFlags = new QPushButton("Copy Flags", this);
    GuiFunct::styleEditorActionButton(copyFlags);
    QObject::connect(copyFlags, SIGNAL(released()),
                      this, SLOT(copyFEnabled()));
    QPushButton *pasteFlags = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(pasteFlags);
    QObject::connect(pasteFlags, SIGNAL(released()),
                      this, SLOT(pasteFEnabled()));
    flagslView->addWidget(copyFlags,0,0);
    flagslView->addWidget(pasteFlags,0,1);
    flagsCard.second->addLayout(flagslView);
    vbox->addWidget(flagsCard.first);
    
    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesForest::~PropertiesForest() {
}

void PropertiesForest::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    worldObj = (WorldObj*)obj;
    forestObj = (ForestObj*)obj;
    ForestObj* tobj = (ForestObj*)obj;
        
    this->infoLabel->setText("Object: "+forestObj->type);
    this->fileName.setText(tobj->treeTexture);    /// Forest Texture
        
    this->tX.setText(QString::number(forestObj->x, 10));
    this->tY.setText(QString::number(-forestObj->y, 10));
    this->sizeX.setText(QString::number(tobj->areaX, 'G', 4));
    this->sizeY.setText(QString::number(tobj->areaZ, 'G', 4));
    this->population.setText(QString::number((int)tobj->population, 10));
    this->densitykm.setText(QString::number((int)(tobj->population*(1000000.0/(tobj->areaX*tobj->areaZ))), 10));
    this->posX.setText(QString::number(forestObj->position[0], 'G', 6));
    this->posY.setText(QString::number(forestObj->position[1], 'G', 6));
    this->posZ.setText(QString::number(-forestObj->position[2], 'G', 6));
    this->quat.setText(
            QString::number(forestObj->qDirection[0], 'G', 4) + " " +
            QString::number(forestObj->qDirection[1], 'G', 4) + " " +
            QString::number(-forestObj->qDirection[2], 'G', 4) + " " +
            QString::number(forestObj->qDirection[3], 'G', 4)
            );
}

void PropertiesForest::sizeEnabled(QString val){
    if(forestObj == NULL)
        return;
    bool ok;
    sizeX.text().toFloat(&ok);
    if(!ok) return;
    if(sizeX.text().toFloat() <= 0) return;
    sizeY.text().toFloat(&ok);
    if(!ok) return;
    if(sizeY.text().toFloat() <= 0) return;
    Undo::SinglePushWorldObjData(worldObj);
    forestObj->set("areaX", sizeX.text().toFloat());
    forestObj->set("areaZ", sizeY.text().toFloat());
    forestObj->setModified();
    forestObj->deleteVBO();
}

void PropertiesForest::populationEnabled(QString val){
    if(forestObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    forestObj->set("population", population.text().toLongLong());
    this->densitykm.setText(QString::number((int)(forestObj->population*(1000000.0/(forestObj->areaX*forestObj->areaZ))), 10));
    forestObj->setModified();
    forestObj->deleteVBO();
}

void PropertiesForest::densitykmEnabled(QString val){
    if(forestObj == NULL)
        return;
    Undo::SinglePushWorldObjData(worldObj);
    this->population.setText(QString::number((int)(densitykm.text().toUInt()/(1000000.0/(forestObj->areaX*forestObj->areaZ))), 10));
    forestObj->set("population", population.text().toLongLong());
    forestObj->setModified();
    forestObj->deleteVBO();
}

bool PropertiesForest::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj != GameObj::worldobj)
        return false;
    if(((WorldObj*)obj)->type == "forest")
        return true;
    return false;
}

void PropertiesForest::enableCustomDetailLevelEnabled(int val){
    if(worldObj == NULL)
        return;
    ForestObj* forestObj = (ForestObj*) worldObj;
    Undo::SinglePushWorldObjData(worldObj);
    if(val == 2){
        customDetailLevel.setEnabled(true);
        customDetailLevel.setText("0");
        forestObj->setCustomDetailLevel(0);
    } else {
        customDetailLevel.setEnabled(false);
        customDetailLevel.setText("");
        forestObj->setCustomDetailLevel(-1);
    }
}

void PropertiesForest::customDetailLevelEdited(QString val){
    if(worldObj == NULL)
        return;
    ForestObj* forestObj = (ForestObj*) worldObj;
    bool ok = false;
    int level = val.toInt(&ok);

    if(ok){
        Undo::SinglePushWorldObjData(worldObj);
        forestObj->setCustomDetailLevel(level);
    }
}

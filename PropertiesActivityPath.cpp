/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesActivityPath.h"
#include "Path.h"
#include "Game.h"
#include "GuiFunct.h"

PropertiesActivityPath::PropertiesActivityPath() {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(3);
    vbox->setContentsMargins(4,4,4,4);
    infoLabel = new QLabel("Path:");
    GuiFunct::styleEditorSubtitle(infoLabel);
    vbox->addWidget(infoLabel);

    QFrame *pathCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(pathCard);
    QFormLayout *pathForm = new QFormLayout(pathCard);
    pathForm->setSpacing(2);
    pathForm->setContentsMargins(6,4,6,4);
    pathForm->addRow("File:", &ePathFName);
    pathForm->addRow("Name:", &eName);
    pathForm->addRow("Start:", &ePathStart);
    pathForm->addRow("End:", &ePathEnd);
    GuiFunct::alignEditorForm(pathForm);
    vbox->addWidget(pathCard);

    QLabel *label = new QLabel(QString::fromUtf8("• Main Route Nodes"));
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);

    QFrame *nodesCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(nodesCard);
    QVBoxLayout *nodesLayout = new QVBoxLayout(nodesCard);
    nodesLayout->setContentsMargins(4,3,4,3);
    nodesLayout->addWidget(&nodeList);
    vbox->addWidget(nodesCard);

    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesActivityPath::~PropertiesActivityPath() {
}

void PropertiesActivityPath::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    pathObj = (Path*)obj;
    this->infoLabel->setText("Object: Path");
    this->ePathFName.setText(pathObj->trPathName);
    this->eName.setText(pathObj->displayName);
    this->ePathStart.setText(pathObj->trPathStart);
    this->ePathEnd.setText(pathObj->trPathEnd);
    
}

bool PropertiesActivityPath::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj == GameObj::activitypath)
        return true;
    return false;
}

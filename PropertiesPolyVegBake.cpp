/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesPolyVegBake.h"
#include "Game.h"
#include "GameObj.h"
#include "GroupObj.h"
#include "GuiFunct.h"
#include "PolyVegObject.h"
#include "WorldObj.h"

#include <QSet>

namespace {
bool isPolyVegBakeGroup(const GroupObj *group){
    if(group == NULL || group->objects.isEmpty())
        return false;
    for(const WorldObj *member : group->objects){
        if(member == NULL || !PolyVegObject::isBakeShape(member->fileName))
            return false;
    }
    return true;
}
}

PropertiesPolyVegBake::PropertiesPolyVegBake(){
    const int alignedLabelWidth =
        qRound(58.0f * qBound(0.75f, Game::uiScale, 1.25f));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0, 1, 1, 1);

    infoLabel = new QLabel("Object: PolyVeg - Bake");
    infoLabel->setStyleSheet(QString("QLabel { color : ") +
                             Game::StyleMainLabel +
                             "; font-weight: bold; }");
    infoLabel->setContentsMargins(3, 0, 0, 0);
    vbox->addWidget(infoLabel);

    QFrame *managedCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(managedCard);
    QVBoxLayout *managedLayout = new QVBoxLayout(managedCard);
    managedLayout->setContentsMargins(6, 4, 6, 4);
    managedText = new QLabel(
        "Generated PolyVeg bake block. Manage it through F6 PolyVeg Planter; "
        "rebake or remove it through the PolyVeg workflow.", managedCard);
    managedText->setWordWrap(true);
    managedLayout->addWidget(managedText);
    vbox->addWidget(managedCard);

    QLabel *identityLabel = new QLabel("Identity");
    GuiFunct::styleEditorSubtitle(identityLabel);
    identityLabel->setContentsMargins(3, 0, 0, 0);
    vbox->addWidget(identityLabel);

    uid.setDisabled(true);
    tX.setDisabled(true);
    tY.setDisabled(true);
    uid.setAlignment(Qt::AlignCenter);
    tX.setAlignment(Qt::AlignCenter);
    tY.setAlignment(Qt::AlignCenter);
    QFormLayout *identityForm = new QFormLayout;
    identityForm->setSpacing(2);
    identityForm->setContentsMargins(3, 0, 3, 0);
    identityForm->addRow("UiD:", &uid);
    identityForm->addRow("Tile X:", &tX);
    identityForm->addRow("Tile Z:", &tY);
    uidFieldLabel = qobject_cast<QLabel*>(identityForm->labelForField(&uid));
    tileXFieldLabel = qobject_cast<QLabel*>(identityForm->labelForField(&tX));
    tileZFieldLabel = qobject_cast<QLabel*>(identityForm->labelForField(&tY));
    uidFieldLabel->setMinimumWidth(alignedLabelWidth);
    vbox->addLayout(identityForm);

    shapeSectionLabel = new QLabel("Bake Shape");
    GuiFunct::styleEditorSubtitle(shapeSectionLabel);
    shapeSectionLabel->setContentsMargins(3, 0, 0, 0);
    vbox->addWidget(shapeSectionLabel);

    fileName.setDisabled(true);
    fileName.setAlignment(Qt::AlignCenter);
    QFormLayout *shapeForm = new QFormLayout;
    shapeForm->setSpacing(2);
    shapeForm->setContentsMargins(3, 0, 3, 0);
    shapeForm->addRow("File:", &fileName);
    fileFieldLabel = qobject_cast<QLabel*>(shapeForm->labelForField(&fileName));
    fileFieldLabel->setMinimumWidth(alignedLabelWidth);
    vbox->addLayout(shapeForm);

    locationSectionLabel = new QLabel("Location");
    GuiFunct::styleEditorSubtitle(locationSectionLabel);
    locationSectionLabel->setContentsMargins(3, 0, 0, 0);
    vbox->addWidget(locationSectionLabel);

    posX.setDisabled(true);
    posY.setDisabled(true);
    posZ.setDisabled(true);
    posX.setAlignment(Qt::AlignCenter);
    posY.setAlignment(Qt::AlignCenter);
    posZ.setAlignment(Qt::AlignCenter);
    QFormLayout *locationForm = new QFormLayout;
    locationForm->setSpacing(2);
    locationForm->setContentsMargins(3, 0, 3, 0);
    locationForm->addRow("X:", &posX);
    locationForm->addRow("Y:", &posY);
    locationForm->addRow("Z:", &posZ);
    positionXFieldLabel = qobject_cast<QLabel*>(locationForm->labelForField(&posX));
    positionYFieldLabel = qobject_cast<QLabel*>(locationForm->labelForField(&posY));
    positionZFieldLabel = qobject_cast<QLabel*>(locationForm->labelForField(&posZ));
    positionXFieldLabel->setMinimumWidth(alignedLabelWidth);
    vbox->addLayout(locationForm);

    QLabel *advancedLabel =
        new QLabel(QString(QChar(0x2022)) + " Advanced");
    GuiFunct::styleEditorSubtitle(advancedLabel);
    advancedLabel->setContentsMargins(3, 0, 0, 0);
    vbox->addWidget(advancedLabel);

    QFrame *advancedCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(advancedCard);
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedCard);
    advancedLayout->setContentsMargins(4, 3, 4, 3);
    hacks.setText("Hacks...");
    hacks.setCheckable(true);
    hacks.setProperty("editorPopupKey", "hacksHelper");
    GuiFunct::styleEditorActionButton(&hacks);
    hacks.setFocusPolicy(Qt::NoFocus);
    hacks.setToolTip("Open applicable repair and PolyVeg cleanup tools.");
    QObject::connect(&hacks, &QPushButton::clicked, this, [this](){
        emit userButtonPressed();
    });
    QObject::connect(&hacks, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&hacks, checked);
        emit hacksToggled(worldObj, &hacks, checked);
    });
    advancedLayout->addWidget(&hacks);
    vbox->addWidget(advancedCard);

    vbox->addStretch(1);
    setLayout(vbox);
}

PropertiesPolyVegBake::~PropertiesPolyVegBake(){
}

QPushButton *PropertiesPolyVegBake::hacksButton(){
    return &hacks;
}

bool PropertiesPolyVegBake::support(GameObj *obj){
    if(obj == NULL || obj->typeObj != GameObj::worldobj)
        return false;
    WorldObj *candidate = static_cast<WorldObj*>(obj);
    if(candidate->typeID == WorldObj::groupobject)
        return isPolyVegBakeGroup(static_cast<GroupObj*>(candidate));
    return PolyVegObject::isBakeShape(candidate->fileName);
}

void PropertiesPolyVegBake::showObj(GameObj *obj){
    if(!support(obj)){
        worldObj = NULL;
        return;
    }
    worldObj = static_cast<WorldObj*>(obj);
    refreshValues();
}

void PropertiesPolyVegBake::updateObj(GameObj *obj){
    if(!support(obj))
        return;
    worldObj = static_cast<WorldObj*>(obj);
    refreshValues();
}

void PropertiesPolyVegBake::refreshValues(){
    if(worldObj == NULL)
        return;

    const bool isGroup = worldObj->typeID == WorldObj::groupobject;
    uidFieldLabel->setText(isGroup ? "Blocks:" : "UiD:");
    shapeSectionLabel->setText(isGroup ? "Bake Shapes" : "Bake Shape");
    fileFieldLabel->setText(isGroup ? "Files:" : "File:");
    locationSectionLabel->setVisible(!isGroup);
    positionXFieldLabel->setVisible(!isGroup);
    positionYFieldLabel->setVisible(!isGroup);
    positionZFieldLabel->setVisible(!isGroup);
    posX.setVisible(!isGroup);
    posY.setVisible(!isGroup);
    posZ.setVisible(!isGroup);

    if(isGroup){
        GroupObj *group = static_cast<GroupObj*>(worldObj);
        QSet<QString> tiles;
        QSet<QString> shapes;
        for(const WorldObj *member : group->objects){
            tiles.insert(QString::number(member->x) + "," +
                         QString::number(-member->y));
            shapes.insert(member->fileName.toLower());
        }

        infoLabel->setText("Object: PolyVeg - Bake Group");
        managedText->setText(
            "Generated PolyVeg bake blocks selected as one tile group. Manage "
            "them through F6 PolyVeg Planter; rebake or remove them through "
            "the PolyVeg workflow.");
        uid.setText(QString::number(group->objects.size()));
        if(tiles.size() == 1 && !group->objects.isEmpty()){
            tX.setText(QString::number(group->objects.first()->x));
            tY.setText(QString::number(-group->objects.first()->y));
        } else {
            tX.setText(QString::number(tiles.size()) + " tiles");
            tY.setText("Multiple");
        }
        fileName.setText(QString::number(shapes.size()) + " generated shapes");
        return;
    }

    infoLabel->setText("Object: PolyVeg - Bake");
    managedText->setText(
        "Generated PolyVeg bake block. Manage it through F6 PolyVeg Planter; "
        "rebake or remove it through the PolyVeg workflow.");
    uid.setText(QString::number(worldObj->UiD, 10));
    tX.setText(QString::number(worldObj->x, 10));
    tY.setText(QString::number(-worldObj->y, 10));
    fileName.setText(worldObj->fileName);
    posX.setText(QString::number(worldObj->position[0], 'G', 6));
    posY.setText(QString::number(worldObj->position[1], 'G', 6));
    posZ.setText(QString::number(-worldObj->position[2], 'G', 6));
}

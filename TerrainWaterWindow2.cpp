/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TerrainWaterWindow2.h"
#include <QtWidgets>
#include "Terrain.h"
#include "GuiFunct.h"

TerrainWaterWindow2::TerrainWaterWindow2(QWidget* parent)
    : EditorPopupWindow(parent, "WATER HELPER", "waterHelper") {
    QVBoxLayout *mainLayout = popupLayout();
    addPopupSubtitle(QString::fromUtf8("• Corner Elevations"));
    QFrame *cornerCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(cornerCard);
    QVBoxLayout *cornerLayout = new QVBoxLayout(cornerCard);
    cornerLayout->setContentsMargins(4,3,4,3);
    cornerLayout->setSpacing(2);

    QLabel *help = new QLabel(
        "Edit the selected tile's four corner water elevations. "
        "Adjacent values are read-only until synchronized.");
    help->setWordWrap(true);
    help->setStyleSheet("QLabel { color: #c7c7c7; padding: 1px 3px 2px 3px; }");
    cornerLayout->addWidget(help);
    
    for(int i = 0; i < 12; i++){
        e[i].setFixedWidth(50);
        e[i].setDisabled(true);
    }
    eAvg.setFixedWidth(50);
    eNW.setFixedWidth(50);
    eNE.setFixedWidth(50);
    eSW.setFixedWidth(50);
    eSE.setFixedWidth(50);
    
    QGridLayout *vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(3,0,3,0);
    vlist1->addWidget(&e[0], 0, 0);
    vlist1->addWidget(&e[1], 0, 2);
    vlist1->addWidget(&e[2], 0, 4);
    vlist1->addWidget(&e[3], 0, 6);
    vlist1->addWidget(new QLabel("NW"), 1, 1);
    vlist1->addWidget(new QLabel("  ---------"), 1, 2);
    vlist1->addWidget(new QLabel("  ---------"), 1, 3);
    vlist1->addWidget(new QLabel("  ---------"), 1, 4);
    vlist1->addWidget(new QLabel("NE"), 1, 5);
    vlist1->addWidget(&e[4], 2, 0);
    vlist1->addWidget(new QLabel("|"), 2, 1);
    vlist1->addWidget(&eNW, 2, 2);
    vlist1->addWidget(new QLabel(" Average:"), 2, 3);
    vlist1->addWidget(&eNE, 2, 4);
    vlist1->addWidget(new QLabel("|"), 2, 5);
    vlist1->addWidget(&e[5], 2, 6);
    vlist1->addWidget(new QLabel("|"), 3, 1);
    vlist1->addWidget(&eAvg, 3, 3);
    //vlist1->addWidget(new QLabel("TILE"), 3, 3);
    vlist1->addWidget(new QLabel("|"), 3, 5);
    vlist1->addWidget(&e[6], 4, 0);
    vlist1->addWidget(new QLabel("|"), 4, 1);
    vlist1->addWidget(&eSW, 4, 2);
    vlist1->addWidget(&eSE, 4, 4);
    vlist1->addWidget(new QLabel("|"), 4, 5);
    vlist1->addWidget(&e[7], 4, 6);
    vlist1->addWidget(new QLabel("SW"), 5, 1);
    vlist1->addWidget(new QLabel("  ---------"), 5, 2);
    vlist1->addWidget(new QLabel("  ---------"), 5, 3);
    vlist1->addWidget(new QLabel("  ---------"), 5, 4);
    vlist1->addWidget(new QLabel("SE"), 5, 5);
    vlist1->addWidget(&e[8], 6, 0);
    vlist1->addWidget(&e[9], 6, 2);
    vlist1->addWidget(&e[10], 6, 4);
    vlist1->addWidget(&e[11], 6, 6);
    QPushButton *bAdjust = new QPushButton("Adjust Adjacent Tiles");
    GuiFunct::styleEditorActionButton(bAdjust);
    bAdjust->setToolTip("Copies this tile's corner water elevations to the adjoining tiles.");
    connect(bAdjust, SIGNAL (released()), this, SLOT (bAdjustEdited()));
    vlist1->addWidget(bAdjust, 7, 0, 1, 7);
    cornerLayout->addLayout(vlist1);
    mainLayout->addWidget(cornerCard);

    addPopupSubtitle(QString::fromUtf8("• Automatic Water"));
    QFrame *automaticCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(automaticCard);
    QVBoxLayout *automaticLayout = new QVBoxLayout(automaticCard);
    automaticLayout->setContentsMargins(4,3,4,3);
    automaticLayout->setSpacing(2);
    QLabel *autoHelp = new QLabel(
        "Trace the watercourse bottom with one blue ruler, then scan outward "
        "to raised terrain. The tile limit prevents the scan escaping "
        "into unrelated low ground.");
    autoHelp->setWordWrap(true);
    autoHelp->setStyleSheet("QLabel { color: #c7c7c7; padding: 1px 3px 2px 3px; }");
    automaticLayout->addWidget(autoHelp);

    waterHeight.setDecimals(2);
    waterHeight.setRange(0.01, 100.0);
    waterHeight.setSingleStep(0.25);
    waterHeight.setValue(0.25);
    waterHeight.setSuffix(" m");
    waterHeight.setToolTip("Water surface height above the traced watercourse bottom.");
    searchDistance.setRange(0, 50);
    searchDistance.setSingleStep(1);
    searchDistance.setValue(1);
    searchDistance.setSuffix(" tile(s)");
    searchDistance.setToolTip(
        "Number of terrain tiles to search outward from every tile crossed by the ruler. "
        "A value of 1 includes the ruler tiles plus one neighboring tile in every direction.");
    QFormLayout *autoForm = new QFormLayout;
    autoForm->setContentsMargins(3,0,3,0);
    autoForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    autoForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    autoForm->addRow("Above bed:", &waterHeight);
    autoForm->addRow("Scan distance:", &searchDistance);
    automaticLayout->addLayout(autoForm);

    QGridLayout *autoButtons = new QGridLayout;
    autoButtons->setContentsMargins(3,0,3,0);
    placeRulerButton = new QPushButton("Ruler (water)");
    placeRulerButton->setCheckable(true);
    QPushButton *processWaterTiles = new QPushButton("Process Water Tiles");
    QPushButton *undoScan = new QPushButton("Undo Last");
    GuiFunct::styleEditorActionButton(placeRulerButton);
    GuiFunct::styleEditorActionButton(processWaterTiles);
    GuiFunct::styleEditorActionButton(undoScan);
    placeRulerButton->setToolTip(
        "Toggle on to clear any previous ruler and place points along the watercourse bottom. "
        "Toggle off to remove the ruler and leave generated water in place.");
    processWaterTiles->setToolTip(
        "Uses the water ruler to load the required terrain and enable connected "
        "water patches.");
    undoScan->setToolTip("Restores the water flags and levels from before the last scan so parameters can be changed.");
    autoButtons->addWidget(placeRulerButton, 0, 0, 1, 2);
    autoButtons->addWidget(processWaterTiles, 1, 0, 1, 2);
    autoButtons->addWidget(undoScan, 2, 0, 1, 2);
    automaticLayout->addLayout(autoButtons);
    mainLayout->addWidget(automaticCard);

    connect(placeRulerButton, &QPushButton::clicked, this, [this](bool checked){
        if(checked)
            emit placeRulerRequested();
        else
            emit removeRulerRequested();
    });
    connect(processWaterTiles, &QPushButton::clicked, this, [this](){
        // Processing needs the completed ruler. Programmatically unchecking
        // the button does not emit clicked(), so it deactivates the placement
        // UI without requesting ruler removal.
        placeRulerButton->setChecked(false);
        emit scanRequested((float)waterHeight.value(), searchDistance.value());
    });
    connect(undoScan, &QPushButton::clicked, this, &TerrainWaterWindow2::undoScanRequested);
    const QList<QPushButton*> waterButtons = {
        placeRulerButton, processWaterTiles, undoScan
    };
    for(QPushButton *button : waterButtons){
        connect(button, &QPushButton::clicked, this, [this](){
            emit userButtonPressed();
        });
    }
    
    connect(&eAvg, SIGNAL (textEdited(QString)), this, SLOT (eAvgTextEdited(QString)));
    connect(&eSW, SIGNAL (textEdited(QString)), this, SLOT (eWaterEdited(QString)));
    connect(&eSE, SIGNAL (textEdited(QString)), this, SLOT (eWaterEdited(QString)));
    connect(&eNW, SIGNAL (textEdited(QString)), this, SLOT (eWaterEdited(QString)));
    connect(&eNE, SIGNAL (textEdited(QString)), this, SLOT (eWaterEdited(QString)));

    finalizePopup();
}

TerrainWaterWindow2::~TerrainWaterWindow2() {
}

void TerrainWaterWindow2::setTerrain(Terrain* t){
    if(t == NULL)
        return;
    if(t == terrain)
        return;
    
    terrain = t;
    eAvg.setText(QString::number(terrain->getAvgVaterLevel()));
    float waterLevels[4];
    terrain->getWaterLevels(waterLevels);
    eNW.setText(QString::number(waterLevels[0]));
    eNE.setText(QString::number(waterLevels[1]));
    eSW.setText(QString::number(waterLevels[2]));
    eSE.setText(QString::number(waterLevels[3]));
    terrain->getAdjacentWaterLevels(we);
    for(int i = 0; i < 12; i++)
        e[i].setText(QString::number(we[i]));
}

void TerrainWaterWindow2::eAvgTextEdited(QString val){
    if(terrain == NULL)
        return;
    bool ok = false;
    float f = val.toFloat(&ok);
    if(!ok)
        return;
    terrain->setAvgWaterLevel(f);
}

void TerrainWaterWindow2::eWaterEdited(QString){
    if(terrain == NULL)
        return;
    bool ok = false;
    float nw = eNW.text().toFloat(&ok);
    if(!ok)
        return;
    float ne = eNE.text().toFloat(&ok);
    if(!ok)
        return;    
    float sw = eSW.text().toFloat(&ok);
    if(!ok)
        return;    
    float se = eSE.text().toFloat(&ok);
    if(!ok)
        return;    
    terrain->setWaterLevel(nw, ne, sw, se);
}

void TerrainWaterWindow2::bAdjustEdited(){
    if(terrain == NULL)
        return;
    
    we[0] = eNW.text().toFloat();
    we[1] = eNW.text().toFloat();
    we[4] = eNW.text().toFloat();
    
    we[2] = eNE.text().toFloat();
    we[3] = eNE.text().toFloat();
    we[5] = eNE.text().toFloat();
    
    we[6] = eSW.text().toFloat();
    we[8] = eSW.text().toFloat();
    we[9] = eSW.text().toFloat();
    
    we[7] = eSE.text().toFloat();
    we[10] = eSE.text().toFloat();
    we[11] = eSE.text().toFloat();
    
    for(int i = 0; i < 12; i++)
        e[i].setText(QString::number(we[i]));
    
    terrain->setAdjacentWaterLevels(we);
}

void TerrainWaterWindow2::activateRuler(){
    if(placeRulerButton == NULL)
        return;

    // Mirror the control state and action without synthesizing clicked(). The
    // successful world placement supplies the normal placement sound once.
    if(!placeRulerButton->isChecked())
        placeRulerButton->setChecked(true);
    emit placeRulerRequested();
}

void TerrainWaterWindow2::deactivateRuler(){
    placeRulerButton->setChecked(false);
    emit removeRulerRequested();
}

void TerrainWaterWindow2::closeEvent(QCloseEvent *event){
    deactivateRuler();
    QWidget::closeEvent(event);
    emit helperClosed();
}

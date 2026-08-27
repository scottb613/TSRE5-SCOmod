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

#include "Game.h"
#include "GuiFunct.h"

TerrainWaterWindow2::TerrainWaterWindow2(QWidget *parent)
    : QWidget(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(5);

    QLabel *heading = new QLabel("WATER TOOLS", this);
    GuiFunct::styleEditorTitle(heading);
    layout->addWidget(heading);

    QLabel *rulerSubtitle = new QLabel(
        QString(QChar(0x2022)) + " Water Ruler", this);
    GuiFunct::styleEditorSubtitle(rulerSubtitle);
    layout->addWidget(rulerSubtitle);

    QFrame *rulerCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(rulerCard);
    QVBoxLayout *rulerLayout = new QVBoxLayout(rulerCard);
    rulerLayout->setContentsMargins(6, 5, 6, 5);
    rulerLayout->setSpacing(4);

    newRulerButton = new QPushButton("New Water Ruler", rulerCard);
    addPointsButton = new QPushButton("Add Points", rulerCard);
    editPointsButton = new QPushButton("Edit Points", rulerCard);
    const QList<QPushButton*> rulerButtons = {
        newRulerButton, addPointsButton, editPointsButton
    };
    modeButtons = new QButtonGroup(this);
    // Selection is managed explicitly so New Water Ruler can toggle off.
    modeButtons->setExclusive(false);
    for(QPushButton *button : rulerButtons){
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        GuiFunct::styleEditorActionButton(button);
        modeButtons->addButton(button);
    }
    newRulerButton->setToolTip(
        "Starts a new water ruler; click again to remove it.");
    addPointsButton->setToolTip(
        "Continues placing terrain-snapped points on the existing water ruler.");
    editPointsButton->setToolTip(
        "Switches to Select mode so existing water-ruler points can be moved.");
    rulerLayout->addWidget(newRulerButton);
    QGridLayout *pointModes = new QGridLayout;
    pointModes->setContentsMargins(0, 0, 0, 0);
    pointModes->setSpacing(4);
    pointModes->addWidget(addPointsButton, 0, 0);
    pointModes->addWidget(editPointsButton, 0, 1);
    pointModes->setColumnStretch(0, 1);
    pointModes->setColumnStretch(1, 1);
    rulerLayout->addLayout(pointModes);
    layout->addWidget(rulerCard);

    QLabel *processSubtitle = new QLabel(
        QString(QChar(0x2022)) + " Process Water", this);
    GuiFunct::styleEditorSubtitle(processSubtitle);
    layout->addWidget(processSubtitle);

    QFrame *processCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(processCard);
    QVBoxLayout *processLayout = new QVBoxLayout(processCard);
    processLayout->setContentsMargins(6, 5, 6, 5);
    processLayout->setSpacing(4);

    waterHeight = new QDoubleSpinBox(processCard);
    waterHeight->setDecimals(2);
    waterHeight->setRange(0.01, 100.0);
    waterHeight->setSingleStep(0.25);
    waterHeight->setValue(0.75);
    waterHeight->setSuffix(" m");
    waterHeight->setToolTip(
        "Sets water above the traced bed and the corrected terrain clearance below water.");
    searchDistance = new QSpinBox(processCard);
    searchDistance->setRange(0, 50);
    searchDistance->setValue(1);
    searchDistance->setSuffix(" tile(s)");
    searchDistance->setToolTip(
        "Terrain-tile radius searched from every tile crossed by the ruler.");
    QFormLayout *parameters = new QFormLayout;
    parameters->setContentsMargins(3, 0, 3, 0);
    parameters->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    parameters->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    parameters->addRow("Above bed:", waterHeight);
    parameters->addRow("Scan distance:", searchDistance);
    processLayout->addLayout(parameters);

    QPushButton *processWaterTiles = new QPushButton(
        "Process Water Tiles", processCard);
    QPushButton *adjustTerrain = new QPushButton(
        "Adjust Terrain", processCard);
    QPushButton *undoScan = new QPushButton("Undo Last", processCard);
    processWaterTiles->setFocusPolicy(Qt::NoFocus);
    adjustTerrain->setFocusPolicy(Qt::NoFocus);
    undoScan->setFocusPolicy(Qt::NoFocus);
    GuiFunct::styleEditorActionButton(processWaterTiles);
    GuiFunct::styleEditorActionButton(adjustTerrain);
    GuiFunct::styleEditorActionButton(undoScan);
    processWaterTiles->setToolTip(
        "Replaces the connected Water Tools result and reconciles its tile seams.");
    adjustTerrain->setToolTip(
        "Optionally lowers near-water terrain beneath the processed water using Above bed as clearance.");
    undoScan->setToolTip(
        "Restores the water or terrain state from before the last Water Tools operation.");
    processLayout->addWidget(processWaterTiles);
    processLayout->addWidget(adjustTerrain);
    processLayout->addWidget(undoScan);
    layout->addWidget(processCard);

    layout->addStretch(1);
    QLabel *statusSubtitle = new QLabel(
        QString(QChar(0x2022)) + " Status", this);
    GuiFunct::styleEditorSubtitle(statusSubtitle);
    layout->addWidget(statusSubtitle);
    QFrame *statusCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(statusCard);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(6, 5, 6, 5);
    statusLayout->setSpacing(4);
    messageCell = new QPlainTextEdit(statusCard);
    messageCell->setReadOnly(true);
    messageCell->setFocusPolicy(Qt::NoFocus);
    messageCell->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    messageCell->setStyleSheet(QString(
        "QPlainTextEdit { color: %1; }").arg(Game::StyleMainLabel));
    messageCell->setFixedHeight(
        messageCell->fontMetrics().lineSpacing() * 2 + 12);
    messageCell->setPlainText("Ready.");
    progress = new QProgressBar(statusCard);
    progress->setTextVisible(true);
    progress->hide();
    statusLayout->addWidget(messageCell);
    statusLayout->addWidget(progress);
    layout->addWidget(statusCard);

    connect(newRulerButton, &QPushButton::clicked, this, [this](bool checked){
        if(!checked){
            selectMode(nullptr);
            setStatus("Ready.");
            emit removeRulerRequested();
            return;
        }
        selectMode(newRulerButton);
        emit placeRulerRequested();
    });
    connect(addPointsButton, &QPushButton::clicked, this, [this](){
        selectMode(addPointsButton);
        emit addPointsRequested();
    });
    connect(editPointsButton, &QPushButton::clicked, this, [this](){
        selectMode(editPointsButton);
        emit editPointsRequested();
    });
    connect(processWaterTiles, &QPushButton::clicked, this, [this](){
        selectMode(nullptr);
        emit scanRequested(
            static_cast<float>(waterHeight->value()), searchDistance->value());
    });
    connect(adjustTerrain, &QPushButton::clicked, this, [this](){
        selectMode(nullptr);
        emit adjustTerrainRequested(
            static_cast<float>(waterHeight->value()), searchDistance->value());
    });
    connect(undoScan, &QPushButton::clicked,
            this, &TerrainWaterWindow2::undoScanRequested);
    const QList<QPushButton*> allButtons = {
        newRulerButton, addPointsButton, editPointsButton,
        processWaterTiles, adjustTerrain, undoScan
    };
    for(QPushButton *button : allButtons){
        connect(button, &QPushButton::clicked,
                this, &TerrainWaterWindow2::userButtonPressed);
    }
}

TerrainWaterWindow2::~TerrainWaterWindow2() {
}

void TerrainWaterWindow2::activateRuler(){
    selectMode(newRulerButton);
    emit placeRulerRequested();
}

void TerrainWaterWindow2::resetSession(){
    selectMode(nullptr);
    setStatus("Ready.");
}

void TerrainWaterWindow2::setStatus(const QString &text){
    messageCell->setPlainText(text);
    messageCell->verticalScrollBar()->setValue(0);
    progress->hide();
}

void TerrainWaterWindow2::setProgress(
        int value, int maximum, const QString &text){
    messageCell->setPlainText(text);
    messageCell->verticalScrollBar()->setValue(0);
    progress->setRange(0, maximum > 0 ? maximum : 0);
    if(maximum > 0)
        progress->setValue(qBound(0, value, maximum));
    progress->show();
}

void TerrainWaterWindow2::selectMode(QPushButton *button){
    for(QAbstractButton *candidate : modeButtons->buttons())
        candidate->setChecked(candidate == button);
}

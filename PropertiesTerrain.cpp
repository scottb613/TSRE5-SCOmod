/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesTerrain.h"
#include "Terrain.h"
#include "Game.h"
#include "TerrainWaterWindow2.h"
#include "GuiFunct.h"

class TerrainHeightHelperWindow : public EditorPopupWindow {
public:
    explicit TerrainHeightHelperWindow(PropertiesTerrain *owner)
        : EditorPopupWindow(owner, "HEIGHT HELPER", "heightHelper"),
          owner(owner) {
        QVBoxLayout *layout = popupLayout();
        addPopupSubtitle(QString::fromUtf8("• Flatten Entire Tile"));

        tileIdentifier.setReadOnly(true);
        tileIdentifier.setAlignment(Qt::AlignCenter);
        tileIdentifier.setFocusPolicy(Qt::NoFocus);
        QFormLayout *tileForm = new QFormLayout;
        tileForm->setContentsMargins(3,0,3,0);
        tileForm->addRow("Tile:", &tileIdentifier);
        layout->addLayout(tileForm);

        QLabel *warning = new QLabel(
            "Flattening replaces every terrain elevation in this tile. "
            "Adjoining tile edges may no longer meet; Undo is available.");
        warning->setWordWrap(true);
        warning->setStyleSheet("QLabel { color: #c7c7c7; padding: 1px 3px 2px 3px; }");
        layout->addWidget(warning);

        elevation.setDecimals(2);
        elevation.setRange(-10000.0, 10000.0);
        elevation.setSuffix(" m");
        elevation.setAlignment(Qt::AlignRight);
        elevation.setToolTip("The elevation that will replace every height sample in the selected tile.");
        QFormLayout *form = new QFormLayout;
        form->setContentsMargins(3,0,3,0);
        form->addRow("Elevation:", &elevation);
        layout->addLayout(form);

        QPushButton *apply = new QPushButton("Flatten Entire Tile");
        apply->setFocusPolicy(Qt::NoFocus);
        apply->setToolTip("Replaces the complete selected tile heightmap with this elevation.");
        layout->addWidget(apply);
        QObject::connect(apply, &QPushButton::clicked, this, [this](){
            this->owner->userButtonPressed();
            if(terrain == NULL)
                return;
            Undo::PushTerrainHeightMap(
                terrain->mojex, terrain->mojez,
                terrain->terrainData, terrain->getSampleCount());
            terrain->setFixedHeight(static_cast<float>(elevation.value()));
        });

        finalizePopup();
    }

    void showForTerrain(Terrain *selectedTerrain){
        setTerrain(selectedTerrain);
        if(terrain == NULL)
            return;
        show();
        raise();
        activateWindow();
        elevation.setFocus();
        elevation.selectAll();
    }

    void setTerrain(Terrain *selectedTerrain){
        if(selectedTerrain == NULL || selectedTerrain == terrain)
            return;
        terrain = selectedTerrain;
        tileIdentifier.setText(QString("%1, %2")
                               .arg(terrain->mojex)
                               .arg(-terrain->mojez));
        const int samples = terrain->getSampleCount();
        double total = 0.0;
        for(int x = 0; x < samples; ++x)
            for(int z = 0; z < samples; ++z)
                total += terrain->terrainData[x][z];
        if(samples > 0)
            elevation.setValue(total / static_cast<double>(samples * samples));
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        owner->heightHelperClosed();
    }

private:
    PropertiesTerrain *owner = NULL;
    Terrain *terrain = NULL;
    QLineEdit tileIdentifier;
    QDoubleSpinBox elevation;
};

PropertiesTerrain::PropertiesTerrain() {
    GuiFunct::applyEditorPanelStyle(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    
    infoLabel = new QLabel("Terrain:");
    infoLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    infoLabel->setContentsMargins(3,0,0,0);
    vbox->addWidget(infoLabel);
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    vlist->addRow("Tile X:",&this->tX);
    vlist->addRow("Tile Z:",&this->tY);
    this->tX.setDisabled(true);
    this->tY.setDisabled(true);
    vlist->addRow("Name:",&this->fileName);
    vbox->addItem(vlist);
    QLabel* label = new QLabel("Water Level:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Average:",&this->eAvgWater);
    QObject::connect(&eAvgWater, SIGNAL(textEdited(QString)),
                      this, SLOT(eAvgWaterEnabled(QString)));
    waterHelperButton.setText("Water Helper...");
    waterHelperButton.setCheckable(true);
    waterHelperButton.setFocusPolicy(Qt::NoFocus);
    QObject::connect(&waterHelperButton, SIGNAL(toggled(bool)),
                      this, SLOT(bWaterEditorEnabled()));
    vbox->addItem(vlist);
    vbox->addWidget(&waterHelperButton);
    
    label = new QLabel("Height Map:");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    heightHelperButton.setText("Height Helper...");
    heightHelperButton.setCheckable(true);
    heightHelperButton.setFocusPolicy(Qt::NoFocus);
    QObject::connect(&heightHelperButton, SIGNAL(toggled(bool)),
                      this, SLOT(bHeightMapResetEnabled()));
    vbox->addWidget(&heightHelperButton);
    
    label = new QLabel("Selected Terrain Patch(s):");
    label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Selected:",&this->tP);
    vlist->addRow("Shader ID:",&this->tS);
    vlist->addRow("Main Tex:",&this->tTex);
    this->tP.setDisabled(true);
    this->tS.setDisabled(true);
    vbox->addItem(vlist);
    QPushButton *bRemoveAllGaps = new QPushButton("Remove All Gaps", this);
    QObject::connect(bRemoveAllGaps, SIGNAL(released()),
                      this, SLOT(bRemoveAllGapsEnabled()));
    QPushButton *bShowWater = new QPushButton("Show", this);
    QObject::connect(bShowWater, SIGNAL(released()), this, SLOT(bShowWaterEnabled()));
    QPushButton *bShowDraw = new QPushButton("Show", this);
    QObject::connect(bShowDraw, SIGNAL(released()), this, SLOT(bShowDrawEnabled()));
    QPushButton *bHideWater = new QPushButton("Hide", this);
    QObject::connect(bHideWater, SIGNAL(released()), this, SLOT(bHideWaterEnabled()));
    QPushButton *bHideDraw = new QPushButton("Hide", this);
    QObject::connect(bHideDraw, SIGNAL(released()), this, SLOT(bHideDrawEnabled()));

    label = new QLabel("Visibility:");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    QGridLayout *vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(3,0,3,0);
    int row = 0;
    vlist1->addWidget(new QLabel("Water:"), row, 0);
    vlist1->addWidget(bShowWater, row, 1);
    vlist1->addWidget(bHideWater, row++, 2);
    vlist1->addWidget(new QLabel("Draw:"), row, 0);
    vlist1->addWidget(bShowDraw, row, 1);
    vlist1->addWidget(bHideDraw, row++, 2);
    vlist1->addWidget(new QLabel("Gaps:"), row, 0);
    vlist1->addWidget(bRemoveAllGaps, row++, 1, 1, 2);
    vbox->addItem(vlist1);
    
    label = new QLabel("Texture Transformation:");
    //label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(3,0,3,0);
    row = 0;
    QPushButton *bCopy = new QPushButton("Copy", this);
    QObject::connect(bCopy, SIGNAL(released()), this, SLOT(bCopyEnabled()));
    vlist1->addWidget(bCopy, row, 0);
    QPushButton *bPaste = new QPushButton("Paste", this);
    QObject::connect(bPaste, SIGNAL(released()), this, SLOT(bPasteEnabled()));
    vlist1->addWidget(bPaste, row++, 1);
    QPushButton *bMirrorX = new QPushButton("Mirror Y", this);
    QObject::connect(bMirrorX, SIGNAL(released()), this, SLOT(bMirrorXEnabled()));
    vlist1->addWidget(bMirrorX, row, 0);
    QPushButton *bMirrorY = new QPushButton("Mirror X", this);
    QObject::connect(bMirrorY, SIGNAL(released()), this, SLOT(bMirrorYEnabled()));
    vlist1->addWidget(bMirrorY, row++, 1);
    QPushButton *bRotate = new QPushButton("Rotate 90°", this);
    QObject::connect(bRotate, SIGNAL(released()), this, SLOT(bRotateEnabled()));
    vlist1->addWidget(bRotate, row, 0);
    //QPushButton *bScale = new QPushButton("Scale...", this);
    //QObject::connect(bScale, SIGNAL(released()), this, SLOT(bScaleEnabled()));
    //vlist1->addWidget(bScale, row, 1);
    QPushButton *bReset = new QPushButton("Reset", this);
    QObject::connect(bReset, SIGNAL(released()), this, SLOT(bResetEnabled()));
    vlist1->addWidget(bReset, row++, 1);
    vbox->addItem(vlist1);
    
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Scale XY", &eScalexy);
    eScalexy.setDecimals(2);
    eScalexy.setRange(0.1, 100.0);
    eScalexy.setSingleStep(1.0);
    QObject::connect(&eScalexy, SIGNAL(editingFinished()), this, SLOT(bScaleEnabled()));
    vlist->addRow("Scale X:", &eScalex);
    eScalex.setDecimals(2);
    eScalex.setRange(0.1, 100.0);
    eScalex.setSingleStep(1.0);
    QObject::connect(&eScalex, SIGNAL(editingFinished()), this, SLOT(bScaleXEnabled()));
    vlist->addRow("Scale Y:", &eScaley);
    eScaley.setDecimals(2);
    eScaley.setRange(0.1, 100.0);
    eScaley.setSingleStep(1.0);
    QObject::connect(&eScaley, SIGNAL(editingFinished()), this, SLOT(bScaleYEnabled()));
    vlist->addRow("Rotation:", &eRotation);
    eRotation.setDisabled(true);
    vbox->addItem(vlist);
    
    label = new QLabel("MSTS Settings:");
    //label->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label->setContentsMargins(3,0,0,0);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Error Bias:",&eBias);
    QObject::connect(&eBias, SIGNAL(textEdited(QString)),
                      this, SLOT(eBiasEnabled(QString)));
    vbox->addItem(vlist);

    QLabel *advancedLabel =
        new QLabel(QString(QChar(0x2022)) + " Advanced");
    GuiFunct::styleEditorSubtitle(advancedLabel);
    vbox->addWidget(advancedLabel);
    hacks.setText("Hacks...");
    hacks.setCheckable(true);
    hacks.setFocusPolicy(Qt::NoFocus);
    hacks.setToolTip("Open specialized repair and full-route cleanup tools.");
    QObject::connect(&hacks, &QPushButton::clicked, this, [this](){
        emit userButtonPressed();
    });
    QObject::connect(&hacks, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&hacks, checked);
        emit hacksToggled(terrainObj, &hacks, checked);
    });
    vbox->addWidget(&hacks);

    vbox->addStretch(1);
    this->setLayout(vbox);
}

PropertiesTerrain::~PropertiesTerrain() {
}

QPushButton *PropertiesTerrain::hacksButton(){
    return &hacks;
}

void PropertiesTerrain::showObj(GameObj* obj){
    if(obj == NULL){
        infoLabel->setText("NULL");
        return;
    }
    terrainObj = (Terrain*)obj;
    
    this->infoLabel->setText("Object: Terrain");

    this->tX.setText(QString::number(terrainObj->mojex));
    this->tY.setText(QString::number(-terrainObj->mojez));
    this->fileName.setText(terrainObj->getTileName());
    this->tP.setText(QString::number(terrainObj->getSelectedPathId()));
    this->tS.setText(QString::number(terrainObj->getSelectedShaderId()));
    this->tTex.setText(terrainObj->getPatchMainTextureName());
    this->tTex.setCursorPosition(0);
    this->eBias.setText(QString::number(terrainObj->getErrorBias()));
    this->eAvgWater.setText(QString::number(terrainObj->getAvgVaterLevel()));
    this->eScalexy.setValue(terrainObj->getPatchScaleTex());
    this->eScalex.setValue(terrainObj->getPatchScaleTexX());
    this->eScaley.setValue(terrainObj->getPatchScaleTexY());
    this->eRotation.setText(terrainObj->getPatchRotationName());
    
    if(waterWindow != NULL)
        waterWindow->setTerrain(terrainObj);
    if(heightWindow != NULL && heightWindow->isVisible())
        heightWindow->setTerrain(terrainObj);
}

void PropertiesTerrain::updateObj(GameObj* obj){
    if(obj == NULL){
        return;
    }
    terrainObj = (Terrain*)obj;
    if(!tP.hasFocus() && !eBias.hasFocus() && !tTex.hasFocus()){
        this->tP.setText(QString::number(terrainObj->getSelectedPathId()));
        this->tS.setText(QString::number(terrainObj->getSelectedShaderId()));
        this->eBias.setText(QString::number(terrainObj->getErrorBias()));
        this->tTex.setText(terrainObj->getPatchMainTextureName());
        this->tTex.setCursorPosition(0);
    }
    if(!eScalexy.hasFocus())
        eScalexy.setValue(terrainObj->getPatchScaleTex());
    if(!eScalex.hasFocus())
        eScalex.setValue(terrainObj->getPatchScaleTexX());
    if(!eScaley.hasFocus())
        eScaley.setValue(terrainObj->getPatchScaleTexY());
    
    this->eRotation.setText(terrainObj->getPatchRotationName());
    
    if(waterWindow != NULL)
        waterWindow->setTerrain(terrainObj);
    if(heightWindow != NULL && heightWindow->isVisible())
        heightWindow->setTerrain(terrainObj);
}

bool PropertiesTerrain::support(GameObj* obj){
    if(obj == NULL)
        return false;
    if(obj->typeObj == GameObj::terrainobj)
        return true;
    return false;
}

void PropertiesTerrain::bWaterEditorEnabled(){
    if(terrainObj == NULL){
        waterHelperButton.setChecked(false);
        return;
    }
    GuiFunct::setEditorPopupButtonActive(
        &waterHelperButton, waterHelperButton.isChecked());
    if(waterWindow == NULL){
        waterWindow = new TerrainWaterWindow2(window());
        QObject::connect(waterWindow, SIGNAL(helperClosed()),
                         this, SLOT(waterHelperClosed()));
        QObject::connect(waterWindow, &TerrainWaterWindow2::userButtonPressed,
                         this, [this](){ emit userButtonPressed(); });
        QObject::connect(waterWindow, &TerrainWaterWindow2::placeRulerRequested,
                         this, &PropertiesTerrain::placeWaterRulerRequested);
        QObject::connect(waterWindow, &TerrainWaterWindow2::scanRequested,
                         this, &PropertiesTerrain::scanWaterRulerRequested);
        QObject::connect(waterWindow, &TerrainWaterWindow2::undoScanRequested,
                         this, &PropertiesTerrain::undoWaterScanRequested);
        QObject::connect(waterWindow, &TerrainWaterWindow2::removeRulerRequested,
                         this, &PropertiesTerrain::removeWaterRulerRequested);
    }
    if(waterHelperButton.isChecked()){
        waterWindow->setTerrain(terrainObj);
        waterWindow->show();
        waterWindow->raise();
        waterWindow->activateWindow();
    } else {
        waterWindow->deactivateRuler();
        waterWindow->hide();
    }
}

void PropertiesTerrain::bHeightMapResetEnabled(){
    if(terrainObj == NULL){
        heightHelperButton.setChecked(false);
        return;
    }
    GuiFunct::setEditorPopupButtonActive(
        &heightHelperButton, heightHelperButton.isChecked());
    if(heightWindow == NULL)
        heightWindow = new TerrainHeightHelperWindow(this);
    if(heightHelperButton.isChecked()){
        heightWindow->showForTerrain(terrainObj);
    } else {
        heightWindow->hide();
    }
}

void PropertiesTerrain::waterHelperClosed(){
    waterHelperButton.blockSignals(true);
    waterHelperButton.setChecked(false);
    waterHelperButton.blockSignals(false);
}

void PropertiesTerrain::heightHelperClosed(){
    heightHelperButton.blockSignals(true);
    heightHelperButton.setChecked(false);
    heightHelperButton.blockSignals(false);
}

void PropertiesTerrain::bRotateEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->rotatePatchTexture();
}

void PropertiesTerrain::bMirrorXEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->mirrorXPatchTexture();
}

void PropertiesTerrain::bMirrorYEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->mirrorYPatchTexture();
}

void PropertiesTerrain::bScaleEnabled(){
    qDebug() << "a";
    if(terrainObj == NULL){
        return;
    }
    terrainObj->scalePatchTexCoords(eScalexy.value());
}

void PropertiesTerrain::bScaleXEnabled(){
    qDebug() << "ax";
    if(terrainObj == NULL){
        return;
    }
    terrainObj->scalePatchTexCoordsX(eScalex.value());
}

void PropertiesTerrain::bScaleYEnabled(){
    qDebug() << "ay";
    if(terrainObj == NULL){
        return;
    }
    terrainObj->scalePatchTexCoordsY(eScaley.value());
}

void PropertiesTerrain::bCopyEnabled(){
    if(terrainObj == NULL){
        return;
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(terrainObj->getPatchTexTransformString());
}

void PropertiesTerrain::bPasteEnabled(){
    if(terrainObj == NULL){
        return;
    }
    QClipboard *clipboard = QApplication::clipboard();
    terrainObj->setPatchTexTransform(clipboard->text());
}

void PropertiesTerrain::bResetEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->resetPatchTexCoords();
}

void PropertiesTerrain::bRemoveAllGapsEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->removeAllGaps();
}

void PropertiesTerrain::bShowWaterEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->setWaterDraw();
}

void PropertiesTerrain::bShowDrawEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->setDraw();
}

void PropertiesTerrain::bHideWaterEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->hideWaterDraw();
}

void PropertiesTerrain::bHideDrawEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->hideDraw();
}

void PropertiesTerrain::bShowAdjacentEnabled(){
    if(terrainObj == NULL){
        return;
    }
    terrainObj->setDrawAdjacent();
}

void PropertiesTerrain::eBiasEnabled(QString val){
    if(terrainObj == NULL){
        return;
    }
    bool ok = false;
    float fval = val.toFloat(&ok);
    if(!ok) return;
    terrainObj->setErrorBias(fval);
}

void PropertiesTerrain::eAvgWaterEnabled(QString val){
    if(terrainObj == NULL){
        return;
    }
    bool ok = false;
    float fval = val.toFloat(&ok);
    if(!ok) return;
    terrainObj->setAvgWaterLevel(fval);
}

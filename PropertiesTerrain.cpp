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
#include "GuiFunct.h"

namespace {

QString terrainMeshResolutionText(Terrain *terrain){
    if(terrain == NULL)
        return QStringLiteral("Unknown");

    const int sampleSize = terrain->getSampleSize();
    if(sampleSize <= 0)
        return QStringLiteral("Unknown");

    const QString spacing = QString::number(sampleSize) + QStringLiteral("m ");
    if(!terrain->lowTile){
        if(sampleSize == 8)
            return spacing + QStringLiteral("Normal");
        if(sampleSize == 4)
            return spacing + QStringLiteral("High Def");
        return spacing + QStringLiteral("Detailed");
    }

    const int samples = terrain->getSampleCount();
    if(samples == 64)
        return spacing + QStringLiteral("Mosaic");
    if(samples == 256)
        return spacing + QStringLiteral("TSRE");
    return spacing + QStringLiteral("DM");
}

} // namespace

class TerrainHeightHelperWindow : public EditorPopupWindow {
public:
    explicit TerrainHeightHelperWindow(PropertiesTerrain *owner)
        : EditorPopupWindow(owner, "HEIGHT HELPER", "heightHelper"),
          owner(owner) {
        QVBoxLayout *layout = popupLayout();
        addPopupSubtitle(QString::fromUtf8("• Flatten Entire Tile"));

        QFrame *flattenCard = new QFrame(this);
        GuiFunct::styleEditorPanelCard(flattenCard);
        QVBoxLayout *flattenLayout = new QVBoxLayout(flattenCard);
        flattenLayout->setContentsMargins(4,3,4,3);
        flattenLayout->setSpacing(3);

        tileIdentifier.setReadOnly(true);
        tileIdentifier.setAlignment(Qt::AlignCenter);
        tileIdentifier.setFocusPolicy(Qt::NoFocus);

        elevation.setDecimals(2);
        elevation.setRange(-10000.0, 10000.0);
        elevation.setSuffix(" m");
        elevation.setAlignment(Qt::AlignRight);
        elevation.setToolTip("The elevation that will replace every height sample in the selected tile.");

        QGridLayout *fields = new QGridLayout;
        fields->setContentsMargins(3,0,3,0);
        fields->setHorizontalSpacing(4);
        fields->setVerticalSpacing(2);
        fields->addWidget(new QLabel("Tile:"), 0, 0);
        fields->addWidget(&tileIdentifier, 0, 1);
        fields->addWidget(new QLabel("Elev:"), 0, 2);
        fields->addWidget(&elevation, 0, 3);
        fields->setColumnStretch(1, 1);
        fields->setColumnStretch(3, 1);
        flattenLayout->addLayout(fields);

        QLabel *warning = new QLabel(
            "Flattening replaces every terrain elevation in this tile. "
            "Adjoining tile edges may no longer meet; Undo is available.");
        warning->setWordWrap(true);
        warning->setStyleSheet("QLabel { color: #c7c7c7; padding: 1px 3px 2px 3px; }");
        flattenLayout->addWidget(warning);

        QPushButton *apply = new QPushButton("Flatten Entire Tile");
        apply->setFocusPolicy(Qt::NoFocus);
        apply->setToolTip("Replaces the complete selected tile heightmap with this elevation.");
        GuiFunct::styleEditorActionButton(apply);
        flattenLayout->addWidget(apply);
        layout->addWidget(flattenCard);
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
        showExclusive();
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
    const int alignedLabelWidth = qRound(
        70.0f * qBound(0.75f, Game::uiScale, 1.25f));
    const auto alignValueColumn = [alignedLabelWidth](QFormLayout *form){
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        for(int row = 0; row < form->rowCount(); ++row){
            QLayoutItem *labelItem = form->itemAt(row, QFormLayout::LabelRole);
            if(labelItem != NULL && labelItem->widget() != NULL)
                labelItem->widget()->setFixedWidth(alignedLabelWidth);
            QLayoutItem *fieldItem = form->itemAt(row, QFormLayout::FieldRole);
            if(fieldItem != NULL && fieldItem->widget() != NULL){
                QWidget *field = fieldItem->widget();
                field->setSizePolicy(QSizePolicy::Expanding,
                                     field->sizePolicy().verticalPolicy());
            }
        }
    };
    
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
    this->fileName.setReadOnly(true);
    this->eObjectCount.setDisabled(true);
    vlist->addRow("Obj #:",&this->eObjectCount);
    this->eMeshResolution.setReadOnly(true);
    this->eMeshResolution.setFocusPolicy(Qt::NoFocus);
    vlist->addRow("Mesh Res:",&this->eMeshResolution);
    alignValueColumn(vlist);
    QFrame *identityCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(identityCard);
    identityCard->setLayout(vlist);
    vbox->addWidget(identityCard);
    QLabel* label = new QLabel(QString(QChar(0x2022)) + " Water Level");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Average:",&this->eAvgWater);
    alignValueColumn(vlist);
    QObject::connect(&eAvgWater, SIGNAL(textEdited(QString)),
                      this, SLOT(eAvgWaterEnabled(QString)));
    QFrame *waterCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(waterCard);
    QVBoxLayout *waterCardLayout = new QVBoxLayout(waterCard);
    waterCardLayout->setContentsMargins(4,3,4,3);
    waterCardLayout->setSpacing(2);
    waterCardLayout->addLayout(vlist);
    vbox->addWidget(waterCard);
    
    label = new QLabel(QString(QChar(0x2022)) + " Tile Elevation");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    heightHelperButton.setText("Set Tile Elevation...");
    heightHelperButton.setCheckable(true);
    heightHelperButton.setProperty("editorPopupKey", "heightHelper");
    heightHelperButton.setFocusPolicy(Qt::NoFocus);
    GuiFunct::styleEditorActionButton(&heightHelperButton);
    QObject::connect(&heightHelperButton, SIGNAL(toggled(bool)),
                      this, SLOT(bHeightMapResetEnabled()));
    QFrame *heightCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(heightCard);
    QVBoxLayout *heightCardLayout = new QVBoxLayout(heightCard);
    heightCardLayout->setContentsMargins(4,3,4,3);
    heightCardLayout->addWidget(&heightHelperButton);
    vbox->addWidget(heightCard);
    
    label = new QLabel(QString(QChar(0x2022)) + " Selected Patch");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Selected:",&this->tP);
    vlist->addRow("Shader ID:",&this->tS);
    vlist->addRow("Main Tex:",&this->tTex);
    this->tTex.setReadOnly(true);
    alignValueColumn(vlist);
    this->tP.setDisabled(true);
    this->tS.setDisabled(true);
    QFrame *patchCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(patchCard);
    patchCard->setLayout(vlist);
    vbox->addWidget(patchCard);
    QPushButton *bRemoveAllGaps = new QPushButton("Remove All Gaps", this);
    GuiFunct::styleEditorActionButton(bRemoveAllGaps);
    QObject::connect(bRemoveAllGaps, SIGNAL(released()),
                      this, SLOT(bRemoveAllGapsEnabled()));
    waterVisibilityButton.setText("Water");
    drawVisibilityButton.setText("Draw");
    waterVisibilityButton.setCheckable(true);
    drawVisibilityButton.setCheckable(true);
    waterVisibilityButton.setFocusPolicy(Qt::NoFocus);
    drawVisibilityButton.setFocusPolicy(Qt::NoFocus);
    GuiFunct::styleEditorActionButton(&waterVisibilityButton);
    GuiFunct::styleEditorActionButton(&drawVisibilityButton);
    QObject::connect(&waterVisibilityButton, &QPushButton::toggled,
                     this, &PropertiesTerrain::bWaterVisibilityToggled);
    QObject::connect(&drawVisibilityButton, &QPushButton::toggled,
                     this, &PropertiesTerrain::bDrawVisibilityToggled);

    label = new QLabel(QString(QChar(0x2022)) + " Visibility");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    QFrame *visibilityCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(visibilityCard);
    QHBoxLayout *visibilityLayout = new QHBoxLayout(visibilityCard);
    visibilityLayout->setContentsMargins(4,3,4,3);
    visibilityLayout->setSpacing(2);
    visibilityLayout->addWidget(&waterVisibilityButton);
    visibilityLayout->addWidget(&drawVisibilityButton);
    vbox->addWidget(visibilityCard);

    label = new QLabel(QString(QChar(0x2022)) + " Patch Actions");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    QFrame *patchActionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(patchActionCard);
    QVBoxLayout *patchActionLayout = new QVBoxLayout(patchActionCard);
    patchActionLayout->setContentsMargins(4,3,4,3);
    patchActionLayout->addWidget(bRemoveAllGaps);
    vbox->addWidget(patchActionCard);
    
    label = new QLabel(QString(QChar(0x2022)) + " Texture Transform");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    QFrame *transformCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(transformCard);
    QVBoxLayout *transformLayout = new QVBoxLayout(transformCard);
    transformLayout->setContentsMargins(4,3,4,3);
    transformLayout->setSpacing(2);
    QGridLayout *vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(0,0,0,0);
    int row = 0;
    QPushButton *bCopy = new QPushButton("Copy", this);
    GuiFunct::styleEditorActionButton(bCopy);
    QObject::connect(bCopy, SIGNAL(released()), this, SLOT(bCopyEnabled()));
    vlist1->addWidget(bCopy, row, 0);
    QPushButton *bPaste = new QPushButton("Paste", this);
    GuiFunct::styleEditorActionButton(bPaste);
    QObject::connect(bPaste, SIGNAL(released()), this, SLOT(bPasteEnabled()));
    vlist1->addWidget(bPaste, row++, 1);
    QPushButton *bMirrorX = new QPushButton("Mirror Y", this);
    GuiFunct::styleEditorActionButton(bMirrorX);
    QObject::connect(bMirrorX, SIGNAL(released()), this, SLOT(bMirrorXEnabled()));
    vlist1->addWidget(bMirrorX, row, 0);
    QPushButton *bMirrorY = new QPushButton("Mirror X", this);
    GuiFunct::styleEditorActionButton(bMirrorY);
    QObject::connect(bMirrorY, SIGNAL(released()), this, SLOT(bMirrorYEnabled()));
    vlist1->addWidget(bMirrorY, row++, 1);
    QPushButton *bRotate = new QPushButton("Rotate 90°", this);
    GuiFunct::styleEditorActionButton(bRotate);
    QObject::connect(bRotate, SIGNAL(released()), this, SLOT(bRotateEnabled()));
    vlist1->addWidget(bRotate, row, 0);
    //QPushButton *bScale = new QPushButton("Scale...", this);
    //QObject::connect(bScale, SIGNAL(released()), this, SLOT(bScaleEnabled()));
    //vlist1->addWidget(bScale, row, 1);
    QPushButton *bReset = new QPushButton("Reset", this);
    GuiFunct::styleEditorActionButton(bReset);
    QObject::connect(bReset, SIGNAL(released()), this, SLOT(bResetEnabled()));
    vlist1->addWidget(bReset, row++, 1);
    transformLayout->addLayout(vlist1);
    
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Scale XY:", &eScalexy);
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
    alignValueColumn(vlist);
    transformLayout->addLayout(vlist);
    vbox->addWidget(transformCard);
    
    label = new QLabel(QString(QChar(0x2022)) + " Terrain Compatibility");
    GuiFunct::styleEditorSubtitle(label);
    vbox->addWidget(label);
    vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Error Bias:",&eBias);
    alignValueColumn(vlist);
    QObject::connect(&eBias, SIGNAL(textEdited(QString)),
                      this, SLOT(eBiasEnabled(QString)));
    QFrame *compatibilityCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(compatibilityCard);
    compatibilityCard->setLayout(vlist);
    vbox->addWidget(compatibilityCard);

    QLabel *advancedLabel =
        new QLabel(QString(QChar(0x2022)) + " Advanced");
    GuiFunct::styleEditorSubtitle(advancedLabel);
    vbox->addWidget(advancedLabel);
    hacks.setText("Hacks...");
    hacks.setCheckable(true);
    hacks.setProperty("editorPopupKey", "hacksHelper");
    hacks.setFocusPolicy(Qt::NoFocus);
    hacks.setToolTip("Open specialized repair and full-route cleanup tools.");
    QObject::connect(&hacks, &QPushButton::clicked, this, [this](){
        emit userButtonPressed();
    });
    QObject::connect(&hacks, &QPushButton::toggled, this, [this](bool checked){
        GuiFunct::setEditorPopupButtonActive(&hacks, checked);
        emit hacksToggled(terrainObj, &hacks, checked);
    });
    GuiFunct::styleEditorActionButton(&hacks);
    QFrame *advancedCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(advancedCard);
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedCard);
    advancedLayout->setContentsMargins(4,3,4,3);
    advancedLayout->addWidget(&hacks);
    vbox->addWidget(advancedCard);

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
    this->eObjectCount.setText(QString::number(currentObjectCount));
    this->eMeshResolution.setText(terrainMeshResolutionText(terrainObj));
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
    syncVisibilityButtons();
    
    if(heightWindow != NULL && heightWindow->isVisible())
        heightWindow->setTerrain(terrainObj);
}

void PropertiesTerrain::naviInfo(int all, int hidden){
    Q_UNUSED(hidden);
    currentObjectCount = all;
}

void PropertiesTerrain::updateObj(GameObj* obj){
    if(obj == NULL){
        return;
    }
    terrainObj = (Terrain*)obj;
    this->eMeshResolution.setText(terrainMeshResolutionText(terrainObj));
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
    syncVisibilityButtons();
    
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
        heightWindow->close();
    }
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

void PropertiesTerrain::bWaterVisibilityToggled(bool checked){
    if(terrainObj == NULL)
        return;
    if(checked)
        terrainObj->setWaterDraw();
    else
        terrainObj->hideWaterDraw();
}

void PropertiesTerrain::bDrawVisibilityToggled(bool checked){
    if(terrainObj == NULL)
        return;
    if(checked)
        terrainObj->setDraw();
    else
        terrainObj->hideDraw();
}

void PropertiesTerrain::syncVisibilityButtons(){
    if(terrainObj == NULL)
        return;
    const bool waterVisible = terrainObj->selectedPatchesWaterVisible();
    const bool drawVisible = terrainObj->selectedPatchesDrawVisible();
    waterVisibilityButton.blockSignals(true);
    drawVisibilityButton.blockSignals(true);
    waterVisibilityButton.setChecked(waterVisible);
    drawVisibilityButton.setChecked(drawVisible);
    waterVisibilityButton.blockSignals(false);
    drawVisibilityButton.blockSignals(false);
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

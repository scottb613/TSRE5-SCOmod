/*
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TerrainTools.h"
#include "TexLib.h"
#include "Brush.h"
#include "Texture.h"
#include "GuiFunct.h"
#include "TransferObj.h"
#include "ClickableLabel.h"
#include "Game.h"
#include "ShapeLib.h"
#include "Terrain.h"
#include "ForestObj.h"
#include "PolyForestObj.h"
#include "TFile.h"
#include "TerrainLib.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

static int scaledUiSize(int base){
    return qRound(base * qMax(1.0f, Game::uiScale));
}

TerrainTools::TerrainTools(QString name)
    : QWidget(){
    setFixedWidth(scaledUiSize(250));
    int row = 0;
    
    texPreview = new QPixmap(192,192);
    defaultTexPreview = new QPixmap(64,64);
    defaultTexPreview->fill(Qt::transparent);
    texPreview->fill(Qt::gray);
    texPreviewLabel = new ClickableLabel("");
    texPreviewLabel->setContentsMargins(0,0,0,0);
    texPreviewLabel->setPixmap(*texPreview);
    for(int i = 0; i < 7; i++){
        texPreviewLabels.push_back(new ClickableLabel(""));
        texPreviewLabels.back()->setContentsMargins(0,0,0,0);
        texPreviewLabels.back()->setPixmap(*defaultTexPreview);
        texPreviewSignals.setMapping(texPreviewLabels.back(), i);
        connect(texPreviewLabels.back(), SIGNAL(clicked()), &texPreviewSignals, SLOT(map()));
    }
    texPreviewSignals.setMapping(texPreviewLabel, 7);
    connect(texPreviewLabel, SIGNAL(clicked()), &texPreviewSignals, SLOT(map()));
    connect(&texPreviewSignals, SIGNAL(mapped(int)), this, SLOT(texPreviewEnabled(int)));

    paintBrush = new Brush();
    
    if(Game::terrBrushColor)        
    {
        paintBrush->color[0] = Game::terrBrushColor->red();
        paintBrush->color[1] = Game::terrBrushColor->green();
        paintBrush->color[2] = Game::terrBrushColor->blue();
    }
    else Game::terrBrushColor = new QColor("#000000");
        
              

    
    QDir dir(QString("tsre_appdata/")+Game::AppDataVersion+"/brush/");
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.png");
    foreach(QString bfile, dir.entryList())
        brushShapes.push_back(QImage(QString("tsre_appdata/")+Game::AppDataVersion+"/brush/"+bfile).convertToFormat(QImage::Format_Grayscale8));
    nextBrushShape();
    
    buttonTools["heightTool"] = new QPushButton("HeightMap +", this);
    buttonTools["pickTerrainTexTool"] = new QPushButton("Pick", this);
    buttonTools["putTerrainTexTool"] = new QPushButton("Put", this);
    buttonTools["waterTerrTool"] = new QPushButton("Water +", this);
    //buttonTools["drawTerrTool"] = new QPushButton("Show/H Tile", this);
    buttonTools["gapsTerrainTool"] = new QPushButton("Gaps +", this);
    //buttonTools["waterHeightTileTool"] = new QPushButton("Water level", this);
    //buttonTools["fixedTileTool"] = new QPushButton("Fixed Height", this);
    //buttonTools["waTileTool"] = new QPushButton("Fixed Height", this);
    if(Game::serverClient == NULL){
        buttonTools["paintToolColor"] = new QPushButton("Color", this);
        buttonTools["paintToolTexture"] = new QPushButton("Texture", this);
        buttonTools["lockTexTool"] = new QPushButton("Lock", this);
    }
    QMapIterator<QString, QPushButton*> i(buttonTools);
    while (i.hasNext()) {
        i.next();
        i.value()->setCheckable(true);
    }
    
    QPushButton *loadTerrainTexTool = new QPushButton("Load...", this);
    
    QGridLayout *vlist3 = new QGridLayout;
    vlist3->setSpacing(2);
    vlist3->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist3->addWidget(buttonTools["heightTool"],row,0);
    vlist3->addWidget(buttonTools["waterTerrTool"],row,1);
    vlist3->addWidget(buttonTools["gapsTerrainTool"],row++,2);
    //vlist3->addWidget(buttonTools["waterHeightTileTool"],row,2);
    //vlist3->addWidget(mapTileShowTool,row,0);
    //vlist3->addWidget(mapTileLoadTool,row,1);
    //vlist3->addWidget(heightTileLoadTool,row++,2);
    
    /*QGridLayout *vlist4 = new QGridLayout;
    vlist4->setSpacing(2);
    vlist4->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist4->addWidget(buttonTools["waterTerrTool"],row,0);
    vlist4->addWidget(buttonTools["drawTerrTool"],row,1);
    vlist4->addWidget(buttonTools["gapsTerrainTool"],row++,2);*/
    
    QGridLayout *vlist0 = new QGridLayout;
    vlist0->setSpacing(2);
    vlist0->setContentsMargins(3,0,1,0);    
    row = 0;
    mirrorSeason = new QPushButton("Mirror Season", this);
    mirrorSeason->setCheckable(true);
    vlist0->addWidget(buttonTools["paintToolColor"],row,0);
    vlist0->addWidget(buttonTools["paintToolTexture"],row,1);
    vlist0->addWidget(buttonTools["lockTexTool"],row++,2);
    
    QGridLayout *vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist1->addWidget(buttonTools["pickTerrainTexTool"],row,0);
    vlist1->addWidget(buttonTools["putTerrainTexTool"],row,1);
    vlist1->addWidget(loadTerrainTexTool,row,2);
    
    colorw = new QPushButton(Game::terrBrushColor->name(), this);
    colorw->setStyleSheet("background-color:" + Game::terrBrushColor->name() + ";");

    
    QLabel *label0;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(0,1,1,1);
    
    QLabel *textureSetLabel = new QLabel("Texture Set:");
    textureSetLabel->setContentsMargins(3,0,0,0);
    textureSetLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(textureSetLabel);

    seasonType = new QComboBox;
    seasonType->setStyleSheet("combobox-popup: 0;");
    seasonType->addItem("Summer");
    seasonType->addItem("Spring");
    seasonType->addItem("Autumn");
    seasonType->addItem("Winter");
    seasonType->addItem("Night");
    vbox->addWidget(seasonType);

    label0 = new QLabel("Edit Terrain Layers:");
    label0->setContentsMargins(3,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist3);
    /*label0 = new QLabel("Terrain Patch:");
    label0->setContentsMargins(3,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist4);*/
    if(Game::serverClient == NULL){
        label0 = new QLabel("Paint Texture:");
        label0->setContentsMargins(3,0,0,0);
        label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
        vbox->addWidget(label0);
        vbox->addItem(vlist0);
    }

    label0 = new QLabel("Texture:");
    label0->setContentsMargins(3,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist1);

    vlist1 = new QGridLayout;
    vlist1->setSpacing(0);
    vlist1->setContentsMargins(0,0,0,0);    
    vlist1->addWidget(texPreviewLabel, 0, 0, 3, 3);
    vlist1->addWidget(texPreviewLabels[0], 0, 3);
    vlist1->addWidget(texPreviewLabels[1], 1, 3);
    vlist1->addWidget(texPreviewLabels[2], 2, 3);
    vlist1->addWidget(texPreviewLabels[3], 3, 2);
    vlist1->addWidget(texPreviewLabels[4], 3, 1);
    vlist1->addWidget(texPreviewLabels[5], 3, 0);
    vlist1->addWidget(texPreviewLabels[6], 3, 3);    
    vbox->addItem(vlist1);
    vbox->setAlignment(vlist1, Qt::AlignHCenter);
    //vbox->addWidget(texPreviewLabel);
    //vbox->setAlignment(texPreviewLabel, Qt::AlignHCenter);

    QLabel *presetLabel = new QLabel("Presets:");
    presetLabel->setContentsMargins(3,0,0,0);
    presetLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(presetLabel);

    presetCombo = new QComboBox;
    presetApply = new QPushButton("Apply", this);
    presetSave = new QPushButton("Save", this);
    presetRemove = new QPushButton("Remove", this);

    QGridLayout *vlistPreset = new QGridLayout;
    vlistPreset->setSpacing(2);
    vlistPreset->setContentsMargins(3,0,1,0);
    vlistPreset->addWidget(presetCombo,0,0,1,3);
    vlistPreset->addWidget(presetApply,1,0);
    vlistPreset->addWidget(presetSave,1,1);
    vlistPreset->addWidget(presetRemove,1,2);
    vbox->addItem(vlistPreset);

    QLabel *label2 = new QLabel("Brush settings:");
    label2->setContentsMargins(3,0,0,0);
    label2->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(label2);
    

    int labelWidth = 70;
    
    // brush
    sSize = new QSlider(Qt::Horizontal);
    sSize->setMinimum(1);
    sSize->setMaximum(100);
    sSize->setValue(paintBrush->size);
    sIntensity = new QSlider(Qt::Horizontal);
    sIntensity->setMinimum(1);
    sIntensity->setMaximum(100);
    sIntensity->setValue(paintBrush->alpha*100);
    sTextureRotation = new QSlider(Qt::Horizontal);
    sTextureRotation->setMinimum(0);
    sTextureRotation->setMaximum(360);
    sTextureRotation->setValue(paintBrush->texRotationDegrees);

    hType = new QComboBox;
    hType->setStyleSheet("combobox-popup: 0;");
    hType->addItem("Add - simple");
    hType->addItem("Add - if inside 'Size' radius");
    hType->addItem("Fixed Height");
    hType->addItem("Flatten");
    hType->addItem("Conform DB");
    hType->setCurrentIndex(paintBrush->hType);
    fheight = new QLineEdit();
    QDoubleValidator* doubleValidator = new QDoubleValidator(-5000, 5000, 2, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    fheight->setValidator(doubleValidator);
    
    QGridLayout *vlist = new QGridLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,1,0);    
    leSize = GuiFunct::newQLineEdit(25,3);
    leSize->setValidator(new QIntValidator(1, 100, this));
    leIntensity = GuiFunct::newQLineEdit(25,3);
    leIntensity->setValidator(new QIntValidator(1, 100, this));
    leTextureRotation = GuiFunct::newQLineEdit(25,3);
    leTextureRotation->setValidator(new QIntValidator(0, 360, this));
    leTextureRotation->setText(QString::number(paintBrush->texRotationDegrees, 10));
    row = 0;
    vlist->addWidget(GuiFunct::newQLabel("Terrtex:", labelWidth),row,0);
    vlist->addWidget(mirrorSeason,row++,1,1,2);
    vlist->addWidget(GuiFunct::newQLabel("Color:", labelWidth),row,0);
    vlist->addWidget(colorw,row++,1,1,2);
    vlist->addWidget(GuiFunct::newQLabel("Size:", labelWidth),row,0);
    vlist->addWidget(leSize,row,1);
    vlist->addWidget(sSize,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Intensity:", labelWidth),row,0);
    vlist->addWidget(leIntensity,row,1);
    vlist->addWidget(sIntensity,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Rotation:", labelWidth),row,0);
    vlist->addWidget(leTextureRotation,row,1);
    vlist->addWidget(sTextureRotation,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Fixed Height:", labelWidth),row,0);
    vlist->addWidget(fheight,row++,1,1,2);
    vlist->addWidget(GuiFunct::newQLabel("Height type:", labelWidth),row,0);
    vlist->addWidget(hType,row++,1,1,2);
    

    vbox->addItem(vlist);
    
    // enbankment
    sEsize = new QSlider(Qt::Horizontal);
    sEsize->setMinimum(1);
    sEsize->setMaximum(3);
    sEsize->setValue(paintBrush->eSize);
    sEemb = new QSlider(Qt::Horizontal);
    sEemb->setMinimum(1);
    sEemb->setMaximum(10);
    sEemb->setValue(paintBrush->eEmb);
    sEcut = new QSlider(Qt::Horizontal);
    sEcut->setMinimum(1);
    sEcut->setMaximum(10);
    sEcut->setValue(paintBrush->eCut);
    sEradius = new QSlider(Qt::Horizontal);
    sEradius->setMinimum(1);
    sEradius->setMaximum(100);
    sEradius->setValue(paintBrush->eRadius);
    leEsize = GuiFunct::newQLineEdit(25,3);
    leEsize->setValidator(new QIntValidator(1, 3, this));
    leEemb = GuiFunct::newQLineEdit(25,3);
    leEemb->setValidator(new QIntValidator(1, 10, this));
    leEcut = GuiFunct::newQLineEdit(25,3);
    leEcut->setValidator(new QIntValidator(1, 10, this));
    leEradius = GuiFunct::newQLineEdit(25,3);
    leEradius->setValidator(new QIntValidator(1, 100, this));
    sun1 = GuiFunct::newQLineEdit(25,3);
    sun2 = GuiFunct::newQLineEdit(25,3);       
    sun3 = GuiFunct::newQLineEdit(25,3);    
    resetDefaults = new QPushButton("Reset Defaults", this);
    resetRouteTerrtex = new QPushButton("Reset Route Terrtex Paint", this);
    setPinPoint = new QPushButton("Set Pinpoint", this);    
    
    QLabel *label3 = new QLabel("Embankment settings:");
    label3->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    label3->setContentsMargins(3,0,0,0);
    vbox->addWidget(label3);

    QGridLayout *vlist2 = new QGridLayout;
    vlist2->setSpacing(2);
    vlist2->setContentsMargins(3,0,1,0);
    row = 0;
    vlist2->addWidget(GuiFunct::newQLabel("Size:", labelWidth),row,0);
    vlist2->addWidget(leEsize,row,1);
    vlist2->addWidget(sEsize,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Embankment:", labelWidth),row,0);
    vlist2->addWidget(leEemb,row,1);
    vlist2->addWidget(sEemb,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Cutting:", labelWidth),row,0);
    vlist2->addWidget(leEcut,row,1);
    vlist2->addWidget(sEcut,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Max Radius:", labelWidth),row,0);
    vlist2->addWidget(leEradius,row,1);
    vlist2->addWidget(sEradius,row++,2);
    
    vlist2->addWidget(setPinPoint,row,0);        
    vlist2->addWidget(resetDefaults,row++,1,1,2);    
    vlist2->addWidget(resetRouteTerrtex,row++,0,1,3);
    
    /*
    
    vlist2->addWidget(GuiFunct::newQLabel("Sky 1:", labelWidth),row,0);
    vlist2->addWidget(sun1,row++,1);
    vlist2->addWidget(GuiFunct::newQLabel("Sky 2:", labelWidth),row,0);
    vlist2->addWidget(sun2,row++,1);
    vlist2->addWidget(GuiFunct::newQLabel("Sky 3:", labelWidth),row,0);
    vlist2->addWidget(sun3,row++,1);
    
     */
    
    
    vbox->addItem(vlist2);
    
    vbox->addStretch(1);
    this->setLayout(vbox);
    
    
    // signals
    QObject::connect(buttonTools["heightTool"], SIGNAL(toggled(bool)),
                      this, SLOT(heightToolEnabled(bool)));
    if(Game::serverClient == NULL){
        QObject::connect(buttonTools["paintToolColor"], SIGNAL(toggled(bool)),
                          this, SLOT(paintColorToolEnabled(bool)));

        QObject::connect(buttonTools["paintToolTexture"], SIGNAL(toggled(bool)),
                          this, SLOT(paintTexToolEnabled(bool)));

        QObject::connect(buttonTools["lockTexTool"], SIGNAL(toggled(bool)),
                          this, SLOT(lockTexToolEnabled(bool)));
    }
    QObject::connect(buttonTools["pickTerrainTexTool"], SIGNAL(toggled(bool)),
                      this, SLOT(pickTexToolEnabled(bool)));
    
    QObject::connect(buttonTools["waterTerrTool"], SIGNAL(toggled(bool)),
                      this, SLOT(waterTerrToolEnabled(bool)));
    
    //QObject::connect(buttonTools["waterHeightTileTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(waterHeightTileToolEnabled(bool)));
    
    //QObject::connect(buttonTools["fixedTileTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(fixedTileToolEnabled(bool)));
    
    QObject::connect(buttonTools["gapsTerrainTool"], SIGNAL(toggled(bool)),
                      this, SLOT(gapsTerrToolEnabled(bool)));
    
    //QObject::connect(buttonTools["drawTerrTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(drawTerrToolEnabled(bool)));
    
    QObject::connect(buttonTools["putTerrainTexTool"], SIGNAL(toggled(bool)),
                      this, SLOT(putTexToolEnabled(bool)));
    
    QObject::connect(loadTerrainTexTool, SIGNAL(released()),
                      this, SLOT(setTexToolEnabled()));
    
    QObject::connect(colorw, SIGNAL(released()),
                      this, SLOT(chooseColorEnabled()));

    QObject::connect(mirrorSeason, SIGNAL(toggled(bool)),
                      this, SLOT(mirrorSeasonEnabled(bool)));
    
    QObject::connect(resetDefaults, SIGNAL(released()),
                      this, SLOT(resetDefaultValues()));

    QObject::connect(resetRouteTerrtex, SIGNAL(released()),
                      this, SLOT(resetRouteTerrtexPaint()));

    QObject::connect(setPinPoint, SIGNAL(released()),
                      this, SLOT(setPinPointBrush()));

    QObject::connect(presetApply, SIGNAL(released()),
                      this, SLOT(applyPaintPreset()));

    QObject::connect(presetSave, SIGNAL(released()),
                      this, SLOT(savePaintPreset()));

    QObject::connect(presetRemove, SIGNAL(released()),
                      this, SLOT(removePaintPreset()));

    // brush
    QObject::connect(sSize, SIGNAL(valueChanged(int)),
                      this, SLOT(setBrushSize(int)));
    
    QObject::connect(sIntensity, SIGNAL(valueChanged(int)),
                      this, SLOT(setBrushAlpha(int)));

    QObject::connect(sTextureRotation, SIGNAL(valueChanged(int)),
                      this, SLOT(setTextureRotation(int)));

    QObject::connect(leSize, SIGNAL(textEdited(QString)),
                      this, SLOT(setBrushSize(QString)));
    
    QObject::connect(leIntensity, SIGNAL(textEdited(QString)),
                      this, SLOT(setBrushAlpha(QString)));

    QObject::connect(leTextureRotation, SIGNAL(textEdited(QString)),
                      this, SLOT(setTextureRotation(QString)));
    
    // embarkment
    QObject::connect(sEsize, SIGNAL(valueChanged(int)),
                      this, SLOT(setEsize(int)));
    
    QObject::connect(leEsize, SIGNAL(textEdited(QString)),
                      this, SLOT(setEsize(QString)));    
    
    QObject::connect(sEemb, SIGNAL(valueChanged(int)),
                      this, SLOT(setEemb(int)));

    QObject::connect(leEemb, SIGNAL(textEdited(QString)),
                      this, SLOT(setEemb(QString)));

    QObject::connect(sEcut, SIGNAL(valueChanged(int)),
                      this, SLOT(setEcut(int)));

    QObject::connect(leEcut, SIGNAL(textEdited(QString)),
                      this, SLOT(setEcut(QString)));
    
    QObject::connect(sEradius, SIGNAL(valueChanged(int)),
                      this, SLOT(setEradius(int)));

    QObject::connect(leEradius, SIGNAL(textEdited(QString)),
                      this, SLOT(setEradius(QString)));
    
    QObject::connect(sun1, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun1(QString)));    

    QObject::connect(sun2, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun2(QString)));    
    
    QObject::connect(sun3, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun3(QString)));    

    
    QObject::connect(fheight, SIGNAL(textEdited(QString)),
                      this, SLOT(setFheight(QString)));
    
    QObject::connect(hType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(setHtype(int)));

    QObject::connect(seasonType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(setSeasonType(int)));
    
    this->setBrushSize(this->sSize->value());
    this->setBrushAlpha(this->sIntensity->value());
    this->setEsize(this->sEsize->value());
    this->setEemb(this->sEemb->value());
    this->setEcut(this->sEcut->value());
    this->setEradius(this->sEradius->value());
    this->fheight->setText("0");
    
    // EFO pre-populate terrainTools values from Settings

    if(Game::terrainTools != NULL)
    {
      this->setEsize(Game::terrainTools[0]);
      this->setEemb(Game::terrainTools[1]);
      this->setEcut(Game::terrainTools[2]);
      this->setEradius(Game::terrainTools[3]);
      this->setBrushSize(Game::terrainTools[4]);
      this->setBrushAlpha(Game::terrainTools[5]); 
      
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      sEsize->setValue(paintBrush->eSize);
      sEemb->setValue(paintBrush->eEmb);
      sEcut->setValue(paintBrush->eCut);
      sEradius->setValue(paintBrush->eRadius);                  
    }
    else
    {
      Game::terrainTools[0] = 1;
      Game::terrainTools[1] = 5;
      Game::terrainTools[2] = 5;
      Game::terrainTools[3] = 9;
      Game::terrainTools[4] = 1;
      Game::terrainTools[5] = 10; 
    }
    refreshPaintPresets();
    
}


TerrainTools::~TerrainTools() {
}

void TerrainTools::nextBrushShape(){
    currentBrushShape++;
    if(currentBrushShape > brushShapes.size()-1)
        currentBrushShape = 0;
    setBrushShapeIndex(currentBrushShape);
}

void TerrainTools::setBrushShapeIndex(int brushIndex){
    if(brushShapes.size() <= 0)
        return;
    if(brushIndex < 0 || brushIndex > brushShapes.size()-1)
        return;
    currentBrushShape = brushIndex;
    paintBrush->brushshape = &brushShapes[currentBrushShape];
    texPreviewLabels[6]->setPixmap(QPixmap::fromImage(*paintBrush->brushshape));
}

void TerrainTools::heightToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("heightTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::paintColorToolEnabled(bool val){
     if(val){
        this->paintBrush->useTexture = false;
        emit setPaintBrush(this->paintBrush);
        emit enableTool("paintToolColor");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::gapsTerrToolEnabled(bool val){
    if(val){
        emit enableTool("gapsTerrainTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::paintTexToolEnabled(bool val){
    if(val){
        this->paintBrush->useTexture = true;
        emit setPaintBrush(this->paintBrush);
        emit enableTool("paintToolTexture");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::mirrorSeasonEnabled(bool val){
    this->paintBrush->mirrorSeason = val;
    emit setPaintBrush(this->paintBrush);
}

void TerrainTools::chooseColorEnabled(){
    QColor aColor(paintBrush->color[0], paintBrush->color[1], paintBrush->color[2]);
    QColor color = QColorDialog::getColor(aColor, this, "Text Color",  QColorDialog::DontUseNativeDialog);
    paintBrush->color[0] = color.red();
    paintBrush->color[1] = color.green();
    paintBrush->color[2] = color.blue();
    colorw->setStyleSheet("background-color:"+color.name()+";");
    colorw->setText(color.name());
}

void TerrainTools::pickTexToolEnabled(bool val){
    if(val){
        emit enableTool("pickTerrainTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::lockTexToolEnabled(bool val){
    if(val){
        emit enableTool("lockTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::waterTerrToolEnabled(bool val){
    if(val){
        emit enableTool("waterTerrTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::drawTerrToolEnabled(bool val){
    if(val){
        emit enableTool("drawTerrTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::waterHeightTileToolEnabled(bool val){
    if(val){
        emit enableTool("waterHeightTileTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::putTexToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("putTerrainTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::fixedTileToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("fixedTileTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::preloadTextures(){
    if(Game::debugOutput) qDebug() << "Preloading" ; 
    if(Game::preloadTextures.size() > 0) 
    {
        foreach (QString val, Game::preloadTextures)
        {        
            preloadTexTool(val);        
        }                
    }
    QTime cTime = QTime::currentTime().addMSecs(300);  
    while (QTime::currentTime() < cTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    texPreviewEnabled(0);
}

void TerrainTools::preloadTexTool(QString filename)
{
    filename = Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),
            Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season), filename);
    if(Game::debugOutput) qDebug() << "preloading " << filename;
    int result = TexLib::getTex(filename);     
    if(result == -1)
        {    
            int tid = TexLib::addTex(filename);
            this->paintBrush->texId = tid;
            this->paintBrush->tex = TexLib::mtex[tid];

            texLastItems.push_back(qMakePair(this->paintBrush->texId, this->paintBrush->tex));
            if(texLastItems.size() > 7){
                texLastItems.removeFirst();
            }
        }
}


void TerrainTools::setTexToolEnabled(){
    QFileDialog fd;
    QStringList terrainSubdirs = Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season);
    QString path = Terrain::routeTerrtexPath(terrainSubdirs.isEmpty() ? "" : terrainSubdirs[0]);
    path.replace("//", "/");
    fd.setDirectory(path);
    fd.setFileMode(QFileDialog::ExistingFiles);
    //QTreeView *tree = fd->findChild <QTreeView*>();
    //tree->setRootIsDecorated(true);
    //tree->setItemsExpandable(true);
    //fd->setFileMode(QFileDialog::F);
    //fd->setOption(QFileDialog::ShowDirsOnly);
    //fd->setViewMode(QFileDialog::Detail);
    int result = fd.exec();
    QString filename;
    if (!result) return;
    
    for(int i = 0; i < fd.selectedFiles().length(); i++){
        filename = fd.selectedFiles()[i];
        TexLib::addTex(filename);
    }
    
    QTime cTime = QTime::currentTime().addMSecs(300);  
    while (QTime::currentTime() < cTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    
    for(int i = 0; i < fd.selectedFiles().length(); i++){
        filename = fd.selectedFiles()[i];
        qDebug()<<"texture file "<<filename;

        int tid = TexLib::addTex(filename);
        this->paintBrush->texId = tid;
        this->paintBrush->tex = TexLib::mtex[tid];

        texLastItems.push_back(qMakePair(this->paintBrush->texId, this->paintBrush->tex));
        if(texLastItems.size() > 7){
            texLastItems.removeFirst();
        }
        updateTexPrev();
    //emit enableTool("setTexTool");
    }
    emit setPaintBrush(this->paintBrush);
    
    //QTimer::singleShot(200, this, SLOT(updateTexPrev()));
}

// brush

void TerrainTools::setBrushSize(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sSize->setValue(ival);
    this->paintBrush->size = ival;
}

void TerrainTools::setBrushAlpha(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sIntensity->setValue(ival);
    this->paintBrush->alpha = (float)ival/100;
}

void TerrainTools::setBrushSize(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leSize->setText(QString::number(val,10));
    this->paintBrush->size = val;
}

void TerrainTools::setBrushAlpha(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leIntensity->setText(QString::number(val,10));
    this->paintBrush->alpha = (float)val/100;
}

void TerrainTools::setTextureRotation(QString val){
    emit setPaintBrush(this->paintBrush);
    int ival = val.toInt(0, 10);
    if(ival < 0) ival = 0;
    if(ival > 360) ival = 360;
    this->sTextureRotation->setValue(ival);
    this->paintBrush->texRotationDegrees = ival;
    this->paintBrush->texTransformation = ival == 0 ? Brush::ROT0 : Brush::CUSTOM;
}

void TerrainTools::setTextureRotation(int val){
    emit setPaintBrush(this->paintBrush);
    if(val < 0) val = 0;
    if(val > 360) val = 360;
    this->leTextureRotation->setText(QString::number(val,10));
    this->paintBrush->texRotationDegrees = val;
    this->paintBrush->texTransformation = val == 0 ? Brush::ROT0 : Brush::CUSTOM;
}

void TerrainTools::setFheight(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    float ival = val.toFloat(0);
    this->paintBrush->hFixed = ival;
}

void TerrainTools::setHtype(int val){
    emit setPaintBrush(this->paintBrush);
    if(val < 0) return;
    this->paintBrush->hType = val;
}

void TerrainTools::setSeasonType(int val){
    if(val == 1) {
        Game::season = "Spring";
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    } else if(val == 2) {
        Game::season = "Autumn";
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    } else if(val == 3) {
        Game::season = "Winter";
        Game::sunLightDirection[0] = 2;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 2;
    } else if(val == 4) {
        Game::season = "Night";
        Game::sunLightDirection[0] = -10;
        Game::sunLightDirection[1] = -10;
        Game::sunLightDirection[2] = -10;
    } else {
        Game::season = "Summer";
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    }

    if (Game::terrainLib != NULL)
        Game::terrainLib->reloadLoaded();
    if (Game::currentShapeLib != NULL)
        Game::currentShapeLib->refreshSeasonTextures();
    if (Game::currentRoute != NULL)
        Game::currentRoute->reloadLoadedWorldObjects();
    ForestObj::InvalidateSeasonTextures();
    PolyForestObj::InvalidateSeasonTextures();

    emit setPaintBrush(this->paintBrush);
}




// embarkment

void TerrainTools::setEsize(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEsize->setText(QString::number(val,10));
    this->paintBrush->eSize = val;
}
void TerrainTools::setEsize(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEsize->setValue(ival);
    this->paintBrush->eSize = (float)ival/100;
}
void TerrainTools::setEemb(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEemb->setText(QString::number(val,10));
    this->paintBrush->eEmb = val;
}
void TerrainTools::setEemb(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEemb->setValue(ival);
    this->paintBrush->eEmb = (float)ival/100;
}
void TerrainTools::setEcut(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEcut->setText(QString::number(val,10));
    this->paintBrush->eCut = val;
}
void TerrainTools::setEcut(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEcut->setValue(ival);
    this->paintBrush->eCut = (float)ival/100;
}
void TerrainTools::setEradius(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEradius->setText(QString::number(val,10));
    this->paintBrush->eRadius = val;
}
void TerrainTools::setEradius(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEradius->setValue(ival);
    this->paintBrush->eRadius = (float)ival/100;
}

void TerrainTools::setSun1(QString val){
        Game::skyColor[0] = val.toFloat();
}

void TerrainTools::setSun2(QString val){
        Game::skyColor[1] = val.toFloat();
}

void TerrainTools::setSun3(QString val){
        Game::skyColor[2] = val.toFloat();
}



//

void TerrainTools::setBrushTextureId(int val){
    emit setPaintBrush(this->paintBrush);
    if(val < 0) return;
    if(TexLib::mtex[val] == NULL) return;
    this->paintBrush->texId = val;
    this->paintBrush->tex = TexLib::mtex[val];
    
    texLastItems.push_back(qMakePair(this->paintBrush->texId, this->paintBrush->tex));
    if(texLastItems.size() > 6){
        texLastItems.removeFirst();
    }
    updateTexPrev();
}

QString TerrainTools::paintPresetFilePath()
{
    return Game::terrainPaintPresetFilePath();
}

QJsonArray TerrainTools::readPaintPresets()
{
    QFile file(paintPresetFilePath());
    if (!file.exists())
        return QJsonArray();
    if (!file.open(QIODevice::ReadOnly))
        return QJsonArray();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return QJsonArray();

    return doc.object()["presets"].toArray();
}

bool TerrainTools::writePaintPresets(const QJsonArray &presets)
{
    QFile file(paintPresetFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QJsonObject root;
    root["version"] = 1;
    root["presets"] = presets;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void TerrainTools::refreshPaintPresets()
{
    if (presetCombo == NULL)
        return;

    QString selected = presetCombo->currentText();
    presetCombo->blockSignals(true);
    presetCombo->clear();

    QJsonArray presets = readPaintPresets();
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        QString name = preset["name"].toString().trimmed();
        if (!name.isEmpty())
            presetCombo->addItem(name);
    }

    int idx = presetCombo->findText(selected);
    if (idx >= 0)
        presetCombo->setCurrentIndex(idx);
    presetCombo->blockSignals(false);
}

QString TerrainTools::currentTexturePresetPath()
{
    if (paintBrush == NULL || paintBrush->tex == NULL)
        return "";

    QString path = paintBrush->tex->pathid;
    path.replace("\\", "/");
    if (path.isEmpty())
        return "";

    return QFileInfo(path).fileName();
}

void TerrainTools::applyTexturePresetPath(QString texturePath)
{
    texturePath = texturePath.trimmed();
    if (texturePath.isEmpty())
        return;

    QString path = texturePath;
    if (QFileInfo(path).isRelative()) {
        path = Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),
                Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season), texturePath);
    }

    if (!QFile::exists(path)) {
        QMessageBox::warning(this, "Paint Preset", "Preset texture was not found:\n" + texturePath);
        return;
    }

    int texId = TexLib::addTex(path);
    if (texId < 0 || TexLib::mtex.find(texId) == TexLib::mtex.end() || TexLib::mtex[texId] == NULL) {
        QMessageBox::warning(this, "Paint Preset", "Preset texture could not be loaded:\n" + texturePath);
        return;
    }

    paintBrush->texId = texId;
    paintBrush->tex = TexLib::mtex[texId];
    paintBrush->useTexture = true;

    texLastItems.push_back(qMakePair(paintBrush->texId, paintBrush->tex));
    if (texLastItems.size() > 7)
        texLastItems.removeFirst();
    updateTexPrev();
}

void TerrainTools::applyPaintPreset()
{
    QString selected = presetCombo->currentText();
    if (selected.isEmpty())
        return;

    QJsonArray presets = readPaintPresets();
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString() != selected)
            continue;

        applyTexturePresetPath(preset["texture"].toString());

        int size = preset["size"].toInt(paintBrush->size);
        if (size < 1) size = 1;
        if (size > 100) size = 100;
        setBrushSize(size);
        sSize->setValue(size);

        int intensity = preset["intensity"].toInt((int)(paintBrush->alpha * 100.0f));
        if (intensity < 1) intensity = 1;
        if (intensity > 100) intensity = 100;
        setBrushAlpha(intensity);
        sIntensity->setValue(intensity);

        if (preset.contains("brush"))
            setBrushShapeIndex(preset["brush"].toInt(currentBrushShape));

        if (preset.contains("rotation")) {
            int rotation = preset["rotation"].toInt(paintBrush->texRotationDegrees);
            if (rotation < 0) rotation = 0;
            if (rotation > 360) rotation = 360;
            setTextureRotation(rotation);
            sTextureRotation->setValue(rotation);
        }

        emit setPaintBrush(paintBrush);
        return;
    }
}

void TerrainTools::savePaintPreset()
{
    QString texturePath = currentTexturePresetPath();
    if (texturePath.isEmpty()) {
        QMessageBox::warning(this, "Paint Preset", "Choose a texture before saving a paint preset.");
        return;
    }

    bool ok = false;
    QString defaultName = presetCombo->currentText();
    QString name = QInputDialog::getText(this, "Save Paint Preset", "Preset name:",
            QLineEdit::Normal, defaultName, &ok).trimmed();
    if (!ok)
        return;
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Paint Preset", "Preset name cannot be empty.");
        return;
    }
    if (name.length() > 48)
        name = name.left(48);

    QJsonArray presets = readPaintPresets();
    QJsonArray updated;
    bool replacing = false;
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString().compare(name, Qt::CaseInsensitive) == 0) {
            replacing = true;
            continue;
        }
        updated.append(preset);
    }

    if (replacing) {
        if (QMessageBox::question(this, "Save Paint Preset",
                "Replace the existing preset named \"" + name + "\"?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
    }

    QJsonObject preset;
    preset["name"] = name;
    preset["texture"] = texturePath;
    preset["size"] = paintBrush->size;
    preset["intensity"] = (int)(paintBrush->alpha * 100.0f + 0.5f);
    preset["brush"] = currentBrushShape;
    preset["rotation"] = paintBrush->texRotationDegrees;
    updated.append(preset);

    if (!writePaintPresets(updated)) {
        QMessageBox::warning(this, "Paint Preset", "Could not write the route paint preset file.");
        return;
    }

    refreshPaintPresets();
    int idx = presetCombo->findText(name);
    if (idx >= 0)
        presetCombo->setCurrentIndex(idx);
}

void TerrainTools::removePaintPreset()
{
    QString selected = presetCombo->currentText();
    if (selected.isEmpty())
        return;

    if (QMessageBox::question(this, "Remove Paint Preset",
            "Remove preset \"" + selected + "\"?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QJsonArray presets = readPaintPresets();
    QJsonArray updated;
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString() != selected)
            updated.append(preset);
    }

    if (!writePaintPresets(updated)) {
        QMessageBox::warning(this, "Paint Preset", "Could not write the route paint preset file.");
        return;
    }
    refreshPaintPresets();
}

void TerrainTools::updateTexPrev(){
    if(!this->paintBrush->tex->loaded)
        return;

    ClickableLabel *tlabel;
    unsigned char * out;
    int idx;
    int res;
    for(int i = 1; i < 8; i++){
        idx = texLastItems.size() - i;
        if(idx < 0)
            continue;
        if(i == 1){
            tlabel = texPreviewLabel;
            res = 192;
            out = this->paintBrush->tex->getImageData(res,res);
            if(this->paintBrush->tex->bytesPerPixel == 3)
                tlabel->setPixmap(QPixmap::fromImage(QImage(out,res,res,QImage::Format_RGB888)));
            if(this->paintBrush->tex->bytesPerPixel == 4)
                tlabel->setPixmap(QPixmap::fromImage(QImage(out,res,res,QImage::Format_RGBA8888)));   
        }// else {
        tlabel = texPreviewLabels[i-1];
        res = 64;
        out = texLastItems[idx].second->getImageData(res,res);
        //}
        if(texLastItems[idx].second->bytesPerPixel == 3)
            tlabel->setPixmap(QPixmap::fromImage(QImage(out,res,res,QImage::Format_RGB888)));
        if(texLastItems[idx].second->bytesPerPixel == 4)
            tlabel->setPixmap(QPixmap::fromImage(QImage(out,res,res,QImage::Format_RGBA8888)));   
    }
}

void TerrainTools::texPreviewEnabled(int val){
    //qDebug() << "TerrTools679:" << val;
    if(val == 6){
        nextBrushShape();
        return;
    }
    // qDebug() << "TerrTools 829:" << val ;
    int idx = texLastItems.size() - val - 1;
    if(idx > texLastItems.size() - 1) return;
    if(idx < 0) return;
    this->paintBrush->tex = texLastItems[idx].second;
    this->paintBrush->texId = texLastItems[idx].first;
    updateTexPrev();
}

void TerrainTools::msg(QString text, QString val){
    if(text == "toolEnabled"){
        QMapIterator<QString, QPushButton*> i(buttonTools);
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(true);
            i.value()->setChecked(false);
        }
        if(buttonTools[val] != NULL)
            buttonTools[val]->setChecked(true);
        i.toFront();
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(false);
        }
    } else if(text == "brushDirection"){
        QString t = buttonTools["heightTool"]->text().left(buttonTools["heightTool"]->text().length() - 1);
        buttonTools["heightTool"]->setText(t+val);
        t = buttonTools["waterTerrTool"]->text().left(buttonTools["waterTerrTool"]->text().length() - 1);
        buttonTools["waterTerrTool"]->setText(t+val);
        t = buttonTools["gapsTerrainTool"]->text().left(buttonTools["gapsTerrainTool"]->text().length() - 1);
        buttonTools["gapsTerrainTool"]->setText(t+val);
    } else if(text == "textureRotation"){
        int rotation = val.toInt();
        if(rotation < 0) rotation = 0;
        if(rotation > 360) rotation = 360;
        sTextureRotation->blockSignals(true);
        leTextureRotation->setText(QString::number(rotation, 10));
        sTextureRotation->setValue(rotation);
        sTextureRotation->blockSignals(false);
        paintBrush->texRotationDegrees = rotation;
    } else if(text == "preloadTextures")
    { 
        qDebug() << "emit received";
        preloadTextures();
    }
}

void TerrainTools::setPinPointBrush()
{
      setBrushSize(1);
      setBrushAlpha(1);   
      
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      
}
void TerrainTools::resetDefaultValues()
{
    if(Game::terrBrushColor)        
    {
        paintBrush->color[0] = Game::terrBrushColor->red();
        paintBrush->color[1] = Game::terrBrushColor->green();
        paintBrush->color[2] = Game::terrBrushColor->blue();
        colorw->setStyleSheet("background-color:" + Game::terrBrushColor->name() + ";");
        colorw->setText(Game::terrBrushColor->name());
    }
    if(Game::terrainTools != NULL)
    {
      setEsize(Game::terrainTools[0]);
      setEemb(Game::terrainTools[1]);
      setEcut(Game::terrainTools[2]);
      setEradius(Game::terrainTools[3]);
      setBrushSize(Game::terrainTools[4]);
      setBrushAlpha(Game::terrainTools[5]);   
      setTextureRotation(0);
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      sTextureRotation->setValue(paintBrush->texRotationDegrees);
      sEsize->setValue(paintBrush->eSize);
      sEemb->setValue(paintBrush->eEmb);
      sEcut->setValue(paintBrush->eCut);
      sEradius->setValue(paintBrush->eRadius);
    }
   
    preloadTextures();
    refreshPaintPresets();

}

void TerrainTools::resetRouteTerrtexPaint()
{
    QVector<QString> unsavedItems;
    if (Game::currentRoute != NULL)
        Game::currentRoute->getUnsavedInfo(unsavedItems);
    else if (Game::terrainLib != NULL)
        Game::terrainLib->getUnsavedInfo(unsavedItems);

    if (!unsavedItems.isEmpty()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint",
                QString("There are %1 pending route change(s).\n\n"
                        "Save your changes, then retry the reset.")
                .arg(unsavedItems.size()));
        return;
    }

    QString routePath = Game::root + "/routes/" + Game::route;
    QString tilePath = routePath + "/tiles";
    QString terrtexPath = routePath + "/terrtex";

    QDir tileDir(tilePath);
    if (!tileDir.exists()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint", "Route tiles folder was not found.");
        return;
    }

    QFileInfoList tileFiles = tileDir.entryInfoList(QStringList() << "*.t", QDir::Files, QDir::Name);
    if (tileFiles.isEmpty()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint", "No terrain tile files were found.");
        return;
    }

    QString warning = "This will reset every terrain tile patch in the current route to terrain.ace "
            "and delete per-tile ACE/DDS files from terrtex whose names start with a tile name.\n\n"
            "This cannot be undone from TSRE. Continue?";
    if (QMessageBox::question(this, "Reset Route Terrtex Paint", warning,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QSet<QString> tileNames;
    int tilesReset = 0;
    int tilesFailed = 0;

    QProgressDialog progress("Resetting route terrain paint...", "Cancel", 0, tileFiles.size(), this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);

    for (int i = 0; i < tileFiles.size(); i++) {
        if (progress.wasCanceled())
            break;

        QFileInfo tileInfo = tileFiles[i];
        QString tileName = tileInfo.completeBaseName();
        tileNames.insert(tileName.toLower());
        progress.setValue(i);
        progress.setLabelText("Resetting " + tileName + ".t");
        qApp->processEvents();

        TFile tfile;
        if (!tfile.readT(tileInfo.absoluteFilePath())) {
            tilesFailed++;
            continue;
        }

        int defaultMat = tfile.getMatByTexture("terrain.ace");
        if (defaultMat < 0)
            defaultMat = tfile.newMat();

        int patches = tfile.patchsetNpatches;
        for (int patch = 0; patch < patches * patches; patch++) {
            tfile.tdata[patch * 13 + 6] = defaultMat;
            tfile.tdata[patch * 13 + 7] = 0.001f;
            tfile.tdata[patch * 13 + 8] = 0.001f;
            tfile.tdata[patch * 13 + 9] = 0.062375f;
            tfile.tdata[patch * 13 + 10] = 0.0f;
            tfile.tdata[patch * 13 + 11] = 0.0f;
            tfile.tdata[patch * 13 + 12] = 0.062375f;
        }

        tfile.save(tileInfo.absoluteFilePath());
        tilesReset++;
    }
    progress.setValue(tileFiles.size());

    int filesDeleted = 0;
    int filesFailed = 0;
    QDir terrtexDir(terrtexPath);
    if (terrtexDir.exists()) {
        QFileInfoList texFiles = terrtexDir.entryInfoList(QStringList() << "*.ace" << "*.ACE" << "*.dds" << "*.DDS", QDir::Files, QDir::Name);
        for (int i = 0; i < texFiles.size(); i++) {
            QFileInfo texInfo = texFiles[i];
            QString baseName = texInfo.completeBaseName().toLower();
            int split = baseName.indexOf('_');
            if (split < 0)
                continue;

            QString tileName = baseName.left(split);
            if (!tileNames.contains(tileName))
                continue;

            if (QFile::remove(texInfo.absoluteFilePath()))
                filesDeleted++;
            else
                filesFailed++;
        }
    }

    int tilesReloaded = 0;
    if (Game::terrainLib != NULL)
        tilesReloaded = Game::terrainLib->reloadLoaded();

    QMessageBox::information(this, "Reset Route Terrtex Paint",
            QString("Reset %1 tile(s).\nFailed tile saves: %2\nDeleted %3 per-tile texture file(s).\nFailed deletes: %4\nReloaded %5 loaded tile(s).")
            .arg(tilesReset)
            .arg(tilesFailed)
            .arg(filesDeleted)
            .arg(filesFailed)
            .arg(tilesReloaded));
}

/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef TERRAINTOOLS_H
#define	TERRAINTOOLS_H

#include <QtWidgets>
#include <QJsonArray>
#include "Route.h"

class Brush;
class ClickableLabel;
class Texture;

class TerrainTools : public QWidget{
    Q_OBJECT

public:
    TerrainTools(QString name);
    virtual ~TerrainTools();
    
public slots:
    void heightToolEnabled(bool val);
    void paintColorToolEnabled(bool val);
    void paintTexToolEnabled(bool val);
    void mirrorSeasonEnabled(bool val);
    void pickTexToolEnabled(bool val);
    void putTexToolEnabled(bool val);
    void waterTerrToolEnabled(bool val);
    void drawTerrToolEnabled(bool val);
    void lockTexToolEnabled(bool val);
    void gapsTerrToolEnabled(bool val);
    void waterHeightTileToolEnabled(bool val);
    void fixedTileToolEnabled(bool val);
    void setTexToolEnabled();
    void preloadTexTool(QString filename);
    
    void chooseColorEnabled();
    void updateTexPrev();
    void setBrushTextureId(int val);
    void applyPaintPreset();
    void savePaintPreset();
    void removePaintPreset();
    // brush
    void setBrushSize(int val);
    void setBrushSize(QString val);
    void setBrushAlpha(int val);
    void setBrushAlpha(QString val);
    void setTextureRotation(int val);
    void setTextureRotation(QString val);
    void setFheight(QString val);
    void setHtype(int val);
    void setSeasonType(int val);
    
    // embarkment
    void setEsize(int val);
    void setEsize(QString val);
    void setEemb(int val);
    void setEemb(QString val);
    void setEcut(int val);
    void setEcut(QString val);
    void setEradius(int val);
    void setEradius(QString val);
    void setSun1(QString val);
    void setSun2(QString val);
    void setSun3(QString val);    

    void resetDefaultValues();
    void resetRouteTerrtexPaint();
    void setPinPointBrush();
    
    void msg(QString text, QString val);
    void texPreviewEnabled(int val);
    void preloadTextures();

    
signals:
    void enableTool(QString name);
    void setPaintBrush(Brush* brush);
    
private:
    Brush* paintBrush;
    QVector<QImage> brushShapes;
    int currentBrushShape = -1;
    void nextBrushShape();
    void updateColorPreview();
    QVector<QPair<int, Texture*>> texLastItems;
    QString paintPresetFilePath();
    void refreshPaintPresets();
    QJsonArray readPaintPresets();
    bool writePaintPresets(const QJsonArray &presets);
    QString currentTexturePresetPath();
    void applyTexturePresetPath(QString texturePath);
    void setBrushShapeIndex(int brushIndex);
    //QVector<QImage
    
    QPixmap* texPreview;
    QPixmap* defaultTexPreview;
    QVector<QPixmap*> texPreviews;
    ClickableLabel* texPreviewLabel;
    QVector<ClickableLabel*> texPreviewLabels;
    QSignalMapper texPreviewSignals;
    QComboBox* presetCombo;
    QPushButton* presetApply;
    QPushButton* presetSave;
    QPushButton* presetRemove;
    
    QPushButton* colorw;
    QPushButton* mirrorSeason;
    QPushButton* resetDefaults;
    QPushButton* resetRouteTerrtex;
    QPushButton* setPinPoint;    
    
    // brush gui
    
    QSlider *sSize;
    QSlider *sIntensity;
    QSlider *sTextureRotation;
    QLineEdit *leSize;
    QLineEdit *leIntensity;
    QLineEdit *leTextureRotation;
    QLineEdit *fheight;
    QComboBox* hType;
    QComboBox* seasonType;
    
    QSlider *sEsize;
    QSlider *sEemb;
    QSlider *sEcut;
    QSlider *sEradius;
    QLineEdit *leEsize;
    QLineEdit *leEemb;
    QLineEdit *leEcut;
    QLineEdit *leEradius;
    QLineEdit *sun1;    
    QLineEdit *sun2;    
    QLineEdit *sun3;        
    
    QMap<QString, QPushButton*> buttonTools;

};

#endif	/* TERRAINTOOLS_H */


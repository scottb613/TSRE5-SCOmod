/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef PROPERTIESTERRAIN_H
#define	PROPERTIESTERRAIN_H

#include "PropertiesAbstract.h"

class Terrain;
class TerrainHeightHelperWindow;

class PropertiesTerrain : public PropertiesAbstract{
    Q_OBJECT
public:
    PropertiesTerrain();
    virtual ~PropertiesTerrain();
    bool support(GameObj* obj);
    void showObj(GameObj* obj);
    void updateObj(GameObj* obj);
    QPushButton *hacksButton();
    
public slots:
    void naviInfo(int all, int hidden);
    void bHeightMapResetEnabled();
    void heightHelperClosed();
    void bRotateEnabled();
    void bCopyEnabled();
    void bPasteEnabled();
    void bScaleEnabled();
    void bScaleXEnabled();
    void bScaleYEnabled();
    void bResetEnabled();
    void bMirrorXEnabled();
    void bMirrorYEnabled();
    void bRemoveAllGapsEnabled();
    void bWaterVisibilityToggled(bool checked);
    void bDrawVisibilityToggled(bool checked);
    void bShowAdjacentEnabled();
    void eBiasEnabled(QString val);
    void eAvgWaterEnabled(QString val);
signals:
    void hacksToggled(GameObj *selection, QPushButton *button, bool checked);

private:
    Terrain* terrainObj = NULL;
    QLineEdit tP;
    QLineEdit tS;
    QLineEdit tTex;
    QLineEdit eAvgWater;
    QLineEdit eBias;
    QLineEdit eObjectCount;
    int currentObjectCount = 0;
    QPushButton waterVisibilityButton;
    QPushButton drawVisibilityButton;
    void syncVisibilityButtons();
    
    QDoubleSpinBox eScalexy;
    QDoubleSpinBox eScalex;
    QDoubleSpinBox eScaley;
    QLineEdit eRotation;
    TerrainHeightHelperWindow* heightWindow = NULL;
    QPushButton heightHelperButton;
    QPushButton hacks;
};

#endif	/* PROPERTIESTERRAIN_H */


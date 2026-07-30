/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */


#ifndef TERRAINWATERWINDOW2_H
#define	TERRAINWATERWINDOW2_H

#include <QtWidgets>
#include "GuiFunct.h"

class Terrain;

class TerrainWaterWindow2 : public EditorPopupWindow {
    Q_OBJECT
public:
    TerrainWaterWindow2(QWidget* parent);
    ~TerrainWaterWindow2() override;
    void setTerrain(Terrain *t);
    void deactivateRuler();

signals:
    void helperClosed();
    void userButtonPressed();
    void placeRulerRequested();
    void scanRequested(float heightAboveBed, int tileRadius);
    void undoScanRequested();
    void removeRulerRequested();
    
public slots:
    void eAvgTextEdited(QString val);   
    void eWaterEdited(QString val);    
    void bAdjustEdited();

protected:
    void closeEvent(QCloseEvent *event) override;
    
private:
    Terrain *terrain = NULL;
    QLineEdit e[12];
    float we[12];
    QLineEdit eAvg;
    QLineEdit eSW;
    QLineEdit eSE;
    QLineEdit eNE;
    QLineEdit eNW;
    QLineEdit tileIdentifier;
    QDoubleSpinBox waterHeight;
    QSpinBox searchDistance;
    QPushButton *placeRulerButton = NULL;
};

#endif	/* TERRAINWATERWINDOW2_H */


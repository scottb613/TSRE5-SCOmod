/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef PROPERTIESPOLYFOREST_H
#define	PROPERTIESPOLYFOREST_H

#include "PropertiesAbstract.h"

class PolyForestObj;

class PropertiesPolyForest : public PropertiesAbstract{
    Q_OBJECT
public:
    PropertiesPolyForest();
    virtual ~PropertiesPolyForest();
    bool support(GameObj* obj);
    void showObj(GameObj* obj);
    
public slots:
    void sizeEnabled(QString val);
    void populationEnabled(QString val);
    void densitykmEnabled(QString val);
    void enableCustomDetailLevelEnabled(int val);
    void customDetailLevelEdited(QString val);
    
signals:
    
private:
    PolyForestObj *polyForestObj;
    QLineEdit sizeX;
    QLineEdit sizeY;
    QLineEdit population;
    QLineEdit densitykm;
};

#endif	/* PROPERTIESPOLYFOREST_H */


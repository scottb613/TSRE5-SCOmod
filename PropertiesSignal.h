/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef PROPERTIESSIGNAL_H
#define	PROPERTIESSIGNAL_H

#include "PropertiesAbstract.h"

class SignalWindow;
class SignalObj;

class PropertiesSignal : public PropertiesAbstract {
    Q_OBJECT
    
public:
    PropertiesSignal();
    virtual ~PropertiesSignal();
    bool support(GameObj* obj);
    void showObj(GameObj* obj);
    void updateObj(GameObj* obj);
    QPushButton *hacksButton();

public slots:
    // EFO added two
    void enableCustomDetailLevelEnabled(int val);  
    void customDetailLevelEdited(QString val);    
    void shiftSignal();
    void flipSignal();
    void showSubObjList(bool checked);
    void subObjectsWindowClosed();
    void checkboxAnimEdited(int val);
    void checkboxTerrainEdited(int val);
    void cShadowTypeEdited(int val);
    void msg(QString name, QString val);
    void editPositionEnabled(QString val);
    
signals:
    void enableTool(QString val);
    void hacksToggled(GameObj *selection, QPushButton *button, bool checked);
    
private:
    QLineEdit name;
    QLineEdit description;
    QCheckBox chFlipShape;
    SignalObj* sobj;
    SignalWindow* signalWindow;
    QPushButton* subObjectsButton = NULL;
    QPushButton hacks;
    
    QCheckBox checkboxAnim;
    QCheckBox checkboxTerrain;
    QComboBox cShadowType;
};

#endif	/* PROPERTIESSIGNAL_H */



#ifndef PROPERTIESHAZARD_H
#define	PROPERTIESHAZARD_H
#include "HazardObj.h"
#include "PropertiesAbstract.h"
#include "WorldObj.h"
#include "Game.h"

class HazardObj;

class PropertiesHazard : public PropertiesAbstract{
    Q_OBJECT 
public:
    PropertiesHazard();
    virtual ~PropertiesHazard();
    bool support(GameObj* obj);
    void showObj(GameObj* obj);
    
public slots:


    
signals:
    
private:
    HazardObj* hazardObj;
    QLineEdit nameHazard;
    QLineEdit trItemRef;
    QLineEdit trItemRef2;
    QLineEdit trItemRData;
    QLineEdit trItemSData;
    QLineEdit trItemPData;    
    
};

#endif	/* PROPERTIESSIDING_H */


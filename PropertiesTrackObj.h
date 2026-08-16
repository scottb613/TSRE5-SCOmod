/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef PROPERTIESTRACKOBJ_H
#define	PROPERTIESTRACKOBJ_H

#include "PropertiesAbstract.h"

class TrackObj;
class GradeHelperWindow;
class HacksWindow;

class PropertiesTrackObj : public PropertiesAbstract{
    Q_OBJECT
public:
    PropertiesTrackObj();
    virtual ~PropertiesTrackObj();
    bool support(GameObj* obj);
    void showObj(GameObj* obj);
    void updateObj(GameObj* obj);
    void activateGradeAssistPlacement();
    int gradeUnitsIndex() const;
    QPushButton *hacksButton();
    
public slots:
    void enableCustomDetailLevelEnabled(int val);
    void customDetailLevelEdited(QString val);
    void fixJNodePosnEnabled();
    void elevPromEnabled(QString val);
    void elevProgEnabled(QString val);
    void elevPropEnabled(QString val);
    void elev1inXmEnabled(QString val);
    void elevStepEnabled(QString val);
    void elevTypeEdited(QString val);
    void editFileNameEnabled();
    void cCollisionTypeEdited(int val);
    void haxRemoveTDBVectorEnabled();
    void haxElevTDBVectorEnabled();
    void haxRemoveTDBTreeEnabled();
    void removeAllInteractivesEnabled();
    void deleteSelectedInstancesEnabled();
    void toggleHacksForSelection(
        GameObj *obj, QPushButton *button, bool checked);
    void adoptHacksButton(QPushButton *button);
    void setHacksSelection(GameObj *obj);
    void eTemplateEdited(QString val);
    void openGradeHelper();
    void gradeHelperWindowClosed();
    void hacksWindowClosed();
    void resetGradeHelper();
        
signals:
    void setMoveStep(float val);
    void requestMainFocus();
    void resetRouteTerrtexRequested();
    void disableRouteWaterRequested();
    void deleteAllPolyVegBakesRequested();
    
private:
    friend class HacksWindow;
    TrackObj* trackObj = NULL;
    WorldObj* hacksSelection = NULL;
    QComboBox elevType;
    QLineEdit elevStep;
    QLineEdit elevProm;
    QLineEdit elevProg;
    QLineEdit elevProp;
    QLineEdit elev1inXm;
    QLabel elevValueLabel;
    QStackedWidget elevValueStack;
    QPushButton gradeLock;
    QPushButton gradeHelper;
    GradeHelperWindow *gradeHelperWindow = NULL;
    HacksWindow *hacksWindow = NULL;
    QPushButton *activeHacksButton = NULL;
    QTimer gradeHelperUiTimer;
    QComboBox cCollisionType;
    QLineEdit eCollisionFlags;
    QLineEdit eSectionIdx;
    QPushButton hacks;
    
    void showElevBox(QString val);
    void setStepValue(float step);
    float getStepValue(float step);
    float currentGradePercent() const;
    void refreshGradeLockUi();
    void refreshGradeHelperUi();
    bool hasSelectedTrackForHacks() const;
    bool hasSelectedStaticForHacks() const;
};

#endif	/* PROPERTIESTRACKOBJ_H */


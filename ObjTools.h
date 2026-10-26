/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef TOOLBOX_H
#define	TOOLBOX_H

#include <QtWidgets>
#include "Route.h"
#include "Ref.h"
#include <deque>

class AutoPlacementWindow;

class ObjTools : public QWidget{
    Q_OBJECT

public:
    ObjTools(QString name);
    virtual ~ObjTools();
    QWidget *autoPlacementPanel() const;
    QWidget *polyVegRecipeHost() const;
    bool polyVegCorridorSelected() const;
    void setPolyVegCorridorStatus(const QString &text);
    void setPolyVegCorridorCounts(int raw, int blocks, int tiles);
    
public slots:
    void routeLoaded(Route * a);
    void refClassSelected(const QString & text);
    void refTrackSelected(const QString & text);
    void refRoadSelected(const QString & text);
    void refOtherSelected(const QString & text);
    void refSearchSelected(const QString & text);
    void resetObjectSearch();
    void clearRecentItems();
    void refListSelected(QListWidgetItem * item);
    void lastItemsListSelected(QListWidgetItem * item);
    void selectToolEnabled(bool val);
    void placeToolEnabled(bool val);
    void autoPlacementButtonEnabled(bool val);
    void itemSelected(Ref::RefItem* item);
    void stickToTDBEnabled(int state);
    void autoPlacementLengthEnabled(QString val);
    void resetRotationButtonEnabled();
    void advancedPlacementButtonEnabled(bool val);
    void autoPlacementDeleteLastEnabled();
    void autoPlacementRotTypeSelected(QString val);
    void autoPlacementTargetSelected(QString val);
    void autoPlacementOffsetEnabled(QString val);
    void autoSnapableRadiusEnabled(QString val);
    void chSnapableOnlyRotation(int val);
    void autoPlacementPanelVisibilityChanged(bool visible);
    void activateAutoPlaceRuler();

    void showLastItemsContextMenu(QPoint val);
    void lastItemsMenuFindSimilar();
    
    void refreshObjLists();
    
    void msg(QString name);
    void msg(QString name, bool val);
    void msg(QString name, int val);
    void msg(QString name, float val);
    void msg(QString name, QString val);
    
signals:
    void enableTool(QString name);
    void userModeChanged();
    void polyVegRecipeHostRequested(bool corridor);
    void polyVegCorridorSettingsChanged(int target, int side, double range,
                                       double setback, double width);
    void plantPolyVegCorridorRequested(bool active);
    void undoPolyVegCorridorRequested();
    void bakePolyVegCorridorRequested();
    void requestMainFocus();

    void sendMsg(QString name);
    void sendMsg(QString name, bool val);
    void sendMsg(QString name, int val);
    void sendMsg(QString name, float val);
    void sendMsg(QString name, QString val);

    /// EFO Status updates
    void updStatus(QString statName, QString statValue);
    

    
private:
    Route* route = NULL;
    QListWidget refList;
    QListWidget lastItems;
    QComboBox refClass;
    QComboBox refTrack;
    QComboBox refRoad;
    QComboBox refOther;
    QPushButton resetSearchButton;
    QPushButton clearRecentButton;
    Ref::RefItem itemRef;
    std::deque<Ref::RefItem*> lastItemsPtr;
    QVector<Ref::RefItem*> currentItemList;
    QCheckBox stickToTDB;
    QCheckBox *autoStickTerrain = nullptr;
    QComboBox *manualPlacementTarget = nullptr;
    QDoubleSpinBox *autoPlacementRange = nullptr;
    QCheckBox stickToRDB;   
    QLineEdit autoPlacementLength;
    QLineEdit searchBox;
    //QPushButton *selectTool;
    //QPushButton *placeTool;
    //QPushButton *autoPlacementButton;
    QMap<QString, QPushButton*> buttonTools;
    
    QFrame advancedPlacementWidget;
    QLineEdit autoPlacementPosX;
    QLineEdit autoPlacementPosY;
    QLineEdit autoPlacementPosZ;
    QLineEdit autoPlacementRotX;
    QLineEdit autoPlacementRotY;
    QLineEdit autoPlacementRotZ;
    QLineEdit autoSnapableRadius;
    QComboBox autoPlacementRotType;
    QComboBox autoPlacementTarget;
    AutoPlacementWindow *autoPlacementWindow = NULL;
    QButtonGroup *autoPlacementModes = nullptr;
    QCheckBox *autoPlacementChoices[3] = {};
    QLabel *autoPlacementLabels[3] = {};
    QPushButton *plantCorridor = nullptr;
    QWidget *polyVegRecipeContainer = nullptr;
    QLabel *corridorStatus = nullptr;
    QLabel *corridorCounts = nullptr;
    QFrame *wireSettingsCard = nullptr;
    QWidget *wireSection = nullptr;

    void resetCategoryCombos(QComboBox* keepActive);
    void populateObjectListForKey(QString key, QString searchText = QString());
    void populateAllObjectList(QString searchText = QString());
    QString activeCategoryKey() const;
    
};

#endif	/* TOOLBOX_H */


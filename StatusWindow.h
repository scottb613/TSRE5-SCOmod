/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef STATUSWINDOW_H
#define	STATUSWINDOW_H

#include <QtWidgets>
#include <QMap>

class Coords;
class PreciseTileCoordinate;
class IghCoordinate;
class LatitudeLongitudeCoordinate;

class StatusWindow : public QWidget {
    Q_OBJECT
public:
    StatusWindow(QWidget* parent);
    virtual ~StatusWindow();

public slots:

    void recStatus(QString statName, QString statVal );
    void cameraButtonAction(bool val);
    void selectButtonAction();
    void placeButtonAction();
    void rotateButtonAction();
    void translateButtonAction();
    void resizeButtonAction();
    void autoTdbButtonAction();
    void stickTerrainButtonAction();
    void brushDirectionButtonAction();
    void cameraLockButtonAction();
    void cameraTerrainButtonAction();
    void objectSelectedButtonAction();
    void placeGuardButtonAction();
    void moveFastButtonAction();
    void moveSlowButtonAction();
    void applyWindowSnap();
    void clearGuardError();
    void jumpTileSelected();
    void naviInfo(int all, int hidden);
    void pointerInfo(float* coords);
    void posInfo(PreciseTileCoordinate* coords);
    void reloadMkrLists();
    void mkrList(QMap<QString, Coords*> list);
    void mkrFilesSelected(QString item);
    void mkrListSelected(QString item);
    void latLonChanged(QString val);
    void xyChanged(QString val);


signals:
    void windowClosed();
    void enableTool(QString name);
    void statusCommand(QString name);
    void jumpTo(PreciseTileCoordinate* coords);
    void sendMsg(QString name, QString val);
    void requestMainFocus();
    void jumpSoundRequested();

protected:
    void hideEvent(QHideEvent *e);
    void moveEvent(QMoveEvent *e);



private:
    /// EFO New
    QPushButton status0;
    QPushButton status1;
    QPushButton status2;
    QPushButton status3;
    QPushButton status4;
    QPushButton status5;
    QPushButton status6;
    QPushButton status7;
    QPushButton status8;
    QPushButton status9;
    QPushButton status10;
    QPushButton status11;
    QPushButton status12;
    QPushButton moveFast;
    QPushButton moveSlow;

    QString statG;
    QString statY;
    QString statS;
    QString statReadout;
    QString statReadoutY;
    QString statR;
    QString lastGuardStatus = "Place Guard: ON";
    bool snapping = false;
    bool guardErrorActive = false;
    QTimer snapTimer;
    QTimer guardErrorTimer;
    QComboBox markerFiles;
    QComboBox markerList;
    QLineEdit txBox;
    QLineEdit tyBox;
    QLineEdit latBox;
    QLineEdit lonBox;
    QLineEdit xBox;
    QLineEdit yBox;
    QLineEdit zBox;
    QLineEdit pxBox;
    QLineEdit pyBox;
    QLineEdit pyBoxx;
    QLineEdit pzBox;
    QLabel tileInfo;
    int lastTX = 0;
    int lastTZ = 0;
    float lastX = 0;
    float lastY = 0;
    float lastZ = 0;
    float lastPX = 0;
    float lastPY = 0;
    float lastPZ = 0;
    bool pointerInfoValid = false;
    bool posInfoValid = false;
    int objCount = 0;
    int objHidden = 0;
    IghCoordinate* igh = NULL;
    LatitudeLongitudeCoordinate* latlon = NULL;
    PreciseTileCoordinate* aCoords = NULL;
    QMap<QString, Coords*> mkrFiles;
    QMap<QString, LatitudeLongitudeCoordinate*> mkrPlaces;
    QString jumpType = "";
    ///

};

#endif	/* STATUSWINDOW_H */


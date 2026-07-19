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
    void applyWindowSnap();
    void clearGuardError();


signals:
    void windowClosed();
    void enableTool(QString name);
    void statusCommand(QString name);

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

    QString statG;
    QString statY;
    QString statS;
    QString statR;
    QString lastGuardStatus = "Place Guard: ON";
    bool snapping = false;
    bool guardErrorActive = false;
    QTimer snapTimer;
    QTimer guardErrorTimer;
    ///

};

#endif	/* STATUSWINDOW_H */


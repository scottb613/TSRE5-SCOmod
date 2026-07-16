/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "StatusWindow.h"
#include "GeoCoordinates.h"
#include <QtWidgets>
#include <QDebug>
#include "Game.h"
#include "Route.h"
#include "RouteEditorGLWidget.h"
#include "ShapeLib.h"

static QPoint snapWindowPosition(QWidget *window){
    const int snapDistance = 10;
    QRect moving = window->frameGeometry();
    QPoint snappedFramePos = moving.topLeft();
    int bestX = snapDistance + 1;
    int bestY = snapDistance + 1;

    QList<QWidget*> targets;
    QWidget *parent = window->parentWidget();
    if(parent != NULL)
        targets.append(parent);
    if(parent != NULL){
        QList<QWidget*> siblings = parent->findChildren<QWidget*>();
        for(int i = 0; i < siblings.size(); i++){
            QWidget *candidate = siblings[i];
            if(candidate == window || !candidate->isWindow() || !candidate->isVisible())
                continue;
            targets.append(candidate);
        }
    }

    for(int i = 0; i < targets.size(); i++){
        QRect target = targets[i]->frameGeometry();
        bool verticalNear = moving.bottom() >= target.top() - snapDistance && moving.top() <= target.bottom() + snapDistance;
        bool horizontalNear = moving.right() >= target.left() - snapDistance && moving.left() <= target.right() + snapDistance;
        int xCandidates[4] = {
            target.left() - moving.left(),
            target.right() - moving.right(),
            target.right() + 1 - moving.left(),
            target.left() - 1 - moving.right()
        };
        int yCandidates[4] = {
            target.top() - moving.top(),
            target.bottom() - moving.bottom(),
            target.bottom() + 1 - moving.top(),
            target.top() - 1 - moving.bottom()
        };

        for(int j = 0; j < 4; j++){
            int dist = qAbs(xCandidates[j]);
            if(verticalNear && dist <= snapDistance && dist < bestX){
                bestX = dist;
                snappedFramePos.setX(moving.left() + xCandidates[j]);
            }
            dist = qAbs(yCandidates[j]);
            if(horizontalNear && dist <= snapDistance && dist < bestY){
                bestY = dist;
                snappedFramePos.setY(moving.top() + yCandidates[j]);
            }
        }
    }

    return window->pos() + (snappedFramePos - moving.topLeft());
}

StatusWindow::StatusWindow(QWidget* parent) : QWidget(parent) {
    this->setWindowFlags(Qt::WindowType::Tool);
    //this->setWindowFlags(Qt::WindowStaysOnTopHint);
    this->setFixedWidth(300);
    this->setFixedHeight(180);
    this->setWindowTitle(tr("Status Window"));
    QStringList winPos = Game::statusPos.split(",");
    if(winPos.count() > 1) this->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
    snapTimer.setSingleShot(true);
    QObject::connect(&snapTimer, SIGNAL(timeout()), this, SLOT(applyWindowSnap()));


    /// EFO New




    QVBoxLayout *v = new QVBoxLayout;
    v->setSpacing(2);
    v->setContentsMargins(0,1,1,1);

    QGridLayout *vbox = new QGridLayout;


/// EFO
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,0,1,0);
    QList<QPushButton*> buttons;
    buttons << &status0 << &status1 << &status2 << &status3 << &status4 << &status5
            << &status6 << &status7 << &status8 << &status9 << &status10 << &status11;
    for(int i = 0; i < buttons.size(); i++){
        buttons[i]->setFlat(false);
        buttons[i]->setFocusPolicy(Qt::NoFocus);
        buttons[i]->setFixedHeight(21);
        buttons[i]->setContentsMargins(0,0,0,0);
    }
    status10.setFlat(true);

    vbox->addWidget(&status4,0,0);
    vbox->addWidget(&status9,0,1);
    vbox->addWidget(&status7,1,0);
    vbox->addWidget(&status8,1,1);
    vbox->addWidget(&status3,2,0);
    vbox->addWidget(&status6,2,1);
    vbox->addWidget(&status1,3,0);
    vbox->addWidget(&status2,3,1);
    vbox->addWidget(&status0,4,0);
    vbox->addWidget(&status5,4,1);
    vbox->addWidget(&status10,5,0);
    vbox->addWidget(&status11,5,1);

    v->addItem(vbox);


    /// EFO end

    this->setLayout(v);

    if(Game::systemTheme == false)
    {
     statG = "QPushButton { background-color: green; color: white; border: 1px solid #777; padding: 1px; } QPushButton:pressed { background-color: #006000; }";
     statY = "QPushButton { background-color: yellow; color: black; border: 1px solid #777; padding: 1px; } QPushButton:pressed { background-color: #d0d000; }";
     statS = "QPushButton { background-color: black; color: white; border: 1px solid #777; padding: 1px; } QPushButton:pressed { background-color: #303030; }";
     statR = "QPushButton { background-color: red; color: black; border: 1px solid #777; padding: 1px; } QPushButton:pressed { background-color: #b00000; }";
    }
    else
    {
     statG = "QPushButton { background-color: #55AA55; color: white; border: 1px solid #888; padding: 1px; } QPushButton:pressed { background-color: #337733; }";
     statY = "QPushButton { background-color: #D6C94A; color: black; border: 1px solid #888; padding: 1px; } QPushButton:pressed { background-color: #aaa033; }";
     statS = "QPushButton { background-color: #202020; color: white; border: 1px solid #666; padding: 1px; } QPushButton:pressed { background-color: #3a3a3a; }";
     statR = "QPushButton { background-color: #CC5555; color: white; border: 1px solid #888; padding: 1px; } QPushButton:pressed { background-color: #993333; }";
    }

    QObject::connect(&status4, SIGNAL(released()), this, SLOT(selectButtonAction()));
    QObject::connect(&status9, SIGNAL(released()), this, SLOT(placeButtonAction()));
    QObject::connect(&status7, SIGNAL(released()), this, SLOT(rotateButtonAction()));
    QObject::connect(&status8, SIGNAL(released()), this, SLOT(translateButtonAction()));
    QObject::connect(&status3, SIGNAL(released()), this, SLOT(resizeButtonAction()));
    QObject::connect(&status6, SIGNAL(released()), this, SLOT(stickTerrainButtonAction()));
    QObject::connect(&status1, SIGNAL(released()), this, SLOT(autoTdbButtonAction()));
    QObject::connect(&status2, SIGNAL(released()), this, SLOT(brushDirectionButtonAction()));
    QObject::connect(&status0, SIGNAL(released()), this, SLOT(cameraLockButtonAction()));
    QObject::connect(&status5, SIGNAL(released()), this, SLOT(cameraTerrainButtonAction()));
    QObject::connect(&status11, SIGNAL(released()), this, SLOT(objectSelectedButtonAction()));

}


StatusWindow::~StatusWindow() {
}

void StatusWindow::hideEvent(QHideEvent *e){
    emit windowClosed();
}

void StatusWindow::moveEvent(QMoveEvent *e){
    QWidget::moveEvent(e);
    if(snapping)
        return;

    snapTimer.start(120);
}

void StatusWindow::applyWindowSnap(){
    if(snapping)
        return;

    QPoint snapped = snapWindowPosition(this);
    if(snapped == pos())
        return;

    snapping = true;
    move(snapped);
    snapping = false;
}


void StatusWindow::cameraButtonAction(bool val){
    qDebug() << "Camera Clicked: " << val;
    if(val)
        emit enableTool("selectTool");
    else
        emit enableTool("selectTool");
}

void StatusWindow::selectButtonAction(){
    emit statusCommand("select");
}

void StatusWindow::placeButtonAction(){
    emit statusCommand("place");
}

void StatusWindow::rotateButtonAction(){
    emit statusCommand("rotate");
}

void StatusWindow::translateButtonAction(){
    emit statusCommand("translate");
}

void StatusWindow::resizeButtonAction(){
    emit statusCommand("resize");
}

void StatusWindow::autoTdbButtonAction(){
    emit statusCommand("autotdb");
}

void StatusWindow::stickTerrainButtonAction(){
    emit statusCommand("stickterr");
}

void StatusWindow::brushDirectionButtonAction(){
    emit statusCommand("brushdir");
}

void StatusWindow::cameraLockButtonAction(){
    emit statusCommand("camera");
}

void StatusWindow::cameraTerrainButtonAction(){
    emit statusCommand("camterr");
}

void StatusWindow::objectSelectedButtonAction(){
    emit statusCommand("clearselect");
}

void StatusWindow::recStatus(QString statName, QString statVal ){
    // These get emitted from REGLW triggers and update here
    if(statName.contains("camera"))    { statVal.replace("Camera Unlocked", "Camera: Free"); statVal.replace("Camera Locked", "Camera: Locked"); status0.setText(statVal); if(statVal.endsWith("Locked")) status0.setStyleSheet(statG); else status0.setStyleSheet(statS);  }
    if(statName.contains("autotdb"))   { status1.setText(statVal); if(statVal.endsWith("ON")) status1.setStyleSheet(statG); else status1.setStyleSheet(statY);  }
    if(statName.contains("brush"))     { status2.setText(statVal); if(statVal.endsWith("+")) status2.setStyleSheet(statG); else status2.setStyleSheet(statY); }
    if(statName.contains("resize"))    { status3.setText(statVal); if(statVal.endsWith("ON")) status3.setStyleSheet(statY); else status3.setStyleSheet(statS);  }
    if(statName.contains("select"))    { status4.setText(statVal); if(statVal.endsWith("ON")) status4.setStyleSheet(statG); else status4.setStyleSheet(statS);  }

    if(statName.contains("camterr"))   { statVal.replace("Cam Terrain Unlocked", "Camera Terrain: Free"); statVal.replace("Cam Terrain Locked", "Camera Terrain: Locked"); status5.setText(statVal); if(statVal.endsWith("Locked")) status5.setStyleSheet(statG); else status5.setStyleSheet(statY);  }
    if(statName.contains("stickterr")) { statVal.replace("StickToTerrain", "Stick To Terrain"); status6.setText(statVal); if(statVal.endsWith("ON")) status6.setStyleSheet(statG); else status6.setStyleSheet(statY);  }
    if(statName.contains("rotate"))    { status7.setText(statVal); if(statVal.endsWith("ON")) status7.setStyleSheet(statY); else status7.setStyleSheet(statS);  }
    if(statName.contains("translate")) { status8.setText(statVal); if(statVal.endsWith("ON")) status8.setStyleSheet(statY); else status8.setStyleSheet(statS);  }
    if(statName.contains("place"))     { statVal.replace("Place:", "Place New:"); status9.setText(statVal); if(statVal.endsWith("ON")) status9.setStyleSheet(statG); else status9.setStyleSheet(statS);  }
    if(statName.contains("timer"))     { status10.setText(statVal + "m elapsed without Save"); if(statVal.toInt() > 10) status10.setStyleSheet(statY); else status10.setStyleSheet(statS);  }
    if(statName.contains("object"))    { if(statVal.size() > 0) {status11.setText(statVal + " Selected"); status11.setStyleSheet(statY); } else {status11.setText(""); status11.setStyleSheet(statS);}  }
}


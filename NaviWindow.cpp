/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "NaviWindow.h"
#include "GeoCoordinates.h"
#include <QtWidgets>
#include <QDebug>
#include "Coords.h"
#include "CoordsMkr.h"
#include "CoordsKml.h"
#include "Game.h"
#include "Route.h"
#include "RouteEditorGLWidget.h"
#include "ShapeLib.h"
#include "Trk.h"
#include "Camera.h"

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



NaviWindow::NaviWindow(QWidget* parent) : QWidget(parent) {
    this->setWindowFlags(Qt::WindowType::Tool);
    //this->setWindowFlags(Qt::WindowStaysOnTopHint);
    this->setFixedWidth(300);
    this->setFixedHeight(180);
    this->setWindowTitle(tr("Navi Window"));
    QStringList winPos = Game::naviPos.split(","); 
    if(winPos.count() > 1) this->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
    snapTimer.setSingleShot(true);
    QObject::connect(&snapTimer, SIGNAL(timeout()), this, SLOT(applyWindowSnap()));
    
    markerFiles.setStyleSheet("combobox-popup: 0;");
    markerFiles.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    markerList.setStyleSheet("combobox-popup: 0;");
    markerList.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QPushButton *jumpButton = new QPushButton("Jump", this);
    QLabel *cameraPosLabel = new QLabel("Camera:", this);
    QLabel *pointerPosLabel = new QLabel("Pointer:", this);
    QLabel *txLabel = new QLabel("X", this);
    QLabel *tyLabel = new QLabel("Y", this);
    QLabel *xLabel = new QLabel("x", this);
    QLabel *yLabel = new QLabel("y", this);
    QLabel *zLabel = new QLabel("z", this);
    QLabel *pxLabel = new QLabel("x", this);
    QLabel *pyLabel = new QLabel("y", this);
    QLabel *pzLabel = new QLabel("z", this);
    QLabel *latLabel = new QLabel("lat", this);
    QLabel *lonLabel = new QLabel("lon", this);
    QLabel *empty = new QLabel(" ", this);
    
    /*
    /// EFO New
    status1.setText("");
    status2.setText("");
    status3.setText("");
    status4.setText("");
    
    status1.setEnabled(false);
    status2.setEnabled(false);
    status3.setEnabled(false);
    status4.setEnabled(false);
  */  
    ///
    
    QLabel *label1 = new QLabel("Position:");
    label1->setContentsMargins(3,0,0,0);
    label1->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");

    QVBoxLayout *v = new QVBoxLayout;
    v->setSpacing(2);
    v->setContentsMargins(0,1,1,1);
    v->addWidget(&markerFiles);
    v->addWidget(&markerList);
    
    QGridLayout *vbox = new QGridLayout;

/*
 * 
/// EFO    
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,0,1,0);    
    //int row = 0;
    vbox->addWidget(&status1,0,0);
    vbox->addWidget(&status2,1,0);
    vbox->addWidget(&status3,0,1);
    vbox->addWidget(&status4,1,1);
    v->addItem(vbox);    


 
    vbox = new QGridLayout;
    /// EFO end
 */     
    
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,0,1,0);    
    vbox->addWidget(pointerPosLabel,0,0);
    vbox->addWidget(pxLabel, 0,1);
    vbox->addWidget(&pxBox,  0,2);
    vbox->addWidget(pyLabel, 0,3);
    vbox->addWidget(&pyBox,  0,4);
    vbox->addWidget(pzLabel, 0,5);
    vbox->addWidget(&pzBox,  0,6);

    
    pxBox.setEnabled(false);
    pyBox.setEnabled(false);   
    pzBox.setEnabled(false);
    
    if(Game::convertUnitD != 'm')
    {
      pyBoxx.setEnabled(false);    
      vbox->addWidget(&pyBoxx, 0,7);
    }

    vbox->addWidget(cameraPosLabel,1,0);
    vbox->addWidget(xLabel,1,1);
    vbox->addWidget(&xBox,1,2);
    vbox->addWidget(yLabel,1,3);
    vbox->addWidget(&yBox,1,4);
    vbox->addWidget(zLabel,1,5);
    vbox->addWidget(&zBox,1,6);
    
//      pRot.setEnabled(false);    
//      vbox->addWidget(&pRot, 1,7);
    
    
    v->addItem(vbox);
        
    vbox = new QGridLayout;
    vbox->setSpacing(2);
    vbox->setContentsMargins(3,0,1,0);    
    //int row = 0;
    vbox->addWidget(txLabel,0,0);
    vbox->addWidget(&txBox,0,1);
    vbox->addWidget(tyLabel,0,2);
    vbox->addWidget(&tyBox,0,3);
    vbox->addWidget(latLabel,1,0);
    vbox->addWidget(&latBox,1,1);
    vbox->addWidget(lonLabel,1,2);
    vbox->addWidget(&lonBox,1,3);
    v->addItem(vbox);
    //vbox = new QGridLayout;
    //vbox->setSpacing(2);
    //vbox->setContentsMargins(3,0,1,0);    
    v->addWidget(&tileInfo);
    v->addWidget(jumpButton);
    //vbox->addStretch(1);
    this->setLayout(v);
    
    QObject::connect(&txBox, SIGNAL(textEdited(QString)),
                      this, SLOT(xyChanged(QString)));
    QObject::connect(&tyBox, SIGNAL(textEdited(QString)),
                      this, SLOT(xyChanged(QString)));
    QObject::connect(&xBox, SIGNAL(textEdited(QString)),
                      this, SLOT(xyChanged(QString)));
    QObject::connect(&yBox, SIGNAL(textEdited(QString)),
                      this, SLOT(xyChanged(QString)));
    QObject::connect(&zBox, SIGNAL(textEdited(QString)),
                      this, SLOT(xyChanged(QString)));
    QObject::connect(&latBox, SIGNAL(textEdited(QString)),
                      this, SLOT(latLonChanged(QString)));
    QObject::connect(&lonBox, SIGNAL(textEdited(QString)),
                      this, SLOT(latLonChanged(QString)));
    
    QObject::connect(jumpButton, SIGNAL(released()),
                      this, SLOT(jumpTileSelected()));
    
    QObject::connect(&markerFiles, SIGNAL(activated(QString)),
                      this, SLOT(mkrFilesSelected(QString)));
    QObject::connect(&markerList, SIGNAL(activated(QString)),
                      this, SLOT(mkrListSelected(QString)));
    
//    QObject::connect(&pRot, SIGNAL(textEdited(QString)),
//                      this, SLOT(camRotChanged(QString)));
    
    tileInfo.setText(" ");
}

/// EFO New... declared as public slot -> this function
//void NaviWindow::camRotChanged(QString camrot)
//{
    /// EFO plug in rotation here
    // this->pRot.setText(camrot);
    
//}

void NaviWindow::latLonChanged(QString val){
    this->jumpType = "latlon";
}
void NaviWindow::xyChanged(QString val){
    this->jumpType = "xy";
}

void NaviWindow::jumpTileSelected(){
    if(aCoords == NULL)
        aCoords = new PreciseTileCoordinate();
             
    if(this->jumpType == "xy"){
        aCoords->setWxyz(xBox.text().toInt(), yBox.text().toInt(), zBox.text().toInt());
        aCoords->TileX = txBox.text().toInt();
        aCoords->TileZ = tyBox.text().toInt();
        emit jumpTo(aCoords);
    }
    if(this->jumpType == "latlon"){
        igh = Game::GeoCoordConverter->ConvertToInternal(latBox.text().toDouble(), lonBox.text().toDouble(), igh);
        aCoords = Game::GeoCoordConverter->ConvertToTile(igh, aCoords);
        aCoords->setWxyz();
        aCoords->wZ = -aCoords->wZ;
        emit jumpTo(aCoords);
    }
    if(this->jumpType == "marker"){
        if(mkrPlaces[markerList.currentText()] == NULL) return;
        igh = Game::GeoCoordConverter->ConvertToInternal(mkrPlaces[markerList.currentText()]->Latitude, mkrPlaces[markerList.currentText()]->Longitude, igh);
        aCoords = Game::GeoCoordConverter->ConvertToTile(igh, aCoords);
        aCoords->setWxyz();
        aCoords->wZ = -aCoords->wZ;
        emit jumpTo(aCoords);
    }   
}


void NaviWindow::naviInfo(int all, int hidden){
    if(all != objCount || hidden != objHidden ){
        objCount = all;
        objHidden = hidden;
        this->tileInfo.setText("Objects: "+QString::number(all, 10)+" ( including "+QString::number(hidden, 10)+" hidden )");
    }
}

void NaviWindow::pointerInfo(float* coords){

    // truncate the conversion value    
    // QString fimpx = QString::number((coords[1] * Game::convertDistance),'f',0);
           
    this->pxBox.setText(QString::number(coords[0]));
    this->pyBox.setText(QString::number(coords[1]));
    //this->pyBoxx.setText(fimpx + " " + Game::convertUnitD );
    this->pyBoxx.setText(QString::number((coords[1] * Game::convertDistance),'f',0) + " " + Game::convertUnitD );
    this->pzBox.setText(QString::number(-coords[2]));

            
}

void NaviWindow::posInfo(PreciseTileCoordinate* coords){
    if(lastX != coords->X || lastY != coords->Y || lastZ != coords->Z || lastTX != coords->TileX || lastTZ != coords->TileZ){
        lastX = coords->wX;
        lastY = coords->wY;
        lastZ = coords->wZ;
        lastTX = coords->TileX;
        lastTZ = coords->TileZ;
        this->txBox.setText(QString::number(lastTX, 10));
        this->tyBox.setText(QString::number(lastTZ, 10));
        this->xBox.setText(QString::number(lastX, 10));
        this->yBox.setText(QString::number(lastY, 10));
        this->zBox.setText(QString::number(-lastZ, 10));
        igh = Game::GeoCoordConverter->ConvertToInternal(coords);
        
        latlon = Game::GeoCoordConverter->ConvertToLatLon(igh);
        this->latBox.setText(QString::number(latlon->Latitude));
        this->lonBox.setText(QString::number(latlon->Longitude));
    }
}


void NaviWindow::reloadMkrLists(){
    if(Game::debugOutput) qDebug() << "navi ReloadMkrLists";    
//    markerFiles.clear();        
    if(Game::debugOutput) qDebug() << "MkrList " << mkrFiles;     
    if(Game::debugOutput) qDebug() << "Game::MkrList" << Game::markerFiles;     
}

void NaviWindow::mkrList(QMap<QString, Coords*> list){
    if(Game::debugOutput) qDebug() << "navi MkrList";    
    markerFiles.clear();    
    mkrFiles = list;
   
    QString routeid = Game::route.toLower() + ".mkr";
    
    for (auto it = list.begin(); it != list.end(); ++it ){

        if(it.value() == NULL)            
            continue;
        if(!it.value()->loaded)
            continue;
        markerFiles.addItem(it.key());
        if(Game::debugOutput) qDebug() << "MKR List----->" << it.key();            
       }
    
    if(markerFiles.count() > 0) {
        if(markerFiles.findText(routeid))
            {
                markerFiles.setCurrentIndex(markerFiles.findText(routeid));
                mkrFilesSelected(routeid);                
            }
        else
                mkrFilesSelected(markerFiles.itemText(0));
    }
}

void NaviWindow::mkrFilesSelected(QString item){
    Coords* c = mkrFiles[item];
    if(Game::debugOutput) qDebug() << "Coords Item ----->" << item ;
    if(c == NULL) return;    
    this->sendMsg("mkrFile", item);
    this->mkrPlaces.clear();    
    QStringList hash;

    for(int i = 0; i < c->markerList.size(); i++){
        if(this->mkrPlaces[c->markerList[i].name] == NULL)
            this->mkrPlaces[c->markerList[i].name] = new LatitudeLongitudeCoordinate();
        this->mkrPlaces[c->markerList[i].name]->Latitude = c->markerList[i].lat;
        this->mkrPlaces[c->markerList[i].name]->Longitude = c->markerList[i].lon;
        hash.append(c->markerList[i].name);
    }
    hash.sort(Qt::CaseInsensitive);
     hash.removeDuplicates();    
    markerList.clear();
    markerList.addItems(hash);
    markerList.setMaxVisibleItems(25);
}

void NaviWindow::mkrListSelected(QString item){
        this->jumpType = "marker";
}
NaviWindow::~NaviWindow() {
}

void NaviWindow::hideEvent(QHideEvent *e){
    emit windowClosed();
}

void NaviWindow::moveEvent(QMoveEvent *e){
    QWidget::moveEvent(e);
    if(snapping)
        return;

    snapTimer.start(120);
}

void NaviWindow::applyWindowSnap(){
    if(snapping)
        return;

    QPoint snapped = snapWindowPosition(this);
    if(snapped == pos())
        return;

    snapping = true;
    move(snapped);
    snapping = false;
}


void NaviWindow::recStatus(QString statName, QString statVal ){
    //qDebug() << "status: " << statVal;
    // this->status1->setText(QString(statVal));
//    if(statName.contains("camRot")) pRot.setText(statVal);

}
  

/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "PropertiesAbstract.h"
#include "WorldObj.h"
#include "TransformWorldObjDialog.h"
#include "RandomTransformWorldObjDialog.h"
#include "GLMatrix.h"
#include "Undo.h"

QString PropertiesAbstract::ElevTypeName = "Permille ‰";

PropertiesAbstract::PropertiesAbstract() : QWidget() {
    setStyleSheet(
        "QWidget { background-color: #303030; color: white; }"
        "QPushButton { background-color: #555555; color: white; border: 1px solid #888; }"
        "QPushButton:hover { background-color: #666666; }"
        "QPushButton:pressed { background-color: #707070; }"
        "QLineEdit, QComboBox { background-color: #202020; color: white; border: 1px solid #666; }"
        "QCheckBox { color: white; }"
        "QCheckBox::indicator { width: 13px; height: 13px; background-color: #202020; border: 1px solid #b0b0b0; }"
        "QCheckBox::indicator:hover { border: 1px solid #f08200; }"
        "QCheckBox::indicator:checked { background-color: #f08200; border: 1px solid #f08200; }"
    );
    foreach (QObject *child, children()) {
        if (child->isWidgetType()) {
            child->installEventFilter(this);
        }
    }
}

PropertiesAbstract::~PropertiesAbstract() {
}

void PropertiesAbstract::showObj(GameObj* obj){
}

void PropertiesAbstract::updateObj(GameObj* obj){
}

bool PropertiesAbstract::support(GameObj* obj){
    return false;
}

void PropertiesAbstract::copyFileNameEnabled(){
    QClipboard *clipboard = QApplication::clipboard();
    //QString originalText = clipboard->text();
    clipboard->setText(this->fileName.text());
}

void PropertiesAbstract::resetRotEnabled(){
    if(worldObj == NULL)
        return;
    worldObj->setNewQdirection();
    worldObj->modified = true;
    worldObj->setMartix();
}
/*
void PropertiesAbstract::enableTool(){
    
}*/

void PropertiesAbstract::msg(QString name, QString val){
    
}

void PropertiesAbstract::copyREnabled(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(this->quat.text());
}

/// EFO appending tileX and tileY
void PropertiesAbstract::copyPEnabled(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(this->posX.text()+" "+this->posY.text()+" "+this->posZ.text()+" "+this->tX.text()+" "+this->tY.text());
}

void PropertiesAbstract::copyFEnabled(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(this->flags.text());
}

void PropertiesAbstract::pasteFEnabled(){
    if(worldObj == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    int val = clipboard->text().toUInt(NULL, 16);
    worldObj->set("staticflags", val);
    quat.setText(clipboard->text());
}

/// EFO appending tileX and tileY
void PropertiesAbstract::copyPREnabled(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(this->posX.text()+" "+this->posY.text()+" "+this->posZ.text()+" "+this->quat.text()+" "+this->tX.text()+" "+this->tY.text());
}

void PropertiesAbstract::rotYEnabled(){
    if(worldObj == NULL)
        return;
    
    Undo::SinglePushWorldObjData(worldObj);
    worldObj->rotate(0,M_PI/2,0);
    worldObj->modified = true;
    worldObj->setMartix();
}

void PropertiesAbstract::pasteREnabled(){
    if(worldObj == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    if(args.length() != 4)
        return;
    float nq[4];
    nq[0] = args[0].toFloat();
    nq[1] = args[1].toFloat();
    nq[2] = -args[2].toFloat();
    nq[3] = args[3].toFloat();
    
    Undo::SinglePushWorldObjData(worldObj);
    worldObj->setQdirection((float*)&nq);
    worldObj->setModified();
    worldObj->setMartix();
    quat.setText(clipboard->text());
}

/// EFO where to add tile to copy paste rot -- append to position 3/4
void PropertiesAbstract::pastePEnabled(){
    if(worldObj == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    if(args.length() != 5)  // was 3
        return;
    float nq[5];  
    nq[0] = args[0].toFloat();
    nq[1] = args[1].toFloat();
    nq[2] = -args[2].toFloat();
    nq[3] = args[3].toInt();
    nq[4] = args[4].toInt();
    
    Undo::SinglePushWorldObjData(worldObj);
    worldObj->setPosition((float*)&nq);
    worldObj->modified = true;
    worldObj->setMartix();
    this->posX.setText(args[0]);
    this->posY.setText(args[1]);
    this->posZ.setText(args[2]);
    this->tX.text() = nq[3];  
    this->tY.text() = nq[4];    
}

/// EFO where to add tile to copy paste rot -- append to position 3/4
void PropertiesAbstract::pastePREnabled(){
    if(worldObj == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    
    
    if(args.length() != 9)   // was 7
        return;
    float nq[5];
    float rq[4];
    nq[0] = args[0].toFloat();
    nq[1] = args[1].toFloat();
    nq[2] = -args[2].toFloat();
    rq[0] = args[3].toFloat();
    rq[1] = args[4].toFloat();
    rq[2] = -args[5].toFloat();
    rq[3] = args[6].toFloat();
    nq[3] = args[7].toInt();
    nq[4] = args[8].toInt();
    
        Undo::SinglePushWorldObjData(worldObj);
    worldObj->setPosition((float*)&nq);
    worldObj->setQdirection((float*)&rq);
    worldObj->setModified();
    worldObj->setMartix();
    this->posX.setText(args[0]);
    this->posY.setText(args[1]);
    this->posZ.setText(args[2]);
    quat.setText(args[3]+" "+args[4]+" "+args[5]+" "+args[6]);
    this->tX.text() = nq[7];  
    this->tY.text() = nq[8];        

    if(Game::debugOutput) qDebug() << "PastePRE: " << args;        
    
}

void PropertiesAbstract::transformEnabled(){
    if(worldObj == NULL)
        return;
    TransformWorldObjDialog transformWindow;
    transformWindow.exec();
    //qDebug() << waterWindow->changed;
    if(transformWindow.isOk){
        Undo::SinglePushWorldObjData(worldObj);
        if(transformWindow.x != 0 || transformWindow.y != 0 || + transformWindow.z != 0){
            float pos[3];
            pos[0] = transformWindow.x;
            pos[1] = transformWindow.y;
            pos[2] = -transformWindow.z;
            if(transformWindow.isUseObjRot){
                Vec3::transformQuat((float*)pos, (float*)pos, (float*)worldObj->qDirection);
            }
            worldObj->translate(pos[0], pos[1], pos[2]);
            worldObj->modified = true;
            worldObj->setMartix();
        }
        if(transformWindow.rx != 0 || transformWindow.ry != 0 || + transformWindow.rz != 0){
            worldObj->rotate(transformWindow.rx*M_PI/180.0, transformWindow.ry*M_PI/180.0, transformWindow.rz*M_PI/180.0);
            worldObj->modified = true;
            worldObj->setMartix();
        }
    }
}

void PropertiesAbstract::rtransformEnabled(){
    if(worldObj == NULL)
        return;
    RandomTransformWorldObjDialog transformWindow;
    transformWindow.exec();
    //qDebug() << waterWindow->changed;
    if(transformWindow.isOk){
        Undo::SinglePushWorldObjData(worldObj);
        worldObj->randomTransform(transformWindow.getTransform());
    }
}

void PropertiesAbstract::showEvent(QShowEvent *event){
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    foreach (QPushButton *button, buttons){
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(userButtonPressed()), Qt::UniqueConnection);
    }
    QWidget::showEvent(event);
}

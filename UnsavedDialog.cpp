/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "UnsavedDialog.h"
#include "GuiFunct.h"

UnsavedDialog::UnsavedDialog() : UnsavedDialog("SQC") {

}

UnsavedDialog::UnsavedDialog(QString buttonLayout) : QDialog(){
    GuiFunct::applyEditorPanelStyle(this);
    //this->setFixedWidth(300);
    items.setFixedWidth(300);
    //this->setFixedSize(300,300);
    qDebug() << buttonLayout;
    //QLabel *label = new QLabel("Save changes in consists?");
    if(buttonLayout == "SQC"){
        bok = new QPushButton("Save All and Quit");
        bexit = new QPushButton("Discard and Quit");
        bcancel = new QPushButton("Cancel");
    } else if(buttonLayout == "STWQC"){
        bok = new QPushButton("Save All and Quit");
        bokt = new QPushButton("Save Terrain and Quit");        
        bokw = new QPushButton("Save World and Quit");        
        bexit = new QPushButton("Discard and Quit");
        bcancel = new QPushButton("Cancel");
    } else if(buttonLayout == "SC"){
        bok = new QPushButton("Save");
        bcancel = new QPushButton("Cancel");
    } else { 
        qDebug() << "#UnsavedDialog: wrong button layout";
        return;
    }
    
    if(buttonLayout.contains("S"))
        connect(bok, SIGNAL (released()), this, SLOT (ok()));
    if(buttonLayout.contains("T")) //// EFO  need to flesh this out for saving terrain and world separately
        connect(bokt, SIGNAL (released()), this, SLOT (okt()));
    if(buttonLayout.contains("W")) //// EFO  need to flesh this out for saving terrain and world separately
        connect(bokw, SIGNAL (released()), this, SLOT (okw()));        
    if(buttonLayout.contains("C"))
        connect(bcancel, SIGNAL (released()), this, SLOT (cancel()));
    if(buttonLayout.contains("Q"))
        connect(bexit, SIGNAL (released()), this, SLOT (exit()));
    
    QGridLayout *vlist = new QGridLayout;
    vlist->setSpacing(2);
    vlist->addWidget(&infoLabel, 0, 0, 1, 3, Qt::AlignCenter);
    //vlist->addWidget(new QLabel("New FileName:"), 1, 0, Qt::AlignLeft);
    //vlist->addWidget(&name, 1, 1, 1, 1, Qt::AlignLeft);
    //QHBoxLayout *vlist1 = new QHBoxLayout;
    vlist->addWidget(&items, 1, 0, 1, 3, Qt::AlignCenter);
    if(buttonLayout == "SQC"){
        vlist->addWidget(bok, 2, 0);
        vlist->addWidget(bexit, 2, 1);
        vlist->addWidget(bcancel, 2, 2);
    } else if(buttonLayout == "STWQC"){
        vlist->addWidget(bok, 2, 0);
        vlist->addWidget(bokt, 2, 1);  //// EFO  need to flesh this out for saving terrain and world separately
        vlist->addWidget(bokw, 2, 2);  //// EFO  need to flesh this out for saving terrain and world separately
        vlist->addWidget(bexit, 2, 3);
        vlist->addWidget(bcancel, 2, 4);
    }
    else if(buttonLayout == "SC"){
        vlist->addWidget(bok, 2, 0, 1, 2);
        vlist->addWidget(bcancel, 2, 2);
    }
    
//    mainLayout->setAlignment(browse, Qt::AlignBottom);
    vlist->setContentsMargins(1,1,1,1);
    this->setLayout(vlist);
    this->layout()->setSizeConstraint( QLayout::SetFixedSize );
    //this->setFixedSize(this->width(),this->height());
}

void UnsavedDialog::setMsg(QString msg){
    infoLabel.setText(msg);
}

void UnsavedDialog::hideButtons(){
    bok->hide();
    bcancel->hide();
    bexit->hide();
}

void UnsavedDialog::cancel(){
    this->changed = 0;
    this->close();
}
void UnsavedDialog::ok(){
    this->changed = 1;
    this->close();
}

//// EFO  need to flesh this out for saving terrain and world separately
void UnsavedDialog::okt(){
    this->changed = 3;
    this->close();
}

void UnsavedDialog::okw(){
    this->changed = 4;
    this->close();
}


void UnsavedDialog::exit(){
    this->changed = 2;
    this->close();
}

UnsavedDialog::~UnsavedDialog() {
}


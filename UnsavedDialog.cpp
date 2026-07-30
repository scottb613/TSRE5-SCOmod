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
#include "Game.h"

UnsavedDialog::UnsavedDialog(QWidget *parent)
    : UnsavedDialog("SQC", parent) {

}

UnsavedDialog::UnsavedDialog(QString buttonLayout, QWidget *parent)
    : QDialog(parent) {
    GuiFunct::styleEditorDialog(this);
    setProperty("scoCenterOnScreen", true);
    const qreal scale = qMax(1.0f, Game::uiScale);
    setMinimumWidth(qRound(520.0f * scale));
    items.setMinimumHeight(qRound(220.0f * scale));
    qDebug() << buttonLayout;
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

    bok->setProperty("dialogRole", "positive");
    if(bexit != NULL)
        bexit->setProperty("dialogRole", "danger");
    if(bcancel != NULL){
        bcancel->setDefault(true);
        bcancel->setFocus();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(4,4,4,4);

    QHBoxLayout *warningRow = new QHBoxLayout;
    warningRow->setSpacing(8);
    QLabel *title = new QLabel("UNSAVED CHANGES");
    title->setStyleSheet(QString(
        "QLabel { color: %1; font-weight: bold;"
        " background-color: #292929; border: none;"
        " border-left: 3px solid %1; padding: 6px 8px; }")
        .arg(Game::StyleYellowButton));
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    warningIcon.setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning)
            .pixmap(qRound(28.0f * scale), qRound(28.0f * scale)));
    warningIcon.setAlignment(Qt::AlignCenter);
    warningRow->addWidget(&warningIcon);
    warningRow->addWidget(title, 1);
    mainLayout->addLayout(warningRow);

    GuiFunct::styleEditorSubtitle(&subtitleLabel);
    subtitleLabel.hide();
    mainLayout->addWidget(&subtitleLabel);
    infoLabel.setWordWrap(true);
    infoLabel.setContentsMargins(6,4,6,2);
    mainLayout->addWidget(&infoLabel);
    mainLayout->addWidget(&items, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->setSpacing(4);
    if(buttonLayout == "SQC"){
        buttons->addWidget(bok);
        buttons->addWidget(bexit);
        buttons->addWidget(bcancel);
    } else if(buttonLayout == "STWQC"){
        buttons->addWidget(bok);
        buttons->addWidget(bokt);
        buttons->addWidget(bokw);
        buttons->addWidget(bexit);
        buttons->addWidget(bcancel);
    }
    else if(buttonLayout == "SC"){
        buttons->addWidget(bok, 2);
        buttons->addWidget(bcancel);
    }
    mainLayout->addLayout(buttons);
}

void UnsavedDialog::setMsg(QString msg){
    infoLabel.setText(msg);
}

void UnsavedDialog::setSubtitle(QString subtitle){
    QString text = subtitle.trimmed();
    if(text.isEmpty()){
        subtitleLabel.clear();
        subtitleLabel.hide();
        return;
    }
    if(!text.startsWith(QChar(0x2022)))
        text.prepend(QString(QChar(0x2022)) + " ");
    subtitleLabel.setText(text.toUpper());
    subtitleLabel.show();
}

void UnsavedDialog::hideButtons(){
    if(bok != NULL)
        bok->hide();
    if(bcancel != NULL)
        bcancel->hide();
    if(bexit != NULL)
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


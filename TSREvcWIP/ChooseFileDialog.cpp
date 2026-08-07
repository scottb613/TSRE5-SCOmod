/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ChooseFileDialog.h"
#include "GuiFunct.h"
#include "Game.h"

ChooseFileDialog::ChooseFileDialog(QWidget *parent) : QDialog(parent){
    GuiFunct::styleEditorDialog(this);
    const qreal scale = qMax(1.0f, Game::uiScale);
    setMinimumWidth(qRound(560.0f * scale));
    items.setMinimumHeight(qRound(220.0f * scale));

    QPushButton* ok = new QPushButton("Edit");
    QPushButton* cancel = new QPushButton("Close");
    ok->setProperty("dialogRole", "primary");
    connect(ok, SIGNAL (released()), this, SLOT (ok()));
    connect(cancel, SIGNAL (released()), this, SLOT (cancel()));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(4,4,4,4);
    QLabel *title = new QLabel("CHOOSE SOURCE FILE");
    GuiFunct::styleEditorTitle(title);
    mainLayout->addWidget(title);
    infoLabel.setWordWrap(true);
    infoLabel.setContentsMargins(6,4,6,2);
    mainLayout->addWidget(&infoLabel);
    mainLayout->addWidget(&items, 1);
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->setSpacing(4);
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    mainLayout->addLayout(buttons);
}

void ChooseFileDialog::setMsg(QString msg){
    infoLabel.setText(msg);
}

void ChooseFileDialog::cancel(){
    this->changed = 0;
    this->close();
}
void ChooseFileDialog::ok(){
    this->changed = 1;
    this->close();
}

ChooseFileDialog::~ChooseFileDialog() {
}


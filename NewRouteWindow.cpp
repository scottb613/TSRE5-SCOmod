/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "NewRouteWindow.h"
#include "GuiFunct.h"
#include "Game.h"

NewRouteWindow::NewRouteWindow() : QDialog(){
    GuiFunct::applyEditorPanelStyle(this);
    GuiFunct::setEditorToolWindowTitle(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedWidth(qRound(320.0f * qMax(1.0f, Game::uiScale)));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4,4,4,4);

    QLabel *title = new QLabel("NEW ROUTE");
    GuiFunct::styleEditorTitle(title);
    mainLayout->addWidget(title);

    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(4);
    vlist->setContentsMargins(0,0,0,0);
    vlist->addRow("Name ID:",&this->name);
    vlist->addRow("Lat:",&this->lat);
    vlist->addRow("Lon:",&this->lon);
    mainLayout->addLayout(vlist);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &NewRouteWindow::ok);
    connect(buttons, &QDialogButtonBox::rejected, this, &NewRouteWindow::cancel);
    mainLayout->addWidget(buttons);
}

void NewRouteWindow::cancel(){
    this->changed = false;
    this->close();
}
void NewRouteWindow::ok(){
    this->changed = true;
    this->close();
}

NewRouteWindow::~NewRouteWindow() {
}


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
#include <QDoubleValidator>
#include <QLocale>

NewRouteWindow::NewRouteWindow() : QDialog(){
    GuiFunct::applyEditorPanelStyle(this);
    GuiFunct::setEditorToolWindowTitle(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedWidth(qRound(320.0f * qBound(0.75f, Game::uiScale, 1.25f)));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4,4,4,4);

    QLabel *title = new QLabel("NEW ROUTE");
    GuiFunct::styleEditorTitle(title);
    mainLayout->addWidget(title);

    QDoubleValidator *latitudeValidator =
        new QDoubleValidator(-90.0, 90.0, 12, this);
    latitudeValidator->setNotation(QDoubleValidator::StandardNotation);
    latitudeValidator->setLocale(QLocale::c());
    lat.setValidator(latitudeValidator);
    lat.setToolTip("Latitude from -90 to 90 degrees. Up to 12 decimal places are accepted.");

    QDoubleValidator *longitudeValidator =
        new QDoubleValidator(-180.0, 180.0, 12, this);
    longitudeValidator->setNotation(QDoubleValidator::StandardNotation);
    longitudeValidator->setLocale(QLocale::c());
    lon.setValidator(longitudeValidator);
    lon.setToolTip("Longitude from -180 to 180 degrees. Up to 12 decimal places are accepted.");

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
    bool latitudeOk = false;
    bool longitudeOk = false;
    const double latitude = lat.text().trimmed().toDouble(&latitudeOk);
    const double longitude = lon.text().trimmed().toDouble(&longitudeOk);
    if(!latitudeOk || !longitudeOk
            || latitude < -90.0 || latitude > 90.0
            || longitude < -180.0 || longitude > 180.0){
        GuiFunct::showEditorStopped(
            this, "Invalid Route Location",
            "Enter a latitude from -90 to 90 and a longitude from -180 to 180. Decimal coordinates may use up to 12 places.");
        return;
    }

    this->changed = true;
    this->close();
}

NewRouteWindow::~NewRouteWindow() {
}


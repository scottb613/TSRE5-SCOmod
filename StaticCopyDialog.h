// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include <QString>
class QWidget;
class StaticObj;
class Route;
namespace StaticCopyUi {
void restoreDestination(QString &destination,bool &workingFolder);
void chooseDestination(QWidget *parent,const QString &sourceRoute,QString &destination,bool &workingFolder);
void copyObject(QWidget *parent,StaticObj *object,Route *route,const QString &destination,bool workingFolder);
}

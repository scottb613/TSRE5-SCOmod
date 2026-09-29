/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */


#ifndef ERRORMESSAGESWINDOW_H
#define ERRORMESSAGESWINDOW_H

#include <QtWidgets>
#include <QMap>

class ErrorMessageProperties;
struct PreciseTileCoordinate;
class GameObj;
class ErrorMessage;

class ErrorMessagesWindow : public QWidget {
    Q_OBJECT
public:
    ErrorMessagesWindow(QWidget* parent);
    virtual ~ErrorMessagesWindow();
    
public slots:
    void show();
    void hideEvent(QHideEvent *e);
    void errorListSelected(QTreeWidgetItem* item, int column);
    void jumpRequestReceived(PreciseTileCoordinate *c);
    void selectRequestReceived(GameObj *o);
    void refreshErrorList();
    void scanRoute();
    void markNoFactor(ErrorMessage *message);
    void resetStatus();
    
signals:
    void windowClosed();
    void jumpTo(PreciseTileCoordinate *c);
    void selectObject(GameObj *o);
    
private:
    QHash<int, QBrush> brushes;
    QTreeWidget errorList;
    ErrorMessageProperties *properties;
    QPushButton *scanButton = nullptr;
    QPushButton *resetButton = nullptr;
    QDoubleSpinBox *orphanLength = nullptr;
    QDoubleSpinBox *verticalThreshold = nullptr;
    QDoubleSpinBox *horizontalThreshold = nullptr;
    QLabel *scanStatus = nullptr;
    bool scanning = false;
    
};

#endif /* ERRORMESSAGESWINDOW_H */


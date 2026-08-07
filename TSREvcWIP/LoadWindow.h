/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef LOADWINDOW_H
#define	LOADWINDOW_H

#include <QtWidgets>

class PreciseTileCoordinate;
class IghCoordinate;
class LatitudeLongitudeCoordinate;
class QProcess;

class LoadWindow : public QWidget {
    Q_OBJECT

public:
    LoadWindow();
    
public slots:
    void exitNow();
    void handleBrowseButton(QString directory = "");

    void routeLoad();
    void consistEditorLoad();
    void restoreLastSession();
    void setNewRoute();
    void setLoadRoute();
    void cRecentEnabled(QString val);
    void rootPathEntered();
signals:
    void showMainWindow();
protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
private:
    void listRoutes();
    void updateStartupButtons(bool validRoot);
    bool readLastSession();
    QTableWidget routeList;
    QComboBox cRecent;
    QLabel rootStatusLabel;
    QPushButton *browse;
    QPushButton *load;
    QPushButton *neww;
    QPushButton *consistEditor;
    QPushButton *restoreLast;
    QPushButton *exit;
    QProcess *consistEditorProcess = NULL;

    
    
    QLineEdit *nowaTrasa;
    QWidget* nowa;
    bool newRoute = false;
    IghCoordinate* igh = NULL;
    LatitudeLongitudeCoordinate* latlon = NULL;
    PreciseTileCoordinate* aCoords = NULL;
    void downloadTemplateRoute(QString path);
    void listRoots();
    bool hasSelectedRoute() const;
};

#endif	/* LOADWINDOW_H */


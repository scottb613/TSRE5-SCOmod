/*
 *   New - manages opening and closing of windows for TSRE vs. leaving all this in Main
 *   hopefully will allow opening/closing windows without exiting and re-entering 
 * 
 *  */

#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QtCore>
#include <QFile>
#include <QTextStream>
#include <QPalette>
#include <QColor>
#include <QStringList>
#include <iostream>
#include "Game.h"
#include "RouteEditorWindow.h"
#include "LoadWindow.h"
#include "CELoadWindow.h"
#include "ShapeViewerWindow.h"
#include "MapWindow.h"
#include "RouteEditorServer.h"
#include "RouteEditorClient.h"
#include "Undo.h"

WindowManager::WindowManager(){};

void WindowManager::LoadConEditor(){
    CELoadWindow* ceLoadWindow = new CELoadWindow();
    
            //// EFO Try to keep window on main window:
        const QScreen* primaryScreen = QApplication::primaryScreen();
        const QSize windowSize = ceLoadWindow->size();
   
        // Calculate the centered position based on both monitors
        const QRect primaryGeometry = primaryScreen->geometry();
        const QPoint centeredPos((primaryGeometry.width() - windowSize.width()) / 2,
                                 (primaryGeometry.height() - windowSize.height()) / 2);
        
        if(Game::debugOutput) qDebug() << "Primary: " << primaryGeometry.width() << "/" << primaryGeometry.height();
        if(Game::debugOutput) qDebug() << "Window: " << windowSize.width() << "/" << windowSize.height();
        
        if(Game::debugOutput) qDebug() << "Window   Orig: " << ceLoadWindow->pos() ;
        
        // Ensure the window stays within the primary monitor bounds
        ceLoadWindow->move(centeredPos.x() >= 0 ? centeredPos.x() : 0,
                    centeredPos.y() >= 0 ? centeredPos.y() : 0);
 
        if(Game::debugOutput) qDebug() << "Window Center: " << ceLoadWindow->pos() ;        
            
        QStringList winPos = Game::mainPos.split(","); 
        if(winPos.count() > 1) ceLoadWindow->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
        
        if(Game::debugOutput) qDebug() << "Window  Final: " << ceLoadWindow->pos() ;        

    ceLoadWindow->show();
}

void WindowManager::LoadShapeViewer(QString arg){
    ShapeViewerWindow* shapeWindow = new ShapeViewerWindow();
    if(arg.length() > 0)
        shapeWindow->loadFile(arg);

        //// EFO Try to keep window on main window:
        const QScreen* primaryScreen = QApplication::primaryScreen();
        const QSize windowSize = shapeWindow->size();

        // Calculate the centered position based on both monitors
        const QRect primaryGeometry = primaryScreen->geometry();
        const QPoint centeredPos((primaryGeometry.width() - windowSize.width()) / 2,
                                 (primaryGeometry.height() - windowSize.height()) / 2);
        
        if(Game::debugOutput) qDebug() << "Primary: " << primaryGeometry.width() << "/" << primaryGeometry.height();
        if(Game::debugOutput) qDebug() << "Window: " << windowSize.width() << "/" << windowSize.height();
        
        if(Game::debugOutput) qDebug() << "Window   Orig: " << shapeWindow->pos() ;
        
        // Ensure the window stays within the primary monitor bounds
        shapeWindow->move(centeredPos.x() >= 0 ? centeredPos.x() : 0,
                    centeredPos.y() >= 0 ? centeredPos.y() : 0);
 
        if(Game::debugOutput) qDebug() << "Window Center: " << shapeWindow->pos() ;        
    
    QStringList winPos = Game::mainPos.split(","); 
    if(winPos.count() > 1) shapeWindow->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());

    shapeWindow->show();
}

void WindowManager::LoadRouteEditor(){
    
    if(Game::serverLogin.length() > 0)
        Game::ServerMode = true;
    
    if(Game::ServerMode){
        Game::useQuadTree = true;
        Undo::UndoEnabled = false;
        // Create Server Client
        Game::serverClient = new RouteEditorClient();
    }
    
    RouteEditorWindow *window = new RouteEditorWindow();
    if(Game::fullscreen){
        window->setWindowFlags(Qt::CustomizeWindowHint);
        window->setWindowState(Qt::WindowMaximized);
    } else {

        window->resize(1280, 800);
        
        //// EFO Try to keep window on main window:
        const QScreen* primaryScreen = QApplication::primaryScreen();
        const QSize windowSize = window->size();

        
       
        // Calculate the centered position based on both monitors
        const QRect primaryGeometry = primaryScreen->geometry();
        const QPoint centeredPos((primaryGeometry.width() - windowSize.width()) / 2,
                                 (primaryGeometry.height() - windowSize.height()) / 2);
        
        if(Game::debugOutput) qDebug() << "Primary: " << primaryGeometry.width() << "/" << primaryGeometry.height();
        if(Game::debugOutput) qDebug() << "Window: " << windowSize.width() << "/" << windowSize.height();
        
        if(Game::debugOutput) qDebug() << "Window   Orig: " << window->pos() ;
        
        // Ensure the window stays within the primary monitor bounds
        window->move(centeredPos.x() >= 0 ? centeredPos.x() : 0,
                    centeredPos.y() >= 0 ? centeredPos.y() : 0);
 
        if(Game::debugOutput) qDebug() << "Window Center: " << window->pos() ;        
        
    
                
        QStringList winPos = Game::mainPos.split(","); 
        if(winPos.count() > 1) window->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
        
        if(Game::debugOutput) qDebug() << "Window  Final: " << window->pos() ;        
        
        

    }
        
    if(!Game::ServerMode){
        LoadWindow *loadWindow = new LoadWindow();
        QObject::connect(window, SIGNAL(exitNow()), loadWindow, SLOT(exitNow()));
        QObject::connect(loadWindow, SIGNAL(showMainWindow()), window, SLOT(showRoute()));

        if(Game::checkRoot(Game::root) && (Game::checkRoute(Game::route) || Game::createNewRoutes)){
            window->showRoute();
        } else {
            // EFO moving to the primary screen to match the main window
            QStringList winPos = Game::mainPos.split(","); 
            if(winPos.count() > 1) loadWindow->move( winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
    
            loadWindow->show(); /// EFO moved inside bracket to fix double-window-load 8.002
        }
            
            

        
    } else {
        QObject::connect(Game::serverClient, SIGNAL(loadRoute()), window, SLOT(showRoute()));
        Game::serverClient->connectNow();
    }
}


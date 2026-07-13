/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QtCore>
#include <QFile>
#include <QTextStream>
#include <QPalette>
#include <QColor>
#include <QStringList>
#include <QSharedMemory>
#include <QMessageBox>
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

QFile logFile;
QTextStream logFileOut;

    
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg){
    const char symbols[] = { 'I', 'E', '!', 'X' };
    QString output = QString("[%1] %2").arg( symbols[type] ).arg( msg );
    if(Game::consoleOutput)
        std::cout << output.toStdString() << std::endl;
    logFileOut << output << "\n";
    logFileOut.flush();
    logFile.flush(); 
    
    if( type == QtFatalMsg ) abort(); 
}

void LoadConEditor(){
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

void LoadShapeViewer(QString arg){
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

void LoadRouteEditor(){
    
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

void RunRouteEditorServer(){
    Game::loadAllWFiles = true;
    Game::gui = false;
    RouteEditorServer *server = new RouteEditorServer();
    //..server->run();
}

enum CommandLineParseResult {
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

QHash<QString, QString> consoleArgs;

CommandLineParseResult parseCommandLineArgs(QCommandLineParser &parser){
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();
    const QCommandLineOption ServerIpOption("ip", "Server IP address.", "ip");
    parser.addOption(ServerIpOption);
    const QCommandLineOption ServerPortOption("port", "Server Port.", "port");
    parser.addOption(ServerPortOption);
    const QCommandLineOption FileOption("file", "Optional file to load with shapeview or play.", "file");
    parser.addOption(FileOption);
    const QCommandLineOption ShapeViewOption("shapeview", "Run ShapeViewer.");
    parser.addOption(ShapeViewOption);
    const QCommandLineOption RouteOption("route", "Route to run.", "file");
    parser.addOption(RouteOption);
    const QCommandLineOption AceConvOption("aceconv", "Run Ace Converter.");
    parser.addOption(AceConvOption);
    /// EFO New Option
    const QCommandLineOption RouteEditOption("routeedit", "Run Route Editor.");
    parser.addOption(RouteEditOption);
    const QCommandLineOption ConEditOption("conedit", "Run Consist Editor.");
    parser.addOption(ConEditOption);
    const QCommandLineOption PlayOption("play", "Play Activity.");
    parser.addOption(PlayOption);
    const QCommandLineOption ServerOption("server", "Run Editor Server.");
    parser.addOption(ServerOption);
    
    if (!parser.parse(QCoreApplication::arguments())) {
        return CommandLineError;
    }
    
    
    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(ServerIpOption)) {
        const QString ip = parser.value(ServerIpOption);
        consoleArgs["IP"] = ip;
    }
    if (parser.isSet(ServerPortOption)) {
        const QString port = parser.value(ServerPortOption);
        consoleArgs["PORT"] = port;
    }
    if (parser.isSet(RouteOption)) {
        const QString route = parser.value(RouteOption);
        consoleArgs["ROUTE"] = route;
    }
    if (parser.isSet(FileOption)) {
        const QString file = parser.value(FileOption);
        consoleArgs["FILENAME"] = file;
    }
    
    if (parser.isSet(ShapeViewOption)) {
        consoleArgs["SV"] = "TRUE";
    }
    if (parser.isSet(AceConvOption)) {
        consoleArgs["ACE"] = "TRUE";
    }
    if (parser.isSet(ConEditOption)) {
        consoleArgs["CON"] = "TRUE";
    }
    
    /// EFO New Option
    if (parser.isSet(RouteEditOption)) {
        consoleArgs["RE"] = "TRUE";
    }
    if (parser.isSet(PlayOption)) {
        consoleArgs["PLAY"] = "TRUE";
    }
    if (parser.isSet(ServerOption)) {
        consoleArgs["SERVER"] = "TRUE";
    }
    
    return CommandLineOk;
}

int main(int argc, char *argv[]){

   // #ifdef  Q_OS_WIN32 
   //     ::ShowWindow( ::GetConsoleWindow(), SW_HIDE ); //hide console window
   // #endif


    /// set the version here to avoid changing Game.cpp so much
//    Game::AppVersion = "v8.005a";
    
    // EFO set log to date/time so it isn't overwritten
    logFile.setFileName("tsre-log-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmm") + ".txt");
    
    logFile.open(QIODevice::WriteOnly);
    logFileOut.setDevice(&logFile);
    qInstallMessageHandler( myMessageOutput );

    
    QLocale lepsze(QLocale::English);
    //loc.setNumberOptions(lepsze.numberOptions());
    QLocale::setDefault(lepsze);
        
    QSurfaceFormat format;
#ifdef __APPLE__
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
    //format.setDepthBufferSize(32);
    //format.setStencilBufferSize(8);
    format.setSamples(Game::AASamples);
    //format.set
    format.setSwapInterval(0);
    //format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setApplicationName(Game::AppName);
    QApplication::setApplicationVersion(Game::AppVersion);
    //QApplication::pr
    QApplication app(argc, argv);

    QSharedMemory singleInstance("TSRE5-single-instance");
    if(!singleInstance.create(1)){
        QMessageBox::warning(NULL, "TSRE5 already running", "TSRE5 is already running.");
        return 0;
    }
    
    QString workingDir = QDir::currentPath();

    qSetMessagePattern("%{file}:%{function}:%{line}: \t%{message}");

        
    if(!Game::UseWorkingDir){
        QDir::setCurrent(QCoreApplication::applicationDirPath());
    }
    
    Game::load();    
    if(Game::debugOutput) qDebug() << "workingDir" << workingDir;
    
    QCommandLineParser parser;
    switch (parseCommandLineArgs(parser)) {
        case CommandLineOk:
            break;
        case CommandLineError:
            return 1;
        case CommandLineVersionRequested:
            printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
                   qPrintable(QCoreApplication::applicationVersion()));
            return 0;
        case CommandLineHelpRequested:
            parser.showHelp();
            Q_UNREACHABLE();
    }

    //app.set
    Game::PixelRatio = app.devicePixelRatio();
    if(Game::debugOutput) qDebug() << "devicePixelRatio"<< app.devicePixelRatio();

    app.setStyle(QStyleFactory::create("Fusion"));

    if(!Game::systemTheme){
        //app.setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53,53,53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25,25,25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53,53,53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(240, 130, 0));
        darkPalette.setColor(QPalette::Highlight, QColor(240, 130, 0));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text , QColor(153,153,153));
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText , QColor(153,153,153));
        app.setPalette(darkPalette);
        app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
        app.setStyleSheet("QPushButton:checked{background-color: #666666;} ");
        Game::StyleMainLabel = "#c4a480";
        Game::StyleGreenButton = "#008800";
        Game::StyleRedButton = "#880000";
        Game::StyleYellowButton = "#888800";
        Game::StyleGreenText = "#55FF55";
        Game::StyleRedText = "#FF5555";
    } else {
        QPalette palette = app.palette();
//        palette.setColor(QPalette::Disabled, QPalette::Text , QColor(160,90,64));
//        palette.setColor(QPalette::Disabled, QPalette::WindowText , QColor(160,90,64));
//        palette.setColor(QPalette::Highlight, QColor(160, 90, 64));
//        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::white);
//        app.setPalette(palette);
//        app.setStyleSheet("QPushButton:checked{background-color: #e0c0a4;} ");
//        Game::StyleMainLabel = "#c4a480";
//        Game::StyleGreenButton = "#008800";
//        Game::StyleRedButton = "#880000";
//        Game::StyleYellowButton = "#888800";
//        Game::StyleGreenText = "#55FF55";
//        Game::StyleRedText = "#FF5555";

    }

    Game::InitAssets();
    
    //Game::window.resize(1280, 720);
    //window.resize(window.sizeHint());
     
    //int desktopArea = QApplication::desktop()->width() *
    //                  QApplication::desktop()->height();
    //int widgetArea = window.width() * window.height();
    //if (((float)widgetArea / (float)desktopArea) < 1.0f)
    //    window.show();
    //else
    //    window.showMaximized();
    
    //Check if file opened with "open in TSRE"
    QStringList args = app.arguments();
    if(args.count() == 2){
        if(QFileInfo::exists(args[1])){
            consoleArgs["SV"] = "TRUE";
            consoleArgs["FILENAME"] = args[1];
        }
    }
    //////////////////////////////////////////
    //qDebug() << "arg1 " << args[1];    
    if(consoleArgs["ROUTE"].length() > 0){
        Game::route = consoleArgs["ROUTE"];
    }
    if(consoleArgs["IP"].length() > 0){
        RouteEditorServer::IP = consoleArgs["IP"];
    }
    if(consoleArgs["PORT"].length() > 0){
        RouteEditorServer::Port = consoleArgs["PORT"].toInt();
        qDebug() << RouteEditorServer::Port ;
    }
    
    if(consoleArgs["ACE"] == "TRUE"){
        // Run ace converter
        qDebug() << "Run ace converter";
        return app.exec();
    }
    
    /// EFO New Option
    if(consoleArgs["RE"] == "TRUE"){
        qDebug() << "Run route editor";
        LoadRouteEditor();
        return app.exec();
    }
    
    if(consoleArgs["CON"] == "TRUE"){
        // Run ace converter
        qDebug() << "Run con editor";
        LoadConEditor();
        return app.exec();
    }

    if(consoleArgs["SV"] == "TRUE"){
        // Run ace converter
        qDebug() << "Run shape viewer";
        LoadShapeViewer(consoleArgs["FILENAME"]);
        return app.exec();
    }
    if(consoleArgs["PLAY"] == "TRUE"){
        // Play
        if(consoleArgs["FILENAME"].length() > 0)
            Game::ActivityToPlay = consoleArgs["FILENAME"];
        else
            Game::ActivityToPlay = "#";
        qDebug() << "Play" << Game::route << Game::ActivityToPlay;
    }
    if(consoleArgs["SERVER"] == "TRUE"){
        Game::checkRoute(Game::route);
        qDebug() << "Run server";
        RunRouteEditorServer();
        return app.exec();
    }
        
    /// EFO New Setting 
    if(Game::startapp.contains("c")) 
        LoadConEditor();
    else if(Game::startapp.contains("s")) 
        LoadShapeViewer("");
    else if(Game::startapp.contains("r")) 
        LoadRouteEditor();   
    else LoadRouteEditor();
    
    // LoadConEditor();

    //MapWindow aaa;
    //aaa.show();
    return app.exec();
 }

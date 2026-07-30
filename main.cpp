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
#include <QProcess>
#include <QMessageBox>
#include <QFontInfo>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QMenu>
#include <QRadioButton>
#include <iostream>
#include "Game.h"
#include "RouteEditorWindow.h"
#include "LoadWindow.h"
#include "ConEditorWindow.h"
#include "ShapeViewerWindow.h"
#include "MapWindow.h"
#include "RouteEditorServer.h"
#include "RouteEditorClient.h"
#include "Undo.h"
#include "GuiFunct.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

class UiInteractionSoundFilter : public QObject {
public:
    explicit UiInteractionSoundFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if(QAbstractButton *button = qobject_cast<QAbstractButton*>(watched)){
            if((qobject_cast<QCheckBox*>(button) || qobject_cast<QRadioButton*>(button))
                    && !button->property("scoUiSoundConnected").toBool()){
                button->setProperty("scoUiSoundConnected", true);
                QObject::connect(button, &QAbstractButton::clicked, this, [](){
                    play("SCOtic.wav");
                });
            }
        } else if(QComboBox *combo = qobject_cast<QComboBox*>(watched)){
            if(!combo->property("scoUiSoundConnected").toBool()){
                combo->setProperty("scoUiSoundConnected", true);
                QObject::connect(combo,
                                 static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                                 this, [](int){ play("SCOtic.wav"); });
            }
        } else if(QMenu *menu = qobject_cast<QMenu*>(watched)){
            if(!menu->property("scoUiSoundConnected").toBool()){
                menu->setProperty("scoUiSoundConnected", true);
                QObject::connect(menu, &QMenu::triggered, this, [](QAction *action){
                    if(action && action->isEnabled() && !action->isSeparator())
                        play("SCOtic.wav");
                });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void play(const QString &fileName) {
        if(!Game::scoSoundEnabled)
            return;
#ifdef Q_OS_WIN
        const QString soundPath = QCoreApplication::applicationDirPath() + "/content/" + fileName;
        if(QFile::exists(soundPath)){
            ::PlaySoundW(NULL, NULL, 0);
            ::PlaySoundW(reinterpret_cast<const wchar_t*>(soundPath.utf16()), NULL,
                         SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        }
#else
        Q_UNUSED(fileName);
#endif
    }
};

QFile logFile;
QTextStream logFileOut;
QHash<QString, QString> consoleArgs;

    
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg){
    char symbol = '?';
    switch(type){
        case QtDebugMsg:    symbol = 'D'; break;
        case QtInfoMsg:     symbol = 'I'; break;
        case QtWarningMsg:  symbol = 'W'; break;
        case QtCriticalMsg: symbol = '!'; break;
        case QtFatalMsg:    symbol = 'X'; break;
    }
    QString output = QString("[%1] %2").arg(symbol).arg(msg);
    if(Game::consoleOutput)
        std::cout << output.toStdString() << std::endl;
    logFileOut << output << "\n";
    logFileOut.flush();
    logFile.flush(); 
    
    if( type == QtFatalMsg ) abort(); 
}

void LoadConsistEditorFromMainLoad(const QString &root){
    Game::root = QDir::cleanPath(root);
    if(!Game::checkCERoot(Game::root)){
        QMessageBox::critical(NULL, "Consist Editor",
                              "The selected Train Simulator folder does not contain "
                              "TRAINS, TRAINSET, and CONSISTS.");
        QCoreApplication::quit();
        return;
    }

    ConEditorWindow *window = new ConEditorWindow();
    window->showMaximized();
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

static RouteEditorWindow *createRouteEditorWindow(){
    RouteEditorWindow *window = new RouteEditorWindow();
    if(Game::fullscreen){
        window->setWindowFlags(Qt::CustomizeWindowHint);
        window->setWindowState(Qt::WindowMaximized);
        return window;
    }

    window->resize(1280, 800);
    const QScreen *primaryScreen = QApplication::primaryScreen();
    if(primaryScreen != NULL){
        const QSize windowSize = window->size();
        const QRect primaryGeometry = primaryScreen->geometry();
        const QPoint centeredPos(
            primaryGeometry.left() + (primaryGeometry.width() - windowSize.width()) / 2,
            primaryGeometry.top() + (primaryGeometry.height() - windowSize.height()) / 2);
        window->move(centeredPos);

        if(Game::debugOutput){
            qDebug() << "Primary:" << primaryGeometry.width()
                     << "/" << primaryGeometry.height();
            qDebug() << "Window:" << windowSize.width() << "/" << windowSize.height();
            qDebug() << "Window Center:" << window->pos();
        }
    }

    QStringList winPos = Game::mainPos.split(",");
    const bool mainDefaultRequested =
            Game::pinnedWindowPosition("mainWindowUseDefault", NULL);
    if(!mainDefaultRequested && winPos.count() > 1)
        window->move(winPos[0].trimmed().toInt(), winPos[1].trimmed().toInt());
    window->applyPinnedMainWindowPosition();

    if(Game::debugOutput)
        qDebug() << "Window Final:" << window->pos();
    return window;
}

void LoadRouteEditor(){
    if(Game::serverLogin.length() > 0)
        Game::ServerMode = true;

    if(Game::ServerMode){
        Game::useQuadTree = true;
        Undo::UndoEnabled = false;
        Game::serverClient = new RouteEditorClient();
        RouteEditorWindow *window = createRouteEditorWindow();
        QObject::connect(Game::serverClient, SIGNAL(loadRoute()), window, SLOT(showRoute()));
        Game::serverClient->connectNow();
        return;
    }

    // Build the startup screen first. Constructing the full Route Editor here
    // used to create native OpenGL/tool windows before Load was visible, which
    // caused a brief small-window flash at application startup.
    LoadWindow *loadWindow = new LoadWindow();
    QSharedPointer<QPointer<RouteEditorWindow> > windowHolder(
        new QPointer<RouteEditorWindow>());
    const auto showRouteEditor = [loadWindow, windowHolder](){
        RouteEditorWindow *window = windowHolder->data();
        if(window == NULL){
            window = createRouteEditorWindow();
            *windowHolder = window;
            QObject::connect(window, SIGNAL(returnToLoadWindow()),
                             loadWindow, SLOT(show()));
        }
        window->showRoute();
    };
    QObject::connect(loadWindow, &LoadWindow::showMainWindow,
                     loadWindow, showRouteEditor);

    if(consoleArgs["RESTORE"] == "TRUE"){
        loadWindow->restoreLastSession();
    } else if(Game::checkRoot(Game::root)
              && (Game::checkRoute(Game::route) || Game::createNewRoutes)){
        showRouteEditor();
    } else {
        loadWindow->show();
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
    const QCommandLineOption RestoreLastSessionOption("restore-last-session", "Restore the saved route editor session after startup.");
    parser.addOption(RestoreLastSessionOption);
    const QCommandLineOption RootOption(
                "root", "Train Simulator root folder.", "folder");
    parser.addOption(RootOption);
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
    if (parser.isSet(RootOption)) {
        consoleArgs["ROOT"] = parser.value(RootOption);
    }
    
    /// EFO New Option
    if (parser.isSet(RouteEditOption)) {
        consoleArgs["RE"] = "TRUE";
    }
    if (parser.isSet(RestoreLastSessionOption)) {
        consoleArgs["RESTORE"] = "TRUE";
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

static int finishApplication(QApplication &app, QSharedMemory &singleInstance){
    const int result = app.exec();
    if(result == Game::RestartAndRestoreExitCode){
        singleInstance.detach();
        if(!QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    QStringList() << "--restore-last-session")){
            QMessageBox::critical(NULL, "Restart and Restore",
                                  "TSRE could not start the replacement process. Please start TSRE manually and choose Restore Last Session.");
            return 1;
        }
        return 0;
    }
    return result;
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
    GuiFunct::installImportantDialogCentering();

    // Main Load starts CE with a process-local environment value rather than
    // a public command-line mode. This keeps CE isolated without providing a
    // standalone launcher path.
    const QString mainLoadConsistRoot =
            qEnvironmentVariable("TSRE_MAIN_LOAD_CONSIST_ROOT");
    const bool mainLoadConsistEditor = !mainLoadConsistRoot.isEmpty();
    const QString instanceKey = mainLoadConsistEditor
            ? QString("TSRE5-consist-editor-%1")
                  .arg(QCoreApplication::applicationPid())
            : QString("TSRE5-single-instance");
    QSharedMemory singleInstance(instanceKey);
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
    UiInteractionSoundFilter uiInteractionSounds(&app);
    app.installEventFilter(&uiInteractionSounds);
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
        darkPalette.setColor(QPalette::ToolTipBase, QColor(37,37,37));
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
        app.setStyleSheet(
            "QToolTip { color: #ffffff; background-color: #252525;"
            " border: 1px solid #777777; padding: 3px; }"
            "QPushButton:checked { color: #232323; background-color: #b47a3b; }");
        Game::StyleMainLabel = "#c4a480";
        Game::StyleGreenButton = "#4b9b5d";
        Game::StyleGreenButtonHover = "#65b778";
        Game::StyleBlueButton = "#4788b5";
        Game::StyleBlueButtonHover = "#61a2cf";
        Game::StyleOrangeButton = "#b47a3b";
        Game::StyleOrangeButtonHover = "#ce9454";
        Game::StyleRedButton = "#a95050";
        Game::StyleRedButtonHover = "#c46868";
        Game::StyleYellowButton = "#b59b4c";
        Game::StyleYellowButtonHover = "#cfb765";
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

    QFont appFont = app.font();
    appFont.setFamily("Segoe UI");
    qreal appPointSize = QFontInfo(appFont).pointSizeF();
    if(appPointSize <= 0)
        appPointSize = 9.0;
    appFont.setPointSizeF(appPointSize * Game::uiScale);
    app.setFont(appFont);

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
    if(consoleArgs["ROOT"].length() > 0){
        Game::root = consoleArgs["ROOT"];
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
        return finishApplication(app, singleInstance);
    }
    
    /// EFO New Option
    if(consoleArgs["RE"] == "TRUE"){
        qDebug() << "Run route editor";
        LoadRouteEditor();
        return finishApplication(app, singleInstance);
    }
    
    if(mainLoadConsistEditor){
        qDebug() << "Run Main Load consist editor";
        LoadConsistEditorFromMainLoad(mainLoadConsistRoot);
        return finishApplication(app, singleInstance);
    }

    if(consoleArgs["SV"] == "TRUE"){
        // Run ace converter
        qDebug() << "Run shape viewer";
        LoadShapeViewer(consoleArgs["FILENAME"]);
        return finishApplication(app, singleInstance);
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
        return finishApplication(app, singleInstance);
    }
        
    LoadRouteEditor();

    //MapWindow aaa;
    //aaa.show();
    return finishApplication(app, singleInstance);
 }

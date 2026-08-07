/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "LoadWindow.h"
#include <QtWidgets>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "Game.h"
#include <QDebug>
#include "NewRouteWindow.h"
#include "GeoCoordinates.h"
#include "TarFile.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

static void playLoadWindowSound(const QString &fileName, bool alwaysPlay = false){
    if(!alwaysPlay && !Game::scoSoundEnabled)
        return;
#ifdef Q_OS_WIN
    QString soundPath = QCoreApplication::applicationDirPath() + "/content/" + fileName;
    if(QFile::exists(soundPath)){
        ::PlaySoundW(NULL, NULL, 0);
        ::PlaySoundW(reinterpret_cast<const wchar_t*>(soundPath.utf16()), NULL,
                     SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
#else
    Q_UNUSED(fileName);
#endif
}

static QString launcherButtonStyle(const QString& accent, const QString& hoverAccent){
    return QString(
        "QPushButton { color: #eeeeee;"
        " background-color: #34373a;"
        " border: 1px solid #50555a; border-left: 3px solid %1;"
        " border-radius: 2px; padding: 1px 5px; }"
        "QPushButton:hover { background-color: #404448;"
        " border-color: #6c7278; border-left-color: %2; }"
        "QPushButton:pressed {"
        " background-color: #292c2f; border-color: #454a4f;"
        " border-left-color: %1;"
        " padding-top: 2px; padding-bottom: 0px; }"
        "QPushButton:disabled { color: #858585; background-color: #303235;"
        " border-color: #45484b; border-left-color: #55585b; }"
    ).arg(accent, hoverAccent);
}

LoadWindow::LoadWindow() {
    //this->setWindowFlags( Qt::CustomizeWindowHint );
    // The persistent launcher explicitly owns application shutdown. Editor
    // windows may close while this screen is hidden without ending the app.
    QApplication::setQuitOnLastWindowClosed(false);
    setWindowTitle(Game::AppName+" "+Game::AppVersion+" Route Editor");
    this->setFixedSize(600, 700);
    QImage* myImage = new QImage();
    myImage->load(Game::sessionSplashImagePath());

    QLabel* myLabel = new QLabel("");
    myLabel->setContentsMargins(0,0,0,0);
    myLabel->setFixedSize(600, 200);
    QLabel* myLabel2 = new QLabel("MSTS/ORTS Root Folder:");
    myLabel2->setContentsMargins(5,0,0,0);
    myLabel2->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }").arg(Game::StyleMainLabel));
    QLabel* myLabel3 = new QLabel("Select route above or enter name for new route: ");
    myLabel3->setContentsMargins(5,0,0,0);
    
    myLabel->setPixmap(QPixmap::fromImage(*myImage).scaled(myLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    browse = new QPushButton("Browse...");
    browse->setMinimumHeight(24);
    browse->setFixedWidth(90);
    connect(browse, SIGNAL (released()), this, SLOT (handleBrowseButton()));
    load = new QPushButton("Load");
    load->setMinimumHeight(24);
    const QString greenButton = launcherButtonStyle(
        Game::StyleGreenButton, Game::StyleGreenButtonHover);
    const QString blueButton = launcherButtonStyle(
        Game::StyleBlueButton, Game::StyleBlueButtonHover);
    const QString orangeButton = launcherButtonStyle(
        Game::StyleOrangeButton, Game::StyleOrangeButtonHover);
    const QString redButton = launcherButtonStyle(
        Game::StyleRedButton, Game::StyleRedButtonHover);
    load->setStyleSheet(greenButton);
    connect(load, SIGNAL (released()), this, SLOT (routeLoad()));
    neww = new QPushButton("New Route");
    neww->setMinimumHeight(24);
    neww->setStyleSheet(orangeButton);
    connect(neww, SIGNAL (released()), this, SLOT (setNewRoute()));
    restoreLast = new QPushButton("Restore");
    restoreLast->setMinimumHeight(24);
    restoreLast->setStyleSheet(greenButton);
    connect(restoreLast, SIGNAL (released()), this, SLOT (restoreLastSession()));
    // Adapted from Eric-from-Trainsim's idea of launching the Consist Editor
    // directly from the selected Train Simulator root.
    consistEditor = new QPushButton("Consist Editor");
    consistEditor->setMinimumHeight(24);
    consistEditor->setStyleSheet(blueButton);
    connect(consistEditor, SIGNAL (released()), this, SLOT (consistEditorLoad()));
    exit = new QPushButton("Exit");
    exit->setMinimumHeight(24);
    exit->setStyleSheet(redButton);

    const QList<QPushButton*> startupButtons = { browse, restoreLast, consistEditor, neww, exit };
    for(QPushButton *button : startupButtons){
        connect(button, &QPushButton::pressed, this, [](){
            playLoadWindowSound("SCOpress.wav", true);
        });
    }
    connect(load, &QPushButton::pressed, this, [this](){
        if(!newRoute && !hasSelectedRoute()){
            playLoadWindowSound("SCObuzz.wav");
            return;
        }
        playLoadWindowSound("SCOpress.wav", true);
    });

    

    nowaTrasa = new QLineEdit();
    QRegularExpression rx("^[a-zA-Z0-9_\\- ]*$");
    QRegularExpressionValidator* v = new QRegularExpressionValidator(rx);
    nowaTrasa->setValidator(v);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(myLabel);
    mainLayout->addWidget(myLabel2);
    QHBoxLayout *rootLayout = new QHBoxLayout;
    rootLayout->setSpacing(3);
    rootLayout->setContentsMargins(1,0,1,0);
    cRecent.setMaxVisibleItems(10);
    cRecent.setEditable(true);
    cRecent.setInsertPolicy(QComboBox::NoInsert);
    cRecent.setMinimumHeight(24);
    cRecent.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    cRecent.setStyleSheet(
        "QComboBox { combobox-popup: 0; color: white; font-weight: normal; }"
        "QComboBox QLineEdit { color: white; font-weight: normal; }");
    cRecent.lineEdit()->setPlaceholderText("Select or browse to the Train Simulator root folder");
    QObject::connect(&cRecent, SIGNAL(textActivated(QString)),
                      this, SLOT(cRecentEnabled(QString)));
    QObject::connect(cRecent.lineEdit(), SIGNAL(returnPressed()),
                      this, SLOT(rootPathEntered()));
    rootLayout->addWidget(&cRecent, 1);
    rootLayout->addWidget(browse);
    mainLayout->addItem(rootLayout);
    rootStatusLabel.setContentsMargins(5,0,5,0);
    rootStatusLabel.setWordWrap(true);
    rootStatusLabel.hide();
    mainLayout->addWidget(&rootStatusLabel);
    routeList.setColumnCount(2);
    routeList.setHorizontalHeaderLabels(QStringList() << "Route" << "Last Modified");
    routeList.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    routeList.verticalHeader()->setVisible(false);
    mainLayout->addWidget(&routeList);
    
    /*nowa = new QWidget();
    QHBoxLayout *vbox1 = new QHBoxLayout;
    vbox1->addWidget(myLabel3);
    vbox1->addWidget(nowaTrasa);
    vbox1->setContentsMargins(0,0,0,0);
    nowa->setLayout(vbox1);
    mainLayout->addWidget(nowa);*/
    
    QWidget* box = new QWidget();
    QVBoxLayout *buttonRows = new QVBoxLayout;
    QHBoxLayout *primaryButtons = new QHBoxLayout;
    primaryButtons->addWidget(load, 2);
    primaryButtons->addWidget(restoreLast, 2);
    primaryButtons->addWidget(exit, 1);
    primaryButtons->setSpacing(4);
    primaryButtons->setContentsMargins(0,0,0,0);
    QHBoxLayout *secondaryButtons = new QHBoxLayout;
    secondaryButtons->addWidget(consistEditor, 3);
    secondaryButtons->addWidget(neww, 1);
    secondaryButtons->setSpacing(4);
    secondaryButtons->setContentsMargins(0,0,0,0);
    buttonRows->addLayout(primaryButtons);
    buttonRows->addLayout(secondaryButtons);
    buttonRows->setSpacing(4);
    buttonRows->setContentsMargins(0,0,0,0);
    box->setLayout(buttonRows);
    mainLayout->addWidget(box);
    
    mainLayout->setAlignment(myLabel, Qt::AlignTop);
    mainLayout->setAlignment(myLabel2, Qt::AlignTop);
    mainLayout->setAlignment(load, Qt::AlignTop);
    mainLayout->setAlignment(box, Qt::AlignBottom);
    //mainLayout->setAlignment(nowa, Qt::AlignBottom);
    mainLayout->setContentsMargins(1,1,1,1);
    //mainLayout->addWidget(naviBox);
    this->setLayout(mainLayout);
            
    
    //nowaTrasa->hide();

    QObject::connect(exit, SIGNAL (released()), this, SLOT(close()));
    QObject::connect(&routeList, SIGNAL(itemClicked(QTableWidgetItem*)),
                      this, SLOT(setLoadRoute()));
    //QObject::connect(nowaTrasa, SIGNAL(textChanged(QString)),
    //                  this, SLOT(setNewRoute()));
    
 QObject::connect(&routeList, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
                      this, SLOT(routeLoad()));
    
    listRoots();
    
    if(Game::checkRoot(Game::root)){
        qDebug()<<"ok";
        updateStartupButtons(true);
        cRecent.setCurrentText(Game::root);
        cRecent.setToolTip(Game::root);
        rootStatusLabel.hide();
        this->listRoutes();
    } else {
        updateStartupButtons(false);
        if(!Game::root.trimmed().isEmpty()){
            cRecent.setCurrentText(Game::root);
            cRecent.setStyleSheet(QString(
                "QComboBox { combobox-popup: 0; color: %1; font-weight: normal; }"
                "QComboBox QLineEdit { color: %1; font-weight: normal; }").arg(Game::StyleRedText));
            rootStatusLabel.setText("Invalid root folder. Choose the Train Simulator folder containing GLOBAL and ROUTES.");
            rootStatusLabel.setStyleSheet(QString("QLabel { color: %1; }").arg(Game::StyleRedText));
            rootStatusLabel.show();
        }
    }
}

void LoadWindow::showEvent(QShowEvent *event){
    QWidget::showEvent(event);
    const bool validRoot = Game::checkRoot(Game::root);
    updateStartupButtons(validRoot);
    if(validRoot){
        newRoute = false;
        load->setText("Load");
        cRecent.setCurrentText(Game::root);
        cRecent.setToolTip(Game::root);
        listRoutes();
    }

    QScreen *screen = QApplication::primaryScreen();
    if(screen == NULL)
        return;
    const QRect available = screen->availableGeometry();
    move(available.left() + (available.width() - width()) / 2,
         available.top() + (available.height() - height()) / 2);
}





void LoadWindow::handleBrowseButton(QString directory){
    if(directory == ""){
        QString startDirectory = Game::checkRoot(Game::root) ? Game::root : QDir::homePath();
        directory = QFileDialog::getExistingDirectory(
            this, "Select MSTS/ORTS Root Folder", startDirectory,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(directory.isEmpty())
            return;
    }

    directory = directory.trimmed();
    cRecent.setCurrentText(directory);
    cRecent.setToolTip(directory);
    updateStartupButtons(false);
    routeList.clearContents();
    routeList.setRowCount(0);
    if(Game::checkRoot(directory)){
        qDebug()<<"ok";
        Game::root = directory;
        updateStartupButtons(true);
        cRecent.setStyleSheet(
            "QComboBox { combobox-popup: 0; color: white; font-weight: normal; }"
            "QComboBox QLineEdit { color: white; font-weight: normal; }");
        rootStatusLabel.hide();
        this->listRoutes();

        int existing = -1;
        for(int i = 0; i < cRecent.count(); i++){
            if(cRecent.itemText(i).compare(directory, Qt::CaseInsensitive) == 0){
                existing = i;
                break;
            }
        }
        if(existing >= 0)
            cRecent.removeItem(existing);
        cRecent.insertItem(0, directory);
        cRecent.setCurrentIndex(0);

        QString path;
        path = "cerecent.txt";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return;
        QTextStream in(&file);
        QString line;
        for(int i = 0; i < cRecent.count(); i++){
            in << cRecent.itemText(i) << "\n";
        }
        in.flush();
        file.close();
    } else {
        playLoadWindowSound("SCObuzz.wav");
        cRecent.setStyleSheet(QString(
            "QComboBox { combobox-popup: 0; color: %1; font-weight: normal; }"
            "QComboBox QLineEdit { color: %1; font-weight: normal; }").arg(Game::StyleRedText));
        rootStatusLabel.setText("Invalid root folder. Choose the Train Simulator folder containing GLOBAL and ROUTES.");
        rootStatusLabel.setStyleSheet(QString("QLabel { color: %1; }").arg(Game::StyleRedText));
        rootStatusLabel.show();
    }
}

void LoadWindow::closeEvent(QCloseEvent *event){
    event->accept();
    QCoreApplication::quit();
}

void LoadWindow::updateStartupButtons(bool validRoot){
    restoreLast->setEnabled(QFile::exists(Game::lastSessionFilePath()));
    restoreLast->show();
    exit->show();
    if(validRoot){
        load->show();
        neww->show();
        consistEditor->show();
        consistEditor->setEnabled(Game::checkCERoot(Game::root));
        consistEditor->setToolTip(consistEditor->isEnabled()
                                  ? "Open the Consist Editor using this Train Simulator folder"
                                  : "This folder does not contain TRAINS, TRAINSET, and CONSISTS");
        load->setMinimumWidth(0);
        neww->setMinimumWidth(0);
        restoreLast->setMinimumWidth(0);
        consistEditor->setMinimumWidth(0);
        exit->setMinimumWidth(0);
        load->setMaximumWidth(QWIDGETSIZE_MAX);
        neww->setMaximumWidth(QWIDGETSIZE_MAX);
        restoreLast->setMaximumWidth(QWIDGETSIZE_MAX);
        consistEditor->setMaximumWidth(QWIDGETSIZE_MAX);
        exit->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        load->hide();
        neww->hide();
        consistEditor->hide();
        restoreLast->setFixedWidth(300);
        exit->setFixedWidth(300);
    }
}

bool LoadWindow::readLastSession(){
    QFile file(Game::lastSessionFilePath());
    if(!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if(parseError.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonObject root = doc.object();
    QString routeRoot = root.value("root").toString();
    QString routeName = root.value("route").toString();
    if(routeRoot.isEmpty() || routeName.isEmpty())
        return false;

    if(!Game::checkRoot(routeRoot))
        return false;

    Game::root = routeRoot;
    if(!Game::checkRoute(routeName))
        return false;

    Game::route = routeName;

    QJsonObject mainWindow = root.value("mainWindow").toObject();
    if(!mainWindow.isEmpty()){
        Game::restoreLastSessionWindowGeometry = true;
        Game::restoreMainX = mainWindow.value("x").toInt();
        Game::restoreMainY = mainWindow.value("y").toInt();
        Game::restoreMainW = mainWindow.value("w").toInt();
        Game::restoreMainH = mainWindow.value("h").toInt();
        Game::restoreMainMaximized = mainWindow.value("maximized").toBool(false);
        Game::mainPos = QString::number(Game::restoreMainX)+","+QString::number(Game::restoreMainY);
    }

    QJsonObject statusWindow = root.value("controlPanel").toObject();
    if(statusWindow.isEmpty())
        statusWindow = root.value("statusWindow").toObject();
    if(!statusWindow.isEmpty()){
        Game::restoreStatusGeometry = true;
        Game::restoreStatusX = statusWindow.value("x").toInt();
        Game::restoreStatusY = statusWindow.value("y").toInt();
        Game::restoreStatusW = statusWindow.value("w").toInt();
        Game::restoreStatusH = statusWindow.value("h").toInt();
        Game::statusPos = QString::number(Game::restoreStatusX)+","+QString::number(Game::restoreStatusY);
    }

    QJsonObject camera = root.value("camera").toObject();
    if(!camera.isEmpty()){
        Game::restoreLastSessionCamera = true;
        Game::restoreCameraTileX = camera.value("tileX").toInt();
        Game::restoreCameraTileZ = camera.value("tileZ").toInt();
        Game::restoreCameraX = (float)camera.value("x").toDouble();
        Game::restoreCameraY = (float)camera.value("y").toDouble();
        Game::restoreCameraZ = (float)camera.value("z").toDouble();
        Game::restoreCameraRotX = (float)camera.value("rotX").toDouble();
        Game::restoreCameraRotY = (float)camera.value("rotY").toDouble();
    }

    return true;
}

void LoadWindow::restoreLastSession(){
    if(!readLastSession()){
        QMessageBox::warning(this, "Restore Last Session", "The last session could not be restored.");
        updateStartupButtons(Game::checkRoot(Game::root));
        return;
    }

    hide();
    emit showMainWindow();
}

void LoadWindow::routeLoad(){
    if(this->newRoute){
        if(!Game::checkRoot(Game::root)) return;
        Game::routeName = Game::route;
        Game::trkName = Game::route;
        Game::writeEnabled = true;
        Game::createNewRoutes = true;
        Game::writeTDB = true;
    }else{
        if(!hasSelectedRoute()) return;
        QTableWidgetItem *routeItem = routeList.item(routeList.currentRow(), 0);
        Game::route = routeItem->text();
        Game::checkRoute(Game::route);
    }
    qDebug() << Game::route;
    hide();
    emit showMainWindow();
}

bool LoadWindow::hasSelectedRoute() const{
    const int row = routeList.currentRow();
    return row >= 0 && routeList.item(row, 0) != NULL;
}

void LoadWindow::consistEditorLoad(){
    if(!Game::checkCERoot(Game::root))
        return;

    if(consistEditorProcess != NULL)
        return;

    consistEditorProcess = new QProcess(this);
    consistEditor->setEnabled(false);

    connect(consistEditorProcess, &QProcess::started, this, [this](){
        hide();
    });
    connect(consistEditorProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus){
        consistEditorProcess->deleteLater();
        consistEditorProcess = NULL;
        consistEditor->setEnabled(Game::checkCERoot(Game::root));
        show();
        raise();
        activateWindow();
    });
    connect(consistEditorProcess, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError error){
        if(error != QProcess::FailedToStart)
            return;
        QMessageBox::critical(this, "Consist Editor",
                              "TSRE could not start the Consist Editor process.");
        consistEditorProcess->deleteLater();
        consistEditorProcess = NULL;
        consistEditor->setEnabled(Game::checkCERoot(Game::root));
        show();
        raise();
        activateWindow();
    });

    consistEditorProcess->setProgram(QCoreApplication::applicationFilePath());
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("TSRE_MAIN_LOAD_CONSIST_ROOT", Game::root);
    consistEditorProcess->setProcessEnvironment(environment);
    consistEditorProcess->setArguments(QStringList());
    consistEditorProcess->start();
}

void LoadWindow::listRoutes(){
    QDir dir(Game::root+"/routes");
    dir.setFilter(QDir::Dirs);
    

    //// EFO this is new code to support multiple columns and clickable sorting
    
    routeList.setSortingEnabled(false);
    routeList.clearContents();
    routeList.setRowCount(0);

    // Set column headers
    routeList.setColumnCount(2);
    routeList.rowHeight(10);
    routeList.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    routeList.verticalHeader()->setVisible(false);
    
    QStringList headers = {"Route", "Last Modified"};
    routeList.setHorizontalHeaderLabels(headers);

    // Get directory entries
    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time );

    // Populate table with directory information
    for (const QFileInfo& entry : entries) {
        // Windows shell shortcuts to folders may be returned by QDir as
        // directories. They are navigation aids, not MSTS/ORTS routes.
        if(entry.fileName().endsWith(".lnk", Qt::CaseInsensitive))
            continue;

        int row = routeList.rowCount();
        routeList.insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(entry.fileName());
        routeList.setItem(row, 0, nameItem);

        QTableWidgetItem* dateItem = new QTableWidgetItem(entry.lastModified().toString("yyyy-MM-dd"));
        routeList.setItem(row, 1, dateItem);
    }
    routeList.resizeColumnsToContents();
    routeList.resizeRowsToContents();
    routeList.setSortingEnabled(true);
    
    
/*   This is Goku's original code -- it had a single column list and used a QListView
 *   EFO commented out and replaced with a QTableWidgetItem to support two columns and sorting    
    routeList.clear();
    dir.setSorting(QDir::Time);
    dir.setSorting(QDir::Name);
    
    foreach(QString dirFile, dir.entryList()){
        if(dirFile == "." || dirFile == "..")   
            continue;
        if(!Game::checkRoute(dirFile))  
            continue;
        this->routeList.addItem(dirFile);  
    }    
  */
} 

void LoadWindow::setLoadRoute(){
    //qDebug() << "load";
    this->load->setText("Load");
    this->newRoute = false;
}

void LoadWindow::cRecentEnabled(QString val){
    handleBrowseButton(val);
}

void LoadWindow::rootPathEntered(){
    handleBrowseButton(cRecent.currentText());
}

void LoadWindow::setNewRoute(){
    //qDebug() << "new";
    //this->load->setText("New");
    
    //Check if template route available.
    QString path = "./tsre_assets/templateRoute_0.6";
    QFile appFile(path);
    if (!appFile.exists()){
        downloadTemplateRoute(path);
    }

    NewRouteWindow newWindow;
    newWindow.name.setText("");
    newWindow.lat.setText("50.0");
    newWindow.lon.setText("20.0");
    newWindow.exec();
    if(newWindow.changed){
        if(newWindow.name.text().length() < 2) return;
        Game::route = newWindow.name.text();
        double lat = newWindow.lat.text().toDouble();
        double lon = newWindow.lon.text().toDouble();
        
        Game::GeoCoordConverter = new GeoMstsCoordinateConverter();
        
        igh = Game::GeoCoordConverter->ConvertToInternal(lat, lon, igh);
        aCoords = Game::GeoCoordConverter->ConvertToTile(igh, aCoords);
        aCoords->setWxyz();
        Game::newRouteX = aCoords->TileX;
        Game::newRouteZ = aCoords->TileZ;
        qDebug() << Game::newRouteX << " " << Game::newRouteZ;
        this->newRoute = true;
        routeLoad();
    }
}

void LoadWindow::exitNow(){
    this->hide();
}

void LoadWindow::downloadTemplateRoute(QString path){
    QDir().mkdir(path);
    
    // Download and extract Route Data
    QNetworkAccessManager* mgr = new QNetworkAccessManager();
    qDebug() << "Wait ..";
    QString Url = "http://koniec.org/tsre5/data/appdata/templateRoute_0.6.tar";
    qDebug() << Url;
    QNetworkRequest req;
    req.setUrl(QUrl(Url));
    qDebug() << req.url();
    QNetworkReply* r = mgr->get(req);
    QEventLoop loop;
    QObject::connect(r, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    
    qDebug() << "Network Reply Loop End";
    QByteArray data = r->readAll();
    FileBuffer *fileData = new FileBuffer((unsigned char*)data.data(), data.length());
    TarFile tarFile(fileData);
    tarFile.extractTo("./tsre_assets/");

}

void LoadWindow::listRoots(){
    QString sh;
    QString path;
    path = "cerecent.txt";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    qDebug() << path;

    QTextStream in(&file);
    QString line;
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        if(line.isEmpty())
            continue;
        bool duplicate = false;
        for(int i = 0; i < cRecent.count(); i++){
            if(cRecent.itemText(i).compare(line, Qt::CaseInsensitive) == 0){
                duplicate = true;
                break;
            }
        }
        if(!duplicate)
            cRecent.addItem(line);
    }
    file.close();
} 


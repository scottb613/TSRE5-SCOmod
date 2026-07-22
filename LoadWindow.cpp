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

static QString loadButtonStyle(const QString& background, const QString& hoverBackground,
                               const QString& textColor, const QString& border,
                               const QString& pressedBackground){
    return QString(
        "QPushButton { color: %1;"
        " background-color: %2;"
        " border: 1px solid %4; border-radius: 1px; padding: 1px 4px; }"
        "QPushButton:hover { background-color: %3; border-color: #f08200; }"
        "QPushButton:pressed {"
        " background-color: %5; border-color: %4;"
        " padding-top: 2px; padding-bottom: 0px; }"
        "QPushButton:disabled { color: #858585; background-color: #3b3b3b; border-color: #4b4b4b; }"
    ).arg(textColor, background, hoverBackground, border, pressedBackground);
}

LoadWindow::LoadWindow() {
    //this->setWindowFlags( Qt::CustomizeWindowHint );
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
    const QString greenButton = loadButtonStyle("#176c25", "#1e8430", "#f2fff4", "#319344", "#104b1a");
    const QString orangeButton = loadButtonStyle("#a85a16", "#c96d1c", "#fff4e8", "#dc802c", "#6f3a0d");
    const QString redButton = loadButtonStyle("#8d3030", "#a63b3b", "#fff0f0", "#bd5151", "#602020");
    load->setStyleSheet(greenButton);
    connect(load, SIGNAL (released()), this, SLOT (routeLoad()));
    neww = new QPushButton("New");
    neww->setMinimumHeight(24);
    neww->setStyleSheet(orangeButton);
    connect(neww, SIGNAL (released()), this, SLOT (setNewRoute()));
    restoreLast = new QPushButton("Restore Last Session");
    restoreLast->setMinimumHeight(24);
    restoreLast->setStyleSheet(greenButton);
    connect(restoreLast, SIGNAL (released()), this, SLOT (restoreLastSession()));
    exit = new QPushButton("Exit");
    exit->setMinimumHeight(24);
    exit->setStyleSheet(redButton);

    const QList<QPushButton*> startupButtons = { browse, load, neww, restoreLast, exit };
    for(QPushButton *button : startupButtons){
        connect(button, &QPushButton::pressed, this, [](){
            playLoadWindowSound("SCOpress.wav", true);
        });
    }

    

    nowaTrasa = new QLineEdit();
    QRegExp rx("^[a-zA-Z0-9\\_\\-\\ ]*$");
    //QRegExp rx("[\\/<>|\":?*].");
    QRegExpValidator* v = new QRegExpValidator(rx);
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
    QObject::connect(&cRecent, SIGNAL(activated(QString)),
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
    routeList.setHorizontalHeaderLabels(QStringList() << "Directory Name" << "Last Modified");
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
    QHBoxLayout *vbox = new QHBoxLayout;
    vbox->addWidget(load);
    vbox->addWidget(neww);
    vbox->addWidget(restoreLast);
    vbox->addWidget(exit);
    vbox->setContentsMargins(0,0,0,0);
    box->setLayout(vbox);
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
        updateStartupButtons(true);
        cRecent.setStyleSheet(
            "QComboBox { combobox-popup: 0; color: white; font-weight: normal; }"
            "QComboBox QLineEdit { color: white; font-weight: normal; }");
        rootStatusLabel.hide();
        Game::root = directory;
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

void LoadWindow::updateStartupButtons(bool validRoot){
    restoreLast->setEnabled(QFile::exists(Game::lastSessionFilePath()));
    restoreLast->show();
    exit->show();
    if(validRoot){
        load->show();
        neww->show();
        load->setFixedWidth(100);
        neww->setFixedWidth(100);
        restoreLast->setFixedWidth(180);
        exit->setFixedWidth(100);
    } else {
        load->hide();
        neww->hide();
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

    this->hide();
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
        if(routeList.currentRow() < 0) return;
        QTableWidgetItem *routeItem = routeList.item(routeList.currentRow(), 0);
        if(routeItem == NULL) return;
        Game::route = routeItem->text();
        Game::checkRoute(Game::route);
    }
    qDebug() << Game::route;
    this->hide();
    emit showMainWindow();
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
    
    QStringList headers = {"Directory Name", "Last Modified"};
    routeList.setHorizontalHeaderLabels(headers);

    // Get directory entries
    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time );

    // Populate table with directory information
    for (const QFileInfo& entry : entries) {
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
    newWindow.setWindowTitle("New route");
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


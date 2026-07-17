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

LoadWindow::LoadWindow() {
    //this->setWindowFlags( Qt::CustomizeWindowHint );
    setWindowTitle(Game::AppName+" "+Game::AppVersion+" Route Editor");
    this->setFixedSize(600, 700);
    QImage* myImage = new QImage();
    myImage->load(QString("tsre_appdata/")+Game::AppDataVersion+"/load.png");

    QLabel* myLabel = new QLabel("");
    myLabel->setContentsMargins(0,0,0,0);
    myLabel->setFixedSize(600, 200);
    QLabel* myLabel2 = new QLabel("Choose folder containing 'Global' and 'Routes': ");
    myLabel2->setContentsMargins(5,0,0,0);
    QLabel* myLabel3 = new QLabel("Select route above or enter name for new route: ");
    myLabel3->setContentsMargins(5,0,0,0);
    
    myLabel->setPixmap(QPixmap::fromImage(*myImage).scaled(myLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    browse = new QPushButton("Browse");
    browse->setMinimumHeight(24);
    connect(browse, SIGNAL (released()), this, SLOT (handleBrowseButton()));
    load = new QPushButton("Load");
    load->setMinimumHeight(24);
    load->setStyleSheet(QString("background-color: ")+Game::StyleGreenButton);
    connect(load, SIGNAL (released()), this, SLOT (routeLoad()));
    neww = new QPushButton("New");
    neww->setMinimumHeight(24);
    neww->setStyleSheet(QString("background-color: ")+Game::StyleYellowButton);
    connect(neww, SIGNAL (released()), this, SLOT (setNewRoute()));
    restoreLast = new QPushButton("Restore Last Session");
    restoreLast->setMinimumHeight(24);
    restoreLast->setStyleSheet(QString("background-color: ")+Game::StyleGreenButton);
    connect(restoreLast, SIGNAL (released()), this, SLOT (restoreLastSession()));
    exit = new QPushButton("Exit");
    exit->setMinimumHeight(24);
    exit->setStyleSheet(QString("background-color: ")+Game::StyleRedButton);

    

    nowaTrasa = new QLineEdit();
    QRegExp rx("^[a-zA-Z0-9\\_\\-\\ ]*$");
    //QRegExp rx("[\\/<>|\":?*].");
    QRegExpValidator* v = new QRegExpValidator(rx);
    nowaTrasa->setValidator(v);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(myLabel);
    mainLayout->addWidget(myLabel2);
    mainLayout->addWidget(browse);
    QFormLayout *recentLayout = new QFormLayout;
    recentLayout->setContentsMargins(0,0,0,0);
    cRecent.setMaxVisibleItems(10);
    cRecent.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    cRecent.setStyleSheet("combobox-popup: 0;");
    QObject::connect(&cRecent, SIGNAL(activated(QString)),
                      this, SLOT(cRecentEnabled(QString)));
    recentLayout->addRow("Recent: ", &cRecent);
    mainLayout->addItem(recentLayout);
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
    mainLayout->setAlignment(browse, Qt::AlignTop);
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
        browse->setText(Game::root);
        browse->setStyleSheet(QString("color: ")+Game::StyleGreenText);
        this->listRoutes();
    } else {
        updateStartupButtons(false);
    }
}





void LoadWindow::handleBrowseButton(QString directory){
    if(directory == ""){
        QFileDialog *fd = new QFileDialog;
        //QTreeView *tree = fd->findChild <QTreeView*>();
        //tree->setRootIsDecorated(true);
        //tree->setItemsExpandable(true);
        fd->setFileMode(QFileDialog::Directory);
        fd->setOption(QFileDialog::ShowDirsOnly);
        //fd->setViewMode(QFileDialog::Detail);
        int result = fd->exec();
        if (result)
        {
            directory = fd->selectedFiles()[0];
            qDebug()<<directory;
        }
    }
    //Game::root = directory;
    browse->setText(directory);
    browse->setStyleSheet(QString("color: ")+Game::StyleRedText);
    load->hide();
    //nowa->hide();
    neww->hide();
    updateStartupButtons(false);
    routeList.clear();
    if(Game::checkRoot(directory)){
        qDebug()<<"ok";
        updateStartupButtons(true);
        browse->setStyleSheet(QString("color: ")+Game::StyleGreenText);
        Game::root = directory;
        this->listRoutes();
        
        int i = 0;
        for(i = 0; i < cRecent.count(); i++){
            if(cRecent.itemText(i) == directory.toLower())
                break;
        }
        if(i == cRecent.count())
            cRecent.addItem(directory.toLower());
        
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

    QJsonObject naviWindow = root.value("naviWindow").toObject();
    if(!naviWindow.isEmpty()){
        Game::restoreNaviGeometry = true;
        Game::restoreNaviX = naviWindow.value("x").toInt();
        Game::restoreNaviY = naviWindow.value("y").toInt();
        Game::restoreNaviW = naviWindow.value("w").toInt();
        Game::restoreNaviH = naviWindow.value("h").toInt();
        Game::naviPos = QString::number(Game::restoreNaviX)+","+QString::number(Game::restoreNaviY);
    }

    QJsonObject statusWindow = root.value("statusWindow").toObject();
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
        line = in.readLine().toLower();
        cRecent.addItem(line);
    }
    file.close();
} 


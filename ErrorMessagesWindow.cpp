/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ErrorMessagesWindow.h"
#include "ErrorMessagesLib.h"
#include <QDebug>
#include "Game.h"
#include "ErrorMessage.h"
#include "ErrorMessageProperties.h"
#include "GeoCoordinates.h"
#include "GuiFunct.h"

static int scaledUiSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
}

ErrorMessagesWindow::ErrorMessagesWindow(QWidget* parent) : QWidget(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    GuiFunct::setEditorToolWindowTitle(this);
    brushes[(int)ErrorMessage::Type_Error] = QBrush(QColor(Game::StyleRedText));
    brushes[(int)ErrorMessage::Type_Warning] = QBrush(QColor(200,200,0));
    brushes[(int)ErrorMessage::Type_Info] = QBrush(QColor(Game::StyleGreenText));
    brushes[(int)ErrorMessage::Type_AutoFix] =QBrush(QColor(20,20,200));
    brushes[1000] = QBrush(QColor(Game::StyleMainLabel));
    this->setWindowFlags(Qt::WindowType::Tool);
    //this->setFixedWidth(350);
    this->setMinimumWidth(scaledUiSize(730));
    this->setFixedHeight(scaledUiSize(430));
    
    properties = new ErrorMessageProperties(this);
    
    QVBoxLayout *errorListLayout = new QVBoxLayout;
    errorListLayout->setContentsMargins(4,4,4,4);
    errorListLayout->setSpacing(4);
    QLabel *title = new QLabel(tr("ERRORS & MESSAGES"), this);
    GuiFunct::styleEditorTitle(title);
    errorListLayout->addWidget(title);
    QLabel *logHeading = new QLabel(
        QString(QChar(0x2022)) + tr(" Message Log"), this);
    GuiFunct::styleEditorSubtitle(logHeading);
    errorListLayout->addWidget(logHeading);
    QFrame *listCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(listCard);
    QVBoxLayout *listCardLayout = new QVBoxLayout(listCard);
    listCardLayout->setContentsMargins(4,4,4,4);
    /*QPushButton *bNewActionEvent = new QPushButton("New Service");
    QObject::connect(bNewActionEvent, SIGNAL(released()),
                      this, SLOT(bNewServiceSelected()));
    QPushButton *bDeleteActionEvent = new QPushButton("Delete");
    QObject::connect(bDeleteActionEvent, SIGNAL(released()),
                      this, SLOT(bDeleteServiceSelected()));*/
    listCardLayout->addWidget(&errorList);
    errorListLayout->addWidget(listCard, 1);
    errorListLayout->addWidget(properties);
    //errorListLayout->addWidget(bNewActionEvent);
    //errorListLayout->addWidget(bDeleteActionEvent);
    QStringList list;
    list.append("ID:");
    list.append("Time:");
    list.append("Type:");
    list.append("Source:");
    list.append("Message:");
    //list.append("Any:");
    //errorList.setFixedWidth(250);
    errorList.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    errorList.setColumnCount(5);
    errorList.setHeaderLabels(list);
    errorList.setRootIsDecorated(false);
    errorList.header()->resizeSection(0,30);    
    errorList.header()->resizeSection(1,50);    
    errorList.header()->resizeSection(2,70);    
    errorList.header()->resizeSection(3,50);    
    errorList.header()->resizeSection(4,500);    
    //QHBoxLayout *v = new QHBoxLayout;
    //v->setSpacing(2);
    //v->setContentsMargins(1,1,1,1);
    //v->addItem(errorListLayout);
    //v->addWidget(serviceProperties);
    this->setLayout(errorListLayout);
    
    QObject::connect(&errorList, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
                      this, SLOT(errorListSelected(QTreeWidgetItem*, int)));
    QObject::connect(properties, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      this, SLOT(jumpRequestReceived(PreciseTileCoordinate*)));
    QObject::connect(properties, SIGNAL(selectObject(GameObj*)),
                      this, SLOT(selectRequestReceived(GameObj*)));
    QObject::connect(properties, SIGNAL(messageUpdated()),
                      this, SLOT(refreshErrorList()));
    refreshErrorList();
}

void ErrorMessagesWindow::selectRequestReceived(GameObj* o){    
    emit selectObject(o);
}

void ErrorMessagesWindow::jumpRequestReceived(PreciseTileCoordinate* c){
    emit jumpTo(c);
}

void ErrorMessagesWindow::errorListSelected(QTreeWidgetItem* item, int column){
    properties->showMessage(ErrorMessagesLib::ErrorMessages[item->type()]);
}

void ErrorMessagesWindow::refreshErrorList(){
    errorList.clear();
    QList<QTreeWidgetItem *> items;
    QStringList list;
 //   qDebug() << "Errors:";
    
    for(int i = ErrorMessagesLib::ErrorMessages.size() - 1; i >= 0 ; i-- ){
        if(ErrorMessagesLib::ErrorMessages[i] == NULL)
            continue;
        
        ErrorMessage *msg = ErrorMessagesLib::ErrorMessages[i];
        list.clear();
        
       //QTime time = QDateTime::fromMSecsSinceEpoch(msg->time).toString("HH:mm:ss");
        //qDebug() << msg->time << time.isValid()<< time.toString();
        list.append(QString::number(i));
        list.append(QDateTime::fromMSecsSinceEpoch(msg->time).toString("HH:mm:ss"));
        list.append(ErrorMessage::TypeNames[msg->type]);
        list.append(ErrorMessage::SourceNames[msg->source]);
        list.append(msg->description);
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0, list, i );
        // Send items in the error log window to the logfile:
        qDebug() << "Route Error Msg " << i << ": " << ErrorMessage::TypeNames[msg->type] << ":" <<  ErrorMessage::SourceNames[msg->source] << ":" << msg->description  ;
        //item->setCheckState(0, Qt::Unchecked);
        //item->setCheckState(1, Qt::Unchecked);
        //item->setCheckState(2, Qt::Unchecked);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setForeground(2, brushes[(int)msg->type]);
        item->setForeground(3, brushes[1000]);
        item->setTextAlignment(1, Qt::AlignCenter);
        item->setTextAlignment(2, Qt::AlignCenter);
        item->setTextAlignment(3, Qt::AlignCenter);
        items.append(item);
    }
    errorList.insertTopLevelItems(0, items);
}

void ErrorMessagesWindow::show(){
     refreshErrorList();
     QWidget::show();
}

ErrorMessagesWindow::~ErrorMessagesWindow() {
}

void ErrorMessagesWindow::hideEvent(QHideEvent *e){
    emit windowClosed();
}

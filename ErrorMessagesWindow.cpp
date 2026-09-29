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
#include "Route.h"
#include <QScopedValueRollback>
#include <exception>
#include <QCryptographicHash>
#include <QDataStream>
#include <QSettings>

// Exclude log time, row number and pointers so a regenerated diagnostic keeps
// its identity. Changed diagnostic details/location are deliberately visible.
static QString noFactorKey(const ErrorMessage &message){
    QByteArray identity;
    QDataStream stream(&identity, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << qint32(message.type) << qint32(message.source)
           << message.description << message.action << bool(message.coords);
    if(message.coords != nullptr)
        stream << message.coords->TileX << message.coords->TileZ
               << message.coords->wX << message.coords->wY << message.coords->wZ;
    return QString::fromLatin1(QCryptographicHash::hash(identity,
            QCryptographicHash::Sha256).toHex());
}

static QString noFactorSettingsPath(){
    return Game::routeAppDataDir() + "/diagnostic-status.ini";
}

static ErrorMessage *messageForRow(const QTreeWidgetItem *item){
    if(item == nullptr)
        return nullptr;
    auto *message = reinterpret_cast<ErrorMessage *>(
            item->data(0, Qt::UserRole).value<quintptr>());
    // Scan replacement can remove records while old rows still exist.
    return ErrorMessagesLib::ErrorMessages.contains(message) ? message : nullptr;
}

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
    this->setMinimumHeight(scaledUiSize(510));
    
    properties = new ErrorMessageProperties(this);
    
    QVBoxLayout *errorListLayout = new QVBoxLayout;
    errorListLayout->setContentsMargins(4,4,4,4);
    errorListLayout->setSpacing(4);
    QLabel *title = new QLabel(tr("ERRORS & MESSAGES"), this);
    GuiFunct::styleEditorTitle(title);
    errorListLayout->addWidget(title);
    QFrame *scanCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(scanCard);
    QGridLayout *scanControls = new QGridLayout(scanCard);
    scanControls->setContentsMargins(4,4,4,4);
    scanControls->setSpacing(2);
    scanControls->setColumnStretch(0,0);
    scanControls->setColumnStretch(6,1);
    scanButton = new QPushButton(tr("Load/Scan World Tiles"), this);
    scanButton->setObjectName("scanAllWorldTilesButton");
    scanButton->setToolTip(tr("Load the full route and check errors, stacked orphan track, standalone pieces and TDB joints. No automatic repairs."));
    GuiFunct::styleEditorActionButton(scanButton);
    QHBoxLayout *scanActions = new QHBoxLayout;
    scanActions->setSpacing(2);
    scanActions->addWidget(scanButton,1);
    scanControls->addLayout(scanActions,1,0,1,7);
    resetButton = new QPushButton(tr("Reset Status"), this);
    GuiFunct::styleEditorActionButton(resetButton);
    resetButton->setToolTip(tr("Clear this route's No Factor choices and show all currently logged records. Scan again to regenerate route diagnostics."));
    scanActions->addWidget(resetButton,1);
    connect(resetButton, &QPushButton::clicked, this, &ErrorMessagesWindow::resetStatus);
    orphanLength = new QDoubleSpinBox(this);
    orphanLength->setObjectName("scanOrphanLengthMetres");
    orphanLength->setRange(1,1000);
    orphanLength->setDecimals(0);
    orphanLength->setSuffix(tr(" m"));
    orphanLength->setValue(100);
    orphanLength->setToolTip(tr("For stacked-track detection: maximum TOTAL length of a separate TDB component with two ends and no junctions. Standalone single pieces are checked at any length."));
    verticalThreshold = new QDoubleSpinBox(this);
    horizontalThreshold = new QDoubleSpinBox(this);
    for(QDoubleSpinBox *threshold : {verticalThreshold,horizontalThreshold}){
        threshold->setRange(0.01,5.00);
        threshold->setDecimals(2);
        threshold->setSingleStep(0.01);
        threshold->setSuffix(tr(" m"));
    }
    verticalThreshold->setObjectName("scanVerticalThresholdMetres");
    horizontalThreshold->setObjectName("scanHorizontalThresholdMetres");
    verticalThreshold->setValue(0.20);
    horizontalThreshold->setValue(0.25);
    int fieldWidth = scaledUiSize(90);
    for(QDoubleSpinBox *field : {orphanLength,verticalThreshold,horizontalThreshold}){
        field->ensurePolished();
        fieldWidth = qMax(fieldWidth,field->sizeHint().width());
    }
    for(QDoubleSpinBox *field : {orphanLength,verticalThreshold,horizontalThreshold})
        field->setFixedWidth(fieldWidth);
    verticalThreshold->setToolTip(tr("Report internal/linked joint gaps above this height difference. Stacked-track and standalone checks are unchanged. Rescan to apply."));
    horizontalThreshold->setToolTip(tr("Report internal/linked joint gaps above this horizontal distance. Stacked-track and standalone checks are unchanged. Rescan to apply."));
    QHBoxLayout *settingCells = new QHBoxLayout;
    settingCells->setSpacing(2);
    const QStringList settingLabels{tr("Orphan Limit:"),tr("Joint Vert:"),tr("Joint Horz:")};
    QDoubleSpinBox *settingFields[] = {orphanLength,verticalThreshold,horizontalThreshold};
    for(int i = 0; i < 3; ++i){
        QFrame *cell = new QFrame(scanCard);
        GuiFunct::styleEditorPanelCard(cell);
        QHBoxLayout *contents = new QHBoxLayout(cell);
        contents->setContentsMargins(4,3,4,3);
        contents->setSpacing(4);
        contents->addWidget(new QLabel(settingLabels[i],cell),1);
        contents->addWidget(settingFields[i]);
        settingCells->addWidget(cell,1);
    }
    scanControls->addLayout(settingCells,0,0,1,7);
    errorListLayout->addWidget(scanCard);
    scanStatus = new QLabel(tr("Full-route scan has not run in this session."), this);
    scanStatus->setWordWrap(true);
    for(QDoubleSpinBox *setting : {orphanLength,verticalThreshold,horizontalThreshold})
        connect(setting, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](){
            scanStatus->setText(tr("Settings changed. Load/Scan World Tiles to refresh results with these thresholds."));
        });
    errorListLayout->addWidget(scanStatus);
    connect(scanButton, &QPushButton::clicked, this, &ErrorMessagesWindow::scanRoute);
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
    errorList.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    errorList.setColumnCount(5);
    errorList.setHeaderLabels(list);
    errorList.setRootIsDecorated(false);
    for(int column = 0; column < 4; ++column)
        errorList.header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    errorList.header()->setSectionResizeMode(4, QHeaderView::Stretch);
    errorList.setWordWrap(false);
    errorList.setTextElideMode(Qt::ElideRight);
    errorList.setUniformRowHeights(true);
    errorList.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    errorList.setSelectionMode(QAbstractItemView::SingleSelection);
    errorList.setSelectionBehavior(QAbstractItemView::SelectRows);
    //QHBoxLayout *v = new QHBoxLayout;
    //v->setSpacing(2);
    //v->setContentsMargins(1,1,1,1);
    //v->addItem(errorListLayout);
    //v->addWidget(serviceProperties);
    this->setLayout(errorListLayout);
    
    connect(&errorList, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *item, QTreeWidgetItem *){ errorListSelected(item,0); });
    connect(properties, &ErrorMessageProperties::noFactorRequested,
            this, &ErrorMessagesWindow::markNoFactor);
    QObject::connect(properties, SIGNAL(jumpTo(PreciseTileCoordinate*)),
                      this, SLOT(jumpRequestReceived(PreciseTileCoordinate*)));
    QObject::connect(properties, SIGNAL(selectObject(GameObj*)),
                      this, SLOT(selectRequestReceived(GameObj*)));
    QObject::connect(properties, SIGNAL(messageUpdated()),
                      this, SLOT(refreshErrorList()));
    refreshErrorList();
}

void ErrorMessagesWindow::scanRoute(){
    if(scanning) return;
    Route* route = Game::currentRoute;
    if(route == nullptr || !route->loaded){
        scanStatus->setText(tr("Open a route before scanning."));
        return;
    }
    QScopedValueRollback<bool> guard(scanning,true);
    scanButton->setEnabled(false);
    resetButton->setEnabled(false);
    orphanLength->setEnabled(false);
    verticalThreshold->setEnabled(false);
    horizontalThreshold->setEnabled(false);
    properties->showMessage(nullptr);
    errorList.setEnabled(false);
    scanStatus->setText(tr("Scanning the full route..."));
    try {
        scanStatus->setText(route->scanAllWorldTiles(orphanLength->value(),this,
                horizontalThreshold->value(),verticalThreshold->value()));
    } catch(const std::exception&) {
        scanStatus->setText(tr("Scan interrupted by a resource or data error. Coverage is incomplete; check the message log."));
    }
    scanning = false;
    refreshErrorList();
    errorList.setEnabled(true);
    orphanLength->setEnabled(true);
    verticalThreshold->setEnabled(true);
    horizontalThreshold->setEnabled(true);
    scanButton->setEnabled(true);
    resetButton->setEnabled(true);
}
void ErrorMessagesWindow::selectRequestReceived(GameObj* o){    
    emit selectObject(o);
}

void ErrorMessagesWindow::jumpRequestReceived(PreciseTileCoordinate* c){
    emit jumpTo(c);
}

void ErrorMessagesWindow::errorListSelected(QTreeWidgetItem* item, int column){
    Q_UNUSED(column);
    properties->showMessage(scanning ? nullptr : messageForRow(item));
}

void ErrorMessagesWindow::markNoFactor(ErrorMessage *message){
    if(scanning || message == nullptr || Game::currentRoute == nullptr)
        return;
    QSettings settings(noFactorSettingsPath(), QSettings::IniFormat);
    settings.setValue("noFactor/" + noFactorKey(*message), true);
    settings.sync();
    if(settings.status() != QSettings::NoError){
        scanStatus->setText(tr("Could not save No Factor status. The record remains visible."));
        return;
    }
    refreshErrorList();
}

void ErrorMessagesWindow::resetStatus(){
    if(scanning || Game::currentRoute == nullptr)
        return;
    QSettings settings(noFactorSettingsPath(), QSettings::IniFormat);
    settings.remove("noFactor");
    settings.sync();
    if(settings.status() != QSettings::NoError){
        scanStatus->setText(tr("Could not reset saved diagnostic status."));
        return;
    }
    refreshErrorList();
    scanStatus->setText(tr("Status reset. All currently logged records are visible; scan to refresh route diagnostics."));
}

void ErrorMessagesWindow::refreshErrorList(){
    ErrorMessage *selectedMessage = messageForRow(errorList.currentItem());
    QTreeWidgetItem *topItem = errorList.itemAt(0,0);
    ErrorMessage *topMessage = messageForRow(topItem);
    const int topOffset = topItem != nullptr ? errorList.visualItemRect(topItem).top() : 0;
    const int oldScroll = errorList.verticalScrollBar()->value();
    const int oldHorizontalScroll = errorList.horizontalScrollBar()->value();
    const QSignalBlocker blockSelection(&errorList);
    errorList.clear();
    resetButton->setEnabled(!scanning && Game::currentRoute != nullptr);
    QSet<QString> dismissed;
    if(Game::currentRoute != nullptr){
        QSettings settings(noFactorSettingsPath(), QSettings::IniFormat);
        settings.beginGroup("noFactor");
        for(const QString &key : settings.childKeys())
            if(settings.value(key).toBool()) dismissed.insert(key);
    }
    QList<QTreeWidgetItem *> items;
    QTreeWidgetItem *selectedRow = nullptr;
    QTreeWidgetItem *topRow = nullptr;
    QStringList list;
 //   qDebug() << "Errors:";
    
    for(int i = ErrorMessagesLib::ErrorMessages.size() - 1; i >= 0 ; i-- ){
        if(ErrorMessagesLib::ErrorMessages[i] == NULL)
            continue;
        
        ErrorMessage *msg = ErrorMessagesLib::ErrorMessages[i];
        if(dismissed.contains(noFactorKey(*msg)))
            continue;
        list.clear();
        
       //QTime time = QDateTime::fromMSecsSinceEpoch(msg->time).toString("HH:mm:ss");
        //qDebug() << msg->time << time.isValid()<< time.toString();
        list.append(QString::number(i));
        list.append(QDateTime::fromMSecsSinceEpoch(msg->time).toString("HH:mm:ss"));
        list.append(ErrorMessage::TypeNames[msg->type]);
        list.append(ErrorMessage::SourceNames[msg->source]);
        list.append(msg->description);
        QTreeWidgetItem *item = new QTreeWidgetItem(list);
        item->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(msg)));
        // Compact, uniform rows keep hit testing stable during selection.
        item->setSizeHint(4, QSize(0, errorList.fontMetrics().lineSpacing() + 4));
        if(msg == selectedMessage) selectedRow = item;
        if(msg == topMessage) topRow = item;
        for(int column = 0; column < list.size(); ++column)
            item->setToolTip(column,list[column]);
        // Send items in the error log window to the logfile:
        if(Game::debugOutput)
            qDebug() << "Route Error Msg " << i << ": " << ErrorMessage::TypeNames[msg->type] << ":" <<  ErrorMessage::SourceNames[msg->source] << ":" << msg->description;
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
    errorList.setCurrentItem(selectedRow);
    properties->showMessage(scanning ? nullptr : messageForRow(selectedRow));
    errorList.doItemsLayout();
    if(topRow != nullptr){
        errorList.scrollToItem(topRow, QAbstractItemView::PositionAtTop);
        errorList.verticalScrollBar()->setValue(
                errorList.verticalScrollBar()->value() - topOffset);
    } else {
        errorList.verticalScrollBar()->setValue(oldScroll);
    }
    errorList.horizontalScrollBar()->setValue(oldHorizontalScroll);
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

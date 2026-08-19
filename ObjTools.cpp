/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TSectionDAT.h"
#include "ObjTools.h"
#include "Route.h"
#include "Game.h"
#include "SigCfg.h"
#include "SignalShape.h"
#include "ForestObj.h"
#include "SpeedPost.h"
#include "SpeedPostDAT.h"
#include "SoundList.h"
#include "TRitem.h"
#include "GuiFunct.h"
#include <QMapIterator>

static int scaledUiSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
}

class AutoPlacementWindow : public QWidget {
public:
    explicit AutoPlacementWindow(ObjTools *owner)
        : QWidget(owner) {
        GuiFunct::applyEditorPanelStyle(this);
        rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(scaledUiSize(6), scaledUiSize(6),
                                       scaledUiSize(6), scaledUiSize(6));
        rootLayout->setSpacing(scaledUiSize(5));
        QLabel *heading = new QLabel("AUTO PLACE", this);
        GuiFunct::styleEditorTitle(heading);
        rootLayout->addWidget(heading);
    }

    QVBoxLayout *contentLayout(){
        return rootLayout;
    }

    void finishLayout(){
        rootLayout->addStretch(1);
    }

private:
    QVBoxLayout *rootLayout = NULL;
};

static QString tdbCategoryLabel(const QString& filename, bool roadShape){
    if(filename.startsWith("SR_", Qt::CaseInsensitive)){
        const QString scaleName = filename.mid(3);
        if(roadShape){
            const int separator = scaleName.indexOf('_');
            const QString family = (separator > 0 ? scaleName.left(separator) : scaleName);
            return family.isEmpty() ? QString("Scale Road") : QString("Scale Road ") + family;
        }

        QRegularExpression railTracks("^((?:\\d+[tx])|xt)([A-Za-z]+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = railTracks.match(scaleName);
        if(match.hasMatch()){
            const QString prefix = QString("sr") + match.captured(1).toLower();
            const QString descriptor = match.captured(2);
            return descriptor.isEmpty() ? prefix : prefix + " " + descriptor;
        }

        const int separator = scaleName.indexOf('_');
        const QString family = (separator > 0 ? scaleName.left(separator) : scaleName);
        return family.isEmpty() ? QString("Scale Rail") : QString("Scale Rail ") + family;
    }

    return filename.left(3).toLower();
}

static QString tdbCategoryKey(const QString& label, bool roadShape){
    QString key = label.toLower();
    key.replace(QRegularExpression("[^a-z0-9]+"), "_");
    return QString("#TDB#%1/%2").arg(roadShape ? "road" : "track", key);
}

ObjTools::ObjTools(QString name)
    : QWidget(){
    //QRadioButton *radio1 = new QRadioButton(tr("&Radio button 1"));
    //QRadioButton *radio2 = new QRadioButton(tr("R&adio button 2"));
    //QRadioButton *radio3 = new QRadioButton(tr("Ra&dio button 3"));
    setFixedWidth(scaledUiSize(250));
    GuiFunct::applyEditorPanelStyle(this);
    QFont objectPanelFont = font();
    if(objectPanelFont.pointSizeF() > 0)
        objectPanelFont.setPointSizeF(objectPanelFont.pointSizeF() * 1.12);
    setFont(objectPanelFont);
    buttonTools["selectTool"] = new QPushButton("Select", this);
    buttonTools["placeTool"] = new QPushButton("Place New", this);
    buttonTools["autoPlaceSimpleTool"] = new QPushButton("Auto Place", this);
    QMapIterator<QString, QPushButton*> i(buttonTools);
    while (i.hasNext()) {
        i.next();
        i.value()->setCheckable(true);
        i.value()->setMinimumHeight(scaledUiSize(20));
    }
    GuiFunct::styleEditorActionButton(buttonTools["selectTool"]);
    GuiFunct::styleEditorActionButton(buttonTools["placeTool"]);

    QPushButton *resetRotationButton = new QPushButton("Reset Rotation", this);
    resetRotationButton->setMinimumHeight(scaledUiSize(20));
    QPushButton *autoPlacementDeleteLast = new QPushButton("Undo Last", this);
    autoPlacementDeleteLast->setMinimumHeight(scaledUiSize(20));
    QObject::connect(autoPlacementDeleteLast, SIGNAL(released()), this, SLOT(autoPlacementDeleteLastEnabled()));
    
    //searchBox = new QLineEdit(this);
    //radio1->setChecked(true);
    
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(3);
    vbox->setContentsMargins(4,3,4,4);
    auto addRule = [vbox]() {
        vbox->addSpacing(scaledUiSize(5));
    };
    QLabel *panelTitle = new QLabel("OBJECT SELECTION");
    GuiFunct::styleEditorTitle(panelTitle);
    vbox->addWidget(panelTitle);
    QFrame *selectionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(selectionCard);
    QFormLayout *vlist = new QFormLayout(selectionCard);
    vlist->setSpacing(2);
    vlist->setContentsMargins(scaledUiSize(6), scaledUiSize(5),
                              scaledUiSize(6), scaledUiSize(5));
    vlist->addRow("Ref file:",&refClass);
    vlist->addRow("Tracks:",&refTrack);
    vlist->addRow("Roads:",&refRoad);
    vlist->addRow("Other:",&refOther);
    resetSearchButton.setText("Reset");
    resetSearchButton.setMinimumHeight(scaledUiSize(20));
    resetSearchButton.setFont(objectPanelFont);
    GuiFunct::styleEditorActionButton(&resetSearchButton);
    vlist->addRow("Search:", &searchBox);
    vlist->addRow(&resetSearchButton);
    QList<QComboBox*> comboBoxes;
    comboBoxes << &refClass << &refTrack << &refRoad << &refOther;
    for(int cb = 0; cb < comboBoxes.size(); cb++){
        comboBoxes[cb]->setFont(objectPanelFont);
        comboBoxes[cb]->setMinimumHeight(scaledUiSize(20));
    }
    searchBox.setFont(objectPanelFont);
    searchBox.setMinimumHeight(scaledUiSize(20));
    vbox->addWidget(selectionCard);
    vbox->addWidget(&refList);
    addRule();
    QLabel *placeTitle = new QLabel(QString(QChar(0x2022)) + " Object Place");
    GuiFunct::styleEditorSubtitle(placeTitle);
    vbox->addWidget(placeTitle);
    QFrame *placeCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(placeCard);
    QGridLayout *vlist3 = new QGridLayout(placeCard);
    vlist3->setSpacing(2);
    vlist3->setContentsMargins(scaledUiSize(6), scaledUiSize(5),
                               scaledUiSize(6), scaledUiSize(5));
    int row = 0;
    vlist3->addWidget(buttonTools["selectTool"],row,0,1,2);
    vlist3->addWidget(buttonTools["placeTool"],row++,2,1,2);
    autoPlacementLength.setText("50");
    autoPlacementLength.setMinimumHeight(scaledUiSize(20));
//    QDoubleValidator* doubleValidator = new QDoubleValidator(-999, 999, 6, this); 
//    QDoubleValidator* doubleValidator1 = new QDoubleValidator(1, 999, 6, this); 
    QDoubleValidator* doubleValidator = new QDoubleValidator(-Game::maxAutoPlacement, Game::maxAutoPlacement, 6, this); 
    QDoubleValidator* doubleValidator1 = new QDoubleValidator(1, Game::maxAutoPlacement, 6, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    doubleValidator1->setNotation(QDoubleValidator::StandardNotation);
    autoPlacementLength.setValidator(doubleValidator1);
    QObject::connect(&autoPlacementLength, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementLengthEnabled(QString)));
    vbox->addWidget(placeCard);
    
    vlist3 = new QGridLayout;
    vlist3->setSpacing(2);
    vlist3->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist3->addWidget(new QLabel("Rotation Type:"),row,0,1,1);
    vlist3->addWidget(&autoPlacementRotType,row++,1,1,6);
    QObject::connect(&autoPlacementRotType, SIGNAL(textActivated(QString)),
                      this, SLOT(autoPlacementRotTypeSelected(QString)));
    autoPlacementRotType.setStyleSheet("combobox-popup: 0;");
    autoPlacementRotType.addItem("Two Point Rotation");
    autoPlacementRotType.addItem("One Point Rotation");
    vlist3->addWidget(new QLabel("Target:"),row,0,1,1);
    vlist3->addWidget(&autoPlacementTarget,row++,1,1,6);
    QObject::connect(&autoPlacementTarget, SIGNAL(textActivated(QString)),
                      this, SLOT(autoPlacementTargetSelected(QString)));
    autoPlacementTarget.setStyleSheet("combobox-popup: 0;");
    autoPlacementTarget.addItem("Tracks");
    autoPlacementTarget.addItem("Roads");
    autoPlacementTarget.addItem("Tracks & Roads");
    autoPlacementTarget.addItem("Snapable");
    vlist3->addWidget(new QLabel("Translate Offset"),row,0);
    vlist3->addWidget(new QLabel("X:"),row,1);
    vlist3->addWidget(&autoPlacementPosX,row,2);
    QObject::connect(&autoPlacementPosX, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementPosX.setText("0");
    vlist3->addWidget(new QLabel("Y:"),row,3);
    vlist3->addWidget(&autoPlacementPosY,row,4);
    QObject::connect(&autoPlacementPosY, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementPosY.setText("0");
    vlist3->addWidget(new QLabel("Z:"),row,5);
    vlist3->addWidget(&autoPlacementPosZ,row++,6);    
    QObject::connect(&autoPlacementPosZ, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementPosZ.setText("0");
    vlist3->addWidget(new QLabel("Rotate Offset"),row,0);
    vlist3->addWidget(new QLabel("X:"),row,1);
    vlist3->addWidget(&autoPlacementRotX,row,2);
    QObject::connect(&autoPlacementRotX, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementRotX.setText("0");
    vlist3->addWidget(new QLabel("Y:"),row,3);
    vlist3->addWidget(&autoPlacementRotY,row,4);
    QObject::connect(&autoPlacementRotY, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementRotY.setText("0");
    vlist3->addWidget(new QLabel("Z:"),row,5);
    vlist3->addWidget(&autoPlacementRotZ,row++,6);    
    QObject::connect(&autoPlacementRotZ, SIGNAL(textEdited(QString)), this, SLOT(autoPlacementOffsetEnabled(QString)));
    autoPlacementRotZ.setText("0");
    vlist3->addWidget(new QLabel("Snapable max radius:"),row,0,1,1);
    vlist3->addWidget(&autoSnapableRadius,row,1,1,3);
    QObject::connect(&autoSnapableRadius, SIGNAL(textEdited(QString)), this, SLOT(autoSnapableRadiusEnabled(QString)));
    autoSnapableRadius.setText(QString::number(Game::snapableRadius));
    autoSnapableRadius.setValidator(doubleValidator1);
    QCheckBox *chSnapableOnlyRotation = new QCheckBox("Only Rot ");
    vlist3->addWidget(chSnapableOnlyRotation,row++,4,1,3);
    chSnapableOnlyRotation->setChecked(Game::snapableOnlyRot);
    QObject::connect(chSnapableOnlyRotation, SIGNAL(stateChanged(int)), this, SLOT(chSnapableOnlyRotation(int)));
    autoPlacementPosX.setValidator(doubleValidator);
    autoPlacementPosY.setValidator(doubleValidator);
    autoPlacementPosZ.setValidator(doubleValidator);
    autoPlacementRotX.setValidator(doubleValidator);
    autoPlacementRotY.setValidator(doubleValidator);
    autoPlacementRotZ.setValidator(doubleValidator);
    advancedPlacementWidget.setLayout(vlist3);
    GuiFunct::styleEditorPanelCard(&advancedPlacementWidget);

    stickToTDB.setText("Stick Target");
    stickToTDB.setChecked(false);

    autoPlacementWindow = new AutoPlacementWindow(this);
    QVBoxLayout *autoPlaceLayout = autoPlacementWindow->contentLayout();
    QLabel *alignmentHeading = new QLabel(QString::fromUtf8("• Alignment & Offsets"));
    GuiFunct::styleEditorSubtitle(alignmentHeading);
    autoPlaceLayout->addWidget(alignmentHeading);
    autoPlaceLayout->addWidget(&advancedPlacementWidget);

    autoPlaceLayout->addSpacing(scaledUiSize(5));
    QLabel *placementHeading = new QLabel(QString::fromUtf8("• Placement"));
    GuiFunct::styleEditorSubtitle(placementHeading);
    autoPlaceLayout->addWidget(placementHeading);

    QFrame *placementCard = new QFrame(autoPlacementWindow);
    GuiFunct::styleEditorPanelCard(placementCard);
    QVBoxLayout *placementCardLayout = new QVBoxLayout(placementCard);
    placementCardLayout->setContentsMargins(scaledUiSize(6), scaledUiSize(5),
                                            scaledUiSize(6), scaledUiSize(5));
    placementCardLayout->setSpacing(scaledUiSize(5));
    QGridLayout *placementLayout = new QGridLayout;
    placementLayout->setSpacing(3);
    placementLayout->setContentsMargins(0,0,0,0);
    placementLayout->addWidget(new QLabel("Spacing:"), 0, 0);
    placementLayout->addWidget(&autoPlacementLength, 0, 1);
    placementLayout->addWidget(new QLabel("m"), 0, 2);
    placementLayout->addWidget(&stickToTDB, 1, 0, 1, 3);
    placementCardLayout->addLayout(placementLayout);

    buttonTools["autoPlaceSimpleTool"]->setText("Commit");
    GuiFunct::styleEditorActionButton(buttonTools["autoPlaceSimpleTool"]);
    GuiFunct::styleEditorActionButton(autoPlacementDeleteLast);
    GuiFunct::styleEditorActionButton(resetRotationButton);
    placementCardLayout->addWidget(buttonTools["autoPlaceSimpleTool"]);
    placementCardLayout->addWidget(autoPlacementDeleteLast);
    placementCardLayout->addWidget(resetRotationButton);
    autoPlaceLayout->addWidget(placementCard);
    autoPlacementWindow->finishLayout();

    QLabel *label2 = new QLabel(QString(QChar(0x2022)) + " Recent Items");
    GuiFunct::styleEditorSubtitle(label2);
    label2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    clearRecentButton.setText("Clear Recent");
    clearRecentButton.setFocusPolicy(Qt::NoFocus);
    GuiFunct::styleEditorActionButton(&clearRecentButton);
    vbox->addWidget(label2);
    QFrame *recentCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(recentCard);
    recentCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *recentLayout = new QVBoxLayout(recentCard);
    recentLayout->setContentsMargins(scaledUiSize(6), scaledUiSize(5),
                                     scaledUiSize(6), scaledUiSize(5));
    recentLayout->setSpacing(scaledUiSize(5));
    recentLayout->addWidget(&clearRecentButton);
    recentLayout->addWidget(&lastItems);
    //refClasssetMargin(0);
    //refTrack->sets >setMargin(0);
    //refRoad->setMargin(0);
    //label1->setMargin(0);
    vbox->addWidget(recentCard);

    lastItems.setFont(objectPanelFont);
    const int visibleRecentRows = qBound(1, Game::numRecentItems, 15);
    lastItems.setMinimumHeight(visibleRecentRows*scaledUiSize(16));
    lastItems.setMaximumHeight(visibleRecentRows*scaledUiSize(16));
    lastItems.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    lastItems.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //QSizePolicy* sizePolicy = new QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    //astItems.setSizePolicy(*sizePolicy);
    //) QSizePolicy::MinimumExpanding);
    //lastItems.setMaximumHeight(999);
    // The OBJECT PLACE and RECENT ITEMS title strips take their space from
    // the main object list, never from the fixed-height recent-items list.
    refList.setMinimumHeight(scaledUiSize(220));
    refList.setFont(objectPanelFont);
    refList.setUniformItemSizes(true);
    refList.setSpacing(1);
    refList.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    refClass.setStyleSheet("QComboBox { combobox-popup: 0; }");
    refTrack.setStyleSheet("QComboBox { combobox-popup: 0; }");
    refRoad.setStyleSheet("QComboBox { combobox-popup: 0; }");
    refOther.setStyleSheet("QComboBox { combobox-popup: 0; }");
    
    //vbox->addStretch(1);
    this->setLayout(vbox);
    
    QObject::connect(&refClass, SIGNAL(textActivated(QString)),
                      this, SLOT(refClassSelected(QString)));
    
    QObject::connect(&refTrack, SIGNAL(textActivated(QString)),
                      this, SLOT(refTrackSelected(QString)));
    
    QObject::connect(&refRoad, SIGNAL(textActivated(QString)),
                      this, SLOT(refRoadSelected(QString)));
    
    QObject::connect(&refOther, SIGNAL(textActivated(QString)),
                      this, SLOT(refOtherSelected(QString)));
    
    QObject::connect(&searchBox, SIGNAL(textEdited(QString)),
                      this, SLOT(refSearchSelected(QString)));

    QObject::connect(&resetSearchButton, SIGNAL(released()),
                      this, SLOT(resetObjectSearch()));
    QObject::connect(&clearRecentButton, SIGNAL(released()),
                      this, SLOT(clearRecentItems()));
    
    QObject::connect(&refList, SIGNAL(itemClicked(QListWidgetItem*)),
                      this, SLOT(refListSelected(QListWidgetItem*)));
    
    QObject::connect(&lastItems, SIGNAL(itemClicked(QListWidgetItem*)),
                      this, SLOT(lastItemsListSelected(QListWidgetItem*)));

    refList.setFocusPolicy(Qt::NoFocus);
    lastItems.setFocusPolicy(Qt::NoFocus);
    
    lastItems.setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(&lastItems, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showLastItemsContextMenu(QPoint)));
    
    QObject::connect(buttonTools["selectTool"], SIGNAL(toggled(bool)),
                      this, SLOT(selectToolEnabled(bool)));
    
    QObject::connect(buttonTools["placeTool"], SIGNAL(toggled(bool)),
                      this, SLOT(placeToolEnabled(bool)));
    
    QObject::connect(buttonTools["autoPlaceSimpleTool"], SIGNAL(toggled(bool)),
                      this, SLOT(autoPlacementButtonEnabled(bool)));
    QObject::connect(resetRotationButton, SIGNAL(released()),
                      this, SLOT(resetRotationButtonEnabled()));
    
    QObject::connect(&stickToTDB, SIGNAL(stateChanged(int)),
                      this, SLOT(stickToTDBEnabled(int)));

    // Sound only for an actual button activation. Programmatic checked-state
    // updates do not emit clicked(), so they remain silent.
    const QList<QPushButton*> panelButtons = findChildren<QPushButton*>();
    for(QPushButton *button : panelButtons)
        QObject::connect(button, SIGNAL(clicked()),
                         this, SIGNAL(userModeChanged()), Qt::UniqueConnection);
}

ObjTools::~ObjTools() {
}

QWidget *ObjTools::autoPlacementPanel() const {
    return autoPlacementWindow;
}

void ObjTools::refreshObjLists(){
    if(route == NULL)
        return;
    routeLoaded(route);
}

void ObjTools::clearRecentItems(){
    lastItems.clear();
    lastItemsPtr.clear();
    emit requestMainFocus();
}

void ObjTools::routeLoaded(Route* a){
    if(a == NULL)
        return;
    this->route = a;
    refList.clear();
    lastItems.clear();
    refClass.clear();
    refTrack.clear();
    refRoad.clear();
    refOther.clear();
    lastItemsPtr.clear();
    currentItemList.clear();
    autoPlacementTarget.setCurrentIndex(2);
    route->placementAutoTargetType = 2;
    route->snapableOnlyRotation = Game::snapableOnlyRot;
       
    QStringList hash;
    const int fullDatabaseIndex = refClass.count();
    refClass.addItem(QString::fromUtf8("•  Full Database  •"),
                     QString("__ALL_OBJECTS__"));
    refClass.setItemData(fullDatabaseIndex, Qt::AlignLeft, Qt::TextAlignmentRole);
    QMapIterator<QString, QVector<Ref::RefItem>> i(route->ref->refItems);
    while (i.hasNext()) {
        i.next();
        hash.append(i.key());
    }
    hash.sort(Qt::CaseInsensitive);
    hash.removeDuplicates();
    for(const QString& key : hash)
        refClass.addItem(key, key);
    refClass.setMaxVisibleItems(35);
    refClass.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    TrackShape * track;

    hash.clear();
    QMap<QString, QString> trackCategoryKeys;
    QMap<QString, QString> roadCategoryKeys;

    QDir globalShapes(Game::root+"/global/shapes");
    QStringList globalShapesList;
    if(Game::ignoreMissingGlobalShapes)
        globalShapesList = globalShapes.entryList();
    if(route->tsection->shape.size() > 0)
    for (auto it = route->tsection->shape.begin(); it != route->tsection->shape.end(); ++it ){
        track = it->second;
        //hash = track->filename.left(3).toStdString();
        if(track == NULL) continue;
        if(track->dyntrack) continue;
        if(Game::ignoreMissingGlobalShapes)
            if(!globalShapesList.contains(track->filename, Qt::CaseInsensitive)) continue;
        const QString categoryLabel = tdbCategoryLabel(track->filename, track->roadshape);
        const QString categoryKey = tdbCategoryKey(categoryLabel, track->roadshape);
        if(track->roadshape)
            roadCategoryKeys.insert(categoryLabel, categoryKey);
        else
            trackCategoryKeys.insert(categoryLabel, categoryKey);
        Ref::RefItem item;
        item.filename.push_back(track->filename);
        item.description = track->filename;
        item.clas = "";
        item.type = "trackobj";
        item.value = track->id;
        route->ref->refItems[categoryKey].push_back(item);
        //qDebug() << QString::fromStdString(it->first) << " " << it->second.size();
        //if(types[hash] != 1){
        //    refTrack.addItem(QString::fromStdString(hash));//, QVariant(QString::fromStdString(hash)));
            //types[hash] = 1;
        //}
      //std::cout << " " << it->first << ":" << it->second;
    }
    refTrack.addItem("ALL");
    for(QMapIterator<QString, QString> category(trackCategoryKeys); category.hasNext(); ){
        category.next();
        refTrack.addItem(category.key(), category.value());
    }
    refTrack.setMaxVisibleItems(35);
    refTrack.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    refRoad.addItem("ALL");
    for(QMapIterator<QString, QString> category(roadCategoryKeys); category.hasNext(); ){
        category.next();
        refRoad.addItem(category.key(), category.value());
    }
    refRoad.setMaxVisibleItems(35);
    refRoad.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //refTrack.s .sortItems(Qt::AscendingOrder);
        
    refTrack.setCurrentText("ALL");

    if(Game::trackDB->loaded)  ///  EFO wrapping this in an IF because the TDB not existing causes a dump without warning
    {
        
        qDebug() << "ObjTools build Signals";
    
        SignalShape * signal;    

        QHashIterator<QString, SignalShape*> i2(Game::trackDB->sigCfg->signalShape);        
        // if(Game::trackDB->sigCfg->signalShape.size() > 0)   /// EFO un-commenting what was commented out
        while (i2.hasNext()) {
            i2.next();
            signal = i2.value();
            if(signal == NULL) continue;
            Ref::RefItem item;
            item.filename.push_back(signal->desc);
            item.description = signal->desc;
            item.clas = "signals";
            item.type = "signal";
            item.value = signal->listId;
            route->ref->refItems[QString("#TSRE#")+"signals"].push_back(item);
        }
        
        for (int i = 0; i < Game::trackDB->speedPostDAT->speedPost.size(); i++){
            if(Game::trackDB->speedPostDAT->speedPost[i]->speedSignShapeCount <= 0)
                continue;
            Ref::RefItem item;
            item.filename.push_back(Game::trackDB->speedPostDAT->speedPost[i]->name);
            item.description = Game::trackDB->speedPostDAT->speedPost[i]->name;
            item.clas = "speedsign";
            item.type = "speedpost";
            item.value = i*1000+TRitem::SIGN;
            route->ref->refItems[QString("#TSRE#")+"speedsign"].push_back(item);
        }
        for (int i = 0; i < Game::trackDB->speedPostDAT->speedPost.size(); i++){
            if(Game::trackDB->speedPostDAT->speedPost[i]->speedResumeSignShapeCount <= 0)
                continue;
            Ref::RefItem item;
            item.filename.push_back(Game::trackDB->speedPostDAT->speedPost[i]->name);
            item.description = Game::trackDB->speedPostDAT->speedPost[i]->name;
            item.clas = "speedresume";
            item.type = "speedpost";
            item.value = i*1000+TRitem::RESUME;
            route->ref->refItems[QString("#TSRE#")+"speedresume"].push_back(item);
        }
        for (int i = 0; i < Game::trackDB->speedPostDAT->speedPost.size(); i++){
            if(Game::trackDB->speedPostDAT->speedPost[i]->speedWarningSignShapeCount <= 0)
                continue;
            Ref::RefItem item;
            item.filename.push_back(Game::trackDB->speedPostDAT->speedPost[i]->name);
            item.description = Game::trackDB->speedPostDAT->speedPost[i]->name;
            item.clas = "speedwarning";
            item.type = "speedpost";
            item.value = i*1000+TRitem::WARNING;
            route->ref->refItems[QString("#TSRE#")+"speedwarning"].push_back(item);
        }
        for (int i = 0; i < Game::trackDB->speedPostDAT->speedPost.size(); i++){
            if(Game::trackDB->speedPostDAT->speedPost[i]->milepostShapeCount <= 0)
                continue;
            Ref::RefItem item;
            item.filename.push_back(Game::trackDB->speedPostDAT->speedPost[i]->name);
            item.description = Game::trackDB->speedPostDAT->speedPost[i]->name;
            item.clas = "milepost";
            item.type = "speedpost";
            item.value = i*1000+TRitem::MILEPOST;
            route->ref->refItems[QString("#TSRE#")+"milepost"].push_back(item);
        }    
    } else {
      qWarning() << "No Track Database Found, will create at Route Save, and some loading functions have been skipped";
    }
    
    if(!Game::roadDB->loaded) qWarning() << "No Road Database Found, will create at Route Save";

    for (int i = 0; i < ForestObj::forestList.size(); i++){
        Ref::RefItem item;
        item.filename.push_back(ForestObj::forestList[i].name);
        item.description = ForestObj::forestList[i].name;
        item.clas = "forests";
        item.type = "forest";
        item.value = i;
        route->ref->refItems[QString("#TSRE#")+"forests"].push_back(item);
    }
    
    foreach (SoundListItem* it, route->soundList->sources){
   //for (auto it = route->soundList->sources.constBegin(); it != route->soundList->sources.constEnd(); ++it ){
        Ref::RefItem item;
        item.filename.push_back(it->name);
        item.description = it->name;
        item.clas = "sound sources";
        item.type = "soundsource";
        item.value = it->id;
        route->ref->refItems[QString("#TSRE#")+"sound sources"].push_back(item);
    }
    foreach (SoundListItem* it, route->soundList->regions){
    //for (auto it = route->soundList->regions.begin(); it != route->soundList->regions.end(); ++it ){
        Ref::RefItem item;
        item.filename.push_back(it->name);
        item.description = it->name;
        item.clas = "sound regions";
        item.type = "soundregion";
        item.value = it->id;
        route->ref->refItems[QString("#TSRE#")+"sound regions"].push_back(item);
    }
    
    const QString nextGenDynamicTrackKey = QString("#TSRE#")
            + "nextgen dynamic track";
    route->ref->refItems[nextGenDynamicTrackKey].clear();
    Ref::RefItem nextGenDynamicTrackItem;
    nextGenDynamicTrackItem.filename.push_back("");
    nextGenDynamicTrackItem.description = "Dynamic Track (Auto-Flex)";
    nextGenDynamicTrackItem.clas = "nextgen dynamic track";
    nextGenDynamicTrackItem.type = "dyntrack";
    route->ref->refItems[nextGenDynamicTrackKey].push_back(
            nextGenDynamicTrackItem);

    Ref::RefItem item;
    item.filename.push_back("");
    item.description = "Ruler";
    item.clas = "tsre tools";
    item.type = "ruler";
    route->ref->refItems[QString("#TSRE#")+"tsre tools"].push_back(item);

    Ref::RefItem waterRulerItem;
    waterRulerItem.filename.push_back("");
    waterRulerItem.description = "Ruler (water)";
    waterRulerItem.clas = "tsre tools";
    // This is a launcher for the Water Helper workflow, not a serialized
    // world-object type. RouteEditorGLWidget intercepts it before placement.
    waterRulerItem.type = "rulerwater";
    route->ref->refItems[QString("#TSRE#")+"tsre tools"].push_back(waterRulerItem);

    Ref::RefItem vegetationRulerItem;
    vegetationRulerItem.filename.push_back("");
    vegetationRulerItem.description = "Ruler (polyveg)";
    vegetationRulerItem.clas = "tsre tools";
    vegetationRulerItem.type = "rulervegetation";
    route->ref->refItems[QString("#TSRE#")+"tsre tools"].push_back(vegetationRulerItem);

    Ref::RefItem gradeRulerItem;
    gradeRulerItem.filename.push_back("");
    gradeRulerItem.description = "Ruler (grade)";
    gradeRulerItem.clas = "tsre tools";
    gradeRulerItem.type = "rulergrade";
    route->ref->refItems[QString("#TSRE#")+"tsre tools"].push_back(gradeRulerItem);
    
    refOther.addItem("ALL");
    refOther.addItem("Signals");
    refOther.addItem("Forests");
    refOther.addItem("Sound Sources");
    refOther.addItem("Sound Regions");    
    refOther.addItem("SpeedSign");
    refOther.addItem("SpeedResume");
    refOther.addItem("SpeedWarning");
    refOther.addItem("Milepost");
    refOther.addItem("Route/Shapes Directory");
    refOther.addItem(QString::fromUtf8("•  NextGen Dynamic Track  •"),
                     QString("nextgen dynamic track"));
    refOther.addItem(QString::fromUtf8("•  TSRE Tools  •"),
                     QString("tsre tools"));
    refOther.setMaxVisibleItems(35);
    
    /// EFO add un-indexed shapes
    
    hash.clear();

    QDir routeShapes(Game::root + "/routes/" + Game::route + "/shapes" );
    routeShapes.setFilter(QDir::Files);          
    
    if(!routeShapes.exists()) qDebug() << "No Route Shapes Directory Found";
    
    foreach(QString dirFile, routeShapes.entryList()){
        if(dirFile.endsWith(".s", Qt::CaseInsensitive))                {
          
            Ref::RefItem item;
            item.filename.push_back(dirFile);
            item.description = dirFile;
            item.clas = "non-indexed";
            item.type = "static";
            route->ref->refItems[QString("#TSRE#")+"route/shapes directory"].push_back(item);
            Route::shapesList.append(dirFile.toLower());
        }
    }

        if((Game::listFiles == true) && (Game::loadAllWFiles == true))
        {    
            /// Compare loaded to shapes directory
            QFile file("./" + Game::route + "_filesNotUsed.txt");    
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                QStringList sortedShapeList = Route::shapesList;
                sortedShapeList.sort();
                for (const QString& fileName : sortedShapeList) {
                    if ((Route::fileList.contains(fileName, Qt::CaseInsensitive) == false) && (Route::trackList.contains(fileName, Qt::CaseInsensitive) == false))
                    {
                       out << fileName << " \n";
//                       qDebug() << fileName << " not used";
                    }
//                    else
  //                      qDebug() << fileName << " is used";
                      // out << fileName << " --- NOT USED \n";                                
                }
                file.close();
            }
        }
    
    resetObjectSearch();
}

void ObjTools::refClassSelected(const QString & text){
    resetCategoryCombos(&refClass);
    QString key = refClass.currentData().toString();
    if(key == "__ALL_OBJECTS__"){
        populateAllObjectList(searchBox.text());
        return;
    }
    if(key.isEmpty())
        key = text;
    populateObjectListForKey(key);
}

void ObjTools::refSearchSelected(const QString & text){
    QString key = activeCategoryKey();
    if(key.length() > 0)
        populateObjectListForKey(key, text);
    else if(refClass.currentData().toString() == "__ALL_OBJECTS__"
            || !text.trimmed().isEmpty())
        populateAllObjectList(text);
    else
        refList.clear();
}

void ObjTools::resetObjectSearch(){
    resetCategoryCombos(NULL);
    searchBox.clear();
    populateAllObjectList();
}

void ObjTools::resetCategoryCombos(QComboBox* keepActive){
    bool oldClass = refClass.blockSignals(true);
    bool oldTrack = refTrack.blockSignals(true);
    bool oldRoad = refRoad.blockSignals(true);
    bool oldOther = refOther.blockSignals(true);

    if(keepActive == NULL){
        const int fullDatabaseIndex = refClass.findData(QString("__ALL_OBJECTS__"));
        refClass.setCurrentIndex(fullDatabaseIndex);
    } else if(keepActive != &refClass)
        refClass.setCurrentIndex(-1);
    if(keepActive != &refTrack)
        refTrack.setCurrentText("ALL");
    if(keepActive != &refRoad)
        refRoad.setCurrentText("ALL");
    if(keepActive != &refOther)
        refOther.setCurrentText("ALL");

    refClass.blockSignals(oldClass);
    refTrack.blockSignals(oldTrack);
    refRoad.blockSignals(oldRoad);
    refOther.blockSignals(oldOther);

    auto showActiveCategory = [](QComboBox &combo, bool active){
        combo.setStyleSheet(active
            ? "QComboBox { combobox-popup: 0; border: 1px solid #ffad3b; }"
            : "QComboBox { combobox-popup: 0; }");
    };
    showActiveCategory(refClass, keepActive == NULL || keepActive == &refClass);
    showActiveCategory(refTrack, keepActive == &refTrack);
    showActiveCategory(refRoad, keepActive == &refRoad);
    showActiveCategory(refOther, keepActive == &refOther);
}

QString ObjTools::activeCategoryKey() const{
    if(refTrack.currentText() != "ALL")
        return refTrack.currentData().toString();
    if(refRoad.currentText() != "ALL")
        return refRoad.currentData().toString();
    if(refOther.currentText() != "ALL"){
        const QString key = refOther.currentData().toString();
        return QString("#TSRE#")
                + (key.isEmpty() ? refOther.currentText().toLower() : key);
    }
    if(refClass.currentIndex() >= 0){
        const QString key = refClass.currentData().toString();
        if(key == "__ALL_OBJECTS__")
            return QString();
        return key.isEmpty() ? refClass.currentText() : key;
    }
    return QString();
}

void ObjTools::populateObjectListForKey(QString key, QString searchText){
    refList.clear();
    currentItemList.clear();
    int idx = 0;

    for (int it = 0; it < route->ref->refItems[key].size(); ++it ){
        Ref::RefItem* item = &route->ref->refItems[key][it];
        if(searchText.length() > 0 && !item->description.contains(searchText, Qt::CaseInsensitive))
            continue;
        new QListWidgetItem ( item->description, &refList, idx++ );
        currentItemList.push_back(item);
    }
    refList.sortItems(Qt::AscendingOrder);
}

void ObjTools::populateAllObjectList(QString searchText){
    refList.clear();
    currentItemList.clear();
    QMapIterator<QString, QVector<Ref::RefItem>> i(route->ref->refItems);
    int idx = 0;
    while (i.hasNext()) {
        i.next();
        for (int it = 0; it < i.value().size(); ++it ){
            Ref::RefItem* item = (Ref::RefItem*)&i.value()[it];
            if(searchText.length() > 0 && !item->description.contains(searchText, Qt::CaseInsensitive))
                continue;
            new QListWidgetItem ( item->description, &refList, idx++ );
            currentItemList.push_back(item);
        }
    }
    refList.sortItems(Qt::AscendingOrder);
}

void ObjTools::refTrackSelected(const QString & text){

    resetCategoryCombos(text == "ALL" ? NULL : &refTrack);
    if(text == "ALL")
        refSearchSelected(searchBox.text());
    else
        populateObjectListForKey(refTrack.currentData().toString(), searchBox.text());
}

void ObjTools::refRoadSelected(const QString & text){

    resetCategoryCombos(text == "ALL" ? NULL : &refRoad);
    if(text == "ALL")
        refSearchSelected(searchBox.text());
    else
        populateObjectListForKey(refRoad.currentData().toString(), searchBox.text());
}

void ObjTools::refOtherSelected(const QString & text){

    resetCategoryCombos(text == "ALL" ? NULL : &refOther);
    if(text == "ALL")
        refSearchSelected(searchBox.text());
    else {
        const QString key = refOther.currentData().toString();
        populateObjectListForKey(QString("#TSRE#")
                + (key.isEmpty() ? text.toLower() : key), searchBox.text());
    }
}

void ObjTools::refListSelected(QListWidgetItem * item){
    try {
        route->ref->selected = currentItemList[item->type()];//&route->ref->refItems[refClass.currentText().toStdString()][refList.currentRow()];
        emit sendMsg("engItemSelected");
    } catch(const std::out_of_range& oor){
        route->ref->selected = NULL;
    }
    lastItems.clearSelection();
    emit requestMainFocus();
}

void ObjTools::lastItemsListSelected(QListWidgetItem * item){
    refList.clearSelection();
    if(Game::debugOutput) qDebug() << "ObjTools506:" << item->type() << " " << item->text();
    route->ref->selected = lastItemsPtr[item->type()];
    emit requestMainFocus();
}

void ObjTools::selectToolEnabled(bool val){
    if(val)
    {
        emit enableTool("selectTool");
        emit updStatus(QString("Stat3"), QString(""));       /// EFO Added to 
    }
    else
        emit enableTool("");
}

void ObjTools::placeToolEnabled(bool val){
    if(val)
    {
        emit enableTool("placeTool");
        emit updStatus(QString("Stat3"), QString(""));        /// EFO Added to 
    }
    else
        emit enableTool("");
}

void ObjTools::autoPlacementButtonEnabled(bool val){
    if(val)
        emit enableTool("autoPlaceSimpleTool");
    else
        emit enableTool("");
}

void ObjTools::autoPlacementPanelVisibilityChanged(bool visible){
    if(!visible){
        if(buttonTools["autoPlaceSimpleTool"] != NULL
        && buttonTools["autoPlaceSimpleTool"]->isChecked())
            buttonTools["autoPlaceSimpleTool"]->setChecked(false);
        emit requestMainFocus();
    }
}

void ObjTools::resetRotationButtonEnabled(){
    emit sendMsg("resetPlaceRotation");
}

void ObjTools::autoPlacementDeleteLastEnabled(){
    emit sendMsg("autoPlacementDeleteLast");
}

void ObjTools::itemSelected(Ref::RefItem* item){     /// EFO Item selected on the list
    if(Game::debugOutput)
    {QString selectedFilename = item->currentFilename;
    if(Game::debugOutput) qDebug() << "ObjTools548:" << "selected: " << selectedFilename;    
    }
    QString text;
    if(item->type.compare("dyntrack", Qt::CaseInsensitive) == 0)
        text = item->description.isEmpty()
                ? "Dynamic Track (Auto-Flex)" : item->description;
    else if(item->description.length() > 1)
        text = item->description;
    else if (item->getShapeName().length() > 1)
        text = item->getShapeName();
    else
        text = item->type;
    
    //Avoid duplicates
    QList<QListWidgetItem*> found = lastItems.findItems(text, Qt::MatchExactly);
       if(Game::debugOutput)  qDebug() << "ObjTools560:" << "found : "<< found.length();
    
    if ((found.length() == 0)   ){    //// Added a check for placed=true    && (item->placed)

    // here's where it gets put on the recent list    
        
    lastItemsPtr.push_back(item);  
    
    /// calculating the item age and order
    new QListWidgetItem ( text, &lastItems, lastItemsPtr.size() - 1 );
    if(lastItems.count() > Game::numRecentItems){
        int val = 2147483646;
        int itID = -1;
        for(int i = 0; i < lastItems.count(); i++){
            if(lastItems.item(i)->type() < val){
                val = lastItems.item(i)->type();
                itID = i;
            }
        }
        if(itID != -1)
            delete lastItems.takeItem(itID);
    }
    
    lastItems.sortItems();
	}
}

void ObjTools::stickToTDBEnabled(int state){
    if(state == Qt::Checked)
        this->sendMsg("stickToTDB", true);
    else
        this->sendMsg("stickToTDB", false);
}

void ObjTools::autoPlacementLengthEnabled(QString val){
    bool ok = false;
    float v = val.toFloat(&ok);
    if(!ok) return;
    
    sendMsg("autoPlacementLength", v);
}

void ObjTools::advancedPlacementButtonEnabled(bool val){
    this->advancedPlacementWidget.setVisible(val);
}

void ObjTools::msg(QString text){
}

void ObjTools::msg(QString text, bool val){
}

void ObjTools::msg(QString text, int val){
}

void ObjTools::msg(QString text, float val){
}

void ObjTools::msg(QString text, QString val){
    if(text == "toolEnabled"){
        QMapIterator<QString, QPushButton*> i(buttonTools);
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(true);
            i.value()->setChecked(false);
        }
        if(buttonTools[val] != NULL)
            buttonTools[val]->setChecked(true);
        i.toFront();
        while (i.hasNext()) {
            i.next();
            if(i.value() == NULL)
                continue;
            i.value()->blockSignals(false);
        }
    }
}

void ObjTools::autoPlacementRotTypeSelected(QString val){
    if(autoPlacementRotType.currentIndex() == 0)
        this->route->placementAutoTwoPointRot = true;
    else
        this->route->placementAutoTwoPointRot = false;
}

void ObjTools::autoPlacementTargetSelected(QString val){
    this->route->placementAutoTargetType = autoPlacementTarget.currentIndex();
}

void ObjTools::autoSnapableRadiusEnabled(QString val){
    float v;
    bool ok = false;
    v = val.toFloat(&ok);
    if(Game::debugOutput) qDebug() << "ObjTools655:" << "Game::snapableRadius"<<v;
    if(ok)
        Game::snapableRadius = v;
}


void ObjTools::chSnapableOnlyRotation(int val){
    if(val == 2)
         this->route->snapableOnlyRotation = true;
    else
         this->route->snapableOnlyRotation = false;
}

void ObjTools::autoPlacementOffsetEnabled(QString val){
    float v;
    bool ok = false;
    v = this->autoPlacementPosX.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoTranslationOffset[0] = v;
    v = this->autoPlacementPosY.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoTranslationOffset[1] = v;
    v = this->autoPlacementPosZ.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoTranslationOffset[2] = v;
    

    v = this->autoPlacementRotX.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoRotationOffset[0] = v;
    v = this->autoPlacementRotY.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoRotationOffset[1] = v;
    v = this->autoPlacementRotZ.text().toFloat(&ok);
    if(ok)
        this->route->placementAutoRotationOffset[2] = v;
}

void ObjTools::showLastItemsContextMenu(QPoint val){
    QPoint globalPos = lastItems.mapToGlobal(val);

    QMenu myMenu;
    myMenu.addAction("Find similar", this, SLOT(lastItemsMenuFindSimilar()));

    myMenu.exec(globalPos);
}

void ObjTools::lastItemsMenuFindSimilar(){
    this->searchBox.setText(lastItems.currentItem()->text().left(6));
    refSearchSelected((const QString)lastItems.currentItem()->text().left(6));
}

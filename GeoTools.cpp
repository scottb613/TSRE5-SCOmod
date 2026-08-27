/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "GeoTools.h"
#include "TexLib.h"
#include "Brush.h"
#include "Texture.h"
#include "GuiFunct.h"
#include "TransferObj.h"
#include "Game.h"
#include "Coords.h"
#include "HeightWindow.h"

#include <functional>

static int scaledUiSize(int base){
    return qRound(base * qBound(0.75f, Game::uiScale, 1.25f));
}

class MapImageryHelperWindow : public EditorPopupWindow {
public:
    MapImageryHelperWindow(QWidget *owner, std::function<void()> closed)
        : EditorPopupWindow(owner, "MAP IMAGERY", "mapImageryHelper", 720),
          closedAction(std::move(closed)) {
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        if(closedAction)
            closedAction();
    }

private:
    std::function<void()> closedAction;
};

GeoTools::GeoTools(QString name)
    : QWidget(){
    GuiFunct::applyEditorPanelStyle(this);
    setFixedWidth(scaledUiSize(250));
    QFont panelFont = font();
    if(panelFont.pointSizeF() > 0)
        panelFont.setPointSizeF(panelFont.pointSizeF() * 1.12);
    setFont(panelFont);
    buttonTools["mapTileShowTool"] = new QPushButton("Map Tile Toggle", this);
    buttonTools["mapTileLoadTool"] = new QPushButton("Load Map", this);
    buttonTools["heightTileLoadTool"] = new QPushButton("Load Height", this);
    buttonTools["makeTileTextureTool"] = new QPushButton("Make Map Tile", this);
    buttonTools["removeTileTextureTool"] = new QPushButton("Remove Map Tile", this);
    QMapIterator<QString, QPushButton*> i(buttonTools);
    while (i.hasNext()) {
        i.next();
        i.value()->setCheckable(true);
        GuiFunct::styleEditorActionButton(i.value());
    }

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(3);
    vbox->setContentsMargins(4,3,4,4);
    auto addSubtitle = [this, vbox](const QString &text) {
        QLabel *label = new QLabel(QString(QChar(0x2022)) + ' ' + text, this);
        GuiFunct::styleEditorSubtitle(label);
        vbox->addWidget(label);
    };
    auto makeCard = [this]() {
        QFrame *card = new QFrame(this);
        GuiFunct::styleEditorPanelCard(card);
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(scaledUiSize(6), scaledUiSize(5),
                                       scaledUiSize(6), scaledUiSize(5));
        cardLayout->setSpacing(scaledUiSize(5));
        return qMakePair(card, cardLayout);
    };
    QLabel *panelTitle = new QLabel("GEODATA EDITOR");
    GuiFunct::styleEditorTitle(panelTitle);
    vbox->addWidget(panelTitle);
        
    addSubtitle("Map Layers");
    const auto mapCard = makeCard();

    QFormLayout *mapProviderLayout = new QFormLayout;
    mapProviderLayout->setSpacing(2);
    mapProviderLayout->setContentsMargins(0,0,0,0);
    mapProvider.addItem("OSM Vector only", "None");
    mapProvider.addItem("Google satellite", "Google");
    mapProvider.addItem("Mapbox satellite", "Mapbox");
    mapProvider.addItem("Custom imagery", "Custom");
    mapProvider.setStyleSheet("combobox-popup: 0;");
    mapProviderLayout->addRow("Imagery:", &mapProvider);
    mapCard.second->addLayout(mapProviderLayout);

    configureMapProviderButton = new QPushButton("Configure Map Imagery", this);
    configureMapProviderButton->setCheckable(true);
    configureMapProviderButton->setProperty("scoSoundOnPress", true);
    configureMapProviderButton->setToolTip("Select and configure optional satellite imagery. OSM Vector does not require an account or API key.");
    GuiFunct::styleEditorActionButton(configureMapProviderButton);
    mapCard.second->addWidget(configureMapProviderButton);
    mapCard.second->addWidget(buttonTools["mapTileLoadTool"]);
    mapCard.second->addWidget(buttonTools["makeTileTextureTool"]);
    mapCard.second->addWidget(buttonTools["removeTileTextureTool"]);
    mapCard.second->addWidget(buttonTools["mapTileShowTool"]);
    vbox->addWidget(mapCard.first);

    addSubtitle("Terrain Heightmap");
    const auto heightCard = makeCard();
    heightCard.second->addWidget(buttonTools["heightTileLoadTool"]);
    vbox->addWidget(heightCard.first);

    addSubtitle("Automatic Tile Creation");
    const auto automaticCard = makeCard();
    QCheckBox *chAutoCreateTile = new QCheckBox("Create New Tiles");
    chAutoCreateTile->setChecked(Game::autoNewTiles);
    QCheckBox *chAutoGeoTerrain = new QCheckBox("Create Terrain Mesh");
    chAutoGeoTerrain->setChecked(Game::autoGeoTerrain);
    automaticCard.second->addWidget(chAutoCreateTile);
    automaticCard.second->addWidget(chAutoGeoTerrain);
    vbox->addWidget(automaticCard.first);

    addSubtitle("Tiles From Marker File");
    const auto markerCard = makeCard();
    markerCard.second->addWidget(&markerFiles);
    markerFiles.setStyleSheet("combobox-popup: 0;");
    QFormLayout *vlist = new QFormLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,3,0);
    vlist->addRow("Radius:",&this->eRadius);
    eRadius.setRange(0,2);
    eRadius.setValue(0);
    markerCard.second->addItem(vlist);
    QPushButton * checkGeodataFiles = new QPushButton("Geodata Availability", this);
    GuiFunct::styleEditorActionButton(checkGeodataFiles);
    QObject::connect(checkGeodataFiles, SIGNAL(released()),
                      this, SLOT(checkGeodataFilesEnabled()));
    markerCard.second->addWidget(checkGeodataFiles);
    
    QPushButton * generateTiles = new QPushButton("Generate Tiles", this);
    GuiFunct::styleEditorActionButton(generateTiles);
    QObject::connect(generateTiles, SIGNAL(released()),
                      this, SLOT(generateTilesEnabled()));
    markerCard.second->addWidget(generateTiles);
    vbox->addWidget(markerCard.first);

    addSubtitle("Distant Terrain");
    const auto distantCard = makeCard();
    QPushButton * checkGeodataLoFiles = new QPushButton("Geodata Availability", this);
    GuiFunct::styleEditorActionButton(checkGeodataLoFiles);
    //QObject::connect(checkGeodataFiles, SIGNAL(released()),
    //                  this, SLOT(checkGeodataFilesEnabled()));
    distantCard.second->addWidget(checkGeodataLoFiles);
    
    QPushButton * generateLoTiles = new QPushButton("Generate Tiles (mkr)", this);
    GuiFunct::styleEditorActionButton(generateLoTiles);
    QObject::connect(generateLoTiles, SIGNAL(released()),
                      this, SLOT(generateLoTilesEnabled()));
    distantCard.second->addWidget(generateLoTiles);
    QPushButton * generateLoTilesFromTDB = new QPushButton("Generate Tiles (tdb)", this);
    GuiFunct::styleEditorActionButton(generateLoTilesFromTDB);
    QObject::connect(generateLoTilesFromTDB, SIGNAL(released()),
                      this, SLOT(generateLoTilesFromTDBEnabled()));
    distantCard.second->addWidget(generateLoTilesFromTDB);
    vbox->addWidget(distantCard.first);
    
    vbox->addStretch(1);
    this->setLayout(vbox);

    QList<QWidget*> controls = findChildren<QWidget*>();
    for(int c = 0; c < controls.size(); c++){
        controls[c]->setFont(panelFont);
        if(qobject_cast<QPushButton*>(controls[c]) != NULL ||
           qobject_cast<QLineEdit*>(controls[c]) != NULL ||
           qobject_cast<QComboBox*>(controls[c]) != NULL ||
           qobject_cast<QSpinBox*>(controls[c]) != NULL)
            controls[c]->setMinimumHeight(scaledUiSize(20));
    }
    
    
    // signals
    QObject::connect(buttonTools["mapTileShowTool"], SIGNAL(toggled(bool)),
                      this, SLOT(mapTileShowToolEnabled(bool)));
    QObject::connect(buttonTools["mapTileLoadTool"], SIGNAL(toggled(bool)),
                      this, SLOT(mapTileLoadToolEnabled(bool)));
    
    QObject::connect(buttonTools["heightTileLoadTool"], SIGNAL(toggled(bool)),
                      this, SLOT(heightTileLoadToolEnabled(bool)));
    
    QObject::connect(buttonTools["makeTileTextureTool"], SIGNAL(toggled(bool)),
                      this, SLOT(makeTileTextureToolEnabled(bool)));
    
    QObject::connect(buttonTools["removeTileTextureTool"], SIGNAL(toggled(bool)),
                      this, SLOT(removeTileTextureToolEnabled(bool)));
    
    QObject::connect(chAutoCreateTile, SIGNAL(stateChanged(int)),
                      this, SLOT(chAutoCreateTileEnabled(int)));
    
    QObject::connect(chAutoGeoTerrain, SIGNAL(stateChanged(int)),
                      this, SLOT(chAutoGeoTerrainEnabled(int)));

    QObject::connect(&mapProvider, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(mapProviderChanged(int)));
    QObject::connect(configureMapProviderButton, &QPushButton::toggled,
                     this, &GeoTools::configureMapProviderEnabled);

    syncMapProviderUi();
    
}

void GeoTools::syncMapProviderUi(){
    const QString selectedProvider = Game::mapEngine.isEmpty() ? "None" : Game::mapEngine;
    int selectedIndex = mapProvider.findData(selectedProvider);
    if(selectedIndex < 0)
        selectedIndex = 0;

    QSignalBlocker blocker(&mapProvider);
    mapProvider.setCurrentIndex(selectedIndex);

    const QString provider = mapProvider.currentData().toString();
    const bool keyedProvider = provider == "Google" || provider == "Mapbox";
    const bool ready = !Game::imageMapsUrl.trimmed().isEmpty()
            && (!keyedProvider || !Game::MapAPIKey.trimmed().isEmpty());
    if(provider == "None")
        mapProvider.setToolTip("OSM Vector is available without an account or API key.");
    else if(ready)
        mapProvider.setToolTip(provider + " imagery is configured and will be offered by the map loader.");
    else
        mapProvider.setToolTip(provider + " is selected but still needs a URL or API key. OSM Vector remains available.");
}

void GeoTools::mapProviderChanged(int index){
    if(index < 0)
        return;
    Game::mapEngine = mapProvider.itemData(index).toString();
    Game::configureMapProvider();
    if(!Game::saveMapProviderSettings())
        QMessageBox::warning(this, "Map Settings Save Failed",
                             "The imagery selection could not be saved to settings.json.");
    syncMapProviderUi();
}

void GeoTools::configureMapProviderEnabled(bool enabled){
    GuiFunct::setEditorPopupButtonActive(configureMapProviderButton, enabled);
    if(!enabled){
        if(mapImageryWindow != NULL)
            mapImageryWindow->close();
        return;
    }

    QPointer<GeoTools> owner(this);
    MapImageryHelperWindow *helper = new MapImageryHelperWindow(
        this, [owner]() {
            if(owner.isNull())
                return;
            owner->mapImageryWindow = NULL;
            GuiFunct::setEditorPopupButtonActive(
                owner->configureMapProviderButton, false);
            const QSignalBlocker blocker(owner->configureMapProviderButton);
            owner->configureMapProviderButton->setChecked(false);
        });
    mapImageryWindow = helper;
    helper->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout *mainLayout = helper->popupLayout();
    QLabel *guidance = new QLabel(
        "OSM Vector is always available without an account or API key. "
        "Satellite services require credentials supplied by their provider.");
    guidance->setWordWrap(true);
    mainLayout->addWidget(guidance);

    QFormLayout *form = new QFormLayout;
    QComboBox *provider = new QComboBox;
    provider->addItem("OSM Vector only", "None");
    provider->addItem("Google satellite", "Google");
    provider->addItem("Mapbox satellite", "Mapbox");
    provider->addItem("Custom imagery", "Custom");

    QSpinBox *resolution = new QSpinBox;
    resolution->setRange(256, 8192);
    resolution->setSingleStep(256);
    resolution->setValue(Game::mapImageResolution);
    resolution->setSuffix(" px");

    QLineEdit *url = new QLineEdit;
    QLineEdit *apiKey = new QLineEdit;
    apiKey->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QSpinBox *zoomOffset = new QSpinBox;
    zoomOffset->setRange(-10, 10);

    QLabel *urlLabel = new QLabel("Image URL:");
    QLabel *keyLabel = new QLabel("API key/token:");
    QLabel *zoomLabel = new QLabel("Zoom offset:");
    form->addRow("Provider:", provider);
    form->addRow("Map resolution:", resolution);
    form->addRow(urlLabel, url);
    form->addRow(keyLabel, apiKey);
    form->addRow(zoomLabel, zoomOffset);
    mainLayout->addLayout(form);

    auto loadProviderFields = [=](const QString& providerName) {
        const QString selected = providerName.toLower();
        const bool usesImagery = selected != "none";
        urlLabel->setVisible(usesImagery);
        url->setVisible(usesImagery);
        keyLabel->setVisible(usesImagery);
        apiKey->setVisible(usesImagery);
        zoomLabel->setVisible(usesImagery);
        zoomOffset->setVisible(usesImagery);

        if(selected == "google"){
            url->setText(Game::googleImageMapsUrl);
            apiKey->setText(Game::googleMapAPIKey);
            zoomOffset->setValue(Game::googleImageMapsZoomOffset);
        } else if(selected == "mapbox"){
            url->setText(Game::mapboxImageMapsUrl);
            apiKey->setText(Game::mapboxMapAPIKey);
            zoomOffset->setValue(Game::mapboxImageMapsZoomOffset);
        } else if(selected == "custom"){
            url->setText(Game::customImageMapsUrl);
            apiKey->setText(Game::customMapAPIKey);
            zoomOffset->setValue(Game::customImageMapsZoomOffset);
        } else {
            url->clear();
            apiKey->clear();
            zoomOffset->setValue(0);
        }
    };

    int providerIndex = provider->findData(Game::mapEngine.isEmpty() ? "None" : Game::mapEngine);
    provider->setCurrentIndex(providerIndex >= 0 ? providerIndex : 0);
    loadProviderFields(provider->currentData().toString());
    QObject::connect(provider, &QComboBox::currentTextChanged, helper, [=]() {
        loadProviderFields(provider->currentData().toString());
    });

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Close);
    mainLayout->addWidget(buttons);
    QObject::connect(buttons->button(QDialogButtonBox::Save),
                     &QPushButton::clicked, helper, [=]() {
        const QString selected = provider->currentData().toString();
        if(selected == "Google"){
            Game::googleImageMapsUrl = url->text().trimmed();
            Game::googleMapAPIKey = apiKey->text().trimmed();
            Game::googleImageMapsZoomOffset = zoomOffset->value();
        } else if(selected == "Mapbox"){
            Game::mapboxImageMapsUrl = url->text().trimmed();
            Game::mapboxMapAPIKey = apiKey->text().trimmed();
            Game::mapboxImageMapsZoomOffset = zoomOffset->value();
        } else if(selected == "Custom"){
            Game::customImageMapsUrl = url->text().trimmed();
            Game::customMapAPIKey = apiKey->text().trimmed();
            Game::customImageMapsZoomOffset = zoomOffset->value();
        }

        Game::mapEngine = selected;
        Game::mapImageResolution = resolution->value();
        Game::configureMapProvider();
        if(!Game::saveMapProviderSettings())
            QMessageBox::warning(helper, "Map Settings Save Failed",
                                 "The F3 imagery settings could not be saved to settings.json.");
        if(!owner.isNull())
            owner->syncMapProviderUi();
        helper->close();
    });
    QObject::connect(buttons->button(QDialogButtonBox::Close),
                     &QPushButton::clicked, helper, &QWidget::close);
    helper->finalizePopup();
    helper->showExclusive();
}

void GeoTools::mkrList(QMap<QString, Coords*> list){
    markerFiles.clear();
    mkrFiles = list;
    for (auto it = list.begin(); it != list.end(); ++it ){
        if(it.value() == NULL)
            continue;
        if(!it.value()->loaded)
            continue;
        if(it.key().startsWith("|"))
            continue;
        markerFiles.addItem(it.key());
    }
    //    mkrFilesSelected(markerFiles.itemText(0));
}

void GeoTools::checkGeodataFilesEnabled(){
    if(markerFiles.count() == 0)
        return;
    Coords* c = mkrFiles[markerFiles.currentText()];
    if(c == NULL) 
        return;
    
    QMap<int, QPair<int, int>*> tileList;
    int radius = eRadius.value();
    c->getTileList(tileList, radius);

    /*int x, z;
    QMapIterator<int, QPair<int, int>*> i2(tileList);
    while (i2.hasNext()) {
        i2.next();
        if(i2.value() == NULL)
            continue;
        x = i2.value()->first;
        z = i2.value()->second;
        qDebug() << x << z;
    }*/
    HeightWindow::CheckForMissingGeodataFiles(tileList);
}

void GeoTools::generateTilesEnabled(){
    if(markerFiles.count() == 0)
        return;
    Coords* c = mkrFiles[markerFiles.currentText()];
    if(c == NULL) 
        return;
    
    QMap<int, QPair<int, int>*> tileList;
    int radius = eRadius.value();
    c->getTileList(tileList, radius);
    
    emit createNewTiles(tileList);
}

void GeoTools::generateLoTilesEnabled(){
    if(markerFiles.count() == 0)
        return;
    Coords* c = mkrFiles[markerFiles.currentText()];
    if(c == NULL) 
        return;
    
    QMap<int, QPair<int, int>*> tileList;
    int radius = eRadius.value();
    c->getTileList(tileList, radius, 8);
    
    emit createNewLoTiles(tileList);
}

void GeoTools::generateLoTilesFromTDBEnabled(){

    TDB* tdb = Game::trackDB;
    if(tdb == NULL) 
        return;
    
    QMap<int, QPair<int, int>*> tileList;
    int radius = eRadius.value();
    tdb->getUsedTileList(tileList, radius, 8);
    
    emit createNewLoTiles(tileList);
}

void GeoTools::chAutoCreateTileEnabled(int state){
    if(state == Qt::Checked)
        Game::autoNewTiles = true;
    else
        Game::autoNewTiles = false;
}

void GeoTools::chAutoGeoTerrainEnabled(int state){
    if(state == Qt::Checked)
        Game::autoGeoTerrain = true;
    else
        Game::autoGeoTerrain = false;
}

void GeoTools::mapTileShowToolEnabled(bool val){
    if(val){
        emit enableTool("mapTileShowTool");
    } else {
        emit enableTool("");
    }
}

void GeoTools::mapTileLoadToolEnabled(bool val){
    if(val){
        emit enableTool("mapTileLoadTool");
    } else {
        emit enableTool("");
    }
}

void GeoTools::heightTileLoadToolEnabled(bool val){
    if(val){
        emit enableTool("heightTileLoadTool");
    } else {
        emit enableTool("");
    }
}

void GeoTools::makeTileTextureToolEnabled(bool val){
    if(val){
        emit enableTool("makeTileTextureTool");
    } else {
        emit enableTool("");
    }
}

void GeoTools::removeTileTextureToolEnabled(bool val){
    if(val){
        emit enableTool("removeTileTextureTool");
    } else {
        emit enableTool("");
    }
}

GeoTools::~GeoTools() {
}

void GeoTools::msg(QString text, QString val){
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

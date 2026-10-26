// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "PolyVegHelper.h"
#include "EditorCollapsibleSection.h"

#include "ForestDefinition.h"
#include "Game.h"
#include "GuiFunct.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QFormLayout>
#include <QStackedWidget>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace {

constexpr int recipeSliderIncrement = 1000;
constexpr int rulerWidthSliderIncrement = 10;

class SelectAllSpinBox : public QSpinBox {
public:
    explicit SelectAllSpinBox(QWidget *parent = nullptr)
        : QSpinBox(parent) {
        lineEdit()->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if(watched == lineEdit() && event->type() == QEvent::MouseButtonPress) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton) {
                QTimer::singleShot(0, lineEdit(), &QLineEdit::selectAll);
            }
        }
        return QSpinBox::eventFilter(watched, event);
    }
};

class SelectAllDoubleSpinBox : public QDoubleSpinBox {
public:
    explicit SelectAllDoubleSpinBox(QWidget *parent = nullptr)
        : QDoubleSpinBox(parent) {
        lineEdit()->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if(watched == lineEdit() && event->type() == QEvent::MouseButtonPress) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton) {
                QTimer::singleShot(0, lineEdit(), &QLineEdit::selectAll);
            }
        }
        return QDoubleSpinBox::eventFilter(watched, event);
    }
};

QString polyVegHelperSettingsPath() {
    return Game::routeAppDataDir() + "/polyveg-helper.json";
}

QJsonObject readPolyVegHelperSettings() {
    QFile file(polyVegHelperSettingsPath());
    if(!file.open(QIODevice::ReadOnly))
        return QJsonObject();
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject();
}

void writePolyVegHelperSettings(const QJsonObject &settings) {
    QSaveFile file(polyVegHelperSettingsPath());
    if(!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to save PolyVeg helper settings" << file.fileName()
                   << file.errorString();
        return;
    }
    file.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
    if(!file.commit())
        qWarning() << "Unable to commit PolyVeg helper settings" << file.fileName()
                   << file.errorString();
}

}

PolyVegHelper::PolyVegHelper(QWidget *parent)
    : QWidget(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    auto *layout = new QVBoxLayout(this);
    const auto scaled = [](int pixels) { return qRound(pixels*qBound(0.75f, Game::uiScale, 1.25f)); };
    layout->setContentsMargins(scaled(6), scaled(6), scaled(6), scaled(6));
    layout->setSpacing(scaled(5));
    auto *heading = new QLabel("PolyVeg Planter", this);
    GuiFunct::styleEditorTitle(heading);
    layout->addWidget(heading);
    recipeHome = new QWidget(this);
    recipeHomeLayout = new QVBoxLayout(recipeHome);
    recipeHomeLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(recipeHome);
    recipePanel = new QFrame(recipeHome);
    GuiFunct::styleEditorPanelCard(recipePanel);
    recipeHomeLayout->addWidget(recipePanel);
    auto *recipeLayout = new QVBoxLayout(recipePanel);
    recipeLayout->setContentsMargins(scaled(6), scaled(5), scaled(6), scaled(5));
    recipeLayout->setSpacing(scaled(3));
    definition = new QComboBox(recipePanel);
    definition->setMinimumWidth(0);
    definition->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    auto *editSchema = new QPushButton("Edit", recipePanel);
    GuiFunct::styleEditorActionButton(editSchema);
    editSchema->setToolTip("Edit schema (Shift+F6).");
    auto *schemaRow = new QHBoxLayout;
    schemaRow->addWidget(new QLabel("Schema:", recipePanel));
    schemaRow->addWidget(definition, 1);
    recipeLayout->addLayout(schemaRow);
    editSchema->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    recipeLayout->addWidget(editSchema);
    connect(editSchema, &QPushButton::clicked, this, &PolyVegHelper::schemaEditorRequested);
    auto borderedCell = [scaled](QWidget *parent, const QString &name,
                                 QWidget *editor, QLabel *caption = nullptr) {
        auto *cell = new QFrame(parent);
        cell->setObjectName(name);
        GuiFunct::styleEditorPanelCard(cell);
        auto *cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(scaled(3), scaled(2), scaled(3), scaled(2));
        cellLayout->setSpacing(scaled(2));
        if(caption) cellLayout->addWidget(caption);
        cellLayout->addWidget(editor);
        return cell;
    };
    auto *values = new QGridLayout;
    values->setContentsMargins(0, 0, 0, 0);
    values->setHorizontalSpacing(scaled(5));
    values->setVerticalSpacing(scaled(2));
    values->setColumnStretch(0, 1);
    values->setColumnStretch(1, 1);
    densityLabel = new QLabel("Density:", recipePanel);
    capLabel = new QLabel("Cap:", recipePanel);
    seedLabel = new QLabel("Seed", recipePanel);
    density = new SelectAllDoubleSpinBox(recipePanel);
    density->setDecimals(0);
    density->setSingleStep(recipeSliderIncrement);
    density->setGroupSeparatorShown(true);
    maximumTrees = new SelectAllSpinBox(recipePanel);
    maximumTrees->setSingleStep(recipeSliderIncrement);
    maximumTrees->setGroupSeparatorShown(true);
    seed = new QComboBox(recipePanel);
    for(int value = 1; value <= 20; ++value) seed->addItem(QString::number(value), value);
    seed->setToolTip("Range: 1–20. The same boundary, settings and seed reproduce the candidate layout.");
    values->addWidget(borderedCell(recipePanel, "polyVegDensityCell", density, densityLabel), 0, 0);
    values->addWidget(borderedCell(recipePanel, "polyVegCapCell", maximumTrees, capLabel), 0, 1);
    values->addWidget(borderedCell(recipePanel, "polyVegSeedCell", seed, seedLabel), 0, 2);
    recipeLayout->addLayout(values);
    useOsm = new QCheckBox("Use OSM", recipePanel);
    useOsm->setChecked(true);
    useOsm->setToolTip("Require mapped forest coverage. Off hides Map Polygons and bypasses OSM; terrain and schema clearances still apply.");
    disablePlantReport = new QCheckBox("No Report", recipePanel);
    disablePlantReport->setChecked(true);
    disablePlantReport->setToolTip("Suppress the planting detail report. Input/cache errors still appear. Enabled by default.");
    auto *plantOptions = new QHBoxLayout;
    plantOptions->setContentsMargins(0, 0, 0, 0);
    plantOptions->setSpacing(scaled(4));
    plantOptions->addWidget(borderedCell(recipePanel, "polyVegOsmCell", useOsm), 1);
    plantOptions->addWidget(borderedCell(recipePanel, "polyVegReportCell", disablePlantReport), 1);
    recipeLayout->addLayout(plantOptions);

    rowControls = new QWidget(recipePanel);
    auto *rowLayout = new QFormLayout(rowControls);
    rowLayout->setFormAlignment(Qt::AlignRight | Qt::AlignTop);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    auto rowField = [this](int maximum, int value) {
        auto *field = new SelectAllSpinBox(rowControls);
        field->setRange(0, maximum);
        field->setValue(value);
        return field;
    };
    rowWidthValue = rowField(500, 10);
    rowSpacingValue = rowField(500, 10);
    rowDirectionValue = rowField(360, 0);
    rowWidthValue->setSpecialValueText("Auto");
    rowSpacingValue->setSpecialValueText("Auto");
    rowWidthValue->setToolTip("Range: 0–500 m. Separation between rows; 0 (Auto) uses the schema minimum separation.");
    rowSpacingValue->setToolTip("Range: 0–500 m. Plant spacing along each row; 0 (Auto) uses the schema minimum separation.");
    rowDirectionValue->setSuffix(QString(QChar(0x00B0)));
    rowDirectionValue->setToolTip("Range: 0–360 degrees clockwise from route-plan north.");
    rowLayout->addRow("Row Separation (m):", rowWidthValue);
    rowLayout->addRow("Plant Spacing (m):", rowSpacingValue);
    rowLayout->addRow("Direction:", rowDirectionValue);
    rowsSection = new EditorCollapsibleSection("Rows", rowControls, recipePanel);
    rows = rowsSection->toggleButton();
    rows->setToolTip("Expand to enable row planting; collapse to use Density. Straight parallel rows in the route plan, independent of corridor direction.");
    recipeLayout->addWidget(rowsSection);

    auto *plantToolSubtitle = new QLabel(QString(QChar(0x2022)) + " Plant Tool", this);
    GuiFunct::styleEditorSubtitle(plantToolSubtitle);
    layout->addWidget(plantToolSubtitle);
    methodButtons = new QButtonGroup(this);
    methodButtons->setExclusive(true);
    auto *methodGrid = new QGridLayout;
    methodGrid->setContentsMargins(0, 0, 0, 0);
    methodGrid->setSpacing(0);
    const QString methodNames[] = {"Map Polygons", "Ruler / Area", "Full Tile"};
    for(int page = 0; page < 3; ++page) {
        methodLabels[page] = new QLabel(methodNames[page], this);
        methodLabels[page]->setAlignment(Qt::AlignCenter);
        methodLabels[page]->setWordWrap(true);
        methodChoices[page] = new QCheckBox(this);
        methodChoices[page]->setAccessibleName(methodNames[page]);
        methodButtons->addButton(methodChoices[page], page);
        auto *labelCell = borderedCell(this,
            QString("polyVegMethodLabel%1").arg(page), methodLabels[page]);
        auto *choiceCell = borderedCell(this,
            QString("polyVegMethodChoice%1").arg(page), methodChoices[page]);
        choiceCell->layout()->setAlignment(methodChoices[page], Qt::AlignHCenter);
        methodGrid->addWidget(labelCell, 0, page);
        methodGrid->addWidget(choiceCell, 1, page);
        methodGrid->setColumnStretch(page, 1);
        if(page == 0) methodChoices[page]->setChecked(true);
        methodLabels[page]->setEnabled(page == 0);
    }
    methodChoices[0]->setToolTip("Plant Map Polygons. Requires Use OSM.");
    methodChoices[1]->setToolTip("Plant along a Ruler or inside a Closed Area.");
    methodChoices[2]->setToolTip("Plant a Full Tile by clicking terrain.");
    layout->addLayout(methodGrid);
    methodPages = new QStackedWidget(this);
    auto *methods = methodPages;
    layout->addWidget(methods);
    auto *mapPage = new QWidget(methods);
    auto *mapLayout = new QVBoxLayout(mapPage);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    floodFill = new QCheckBox("Flood Fill", mapPage);
    floodFill->setToolTip("Plant all polygons on the clicked tile with the same LIDEX category and fill colour.");
    mapLayout->addWidget(floodFill);
    auto *mapHelp = new QLabel("Right-click a map polygon and choose Plant PolyVeg.", mapPage);
    mapHelp->setWordWrap(true);
    mapLayout->addWidget(mapHelp);
    methods->addWidget(mapPage);
    auto *rulerPage = new QWidget(methods);
    auto *rulerLayout = new QVBoxLayout(rulerPage);
    rulerLayout->setContentsMargins(0, 0, 0, 0);
    rulerWidthValue = new SelectAllSpinBox(rulerPage);
    rulerWidthValue->setRange(0, 1000);
    rulerWidthValue->setSingleStep(rulerWidthSliderIncrement);
    rulerWidthValue->setValue(100);
    rulerWidthValue->setToolTip("Corridor: full width. Area: exterior buffer width. Metres.");
    auto *widthRow = new QFormLayout;
    widthRow->setFormAlignment(Qt::AlignRight | Qt::AlignTop);
    widthRow->addRow("Width (m):", rulerWidthValue);
    rulerLayout->addLayout(widthRow);
    placeRuler = new QPushButton("New Ruler", rulerPage);
    addRulerPoints = new QPushButton("Add Points", rulerPage);
    editRulerPoints = new QPushButton("Edit Points", rulerPage);
    auto *plantRuler = new QPushButton("Plant Ruler", rulerPage);
    rulerModeButtons = new QButtonGroup(this);
    rulerModeButtons->setExclusive(false);
    auto *rulerActions = new QGridLayout;
    int index = 0;
    for(auto *button : {placeRuler, addRulerPoints, editRulerPoints, plantRuler}) {
        GuiFunct::styleEditorActionButton(button);
        rulerActions->addWidget(button, index / 2, index % 2);
        if(button != plantRuler) {
            button->setCheckable(true);
            rulerModeButtons->addButton(button);
        }
        ++index;
    }
    rulerLayout->addLayout(rulerActions);
    rulerArea = new QCheckBox("Closed Area", rulerPage);
    rulerOverride = new QRadioButton("Unrestricted", rulerPage);
    rulerOverride->setAutoExclusive(false);
    auto *rulerOptions = new QHBoxLayout;
    rulerOptions->setContentsMargins(0, 0, 0, 0);
    rulerOptions->setSpacing(scaled(4));
    rulerOptions->addWidget(borderedCell(rulerPage, "polyVegClosedAreaCell", rulerArea), 1);
    rulerOptions->addWidget(borderedCell(rulerPage, "polyVegUnrestrictedCell", rulerOverride), 1);
    rulerLayout->addLayout(rulerOptions);
    methods->addWidget(rulerPage);
    auto *tilePage = new QWidget(methods);
    auto *tileLayout = new QVBoxLayout(tilePage);
    tileLayout->setContentsMargins(0, 0, 0, 0);
    plantFullTile = new QPushButton("Plant Full Tile", tilePage);
    plantFullTile->setCheckable(true);
    GuiFunct::styleEditorActionButton(plantFullTile);
    plantFullTile->setToolTip("Arm, then click terrain to plant one tile. Click again to cancel.");
    tileLayout->addWidget(plantFullTile);
    methods->addWidget(tilePage);
    connect(methodButtons, &QButtonGroup::idToggled, this,
            [this](int, bool checked) { if(checked) selectMethodPage(); });

    for(int i = 1; i < methods->count(); ++i)
        methods->widget(i)->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    auto *bakeOpsSubtitle = new QLabel(QString(QChar(0x2022)) + " Bake Ops", this);
    GuiFunct::styleEditorSubtitle(bakeOpsSubtitle);
    layout->addWidget(bakeOpsSubtitle);
    auto *bakeBody = new QWidget(this);
    auto *bakeActions = new QGridLayout(bakeBody);
    bakeActions->setContentsMargins(0, 0, 0, 0);
    bakeTile = new QPushButton("Bake Tile", bakeBody);
    auto *bake = bakeTile;
    bakeTile->setCheckable(true);
    bakeTile->setToolTip("Arm, then click the tile to bake its raw PolyVeg. Resets after one click. Click again, press Escape or change tools to cancel.");
    auto *bakeAll = new QPushButton("Bake Area", bakeBody);
    clearBake = new QPushButton("Clear Bake", bakeBody);
    clearBake->setCheckable(true);
    clearBake->setToolTip("Arm, then click a tile to remove all its PolyVeg bakes. Resets after one click. Save commits JSON and generated-file cleanup; Undo before Save restores the bakes. Raw plants remain.");
    for(auto *button : {bake, bakeAll, clearBake}) GuiFunct::styleEditorActionButton(button);
    bakeActions->addWidget(bake, 0, 0);
    bakeActions->addWidget(clearBake, 0, 1);
    bakeActions->addWidget(bakeAll, 1, 0, 1, 2);
    bakeActions->setColumnStretch(0, 1);
    bakeActions->setColumnStretch(1, 1);
    bakeAll->setToolTip("Bake pending PolyVeg on loaded tiles inside camera Tile LOD. Save also bakes pending PolyVeg.");
    layout->addWidget(bakeBody);
    statusMessage = new QPlainTextEdit(this);
    statusMessage->setReadOnly(true);
    statusMessage->setFocusPolicy(Qt::NoFocus);
    statusMessage->setFixedHeight(statusMessage->fontMetrics().lineSpacing() * 2 + 12);
    statusMessage->setPlainText("Ready.");
    layout->addWidget(statusMessage);
    auto *statusCard = new QFrame(this);
    statusCard->setObjectName("polyVegStatusCard");
    GuiFunct::styleEditorPanelCard(statusCard);
    statusCard->setStyleSheet("QLabel[polyVegStatusValue=\"true\"] { color: #b76512; border: none; }");
    statusCard->setToolTip("Loaded-world totals: individual raw PolyVeg, baked blocks, and tiles containing baked blocks.");
    auto *counts = new QHBoxLayout(statusCard);
    counts->setContentsMargins(scaled(6), scaled(5), scaled(6), scaled(5));
    counts->setSpacing(scaled(4));
    statusRawValue = new QLabel("0", statusCard);
    statusBakedValue = new QLabel("0", statusCard);
    statusBakedTilesValue = new QLabel("0", statusCard);
    for(auto item : {qMakePair(QString("Raw:"), statusRawValue),
                     qMakePair(QString("Blocks:"), statusBakedValue),
                     qMakePair(QString("Tiles:"), statusBakedTilesValue)}) {
        counts->addWidget(new QLabel(item.first, statusCard));
        counts->addWidget(item.second, 1);
        item.second->setProperty("polyVegStatusValue", true);
        item.second->setAlignment(Qt::AlignCenter);
    }
    layout->addWidget(statusCard);
    layout->addStretch(1);
    connect(definition, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        applyDefinitionSettings(true);
        publishSettings();
    });
    connect(density, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &PolyVegHelper::publishSettings);
    connect(maximumTrees, qOverload<int>(&QSpinBox::valueChanged), this, &PolyVegHelper::publishSettings);
    connect(seed, qOverload<int>(&QComboBox::currentIndexChanged), this, &PolyVegHelper::publishSettings);
    connect(useOsm, &QCheckBox::toggled, this, [this](bool checked) {
        floodFill->setEnabled(checked);
        updateMethodChoices();
        publishSettings();
    });
    for(auto *check : {floodFill, disablePlantReport})
        connect(check, &QCheckBox::toggled, this, &PolyVegHelper::publishSettings);
    connect(rows, &QPushButton::toggled, this, [this](bool checked) {
        rowControls->setVisible(checked);
        publishSettings();
    });
    for(auto *field : {rowWidthValue, rowSpacingValue, rowDirectionValue})
        connect(field, qOverload<int>(&QSpinBox::valueChanged), this, &PolyVegHelper::publishSettings);
    connect(clearBake, &QPushButton::toggled, this, [this](bool active) {
        if(active) {
            bakeTile->setChecked(false);
            plantFullTile->setChecked(false);
            selectRulerMode(nullptr);
        }
        emit clearBakeRequested(active);
    });
    connect(bakeTile, &QPushButton::toggled, this, [this](bool active) {
        if(active) {
            clearBake->setChecked(false);
            plantFullTile->setChecked(false);
            selectRulerMode(nullptr);
        }
        emit bakeRequested(active);
    });
    connect(bakeAll, &QPushButton::clicked, this, [this]() {
        clearPlacementTools();
        emit bakeAllRequested();
    });
    connect(placeRuler, &QPushButton::clicked, this, [this](bool checked) {
        selectRulerMode(checked ? placeRuler : nullptr);
        if(checked) emit placeRulerRequested(rulerWidthValue->value(), rulerArea->isChecked());
        else emit removeRulerRequested();
    });
    connect(addRulerPoints, &QPushButton::clicked, this, [this]() {
        selectRulerMode(addRulerPoints); emit addRulerPointsRequested();
    });
    connect(editRulerPoints, &QPushButton::clicked, this, [this]() {
        selectRulerMode(editRulerPoints); emit editRulerPointsRequested();
    });
    connect(rulerArea, &QCheckBox::toggled, this, [this](bool checked) {
        if(rulerModeButtons->checkedButton()) emit rulerAreaChanged(checked);
    });
    connect(rulerWidthValue, qOverload<int>(&QSpinBox::valueChanged), this, &PolyVegHelper::rulerWidthChanged);
    connect(plantFullTile, &QPushButton::toggled, this, [this](bool active) {
        if(active) selectRulerMode(nullptr);
        emit fullTilePlantingRequested(active);
    });
    connect(plantRuler, &QPushButton::clicked, this, [this]() {
        plantFullTile->setChecked(false);
        emit plantRulerRequested(rulerOverride->isChecked());
    });
    for(auto *form : findChildren<QFormLayout*>()) form->setVerticalSpacing(scaled(3));
    resizeNumericFields();
}

void PolyVegHelper::selectMethodPage() {
    const int page = methodButtons->checkedId();
    for(int i = 0; i < 3; ++i)
        methodLabels[i]->setEnabled(i == page && methodChoices[i]->isEnabled());
    if(page < 0 || page >= methodPages->count() || page == methodPages->currentIndex()) return;
    clearPlacementTools();
    emit removeRulerRequested();
    for(int i = 0; i < methodPages->count(); ++i)
        methodPages->widget(i)->setSizePolicy(QSizePolicy::Preferred,
            i == page ? QSizePolicy::Preferred : QSizePolicy::Ignored);
    methodPages->setCurrentIndex(page);
}

void PolyVegHelper::updateMethodChoices() {
    const bool mapAvailable = useOsm->isChecked();
    methodChoices[0]->setEnabled(mapAvailable);
    // Keep unavailable choices visible. Fall forward only when the selected
    // method becomes unavailable; other active tools keep their selection.
    if(!mapAvailable && methodButtons->checkedId() == 0)
        methodChoices[1]->setChecked(true);
    selectMethodPage();
}

void PolyVegHelper::setRecipeHost(QWidget *host) {
    auto *destination = host ? qobject_cast<QVBoxLayout*>(host->layout()) : recipeHomeLayout;
    if(destination) destination->addWidget(recipePanel);
    recipePanel->show();
    rowsSection->setVisible(host == nullptr);
    recipeHostWidth = host ? host->property("numericFieldWidth").toInt() : 0;
    resizeNumericFields();
}

void PolyVegHelper::resizeNumericFields() {
    // Density/Cap share their compact width; Seed is the intentional narrow
    // third column. Other numeric rows retain the aligned panel width.
    int width = 0;
    for(QAbstractSpinBox *field : QList<QAbstractSpinBox*>{density, maximumTrees,
                      rowWidthValue, rowSpacingValue, rowDirectionValue, rulerWidthValue}) {
        field->setMinimumWidth(0);
        field->setMaximumWidth(QWIDGETSIZE_MAX);
        width = qMax(width, field->sizeHint().width());
    }
    density->setFixedWidth(width);
    maximumTrees->setFixedWidth(width);
    for(auto *field : {rowWidthValue, rowSpacingValue, rowDirectionValue, rulerWidthValue})
        field->setFixedWidth(qMax(width, recipeHostWidth));
    seed->setMinimumWidth(0);
    seed->setMaximumWidth(QWIDGETSIZE_MAX);
    seed->setFixedWidth(seed->sizeHint().width());
}

void PolyVegHelper::openForCurrentRoute() {
    const QString routePath = Game::root + "/routes/" + Game::route;
    loadDefinitions();
    const bool overrideAvailable = QFileInfo::exists(
        routePath
        + "/osm_data/polyveg-exclusions.geojson");
    rulerOverride->setEnabled(overrideAvailable);
    rulerOverride->setChecked(false);
    rulerOverride->setToolTip(overrideAvailable
        ? "Allows the ruler to plant anywhere while retaining the normal water, road, track, building, and developed-land exclusions."
        : "Unrestricted planting requires the LIDEX PolyVeg exclusion cache.");
    setStatus("Ready.");
    emit countsRequested();
}

void PolyVegHelper::clearRulerTools() {
    selectRulerMode(nullptr);
}

void PolyVegHelper::clearPlacementTools() {
    bakeTile->setChecked(false);
    clearBake->setChecked(false);
    plantFullTile->setChecked(false);
    selectRulerMode(nullptr);
    setStatus("Ready.");
}

void PolyVegHelper::setBakeTileActive(bool active) {
    const QSignalBlocker blocker(bakeTile);
    bakeTile->setChecked(active);
}

void PolyVegHelper::setClearBakeActive(bool active) {
    const QSignalBlocker blocker(clearBake);
    clearBake->setChecked(active);
}

void PolyVegHelper::setFullTilePlantingActive(bool active) {
    const QSignalBlocker blocker(plantFullTile);
    plantFullTile->setChecked(active);
}

void PolyVegHelper::setStatus(const QString &text) {
    statusMessage->setPlainText(text);
    statusMessage->verticalScrollBar()->setValue(0);
}

void PolyVegHelper::activateRuler() {
    methodChoices[1]->setChecked(true);
    selectRulerMode(placeRuler);
    emit placeRulerRequested(rulerWidthValue->value(), rulerArea->isChecked());
}

void PolyVegHelper::selectRulerMode(QPushButton *button) {
    for(QAbstractButton *candidate : rulerModeButtons->buttons())
        candidate->setChecked(candidate == button);
}

void PolyVegHelper::loadDefinitions() {
    QString previousId = definition->currentData().toString();
    const QJsonObject savedSettings = readPolyVegHelperSettings();
    const QString routeKey = Game::root + "/routes/" + Game::route;
    // Start each route/session in density mode. F5/F6 refreshes within this
    // route preserve an explicit Rows choice made during the current session.
    const bool keepRowsEnabled = rowsRouteKey == routeKey && rows->isChecked();
    rowsRouteKey = routeKey;
    {
        const QSignalBlocker osmBlocker(useOsm);
        const QSignalBlocker floodBlocker(floodFill);
        const QSignalBlocker reportBlocker(disablePlantReport);
        const QSignalBlocker rowsBlocker(rows);
        const QSignalBlocker rowWidthValueBlocker(rowWidthValue);
        const QSignalBlocker rowSpacingValueBlocker(rowSpacingValue);
        const QSignalBlocker rowDirectionValueBlocker(rowDirectionValue);
        useOsm->setChecked(savedSettings.value("useOsm").toBool(true));
        floodFill->setEnabled(useOsm->isChecked());
        floodFill->setChecked(savedSettings.value("floodFill").toBool(false));
        // Apply the requested new default once, then retain the user's choice.
        disablePlantReport->setChecked(savedSettings.value("noReportDefaultVersion").toInt() >= 1
            ? savedSettings.value("disablePlantReport").toBool(true) : true);
        rows->setChecked(keepRowsEnabled);
        rowWidthValue->setValue(qBound(0,
            savedSettings.value("rowWidthMetres").toInt(10), 500));
        rowSpacingValue->setValue(qBound(0,
            savedSettings.value("rowSpacingMetres").toInt(10), 500));
        rowDirectionValue->setValue(qBound(0,
            savedSettings.value("rowDirectionDegrees").toInt(0), 360));
    }
    rowsSection->setExpanded(rows->isChecked());
    updateMethodChoices();
    if(previousId.isEmpty())
        previousId = savedSettings.value("selectedDefinition").toString();
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(
        Game::root + "/routes/" + Game::route);

    const QSignalBlocker blocker(definition);
    definition->clear();
    if(!result.isValid()) {
        definition->addItem(QString::fromUtf8("•  NO SCHEMA  •"));
        definition->setEnabled(false);
        densityLabel->setEnabled(false);
        density->setEnabled(false);
        definition->setToolTip(result.errors.join("\n"));
        return;
    }

    definition->setEnabled(true);
    definition->setToolTip(QString());
    densityLabel->setEnabled(true);
    density->setEnabled(true);
    int selectedIndex = 0;
    for(const ForestRecipeDefinition &recipe : result.catalog.polyVeg) {
        const QString displayName = recipe.name == "SCO Northeastern Mixed Woodland"
            ? "SCO NE Mix Woodland" : recipe.name;
        definition->addItem(displayName, recipe.id);
        const int index = definition->count() - 1;
        definition->setItemData(index, recipe.defaultDensityPerSquareMetre,
                                Qt::UserRole + 1);
        definition->setItemData(index, recipe.densityLimitsPerSquareMetre.minimum,
                                Qt::UserRole + 2);
        definition->setItemData(index, recipe.densityLimitsPerSquareMetre.maximum,
                                Qt::UserRole + 3);
        definition->setItemData(index, recipe.defaultMaximumTrees,
                                Qt::UserRole + 4);
        definition->setItemData(index, recipe.minimumMaximumTrees,
                                Qt::UserRole + 5);
        definition->setItemData(index, recipe.maximumMaximumTrees,
                                Qt::UserRole + 6);
        if(recipe.id == previousId) selectedIndex = index;
    }
    definition->setCurrentIndex(selectedIndex);
    applyDefinitionSettings(true);
    publishSettings();
}

void PolyVegHelper::applyDefinitionSettings(bool restoreSavedValues) {
    if(definition->currentIndex() < 0 || !definition->isEnabled())
        return;

    const QSignalBlocker densityBlocker(density);
    const QSignalBlocker maximumBlocker(maximumTrees);
    const QSignalBlocker seedBlocker(seed);

    const double minimumDensity =
        definition->currentData(Qt::UserRole + 2).toDouble()*1000000.0;
    const double maximumDensity =
        definition->currentData(Qt::UserRole + 3).toDouble()*1000000.0;
    const int minimumTrees = definition->currentData(Qt::UserRole + 5).toInt();
    const int maximumTreeLimit = definition->currentData(Qt::UserRole + 6).toInt();
    density->setRange(minimumDensity, maximumDensity);

    maximumTrees->setRange(minimumTrees, maximumTreeLimit);
    const QString densityRange = QString("Range: %1–%2 plants/km². Used while Rows is collapsed.")
        .arg(density->minimum(), 0, 'f', 0)
        .arg(density->maximum(), 0, 'f', 0);
    density->setToolTip(densityRange);
    densityLabel->setToolTip(densityRange);
    const QString capRange = QString("Range: %1–%2 objects. Cap applies to the whole planting operation; Replace All Forest Regions uses this cap per tile.")
        .arg(maximumTrees->minimum()).arg(maximumTrees->maximum());
    maximumTrees->setToolTip(capRange);
    capLabel->setToolTip(capRange);

    double selectedDensity =
        definition->currentData(Qt::UserRole + 1).toDouble()*1000000.0;
    int selectedMaximumTrees = definition->currentData(Qt::UserRole + 4).toInt();
    int selectedSeed = 1;
    if(restoreSavedValues) {
        const QString definitionId = definition->currentData().toString();
        const QJsonObject recipeSettings = readPolyVegHelperSettings()
            .value("definitions").toObject().value(definitionId).toObject();
        if(recipeSettings.contains("densityPerSquareKilometre"))
            selectedDensity = recipeSettings.value(
                "densityPerSquareKilometre").toDouble(selectedDensity);
        if(recipeSettings.contains("maximumTrees"))
            selectedMaximumTrees = recipeSettings.value(
                "maximumTrees").toInt(selectedMaximumTrees);
        if(recipeSettings.contains("seed"))
            selectedSeed = recipeSettings.value("seed").toInt(selectedSeed);
    }
    density->setValue(qBound(minimumDensity, selectedDensity, maximumDensity));
    maximumTrees->setValue(qBound(minimumTrees, selectedMaximumTrees,
                                  maximumTreeLimit));
    int selectedSeedIndex = seed->findData(qMax(0, selectedSeed));
    if(selectedSeedIndex < 0) {
        seed->addItem(QString::number(qMax(0, selectedSeed)),
                      qMax(0, selectedSeed));
        selectedSeedIndex = seed->count() - 1;
    }
    seed->setCurrentIndex(selectedSeedIndex);
    QString seedRange = "Range: 1–20.";
    QStringList restoredSeeds;
    for(int i = 0; i < seed->count(); ++i) {
        const int value = seed->itemData(i).toInt();
        if(value < 1 || value > 20) restoredSeeds.append(QString::number(value));
    }
    if(!restoredSeeds.isEmpty()) seedRange += " Restored choices also available: " + restoredSeeds.join(", ") + ".";
    seedRange += " The same boundary, settings and seed reproduce the candidate layout.";
    seed->setToolTip(seedRange);
    seedLabel->setToolTip(seedRange);
    resizeNumericFields();
}

void PolyVegHelper::publishSettings() {
    if(definition->currentIndex() < 0 || !definition->isEnabled()) return;
    const QString definitionId = definition->currentData().toString();
    QJsonObject savedSettings = readPolyVegHelperSettings();
    QJsonObject definitions = savedSettings.value("definitions").toObject();
    QJsonObject recipeSettings;
    recipeSettings.insert("densityPerSquareKilometre", density->value());
    recipeSettings.insert("maximumTrees", maximumTrees->value());
    recipeSettings.insert("seed", seed->currentData().toInt());
    definitions.insert(definitionId, recipeSettings);
    savedSettings.insert("schemaVersion", 3);
    savedSettings.insert("selectedDefinition", definitionId);
    savedSettings.insert("definitions", definitions);
    savedSettings.remove("limitToPointerTile");
    savedSettings.insert("useOsm", useOsm->isChecked());
    savedSettings.insert("floodFill", floodFill->isChecked());
    savedSettings.insert("disablePlantReport", disablePlantReport->isChecked());
    savedSettings.insert("noReportDefaultVersion", 1);
    savedSettings.insert("rowsEnabled", rows->isChecked());
    savedSettings.insert("rowWidthMetres", rowWidthValue->value());
    savedSettings.insert("rowSpacingMetres", rowSpacingValue->value());
    savedSettings.insert("rowDirectionDegrees", rowDirectionValue->value());
    writePolyVegHelperSettings(savedSettings);

    emit settingsChanged(definitionId,
                         density->value()/1000000.0,
                         maximumTrees->value(),
                         seed->currentData().toULongLong(),
                         useOsm->isChecked(),
                         useOsm->isChecked() && floodFill->isChecked(),
                         disablePlantReport->isChecked(),
                         rows->isChecked(), rowWidthValue->value(), rowSpacingValue->value(),
                         rowDirectionValue->value());
}

void PolyVegHelper::setTileCounts(int rawCount, int bakedCount,
                                  int bakedTileCount) {
    statusRawValue->setText(QString::number(rawCount));
    statusBakedValue->setText(QString::number(bakedCount));
    statusBakedTilesValue->setText(QString::number(bakedTileCount));
    // Counts are delivered during scene edits too; repaint the framed values
    // immediately instead of waiting for another panel interaction.
    for(auto *value : {statusRawValue, statusBakedValue, statusBakedTilesValue})
        if(value->isVisible()) value->repaint();
}

#include "PolyVegHelper.h"

#include "ForestDefinition.h"
#include "Game.h"
#include "GuiFunct.h"

#include <QApplication>
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
#include <QPushButton>
#include <QRadioButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

namespace {

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

class CyclingJumpButton : public QPushButton {
public:
    CyclingJumpButton(const QString &text, QWidget *parent,
                      std::function<void()> advance,
                      std::function<void()> reset)
        : QPushButton(text, parent), advanceAction(std::move(advance)),
          resetAction(std::move(reset)) {
        clickTimer.setSingleShot(true);
        clickTimer.setInterval(QApplication::doubleClickInterval());
        inactivityTimer.setSingleShot(true);
        inactivityTimer.setInterval(15000);
        QObject::connect(&clickTimer, &QTimer::timeout, this, [this]() {
            if(advanceAction) advanceAction();
            inactivityTimer.start();
        });
        QObject::connect(&inactivityTimer, &QTimer::timeout, this, [this]() {
            if(resetAction) resetAction();
        });
        setToolTip("Single-click advances from nearest to progressively farther "
                   "tiles. Double-click resets immediately; 15 seconds without "
                   "another click resets automatically.");
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        QPushButton::mouseReleaseEvent(event);
        if(event->button() != Qt::LeftButton) return;
        if(suppressRelease) {
            suppressRelease = false;
            return;
        }
        clickTimer.start();
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override {
        clickTimer.stop();
        inactivityTimer.stop();
        suppressRelease = true;
        QPushButton::mouseDoubleClickEvent(event);
        if(resetAction) resetAction();
    }

private:
    QTimer clickTimer;
    QTimer inactivityTimer;
    std::function<void()> advanceAction;
    std::function<void()> resetAction;
    bool suppressRelease = false;
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
    const qreal panelScale = qMax(1.0f, Game::uiScale);
    const int numericFieldWidth = qRound(82.0 * panelScale);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(5);
    auto addSubtitle = [this, layout](const QString &text) {
        QLabel *label = new QLabel(QString(QChar(0x2022)) + ' ' + text, this);
        GuiFunct::styleEditorSubtitle(label);
        layout->addWidget(label);
    };

    QLabel *heading = new QLabel("POLYVEG PLANTER", this);
    GuiFunct::styleEditorTitle(heading);
    layout->addWidget(heading);

    addSubtitle("Planting");
    QGridLayout *planting = new QGridLayout;
    planting->setColumnStretch(1, 1);
    planting->setColumnStretch(3, 1);
    definition = new QComboBox(this);
    density = new SelectAllDoubleSpinBox(this);
    density->setDecimals(0);
    density->setSingleStep(1000.0);
    density->setGroupSeparatorShown(true);
    density->setSuffix(" trees/km2");
    densitySlider = new QSlider(Qt::Horizontal, this);
    maximumTrees = new SelectAllSpinBox(this);
    maximumTrees->setGroupSeparatorShown(true);
    maximumTrees->setSuffix(" trees");
    maximumTreesSlider = new QSlider(Qt::Horizontal, this);
    seed = new QComboBox(this);
    for(int value = 1; value <= 20; ++value)
        seed->addItem(QString::number(value), value);
    seed->setFixedWidth(80);
    seed->setToolTip(
        "The seed selects a repeatable random tree layout. Use the same seed "
        "to reproduce a layout, or choose another seed to rearrange the trees.");
    planting->addWidget(new QLabel("Definition:", this), 0, 0);
    planting->addWidget(definition, 0, 1, 1, 3);
    densityLabel = new QLabel("Density:", this);
    planting->addWidget(densityLabel, 1, 0);
    planting->addWidget(density, 1, 1);
    planting->addWidget(new QLabel("Cap:", this), 1, 2);
    planting->addWidget(maximumTrees, 1, 3);
    planting->addWidget(densitySlider, 2, 0, 1, 2);
    planting->addWidget(maximumTreesSlider, 2, 2, 1, 2);
    planting->addWidget(new QLabel("Seed:", this), 3, 0);
    planting->addWidget(seed, 3, 1, Qt::AlignLeft);
    layout->addLayout(planting);

    floodFill = new QCheckBox("Flood Fill", this);
    floodFill->setToolTip(
        "Off plants only the polygon beneath the pointer, clipped to the "
        "pointer tile. On plants every polygon on that tile with the same "
        "LIDEX category and fill color.");
    disablePlantReport = new QCheckBox("Disable Plant Report", this);
    disablePlantReport->setToolTip(
        "Suppresses the successful post-plant report. Successful planting still "
        "plays SCOsuccess.wav when UI Sounds is enabled.");
    rows = new QCheckBox("Rows", this);
    rows->setToolTip(
        "Aligns planting positions into continuous parallel rows. Row Width "
        "sets the spacing between rows and Direction rotates the row pattern.");
    QVBoxLayout *plantOptions = new QVBoxLayout;
    plantOptions->setContentsMargins(18, 0, 0, 0);
    plantOptions->addWidget(floodFill);
    plantOptions->addWidget(disablePlantReport);
    plantOptions->addWidget(rows);

    rowControls = new QWidget(this);
    QVBoxLayout *rowLayout = new QVBoxLayout(rowControls);
    rowLayout->setContentsMargins(18, 0, 0, 0);
    rowLayout->setSpacing(3);
    QHBoxLayout *rowWidthHeading = new QHBoxLayout;
    rowWidthHeading->addWidget(new QLabel("Row Width:", rowControls));
    rowWidthHeading->addStretch(1);
    rowWidthValue = new SelectAllSpinBox(rowControls);
    rowWidthValue->setRange(0, 500);
    rowWidthValue->setSingleStep(1);
    rowWidthValue->setSuffix(" m");
    rowWidthValue->setSpecialValueText("Auto");
    rowWidthValue->setValue(10);
    rowWidthValue->setFixedWidth(numericFieldWidth);
    rowWidthHeading->addWidget(rowWidthValue);
    rowLayout->addLayout(rowWidthHeading);
    rowWidth = new QSlider(Qt::Horizontal, rowControls);
    rowWidth->setRange(0, 500);
    rowWidth->setSingleStep(1);
    rowWidth->setPageStep(10);
    rowWidth->setValue(10);
    rowWidth->setToolTip(
        "Range - 0-500 m. 0 uses the selected definition's minimum separation.");
    rowWidthValue->setToolTip(rowWidth->toolTip());
    rowLayout->addWidget(rowWidth);

    QHBoxLayout *rowSpacingHeading = new QHBoxLayout;
    rowSpacingHeading->addWidget(new QLabel("Spacing:", rowControls));
    rowSpacingHeading->addStretch(1);
    rowSpacingValue = new SelectAllSpinBox(rowControls);
    rowSpacingValue->setRange(0, 500);
    rowSpacingValue->setSingleStep(1);
    rowSpacingValue->setSuffix(" m");
    rowSpacingValue->setSpecialValueText("Auto");
    rowSpacingValue->setValue(10);
    rowSpacingValue->setFixedWidth(numericFieldWidth);
    rowSpacingHeading->addWidget(rowSpacingValue);
    rowLayout->addLayout(rowSpacingHeading);
    rowSpacing = new QSlider(Qt::Horizontal, rowControls);
    rowSpacing->setRange(0, 500);
    rowSpacing->setSingleStep(1);
    rowSpacing->setPageStep(10);
    rowSpacing->setValue(10);
    rowSpacing->setToolTip(
        "Range - 0-500 m. Sets plant-to-plant spacing along each row; "
        "0 uses the selected definition's minimum separation.");
    rowSpacingValue->setToolTip(rowSpacing->toolTip());
    rowLayout->addWidget(rowSpacing);

    QHBoxLayout *rowDirectionHeading = new QHBoxLayout;
    rowDirectionHeading->addWidget(new QLabel("Direction:", rowControls));
    rowDirectionHeading->addStretch(1);
    rowDirectionValue = new SelectAllSpinBox(rowControls);
    rowDirectionValue->setRange(0, 360);
    rowDirectionValue->setSingleStep(1);
    rowDirectionValue->setSuffix(QString(QChar(0x00B0)));
    rowDirectionValue->setValue(0);
    rowDirectionValue->setFixedWidth(numericFieldWidth);
    rowDirectionHeading->addWidget(rowDirectionValue);
    rowLayout->addLayout(rowDirectionHeading);
    rowDirection = new QSlider(Qt::Horizontal, rowControls);
    rowDirection->setRange(0, 360);
    rowDirection->setSingleStep(1);
    rowDirection->setPageStep(15);
    rowDirection->setValue(0);
    rowDirection->setToolTip(
        "Range - 0-360 degrees clockwise from the route plan's north axis.");
    rowDirectionValue->setToolTip(rowDirection->toolTip());
    rowLayout->addWidget(rowDirection);
    rowControls->setVisible(false);
    plantOptions->addWidget(rowControls);
    layout->addLayout(plantOptions);

    addSubtitle("Planting Tools");
    QHBoxLayout *widthHeading = new QHBoxLayout;
    widthHeading->addWidget(new QLabel("Ruler Width:", this));
    widthHeading->addStretch(1);
    rulerWidthValue = new SelectAllSpinBox(this);
    rulerWidthValue->setRange(0, 1000);
    rulerWidthValue->setSingleStep(10);
    rulerWidthValue->setSuffix(" m");
    rulerWidthValue->setValue(100);
    rulerWidthValue->setFixedWidth(numericFieldWidth);
    widthHeading->addWidget(rulerWidthValue);
    layout->addLayout(widthHeading);
    rulerWidth = new QSlider(Qt::Horizontal, this);
    rulerWidth->setRange(0, 1000);
    rulerWidth->setSingleStep(10);
    rulerWidth->setPageStep(100);
    rulerWidth->setValue(100);
    rulerWidth->setToolTip("Range - 0-1000 m");
    rulerWidthValue->setToolTip(rulerWidth->toolTip());
    layout->addWidget(rulerWidth);

    QHBoxLayout *rulerButtons = new QHBoxLayout;
    placeRuler = new QPushButton("Ruler (PolyVeg)", this);
    placeRuler->setCheckable(true);
    placeRuler->setToolTip(
        "Click once to place a PolyVeg ruler. Click the orange button again "
        "to remove it. The first point locks the ruler to one tile.");
    QPushButton *plantRuler = new QPushButton("Plant PolyVeg Ruler", this);
    rulerButtons->addWidget(placeRuler);
    rulerButtons->addWidget(plantRuler);
    layout->addLayout(rulerButtons);

    rulerArea = new QCheckBox("Area", this);
    rulerArea->setToolTip(
        "Off creates a corridor ruler. On keeps the clicked polygon interior "
        "filled and applies Width as an exterior buffer.");

    rulerOverride = new QRadioButton("Unrestricted", this);
    rulerOverride->setAutoExclusive(false);
    rulerOverride->setToolTip(
        "Allows the ruler to plant anywhere while retaining the normal water, "
        "road, track, building, and developed-land exclusions.");
    QVBoxLayout *rulerOptions = new QVBoxLayout;
    rulerOptions->setContentsMargins(18, 0, 0, 0);
    rulerOptions->addWidget(rulerArea);
    rulerOptions->addWidget(rulerOverride);
    layout->addLayout(rulerOptions);

    addSubtitle("Bake");
    QHBoxLayout *bakeButtons = new QHBoxLayout;
    QPushButton *bake = new QPushButton("Bake PolyVeg Tile", this);
    QPushButton *bakeAll = new QPushButton("Bake PolyVeg LOD", this);
    bakeAll->setToolTip(
        "Bakes loaded tiles inside the current camera tile LOD that contain "
        "unbaked PolyVeg. It never loads the rest of the route.");
    bakeButtons->addWidget(bake);
    bakeButtons->addWidget(bakeAll);
    layout->addLayout(bakeButtons);

    addSubtitle("Jump");
    CyclingJumpButton *jumpRaw = new CyclingJumpButton(
        "PolyVeg Raw", this,
        [this]() { emit jumpRawRequested(); },
        [this]() { emit resetRawJumpRequested(); });
    CyclingJumpButton *jumpBake = new CyclingJumpButton(
        "PolyVeg Bake", this,
        [this]() { emit jumpBakeRequested(); },
        [this]() { emit resetBakeJumpRequested(); });
    QHBoxLayout *jumpButtons = new QHBoxLayout;
    jumpButtons->addWidget(jumpRaw);
    jumpButtons->addWidget(jumpBake);
    layout->addLayout(jumpButtons);
    layout->addStretch(1);

    addSubtitle("Status");
    QFrame *statusCard = new QFrame(this);
    statusCard->setObjectName("polyVegStatusCard");
    statusCard->setStyleSheet(QString(
        "QFrame#polyVegStatusCard { background-color: #252525; "
        "border: 1px solid #454545; border-radius: 3px; }"
        "QLabel[polyVegStatusValue=\"true\"] { color: %1; "
        "font-weight: bold; border: none; }"
        "QLabel[polyVegStatusCaption=\"true\"] { color: #c7c7c7; "
        "border: none; }").arg(Game::StyleMainLabel));
    statusCard->setToolTip(
        "Loaded-world totals: individual Raw objects, generated Baked block "
        "objects, and loaded tiles containing at least one baked block.");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(qRound(8.0 * panelScale),
                                     qRound(6.0 * panelScale),
                                     qRound(8.0 * panelScale),
                                     qRound(6.0 * panelScale));
    statusLayout->setSpacing(qRound(5.0 * panelScale));

    QGridLayout *bakedTilesLayout = new QGridLayout;
    bakedTilesLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *bakedTilesCaption = new QLabel("BAKED TILES", statusCard);
    bakedTilesCaption->setAlignment(Qt::AlignCenter);
    bakedTilesCaption->setProperty("polyVegStatusCaption", true);
    statusBakedTilesValue = new QLabel("0", statusCard);
    statusBakedTilesValue->setAlignment(Qt::AlignCenter);
    statusBakedTilesValue->setProperty("polyVegStatusValue", true);
    bakedTilesLayout->addWidget(statusBakedTilesValue, 0, 0);
    bakedTilesLayout->addWidget(bakedTilesCaption, 1, 0);
    bakedTilesLayout->setColumnStretch(0, 1);
    statusLayout->addLayout(bakedTilesLayout);

    QFrame *statusSeparator = new QFrame(statusCard);
    statusSeparator->setFrameShape(QFrame::HLine);
    statusSeparator->setFrameShadow(QFrame::Sunken);
    statusLayout->addWidget(statusSeparator);

    QGridLayout *objectStatusLayout = new QGridLayout;
    objectStatusLayout->setContentsMargins(0, 0, 0, 0);
    objectStatusLayout->setHorizontalSpacing(qRound(12.0 * panelScale));
    objectStatusLayout->setVerticalSpacing(0);
    const QStringList objectCaptions {
        QStringLiteral("RAW OBJECTS"),
        QStringLiteral("BAKED BLOCKS")
    };
    QLabel **objectValues[] { &statusRawValue, &statusBakedValue };
    for(int column = 0; column < 2; ++column) {
        *objectValues[column] = new QLabel("0", statusCard);
        (*objectValues[column])->setAlignment(Qt::AlignCenter);
        (*objectValues[column])->setProperty("polyVegStatusValue", true);
        QLabel *caption = new QLabel(objectCaptions[column], statusCard);
        caption->setAlignment(Qt::AlignCenter);
        caption->setProperty("polyVegStatusCaption", true);
        objectStatusLayout->addWidget(*objectValues[column], 0, column);
        objectStatusLayout->addWidget(caption, 1, column);
        objectStatusLayout->setColumnStretch(column, 1);
    }
    statusLayout->addLayout(objectStatusLayout);
    layout->addWidget(statusCard);

    connect(definition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){
        applyDefinitionSettings(true);
        publishSettings();
    });
    connect(density, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value){
        const QSignalBlocker blocker(densitySlider);
        densitySlider->setValue(qRound(value));
        publishSettings();
    });
    connect(densitySlider, &QSlider::valueChanged, this, [this](int value){
        density->setValue(value);
    });
    connect(maximumTrees, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){
        const QSignalBlocker blocker(maximumTreesSlider);
        maximumTreesSlider->setValue(value);
        publishSettings();
    });
    connect(maximumTreesSlider, &QSlider::valueChanged,
            maximumTrees, &QSpinBox::setValue);
    connect(seed, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ publishSettings(); });
    connect(floodFill, &QCheckBox::toggled,
            this, [this](bool){ publishSettings(); });
    connect(disablePlantReport, &QCheckBox::toggled,
            this, [this](bool){ publishSettings(); });
    connect(rows, &QCheckBox::toggled, this, [this](bool checked){
        rowControls->setVisible(checked);
        densityLabel->setEnabled(!checked);
        density->setEnabled(!checked);
        densitySlider->setEnabled(!checked);
        publishSettings();
    });
    connect(rowWidth, &QSlider::valueChanged, this, [this](int value){
        const QSignalBlocker blocker(rowWidthValue);
        rowWidthValue->setValue(value);
        publishSettings();
    });
    connect(rowWidthValue, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){
        const QSignalBlocker blocker(rowWidth);
        rowWidth->setValue(value);
        publishSettings();
    });
    connect(rowSpacing, &QSlider::valueChanged, this, [this](int value){
        const QSignalBlocker blocker(rowSpacingValue);
        rowSpacingValue->setValue(value);
        publishSettings();
    });
    connect(rowSpacingValue, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){
        const QSignalBlocker blocker(rowSpacing);
        rowSpacing->setValue(value);
        publishSettings();
    });
    connect(rowDirection, &QSlider::valueChanged, this, [this](int value){
        const QSignalBlocker blocker(rowDirectionValue);
        rowDirectionValue->setValue(value);
        publishSettings();
    });
    connect(rowDirectionValue, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){
        const QSignalBlocker blocker(rowDirection);
        rowDirection->setValue(value);
        publishSettings();
    });
    connect(bake, &QPushButton::clicked, this, &PolyVegHelper::bakeRequested);
    connect(bakeAll, &QPushButton::clicked, this, &PolyVegHelper::bakeAllRequested);
    connect(placeRuler, &QPushButton::toggled, this, [this](bool checked){
        if(checked) {
            emit placeRulerRequested(rulerWidth->value(),
                                     rulerArea->isChecked());
        } else {
            emit removeRulerRequested();
        }
    });
    connect(rulerArea, &QCheckBox::toggled, this, [this](bool checked){
        if(placeRuler->isChecked())
            emit rulerAreaChanged(checked);
    });
    connect(rulerWidth, &QSlider::valueChanged, this, [this](int value){
        const QSignalBlocker blocker(rulerWidthValue);
        rulerWidthValue->setValue(value);
        emit rulerWidthChanged(value);
    });
    connect(rulerWidthValue, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){
        const QSignalBlocker blocker(rulerWidth);
        rulerWidth->setValue(value);
        emit rulerWidthChanged(value);
    });
    connect(plantRuler, &QPushButton::clicked, this, [this](){
        emit plantRulerRequested(rulerOverride->isChecked());
    });
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
    emit countsRequested();
}

void PolyVegHelper::clearPlacementTools() {
    const QSignalBlocker rulerBlocker(placeRuler);
    placeRuler->setChecked(false);
}

void PolyVegHelper::loadDefinitions() {
    QString previousId = definition->currentData().toString();
    const QJsonObject savedSettings = readPolyVegHelperSettings();
    {
        const QSignalBlocker floodBlocker(floodFill);
        const QSignalBlocker reportBlocker(disablePlantReport);
        const QSignalBlocker rowsBlocker(rows);
        const QSignalBlocker rowWidthBlocker(rowWidth);
        const QSignalBlocker rowWidthValueBlocker(rowWidthValue);
        const QSignalBlocker rowSpacingBlocker(rowSpacing);
        const QSignalBlocker rowSpacingValueBlocker(rowSpacingValue);
        const QSignalBlocker rowDirectionBlocker(rowDirection);
        const QSignalBlocker rowDirectionValueBlocker(rowDirectionValue);
        floodFill->setChecked(savedSettings.value("floodFill").toBool(false));
        disablePlantReport->setChecked(
            savedSettings.value("disablePlantReport").toBool(false));
        rows->setChecked(savedSettings.value("rowsEnabled").toBool(false));
        rowWidth->setValue(qBound(0,
            savedSettings.value("rowWidthMetres").toInt(10), 500));
        rowSpacing->setValue(qBound(0,
            savedSettings.value("rowSpacingMetres").toInt(10), 500));
        rowDirection->setValue(qBound(0,
            savedSettings.value("rowDirectionDegrees").toInt(0), 360));
        rowWidthValue->setValue(rowWidth->value());
        rowSpacingValue->setValue(rowSpacing->value());
        rowDirectionValue->setValue(rowDirection->value());
    }
    rowControls->setVisible(rows->isChecked());
    densityLabel->setEnabled(!rows->isChecked());
    density->setEnabled(!rows->isChecked());
    densitySlider->setEnabled(!rows->isChecked());
    if(previousId.isEmpty())
        previousId = savedSettings.value("selectedDefinition").toString();
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(
        Game::root + "/routes/" + Game::route);

    const QSignalBlocker blocker(definition);
    definition->clear();
    if(!result.isValid()) {
        definition->addItem("polyveg.json unavailable");
        definition->setEnabled(false);
        density->setEnabled(false);
        definition->setToolTip(result.errors.join("\n"));
        return;
    }

    definition->setEnabled(true);
    definition->setToolTip(QString());
    densityLabel->setEnabled(!rows->isChecked());
    density->setEnabled(!rows->isChecked());
    densitySlider->setEnabled(!rows->isChecked());
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
    const QSignalBlocker densitySliderBlocker(densitySlider);
    const QSignalBlocker maximumBlocker(maximumTrees);
    const QSignalBlocker maximumSliderBlocker(maximumTreesSlider);
    const QSignalBlocker seedBlocker(seed);

    const double minimumDensity =
        definition->currentData(Qt::UserRole + 2).toDouble()*1000000.0;
    const double maximumDensity =
        definition->currentData(Qt::UserRole + 3).toDouble()*1000000.0;
    const int minimumTrees = definition->currentData(Qt::UserRole + 5).toInt();
    const int maximumTreeLimit = definition->currentData(Qt::UserRole + 6).toInt();
    density->setRange(minimumDensity, maximumDensity);
    densitySlider->setRange(qRound(minimumDensity), qRound(maximumDensity));
    maximumTrees->setRange(minimumTrees, maximumTreeLimit);
    maximumTreesSlider->setRange(minimumTrees, maximumTreeLimit);
    const QString densityRange = QString("Range - %1-%2/km2")
        .arg(minimumDensity, 0, 'f', 0)
        .arg(maximumDensity, 0, 'f', 0);
    density->setToolTip(densityRange);
    densitySlider->setToolTip(densityRange);
    const QString capRange = QString("Range - %1-%2")
        .arg(minimumTrees).arg(maximumTreeLimit);
    maximumTrees->setToolTip(capRange);
    maximumTreesSlider->setToolTip(capRange);

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
    densitySlider->setValue(qRound(density->value()));
    maximumTrees->setValue(qBound(minimumTrees, selectedMaximumTrees,
                                  maximumTreeLimit));
    maximumTreesSlider->setValue(maximumTrees->value());
    int selectedSeedIndex = seed->findData(qMax(0, selectedSeed));
    if(selectedSeedIndex < 0) {
        seed->addItem(QString::number(qMax(0, selectedSeed)),
                      qMax(0, selectedSeed));
        selectedSeedIndex = seed->count() - 1;
    }
    seed->setCurrentIndex(selectedSeedIndex);
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
    savedSettings.insert("floodFill", floodFill->isChecked());
    savedSettings.insert("disablePlantReport", disablePlantReport->isChecked());
    savedSettings.insert("rowsEnabled", rows->isChecked());
    savedSettings.insert("rowWidthMetres", rowWidth->value());
    savedSettings.insert("rowSpacingMetres", rowSpacing->value());
    savedSettings.insert("rowDirectionDegrees", rowDirection->value());
    writePolyVegHelperSettings(savedSettings);

    emit settingsChanged(definitionId,
                         density->value()/1000000.0,
                         maximumTrees->value(),
                         seed->currentData().toULongLong(),
                         floodFill->isChecked(),
                         disablePlantReport->isChecked(),
                         rows->isChecked(), rowWidth->value(), rowSpacing->value(),
                         rowDirection->value());
}

void PolyVegHelper::setTileCounts(int rawCount, int bakedCount,
                                  int bakedTileCount) {
    statusRawValue->setText(QString::number(rawCount));
    statusBakedValue->setText(QString::number(bakedCount));
    statusBakedTilesValue->setText(QString::number(bakedTileCount));
}

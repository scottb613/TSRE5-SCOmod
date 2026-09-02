// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "PolyVegSchemaEditor.h"

#include "AceLib.h"
#include "DdsLib.h"
#include "ForestDefinition.h"
#include "Game.h"
#include "GuiFunct.h"
#include "ImageLib.h"
#include "TexLib.h"
#include "Texture.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QCryptographicHash>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QSlider>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>
#include <memory>

namespace {

QString assetPickerPlaceholder() {
    return QString(QChar(0x2022)) + " SELECT ASSET " + QChar(0x2022);
}

int uiSize(int value) {
    return qRound(value * qBound(0.75f, Game::uiScale, 1.25f));
}

QLabel *titleLabel(const QString &text, QWidget *parent) {
    QLabel *label = new QLabel(text.toUpper(), parent);
    GuiFunct::styleEditorTitle(label);
    return label;
}

QLabel *subtitleLabel(const QString &text, QWidget *parent) {
    QLabel *label = new QLabel(QString(QChar(0x2022)) + " " + text.toUpper(), parent);
    GuiFunct::styleEditorSubtitle(label);
    return label;
}

QPushButton *actionButton(const QString &text, QWidget *parent) {
    QPushButton *button = new QPushButton(text, parent);
    button->setProperty("scoSuppressClickSound", true);
    if(PolyVegSchemaEditor *editor =
           qobject_cast<PolyVegSchemaEditor *>(parent->window())) {
        QObject::connect(button, &QPushButton::pressed,
                         editor,
                         &PolyVegSchemaEditor::userButtonSoundRequested);
    }
    GuiFunct::styleEditorActionButton(button);
    return button;
}

QDoubleSpinBox *numberField(double minimum, double maximum, int decimals,
                            QWidget *parent) {
    QDoubleSpinBox *field = new QDoubleSpinBox(parent);
    field->setRange(minimum, maximum);
    field->setDecimals(decimals);
    field->setKeyboardTracking(false);
    field->setButtonSymbols(QAbstractSpinBox::NoButtons);
    return field;
}

void quietEditorField(QWidget *field) {
    field->setStyleSheet(
        "QLineEdit, QTextEdit, QComboBox, QAbstractSpinBox {"
        " border: 1px solid #4a4a4a;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QAbstractSpinBox:focus {"
        " border: 1px solid #5b5b5b;"
        "}"
        "QLineEdit:hover, QTextEdit:hover, QComboBox:hover, QAbstractSpinBox:hover {"
        " border: 1px solid #f28c00;"
        "}"
        "QLineEdit[readOnly=\"true\"], QLineEdit[readOnly=\"true\"]:focus,"
        "QLineEdit[readOnly=\"true\"]:hover {"
        " border: 1px solid #3f3f3f; color: #a8a8a8;"
        "}");
}

QJsonArray numberRange(double minimum, double maximum) {
    QJsonArray result;
    result.append(minimum);
    result.append(maximum);
    return result;
}

QString idFromShape(const QString &shapeName) {
    QString id = QFileInfo(shapeName).completeBaseName().toLower();
    id.replace(QRegularExpression("[^a-z0-9._-]+"), "-");
    id.remove(QRegularExpression("^-+|-+$"));
    return id.isEmpty() ? QStringLiteral("vegetation") : id;
}

struct ThumbnailSource {
    QString shapePath;
    QString texturePath;
    QString cachePath;
};

ThumbnailSource thumbnailSource(const QString &routePath,
                                const QString &shapeName) {
    ThumbnailSource source;
    source.shapePath = QDir(routePath + "/Shapes").absoluteFilePath(shapeName);
    QFile shapeFile(source.shapePath);
    QString textureName;
    if(shapeFile.open(QIODevice::ReadOnly)) {
        QByteArray bytes = shapeFile.readAll();
        QByteArray compact;
        compact.reserve(bytes.size());
        for(char value : bytes) {
            if(value != '\0')
                compact.append(value);
        }
        const QString text = QString::fromLatin1(compact);
        const QRegularExpression expression(
            "([A-Za-z0-9_. -]+\\.(?:ace|dds|png|bmp|tga))",
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = expression.match(text);
        if(match.hasMatch())
            textureName = QFileInfo(match.captured(1).trimmed()).fileName();
    }
    if(textureName.isEmpty())
        textureName = QFileInfo(shapeName).completeBaseName() + ".ace";

    const QStringList textureRoots = {
        routePath + "/Textures",
        routePath + "/textures",
        Game::root + "/GLOBAL/TEXTURES",
        Game::root + "/Global/Textures"
    };
    source.texturePath = TexLib::resolveTexturePath(textureRoots, textureName, true);
    if(source.texturePath.isEmpty() || !QFileInfo::exists(source.texturePath)) {
        const QString wantedBase = QFileInfo(textureName).completeBaseName();
        for(const QString &root : textureRoots) {
            QDir directory(root);
            const QStringList candidates = directory.entryList(
                QStringList() << "*.dds" << "*.ace" << "*.png" << "*.bmp" << "*.tga",
                QDir::Files, QDir::Name | QDir::IgnoreCase);
            for(const QString &candidate : candidates) {
                if(QFileInfo(candidate).completeBaseName().compare(
                       wantedBase, Qt::CaseInsensitive) == 0) {
                    source.texturePath = directory.absoluteFilePath(candidate);
                    break;
                }
            }
            if(QFileInfo::exists(source.texturePath))
                break;
        }
    }

    const QFileInfo shapeInfo(source.shapePath);
    const QFileInfo textureInfo(source.texturePath);
    QByteArray cacheKey;
    cacheKey.append(QFileInfo(source.shapePath).absoluteFilePath().toUtf8());
    cacheKey.append(QByteArray::number(shapeInfo.lastModified().toMSecsSinceEpoch()));
    cacheKey.append(QFileInfo(source.texturePath).absoluteFilePath().toUtf8());
    cacheKey.append(QByteArray::number(textureInfo.lastModified().toMSecsSinceEpoch()));
    cacheKey.append("192");
    const QString cacheDirectory =
        Game::routeAppDataDir() + "/polyveg-thumbnails";
    source.cachePath = QDir(cacheDirectory).absoluteFilePath(
        QString::fromLatin1(QCryptographicHash::hash(
            cacheKey, QCryptographicHash::Sha1).toHex()) + ".png");
    return source;
}

} // namespace

PolyVegSchemaEditor::PolyVegSchemaEditor(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(Game::AppName + " " + Game::AppVersion + " Schema Editor");
    setAttribute(Qt::WA_DeleteOnClose, false);

    QWidget *central = new QWidget(this);
    GuiFunct::applyEditorPanelStyle(central);
    setCentralWidget(central);
    QHBoxLayout *workspace = new QHBoxLayout(central);
    workspace->setContentsMargins(uiSize(5), uiSize(5), uiSize(5), uiSize(5));
    workspace->setSpacing(uiSize(6));

    QWidget *controlPanel = new QWidget(central);
    controlPanel->setFixedWidth(uiSize(320));
    QVBoxLayout *controls = new QVBoxLayout(controlPanel);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(uiSize(5));
    controls->addWidget(titleLabel("Schema Editor", controlPanel));
    controls->addWidget(subtitleLabel("Schema", controlPanel));

    QFrame *schemaCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(schemaCard);
    QVBoxLayout *schemaCardLayout = new QVBoxLayout(schemaCard);
    schemaCardLayout->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    schemaCardLayout->setSpacing(uiSize(5));
    schemaList = new QComboBox(schemaCard);
    schemaCardLayout->addWidget(schemaList);

    QHBoxLayout *schemaActions = new QHBoxLayout;
    schemaActions->setSpacing(uiSize(4));
    QPushButton *newSchema = actionButton("NEW", schemaCard);
    QPushButton *duplicateSchema = actionButton("DUPLICATE", schemaCard);
    QPushButton *deleteSchema = actionButton("DELETE", schemaCard);
    schemaActions->addWidget(newSchema);
    schemaActions->addWidget(duplicateSchema);
    schemaActions->addWidget(deleteSchema);
    schemaCardLayout->addLayout(schemaActions);
    controls->addWidget(schemaCard);

    QFrame *identityCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(identityCard);
    QFormLayout *identity = new QFormLayout(identityCard);
    identity->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    schemaName = new QLineEdit(identityCard);
    schemaDescription = new QTextEdit(identityCard);
    schemaDescription->setFixedHeight(uiSize(82));
    identity->addRow("Name:", schemaName);
    identity->addRow("Desc:", schemaDescription);
    GuiFunct::alignEditorForm(identity, 52);
    controls->addWidget(identityCard);

    controls->addWidget(subtitleLabel("Asset Library", controlPanel));
    QFrame *assetLibraryCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(assetLibraryCard);
    QVBoxLayout *assetLibrary = new QVBoxLayout(assetLibraryCard);
    assetLibrary->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    assetLibrary->setSpacing(uiSize(5));
    assetPicker = new QComboBox(assetLibraryCard);
    assetPicker->setEditable(true);
    assetPicker->setInsertPolicy(QComboBox::NoInsert);
    assetPicker->setMaxVisibleItems(24);
    assetLibrary->addWidget(assetPicker);
    QPushButton *addShape = actionButton("ADD ASSET", assetLibraryCard);
    QPushButton *buildThumbnails = actionButton("BUILD THUMBNAILS", assetLibraryCard);
    addShape->setToolTip("Add the selected route shape to this schema.");
    buildThumbnails->setToolTip(
        "Build static square thumbnails for assets used by the selected schema.");
    assetLibrary->addWidget(addShape);
    assetLibrary->addWidget(buildThumbnails);
    controls->addWidget(assetLibraryCard);

    auto updateScalar = [this](const QString &section, const QString &key,
                               double value) {
        if(loadingFields || currentSchemaIndex < 0)
            return;
        QJsonObject recipe = currentRecipe();
        QJsonObject object = recipe.value(section).toObject();
        object[key] = value;
        recipe[section] = object;
        setCurrentRecipe(recipe);
    };
    auto addScalar = [this, updateScalar](
                         QFormLayout *form, const QString &label,
                         const QString &section, const QString &key,
                         double minimum, double maximum, int decimals,
                         double sliderIncrement, const QString &toolTip) {
        QWidget *row = new QWidget(form->parentWidget());
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(uiSize(4));
        QDoubleSpinBox *field = numberField(minimum, maximum, decimals, row);
        field->setSingleStep(sliderIncrement);
        field->setToolTip(toolTip);
        field->setProperty("schemaSection", section);
        field->setProperty("schemaKey", key);
        field->setFixedWidth(uiSize(80));
        QSlider *slider = new QSlider(Qt::Horizontal, row);
        slider->setToolTip(toolTip);
        slider->setRange(static_cast<int>(std::ceil(minimum / sliderIncrement)),
                         static_cast<int>(std::floor(maximum / sliderIncrement)));
        rowLayout->addWidget(field, 0);
        rowLayout->addWidget(slider, 1);
        form->addRow(label, row);
        connect(field, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [updateScalar, section, key, slider,
                       sliderIncrement](double value) {
            const QSignalBlocker blocker(slider);
            slider->setValue(qRound(value / sliderIncrement));
            updateScalar(section, key, value);
        });
        connect(slider, &QSlider::valueChanged, field,
                [field, sliderIncrement](int value) {
            field->setValue(static_cast<double>(value) * sliderIncrement);
        });
        return field;
    };

    controls->addWidget(subtitleLabel("Recipe Defaults", controlPanel));
    QFrame *defaultsCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(defaultsCard);
    QFormLayout *defaultsForm = new QFormLayout(defaultsCard);
    defaultsForm->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    addScalar(defaultsForm, "Density:", "defaults", "densityPerSquareKilometre",
              0.0, 100000.0, 0, 1000.0,
              "Default trees per square kilometre.");
    addScalar(defaultsForm, "Cap:", "defaults", "maximumTrees",
              0.0, 500000.0, 0, 1000.0,
              "Default maximum trees per planting operation.");
    addScalar(defaultsForm, "Track clear:", "defaults", "trackClearanceMetres",
              0.0, 100.0, 2, 0.5,
              "Minimum planting distance from track in metres.");
    addScalar(defaultsForm, "Road clear:", "defaults", "roadClearanceMetres",
              0.0, 100.0, 2, 0.5,
              "Minimum planting distance from road in metres.");
    addScalar(defaultsForm, "Water clear:", "defaults", "waterClearanceMetres",
              0.0, 100.0, 2, 0.5,
              "Minimum planting distance from submerged water terrain in metres.");
    GuiFunct::alignEditorForm(defaultsForm, 75);
    controls->addWidget(defaultsCard);

    controls->addWidget(subtitleLabel("Recipe Limits", controlPanel));
    QFrame *limitsCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(limitsCard);
    QFormLayout *limitsForm = new QFormLayout(limitsCard);
    limitsForm->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    auto addLimit = [this](QFormLayout *form, const QString &label,
                           const QString &key, double maximum, int decimals,
                           double singleStep,
                           const QString &valueDescription) {
        QWidget *row = new QWidget(form->parentWidget());
        QHBoxLayout *range = new QHBoxLayout(row);
        range->setContentsMargins(0, 0, 0, 0);
        range->setSpacing(uiSize(3));
        for(int index = 0; index < 2; ++index) {
            QDoubleSpinBox *field = numberField(0.0, maximum, decimals, row);
            field->setSingleStep(singleStep);
            field->setToolTip(QString(index == 0 ? "Minimum " : "Maximum ")
                              + valueDescription);
            field->setProperty("schemaRangeSection", "limits");
            field->setProperty("schemaRangeKey", key);
            field->setProperty("schemaRangeIndex", index);
            range->addWidget(field);
            connect(field, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this, key, field, index](double value) {
                if(loadingFields || currentSchemaIndex < 0)
                    return;
                QJsonObject recipe = currentRecipe();
                QJsonObject limits = recipe.value("limits").toObject();
                QJsonArray values = limits.value(key).toArray();
                while(values.size() < 2)
                    values.append(0.0);
                values[index] = value;
                limits[key] = values;
                recipe["limits"] = limits;
                setCurrentRecipe(recipe);
            });
        }
        form->addRow(label, row);
    };
    addLimit(limitsForm, "Density:", "densityPerSquareKilometre",
             200000.0, 0, 1000.0,
             "recipe density per square kilometre.");
    addLimit(limitsForm, "Cap:", "maximumTrees", 1000000.0, 0, 1000.0,
             "tree cap per planting operation.");
    GuiFunct::alignEditorForm(limitsForm, 75);
    controls->addWidget(limitsCard);

    controls->addWidget(subtitleLabel("Distribution", controlPanel));
    QFrame *distributionCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(distributionCard);
    QFormLayout *distributionForm = new QFormLayout(distributionCard);
    distributionForm->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    addScalar(distributionForm, "Slope:", "distribution", "maximumSlopeDegrees",
              0.0, 90.0, 2, 1.0,
              "Steepest terrain allowed, in degrees.");
    addScalar(distributionForm, "Feather:", "distribution", "edgeFeatherMetres",
              0.0, 100.0, 2, 0.5,
              "Distance over which planting thins near edges.");
    addScalar(distributionForm, "Spacing:", "distribution", "minimumSeparationMetres",
              0.0, 50.0, 2, 0.01,
              "Minimum centre-to-centre spacing in metres.");
    GuiFunct::alignEditorForm(distributionForm, 75);
    controls->addWidget(distributionCard);

    QLabel *fileSpacer = subtitleLabel("File", controlPanel);
    fileSpacer->setMinimumHeight(fileSpacer->sizeHint().height());
    fileSpacer->clear();
    controls->addWidget(fileSpacer);
    QFrame *fileCard = new QFrame(controlPanel);
    GuiFunct::styleEditorPanelCard(fileCard);
    QVBoxLayout *fileActions = new QVBoxLayout(fileCard);
    fileActions->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
    QPushButton *save = actionButton("SAVE SCHEMA", fileCard);
    QPushButton *revert = actionButton("RELOAD FROM DISK", fileCard);
    QPushButton *exit = actionButton("EXIT", fileCard);
    exit->setToolTip("Return to the main display with the F6 PolyVeg Planter open.");
    fileActions->addWidget(save);
    fileActions->addWidget(revert);
    fileActions->addWidget(exit);
    controls->addWidget(fileCard);
    controls->addStretch(1);
    statusLabel = new QLabel(controlPanel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(uiSize(42));
    controls->addWidget(statusLabel);
    QScrollArea *controlScroll = new QScrollArea(central);
    controlScroll->setWidgetResizable(true);
    controlScroll->setFrameShape(QFrame::NoFrame);
    controlScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlScroll->setFixedWidth(uiSize(390));
    controlPanel->setFixedWidth(uiSize(374));
    controlScroll->setWidget(controlPanel);
    workspace->addWidget(controlScroll);

    QFrame *workspaceDivider = new QFrame(central);
    workspaceDivider->setFrameShape(QFrame::NoFrame);
    workspaceDivider->setFixedWidth(uiSize(5));
    workspaceDivider->setStyleSheet(
        "background: #171717;"
        "border-left: 1px solid #5a5a5a;"
        "border-right: 1px solid #050505;");
    workspace->addWidget(workspaceDivider);

    QWidget *contentPanel = new QWidget(central);
    QVBoxLayout *content = new QVBoxLayout(contentPanel);
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(uiSize(5));
    content->addWidget(titleLabel("Vegetation Assets", contentPanel));
    content->addWidget(subtitleLabel("Schema Assets", contentPanel));

    assetScroll = new QScrollArea(contentPanel);
    assetScroll->setWidgetResizable(true);
    assetScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    assetScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    assetScroll->setFrameShape(QFrame::NoFrame);
    assetContainer = new QWidget(assetScroll);
    assetGrid = new QGridLayout(assetContainer);
    assetGrid->setContentsMargins(0, 0, uiSize(4), uiSize(4));
    assetGrid->setHorizontalSpacing(uiSize(6));
    assetGrid->setVerticalSpacing(uiSize(6));
    assetScroll->setWidget(assetContainer);
    content->addWidget(assetScroll, 1);
    workspace->addWidget(contentPanel, 1);

    for(QLineEdit *field : central->findChildren<QLineEdit *>())
        quietEditorField(field);
    for(QTextEdit *field : central->findChildren<QTextEdit *>())
        quietEditorField(field);
    for(QComboBox *field : central->findChildren<QComboBox *>())
        quietEditorField(field);
    for(QDoubleSpinBox *field : central->findChildren<QDoubleSpinBox *>())
        quietEditorField(field);

    connect(schemaList, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if(loadingFields)
            return;
        currentSchemaIndex = index;
        loadSchemaFields();
        refreshAssetGrid();
    });
    connect(schemaName, &QLineEdit::editingFinished, this, [this]() {
        if(loadingFields || currentSchemaIndex < 0)
            return;
        QJsonObject recipe = currentRecipe();
        recipe["name"] = schemaName->text().trimmed();
        setCurrentRecipe(recipe);
        refreshSchemaList(currentSchemaIndex);
    });
    connect(schemaDescription, &QTextEdit::textChanged, this, [this]() {
        if(loadingFields || currentSchemaIndex < 0)
            return;
        QJsonObject recipe = currentRecipe();
        recipe["description"] = schemaDescription->toPlainText();
        setCurrentRecipe(recipe);
    });
    connect(addShape, &QPushButton::clicked, this, [this]() {
        const QString selectedShape = assetPicker->currentText().trimmed();
        if(currentSchemaIndex < 0 || selectedShape.isEmpty()
            || selectedShape == assetPickerPlaceholder())
            return;
        QJsonObject recipe = currentRecipe();
        QJsonArray vegetation = recipe.value("vegetation").toArray();
        QJsonObject entry = defaultVegetation(selectedShape);
        entry["id"] = uniqueVegetationId(entry.value("id").toString(), vegetation);
        vegetation.append(entry);
        recipe["vegetation"] = vegetation;
        setCurrentRecipe(recipe);
        refreshAssetGrid();
    });
    connect(buildThumbnails, &QPushButton::clicked, this, [this]() {
        if(currentSchemaIndex < 0)
            return;
        const QJsonArray vegetation = currentRecipe().value("vegetation").toArray();
        QProgressDialog progress("Building static vegetation thumbnails...",
                                 "Cancel", 0, vegetation.size(), this);
        progress.setWindowModality(Qt::WindowModal);
        GuiFunct::applyEditorPanelStyle(&progress);
        int built = 0;
        int failed = 0;
        for(int index = 0;
            index < vegetation.size() && !progress.wasCanceled(); ++index) {
            progress.setValue(index);
            QApplication::processEvents();
            QString error;
            if(buildThumbnail(
                   vegetation.at(index).toObject().value("shape").toString(),
                   &error))
                ++built;
            else
                ++failed;
        }
        progress.setValue(vegetation.size());
        refreshAssetGrid();
        setStatus(QString("%1 thumbnail(s) built, %2 unavailable")
                  .arg(built).arg(failed), failed > 0);
    });
    connect(newSchema, &QPushButton::clicked, this, [this]() {
        bool accepted = false;
        const QString name = QInputDialog::getText(
            this, "NEW SCHEMA", "Schema name:", QLineEdit::Normal,
            QString(), &accepted).trimmed();
        if(!accepted)
            return;
        if(name.isEmpty()) {
            setStatus("A schema name is required.", true);
            return;
        }
        rootObject["schemaVersion"] = 1;
        QJsonObject recipe;
        recipe["id"] = uniqueSchemaId(name);
        recipe["name"] = name;
        recipe["description"] = "";
        QJsonObject defaults;
        defaults["densityPerSquareKilometre"] = 10000.0;
        defaults["trackClearanceMetres"] = 8.0;
        defaults["roadClearanceMetres"] = 4.0;
        defaults["waterClearanceMetres"] = 8.0;
        defaults["maximumTrees"] = 50000;
        recipe["defaults"] = defaults;
        QJsonObject limits;
        limits["densityPerSquareKilometre"] = numberRange(1000.0, 60000.0);
        limits["maximumTrees"] = numberRange(1000.0, 250000.0);
        recipe["limits"] = limits;
        QJsonObject distribution;
        distribution["maximumSlopeDegrees"] = 38.0;
        distribution["edgeFeatherMetres"] = 4.0;
        distribution["minimumSeparationMetres"] = 0.5;
        recipe["distribution"] = distribution;
        recipe["vegetation"] = QJsonArray();
        QJsonArray recipes = rootObject.value("polyVeg").toArray();
        recipes.append(recipe);
        rootObject["polyVeg"] = recipes;
        refreshSchemaList(recipes.size() - 1);
        loadSchemaFields();
        refreshAssetGrid();
        setStatus("New schema created. Add at least one asset before saving.");
    });
    connect(duplicateSchema, &QPushButton::clicked, this, [this]() {
        if(currentSchemaIndex < 0)
            return;
        QJsonObject recipe = currentRecipe();
        recipe["id"] = uniqueSchemaId(recipe.value("id").toString() + "-copy");
        recipe["name"] = recipe.value("name").toString() + " Copy";
        QJsonArray recipes = rootObject.value("polyVeg").toArray();
        recipes.append(recipe);
        rootObject["polyVeg"] = recipes;
        refreshSchemaList(recipes.size() - 1);
        loadSchemaFields();
        refreshAssetGrid();
    });
    connect(deleteSchema, &QPushButton::clicked, this, [this]() {
        QJsonArray recipes = rootObject.value("polyVeg").toArray();
        if(recipes.size() <= 1) {
            setStatus("A catalog must retain at least one schema.", true);
            return;
        }
        if(!GuiFunct::confirmDestructiveAction(
                this, "DELETE SCHEMA",
                "Delete the selected PolyVeg schema from this catalog?"))
            return;
        recipes.removeAt(currentSchemaIndex);
        rootObject["polyVeg"] = recipes;
        refreshSchemaList(qMin(currentSchemaIndex, recipes.size() - 1));
        loadSchemaFields();
        refreshAssetGrid();
    });
    connect(revert, &QPushButton::clicked, this, [this]() { loadCatalog(); });
    connect(exit, &QPushButton::clicked,
            this, &PolyVegSchemaEditor::exitToPlanterRequested);
    connect(save, &QPushButton::clicked, this, [this]() {
        if(rootObject.value("polyVeg").toArray().isEmpty()) {
            setStatus("There is no schema to save.", true);
            return;
        }
        QJsonObject catalogToSave = rootObject;
        QJsonArray recipes = catalogToSave.value("polyVeg").toArray();
        for(int recipeIndex = 0; recipeIndex < recipes.size(); ++recipeIndex) {
            QJsonObject recipe = recipes.at(recipeIndex).toObject();
            QJsonObject defaults = recipe.value("defaults").toObject();
            defaults.remove("widthMetres");
            defaults.remove("terrainDepthMetres");
            defaults.remove("variationScale");
            recipe["defaults"] = defaults;
            QJsonObject limits = recipe.value("limits").toObject();
            limits.remove("widthMetres");
            recipe["limits"] = limits;
            QJsonObject distribution = recipe.value("distribution").toObject();
            distribution.remove("preventFootprintOverlap");
            recipe["distribution"] = distribution;
            QJsonArray vegetation = recipe.value("vegetation").toArray();
            for(int assetIndex = 0; assetIndex < vegetation.size(); ++assetIndex) {
                QJsonObject asset = vegetation.at(assetIndex).toObject();
                asset.remove("stratum");
                asset.remove("maximumSlopeDegrees");
                vegetation[assetIndex] = asset;
            }
            recipe["vegetation"] = vegetation;
            recipes[recipeIndex] = recipe;
        }
        catalogToSave["polyVeg"] = recipes;

        QDir openRails(routePath() + "/OpenRails");
        if(!openRails.exists() && !QDir().mkpath(openRails.absolutePath())) {
            setStatus("Unable to create the route OpenRails directory.", true);
            return;
        }
        QTemporaryFile validationFile(openRails.absoluteFilePath("polyveg-validate-XXXXXX.json"));
        if(!validationFile.open()) {
            setStatus("Unable to create a validation file.", true);
            return;
        }
        validationFile.write(QJsonDocument(catalogToSave).toJson(QJsonDocument::Indented));
        validationFile.close();
        const ForestCatalogLoadResult validation =
            ForestDefinitionLoader::loadFile(validationFile.fileName(), shapesPath);
        if(!validation.isValid()) {
            setStatus("SAVE BLOCKED: " + validation.errors.join(" "), true);
            return;
        }
        QSaveFile output(catalogPath);
        if(!output.open(QIODevice::WriteOnly)
                || output.write(QJsonDocument(catalogToSave).toJson(QJsonDocument::Indented)) < 0
                || !output.commit()) {
            setStatus("Unable to write " + QDir::toNativeSeparators(catalogPath), true);
            return;
        }
        rootObject = catalogToSave;
        setStatus("SAVED: " + QFileInfo(catalogPath).fileName());
        emit schemaSaved();
        loadCatalog();
    });

    QAction *closeShortcut = new QAction(this);
    closeShortcut->setShortcut(QKeySequence("Shift+F6"));
    closeShortcut->setShortcutContext(Qt::WindowShortcut);
    addAction(closeShortcut);
    connect(closeShortcut, &QAction::triggered, this, [this]() {
        emit userToggleSoundRequested();
        close();
    });

    resize(uiSize(1500), uiSize(900));
    hide();
}

PolyVegSchemaEditor::~PolyVegSchemaEditor() = default;

QString PolyVegSchemaEditor::routePath() const {
    return Game::root + "/routes/" + Game::route;
}

QJsonObject PolyVegSchemaEditor::currentRecipe() const {
    const QJsonArray recipes = rootObject.value("polyVeg").toArray();
    return currentSchemaIndex >= 0 && currentSchemaIndex < recipes.size()
        ? recipes.at(currentSchemaIndex).toObject() : QJsonObject();
}

void PolyVegSchemaEditor::setCurrentRecipe(const QJsonObject &recipe) {
    QJsonArray recipes = rootObject.value("polyVeg").toArray();
    if(currentSchemaIndex < 0 || currentSchemaIndex >= recipes.size())
        return;
    recipes[currentSchemaIndex] = recipe;
    rootObject["polyVeg"] = recipes;
}

void PolyVegSchemaEditor::showForCurrentRoute() {
    loadCatalog();
    showMaximized();
    raise();
    activateWindow();
    emit visibilityChanged(true);
}

void PolyVegSchemaEditor::loadCatalog() {
    loadedRoutePath = routePath();
    catalogPath = loadedRoutePath + "/OpenRails/polyveg.json";
    shapesPath = loadedRoutePath + "/shapes";
    refreshShapeList();

    const ForestCatalogLoadResult validation =
        ForestDefinitionLoader::loadFile(catalogPath, shapesPath);
    QFile input(catalogPath);
    QJsonParseError parseError;
    if(!input.open(QIODevice::ReadOnly)) {
        rootObject = QJsonObject();
        refreshSchemaList();
        refreshAssetGrid();
        setStatus("Unable to open " + QDir::toNativeSeparators(catalogPath), true);
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        rootObject = QJsonObject();
        refreshSchemaList();
        refreshAssetGrid();
        setStatus("Invalid polyveg.json: " + parseError.errorString(), true);
        return;
    }
    rootObject = document.object();
    refreshSchemaList(0);
    loadSchemaFields();
    refreshAssetGrid();
    if(validation.isValid())
        setStatus(QString("%1 schema(s), %2 route shape(s)")
                  .arg(rootObject.value("polyVeg").toArray().size())
                  .arg(qMax(0, assetPicker->count() - 1)));
    else
        setStatus("CATALOG ERRORS: " + validation.errors.join(" "), true);
}

void PolyVegSchemaEditor::refreshSchemaList(int preferredIndex) {
    const QSignalBlocker blocker(schemaList);
    schemaList->clear();
    const QJsonArray recipes = rootObject.value("polyVeg").toArray();
    for(const QJsonValue &value : recipes) {
        const QJsonObject recipe = value.toObject();
        const QString name = recipe.value("name").toString().trimmed();
        schemaList->addItem(name.isEmpty() ? recipe.value("id").toString() : name);
    }
    if(recipes.isEmpty()) {
        currentSchemaIndex = -1;
        return;
    }
    currentSchemaIndex = qBound(0,
        preferredIndex < 0 ? currentSchemaIndex : preferredIndex,
        recipes.size() - 1);
    if(currentSchemaIndex < 0)
        currentSchemaIndex = 0;
    schemaList->setCurrentIndex(currentSchemaIndex);
}

void PolyVegSchemaEditor::loadSchemaFields() {
    loadingFields = true;
    const QJsonObject recipe = currentRecipe();
    schemaName->setText(recipe.value("name").toString());
    schemaDescription->setPlainText(recipe.value("description").toString());
    const bool enabled = currentSchemaIndex >= 0;
    schemaName->setEnabled(enabled);
    schemaDescription->setEnabled(enabled);
    for(QDoubleSpinBox *field : findChildren<QDoubleSpinBox *>()) {
        const QString section = field->property("schemaSection").toString();
        const QString key = field->property("schemaKey").toString();
        if(!section.isEmpty() && !key.isEmpty()) {
            field->setValue(recipe.value(section).toObject().value(key).toDouble());
        }
        const QString rangeSection =
            field->property("schemaRangeSection").toString();
        const QString rangeKey = field->property("schemaRangeKey").toString();
        if(!rangeSection.isEmpty() && !rangeKey.isEmpty()) {
            const QJsonArray values =
                recipe.value(rangeSection).toObject().value(rangeKey).toArray();
            const int index = field->property("schemaRangeIndex").toInt();
            const QSignalBlocker blocker(field);
            field->setValue(index < values.size()
                ? values.at(index).toDouble() : 0.0);
        }
    }
    for(QCheckBox *field : findChildren<QCheckBox *>()) {
        const QString section = field->property("schemaSection").toString();
        const QString key = field->property("schemaKey").toString();
        if(section.isEmpty() || key.isEmpty())
            continue;
        const QSignalBlocker blocker(field);
        field->setChecked(
            recipe.value(section).toObject().value(key).toBool());
    }
    loadingFields = false;
}

void PolyVegSchemaEditor::refreshShapeList() {
    const QSignalBlocker blocker(assetPicker);
    assetPicker->clear();
    assetPicker->addItem(assetPickerPlaceholder());
    QDir shapes(shapesPath);
    shapes.setNameFilters(QStringList() << "*.s");
    shapes.setFilter(QDir::Files | QDir::Readable);
    shapes.setSorting(QDir::Name | QDir::IgnoreCase);
    assetPicker->addItems(shapes.entryList());
    assetPicker->setCurrentIndex(0);
    QCompleter *completer = assetPicker->completer();
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
}

void PolyVegSchemaEditor::refreshAssetGrid() {
    while(QLayoutItem *item = assetGrid->takeAt(0)) {
        if(item->widget() != nullptr)
            item->widget()->deleteLater();
        delete item;
    }
    assetCards.clear();
    const QJsonArray vegetation = currentRecipe().value("vegetation").toArray();
    for(int index = 0; index < vegetation.size(); ++index) {
        const QJsonObject entry = vegetation.at(index).toObject();
        QFrame *card = new QFrame(assetContainer);
        GuiFunct::styleEditorPanelCard(card);
        card->setMinimumWidth(uiSize(400));
        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(uiSize(7), uiSize(6), uiSize(7), uiSize(6));
        cardLayout->setSpacing(uiSize(6));

        QWidget *previewPanel = new QWidget(card);
        previewPanel->setFixedWidth(uiSize(166));
        QVBoxLayout *previewLayout = new QVBoxLayout(previewPanel);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(uiSize(4));
        QLabel *thumbnail = new QLabel(previewPanel);
        thumbnail->setFixedSize(uiSize(160), uiSize(160));
        thumbnail->setAlignment(Qt::AlignCenter);
        thumbnail->setStyleSheet(
            "QLabel { background: #080808; border: 1px solid #000000; }");
        const QString cachedThumbnail =
            thumbnailPath(entry.value("shape").toString());
        QPixmap pixmap(cachedThumbnail);
        if(pixmap.isNull()) {
            pixmap = QPixmap(uiSize(160), uiSize(160));
            pixmap.fill(QColor("#101010"));
            QPainter painter(&pixmap);
            painter.setPen(QColor("#777777"));
            painter.drawText(pixmap.rect().adjusted(uiSize(8), uiSize(8),
                                                    -uiSize(8), -uiSize(8)),
                             Qt::AlignCenter | Qt::TextWordWrap,
                             "NO THUMBNAIL\n" +
                             QFileInfo(entry.value("shape").toString())
                                 .completeBaseName());
        }
        thumbnail->setPixmap(pixmap.scaled(
            thumbnail->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        previewLayout->addWidget(thumbnail, 0, Qt::AlignHCenter);
        QLabel *shapeName = new QLabel(entry.value("shape").toString(), previewPanel);
        shapeName->setAlignment(Qt::AlignCenter);
        shapeName->setWordWrap(true);
        shapeName->setToolTip(entry.value("shape").toString());
        previewLayout->addWidget(shapeName);
        QPushButton *remove = actionButton("REMOVE ASSET", previewPanel);
        remove->setToolTip("Remove this asset from the selected schema.");
        previewLayout->addWidget(remove);
        previewLayout->addStretch(1);
        cardLayout->addWidget(previewPanel);

        QWidget *definitionPanel = new QWidget(card);
        QVBoxLayout *layout = new QVBoxLayout(definitionPanel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(uiSize(4));
        cardLayout->addWidget(definitionPanel, 1);

        QLineEdit *name = new QLineEdit(entry.value("name").toString(), card);
        layout->addWidget(name);

        QFormLayout *form = new QFormLayout;
        form->setContentsMargins(0, 0, 0, 0);
        form->setHorizontalSpacing(uiSize(5));
        form->setVerticalSpacing(uiSize(3));
        QDoubleSpinBox *weight = numberField(0.01, 1000000.0, 2, card);
        weight->setValue(entry.value("proportion").toDouble());
        QDoubleSpinBox *radius = numberField(0.01, 10000.0, 2, card);
        radius->setValue(entry.value("footprintRadiusMetres").toDouble());
        form->addRow("Weight:", weight);
        form->addRow("Radius:", radius);

        auto addRange = [card, form](const QString &label, const QJsonArray &values,
                                     double minimum, double maximum, int decimals) {
            QWidget *row = new QWidget(card);
            QHBoxLayout *range = new QHBoxLayout(row);
            range->setContentsMargins(0, 0, 0, 0);
            range->setSpacing(uiSize(3));
            QDoubleSpinBox *low = numberField(minimum, maximum, decimals, row);
            QDoubleSpinBox *high = numberField(minimum, maximum, decimals, row);
            low->setValue(values.size() > 0 ? values.at(0).toDouble() : minimum);
            high->setValue(values.size() > 1 ? values.at(1).toDouble() : maximum);
            range->addWidget(low);
            range->addWidget(high);
            form->addRow(label, row);
            return QPair<QDoubleSpinBox *, QDoubleSpinBox *>(low, high);
        };
        const auto yaw = addRange("Yaw:", entry.value("yawDegrees").toArray(),
                                  -360.0, 360.0, 1);
        const auto scale = addRange("Scale:", entry.value("uniformScale").toArray(),
                                    0.01, 100.0, 2);

        QCheckBox *useDepth = new QCheckBox("Plant depth", card);
        QDoubleSpinBox *depth = numberField(0.0, 1000.0, 2, card);
        useDepth->setChecked(entry.contains("plantingDepthMetres"));
        depth->setEnabled(useDepth->isChecked());
        depth->setValue(entry.value("plantingDepthMetres").toDouble());
        QWidget *depthRow = new QWidget(card);
        QHBoxLayout *depthLayout = new QHBoxLayout(depthRow);
        depthLayout->setContentsMargins(0, 0, 0, 0);
        depthLayout->addWidget(useDepth);
        depthLayout->addWidget(depth);
        form->addRow("", depthRow);

        GuiFunct::alignEditorForm(form, 52);
        layout->addLayout(form);

        connect(name, &QLineEdit::editingFinished, this, [this, index, name]() {
            updateVegetation(index, [name](QJsonObject &object) {
                object["name"] = name->text().trimmed();
            });
        });
        connect(weight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, index](double value) {
            updateVegetation(index, [value](QJsonObject &object) {
                object["proportion"] = value;
            });
        });
        connect(radius, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, index](double value) {
            updateVegetation(index, [value](QJsonObject &object) {
                object["footprintRadiusMetres"] = value;
            });
        });
        auto updateRange = [this, index](const QString &key,
                                         QDoubleSpinBox *low,
                                         QDoubleSpinBox *high) {
            updateVegetation(index, [key, low, high](QJsonObject &object) {
                object[key] = numberRange(low->value(), high->value());
            });
        };
        connect(yaw.first, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [updateRange, yaw](double) { updateRange("yawDegrees", yaw.first, yaw.second); });
        connect(yaw.second, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [updateRange, yaw](double) { updateRange("yawDegrees", yaw.first, yaw.second); });
        connect(scale.first, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [updateRange, scale](double) { updateRange("uniformScale", scale.first, scale.second); });
        connect(scale.second, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [updateRange, scale](double) { updateRange("uniformScale", scale.first, scale.second); });
        connect(useDepth, &QCheckBox::toggled, this, [this, index, depth](bool enabled) {
            depth->setEnabled(enabled);
            updateVegetation(index, [enabled, depth](QJsonObject &object) {
                if(enabled) object["plantingDepthMetres"] = depth->value();
                else object.remove("plantingDepthMetres");
            });
        });
        connect(depth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, index, useDepth](double value) {
            if(useDepth->isChecked())
                updateVegetation(index, [value](QJsonObject &object) {
                    object["plantingDepthMetres"] = value;
                });
        });
        connect(remove, &QPushButton::clicked, this, [this, index]() {
            if(!GuiFunct::confirmDestructiveAction(
                    this, "REMOVE ASSET",
                    "Remove this vegetation asset from the selected schema?"))
                return;
            QJsonObject recipe = currentRecipe();
            QJsonArray vegetation = recipe.value("vegetation").toArray();
            vegetation.removeAt(index);
            recipe["vegetation"] = vegetation;
            setCurrentRecipe(recipe);
            refreshAssetGrid();
        });
        for(QLineEdit *field : card->findChildren<QLineEdit *>())
            quietEditorField(field);
        for(QComboBox *field : card->findChildren<QComboBox *>())
            quietEditorField(field);
        for(QDoubleSpinBox *field : card->findChildren<QDoubleSpinBox *>())
            quietEditorField(field);
        assetCards.append(card);
    }
    relayoutAssetCards();
}

void PolyVegSchemaEditor::relayoutAssetCards() {
    if(assetGrid == nullptr || assetScroll == nullptr)
        return;
    const int available = qMax(uiSize(400), assetScroll->viewport()->width());
    const int columns = qBound(1, available / uiSize(430), 4);
    for(QWidget *card : assetCards)
        assetGrid->removeWidget(card);
    for(int index = 0; index < assetCards.size(); ++index)
        assetGrid->addWidget(assetCards.at(index), index / columns, index % columns,
                             Qt::AlignTop);
    for(int column = 0; column < columns; ++column)
        assetGrid->setColumnStretch(column, 1);
    assetGrid->setRowStretch((assetCards.size() + columns - 1) / columns, 1);
}

QString PolyVegSchemaEditor::thumbnailPath(const QString &shapeName) const {
    const ThumbnailSource source = thumbnailSource(routePath(), shapeName);
    return QFileInfo::exists(source.cachePath) ? source.cachePath : QString();
}

bool PolyVegSchemaEditor::buildThumbnail(const QString &shapeName,
                                         QString *error) {
    const ThumbnailSource source = thumbnailSource(routePath(), shapeName);
    if(!QFileInfo::exists(source.texturePath)) {
        if(error != nullptr)
            *error = "Texture not found for " + shapeName;
        return false;
    }
    if(!QDir().mkpath(QFileInfo(source.cachePath).absolutePath())) {
        if(error != nullptr)
            *error = "Unable to create the thumbnail cache.";
        return false;
    }

    Texture texture(source.texturePath);
    const QString suffix = QFileInfo(source.texturePath).suffix().toLower();
    if(suffix == "ace") {
        AceLib decoder;
        decoder.texture = &texture;
        decoder.run();
    } else if(suffix == "dds") {
        DdsLib decoder;
        decoder.texture = &texture;
        decoder.run();
    } else {
        ImageLib decoder;
        decoder.texture = &texture;
        decoder.run();
    }
    std::unique_ptr<unsigned char[]> decodedStorage(texture.imageData);
    texture.imageData = nullptr;
    if(!texture.loaded || decodedStorage == nullptr
            || texture.width <= 0 || texture.height <= 0
            || (texture.bytesPerPixel != 3 && texture.bytesPerPixel != 4)) {
        if(error != nullptr)
            *error = "Unsupported texture for " + shapeName;
        return false;
    }

    const QImage::Format format = texture.bytesPerPixel == 4
        ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
    QImage decoded(decodedStorage.get(), texture.width, texture.height,
                   texture.width * texture.bytesPerPixel, format);
    decoded = decoded.copy();
    if(decoded.isNull()) {
        if(error != nullptr)
            *error = "Unable to decode texture for " + shapeName;
        return false;
    }

    const int size = 192;
    QImage square(size, size, QImage::Format_ARGB32);
    square.fill(QColor("#080808"));
    QPainter painter(&square);
    const QImage scaled = decoded.scaled(
        size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawImage((size - scaled.width()) / 2,
                      (size - scaled.height()) / 2, scaled);
    painter.end();
    if(!square.save(source.cachePath, "PNG")) {
        if(error != nullptr)
            *error = "Unable to save thumbnail for " + shapeName;
        return false;
    }
    return true;
}

void PolyVegSchemaEditor::updateVegetation(
    int index, const std::function<void(QJsonObject &)> &change) {
    QJsonObject recipe = currentRecipe();
    QJsonArray vegetation = recipe.value("vegetation").toArray();
    if(index < 0 || index >= vegetation.size())
        return;
    QJsonObject entry = vegetation.at(index).toObject();
    change(entry);
    vegetation[index] = entry;
    recipe["vegetation"] = vegetation;
    setCurrentRecipe(recipe);
}

QString PolyVegSchemaEditor::uniqueSchemaId(const QString &base) const {
    QString clean = base.toLower();
    clean.replace(QRegularExpression("[^a-z0-9._-]+"), "-");
    clean.remove(QRegularExpression("^-+|-+$"));
    if(clean.isEmpty()) clean = "schema";
    QSet<QString> existing;
    for(const QJsonValue &value : rootObject.value("polyVeg").toArray())
        existing.insert(value.toObject().value("id").toString().toCaseFolded());
    QString candidate = clean;
    int suffix = 2;
    while(existing.contains(candidate.toCaseFolded()))
        candidate = clean + "-" + QString::number(suffix++);
    return candidate;
}

QString PolyVegSchemaEditor::uniqueVegetationId(
    const QString &base, const QJsonArray &vegetation) const {
    QSet<QString> existing;
    for(const QJsonValue &value : vegetation)
        existing.insert(value.toObject().value("id").toString().toCaseFolded());
    QString candidate = base;
    int suffix = 2;
    while(existing.contains(candidate.toCaseFolded()))
        candidate = base + "-" + QString::number(suffix++);
    return candidate;
}

QJsonObject PolyVegSchemaEditor::defaultVegetation(const QString &shapeName) const {
    QJsonObject entry;
    const QString id = idFromShape(shapeName);
    QString name = QFileInfo(shapeName).completeBaseName();
    name.replace('_', ' ');
    entry["id"] = id;
    entry["name"] = name;
    entry["shape"] = shapeName;
    entry["proportion"] = 1.0;
    entry["yawDegrees"] = numberRange(0.0, 360.0);
    entry["uniformScale"] = numberRange(0.8, 1.2);
    entry["footprintRadiusMetres"] = 0.75;
    return entry;
}

void PolyVegSchemaEditor::setStatus(const QString &text, bool error) {
    statusLabel->setText(text);
    statusLabel->setStyleSheet(QString("color: %1; font-weight: %2;")
        .arg(error ? Game::StyleRedButton : Game::StyleMainLabel)
        .arg(error ? "bold" : "normal"));
}

void PolyVegSchemaEditor::closeEvent(QCloseEvent *event) {
    hide();
    emit visibilityChanged(false);
    event->ignore();
}

void PolyVegSchemaEditor::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    QTimer::singleShot(0, this, [this]() { relayoutAssetCards(); });
}

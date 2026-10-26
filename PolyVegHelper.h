// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef POLYVEGHELPER_H
#define POLYVEGHELPER_H

#include <QWidget>

class QComboBox;
class QButtonGroup;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QPushButton;
class QRadioButton;
class QFrame;
class QVBoxLayout;
class QStackedWidget;
class EditorCollapsibleSection;

class PolyVegHelper : public QWidget
{
    Q_OBJECT

public:
    explicit PolyVegHelper(QWidget *parent = nullptr);
    void activateRuler();
    void setRecipeHost(QWidget *host = nullptr);

public slots:
    void openForCurrentRoute();
    void clearPlacementTools();
    void clearRulerTools();
    void setFullTilePlantingActive(bool active);
    void setClearBakeActive(bool active);
    void setBakeTileActive(bool active);
    void setStatus(const QString &text);
    void setTileCounts(int rawCount, int bakedCount, int bakedTileCount);

signals:
    void settingsChanged(QString recipeId, double density, int maximumTrees,
                         quint64 seed, bool useOsm, bool floodFill,
                         bool disablePlantReport, bool rowsEnabled,
                         double rowWidthMetres, double rowSpacingMetres,
                         double rowDirectionDegrees);
    void bakeRequested(bool active);
    void bakeAllRequested();
    void countsRequested();
    void placeRulerRequested(double widthMetres, bool closedShape);
    void removeRulerRequested();
    void addRulerPointsRequested();
    void editRulerPointsRequested();
    void rulerAreaChanged(bool closedShape);
    void rulerWidthChanged(double widthMetres);
    void plantRulerRequested(bool overrideForestCoverage);
    void fullTilePlantingRequested(bool active);
    void clearBakeRequested(bool active);
    void schemaEditorRequested();

private slots:
    void loadDefinitions();
    void publishSettings();

private:
    void resizeNumericFields();
    void updateMethodChoices();
    void selectMethodPage();
    int recipeHostWidth = 0;
    QString rowsRouteKey;
    QWidget *recipeHome = nullptr;
    QVBoxLayout *recipeHomeLayout = nullptr;
    QFrame *recipePanel = nullptr;
    QButtonGroup *methodButtons = nullptr;
    QCheckBox *methodChoices[3] = {};
    QLabel *methodLabels[3] = {};
    QStackedWidget *methodPages = nullptr;

    void applyDefinitionSettings(bool restoreSavedValues);
    void selectRulerMode(QPushButton *button);

    QComboBox *definition = nullptr;
    QLabel *densityLabel = nullptr;
    QLabel *capLabel = nullptr;
    QLabel *seedLabel = nullptr;
    QDoubleSpinBox *density = nullptr;
    QSpinBox *maximumTrees = nullptr;
    QComboBox *seed = nullptr;
    QCheckBox *useOsm = nullptr;
    QCheckBox *floodFill = nullptr;
    QCheckBox *disablePlantReport = nullptr;
    QPushButton *rows = nullptr;
    EditorCollapsibleSection *rowsSection = nullptr;
    QWidget *rowControls = nullptr;
    QSpinBox *rowWidthValue = nullptr;
    QSpinBox *rowSpacingValue = nullptr;
    QSpinBox *rowDirectionValue = nullptr;
    QSpinBox *rulerWidthValue = nullptr;
    QLabel *statusRawValue = nullptr;
    QLabel *statusBakedValue = nullptr;
    QLabel *statusBakedTilesValue = nullptr;
    QPlainTextEdit *statusMessage = nullptr;
    QRadioButton *rulerOverride = nullptr;
    QPushButton *placeRuler = nullptr;
    QPushButton *plantFullTile = nullptr;
    QPushButton *clearBake = nullptr;
    QPushButton *bakeTile = nullptr;
    QPushButton *addRulerPoints = nullptr;
    QPushButton *editRulerPoints = nullptr;
    QButtonGroup *rulerModeButtons = nullptr;
    QCheckBox *rulerArea = nullptr;
};

#endif

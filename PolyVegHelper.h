#ifndef POLYVEGHELPER_H
#define POLYVEGHELPER_H

#include <QWidget>

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QRadioButton;
class QSlider;

class PolyVegHelper : public QWidget
{
    Q_OBJECT

public:
    explicit PolyVegHelper(QWidget *parent = nullptr);

public slots:
    void openForCurrentRoute();
    void clearPlacementTools();
    void setTileCounts(int rawCount, int bakedCount, int bakedTileCount);

signals:
    void settingsChanged(QString recipeId, double density, int maximumTrees,
                         quint64 seed, bool floodFill,
                         bool disablePlantReport, bool rowsEnabled,
                         double rowWidthMetres, double rowSpacingMetres,
                         double rowDirectionDegrees);
    void bakeRequested();
    void bakeAllRequested();
    void countsRequested();
    void placeRulerRequested(double widthMetres, bool closedShape);
    void removeRulerRequested();
    void rulerAreaChanged(bool closedShape);
    void rulerWidthChanged(double widthMetres);
    void plantRulerRequested(bool overrideForestCoverage);
    void jumpRawRequested();
    void resetRawJumpRequested();
    void jumpBakeRequested();
    void resetBakeJumpRequested();
    void toggleMapTilesRequested();

private slots:
    void loadDefinitions();
    void publishSettings();

private:
    void applyDefinitionSettings(bool restoreSavedValues);

    QComboBox *definition = nullptr;
    QLabel *densityLabel = nullptr;
    QDoubleSpinBox *density = nullptr;
    QSlider *densitySlider = nullptr;
    QSpinBox *maximumTrees = nullptr;
    QSlider *maximumTreesSlider = nullptr;
    QComboBox *seed = nullptr;
    QCheckBox *floodFill = nullptr;
    QCheckBox *disablePlantReport = nullptr;
    QCheckBox *rows = nullptr;
    QWidget *rowControls = nullptr;
    QSlider *rowWidth = nullptr;
    QSpinBox *rowWidthValue = nullptr;
    QSlider *rowSpacing = nullptr;
    QSpinBox *rowSpacingValue = nullptr;
    QSlider *rowDirection = nullptr;
    QSpinBox *rowDirectionValue = nullptr;
    QSlider *rulerWidth = nullptr;
    QSpinBox *rulerWidthValue = nullptr;
    QLabel *statusRawValue = nullptr;
    QLabel *statusBakedValue = nullptr;
    QLabel *statusBakedTilesValue = nullptr;
    QRadioButton *rulerOverride = nullptr;
    QPushButton *placeRuler = nullptr;
    QCheckBox *rulerArea = nullptr;
};

#endif

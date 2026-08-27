/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef TERRAINWATERWINDOW2_H
#define TERRAINWATERWINDOW2_H

#include <QString>
#include <QWidget>

class QButtonGroup;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;

class TerrainWaterWindow2 : public QWidget {
    Q_OBJECT
public:
    explicit TerrainWaterWindow2(QWidget *parent = nullptr);
    ~TerrainWaterWindow2() override;
    void activateRuler();
    void resetSession();

signals:
    void userButtonPressed();
    void placeRulerRequested();
    void removeRulerRequested();
    void addPointsRequested();
    void editPointsRequested();
    void scanRequested(float heightAboveBed, int tileRadius);
    void adjustTerrainRequested(float clearance, int tileRadius);
    void undoScanRequested();

public slots:
    void setStatus(const QString &text);
    void setProgress(int value, int maximum, const QString &text);

private:
    void selectMode(QPushButton *button);
    QDoubleSpinBox *waterHeight = nullptr;
    QSpinBox *searchDistance = nullptr;
    QPlainTextEdit *messageCell = nullptr;
    QProgressBar *progress = nullptr;
    QPushButton *newRulerButton = nullptr;
    QPushButton *addPointsButton = nullptr;
    QPushButton *editPointsButton = nullptr;
    QButtonGroup *modeButtons = nullptr;
};

#endif /* TERRAINWATERWINDOW2_H */

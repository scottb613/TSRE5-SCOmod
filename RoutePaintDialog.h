// TSRE GenX route paint editor. GNU GPL v3 or later.
#pragma once
#include <QDialog>
#include <QHash>
#include "RoutePaintPlan.h"
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class RoutePaintDialog : public QDialog {
public:
    RoutePaintDialog(const QJsonArray &presets, QWidget *parent);
    RoutePaintPlan::Plan plan() const;
private:
    QComboBox *presets[10], *targets[10];
    QCheckBox *mirrors[10];
    QLabel *pairStatus[10];
    QPushButton *commit;
    QJsonArray availablePresets;
    QHash<QString,bool> mirrorValid;
    QHash<QString,QString> mirrorErrors;
    void refreshRow(int row);
    void refreshCommit();
};

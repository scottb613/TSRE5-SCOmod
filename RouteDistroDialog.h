// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include "RouteDistro.h"
#include <QDialog>
#include <functional>
class QLabel;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QPlainTextEdit;
class QCloseEvent;
class QComboBox;
class QTabWidget;

class RouteDistroDialog final : public QDialog {
public:
    RouteDistroDialog(const QString &root, const QString &installationRoot,
                     QWidget *parent, std::function<bool()> canClean,
                     std::function<void()> refreshTerrain = {});
    void reject() override;
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    QString root, installationRoot;
    RouteDistro::Report report;
    std::function<bool()> canClean;
    std::function<void()> refreshTerrain;
    QLabel *status=nullptr, *selection=nullptr;
    QPushButton *scanButton=nullptr, *cleanButton=nullptr, *closeButton=nullptr;
    QPushButton *allButton=nullptr, *noneButton=nullptr;
    QPushButton *selectCategoryButton=nullptr, *clearCategoryButton=nullptr, *exportButton=nullptr;
    QCheckBox *keepEditor=nullptr;
    QCheckBox *consolidate=nullptr, *editorCaches=nullptr;
    QTreeWidget *items=nullptr;
    QTreeWidget *warnings=nullptr;
    QComboBox *category=nullptr;
    QTabWidget *tabs=nullptr;
    QPlainTextEdit *details=nullptr;
    bool busy=false, reviewValid=false;
    bool hasScan=false;
    QStringList operationReports;
    void scan();
    void clean();
    void updateSelection();
    void selectCategory(Qt::CheckState state);
    QStringList checkedPaths() const;
    void exportReport();
};

// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef POLYVEGSCHEMAEDITOR_H
#define POLYVEGSCHEMAEDITOR_H

#include <QJsonObject>
#include <QMainWindow>
#include <QPixmap>
#include <QVector>

#include <functional>

class QCloseEvent;
class QComboBox;
class QGridLayout;
class QJsonArray;
class QLabel;
class QLineEdit;
class QResizeEvent;
class QScrollArea;
class QTextEdit;
class QWidget;

class PolyVegSchemaEditor : public QMainWindow {
    Q_OBJECT
public:
    explicit PolyVegSchemaEditor(QWidget *parent = nullptr);
    ~PolyVegSchemaEditor() override;

public slots:
    void showForCurrentRoute();

signals:
    void visibilityChanged(bool visible);
    void schemaSaved();
    void exitToPlanterRequested();
    void userToggleSoundRequested();
    void userButtonSoundRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString routePath() const;
    QJsonObject currentRecipe() const;
    void setCurrentRecipe(const QJsonObject &recipe);
    void loadCatalog();
    void loadSchemaFields();
    void refreshSchemaList(int preferredIndex = -1);
    void refreshShapeList();
    void refreshAssetGrid();
    void relayoutAssetCards();
    QString thumbnailPath(const QString &shapeName) const;
    bool buildThumbnail(const QString &shapeName, QString *error);
    void updateVegetation(int index,
                          const std::function<void(QJsonObject &)> &change);
    QString uniqueSchemaId(const QString &base) const;
    QString uniqueVegetationId(const QString &base,
                               const QJsonArray &vegetation) const;
    QJsonObject defaultVegetation(const QString &shapeName) const;
    void setStatus(const QString &text, bool error = false);

    QJsonObject rootObject;
    QString loadedRoutePath;
    QString catalogPath;
    QString shapesPath;
    int currentSchemaIndex = -1;
    bool loadingFields = false;

    QComboBox *schemaList = nullptr;
    QLineEdit *schemaName = nullptr;
    QTextEdit *schemaDescription = nullptr;
    QComboBox *assetPicker = nullptr;
    QLabel *statusLabel = nullptr;
    QScrollArea *assetScroll = nullptr;
    QWidget *assetContainer = nullptr;
    QGridLayout *assetGrid = nullptr;
    QVector<QWidget *> assetCards;
};

#endif

/*
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TerrainTools.h"
#include "TexLib.h"
#include "Brush.h"
#include "Texture.h"
#include "GuiFunct.h"
#include "TransferObj.h"
#include "ClickableLabel.h"
#include "Game.h"
#include "ShapeLib.h"
#include "Terrain.h"
#include "ForestObj.h"
#include "PolyForestObj.h"
#include "TFile.h"
#include "TerrainLib.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QFileSystemModel>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>

static int scaledUiSize(int base){
    return qRound(base * qMax(1.0f, Game::uiScale));
}

static bool textureReadyForPreview(Texture *texture){
    return texture != NULL
        && texture->loaded
        && !texture->missing
        && !texture->error
        && texture->width > 0
        && texture->height > 0
        && (texture->bytesPerPixel == 3 || texture->bytesPerPixel == 4)
        && (texture->imageData != NULL
            || (texture->glLoaded && texture->tex != NULL));
}

static Texture* loadTextureForPreview(const QString &path){
    if(!QFile::exists(path))
        return NULL;

    int textureId = TexLib::getTex(path);
    if(textureId < 0)
        textureId = TexLib::addTex(path, true);
    if(textureId < 0)
        return NULL;

    const auto textureIt = TexLib::mtex.find(textureId);
    if(textureIt == TexLib::mtex.end() || !textureReadyForPreview(textureIt->second))
        return NULL;
    return textureIt->second;
}

static void setTexturePreview(QLabel *label, Texture *texture, int resolution){
    if(label == NULL || !textureReadyForPreview(texture))
        return;

    unsigned char *imageData = texture->getImageData(resolution, resolution);
    if(texture->bytesPerPixel == 3)
        label->setPixmap(QPixmap::fromImage(
            QImage(imageData, resolution, resolution,
                   resolution * texture->bytesPerPixel,
                   QImage::Format_RGB888)));
    else if(texture->bytesPerPixel == 4)
        label->setPixmap(QPixmap::fromImage(
            QImage(imageData, resolution, resolution,
                   resolution * texture->bytesPerPixel,
                   QImage::Format_RGBA8888)));
}

class TerrtexFileFilterModel : public QSortFilterProxyModel {
public:
    explicit TerrtexFileFilterModel(QObject *parent = NULL)
        : QSortFilterProxyModel(parent){
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        QFileSystemModel *fileModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if(fileModel == NULL)
            return true;

        const QModelIndex sourceIndex = fileModel->index(sourceRow, 0, sourceParent);
        const QFileInfo fileInfo = fileModel->fileInfo(sourceIndex);
        if(fileInfo.isDir())
            return true;

        const QString fileName = fileInfo.fileName().toLower();
        return !fileName.startsWith("mosaic") && !fileName.startsWith("-");
    }
};

static QPoint terrainUtilitiesSnapPosition(QWidget *window){
    const int snapDistance = 10;
    QRect moving = window->frameGeometry();
    QPoint snappedFramePos = moving.topLeft();
    int bestX = snapDistance + 1;
    int bestY = snapDistance + 1;

    QScreen *screen = QGuiApplication::screenAt(moving.center());
    if(screen != NULL){
        const QRect available = screen->availableGeometry();
        const int xCandidates[3] = {
            available.left() - moving.left(),
            available.right() - moving.right(),
            available.center().x() - moving.center().x()
        };
        const int yCandidates[3] = {
            available.top() - moving.top(),
            available.bottom() - moving.bottom(),
            available.center().y() - moving.center().y()
        };
        for(int i = 0; i < 3; ++i){
            if(qAbs(xCandidates[i]) <= snapDistance && qAbs(xCandidates[i]) < bestX){
                bestX = qAbs(xCandidates[i]);
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            if(qAbs(yCandidates[i]) <= snapDistance && qAbs(yCandidates[i]) < bestY){
                bestY = qAbs(yCandidates[i]);
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }

    for(QWidget *targetWidget : QApplication::topLevelWidgets()){
        if(targetWidget == window || !targetWidget->isVisible())
            continue;
        const QRect target = targetWidget->frameGeometry();
        const bool verticalNear = moving.bottom() >= target.top() - snapDistance
                               && moving.top() <= target.bottom() + snapDistance;
        const bool horizontalNear = moving.right() >= target.left() - snapDistance
                                 && moving.left() <= target.right() + snapDistance;
        const int xCandidates[5] = {
            target.left() - moving.left(), target.right() - moving.right(),
            target.right() + 1 - moving.left(), target.left() - 1 - moving.right(),
            target.center().x() - moving.center().x()
        };
        const int yCandidates[5] = {
            target.top() - moving.top(), target.bottom() - moving.bottom(),
            target.bottom() + 1 - moving.top(), target.top() - 1 - moving.bottom(),
            target.center().y() - moving.center().y()
        };
        for(int i = 0; i < 5; ++i){
            int distance = qAbs(xCandidates[i]);
            if(verticalNear && distance <= snapDistance && distance < bestX){
                bestX = distance;
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            distance = qAbs(yCandidates[i]);
            if(horizontalNear && distance <= snapDistance && distance < bestY){
                bestY = distance;
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }
    return window->pos() + (snappedFramePos - moving.topLeft());
}

class TerrainUtilitiesWindow : public QWidget {
public:
    TerrainUtilitiesWindow(TerrainTools *owner, QPushButton *toggleButton,
                           QPushButton *resetTerrtexButton)
        : QWidget(owner, Qt::Tool), owner(owner), toggleButton(toggleButton) {
        setWindowTitle(QString());
        setFixedWidth(scaledUiSize(310));
        setStyleSheet(GuiFunct::scoEditorPanelStyle());

        QVBoxLayout *rootLayout = new QVBoxLayout(this);
        rootLayout->setSpacing(4);
        rootLayout->setContentsMargins(4,4,4,4);

        QLabel *heading = new QLabel("TERRAIN UTILITIES");
        GuiFunct::styleEditorTitle(heading);
        QHBoxLayout *headingRow = new QHBoxLayout;
        headingRow->setContentsMargins(0,0,0,0);
        headingRow->addWidget(heading);
        headingRow->addStretch();

        pinButton.setText("Pin");
        pinButton.setCheckable(true);
        pinButton.setFocusPolicy(Qt::NoFocus);
        QFont pinFont = pinButton.font();
        pinFont.setBold(false);
        if(pinFont.pointSizeF() > 0)
            pinFont.setPointSizeF(qMax(7.0, pinFont.pointSizeF() * 0.85));
        pinButton.setFont(pinFont);
        pinButton.setFixedSize(scaledUiSize(30), scaledUiSize(17));
        positionPinned = Game::pinnedWindowPosition("terrainUtilities", NULL);
        pinButton.setChecked(positionPinned);
        updatePinAppearance();
        headingRow->addWidget(&pinButton);
        rootLayout->addLayout(headingRow);

        QLabel *conformHeading = new QLabel(QString(QChar(0x2022)) + " Terrain Conform");
        GuiFunct::styleEditorSubtitle(conformHeading);
        rootLayout->addWidget(conformHeading);

        QGridLayout *conformLayout = new QGridLayout;
        conformLayout->setSpacing(3);
        conformLayout->setContentsMargins(3,0,3,0);
        const QString biasToolTip =
            "Height added when terrain is conformed to this database. "
            "Positive raises the terrain; negative lowers it.";
        tdbHeightBias.setText(QString::number(Game::terrainConformTdbBias, 'f', 2));
        rdbHeightBias.setText(QString::number(Game::terrainConformRdbBias, 'f', 2));
        tdbHeightBias.setValidator(new QDoubleValidator(-5.0, 5.0, 2, &tdbHeightBias));
        rdbHeightBias.setValidator(new QDoubleValidator(-5.0, 5.0, 2, &rdbHeightBias));
        tdbHeightBias.setAlignment(Qt::AlignRight);
        rdbHeightBias.setAlignment(Qt::AlignRight);
        tdbHeightBias.setToolTip(biasToolTip);
        rdbHeightBias.setToolTip(biasToolTip);
        conformLayout->addWidget(new QLabel("TDB Height Bias:"), 0, 0);
        conformLayout->addWidget(&tdbHeightBias, 0, 1);
        conformLayout->addWidget(new QLabel("m"), 0, 2);
        conformLayout->addWidget(new QLabel("RDB Height Bias:"), 1, 0);
        conformLayout->addWidget(&rdbHeightBias, 1, 1);
        conformLayout->addWidget(new QLabel("m"), 1, 2);
        rootLayout->addLayout(conformLayout);

        rootLayout->addSpacing(scaledUiSize(5));
        QLabel *terrtexHeading = new QLabel(QString(QChar(0x2022)) + " TERRTEX Maintenance");
        GuiFunct::styleEditorSubtitle(terrtexHeading);
        rootLayout->addWidget(terrtexHeading);
        rootLayout->addWidget(resetTerrtexButton);

        QList<QWidget*> controls = findChildren<QWidget*>();
        for(QWidget *control : controls){
            if(qobject_cast<QPushButton*>(control) != NULL ||
               qobject_cast<QLineEdit*>(control) != NULL)
                control->setMinimumHeight(scaledUiSize(20));
        }

        QObject::connect(&tdbHeightBias, &QLineEdit::editingFinished, this, [this](){
            bool valid = false;
            float value = tdbHeightBias.text().toFloat(&valid);
            if(valid)
                Game::terrainConformTdbBias = qBound(-5.0f, value, 5.0f);
            tdbHeightBias.setText(QString::number(Game::terrainConformTdbBias, 'f', 2));
        });
        QObject::connect(&rdbHeightBias, &QLineEdit::editingFinished, this, [this](){
            bool valid = false;
            float value = rdbHeightBias.text().toFloat(&valid);
            if(valid)
                Game::terrainConformRdbBias = qBound(-5.0f, value, 5.0f);
            rdbHeightBias.setText(QString::number(Game::terrainConformRdbBias, 'f', 2));
        });

        snapTimer.setSingleShot(true);
        pinSaveTimer.setSingleShot(true);
        QObject::connect(&snapTimer, &QTimer::timeout, this, [this](){
            if(snapping)
                return;
            const QPoint snapped = terrainUtilitiesSnapPosition(this);
            if(snapped != pos()){
                snapping = true;
                move(snapped);
                snapping = false;
            }
        });
        QObject::connect(&pinSaveTimer, &QTimer::timeout, this, [this](){
            if(positionPinned)
                Game::savePinnedWindowPosition("terrainUtilities", pos());
        });
        QObject::connect(&pinButton, &QToolButton::toggled, this, [this](bool pinned){
            positionPinned = pinned;
            updatePinAppearance();
            if(pinned){
                Game::clearPinnedWindowPosition("terrainUtilitiesUseDefault");
                Game::savePinnedWindowPosition("terrainUtilities", pos());
            } else {
                pinSaveTimer.stop();
                Game::clearPinnedWindowPosition("terrainUtilities");
                Game::savePinnedWindowPosition("terrainUtilitiesUseDefault", QPoint(0, 0));
                moveToDefaultPosition();
            }
        });

        QPoint pinnedPosition;
        if(Game::pinnedWindowPosition("terrainUtilities", &pinnedPosition))
            move(Game::visibleWindowPosition(pinnedPosition, size()));

        rootLayout->activate();
        setFixedHeight(rootLayout->sizeHint().height());
    }

    void showForOwner(){
        tdbHeightBias.setText(QString::number(Game::terrainConformTdbBias, 'f', 2));
        rdbHeightBias.setText(QString::number(Game::terrainConformRdbBias, 'f', 2));
        if(!everShown && !positionPinned){
            moveToDefaultPosition();
            everShown = true;
        }
        show();
        raise();
        activateWindow();
    }

protected:
    void moveEvent(QMoveEvent *event) override {
        QWidget::moveEvent(event);
        if(!snapping){
            snapTimer.start(120);
            if(positionPinned)
                pinSaveTimer.start(240);
        }
    }

    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        if(toggleButton != NULL){
            toggleButton->blockSignals(true);
            toggleButton->setChecked(false);
            toggleButton->blockSignals(false);
        }
    }

private:
    void moveToDefaultPosition(){
        const QPoint ownerTopLeft = owner->mapToGlobal(QPoint(0, 0));
        const QPoint desired(ownerTopLeft.x() - width() - 1, ownerTopLeft.y());
        move(Game::visibleWindowPosition(desired, size()));
    }

    void updatePinAppearance(){
        pinButton.setToolTip(positionPinned
            ? "The Terrain Utilities position is saved between sessions."
            : "Save the current Terrain Utilities position between sessions.");
        if(positionPinned){
            pinButton.setStyleSheet(QString(
                "QToolButton { color: #232323; background-color: %1;"
                " border: 1px solid %1; padding: 0px 3px; font-weight: normal; }"
                "QToolButton:hover { border-color: #e4c5a3; }"
                "QToolButton:pressed { background-color: #a98a69; }").arg(Game::StyleMainLabel));
        } else {
            pinButton.setStyleSheet(
                "QToolButton { color: #e7eaec; background-color: #26292c;"
                " border: 1px solid #383d41; padding: 0px 3px; font-weight: normal; }"
                "QToolButton:hover { background-color: #303438; border-color: #f08200; }"
                "QToolButton:pressed { background-color: #191b1d; }");
        }
    }

    TerrainTools *owner;
    QPushButton *toggleButton;
    QLineEdit tdbHeightBias;
    QLineEdit rdbHeightBias;
    QToolButton pinButton;
    QTimer snapTimer;
    QTimer pinSaveTimer;
    bool snapping = false;
    bool positionPinned = false;
    bool everShown = false;
};

TerrainTools::TerrainTools(QString name)
    : QWidget(){
    setFixedWidth(scaledUiSize(250));
    GuiFunct::applyEditorPanelStyle(this);
    QFont panelFont = font();
    if(panelFont.pointSizeF() > 0)
        panelFont.setPointSizeF(panelFont.pointSizeF() * 1.12);
    setFont(panelFont);
    int row = 0;
    
    texPreview = new QPixmap(106,106);
    defaultTexPreview = new QPixmap(36,36);
    defaultTexPreview->fill(Qt::transparent);
    texPreview->fill(Qt::black);
    texPreviewLabel = new ClickableLabel("");
    texPreviewLabel->setContentsMargins(0,0,0,0);
    texPreviewLabel->setFixedSize(108,108);
    texPreviewLabel->setAlignment(Qt::AlignCenter);
    texPreviewLabel->setStyleSheet("background-color: #171717; border: 1px solid #555555;");
    texPreviewLabel->setPixmap(*texPreview);
    texPreviewLabel->setToolTip("#000000");
    mirrorTexPreviewLabel = new QLabel("");
    mirrorTexPreviewLabel->setFixedSize(108,108);
    mirrorTexPreviewLabel->setAlignment(Qt::AlignCenter);
    mirrorTexPreviewLabel->setStyleSheet(
        "QLabel { background-color: #171717; color: #9b9b9b; border: 1px solid #555555; }");
    mirrorTexPreviewLabel->setPixmap(*texPreview);
    mirrorPairStatus = new QLabel("");
    mirrorPairStatus->setMinimumHeight(scaledUiSize(18));
    mirrorPairStatus->setAlignment(Qt::AlignCenter);
    mirrorPairStatus->setStyleSheet("QLabel { color: #b0b0b0; padding: 1px; }");
    for(int i = 0; i < 7; i++){
        texPreviewLabels.push_back(new ClickableLabel(""));
        texPreviewLabels.back()->setContentsMargins(0,0,0,0);
        texPreviewLabels.back()->setFixedSize(38,38);
        texPreviewLabels.back()->setAlignment(Qt::AlignCenter);
        texPreviewLabels.back()->setStyleSheet("background-color: #171717; border: 1px solid #4d4d4d;");
        texPreviewLabels.back()->setPixmap(*defaultTexPreview);
        texPreviewSignals.setMapping(texPreviewLabels.back(), i);
        connect(texPreviewLabels.back(), SIGNAL(clicked()), &texPreviewSignals, SLOT(map()));
    }
    texPreviewSignals.setMapping(texPreviewLabel, 7);
    connect(texPreviewLabel, SIGNAL(clicked()), &texPreviewSignals, SLOT(map()));
    connect(&texPreviewSignals, SIGNAL(mapped(int)), this, SLOT(texPreviewEnabled(int)));

    paintBrush = new Brush();
    
    if(Game::terrBrushColor)        
    {
        paintBrush->color[0] = Game::terrBrushColor->red();
        paintBrush->color[1] = Game::terrBrushColor->green();
        paintBrush->color[2] = Game::terrBrushColor->blue();
    }
    else Game::terrBrushColor = new QColor("#000000");
        
              

    
    QDir dir(QString("tsre_appdata/")+Game::AppDataVersion+"/brush/");
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList()<<"*.png");
    foreach(QString bfile, dir.entryList())
        brushShapes.push_back(QImage(QString("tsre_appdata/")+Game::AppDataVersion+"/brush/"+bfile).convertToFormat(QImage::Format_Grayscale8));
    nextBrushShape();
    
    buttonTools["heightTool"] = new QPushButton("Height +", this);
    buttonTools["pickTerrainTexTool"] = new QPushButton("Pick", this);
    buttonTools["putTerrainTexTool"] = new QPushButton("Put", this);
    buttonTools["waterTerrTool"] = new QPushButton("Water +", this);
    //buttonTools["drawTerrTool"] = new QPushButton("Show/H Tile", this);
    buttonTools["gapsTerrainTool"] = new QPushButton("Gaps +", this);
    //buttonTools["waterHeightTileTool"] = new QPushButton("Water level", this);
    //buttonTools["fixedTileTool"] = new QPushButton("Fixed Height", this);
    //buttonTools["waTileTool"] = new QPushButton("Fixed Height", this);
    if(Game::serverClient == NULL){
        buttonTools["paintToolColor"] = new QPushButton("Color", this);
        buttonTools["paintToolTexture"] = new QPushButton("Texture", this);
        buttonTools["lockTexTool"] = new QPushButton("Lock", this);
    }
    QMapIterator<QString, QPushButton*> i(buttonTools);
    while (i.hasNext()) {
        i.next();
        i.value()->setCheckable(true);
    }
    
    QPushButton *loadTerrainTexTool = new QPushButton("Load...", this);
    loadTerrainTexTool->setCheckable(true);
    loadTerrainTexTool->setProperty("scoSoundOnPress", true);
    
    QGridLayout *vlist3 = new QGridLayout;
    vlist3->setSpacing(2);
    vlist3->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist3->addWidget(buttonTools["heightTool"],row,0);
    vlist3->addWidget(buttonTools["waterTerrTool"],row,1);
    vlist3->addWidget(buttonTools["gapsTerrainTool"],row++,2);
    //vlist3->addWidget(buttonTools["waterHeightTileTool"],row,2);
    //vlist3->addWidget(mapTileShowTool,row,0);
    //vlist3->addWidget(mapTileLoadTool,row,1);
    //vlist3->addWidget(heightTileLoadTool,row++,2);
    
    /*QGridLayout *vlist4 = new QGridLayout;
    vlist4->setSpacing(2);
    vlist4->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist4->addWidget(buttonTools["waterTerrTool"],row,0);
    vlist4->addWidget(buttonTools["drawTerrTool"],row,1);
    vlist4->addWidget(buttonTools["gapsTerrainTool"],row++,2);*/
    
    QGridLayout *vlist0 = new QGridLayout;
    vlist0->setSpacing(2);
    vlist0->setContentsMargins(3,0,1,0);    
    row = 0;
    mirrorSeason = new QPushButton("Mirror Season", this);
    mirrorSeason->setCheckable(true);
    mirrorSeason->setProperty("scoSuppressClickSound", true);
    mirrorSeason->setToolTip("Requires matching textures in the main TERRTEX and SNOW folders.");
    vlist0->addWidget(buttonTools["paintToolColor"],row,0);
    vlist0->addWidget(buttonTools["paintToolTexture"],row,1);
    vlist0->addWidget(buttonTools["lockTexTool"],row++,2);
    vlist0->addWidget(mirrorSeason,row++,0,1,3);
    
    QGridLayout *vlist1 = new QGridLayout;
    vlist1->setSpacing(2);
    vlist1->setContentsMargins(3,0,1,0);    
    row = 0;
    vlist1->addWidget(buttonTools["pickTerrainTexTool"],row,0);
    vlist1->addWidget(buttonTools["putTerrainTexTool"],row,1);
    vlist1->addWidget(loadTerrainTexTool,row++,2);
    hideGeneratedTerrtex = new QCheckBox("Hide generated tile textures", this);
    hideGeneratedTerrtex->setChecked(true);
    hideGeneratedTerrtex->setToolTip(
        "Hide TERRTEX files whose names begin with \"mosaic\" or \"-\" in the Load window.");
    vlist1->addWidget(hideGeneratedTerrtex,row,0,1,3);
    
    colorw = new QPushButton(Game::terrBrushColor->name(), this);
    colorw->setStyleSheet("background-color:" + Game::terrBrushColor->name() + ";");

    
    QLabel *label0;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(3);
    vbox->setContentsMargins(4,3,4,4);
    auto addRule = [vbox]() {
        vbox->addSpacing(scaledUiSize(5));
    };
    
    QLabel *panelTitle = new QLabel("TERRAIN EDITOR");
    GuiFunct::styleEditorTitle(panelTitle);
    vbox->addWidget(panelTitle);
    QLabel *textureSetLabel = new QLabel("• Texture Set");
    textureSetLabel->setContentsMargins(12,0,0,0);
    textureSetLabel->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    vbox->addWidget(textureSetLabel);

    seasonType = new QComboBox;
    seasonType->setStyleSheet("combobox-popup: 0;");
    seasonType->addItem("Summer");
    seasonType->addItem("Spring");
    seasonType->addItem("Autumn");
    seasonType->addItem("Winter");
    seasonType->addItem("Night");
    vbox->addWidget(seasonType);

    addRule();
    label0 = new QLabel("• Edit Terrain Layers");
    label0->setContentsMargins(12,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist3);
    /*label0 = new QLabel("Terrain Patch:");
    label0->setContentsMargins(3,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist4);*/
    if(Game::serverClient == NULL){
        label0 = new QLabel("• Paint Texture");
        label0->setContentsMargins(12,0,0,0);
        label0->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel + "; font-weight: bold; }");
        vbox->addWidget(label0);
        vbox->addItem(vlist0);
    }

    addRule();
    label0 = new QLabel("• Texture Preview");
    label0->setContentsMargins(12,0,0,0);
    label0->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    vbox->addWidget(label0);
    vbox->addItem(vlist1);

    QGridLayout *pairedPreview = new QGridLayout;
    pairedPreview->setHorizontalSpacing(6);
    pairedPreview->setVerticalSpacing(3);
    pairedPreview->setContentsMargins(5,1,5,1);
    QLabel *mainPreviewTitle = new QLabel("MAIN TERRTEX");
    QLabel *snowPreviewTitle = new QLabel("SNOW TERRTEX");
    mainPreviewTitle->setAlignment(Qt::AlignCenter);
    snowPreviewTitle->setAlignment(Qt::AlignCenter);
    mainPreviewTitle->setStyleSheet(QString(
        "QLabel { color: %1; font-weight: bold; padding: 1px; }").arg(Game::StyleMainLabel));
    snowPreviewTitle->setStyleSheet(mainPreviewTitle->styleSheet());
    pairedPreview->addWidget(mainPreviewTitle,0,0);
    pairedPreview->addWidget(snowPreviewTitle,0,1);
    pairedPreview->addWidget(texPreviewLabel,1,0,Qt::AlignHCenter);
    pairedPreview->addWidget(mirrorTexPreviewLabel,1,1,Qt::AlignHCenter);
    pairedPreview->addWidget(mirrorPairStatus,2,0,1,2);
    vbox->addItem(pairedPreview);

    QGridLayout *swatchGrid = new QGridLayout;
    swatchGrid->setHorizontalSpacing(3);
    swatchGrid->setVerticalSpacing(2);
    swatchGrid->setContentsMargins(10,1,10,3);
    QLabel *recentLabel = new QLabel("RECENT");
    QLabel *brushLabel = new QLabel("BRUSH");
    clearRecentTextures = new QPushButton("Clear Recent", this);
    clearRecentTextures->setFixedWidth(120);
    clearRecentTextures->setToolTip("Clear recent textures and reset the active texture swatches.");
    recentLabel->setAlignment(Qt::AlignCenter);
    brushLabel->setAlignment(Qt::AlignCenter);
    const QString swatchLabelStyle =
        "QLabel { color: #b0b0b0; font-size: 9px; font-weight: bold; padding: 0px 2px; }";
    recentLabel->setStyleSheet(swatchLabelStyle);
    brushLabel->setStyleSheet(swatchLabelStyle);
    swatchGrid->addWidget(recentLabel,0,0,1,3);
    swatchGrid->addWidget(brushLabel,0,4);
    for(int previewIndex = 0; previewIndex < 6; previewIndex++)
        swatchGrid->addWidget(texPreviewLabels[previewIndex],
                1 + previewIndex / 3, previewIndex % 3, Qt::AlignLeft);
    swatchGrid->setColumnMinimumWidth(3, scaledUiSize(18));
    swatchGrid->addWidget(texPreviewLabels[6],1,4,2,1,Qt::AlignTop | Qt::AlignHCenter);
    swatchGrid->addWidget(clearRecentTextures,3,0,1,3,Qt::AlignHCenter);
    swatchGrid->setColumnStretch(3,1);
    vbox->addItem(swatchGrid);

    QLabel *presetLabel = new QLabel("• Presets");
    presetLabel->setContentsMargins(12,0,0,0);
    presetLabel->setStyleSheet(QString("QLabel { color: ") + Game::StyleMainLabel + "; font-weight: bold; }");
    vbox->addWidget(presetLabel);

    presetCombo = new QComboBox;
    presetApply = new QPushButton("Apply", this);
    presetSave = new QPushButton("Save", this);
    presetRemove = new QPushButton("Remove", this);

    QGridLayout *vlistPreset = new QGridLayout;
    vlistPreset->setSpacing(2);
    vlistPreset->setContentsMargins(3,0,1,0);
    vlistPreset->addWidget(presetCombo,0,0,1,3);
    vlistPreset->addWidget(presetApply,1,0);
    vlistPreset->addWidget(presetSave,1,1);
    vlistPreset->addWidget(presetRemove,1,2);
    vbox->addItem(vlistPreset);

    addRule();
    QLabel *label2 = new QLabel("• Brush Settings");
    label2->setContentsMargins(12,0,0,0);
    label2->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    vbox->addWidget(label2);
    

    int labelWidth = 70;
    
    // brush
    sSize = new QSlider(Qt::Horizontal);
    sSize->setMinimum(1);
    sSize->setMaximum(100);
    sSize->setValue(paintBrush->size);
    sIntensity = new QSlider(Qt::Horizontal);
    sIntensity->setMinimum(1);
    sIntensity->setMaximum(100);
    sIntensity->setValue(paintBrush->alpha*100);
    sTextureRotation = new QSlider(Qt::Horizontal);
    sTextureRotation->setMinimum(0);
    sTextureRotation->setMaximum(360);
    sTextureRotation->setValue(paintBrush->texRotationDegrees);

    hType = new QComboBox;
    hType->setStyleSheet("combobox-popup: 0;");
    hType->addItem("Add - simple");
    hType->addItem("Add - if inside 'Size' radius");
    hType->addItem("Fixed Height");
    hType->addItem("Flatten");
    hType->addItem("Conform DB");
    hType->setCurrentIndex(paintBrush->hType);
    fheight = new QLineEdit();
    QDoubleValidator* doubleValidator = new QDoubleValidator(-5000, 5000, 2, this); 
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    fheight->setValidator(doubleValidator);
    
    QGridLayout *vlist = new QGridLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(3,0,1,0);    
    leSize = GuiFunct::newQLineEdit(25,3);
    leSize->setValidator(new QIntValidator(1, 100, this));
    leIntensity = GuiFunct::newQLineEdit(25,3);
    leIntensity->setValidator(new QIntValidator(1, 100, this));
    leTextureRotation = GuiFunct::newQLineEdit(25,3);
    leTextureRotation->setValidator(new QIntValidator(0, 360, this));
    leTextureRotation->setText(QString::number(paintBrush->texRotationDegrees, 10));
    row = 0;
    vlist->addWidget(GuiFunct::newQLabel("Color:", labelWidth),row,0);
    vlist->addWidget(colorw,row++,1,1,2);
    vlist->addWidget(GuiFunct::newQLabel("Size:", labelWidth),row,0);
    vlist->addWidget(leSize,row,1);
    vlist->addWidget(sSize,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Intensity:", labelWidth),row,0);
    vlist->addWidget(leIntensity,row,1);
    vlist->addWidget(sIntensity,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Rotation:", labelWidth),row,0);
    vlist->addWidget(leTextureRotation,row,1);
    vlist->addWidget(sTextureRotation,row++,2);
    vlist->addWidget(GuiFunct::newQLabel("Fixed Height:", labelWidth),row,0);
    vlist->addWidget(fheight,row++,1,1,2);
    vlist->addWidget(GuiFunct::newQLabel("Height type:", labelWidth),row,0);
    vlist->addWidget(hType,row++,1,1,2);
    

    vbox->addItem(vlist);
    
    // enbankment
    sEsize = new QSlider(Qt::Horizontal);
    sEsize->setMinimum(1);
    sEsize->setMaximum(3);
    sEsize->setValue(paintBrush->eSize);
    sEemb = new QSlider(Qt::Horizontal);
    sEemb->setMinimum(1);
    sEemb->setMaximum(10);
    sEemb->setValue(paintBrush->eEmb);
    sEcut = new QSlider(Qt::Horizontal);
    sEcut->setMinimum(1);
    sEcut->setMaximum(10);
    sEcut->setValue(paintBrush->eCut);
    sEradius = new QSlider(Qt::Horizontal);
    sEradius->setMinimum(1);
    sEradius->setMaximum(100);
    sEradius->setValue(paintBrush->eRadius);
    leEsize = GuiFunct::newQLineEdit(25,3);
    leEsize->setValidator(new QIntValidator(1, 3, this));
    leEemb = GuiFunct::newQLineEdit(25,3);
    leEemb->setValidator(new QIntValidator(1, 10, this));
    leEcut = GuiFunct::newQLineEdit(25,3);
    leEcut->setValidator(new QIntValidator(1, 10, this));
    leEradius = GuiFunct::newQLineEdit(25,3);
    leEradius->setValidator(new QIntValidator(1, 100, this));
    sun1 = GuiFunct::newQLineEdit(25,3);
    sun2 = GuiFunct::newQLineEdit(25,3);       
    sun3 = GuiFunct::newQLineEdit(25,3);    
    resetDefaults = new QPushButton("Reset Defaults", this);
    resetRouteTerrtex = new QPushButton("Reset Route Terrtex Paint", this);
    terrainUtilitiesButton = new QPushButton("Terrain Utilities...", this);
    terrainUtilitiesButton->setCheckable(true);
    terrainUtilitiesWindow = new TerrainUtilitiesWindow(
        this, terrainUtilitiesButton, resetRouteTerrtex);
    setPinPoint = new QPushButton("Set Pinpoint", this);    
    
    addRule();
    QLabel *label3 = new QLabel("• Embankment Settings");
    label3->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-weight: bold; }");
    label3->setContentsMargins(12,0,0,0);
    vbox->addWidget(label3);

    QGridLayout *vlist2 = new QGridLayout;
    vlist2->setSpacing(2);
    vlist2->setContentsMargins(3,0,1,0);
    row = 0;
    vlist2->addWidget(GuiFunct::newQLabel("Size:", labelWidth),row,0);
    vlist2->addWidget(leEsize,row,1);
    vlist2->addWidget(sEsize,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Embankment:", labelWidth),row,0);
    vlist2->addWidget(leEemb,row,1);
    vlist2->addWidget(sEemb,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Cutting:", labelWidth),row,0);
    vlist2->addWidget(leEcut,row,1);
    vlist2->addWidget(sEcut,row++,2);
    vlist2->addWidget(GuiFunct::newQLabel("Max Radius:", labelWidth),row,0);
    vlist2->addWidget(leEradius,row,1);
    vlist2->addWidget(sEradius,row++,2);
    
    vlist2->addWidget(setPinPoint,row,0);        
    vlist2->addWidget(resetDefaults,row++,1,1,2);
    
    /*
    
    vlist2->addWidget(GuiFunct::newQLabel("Sky 1:", labelWidth),row,0);
    vlist2->addWidget(sun1,row++,1);
    vlist2->addWidget(GuiFunct::newQLabel("Sky 2:", labelWidth),row,0);
    vlist2->addWidget(sun2,row++,1);
    vlist2->addWidget(GuiFunct::newQLabel("Sky 3:", labelWidth),row,0);
    vlist2->addWidget(sun3,row++,1);
    
     */
    
    
    vbox->addItem(vlist2);
    addRule();
    QLabel *advancedLabel = new QLabel(QString(QChar(0x2022)) + " Advanced");
    GuiFunct::styleEditorSubtitle(advancedLabel);
    vbox->addWidget(advancedLabel);
    vbox->addWidget(terrainUtilitiesButton);
    
    vbox->addStretch(1);
    this->setLayout(vbox);

    QList<QWidget*> controls = findChildren<QWidget*>();
    for(int c = 0; c < controls.size(); c++){
        controls[c]->setFont(panelFont);
        if(qobject_cast<QPushButton*>(controls[c]) != NULL ||
           qobject_cast<QLineEdit*>(controls[c]) != NULL ||
           qobject_cast<QComboBox*>(controls[c]) != NULL)
            controls[c]->setMinimumHeight(scaledUiSize(20));
    }
    
    
    // signals
    QObject::connect(buttonTools["heightTool"], SIGNAL(toggled(bool)),
                      this, SLOT(heightToolEnabled(bool)));
    if(Game::serverClient == NULL){
        QObject::connect(buttonTools["paintToolColor"], SIGNAL(toggled(bool)),
                          this, SLOT(paintColorToolEnabled(bool)));

        QObject::connect(buttonTools["paintToolTexture"], SIGNAL(toggled(bool)),
                          this, SLOT(paintTexToolEnabled(bool)));

        QObject::connect(buttonTools["lockTexTool"], SIGNAL(toggled(bool)),
                          this, SLOT(lockTexToolEnabled(bool)));
    }
    QObject::connect(buttonTools["pickTerrainTexTool"], SIGNAL(toggled(bool)),
                      this, SLOT(pickTexToolEnabled(bool)));
    
    QObject::connect(buttonTools["waterTerrTool"], SIGNAL(toggled(bool)),
                      this, SLOT(waterTerrToolEnabled(bool)));
    
    //QObject::connect(buttonTools["waterHeightTileTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(waterHeightTileToolEnabled(bool)));
    
    //QObject::connect(buttonTools["fixedTileTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(fixedTileToolEnabled(bool)));
    
    QObject::connect(buttonTools["gapsTerrainTool"], SIGNAL(toggled(bool)),
                      this, SLOT(gapsTerrToolEnabled(bool)));
    
    //QObject::connect(buttonTools["drawTerrTool"], SIGNAL(toggled(bool)),
    //                  this, SLOT(drawTerrToolEnabled(bool)));
    
    QObject::connect(buttonTools["putTerrainTexTool"], SIGNAL(toggled(bool)),
                      this, SLOT(putTexToolEnabled(bool)));
    
    QObject::connect(loadTerrainTexTool, SIGNAL(clicked()),
                      this, SLOT(setTexToolEnabled()));
    
    QObject::connect(colorw, SIGNAL(released()),
                      this, SLOT(chooseColorEnabled()));

    QObject::connect(mirrorSeason, SIGNAL(toggled(bool)),
                      this, SLOT(mirrorSeasonEnabled(bool)));
    
    QObject::connect(resetDefaults, SIGNAL(released()),
                      this, SLOT(resetDefaultValues()));

    QObject::connect(resetRouteTerrtex, SIGNAL(released()),
                      this, SLOT(resetRouteTerrtexPaint()));

    QObject::connect(terrainUtilitiesButton, &QPushButton::toggled,
                      this, [this](bool visible){
        if(terrainUtilitiesWindow == NULL)
            return;
        if(visible)
            terrainUtilitiesWindow->showForOwner();
        else
            terrainUtilitiesWindow->hide();
    });

    QObject::connect(setPinPoint, SIGNAL(released()),
                      this, SLOT(setPinPointBrush()));

    QObject::connect(presetApply, SIGNAL(released()),
                      this, SLOT(applyPaintPreset()));

    QObject::connect(presetSave, SIGNAL(released()),
                      this, SLOT(savePaintPreset()));

    QObject::connect(presetRemove, SIGNAL(released()),
                      this, SLOT(removePaintPreset()));
    QObject::connect(clearRecentTextures, SIGNAL(released()),
                      this, SLOT(clearRecentTextureHistory()));

    // brush
    QObject::connect(sSize, SIGNAL(valueChanged(int)),
                      this, SLOT(setBrushSize(int)));
    
    QObject::connect(sIntensity, SIGNAL(valueChanged(int)),
                      this, SLOT(setBrushAlpha(int)));

    QObject::connect(sTextureRotation, SIGNAL(valueChanged(int)),
                      this, SLOT(setTextureRotation(int)));

    QObject::connect(leSize, SIGNAL(textEdited(QString)),
                      this, SLOT(setBrushSize(QString)));
    
    QObject::connect(leIntensity, SIGNAL(textEdited(QString)),
                      this, SLOT(setBrushAlpha(QString)));

    QObject::connect(leTextureRotation, SIGNAL(textEdited(QString)),
                      this, SLOT(setTextureRotation(QString)));
    
    // embarkment
    QObject::connect(sEsize, SIGNAL(valueChanged(int)),
                      this, SLOT(setEsize(int)));
    
    QObject::connect(leEsize, SIGNAL(textEdited(QString)),
                      this, SLOT(setEsize(QString)));    
    
    QObject::connect(sEemb, SIGNAL(valueChanged(int)),
                      this, SLOT(setEemb(int)));

    QObject::connect(leEemb, SIGNAL(textEdited(QString)),
                      this, SLOT(setEemb(QString)));

    QObject::connect(sEcut, SIGNAL(valueChanged(int)),
                      this, SLOT(setEcut(int)));

    QObject::connect(leEcut, SIGNAL(textEdited(QString)),
                      this, SLOT(setEcut(QString)));
    
    QObject::connect(sEradius, SIGNAL(valueChanged(int)),
                      this, SLOT(setEradius(int)));

    QObject::connect(leEradius, SIGNAL(textEdited(QString)),
                      this, SLOT(setEradius(QString)));
    
    QObject::connect(sun1, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun1(QString)));    

    QObject::connect(sun2, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun2(QString)));    
    
    QObject::connect(sun3, SIGNAL(textEdited(QString)),
                      this, SLOT(setSun3(QString)));    

    
    QObject::connect(fheight, SIGNAL(textEdited(QString)),
                      this, SLOT(setFheight(QString)));
    
    QObject::connect(hType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(setHtype(int)));

    QObject::connect(seasonType, SIGNAL(currentIndexChanged(int)),
                      this, SLOT(setSeasonType(int)));
    
    this->setBrushSize(this->sSize->value());
    this->setBrushAlpha(this->sIntensity->value());
    this->setEsize(this->sEsize->value());
    this->setEemb(this->sEemb->value());
    this->setEcut(this->sEcut->value());
    this->setEradius(this->sEradius->value());
    this->fheight->setText("0");
    
    // EFO pre-populate terrainTools values from Settings

    if(Game::terrainTools != NULL)
    {
      this->setEsize(Game::terrainTools[0]);
      this->setEemb(Game::terrainTools[1]);
      this->setEcut(Game::terrainTools[2]);
      this->setEradius(Game::terrainTools[3]);
      this->setBrushSize(Game::terrainTools[4]);
      this->setBrushAlpha(Game::terrainTools[5]); 
      
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      sEsize->setValue(paintBrush->eSize);
      sEemb->setValue(paintBrush->eEmb);
      sEcut->setValue(paintBrush->eCut);
      sEradius->setValue(paintBrush->eRadius);                  
    }
    else
    {
      Game::terrainTools[0] = 1;
      Game::terrainTools[1] = 5;
      Game::terrainTools[2] = 5;
      Game::terrainTools[3] = 9;
      Game::terrainTools[4] = 1;
      Game::terrainTools[5] = 10; 
    }
    refreshPaintPresets();
    
}


TerrainTools::~TerrainTools() {
}

void TerrainTools::nextBrushShape(){
    currentBrushShape++;
    if(currentBrushShape > brushShapes.size()-1)
        currentBrushShape = 0;
    setBrushShapeIndex(currentBrushShape);
}

void TerrainTools::setBrushShapeIndex(int brushIndex){
    if(brushShapes.size() <= 0)
        return;
    if(brushIndex < 0 || brushIndex > brushShapes.size()-1)
        return;
    currentBrushShape = brushIndex;
    paintBrush->brushshape = &brushShapes[currentBrushShape];
    texPreviewLabels[6]->setPixmap(QPixmap::fromImage(*paintBrush->brushshape)
                                   .scaled(36, 36, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void TerrainTools::heightToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("heightTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::updateColorPreview(){
    if(paintBrush != NULL && paintBrush->tex != NULL)
        refreshMirrorTexturePreview(false);
}

void TerrainTools::paintColorToolEnabled(bool val){
     if(val){
        this->paintBrush->useTexture = false;
        updateColorPreview();
        emit setPaintBrush(this->paintBrush);
        emit enableTool("paintToolColor");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::gapsTerrToolEnabled(bool val){
    if(val){
        emit enableTool("gapsTerrainTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::paintTexToolEnabled(bool val){
    if(val){
        this->paintBrush->useTexture = true;
        updateTexPrev();
        emit setPaintBrush(this->paintBrush);
        emit enableTool("paintToolTexture");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::mirrorSeasonEnabled(bool val){
    if(val && !refreshMirrorTexturePreview(false)){
        QSignalBlocker blocker(mirrorSeason);
        mirrorSeason->setChecked(false);
        this->paintBrush->mirrorSeason = false;
        showMirrorStatusBriefly("No Mirror");
        emit mirrorSeasonError();
        emit setPaintBrush(this->paintBrush);
        return;
    }
    this->paintBrush->mirrorSeason = val;
    updateActiveTexturePreviewBorders();
    emit mirrorSeasonAccepted();
    emit setPaintBrush(this->paintBrush);
}

void TerrainTools::chooseColorEnabled(){
    QColor aColor(paintBrush->color[0], paintBrush->color[1], paintBrush->color[2]);
    QColor color = QColorDialog::getColor(aColor, this, "Text Color",  QColorDialog::DontUseNativeDialog);
    if(!color.isValid())
        return;
    paintBrush->color[0] = color.red();
    paintBrush->color[1] = color.green();
    paintBrush->color[2] = color.blue();
    colorw->setStyleSheet("background-color:"+color.name()+";");
    colorw->setText(color.name());
    if(!paintBrush->useTexture)
        updateColorPreview();
}

void TerrainTools::pickTexToolEnabled(bool val){
    if(val){
        emit enableTool("pickTerrainTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::lockTexToolEnabled(bool val){
    if(val){
        emit enableTool("lockTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::waterTerrToolEnabled(bool val){
    if(val){
        emit enableTool("waterTerrTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::drawTerrToolEnabled(bool val){
    if(val){
        emit enableTool("drawTerrTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::waterHeightTileToolEnabled(bool val){
    if(val){
        emit enableTool("waterHeightTileTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::putTexToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("putTerrainTexTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::fixedTileToolEnabled(bool val){
    if(val){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("fixedTileTool");
    } else {
        emit enableTool("");
    }
}

void TerrainTools::preloadTextures(){
    if(Game::debugOutput) qDebug() << "Preloading" ; 
    if(Game::preloadTextures.size() > 0) 
    {
        foreach (QString val, Game::preloadTextures)
        {        
            preloadTexTool(val);        
        }                
    }
    QTime cTime = QTime::currentTime().addMSecs(300);  
    while (QTime::currentTime() < cTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    texPreviewEnabled(0);
}

void TerrainTools::preloadTexTool(QString filename)
{
    filename = Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),
            Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season), filename);
    if(!QFile::exists(filename)){
        QFileInfo requested(filename);
        const QString baseName = requested.absolutePath() + "/" + requested.completeBaseName();
        const QStringList alternatives = QStringList() << "ace" << "dds" << "bmp" << "png";
        QString existingAlternative;
        for(const QString &extension : alternatives){
            const QString candidate = baseName + "." + extension;
            if(QFile::exists(candidate)){
                existingAlternative = candidate;
                break;
            }
        }
        if(existingAlternative.isEmpty())
            return;
        filename = existingAlternative;
    }
    if(Game::debugOutput) qDebug() << "preloading " << filename;
    int result = TexLib::getTex(filename);     
    if(result == -1)
        {    
            int tid = TexLib::addTex(filename);
            this->paintBrush->texId = tid;
            this->paintBrush->tex = TexLib::mtex[tid];

            texLastItems.push_back(qMakePair(this->paintBrush->texId, this->paintBrush->tex));
            if(texLastItems.size() > 7){
                texLastItems.removeFirst();
            }
        }
}


void TerrainTools::setTexToolEnabled(){
    QPushButton *loadButton = qobject_cast<QPushButton*>(sender());
    if(loadButton != NULL){
        loadButton->setChecked(true);
        loadButton->repaint();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    QFileDialog fd(this, "Load Terrain Texture");
    if(hideGeneratedTerrtex != NULL && hideGeneratedTerrtex->isChecked()){
        fd.setOption(QFileDialog::DontUseNativeDialog, true);
        TerrtexFileFilterModel *fileFilter = new TerrtexFileFilterModel(&fd);
        fd.setProxyModel(fileFilter);
    }
    QStringList terrainSubdirs = Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season);
    QString path = Terrain::routeTerrtexPath(terrainSubdirs.isEmpty() ? "" : terrainSubdirs[0]);
    path.replace("//", "/");
    fd.setDirectory(path);
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilters(QStringList()
        << "Terrain textures (*.ace *.dds *.jpg *.jpeg *.png *.bmp *.tga)"
        << "All files (*)");
    if(fd.testOption(QFileDialog::DontUseNativeDialog)){
        QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>();
        if(buttonBox != NULL)
            buttonBox->setContentsMargins(0,0,6,4);
    }
    //QTreeView *tree = fd->findChild <QTreeView*>();
    //tree->setRootIsDecorated(true);
    //tree->setItemsExpandable(true);
    //fd->setFileMode(QFileDialog::F);
    //fd->setOption(QFileDialog::ShowDirsOnly);
    //fd->setViewMode(QFileDialog::Detail);
    int result = fd.exec();
    if(loadButton != NULL)
        loadButton->setChecked(false);
    if (!result)
        return;

    QStringList failedTextures;
    bool loadedAnyTexture = false;
    const QStringList supportedExtensions = QStringList()
        << "ace" << "dds" << "jpg" << "jpeg" << "png" << "bmp" << "tga";
    for(const QString &filename : fd.selectedFiles()){
        const QFileInfo fileInfo(filename);
        if(!fileInfo.exists()
                || !supportedExtensions.contains(fileInfo.suffix().toLower())){
            failedTextures.append(fileInfo.fileName());
            continue;
        }

        qDebug() << "texture file " << filename;
        const int tid = TexLib::addTex(filename);
        const auto textureIt = TexLib::mtex.find(tid);
        if(tid < 0 || textureIt == TexLib::mtex.end() || textureIt->second == NULL){
            failedTextures.append(fileInfo.fileName());
            continue;
        }

        Texture *texture = textureIt->second;
        QElapsedTimer loadTimer;
        loadTimer.start();
        while(!texture->loaded && !texture->missing && !texture->error
                && loadTimer.elapsed() < 5000){
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        if(!textureReadyForPreview(texture)){
            failedTextures.append(fileInfo.fileName());
            continue;
        }

        this->paintBrush->texId = tid;
        this->paintBrush->tex = texture;
        this->paintBrush->useTexture = true;
        loadedAnyTexture = true;
        texLastItems.push_back(qMakePair(tid, texture));
        if(texLastItems.size() > 7){
            texLastItems.removeFirst();
        }
        updateTexPrev();
    }

    if(loadedAnyTexture){
        emit setPaintBrush(this->paintBrush);
        emit enableTool("paintToolTexture");
    }
    if(!failedTextures.isEmpty()){
        QMessageBox::warning(this, "Load Terrain Texture",
            "The following file(s) could not be decoded as terrain textures:\n\n"
            + failedTextures.join("\n"));
    }
}

// brush

void TerrainTools::setBrushSize(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sSize->setValue(ival);
    this->paintBrush->size = ival;
}

void TerrainTools::setBrushAlpha(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sIntensity->setValue(ival);
    this->paintBrush->alpha = (float)ival/100;
}

void TerrainTools::setBrushSize(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leSize->setText(QString::number(val,10));
    this->paintBrush->size = val;
}

void TerrainTools::setBrushAlpha(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leIntensity->setText(QString::number(val,10));
    this->paintBrush->alpha = (float)val/100;
}

void TerrainTools::setTextureRotation(QString val){
    int ival = val.toInt(0, 10);
    if(ival < 0) ival = 0;
    if(ival > 360) ival = 360;
    this->sTextureRotation->setValue(ival);
    this->paintBrush->texRotationDegrees = ival;
    this->paintBrush->texTransformation = ival == 0 ? Brush::ROT0 : Brush::CUSTOM;
    emit setPaintBrush(this->paintBrush);
}

void TerrainTools::setTextureRotation(int val){
    if(val < 0) val = 0;
    if(val > 360) val = 360;
    this->leTextureRotation->setText(QString::number(val,10));
    this->paintBrush->texRotationDegrees = val;
    this->paintBrush->texTransformation = val == 0 ? Brush::ROT0 : Brush::CUSTOM;
    emit setPaintBrush(this->paintBrush);
}

void TerrainTools::setFheight(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    float ival = val.toFloat(0);
    this->paintBrush->hFixed = ival;
}

void TerrainTools::setHtype(int val){
    emit setPaintBrush(this->paintBrush);
    if(val < 0) return;
    this->paintBrush->hType = val;
}

void TerrainTools::setSeasonType(int val){
    QString requestedSeason = "Summer";
    if(val == 1)
        requestedSeason = "Spring";
    else if(val == 2)
        requestedSeason = "Autumn";
    else if(val == 3)
        requestedSeason = "Winter";
    else if(val == 4)
        requestedSeason = "Night";

    if (requestedSeason.compare(Game::season, Qt::CaseInsensitive) == 0)
        return;

    QVector<QString> unsavedTerrain;
    if (Game::terrainLib != NULL)
        Game::terrainLib->getUnsavedInfo(unsavedTerrain);
    if (!unsavedTerrain.isEmpty()) {
        QMessageBox::warning(this, "Texture Set",
                QString("Save the current terrain changes before switching texture sets.\n\n"
                        "%1 terrain tile(s) have unsaved changes, including terrain paint.")
                .arg(unsavedTerrain.size()));

        int previousIndex = 0;
        QString previousSeason = Game::season.trimmed().toLower();
        if (previousSeason == "spring")
            previousIndex = 1;
        else if (previousSeason == "autumn" || previousSeason == "fall")
            previousIndex = 2;
        else if (previousSeason == "winter" || previousSeason == "snow")
            previousIndex = 3;
        else if (previousSeason == "night")
            previousIndex = 4;

        QSignalBlocker blocker(seasonType);
        seasonType->setCurrentIndex(previousIndex);
        return;
    }

    if(val == 1) {
        Game::season = requestedSeason;
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    } else if(val == 2) {
        Game::season = requestedSeason;
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    } else if(val == 3) {
        Game::season = requestedSeason;
        Game::sunLightDirection[0] = 2;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 2;
    } else if(val == 4) {
        Game::season = requestedSeason;
        Game::sunLightDirection[0] = -10;
        Game::sunLightDirection[1] = -10;
        Game::sunLightDirection[2] = -10;
    } else {
        Game::season = requestedSeason;
        Game::sunLightDirection[0] = -1;
        Game::sunLightDirection[1] = 2;
        Game::sunLightDirection[2] = 1;
    }

    if (Game::terrainLib != NULL)
        Game::terrainLib->reloadLoaded();
    if (Game::currentShapeLib != NULL)
        Game::currentShapeLib->refreshSeasonTextures();
    if (Game::currentRoute != NULL)
        Game::currentRoute->reloadLoadedWorldObjects();
    ForestObj::InvalidateSeasonTextures();
    PolyForestObj::InvalidateSeasonTextures();

    updateActiveTexturePreviewBorders();
    emit setPaintBrush(this->paintBrush);
}




// embarkment

void TerrainTools::setEsize(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEsize->setText(QString::number(val,10));
    this->paintBrush->eSize = val;
}
void TerrainTools::setEsize(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEsize->setValue(ival);
    this->paintBrush->eSize = (float)ival/100;
}
void TerrainTools::setEemb(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEemb->setText(QString::number(val,10));
    this->paintBrush->eEmb = val;
}
void TerrainTools::setEemb(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEemb->setValue(ival);
    this->paintBrush->eEmb = (float)ival/100;
}
void TerrainTools::setEcut(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEcut->setText(QString::number(val,10));
    this->paintBrush->eCut = val;
}
void TerrainTools::setEcut(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEcut->setValue(ival);
    this->paintBrush->eCut = (float)ival/100;
}
void TerrainTools::setEradius(int val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    this->leEradius->setText(QString::number(val,10));
    this->paintBrush->eRadius = val;
}
void TerrainTools::setEradius(QString val){
    emit setPaintBrush(this->paintBrush);
    //qDebug() << "a";
    int ival = val.toInt(0, 10);
    this->sEradius->setValue(ival);
    this->paintBrush->eRadius = (float)ival/100;
}

void TerrainTools::setSun1(QString val){
        Game::skyColor[0] = val.toFloat();
}

void TerrainTools::setSun2(QString val){
        Game::skyColor[1] = val.toFloat();
}

void TerrainTools::setSun3(QString val){
        Game::skyColor[2] = val.toFloat();
}



//

void TerrainTools::setBrushTextureId(int val){
    if(val < 0)
        return;
    const auto textureIt = TexLib::mtex.find(val);
    if(textureIt == TexLib::mtex.end()
            || !textureReadyForPreview(textureIt->second))
        return;

    this->paintBrush->texId = val;
    this->paintBrush->tex = textureIt->second;
    this->paintBrush->useTexture = true;

    texLastItems.push_back(qMakePair(this->paintBrush->texId, this->paintBrush->tex));
    if(texLastItems.size() > 7){
        texLastItems.removeFirst();
    }
    updateTexPrev();
    emit setPaintBrush(this->paintBrush);
    emit enableTool("paintToolTexture");
}

QString TerrainTools::paintPresetFilePath()
{
    return Game::terrainPaintPresetFilePath();
}

QJsonArray TerrainTools::readPaintPresets()
{
    QFile file(paintPresetFilePath());
    if (!file.exists())
        return QJsonArray();
    if (!file.open(QIODevice::ReadOnly))
        return QJsonArray();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return QJsonArray();

    return doc.object()["presets"].toArray();
}

bool TerrainTools::writePaintPresets(const QJsonArray &presets)
{
    QFile file(paintPresetFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QJsonObject root;
    root["version"] = 1;
    root["presets"] = presets;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void TerrainTools::refreshPaintPresets()
{
    if (presetCombo == NULL)
        return;

    QString selected = presetCombo->currentText();
    presetCombo->blockSignals(true);
    presetCombo->clear();

    QJsonArray presets = readPaintPresets();
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        QString name = preset["name"].toString().trimmed();
        if (!name.isEmpty())
            presetCombo->addItem(name);
    }

    int idx = presetCombo->findText(selected);
    if (idx >= 0)
        presetCombo->setCurrentIndex(idx);
    presetCombo->blockSignals(false);
}

QString TerrainTools::currentTexturePresetPath()
{
    if (paintBrush == NULL || paintBrush->tex == NULL)
        return "";

    QString path = paintBrush->tex->pathid;
    path.replace("\\", "/");
    if (path.isEmpty())
        return "";

    return QFileInfo(path).fileName();
}

void TerrainTools::applyTexturePresetPath(QString texturePath)
{
    texturePath = texturePath.trimmed();
    if (texturePath.isEmpty())
        return;

    QString path = texturePath;
    if (QFileInfo(path).isRelative()) {
        path = Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),
                Terrain::textureSubdirCandidatesForFlags(Game::TextureFlags["snow"], Game::season), texturePath);
    }

    if (!QFile::exists(path)) {
        QMessageBox::warning(this, "Paint Preset", "Preset texture was not found:\n" + texturePath);
        return;
    }

    int texId = TexLib::addTex(path);
    if (texId < 0 || TexLib::mtex.find(texId) == TexLib::mtex.end() || TexLib::mtex[texId] == NULL) {
        QMessageBox::warning(this, "Paint Preset", "Preset texture could not be loaded:\n" + texturePath);
        return;
    }

    paintBrush->texId = texId;
    paintBrush->tex = TexLib::mtex[texId];
    paintBrush->useTexture = true;

    texLastItems.push_back(qMakePair(paintBrush->texId, paintBrush->tex));
    if (texLastItems.size() > 7)
        texLastItems.removeFirst();
    updateTexPrev();
}

void TerrainTools::applyPaintPreset()
{
    QString selected = presetCombo->currentText();
    if (selected.isEmpty())
        return;

    QJsonArray presets = readPaintPresets();
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString() != selected)
            continue;

        applyTexturePresetPath(preset["texture"].toString());

        int size = preset["size"].toInt(paintBrush->size);
        if (size < 1) size = 1;
        if (size > 100) size = 100;
        setBrushSize(size);
        sSize->setValue(size);

        int intensity = preset["intensity"].toInt((int)(paintBrush->alpha * 100.0f));
        if (intensity < 1) intensity = 1;
        if (intensity > 100) intensity = 100;
        setBrushAlpha(intensity);
        sIntensity->setValue(intensity);

        if (preset.contains("brush"))
            setBrushShapeIndex(preset["brush"].toInt(currentBrushShape));

        if (preset.contains("rotation")) {
            int rotation = preset["rotation"].toInt(paintBrush->texRotationDegrees);
            if (rotation < 0) rotation = 0;
            if (rotation > 360) rotation = 360;
            setTextureRotation(rotation);
            sTextureRotation->setValue(rotation);
        }

        emit setPaintBrush(paintBrush);
        return;
    }
}

void TerrainTools::savePaintPreset()
{
    QString texturePath = currentTexturePresetPath();
    if (texturePath.isEmpty()) {
        QMessageBox::warning(this, "Paint Preset", "Choose a texture before saving a paint preset.");
        return;
    }

    bool ok = false;
    QString defaultName = presetCombo->currentText();
    QString name = QInputDialog::getText(this, "Save Paint Preset", "Preset name:",
            QLineEdit::Normal, defaultName, &ok).trimmed();
    if (!ok)
        return;
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Paint Preset", "Preset name cannot be empty.");
        return;
    }
    if (name.length() > 48)
        name = name.left(48);

    QJsonArray presets = readPaintPresets();
    QJsonArray updated;
    bool replacing = false;
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString().compare(name, Qt::CaseInsensitive) == 0) {
            replacing = true;
            continue;
        }
        updated.append(preset);
    }

    if (replacing) {
        if (QMessageBox::question(this, "Save Paint Preset",
                "Replace the existing preset named \"" + name + "\"?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
    }

    QJsonObject preset;
    preset["name"] = name;
    preset["texture"] = texturePath;
    preset["size"] = paintBrush->size;
    preset["intensity"] = (int)(paintBrush->alpha * 100.0f + 0.5f);
    preset["brush"] = currentBrushShape;
    preset["rotation"] = paintBrush->texRotationDegrees;
    updated.append(preset);

    if (!writePaintPresets(updated)) {
        QMessageBox::warning(this, "Paint Preset", "Could not write the route paint preset file.");
        return;
    }

    refreshPaintPresets();
    int idx = presetCombo->findText(name);
    if (idx >= 0)
        presetCombo->setCurrentIndex(idx);
}

void TerrainTools::removePaintPreset()
{
    QString selected = presetCombo->currentText();
    if (selected.isEmpty())
        return;

    if (QMessageBox::question(this, "Remove Paint Preset",
            "Remove preset \"" + selected + "\"?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QJsonArray presets = readPaintPresets();
    QJsonArray updated;
    for (int i = 0; i < presets.size(); i++) {
        QJsonObject preset = presets[i].toObject();
        if (preset["name"].toString() != selected)
            updated.append(preset);
    }

    if (!writePaintPresets(updated)) {
        QMessageBox::warning(this, "Paint Preset", "Could not write the route paint preset file.");
        return;
    }
    refreshPaintPresets();
}

void TerrainTools::clearRecentTextureHistory(){
    const bool textureToolWasActive = buttonTools.contains("paintToolTexture")
            && buttonTools["paintToolTexture"] != NULL
            && buttonTools["paintToolTexture"]->isChecked();

    texLastItems.clear();
    if(paintBrush != NULL){
        paintBrush->texId = -1;
        paintBrush->tex = NULL;
        paintBrush->useTexture = false;
        paintBrush->mirrorSeason = false;
    }
    mirrorTexturePairReady = false;
    mainTexturePreviewReady = false;
    snowTexturePreviewReady = false;

    QPixmap blackPreview(106,106);
    blackPreview.fill(Qt::black);
    texPreviewLabel->setPixmap(blackPreview);
    texPreviewLabel->setText("");
    texPreviewLabel->setToolTip("");
    texPreviewLabel->setStyleSheet(
        "QLabel { background-color: #171717; border: 1px solid #555555; }");
    mirrorTexPreviewLabel->setPixmap(blackPreview);
    mirrorTexPreviewLabel->setText("");
    mirrorTexPreviewLabel->setToolTip("");
    mirrorTexPreviewLabel->setStyleSheet(
        "QLabel { background-color: #171717; border: 1px solid #555555; }");

    for(int i = 0; i < 6 && i < texPreviewLabels.size(); i++){
        texPreviewLabels[i]->setPixmap(*defaultTexPreview);
        texPreviewLabels[i]->setToolTip("");
    }
    mirrorPairStatus->setText("");

    {
        QSignalBlocker blocker(mirrorSeason);
        mirrorSeason->setChecked(false);
    }
    if(buttonTools.contains("paintToolTexture")
            && buttonTools["paintToolTexture"] != NULL){
        QSignalBlocker blocker(buttonTools["paintToolTexture"]);
        buttonTools["paintToolTexture"]->setChecked(false);
    }

    emit setPaintBrush(paintBrush);
    if(textureToolWasActive)
        emit enableTool("");
}

void TerrainTools::updateTexPrev(){
    if(this->paintBrush == NULL || !textureReadyForPreview(this->paintBrush->tex))
        return;

    ClickableLabel *tlabel;
    unsigned char * out;
    int idx;
    int res;
    for(int i = 1; i < 8; i++){
        idx = texLastItems.size() - i;
        if(idx < 0)
            continue;
        Texture *previewTexture = texLastItems[idx].second;
        if(!textureReadyForPreview(previewTexture))
            continue;
        QString textureName = QFileInfo(previewTexture->pathid).fileName();
        if(textureName.isEmpty())
            textureName = previewTexture->pathid;
        if(i == 1){
            tlabel = texPreviewLabel;
            res = 106;
            if(paintBrush->useTexture){
                out = this->paintBrush->tex->getImageData(res,res);
                if(this->paintBrush->tex->bytesPerPixel == 3)
                    tlabel->setPixmap(QPixmap::fromImage(
                        QImage(out, res, res, res * this->paintBrush->tex->bytesPerPixel,
                               QImage::Format_RGB888)));
                if(this->paintBrush->tex->bytesPerPixel == 4)
                    tlabel->setPixmap(QPixmap::fromImage(
                        QImage(out, res, res, res * this->paintBrush->tex->bytesPerPixel,
                               QImage::Format_RGBA8888)));
                tlabel->setToolTip(textureName);
            } else {
                updateColorPreview();
            }
        }// else {
        tlabel = texPreviewLabels[i-1];
        res = 36;
        out = previewTexture->getImageData(res,res);
        //}
        if(previewTexture->bytesPerPixel == 3)
            tlabel->setPixmap(QPixmap::fromImage(
                QImage(out, res, res, res * previewTexture->bytesPerPixel,
                       QImage::Format_RGB888)));
        if(previewTexture->bytesPerPixel == 4)
            tlabel->setPixmap(QPixmap::fromImage(
                QImage(out, res, res, res * previewTexture->bytesPerPixel,
                       QImage::Format_RGBA8888)));
        tlabel->setToolTip(textureName);
    }
    refreshMirrorTexturePreview(true);
}

bool TerrainTools::refreshMirrorTexturePreview(bool showBriefErrorIfDisabled){
    const bool mirrorWasActive = mirrorSeason != NULL && mirrorSeason->isChecked();
    mirrorTexturePairReady = false;

    if(paintBrush == NULL || paintBrush->tex == NULL){
        mirrorTexturePairReady = false;
        return false;
    }

    const QString textureName = QFileInfo(paintBrush->tex->pathid).fileName();
    if(textureName.isEmpty()){
        mirrorTexturePairReady = false;
        return false;
    }

    QString mainPath = Terrain::routeTerrtexPath() + textureName;
    QString snowPath = Terrain::routeTerrtexPath("snow/") + textureName;
    mainPath.replace("//", "/");
    snowPath.replace("//", "/");

    const QString season = Game::season.trimmed().toLower();
    const bool snowSeason = season == "winter"
            || season == "snow"
            || season == "wintersnow"
            || season == "autumnsnow"
            || season == "springsnow";

    // The active side must show the exact texture object selected by the user.
    // Reloading a same-named file from TERRTEX can resolve to a different
    // source/variant and makes the large swatch disagree with Recent.
    Texture *mainTexture = snowSeason
            ? loadTextureForPreview(mainPath)
            : paintBrush->tex;
    Texture *snowTexture = snowSeason
            ? paintBrush->tex
            : loadTextureForPreview(snowPath);
    const QString mainPreviewPath = snowSeason
            ? mainPath : paintBrush->tex->pathid;
    const QString snowPreviewPath = snowSeason
            ? paintBrush->tex->pathid : snowPath;
    const bool mainReady = textureReadyForPreview(mainTexture);
    const bool snowReady = textureReadyForPreview(snowTexture);
    mainTexturePreviewReady = mainReady;
    snowTexturePreviewReady = snowReady;

    auto showPreviewState = [](QLabel *label, Texture *texture, bool ready,
            const QString &path){
        label->setPixmap(QPixmap());
        label->setText("");
        label->setToolTip(QFileInfo(path).fileName());
        label->setStyleSheet(ready
            ? "QLabel { background-color: #171717; border: 1px solid #70590e; }"
            : "QLabel { background-color: #171717; border: 1px solid #555555; }");
        if(ready)
            setTexturePreview(label, texture, 106);
    };

    showPreviewState(texPreviewLabel, mainTexture, mainReady, mainPreviewPath);
    showPreviewState(mirrorTexPreviewLabel, snowTexture, snowReady, snowPreviewPath);

    mirrorTexturePairReady = mainReady && snowReady;
    updateActiveTexturePreviewBorders();
    if(mirrorTexturePairReady){
        mirrorPairStatus->setText("");
        return true;
    }

    if(mirrorWasActive){
        QSignalBlocker blocker(mirrorSeason);
        mirrorSeason->setChecked(false);
        paintBrush->mirrorSeason = false;
        emit setPaintBrush(paintBrush);
        if(showBriefErrorIfDisabled)
            showMirrorStatusBriefly("No Mirror");
    }
    return false;
}

void TerrainTools::updateActiveTexturePreviewBorders(){
    const bool hasTexture = paintBrush != NULL
            && paintBrush->useTexture
            && paintBrush->tex != NULL;
    const bool mirrorActive = hasTexture
            && paintBrush->mirrorSeason
            && mirrorTexturePairReady;
    const QString season = Game::season.trimmed().toLower();
    const bool snowSeason = season == "winter"
            || season == "snow"
            || season == "wintersnow"
            || season == "autumnsnow"
            || season == "springsnow";
    const bool mainActive = hasTexture && (mirrorActive || !snowSeason);
    const bool snowActive = hasTexture && (mirrorActive || snowSeason);

    auto applyBorder = [](QLabel *label, bool ready, bool active){
        const QString border = active && ready
                ? "#ffe140"
                : (ready ? "#70590e" : "#555555");
        label->setStyleSheet(QString(
            "QLabel { background-color: #171717; border: 1px solid %1; }")
            .arg(border));
    };
    applyBorder(texPreviewLabel, mainTexturePreviewReady, mainActive);
    applyBorder(mirrorTexPreviewLabel, snowTexturePreviewReady, snowActive);
}

void TerrainTools::showMirrorStatusBriefly(QString text){
    mirrorPairStatus->setText(text);
    mirrorPairStatus->setStyleSheet(
        "QLabel { color: #e28b7c; font-weight: bold; padding: 1px; }");
    QTimer::singleShot(1800, this, [this, text](){
        if(mirrorPairStatus != NULL && mirrorPairStatus->text() == text){
            mirrorPairStatus->setText("");
            mirrorPairStatus->setStyleSheet(
                "QLabel { color: #b0b0b0; padding: 1px; }");
        }
    });
}

void TerrainTools::texPreviewEnabled(int val){
    //qDebug() << "TerrTools679:" << val;
    if(val == 6){
        nextBrushShape();
        return;
    }
    // qDebug() << "TerrTools 829:" << val ;
    int idx = texLastItems.size() - val - 1;
    if(idx > texLastItems.size() - 1) return;
    if(idx < 0) return;
    this->paintBrush->tex = texLastItems[idx].second;
    this->paintBrush->texId = texLastItems[idx].first;
    updateTexPrev();
}

void TerrainTools::msg(QString text, QString val){
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
    } else if(text == "brushDirection"){
        QString t = buttonTools["heightTool"]->text().left(buttonTools["heightTool"]->text().length() - 1);
        buttonTools["heightTool"]->setText(t+val);
        t = buttonTools["waterTerrTool"]->text().left(buttonTools["waterTerrTool"]->text().length() - 1);
        buttonTools["waterTerrTool"]->setText(t+val);
        t = buttonTools["gapsTerrainTool"]->text().left(buttonTools["gapsTerrainTool"]->text().length() - 1);
        buttonTools["gapsTerrainTool"]->setText(t+val);
    } else if(text == "textureRotation"){
        int rotation = val.toInt();
        if(rotation < 0) rotation = 0;
        if(rotation > 360) rotation = 360;
        sTextureRotation->blockSignals(true);
        leTextureRotation->setText(QString::number(rotation, 10));
        sTextureRotation->setValue(rotation);
        sTextureRotation->blockSignals(false);
        paintBrush->texRotationDegrees = rotation;
    } else if(text == "preloadTextures")
    { 
        qDebug() << "emit received";
        preloadTextures();
    }
}

void TerrainTools::setPinPointBrush()
{
      setBrushSize(1);
      setBrushAlpha(1);   
      
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      
}
void TerrainTools::resetDefaultValues()
{
    if(Game::terrBrushColor)        
    {
        paintBrush->color[0] = Game::terrBrushColor->red();
        paintBrush->color[1] = Game::terrBrushColor->green();
        paintBrush->color[2] = Game::terrBrushColor->blue();
        colorw->setStyleSheet("background-color:" + Game::terrBrushColor->name() + ";");
        colorw->setText(Game::terrBrushColor->name());
    }
    if(Game::terrainTools != NULL)
    {
      setEsize(Game::terrainTools[0]);
      setEemb(Game::terrainTools[1]);
      setEcut(Game::terrainTools[2]);
      setEradius(Game::terrainTools[3]);
      setBrushSize(Game::terrainTools[4]);
      setBrushAlpha(Game::terrainTools[5]);   
      setTextureRotation(0);
      sSize->setValue(paintBrush->size);
      sIntensity->setValue(paintBrush->alpha*100);
      sTextureRotation->setValue(paintBrush->texRotationDegrees);
      sEsize->setValue(paintBrush->eSize);
      sEemb->setValue(paintBrush->eEmb);
      sEcut->setValue(paintBrush->eCut);
      sEradius->setValue(paintBrush->eRadius);
    }
   
    preloadTextures();
    refreshPaintPresets();

}

void TerrainTools::resetRouteTerrtexPaint()
{
    QVector<QString> unsavedItems;
    if (Game::currentRoute != NULL)
        Game::currentRoute->getUnsavedInfo(unsavedItems);
    else if (Game::terrainLib != NULL)
        Game::terrainLib->getUnsavedInfo(unsavedItems);

    if (!unsavedItems.isEmpty()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint",
                QString("There are %1 pending route change(s).\n\n"
                        "Save your changes, then retry the reset.")
                .arg(unsavedItems.size()));
        return;
    }

    QString routePath = Game::root + "/routes/" + Game::route;
    QString tilePath = routePath + "/tiles";
    QString terrtexPath = routePath + "/terrtex";

    QDir tileDir(tilePath);
    if (!tileDir.exists()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint", "Route tiles folder was not found.");
        return;
    }

    QFileInfoList tileFiles = tileDir.entryInfoList(QStringList() << "*.t", QDir::Files, QDir::Name);
    if (tileFiles.isEmpty()) {
        QMessageBox::warning(this, "Reset Route Terrtex Paint", "No terrain tile files were found.");
        return;
    }

    QString warning = "This will reset every terrain tile patch in the current route to terrain.ace "
            "and delete per-tile ACE/DDS files from terrtex whose names start with a tile name.\n\n"
            "This cannot be undone from TSRE. Continue?";
    if (QMessageBox::question(this, "Reset Route Terrtex Paint", warning,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QSet<QString> tileNames;
    int tilesReset = 0;
    int tilesFailed = 0;

    QProgressDialog progress("Resetting route terrain paint...", "Cancel", 0, tileFiles.size(), this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);

    for (int i = 0; i < tileFiles.size(); i++) {
        if (progress.wasCanceled())
            break;

        QFileInfo tileInfo = tileFiles[i];
        QString tileName = tileInfo.completeBaseName();
        tileNames.insert(tileName.toLower());
        progress.setValue(i);
        progress.setLabelText("Resetting " + tileName + ".t");
        qApp->processEvents();

        TFile tfile;
        if (!tfile.readT(tileInfo.absoluteFilePath())) {
            tilesFailed++;
            continue;
        }

        int defaultMat = tfile.getMatByTexture("terrain.ace");
        if (defaultMat < 0)
            defaultMat = tfile.newMat();

        int patches = tfile.patchsetNpatches;
        for (int patch = 0; patch < patches * patches; patch++) {
            tfile.tdata[patch * 13 + 6] = defaultMat;
            tfile.tdata[patch * 13 + 7] = 0.001f;
            tfile.tdata[patch * 13 + 8] = 0.001f;
            tfile.tdata[patch * 13 + 9] = 0.062375f;
            tfile.tdata[patch * 13 + 10] = 0.0f;
            tfile.tdata[patch * 13 + 11] = 0.0f;
            tfile.tdata[patch * 13 + 12] = 0.062375f;
        }

        tfile.save(tileInfo.absoluteFilePath());
        tilesReset++;
    }
    progress.setValue(tileFiles.size());

    int filesDeleted = 0;
    int filesFailed = 0;
    QDir terrtexDir(terrtexPath);
    if (terrtexDir.exists()) {
        QFileInfoList texFiles = terrtexDir.entryInfoList(QStringList() << "*.ace" << "*.ACE" << "*.dds" << "*.DDS", QDir::Files, QDir::Name);
        for (int i = 0; i < texFiles.size(); i++) {
            QFileInfo texInfo = texFiles[i];
            QString baseName = texInfo.completeBaseName().toLower();
            int split = baseName.indexOf('_');
            if (split < 0)
                continue;

            QString tileName = baseName.left(split);
            if (!tileNames.contains(tileName))
                continue;

            if (QFile::remove(texInfo.absoluteFilePath()))
                filesDeleted++;
            else
                filesFailed++;
        }
    }

    int tilesReloaded = 0;
    if (Game::terrainLib != NULL)
        tilesReloaded = Game::terrainLib->reloadLoaded();

    QMessageBox::information(this, "Reset Route Terrtex Paint",
            QString("Reset %1 tile(s).\nFailed tile saves: %2\nDeleted %3 per-tile texture file(s).\nFailed deletes: %4\nReloaded %5 loaded tile(s).")
            .arg(tilesReset)
            .arg(tilesFailed)
            .arg(filesDeleted)
            .arg(filesFailed)
            .arg(tilesReloaded));
}

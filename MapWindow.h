/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef MAPWINDOW_H
#define	MAPWINDOW_H

#include <QtWidgets>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class QNetworkReply;
class QImage;
class IghCoordinate;
class LatitudeLongitudeCoordinate;
struct PreciseTileCoordinate;
class QPushButton;
class MapData;

class MapWindow : public QDialog {
    Q_OBJECT
public:
    int tileX;
    int tileZ;
    int tileSize;
    
    static std::unordered_map<int, QImage*> mapTileImages;
    static std::unordered_set<int> diskLoadedMapTiles;
    static int isAlpha;
    static bool routeMapOverlaysVisible;
    static QHash<QString, bool> tileMapVisibilityOverrides;
    static bool LoadMapFromDisk(int x, int z);
    static void releaseDiskMapFromMemory(int x, int z);
    static void clearMapTileImages();
    static void loadMapOverlayState();
    static void unloadMapOverlayState();
    static void clearMapOverlayState();
    static bool mapOverlayVisibleForTile(int x, int z);
    static void setTileMapOverlayVisible(int x, int z, bool visible);
    static void setRouteMapOverlaysVisible(bool visible);
    explicit MapWindow(QWidget *parent = nullptr);
    virtual ~MapWindow();
    bool ok = false;

    int exec();
    
public slots:
    void load();
    void saveToDisk();
    void colorComboActivated(QString val);
    void alphaBoxActivated(int val);
    void reload();
    void isStatusInfo(QString val);

private:
    QVector<MapData*> mapServices;
    MapData *dane = NULL;
    QLabel* imageLabel;
    float minlat, minlon, maxlat, maxlon;
    bool invert = false;
    IghCoordinate* igh = NULL;
    LatitudeLongitudeCoordinate* minLatlon = NULL;
    LatitudeLongitudeCoordinate* maxLatlon = NULL;
    PreciseTileCoordinate* aCoords = NULL;
    QPushButton *loadButton = NULL;
    QSpinBox alphaBox;
    QComboBox mapServicesCombo;
};

#endif	/* MAPWINDOW_H */


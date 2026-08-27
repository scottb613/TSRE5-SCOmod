/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "RouteEditorGLWidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QJsonObject>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include <math.h>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif
#include "GLUU.h"
#include "SFile.h"
#include "ReadFile.h"
#include "FileBuffer.h"
#include "Route.h"
#include "GLMatrix.h"
#include "Eng.h"
#include "Tile.h"
#include "Game.h"
#include "GuiFunct.h"
#include "GLH.h"
#include "Vector2f.h"
#include "TerrainLib.h"
#include "TDB.h"
#include "Brush.h"
#include "GeoCoordinates.h"
#include "MapWindow.h"
#include "TexLib.h"
#include "TerrainTreeWindow.h"
#include "ShapeLib.h"
#include "EngLib.h"
#include "QOpenGLFunctions_3_3_Core"
#include "Undo.h"
#include "Environment.h"
#include "Terrain.h"
#include "WaterBedClearanceMath.h"
#include "RulerObj.h"
#include <QQueue>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <limits>
#include "ActivityObject.h"
#include "Consist.h"
#include "Path.h"
#include "GuiFunct.h"
#include "GuiGlCompass.h"
#include "Skydome.h"
#include "OpenGL3Renderer.h"
#include <QDebug>
#include "RouteEditorClient.h"
#include "RouteClient.h"
#include "ClientInfo.h"
#include "StatusWindow.h"
#include "Texture.h"
#include "ObjTools.h"
#include "RouteMergeDialog.h"
#include "TerrainTools.h" // Include the dialog header
#include "TRitem.h"
#include "TrackObj.h"
#include "DynTrackObj.h"
#include "TDB.h"
#include "ForestDefinition.h"
#include "ForestGenerator.h"
#include "ForestOsmCache.h"
#include "ForestPatchBaker.h"
#include "ForestBakeManifest.h"
#include "PolyVegObject.h"
#include "TrackItemObj.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace {

bool osmRuleMatches(const ForestOsmMatchRule &rule,
                    const QHash<QString, QString> &tags) {
    for(auto it = rule.tags.constBegin(); it != rule.tags.constEnd(); ++it) {
        if(!tags.contains(it.key()) || !it.value().contains(tags.value(it.key())))
            return false;
    }
    return !rule.tags.isEmpty();
}

bool osmRecipeMatches(const ForestRecipeDefinition &recipe,
                      const ForestOsmPolygon &polygon) {
    if(!recipe.osmCategories.isEmpty()
            && !recipe.osmCategories.contains(polygon.category))
        return false;
    if(recipe.osmMatchAny.isEmpty())
        return !recipe.osmCategories.isEmpty();
    for(const ForestOsmMatchRule &rule : recipe.osmMatchAny) {
        if(osmRuleMatches(rule, polygon.tags)) return true;
    }
    return false;
}

QString polyVegFloodKey(const ForestOsmPolygon &polygon) {
    const QString fillColor = polygon.tags.value(QStringLiteral("fillcolor"));
    const QString styleId = polygon.tags.value(QStringLiteral("styleid"));
    return polygon.category + QChar('|')
        + (fillColor.isEmpty() ? styleId : fillColor);
}

int tileForPlanCoordinate(double coordinate) {
    return static_cast<int>(std::floor((coordinate + 1024.0) / 2048.0));
}

quint64 polyVegTileKey(int tileX, int tileZ) {
    return (static_cast<quint64>(static_cast<quint32>(tileX)) << 32)
        | static_cast<quint32>(tileZ);
}

class PolyVegViewportFreeze {
public:
    explicit PolyVegViewportFreeze(QWidget *viewport)
        : viewport(viewport), restoreUpdates(
              viewport != nullptr && viewport->updatesEnabled()) {
        if(restoreUpdates)
            viewport->setUpdatesEnabled(false);
    }

    ~PolyVegViewportFreeze() {
        finish();
    }

    PolyVegViewportFreeze(const PolyVegViewportFreeze &) = delete;
    PolyVegViewportFreeze &operator=(const PolyVegViewportFreeze &) = delete;

    void finish() {
        if(!restoreUpdates || viewport == nullptr)
            return;
        viewport->setUpdatesEnabled(true);
        viewport->update();
        restoreUpdates = false;
    }

private:
    QWidget *viewport = nullptr;
    bool restoreUpdates = false;
};

bool pointInRing(const ForestPlanPoint &point, const ForestPlanRing &ring) {
    bool inside = false;
    for(int i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const ForestPlanPoint &a = ring.at(i);
        const ForestPlanPoint &b = ring.at(j);
        if(((a.z > point.z) != (b.z > point.z))
                && point.x < (b.x-a.x)*(point.z-a.z)/(b.z-a.z)+a.x)
            inside = !inside;
    }
    return inside;
}

bool pointInBoundary(const ForestPlanPoint &point,
                     const ForestPlantingBoundary &boundary) {
    if(!pointInRing(point, boundary.outer)) return false;
    for(const ForestPlanRing &hole : boundary.holes)
        if(pointInRing(point, hole)) return false;
    return true;
}

QVector<ForestPlantingBoundary> clipPolyVegBoundaryToTile(
        const ForestPlantingBoundary &boundary, int tileX, int tileZ) {
    QPainterPath source;
    source.setFillRule(Qt::OddEvenFill);
    auto appendRing = [&source](const ForestPlanRing &ring) {
        if(ring.size() < 3) return;
        source.moveTo(ring.first().x, ring.first().z);
        for(int index = 1; index < ring.size(); ++index)
            source.lineTo(ring[index].x, ring[index].z);
        source.closeSubpath();
    };
    appendRing(boundary.outer);
    for(const ForestPlanRing &hole : boundary.holes)
        appendRing(hole);

    const QRectF tileBounds(tileX*2048.0 - 1024.0,
                            tileZ*2048.0 - 1024.0,
                            2048.0, 2048.0);
    QPainterPath tilePath;
    tilePath.addRect(tileBounds);
    const QList<QPolygonF> clippedPolygons =
        source.intersected(tilePath).toFillPolygons();
    QVector<ForestPlantingBoundary> clipped;
    for(const QPolygonF &polygon : clippedPolygons) {
        ForestPlantingBoundary piece;
        for(const QPointF &point : polygon) {
            if(!piece.outer.isEmpty()
                    && qFuzzyCompare(piece.outer.last().x + 1.0, point.x() + 1.0)
                    && qFuzzyCompare(piece.outer.last().z + 1.0, point.y() + 1.0))
                continue;
            piece.outer.append({point.x(), point.y()});
        }
        if(piece.outer.size() > 1
                && qFuzzyCompare(piece.outer.first().x + 1.0,
                                 piece.outer.last().x + 1.0)
                && qFuzzyCompare(piece.outer.first().z + 1.0,
                                 piece.outer.last().z + 1.0))
            piece.outer.removeLast();
        if(piece.outer.size() >= 3)
            clipped.append(piece);
    }
    return clipped;
}

bool polyVegTerrainHeight(double planX, double planZ, float &height) {
    int tileX = tileForPlanCoordinate(planX);
    int tileZ = tileForPlanCoordinate(planZ);
    float localX = static_cast<float>(planX - tileX*2048.0);
    float localZ = static_cast<float>(planZ - tileZ*2048.0);
    Game::check_coords(tileX, tileZ, localX, localZ);
    if(Game::terrainLib == nullptr || !Game::terrainLib->load(tileX, tileZ))
        return false;
    height = Game::terrainLib->getHeight(
        tileX, tileZ, localX, localZ, false);
    return std::isfinite(height);
}

class PolyVegDatabaseClearance {
public:
    PolyVegDatabaseClearance(TDB *database, double clearanceMetres)
        : database(database), clearanceSquared(
              clearanceMetres*clearanceMetres) {}

    bool blocks(double planX, double terrainY, double planZ) {
        if(database == nullptr || !database->loaded || clearanceSquared <= 0.0)
            return false;
        const int tileX = tileForPlanCoordinate(planX);
        const int tileZ = tileForPlanCoordinate(planZ);
        const quint64 key = polyVegTileKey(tileX, tileZ);
        if(!segments.contains(key))
            loadTile(tileX, tileZ, key);

        const auto nearbyIt = segments.constFind(key);
        if(nearbyIt == segments.constEnd())
            return false;
        const QVector<Segment> &nearby = nearbyIt.value();
        for(const Segment &segment : nearby) {
            const double vx = segment.bx-segment.ax;
            const double vy = segment.by-segment.ay;
            const double vz = segment.bz-segment.az;
            const double lengthSquared = vx*vx+vy*vy+vz*vz;
            double t = 0.0;
            if(lengthSquared > 0.000001)
                t = std::clamp(((planX-segment.ax)*vx
                    +(terrainY-segment.ay)*vy+(planZ-segment.az)*vz)
                    / lengthSquared, 0.0, 1.0);
            const double dx = planX-(segment.ax+vx*t);
            const double dy = terrainY-(segment.ay+vy*t);
            const double dz = planZ-(segment.az+vz*t);
            if(dx*dx+dy*dy+dz*dz < clearanceSquared)
                return true;
        }
        return false;
    }

private:
    struct Segment {
        double ax, ay, az;
        double bx, by, bz;
    };

    void loadTile(int tileX, int tileZ, quint64 key) {
        QVector<Segment> copied;
        float tile[2] {static_cast<float>(tileX), static_cast<float>(tileZ)};
        float *lineBuffer = nullptr;
        int length = 0;
        database->getLines(lineBuffer, length, tile);
        copied.reserve(length);
        for(int index = 0; lineBuffer != nullptr && index < length*12;
                index += 12) {
            copied.append({
                tileX*2048.0+lineBuffer[index], lineBuffer[index+1],
                tileZ*2048.0+lineBuffer[index+2],
                tileX*2048.0+lineBuffer[index+6], lineBuffer[index+7],
                tileZ*2048.0+lineBuffer[index+8]
            });
        }
        segments.insert(key, copied);
    }

    TDB *database = nullptr;
    double clearanceSquared = 0.0;
    QHash<quint64, QVector<Segment>> segments;
};

class PolyVegWaterClearance {
public:
    explicit PolyVegWaterClearance(double clearanceMetres)
        : clearanceMetres(clearanceMetres) {}

    bool blocks(double planX, double planZ) {
        if(Game::terrainLib == nullptr || clearanceMetres <= 0.0)
            return false;
        const int centreTileX = tileForPlanCoordinate(planX);
        const int centreTileZ = tileForPlanCoordinate(planZ);
        const double searchDistance = clearanceMetres;
        const int tileRadius = static_cast<int>(
            std::ceil(searchDistance/2048.0));
        const QPointF candidate(planX, planZ);
        for(int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for(int dx = -tileRadius; dx <= tileRadius; ++dx) {
                const int tileX = centreTileX+dx;
                const int tileZ = centreTileZ+dz;
                const double minimumX = tileX*2048.0-1024.0;
                const double minimumZ = tileZ*2048.0-1024.0;
                const double distanceX = planX < minimumX
                    ? minimumX-planX
                    : (planX > minimumX+2048.0
                        ? planX-(minimumX+2048.0) : 0.0);
                const double distanceZ = planZ < minimumZ
                    ? minimumZ-planZ
                    : (planZ > minimumZ+2048.0
                        ? planZ-(minimumZ+2048.0) : 0.0);
                if(std::hypot(distanceX, distanceZ) > searchDistance)
                    continue;

                const quint64 key = polyVegTileKey(tileX, tileZ);
                if(!exclusionPaths.contains(key))
                    loadTile(tileX, tileZ, key);
                const auto path = exclusionPaths.constFind(key);
                if(path != exclusionPaths.constEnd()
                        && path.value().contains(candidate))
                    return true;
            }
        }
        return false;
    }

private:
    void loadTile(int tileX, int tileZ, quint64 key) {
        QPainterPath submergedCells;
        if(!Game::terrainLib->load(tileX, tileZ)) {
            exclusionPaths.insert(key, submergedCells);
            return;
        }
        Terrain *terrain = Game::terrainLib->getTerrainByXY(
            tileX, tileZ, false);
        if(terrain == nullptr || !terrain->loaded || !terrain->hasAnyWater()) {
            exclusionPaths.insert(key, submergedCells);
            return;
        }
        const int samples = terrain->getSampleCount();
        const int sampleSize = terrain->getSampleSize();
        const double tileSize = static_cast<double>(samples)*sampleSize;
        if(samples <= 0 || sampleSize <= 0
                || std::fabs(tileSize-2048.0) > 0.001) {
            exclusionPaths.insert(key, submergedCells);
            return;
        }

        const double tileMinimumX = tileX*2048.0-1024.0;
        const double tileMinimumZ = tileZ*2048.0-1024.0;
        for(int sampleZ = 0; sampleZ < samples; ++sampleZ) {
            const float localZ = static_cast<float>(
                -1024.0+(sampleZ+0.5)*sampleSize);
            int submergedRunStart = -1;
            for(int sampleX = 0; sampleX < samples; ++sampleX) {
                const float localX = static_cast<float>(
                    -1024.0+(sampleX+0.5)*sampleSize);
                const float centreHeight = 0.25f*(
                    terrain->terrainData[sampleZ][sampleX]
                    + terrain->terrainData[sampleZ][sampleX+1]
                    + terrain->terrainData[sampleZ+1][sampleX]
                    + terrain->terrainData[sampleZ+1][sampleX+1]);
                const bool submerged = terrain->isTerrainSubmergedAt(
                    tileX, tileZ, localX, localZ, centreHeight);
                if(submerged && submergedRunStart < 0)
                    submergedRunStart = sampleX;
                if(!submerged && submergedRunStart >= 0) {
                    submergedCells.addRect(
                        tileMinimumX+submergedRunStart*sampleSize,
                        tileMinimumZ+sampleZ*sampleSize,
                        (sampleX-submergedRunStart)*sampleSize, sampleSize);
                    submergedRunStart = -1;
                }
            }
            if(submergedRunStart >= 0)
                submergedCells.addRect(
                    tileMinimumX+submergedRunStart*sampleSize,
                    tileMinimumZ+sampleZ*sampleSize,
                    (samples-submergedRunStart)*sampleSize, sampleSize);
        }
        if(submergedCells.isEmpty()) {
            exclusionPaths.insert(key, submergedCells);
            return;
        }
        submergedCells = submergedCells.simplified();
        QPainterPathStroker setback;
        setback.setWidth(clearanceMetres*2.0);
        setback.setCapStyle(Qt::RoundCap);
        setback.setJoinStyle(Qt::RoundJoin);
        exclusionPaths.insert(key,
            submergedCells.united(setback.createStroke(submergedCells)));
    }

    double clearanceMetres = 0.0;
    QHash<quint64, QPainterPath> exclusionPaths;
};

bool polyVegTerrainSlopeAccepted(double planX, double planZ,
                                 double maximumSlopeDegrees) {
    if(maximumSlopeDegrees >= 90.0)
        return true;
    constexpr double sampleRadiusMetres = 8.0;
    float west = 0.0f, east = 0.0f, north = 0.0f, south = 0.0f;
    if(!polyVegTerrainHeight(planX-sampleRadiusMetres, planZ, west)
            || !polyVegTerrainHeight(planX+sampleRadiusMetres, planZ, east)
            || !polyVegTerrainHeight(planX, planZ-sampleRadiusMetres, north)
            || !polyVegTerrainHeight(planX, planZ+sampleRadiusMetres, south))
        return false;
    const double span = sampleRadiusMetres*2.0;
    const double gradientX = (east-west)/span;
    const double gradientZ = (south-north)/span;
    const double slopeDegrees = std::atan(std::hypot(gradientX, gradientZ))
        * 180.0/M_PI;
    return slopeDegrees <= maximumSlopeDegrees;
}

} // namespace


RouteEditorGLWidget::RouteEditorGLWidget(QWidget *parent)
: QOpenGLWidget(parent),
m_xRot(0),
m_yRot(0),
m_zRot(0) {

    this->installEventFilter(this);

    waterMessageLabel = new QLabel(this);
    waterMessageLabel->setAlignment(Qt::AlignCenter);
    waterMessageLabel->setWordWrap(true);
    waterMessageLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    const float messageScale = qMax(0.75f, Game::uiScale);
    QFont messageFont = QApplication::font();
    qreal messagePointSize = messageFont.pointSizeF();
    if(messagePointSize <= 0)
        messagePointSize = 9.0;
    messageFont.setPointSizeF(messagePointSize * 1.15);
    messageFont.setWeight(QFont::DemiBold);
    waterMessageLabel->setFont(messageFont);
    waterMessageLabel->setStyleSheet(QString(
        "QLabel {"
        " color: %1;"
        " background-color: rgba(31, 30, 27, 235);"
        " border: 1px solid %2;"
        " border-radius: %3px;"
        " padding: %4px %5px;"
        "}")
        .arg(Game::StyleYellowButtonHover)
        .arg(Game::StyleYellowButton)
        .arg(qRound(4.0f * messageScale))
        .arg(qRound(8.0f * messageScale))
        .arg(qRound(14.0f * messageScale)));
    waterMessageLabel->hide();

    connect(this, &RouteEditorGLWidget::waterHelperStatus,
            this, [this](const QString &text){
        showWaterMessage(text);
    });
}





bool RouteEditorGLWidget::eventFilter(QObject *object, QEvent *event){
    if (event->type() == QEvent::FocusIn){
        //qDebug() << "aaaaa";
        bolckContextMenu = true;
    }
    return false;
}

RouteEditorGLWidget::~RouteEditorGLWidget() {
    delete polyVegBakeMarker;
    cleanup();
}

QSize RouteEditorGLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize RouteEditorGLWidget::sizeHint() const {
    return QSize(1000, 700);
}

void RouteEditorGLWidget::cleanup() {
    makeCurrent();
    //delete gluu->m_program;
    //gluu->m_program = 0;
    doneCurrent();
}

void RouteEditorGLWidget::timerEvent(QTimerEvent *) {
    // The OpenGL widget and its timer persist at Main Load. During a restored
    // route load, progress processing can dispatch this timer after the Route
    // pointer is assigned but before loading and camera initialization finish.
    if(route == NULL || !route->loaded || camera == NULL)
        return;
    Game::currentShapeLib = currentShapeLib;
    timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeNow - lastTime < 1)
        fps = 1;
    else
        fps = 1000.0 / (timeNow - lastTime);
    if (fps < 10) fps = 10;

    if (timeNow % 200 < lastTime % 200) {
        //qDebug() << "new second" << timeNow;
        if (selectedObj != NULL)
        {
          emit updateProperties(selectedObj);

            QString selectedType = "";
            Game::objSelected = true;
            if(selectedObj->typeObj == GameObj::terrainobj) {
                selectedType = "Terrain";
            } else if(selectedObj->typeObj == GameObj::tritemobj) {
                TRitem* trItem = (TRitem*)selectedObj;
                if(trItem->tdbId == 1)
                    selectedType = "Road";
                else if(trItem->tdbId == 0)
                    selectedType = "Track";
                else
                    selectedType = "Track Item";
            } else if(selectedObj->typeObj == GameObj::activityobj) {
                selectedType = "Activity";
            } else if(selectedObj->typeObj == GameObj::consistobj) {
                selectedType = "Consist";
            } else if(selectedObj->typeObj == GameObj::worldobj) {
                WorldObj* worldObj = (WorldObj*)selectedObj;
                if(worldObj->typeID == WorldObj::sstatic)
                    selectedType = PolyVegObject::labelForShape(worldObj->fileName);
                else if(worldObj->typeID == WorldObj::trackobj || worldObj->typeID == WorldObj::dyntrack)
                    selectedType = "Track";
                else if(worldObj->typeID == WorldObj::platform || worldObj->typeID == WorldObj::siding ||
                        worldObj->typeID == WorldObj::carspawner || worldObj->typeID == WorldObj::pickup ||
                        worldObj->typeID == WorldObj::levelcr || worldObj->typeID == WorldObj::hazard)
                    selectedType = "Interactive";
                else if(worldObj->typeID == WorldObj::signal || worldObj->typeID == WorldObj::speedpost)
                    selectedType = "Track Item";
                else if(worldObj->typeID == WorldObj::forest)
                    selectedType = "Forest";
                else if(worldObj->typeID == WorldObj::transfer)
                    selectedType = "Transfer";
                else if(worldObj->typeID == WorldObj::soundsource || worldObj->typeID == WorldObj::soundregion)
                    selectedType = "Sound";
                else if(worldObj->typeID == WorldObj::ruler)
                    selectedType = "Ruler";
                else if(worldObj->typeID == WorldObj::groupobject)
                    selectedType = "Group";
                else
                    selectedType = "World Object";
            } else {
                selectedType = selectedObj->getName();
            }
            emit updStatus(QString("object"), selectedType);
        } else {
            Game::objSelected = false;
            emit updStatus(QString("object"), QString(""));
        }
        Undo::StateEndIfLongTime();
    }

    if (timeNow % 100 < lastTime % 100) {
        //qDebug() << "new second" << timeNow;
        if(Game::serverClient != NULL){
            Game::serverClient->updatePointerPosition((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);

        }


    }
    if (timeNow % 250 < lastTime % 250) {

        gluu->setMatrixUniforms();

       //// This is a fake signal mechanism using a Game value
       if(Game::resetTools == true)
       {
           // qDebug() << " fake signal ";
            emit enableTool("selectTool");
            Game::resetTools = false;
       }

        /// try to send camera status every half second or so?
        if (Game::lockCamera) emit updStatus(QString("camera"), QString("Camera LOCK")); else emit updStatus(QString("camera"), QString("Camera FREE"));
        if (Game::cameraStickToTerrain) emit updStatus(QString("camterr"), QString("Cam Terrain LOCK")); else emit updStatus(QString("camterr"), QString("Cam Terrain FREE"));

        if(autoAddToTDB == true) emit updStatus(QString("autotdb"), QString("AutoTDB: ON")); else emit updStatus(QString("autotdb"), QString("AutoTDB: OFF"));  /// EFO Added to
        if(Game::writeTDB == false) emit updStatus(QString("autotdb"), QString("WriteTDB: OFF"));   /// EFO Added to
        if(stickPointerToTerrain == true) emit updStatus(QString("stickterr"), QString("StickToTerrain: ON")); else emit updStatus(QString("stickterr"), QString("StickToTerrain: OFF"));  /// EFO Added to
        if(resizeTool == true) emit updStatus(QString("resize"), QString("Resize: ON")); else emit updStatus(QString("resize"), QString("Resize: OFF"));  /// EFO Added to
        if(translateTool == true) emit updStatus(QString("translate"), QString("Translate: ON")); else emit updStatus(QString("translate"), QString("Translate: OFF"));  /// EFO Added to
        if(rotateTool == true) emit updStatus(QString("rotate"), QString("Rotate: ON")); else emit updStatus(QString("rotate"), QString("Rotate: OFF"));  /// EFO Added to

        if(toolEnabled == "placeTool") emit updStatus(QString("place"), QString("Place: ON")); else emit updStatus(QString("place"), QString("Place: OFF"));  /// EFO Added to
        if(toolEnabled == "selectTool") emit updStatus(QString("select"), QString("Select: ON")); else emit updStatus(QString("select"), QString("Select: OFF"));  /// EFO Added to
        if(placeGuardEnabled) emit updStatus(QString("guard"), QString("Place Guard: ON")); else emit updStatus(QString("guard"), QString("Place Guard: OFF"));

        if(toolEnabled == "heightTool") {
            if(defaultPaintBrush->direction == 1) emit updStatus(QString("brushdir"), QString("Terrain Brush: +")); else emit updStatus(QString("brushdir"), QString("Terrain Brush: -"));  /// EFO Added to
        } else {
            emit updStatus(QString("brushdir"), QString("Terrain Brush: OFF"));
        }

        /// Try to capture player cam rotation


        //        if(resizeTool == true)  reloadRefFile updStatus(QString("resize"), QString("Resize: ON")); else emit updStatus(QString("resize"), QString("Resize: OFF"));  /// EFO Added to
        //        emit updStatus(QString("Stat3"), QString(""));
    }

    if (timeNow % 15000 < lastTime % 15000) {

        unsigned long long int worktime = (timeNow - timeSaved)/60000 ;
        emit updStatus(QString("timer"), QString( QString::number(worktime ))); /// EFO Added to
        emit updStatus(QString("camRot"), QString(QString::number(camera->getRotX())));
    }


    route->updateAnimatedWorld(camera->pozT, (float) (timeNow - lastTime) / 1000.0);

    lastTime = timeNow;

    if (Game::allowObjLag < Game::maxObjLag)
        Game::allowObjLag += 2;

    camera->update(fps);

    update();
}

bool RouteEditorGLWidget::initRoute(){
    MapWindow::loadMapOverlayState();
    // Init Shape and Trains libs
    currentShapeLib = new ShapeLib();
    Game::currentShapeLib = currentShapeLib;
    engLib = new EngLib();
    Game::currentEngLib = engLib;
    timeSaved = QDateTime::currentMSecsSinceEpoch();


    // Init Route
    if(Game::serverClient != NULL){
        if(Game::debugOutput) qDebug() << "RouteClient";
        route = new RouteClient();
        QObject::connect(route, SIGNAL(initDone()), this, SLOT(initRoute2()));
        route->load();
        return true;
    } else {
        route = new Route();
        route->load();
        if (!route->loaded){
            return false;
        }


        qDebug() << "Initializing 3D viewer";
        initRoute2();



        return true;
    }
    return false;
}

void RouteEditorGLWidget::initRoute2(){
    QObject::connect(route, SIGNAL(objectSelected(GameObj*)), this, SLOT(objectSelected(GameObj*)));
    QObject::connect(route, SIGNAL(objectSelected(QVector<GameObj*>)), this, SLOT(objectSelected(QVector<GameObj*>)));
    QObject::connect(route, SIGNAL(sendMsg(QString)), this, SLOT(msg(QString)));

    // Init Camera
    cameraInit();

    // The OpenGL widget persists when the editor returns to Main Load, so
    // initializeGL() runs only for the first route. Refresh marker controls
    // here for every route session, including routes opened after returning.
    emit mkrList(route->getMkrList());
    emit routeLoaded(route);
    emit showWindow();
    // Let the newly shown OpenGL surface present its first complete frame
    // before terrain-palette texture decoding uses the GUI thread.
    QTimer::singleShot(100, this, [this](){ emit preloadTexturesSignal(); });

    return;
}

void RouteEditorGLWidget::cameraInit(){
    float * aaa = new float[2] { 0, 0 };
    cameraFree = new CameraFree(aaa);
    //cameraObj = new CameraConsist();
    camera = cameraFree;
    float spos[3];
    if (Game::start == 2) {
        camera->setPozT(Game::startTileX, -Game::startTileY);
    } else {
        camera->setPozT(route->getStartTileX(), -route->getStartTileZ());
        spos[0] = route->getStartpX();
        spos[2] = -route->getStartpZ();
    }
    if (Game::terrainLib->load(route->getStartTileX(), -route->getStartTileZ())) {
        spos[1] = 20 + Game::terrainLib->getHeight(route->getStartTileX(), -route->getStartTileZ(), route->getStartpX(), -route->getStartpZ());
    } else {
        spos[1] = 0;
    }
    camera->setPos((float*) &spos);
    if(Game::restoreLastSessionCamera){
        camera->setPozT(Game::restoreCameraTileX, Game::restoreCameraTileZ);
        camera->setPos(Game::restoreCameraX, Game::restoreCameraY, Game::restoreCameraZ);
        camera->setPlayerRot(Game::restoreCameraRotX, Game::restoreCameraRotY);
        Game::terrainLib->load(Game::restoreCameraTileX, Game::restoreCameraTileZ);
        Game::restoreLastSessionCamera = false;
    }
}

QJsonObject RouteEditorGLWidget::getSessionCameraState() const{
    QJsonObject state;
    if(camera == NULL)
        return state;

    float* pos = camera->getPos();
    state["tileX"] = (int)camera->pozT[0];
    state["tileZ"] = (int)camera->pozT[1];
    state["x"] = pos[0];
    state["y"] = pos[1];
    state["z"] = pos[2];
    state["rotX"] = camera->getRotX();
    state["rotY"] = camera->getRotY();
    return state;
}

void RouteEditorGLWidget::initializeGL() {

    gluu = GLUU::get();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &RouteEditorGLWidget::cleanup);
    if(Game::debugOutput) qDebug() << "# InitializeOpenGLFunctions";

    initializeOpenGLFunctions();

    Game::currentRenderer = new OpenGL3Renderer();

    //funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
    //if (!funcs) {
    //    qWarning() << "Could not obtain required OpenGL context version";
    //    exit(1);
    //}
    //funcs->initializeOpenGLFunctions();/**/
    glClearColor(0, 0, 0, 1);
    //qDebug() << "gluu->initShader();";
    if(Game::debugOutput) qDebug() << "# InitShaders";
    gluu->initShader();
    if(Game::debugOutput) qDebug() << "# InitShaders finished";
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glLineWidth(Game::oglDefaultLineWidth);

    //sFile = new SFile("F:/TrainSim/trains/trainset/pkp_sp47/pkp_sp47-001.s", "F:/TrainSim/trains/trainset/pkp_sp47");
    //sFile = new SFile("f:/train simulator/routes/cmk/shapes/cottage3.s", "cottage3.s", "f:/train simulator/routes/cmk/textures");
    //eng = new Eng("F:/Train Simulator/trains/trainset/PKP-ST44-992/","PKP-ST44-992.eng",0);
    //sFile->Load("f:/train simulator/routes/cmk/shapes/cottage3.s");
    //tile = new Tile(-5303,-14963);
    //qDebug() << "route = new Route();";


    lastTime = QDateTime::currentMSecsSinceEpoch();


    int timerStep = 15;
    if (Game::fpsLimit > 0)
        timerStep = 1000 / Game::fpsLimit;
    timer.start(timerStep, this);
    setFocus();
    setMouseTracking(true);
    pointer3d = new Pointer3d();
    compass = new GuiGlCompass();
    compassPointer = new OglObj();
    float *punkty = new float[3 * 6];
    int ptr = 0;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.975;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.96;
    punkty[ptr++] = 0;
    compassPointer->setLineWidth(2);
    compassPointer->setMaterial(0.0, 0.0, 0.0);
    compassPointer->init(punkty, ptr, RenderItem::V, GL_LINES);
    delete[] punkty;

    //selectedObj = NULL;
    setSelectedObj(NULL);
    groupObj = new GroupObj();
    copyPasteGroupObj = new GroupObj();
    defaultPaintBrush = new Brush();
    mapWindow = new MapWindow(this);
    Quat::fill(this->placeRot);

    gluu->makeShadowFramebuffer(FramebufferName1, depthTexture1, Game::shadowMapSize, GL_TEXTURE2);
    gluu->makeShadowFramebuffer(FramebufferName2, depthTexture2, Game::shadowLowMapSize, GL_TEXTURE3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);


    moveStep = Game::DefaultMoveStep;
    moveMaxStep = Game::DefaultMoveStep;
    defaultMoveStep = moveStep;
}

void RouteEditorGLWidget::reloadRefFile(){
    route->loadAddons();
    //route->ref = new Ref((Game::root + "/routes/" + Game::route + "/" + Game::routeName + ".ref"));
    emit refreshObjLists();
}

void RouteEditorGLWidget::reloadMkrFiles(){
    if(Game::debugOutput) qDebug() << "route->loadMkrList;";
    route->loadMkrList();
    if(Game::debugOutput) qDebug() << "REGL->emit mkrList(route->getMkrList())";
    emit mkrList(route->getMkrList());
}

void RouteEditorGLWidget::setCameraObject(GameObj* obj){
    camera->setCameraObject(obj);
}

void RouteEditorGLWidget::setMoveStep(float val){
    moveStep = val;
    moveMaxStep = val;
}

void RouteEditorGLWidget::paintGL(){
    paintGL2();
    return;

    // Here is not finishes, future version of TSRE renderer.
    // Unlike old TSRE renderer, here collect all render items first
    // And then use renderer to render them

    Game::currentShapeLib = currentShapeLib;
    if (route == NULL) return;
    if (!route->loaded) return;

    // Render Shadows
    //if (Game::shadowsEnabled > 0)
    //    renderShadowMaps();

    // Render Scene
    //gluu->currentShader = gluu->shaders["StandardBloom"];
    gluu->currentShader = gluu->shaders["StandardFog"];
    gluu->currentShader->bind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int renderMode = GLUU::RENDER_DEFAULT;
    if (selection)
        renderMode = GLUU::RENDER_SELECTION;

    glClearColor(gluu->skyColor[0], gluu->skyColor[1], gluu->skyColor[2], 1.0);
    glViewport(0, 0, (float) this->width() * Game::PixelRatio, (float) this->height() * Game::PixelRatio);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);

    Mat4::perspective(gluu->fMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->fMatrix, gluu->fMatrix, camera->getMatrix());

    // Render Skydome
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 100.0f, 10000.0f);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, camera->getPos());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, -50, 0);
    Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, 2.0, 0, 1, 0);
    gluu->setMatrixUniforms();
    gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
    //route->skydome->render(gluu, renderMode);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render Low Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 600.0f, Game::distantLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    //gluu->currentShader->setUniformValue(gluu->currentShader->lod, -0.5f);
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, route->getDistantTerrainYOffset(), 0);
    //Game::terrainLib->renderLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    //for(int i = 0; i < route->env->waterCount; i++)
    //    Game::terrainLib->renderWaterLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render High Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    Game::terrainLib->pushRenderItems(camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);

    // Render World
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();

    if (stickPointerToTerrain && Game::viewTerrainShape)

        if (!selection) pushRenderPointer();

    route->pushRenderItems(camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, renderMode);
    if(!selection)
        pushPolyVegBakeMarkers();
    //if (!selection)
    //for(int i = 0; i < route->env->waterCount; i++)
    //    Game::terrainLib->renderWater(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);

    //if (!stickPointerToTerrain || !Game::viewTerrainShape)
    //    if (!selection) pushRenderPointer();

    // render compass
    /*if (!selection && Game::viewCompass){
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, 1.0, 1.0 - 2*(float(this->height()) / this->width()), 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);

        compass->pushRenderItem(camera->getRotX()+M_PI);
        compassPointer->pushRenderItem();
    }*/


    Game::currentRenderer->renderFrame();
    // Handle Selection
    //handleSelection();


    // Set Info
    //if (this->isActiveWindow()) {
    //    emit this->naviInfo(route->getTileObjCount((int) camera->pozT[0], (int) camera->pozT[1]), route->getTileHiddenObjCount((int) camera->pozT[0], (int) camera->pozT[1]));
    //    emit this->posInfo(camera->getCurrentPos());
    //    emit this->pointerInfo(aktPointerPos);
    //}
}

void RouteEditorGLWidget::paintGL2() {
    Game::currentShapeLib = currentShapeLib;
    if (route == NULL) return;
    if (!route->loaded) return;

    // Render Shadows
    if (Game::shadowsEnabled > 0)
       renderShadowMaps();

    // Render Scene
    //gluu->currentShader = gluu->shaders["StandardBloom"];
    gluu->currentShader = gluu->shaders["StandardFog"];
    gluu->currentShader->bind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int renderMode = GLUU::RENDER_DEFAULT;
    if (selection)
        renderMode = GLUU::RENDER_SELECTION;
    gluu->currentShader->setUniformValue(
        gluu->currentShader->shaderSelectionPass,
        renderMode == GLUU::RENDER_SELECTION ? 1.0f : 0.0f);

    glClearColor(gluu->skyColor[0], gluu->skyColor[1], gluu->skyColor[2], 1.0);
    glViewport(0, 0, (float) this->width() * Game::PixelRatio, (float) this->height() * Game::PixelRatio);
    Mat4::identity(gluu->mvMatrix);

    Mat4::perspective(gluu->fMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->fMatrix, gluu->fMatrix, camera->getMatrix());

    // Render Skydome
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 100.0f, 10000.0f);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, camera->getPos());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, -50, 0);
    Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, 2.0, 0, 1, 0);
    gluu->setMatrixUniforms();
    gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
    route->skydome->render(gluu, renderMode);
    Mat4::identity(gluu->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render Low Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 600.0f, Game::distantLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    //gluu->currentShader->setUniformValue(gluu->currentShader->lod, -0.5f);
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, route->getDistantTerrainYOffset(), 0);
    Game::terrainLib->renderLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    // Water and shallow terrain can converge to the same depth value as the
    // camera pulls back. Bias only the rendered water toward the camera; saved
    // elevations and terrain geometry remain unchanged.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    for(int i = 0; i < route->env->waterCount; i++)
        Game::terrainLib->renderWaterLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);
    glDisable(GL_POLYGON_OFFSET_FILL);
    Mat4::identity(gluu->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render High Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    Game::terrainLib->render(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    //glClear(GL_DEPTH_BUFFER_BIT);
    // Render World
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();

    if (stickPointerToTerrain && Game::viewTerrainShape)
        if (!selection) drawPointer();

    route->render(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, renderMode);
    renderPolyVegBakeMarkers();

    //if (!selection)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    for(int i = 0; i < route->env->waterCount; i++)
        Game::terrainLib->renderWater(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);
    glDisable(GL_POLYGON_OFFSET_FILL);

    if (!stickPointerToTerrain || !Game::viewTerrainShape)
        if (!selection) drawPointer();

    // render compass
    if (!selection && Game::viewCompass){
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, 1.0, 1.0 - 2*(float(this->height()) / this->width()), 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);

        compass->render(static_cast<float>(camera->getRotX() + M_PI));
        compassPointer->render();
    }


    // Handle Selection
    handleSelection();

    // Set Info
    if (this->isActiveWindow() && timeNow - lastDisplayInfoUpdate >= 100) {
        lastDisplayInfoUpdate = timeNow;
        emit this->naviInfo(route->getTileObjCount((int) camera->pozT[0], (int) camera->pozT[1]), route->getTileHiddenObjCount((int) camera->pozT[0], (int) camera->pozT[1]));
        emit this->posInfo(camera->getCurrentPos());
        emit this->pointerInfo(aktPointerPos);

    }
}

void RouteEditorGLWidget::renderShadowMaps() {
    float* lookAt = Mat4::create();
    float* out1 = Vec3::create();
    Vec3::set(out1, 0, 1, 0);
    float *ld = Vec3::create();
    Vec3::set(ld, -1.0, 1.5, 1.0);
    float *aaa = camera->getPos();
    //float *lt = camera->getTarget();
    //Vec3::sub(lt, lt, aaa);
    //lt[0] = lt[0]*100;
    //lt[1] = 0;
    //lt[2] = lt[2]*100;
    //Vec3::add(aaa, aaa, lt);
    //aaa[2] = -aaa[2];
    //aaa[0] = -aaa[0];
    Vec3::add(ld, ld, aaa);
    Mat4::ortho(gluu->pShadowMatrix, -150, 150, -150, 150, -200, 200);
    Mat4::ortho(gluu->pShadowMatrix2, -700, 700, -700, 700, -700, 700);
    Mat4::lookAt(lookAt, ld, aaa, out1);
    Mat4::multiply(gluu->pShadowMatrix, gluu->pShadowMatrix, lookAt);
    Mat4::multiply(gluu->pShadowMatrix2, gluu->pShadowMatrix2, lookAt);

    gluu->currentShader = gluu->shaders["Shadows"];
    gluu->currentShader->bind();
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(gluu->objStrMatrix);
    gluu->setMatrixUniforms();
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName1);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Game::shadowMapSize, Game::shadowMapSize);
    int tempLod = Game::objectLod;
    Game::objectLod = 600;
    Game::terrainLib->renderEmpty(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3);
    route->renderShadowMap(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, selection);

    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(gluu->objStrMatrix);
    float *tmatrix = gluu->pShadowMatrix;
    gluu->pShadowMatrix = gluu->pShadowMatrix2;
    gluu->setMatrixUniforms();
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName2);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Game::shadowLowMapSize, Game::shadowLowMapSize);
    Game::objectLod = 1000;
    route->renderShadowMap(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, selection);
    gluu->pShadowMatrix2 = gluu->pShadowMatrix;
    gluu->pShadowMatrix = tmatrix;
    Game::objectLod = tempLod;
    gluu->currentShader->release();
}

void RouteEditorGLWidget::handleSelection() {
    if (selection) {
        int x = mousex;
        int y = mousey;

        float winZ[4];

        int viewport[4];
        float mvmatrix[16];
        float projmatrix[16];

        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
        glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);
        int realy = viewport[3] - (int) y - 1;
        glReadPixels(x, realy, 1, 1, GL_RGBA, GL_FLOAT, &winZ);

        // if(Game::debugOutput) qDebug() << "REGLW676:" << winZ[0] << " " << winZ[1] << " " << winZ[2] << " " << winZ[3];
        int colorHash = (int) (winZ[0]*255)*256 * 256 + (int) (winZ[1]*255)*256 + (int) (winZ[2]*255);
        // if(Game::debugOutput) qDebug() << "REGLW678:" << colorHash;
        int ww = (colorHash >> 20) & 0xF;
        // if(Game::debugOutput) qDebug() << "REGLW680:" << "ww"<< ww;

        // WorldObj Selected
        if(ww == 0){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB){
                    route->addToTDBIfNotExist((WorldObj*)selectedObj);
                }
                if(Game::debugOutput) qDebug() << "REGLW 687";
                setSelectedObj(NULL);
            }
        } else if( ww >= 1 && ww <= 9 ){
            int UiD = (colorHash >> 4) & 0xFFFF;
            //if(UiD >= 50000)
            //    UiD += 50000;
            int cdata = colorHash & 0xF;
            int wx = 0;
            int wz = 0;
            if (ww == 1 || ww == 2 || ww == 3) wx = camera->pozT[0] - 1;
            if (ww == 4 || ww == 5 || ww == 6) wx = camera->pozT[0];
            if (ww == 7 || ww == 8 || ww == 9) wx = camera->pozT[0] + 1;
            if (ww == 1 || ww == 4 || ww == 7) wz = camera->pozT[1] - 1;
            if (ww == 2 || ww == 5 || ww == 8) wz = camera->pozT[1];
            if (ww == 3 || ww == 6 || ww == 9) wz = camera->pozT[1] + 1;
            // if(Game::debugOutput) qDebug() << "REGLW703:" << "color data: " << cdata;
            // if(Game::debugOutput) qDebug() << "REGLW704:" << wx << " " << wz << " " << UiD;
            WorldObj *selectedWorldObj = (WorldObj*) selectedObj;
            if (keyControlEnabled) {
                if (selectedWorldObj == NULL){
                    setSelectedObj(groupObj);
                    selectedWorldObj = (WorldObj*) selectedObj;
                } else if (selectedWorldObj->typeObj != GameObj::worldobj){
                    selectedWorldObj->unselect();
                    setSelectedObj(groupObj);
                } else if (selectedWorldObj->typeObj == GameObj::worldobj) {
                    groupObj->addObject(selectedWorldObj);
                    setSelectedObj(groupObj);
                }
                groupObj->addObject(route->getObj(wx, wz, UiD));
                if (groupObj->count() == 0) {
                    if(Game::debugOutput) qDebug() << "brak obiektu";
                    groupObj->unselect();
                    setSelectedObj(NULL);
                }
            } else {
                WorldObj* twobj = route->getObj(wx, wz, UiD);
                if(twobj != NULL && twobj->typeID == WorldObj::sstatic
                        && PolyVegObject::isBakeShape(twobj->fileName)) {
                    selectPolyVegBakeTile(wx, wz);
                } else {
                  if (selectedWorldObj != NULL && twobj != selectedWorldObj) {
                    selectedWorldObj->unselect();
                    if (autoAddToTDB) {
                        route->addToTDBIfNotExist(selectedWorldObj); if(Game::debugOutput) qDebug() << "REGLW 728";
                    }
                }
                lastSelectedObj = selectedObj;
                RulerObj *clickedRuler = dynamic_cast<RulerObj*>(twobj);
                const bool refreshObjectProperties = clickedRuler == NULL
                        || (!clickedRuler->isWaterRuler()
                            && !clickedRuler->isVegetationRuler());
                setSelectedObj(twobj, refreshObjectProperties);
                if (selectedObj == NULL) {
                    if(Game::debugOutput) qDebug() << "brak obiektu";
                } else {
                    selectedObj->select(cdata);
                    RulerObj *selectedRuler = dynamic_cast<RulerObj*>(selectedObj);
                    if(selectedRuler != NULL && selectedRuler->isSpecialRuler()){
                        int pointerTileX = (int)camera->pozT[0];
                        int pointerTileZ = (int)camera->pozT[1];
                        float pointerPosition[3];
                        Vec3::copy(pointerPosition, aktPointerPos);
                        Game::check_coords(pointerTileX, pointerTileZ, pointerPosition);
                        selectedRuler->selectSpecialPoint(
                            cdata, pointerTileX, pointerTileZ, pointerPosition);
                    }
                  }
                }
            }
        } else if(ww == RulerObj::SpecialSelectionWindow){
            const int rulerKind =
                    (colorHash >> RulerObj::SpecialSelectionKindShift) & 0x3;
            const int pointIndex =
                    colorHash & RulerObj::SpecialSelectionPointMask;
            RulerObj *clickedRuler = NULL;
            if(rulerKind == RulerObj::WaterSelection){
                if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
                    activeWaterRuler = route->findWaterRuler(false);
                clickedRuler = activeWaterRuler;
            } else if(rulerKind == RulerObj::VegetationSelection){
                if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
                    activeVegetationRuler = route->findVegetationRuler(false);
                clickedRuler = activeVegetationRuler;
            } else if(rulerKind == RulerObj::GradeSelection){
                if(activeGradeRuler == NULL || !activeGradeRuler->loaded)
                    activeGradeRuler = route->findGradeRuler(false);
                clickedRuler = activeGradeRuler;
            }
            if(clickedRuler != NULL
                    && pointIndex < clickedRuler->getPointCount()){
                if(selectedObj != NULL && selectedObj != clickedRuler)
                    selectedObj->unselect();
                lastSelectedObj = selectedObj;
                setSelectedObj(clickedRuler, false);
                clickedRuler->select(pointIndex);
            }
        } else if( ww == 10 ){
            int wx = camera->pozT[0] - 1 + ((colorHash >> 10) & 0x3);
            int wz = camera->pozT[1] - 1 + ((colorHash >> 8) & 0x3);
            int UiD = (colorHash) & 0xFF;
            // if(Game::debugOutput) qDebug() << "REGLW743:" << wx << wz << UiD;
            if (selectedObj != NULL) {
                if ((keyControlEnabled || keyShiftEnabled) && selectedObj->typeObj == GameObj::terrainobj ) {
                    Terrain * tt = (Terrain*) selectedObj;
                    if(!tt->isXYinside(wx, wz)){// >mojex != wx || tt->mojez != wz){
                        selectedObj->unselect();
                        setSelectedObj(NULL);
                    }
                } else {
                    selectedObj->unselect();
                    if (autoAddToTDB)
                        route->addToTDBIfNotExist((WorldObj*)selectedObj); // if(Game::debugOutput) qDebug() << "REGLW 754";
                    setSelectedObj(NULL);
                }
            }
            Terrain *t = Game::terrainLib->getTerrainByXY(wx, wz);
            if (t == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                t->select(UiD, keyControlEnabled);
            }
            setSelectedObj((GameObj*)t);
        } else if( ww == 11 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB){
                    route->addToTDBIfNotExist((WorldObj*)selectedObj);
                }
                if(Game::debugOutput) qDebug() << "REGLW 769";
                setSelectedObj(NULL);
            }
            int CID = ((colorHash) >> 8) & 0xFFF;
            int EID = ((colorHash)) & 0xFF;
            if(Game::debugOutput) qDebug() << "REGLW 774:"  << CID << EID;
            setSelectedObj((GameObj*)route->getActivityObject(CID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                //qDebug() << "eid"<<EID;
                selectedObj->select(EID);
                setSelectedObj(selectedObj);
            }
        } else if( ww == 12 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB){
                    route->addToTDBIfNotExist((WorldObj*)selectedObj);
                }
                if(Game::debugOutput) qDebug() << "REGLW 787";
                setSelectedObj(NULL);
            }
            int TID = ((colorHash) >> 19) & 0x1;
            int UID = ((colorHash)) & 0xFFFF;
            if(Game::debugOutput) qDebug() << "REGL 792:" << TID << UID;
            setSelectedObj((GameObj*)route->getTrackItem(TID, UID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                selectedObj->select();
            }
        } else if( ww == 13 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB){
                    route->addToTDBIfNotExist((WorldObj*)selectedObj);
                }
                if(Game::debugOutput) qDebug() << "REGLW 803";
                setSelectedObj(NULL);
            }
            int CID = ((colorHash) >> 8) & 0xFFF;
            int EID = ((colorHash)) & 0xFF;
            if(Game::debugOutput) qDebug() << "REGLW808:" << CID << EID;
            setSelectedObj((GameObj*)route->getActivityConsist(CID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                //qDebug() << "eid"<<EID;
                selectedObj->select(EID);
                setSelectedObj(selectedObj);
            }
        } else {
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB){
                    route->addToTDBIfNotExist((WorldObj*)selectedObj);
                }
                if(Game::debugOutput) qDebug() << "REGLW 821";
                setSelectedObj(NULL);
            }
        }

        //qDebug() << "selection" << selection;
        selection = false;// !selection;
        paintGL();
    }
}

void RouteEditorGLWidget::pushRenderPointer() {

    int x = mousex;
    int y = mousey;

    static unsigned long long int oldTime = 0;
    unsigned long long int newTime = QDateTime::currentMSecsSinceEpoch();
    static float winZ[4];
    int viewport[4];
    //float wcoord[4];


    glGetIntegerv(GL_VIEWPORT, viewport);
    //glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
    //glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);
    int realy = viewport[3] - (int) y - 1;
    if(newTime - oldTime > 200){
        glReadPixels(x, realy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        oldTime = newTime;
    }
    GLH::glhUnProjectf((float) x, (float) realy, winZ[0], //
            gluu->mvMatrix,
            gluu->pMatrix,
            viewport,
            aktPointerPos);
    return;
    //qDebug()<<aktPointerPos[0]<< aktPointerPos[1]<< aktPointerPos[2];
    /*if (Game::viewPointer3d) {
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        pointer3d->render();
        gluu->mvPopMatrix();
    }*/
}

void RouteEditorGLWidget::drawPointer() {
    int x = mousex;
    int y = mousey;

    static float winZ[4];
    int viewport[4] = {0, 0,
        (int)(this->width() * Game::PixelRatio),
        (int)(this->height() * Game::PixelRatio)};
    //float wcoord[4];



    int realy = viewport[3] - (int) y - 1;
    float *cameraPos = camera->getPos();
    float *cameraTarget = camera->getTarget();
    const bool pointerInputChanged = !pointerDepthValid
            || pointerDepthMouseX != x || pointerDepthMouseY != y
            || pointerDepthTileX != (int)camera->pozT[0]
            || pointerDepthTileZ != (int)camera->pozT[1]
            || pointerDepthCameraPos[0] != cameraPos[0]
            || pointerDepthCameraPos[1] != cameraPos[1]
            || pointerDepthCameraPos[2] != cameraPos[2]
            || pointerDepthCameraTarget[0] != cameraTarget[0]
            || pointerDepthCameraTarget[1] != cameraTarget[1]
            || pointerDepthCameraTarget[2] != cameraTarget[2];
    const unsigned long long int newTime = QDateTime::currentMSecsSinceEpoch();
    if(pointerInputChanged && newTime - lastPointerDepthRead >= 50){
        glReadPixels(x, realy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        lastPointerDepthRead = newTime;
        pointerDepthMouseX = x;
        pointerDepthMouseY = y;
        pointerDepthTileX = (int)camera->pozT[0];
        pointerDepthTileZ = (int)camera->pozT[1];
        pointerDepthCameraPos[0] = cameraPos[0];
        pointerDepthCameraPos[1] = cameraPos[1];
        pointerDepthCameraPos[2] = cameraPos[2];
        pointerDepthCameraTarget[0] = cameraTarget[0];
        pointerDepthCameraTarget[1] = cameraTarget[1];
        pointerDepthCameraTarget[2] = cameraTarget[2];
        pointerDepthValid = true;
    }
    GLH::glhUnProjectf((float) x, (float) realy, winZ[0], //
            gluu->mvMatrix,
            gluu->pMatrix,
            viewport,
            aktPointerPos);
    //qDebug()<<aktPointerPos[0]<< aktPointerPos[1]<< aktPointerPos[2];
    if (Game::viewPointer3d) {
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        pointer3d->render();
        gluu->mvPopMatrix();

        if(Game::serverClient != NULL){
            foreach(ClientInfo *info, Game::serverClient->clientUsersList){
                if(info == NULL)
                    continue;
                if(info->username == Game::serverClient->username)
                    continue;
                gluu->mvPushMatrix();
                //qDebug() << camera->pozT[0] << info->X;
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048*(info->X-camera->pozT[0])+info->x, info->y, 2048*(info->Z-camera->pozT[1])+info->z);
                Mat4::identity(gluu->objStrMatrix);
                gluu->setMatrixUniforms();
                //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                info->render(camera->getRotX());
                gluu->mvPopMatrix();
            }
        }
    }
}

bool RouteEditorGLWidget::pointerNearPlacementDb(Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ) {
    if (item == NULL || pointerPos == NULL)
        return true;

    if (WorldObj::isTrackObj(item->type) == 0)
        return true;

    float sample[3];
    Vec3::copy(sample, pointerPos);
    float dbHeight = 0;
    return route->findNearestDbHeight(pointerTileX, pointerTileZ, sample, 3.0f, dbHeight);
}

bool RouteEditorGLWidget::validatePlacement(WorldObj* obj, Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ) {
    if (!placeGuardEnabled || obj == NULL || camera == NULL)
        return true;

    if (qAbs(obj->x - (int)camera->pozT[0]) > 1 || qAbs(obj->y - (int)camera->pozT[1]) > 1)
        return false;

    if (!pointerNearPlacementDb(item, pointerPos, pointerTileX, pointerTileZ))
        return false;

    if (item != NULL && WorldObj::isTrackObj(item->type) != 0) {
        float sample[3];
        Vec3::copy(sample, obj->position);
        float dbHeight = 0;
        if (!route->findNearestDbHeight(obj->x, obj->y, sample, 10.0f, dbHeight))
            return false;
        return qAbs(obj->position[1] - dbHeight) <= 10.0f;
    }

    Terrain *terrain = Game::terrainLib->getTerrainByXY(obj->x, obj->y, false);
    if (terrain == NULL || !terrain->loaded)
        return false;

    float ground = Game::terrainLib->getHeight(obj->x, obj->y, obj->position[0], obj->position[2]);
    float delta = obj->position[1] - ground;

    if (obj->typeID == WorldObj::trackobj || obj->typeID == WorldObj::dyntrack)
        return delta <= 100.0f && delta >= -50.0f;

    return delta <= 1.0f && delta >= -1.0f;
}

void RouteEditorGLWidget::rejectPlacement() {
    if (selectedObj != NULL)
        selectedObj->unselect();

    setSelectedObj(NULL);
    Undo::StateEnd();
    Undo::UndoLast();
    mouseLPressed = false;
    showPlacementGuardError();
}

void RouteEditorGLWidget::playPlacementSound(QString fileName) {
    if(!Game::scoSoundEnabled)
        return;
#ifdef Q_OS_WIN
    QString soundPath = QCoreApplication::applicationDirPath() + "/content/" + fileName;
    if (QFile::exists(soundPath)) {
        ::PlaySoundW(NULL, NULL, 0);
        ::PlaySoundW(reinterpret_cast<const wchar_t*>(soundPath.utf16()), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
#else
    Q_UNUSED(fileName);
#endif
}

void RouteEditorGLWidget::queuePolyVegSuccessSound() {
    QTimer::singleShot(0, this, [this](){
        playPlacementSound("SCOsuccess.wav");
    });
}

float RouteEditorGLWidget::trackGradePercent(GameObj *obj) const {
    if(obj == NULL || obj->typeObj != GameObj::worldobj)
        return 1000000.0f;
    WorldObj *world = (WorldObj*)obj;
    if(world->typeID != WorldObj::trackobj)
        return 1000000.0f;
    return qTan(((TrackObj*)world)->getElevation()) * 100.0f;
}

bool RouteEditorGLWidget::applyGradeLockToPlacedTrack(bool previousTrackValid, int previousX, int previousY,
                                                      unsigned int previousUid, float previousGrade,
                                                      bool &gradeAchieved) {
    gradeAchieved = false;
    float placedGrade = trackGradePercent(selectedObj);
    if(placedGrade > 999999.0f)
        return !Game::gradeAssistEnabled;

    TrackObj *placedTrack = (TrackObj*)selectedObj;
    if(Game::gradeAssistEnabled){
        const bool gradeMatches = previousTrackValid
                && qAbs(previousGrade - Game::gradeAssistCurrentPercent) <= 0.002f;
        const bool connectionMatches = route != NULL
                && route->placementEndpointBelongsToTrack(placedTrack,
                                                          previousX, previousY, previousUid);
        if(!gradeMatches || !connectionMatches)
            return false;

        const float appliedGrade = Game::gradeAssistNextPercent;
        placedTrack->setElevation(appliedGrade * 10.0f);
        Game::gradeAssistCurrentPercent = appliedGrade;

        const float remaining = Game::gradeAssistTargetPercent - Game::gradeAssistCurrentPercent;
        if(qAbs(remaining) <= 0.0005f){
            Game::gradeAssistCurrentPercent = Game::gradeAssistTargetPercent;
            Game::gradeAssistNextPercent = Game::gradeAssistTargetPercent;
            Game::gradeAssistEnabled = false;
            Game::gradeAssistTargetReached = true;
            Game::gradeLockEnabled = true;
            Game::gradeLockedPercent = Game::gradeAssistTargetPercent;
            gradeAchieved = true;
        } else {
            const float increment = qMin(qAbs(remaining), Game::gradeAssistStepPercent);
            Game::gradeAssistNextPercent = Game::gradeAssistCurrentPercent
                    + (remaining > 0.0f ? increment : -increment);
        }
    } else if(Game::gradeLockEnabled){
        placedTrack->setElevation(Game::gradeLockedPercent * 10.0f);
    }
    return true;
}

void RouteEditorGLWidget::showPlacementSuccess() {
    playPlacementSound("SCOprogress.wav");
}

void RouteEditorGLWidget::showModeChange() {
    playPlacementSound("SCOpress.wav");
}

void RouteEditorGLWidget::userPlacementSound() {
    showPlacementSuccess();
}

void RouteEditorGLWidget::userPanelToggleSound() {
    playPlacementSound("SCOtic.wav");
}

void RouteEditorGLWidget::userModeChangeSound() {
    showModeChange();
}

void RouteEditorGLWidget::userErrorSound() {
    playPlacementSound("SCObuzz.wav");
}

void RouteEditorGLWidget::userJumpSound() {
    showModeChange();
    QTimer::singleShot(1000, this, [this](){
        playPlacementSound("SCOchirp.wav");
    });
}

void RouteEditorGLWidget::showPlacementGuardError() {
    playPlacementSound("SCObuzz.wav");
    emit updStatus(QString("guarderror"), QString("ERROR"));
}

void RouteEditorGLWidget::flexResult(bool success) {
    if(success) {
        // Flex changes the generated sections, so rebuild this object's TDB
        // entry before reporting success. Normal track joins use this same
        // database path; leaving the old Dyntrack section cached produces
        // endpoint gaps and duplicate-looking TDB markers.
        if(route != NULL && selectedObj != NULL &&
           selectedObj->typeObj == GameObj::worldobj &&
           ((WorldObj*)selectedObj)->type == "dyntrack") {
            WorldObj* dyntrack = (WorldObj*)selectedObj;
            if(!route->removeTrackFromTDB(dyntrack)) {
                showPlacementGuardError();
                return;
            }
            route->addToTDB(dyntrack);
        }
        showPlacementSuccess();
    } else {
        showPlacementGuardError();
    }
}

void RouteEditorGLWidget::focusEditor() {
    QWidget *editorWindow = window();
    if(editorWindow != NULL)
        editorWindow->activateWindow();
    setFocus(Qt::OtherFocusReason);
}

void RouteEditorGLWidget::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
    positionWaterMessage();
    //gluu->m_proj.setToIdentity();
    //gluu->m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void RouteEditorGLWidget::keyPressEvent(QKeyEvent * event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;

    if(event->key() == Qt::Key_A
            && event->modifiers().testFlag(Qt::AltModifier)){
        selectAllTerrainPatchesOnSelectedTile();
        event->accept();
        return;
    }

    if(event->modifiers() == Qt::NoModifier){
        if(event->isAutoRepeat()){
            switch(event->key()){
                case Qt::Key_E:
                case Qt::Key_Q:
                case Qt::Key_R:
                case Qt::Key_T:
                case Qt::Key_Y:
                case Qt::Key_M:
                    event->accept();
                    return;
                default:
                    break;
            }
        }

        switch(event->key()){
            case Qt::Key_E:
                statusPanelCommand("select");
                event->accept();
                return;
            case Qt::Key_Q:
                statusPanelCommand("place");
                event->accept();
                return;
            case Qt::Key_R:
                statusPanelCommand("rotate");
                event->accept();
                return;
            case Qt::Key_T:
                statusPanelCommand("translate");
                event->accept();
                return;
            case Qt::Key_Y:
                statusPanelCommand("resize");
                event->accept();
                return;
            case Qt::Key_M:
                toggleRouteMapOverlays();
                event->accept();
                return;
            default:
                break;
        }
    }

    camera->keyDown(event);

    Undo::StateBeginIfNotExist();

    // EFO Key events

    switch (event->key()) {
        case Qt::Key_Control:
            moveStep = moveMaxStep / 10.0;
            keyControlEnabled = true;
            break;
        case Qt::Key_Shift:
            keyShiftEnabled = true;
            break;
        case Qt::Key_Alt:
            moveStep = moveMaxStep * 10.0;
            keyAltEnabled = true;
            break;
        case Qt::Key_B:
        {
            if (GuiFunct::confirmDestructiveAction(
                    this, "Create New Tile",
                    "Create a new terrain tile at the current location?")){
                // New Tile != New Terrain. Need fix for distant terrain!
                int out = 0;
                out = route->newTile((int) camera->pozT[0], (int) camera->pozT[1]);
                if(out == 1){
                    if (GuiFunct::confirmDestructiveAction(
                            this, "Overwrite Existing Tile",
                            "A terrain tile already exists at this location.\n\n"
                            "Overwrite it?"))
                        route->newTile((int) camera->pozT[0], (int) camera->pozT[1], true);
                }
            }
        }
            break;
        /// EFO Added to
        case Qt::Key_Escape:
            resizeTool = false;
            translateTool = false;
            rotateTool = false;
            showModeChange();

            break;

        /// EFO Added to
        case Qt::Key_E:
            break;

        case Qt::Key_R:
            break;
        case Qt::Key_T:
            break;
        case Qt::Key_Y:
            break;
        case Qt::Key_Q:
            if (keyControlEnabled)
            {
                autoAddToTDB = !autoAddToTDB;
                showModeChange();
                break;
            }
            else if (keyShiftEnabled)
            {
                stickPointerToTerrain = !stickPointerToTerrain;
                showModeChange();
                break;
            }
            else
            {
            }
            break;
        case Qt::Key_Home:
            aktPointerPos[1] += 40;
            jumpTo(camera->pozT, aktPointerPos);
            aktPointerPos[1] -= 40;
            break;
        default:
            break;
    }
    if (toolEnabled == "heightTool" || toolEnabled == "waterTerrTool" || toolEnabled == "gapsTerrainTool") {
        switch (event->key()) {
            case Qt::Key_Z:
                if (!keyControlEnabled) {
                    this->defaultPaintBrush->direction = -this->defaultPaintBrush->direction;
                    if (this->defaultPaintBrush->direction == 1)
                    {
                        emit sendMsg(QString("brushDirection"), QString("+"));
                    }
                    else
                    {
                        emit sendMsg(QString("brushDirection"), QString("-"));
                    }
                }
                break;
            default:
                break;
        }
    }
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        Vector2f a;

        switch (event->key()) {
            case Qt::Key_Up:
                if (Game::usenNumPad)
                    break;
                [[fallthrough]];
            case Qt::Key_8:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(moveStep, 0, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(moveStep / 10, 0, 0);
                } else if (selectedObj != NULL) {
                    a.y = moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Down:
                if (Game::usenNumPad)
                    break;
                [[fallthrough]];
            case Qt::Key_2:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(-moveStep, 0, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(-moveStep / 10, 0, 0);
                } else if (selectedObj != NULL) {
                    a.y = -moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Left:
                if (Game::usenNumPad)
                    break;
                [[fallthrough]];
            case Qt::Key_4:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, moveStep, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, -moveStep / 10, 0);
                } else if (selectedObj != NULL) {
                    a.x = moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Right:
                if (Game::usenNumPad)
                    break;
                [[fallthrough]];
            case Qt::Key_6:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, -moveStep, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, moveStep / 10, 0);
                } else if (selectedObj != NULL) {
                    a.x = -moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_PageUp:
                //Game::cameraFov += 1;
                //qDebug() << Game::cameraFov;
            case Qt::Key_9:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, 0, moveStep);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, 0, moveStep / 10);
                } else if (selectedObj != NULL) {
                    selectedObj->translate(0, moveStep, 0);
                }
                break;
            case Qt::Key_PageDown:
                //Game::cameraFov -= 1;
                //qDebug() << Game::cameraFov;
            case Qt::Key_3:
            case Qt::Key_7:
                Undo::PushGameObjData(selectedObj);
                if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, 0, -moveStep / 10);
                } else if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, 0, -moveStep);
                } else if (selectedObj != NULL) {
                    selectedObj->translate(0, -moveStep, 0);
                }
                break;
            case Qt::Key_F:
                if(event->modifiers() & Qt::ControlModifier)
                    setTerrainToSelectedObjTile();
                else if(event->modifiers() & Qt::ShiftModifier)
                    smoothTerrainToObj();
                else
                    setTerrainToObj();
                break;
            case Qt::Key_H:
                adjustObjPositionToTerrainMenu();
                break;
            case Qt::Key_N:
                adjustObjRotationToTerrainMenu();
                break;
            case Qt::Key_Delete:
                if (selectedObj != NULL) {
                    if(selectedObj->typeObj == GameObj::worldobj){
                        WorldObj *worldObject = (WorldObj*)selectedObj;
                        bool polyVegBake = worldObject->typeID == WorldObj::sstatic
                            && PolyVegObject::isBakeShape(worldObject->fileName);
                        if(worldObject->typeID == WorldObj::groupobject) {
                            GroupObj *group = static_cast<GroupObj*>(worldObject);
                            for(WorldObj *member : group->objects)
                                polyVegBake = polyVegBake
                                    || (member != NULL
                                        && member->typeID == WorldObj::sstatic
                                        && PolyVegObject::isBakeShape(member->fileName));
                        }
                        route->deleteObj(worldObject);
                        selectedObj->unselect();
                        if(polyVegBake) refreshPolyVegTileCounts();
                    }
                    if(selectedObj->typeObj == GameObj::tritemobj){
                        if(GuiFunct::confirmDestructiveAction(
                                this, "Remove Track Item",
                                "Remove this track item?\n\n"
                                "This can damage the route when used without "
                                "understanding its database links.")){
                            route->deleteTrackItem((TRitem*)selectedObj);
                            selectedObj->unselect();
                        }
                    }
                    if(selectedObj->typeObj == GameObj::activityobj){
                        selectedObj->remove();
                        emit sendMsg("refreshActivityTools");
                    }
                    setSelectedObj(NULL);
                    lastSelectedObj = NULL;
                }
                break;
            case Qt::Key_C:
                if (selectedObj != NULL) {
                    selectedObj->unselect();
                    if(selectedObj->typeObj == GameObj::worldobj){
                        setSelectedObj(route->placeObject(((WorldObj*)selectedObj)->x, ((WorldObj*)selectedObj)->y, ((WorldObj*)selectedObj)->position, ((WorldObj*)selectedObj)->qDirection, 0, ((WorldObj*)selectedObj)->getRefInfo()));
                        if (selectedObj != NULL) {
                            selectedObj->select();
                        }
                    }
                }
                break;
            case Qt::Key_P:
                if (keyControlEnabled)
                    pickObjForPlacement();
                else if(keyShiftEnabled)
                    pickObjRotElevForPlacement();
                else
                    pickObjRotForPlacement();
                break;
            case Qt::Key_Z:
                //route->refreshObj(selectedWorldObj);
                //route->trackDB->setDefaultEnd(0);
                //route->addToTDB(selectedWorldObj, (float*)&lastNewObjPosT, (float*)&selectedWorldObj->position);
                Undo::StateBegin();
                Undo::PushTrackDB(Game::trackDB, false);
                Undo::PushTrackDB(Game::roadDB, true);
                route->toggleToTDB((WorldObj*)selectedObj);
                Undo::StateEnd();
                // EFO keep object selected
                // next three lines commented out
                 if (selectedObj != NULL) selectedObj->unselect();
                 lastSelectedObj = selectedObj;
                 setSelectedObj(NULL);
                break;
            case Qt::Key_X:
                if (selectedObj == NULL)
                    return;
                if (selectedObj->typeObj != WorldObj::worldobj)
                    return;
                {
                    WorldObj *worldObj = (WorldObj*)selectedObj;
                    const bool preserveTrackGrade = worldObj->typeID == WorldObj::trackobj;
                    float trackGradeBeforeFlip = preserveTrackGrade
                            ? trackGradePercent(selectedObj) : 0.0f;
                    if(preserveTrackGrade){
                        if(Game::gradeAssistEnabled || Game::gradeAssistTargetReached)
                            trackGradeBeforeFlip = Game::gradeAssistCurrentPercent;
                        else if(Game::gradeLockEnabled)
                            trackGradeBeforeFlip = Game::gradeLockedPercent;
                    }
                    //route->refreshObj(selectedWorldObj);
                    route->flipObject(worldObj);
                    if(preserveTrackGrade){
                        // Swapping the placement end rebuilds the track quaternion
                        // from the database and would otherwise flatten the piece.
                        // Restore the same physical grade; TrackObj accounts for
                        // the reversed endpoint sign internally.
                        ((TrackObj*)worldObj)->setElevation(trackGradeBeforeFlip * 10.0f);
                    } else if(placeElev != 0) {
                        selectedObj->rotate(placeElev, 0, 0);
                    }
                }
                //selectToolresetRot();
                break;
            default:
                break;
        }
    }
}

void RouteEditorGLWidget::keyReleaseEvent(QKeyEvent * event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    camera->keyUp(event);
    switch (event->key()) {
            //case Qt::Key_Alt:
        case Qt::Key_Control:
            moveStep = moveMaxStep;
            keyControlEnabled = false;
            break;
        case Qt::Key_Shift:
            keyShiftEnabled = false;
            break;
        case Qt::Key_Alt:
            moveStep = moveMaxStep;
            keyAltEnabled = false;
            break;
        default:
            break;
    }
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        switch (event->key()) {
            default:
                break;
        }
    }
}

void RouteEditorGLWidget::mousePressEvent(QMouseEvent *event) {
    Game::currentShapeLib = currentShapeLib;
    bolckContextMenu = false;
    if (!route->loaded) return;
    m_lastPos = event->pos();
    m_lastPos *= Game::PixelRatio;
    mousex = m_lastPos.x();
    mousey = m_lastPos.y();
    mouseClick = true;  // if(Game::debugOutput) qDebug() << "REGLW 1260";
    if ((event->button()) == Qt::RightButton) {
        mouseRPressed = true;
        camera->MouseDown(event);
    }
    if ((event->button()) == Qt::LeftButton) {
        Undo::StateBegin();
        mouseLPressed = true;
        lastMousePressTime = QDateTime::currentMSecsSinceEpoch();

        /// placing an item onto the world EFO

        const bool rulerWaterPlacement = toolEnabled == "placeTool"
            && route->ref->selected != NULL
            && route->ref->selected->type.compare("rulerwater", Qt::CaseInsensitive) == 0;
        if(rulerWaterPlacement){
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(
                pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit waterPanelStatus("Terrain unavailable at this point.");
            } else {
                // Open Water Tools and start its water-ruler mode through
                // the same signal path used by the user-facing controls. The
                // independent waterRulerTool block below consumes this click
                // as the first terrain-snapped point.
                emit waterRulerPlacementRequested();
            }
        }

        const bool rulerVegetationPlacement = toolEnabled == "placeTool"
            && route->ref->selected != NULL
            && route->ref->selected->type.compare("rulervegetation", Qt::CaseInsensitive) == 0;
        if(rulerVegetationPlacement){
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(
                pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit polyVegPanelStatus("Terrain unavailable at this point.");
            } else {
                emit polyVegRulerPlacementRequested();
            }
        }

        const bool rulerGradePlacement = toolEnabled == "placeTool"
            && route->ref->selected != NULL
            && route->ref->selected->type.compare("rulergrade", Qt::CaseInsensitive) == 0;
        if(rulerGradePlacement){
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(
                pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit waterHelperStatus("Unable to load terrain beneath the grade ruler point.");
            } else {
                RulerObj *selectedRuler = selectedObj != NULL
                    ? dynamic_cast<RulerObj*>(selectedObj) : NULL;
                const bool selectedSpecialRuler = selectedRuler != NULL
                    && selectedRuler->isSpecialRuler();
                route->deleteSpecialRulers();
                if(selectedSpecialRuler)
                    setSelectedObj(NULL);
                activeWaterRuler = NULL;
                activeVegetationRuler = NULL;
                activeGradeRuler = NULL;
                enableTool("gradeRulerTool");
                emit waterHelperStatus(
                    "Grade ruler active - place the two grade endpoints.");
            }
        }

        if (toolEnabled == "placeTool" && !rulerWaterPlacement
                && !rulerVegetationPlacement && !rulerGradePlacement) {

        //QString selectedFilename = selectedObj->objectName();
        //qDebug() << "placed objName: " << selectedFilename;


         // if(Game::debugOutput) qDebug() << "REGLW 1278 placed: " ;

            bool previousTrackValid = false;
            int previousTrackX = 0;
            int previousTrackY = 0;
            unsigned int previousTrackUid = 0;
            float previousTrackGrade = 1000000.0f;
            GameObj *previousTrackObject = NULL;
            if(selectedObj != NULL && selectedObj->typeObj == GameObj::worldobj){
                WorldObj *previousWorld = (WorldObj*)selectedObj;
                if(previousWorld->typeID == WorldObj::trackobj){
                    previousTrackValid = true;
                    previousTrackObject = selectedObj;
                    previousTrackX = previousWorld->x;
                    previousTrackY = previousWorld->y;
                    previousTrackUid = previousWorld->UiD;
                    previousTrackGrade = trackGradePercent(selectedObj);
                }
            }
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    if (selectedObj->typeObj == GameObj::worldobj)
                        route->addToTDBIfNotExist((WorldObj*) selectedObj);  // if(Game::debugOutput) qDebug() << "REGLW 1284";
            }
            Undo::StateBeginIfNotExist();

            if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
             lastNewObjPos[2] = aktPointerPos[2];
            float *q = Quat::create();
            Quat::copy(q, this->placeRot);
            setSelectedObj(route->placeObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, q, placeElev)); if(Game::debugOutput) qDebug() << "REGLW 1294";
            if (selectedObj == NULL)
                showPlacementGuardError();
            if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, route->ref->selected, placementPointer, placementTileX, placementTileZ)) {
                rejectPlacement();
                return;
            }
            if (selectedObj != NULL) {
                bool gradeAchieved = false;
                if(!applyGradeLockToPlacedTrack(previousTrackValid, previousTrackX, previousTrackY,
                                                previousTrackUid, previousTrackGrade, gradeAchieved)){
                    rejectPlacement();
                    if(previousTrackObject != NULL){
                        setSelectedObj(previousTrackObject);
                        previousTrackObject->select();
                    }
                    return;
                }
                showPlacementSuccess();
                emit itemSelected(route->ref->selected);
                if(gradeAchieved){
                    QTimer::singleShot(1000, this, [this](){
                        playPlacementSound("SCOchirp.wav");
                    });
                }
                selectedObj->select();
            }
        }
        if (toolEnabled == "autoPlaceSimpleTool") {
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    if (selectedObj->typeObj == GameObj::worldobj)
                        route->addToTDBIfNotExist((WorldObj*) selectedObj);
            }
            Undo::StateBeginIfNotExist();
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
            lastNewObjPos[2] = aktPointerPos[2];
            int mode = 0;
            if (keyControlEnabled)
                mode = 1;
            if (keyShiftEnabled)
                mode = 2;
            setSelectedObj(route->autoPlaceObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, mode));
            if (selectedObj == NULL)
                showPlacementGuardError();
            if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, route->ref->selected, placementPointer, placementTileX, placementTileZ)) {
                rejectPlacement();
                return;
            }
            if (selectedObj != NULL) {
                showPlacementSuccess();
                emit itemSelected(route->ref->selected);
                selectedObj->select();
            }
        }
        if (toolEnabled == "waterRulerTool") {
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit waterPanelStatus("Terrain unavailable at this point.");
            } else {
                float bedHeight = Game::terrainLib->getHeight(
                    pointTileX, pointTileZ, point[0], point[2], false);
                if(bedHeight > -10000.0f)
                    point[1] = bedHeight;

                if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
                    activeWaterRuler = route->findWaterRuler(true);

                if(activeWaterRuler == NULL){
                    activeWaterRuler = route->placeWaterRuler(pointTileX, pointTileZ, point);
                } else {
                    Undo::PushWorldObjData(activeWaterRuler);
                    activeWaterRuler->appendWaterPoint(pointTileX, pointTileZ, point);
                }
                waterScanUndoAvailable = false;

                if(activeWaterRuler != NULL){
                    activeVegetationRuler = NULL;
                    activeGradeRuler = NULL;
                    if(selectedObj != NULL && selectedObj != activeWaterRuler)
                        selectedObj->unselect();
                    // Ruler placement does not need to replace the currently
                    // displayed Object Properties panel on every added point.
                    setSelectedObj(activeWaterRuler, false);
                    activeWaterRuler->select(
                        activeWaterRuler->getPointCount() - 1);
                    if(rulerWaterPlacement)
                        showPlacementSuccess();
                } else {
                    emit waterPanelStatus("Could not create the water ruler.");
                }
            }
        }
        if (toolEnabled == "vegetationRulerTool") {
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(
                pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit polyVegPanelStatus("Terrain unavailable at this point.");
            } else {
                float terrainHeight = Game::terrainLib->getHeight(
                    pointTileX, pointTileZ, point[0], point[2], false);
                if(terrainHeight > -10000.0f)
                    point[1] = terrainHeight;

                if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
                    activeVegetationRuler = route->findVegetationRuler(true);
                if(activeVegetationRuler != NULL
                        && !activeVegetationRuler->acceptsVegetationTile(
                            pointTileX, pointTileZ)) {
                    playPlacementSound("SCOpluck.wav");
                    emit polyVegPanelStatus(
                        "Points must stay on the first tile.");
                } else if(activeVegetationRuler == NULL){
                    activeVegetationRuler = route->placeVegetationRuler(
                        pointTileX, pointTileZ, point);
                    if(activeVegetationRuler != NULL)
                        activeVegetationRuler->setVegetationArea(polyVegRulerArea);
                } else {
                    Undo::PushWorldObjData(activeVegetationRuler);
                    activeVegetationRuler->appendVegetationPoint(
                        pointTileX, pointTileZ, point);
                }

                if(activeVegetationRuler != NULL){
                    activeVegetationRuler->setVegetationWidth(
                        static_cast<float>(polyVegRulerWidth));
                    activeWaterRuler = NULL;
                    activeGradeRuler = NULL;
                    if(selectedObj != NULL && selectedObj != activeVegetationRuler)
                        selectedObj->unselect();
                    setSelectedObj(activeVegetationRuler, false);
                    activeVegetationRuler->select(
                        activeVegetationRuler->getPointCount() - 1);
                    if(rulerVegetationPlacement)
                        playPlacementSound("SCOpluck.wav");
                } else {
                    emit polyVegPanelStatus("Could not create the PolyVeg ruler.");
                }
            }
        }
        if (toolEnabled == "gradeRulerTool") {
            int pointTileX = (int)camera->pozT[0];
            int pointTileZ = (int)camera->pozT[1];
            float point[3];
            Vec3::copy(point, aktPointerPos);
            Game::check_coords(pointTileX, pointTileZ, point);
            Terrain *pointTerrain = Game::terrainLib->getTerrainByXY(
                pointTileX, pointTileZ, true);
            if(pointTerrain == NULL || !pointTerrain->loaded){
                emit waterHelperStatus("Unable to load terrain beneath the grade ruler point.");
            } else {
                float terrainHeight = Game::terrainLib->getHeight(
                    pointTileX, pointTileZ, point[0], point[2], false);
                if(terrainHeight > -10000.0f)
                    point[1] = terrainHeight;

                if(activeGradeRuler == NULL || !activeGradeRuler->loaded)
                    activeGradeRuler = route->findGradeRuler(true);
                if(activeGradeRuler == NULL){
                    activeGradeRuler = route->placeGradeRuler(
                        pointTileX, pointTileZ, point);
                } else if(activeGradeRuler->getPointCount() < 2){
                    Undo::PushWorldObjData(activeGradeRuler);
                    activeGradeRuler->appendGradePoint(pointTileX, pointTileZ, point);
                }

                if(activeGradeRuler != NULL){
                    activeWaterRuler = NULL;
                    activeVegetationRuler = NULL;
                    if(selectedObj != NULL && selectedObj != activeGradeRuler)
                        selectedObj->unselect();
                    setSelectedObj(activeGradeRuler);
                    activeGradeRuler->select(
                        activeGradeRuler->getPointCount() - 1);
                    if(rulerGradePlacement)
                        showPlacementSuccess();
                    if(activeGradeRuler->getPointCount() >= 2){
                        emit waterHelperStatus(
                            "Grade ruler complete - press Select to move either endpoint.");
                    }
                } else {
                    emit waterHelperStatus("Unable to create the grade ruler.");
                }
            }
        }
        if (toolEnabled == "selectTool") {
            if (!translateTool && !rotateTool && !resizeTool)
                selection = true;
            if (selectedObj != NULL) {
                mouseLPressed = true;
                if (translateTool) {
                    if (selectedObj->typeObj == GameObj::worldobj) {
                        Undo::PushGameObjData((WorldObj*) selectedObj);
                        float tempPos[3];
                        int tx = camera->pozT[0];
                        int tz = camera->pozT[1];
                        route->getPointerPosition(tempPos, tx, tz, aktPointerPos);
                        ((WorldObj*) selectedObj)->setPosition(tx, tz, tempPos);
                        ((WorldObj*) selectedObj)->setMartix();
                    }
                }
                if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "signalLinkTool") {
            Undo::PushGameObjData((WorldObj*) selectedObj);
            Undo::PushTrackDB(Game::trackDB);
            route->linkSignal((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, (WorldObj*) selectedObj);
            enableTool("");
        }
        if (toolEnabled == "FlexTool") {
            if(route != NULL && selectedObj != NULL
                    && selectedObj->typeObj == GameObj::worldobj
                    && !route->canRemoveTrackFromTDB(
                        static_cast<WorldObj*>(selectedObj), true)) {
                showPlacementGuardError();
                enableTool("selectTool");
                return;
            }
            emit flexData((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "heightTool") {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            route->paintHeightMap(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled.startsWith("paintTool")) {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            if (keyControlEnabled)
                route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 0);
            else if (keyShiftEnabled)
                route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, NULL);
            else
                Game::terrainLib->paintTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "pickTerrainTexTool") {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            int textureId = Game::terrainLib->getTexture((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            makeCurrent();
            emit setBrushTextureId(textureId);
            doneCurrent();
        }
        if (toolEnabled == "putTerrainTexTool") {
            Game::terrainLib->setTerrainTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            lastPointerPos[0] = aktPointerPos[0];
            lastPointerPos[1] = aktPointerPos[1];
            lastPointerPos[2] = aktPointerPos[2];
        }
        if (toolEnabled == "waterTerrTool") {
            Game::terrainLib->toggleWaterDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
        }
        if (toolEnabled == "drawTerrTool") {
            Game::terrainLib->toggleDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "waterHeightTileTool") {
            Game::terrainLib->setWaterLevelGui((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "fixedTileTool") {
            Game::terrainLib->setFixedTileHeight(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "mapTileShowTool") {
            Game::terrainLib->setTileBlob((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "mapTileLoadTool") {
            int x = (int) camera->pozT[0];
            int z = (int) camera->pozT[1];
            float posx = aktPointerPos[0];
            float posz = aktPointerPos[2];
            Game::check_coords(x, z, posx, posz);
            Terrain *t = Game::terrainLib->getTerrainByXY(x, z);
            if(t == NULL)
                return;
            if(!t->loaded)
                return;
            delete mapWindow;
            mapWindow = new MapWindow(this);
            t->getLowCornerTileXY(mapWindow->tileX, mapWindow->tileZ);
            mapWindow->tileSize = t->getSampleCount()*t->getSampleSize();
            mapWindow->exec();
        }
        if (toolEnabled == "heightTileLoadTool") {
            Game::terrainLib->setHeightFromGeoGui((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "lockTexTool") {
            Game::terrainLib->lockTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "gapsTerrainTool") {
            Game::terrainLib->toggleGaps((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
        }
        if (toolEnabled == "makeTileTextureTool") {
            Game::terrainLib->makeTextureFromMap((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "removeTileTextureTool") {
            Game::terrainLib->removeTileTextureFromMap((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "actNewLooseConsistTool") {
            route->actNewLooseConsist((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            emit sendMsg("refreshActivityTools");
        }
        if (toolEnabled == "actNewSpeedZoneTool") {
            route->actNewNewSpeedZone((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            emit sendMsg("refreshActivityTools");
        }
        if (toolEnabled == "pickNewEventLocationTool") {
            route->actPickNewEventLocation((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            enableTool("");
        }
        if (toolEnabled == "") {
            camera->MouseDown(event);
        }
    }
    setFocus();
}

void RouteEditorGLWidget::wheelEvent(QWheelEvent *event) {
    const int verticalDelta = event->angleDelta().y();
    float numDegrees = 0.01 * verticalDelta;

    if (verticalDelta != 0) {
        if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
            /// Move the selected object up or down
            if (selectedObj != NULL) {
                if (selectedObj->typeObj == GameObj::worldobj) {
                    Undo::StateBeginIfNotExist();
                    Undo::PushGameObjData(selectedObj);
                    ((WorldObj*) selectedObj)->translate(0, numDegrees*moveStep, 0);
                }
                else
                {
                /// EFO Move the camera forward or backward if no object selected
                if(numDegrees != 0)  { camera->moveForward( (numDegrees*10) ); }
                }
            }
        } else {
            /// EFO Move the camera forward or backward if in free view
            if(numDegrees != 0)  { camera->moveForward( (numDegrees*10) ); }
        }
    } else {

    }
    event->accept();
    //qDebug() << "scrollwheel: " << numDegrees;
}

void RouteEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    camera->MouseUp(event);
    if ((event->button()) == Qt::RightButton) {
        mouseRPressed = false;
        if(mouseClick && !bolckContextMenu)
            showContextMenu(event->pos());
    }
    if ((event->button()) == Qt::LeftButton) {
        if(selectedObj != NULL && selectedObj->typeObj == GameObj::worldobj){
            RulerObj *selectedRuler = dynamic_cast<RulerObj*>((WorldObj*)selectedObj);
            if(selectedRuler != NULL && selectedRuler->isSpecialRuler()){
                const bool pointDragCompleted =
                    selectedRuler->snapSelectedSpecialPointToTerrain();
                if(pointDragCompleted && selectedRuler->isWaterRuler())
                    waterScanUndoAvailable = false;
                selectedRuler->setMartix();
            }
        }
        mouseLPressed = false;
        Undo::StateEnd();
    }
    mouseClick = false;
    bolckContextMenu = false;
}

void RouteEditorGLWidget::mouseMoveEvent(QMouseEvent *event) {
    mouseClick = false;
    bolckContextMenu = false;
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    /*int dx = event->position().x() - m_lastPos.x();
    int dy = event->position().y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {

    } else if (event->buttons() & Qt::RightButton) {

    }*/
    mousex = event->position().x() * Game::PixelRatio;
    mousey = event->position().y() * Game::PixelRatio;

    if ((event->buttons() & 2) == Qt::RightButton) {
        camera->MouseMove(event);
    }
    if ((event->buttons() & 1) == Qt::LeftButton) {
        if (toolEnabled.startsWith("paintTool") && mouseLPressed == true) {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                Game::terrainLib->paintTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            }
        }
        if (toolEnabled == "heightTool" && mouseLPressed == true) {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                route->paintHeightMap(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            }
        }
        if (toolEnabled == "waterTerrTool") {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                Game::terrainLib->toggleWaterDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
            }
        }
        if (toolEnabled == "putTerrainTexTool" && mouseLPressed == true) { if(Game::debugOutput) qDebug() << "Put: " << defaultPaintBrush->tex->pathid;
            if (fabs(lastPointerPos[0] - aktPointerPos[0]) > 32 || fabs(lastPointerPos[2] - aktPointerPos[2]) > 32) {
                Game::terrainLib->setTerrainTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "selectTool") {
            if (selectedObj != NULL && mouseLPressed) {
                if (!translateTool && !rotateTool && !resizeTool) {
                long long int ntime = QDateTime::currentMSecsSinceEpoch();
                if (ntime - lastMousePressTime > 200) {
                        if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                        Undo::PushGameObjData(selectedObj);
                        if (keyShiftEnabled) {
                            float val = mousex - m_lastPos.x();
                            selectedObj->rotate(0, val * moveStep * 0.1, 0);
                        } else {
                            if(selectedObj->typeObj == GameObj::worldobj)
                                route->dragWorldObject((WorldObj*)selectedObj, camera->pozT[0], camera->pozT[1], aktPointerPos);
                            if(selectedObj->typeObj == GameObj::activityobj)
                                selectedObj->setPosition((int)camera->pozT[0], (int)camera->pozT[1], aktPointerPos);
                        }
                    }
                }
                if (translateTool) {
                    Undo::PushGameObjData(selectedObj);
                    selectedObj->setPosition(camera->pozT[0], camera->pozT[1], aktPointerPos);
                    selectedObj->setMartix();
                }
                if (rotateTool) {
                    Undo::PushGameObjData(selectedObj);
                    float val = mousex - m_lastPos.x();
                    selectedObj->rotate(0, val * moveStep * 0.1, 0);
                }
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "placeTool") {
            if (selectedObj != NULL && mouseLPressed) {
                long long int ntime = QDateTime::currentMSecsSinceEpoch();
                if (ntime - lastMousePressTime > 200) {
                    if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                    Undo::PushGameObjData(selectedObj);
                    if (keyShiftEnabled) {
                        float val = mousex - m_lastPos.x();
                        selectedObj->rotate(0, val * moveStep * 0.1, 0);
                    } else {
                        route->dragWorldObject((WorldObj*)selectedObj, camera->pozT[0], camera->pozT[1], aktPointerPos);
                    }
                }
            }
        }
        if (toolEnabled == "") {
            camera->MouseMove(event);
        }
    }
    m_lastPos = event->pos();
    m_lastPos *= Game::PixelRatio;
}

void RouteEditorGLWidget::enableTool(QString name) {
    if(Game::debugOutput) qDebug() << name;
    if(name != "placeTool" && (Game::gradeAssistEnabled || Game::gradeAssistTargetReached)){
        Game::gradeAssistEnabled = false;
        Game::gradeAssistTargetReached = false;
        emit resetGradeHelperRequested();
    }
    toolEnabled = name;
    //if(toolEnabled == "placeTool" || toolEnabled == "selectTool" || toolEnabled == "autoPlaceSimpleTool"){
    resizeTool = false;
    translateTool = false;
    rotateTool = false;
    //}
    emit sendMsg("toolEnabled", name);
}

void RouteEditorGLWidget::setSpecialRulerPanelControlsActive(bool active) {
    if(specialRulerPanelControlsActive == active)
        return;
    specialRulerPanelControlsActive = active;
    emit primaryEditorToolsEnabled(!active);
}

void RouteEditorGLWidget::toggleRouteMapOverlays() {
    if(Game::terrainLib == NULL)
        return;
    Game::terrainLib->setRouteMapOverlayVisible(
        !MapWindow::routeMapOverlaysVisible);
}

void RouteEditorGLWidget::positionWaterMessage(){
    if(waterMessageLabel == NULL)
        return;
    const float scale = qMax(0.75f, Game::uiScale);
    const int sideMargin = qRound(36.0f * scale);
    const int topMargin = qRound(36.0f * scale);
    const int availableWidth = qMax(qRound(180.0f * scale),
                                    width() - sideMargin * 2);
    const int maximumWidth = qMin(qRound(780.0f * scale), availableWidth);
    waterMessageLabel->setMaximumWidth(maximumWidth);
    waterMessageLabel->adjustSize();
    waterMessageLabel->move((width() - waterMessageLabel->width()) / 2, topMargin);
}

void RouteEditorGLWidget::showWaterMessage(const QString &text, int visibleMilliseconds){
    if(waterMessageLabel == NULL || text.isEmpty())
        return;
    waterMessageGeneration++;
    const int generation = waterMessageGeneration;
    waterMessageLabel->setText(text);
    positionWaterMessage();
    waterMessageLabel->show();
    waterMessageLabel->raise();
    QTimer::singleShot(visibleMilliseconds, this, [this, generation](){
        if(waterMessageLabel != NULL && generation == waterMessageGeneration)
            waterMessageLabel->hide();
    });
}

void RouteEditorGLWidget::statusPanelCommand(QString name) {
    if(name == "movefast"){
        cameraMoveSpeedLock = (cameraMoveSpeedLock == 1) ? 0 : 1;
        if(camera != NULL)
            camera->setMoveSpeedLock(cameraMoveSpeedLock);
        emit updStatus(QString("movefast"), cameraMoveSpeedLock == 1 ? QString("ON") : QString("OFF"));
        emit updStatus(QString("moveslow"), QString("OFF"));
        showModeChange();
        QTimer::singleShot(0, this, SLOT(focusEditor()));
        return;
    }
    if(name == "moveslow"){
        cameraMoveSpeedLock = (cameraMoveSpeedLock == -1) ? 0 : -1;
        if(camera != NULL)
            camera->setMoveSpeedLock(cameraMoveSpeedLock);
        emit updStatus(QString("moveslow"), cameraMoveSpeedLock == -1 ? QString("ON") : QString("OFF"));
        emit updStatus(QString("movefast"), QString("OFF"));
        showModeChange();
        QTimer::singleShot(0, this, SLOT(focusEditor()));
        return;
    }
    if((name == "select" || name == "place")
            && specialRulerPanelControlsActive){
        return;
    }
    if(name == "select"){
        if(toolEnabled == "selectTool" && !resizeTool && !rotateTool && !translateTool){
            enableTool("");
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolSelect();
        showModeChange();
        return;
    }
    if(name == "place"){
        const bool waterRulerSelected = route != NULL
                && route->ref != NULL
                && route->ref->selected != NULL
                && route->ref->selected->type.compare(
                    "rulerwater", Qt::CaseInsensitive) == 0;
        if(waterRulerSelected){
            emit waterRulerPlacementRequested();
            showModeChange();
            return;
        }
        const bool polyVegRulerSelected = route != NULL
                && route->ref != NULL
                && route->ref->selected != NULL
                && route->ref->selected->type.compare(
                    "rulervegetation", Qt::CaseInsensitive) == 0;
        if(polyVegRulerSelected){
            emit polyVegRulerPlacementRequested();
            showModeChange();
            return;
        }
        if(toolEnabled == "placeTool"){
            enableTool("");
            showModeChange();
            return;
        }
        enableTool("placeTool");
        showModeChange();
        return;
    }
    if(name == "rotate"){
        if(toolEnabled == "selectTool" && rotateTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolRotate();
        showModeChange();
        return;
    }
    if(name == "translate"){
        if(toolEnabled == "selectTool" && translateTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolTranslate();
        showModeChange();
        return;
    }
    if(name == "resize"){
        if(toolEnabled == "selectTool" && resizeTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolScale();
        showModeChange();
        return;
    }
    if(name == "autotdb"){
        if(Game::writeTDB)
            autoAddToTDB = !autoAddToTDB;
        showModeChange();
        return;
    }
    if(name == "stickterr"){
        if(stickPointerToTerrain)
            placeToolStickAll();
        else
            placeToolStickTerrain();
        showModeChange();
        return;
    }
    if(name == "brushdir"){
        if(defaultPaintBrush == NULL)
            return;
        if(defaultPaintBrush->direction == 1)
            toolBrushDirectionDown();
        else
            toolBrushDirectionUp();
        showModeChange();
        return;
    }
    if(name == "camera"){
        Game::lockCamera = !Game::lockCamera;
        if(camera != NULL)
            camera->setLockYaxis(Game::lockCamera);
        showModeChange();
        return;
    }
    if(name == "camterr"){
        Game::cameraStickToTerrain = !Game::cameraStickToTerrain;
        showModeChange();
        return;
    }
    if(name == "guard"){
        placeGuardEnabled = !placeGuardEnabled;
        showModeChange();
        return;
    }
    if(name == "clearselect"){
        if(selectedObj != NULL){
            selectedObj->unselect();
            setSelectedObj(NULL);
        }
        lastSelectedObj = NULL;
        return;
    }
}

void RouteEditorGLWidget::jumpTo(PreciseTileCoordinate* c) {
    jumpTo(c->TileX, -c->TileZ, c->wX, c->wY, -c->wZ);
}

void RouteEditorGLWidget::jumpTo(float *posT, float *pos) {
    int X = posT[0];
    int Z = posT[1];
    float x = pos[0];
    float z = pos[2];
    Game::check_coords(X, Z, x, z);
    jumpTo(X, Z, x, pos[1], z);
}

void RouteEditorGLWidget::jumpTo(int X, int Z, float x, float y, float z) {
    if(Game::debugOutput) qDebug() << "jump: " << X << " " << Z;
    Game::terrainLib->load(X, Z);
    float h = Game::terrainLib->getHeight(X, Z, x, z);
    //if(h == -1)
        y = y + 10;
    if ((y < h) || (y > h + 100))
        y = h + 20;

    camera->setPozT(X, Z);
    camera->setPos(x, y, z);

}

/// EFO Object Selected
void RouteEditorGLWidget::objectSelected(GameObj* obj){
    RulerObj *restoredRuler = obj != NULL
            ? dynamic_cast<RulerObj*>(obj) : NULL;
    const bool restoredSpecialRuler = restoredRuler != NULL
            && restoredRuler->isSpecialRuler();
    if (selectedObj != NULL) {
        selectedObj->unselect();
        setSelectedObj(NULL, !restoredSpecialRuler);
    }
    if(obj == NULL)
        return;

    if(restoredSpecialRuler){
        activeWaterRuler = restoredRuler->isWaterRuler()
                ? restoredRuler : NULL;
        activeVegetationRuler = restoredRuler->isVegetationRuler()
                ? restoredRuler : NULL;
        activeGradeRuler = restoredRuler->isGradeRuler()
                ? restoredRuler : NULL;
        const bool matchingPlacementMode =
                (restoredRuler->isWaterRuler()
                    && toolEnabled == "waterRulerTool")
                || (restoredRuler->isVegetationRuler()
                    && toolEnabled == "vegetationRulerTool")
                || (restoredRuler->isGradeRuler()
                    && toolEnabled == "gradeRulerTool");
        if(!matchingPlacementMode)
            toolEnabled = "selectTool";
        restoredRuler->select();
        setSelectedObj(restoredRuler, false);
        return;
    }

    toolEnabled = "selectTool";   /// set the selected tool
    obj->select();
    setSelectedObj(obj);
}

void RouteEditorGLWidget::objectSelected(QVector<GameObj*> obj){
    if (selectedObj != NULL) {
        selectedObj->unselect();
        setSelectedObj(NULL);
    }
    if(obj.size() == 0)
        return;
    for(int i = 0; i < obj.size(); i++){
        groupObj->addObject((WorldObj*)obj[i]);
        groupObj->select();
        setSelectedObj(groupObj);
    }
}

void RouteEditorGLWidget::setPaintBrush(Brush* brush) {
    this->defaultPaintBrush = brush;
    Terrain::DefaultBrush = brush;
}

void RouteEditorGLWidget::setSelectedObj(GameObj* o, bool refreshProperties) {
    selectedObj = o;
    Game::currentSelectedGameObj = selectedObj;
    if(refreshProperties)
        emit showProperties(selectedObj);
    if (o != NULL)
        if (o->typeObj == o->worldobj)
           emit sendMsg("showShape", ((WorldObj*) o)->getShapePath());
}

void RouteEditorGLWidget::editCopy() {
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        if (selectedObj != NULL) {
            if (selectedObj->typeObj == GameObj::worldobj) {
                WorldObj *selectedWorldObj = (WorldObj*) selectedObj;
                if (selectedWorldObj->typeID == WorldObj::groupobject) {
                    delete copyPasteGroupObj;
                    copyPasteGroupObj = new GroupObj(*groupObj);
                    copyPasteObj = copyPasteGroupObj;
                } else {
                    copyPasteObj = selectedWorldObj;
                }
            }
        }
    }
}

void RouteEditorGLWidget::copySelectionInfo() {
    if (selectedObj == NULL)
        return;

    const auto number = [](double value) {
        return QString::number(value, 'f', 6);
    };
    const auto yesNo = [](bool value) {
        return value ? QString("Yes") : QString("No");
    };
    const auto worldTypeName = [](WorldObj::TypeID typeId) {
        switch (typeId) {
        case WorldObj::sstatic:       return QString("Static Object");
        case WorldObj::signal:        return QString("Signal");
        case WorldObj::speedpost:     return QString("Speedpost");
        case WorldObj::trackobj:      return QString("Track Object");
        case WorldObj::gantry:        return QString("Gantry");
        case WorldObj::collideobject: return QString("Collision Object");
        case WorldObj::dyntrack:      return QString("Dynamic Track");
        case WorldObj::forest:        return QString("Forest");
        case WorldObj::transfer:      return QString("Transfer");
        case WorldObj::platform:      return QString("Platform");
        case WorldObj::siding:        return QString("Siding");
        case WorldObj::carspawner:    return QString("Car Spawner");
        case WorldObj::levelcr:       return QString("Level Crossing");
        case WorldObj::pickup:        return QString("Pickup");
        case WorldObj::hazard:        return QString("Hazard");
        case WorldObj::soundsource:   return QString("Sound Source");
        case WorldObj::soundregion:   return QString("Sound Region");
        case WorldObj::groupobject:   return QString("Object Group");
        case WorldObj::ruler:         return QString("Ruler");
        default:                      return QString("World Object");
        }
    };

    QStringList info;
    info << "TSRE Selection Info";
    info << "App Version: " + Game::AppVersion;
    info << "Route: " + (Game::route.isEmpty() ? QString("(unknown)") : Game::route);
    if (!Game::routeName.isEmpty() && Game::routeName != Game::route)
        info << "Route Name: " + Game::routeName;

    if (selectedObj->typeObj == GameObj::worldobj) {
        WorldObj *obj = static_cast<WorldObj*>(selectedObj);
        float *position = obj->getPosition();
        float *quaternion = obj->getQuatRotation();
        if (position == NULL)
            position = obj->position;
        if (quaternion == NULL)
            quaternion = obj->qDirection;

        info << "Selection Type: " + worldTypeName(obj->typeID);
        info << "Object Type: " + (obj->type.isEmpty() ? QString("(unknown)") : obj->type);
        if (!obj->fileName.isEmpty())
            info << "Object Name: " + obj->fileName;
        info << "UID: " + QString::number(obj->UiD);
        info << QString("Tile X/Y: %1, %2").arg(obj->x).arg(-obj->y);
        info << "Terrain Tile: " + Terrain::getTileName(obj->x, -obj->y);
        info << QString("Position X/Y/Z: %1, %2, %3")
                    .arg(number(position[0]), number(position[1]), number(-position[2]));
        info << QString("Quaternion X/Y/Z/W: %1, %2, %3, %4")
                    .arg(number(quaternion[0]), number(quaternion[1]),
                         number(-quaternion[2]), number(quaternion[3]));

        if (obj->typeID == WorldObj::trackobj || obj->typeID == WorldObj::dyntrack) {
            info << "Track Section Index: " + QString::number(obj->sectionIdx);
            const float displayedElevation = obj->typeID == WorldObj::dyntrack
                    ? static_cast<DynTrackObj*>(obj)->getAverageElevation()
                    : obj->getElevation();
            info << "Grade: " + number(tan(displayedElevation) * 100.0) + "%";
            info << "Elevation Radians: " + number(displayedElevation);
        }

        info << "Static Flags: 0x" + QString::number(obj->staticFlags, 16).toUpper();
        info << "Detail Level: " + QString::number(obj->getCurrentDetailLevel());
        info << "Loaded: " + yesNo(obj->loaded);
        info << "Modified: " + yesNo(obj->modified);
        info << "Flipped: " + yesNo(obj->flipped);

        if (obj->typeID == WorldObj::groupobject) {
            GroupObj *group = static_cast<GroupObj*>(obj);
            info << "Group Object Count: " + QString::number(group->objects.size());
            for (int i = 0; i < group->objects.size(); ++i) {
                WorldObj *member = group->objects[i];
                if (member == NULL)
                    continue;
                info << QString("Group Item %1: UID %2 | %3 | %4 | Tile %5,%6 | Pos %7,%8,%9")
                            .arg(i + 1).arg(member->UiD).arg(worldTypeName(member->typeID))
                            .arg(member->fileName.isEmpty() ? member->type : member->fileName)
                            .arg(member->x).arg(-member->y)
                            .arg(number(member->position[0]), number(member->position[1]),
                                 number(-member->position[2]));
            }
        }
    } else if (selectedObj->typeObj == GameObj::terrainobj) {
        Terrain *terrain = static_cast<Terrain*>(selectedObj);
        info << "Selection Type: Terrain";
        info << QString("Tile X/Y: %1, %2").arg(terrain->mojex).arg(-terrain->mojez);
        info << "Terrain Tile: " + terrain->getTileName();
        info << QString("Pointer Position X/Y/Z: %1, %2, %3")
                    .arg(number(aktPointerPos[0]), number(aktPointerPos[1]), number(-aktPointerPos[2]));
        QString texture = terrain->getPatchMainTextureName();
        if (!texture.isEmpty())
            info << "Selected Patch Texture: " + texture;
        info << "Selected Texture Path ID: " + QString::number(terrain->getSelectedPathId());
        info << "Selected Shader ID: " + QString::number(terrain->getSelectedShaderId());
        info << "Modified: " + yesNo(terrain->isModified());
    } else if (selectedObj->typeObj == GameObj::tritemobj) {
        TRitem *item = static_cast<TRitem*>(selectedObj);
        info << "Selection Type: Track Item";
        info << "Item Type: " + item->type;
        info << "Item Name: " + item->getTrackItemName();
        info << "Track Item ID: " + QString::number(item->trItemId);
        info << "Database: " + (item->tdbId == 0 ? QString("Track (TDB)")
                                                   : item->tdbId == 1 ? QString("Road (RDB)")
                                                                      : QString::number(item->tdbId));
        info << "Track Position: " + number(item->getTrackPosition());
        if (item->trItemRData != NULL) {
            info << QString("Tile X/Y: %1, %2")
                        .arg(number(item->trItemRData[3]), number(item->trItemRData[4]));
            info << "Terrain Tile: " + Terrain::getTileName((int)item->trItemRData[3],
                                                             (int)item->trItemRData[4]);
            info << QString("Position X/Y/Z: %1, %2, %3")
                        .arg(number(item->trItemRData[0]), number(item->trItemRData[1]),
                             number(item->trItemRData[2]));
        } else if (item->trItemPData != NULL) {
            info << QString("Tile X/Y: %1, %2")
                        .arg(number(item->trItemPData[2]), number(item->trItemPData[3]));
            info << QString("Position X/Z: %1, %2")
                        .arg(number(item->trItemPData[0]), number(item->trItemPData[1]));
        }
    } else if (selectedObj->typeObj == GameObj::activityobj) {
        ActivityObject *activity = static_cast<ActivityObject*>(selectedObj);
        info << "Selection Type: Activity Object";
        info << "Activity Object Type: " + activity->objectType;
        info << "Activity Object ID: " + QString::number(activity->id);
        QString parent = activity->getParentName();
        if (!parent.isEmpty())
            info << "Activity: " + parent;
        info << "Selected Element ID: " + QString::number(activity->getSelectedElementId());
        info << "Direction: " + number(activity->direction);
        float pos[5];
        if (activity->getElementPosition(activity->getSelectedElementId(), pos)) {
            info << QString("Tile X/Y: %1, %2").arg(number(pos[0]), number(pos[1]));
            info << QString("Position X/Y/Z: %1, %2, %3")
                        .arg(number(pos[2]), number(pos[3]), number(-pos[4]));
        }
    } else if (selectedObj->typeObj == GameObj::consistobj) {
        Consist *consist = static_cast<Consist*>(selectedObj);
        info << "Selection Type: Consist";
        info << "Consist Name: " + consist->name;
        if (!consist->displayName.isEmpty())
            info << "Display Name: " + consist->displayName;
        info << "Serial: " + QString::number(consist->serial);
        info << "Selected Vehicle Index: " + QString::number(consist->selectedIdx);
        info << "Vehicle Count: " + QString::number(consist->engItems.size());
        info << "Length: " + number(consist->conLength);
        info << "Mass: " + number(consist->mass);
        info << "On Track: " + yesNo(consist->isOnTrack);
        float pos[5];
        if (consist->selectedIdx >= 0 && consist->getWagonWorldPosition(consist->selectedIdx, pos)) {
            info << QString("Tile X/Y: %1, %2").arg(number(pos[0]), number(pos[1]));
            info << QString("Position X/Y/Z: %1, %2, %3")
                        .arg(number(pos[2]), number(pos[3]), number(-pos[4]));
        }
    } else if (selectedObj->typeObj == GameObj::activitypath) {
        Path *path = static_cast<Path*>(selectedObj);
        info << "Selection Type: Activity Path";
        info << "Path Name: " + path->displayName;
        info << "Path File: " + path->trPathName;
        info << "Path Start: " + path->trPathStart;
        info << "Path End: " + path->trPathEnd;
        info << "Path ID: " + path->pathId;
    } else {
        info << "Selection Type: " + selectedObj->getName();
        info << "Selection Class ID: " + QString::number((int)selectedObj->typeObj);
    }

    QApplication::clipboard()->setText(info.join('\n'));
    if (Game::debugOutput)
        qDebug().noquote() << info.join('\n');
}

void RouteEditorGLWidget::editPaste() {
    if(Game::debugOutput) qDebug() << "EditPaste Start";
    Undo::StateBeginIfNotExist();
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        if (copyPasteObj != NULL) {
            //qDebug() << "EditPaste selectedObj->unselect";
            if (selectedObj != NULL)
                selectedObj->unselect();
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
            lastNewObjPos[2] = aktPointerPos[2];
            //qDebug() << "EditPaste copyPasteObj->typeObj";
            if(copyPasteObj->typeObj == GameObj::worldobj) {
               // qDebug() << "EditPaste copyPasteObj->typeID";
                if (copyPasteObj->typeID == WorldObj::groupobject) {
                    //qDebug() << "EditPaste groupobject";
                    groupObj->fromNewObjects((GroupObj*) copyPasteObj, route, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
                    //qDebug() << "EditPaste setSelectedObj";
                    setSelectedObj(groupObj);
                } else {
                    Ref::RefItem* refInfo = copyPasteObj->getRefInfo();
                    //qDebug() << "EditPaste object";
                    float *q = Quat::create();
                    Quat::copy(q, copyPasteObj->qDirection);
                    setSelectedObj(route->placeObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, q, 0, refInfo));
                    if (selectedObj == NULL)
                        showPlacementGuardError();
                    //qDebug() << "EditPaste setSelectedObj";
                    if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, refInfo, placementPointer, placementTileX, placementTileZ)) {
                        rejectPlacement();
                        return;
                    }
                    if (selectedObj != NULL) {
                        showPlacementSuccess();
                        if (refInfo != NULL)
                            emit itemSelected(refInfo);
                        selectedObj->select();
                    }
                }
            }
        }
    }
    if(Game::debugOutput) qDebug() << "EditPaste End";
}

void RouteEditorGLWidget::editSelect() {
    enableTool("selectTool");
}

void RouteEditorGLWidget::selectToolresetMoveStep(){
    Game::DefaultMoveStep = defaultMoveStep;
    moveStep = defaultMoveStep;
    moveMaxStep = defaultMoveStep;
}

void RouteEditorGLWidget::selectToolresetRot(){
    Quat::fill(this->placeRot);
    placeElev = 0;
}

void RouteEditorGLWidget::selectToolresetVert(){

    WorldObj* obj = (WorldObj*)selectedObj;

    float nq[4];
    nq[0] = 0.0f;              // Set first value to 0
    nq[1] = obj->qDirection[1]; // Keep existing second value
    nq[2] = 0.0f;              // Set third value to 0
    nq[3] = obj->qDirection[3]; // Keep existing fourth value

    // 4. Update the object
    Undo::SinglePushWorldObjData(obj);

    // Pass the modified float array back to the object
    obj->setQdirection(nq);
    obj->setModified();
    obj->setMartix();
}


void RouteEditorGLWidget::selectToolSelect(){
    resizeTool = false;
    translateTool = false;
    rotateTool = false;
}

void RouteEditorGLWidget::selectToolRotate(){
    resizeTool = false;
    translateTool = false;
    rotateTool = true;
}

void RouteEditorGLWidget::selectToolTranslate(){
    resizeTool = false;
    translateTool = true;
    rotateTool = false;
}

void RouteEditorGLWidget::selectToolScale(){
    resizeTool = true;
    translateTool = false;
    rotateTool = false;
}

void RouteEditorGLWidget::toolBrushDirectionUp(){
    defaultPaintBrush->direction = 1;
    emit sendMsg(QString("brushDirection"), QString("+"));
}

void RouteEditorGLWidget::toolBrushDirectionDown(){
    defaultPaintBrush->direction = -1;
    emit sendMsg(QString("brushDirection"), QString("-"));
}
void RouteEditorGLWidget::putTerrainTexToolSelectRandom(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->RANDOM;
}

void RouteEditorGLWidget::putTerrainTexToolSelectPresent(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->PRESENT;
}

void RouteEditorGLWidget::putTerrainTexToolSelect0(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT0;
    defaultPaintBrush->texRotationDegrees = 0;
    emit sendMsg(QString("textureRotation"), QString("0"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect90(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT90;
    defaultPaintBrush->texRotationDegrees = 90;
    emit sendMsg(QString("textureRotation"), QString("90"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect180(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT180;
    defaultPaintBrush->texRotationDegrees = 180;
    emit sendMsg(QString("textureRotation"), QString("180"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect270(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT270;
    defaultPaintBrush->texRotationDegrees = 270;
    emit sendMsg(QString("textureRotation"), QString("270"));
}

void RouteEditorGLWidget::placeToolStickTerrain(){
    stickPointerToTerrain = true;
}

void RouteEditorGLWidget::placeToolStickAll(){
    stickPointerToTerrain = false;
}

void RouteEditorGLWidget::paintToolObjSelected(){
    route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, (WorldObj*) selectedObj);
}

void RouteEditorGLWidget::paintToolObj(){
    route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, NULL);
}

void RouteEditorGLWidget::paintToolTDB(){
    route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 0);
}

void RouteEditorGLWidget::paintToolTDBVector(){
    route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 1);
}

void RouteEditorGLWidget::paintToolTileTrack(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int sections = route->setTerrainTextureToTileTrack(tileX, tileZ, defaultPaintBrush);
    if(sections > 0)
        emit sendMsg(QString("msg"), QString("Autopainted track on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(sections) + QString(" sections."));
    else
        emit sendMsg(QString("msg"), QString("No track sections found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolTileRoad(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int sections = route->setTerrainTextureToTileRoad(tileX, tileZ, defaultPaintBrush);
    if(sections > 0)
        emit sendMsg(QString("msg"), QString("Autopainted roads on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(sections) + QString(" sections."));
    else
        emit sendMsg(QString("msg"), QString("No road sections found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolWaterEdges(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int paintedEdges = Game::terrainLib->paintWaterEdges(defaultPaintBrush, tileX, tileZ);
    if(paintedEdges > 0)
        emit sendMsg(QString("msg"), QString("Autopainted water edges on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(paintedEdges));
    else
        emit sendMsg(QString("msg"), QString("No water edges found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolResetTile(){
    QVector<QString> unsavedItems;
    getUnsavedInfo(unsavedItems);
    if (!unsavedItems.isEmpty()) {
        QMessageBox::warning(this, "Reset Tile",
                QString("There are %1 pending route change(s).\n\nSave your changes, then retry Reset Tile.")
                .arg(unsavedItems.size()));
        return;
    }

    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    QString msg = QString("Reset terrain paint on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("?");
    if (!GuiFunct::confirmDestructiveAction(
            this, "Reset Tile", msg))
        return;

    int filesDeleted = 0;
    int filesFailed = 0;
    if (!route->resetTerrainTextureOnTile(tileX, tileZ, filesDeleted, filesFailed)) {
        emit sendMsg(QString("msg"), QString("Reset Tile failed on ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
        return;
    }

    emit sendMsg(QString("msg"), QString("Reset tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(". Deleted ") + QString::number(filesDeleted) + QString(" texture file(s), failed ") + QString::number(filesFailed) + QString("."));
}

void RouteEditorGLWidget::plantNearestOsmForest() {
    if(route == NULL || camera == NULL || camera->pozT == NULL)
        return;

    QProgressDialog progress("Loading PolyVeg planting data...", QString(),
                             0, 1000, this);
    progress.setWindowTitle(Game::AppName);
    progress.setWindowModality(Qt::WindowModal);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    progress.setProperty("scoCenterOnScreen", true);
    GuiFunct::styleEditorDialog(&progress);
    progress.show();
    QApplication::processEvents();
    PolyVegViewportFreeze viewportFreeze(this);

    const QString routePath = Game::root + "/routes/" + Game::route;
    const ForestCatalogLoadResult catalogResult =
        ForestDefinitionLoader::loadRoute(routePath);
    progress.setValue(150);
    QApplication::processEvents();
    if(!catalogResult.isValid()) {
        progress.close();
        GuiFunct::showEditorStopped(this, "Plant PolyVeg",
            "polyveg.json could not be loaded:\n\n"
            + catalogResult.errors.join("\n"));
        return;
    }
    const ForestOsmCacheLoadResult cacheResult = ForestOsmCache::loadRoute(routePath);
    progress.setValue(500);
    QApplication::processEvents();
    if(!cacheResult.isValid()) {
        progress.close();
        GuiFunct::showEditorStopped(this, "Plant PolyVeg",
            "The route-local PolyVeg polygon cache could not be loaded:\n\n"
            + cacheResult.errors.join("\n"));
        return;
    }

    int pointerTileX = static_cast<int>(camera->pozT[0]);
    int pointerTileZ = static_cast<int>(camera->pozT[1]);
    float pointerX = aktPointerPos[0];
    float pointerZ = aktPointerPos[2];
    Game::check_coords(pointerTileX, pointerTileZ, pointerX, pointerZ);
    if(tileHasPolyVegBake(pointerTileX, pointerTileZ)) {
        progress.close();
        GuiFunct::showEditorNotice(this, "Plant PolyVeg",
            "The pointer tile is baked. Delete every PolyVeg - Bake object "
            "on that tile before planting again.");
        return;
    }
    const double cameraX = pointerTileX*2048.0 + pointerX;
    const double cameraZ = pointerTileZ*2048.0 + pointerZ;
    const ForestOsmPolygon *selectedPolygon = NULL;
    const ForestPlanPoint pointerPoint {cameraX, cameraZ};
    double selectedBoundsArea = std::numeric_limits<double>::max();
    int selectedDrawOrder = std::numeric_limits<int>::min();

    // First resolve the visible PolyVeg surface under the pointer without
    // considering the selected planting definition. A definition must never
    // make TSRE fall through a visible non-matching surface to one underneath.
    for(const ForestOsmPolygon &polygon : cacheResult.polygons) {
        if(cameraX < polygon.minimumX || cameraX > polygon.maximumX
                || cameraZ < polygon.minimumZ || cameraZ > polygon.maximumZ
                || !pointInBoundary(pointerPoint, polygon.boundary))
            continue;
        const double boundsArea = (polygon.maximumX-polygon.minimumX)
            *(polygon.maximumZ-polygon.minimumZ);
        if(polygon.drawOrder > selectedDrawOrder
                || (polygon.drawOrder == selectedDrawOrder
                    && boundsArea < selectedBoundsArea)) {
            selectedDrawOrder = polygon.drawOrder;
            selectedBoundsArea = boundsArea;
            selectedPolygon = &polygon;
        }
    }
    if(selectedPolygon == NULL) {
        progress.close();
        GuiFunct::showEditorNotice(this, "Plant PolyVeg",
            "No cached PolyVeg polygon exists beneath the pointer.");
        return;
    }

    const ForestRecipeDefinition *selectedRecipe = NULL;
    if(!polyVegRecipeId.isEmpty()) {
        // The visible OSM category chooses the planting boundary. The helper's
        // Definition is an explicit operator choice of what to plant there.
        for(const ForestRecipeDefinition &candidateRecipe
                : catalogResult.catalog.polyVeg) {
            if(candidateRecipe.id == polyVegRecipeId) {
                selectedRecipe = &candidateRecipe;
                break;
            }
        }
    } else {
        // Retain automatic matching only when no helper definition was chosen.
        for(const ForestRecipeDefinition &candidateRecipe
                : catalogResult.catalog.polyVeg) {
            if(osmRecipeMatches(candidateRecipe, *selectedPolygon)
                    && (selectedRecipe == NULL
                        || candidateRecipe.osmPriority > selectedRecipe->osmPriority))
                selectedRecipe = &candidateRecipe;
        }
        // A single route definition is unambiguous even when the visible map
        // category has no automatic OSM match (for example light-green
        // grassland planted with a route's only mixed-woodland definition).
        if(selectedRecipe == NULL && catalogResult.catalog.polyVeg.size() == 1)
            selectedRecipe = &catalogResult.catalog.polyVeg.first();
    }
    if(selectedRecipe == NULL) {
        progress.close();
        GuiFunct::showEditorNotice(this, "Plant PolyVeg",
            QString("No usable PolyVeg definition is selected for the visible "
                    "'%1' polygon beneath the pointer.")
                .arg(selectedPolygon->category));
        return;
    }

    const ForestRecipeDefinition &recipe = *selectedRecipe;
    QVector<ForestPlantingBoundary> plantingPieces;
    QStringList plantingPieceKeys;
    const double tileMinimumX = pointerTileX*2048.0 - 1024.0;
    const double tileMaximumX = tileMinimumX + 2048.0;
    const double tileMinimumZ = pointerTileZ*2048.0 - 1024.0;
    const double tileMaximumZ = tileMinimumZ + 2048.0;
    if(polyVegFloodFill) {
        progress.setLabelText("Preparing Flood Fill polygons on the pointer tile...");
        for(int polygonIndex = 0; polygonIndex < cacheResult.polygons.size();
                ++polygonIndex) {
            if((polygonIndex & 0x3F) == 0) {
                progress.setValue(500 + (cacheResult.polygons.isEmpty() ? 0
                    : polygonIndex*350/cacheResult.polygons.size()));
                QApplication::processEvents();
            }
            const ForestOsmPolygon &polygon = cacheResult.polygons[polygonIndex];
            if(polyVegFloodKey(polygon) != polyVegFloodKey(*selectedPolygon)
                    || polygon.maximumX < tileMinimumX
                    || polygon.minimumX > tileMaximumX
                    || polygon.maximumZ < tileMinimumZ
                    || polygon.minimumZ > tileMaximumZ)
                continue;
            const QVector<ForestPlantingBoundary> clipped =
                clipPolyVegBoundaryToTile(
                    polygon.boundary, pointerTileX, pointerTileZ);
            for(int clippedIndex = 0; clippedIndex < clipped.size(); ++clippedIndex) {
                plantingPieces.append(clipped[clippedIndex]);
                plantingPieceKeys.append(polygon.featureId
                    + QStringLiteral("/tile/")
                    + QString::number(pointerTileX) + QStringLiteral("/")
                    + QString::number(pointerTileZ) + QStringLiteral("/")
                    + QString::number(clippedIndex));
            }
        }
    } else {
        const QVector<ForestPlantingBoundary> clipped =
            clipPolyVegBoundaryToTile(
                selectedPolygon->boundary, pointerTileX, pointerTileZ);
        for(int clippedIndex = 0; clippedIndex < clipped.size(); ++clippedIndex) {
            plantingPieces.append(clipped[clippedIndex]);
            plantingPieceKeys.append(selectedPolygon->featureId
                + QStringLiteral("/tile/")
                + QString::number(pointerTileX) + QStringLiteral("/")
                + QString::number(pointerTileZ) + QStringLiteral("/")
                + QString::number(clippedIndex));
        }
    }
    if(plantingPieces.isEmpty()) {
        progress.close();
        GuiFunct::showEditorNotice(this, "Plant PolyVeg",
            "The selected PolyVeg surface has no plantable area inside the pointer tile.");
        return;
    }

    ForestGenerationSettings settings;
    settings.densityPerSquareMetre = polyVegDensity > 0.0
        ? polyVegDensity : recipe.defaultDensityPerSquareMetre;
    settings.maximumTrees = polyVegMaximumTrees > 0
        ? polyVegMaximumTrees : recipe.defaultMaximumTrees;
    const int selectedMaximumTrees = settings.maximumTrees;
    if(!polyVegRowsEnabled
            && (settings.densityPerSquareMetre
                < recipe.densityLimitsPerSquareMetre.minimum
                || settings.densityPerSquareMetre
                > recipe.densityLimitsPerSquareMetre.maximum)) {
        progress.close();
        GuiFunct::showEditorStopped(this, "PolyVeg Density",
            QString("The requested density is outside this definition's allowed range.\n\n"
                    "Requested: %1 trees/km2\nAllowed: %2-%3 trees/km2\n\n"
                    "Choose a density inside the displayed range and retry.")
                .arg(settings.densityPerSquareMetre*1000000.0, 0, 'f', 0)
                .arg(recipe.densityLimitsPerSquareMetre.minimum*1000000.0, 0, 'f', 0)
                .arg(recipe.densityLimitsPerSquareMetre.maximum*1000000.0, 0, 'f', 0));
        return;
    }
    settings.rowsEnabled = polyVegRowsEnabled;
    settings.rowWidthMetres = polyVegRowWidthMetres;
    settings.rowSpacingMetres = polyVegRowSpacingMetres;
    settings.rowDirectionDegrees = polyVegRowDirectionDegrees;
    PolyVegDatabaseClearance trackClearance(
        Game::trackDB, recipe.defaultTrackClearanceMetres);
    PolyVegDatabaseClearance roadClearance(
        Game::roadDB, recipe.defaultRoadClearanceMetres);
    PolyVegWaterClearance waterClearance(
        recipe.defaultWaterClearanceMetres);
    int totalSlopeRejections = 0;
    int totalTrackRejections = 0;
    int totalRoadRejections = 0;
    int totalWaterRejections = 0;
    settings.acceptsTerrain = [&recipe, &trackClearance, &roadClearance,
            &waterClearance,
            &totalSlopeRejections, &totalTrackRejections,
            &totalRoadRejections, &totalWaterRejections](double x, double z) {
        if(!polyVegTerrainSlopeAccepted(x, z, recipe.maximumSlopeDegrees)) {
            ++totalSlopeRejections;
            return false;
        }
        float terrainY = 0.0f;
        if(!polyVegTerrainHeight(x, z, terrainY)) {
            ++totalSlopeRejections;
            return false;
        }
        if(trackClearance.blocks(x, terrainY, z)) {
            ++totalTrackRejections;
            return false;
        }
        if(roadClearance.blocks(x, terrainY, z)) {
            ++totalRoadRejections;
            return false;
        }
        if(waterClearance.blocks(x, z)) {
            ++totalWaterRejections;
            return false;
        }
        return true;
    };
    QVector<ForestCandidate> generatedCandidates;
    double usableAreaSquareMetres = 0.0;
    int totalAttempts = 0;
    int totalRequested = 0;
    int totalGenerationTargets = 0;
    bool pieceOperationLimitApplied = false;
    QStringList generationErrors;
    int skippedUnusablePieces = 0;
    progress.setRange(0, qMax(1, plantingPieces.size()) * 1000 + 1000);
    progress.setLabelText("Preparing carved PolyVeg polygons...");
    progress.setValue(0);
    QApplication::processEvents();
    QElapsedTimer candidateGenerationTimer;
    candidateGenerationTimer.start();
    for(int pieceIndex = 0; pieceIndex < plantingPieces.size(); ++pieceIndex) {
        // The operator's Maximum trees applies once to the complete visible
        // PolyVeg feature, not independently to each exclusion-carved piece.
        settings.maximumTrees = 0;
        settings.seed = static_cast<std::uint64_t>(qHash(
            plantingPieceKeys[pieceIndex] + QStringLiteral("/osm-forest/piece/")
            + QString::number(pieceIndex), static_cast<size_t>(polyVegSeed)));
        settings.progress = [&progress, pieceIndex, &plantingPieces](
                int attempts, int maximumAttempts, int accepted, int target) {
            const int pieceProgress = maximumAttempts > 0
                ? qBound(0, attempts * 1000 / maximumAttempts, 1000) : 0;
            progress.setValue(pieceIndex * 1000 + pieceProgress);
            progress.setLabelText(QString(
                "Evaluating carved polygon %1 of %2...\n"
                "%3 of %4 candidates accepted; %5 attempts")
                .arg(pieceIndex + 1).arg(plantingPieces.size())
                .arg(accepted).arg(target).arg(attempts));
            QApplication::processEvents();
        };
        const ForestGenerationResult pieceResult = ForestGenerator::generate(
            recipe, plantingPieces[pieceIndex], settings);
        if(!pieceResult.errors.isEmpty()) {
            generationErrors.append(pieceResult.errors);
            ++skippedUnusablePieces;
            continue;
        }
        usableAreaSquareMetres += pieceResult.usableAreaSquareMetres;
        totalAttempts += pieceResult.attempts;
        totalRequested += pieceResult.requestedCount;
        totalGenerationTargets += pieceResult.targetCount;
        pieceOperationLimitApplied = pieceOperationLimitApplied
            || pieceResult.objectLimitApplied;
        generatedCandidates.append(pieceResult.candidates);
    }
    settings.progress = {};
    if(generatedCandidates.isEmpty()) {
        progress.close();
        GuiFunct::showEditorStopped(this, "Plant PolyVeg",
            generationErrors.isEmpty()
                ? "The PolyVeg polygon beneath the pointer produced no planting candidates."
                : generationErrors.join("\n"));
        return;
    }
    const qint64 candidateGenerationMilliseconds =
        candidateGenerationTimer.elapsed();

    const int generatedBeforeLimit = generatedCandidates.size();
    if(generatedCandidates.size() > selectedMaximumTrees) {
        QVector<ForestCandidate> limited;
        limited.reserve(selectedMaximumTrees);
        const double stride = static_cast<double>(generatedCandidates.size())
            / selectedMaximumTrees;
        for(int index = 0; index < selectedMaximumTrees; ++index)
            limited.append(generatedCandidates.at(std::min(
                static_cast<int>(std::floor(index*stride)),
                static_cast<int>(generatedCandidates.size()) - 1)));
        generatedCandidates = limited;
    }

    QStringList limitations;
    if(pieceOperationLimitApplied || totalRequested > selectedMaximumTrees)
        limitations.append(polyVegRowsEnabled
            ? QString("The row layout called for %1 plants, but this definition "
                      "allows at most %2 in one planting operation.")
                .arg(totalRequested).arg(selectedMaximumTrees)
            : QString("The requested density called for %1 trees, but this definition "
                      "allows at most %2 trees in one planting operation.")
                .arg(totalRequested).arg(selectedMaximumTrees));
    if(generatedBeforeLimit < totalGenerationTargets)
        limitations.append(QString(
            "Placement rules accepted only %1 of %2 attempted target positions "
            "before the generation-attempt limit was reached.")
            .arg(generatedBeforeLimit).arg(totalGenerationTargets));
    if(skippedUnusablePieces > 0)
        limitations.append(QString(
            "%1 carved polygon piece(s) had no usable planting area and were skipped.")
            .arg(skippedUnusablePieces));
    if(totalTrackRejections > 0)
        limitations.append(QString(
            "%1 candidate position(s) were rejected within the %2 m TDB clearance.")
            .arg(totalTrackRejections)
            .arg(recipe.defaultTrackClearanceMetres, 0, 'f', 1));
    if(totalRoadRejections > 0)
        limitations.append(QString(
            "%1 candidate position(s) were rejected within the %2 m RDB clearance.")
            .arg(totalRoadRejections)
            .arg(recipe.defaultRoadClearanceMetres, 0, 'f', 1));
    if(totalWaterRejections > 0)
        limitations.append(QString(
            "%1 candidate position(s) were rejected within the %2 m submerged-water clearance.")
            .arg(totalWaterRejections)
            .arg(recipe.defaultWaterClearanceMetres, 0, 'f', 1));
    int placedCount = 0;
    int skippedTerrain = 0;
    int skippedBakedTiles = 0;
    progress.setRange(0, qMax(1, generatedCandidates.size()));
    progress.setValue(0);
    progress.setLabelText("Placing PolyVeg on route terrain...");
    progress.show();
    QApplication::processEvents();
    Undo::StateBegin();
    QHash<quint64, bool> bakedTileStatus;
    // The pointer tile was checked before candidate generation. Cache that
    // result so planting thousands of raw objects cannot turn the bake guard
    // into an ever-growing scan of the same world-tile object array.
    bakedTileStatus.insert(polyVegTileKey(pointerTileX, pointerTileZ), false);
    QVector<Ref::RefItem> placementReferences;
    placementReferences.reserve(recipe.vegetation.size());
    for(const ForestVegetationDefinition &vegetation : recipe.vegetation) {
        Ref::RefItem reference;
        reference.type = QStringLiteral("static");
        reference.clas = QStringLiteral("OSM Forest");
        reference.filename.append(vegetation.shape);
        placementReferences.append(reference);
    }
    QElapsedTimer placementProgressTimer;
    placementProgressTimer.start();
    QElapsedTimer placementOperationTimer;
    placementOperationTimer.start();
    for(int candidateIndex = 0; candidateIndex < generatedCandidates.size();
            ++candidateIndex) {
        const ForestCandidate &candidate = generatedCandidates[candidateIndex];
        if(candidateIndex == 0 || placementProgressTimer.elapsed() >= 125) {
            progress.setValue(candidateIndex);
            progress.setLabelText(QString(
                "Placing PolyVeg on route terrain...\n%1 of %2 objects")
                .arg(candidateIndex).arg(generatedCandidates.size()));
            QApplication::processEvents();
            placementProgressTimer.restart();
        }
        int tileX = tileForPlanCoordinate(candidate.x);
        int tileZ = tileForPlanCoordinate(candidate.z);
        const quint64 candidateTileKey = polyVegTileKey(tileX, tileZ);
        auto bakedIt = bakedTileStatus.constFind(candidateTileKey);
        if(bakedIt == bakedTileStatus.constEnd()) {
            const bool baked = tileHasPolyVegBake(tileX, tileZ);
            bakedTileStatus.insert(candidateTileKey, baked);
            bakedIt = bakedTileStatus.constFind(candidateTileKey);
        }
        if(bakedIt.value()) {
            ++skippedBakedTiles;
            continue;
        }
        float position[3] {
            static_cast<float>(candidate.x - tileX*2048.0),
            0.0f,
            static_cast<float>(candidate.z - tileZ*2048.0)
        };
        Game::check_coords(tileX, tileZ, position);
        if(!Game::terrainLib->load(tileX, tileZ)) {
            ++skippedTerrain;
            continue;
        }
        const float terrainHeight = Game::terrainLib->getHeight(
            tileX, tileZ, position[0], position[2], false);
        if(!std::isfinite(terrainHeight)) {
            ++skippedTerrain;
            continue;
        }
        const ForestVegetationDefinition &vegetation =
            recipe.vegetation.at(candidate.vegetationIndex);
        position[1] = terrainHeight - (vegetation.hasPlantingDepth
            ? static_cast<float>(vegetation.plantingDepthMetres) : 0.0f);

        Ref::RefItem &reference =
            placementReferences[candidate.vegetationIndex];
        float rotation[4];
        Quat::fill(rotation);
        Quat::rotateY(rotation, rotation,
            static_cast<float>(candidate.yawDegrees*M_PI/180.0));
        WorldObj *placed = route->placeObject(
            tileX, tileZ, position, rotation, 0.0f, &reference);
        if(placed == NULL) continue;
        placed->setUniformMatrixScale(static_cast<float>(candidate.uniformScale));
        ++placedCount;
    }
    Undo::StateEnd();
    const qint64 placementMilliseconds = placementOperationTimer.elapsed();
    progress.setValue(generatedCandidates.size());
    progress.close();
    viewportFreeze.finish();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    if(placedCount == 0) {
        GuiFunct::showEditorStopped(this, "Plant PolyVeg",
            "No objects could be placed on loaded route terrain.");
        return;
    }
    emit sendMsg(QString("msg"), QString(
        "PolyVeg planted %1 objects across %2 carved piece(s) from feature %3. "
        "Candidate generation: %4 s; placement: %5 s.")
        .arg(placedCount).arg(plantingPieces.size()).arg(selectedPolygon->featureId)
        .arg(candidateGenerationMilliseconds / 1000.0, 0, 'f', 2)
        .arg(placementMilliseconds / 1000.0, 0, 'f', 2));
    QStringList postNotes = limitations;
    if(skippedTerrain > 0)
        postNotes.append(QString("%1 tree(s) were skipped because route terrain was unavailable.")
            .arg(skippedTerrain));
    if(skippedBakedTiles > 0)
        postNotes.append(QString("%1 tree(s) were blocked by baked tiles.")
            .arg(skippedBakedTiles));
    const int otherSkips = generatedCandidates.size()-placedCount
        -skippedTerrain-skippedBakedTiles;
    if(otherSkips > 0)
        postNotes.append(QString("%1 tree object(s) could not be created.")
            .arg(otherSkips));
    const QString postLimitText = postNotes.isEmpty()
        ? QString() : QString("\n\nLIMITS / SKIPS\n%1").arg(postNotes.join("\n"));
    QTimer::singleShot(0, this,
        &RouteEditorGLWidget::refreshPolyVegTileCounts);
    if(polyVegDisablePlantReport) {
        // The shared menu/button sound runs after this long synchronous action
        // returns. Queue the success cue so it remains the final audible result.
        queuePolyVegSuccessSound();
        return;
    }
    GuiFunct::showEditorNotice(this, "Plant PolyVeg",
        QString("Planted %1 scaled vegetation objects across all %2 carved piece(s) "
                "of the selected PolyVeg feature.\n\nFeature: %3\nTerrain skips: %4\n"
                "Candidates before operation limit: %5\nGeneration attempts: %6\n"
                "Generation time: %7 s\nPlacement time: %8 s\n"
                "Blocked by baked tiles: %9\nTDB rejects: %10\nRDB rejects: %11\n"
                "Water rejects: %12%13\n\n"
                "The camera position was preserved. Save normally to keep it, or Undo "
                "to remove the operation.")
            .arg(placedCount).arg(plantingPieces.size()).arg(selectedPolygon->featureId)
            .arg(skippedTerrain).arg(generatedBeforeLimit).arg(totalAttempts)
            .arg(candidateGenerationMilliseconds / 1000.0, 0, 'f', 2)
            .arg(placementMilliseconds / 1000.0, 0, 'f', 2)
            .arg(skippedBakedTiles).arg(totalTrackRejections)
            .arg(totalRoadRejections).arg(totalWaterRejections)
            .arg(postLimitText));
    queuePolyVegSuccessSound();
}

void RouteEditorGLWidget::setPolyVegSettings(QString recipeId,
                                             double density,
                                             int maximumTrees,
                                             quint64 seed,
                                             bool floodFill,
                                             bool disablePlantReport,
                                             bool rowsEnabled,
                                             double rowWidthMetres,
                                             double rowSpacingMetres,
                                             double rowDirectionDegrees) {
    polyVegRecipeId = recipeId;
    polyVegDensity = density;
    polyVegMaximumTrees = maximumTrees;
    polyVegSeed = seed;
    polyVegFloodFill = floodFill;
    polyVegDisablePlantReport = disablePlantReport;
    polyVegRowsEnabled = rowsEnabled;
    polyVegRowWidthMetres = rowWidthMetres;
    polyVegRowSpacingMetres = rowSpacingMetres;
    polyVegRowDirectionDegrees = rowDirectionDegrees;
}

void RouteEditorGLWidget::plantConfiguredPolyVeg() {
    plantNearestOsmForest();
}

void RouteEditorGLWidget::requestPolyVegHelper() {
    emit polyVegHelperRequested();
}

bool RouteEditorGLWidget::tileHasPolyVegBake(int tileX, int tileZ) {
    if(route == NULL) return false;
    Tile *worldTile = route->requestTile(tileX, tileZ, false);
    if(worldTile == NULL || worldTile->loaded != 1) return false;
    for(int index = 0; index < worldTile->jestObiektow; ++index) {
        WorldObj *object = worldTile->obiekty[index];
        if(object != NULL && object->loaded
                && object->typeID == WorldObj::sstatic
                && PolyVegObject::isBakeShape(object->fileName))
            return true;
    }
    return false;
}

void RouteEditorGLWidget::placePolyVegRuler(
        double widthMetres, bool closedShape) {
    if(route == NULL)
        return;
    polyVegRulerArea = closedShape;
    polyVegRulerWidth = std::clamp(
        widthMetres, closedShape ? 0.0 : 1.0, 2000.0);
    removePolyVegRuler();
    RulerObj *selectedRuler = selectedObj != NULL
        ? dynamic_cast<RulerObj*>(selectedObj) : NULL;
    const bool selectedSpecialRuler = selectedRuler != NULL
        && selectedRuler->isSpecialRuler();
    route->deleteSpecialRulers();
    if(selectedSpecialRuler)
        setSelectedObj(NULL);
    activeWaterRuler = NULL;
    activeVegetationRuler = NULL;
    activeGradeRuler = NULL;
    setSpecialRulerPanelControlsActive(true);
    enableTool("vegetationRulerTool");
    emit polyVegPanelStatus(
        closedShape
            ? "New Area ruler: click to add points."
            : "New Corridor ruler: click to add points.");
}

void RouteEditorGLWidget::addPolyVegRulerPoints() {
    if(route == NULL)
        return;
    if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
        activeVegetationRuler = route->findVegetationRuler(false);
    if(activeVegetationRuler == NULL){
        emit polyVegPanelStatus("No ruler. Choose New Ruler.");
        return;
    }
    setSpecialRulerPanelControlsActive(true);
    if(selectedObj != NULL && selectedObj != activeVegetationRuler)
        selectedObj->unselect();
    setSelectedObj(activeVegetationRuler, false);
    activeVegetationRuler->select(
        qMax(0, activeVegetationRuler->getPointCount() - 1));
    enableTool("vegetationRulerTool");
    emit polyVegPanelStatus(
        QString("Add Points: %1 points.")
        .arg(activeVegetationRuler->getPointCount()));
}

void RouteEditorGLWidget::editPolyVegRulerPoints() {
    if(route == NULL)
        return;
    if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
        activeVegetationRuler = route->findVegetationRuler(false);
    if(activeVegetationRuler == NULL){
        emit polyVegPanelStatus("No ruler. Choose New Ruler.");
        return;
    }
    setSpecialRulerPanelControlsActive(true);
    enableTool("selectTool");
    if(selectedObj != NULL && selectedObj != activeVegetationRuler)
        selectedObj->unselect();
    setSelectedObj(activeVegetationRuler, false);
    emit polyVegPanelStatus("Edit Points: drag a control point.");
}

void RouteEditorGLWidget::setPolyVegRulerWidth(double widthMetres) {
    polyVegRulerWidth = std::clamp(
        widthMetres, polyVegRulerArea ? 0.0 : 1.0, 2000.0);
    if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
        activeVegetationRuler = route != NULL
            ? route->findVegetationRuler(false) : NULL;
    if(activeVegetationRuler != NULL)
        activeVegetationRuler->setVegetationWidth(
            static_cast<float>(polyVegRulerWidth));
}

void RouteEditorGLWidget::setPolyVegRulerArea(bool closedShape) {
    polyVegRulerArea = closedShape;
    if(!closedShape)
        polyVegRulerWidth = std::max(1.0, polyVegRulerWidth);
    if(activeVegetationRuler == NULL || !activeVegetationRuler->loaded)
        activeVegetationRuler = route != NULL
            ? route->findVegetationRuler(false) : NULL;
    if(activeVegetationRuler != NULL)
        activeVegetationRuler->setVegetationArea(closedShape);
    emit polyVegPanelStatus(closedShape ? "Area mode." : "Corridor mode.");
}

void RouteEditorGLWidget::removePolyVegRuler() {
    setSpecialRulerPanelControlsActive(false);
    if(route == NULL) return;
    RulerObj *ruler = route->findVegetationRuler(false);
    if(ruler != NULL) {
        if(selectedObj == ruler) setSelectedObj(NULL);
        route->deleteObj(ruler);
    }
    activeVegetationRuler = NULL;
    if(toolEnabled == "vegetationRulerTool") enableTool("selectTool");
}

void RouteEditorGLWidget::plantPolyVegRuler(bool overrideForestCoverage) {
    if(route == NULL) return;
    RulerObj *ruler = route->findVegetationRuler(false);
    const bool areaMode = ruler != NULL && ruler->isVegetationArea();
    const int minimumPoints = areaMode ? 3 : 2;
    if(ruler == NULL || ruler->getPointCount() < minimumPoints) {
        GuiFunct::showEditorNotice(this, "Plant Ruler (polyveg)",
            areaMode
                ? "An Area ruler requires at least three control points."
                : "Place a Ruler (polyveg) with at least two control points first.");
        return;
    }
    if(areaMode && !ruler->hasValidVegetationArea()) {
        GuiFunct::showEditorStopped(this, "Plant Area (polyveg)",
            "The area cannot be planted because its boundary crosses or "
            "overlaps itself. Move the control points until the closed "
            "light-green outline forms one simple polygon.");
        return;
    }

    QProgressDialog preparation("Loading PolyVeg planting data...", QString(),
                                0, 1000, this);
    preparation.setWindowTitle(Game::AppName);
    preparation.setWindowModality(Qt::WindowModal);
    preparation.setAutoClose(false);
    preparation.setAutoReset(false);
    preparation.setMinimumDuration(0);
    preparation.setValue(0);
    preparation.setProperty("scoCenterOnScreen", true);
    GuiFunct::styleEditorDialog(&preparation);
    preparation.show();
    QApplication::processEvents();
    PolyVegViewportFreeze generationViewportFreeze(this);

    const QString routePath = Game::root + "/routes/" + Game::route;
    const ForestCatalogLoadResult catalogResult =
        ForestDefinitionLoader::loadRoute(routePath);
    const ForestOsmCacheLoadResult cacheResult = overrideForestCoverage
        ? ForestOsmCache::loadFile(routePath
            + "/osm_data/polyveg-exclusions.geojson")
        : ForestOsmCache::loadRoute(routePath);
    preparation.setValue(120);
    QApplication::processEvents();
    if(!catalogResult.isValid() || !cacheResult.isValid()) {
        preparation.close();
        GuiFunct::showEditorStopped(this, "Plant Ruler (polyveg)",
            overrideForestCoverage
                ? "polyveg.json and the SCO LIDEX PolyVeg exclusion cache are required "
                  "for Ruler (polyveg) override planting."
                : "polyveg.json and the SCO LIDEX PolyVeg polygon cache are required so "
                  "ruler planting can use the same exclusions as polygon planting.");
        return;
    }
    const ForestRecipeDefinition *recipe = NULL;
    for(const ForestRecipeDefinition &candidate : catalogResult.catalog.polyVeg)
        if(candidate.id == polyVegRecipeId) { recipe = &candidate; break; }
    if(recipe == NULL) {
        preparation.close();
        GuiFunct::showEditorStopped(this, "Plant Ruler (polyveg)",
            "The selected PolyVeg definition is unavailable.");
        return;
    }

    struct OffsetPoint { double x; double z; };
    QVector<OffsetPoint> points;
    for(int index = 0; index < ruler->getPointCount(); ++index) {
        float world[3] {0.0f, 0.0f, 0.0f};
        ruler->getPointWorldPosition(index, world);
        points.append({world[0], world[2]});
    }
    const double halfWidth = ruler->getVegetationWidth()*0.5;
    ForestPlantingBoundary corridor;
    if(areaMode) {
        QPainterPath polygonPath;
        polygonPath.moveTo(points.first().x, points.first().z);
        for(int index = 1; index < points.size(); ++index)
            polygonPath.lineTo(points[index].x, points[index].z);
        polygonPath.closeSubpath();
        QPainterPath bufferedPath = polygonPath;
        if(ruler->getVegetationWidth() > 0.0f) {
            QPainterPathStroker stroker;
            stroker.setWidth(ruler->getVegetationWidth()*2.0);
            stroker.setJoinStyle(Qt::RoundJoin);
            stroker.setCapStyle(Qt::RoundCap);
            bufferedPath = polygonPath.united(stroker.createStroke(polygonPath));
        }
        const QList<QPolygonF> bufferedPolygons = bufferedPath.toFillPolygons();
        double largestArea = -1.0;
        for(const QPolygonF &polygon : bufferedPolygons) {
            double twiceArea = 0.0;
            for(int index = 0; index < polygon.size(); ++index) {
                const QPointF &a = polygon[index];
                const QPointF &b = polygon[(index+1)%polygon.size()];
                twiceArea += a.x()*b.y()-b.x()*a.y();
            }
            if(std::fabs(twiceArea) <= largestArea)
                continue;
            ForestPlanRing outline;
            outline.reserve(polygon.size());
            for(const QPointF &point : polygon)
                outline.append({point.x(), point.y()});
            corridor.outer = outline;
            largestArea = std::fabs(twiceArea);
        }
    } else {
    QVector<OffsetPoint> normals(points.size()-1);
    for(int index = 0; index < points.size()-1; ++index) {
        const double dx = points[index+1].x-points[index].x;
        const double dz = points[index+1].z-points[index].z;
        const double length = std::hypot(dx, dz);
        normals[index] = length > 0.001
            ? OffsetPoint{-dz/length, dx/length}
            : (index > 0 ? normals[index-1] : OffsetPoint{0.0, 1.0});
    }
    QVector<OffsetPoint> left(points.size()), right(points.size());
    left[0] = {points[0].x+normals[0].x*halfWidth,
               points[0].z+normals[0].z*halfWidth};
    right[0] = {points[0].x-normals[0].x*halfWidth,
                points[0].z-normals[0].z*halfWidth};
    for(int index = 1; index < points.size()-1; ++index) {
        double mx = normals[index-1].x+normals[index].x;
        double mz = normals[index-1].z+normals[index].z;
        const double denominator = mx*normals[index].x+mz*normals[index].z;
        double scale = std::fabs(denominator) > 0.05
            ? halfWidth/denominator : halfWidth;
        if(std::fabs(scale) > halfWidth*10.0) {
            mx = normals[index].x; mz = normals[index].z; scale = halfWidth;
        }
        left[index] = {points[index].x+mx*scale, points[index].z+mz*scale};
        right[index] = {points[index].x-mx*scale, points[index].z-mz*scale};
    }
    const int last = points.size()-1;
    left[last] = {points[last].x+normals[last-1].x*halfWidth,
                  points[last].z+normals[last-1].z*halfWidth};
    right[last] = {points[last].x-normals[last-1].x*halfWidth,
                   points[last].z-normals[last-1].z*halfWidth};
    for(const OffsetPoint &point : left) corridor.outer.append({point.x, point.z});
    for(int index = right.size()-1; index >= 0; --index)
        corridor.outer.append({right[index].x, right[index].z});
    }

    const int rulerTileX = tileForPlanCoordinate(points.first().x);
    const int rulerTileZ = tileForPlanCoordinate(points.first().z);
    const QVector<ForestPlantingBoundary> clipped =
        clipPolyVegBoundaryToTile(corridor, rulerTileX, rulerTileZ);
    if(clipped.isEmpty()) {
        preparation.close();
        GuiFunct::showEditorStopped(this,
            areaMode ? "Plant Area (polyveg)" : "Plant Ruler (polyveg)",
            "The planting boundary has no usable area inside its first tile.");
        return;
    }
    corridor = clipped.first();
    preparation.setValue(180);
    preparation.setLabelText("Generating PolyVeg candidates...");
    QApplication::processEvents();

    ForestGenerationSettings settings;
    settings.densityPerSquareMetre = polyVegDensity > 0.0
        ? polyVegDensity : recipe->defaultDensityPerSquareMetre;
    settings.maximumTrees = polyVegMaximumTrees > 0
        ? polyVegMaximumTrees : recipe->defaultMaximumTrees;
    settings.rowsEnabled = polyVegRowsEnabled;
    settings.rowWidthMetres = polyVegRowWidthMetres;
    settings.rowSpacingMetres = polyVegRowSpacingMetres;
    settings.rowDirectionDegrees = polyVegRowDirectionDegrees;
    PolyVegDatabaseClearance trackClearance(
        Game::trackDB, recipe->defaultTrackClearanceMetres);
    PolyVegDatabaseClearance roadClearance(
        Game::roadDB, recipe->defaultRoadClearanceMetres);
    PolyVegWaterClearance waterClearance(
        recipe->defaultWaterClearanceMetres);
    int rejectedBySlope = 0;
    int rejectedByTrack = 0;
    int rejectedByRoad = 0;
    int rejectedByWater = 0;
    settings.acceptsTerrain = [recipe, &trackClearance, &roadClearance,
            &waterClearance,
            &rejectedBySlope, &rejectedByTrack,
            &rejectedByRoad, &rejectedByWater](double x, double z) {
        if(!polyVegTerrainSlopeAccepted(x, z, recipe->maximumSlopeDegrees)) {
            ++rejectedBySlope;
            return false;
        }
        float terrainY = 0.0f;
        if(!polyVegTerrainHeight(x, z, terrainY)) {
            ++rejectedBySlope;
            return false;
        }
        if(trackClearance.blocks(x, terrainY, z)) {
            ++rejectedByTrack;
            return false;
        }
        if(roadClearance.blocks(x, terrainY, z)) {
            ++rejectedByRoad;
            return false;
        }
        if(waterClearance.blocks(x, z)) {
            ++rejectedByWater;
            return false;
        }
        return true;
    };
    settings.seed = static_cast<std::uint64_t>(polyVegSeed);
    settings.progress = [&preparation](
            int attempts, int maximumAttempts, int accepted, int target) {
        const int generationProgress = maximumAttempts > 0
            ? qBound(0, attempts*570/maximumAttempts, 570) : 0;
        preparation.setValue(180+generationProgress);
        preparation.setLabelText(QString(
            "Generating PolyVeg candidates...\n%1 of %2 accepted; %3 attempts")
            .arg(accepted).arg(target).arg(attempts));
        QApplication::processEvents();
    };
    ForestGenerationResult generated = ForestGenerator::generate(
        *recipe, corridor, settings);
    settings.progress = {};
    if(!generated.isValid()) {
        preparation.close();
        GuiFunct::showEditorStopped(this, "Plant Ruler (polyveg)",
            generated.errors.join("\n"));
        return;
    }

    QVector<ForestCandidate> candidates;
    int excludedByOsm = 0;
    int excludedByBake = 0;
    QVector<const ForestOsmPolygon*> tilePolygons;
    const double tileMinimumX = rulerTileX*2048.0-1024.0;
    const double tileMaximumX = tileMinimumX+2048.0;
    const double tileMinimumZ = rulerTileZ*2048.0-1024.0;
    const double tileMaximumZ = tileMinimumZ+2048.0;
    for(const ForestOsmPolygon &polygon : cacheResult.polygons) {
        if(polygon.maximumX < tileMinimumX || polygon.minimumX > tileMaximumX
                || polygon.maximumZ < tileMinimumZ
                || polygon.minimumZ > tileMaximumZ)
            continue;
        tilePolygons.append(&polygon);
    }
    QHash<quint64, bool> rulerBakedTileStatus;
    auto rulerTileIsBaked = [this, &rulerBakedTileStatus](int tileX, int tileZ) {
        const quint64 key = polyVegTileKey(tileX, tileZ);
        const auto cached = rulerBakedTileStatus.constFind(key);
        if(cached != rulerBakedTileStatus.constEnd())
            return cached.value();
        const bool baked = tileHasPolyVegBake(tileX, tileZ);
        rulerBakedTileStatus.insert(key, baked);
        return baked;
    };
    preparation.setValue(750);
    preparation.setLabelText("Applying PolyVeg coverage and baked-tile checks...");
    QApplication::processEvents();
    for(int candidateIndex = 0;
            candidateIndex < generated.candidates.size(); ++candidateIndex) {
        if((candidateIndex & 0xFF) == 0) {
            preparation.setValue(750 + (generated.candidates.isEmpty() ? 0
                : candidateIndex*250/generated.candidates.size()));
            QApplication::processEvents();
        }
        const ForestCandidate &candidate = generated.candidates[candidateIndex];
        const ForestPlanPoint point {candidate.x, candidate.z};
        bool matchedCachePolygon = false;
        for(const ForestOsmPolygon *polygon : tilePolygons) {
            if(candidate.x < polygon->minimumX || candidate.x > polygon->maximumX
                    || candidate.z < polygon->minimumZ || candidate.z > polygon->maximumZ)
                continue;
            if(pointInBoundary(point, polygon->boundary)) {
                matchedCachePolygon = true;
                break;
            }
        }
        const bool permitted = overrideForestCoverage
            ? !matchedCachePolygon : matchedCachePolygon;
        if(!permitted) { ++excludedByOsm; continue; }
        if(rulerTileIsBaked(tileForPlanCoordinate(candidate.x),
                            tileForPlanCoordinate(candidate.z))) {
            ++excludedByBake;
            continue;
        }
        candidates.append(candidate);
    }
    preparation.setValue(1000);
    preparation.close();
    generationViewportFreeze.finish();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    if(candidates.isEmpty()) {
        GuiFunct::showEditorNotice(this, "Plant Ruler (polyveg)",
            "No plantable positions remain after OSM, TDB/RDB/water "
            "clearance, slope and baked-tile checks.");
        return;
    }
    QString plantPrompt = QString(areaMode
        ? "Plant %1 objects inside the closed area plus its %2 m exterior buffer?\n\n"
        : "Plant %1 objects inside the %2 m ruler corridor?\n\n")
        .arg(candidates.size()).arg(ruler->getVegetationWidth(), 0, 'f', 0);
    plantPrompt += QString(
        "OSM exclusions: %1\nBlocked by baked tiles: %2\n"
        "Rejected above %3 degrees slope: %4\n"
        "TDB clearance rejects (%5 m): %6\n"
        "RDB clearance rejects (%7 m): %8\n"
        "Water clearance rejects (%9 m): %10")
        .arg(excludedByOsm).arg(excludedByBake)
        .arg(recipe->maximumSlopeDegrees, 0, 'f', 1)
        .arg(rejectedBySlope)
        .arg(recipe->defaultTrackClearanceMetres, 0, 'f', 1)
        .arg(rejectedByTrack)
        .arg(recipe->defaultRoadClearanceMetres, 0, 'f', 1)
        .arg(rejectedByRoad)
        .arg(recipe->defaultWaterClearanceMetres, 0, 'f', 1)
        .arg(rejectedByWater);
    if(polyVegRowsEnabled) {
        plantPrompt += QString("\nRows: %1 m wide, %2 m spacing at %3 degrees")
            .arg(polyVegRowWidthMetres > 0.0
                    ? polyVegRowWidthMetres
                    : recipe->minimumSeparationMetres, 0, 'f', 0)
            .arg(polyVegRowSpacingMetres > 0.0
                    ? polyVegRowSpacingMetres
                    : recipe->minimumSeparationMetres, 0, 'f', 0)
            .arg(polyVegRowDirectionDegrees, 0, 'f', 0);
    }
    if(!polyVegDisablePlantReport
            && !GuiFunct::confirmDestructiveAction(this,
                areaMode ? "Plant Area (polyveg)" : "Plant Ruler (polyveg)",
                plantPrompt))
        return;

    int placed = 0;
    int terrainSkips = 0;
    QVector<Ref::RefItem> rulerPlacementReferences;
    rulerPlacementReferences.reserve(recipe->vegetation.size());
    for(const ForestVegetationDefinition &vegetation : recipe->vegetation) {
        Ref::RefItem reference;
        reference.type = QStringLiteral("static");
        reference.clas = QStringLiteral("OSM Forest");
        reference.filename.append(vegetation.shape);
        rulerPlacementReferences.append(reference);
    }
    PolyVegViewportFreeze placementViewportFreeze(this);
    Undo::StateBegin();
    for(const ForestCandidate &candidate : candidates) {
        int tileX = tileForPlanCoordinate(candidate.x);
        int tileZ = tileForPlanCoordinate(candidate.z);
        // Preserve the baked-tile hard boundary while reusing the result for
        // every candidate on that tile during this raw-object-only operation.
        if(rulerTileIsBaked(tileX, tileZ)) continue;
        float position[3] {static_cast<float>(candidate.x-tileX*2048.0), 0.0f,
                           static_cast<float>(candidate.z-tileZ*2048.0)};
        Game::check_coords(tileX, tileZ, position);
        if(!Game::terrainLib->load(tileX, tileZ)) { ++terrainSkips; continue; }
        const float height = Game::terrainLib->getHeight(
            tileX, tileZ, position[0], position[2], false);
        if(!std::isfinite(height)) { ++terrainSkips; continue; }
        const ForestVegetationDefinition &vegetation =
            recipe->vegetation.at(candidate.vegetationIndex);
        position[1] = height-(vegetation.hasPlantingDepth
            ? static_cast<float>(vegetation.plantingDepthMetres) : 0.0f);
        Ref::RefItem &reference =
            rulerPlacementReferences[candidate.vegetationIndex];
        float rotation[4]; Quat::fill(rotation);
        Quat::rotateY(rotation, rotation,
            static_cast<float>(candidate.yawDegrees*M_PI/180.0));
        WorldObj *object = route->placeObject(
            tileX, tileZ, position, rotation, 0.0f, &reference);
        if(object == NULL) continue;
        object->setUniformMatrixScale(static_cast<float>(candidate.uniformScale));
        ++placed;
    }
    Undo::StateEnd();
    placementViewportFreeze.finish();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QTimer::singleShot(0, this,
        &RouteEditorGLWidget::refreshPolyVegTileCounts);
    emit sendMsg("msg", QString("%1 planted %2 objects; %3 terrain skips.")
        .arg(areaMode ? "Area (polyveg)" : "Ruler (polyveg)")
        .arg(placed).arg(terrainSkips));
    if(placed <= 0) {
        GuiFunct::showEditorStopped(this,
            areaMode ? "Plant Area (polyveg)" : "Plant Ruler (polyveg)",
            "No objects could be placed on loaded route terrain.");
        return;
    }
    if(polyVegDisablePlantReport) {
        queuePolyVegSuccessSound();
        return;
    }
    GuiFunct::showEditorNotice(this,
        areaMode ? "Plant Area (polyveg)" : "Plant Ruler (polyveg)",
        QString("Planted %1 vegetation objects.\n\nTerrain skips: %2\n"
                "OSM exclusions: %3\nBlocked by baked tiles: %4\n"
                "TDB rejects: %5\nRDB rejects: %6\nWater rejects: %7\n\n"
                "Save normally to keep the planting, or Undo to remove it.")
            .arg(placed).arg(terrainSkips).arg(excludedByOsm)
            .arg(excludedByBake).arg(rejectedByTrack).arg(rejectedByRoad)
            .arg(rejectedByWater));
    queuePolyVegSuccessSound();
}

void RouteEditorGLWidget::refreshPolyVegTileCounts() {
    if(route == NULL) return;
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(
        Game::root + "/routes/" + Game::route);
    QSet<QString> rawShapes;
    if(result.isValid())
        for(const ForestRecipeDefinition &recipe : result.catalog.polyVeg)
            for(const ForestVegetationDefinition &vegetation : recipe.vegetation)
                rawShapes.insert(vegetation.shape.toLower());

    int totalRawCount = 0;
    int totalBakeCount = 0;
    int totalBakedTileCount = 0;
    for(auto it = route->tile.constBegin(); it != route->tile.constEnd(); ++it) {
        Tile *candidateTile = it.value();
        if(candidateTile == NULL || candidateTile->loaded != 1) continue;
        int tileRaw = 0;
        int tileBake = 0;
        for(int index = 0; index < candidateTile->jestObiektow; ++index) {
            WorldObj *object = candidateTile->obiekty[index];
            if(object == NULL || !object->loaded) continue;
            QString shapeName = object->fileName;
            shapeName.replace('\\', '/');
            shapeName = shapeName.section('/', -1);
            if(rawShapes.contains(shapeName.toLower())) {
                ++tileRaw;
            } else if(PolyVegObject::isBakeShape(shapeName)) {
                ++tileBake;
            }
        }
        totalRawCount += tileRaw;
        totalBakeCount += tileBake;
        if(tileBake > 0) ++totalBakedTileCount;
    }
    emit polyVegTileCounts(
        totalRawCount, totalBakeCount, totalBakedTileCount);
}

QVector<QPair<int, int>> RouteEditorGLWidget::polyVegTiles(bool wantBaked) const {
    QVector<QPair<int, int>> matches;
    if(route == NULL) return matches;
    const ForestCatalogLoadResult result = ForestDefinitionLoader::loadRoute(
        Game::root + "/routes/" + Game::route);
    if(!result.isValid()) return matches;
    QSet<QString> rawShapes;
    for(const ForestRecipeDefinition &recipe : result.catalog.polyVeg)
        for(const ForestVegetationDefinition &vegetation : recipe.vegetation)
            rawShapes.insert(vegetation.shape.toLower());
    for(auto it = route->tile.constBegin(); it != route->tile.constEnd(); ++it) {
        Tile *worldTile = it.value();
        if(worldTile == NULL || worldTile->loaded != 1) continue;
        bool raw = false;
        bool baked = false;
        for(int index = 0; index < worldTile->jestObiektow; ++index) {
            WorldObj *object = worldTile->obiekty[index];
            if(object == NULL || !object->loaded
                    || object->typeID != WorldObj::sstatic) continue;
            raw = raw || rawShapes.contains(object->fileName.toLower());
            baked = baked || PolyVegObject::isBakeShape(object->fileName);
        }
        if((wantBaked && baked) || (!wantBaked && raw && !baked))
            matches.append(qMakePair(worldTile->x, worldTile->z));
    }
    return matches;
}

void RouteEditorGLWidget::jumpNextPolyVegTile(bool baked) {
    QVector<QPair<int, int>> &tiles = baked
        ? polyVegBakeJumpTiles : polyVegRawJumpTiles;
    int &index = baked ? polyVegBakeJumpIndex : polyVegRawJumpIndex;
    if(tiles.isEmpty()) {
        tiles = polyVegTiles(baked);
        if(tiles.isEmpty()) {
            GuiFunct::showEditorNotice(this, "PolyVeg Planter",
                baked ? "No loaded tiles contain PolyVeg - Bake."
                      : "No loaded tiles contain unbaked PolyVeg - Raw.");
            index = -1;
            return;
        }
        const int originX = camera != NULL && camera->pozT != NULL
            ? static_cast<int>(camera->pozT[0]) : 0;
        const int originZ = camera != NULL && camera->pozT != NULL
            ? static_cast<int>(camera->pozT[1]) : 0;
        std::sort(tiles.begin(), tiles.end(), [originX, originZ](
                const QPair<int, int> &a, const QPair<int, int> &b) {
            const qint64 adx = a.first-originX;
            const qint64 adz = a.second-originZ;
            const qint64 bdx = b.first-originX;
            const qint64 bdz = b.second-originZ;
            const qint64 distanceA = adx*adx+adz*adz;
            const qint64 distanceB = bdx*bdx+bdz*bdz;
            if(distanceA != distanceB) return distanceA < distanceB;
            return a.first == b.first ? a.second < b.second : a.first < b.first;
        });
        index = -1;
    }
    if(index + 1 >= tiles.size()) {
        GuiFunct::showEditorNotice(this, "PolyVeg Planter",
            QString("No farther PolyVeg - %1 tile remains. Double-click the "
                    "button to restart from the current view.")
                .arg(baked ? "Bake" : "Raw"));
        return;
    }
    ++index;
    const int tileX = tiles[index].first;
    const int tileZ = tiles[index].second;
    float height = Game::terrainLib->getHeight(tileX, tileZ, 0.0f, 0.0f, false);
    if(!std::isfinite(height)) height = 0.0f;
    if(!baked) {
        jumpTo(tileX, tileZ, 0.0f, height + 75.0f, 0.0f);
    } else if(camera != NULL) {
        // The bake marker is rendered 300 m above the tile centre. Retain the
        // operator's compass heading, stand off 100 m directly behind that
        // heading, and level the view so the marker lands at screen centre.
        const float heading = camera->getRotX();
        const double markerX = tileX*2048.0;
        const double markerZ = tileZ*2048.0;
        const double cameraPlanX = markerX - std::sin(heading)*100.0;
        const double cameraPlanZ = markerZ - std::cos(heading)*100.0;
        int cameraTileX = tileForPlanCoordinate(cameraPlanX);
        int cameraTileZ = tileForPlanCoordinate(cameraPlanZ);
        float localX = static_cast<float>(cameraPlanX-cameraTileX*2048.0);
        float localZ = static_cast<float>(cameraPlanZ-cameraTileZ*2048.0);
        Game::check_coords(cameraTileX, cameraTileZ, localX, localZ);
        Game::terrainLib->load(cameraTileX, cameraTileZ);
        camera->setPozT(cameraTileX, cameraTileZ);
        camera->setPos(localX, height + 300.0f, localZ);
        camera->setPlayerRot(heading, 0.0f);
    }
    refreshPolyVegTileCounts();
}

void RouteEditorGLWidget::jumpNextPolyVegRawTile() {
    jumpNextPolyVegTile(false);
}

void RouteEditorGLWidget::resetPolyVegRawJump() {
    polyVegRawJumpTiles.clear();
    polyVegRawJumpIndex = -1;
}

void RouteEditorGLWidget::jumpNextPolyVegBakeTile() {
    jumpNextPolyVegTile(true);
}

void RouteEditorGLWidget::resetPolyVegBakeJump() {
    polyVegBakeJumpTiles.clear();
    polyVegBakeJumpIndex = -1;
}

void RouteEditorGLWidget::renderPolyVegBakeMarkers() {
    if(!polyVegHelperVisible || route == NULL || camera == NULL
            || camera->pozT == NULL) return;
    if(polyVegBakeMarker == NULL) {
        polyVegBakeMarker = new OglObj();
        float cube[] {
            -1,-1, 1,  1,-1, 1,  1, 1, 1,  -1,-1, 1,  1, 1, 1, -1, 1, 1,
             1,-1,-1, -1,-1,-1, -1, 1,-1,   1,-1,-1, -1, 1,-1,  1, 1,-1,
            -1,-1,-1, -1,-1, 1, -1, 1, 1,  -1,-1,-1, -1, 1, 1, -1, 1,-1,
             1,-1, 1,  1,-1,-1,  1, 1,-1,   1,-1, 1,  1, 1,-1,  1, 1, 1,
            -1, 1, 1,  1, 1, 1,  1, 1,-1,  -1, 1, 1,  1, 1,-1, -1, 1,-1,
            -1,-1,-1,  1,-1,-1,  1,-1, 1,  -1,-1,-1,  1,-1, 1, -1,-1, 1
        };
        for(float &coordinate : cube) coordinate *= 18.0f;
        polyVegBakeMarker->setMaterial(0.2f, 0.85f, 0.35f);
        polyVegBakeMarker->initLitTriangles(cube, 108);
    }
    if(!selection) {
        Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180,
            float(this->width()) / this->height(), 0.2f,
            std::max(1.0f, static_cast<float>(Game::objectLod) * 3.0f));
        Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
        gluu->setMatrixUniforms();
    }
    for(auto it = route->tile.constBegin(); it != route->tile.constEnd(); ++it) {
        Tile *worldTile = it.value();
        if(worldTile == NULL || worldTile->loaded != 1) continue;
        bool hasBake = false;
        int firstBakeIndex = -1;
        for(int index = 0; index < worldTile->jestObiektow; ++index) {
            WorldObj *object = worldTile->obiekty[index];
            if(object != NULL && object->loaded
                    && object->typeID == WorldObj::sstatic
                    && PolyVegObject::isBakeShape(object->fileName)) {
                hasBake = true;
                firstBakeIndex = index;
                break;
            }
        }
        if(!hasBake) continue;
        const float height = Game::terrainLib->getHeight(
            worldTile->x, worldTile->z, 0.0f, 0.0f, false);
        if(!std::isfinite(height)) continue;
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix,
            2048.0f*(worldTile->x-camera->pozT[0]),
            height + 300.0f,
            2048.0f*(worldTile->z-camera->pozT[1]));
        gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform,
            *reinterpret_cast<float(*)[4][4]>(gluu->mvMatrix));
        int selectionColor = 0;
        if(selection) {
            const int dx = worldTile->x-static_cast<int>(camera->pozT[0]);
            const int dz = worldTile->z-static_cast<int>(camera->pozT[1]);
            if(dx < -1 || dx > 1 || dz < -1 || dz > 1
                    || firstBakeIndex < 0 || firstBakeIndex > 0xFFFF) {
                gluu->mvPopMatrix();
                continue;
            }
            const int worldWindow = (dx+1)*3+(dz+1)+1;
            selectionColor = (worldWindow << 20) | (firstBakeIndex << 4);
        }
        polyVegBakeMarker->render(selectionColor);
        gluu->mvPopMatrix();
    }
    if(!selection) {
        Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180,
            float(this->width()) / this->height(), 0.2f, Game::objectLod);
        Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
        gluu->setMatrixUniforms();
    }
}

void RouteEditorGLWidget::setPolyVegHelperVisible(bool visible) {
    polyVegHelperVisible = visible;
    if(visible) {
        QTimer::singleShot(0, this,
            &RouteEditorGLWidget::refreshPolyVegTileCounts);
    }
    update();
}

void RouteEditorGLWidget::selectPolyVegBakeTile(int tileX, int tileZ) {
    if(route == NULL || groupObj == NULL) return;
    Tile *worldTile = route->requestTile(tileX, tileZ, false);
    if(worldTile == NULL || worldTile->loaded != 1) return;
    if(selectedObj != NULL) selectedObj->unselect();
    groupObj->objects.clear();
    for(int index = 0; index < worldTile->jestObiektow; ++index) {
        WorldObj *object = worldTile->obiekty[index];
        if(object != NULL && object->loaded
                && object->typeID == WorldObj::sstatic
                && PolyVegObject::isBakeShape(object->fileName))
            groupObj->objects.append(object);
    }
    if(groupObj->count() == 0) {
        setSelectedObj(NULL);
        return;
    }
    groupObj->select();
    setSelectedObj(groupObj);
}

void RouteEditorGLWidget::pushPolyVegBakeMarkers() {
    if(!polyVegHelperVisible || route == NULL
            || camera == NULL || camera->pozT == NULL) return;
    if(polyVegBakeMarker == NULL) {
        polyVegBakeMarker = new OglObj();
        float cube[] {
            -1,-1, 1,  1,-1, 1,  1, 1, 1,  -1,-1, 1,  1, 1, 1, -1, 1, 1,
             1,-1,-1, -1,-1,-1, -1, 1,-1,   1,-1,-1, -1, 1,-1,  1, 1,-1,
            -1,-1,-1, -1,-1, 1, -1, 1, 1,  -1,-1,-1, -1, 1, 1, -1, 1,-1,
             1,-1, 1,  1,-1,-1,  1, 1,-1,   1,-1, 1,  1, 1,-1,  1, 1, 1,
            -1, 1, 1,  1, 1, 1,  1, 1,-1,  -1, 1, 1,  1, 1,-1, -1, 1,-1,
            -1,-1,-1,  1,-1,-1,  1,-1, 1,  -1,-1,-1,  1,-1, 1, -1,-1, 1
        };
        for(float &coordinate : cube) coordinate *= 18.0f;
        polyVegBakeMarker->setMaterial(0.2f, 0.85f, 0.35f);
        polyVegBakeMarker->initLitTriangles(cube, 108);
    }
    for(auto it = route->tile.constBegin(); it != route->tile.constEnd(); ++it) {
        Tile *worldTile = it.value();
        if(worldTile == NULL || worldTile->loaded != 1) continue;
        bool hasBake = false;
        for(int index = 0; index < worldTile->jestObiektow; ++index) {
            WorldObj *object = worldTile->obiekty[index];
            if(object != NULL && object->loaded
                    && object->typeID == WorldObj::sstatic
                    && PolyVegObject::isBakeShape(object->fileName)) {
                hasBake = true;
                break;
            }
        }
        if(!hasBake) continue;
        const float height = Game::terrainLib->getHeight(
            worldTile->x, worldTile->z, 0.0f, 0.0f, false);
        if(!std::isfinite(height)) continue;
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix,
            2048.0f*(worldTile->x-camera->pozT[0]), height + 300.0f,
            2048.0f*(worldTile->z-camera->pozT[1]));
        polyVegBakeMarker->pushRenderItem();
        gluu->mvPopMatrix();
    }
}

void RouteEditorGLWidget::bakeVegetationCurrentTile() {
    bakeVegetationTile(false);
}

void RouteEditorGLWidget::bakeVegetationPointerTile() {
    bakeVegetationTile(true);
}

static void configurePolyVegBakeDialog(QDialog &dialog, QLabel &status) {
    dialog.setWindowTitle(Game::AppName);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setModal(true);
    dialog.setProperty("scoCenterOnScreen", true);
    dialog.setMinimumWidth(360);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel("PROCESSING BAKE", &dialog);
    GuiFunct::styleEditorTitle(title);
    layout->addWidget(title);
    status.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    status.setWordWrap(true);
    layout->addWidget(&status);
    GuiFunct::styleEditorDialog(&dialog);
}

static void allowPolyVegBakePaint(QWidget *viewport, int milliseconds) {
    if(viewport != NULL)
        viewport->update();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    QEventLoop paintInterval;
    QTimer::singleShot(milliseconds, &paintInterval, &QEventLoop::quit);
    paintInterval.exec(QEventLoop::ExcludeUserInputEvents);

    if(viewport != NULL)
        viewport->update();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool RouteEditorGLWidget::bakeVegetationTile(bool usePointerTile) {
    if(route == NULL || camera == NULL || camera->pozT == NULL)
        return false;

    int tileX = static_cast<int>(camera->pozT[0]);
    int tileZ = static_cast<int>(camera->pozT[1]);
    if(polyVegBatchBake) {
        tileX = polyVegBatchTileX;
        tileZ = polyVegBatchTileZ;
    } else if(usePointerTile) {
        float pointerX = aktPointerPos[0];
        float pointerZ = aktPointerPos[2];
        Game::check_coords(tileX, tileZ, pointerX, pointerZ);
    }
    Tile *worldTile = route->requestTile(tileX, tileZ, false);
    if(worldTile == NULL || worldTile->loaded != 1) {
        GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile",
            usePointerTile ? "The pointer world tile is not loaded."
                           : "The current world tile is not loaded.");
        return false;
    }
    for(int index = 0; index < worldTile->jestObiektow; ++index) {
        WorldObj *object = worldTile->obiekty[index];
        if(object != NULL && object->loaded && object->typeID == WorldObj::sstatic
                && PolyVegObject::isBakeShape(object->fileName)) {
            if(!polyVegBatchBake) GuiFunct::showEditorNotice(this, "Bake PolyVeg Tile",
                "This tile already contains a PolyVeg bake.\n\n"
                "Select and delete every PolyVeg - Bake object on this tile "
                "before baking it again.");
            return false;
        }
    }

    const QString routePath = Game::root + "/routes/" + Game::route;
    const ForestCatalogLoadResult catalogResult =
        ForestDefinitionLoader::loadRoute(routePath);
    if(!catalogResult.isValid()) {
        GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile",
            "polyveg.json could not be loaded:\n\n" + catalogResult.errors.join("\n"));
        return false;
    }

    QSet<QString> vegetationShapes;
    for(const ForestRecipeDefinition &recipe : catalogResult.catalog.polyVeg)
        for(const ForestVegetationDefinition &vegetation : recipe.vegetation)
            vegetationShapes.insert(vegetation.shape.toLower());

    QVector<WorldObj*> sourceObjects;
    QVector<ForestBakeInstance> instances;
    for(int index = 0; index < worldTile->jestObiektow; ++index) {
        WorldObj *object = worldTile->obiekty[index];
        if(object == NULL || !object->loaded || object->typeID != WorldObj::sstatic
                || !vegetationShapes.contains(object->fileName.toLower()))
            continue;
        ForestBakeInstance instance;
        instance.shapePath = routePath + "/shapes/" + object->fileName;
        instance.x = tileX*2048.0 + object->position[0];
        instance.y = object->position[1];
        instance.z = tileZ*2048.0 - object->position[2];
        const float *q = object->qDirection;
        instance.yawDegrees = std::atan2(
            2.0*(q[3]*q[1] + q[0]*q[2]),
            1.0 - 2.0*(q[1]*q[1] + q[2]*q[2])) * 180.0/M_PI;
        instance.uniformScale = object->getUniformMatrixScale();
        sourceObjects.append(object);
        instances.append(instance);
    }
    if(instances.isEmpty()) {
        if(!polyVegBatchBake) GuiFunct::showEditorNotice(this, "Bake PolyVeg Tile",
            usePointerTile
                ? "The pointer tile contains no unbaked vegetation shapes listed in polyveg.json."
                : "The current tile contains no unbaked vegetation shapes listed in polyveg.json.");
        return false;
    }
    const int sourceObjectCount = sourceObjects.size();

    auto signedTile = [](int value) {
        return QString("%1%2").arg(value < 0 ? '-' : '+')
            .arg(std::abs(value), 5, 10, QLatin1Char('0'));
    };
    const QString shapesPath = routePath + "/shapes";
    const QString confirmation = QString(
        "Bake configured vegetation on the pointer tile into 4x4 patch blocks?\n\n"
        "Tile: %1, %2\nSource objects: %3\n\n"
        "Only static shapes listed in polyveg.json will be replaced. "
        "Bake clears Undo history and purges this tile's source objects from "
        "memory. Delete the bake and replant to retry.")
        .arg(tileX).arg(tileZ).arg(sourceObjects.size());
    if(!polyVegBatchBake
            && !GuiFunct::confirmDestructiveAction(this, "Bake Vegetation", confirmation))
        return false;

    QDialog singleBakeDialog(this);
    QLabel singleBakeStatus(
        QString("Bake 1 of 1\nTile %1, %2").arg(tileX).arg(tileZ),
        &singleBakeDialog);
    if(!polyVegBatchBake) {
        configurePolyVegBakeDialog(singleBakeDialog, singleBakeStatus);
        singleBakeDialog.show();
        singleBakeDialog.raise();
        allowPolyVegBakePaint(this, 500);
    }
    PolyVegViewportFreeze bakeViewportFreeze(this);

    const ForestPatchBakeResult baked = ForestPatchBaker::bake(instances, 4);
    if(!baked.isValid() || baked.patches.isEmpty()) {
        if(!polyVegBatchBake)
            singleBakeDialog.close();
        GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile",
            baked.errors.isEmpty() ? "No occupied 4x4 vegetation blocks were produced."
                                   : baked.errors.join("\n"));
        return false;
    }
    QStringList outputNames;
    for(const ForestBakedPatch &patch : baked.patches) {
        const QString name = QString("V%1%2-%3%4.s")
            .arg(signedTile(tileX), signedTile(-tileZ))
            .arg(patch.key.patchX).arg(patch.key.patchZ);
        outputNames.append(name);
    }

    QString error;
    QStringList generatedFiles;
    ForestBakeSession operationFiles;
    const QString manifestPath = routePath + "/OpenRails/forest-bakes.json";
    for(const QString &outputName : outputNames) {
        generatedFiles.append(shapesPath + '/' + outputName);
        generatedFiles.append(shapesPath + '/'
            + QFileInfo(outputName).completeBaseName() + ".sd");
    }
    QStringList transactionFiles = generatedFiles;
    transactionFiles.append(manifestPath);
    for(const QString &path : transactionFiles) {
        if(!operationFiles.rememberFile(path, error)
                || !polyVegBakeSession.rememberFile(path, error)) {
            if(!polyVegBatchBake)
                singleBakeDialog.close();
            GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile", error);
            return false;
        }
    }

    for(int index = 0; index < baked.patches.size(); ++index) {
        const QString shapePath = shapesPath + '/' + outputNames[index];
        if(!ForestShapeTextIO::writePatch(shapePath, baked.patches[index], error)
                || !ForestShapeTextIO::writeDescriptor(shapePath, error)) {
            QString rollbackError;
            if(!operationFiles.rollback(rollbackError))
                error += "\n\n" + rollbackError;
            if(!polyVegBatchBake)
                singleBakeDialog.close();
            GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile", error);
            return false;
        }
    }

    // A delete/replant/rebake cycle reuses the same short V- shape names.
    // Invalidate any geometry retained from the previous bake before the new
    // world objects acquire those cached SFile entries.
    for(const QString &outputName : outputNames)
        currentShapeLib->reloadShapeIfCached(shapesPath + '/' + outputName);

    Undo::StateBegin();
    int placedBlocks = 0;
    QVector<WorldObj*> placedBakeObjects;
    placedBakeObjects.reserve(baked.patches.size());
    for(int index = 0; index < baked.patches.size(); ++index) {
        const ForestBakedPatch &patch = baked.patches[index];
        const float worldFilePositionZ = static_cast<float>(
            patch.originZ - tileZ*2048.0);
        float position[3] {
            static_cast<float>(patch.originX - tileX*2048.0),
            static_cast<float>(patch.originY),
            -worldFilePositionZ
        };
        float rotation[4];
        Quat::fill(rotation);
        Ref::RefItem reference;
        reference.type = "static";
        reference.clas = "Baked Vegetation";
        reference.filename.append(outputNames[index]);
        WorldObj *placed = route->placeObject(
            tileX, tileZ, position, rotation, 0.0f, &reference);
        if(placed == NULL) continue;
        placedBakeObjects.append(placed);
        ++placedBlocks;
    }
    Undo::StateEnd();
    if(placedBlocks != baked.patches.size()) {
        Undo::UndoLast();
        worldTile->purgeObjects(placedBakeObjects);
        QString rollbackError;
        operationFiles.rollback(rollbackError);
        if(!polyVegBatchBake)
            singleBakeDialog.close();
        GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile",
            QString("The baked blocks could not all be placed. The partial "
                    "bake was removed and the raw vegetation was left intact.")
            + (rollbackError.isEmpty() ? QString()
                : "\n\n" + rollbackError));
        return false;
    }

    bool manifestPublished = true;
    for(int index = 0; index < baked.patches.size(); ++index) {
        const ForestBakedPatch &patch = baked.patches[index];
        const float worldFilePositionZ = static_cast<float>(
            patch.originZ - tileZ*2048.0);
        ForestBakeManifestEntry entry;
        entry.id = QString("%1,%2:%3,%4").arg(tileX).arg(-tileZ)
            .arg(patch.key.patchX).arg(patch.key.patchZ);
        entry.shapeFile = outputNames[index];
        entry.tileX = tileX;
        entry.tileZ = -tileZ;
        entry.blockX = patch.key.patchX;
        entry.blockZ = patch.key.patchZ;
        entry.positionX = patch.originX - tileX*2048.0;
        entry.positionY = patch.originY;
        entry.positionZ = worldFilePositionZ;
        QDir().mkpath(routePath + "/OpenRails");
        if(!ForestBakeManifest::upsert(manifestPath, entry, error)) {
            manifestPublished = false;
            break;
        }
    }
    if(!manifestPublished) {
        Undo::UndoLast();
        worldTile->purgeObjects(placedBakeObjects);
        QString rollbackError;
        if(!operationFiles.rollback(rollbackError))
            error += "\n\n" + rollbackError;
        if(!polyVegBatchBake)
            singleBakeDialog.close();
        GuiFunct::showEditorStopped(this, "Bake PolyVeg Tile",
            "The generated bake manifest could not be published. The partial "
            "bake was removed and the raw vegetation was left intact.\n\n"
            + error);
        return false;
    }

    operationFiles.commit();

    if(selectedObj != NULL && selectedObj->typeObj == GameObj::worldobj
            && sourceObjects.contains(static_cast<WorldObj*>(selectedObj)))
        setSelectedObj(NULL);
    Undo::Clear();
    const int purgedSources = worldTile->purgeObjects(sourceObjects);
    sourceObjects.clear();
    if(purgedSources != sourceObjectCount)
        emit sendMsg("msg", QString(
            "PolyVeg bake purged %1 of %2 expected raw objects on tile %3, %4.")
            .arg(purgedSources).arg(sourceObjectCount).arg(tileX).arg(tileZ));

    bakeViewportFreeze.finish();
    update();
    if(polyVegBatchBake) {
        polyVegBatchSourceCount += sourceObjectCount;
        polyVegBatchBlockCount += placedBlocks;
    }

    emit sendMsg("msg", QString("Baked %1 vegetation objects into %2 block(s) on tile %3, %4.")
        .arg(sourceObjectCount).arg(placedBlocks).arg(tileX).arg(tileZ));
    if(!polyVegBatchBake) {
        QTimer::singleShot(0, this,
            &RouteEditorGLWidget::refreshPolyVegTileCounts);
        allowPolyVegBakePaint(this, 250);
        singleBakeDialog.close();
        GuiFunct::showEditorNotice(this, "Bake PolyVeg Tile",
            QString("Baked %1 vegetation objects into %2 short-name 4x4 block(s).\n\n"
                    "The source objects were purged from memory and Bake cannot "
                    "be undone. Save normally to keep it; delete the bake and "
                    "replant to retry.")
                .arg(sourceObjectCount).arg(placedBlocks));
        queuePolyVegSuccessSound();
    }
    return true;
}

void RouteEditorGLWidget::bakeAllVegetation() {
    if(route == NULL || camera == NULL || camera->pozT == NULL)
        return;

    QVector<QPair<int, int>> tiles;
    const QVector<QPair<int, int>> loadedRawTiles = polyVegTiles(false);
    const int cameraTileX = static_cast<int>(camera->pozT[0]);
    const int cameraTileZ = static_cast<int>(camera->pozT[1]);
    const int tileRadius = qMax(0, Game::tileLod);
    for(const QPair<int, int> &tile : loadedRawTiles) {
        if(qAbs(tile.first-cameraTileX) <= tileRadius
                && qAbs(tile.second-cameraTileZ) <= tileRadius)
            tiles.append(tile);
    }
    if(tiles.isEmpty()) {
        GuiFunct::showEditorNotice(this, "Bake PolyVeg LOD",
            "No loaded tile inside the current camera LOD contains unbaked PolyVeg - Raw objects.");
        return;
    }
    if(!GuiFunct::confirmDestructiveAction(this, "Bake PolyVeg LOD",
            QString("Bake every pending loaded PolyVeg tile inside the current camera LOD?\n\n"
                    "LOD tiles: %1\n\n"
                    "All configured raw vegetation on those tiles will be replaced "
                    "by 4x4 bake blocks. Bake clears Undo history and purges only "
                    "the baked tiles' raw objects from memory.")
                .arg(tiles.size())))
        return;

    QDialog processing(this);
    QLabel processingStatus(&processing);
    configurePolyVegBakeDialog(processing, processingStatus);
    processing.show();
    processing.raise();
    QApplication::processEvents();
    PolyVegViewportFreeze bakeViewportFreeze(this);

    polyVegBatchBake = true;
    polyVegBatchSourceCount = 0;
    polyVegBatchBlockCount = 0;
    int successfulTiles = 0;
    for(int index = 0; index < tiles.size(); ++index) {
        polyVegBatchTileX = tiles[index].first;
        polyVegBatchTileZ = tiles[index].second;
        processingStatus.setText(QString(
            "Bake %1 of %2\nTile %3, %4")
            .arg(index + 1).arg(tiles.size())
            .arg(polyVegBatchTileX).arg(polyVegBatchTileZ));
        allowPolyVegBakePaint(this, 500);
        if(bakeVegetationTile(false))
            ++successfulTiles;
    }
    polyVegBatchBake = false;

    bakeViewportFreeze.finish();
    QTimer::singleShot(0, this,
        &RouteEditorGLWidget::refreshPolyVegTileCounts);
    allowPolyVegBakePaint(this, 250);
    processing.close();
    GuiFunct::showEditorNotice(this, "Bake PolyVeg LOD",
        QString("Baked %1 raw vegetation objects into %2 block(s) across %3 of %4 tile(s).\n\n"
                "The baked tiles' raw objects were purged from memory and Bake "
                "cannot be undone. Save normally to keep it; delete bakes and "
                "replant to retry.")
            .arg(polyVegBatchSourceCount).arg(polyVegBatchBlockCount)
            .arg(successfulTiles).arg(tiles.size()));
    if(successfulTiles == tiles.size())
        queuePolyVegSuccessSound();
}

void RouteEditorGLWidget::deleteAllPolyVegBakes() {
    if(route == NULL)
        return;
    if(!GuiFunct::confirmDestructiveAction(this, "Delete All PolyVeg Bakes",
            "Delete every PolyVeg - Bake object across the route?\n\n"
            "The deletion remains one Undo operation until Save commits it. "
            "On Save, only now-unreferenced generated bake assets are removed."))
        return;

    const int removed = route->deleteAllPolyVegBakes(true);
    refreshPolyVegTileCounts();
    update();
    if(removed == 0) {
        ForestBakePruneResult cleanup;
        QString cleanupError;
        const QString routePath =
            Game::root + "/routes/" + Game::route;
        if(!ForestBakeManifest::pruneUnreferenced(
                routePath, cleanup, cleanupError)) {
            GuiFunct::showEditorNotice(this, "Delete All PolyVeg Bakes",
                "No PolyVeg bake objects were found, and orphan cleanup "
                "could not complete.\n\n" + cleanupError);
            return;
        }
        if(cleanup.removedAssets > 0 || cleanup.removedManifest) {
            GuiFunct::showEditorNotice(this, "Delete All PolyVeg Bakes",
                QString("No PolyVeg bake objects were found.\n\n"
                        "Removed %1 orphaned generated bake file(s)%2.")
                    .arg(cleanup.removedAssets)
                    .arg(cleanup.removedManifest
                        ? " and the empty forest-bakes.json manifest"
                        : ""));
            return;
        }
        GuiFunct::showEditorNotice(this, "Delete All PolyVeg Bakes",
            "No PolyVeg bake objects or orphaned generated bake files "
            "were found on the route.");
        return;
    }
    GuiFunct::showEditorNotice(this, "Delete All PolyVeg Bakes",
        QString("Deleted %1 PolyVeg bake object(s).\n\n"
                "Save to commit asset cleanup, or Undo to restore the bakes.")
            .arg(removed));
}

void RouteEditorGLWidget::editFind1x1() {
    editFind(0);
}

void RouteEditorGLWidget::editFind3x3() {
    editFind(1);
}

void RouteEditorGLWidget::editFind(int radius) {
    if (selectedObj != NULL)
        if (selectedObj->typeObj == GameObj::worldobj){
            selectedObj->unselect();
            route->findSimilar((WorldObj*)selectedObj, groupObj, camera->pozT, radius);
            setSelectedObj(groupObj);
            if (groupObj->count() == 0) {
                groupObj->unselect();
                setSelectedObj(NULL);
            }
        }
}

void RouteEditorGLWidget::editUndo() {
    Undo::UndoLast();
    QTimer::singleShot(0, this,
        &RouteEditorGLWidget::refreshPolyVegTileCounts);
}

bool RouteEditorGLWidget::discardUnsavedPolyVegBakeFiles(QString &error) {
    return polyVegBakeSession.rollback(error);
}

void RouteEditorGLWidget::showTrkEditr() {
    if (route != NULL)
        route->showTrkEditr();
}




void RouteEditorGLWidget::setTerrainToObj(){
    Undo::StateBegin();
    if (selectedObj != NULL)
        route->setTerrainToTrackObj((WorldObj*)selectedObj, defaultPaintBrush);
    else
        route->setTerrainToTrackObj((WorldObj*)lastSelectedObj, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::smoothTerrainToObj(){
    Undo::StateBegin();
    if (selectedObj != NULL)
        route->smoothTerrainToTrackObj((WorldObj*)selectedObj, defaultPaintBrush);
    else
        route->smoothTerrainToTrackObj((WorldObj*)lastSelectedObj, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::placeWaterRuler(){
    if(route == NULL)
        return;
    if(waterScanPending){
        emit waterPanelStatus("Ruler locked while processing.");
        return;
    }
    RulerObj *selectedRuler = selectedObj != NULL
        ? dynamic_cast<RulerObj*>(selectedObj) : NULL;
    const bool selectedSpecialRuler = selectedRuler != NULL
        && selectedRuler->isSpecialRuler();
    Undo::StateBeginIfNotExist();
    route->deleteSpecialRulers();
    if(selectedSpecialRuler)
        setSelectedObj(NULL);
    activeWaterRuler = NULL;
    activeVegetationRuler = NULL;
    activeGradeRuler = NULL;
    waterScanUndoAvailable = false;
    setSpecialRulerPanelControlsActive(true);
    enableTool("waterRulerTool");
    emit waterPanelStatus("New Ruler: click to add points.");
}

void RouteEditorGLWidget::addWaterRulerPoints(){
    if(route == NULL)
        return;
    if(waterScanPending){
        emit waterPanelStatus("Ruler locked while processing.");
        return;
    }
    if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
        activeWaterRuler = route->findWaterRuler(true);
    if(activeWaterRuler == NULL){
        enableTool("selectTool");
        emit waterPanelStatus("No ruler. Choose New Water Ruler.");
        return;
    }
    setSpecialRulerPanelControlsActive(true);
    if(selectedObj != NULL && selectedObj != activeWaterRuler)
        selectedObj->unselect();
    setSelectedObj(activeWaterRuler, false);
    activeWaterRuler->select(
        qMax(0, activeWaterRuler->getPointCount() - 1));
    enableTool("waterRulerTool");
    emit waterPanelStatus(
        QString("Add Points: %1 points.")
        .arg(activeWaterRuler->getPointCount()));
}

void RouteEditorGLWidget::editWaterRulerPoints(){
    if(route == NULL)
        return;
    if(waterScanPending){
        emit waterPanelStatus("Ruler locked while processing.");
        return;
    }
    if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
        activeWaterRuler = route->findWaterRuler(true);
    if(activeWaterRuler == NULL){
        enableTool("selectTool");
        emit waterPanelStatus("No ruler. Choose New Water Ruler.");
        return;
    }
    setSpecialRulerPanelControlsActive(true);
    enableTool("selectTool");
    if(selectedObj != NULL && selectedObj != activeWaterRuler)
        selectedObj->unselect();
    setSelectedObj(activeWaterRuler, false);
    emit waterPanelStatus("Edit Points: drag a control point.");
}

void RouteEditorGLWidget::scanWaterRuler(float heightAboveBed, int tileRadius){
    queueWaterRulerOperation(heightAboveBed, tileRadius, false);
}

void RouteEditorGLWidget::adjustWaterTerrain(
        float clearance, int tileRadius){
    queueWaterRulerOperation(clearance, tileRadius, true);
}

void RouteEditorGLWidget::queueWaterRulerOperation(
        float heightAboveBed, int tileRadius, bool terrainOnly){
    if(route == NULL)
        return;
    enableTool("selectTool");
    if(waterScanPending){
        emit waterPanelStatus("Processing is already pending.");
        return;
    }
    if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
        activeWaterRuler = route->findWaterRuler(true);
    if(activeWaterRuler == NULL){
        emit waterPanelStatus("No ruler to process.");
        return;
    }
    if(activeWaterRuler->getPointCount() < 2){
        emit waterPanelStatus("At least two points are required.");
        return;
    }
    waterScanPending = true;
    emit waterPanelStatus(
        terrainOnly
            ? QString("Adjusting terrain from %1 ruler points...")
                .arg(activeWaterRuler->getPointCount())
            : QString("Processing water from %1 ruler points...")
                .arg(activeWaterRuler->getPointCount()));

    // Paint the pending message before entering the synchronous operation, but
    // do not accept another click or a Save command in between. The previous
    // two-second timer allowed Save to complete before terrain adjustment had
    // even started, leaving the adjusted height map dirty but absent from the
    // route save that the operator had just requested.
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    runWaterRulerScan(heightAboveBed, tileRadius, terrainOnly);
    waterScanPending = false;
}

void RouteEditorGLWidget::runWaterRulerScan(
        float heightAboveBed, int tileRadius, bool terrainOnly){
    heightAboveBed = qMax(0.01f, heightAboveBed);
    tileRadius = qBound(0, tileRadius, 50);

    struct WaterPathPoint {
        float x;
        float y;
        float z;
    };
    QVector<WaterPathPoint> path;
    for(int i = 0; i < activeWaterRuler->getPointCount(); i++){
        float p[3] = {0,0,0};
        activeWaterRuler->getPointWorldPosition(i, p);
        if(!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])){
            emit waterPanelStatus("Invalid point. Start a new ruler.");
            return;
        }
        WaterPathPoint wp = {p[0], p[1], p[2]};
        path.push_back(wp);
    }

    auto guidedSurface = [&path, heightAboveBed](float x, float z) {
        float bestDist2 = std::numeric_limits<float>::max();
        float bestHeight = path.first().y + heightAboveBed;
        for(int i = 0; i < path.size() - 1; i++){
            float dx = path[i + 1].x - path[i].x;
            float dz = path[i + 1].z - path[i].z;
            float len2 = dx * dx + dz * dz;
            float t = 0.0f;
            if(len2 > 0.0001f)
                t = qBound(0.0f,
                    ((x - path[i].x) * dx + (z - path[i].z) * dz) / len2,
                    1.0f);
            float px = path[i].x + dx * t;
            float pz = path[i].z + dz * t;
            float ddx = x - px;
            float ddz = z - pz;
            float dist2 = ddx * ddx + ddz * ddz;
            if(dist2 < bestDist2){
                bestDist2 = dist2;
                bestHeight = path[i].y + (path[i + 1].y - path[i].y) * t
                             + heightAboveBed;
            }
        }
        return bestHeight;
    };

    auto tileForWorld = [](float world) {
        return (int)std::floor((world + 1024.0f) / 2048.0f);
    };
    auto coordKey = [](int x, int z) {
        return (quint64)(quint32)x << 32 | (quint32)z;
    };

    // Build a narrow tile corridor that follows every ruler segment. A simple
    // bounding rectangle can become enormous on a long diagonal or winding
    // river and was the source of avoidable memory pressure.
    QSet<quint64> corridorTiles;
    for(int i = 0; i < path.size() - 1; i++){
        float dx = path[i + 1].x - path[i].x;
        float dz = path[i + 1].z - path[i].z;
        float length = std::sqrt(dx * dx + dz * dz);
        int steps = qMax(1, (int)std::ceil(length / 1024.0f));
        for(int s = 0; s <= steps; s++){
            float t = (float)s / (float)steps;
            int centerX = tileForWorld(path[i].x + dx * t);
            int centerZ = tileForWorld(path[i].z + dz * t);
            for(int oz = -tileRadius; oz <= tileRadius; oz++)
                for(int ox = -tileRadius; ox <= tileRadius; ox++)
                    corridorTiles.insert(coordKey(centerX + ox, centerZ + oz));
        }
    }
    const int maximumCorridorTiles = 4096;
    if(corridorTiles.isEmpty() || corridorTiles.size() > maximumCorridorTiles){
        emit waterPanelStatus(
            QString("Stopped: %1 tiles exceeds the %2-tile limit.")
            .arg(corridorTiles.size()).arg(maximumCorridorTiles));
        return;
    }

    int tileTotal = corridorTiles.size();
    QHash<quint64, Terrain*> corridorTerrain;
    emit waterPanelProgress(0, tileTotal, "Loading search tiles...");
    for(quint64 tileKey : corridorTiles){
        int tx = (int)(qint32)(tileKey >> 32);
        int tz = (int)(qint32)(tileKey & 0xffffffffu);
        Terrain *terrain = Game::terrainLib->getTerrainByXY(tx, tz, true);
        if(terrain != NULL && terrain->loaded)
            corridorTerrain.insert(tileKey, terrain);
    }
    if(corridorTerrain.isEmpty()){
        emit waterPanelStatus("No terrain tiles were available.");
        return;
    }

    Terrain *referenceTerrain = corridorTerrain.constBegin().value();
    float patchSize = (float)referenceTerrain->getPatchSize();
    if(!std::isfinite(patchSize) || patchSize < 1.0f){
        emit waterPanelStatus("Stopped: invalid terrain patch size.");
        return;
    }
    auto patchGrid = [patchSize](float world) {
        return (int)std::floor((world + 1024.0f) / patchSize);
    };
    auto patchCenter = [patchSize](int grid) {
        return grid * patchSize - 1024.0f + patchSize * 0.5f;
    };

    QQueue<QPair<int,int>> open;
    QSet<quint64> seedKeys;
    for(int i = 0; i < path.size() - 1; i++){
        float dx = path[i + 1].x - path[i].x;
        float dz = path[i + 1].z - path[i].z;
        float length = std::sqrt(dx * dx + dz * dz);
        int steps = qMax(1, (int)std::ceil(length / (patchSize * 0.5f)));
        for(int s = 0; s <= steps; s++){
            float t = (float)s / (float)steps;
            int gx = patchGrid(path[i].x + dx * t);
            int gz = patchGrid(path[i].z + dz * t);
            quint64 key = coordKey(gx, gz);
            if(!seedKeys.contains(key)){
                seedKeys.insert(key);
                open.enqueue(qMakePair(gx, gz));
            }
        }
    }

    // Every patch on a tile uses the same four ruler-derived corner heights.
    // Cache them once per tile so long rulers do not rescan every ruler segment
    // for every shoreline probe.
    QHash<quint64, QVector<float>> proposedLevelCache;
    proposedLevelCache.reserve(corridorTerrain.size());
    auto proposedTileLevels = [&guidedSurface, &proposedLevelCache, &coordKey](
            int tx, int tz) {
        const quint64 tileKey = coordKey(tx, tz);
        auto cached = proposedLevelCache.constFind(tileKey);
        if(cached != proposedLevelCache.constEnd())
            return cached.value();
        float west = tx * 2048.0f - 1024.0f;
        float east = west + 2048.0f;
        float north = tz * 2048.0f - 1024.0f;
        float south = north + 2048.0f;
        QVector<float> levels(4);
        levels[0] = guidedSurface(west, north);
        levels[1] = guidedSurface(east, north);
        levels[2] = guidedSurface(west, south);
        levels[3] = guidedSurface(east, south);
        proposedLevelCache.insert(tileKey, levels);
        return levels;
    };
    const float shorelineClearance = 0.05f;
    auto patchTouchesWater = [&corridorTerrain, &coordKey, &tileForWorld,
                              &proposedTileLevels, patchSize,
                              shorelineClearance](float worldX, float worldZ) {
        const int tx = tileForWorld(worldX);
        const int tz = tileForWorld(worldZ);
        Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
        if(terrain == NULL || terrain->getSampleSize() <= 0)
            return false;

        const int sampleSize = terrain->getSampleSize();
        const int samplesAcross = qRound(patchSize / sampleSize);
        if(samplesAcross < 2 || terrain->getSampleCount() * sampleSize != 2048)
            return false;

        // Terrain patches and native posts share the same exact grid. Inspect
        // every post strictly inside this patch, avoiding its shared boundary
        // so a low post belonging to the neighboring patch cannot widen the
        // shoreline. This is a bounded one-shot scan, not brush-time work.
        const float west = worldX - tx * 2048.0f - patchSize * 0.5f;
        const float north = worldZ - tz * 2048.0f - patchSize * 0.5f;
        const int firstSampleX = qRound((west + 1024.0f) / sampleSize);
        const int firstSampleZ = qRound((north + 1024.0f) / sampleSize);
        if(firstSampleX < 0 || firstSampleZ < 0
                || firstSampleX + samplesAcross > terrain->getSampleCount()
                || firstSampleZ + samplesAcross > terrain->getSampleCount())
            return false;
        const QVector<float> waterLevels = proposedTileLevels(tx, tz);
        for(int offsetZ = 1; offsetZ < samplesAcross; ++offsetZ){
            const int sampleZ = firstSampleZ + offsetZ;
            const float localZ = sampleZ * sampleSize - 1024.0f;
            for(int offsetX = 1; offsetX < samplesAcross; ++offsetX){
                const int sampleX = firstSampleX + offsetX;
                const float localX = sampleX * sampleSize - 1024.0f;
                if(terrain->terrainData[sampleZ][sampleX]
                        <= WaterBedClearanceMath::bilinearWaterHeight(
                               waterLevels.constData(), localX, localZ)
                           + shorelineClearance)
                    return true;
            }
        }
        return false;
    };
    auto edgePatchCellsEnterBedBand = [
            &corridorTerrain, &coordKey, &tileForWorld,
            &proposedTileLevels, patchSize,
            heightAboveBed](float worldX, float worldZ) {
        const int tx = tileForWorld(worldX);
        const int tz = tileForWorld(worldZ);
        Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
        if(terrain == NULL)
            return false;

        const int sampleSize = terrain->getSampleSize();
        const int samplesAcross = qRound(patchSize / sampleSize);
        constexpr int probeSpacing = 2;
        constexpr int minimumSubmergedProbes = 4;
        if(samplesAcross < 2 || sampleSize < probeSpacing * 2
                || sampleSize % probeSpacing != 0
                || terrain->getSampleCount() * sampleSize != 2048)
            return false;

        const float west = worldX - tx * 2048.0f - patchSize * 0.5f;
        const float north = worldZ - tz * 2048.0f - patchSize * 0.5f;
        const int firstSampleX = qRound((west + 1024.0f) / sampleSize);
        const int firstSampleZ = qRound((north + 1024.0f) / sampleSize);
        if(firstSampleX < 0 || firstSampleZ < 0
                || firstSampleX + samplesAcross > terrain->getSampleCount()
                || firstSampleZ + samplesAcross > terrain->getSampleCount())
            return false;

        const QVector<float> waterLevels = proposedTileLevels(tx, tz);
        int submergedProbes = 0;
        for(int offsetZ = 0; offsetZ < samplesAcross; ++offsetZ){
            for(int offsetX = 0; offsetX < samplesAcross; ++offsetX){
                // The post test already covers the patch interior. Only the
                // first terrain cell inside each edge can contain a visible
                // crossing that falls between a shared edge post and the
                // first interior post. Probe those cells on the same physical
                // 2 m lattice for both 4 m and 8 m terrain. Four probes inside
                // the adjustable bed band represent about 16 square metres of
                // evidence, rejecting isolated contacts without hiding an inlet.
                if(offsetX != 0 && offsetX != samplesAcross - 1
                        && offsetZ != 0 && offsetZ != samplesAcross - 1)
                    continue;
                const int sampleX = firstSampleX + offsetX;
                const int sampleZ = firstSampleZ + offsetZ;
                const float h00 = terrain->terrainData[sampleZ][sampleX];
                const float h10 = terrain->terrainData[sampleZ][sampleX + 1];
                const float h01 = terrain->terrainData[sampleZ + 1][sampleX];
                const float h11 = terrain->terrainData[sampleZ + 1][sampleX + 1];
                for(int probeZ = probeSpacing; probeZ < sampleSize;
                        probeZ += probeSpacing){
                    const float fractionZ =
                        static_cast<float>(probeZ) / sampleSize;
                    for(int probeX = probeSpacing; probeX < sampleSize;
                            probeX += probeSpacing){
                        const float fractionX =
                            static_cast<float>(probeX) / sampleSize;
                        const float terrainHeight =
                            h00 * (1.0f - fractionX) * (1.0f - fractionZ)
                          + h10 * fractionX * (1.0f - fractionZ)
                          + h01 * (1.0f - fractionX) * fractionZ
                          + h11 * fractionX * fractionZ;
                        const float localX =
                            (sampleX + fractionX) * sampleSize - 1024.0f;
                        const float localZ =
                            (sampleZ + fractionZ) * sampleSize - 1024.0f;
                        if(terrainHeight
                                < WaterBedClearanceMath::bilinearWaterHeight(
                                      waterLevels.constData(), localX, localZ)
                                  + heightAboveBed
                                && ++submergedProbes
                                   >= minimumSubmergedProbes)
                            return true;
                    }
                }
            }
        }
        return false;
    };
    auto patchHasWater = [&corridorTerrain, &coordKey, &tileForWorld](
            float worldX, float worldZ) {
        const int tx = tileForWorld(worldX);
        const int tz = tileForWorld(worldZ);
        Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
        if(terrain == NULL)
            return false;
        float localX = worldX - tx * 2048.0f;
        float localZ = worldZ - tz * 2048.0f;
        return (terrain->getPatchFlags(tx, tz, localX, localZ)
                & 0x10000c0) == 0x10000c0;
    };

    QSet<quint64> visited;
    QSet<quint64> wetPatches;
    QSet<quint64> wetTiles;
    int patchesPerTile = qMax(1, (int)std::ceil(2048.0f / patchSize));
    const int maximumVisited = qMin(
        500000, qMax(1024, corridorTiles.size() * patchesPerTile * patchesPerTile));
    emit waterPanelProgress(0, 0, "Finding shoreline...");
    while(!open.isEmpty() && visited.size() < maximumVisited){
        QPair<int,int> cell = open.dequeue();
        quint64 cellKey = coordKey(cell.first, cell.second);
        if(visited.contains(cellKey))
            continue;
        visited.insert(cellKey);

        float worldX = patchCenter(cell.first);
        float worldZ = patchCenter(cell.second);
        int tx = tileForWorld(worldX);
        int tz = tileForWorld(worldZ);
        Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
        if(terrain == NULL)
            continue;
        const bool wet = terrainOnly
                ? patchHasWater(worldX, worldZ)
                : seedKeys.contains(cellKey)
                    || patchTouchesWater(worldX, worldZ);
        if(!wet)
            continue;

        wetPatches.insert(cellKey);
        wetTiles.insert(coordKey(tx, tz));
        open.enqueue(qMakePair(cell.first - 1, cell.second));
        open.enqueue(qMakePair(cell.first + 1, cell.second));
        open.enqueue(qMakePair(cell.first, cell.second - 1));
        open.enqueue(qMakePair(cell.first, cell.second + 1));
        open.enqueue(qMakePair(cell.first - 1, cell.second - 1));
        open.enqueue(qMakePair(cell.first + 1, cell.second - 1));
        open.enqueue(qMakePair(cell.first - 1, cell.second + 1));
        open.enqueue(qMakePair(cell.first + 1, cell.second + 1));

    }
    if(!open.isEmpty()){
        emit waterPanelStatus(
            "Stopped: shoreline exceeds the processing limit.");
        return;
    }
    if(!terrainOnly && !wetPatches.isEmpty()){
        // A shallow terrain cell can enter the bed-adjustment band between its
        // shared edge post and first interior post even though every strict
        // interior post is currently dry. Use the same Above bed limit that
        // Adjust Terrain will apply, so placement anticipates the posts that
        // the following operation may lower instead of leaving a raised shelf.
        // Check only the immediate frontier produced by the normal flood and
        // add all confirmed cells together so the fallback cannot cascade into
        // high banks or island cores.
        QSet<quint64> edgeCandidates;
        for(quint64 patchKey : wetPatches){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            for(int offsetZ = -1; offsetZ <= 1; ++offsetZ){
                for(int offsetX = -1; offsetX <= 1; ++offsetX){
                    if(offsetX == 0 && offsetZ == 0)
                        continue;
                    const quint64 candidate = coordKey(
                        gx + offsetX, gz + offsetZ);
                    if(!wetPatches.contains(candidate))
                        edgeCandidates.insert(candidate);
                }
            }
        }
        QSet<quint64> edgeAdditions;
        for(quint64 candidate : edgeCandidates){
            const int gx = (int)(qint32)(candidate >> 32);
            const int gz = (int)(qint32)(candidate & 0xffffffffu);
            if(edgePatchCellsEnterBedBand(
                    patchCenter(gx), patchCenter(gz)))
                edgeAdditions.insert(candidate);
        }
        for(quint64 addition : edgeAdditions){
            const int gx = (int)(qint32)(addition >> 32);
            const int gz = (int)(qint32)(addition & 0xffffffffu);
            wetPatches.insert(addition);
            wetTiles.insert(coordKey(
                tileForWorld(patchCenter(gx)),
                tileForWorld(patchCenter(gz))));
        }
    }
    if(wetPatches.isEmpty()){
        emit waterPanelStatus(terrainOnly
            ? "No processed water found. Process Water Tiles first."
            : "No connected water patches found.");
        return;
    }

    QHash<quint64, QVector<float>> finalTileLevels;
    for(quint64 tileKey : wetTiles){
        int tx = (int)(qint32)(tileKey >> 32);
        int tz = (int)(qint32)(tileKey & 0xffffffffu);
        Terrain *terrain = corridorTerrain.value(tileKey, NULL);
        if(terrain == NULL)
            continue;
        QVector<float> levels(4);
        if(terrainOnly){
            terrain->getWaterLevels(levels.data());
        } else {
            levels = proposedTileLevels(tx, tz);
        }
        finalTileLevels.insert(tileKey, levels);
    }

    if(!terrainOnly){
        struct CornerLevel {
            double sum = 0.0;
            int count = 0;
        };
        QHash<quint64, CornerLevel> cornerLevels;
        for(auto it = finalTileLevels.constBegin();
                it != finalTileLevels.constEnd(); ++it){
            const int tx = (int)(qint32)(it.key() >> 32);
            const int tz = (int)(qint32)(it.key() & 0xffffffffu);
            const int cornerX[4] = {tx, tx + 1, tx, tx + 1};
            const int cornerZ[4] = {tz, tz, tz + 1, tz + 1};
            for(int corner = 0; corner < 4; ++corner){
                CornerLevel &level = cornerLevels[
                    coordKey(cornerX[corner], cornerZ[corner])];
                level.sum += it.value()[corner];
                level.count++;
            }
        }
        for(auto it = finalTileLevels.begin();
                it != finalTileLevels.end(); ++it){
            const int tx = (int)(qint32)(it.key() >> 32);
            const int tz = (int)(qint32)(it.key() & 0xffffffffu);
            const int cornerX[4] = {tx, tx + 1, tx, tx + 1};
            const int cornerZ[4] = {tz, tz, tz + 1, tz + 1};
            for(int corner = 0; corner < 4; ++corner){
                const CornerLevel level = cornerLevels.value(
                    coordKey(cornerX[corner], cornerZ[corner]));
                if(level.count > 0)
                    it.value()[corner] = (float)(level.sum / level.count);
            }
        }
    }

    QSet<quint64> waterPatchesToClear;
    if(!terrainOnly){
        QQueue<QPair<int,int>> replacementOpen;
        for(quint64 patchKey : wetPatches){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            if(patchHasWater(patchCenter(gx), patchCenter(gz)))
                replacementOpen.enqueue(qMakePair(gx, gz));
        }
        for(quint64 patchKey : seedKeys){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            if(patchHasWater(patchCenter(gx), patchCenter(gz)))
                replacementOpen.enqueue(qMakePair(gx, gz));
        }

        QSet<quint64> examinedExisting;
        QSet<quint64> connectedExisting;
        while(!replacementOpen.isEmpty()
                && examinedExisting.size() < maximumVisited){
            const QPair<int,int> cell = replacementOpen.dequeue();
            const quint64 key = coordKey(cell.first, cell.second);
            if(examinedExisting.contains(key))
                continue;
            examinedExisting.insert(key);
            if(!patchHasWater(
                    patchCenter(cell.first), patchCenter(cell.second)))
                continue;
            connectedExisting.insert(key);
            for(int offsetZ = -1; offsetZ <= 1; ++offsetZ)
                for(int offsetX = -1; offsetX <= 1; ++offsetX)
                    if(offsetX != 0 || offsetZ != 0)
                        replacementOpen.enqueue(qMakePair(
                            cell.first + offsetX, cell.second + offsetZ));
        }
        if(!replacementOpen.isEmpty()){
            emit waterPanelStatus(
                "Stopped: existing water exceeds the replacement limit.");
            return;
        }
        waterPatchesToClear = connectedExisting;
        waterPatchesToClear.subtract(wetPatches);
    }

    struct BedSample {
        float height = 0.0f;
        float waterHeight = 0.0f;
        float newHeight = 0.0f;
        bool waterMaskEdge = false;
    };
    QHash<quint64, BedSample> bedSamples;
    int adjustedBedPosts = 0;
    float largestBedDrop = 0.0f;
    Terrain *bedReferenceTerrain = wetTiles.isEmpty()
        ? NULL : corridorTerrain.value(*wetTiles.constBegin(), NULL);
    const int bedSampleSize = bedReferenceTerrain != NULL
        ? bedReferenceTerrain->getSampleSize() : 0;
    const int integerPatchSize = qRound(patchSize);
    bool bedGridSupported = bedSampleSize > 0
        && 2048 % bedSampleSize == 0
        && integerPatchSize > 0
        && integerPatchSize % bedSampleSize == 0;
    if(terrainOnly && bedGridSupported){
        const int expectedSamples = 2048 / bedSampleSize;
        for(quint64 tileKey : wetTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            if(terrain == NULL
                    || terrain->getSampleSize() != bedSampleSize
                    || terrain->getSampleCount() != expectedSamples
                    || finalTileLevels.value(tileKey).size() != 4){
                bedGridSupported = false;
                break;
            }
        }
    }
    if(terrainOnly && !bedGridSupported){
        emit waterPanelStatus("Stopped: unsupported terrain sample grid.");
        return;
    }
    if(terrainOnly && bedSampleSize > 0){
        emit waterPanelProgress(0, 0, "Checking bed clearance...");
        const int samplesPerTile = 2048 / bedSampleSize;
        const int samplesPerPatch = integerPatchSize / bedSampleSize;
        auto floorDivide = [](int value, int divisor) {
            int quotient = value / divisor;
            if(value % divisor < 0)
                --quotient;
            return quotient;
        };
        auto touchesWetPatch = [&wetPatches, &coordKey, &floorDivide,
                                samplesPerPatch](int gridX, int gridZ) {
            const int eastPatch = floorDivide(gridX, samplesPerPatch);
            const int southPatch = floorDivide(gridZ, samplesPerPatch);
            const bool onXBoundary = gridX % samplesPerPatch == 0;
            const bool onZBoundary = gridZ % samplesPerPatch == 0;
            const int xCount = onXBoundary ? 2 : 1;
            const int zCount = onZBoundary ? 2 : 1;
            for(int zIndex = 0; zIndex < zCount; ++zIndex){
                for(int xIndex = 0; xIndex < xCount; ++xIndex){
                    if(wetPatches.contains(coordKey(
                            eastPatch - xIndex,
                            southPatch - zIndex)))
                        return true;
                }
            }
            return false;
        };
        auto sampleKey = [&coordKey, samplesPerTile](
                int tileX, int tileZ, int sampleX, int sampleZ) {
            // Patch coordinates use (world + 1024) / patch size. Terrain
            // sample zero therefore aligns with tile * samplesPerTile for
            // both 512/4 m and 256/8 m terrain.
            return coordKey(tileX * samplesPerTile + sampleX,
                            tileZ * samplesPerTile + sampleZ);
        };
        auto waterHeightForTile = [](const QVector<float> &levels,
                                     float localX, float localZ) {
            return WaterBedClearanceMath::bilinearWaterHeight(
                levels.constData(), localX, localZ);
        };

        for(quint64 tileKey : wetTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            const QVector<float> levels = finalTileLevels.value(tileKey);
            if(terrain == NULL || terrain->getSampleSize() != bedSampleSize
                    || terrain->getSampleCount() != samplesPerTile
                    || levels.size() != 4)
                continue;
            const int tx = (int)(qint32)(tileKey >> 32);
            const int tz = (int)(qint32)(tileKey & 0xffffffffu);
            const int sampleCount = terrain->getSampleCount();
            for(int sampleZ = 0; sampleZ <= sampleCount; ++sampleZ){
                const float localZ = sampleZ * bedSampleSize - 1024.0f;
                for(int sampleX = 0; sampleX <= sampleCount; ++sampleX){
                    const float localX = sampleX * bedSampleSize - 1024.0f;
                    const int gridX = tx * samplesPerTile + sampleX;
                    const int gridZ = tz * samplesPerTile + sampleZ;
                    if(!touchesWetPatch(gridX, gridZ))
                        continue;
                    const quint64 key = sampleKey(
                        tx, tz, sampleX, sampleZ);
                    const float terrainHeight =
                        terrain->terrainData[sampleZ][sampleX];
                    const float waterHeight = waterHeightForTile(
                        levels, localX, localZ);
                    auto existing = bedSamples.find(key);
                    if(existing != bedSamples.end()){
                        // Shared tile samples should already agree. If legacy
                        // data does not, the highest terrain and lowest water
                        // interpretation is the conservative, deterministic
                        // choice and cannot erase an exposed seam post.
                        existing.value().height = qMax(
                            existing.value().height, terrainHeight);
                        existing.value().waterHeight = qMin(
                            existing.value().waterHeight, waterHeight);
                        existing.value().newHeight = existing.value().height;
                        continue;
                    }
                    BedSample sample;
                    sample.height = terrainHeight;
                    sample.waterHeight = waterHeight;
                    sample.newHeight = sample.height;
                    bedSamples.insert(key, sample);
                }
            }
        }

        // Terrain that stands meaningfully above water is rejected later as a
        // protected bank/island core. A water-mask edge remains repairable but
        // receives half clearance to soften the transition into that core.
        const int neighborOffsets[8][2] = {
            {-1,-1}, {0,-1}, {1,-1}, {-1,0},
            {1,0}, {-1,1}, {0,1}, {1,1}
        };
        for(auto it = bedSamples.begin(); it != bedSamples.end(); ++it){
            const int gridX = (int)(qint32)(it.key() >> 32);
            const int gridZ = (int)(qint32)(it.key() & 0xffffffffu);
            bool maskEdge = false;
            for(const auto &offset : neighborOffsets){
                if(!bedSamples.contains(coordKey(
                        gridX + offset[0], gridZ + offset[1]))){
                    maskEdge = true;
                    break;
                }
            }
            it.value().waterMaskEdge = maskEdge;
        }

        for(auto it = bedSamples.begin(); it != bedSamples.end(); ++it){
            BedSample &sample = it.value();
            const float depth = sample.waterHeight - sample.height;
            if(depth <= -heightAboveBed || depth >= heightAboveBed)
                continue;

            const float taper = WaterBedClearanceMath::shoreTaperFactor(
                sample.waterMaskEdge);
            const float drop = WaterBedClearanceMath::shallowBedDrop(
                sample.height, sample.waterHeight, heightAboveBed, taper);
            if(drop < 0.001f)
                continue;
            sample.newHeight = sample.height - drop;
            largestBedDrop = qMax(largestBedDrop, drop);
            adjustedBedPosts++;
        }
    }

    if(terrainOnly && adjustedBedPosts == 0){
        emit waterPanelStatus("Terrain already meets the selected clearance.");
        return;
    }

    Undo::StateBegin();
    QSet<quint64> affectedWaterTiles = wetTiles;
    if(!terrainOnly){
        for(quint64 patchKey : waterPatchesToClear){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            affectedWaterTiles.insert(coordKey(
                tileForWorld(patchCenter(gx)),
                tileForWorld(patchCenter(gz))));
        }
        for(quint64 tileKey : affectedWaterTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            if(terrain != NULL)
                Undo::PushTerrainWater(terrain);
        }
        for(quint64 tileKey : wetTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            const QVector<float> levels = finalTileLevels.value(tileKey);
            if(terrain != NULL && levels.size() == 4)
                terrain->setWaterLevel(
                    levels[0], levels[1], levels[2], levels[3]);
        }
    }
    int applied = 0;
    if(!terrainOnly){
        for(quint64 patchKey : waterPatchesToClear){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            const float worldX = patchCenter(gx);
            const float worldZ = patchCenter(gz);
            const int tx = tileForWorld(worldX);
            const int tz = tileForWorld(worldZ);
            Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
            if(terrain != NULL)
                terrain->toggleWaterDraw(
                    tx, tz, worldX - tx * 2048.0f,
                    worldZ - tz * 2048.0f, -1);
        }
        for(quint64 patchKey : wetPatches){
            const int gx = (int)(qint32)(patchKey >> 32);
            const int gz = (int)(qint32)(patchKey & 0xffffffffu);
            const float worldX = patchCenter(gx);
            const float worldZ = patchCenter(gz);
            const int tx = tileForWorld(worldX);
            const int tz = tileForWorld(worldZ);
            Terrain *terrain = corridorTerrain.value(coordKey(tx, tz), NULL);
            if(terrain == NULL)
                continue;
            terrain->toggleWaterDraw(
                tx, tz, worldX - tx * 2048.0f,
                worldZ - tz * 2048.0f, 1);
            applied++;
        }
    }
    QSet<Terrain*> changedBedTerrains;
    if(terrainOnly && adjustedBedPosts > 0){
        const int sampleSize = bedSampleSize;
        const int samplesPerTile = 2048 / sampleSize;
        auto sampleKey = [&coordKey, samplesPerTile](
                int tileX, int tileZ, int sampleX, int sampleZ) {
            return coordKey(tileX * samplesPerTile + sampleX,
                            tileZ * samplesPerTile + sampleZ);
        };
        for(quint64 tileKey : wetTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            if(terrain == NULL || terrain->getSampleSize() != sampleSize
                    || terrain->getSampleCount() != samplesPerTile)
                continue;
            const int tx = (int)(qint32)(tileKey >> 32);
            const int tz = (int)(qint32)(tileKey & 0xffffffffu);
            const int sampleCount = terrain->getSampleCount();
            for(int sampleZ = 0; sampleZ <= sampleCount; ++sampleZ){
                for(int sampleX = 0; sampleX <= sampleCount; ++sampleX){
                    const auto sample = bedSamples.constFind(
                        sampleKey(tx, tz, sampleX, sampleZ));
                    if(sample == bedSamples.constEnd()
                            || sample.value().newHeight
                               >= terrain->terrainData[sampleZ][sampleX] - 0.0005f)
                        continue;
                    if(!changedBedTerrains.contains(terrain)){
                        Undo::PushTerrainHeightMap(
                            terrain->mojex, terrain->mojez,
                            terrain->terrainData, terrain->getSampleCount());
                        changedBedTerrains.insert(terrain);
                    }
                    terrain->terrainData[sampleZ][sampleX] =
                        qMin(terrain->terrainData[sampleZ][sampleX],
                             sample.value().newHeight);
                    const float localX = sampleX * sampleSize - 1024.0f;
                    const float localZ = sampleZ * sampleSize - 1024.0f;
                    terrain->setErrorBias(
                        tx, tz,
                        qMin(localX, 1024.0f - 0.001f),
                        qMin(localZ, 1024.0f - 0.001f), 0);
                }
            }
        }
    }
    if(!terrainOnly){
        for(quint64 tileKey : affectedWaterTiles){
            Terrain *terrain = corridorTerrain.value(tileKey, NULL);
            if(terrain != NULL)
                terrain->refreshWaterShapes();
        }
    }
    for(Terrain *terrain : changedBedTerrains){
        terrain->setModified(true);
        terrain->refresh();
        Game::terrainLib->updateTerrainHeightmap(terrain);
    }
    Undo::StateEnd();
    waterScanUndoAvailable = true;
    playPlacementSound("SCOchirp.wav");
    if(terrainOnly){
        emit waterPanelStatus(
            QString("Terrain adjusted: %1 posts lowered.")
            .arg(adjustedBedPosts));
        if(Game::debugOutput)
            qDebug() << "Water shallow-bed repair" << adjustedBedPosts
                     << "posts; max drop" << largestBedDrop;
    } else {
        emit waterPanelStatus(
            QString("Water processed: %1 patches; %2 old cleared.")
            .arg(applied).arg(waterPatchesToClear.size()));
    }
}

void RouteEditorGLWidget::undoWaterScan(){
    if(!waterScanUndoAvailable){
        emit waterPanelStatus("Nothing to undo.");
        return;
    }
    Undo::UndoLast();
    waterScanUndoAvailable = false;
}

void RouteEditorGLWidget::removeWaterRuler(){
    setSpecialRulerPanelControlsActive(false);
    if(route == NULL)
        return;
    enableTool("selectTool");
    if(activeWaterRuler == NULL || !activeWaterRuler->loaded)
        activeWaterRuler = route->findWaterRuler(true);
    if(activeWaterRuler == NULL){
        return;
    }
    Undo::StateBegin();
    route->deleteObj(activeWaterRuler);
    Undo::StateEnd();
    waterScanUndoAvailable = false;
    if(selectedObj == activeWaterRuler)
        setSelectedObj(NULL);
    activeWaterRuler = NULL;
}

void RouteEditorGLWidget::setTerrainToNearestDbTile(){
    if(route == NULL)
        return;

    Undo::StateBegin();
    route->setTerrainToNearestDbTile((int)camera->pozT[0], (int)camera->pozT[1], aktPointerPos, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::setTerrainToSelectedObjTile(){
    if(route == NULL || selectedObj == NULL)
        return;
    if(selectedObj->typeObj != GameObj::worldobj)
        return;

    WorldObj *obj = (WorldObj*)selectedObj;
    if(obj->type != "trackobj" && obj->type != "dyntrack")
        return;

    Undo::StateBegin();
    route->setTerrainToTrackObjTile(obj, defaultPaintBrush, (int)camera->pozT[0], (int)camera->pozT[1]);
    Undo::StateEnd();
}

void RouteEditorGLWidget::selectAllTerrainPatchesOnSelectedTile(){
    if(selectedObj == NULL || selectedObj->typeObj != GameObj::terrainobj)
        return;

    ((Terrain*)selectedObj)->selectAllPatches();
}

void RouteEditorGLWidget::adjustObjPositionToTerrainMenu(){
    Undo::StateBeginIfNotExist();
    Undo::PushGameObjData(selectedObj);
    if (selectedObj != NULL)
        if(selectedObj->typeObj == GameObj::worldobj)
            ((WorldObj*)selectedObj)->adjustPositionToTerrain();
}

void RouteEditorGLWidget::adjustObjRotationToTerrainMenu(){
    Undo::StateBeginIfNotExist();
    Undo::PushGameObjData(selectedObj);
    if (selectedObj != NULL)
        if(selectedObj->typeObj == GameObj::worldobj)
            ((WorldObj*)selectedObj)->adjustRotationToTerrain();
}

void RouteEditorGLWidget::pickObjForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            Quat::copy(this->placeRot, ((WorldObj*)selectedObj)->qDirection);
            route->ref->selected = ((WorldObj*)selectedObj)->getRefInfo();
        }
    }
}

void RouteEditorGLWidget::pickObjRotForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            placeElev = 0;
            Quat::copy(this->placeRot, ((WorldObj*)selectedObj)->qDirection);
        }
    }
}

void RouteEditorGLWidget::pickObjRotForCamera(){
    /*
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            int camobjtx = ((WorldObj*)selectedObj)->x;
            int camobjty = ((WorldObj*)selectedObj)->y;

            double camobjx = ((WorldObj*)selectedObj)->position[0];
            double camobjy = ((WorldObj*)selectedObj)->position[1]+10;
            double camobjz = ((WorldObj*)selectedObj)->position[2];
            double currcam = camera->getRotX();

            camera->setPozT(camobjtx,camobjty);
            camera->setPos(camobjx,camobjy,camobjz);

            double camrotw = ((WorldObj*)selectedObj)->qDirection[1];
            double camrotz = ((WorldObj*)selectedObj)->qDirection[3];
            double camrot = (2.0 * std::atan2(camrotw, camrotz));

            camera->setPlayerRot(camrot, 0.0f);
            selectedObj->unselect(); setSelectedObj(NULL);

         }
    }
    else
    {
     * */
            qDebug() << "trying repos cam to nearest object";

        ////if(CamObj == NULL)
            ////{
            CamObj = NULL;
            float campos[3];
            campos[0] = aktPointerPos[0];
            campos[1] = aktPointerPos[1];
            campos[2] = aktPointerPos[2];

            CamObj = route->findNearestObj((int)camera->pozT[0] , (int) camera->pozT[1],campos );
            //qDebug() << "CamObj:        " << CamObj->fileName;
            //qDebug() << "aktPointerPos: " << camera->pozT[0] << " " << camera->pozT[1] << " " <<  aktPointerPos[0] << " " <<  aktPointerPos[1] << " " <<  -aktPointerPos[2];
            ////}
            if(CamObj == NULL)
            {   qDebug() << "camObj still null";
                return;
            }
            int camobjtx = CamObj->x;
            int camobjty = CamObj->y;

            double camobjx = CamObj->position[0];
            double camobjy = CamObj->position[1]+10;
            double camobjz = CamObj->position[2];
            double currcam = camera->getRotX();

            camera->setPozT(camobjtx,camobjty);
            camera->setPos(camobjx,camobjy,camobjz);

            double camrotw = CamObj->qDirection[1];
            double camrotz = CamObj->qDirection[3];
            double camrot = (2.0 * std::atan2(camrotw, camrotz));

            while (camrot > 2.0 * M_PI)
                camrot -= 2.0 * M_PI;
            while (camrot < 0)
                camrot += 2.0 * M_PI;

            while (currcam > 2.0 * M_PI)
                currcam -= 2.0 * M_PI;
            while (currcam < 0)
                currcam += 2.0 * M_PI;


            if((currcam - camrot) > (M_PI/2)) camrot = camrot - M_PI;
            if((currcam - camrot) < (-M_PI/2)) camrot = camrot + M_PI;

            while (camrot > 2.0 * M_PI)
                camrot -= 2.0 * M_PI;
            while (camrot < 0)
                camrot += 2.0 * M_PI;

            qDebug() << "starting = " << currcam << " camrot = " << camrot;

            camera->setPlayerRot(camrot, 0.0f);
            CamObj = NULL;
            return;
  //      }
//    }
}

void RouteEditorGLWidget::pickObjRotForCameraFlip(){
            double camrot = (camera->getRotX()) + M_PI;
            camera->setPlayerRot(camrot, 0.0f);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamN() {
    double camrot = (M_PI);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;

    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);

}

void RouteEditorGLWidget::resetCamS() {
    double camrot = 0;
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamE() {
    double camrot = (M_PI/2);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}
void RouteEditorGLWidget::resetCamW() {
    double camrot = (M_PI/-2);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamD() {
    double camrot = 0;
    double camelev = (M_PI/-2);
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamZ() {
    double camrot = 0;
    double camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}


// tangentTarget

void RouteEditorGLWidget::tangentOrigin(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){

            StartObject = selectedObj;
            //if(Game::debugOutput)
                if(Game::debugOutput) qDebug() << "Tangent Start: " << ((WorldObj*)StartObject)->x << " " << ((WorldObj*)StartObject)->y << " " << ((WorldObj*)StartObject)->position[0] << " " << ((WorldObj*)StartObject)->position[1] << " " << ((WorldObj*)StartObject)->position[2];


            if(EndObject != NULL) tangentMath();
         }
    }
}
void RouteEditorGLWidget::tangentTarget(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){

            EndObject = selectedObj;
            // if(Game::debugOutput)
                if(Game::debugOutput) qDebug() << "Tangent End: " << ((WorldObj*)EndObject)->x << " " << ((WorldObj*)EndObject)->y << " " << ((WorldObj*)EndObject)->position[0] << " " << ((WorldObj*)EndObject)->position[1] << " " << ((WorldObj*)EndObject)->position[2];

            if(StartObject != NULL) tangentMath();
         }
    }
}

void RouteEditorGLWidget::tangentMath(){


    int stx = ((WorldObj*)StartObject)->x;
    int etx = ((WorldObj*)EndObject)->x;
    int stz = ((WorldObj*)StartObject)->y;
    int etz = ((WorldObj*)EndObject)->y;
    float spx = ((WorldObj*)StartObject)->position[0];
    float epx = ((WorldObj*)EndObject)->position[0];
    float spy = ((WorldObj*)StartObject)->position[1];
    float epy = ((WorldObj*)EndObject)->position[1];
    float spz = ((WorldObj*)StartObject)->position[2];
    float epz = ((WorldObj*)EndObject)->position[2];



    float dx = (2048 * (etx - stx)) + epx - spx;
     if(Game::debugOutput) qDebug() << "DX: " << dx ;

    float dy = epy - spy;
     if(Game::debugOutput) qDebug() << "DY: " << dy ;

    float dz = -((2048 * (etz - stz)) + epz - spz);
     if(Game::debugOutput) qDebug() << "DZ: " << dz ;

    double heading; // d
    if (abs(dx) > 0)
    {
        if (dx > 0) { heading = 90 - 180 / M_PI * atan(dz / dx); }
        else { heading = -90 - 180 / M_PI * atan(dz / dx); }
    }
    else { heading = -90 + copysignf(1.0, dz) * 90; }

     if(Game::debugOutput) qDebug() << "HD: " << heading ;

    double distance = sqrt((dx * dx) + (dy * dy) + (dz * dz)); // d
     if(Game::debugOutput) qDebug() << "DI: " << distance ;

    double slope; // d
    if (abs(dy / distance) < 0.99999)
    { slope = atan(dy / sqrt(dx * dx + dz * dz)); }
    else { slope = copysignf(1.0, dy / distance) * 1000; }

     if(Game::debugOutput) qDebug() << "SL: " << slope ;

    double slopedeg = 0;  // d
    if (abs(dy / distance) < 0.99999)
    { slope = 180 / M_PI * atan(dy / sqrt(dx * dx + dz * dz)); }
    else { slope = copysignf(1.0, dy / distance) * 90; }

     if(Game::debugOutput) qDebug() << "SD: " << slopedeg ;

    double headingcos = cos(-0.5 * heading * M_PI / 180); // d
    double headingsin = sin(-0.5 * heading * M_PI / 180); // d
    double slopecos = 1;  // d
    double slopesin = 0; // d
    double slopedegcos = 1; // d
    double slopedegsin = 0; // d

    double QD4 = (headingcos * slopecos * slopedegcos) + (headingsin * slopesin * slopedegsin);  // d
    double QD2 = (headingsin * slopecos * slopedegcos) - (headingcos * slopesin * slopedegsin); // d
    double QD3 = (headingcos * slopesin * slopedegcos) + (headingsin * slopecos * slopedegsin); // d
    double QD1 = (headingcos * slopecos * slopedegsin) + (headingsin * slopesin * slopedegcos); // d

//    QD = Math.Round(QD1, 6).toString() + " " + Math.Round(QD2, 6).toString() + " " + Math.Round(QD3, 6).toString() + " " + Math.Round(QD4, 6).toString();
//    DQ = Math.Round(QD3, 6).toString() + " " + Math.Round(QD4, 6).toString() + " " + Math.Round(QD1, 6).toString() + " " + Math.Round(QD2 * -1, 6).toString();

    QClipboard *clipboard = QApplication::clipboard();
    QString qd = QString::number(QD1) + " " + QString::number(QD2) + " " + QString::number(QD3) + " " + QString::number(QD4) ;


    clipboard->setText(qd);
    if(Game::debugOutput) qDebug() << "Final QD:" << qd;

}

void RouteEditorGLWidget::TangentApplyRot(){
    if(selectedObj->typeObj != GameObj::worldobj)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    if(args.length() != 4)
        return;
    float nq[4];
    nq[0] = args[0].toFloat();
    nq[1] = args[1].toFloat();
    nq[2] = -args[2].toFloat();
    nq[3] = args[3].toFloat();

    Undo::SinglePushWorldObjData((WorldObj*)selectedObj);
    ((WorldObj*)selectedObj)->setQdirection((float*)&nq);
    ((WorldObj*)selectedObj)->setModified();
    ((WorldObj*)selectedObj)->setMartix();
    // quat.setText(clipboard->text());
}

void RouteEditorGLWidget::pickObjRotElevForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            Quat::fill(this->placeRot);
            placeElev = ((WorldObj*)selectedObj)->getElevation();
        }
    }
}

void RouteEditorGLWidget::showContextMenu(const QPoint & point) {
    if(defaultMenuActions["undo"] == NULL){
        defaultMenuActions["undo"] = new QAction(tr("&Undo"), this);
        QObject::connect(defaultMenuActions["undo"], SIGNAL(triggered()), this, SLOT(editUndo()));
    }
    if(defaultMenuActions["copy"] == NULL){
        defaultMenuActions["copy"] = new QAction(tr("&Copy"), this);
        QObject::connect(defaultMenuActions["copy"], SIGNAL(triggered()), this, SLOT(editCopy()));
    }
    if(defaultMenuActions["copyInfo"] == NULL){
        defaultMenuActions["copyInfo"] = new QAction(tr("Copy &Info"), this);
        QObject::connect(defaultMenuActions["copyInfo"], SIGNAL(triggered()), this, SLOT(copySelectionInfo()));
    }
    if(defaultMenuActions["paste"] == NULL){
        defaultMenuActions["paste"] = new QAction(tr("&Paste"), this);
        QObject::connect(defaultMenuActions["paste"], SIGNAL(triggered()), this, SLOT(editPaste()));
    }
    if(defaultMenuActions["find1x1"] == NULL){
        defaultMenuActions["find1x1"] = new QAction(tr("&Select Similar 1x1"), this);
        QObject::connect(defaultMenuActions["find1x1"], SIGNAL(triggered()), this, SLOT(editFind1x1()));
    }
    if(defaultMenuActions["find3x3"] == NULL){
        defaultMenuActions["find3x3"] = new QAction(tr("&Select Similar 3x3"), this);
        QObject::connect(defaultMenuActions["find3x3"], SIGNAL(triggered()), this, SLOT(editFind3x3()));
    }
    if(defaultMenuActions["select"] == NULL){
        defaultMenuActions["select"] = new QAction(tr("&Select Tool"), this);
        QObject::connect(defaultMenuActions["select"], SIGNAL(triggered()), this, SLOT(editSelect()));
    }
    if(defaultMenuActions["setTerrToObj"] == NULL){
        defaultMenuActions["setTerrToObj"] = new QAction(tr("&Set Terrain to Object"));
        QObject::connect(defaultMenuActions["setTerrToObj"], SIGNAL(triggered()), this, SLOT(setTerrainToObj()));
    }
    if(defaultMenuActions["setPosToTerr"] == NULL){
        defaultMenuActions["setPosToTerr"] = new QAction(tr("&Set position to Terrain"));
        QObject::connect(defaultMenuActions["setPosToTerr"], SIGNAL(triggered()), this, SLOT(adjustObjPositionToTerrainMenu()));
    }
    if(defaultMenuActions["setRotToTerr"] == NULL){
        defaultMenuActions["setRotToTerr"] = new QAction(tr("&Set rotation to Terrain"));
        QObject::connect(defaultMenuActions["setRotToTerr"], SIGNAL(triggered()), this, SLOT(adjustObjRotationToTerrainMenu()));
    }
    if(defaultMenuActions["pickObj"] == NULL){
        defaultMenuActions["pickObj"] = new QAction(tr("&Pick for placement"));
        QObject::connect(defaultMenuActions["pickObj"], SIGNAL(triggered()), this, SLOT(pickObjForPlacement()));
    }
    if(defaultMenuActions["pickObjRot"] == NULL){
        defaultMenuActions["pickObjRot"] = new QAction(tr("&Pick rotation for placement"));
        QObject::connect(defaultMenuActions["pickObjRot"], SIGNAL(triggered()), this, SLOT(pickObjRotForPlacement()));
    }
    if(defaultMenuActions["pickObjElev"] == NULL){
        defaultMenuActions["pickObjElev"] = new QAction(tr("&Pick elevation for placement"));
        QObject::connect(defaultMenuActions["pickObjElev"], SIGNAL(triggered()), this, SLOT(pickObjRotElevForPlacement()));
    }

    if(defaultMenuActions["pickObjRotCam"] == NULL){
        defaultMenuActions["pickObjRotCam"] = new QAction(tr("&Reposition camera to object"));
        QObject::connect(defaultMenuActions["pickObjRotCam"], SIGNAL(triggered()), this, SLOT(pickObjRotForCamera()));
    }

    if(defaultMenuActions["pickObjRotCamFlip"] == NULL){
        defaultMenuActions["pickObjRotCamFlip"] = new QAction(tr("&Flip camera 180 degrees"));
        QObject::connect(defaultMenuActions["pickObjRotCamFlip"], SIGNAL(triggered()), this, SLOT(pickObjRotForCameraFlip()));
    }

    if(defaultMenuActions["resetCamN"] == NULL){
        defaultMenuActions["resetCamN"] = new QAction(tr("Face &North"));
        QObject::connect(defaultMenuActions["resetCamN"], SIGNAL(triggered()), this, SLOT(resetCamN()));
    }

    if(defaultMenuActions["resetCamS"] == NULL){
        defaultMenuActions["resetCamS"] = new QAction(tr("Face &South"));
        QObject::connect(defaultMenuActions["resetCamS"], SIGNAL(triggered()), this, SLOT(resetCamS()));
    }
    if(defaultMenuActions["resetCamE"] == NULL){
        defaultMenuActions["resetCamE"] = new QAction(tr("Face &East"));
        QObject::connect(defaultMenuActions["resetCamE"], SIGNAL(triggered()), this, SLOT(resetCamE()));
    }
    if(defaultMenuActions["resetCamW"] == NULL){
        defaultMenuActions["resetCamW"] = new QAction(tr("Face &West"));
        QObject::connect(defaultMenuActions["resetCamW"], SIGNAL(triggered()), this, SLOT(resetCamW()));
    }

    if(defaultMenuActions["resetCamD"] == NULL){
        defaultMenuActions["resetCamD"] = new QAction(tr("Face &Down"));
        QObject::connect(defaultMenuActions["resetCamD"], SIGNAL(triggered()), this, SLOT(resetCamD()));
    }

    if(defaultMenuActions["resetCamZ"] == NULL){
        defaultMenuActions["resetCamZ"] = new QAction(tr("Default"));
        QObject::connect(defaultMenuActions["resetCamZ"], SIGNAL(triggered()), this, SLOT(resetCamZ()));
    }

    if(defaultMenuActions["TangentOrigin"] == NULL){
        defaultMenuActions["TangentOrigin"] = new QAction(tr("Tangent &Origin"));
        QObject::connect(defaultMenuActions["TangentOrigin"], SIGNAL(triggered()), this, SLOT(tangentOrigin()));
    }

    if(defaultMenuActions["TangentTarget"] == NULL){
        defaultMenuActions["TangentTarget"] = new QAction(tr("Tangent &Target"));
        QObject::connect(defaultMenuActions["TangentTarget"], SIGNAL(triggered()), this, SLOT(tangentTarget()));
    }

    if(defaultMenuActions["TangentApply"] == NULL){
        defaultMenuActions["TangentApply"] = new QAction(tr("Tangent &Apply Rotation"));
        QObject::connect(defaultMenuActions["TangentApply"], SIGNAL(triggered()), this, SLOT(TangentApplyRot()));
    }



    QMenu menu;
    QMenu menuTool;
    QMenu menuCamera;
    QMenu menuPointer;
    QString menuStyle = QString(
        "QMenu::separator {\
          color: ")+Game::StyleMainLabel+";\
        }";
    menu.setStyleSheet(menuStyle);
    if(selectedObj != NULL){
        menu.addSection("Object: " + selectedObj->getName());
        selectedObj->pushContextMenuActions(&menu);

        if(selectedObj->typeObj == selectedObj->worldobj){
            menu.addAction(defaultMenuActions["setTerrToObj"]);
            menu.addAction(defaultMenuActions["setPosToTerr"]);
            menu.addAction(defaultMenuActions["setRotToTerr"]);
            menu.addAction(defaultMenuActions["pickObj"]);
            menu.addAction(defaultMenuActions["pickObjRot"]);
            menu.addAction(defaultMenuActions["pickObjElev"]);

            menu.addAction(defaultMenuActions["find1x1"]);
            menu.addAction(defaultMenuActions["find3x3"]);
        }




    }
    if(selectedObj == NULL && polyVegHelperVisible) {
        menu.addSection("PolyVeg");
        if(defaultMenuActions["plantPolyVeg"] == NULL) {
            defaultMenuActions["plantPolyVeg"] = new QAction(tr("Plant PolyVeg"), this);
            QObject::connect(defaultMenuActions["plantPolyVeg"], &QAction::triggered,
                             this, &RouteEditorGLWidget::plantConfiguredPolyVeg);
        }
        if(defaultMenuActions["bakePolyVegTile"] == NULL) {
            defaultMenuActions["bakePolyVegTile"] = new QAction(tr("Bake PolyVeg Tile"), this);
            QObject::connect(defaultMenuActions["bakePolyVegTile"], &QAction::triggered,
                             this, &RouteEditorGLWidget::bakeVegetationPointerTile);
        }
        menu.addAction(defaultMenuActions["plantPolyVeg"]);
        menu.addAction(defaultMenuActions["bakePolyVegTile"]);
    }
    if(toolEnabled == ""){
        if(selectedObj != NULL)
            menu.addSection("No Tool");
    } else {
        QString toolName = toolEnabled;
        toolName[0] = toolName[0].toUpper();
        menu.addSection(toolName);
        if (toolEnabled == "selectTool"){
            menuTool.setTitle("Mode");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["selectToolSelect"] == NULL){
                defaultMenuActions["selectToolSelect"] = GuiFunct::newMenuCheckAction(tr("&Select"), this, !resizeTool|!rotateTool|!translateTool);
                QObject::connect(defaultMenuActions["selectToolSelect"], SIGNAL(triggered()), this, SLOT(selectToolSelect()));
            }
            defaultMenuActions["selectToolSelect"]->setChecked(!resizeTool&!rotateTool&!translateTool);
            if(defaultMenuActions["selectToolRotate"] == NULL){
                defaultMenuActions["selectToolRotate"] = GuiFunct::newMenuCheckAction(tr("&Rotate"), this, rotateTool);
                QObject::connect(defaultMenuActions["selectToolRotate"], SIGNAL(triggered()), this, SLOT(selectToolRotate()));
            }
            defaultMenuActions["selectToolRotate"]->setChecked(rotateTool);
            if(defaultMenuActions["selectToolTranslate"] == NULL){
                defaultMenuActions["selectToolTranslate"] = GuiFunct::newMenuCheckAction(tr("&Translate"), this, translateTool);
                QObject::connect(defaultMenuActions["selectToolTranslate"], SIGNAL(triggered()), this, SLOT(selectToolTranslate()));
            }
            defaultMenuActions["selectToolTranslate"]->setChecked(translateTool);
            if(defaultMenuActions["selectToolScale"] == NULL){
                defaultMenuActions["selectToolScale"] = GuiFunct::newMenuCheckAction(tr("&Custom"), this, resizeTool);
                QObject::connect(defaultMenuActions["selectToolScale"], SIGNAL(triggered()), this, SLOT(selectToolScale()));
            }
            defaultMenuActions["selectToolScale"]->setChecked(resizeTool);
            menuTool.addAction(defaultMenuActions["selectToolSelect"]);
            menuTool.addAction(defaultMenuActions["selectToolRotate"]);
            menuTool.addAction(defaultMenuActions["selectToolTranslate"]);
            menuTool.addAction(defaultMenuActions["selectToolScale"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            menuPointer.setTitle("Pointer");
            menu.addMenu(&menuPointer);
            if(defaultMenuActions["placeToolStickToTerrain"] == NULL){
                defaultMenuActions["placeToolStickToTerrain"] = GuiFunct::newMenuCheckAction(tr("&Stick to Terrain"), this, stickPointerToTerrain);
                QObject::connect(defaultMenuActions["placeToolStickToTerrain"], SIGNAL(triggered()), this, SLOT(placeToolStickTerrain()));
            }
            defaultMenuActions["placeToolStickToTerrain"]->setChecked(stickPointerToTerrain);
            if(defaultMenuActions["placeToolStickToAll"] == NULL){
                defaultMenuActions["placeToolStickToAll"] = GuiFunct::newMenuCheckAction(tr("&Stick to All"), this, !stickPointerToTerrain);
                QObject::connect(defaultMenuActions["placeToolStickToAll"], SIGNAL(triggered()), this, SLOT(placeToolStickAll()));
            }
            defaultMenuActions["placeToolStickToAll"]->setChecked(!stickPointerToTerrain);
            menuPointer.addAction(defaultMenuActions["placeToolStickToTerrain"]);
            menuPointer.addAction(defaultMenuActions["placeToolStickToAll"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetMoveStep"] == NULL){
                defaultMenuActions["resetMoveStep"] = new QAction(tr("&Reset MoveStep"), this);
                QObject::connect(defaultMenuActions["resetMoveStep"], SIGNAL(triggered()), this, SLOT(selectToolresetMoveStep()));
            }
            menu.addAction(defaultMenuActions["resetMoveStep"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetRot"] == NULL){
                defaultMenuActions["resetRot"] = new QAction(tr("&Reset Rotation"), this);
                QObject::connect(defaultMenuActions["resetRot"], SIGNAL(triggered()), this, SLOT(selectToolresetRot()));
            }
            menu.addAction(defaultMenuActions["resetRot"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetVert"] == NULL){
                defaultMenuActions["resetVert"] = new QAction(tr("Reset &Vertical"), this);
                QObject::connect(defaultMenuActions["resetVert"], SIGNAL(triggered()), this, SLOT(selectToolresetVert()));
            }
            menu.addAction(defaultMenuActions["resetVert"]);
        }


        if (toolEnabled == "heightTool" || toolEnabled == "waterTerrTool" || toolEnabled == "gapsTerrainTool"){
            menu.addMenu(&menuTool);
            if(defaultMenuActions["toolDirectionUp"] == NULL){
                defaultMenuActions["toolDirectionUp"] = GuiFunct::newMenuCheckAction(tr("&Up"), this, (defaultPaintBrush->direction+1));
                QObject::connect(defaultMenuActions["toolDirectionUp"], SIGNAL(triggered()), this, SLOT(toolBrushDirectionUp()));
            }
            defaultMenuActions["toolDirectionUp"]->setChecked((defaultPaintBrush->direction+1));
            if(defaultMenuActions["toolDirectionDown"] == NULL){
                defaultMenuActions["toolDirectionDown"] = GuiFunct::newMenuCheckAction(tr("&Down"), this, !((defaultPaintBrush->direction+1)));
                QObject::connect(defaultMenuActions["toolDirectionDown"], SIGNAL(triggered()), this, SLOT(toolBrushDirectionDown()));
            }
            defaultMenuActions["toolDirectionDown"]->setChecked(!((defaultPaintBrush->direction+1)));
            menuTool.addAction(defaultMenuActions["toolDirectionUp"]);
            menuTool.addAction(defaultMenuActions["toolDirectionDown"]);

            if (toolEnabled == "heightTool"){
                menuTool.setTitle("Paint Direction");
                defaultMenuActions["toolDirectionUp"]->setText("Up");
                defaultMenuActions["toolDirectionDown"]->setText("Down");
            }
            if (toolEnabled == "waterTerrTool"){
                menuTool.setTitle("Water");
                defaultMenuActions["toolDirectionUp"]->setText("Show");
                defaultMenuActions["toolDirectionDown"]->setText("Hide");
            }
            if (toolEnabled == "gapsTerrainTool"){
                menuTool.setTitle("Gaps");
                defaultMenuActions["toolDirectionUp"]->setText("Show");
                defaultMenuActions["toolDirectionDown"]->setText("Hide");
            }
        }
        if (toolEnabled == "putTerrainTexTool"){
            menuTool.setTitle("Default");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["putTerrainTexRandom"] == NULL){
                defaultMenuActions["putTerrainTexRandom"] = GuiFunct::newMenuCheckAction(tr("&Random"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->RANDOM);
                QObject::connect(defaultMenuActions["putTerrainTexRandom"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelectRandom()));
            }
            defaultMenuActions["putTerrainTexRandom"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->RANDOM);
            if(defaultMenuActions["putTerrainTexPresent"] == NULL){
                defaultMenuActions["putTerrainTexPresent"] = GuiFunct::newMenuCheckAction(tr("&Present"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->PRESENT);
                QObject::connect(defaultMenuActions["putTerrainTexPresent"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelectPresent()));
            }
            defaultMenuActions["putTerrainTexPresent"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->PRESENT);
            if(defaultMenuActions["putTerrainTex0"] == NULL){
                defaultMenuActions["putTerrainTex0"] = GuiFunct::newMenuCheckAction(tr("&Rotate 0°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT0);
                QObject::connect(defaultMenuActions["putTerrainTex0"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect0()));
            }
            defaultMenuActions["putTerrainTex0"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT0);
            if(defaultMenuActions["putTerrainTex90"] == NULL){
                defaultMenuActions["putTerrainTex90"] = GuiFunct::newMenuCheckAction(tr("&Rotate 90°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT90);
                QObject::connect(defaultMenuActions["putTerrainTex90"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect90()));
            }
            defaultMenuActions["putTerrainTex90"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT90);
            if(defaultMenuActions["putTerrainTex180"] == NULL){
                defaultMenuActions["putTerrainTex180"] = GuiFunct::newMenuCheckAction(tr("&Rotate 180°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT180);
                QObject::connect(defaultMenuActions["putTerrainTex180"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect180()));
            }
            defaultMenuActions["putTerrainTex180"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT180);
            if(defaultMenuActions["putTerrainTex270"] == NULL){
                defaultMenuActions["putTerrainTex270"] = GuiFunct::newMenuCheckAction(tr("&Rotate 270°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT270);
                QObject::connect(defaultMenuActions["putTerrainTex270"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect270()));
            }
            defaultMenuActions["putTerrainTex270"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT270);
            menuTool.addAction(defaultMenuActions["putTerrainTexRandom"]);
            menuTool.addAction(defaultMenuActions["putTerrainTexPresent"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex0"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex90"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex180"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex270"]);
        }
        if (toolEnabled.startsWith("paintTool")){
            menuTool.setTitle("Auto Paint");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["paintToolObjSelected"] == NULL){
                defaultMenuActions["paintToolObjSelected"] = new QAction(tr("&Selected Object"), this);
                QObject::connect(defaultMenuActions["paintToolObjSelected"], SIGNAL(triggered()), this, SLOT(paintToolObjSelected()));
            }
            if(defaultMenuActions["paintToolObj"] == NULL){
                defaultMenuActions["paintToolObj"] = new QAction(tr("&Nearest Object"), this);
                QObject::connect(defaultMenuActions["paintToolObj"], SIGNAL(triggered()), this, SLOT(paintToolObj()));
            }
            if(defaultMenuActions["paintToolTDB"] == NULL){
                defaultMenuActions["paintToolTDB"] = new QAction(tr("&Nearest Track or Road"), this);
                QObject::connect(defaultMenuActions["paintToolTDB"], SIGNAL(triggered()), this, SLOT(paintToolTDB()));
            }
            if(defaultMenuActions["paintToolTDBVector"] == NULL){
                defaultMenuActions["paintToolTDBVector"] = new QAction(tr("&Nearest TDB/RDB Vector"), this);
                QObject::connect(defaultMenuActions["paintToolTDBVector"], SIGNAL(triggered()), this, SLOT(paintToolTDBVector()));
            }
            if(defaultMenuActions["paintToolTileTrack"] == NULL){
                defaultMenuActions["paintToolTileTrack"] = new QAction(tr("&Track on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolTileTrack"], SIGNAL(triggered()), this, SLOT(paintToolTileTrack()));
            }
            if(defaultMenuActions["paintToolTileRoad"] == NULL){
                defaultMenuActions["paintToolTileRoad"] = new QAction(tr("&Roads on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolTileRoad"], SIGNAL(triggered()), this, SLOT(paintToolTileRoad()));
            }
            if(defaultMenuActions["paintToolWaterEdges"] == NULL){
                defaultMenuActions["paintToolWaterEdges"] = new QAction(tr("&Water on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolWaterEdges"], SIGNAL(triggered()), this, SLOT(paintToolWaterEdges()));
            }
            if(defaultMenuActions["paintToolResetTile"] == NULL){
                defaultMenuActions["paintToolResetTile"] = new QAction(tr("&Reset Tile Paint"), this);
                QObject::connect(defaultMenuActions["paintToolResetTile"], SIGNAL(triggered()), this, SLOT(paintToolResetTile()));
            }
            menuTool.addAction(defaultMenuActions["paintToolObjSelected"]);
            menuTool.addAction(defaultMenuActions["paintToolObj"]);
            menuTool.addAction(defaultMenuActions["paintToolTDB"]);
            menuTool.addAction(defaultMenuActions["paintToolTDBVector"]);
            menuTool.addSeparator();
            menuTool.addAction(defaultMenuActions["paintToolTileTrack"]);
            menuTool.addAction(defaultMenuActions["paintToolTileRoad"]);
            menuTool.addAction(defaultMenuActions["paintToolWaterEdges"]);
            menu.addAction(defaultMenuActions["paintToolResetTile"]);
        }

    }

    // EFO WFH tools
    if(selectedObj != NULL){
    menu.addSection("WFH Tools");
        menu.addAction(defaultMenuActions["TangentOrigin"]);
        menu.addAction(defaultMenuActions["TangentTarget"]);
        menu.addAction(defaultMenuActions["TangentApply"]);
    }

    // EFO new Camera menu
    menu.addSection("Camera");
    //if(selectedObj != NULL){
        menu.addAction(defaultMenuActions["pickObjRotCam"]);
    //}
    menu.addAction(defaultMenuActions["pickObjRotCamFlip"]);

    menuCamera.setTitle("Reset Camera");
            menu.addMenu(&menuCamera);
            menuCamera.addAction(defaultMenuActions["resetCamN"]);
            menuCamera.addAction(defaultMenuActions["resetCamS"]);
            menuCamera.addAction(defaultMenuActions["resetCamE"]);
            menuCamera.addAction(defaultMenuActions["resetCamW"]);
            menuCamera.addAction(defaultMenuActions["resetCamD"]);
            menuCamera.addAction(defaultMenuActions["resetCamZ"]);

    menu.addSeparator();
    menu.addSection("Edit");
    menu.addAction(defaultMenuActions["undo"]);
    menu.addAction(defaultMenuActions["copy"]);
    defaultMenuActions["copyInfo"]->setEnabled(selectedObj != NULL);
    menu.addAction(defaultMenuActions["copyInfo"]);
    menu.addAction(defaultMenuActions["paste"]);
    menu.addSeparator();
    menu.addAction(defaultMenuActions["select"]);

    menu.exec(mapToGlobal(point));
}

void RouteEditorGLWidget::createNewTiles(QMap<int, QPair<int, int>*> list){
    int x, z;
    QMapIterator<int, QPair<int, int>*> i2(list);
    while (i2.hasNext()) {
        i2.next();
        if(i2.value() == NULL)
            continue;
        x = i2.value()->first;
        z = i2.value()->second;
        if(Game::debugOutput) qDebug() << x << z;
        route->newTile(x, -z);
    }
}

void RouteEditorGLWidget::createNewLoTiles(QMap<int, QPair<int, int>*> list){
    int x, z;
    QMapIterator<int, QPair<int, int>*> i2(list);
    if (!Game::writeEnabled) return;
    while (i2.hasNext()) {
        i2.next();
        if(i2.value() == NULL)
            continue;
        x = i2.value()->first;
        z = i2.value()->second;
        if(Game::debugOutput) qDebug() << x << z;
        Game::terrainLib->setLowTerrainAsCurrent();
        Game::terrainLib->saveEmpty(x, z);
        Game::terrainLib->reload(x, -z);
        if(Game::autoGeoTerrain){
            float pos[3];
            Vec3::set(pos, 0, 0, 0);
            Game::terrainLib->setHeightFromGeo(x, -z, (float*)&pos);
        }
        Game::terrainLib->setDetailedTerrainAsCurrent();
    }
}

void RouteEditorGLWidget::getUnsavedInfo(QVector<QString> &items) {
    if (this->route == NULL)
        return;
    route->getUnsavedInfo(items);
}

bool RouteEditorGLWidget::saveRoute() {
    if(route == NULL){
        emit updStatus(QString("stat0"), QString("SAVE FAILED"));
        return false;
    }

    // A save attempt writes modified world tiles before terrain, route-key,
    // activity, and cleanup stages can report failure. From this point onward
    // rolling generated bake files back could leave an already-written world
    // file referring to a deleted shape. Retaining a now-orphaned bake asset is
    // safe and recoverable by the next successful manifest cleanup.
    polyVegBakeSession.commit();
    route->save();
    if(!route->lastSaveSucceeded()){
        emit updStatus(QString("stat0"), QString("SAVE FAILED"));
        return false;
    }

    timeSaved = timeNow;
    emit updStatus(QString("stat0"), QString("Saved"));
    return true;
}

void RouteEditorGLWidget::msg(QString text) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "saveError") {
        userErrorSound();
        return;
    }
    if (text == "save") {
        saveRoute();
        return;
    }
    if (text == "unselect") {
        setSelectedObj(NULL);
        lastSelectedObj = NULL;
        return;
    }
    if (text == "createPaths") {
        route->createNewPaths();
        return;
    }
    if (text == "resetPlaceRotation") {
        Quat::fill(this->placeRot);
        return;
    }
    if (text == "showTerrainTreeEditr") {
        TerrainTreeWindow ttWindow;
        ttWindow.exec();
        return;
    }
    if (text == "engItemSelected") {
        //QString pathid = ;
        //QString name = pathid.split("/").last();
        //QString texpath = pathid.left(pathid.length() - name.length());
        emit sendMsg("showShape", route->ref->selected->getShapePath());
    }
    if (text == "autoPlacementDeleteLast") {
        route->autoPlacementDeleteLast();
    }
    if (text == "editDetailedTerrain") {
        Game::terrainLib->setDetailedAsCurrent();
    }if (text == "editDistantTerrain") {
        Game::terrainLib->setDistantAsCurrent();
    }
}

void RouteEditorGLWidget::msg(QString text, bool val) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "stickToTDB") {
        this->route->placementStickToTarget = val;
        return;
    }
}

void RouteEditorGLWidget::msg(QString text, int val) {
}

void RouteEditorGLWidget::msg(QString text, float val) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "autoPlacementLength") {
        this->route->placementAutoLength = val;
        return;
    }
}

void RouteEditorGLWidget::msg(QString text, QString val) {
    //qDebug() << text;
    if (text == "mkrFile") {
        this->route->setMkrFile(val);
        return;
    }
}

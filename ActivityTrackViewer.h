/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 */

#ifndef ACTIVITYTRACKVIEWER_H
#define ACTIVITYTRACKVIEWER_H

#include <QHash>
#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QSet>
#include <QVector>
#include <QWidget>

class Path;
class Route;
class QKeyEvent;

class ActivityTrackViewer : public QWidget {
    Q_OBJECT
public:
    enum PathPlacementMode {
        PlaceAutomatic,
        PlaceStart,
        PlaceEnd,
        PlaceReverse,
        PlacePassingSiding,
        PlaceWait
    };

    explicit ActivityTrackViewer(QWidget *parent = NULL);

    void setRoute(Route *route);
    void setSelectedPath(Path *path);
    int trackSegmentCount() const;

public slots:
    void ensureCache();
    void fitRoute();
    void fitSelectedPath();
    void setShowJunctions(bool show);
    void setShowTileGrid(bool show);
    void setShowInteractives(bool show);
    void setShowMarkers(bool show);
    void setShowMapLabels(bool show);
    void rotateView90();
    void beginPathCreation(Path *path);
    void beginPathEdit(Path *path);
    void choosePathStart();
    void choosePathEnd();
    void reverseStartDirection();
    void choosePathReverse();
    void choosePassingSiding();
    void choosePathWait(int waitSeconds);
    void removeLastReverse();
    void undoDraftEdit();
    void redoDraftEdit();
    void deleteSelectedDraftObject();
    void validateDraftPath();
    void editPathMetadata();
    void saveDraftPath();
    void cancelPathEdit();
    void clearTransientMessage();

signals:
    void statusChanged(QString text);
    void junctionSelected(QString text);
    void pathDraftStatus(QString text);
    void pathPlacementModeChanged(int mode);
    void pathEditingStateChanged(bool active);
    void pathMetadataChanged(Path *path);
    void pathSaved(Path *path);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void leaveEvent(QEvent *event);

private:
    struct DraftPassingSiding;
    QPointF screenToWorld(const QPointF &screenPoint) const;
    QTransform worldTransform() const;
    void rebuildTrackCache();
    void rebuildPathCache();
    void fitBounds(const QRectF &bounds);
    void updateStatus(const QPointF *worldPoint = NULL);
    int junctionAt(const QPointF &screenPoint) const;
    QPointF vectorDirectionFromJunction(int vectorNodeId, const QPointF &junction) const;
    QString junctionDescription(int index) const;
    bool nearestTrackPoint(const QPointF &screenPoint, QPointF &worldPoint, int &vectorNodeId) const;
    bool computeDraftRoute();
    bool computeRouteLeg(const QPointF &from, int fromVector, const QPointF &to, int toVector,
                         QHash<int, int> &switchRoutes, QVector<QLineF> &output,
                         QVector<int> *orderedVectors = NULL,
                         QVector<int> *orderedJunctions = NULL,
                         int overlapLeg = -1);
    bool extendRouteToNaturalEnd(const QPointF &from, int fromVector,
                                 const QPointF &travelDirection, QVector<QLineF> &output,
                                 QPointF &naturalEnd, int &naturalEndVector,
                                 QHash<int, int> &switchRoutes,
                                 int overlapLeg = -1);
    void rebuildOverlapLines();
    void appendVectorSlice(int vectorNodeId, const QPointF &from, const QPointF &to,
                           QVector<QLineF> &output, QVector<int> *vectorIds = NULL,
                           QVector<int> *segmentIds = NULL,
                           QVector<int> *legIds = NULL, int legId = -1) const;
    QPointF junctionPosition(int nodeId) const;
    void placeDraftPoint(const QPointF &screenPoint, bool moveStart);
    bool toggleDraftSwitch(const QPointF &screenPoint);
    bool addPassingSiding(const QPointF &screenPoint);
    bool calculatePassingSiding(int clickedVector, const QHash<int, int> &switchRoutes,
                                DraftPassingSiding &siding);
    bool recalculatePassingSiding(const DraftPassingSiding &original,
                                  const QHash<int, int> &switchRoutes,
                                  DraftPassingSiding &siding);
    bool resolvePassingSwitchRoutes(const QVector<int> &vectors,
                                    const QVector<int> &junctionIds,
                                    QHash<int, int> &switchRoutes) const;
    bool selectDraftObject(const QPointF &screenPoint);
    void showTransientMessage(const QString &text, bool error);
    QString waitPointKind(int value) const;
    QString waitPointDescription(int value) const;
    QString waitPointMarker(int value) const;
    static QVector<QLineF> simplifiedLines(const QVector<QLineF> &source, qreal minimumLength);
    void capturePathSessionMetadata(Path *path);
    void restorePathSessionMetadata();

    struct JunctionInfo {
        int nodeId = -1;
        QPointF position;
        int pins[3] = {0, 0, 0};
        int mainRoute = 0;
        int shapeId = -1;
        bool hasShapeMainRoute = false;
        QPointF directions[3];
    };
    enum MapInteractiveType {
        MapSignal, MapStation, MapSiding, MapPickup, MapCrossing, MapMarker
    };
    struct MapInteractive {
        MapInteractiveType type = MapMarker;
        QPointF position;
        QString label;
        int sourceId = -1;
    };

    Route *route = NULL;
    Path *selectedPath = NULL;
    QVector<QLineF> trackSegments;
    QVector<QLineF> trackSegmentsMedium;
    QVector<QLineF> trackSegmentsOverview;
    QHash<int, QVector<QLineF>> vectorSegments;
    QPainterPath selectedPathLine;
    QVector<JunctionInfo> junctions;
    QVector<MapInteractive> mapInteractives;
    QVector<QLineF> platformLines;
    QVector<QLineF> crossingTrackLines;
    QVector<QPointF> endpoints;
    QVector<QPointF> pathPoints;
    QVector<QPointF> savedReversePoints;
    QRectF routeBounds;
    QRectF selectedPathBounds;
    QPointF viewCenter;
    QPointF lastMousePosition;
    qreal pixelsPerMeter = 0.05;
    int viewQuarterTurns = 0;
    int segmentCount = 0;
    int selectedJunction = -1;
    int hoveredJunction = -1;
    bool dragging = false;
    bool showJunctions = true;
    bool showTileGrid = true;
    bool showInteractives = true;
    bool showMarkers = true;
    bool showMapLabels = true;
    bool hasFittedRoute = false;
    bool cacheDirty = false;
    bool creatingPath = false;
    bool draftStartSelected = false;
    bool draftEndSelected = false;
    bool draftForward = true;
    QPointF draftStart;
    QPointF draftEnd;
    int draftStartVector = -1;
    int draftEndVector = -1;
    QVector<QLineF> draftRouteLines;
    QVector<QLineF> draftRouteMedium;
    QVector<QLineF> draftRouteOverview;
    QVector<int> draftRouteLineVectors;
    QVector<int> draftRouteLineSegments;
    QVector<int> draftRouteLineLegs;
    QSet<int> draftRouteVectors;
    QVector<QLineF> draftOverlapLines;
    QVector<QLineF> draftOverlapMedium;
    QVector<QLineF> draftOverlapOverview;
    struct DraftReversePoint {
        QPointF position;
        int vectorNodeId = -1;
    };
    QVector<DraftReversePoint> draftReversePoints;
    struct DraftWaitPoint {
        QPointF position;
        int vectorNodeId = -1;
        int legId = -1;
        int waitSeconds = 60;
    };
    QVector<DraftWaitPoint> draftWaitPoints;
    struct DraftPassingSiding {
        QVector<QLineF> lines;
        QVector<int> vectors;
        QVector<int> junctionIds;
        QPointF start;
        QPointF end;
        int startJunction = -1;
        int endJunction = -1;
        int seedVector = -1;
        QHash<int, int> switchRoutes;
        QHash<int, int> manualSwitchRoutes;
    };
    struct DraftState {
        bool startSelected = false;
        bool endSelected = false;
        bool forward = true;
        QPointF start;
        QPointF end;
        int startVector = -1;
        int endVector = -1;
        QVector<DraftReversePoint> reversePoints;
        QVector<DraftWaitPoint> waitPoints;
        QVector<DraftPassingSiding> passingSidings;
        QVector<QHash<int, int>> legSwitchRoutes;
    };
    DraftState captureDraftState() const;
    void restoreDraftState(const DraftState &state);
    void recordUndoState();
    void setPathPlacementMode(PathPlacementMode mode);
    QVector<DraftPassingSiding> draftPassingSidings;
    QVector<QHash<int, int>> draftLegSwitchRoutes;
    QVector<DraftState> undoStates;
    QVector<DraftState> redoStates;
    enum DraftSelectionType { SelectNone, SelectStart, SelectEnd, SelectReverse, SelectPassingSiding, SelectWait };
    PathPlacementMode pathPlacementMode = PlaceAutomatic;
    DraftSelectionType selectedDraftType = SelectNone;
    int selectedDraftIndex = -1;
    int pendingWaitSeconds = 60;
    bool pendingCreationClick = false;
    QPointF creationPressPosition;
    QString transientMessage;
    bool transientMessageIsError = false;
    bool pathSessionMetadataValid = false;
    QString sessionPath;
    QString sessionName;
    QString sessionNameId;
    QString sessionPathId;
    QString sessionTrPathName;
    QString sessionDisplayName;
    QString sessionTrPathStart;
    QString sessionTrPathEnd;
    unsigned int sessionTrPathFlags = 0;
    bool sessionMetadataConfirmed = false;
};

#endif

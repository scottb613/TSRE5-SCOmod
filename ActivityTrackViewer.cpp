/*  This file is part of TSRE5.
 *
 *  Native 2D TrackDB viewer for the Activity Builder.
 */

#include "ActivityTrackViewer.h"

#include "Game.h"
#include "Path.h"
#include "Route.h"
#include "TDB.h"
#include "TRitem.h"
#include "TRnode.h"
#include "Coords.h"
#include "TrackShape.h"
#include "TSectionDAT.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

ActivityTrackViewer::ActivityTrackViewer(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(500, 400);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    setAutoFillBackground(false);
}

QString ActivityTrackViewer::waitPointKind(int value) const {
    if(value >= 30000 && value < 40000)
        return tr("Wait Until");
    if(value >= 40000 && value < 60000)
        return tr("Uncouple");
    if(value == 60001)
        return tr("Join / Split");
    if(value == 60002)
        return tr("Pass-Red");
    if(value >= 60011 && value <= 60020)
        return tr("Horn");
    return tr("Wait");
}

QString ActivityTrackViewer::waitPointDescription(int value) const {
    if(value >= 30000 && value < 40000){
        const int clockValue = value - 30000;
        return tr("Wait until %1:%2")
            .arg(clockValue / 100, 2, 10, QChar('0'))
            .arg(clockValue % 100, 2, 10, QChar('0'));
    }
    if(value >= 40000 && value < 60000){
        const bool keepRear = value >= 50000;
        const int operation = value - (keepRear ? 50000 : 40000);
        const int cars = operation / 100;
        const int seconds = operation % 100;
        return tr("Uncouple; keep %1 %2 car(s), then pause %3 sec")
            .arg(keepRear ? tr("rear") : tr("front"))
            .arg(cars)
            .arg(seconds);
    }
    if(value == 60001)
        return tr("Join the nearby train and continue the join/split move");
    if(value == 60002)
        return tr("Request permission to pass the next red signal");
    if(value >= 60011 && value <= 60020)
        return tr("Blow the horn for %1 sec").arg(value - 60010);
    return tr("Wait for %1 min %2 sec").arg(value / 60).arg(value % 60);
}

QString ActivityTrackViewer::waitPointMarker(int value) const {
    if(value >= 30000 && value < 40000)
        return tr("T");
    if(value >= 40000 && value < 60000)
        return tr("U");
    if(value == 60001)
        return tr("J");
    if(value == 60002)
        return tr("R");
    if(value >= 60011 && value <= 60020)
        return tr("H");
    return tr("W");
}

void ActivityTrackViewer::setRoute(Route *newRoute){
    route = newRoute;
    viewQuarterTurns = 0;
    hasFittedRoute = false;
    trackSegments.clear();
    trackSegmentsMedium.clear();
    trackSegmentsOverview.clear();
    vectorSegments.clear();
    junctions.clear();
    endpoints.clear();
    mapInteractives.clear();
    platformLines.clear();
    routeBounds = QRectF();
    segmentCount = 0;
    selectedJunction = -1;
    hoveredJunction = -1;
    cacheDirty = true;
    rebuildPathCache();
    if(isVisible())
        ensureCache();
    else {
        updateStatus();
        update();
    }
}

void ActivityTrackViewer::setSelectedPath(Path *path){
    selectedPath = path;
    creatingPath = false;
    draftStartSelected = false;
    draftEndSelected = false;
    draftStartVector = -1;
    draftEndVector = -1;
    draftRouteLines.clear();
    draftRouteMedium.clear();
    draftRouteOverview.clear();
    draftRouteLineVectors.clear();
    draftRouteLineSegments.clear();
    draftRouteLineLegs.clear();
    draftRouteVectors.clear();
    draftOverlapLines.clear();
    draftOverlapMedium.clear();
    draftOverlapOverview.clear();
    draftReversePoints.clear();
    draftWaitPoints.clear();
    draftPassingSidings.clear();
    draftLegSwitchRoutes.clear();
    draftLegSwitchRoutes.push_back(QHash<int, int>());
    undoStates.clear();
    redoStates.clear();
    setPathPlacementMode(PlaceAutomatic);
    selectedDraftType = SelectNone;
    selectedDraftIndex = -1;
    draftForward = true;
    rebuildPathCache();
    updateStatus();
    update();
}

void ActivityTrackViewer::beginPathCreation(Path *path){
    selectedPath = path;
    creatingPath = true;
    draftStartSelected = false;
    draftEndSelected = false;
    draftStartVector = -1;
    draftEndVector = -1;
    draftRouteLines.clear();
    draftRouteMedium.clear();
    draftRouteOverview.clear();
    draftRouteLineVectors.clear();
    draftRouteLineSegments.clear();
    draftRouteLineLegs.clear();
    draftRouteVectors.clear();
    draftOverlapLines.clear();
    draftOverlapMedium.clear();
    draftOverlapOverview.clear();
    draftReversePoints.clear();
    draftWaitPoints.clear();
    draftPassingSidings.clear();
    draftLegSwitchRoutes.clear();
    draftLegSwitchRoutes.push_back(QHash<int, int>());
    undoStates.clear();
    redoStates.clear();
    setPathPlacementMode(PlaceStart);
    selectedDraftType = SelectNone;
    selectedDraftIndex = -1;
    draftForward = true;
    rebuildPathCache();
    emit pathDraftStatus(tr("<b>Creating standalone path: %1</b><br>"
                            "Click a track line to choose the starting point. "
                            "The previous path has been cleared.")
                         .arg(path != NULL ? path->displayName.toHtmlEscaped() : tr("Untitled")));
    updateStatus();
    update();
}

void ActivityTrackViewer::beginPathEdit(Path *path){
    if(path == NULL || path->trPathNode.isEmpty() || path->trackPdp.isEmpty()){
        emit pathDraftStatus(tr("<b>Path cannot be edited</b><br>The selected path has no readable path nodes."));
        return;
    }
    if(cacheDirty)
        rebuildTrackCache();

    selectedPath = path;
    creatingPath = true;
    draftStartSelected = false;
    draftEndSelected = false;
    draftStartVector = -1;
    draftEndVector = -1;
    draftReversePoints.clear();
    draftWaitPoints.clear();
    draftPassingSidings.clear();
    draftLegSwitchRoutes.clear();
    draftRouteLines.clear();
    draftRouteMedium.clear();
    draftRouteOverview.clear();
    draftRouteLineVectors.clear();
    draftRouteLineSegments.clear();
    draftRouteLineLegs.clear();
    draftRouteVectors.clear();
    draftOverlapLines.clear();
    draftOverlapMedium.clear();
    draftOverlapOverview.clear();
    undoStates.clear();
    redoStates.clear();
    selectedDraftType = SelectNone;
    selectedDraftIndex = -1;
    setPathPlacementMode(PlaceAutomatic);
    draftForward = true;

    auto controlPoint = [path](int nodeIndex) {
        if(nodeIndex < 0 || nodeIndex >= path->trPathNode.size() ||
           path->trPathNode[nodeIndex] == NULL)
            return QPointF();
        const unsigned int pdpIndex = path->trPathNode[nodeIndex][3];
        if(pdpIndex >= static_cast<unsigned int>(path->trackPdp.size()) ||
           path->trackPdp[pdpIndex] == NULL)
            return QPointF();
        const float *pdp = path->trackPdp[pdpIndex];
        return QPointF(pdp[0] * 2048.0 + pdp[2],
                       -pdp[1] * 2048.0 - pdp[4]);
    };
    auto controlType = [path](int nodeIndex) {
        if(nodeIndex < 0 || nodeIndex >= path->trPathNode.size() ||
           path->trPathNode[nodeIndex] == NULL)
            return 0;
        const unsigned int pdpIndex = path->trPathNode[nodeIndex][3];
        if(pdpIndex >= static_cast<unsigned int>(path->trackPdp.size()) ||
           path->trackPdp[pdpIndex] == NULL)
            return 0;
        return qRound(path->trackPdp[pdpIndex][5]);
    };
    auto nearestVector = [this](const QPointF &target) {
        int bestVector = -1;
        qreal bestSquared = std::numeric_limits<qreal>::max();
        for(auto it = vectorSegments.constBegin(); it != vectorSegments.constEnd(); ++it){
            for(const QLineF &line : it.value()){
                const QPointF delta = line.p2() - line.p1();
                const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
                qreal t = 0.0;
                if(lengthSquared > 0.000001){
                    const QPointF relative = target - line.p1();
                    t = qBound<qreal>(0.0, (relative.x() * delta.x() +
                                            relative.y() * delta.y()) / lengthSquared, 1.0);
                }
                const QPointF error = target - (line.p1() + delta * t);
                const qreal squared = error.x() * error.x() + error.y() * error.y();
                if(squared < bestSquared){
                    bestSquared = squared;
                    bestVector = it.key();
                }
            }
        }
        return bestVector;
    };
    auto nearestJunctionId = [this](const QPointF &target) {
        int nodeId = -1;
        qreal best = std::numeric_limits<qreal>::max();
        for(const JunctionInfo &junction : junctions){
            const qreal distance = QLineF(target, junction.position).length();
            if(distance < best){
                best = distance;
                nodeId = junction.nodeId;
            }
        }
        return best <= 5.0 ? nodeId : -1;
    };

    QVector<int> mainIndices;
    QSet<int> mainIndexSet;
    unsigned int current = 0;
    for(int guard = 0; guard < path->trPathNode.size() + 1; guard++){
        if(current >= static_cast<unsigned int>(path->trPathNode.size()) ||
           path->trPathNode[current] == NULL)
            break;
        const int index = static_cast<int>(current);
        if(mainIndexSet.contains(index))
            break;
        mainIndices.push_back(index);
        mainIndexSet.insert(index);
        current = path->trPathNode[index][1];
    }

    QVector<int> vectorControls;
    for(int nodeIndex : mainIndices){
        if(controlType(nodeIndex) != 1)
            continue;
        const QPointF point = controlPoint(nodeIndex);
        const int vectorId = nearestVector(point);
        if(vectorId < 0)
            continue;
        vectorControls.push_back(nodeIndex);
        if(vectorControls.size() == 1){
            draftStart = point;
            draftStartVector = vectorId;
            draftStartSelected = true;
        } else if((path->trPathNode[nodeIndex][0] & 1u) != 0u){
            DraftReversePoint reverse;
            reverse.position = point;
            reverse.vectorNodeId = vectorId;
            draftReversePoints.push_back(reverse);
        } else if((path->trPathNode[nodeIndex][0] & 2u) != 0u){
            DraftWaitPoint wait;
            wait.position = point;
            wait.vectorNodeId = vectorId;
            wait.waitSeconds = qBound(1,
                static_cast<int>((path->trPathNode[nodeIndex][0] >> 16) & 0xffffu),
                65535);
            draftWaitPoints.push_back(wait);
        }
        draftEnd = point;
        draftEndVector = vectorId;
        draftEndSelected = true;
    }
    if(vectorControls.size() < 2){
        setSelectedPath(path);
        emit pathDraftStatus(tr("<b>Path cannot be edited safely</b><br>Start and endpoint controls could not be resolved on the TrackDB."));
        return;
    }

    // A saved Path already contains the switch choices used by its main
    // linked list. Seed every reverse leg with those choices; irrelevant
    // switches are harmless, while facing switches reproduce the file exactly.
    path->getMapLines();
    QHash<int, int> savedRoutes;
    QMap<int, int> *directions = path->getJunctionDirections();
    if(directions != NULL){
        for(auto it = directions->constBegin(); it != directions->constEnd(); ++it)
            savedRoutes.insert(it.key(), qBound(1, it.value() + 1, 2));
    }
    for(int leg = 0; leg <= draftReversePoints.size(); leg++)
        draftLegSwitchRoutes.push_back(savedRoutes);
    if(!computeDraftRoute()){
        setSelectedPath(path);
        emit pathDraftStatus(tr("<b>Path cannot be edited safely</b><br>The saved main route could not be reconstructed through its switches."));
        return;
    }
    // Associate imported wait nodes with the reconstructed reverse leg.
    for(DraftWaitPoint &wait : draftWaitPoints){
        qreal best = std::numeric_limits<qreal>::max();
        for(int i = 0; i < draftRouteLines.size(); i++){
            if(i >= draftRouteLineVectors.size() ||
               draftRouteLineVectors[i] != wait.vectorNodeId)
                continue;
            const QLineF &line = draftRouteLines[i];
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = wait.position - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() +
                                        relative.y() * delta.y()) / lengthSquared, 1.0);
            }
            const qreal distance = QLineF(wait.position, line.p1() + delta * t).length();
            if(distance < best){
                best = distance;
                wait.legId = i < draftRouteLineLegs.size() ? draftRouteLineLegs[i] : 0;
            }
        }
    }

    // Recreate each saved passing branch from the alternate vector at its
    // main-path switch. The TrackDB graph supplies the branch geometry; the
    // saved nextSiding link supplies the exact pair of boundary switches.
    int passingCount = 0;
    bool passingImportFailed = false;
    QHash<int, int> combinedRoutes;
    for(const QHash<int, int> &legRoutes : draftLegSwitchRoutes){
        for(auto it = legRoutes.constBegin(); it != legRoutes.constEnd(); ++it)
            combinedRoutes.insert(it.key(), it.value());
    }
    for(int mainNodeIndex : mainIndices){
        const unsigned int branchValue = path->trPathNode[mainNodeIndex][2];
        if(branchValue >= static_cast<unsigned int>(path->trPathNode.size()))
            continue;
        int branchIndex = static_cast<int>(branchValue);
        QSet<int> visitedBranch;
        QVector<int> savedInternalJunctions;
        while(branchIndex >= 0 && branchIndex < path->trPathNode.size() &&
              !mainIndexSet.contains(branchIndex) && !visitedBranch.contains(branchIndex)){
            visitedBranch.insert(branchIndex);
            if(controlType(branchIndex) == 2){
                const int junctionId = nearestJunctionId(controlPoint(branchIndex));
                if(junctionId > 0 && !savedInternalJunctions.contains(junctionId))
                    savedInternalJunctions.push_back(junctionId);
            }
            const unsigned int next = path->trPathNode[branchIndex][2];
            branchIndex = next < static_cast<unsigned int>(path->trPathNode.size())
                ? static_cast<int>(next) : -1;
        }
        if(branchIndex < 0 || !mainIndexSet.contains(branchIndex)){
            passingImportFailed = true;
            continue;
        }
        const int startJunction = nearestJunctionId(controlPoint(mainNodeIndex));
        const int endJunction = nearestJunctionId(controlPoint(branchIndex));
        const JunctionInfo *startInfo = NULL;
        for(const JunctionInfo &junction : junctions){
            if(junction.nodeId == startJunction){
                startInfo = &junction;
                break;
            }
        }
        bool imported = false;

        // TSRE-written passing paths contain their exact ordered internal
        // junctions. Rebuild that saved route directly instead of asking the
        // automatic finder to choose its default (usually longest) branch.
        // This preserves a passing path shortened by throwing one of its
        // switches before saving.
        QVector<int> savedJunctionSequence;
        if(startJunction > 0)
            savedJunctionSequence.push_back(startJunction);
        for(int junctionId : savedInternalJunctions){
            if(junctionId != startJunction && junctionId != endJunction)
                savedJunctionSequence.push_back(junctionId);
        }
        if(endJunction > 0 && endJunction != startJunction)
            savedJunctionSequence.push_back(endJunction);

        QVector<int> savedVectors;
        bool exactRoute = savedJunctionSequence.size() >= 2;
        for(int sequenceIndex = 0;
            exactRoute && sequenceIndex + 1 < savedJunctionSequence.size();
            sequenceIndex++){
            const JunctionInfo *a = NULL;
            const JunctionInfo *b = NULL;
            for(const JunctionInfo &junction : junctions){
                if(junction.nodeId == savedJunctionSequence[sequenceIndex])
                    a = &junction;
                if(junction.nodeId == savedJunctionSequence[sequenceIndex + 1])
                    b = &junction;
            }
            int sharedVector = -1;
            if(a != NULL && b != NULL){
                for(int aPin = 0; aPin < 3 && sharedVector < 0; aPin++){
                    if(a->pins[aPin] <= 0)
                        continue;
                    for(int bPin = 0; bPin < 3; bPin++){
                        if(a->pins[aPin] == b->pins[bPin]){
                            sharedVector = a->pins[aPin];
                            break;
                        }
                    }
                }
            }
            if(sharedVector <= 0)
                exactRoute = false;
            else
                savedVectors.push_back(sharedVector);
        }

        if(exactRoute && !savedVectors.isEmpty()){
            const JunctionInfo *endInfo = NULL;
            for(const JunctionInfo &junction : junctions){
                if(junction.nodeId == endJunction){
                    endInfo = &junction;
                    break;
                }
            }
            QHash<int, int> exactSwitchRoutes;
            bool exactSwitchRouteFound = false;
            if(startInfo != NULL && endInfo != NULL){
                for(int startPin = 0; startPin < 3 && !exactSwitchRouteFound; startPin++){
                    const int mainStartVector = startInfo->pins[startPin];
                    if(mainStartVector <= 0 ||
                       mainStartVector == savedVectors.first() ||
                       !draftRouteVectors.contains(mainStartVector))
                        continue;
                    for(int endPin = 0; endPin < 3 && !exactSwitchRouteFound; endPin++){
                        const int mainEndVector = endInfo->pins[endPin];
                        if(mainEndVector <= 0 ||
                           mainEndVector == savedVectors.last() ||
                           !draftRouteVectors.contains(mainEndVector))
                            continue;
                        QVector<int> verificationVectors;
                        verificationVectors.push_back(mainStartVector);
                        verificationVectors += savedVectors;
                        verificationVectors.push_back(mainEndVector);
                        QHash<int, int> candidateRoutes;
                        if(resolvePassingSwitchRoutes(verificationVectors,
                                                      savedJunctionSequence,
                                                      candidateRoutes)){
                            exactSwitchRoutes = candidateRoutes;
                            exactSwitchRouteFound = true;
                        }
                    }
                }
            }
            if(exactSwitchRouteFound){
                DraftPassingSiding siding;
                siding.seedVector = savedVectors[savedVectors.size() / 2];
                siding.vectors = savedVectors;
                siding.junctionIds = savedInternalJunctions;
                siding.switchRoutes = exactSwitchRoutes;
                for(int junctionId : savedInternalJunctions){
                    if(exactSwitchRoutes.contains(junctionId))
                        siding.manualSwitchRoutes.insert(
                            junctionId, exactSwitchRoutes.value(junctionId));
                }
                siding.startJunction = startJunction;
                siding.endJunction = endJunction;
                siding.start = junctionPosition(startJunction);
                siding.end = junctionPosition(endJunction);
                for(int vectorIndex = 0; vectorIndex < siding.vectors.size(); vectorIndex++){
                    const QPointF from = vectorIndex == 0
                        ? siding.start
                        : junctionPosition(savedJunctionSequence[vectorIndex]);
                    const QPointF to = vectorIndex == siding.vectors.size() - 1
                        ? siding.end
                        : junctionPosition(savedJunctionSequence[vectorIndex + 1]);
                    appendVectorSlice(siding.vectors[vectorIndex], from, to, siding.lines);
                }
                if(!siding.lines.isEmpty()){
                    draftPassingSidings.push_back(siding);
                    passingCount++;
                    imported = true;
                }
            }
        }

        // Older or hand-authored PAT files may omit the internal branch
        // junctions. Retain the graph-based importer as their fallback.
        if(startInfo != NULL){
            for(int pin = 0; pin < 3 && !imported; pin++){
                const int seedVector = startInfo->pins[pin];
                if(seedVector <= 0 || draftRouteVectors.contains(seedVector))
                    continue;
                DraftPassingSiding siding;
                if(!calculatePassingSiding(seedVector, combinedRoutes, siding))
                    continue;
                const bool sameBoundaries =
                    (siding.startJunction == startJunction && siding.endJunction == endJunction) ||
                    (siding.startJunction == endJunction && siding.endJunction == startJunction);
                if(!sameBoundaries)
                    continue;
                draftPassingSidings.push_back(siding);
                passingCount++;
                imported = true;
            }
        }
        if(!imported)
            passingImportFailed = true;
    }
    if(passingImportFailed){
        setSelectedPath(path);
        emit pathDraftStatus(tr("<b>Path left read-only</b><br>A saved passing branch could not be reconstructed exactly, so no destructive edit was opened."));
        showTransientMessage(tr("EDIT NOT OPENED - passing path could not be reconstructed"), true);
        return;
    }

    rebuildOverlapLines();
    selectedPathLine = QPainterPath();
    pathPoints.clear();
    emit pathDraftStatus(tr("<b>Editing standalone path: %1</b><br>"
                            "%2 reverse point(s), %3 wait point(s), %4 passing siding(s). Click a point to select it; press Delete to remove it.")
                         .arg(path->displayName.toHtmlEscaped())
                         .arg(draftReversePoints.size()).arg(draftWaitPoints.size()).arg(passingCount));
    updateStatus();
    update();
}

void ActivityTrackViewer::choosePathStart(){
    if(!creatingPath){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>No editable path</b><br>Create or edit a standalone path first."));
        return;
    }
    if(pathPlacementMode == PlaceStart){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Place Start cancelled</b>"));
        return;
    }
    setPathPlacementMode(PlaceStart);
    emit pathDraftStatus(tr("<b>Set Start</b><br>Click the track where the train begins."));
    update();
}

void ActivityTrackViewer::choosePathEnd(){
    if(!creatingPath || !draftStartSelected || draftRouteLines.isEmpty()){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Set the path start first</b><br>An endpoint must be placed on an existing magenta path."));
        return;
    }
    if(pathPlacementMode == PlaceEnd){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Place Endpoint cancelled</b>"));
        return;
    }
    setPathPlacementMode(PlaceEnd);
    emit pathDraftStatus(tr("<b>Set End</b><br>Click the track where the path ends."));
    update();
}

void ActivityTrackViewer::reverseStartDirection(){
    setPathPlacementMode(PlaceAutomatic);
    if(!creatingPath || !draftStartSelected){
        emit pathDraftStatus(tr("<b>Set the path start first</b>"));
        return;
    }
    recordUndoState();
    draftForward = !draftForward;
    draftEndSelected = false;
    draftWaitPoints.clear();
    draftPassingSidings.clear();
    computeDraftRoute();
    emit pathDraftStatus(tr("<b>Start direction reversed</b><br>The endpoint was cleared; magenta now flows to the natural track end."));
    update();
}

void ActivityTrackViewer::choosePathReverse(){
    if(!creatingPath || !draftStartSelected){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Set the path start first</b><br>A reverse point needs an existing path start."));
        return;
    }
    if(pathPlacementMode == PlaceReverse){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Reverse Point cancelled</b>"));
        return;
    }
    setPathPlacementMode(PlaceReverse);
    emit pathDraftStatus(tr("<b>Add Reverse Point</b><br>Click the track where the train changes direction."));
    update();
}

void ActivityTrackViewer::choosePassingSiding(){
    if(!creatingPath || !draftStartSelected || draftRouteLines.isEmpty()){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Build the main path first</b><br>A passing siding must reconnect to an existing magenta path."));
        return;
    }
    if(pathPlacementMode == PlacePassingSiding){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Add Passing Siding cancelled</b>"));
        return;
    }
    setPathPlacementMode(PlacePassingSiding);
    emit pathDraftStatus(tr("<b>Add Passing Siding</b><br>Click anywhere on the alternate siding track. TSRE will find both connections to the main path."));
    update();
}

void ActivityTrackViewer::choosePathWait(int waitSeconds){
    if(!creatingPath || !draftStartSelected || draftRouteLines.isEmpty()){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Build the main path first</b><br>A path control point must be placed on the magenta path."));
        return;
    }
    if(pathPlacementMode == PlaceWait && pendingWaitSeconds == waitSeconds){
        setPathPlacementMode(PlaceAutomatic);
        emit pathDraftStatus(tr("<b>Path control placement cancelled</b>"));
        return;
    }
    pendingWaitSeconds = qBound(1, waitSeconds, 65535);
    setPathPlacementMode(PlaceWait);
    emit pathDraftStatus(tr("<b>Place %1 Point</b><br>%2.<br>Click the magenta path.")
                         .arg(waitPointKind(pendingWaitSeconds).toHtmlEscaped())
                         .arg(waitPointDescription(pendingWaitSeconds).toHtmlEscaped()));
    update();
}

void ActivityTrackViewer::setPathPlacementMode(PathPlacementMode mode){
    pathPlacementMode = mode;
    emit pathPlacementModeChanged(static_cast<int>(mode));
    update();
}

void ActivityTrackViewer::removeLastReverse(){
    if(draftReversePoints.isEmpty()){
        emit pathDraftStatus(tr("<b>No reverse points to remove</b>"));
        return;
    }
    recordUndoState();
    draftReversePoints.removeLast();
    draftWaitPoints.clear();
    draftPassingSidings.clear();
    if(draftLegSwitchRoutes.size() > 1)
        draftLegSwitchRoutes.removeLast();
    const bool valid = !draftEndSelected || computeDraftRoute();
    emit pathDraftStatus(tr("<b>Last reverse point removed</b><br>%1 reverse point(s).%2")
                         .arg(draftReversePoints.size())
                         .arg(valid ? QString() : tr(" The remaining route is not continuous.")));
    update();
}

void ActivityTrackViewer::validateDraftPath(){
    setPathPlacementMode(PlaceAutomatic);
    if(!draftStartSelected || !draftEndSelected){
        emit pathDraftStatus(tr("<b>Path incomplete</b><br>Both a start and an end are required."));
        return;
    }
    if(!computeDraftRoute()){
        emit pathDraftStatus(tr("<b>Path invalid</b><br>One or more legs cannot be connected through the selected switches."));
        return;
    }
    for(const DraftWaitPoint &wait : draftWaitPoints){
        bool onRoute = false;
        for(int i = 0; i < draftRouteLineVectors.size(); i++){
            if(draftRouteLineVectors[i] == wait.vectorNodeId &&
               i < draftRouteLineLegs.size() && draftRouteLineLegs[i] == wait.legId){
                onRoute = true;
                break;
            }
        }
        if(!onRoute){
            emit pathDraftStatus(tr("<b>Path needs attention</b><br>A wait point is no longer on the current magenta route."));
            showTransientMessage(tr("CHECK FAILED - wait point is off the current route"), true);
            return;
        }
    }
    emit pathDraftStatus(tr("<b>Path preview is continuous</b><br>%1 reverse point(s), %2 wait point(s), %3 manually set switch(es), %4 passing siding(s).%5")
                         .arg(draftReversePoints.size())
                         .arg(draftWaitPoints.size())
                         .arg(draftLegSwitchRoutes.isEmpty() ? 0 : draftLegSwitchRoutes.last().size())
                         .arg(draftPassingSidings.size())
                         .arg(draftOverlapLines.isEmpty()
                              ? tr(" No overlapping track.")
                             : tr(" Cyan marks track used more than once.")));
}

void ActivityTrackViewer::saveDraftPath(){
    setPathPlacementMode(PlaceAutomatic);
    if(selectedPath == NULL || !creatingPath){
        emit pathDraftStatus(tr("<b>No editable path selected</b><br>Create or edit a standalone path first."));
        return;
    }
    if(!draftStartSelected || !draftEndSelected){
        emit pathDraftStatus(tr("<b>Path incomplete</b><br>Set both a start and an endpoint before saving."));
        showTransientMessage(tr("PATH NOT SAVED - start and endpoint required"), true);
        return;
    }
    if(!computeDraftRoute()){
        emit pathDraftStatus(tr("<b>Path not saved</b><br>The main route is not continuous through the selected switches."));
        showTransientMessage(tr("PATH NOT SAVED - route is not continuous"), true);
        return;
    }

    struct SaveControl {
        QPointF position;
        int type = 1;              // TrackPDP type: 1 vector, 2 junction
        int vectorId = -1;
        int junctionId = -1;
        unsigned int flags = 0;    // TrPathNode flags; bit 0 is reverse
        int nextMain = -1;
        int nextSiding = -1;
    };
    QVector<SaveControl> controls;
    auto appendVectorControl = [&controls](const QPointF &position, int vectorId, unsigned int flags) {
        SaveControl control;
        control.position = position;
        control.vectorId = vectorId;
        control.flags = flags;
        controls.push_back(control);
    };
    auto appendJunctionControl = [this, &controls](int junctionId) {
        if(!controls.isEmpty() && controls.last().type == 2 &&
           controls.last().junctionId == junctionId)
            return;
        SaveControl control;
        control.type = 2;
        control.junctionId = junctionId;
        control.position = junctionPosition(junctionId);
        controls.push_back(control);
    };

    appendVectorControl(draftStart, draftStartVector, 0);
    QPointF legStart = draftStart;
    int legStartVector = draftStartVector;
    for(int reverseIndex = 0; reverseIndex <= draftReversePoints.size(); reverseIndex++){
        const bool finalLeg = reverseIndex == draftReversePoints.size();
        const QPointF legEnd = finalLeg ? draftEnd : draftReversePoints[reverseIndex].position;
        const int legEndVector = finalLeg ? draftEndVector : draftReversePoints[reverseIndex].vectorNodeId;
        QVector<QLineF> ignoredLines;
        QVector<int> orderedVectors;
        QVector<int> orderedJunctions;
        QHash<int, int> saveLegRoutes = draftLegSwitchRoutes.value(reverseIndex);
        if(!computeRouteLeg(legStart, legStartVector, legEnd, legEndVector,
                            saveLegRoutes, ignoredLines,
                            &orderedVectors, &orderedJunctions)){
            emit pathDraftStatus(tr("<b>Path not saved</b><br>Leg %1 could not be reconstructed.")
                                 .arg(reverseIndex + 1));
            showTransientMessage(tr("PATH NOT SAVED - route reconstruction failed"), true);
            return;
        }
        QSet<int> savedWaits;
        for(int vectorIndex = 0; vectorIndex < orderedVectors.size(); vectorIndex++){
            const int vectorId = orderedVectors[vectorIndex];
            const QPointF vectorFrom = vectorIndex == 0
                ? legStart : junctionPosition(orderedJunctions.value(vectorIndex - 1));
            QVector<int> waitsOnVector;
            for(int waitIndex = 0; waitIndex < draftWaitPoints.size(); waitIndex++){
                const DraftWaitPoint &wait = draftWaitPoints[waitIndex];
                if(wait.legId == reverseIndex && wait.vectorNodeId == vectorId)
                    waitsOnVector.push_back(waitIndex);
            }
            std::sort(waitsOnVector.begin(), waitsOnVector.end(),
                      [this, &vectorFrom](int a, int b) {
                return QLineF(vectorFrom, draftWaitPoints[a].position).length() <
                       QLineF(vectorFrom, draftWaitPoints[b].position).length();
            });
            for(int waitIndex : waitsOnVector){
                if(savedWaits.contains(waitIndex))
                    continue;
                const DraftWaitPoint &wait = draftWaitPoints[waitIndex];
                const unsigned int flags =
                    (static_cast<unsigned int>(qBound(1, wait.waitSeconds, 65535)) << 16) | 2u;
                appendVectorControl(wait.position, wait.vectorNodeId, flags);
                savedWaits.insert(waitIndex);
            }
            if(vectorIndex < orderedJunctions.size())
                appendJunctionControl(orderedJunctions[vectorIndex]);
        }
        int expectedWaits = 0;
        for(const DraftWaitPoint &wait : draftWaitPoints){
            if(wait.legId == reverseIndex)
                expectedWaits++;
        }
        if(savedWaits.size() != expectedWaits){
            emit pathDraftStatus(tr("<b>Path not saved</b><br>A wait point is no longer on leg %1.")
                                 .arg(reverseIndex + 1));
            showTransientMessage(tr("PATH NOT SAVED - wait point is off the current route"), true);
            return;
        }
        appendVectorControl(legEnd, legEndVector, finalLeg ? 0u : 1u);
        legStart = legEnd;
        legStartVector = legEndVector;
    }
    if(controls.size() < 2){
        showTransientMessage(tr("PATH NOT SAVED - too few control points"), true);
        return;
    }
    for(int i = 0; i + 1 < controls.size(); i++)
        controls[i].nextMain = i + 1;

    // Add passing paths as alternate linked lists. Keeping the main controls
    // first makes their indexes stable while any number of siding nodes follow.
    for(const DraftPassingSiding &siding : draftPassingSidings){
        int startIndex = -1;
        int endIndex = -1;
        for(int i = 0; i < controls.size(); i++){
            if(controls[i].type != 2)
                continue;
            if(startIndex < 0 && controls[i].junctionId == siding.startJunction)
                startIndex = i;
            if(startIndex >= 0 && controls[i].junctionId == siding.endJunction){
                endIndex = i;
                break;
            }
        }
        if(startIndex < 0 || endIndex <= startIndex){
            emit pathDraftStatus(tr("<b>Path not saved</b><br>An orange passing path does not reconnect to the ordered main path."));
            showTransientMessage(tr("PATH NOT SAVED - passing path boundary mismatch"), true);
            return;
        }
        if(controls[startIndex].nextSiding >= 0){
            emit pathDraftStatus(tr("<b>Path not saved</b><br>Two passing paths begin at the same main-path switch."));
            showTransientMessage(tr("PATH NOT SAVED - overlapping passing starts"), true);
            return;
        }
        if(siding.junctionIds.isEmpty()){
            controls[startIndex].nextSiding = endIndex;
            continue;
        }
        const int firstSidingIndex = controls.size();
        controls[startIndex].nextSiding = firstSidingIndex;
        for(int i = 0; i < siding.junctionIds.size(); i++){
            SaveControl control;
            control.type = 2;
            control.junctionId = siding.junctionIds[i];
            control.position = junctionPosition(control.junctionId);
            control.nextSiding = i + 1 < siding.junctionIds.size()
                ? controls.size() + 1 : endIndex;
            controls.push_back(control);
        }
    }

    struct Pdp {
        int tileX = 0;
        int tileZ = 0;
        float x = 0;
        float y = 0;
        float z = 0;
        int type = 1;
        int extra = 0;
    };
    QVector<Pdp> pdps;
    QVector<int> controlPdp;
    QHash<int, int> junctionPdp;
    TDB *tdb = Game::trackDB;
    for(const SaveControl &control : controls){
        if(control.type == 2 && junctionPdp.contains(control.junctionId)){
            controlPdp.push_back(junctionPdp.value(control.junctionId));
            continue;
        }
        Pdp pdp;
        pdp.type = control.type;
        pdp.extra = control.type == 1 && control.flags != 0 ? 1 : 0;
        if(control.type == 2 && tdb != NULL && tdb->trackNodes[control.junctionId] != NULL){
            TRnode *node = tdb->trackNodes[control.junctionId];
            pdp.tileX = qRound(node->UiD[4]);
            pdp.tileZ = qRound(node->UiD[5]);
            pdp.x = node->UiD[6];
            pdp.y = node->UiD[7];
            pdp.z = node->UiD[8];
        } else if(tdb != NULL && tdb->trackNodes[control.vectorId] != NULL &&
                  tdb->trackNodes[control.vectorId]->iTrv > 0){
            TRnode *node = tdb->trackNodes[control.vectorId];
            const int anchorTileX = qRound(node->trVectorSection[0].param[8]);
            const int anchorTileZ = qRound(node->trVectorSection[0].param[9]);
            float *buffer = NULL;
            int length = 0;
            tdb->getVectorSectionLine(buffer, length, anchorTileX, -anchorTileZ,
                                      control.vectorId, false, 1);
            qreal best = std::numeric_limits<qreal>::max();
            for(int i = 0; buffer != NULL && i + 11 < length; i += 12){
                const QPointF a(buffer[i] + anchorTileX * 2048.0,
                                buffer[i + 2] - anchorTileZ * 2048.0);
                const QPointF b(buffer[i + 6] + anchorTileX * 2048.0,
                                buffer[i + 8] - anchorTileZ * 2048.0);
                const QPointF delta = b - a;
                const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
                qreal t = 0.0;
                if(lengthSquared > 0.000001){
                    const QPointF relative = control.position - a;
                    t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                            lengthSquared, 1.0);
                }
                const qreal error = QLineF(control.position, a + delta * t).length();
                if(error < best){
                    best = error;
                    pdp.tileX = anchorTileX;
                    pdp.tileZ = anchorTileZ;
                    pdp.x = buffer[i] + (buffer[i + 6] - buffer[i]) * t;
                    pdp.y = buffer[i + 1] + (buffer[i + 7] - buffer[i + 1]) * t;
                    pdp.z = -(buffer[i + 2] + (buffer[i + 8] - buffer[i + 2]) * t);
                }
            }
            delete[] buffer;
            Game::check_coords(pdp.tileX, pdp.tileZ, pdp.x, pdp.z);
        } else {
            pdp.tileX = qFloor((control.position.x() + 1024.0) / 2048.0);
            pdp.tileZ = qFloor((-control.position.y() + 1024.0) / 2048.0);
            pdp.x = control.position.x() - pdp.tileX * 2048.0;
            pdp.z = -control.position.y() - pdp.tileZ * 2048.0;
        }
        const int pdpIndex = pdps.size();
        pdps.push_back(pdp);
        controlPdp.push_back(pdpIndex);
        if(control.type == 2)
            junctionPdp.insert(control.junctionId, pdpIndex);
    }

    QSaveFile file(selectedPath->pathid);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        emit pathDraftStatus(tr("<b>Path save failed</b><br>%1").arg(file.errorString().toHtmlEscaped()));
        showTransientMessage(tr("PATH SAVE FAILED"), true);
        return;
    }
    QTextStream out(&file);
    out.setCodec("UTF-16");
    out.setGenerateByteOrderMark(true);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(3);
    auto quoted = [](QString text) {
        text.replace('"', "''");
        return QString("\"%1\"").arg(text);
    };
    auto nextValue = [](int index) -> quint32 {
        return index < 0 ? 0xffffffffu : static_cast<quint32>(index);
    };
    const QString pathName = selectedPath->trPathName.isEmpty()
        ? selectedPath->nameId : selectedPath->trPathName;
    if(selectedPath->trPathStart.isEmpty())
        selectedPath->trPathStart = tr("START");
    if(selectedPath->trPathEnd.isEmpty())
        selectedPath->trPathEnd = tr("END");
    out << "SIMISA@@@@@@@@@@JINX0P0t______\n\n";
    out << "Serial ( 1 )\n";
    out << "TrackPDPs (\n";
    for(const Pdp &pdp : pdps)
        out << "\tTrackPDP ( " << pdp.tileX << " " << pdp.tileZ << " "
            << pdp.x << " " << pdp.y << " " << pdp.z << " "
            << pdp.type << " " << pdp.extra << " )\n";
    out << ")\n";
    out << "TrackPath (\n";
    out << "\tTrPathName ( " << quoted(pathName) << " )\n";
    out << "\tTrPathFlags ( 00000000 )\n";
    out << "\tName ( " << quoted(selectedPath->displayName) << " )\n";
    out << "\tTrPathStart ( " << quoted(selectedPath->trPathStart) << " )\n";
    out << "\tTrPathEnd ( " << quoted(selectedPath->trPathEnd) << " )\n";
    out << "\tTrPathNodes ( " << controls.size() << "\n";
    for(int i = 0; i < controls.size(); i++){
        const SaveControl &control = controls[i];
        out << "\t\tTrPathNode ( "
            << QString("%1").arg(control.flags, 8, 16, QChar('0')) << " "
            << nextValue(control.nextMain) << " " << nextValue(control.nextSiding) << " "
            << controlPdp[i] << " )\n";
    }
    out << "\t)\n)\n";
    out.flush();
    if(out.status() != QTextStream::Ok || !file.commit()){
        emit pathDraftStatus(tr("<b>Path save failed</b><br>The file could not be committed safely."));
        showTransientMessage(tr("PATH SAVE FAILED"), true);
        return;
    }

    for(float *pdp : selectedPath->trackPdp)
        delete[] pdp;
    for(unsigned int *node : selectedPath->trPathNode)
        delete[] node;
    selectedPath->trackPdp.clear();
    selectedPath->trPathNode.clear();
    selectedPath->node.clear();
    for(const Pdp &pdp : pdps){
        float *values = new float[7];
        values[0] = pdp.tileX; values[1] = pdp.tileZ;
        values[2] = pdp.x; values[3] = pdp.y; values[4] = pdp.z;
        values[5] = pdp.type; values[6] = pdp.extra;
        selectedPath->trackPdp.push_back(values);
    }
    for(int i = 0; i < controls.size(); i++){
        unsigned int *values = new unsigned int[4];
        values[0] = controls[i].flags;
        values[1] = nextValue(controls[i].nextMain);
        values[2] = nextValue(controls[i].nextSiding);
        values[3] = controlPdp[i];
        selectedPath->trPathNode.push_back(values);
    }
    selectedPath->trPathName = pathName;
    selectedPath->trPathFlags = 0;
    selectedPath->initRoute();
    selectedPath->markSaved();
    const int savedControlCount = controls.size();
    const int savedPassingCount = draftPassingSidings.size();
    const QString savedPathId = selectedPath->pathid;
    Path *savedPath = selectedPath;
    // Saving completes the edit session. Rebuild the normal selected-path
    // cache immediately so magenta/orange changes to saved yellow without
    // requiring the user to select another path first.
    setSelectedPath(savedPath);
    showTransientMessage(tr("PATH SAVED - %1 nodes, %2 passing path(s)")
                         .arg(savedControlCount).arg(savedPassingCount), false);
    emit pathDraftStatus(tr("<b>Path saved</b><br>%1<br>%2 main/passing nodes written in MSTS/ORTS format.")
                         .arg(savedPathId.toHtmlEscaped()).arg(savedControlCount));
}

ActivityTrackViewer::DraftState ActivityTrackViewer::captureDraftState() const {
    DraftState state;
    state.startSelected = draftStartSelected;
    state.endSelected = draftEndSelected;
    state.forward = draftForward;
    state.start = draftStart;
    state.end = draftEnd;
    state.startVector = draftStartVector;
    state.endVector = draftEndVector;
    state.reversePoints = draftReversePoints;
    state.waitPoints = draftWaitPoints;
    state.passingSidings = draftPassingSidings;
    state.legSwitchRoutes = draftLegSwitchRoutes;
    return state;
}

void ActivityTrackViewer::recordUndoState(){
    undoStates.push_back(captureDraftState());
    if(undoStates.size() > 100)
        undoStates.removeFirst();
    redoStates.clear();
}

void ActivityTrackViewer::restoreDraftState(const DraftState &state){
    draftStartSelected = state.startSelected;
    draftEndSelected = state.endSelected;
    draftForward = state.forward;
    draftStart = state.start;
    draftEnd = state.end;
    draftStartVector = state.startVector;
    draftEndVector = state.endVector;
    draftReversePoints = state.reversePoints;
    draftWaitPoints = state.waitPoints;
    draftPassingSidings = state.passingSidings;
    draftLegSwitchRoutes = state.legSwitchRoutes;
    if(draftLegSwitchRoutes.isEmpty())
        draftLegSwitchRoutes.push_back(QHash<int, int>());
    setPathPlacementMode(PlaceAutomatic);
    selectedDraftType = SelectNone;
    selectedDraftIndex = -1;
    if(draftStartSelected)
        computeDraftRoute();
    else {
        draftRouteLines.clear();
        draftRouteMedium.clear();
        draftRouteOverview.clear();
        draftOverlapLines.clear();
        draftOverlapMedium.clear();
        draftOverlapOverview.clear();
    }
    update();
}

void ActivityTrackViewer::undoDraftEdit(){
    setPathPlacementMode(PlaceAutomatic);
    if(undoStates.isEmpty()){
        emit pathDraftStatus(tr("<b>Nothing to undo</b>"));
        return;
    }
    redoStates.push_back(captureDraftState());
    const DraftState state = undoStates.takeLast();
    restoreDraftState(state);
    emit pathDraftStatus(tr("<b>Undo</b><br>The previous path state has been restored."));
}

void ActivityTrackViewer::redoDraftEdit(){
    setPathPlacementMode(PlaceAutomatic);
    if(redoStates.isEmpty()){
        emit pathDraftStatus(tr("<b>Nothing to redo</b>"));
        return;
    }
    undoStates.push_back(captureDraftState());
    const DraftState state = redoStates.takeLast();
    restoreDraftState(state);
    emit pathDraftStatus(tr("<b>Redo</b><br>The path edit has been restored."));
}

void ActivityTrackViewer::deleteSelectedDraftObject(){
    if(selectedDraftType == SelectNone){
        emit pathDraftStatus(tr("<b>Nothing selected</b><br>Click a start, endpoint, wait point, reverse diamond, or orange passing siding first."));
        return;
    }
    recordUndoState();
    if(selectedDraftType == SelectStart){
        draftStartSelected = false;
        draftEndSelected = false;
        draftReversePoints.clear();
        draftWaitPoints.clear();
        draftPassingSidings.clear();
        draftLegSwitchRoutes.clear();
        draftLegSwitchRoutes.push_back(QHash<int, int>());
    } else if(selectedDraftType == SelectEnd){
        draftEndSelected = false;
    } else if(selectedDraftType == SelectReverse && selectedDraftIndex >= 0 &&
              selectedDraftIndex < draftReversePoints.size()){
        draftReversePoints.remove(selectedDraftIndex);
        if(selectedDraftIndex + 1 < draftLegSwitchRoutes.size())
            draftLegSwitchRoutes.remove(selectedDraftIndex + 1);
        draftEndSelected = false;
        draftWaitPoints.clear();
        draftPassingSidings.clear();
    } else if(selectedDraftType == SelectWait && selectedDraftIndex >= 0 &&
              selectedDraftIndex < draftWaitPoints.size()){
        draftWaitPoints.remove(selectedDraftIndex);
    } else if(selectedDraftType == SelectPassingSiding && selectedDraftIndex >= 0 &&
              selectedDraftIndex < draftPassingSidings.size()){
        draftPassingSidings.remove(selectedDraftIndex);
    }
    selectedDraftType = SelectNone;
    selectedDraftIndex = -1;
    if(draftStartSelected)
        computeDraftRoute();
    else {
        draftRouteLines.clear();
        draftRouteMedium.clear();
        draftRouteOverview.clear();
    }
    emit pathDraftStatus(tr("<b>Selected path object deleted</b><br>Use Undo to restore it."));
    update();
}

int ActivityTrackViewer::trackSegmentCount() const {
    return segmentCount;
}

void ActivityTrackViewer::ensureCache(){
    if(!cacheDirty)
        return;
    rebuildTrackCache();
    QTimer::singleShot(0, this, SLOT(fitRoute()));
}

void ActivityTrackViewer::rebuildTrackCache(){
    cacheDirty = false;
    trackSegments.clear();
    trackSegmentsMedium.clear();
    trackSegmentsOverview.clear();
    vectorSegments.clear();
    junctions.clear();
    endpoints.clear();
    mapInteractives.clear();
    platformLines.clear();
    routeBounds = QRectF();
    segmentCount = 0;
    selectedJunction = -1;
    hoveredJunction = -1;

    if(route == NULL || Game::trackDB == NULL || !Game::trackDB->loaded){
        updateStatus();
        update();
        return;
    }

    TDB *tdb = Game::trackDB;
    for(const auto &entry : tdb->trackNodes){
        TRnode *node = entry.second;
        if(node == NULL || node->typ < 0)
            continue;

        if(node->typ == 1 && node->iTrv > 0){
            float *lineBuffer = NULL;
            int length = 0;
            // Keep the generated floats close to zero. At route-scale absolute
            // coordinates (often tens of millions of metres), a float no longer
            // has sub-metre precision and diagonal track becomes stair-stepped.
            const int anchorTileX = qRound(node->trVectorSection[0].param[8]);
            const int anchorTileZ = qRound(node->trVectorSection[0].param[9]);
            const QPointF worldOffset(anchorTileX * 2048.0, -anchorTileZ * 2048.0);
            // One-metre chords preserve switch and tight-curve geometry at deep zoom.
            tdb->getVectorSectionLine(lineBuffer, length, anchorTileX, -anchorTileZ,
                                      entry.first, false, 1);
            if(lineBuffer != NULL){
                QVector<QLineF> nodeLines;
                QPointF previousEnd;
                bool hasPreviousEnd = false;
                for(int i = 0; i + 11 < length; i += 12){
                    const QPointF start = QPointF(lineBuffer[i], lineBuffer[i + 2]) + worldOffset;
                    const QPointF end = QPointF(lineBuffer[i + 6], lineBuffer[i + 8]) + worldOffset;
                    if(!qIsFinite(start.x()) || !qIsFinite(start.y()) ||
                       !qIsFinite(end.x()) || !qIsFinite(end.y()))
                        continue;
                    // A vector node is continuous by definition. Join section boundaries;
                    // this also gives a visible fallback when a referenced TSECTION entry
                    // is missing, instead of silently leaving a hole in the route map.
                    if(hasPreviousEnd && QLineF(previousEnd, start).length() > 0.02){
                        const QLineF join(previousEnd, start);
                        nodeLines.push_back(join);
                        trackSegments.push_back(join);
                    }
                    const QLineF line(start, end);
                    nodeLines.push_back(line);
                    trackSegments.push_back(line);
                    previousEnd = end;
                    hasPreviousEnd = true;
                }
                if(!nodeLines.isEmpty())
                    vectorSegments.insert(entry.first, nodeLines);
                delete[] lineBuffer;
            }
        } else if(node->typ == 2){
            JunctionInfo junction;
            junction.nodeId = entry.first;
            junction.position = QPointF(node->UiD[4] * 2048.0 + node->UiD[6],
                                        -node->UiD[5] * 2048.0 - node->UiD[8]);
            junction.shapeId = node->args[1];
            for(int pin = 0; pin < 3; pin++)
                junction.pins[pin] = node->TrPinS[pin];
            if(tdb->tsection != NULL){
                const auto shapeIt = tdb->tsection->shape.find(junction.shapeId);
                if(shapeIt != tdb->tsection->shape.end() && shapeIt->second != NULL &&
                   shapeIt->second->mainroute >= 0 && shapeIt->second->mainroute < 2){
                    junction.mainRoute = shapeIt->second->mainroute;
                    junction.hasShapeMainRoute = true;
                }
            }
            junctions.push_back(junction);
        } else if(node->typ == 0){
            endpoints.push_back(QPointF(node->UiD[4] * 2048.0 + node->UiD[6],
                                        -node->UiD[5] * 2048.0 - node->UiD[8]));
        }
    }

    // Resolve each switch leg against the already sampled connected vector nodes.
    for(JunctionInfo &junction : junctions){
        for(int pin = 0; pin < 3; pin++){
            junction.directions[pin] = vectorDirectionFromJunction(junction.pins[pin], junction.position);
            const auto linesIt = vectorSegments.constFind(junction.pins[pin]);
            if(linesIt != vectorSegments.constEnd() && !linesIt->isEmpty()){
                const QPointF first = linesIt->first().p1();
                const QPointF last = linesIt->last().p2();
                const QPointF nearest = QLineF(junction.position, first).length() < QLineF(junction.position, last).length()
                                      ? first : last;
                if(QLineF(junction.position, nearest).length() > 0.02){
                    const QLineF connector(junction.position, nearest);
                    trackSegments.push_back(connector);
                    trackSegmentsMedium.push_back(connector);
                    trackSegmentsOverview.push_back(connector);
                }
            }
        }
    }

    // Pull read-only operational objects from the same TrackDB data used by
    // ORTS Map. Fold paired platform/siding endpoints into one useful symbol.
    QSet<int> pairedPlatformItems;
    QSet<QString> stationNames;
    QSet<QString> sidingNames;
    auto trackItemPoint = [](const TRitem *item) {
        if(item == NULL || item->trItemRData == NULL)
            return QPointF();
        return QPointF(item->trItemRData[3] * 2048.0 + item->trItemRData[0],
                       -item->trItemRData[4] * 2048.0 - item->trItemRData[2]);
    };
    for(const auto &entry : tdb->trackItems){
        TRitem *item = entry.second;
        if(item == NULL || item->trItemRData == NULL)
            continue;
        const QPointF point = trackItemPoint(item);
        if(item->type == "signalitem"){
            MapInteractive signal;
            signal.type = MapSignal;
            signal.position = point;
            signal.sourceId = entry.first;
            signal.label = item->trSignalType4.isEmpty()
                ? tr("Signal %1").arg(entry.first) : item->trSignalType4;
            mapInteractives.push_back(signal);
        } else if(item->type == "platformitem"){
            QPointF otherPoint = point;
            int otherId = -1;
            if(item->platformTrItemData != NULL){
                otherId = static_cast<int>(item->platformTrItemData[1]);
                const auto otherIt = tdb->trackItems.find(otherId);
                if(otherIt != tdb->trackItems.end() && otherIt->second != NULL &&
                   otherIt->second->trItemRData != NULL)
                    otherPoint = trackItemPoint(otherIt->second);
                if(!pairedPlatformItems.contains(entry.first) &&
                   !pairedPlatformItems.contains(otherId) &&
                   QLineF(point, otherPoint).length() > 0.1){
                    platformLines.push_back(QLineF(point, otherPoint));
                    pairedPlatformItems.insert(entry.first);
                    pairedPlatformItems.insert(otherId);
                }
            }
            const QString label = !item->stationName.trimmed().isEmpty()
                ? item->stationName.trimmed() : item->platformName.trimmed();
            const QString key = label.toLower();
            if(!label.isEmpty() && !stationNames.contains(key)){
                stationNames.insert(key);
                MapInteractive station;
                station.type = MapStation;
                station.position = (point + otherPoint) * 0.5;
                station.sourceId = entry.first;
                station.label = label;
                mapInteractives.push_back(station);
            }
        } else if(item->type == "sidingitem"){
            QPointF otherPoint = point;
            if(item->platformTrItemData != NULL){
                const int otherId = static_cast<int>(item->platformTrItemData[1]);
                const auto otherIt = tdb->trackItems.find(otherId);
                if(otherIt != tdb->trackItems.end() && otherIt->second != NULL &&
                   otherIt->second->trItemRData != NULL)
                    otherPoint = trackItemPoint(otherIt->second);
            }
            const QString label = item->platformName.trimmed();
            const QString key = label.toLower();
            if(!label.isEmpty() && !sidingNames.contains(key)){
                sidingNames.insert(key);
                MapInteractive siding;
                siding.type = MapSiding;
                siding.position = (point + otherPoint) * 0.5;
                siding.sourceId = entry.first;
                siding.label = label;
                mapInteractives.push_back(siding);
            }
        } else if(item->type == "pickupitem"){
            MapInteractive pickup;
            pickup.type = MapPickup;
            pickup.position = point;
            pickup.sourceId = entry.first;
            pickup.label = tr("Service point");
            mapInteractives.push_back(pickup);
        }
    }

    // Route-authored marker files are already parsed by Route. Display only
    // the .mkr whose filename matches the route name.
    if(route != NULL){
        const QMap<QString, Coords*> markerFiles = route->getMkrList();
        const QString routeMarkerName = (Game::routeName + ".mkr").toLower();
        const auto markerFile = markerFiles.constFind(routeMarkerName);
        if(markerFile != markerFiles.constEnd() &&
           markerFile.value() != NULL && markerFile.value()->loaded){
            for(const Coords::Marker &marker : markerFile.value()->markerList){
                if(marker.type != 0 ||
                   marker.tileX.isEmpty() || marker.tileZ.isEmpty() ||
                   marker.x.isEmpty() || marker.z.isEmpty())
                    continue;
                MapInteractive mapMarker;
                mapMarker.type = MapMarker;
                mapMarker.position = QPointF(marker.tileX[0] * 2048.0 + marker.x[0],
                                             -marker.tileZ[0] * 2048.0 + marker.z[0]);
                mapMarker.label = marker.name;
                mapInteractives.push_back(mapMarker);
            }
        }
    }

    // Precompute route-wide levels of detail once. The one-metre source stays
    // available for switch inspection, while overview redraws submit only the
    // detail that can produce a visible pixel at that scale.
    for(auto it = vectorSegments.constBegin(); it != vectorSegments.constEnd(); ++it){
        trackSegmentsMedium += simplifiedLines(it.value(), 8.0);
        trackSegmentsOverview += simplifiedLines(it.value(), 48.0);
    }

    if(!trackSegments.isEmpty()){
        routeBounds = QRectF(trackSegments.first().p1(), QSizeF(1, 1));
        for(const QLineF &line : trackSegments)
            routeBounds |= QRectF(line.p1(), line.p2()).normalized();
    }
    segmentCount = trackSegments.size();
    updateStatus();
    update();
}

void ActivityTrackViewer::rebuildPathCache(){
    selectedPathLine = QPainterPath();
    pathPoints.clear();
    selectedPathBounds = QRectF();
    if(selectedPath == NULL)
        return;

    const QVector<QLineF> &lines = selectedPath->getMapLines();
    for(const QLineF &line : lines){
        selectedPathLine.moveTo(line.p1());
        selectedPathLine.lineTo(line.p2());
    }
    for(const Path::PathNode &node : selectedPath->node)
        pathPoints.push_back(QPointF(node.tilex * 2048.0 + node.pos[0],
                                     node.tilez * 2048.0 + node.pos[2]));

    selectedPathBounds = selectedPathLine.boundingRect();
    if(selectedPathBounds.isEmpty() && !pathPoints.isEmpty()){
        selectedPathBounds = QRectF(pathPoints.first(), QSizeF(1, 1));
        for(const QPointF &point : pathPoints)
            selectedPathBounds |= QRectF(point, QSizeF(1, 1));
    }
}

QTransform ActivityTrackViewer::worldTransform() const {
    QTransform transform;
    transform.translate(width() * 0.5, height() * 0.5);
    transform.rotate(viewQuarterTurns * 90.0);
    // Stored map Y is -route-Z. Keeping screen Y positive here places route
    // +Z (north) toward the top of the viewer.
    transform.scale(pixelsPerMeter, pixelsPerMeter);
    transform.translate(-viewCenter.x(), -viewCenter.y());
    return transform;
}

QPointF ActivityTrackViewer::screenToWorld(const QPointF &screenPoint) const {
    bool invertible = false;
    const QTransform inverse = worldTransform().inverted(&invertible);
    return invertible ? inverse.map(screenPoint) : viewCenter;
}

void ActivityTrackViewer::fitBounds(const QRectF &bounds){
    if(bounds.isEmpty() || width() < 2 || height() < 2)
        return;
    const qreal margin = 40.0;
    const qreal viewWidth = (viewQuarterTurns % 2) ? bounds.height() : bounds.width();
    const qreal viewHeight = (viewQuarterTurns % 2) ? bounds.width() : bounds.height();
    const qreal sx = (width() - margin * 2.0) / qMax<qreal>(viewWidth, 1.0);
    const qreal sy = (height() - margin * 2.0) / qMax<qreal>(viewHeight, 1.0);
    pixelsPerMeter = qBound<qreal>(0.00005, qMin(sx, sy), 100.0);
    viewCenter = bounds.center();
    updateStatus();
    update();
}

void ActivityTrackViewer::fitRoute(){
    if(routeBounds.isEmpty())
        return;
    hasFittedRoute = true;
    fitBounds(routeBounds);
}

void ActivityTrackViewer::fitSelectedPath(){
    if(selectedPathBounds.isEmpty())
        fitRoute();
    else
        fitBounds(selectedPathBounds);
}

void ActivityTrackViewer::setShowJunctions(bool show){
    showJunctions = show;
    update();
}

void ActivityTrackViewer::setShowTileGrid(bool show){
    showTileGrid = show;
    update();
}

void ActivityTrackViewer::setShowInteractives(bool show){
    showInteractives = show;
    update();
}

void ActivityTrackViewer::setShowMarkers(bool show){
    showMarkers = show;
    update();
}

void ActivityTrackViewer::setShowMapLabels(bool show){
    showMapLabels = show;
    update();
}

void ActivityTrackViewer::rotateView90(){
    viewQuarterTurns = (viewQuarterTurns + 1) % 4;
    static const char *upDirections[] = {"North", "West", "South", "East"};
    emit statusChanged(tr("View rotated 90 degrees - %1 is now up. Geographic north remains marked by N.")
                       .arg(tr(upDirections[viewQuarterTurns])));
    update();
}

void ActivityTrackViewer::showTransientMessage(const QString &text, bool error){
    transientMessage = text;
    transientMessageIsError = error;
    update();
    QTimer::singleShot(3000, this, SLOT(clearTransientMessage()));
}

void ActivityTrackViewer::clearTransientMessage(){
    transientMessage.clear();
    transientMessageIsError = false;
    update();
}

void ActivityTrackViewer::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 21, 24));

    if(trackSegments.isEmpty()){
        painter.setPen(QColor(175, 180, 185));
        painter.drawText(rect(), Qt::AlignCenter,
                         route == NULL ? tr("Load a route to view its TrackDB")
                                       : tr("This route has no readable TrackDB geometry"));
        return;
    }

    const QTransform transform = worldTransform();
    painter.setWorldTransform(transform);

    if(showTileGrid){
        const QRectF visibleWorld = transform.inverted().mapRect(rect());
        const int firstX = qFloor(visibleWorld.left() / 2048.0);
        const int lastX = qCeil(visibleWorld.right() / 2048.0);
        const int firstY = qFloor(visibleWorld.top() / 2048.0);
        const int lastY = qCeil(visibleWorld.bottom() / 2048.0);
        QPen gridPen(QColor(42, 47, 52));
        gridPen.setCosmetic(true);
        painter.setPen(gridPen);
        for(int x = firstX; x <= lastX && x - firstX < 250; x++)
            painter.drawLine(QPointF(x * 2048.0, visibleWorld.top()),
                             QPointF(x * 2048.0, visibleWorld.bottom()));
        for(int y = firstY; y <= lastY && y - firstY < 250; y++)
            painter.drawLine(QPointF(visibleWorld.left(), y * 2048.0),
                             QPointF(visibleWorld.right(), y * 2048.0));
    }

    // ORTS draws platform extents beneath the rail so the track remains
    // legible. Fade them at overview scale instead of letting them dominate.
    if(showInteractives && !platformLines.isEmpty() && pixelsPerMeter >= 0.04){
        painter.resetTransform();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const int alpha = pixelsPerMeter < 0.2 ? 90 : (pixelsPerMeter < 0.8 ? 145 : 210);
        QPen platformPen(QColor(70, 155, 225, alpha));
        platformPen.setWidthF(pixelsPerMeter < 0.8 ? 3.0 : 5.0);
        platformPen.setCapStyle(Qt::RoundCap);
        painter.setPen(platformPen);
        QVector<QLineF> screenPlatforms;
        screenPlatforms.reserve(platformLines.size());
        for(const QLineF &line : platformLines)
            screenPlatforms.push_back(QLineF(transform.map(line.p1()), transform.map(line.p2())));
        painter.drawLines(screenPlatforms);
    }

    // Draw TrackDB geometry in screen coordinates. QPainterPath loses useful
    // precision when very large route coordinates are deeply magnified.
    painter.resetTransform();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen trackPen(QColor(132, 139, 145));
    trackPen.setWidthF(1.15);
    trackPen.setCosmetic(true);
    painter.setPen(trackPen);
    painter.setBrush(Qt::NoBrush);
    const QRectF visibleScreen = rect().adjusted(-3, -3, 3, 3);
    const QVector<QLineF> &visibleTrackSegments = pixelsPerMeter >= 0.35
        ? trackSegments
        : (pixelsPerMeter >= 0.06 ? trackSegmentsMedium : trackSegmentsOverview);
    QVector<QLineF> screenLines;
    screenLines.reserve(visibleTrackSegments.size());
    for(const QLineF &line : visibleTrackSegments){
        const QLineF screenLine(transform.map(line.p1()), transform.map(line.p2()));
        if(QRectF(screenLine.p1(), screenLine.p2()).normalized().adjusted(-2, -2, 2, 2).intersects(visibleScreen))
            screenLines.push_back(screenLine);
    }
    if(!screenLines.isEmpty())
        painter.drawLines(screenLines);

    // Below one screen pixel per metre the switch controls merge into blobs
    // and no longer communicate useful topology. Hide them until the user
    // zooms back to an inspection scale.
    if(showJunctions && pixelsPerMeter >= (1.0 / 1.2)){
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal radius = qBound<qreal>(3.0, 2.2 + qLn(1.0 + pixelsPerMeter) * 1.5, 8.0);
        const QHash<int, int> *currentSwitchRoutes = draftLegSwitchRoutes.isEmpty()
            ? NULL : &draftLegSwitchRoutes.last();
        for(int i = 0; i < junctions.size(); i++){
            const JunctionInfo &junction = junctions[i];
            const QPointF center = transform.map(junction.position);
            if(!visibleScreen.contains(center))
                continue;

            // At inspection zoom, show the trunk, both exits, and highlight
            // the route defined as MainRoute in TSECTION.DAT.
            if(pixelsPerMeter >= 0.5){
                const int activePin = currentSwitchRoutes != NULL && currentSwitchRoutes->contains(junction.nodeId)
                    ? currentSwitchRoutes->value(junction.nodeId)
                    : qBound(1, junction.mainRoute + 1, 2);
                for(int pin = 0; pin < 3; pin++){
                    const QPointF worldDirection = junction.directions[pin];
                    QPointF direction = transform.map(junction.position + worldDirection) - center;
                    const qreal directionLength = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
                    if(directionLength < 0.0001)
                        continue;
                    direction /= directionLength;
                    const qreal length = pin == 0 ? 18.0 : 22.0;
                    QPen legPen(pin == 0 || pin == activePin ? QColor(255, 202, 72)
                                                            : QColor(118, 91, 53));
                    legPen.setWidthF(pin == 0 || pin == activePin ? 3.2 : 2.0);
                    legPen.setCapStyle(Qt::RoundCap);
                    painter.setPen(legPen);
                    painter.drawLine(center, center + direction * length);
                }
            }

            QPen junctionPen(i == selectedJunction ? QColor(255, 255, 255) : QColor(255, 174, 56));
            junctionPen.setWidthF(i == selectedJunction ? 2.0 : 1.0);
            painter.setPen(junctionPen);
            painter.setBrush(i == selectedJunction ? QColor(255, 202, 72) : QColor(255, 174, 56));
            painter.drawEllipse(center, radius, radius);
        }

        painter.setPen(QColor(100, 168, 255));
        painter.setBrush(QColor(100, 168, 255));
        const qreal endpointRadius = radius * 0.75;
        for(const QPointF &point : endpoints)
            painter.drawEllipse(transform.map(point), endpointRadius, endpointRadius);
    }

    if(creatingPath && !draftRouteLines.isEmpty()){
        const bool overviewPathStyle = pixelsPerMeter < 1.0;
        QPen routeHalo(QColor(30, 0, 28, 190));
        routeHalo.setWidthF(overviewPathStyle ? 4.6 : 6.0);
        routeHalo.setCapStyle(Qt::RoundCap);
        routeHalo.setJoinStyle(Qt::RoundJoin);
        painter.setPen(routeHalo);
        QVector<QLineF> screenRoute;
        const QVector<QLineF> &visibleDraftRoute = pixelsPerMeter >= 0.35
            ? draftRouteLines
            : (pixelsPerMeter >= 0.06 ? draftRouteMedium : draftRouteOverview);
        screenRoute.reserve(visibleDraftRoute.size());
        for(const QLineF &line : visibleDraftRoute)
            screenRoute.push_back(QLineF(transform.map(line.p1()), transform.map(line.p2())));
        painter.drawLines(screenRoute);
        QPen routePen(QColor(255, 0, 210));
        routePen.setWidthF(overviewPathStyle ? 2.35 : 3.2);
        routePen.setCapStyle(Qt::RoundCap);
        routePen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(routePen);
        painter.drawLines(screenRoute);

        const QVector<QLineF> &visibleOverlap = pixelsPerMeter >= 0.35
            ? draftOverlapLines
            : (pixelsPerMeter >= 0.06 ? draftOverlapMedium : draftOverlapOverview);
        if(!visibleOverlap.isEmpty()){
            QVector<QLineF> screenOverlap;
            screenOverlap.reserve(visibleOverlap.size());
            for(const QLineF &line : visibleOverlap)
                screenOverlap.push_back(QLineF(transform.map(line.p1()), transform.map(line.p2())));
            QPen overlapPen(QColor(0, 235, 255));
            overlapPen.setWidthF(overviewPathStyle ? 3.1 : 4.5);
            overlapPen.setCapStyle(Qt::RoundCap);
            painter.setPen(overlapPen);
            painter.drawLines(screenOverlap);
        }
    }

    if(creatingPath && !draftPassingSidings.isEmpty()){
        const bool overviewPathStyle = pixelsPerMeter < 1.0;
        const qreal pointScale = overviewPathStyle
            ? 1.0 + qMin<qreal>(0.28, (1.0 - pixelsPerMeter) * 0.28) : 1.0;
        QVector<QLineF> screenSidings;
        for(const DraftPassingSiding &siding : draftPassingSidings){
            for(const QLineF &line : siding.lines)
                screenSidings.push_back(QLineF(transform.map(line.p1()), transform.map(line.p2())));
        }
        QPen sidingHalo(QColor(30, 16, 0, 220));
        sidingHalo.setWidthF(overviewPathStyle ? 4.6 : 6.0);
        sidingHalo.setCapStyle(Qt::RoundCap);
        painter.setPen(sidingHalo);
        painter.drawLines(screenSidings);
        QPen sidingPen(QColor(255, 145, 25));
        sidingPen.setWidthF(overviewPathStyle ? 2.55 : 3.5);
        sidingPen.setCapStyle(Qt::RoundCap);
        sidingPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(sidingPen);
        painter.drawLines(screenSidings);
        if(!overviewPathStyle){
            painter.setBrush(QColor(255, 145, 25));
            painter.setPen(QPen(QColor(255, 235, 205), 1.5));
            for(const DraftPassingSiding &siding : draftPassingSidings){
                const qreal halfSize = 5.0 * pointScale;
                painter.drawRect(QRectF(transform.map(siding.start) - QPointF(halfSize, halfSize),
                                        QSizeF(halfSize * 2.0, halfSize * 2.0)));
                painter.drawRect(QRectF(transform.map(siding.end) - QPointF(halfSize, halfSize),
                                        QSizeF(halfSize * 2.0, halfSize * 2.0)));
            }
        }
    }

    if(creatingPath && !draftReversePoints.isEmpty()){
        QPen reversePen(QColor(230, 255, 255));
        reversePen.setWidthF(2.0);
        painter.setPen(reversePen);
        painter.setBrush(QColor(0, 190, 220));
        for(const DraftReversePoint &reverse : draftReversePoints){
            const QPointF center = transform.map(reverse.position);
            const qreal pointScale = pixelsPerMeter >= 1.0
                ? 1.0 : 1.0 + qMin<qreal>(0.28, (1.0 - pixelsPerMeter) * 0.28);
            const qreal radius = 9.0 * pointScale;
            QPolygonF diamond;
            diamond << center + QPointF(0, -radius) << center + QPointF(radius, 0)
                    << center + QPointF(0, radius) << center + QPointF(-radius, 0);
            painter.drawPolygon(diamond);
        }
    }

    if(creatingPath && pixelsPerMeter >= 1.0 && !draftWaitPoints.isEmpty()){
        const qreal pointScale = pixelsPerMeter >= 1.0
            ? 1.0 : 1.0 + qMin<qreal>(0.22, (1.0 - pixelsPerMeter) * 0.22);
        QFont waitFont = painter.font();
        waitFont.setBold(true);
        painter.setFont(waitFont);
        for(const DraftWaitPoint &wait : draftWaitPoints){
            const QPointF center = transform.map(wait.position);
            const qreal radius = 8.5 * pointScale;
            painter.setPen(QPen(QColor(255, 244, 205), 2.0));
            painter.setBrush(QColor(245, 185, 65));
            painter.drawEllipse(center, radius, radius);
            painter.setPen(QColor(35, 28, 10));
            painter.drawText(QRectF(center - QPointF(radius, radius),
                                    QSizeF(radius * 2, radius * 2)),
                             Qt::AlignCenter,
                             waitPointMarker(wait.waitSeconds));
        }
    }

    if(creatingPath && draftEndSelected){
        const QPointF end = transform.map(draftEnd);
        const qreal pointScale = pixelsPerMeter >= 1.0
            ? 1.0 : 1.0 + qMin<qreal>(0.28, (1.0 - pixelsPerMeter) * 0.28);
        QPen endPen(QColor(255, 225, 250));
        endPen.setWidthF(2.0);
        painter.setPen(endPen);
        painter.setBrush(QColor(235, 50, 155));
        painter.drawEllipse(end, 7.0 * pointScale, 7.0 * pointScale);
    }

    // Path controls are deliberately painted after every magenta/cyan/orange
    // route line so their click locations remain unmistakable.
    if(creatingPath && draftStartSelected){
        const QPointF start = transform.map(draftStart);
        const qreal pointScale = pixelsPerMeter >= 1.0
            ? 1.0 : 1.0 + qMin<qreal>(0.28, (1.0 - pixelsPerMeter) * 0.28);
        QPen startPen(QColor(225, 255, 230));
        startPen.setWidthF(2.0);
        painter.setPen(startPen);
        painter.setBrush(QColor(65, 220, 105));
        painter.drawEllipse(start, 7.0 * pointScale, 7.0 * pointScale);
        painter.drawLine(start + QPointF(-11 * pointScale, 0), start + QPointF(11 * pointScale, 0));
        painter.drawLine(start + QPointF(0, -11 * pointScale), start + QPointF(0, 11 * pointScale));
    }

    if(!selectedPathLine.isEmpty()){
        painter.setWorldTransform(transform);
        QPen haloPen(QColor(0, 0, 0, 165));
        haloPen.setWidthF(5.0);
        haloPen.setCosmetic(true);
        painter.setPen(haloPen);
        painter.drawPath(selectedPathLine);

        QPen pathPen(QColor(255, 225, 64));
        pathPen.setWidthF(2.8);
        pathPen.setCosmetic(true);
        painter.setPen(pathPen);
        painter.drawPath(selectedPathLine);
    }

    if(!pathPoints.isEmpty()){
        painter.setWorldTransform(transform);
        const qreal markerRadius = qBound<qreal>(5.25 / pixelsPerMeter, 6.0, 140.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(65, 220, 105));
        painter.drawEllipse(pathPoints.first(), markerRadius, markerRadius);
        if(pathPoints.size() > 1){
            painter.setBrush(QColor(235, 75, 75));
            painter.drawEllipse(pathPoints.last(), markerRadius, markerRadius);
        }
    }

    if(creatingPath && selectedDraftType != SelectNone){
        painter.resetTransform();
        QPen selectionPen(QColor(255, 255, 255));
        selectionPen.setWidthF(2.0);
        selectionPen.setStyle(Qt::DashLine);
        painter.setPen(selectionPen);
        painter.setBrush(Qt::NoBrush);
        if(selectedDraftType == SelectStart)
            painter.drawEllipse(transform.map(draftStart), 13.0, 13.0);
        else if(selectedDraftType == SelectEnd)
            painter.drawEllipse(transform.map(draftEnd), 13.0, 13.0);
        else if(selectedDraftType == SelectReverse && selectedDraftIndex >= 0 &&
                selectedDraftIndex < draftReversePoints.size())
            painter.drawEllipse(transform.map(draftReversePoints[selectedDraftIndex].position), 14.0, 14.0);
        else if(selectedDraftType == SelectWait && pixelsPerMeter >= 1.0 &&
                selectedDraftIndex >= 0 &&
                selectedDraftIndex < draftWaitPoints.size())
            painter.drawEllipse(transform.map(draftWaitPoints[selectedDraftIndex].position), 14.0, 14.0);
        else if(selectedDraftType == SelectPassingSiding && selectedDraftIndex >= 0 &&
                selectedDraftIndex < draftPassingSidings.size()){
            QVector<QLineF> selectedLines;
            for(const QLineF &line : draftPassingSidings[selectedDraftIndex].lines)
                selectedLines.push_back(QLineF(transform.map(line.p1()), transform.map(line.p2())));
            painter.drawLines(selectedLines);
        }
    }

    // Operational symbols sit above track/path geometry. Labels remain
    // screen-upright and use a small collision list, following ORTS Map's
    // approach while reserving switch locations for our final control layer.
    painter.resetTransform();
    if((showInteractives || showMarkers) && !mapInteractives.isEmpty()){
        painter.setRenderHint(QPainter::Antialiasing, true);
        QVector<QRectF> occupiedLabels;
        QFont labelFont = painter.font();
        const qreal closeZoomGrowth = qBound<qreal>(
            0.0, qLn(1.0 + pixelsPerMeter) * 1.7, 4.0);
        labelFont.setPointSizeF(qMax<qreal>(
            labelFont.pointSizeF(), 8.0 + closeZoomGrowth));
        painter.setFont(labelFont);
        const QFontMetrics metrics(labelFont);

        auto minimumSymbolZoom = [](MapInteractiveType type) {
            if(type == MapStation)
                return 0.035;
            if(type == MapMarker)
                return 0.0;
            return 0.15;
        };
        auto minimumLabelZoom = [](MapInteractiveType type) {
            if(type == MapStation)
                return 0.08;
            if(type == MapMarker)
                return 0.0;
            return 0.35;
        };
        auto symbolColor = [](MapInteractiveType type) {
            switch(type){
                case MapSignal: return QColor(90, 225, 125);
                case MapStation: return QColor(85, 175, 245);
                case MapSiding: return QColor(115, 165, 235);
                case MapPickup: return QColor(245, 185, 65);
                case MapMarker: return QColor(205, 120, 240);
            }
            return QColor(220, 220, 220);
        };

        const MapInteractiveType priorities[] = {
            MapStation, MapSignal, MapPickup, MapSiding, MapMarker
        };
        for(MapInteractiveType priority : priorities){
            for(const MapInteractive &item : mapInteractives){
                if((item.type == MapMarker && !showMarkers) ||
                   (item.type != MapMarker && !showInteractives))
                    continue;
                if(item.type != priority || pixelsPerMeter < minimumSymbolZoom(item.type))
                    continue;
                QPointF center = transform.map(item.position);
                if(!visibleScreen.adjusted(-80, -40, 80, 40).contains(center))
                    continue;
                qreal markerScale = 1.0;
                if(item.type == MapMarker){
                    const qreal overviewRatio = 0.08 /
                        qMax<qreal>(pixelsPerMeter, 0.001);
                    markerScale = qBound<qreal>(
                        1.0, 1.0 + qLn(qMax<qreal>(1.0, overviewRatio)) * 0.12, 1.45);
                }
                const QColor color = symbolColor(item.type);
                QPen outline(QColor(8, 10, 12, 235));
                outline.setWidthF(3.5);
                painter.setPen(outline);
                painter.setBrush(color);

                if(item.type == MapSignal){
                    // Keep the head clear of the route/path line. A compact
                    // green-over-red pair reads as a signal at a glance.
                    center += QPointF(16, 0);
                    painter.setPen(QPen(QColor(8, 10, 12, 235), 2.5));
                    painter.setBrush(QColor(25, 29, 32));
                    painter.drawRoundedRect(
                        QRectF(center - QPointF(6.5, 11.5), QSizeF(13, 23)), 4, 4);
                    painter.setPen(QPen(QColor(215, 255, 225), 1.1));
                    painter.setBrush(QColor(90, 225, 125));
                    painter.drawEllipse(center + QPointF(0, -5.5), 4.2, 4.2);
                    painter.setPen(QPen(QColor(255, 220, 220), 1.1));
                    painter.setBrush(QColor(225, 75, 75));
                    painter.drawEllipse(center + QPointF(0, 5.5), 4.2, 4.2);
                } else if(item.type == MapStation){
                    painter.drawRoundedRect(QRectF(center - QPointF(7, 4), QSizeF(14, 8)), 2, 2);
                    painter.setPen(QPen(QColor(225, 245, 255), 1.2));
                    painter.drawLine(center + QPointF(-4, 0), center + QPointF(4, 0));
                } else if(item.type == MapSiding){
                    QPolygonF diamond;
                    diamond << center + QPointF(0, -5) << center + QPointF(5, 0)
                            << center + QPointF(0, 5) << center + QPointF(-5, 0);
                    painter.drawPolygon(diamond);
                } else if(item.type == MapPickup){
                    painter.drawRoundedRect(QRectF(center - QPointF(5, 6), QSizeF(10, 12)), 2, 2);
                    painter.setPen(QPen(QColor(35, 28, 10), 1.4));
                    painter.drawText(QRectF(center - QPointF(5, 7), QSizeF(10, 14)),
                                     Qt::AlignCenter, tr("P"));
                } else {
                    // Type-0 route landmarks remain visible at every overview
                    // scale. The coordinate is the anchor dot; the compact
                    // flag rises above it instead of obscuring the route.
                    painter.setPen(QPen(QColor(8, 10, 12, 235), 3.0));
                    painter.setBrush(color);
                    painter.drawEllipse(center, 2.5, 2.5);
                    const QPointF mastTop = center + QPointF(0, -13) * markerScale;
                    painter.drawLine(center, mastTop);
                    QPolygonF flag;
                    flag << mastTop
                         << center + QPointF(12, -9) * markerScale
                         << center + QPointF(0, -4) * markerScale;
                    painter.setPen(QPen(QColor(238, 205, 250), 1.4));
                    painter.setBrush(color);
                    painter.drawPolygon(flag);
                }

                if(!showMapLabels || item.label.trimmed().isEmpty() ||
                   pixelsPerMeter < minimumLabelZoom(item.type))
                    continue;
                QRectF labelRect = metrics.boundingRect(item.label);
                labelRect.adjust(-4, -2, 4, 2);
                if(item.type == MapMarker)
                    labelRect.moveTopLeft(center + QPointF(
                        15 * markerScale, -13 * markerScale - labelRect.height() * 0.5));
                else
                    labelRect.moveTopLeft(center + QPointF(10, -labelRect.height() * 0.5));
                bool blocked = false;
                for(const QRectF &used : occupiedLabels){
                    if(used.adjusted(-3, -2, 3, 2).intersects(labelRect)){
                        blocked = true;
                        break;
                    }
                }
                if(!blocked && item.type != MapMarker &&
                   showJunctions && pixelsPerMeter >= (1.0 / 1.2)){
                    for(const JunctionInfo &junction : junctions){
                        if(labelRect.adjusted(-10, -6, 10, 6).contains(transform.map(junction.position))){
                            blocked = true;
                            break;
                        }
                    }
                }
                if(blocked)
                    continue;
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(12, 15, 18, 205));
                painter.drawRoundedRect(labelRect, 3, 3);
                painter.setPen(QColor(225, 232, 238));
                painter.drawText(labelRect.adjusted(4, 1, -4, -1),
                                 Qt::AlignLeft | Qt::AlignVCenter, item.label);
                occupiedLabels.push_back(labelRect);
            }
        }
    }

    // Interactive switch controls are always the topmost map layer. Path and
    // overlap colors must never hide the object the user is trying to click.
    painter.resetTransform();
    if(showJunctions && pixelsPerMeter >= (1.0 / 1.2)){
        const QHash<int, int> *currentSwitchRoutes = draftLegSwitchRoutes.isEmpty()
            ? NULL : &draftLegSwitchRoutes.last();
        // Keep overview maps readable while retaining a generous invisible
        // click target. The control grows only when zoomed in for inspection.
        const qreal radius = qBound<qreal>(3.5,
            3.2 + qLn(1.0 + pixelsPerMeter) * 2.2, 8.0);
        for(int i = 0; i < junctions.size(); i++){
            const JunctionInfo &junction = junctions[i];
            const QPointF center = transform.map(junction.position);
            if(!visibleScreen.adjusted(-20, -20, 20, 20).contains(center))
                continue;
            const int defaultPin = qBound(1, junction.mainRoute + 1, 2);
            const int activePin = currentSwitchRoutes != NULL && currentSwitchRoutes->contains(junction.nodeId)
                ? currentSwitchRoutes->value(junction.nodeId) : defaultPin;
            const bool changedFromDefault = activePin != defaultPin;
            const bool hovered = i == hoveredJunction;
            const bool selected = i == selectedJunction;

            QPen halo(QColor(8, 10, 12, 230));
            halo.setWidthF(5.0);
            painter.setPen(halo);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, radius + 1.5, radius + 1.5);

            QPen markerPen(changedFromDefault ? QColor(225, 255, 255)
                                              : (hovered || selected ? QColor(255, 255, 255)
                                                                     : QColor(255, 211, 120)));
            markerPen.setWidthF(changedFromDefault || hovered || selected ? 2.5 : 1.4);
            painter.setPen(markerPen);
            painter.setBrush(changedFromDefault ? QColor(0, 205, 225) : QColor(255, 174, 56));
            painter.drawEllipse(center, radius, radius);

            if(changedFromDefault){
                QPen changedRing(QColor(0, 235, 255));
                changedRing.setWidthF(2.2);
                painter.setPen(changedRing);
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(center, radius + 2.5, radius + 2.5);
            }
            if(hovered){
                QPen hoverRing(QColor(255, 255, 255, 230));
                hoverRing.setWidthF(2.0);
                painter.setPen(hoverRing);
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(center, radius + 5.0, radius + 5.0);
            }
        }
    }

    painter.resetTransform();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QColor(205, 210, 215));
    painter.setBrush(QColor(12, 14, 16, 185));
    const QString scaleText = pixelsPerMeter > 0.0
        ? tr("1 px = %1 m").arg(1.0 / pixelsPerMeter, 0, 'f', pixelsPerMeter > 0.2 ? 1 : 0)
        : QString();
    const QRect scaleBox(10, height() - 34, 145, 24);
    painter.drawRoundedRect(scaleBox, 3, 3);
    painter.drawText(scaleBox.adjusted(8, 0, -5, 0), Qt::AlignVCenter, scaleText);

    if(creatingPath || !transientMessage.isEmpty()){
        QString instruction;
        if(!transientMessage.isEmpty())
            instruction = transientMessage;
        else if(pathPlacementMode == PlaceReverse)
            instruction = tr("ADD REVERSE - click track where direction changes; drag to pan");
        else if(pathPlacementMode == PlacePassingSiding)
            instruction = tr("ADD PASSING SIDING - click the alternate track between two main-path switches");
        else if(pathPlacementMode == PlaceWait)
            instruction = tr("ADD WAIT POINT - click the magenta path");
        else if(pathPlacementMode == PlaceEnd)
            instruction = tr("PLACE ENDPOINT - click anywhere on the magenta flow");
        else if(pathPlacementMode == PlaceStart || !draftStartSelected)
            instruction = tr("SET START - click a track line; drag to pan");
        else if(!draftEndSelected)
            instruction = tr("FLOWING PATH - click any orange switch, or choose Place Endpoint");
        else
            instruction = tr("ENDPOINT SET - orange switches remain live and will resume flow");
        QFont instructionFont = painter.font();
        instructionFont.setBold(true);
        painter.setFont(instructionFont);
        const int instructionWidth = qMin(620, qMax(260, width() - 340));
        const QRect instructionBox((width() - instructionWidth) / 2,
                                   height() - 48, instructionWidth, 30);
        painter.setPen(QColor(225, 235, 225));
        painter.setBrush(transientMessageIsError ? QColor(145, 25, 30, 235)
                                                 : QColor(20, 75, 38, 225));
        painter.drawRoundedRect(instructionBox, 4, 4);
        painter.drawText(instructionBox.adjusted(10, 0, -8, 0), Qt::AlignVCenter, instruction);
    }

    // The compass rotates with the camera and always points to geographic north.
    QPointF northDirection = transform.map(viewCenter + QPointF(0.0, -1.0)) -
                             transform.map(viewCenter);
    const qreal northLength = qSqrt(northDirection.x() * northDirection.x() +
                                    northDirection.y() * northDirection.y());
    if(northLength > 0.0001)
        northDirection /= northLength;
    const QPointF compassCenter(width() - 42.0, 48.0);
    const QPointF northTip = compassCenter + northDirection * 18.0;
    const QPointF northBase = compassCenter - northDirection * 13.0;
    const QPointF perpendicular(-northDirection.y(), northDirection.x());
    QPen northPen(QColor(225, 235, 240));
    northPen.setWidthF(2.0);
    painter.setPen(northPen);
    painter.setBrush(QColor(225, 235, 240));
    painter.drawLine(northBase, northTip);
    QPolygonF northArrow;
    northArrow << northTip
               << northTip - northDirection * 9.0 + perpendicular * 5.0
               << northTip - northDirection * 9.0 - perpendicular * 5.0;
    painter.drawPolygon(northArrow);
    const QPointF labelCenter = northTip + northDirection * 11.0;
    painter.drawText(QRectF(labelCenter - QPointF(10, 9), QSizeF(20, 18)), Qt::AlignCenter, tr("N"));
}

void ActivityTrackViewer::resizeEvent(QResizeEvent *event){
    QWidget::resizeEvent(event);
    if(!hasFittedRoute && !routeBounds.isEmpty())
        QTimer::singleShot(0, this, SLOT(fitRoute()));
}

void ActivityTrackViewer::mousePressEvent(QMouseEvent *event){
    setFocus(Qt::MouseFocusReason);
    if(event->button() == Qt::LeftButton && creatingPath){
        pendingCreationClick = true;
        creationPressPosition = event->pos();
        lastMousePosition = event->pos();
        event->accept();
        return;
    }
    if(event->button() == Qt::LeftButton && showJunctions){
        const int hit = junctionAt(event->pos());
        if(hit >= 0){
            selectedJunction = hit;
            emit junctionSelected(junctionDescription(hit));
            update();
            event->accept();
            return;
        }
    }
    if(event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton){
        dragging = true;
        lastMousePosition = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ActivityTrackViewer::mouseMoveEvent(QMouseEvent *event){
    if(pendingCreationClick &&
       QLineF(creationPressPosition, event->pos()).length() > 4.0){
        pendingCreationClick = false;
        dragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    if(dragging){
        if(hoveredJunction != -1){
            hoveredJunction = -1;
            update();
        }
        const QPointF worldBefore = screenToWorld(lastMousePosition);
        const QPointF worldAfter = screenToWorld(event->pos());
        viewCenter += worldBefore - worldAfter;
        lastMousePosition = event->pos();
        update();
    } else {
        const int hit = showJunctions ? junctionAt(event->pos()) : -1;
        if(hit != hoveredJunction){
            hoveredJunction = hit;
            update();
        }
        setCursor(hit >= 0 ? Qt::PointingHandCursor : Qt::CrossCursor);
    }
    if(!dragging){
        const QPointF world = screenToWorld(event->pos());
        updateStatus(&world);
    }
}

void ActivityTrackViewer::mouseReleaseEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton && pendingCreationClick){
        pendingCreationClick = false;
        placeDraftPoint(event->pos(), (event->modifiers() & Qt::ControlModifier) != 0);
        event->accept();
        return;
    }
    if(dragging && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)){
        dragging = false;
        setCursor(Qt::CrossCursor);
        const QPointF world = screenToWorld(event->pos());
        updateStatus(&world);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ActivityTrackViewer::mouseDoubleClickEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        fitRoute();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ActivityTrackViewer::wheelEvent(QWheelEvent *event){
    if(trackSegments.isEmpty())
        return;
    const QPointF mouse = event->position();
    const QPointF before = screenToWorld(mouse);
    const qreal factor = qPow(1.0015, event->angleDelta().y());
    pixelsPerMeter = qBound<qreal>(0.00005, pixelsPerMeter * factor, 100.0);
    const QPointF after = screenToWorld(mouse);
    viewCenter += before - after;
    updateStatus(&before);
    update();
    event->accept();
}

void ActivityTrackViewer::keyPressEvent(QKeyEvent *event){
    if(creatingPath && (event->key() == Qt::Key_Delete ||
                        event->key() == Qt::Key_Backspace)){
        deleteSelectedDraftObject();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ActivityTrackViewer::leaveEvent(QEvent *event){
    if(hoveredJunction != -1){
        hoveredJunction = -1;
        update();
    }
    updateStatus();
    QWidget::leaveEvent(event);
}

void ActivityTrackViewer::updateStatus(const QPointF *worldPoint){
    QString text;
    if(route == NULL)
        text = tr("No route loaded");
    else
        text = tr("%1 track segments  •  %2 junctions  •  wheel: zoom  •  drag: pan  •  double-click: fit")
                   .arg(segmentCount).arg(junctions.size());
    if(worldPoint != NULL){
        const int tileX = qFloor(worldPoint->x() / 2048.0);
        const int tileZ = qFloor(-worldPoint->y() / 2048.0);
        text += tr("  •  tile %1, %2  •  %3 m, %4 m")
                    .arg(tileX).arg(tileZ)
                    .arg(worldPoint->x(), 0, 'f', 0)
                    .arg(worldPoint->y(), 0, 'f', 0);
    }
    emit statusChanged(text);
}

int ActivityTrackViewer::junctionAt(const QPointF &screenPoint) const {
    // Switches are intentionally hidden and inactive at overview scale.
    if(pixelsPerMeter < (1.0 / 1.2))
        return -1;
    const QTransform transform = worldTransform();
    const qreal hitRadius = qBound<qreal>(16.0,
        14.0 + qLn(1.0 + pixelsPerMeter) * 2.5, 24.0);
    int nearest = -1;
    qreal nearestDistance = hitRadius;
    for(int i = 0; i < junctions.size(); i++){
        const QLineF distance(screenPoint, transform.map(junctions[i].position));
        if(distance.length() <= nearestDistance){
            nearestDistance = distance.length();
            nearest = i;
        }
    }
    return nearest;
}

QPointF ActivityTrackViewer::vectorDirectionFromJunction(int vectorNodeId, const QPointF &junction) const {
    const auto linesIt = vectorSegments.constFind(vectorNodeId);
    if(linesIt == vectorSegments.constEnd() || linesIt->isEmpty())
        return QPointF();

    const QVector<QLineF> &lines = linesIt.value();
    const bool fromStart = QLineF(junction, lines.first().p1()).length() <=
                           QLineF(junction, lines.last().p2()).length();
    QPointF direction;
    if(fromStart){
        for(int i = 0; i < lines.size(); i++){
            direction = lines[i].p2() - junction;
            if(QLineF(QPointF(), direction).length() >= 4.0)
                break;
        }
    } else {
        for(int i = lines.size() - 1; i >= 0; i--){
            direction = lines[i].p1() - junction;
            if(QLineF(QPointF(), direction).length() >= 4.0)
                break;
        }
    }
    const qreal length = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    return length > 0.0001 ? direction / length : QPointF();
}

QString ActivityTrackViewer::junctionDescription(int index) const {
    if(index < 0 || index >= junctions.size())
        return tr("No switch selected");
    const JunctionInfo &junction = junctions[index];
    const int activePin = junction.mainRoute + 1;
    const QString state = junction.hasShapeMainRoute
        ? tr("Main route %1 (outgoing pin %2)").arg(junction.mainRoute).arg(activePin)
        : tr("Route 0 fallback (shape has no MainRoute)");
    return tr("<b>Switch node %1</b><br>Default state: <b>%2</b><br>"
              "Shape: %3<br>Trunk: pin 0 → vector %4<br>"
              "Exit 0: pin 1 → vector %5<br>Exit 1: pin 2 → vector %6<br>"
              "<span style='color:#aeb5bb'>Read-only. A live ORTS SelectedRoute is not stored in the route TDB.</span>")
           .arg(junction.nodeId).arg(state).arg(junction.shapeId)
           .arg(junction.pins[0]).arg(junction.pins[1]).arg(junction.pins[2]);
}

bool ActivityTrackViewer::nearestTrackPoint(const QPointF &screenPoint, QPointF &worldPoint,
                                            int &vectorNodeId) const {
    const QTransform transform = worldTransform();
    qreal bestDistanceSquared = 14.0 * 14.0;
    bool found = false;
    for(auto nodeIt = vectorSegments.constBegin(); nodeIt != vectorSegments.constEnd(); ++nodeIt){
        const QVector<QLineF> &lines = nodeIt.value();
        for(const QLineF &worldLine : lines){
            const QPointF a = transform.map(worldLine.p1());
            const QPointF b = transform.map(worldLine.p2());
            const QPointF ab = b - a;
            const qreal lengthSquared = ab.x() * ab.x() + ab.y() * ab.y();
            if(lengthSquared < 0.000001)
                continue;
            const QPointF ap = screenPoint - a;
            const qreal t = qBound<qreal>(0.0,
                (ap.x() * ab.x() + ap.y() * ab.y()) / lengthSquared, 1.0);
            const QPointF closest = a + ab * t;
            const QPointF delta = screenPoint - closest;
            const qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
            if(distanceSquared <= bestDistanceSquared){
                bestDistanceSquared = distanceSquared;
                worldPoint = worldLine.p1() + (worldLine.p2() - worldLine.p1()) * t;
                vectorNodeId = nodeIt.key();
                found = true;
            }
        }
    }
    return found;
}

QPointF ActivityTrackViewer::junctionPosition(int nodeId) const {
    for(const JunctionInfo &junction : junctions){
        if(junction.nodeId == nodeId)
            return junction.position;
    }
    return QPointF();
}

void ActivityTrackViewer::placeDraftPoint(const QPointF &screenPoint, bool moveStart){
    Q_UNUSED(moveStart);
    if(junctionAt(screenPoint) >= 0){
        toggleDraftSwitch(screenPoint);
        return;
    }

    if(pathPlacementMode == PlacePassingSiding){
        addPassingSiding(screenPoint);
        return;
    }

    if(pathPlacementMode == PlaceAutomatic && selectDraftObject(screenPoint))
        return;

    if(pathPlacementMode == PlaceAutomatic){
        emit pathDraftStatus(tr("<b>No placement tool selected</b><br>Orange switches can always be thrown. "
                                "Use Set Start, Place Endpoint, or Add Reverse Point before clicking track."));
        return;
    }

    QPointF snappedPoint;
    int vectorNodeId = -1;
    if(!nearestTrackPoint(screenPoint, snappedPoint, vectorNodeId)){
        emit pathDraftStatus(tr("<b>No track selected</b><br>Zoom closer and click directly on a track line."));
        return;
    }

    auto pointOnCurrentFlow = [this](const QPointF &point) {
        const qreal tolerance = 12.0 / qMax<qreal>(pixelsPerMeter, 0.00005);
        qreal best = std::numeric_limits<qreal>::max();
        for(const QLineF &line : draftRouteLines){
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = point - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                        lengthSquared, 1.0);
            }
            best = qMin(best, QLineF(point, line.p1() + delta * t).length());
        }
        return best <= tolerance;
    };

    if(pathPlacementMode == PlaceWait){
        if(!pointOnCurrentFlow(snappedPoint)){
            emit pathDraftStatus(tr("<b>Path control must be on magenta</b><br>Click directly on the current main path."));
            return;
        }
        int legId = -1;
        qreal best = std::numeric_limits<qreal>::max();
        for(int i = 0; i < draftRouteLines.size(); i++){
            if(i >= draftRouteLineVectors.size() ||
               draftRouteLineVectors[i] != vectorNodeId)
                continue;
            const QLineF &line = draftRouteLines[i];
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = snappedPoint - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() +
                                        relative.y() * delta.y()) / lengthSquared, 1.0);
            }
            const qreal distance = QLineF(snappedPoint, line.p1() + delta * t).length();
            if(distance < best){
                best = distance;
                legId = i < draftRouteLineLegs.size() ? draftRouteLineLegs[i] : 0;
            }
        }
        if(legId < 0){
            emit pathDraftStatus(tr("<b>Path control could not be assigned</b><br>Click a visible magenta segment."));
            return;
        }
        DraftWaitPoint wait;
        wait.position = snappedPoint;
        wait.vectorNodeId = vectorNodeId;
        wait.legId = legId;
        wait.waitSeconds = pendingWaitSeconds;
        recordUndoState();
        draftWaitPoints.push_back(wait);
        setPathPlacementMode(PlaceAutomatic);
        selectedDraftType = SelectWait;
        selectedDraftIndex = draftWaitPoints.size() - 1;
        emit pathDraftStatus(tr("<b>%1 point added</b><br>%2. Select it and press Delete to remove it.")
                             .arg(waitPointKind(wait.waitSeconds).toHtmlEscaped())
                             .arg(waitPointDescription(wait.waitSeconds).toHtmlEscaped()));
    } else if(pathPlacementMode == PlaceReverse){
        if(!pointOnCurrentFlow(snappedPoint)){
            emit pathDraftStatus(tr("<b>Reverse point must be on magenta</b><br>Throw switches until the track is part of the current flow."));
            return;
        }
        DraftReversePoint reverse;
        reverse.position = snappedPoint;
        reverse.vectorNodeId = vectorNodeId;
        recordUndoState();
        draftWaitPoints.clear();
        draftReversePoints.push_back(reverse);
        draftPassingSidings.clear();
        if(draftLegSwitchRoutes.isEmpty())
            draftLegSwitchRoutes.push_back(QHash<int, int>());
        draftLegSwitchRoutes.push_back(draftLegSwitchRoutes.last());
        draftEndSelected = false;
        setPathPlacementMode(PlaceAutomatic);
        if(computeDraftRoute())
            emit pathDraftStatus(tr("<b>Reverse point added</b><br>%1 reverse point(s). The endpoint was cleared and flow continues to the natural end.")
                                 .arg(draftReversePoints.size()));
        else
            emit pathDraftStatus(tr("<b>Reverse point cannot be reached</b><br>Throw an intervening switch or remove the last reverse point."));
    } else if(!draftStartSelected || pathPlacementMode == PlaceStart){
        recordUndoState();
        draftStart = snappedPoint;
        draftStartVector = vectorNodeId;
        draftStartSelected = true;
        draftEndSelected = false;
        draftReversePoints.clear();
        draftWaitPoints.clear();
        draftPassingSidings.clear();
        draftLegSwitchRoutes.clear();
        draftLegSwitchRoutes.push_back(QHash<int, int>());
        draftForward = true;
        setPathPlacementMode(PlaceAutomatic);
        computeDraftRoute();
        emit pathDraftStatus(tr("<b>Path start selected</b><br>Magenta flows to the natural end. "
                                "Use Reverse Start Direction if it flowed the wrong way, or click the magenta route to place the red endpoint."));
    } else {
        if(!pointOnCurrentFlow(snappedPoint)){
            emit pathDraftStatus(tr("<b>Endpoint must be on magenta</b><br>Throw switches until the desired track is part of the current flow."));
            return;
        }
        draftEnd = snappedPoint;
        draftEndVector = vectorNodeId;
        recordUndoState();
        draftEndSelected = true;
        setPathPlacementMode(PlaceAutomatic);
        if(computeDraftRoute()){
            emit pathDraftStatus(tr("<b>Endpoint placed</b><br>Start vector: %1<br>End vector: %2<br>"
                                    "Throwing a switch will clear this endpoint and resume natural flow.")
                                 .arg(draftStartVector).arg(draftEndVector));
        } else {
            draftEndSelected = false;
            computeDraftRoute();
            emit pathDraftStatus(tr("<b>Endpoint is not on the current flow</b><br>"
                                    "Place it on the magenta route or throw a switch first.")
                                 .arg(draftStartVector).arg(draftEndVector));
        }
    }
    update();
}

bool ActivityTrackViewer::selectDraftObject(const QPointF &screenPoint){
    const QTransform transform = worldTransform();
    auto nearPoint = [&screenPoint](const QPointF &point, qreal radius) {
        return QLineF(screenPoint, point).length() <= radius;
    };
    DraftSelectionType type = SelectNone;
    int index = -1;
    QString description;
    if(draftEndSelected && nearPoint(transform.map(draftEnd), 14.0)){
        type = SelectEnd;
        description = tr("Endpoint");
    } else if(draftStartSelected && nearPoint(transform.map(draftStart), 14.0)){
        type = SelectStart;
        description = tr("Start point");
    } else {
        for(int i = 0; i < draftReversePoints.size(); i++){
            if(nearPoint(transform.map(draftReversePoints[i].position), 14.0)){
                type = SelectReverse;
                index = i;
                description = tr("Reverse point %1").arg(i + 1);
                break;
            }
        }
        if(type == SelectNone){
            for(int i = 0; i < draftWaitPoints.size(); i++){
                if(nearPoint(transform.map(draftWaitPoints[i].position), 14.0)){
                    type = SelectWait;
                    index = i;
                    description = tr("Point %1 — %2")
                        .arg(i + 1)
                        .arg(waitPointDescription(draftWaitPoints[i].waitSeconds));
                    break;
                }
            }
        }
    }
    if(type == SelectNone){
        qreal bestDistance = 10.0;
        for(int i = 0; i < draftPassingSidings.size(); i++){
            for(const QLineF &line : draftPassingSidings[i].lines){
                const QPointF a = transform.map(line.p1());
                const QPointF delta = transform.map(line.p2()) - a;
                const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
                qreal t = 0.0;
                if(lengthSquared > 0.000001){
                    const QPointF relative = screenPoint - a;
                    t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                            lengthSquared, 1.0);
                }
                const qreal distance = QLineF(screenPoint, a + delta * t).length();
                if(distance <= bestDistance){
                    bestDistance = distance;
                    type = SelectPassingSiding;
                    index = i;
                    description = tr("Passing siding %1").arg(i + 1);
                }
            }
        }
    }
    if(type == SelectNone)
        return false;
    selectedDraftType = type;
    selectedDraftIndex = index;
    emit pathDraftStatus(tr("<b>%1 selected</b><br>Press Delete to remove it, or Ctrl+Z as needed.")
                         .arg(description));
    update();
    return true;
}

bool ActivityTrackViewer::toggleDraftSwitch(const QPointF &screenPoint){
    const int index = junctionAt(screenPoint);
    if(index < 0){
        emit pathDraftStatus(tr("<b>No switch selected</b><br>Zoom closer and click an orange switch."));
        return false;
    }
    const JunctionInfo &junction = junctions[index];

    // A switch inside an orange route belongs to that passing path's current
    // traversal, not to the magenta main leg. Recalculate transactionally.
    int passingIndex = -1;
    for(int i = 0; i < draftPassingSidings.size() && passingIndex < 0; i++){
        if(junction.nodeId == draftPassingSidings[i].startJunction ||
           junction.nodeId == draftPassingSidings[i].endJunction){
            passingIndex = i;
            break;
        }
        for(const QLineF &line : draftPassingSidings[i].lines){
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = junction.position - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                        lengthSquared, 1.0);
            }
            if(QLineF(junction.position, line.p1() + delta * t).length() <= 8.0){
                passingIndex = i;
                break;
            }
        }
    }
    if(passingIndex >= 0){
        const DraftState before = captureDraftState();
        QHash<int, int> proposedRoutes = draftPassingSidings[passingIndex].manualSwitchRoutes;
        const int currentPassingExit = draftPassingSidings[passingIndex].switchRoutes.contains(junction.nodeId)
            ? draftPassingSidings[passingIndex].switchRoutes.value(junction.nodeId)
            : qBound(1, junction.mainRoute + 1, 2);
        proposedRoutes.insert(junction.nodeId, currentPassingExit == 1 ? 2 : 1);
        DraftPassingSiding recalculated;
        // A switch on the orange route may lead through a crossover vector to
        // a different turnout on magenta. Always rediscover both boundaries
        // from the TrackDB after a throw; preserving the old endpoints would
        // reject exactly the valid short/long alternatives the user selected.
        const bool recalculatedOk = calculatePassingSiding(
            draftPassingSidings[passingIndex].seedVector, proposedRoutes, recalculated);
        if(recalculatedOk){
            undoStates.push_back(before);
            if(undoStates.size() > 100)
                undoStates.removeFirst();
            redoStates.clear();
            draftPassingSidings[passingIndex] = recalculated;
            selectedJunction = index;
            showTransientMessage(tr("PASSING PATH RECALCULATED - switch %1 now uses exit %2")
                                 .arg(junction.nodeId).arg(proposedRoutes.value(junction.nodeId)), false);
            emit pathDraftStatus(tr("<b>Passing path recalculated</b><br>The orange route remains connected at both ends."));
            update();
            return true;
        }
        showTransientMessage(tr("PASSING PATH CANNOT RECONNECT - switch restored"), true);
        emit pathDraftStatus(tr("<b>Switch throw rejected</b><br>No valid passing route reconnects at both ends. The prior alignment was restored."));
        update();
        return false;
    }

    const DraftState before = captureDraftState();
    if(draftLegSwitchRoutes.isEmpty())
        draftLegSwitchRoutes.push_back(QHash<int, int>());
    QHash<int, int> &currentLegRoutes = draftLegSwitchRoutes.last();
    const int current = currentLegRoutes.contains(junction.nodeId)
        ? currentLegRoutes.value(junction.nodeId)
        : qBound(1, junction.mainRoute + 1, 2);
    currentLegRoutes.insert(junction.nodeId, current == 1 ? 2 : 1);
    selectedJunction = index;
    draftEndSelected = false;
    bool valid = computeDraftRoute();
    QVector<DraftPassingSiding> recalculatedSidings;
    if(valid){
        for(const DraftPassingSiding &oldSiding : before.passingSidings){
            DraftPassingSiding recalculated;
            if(!calculatePassingSiding(oldSiding.seedVector, oldSiding.manualSwitchRoutes, recalculated)){
                valid = false;
                break;
            }
            recalculatedSidings.push_back(recalculated);
        }
    }
    emit junctionSelected(junctionDescription(index));
    if(valid){
        undoStates.push_back(before);
        if(undoStates.size() > 100)
            undoStates.removeFirst();
        redoStates.clear();
        draftPassingSidings = recalculatedSidings;
        showTransientMessage(tr("PATHS RECALCULATED - switch %1 now uses exit %2")
                             .arg(junction.nodeId).arg(currentLegRoutes.value(junction.nodeId)), false);
        emit pathDraftStatus(tr("<b>Switch %1 thrown to exit %2</b><br>"
                                "The endpoint was removed and all passing paths were recalculated.")
                             .arg(junction.nodeId).arg(currentLegRoutes.value(junction.nodeId)));
    } else {
        restoreDraftState(before);
        selectedJunction = index;
        showTransientMessage(tr("NO VALID RECONNECTION - switch %1 restored").arg(junction.nodeId), true);
        emit pathDraftStatus(tr("<b>Switch throw rejected</b><br>The main or passing path could not reconnect, so the prior alignment was restored."));
    }
    update();
    return true;
}

bool ActivityTrackViewer::addPassingSiding(const QPointF &screenPoint){
    QPointF clickedPoint;
    int clickedVector = -1;
    if(!nearestTrackPoint(screenPoint, clickedPoint, clickedVector)){
        emit pathDraftStatus(tr("<b>No siding track selected</b><br>Zoom closer and click directly on the alternate track."));
        return false;
    }
    DraftPassingSiding siding;
    if(!calculatePassingSiding(clickedVector, QHash<int, int>(), siding))
        return false;
    for(const DraftPassingSiding &existing : draftPassingSidings){
        if(existing.startJunction == siding.startJunction && existing.endJunction == siding.endJunction){
            emit pathDraftStatus(tr("<b>Passing siding already assigned</b>"));
            setPathPlacementMode(PlaceAutomatic);
            return false;
        }
    }
    recordUndoState();
    draftPassingSidings.push_back(siding);
    setPathPlacementMode(PlaceAutomatic);
    emit pathDraftStatus(tr("<b>Passing siding assigned</b><br>Orange connects switch %1 to switch %2. Both ends rejoin the main path.")
                         .arg(siding.startJunction).arg(siding.endJunction));
    update();
    return true;
}

bool ActivityTrackViewer::calculatePassingSiding(int clickedVector,
                                                  const QHash<int, int> &switchRoutes,
                                                  DraftPassingSiding &siding){

    // Main-path vectors are recorded directly by the route solver. Inferring
    // them from painted fragments fails at sliced start/end sections.
    if(draftRouteVectors.contains(clickedVector)){
        emit pathDraftStatus(tr("<b>That track is already part of the main path</b><br>Click the alternate siding track between the two switches."));
        return false;
    }

    auto pointRouteDistance = [this](const QPointF &point) {
        qreal best = std::numeric_limits<qreal>::max();
        for(const QLineF &line : draftRouteLines){
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = point - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                        lengthSquared, 1.0);
            }
            best = qMin(best, QLineF(point, line.p1() + delta * t).length());
        }
        return best;
    };

    QHash<int, QVector<QPair<int, int>>> adjacency;
    for(const JunctionInfo &junction : junctions){
        if(junction.pins[0] <= 0)
            continue;
        // An explicitly thrown crossover is allowed to bypass an early main
        // connection and continue toward a later valid SidingEnd.
        const int firstExit = switchRoutes.contains(junction.nodeId)
            ? switchRoutes.value(junction.nodeId) : 1;
        const int lastExit = switchRoutes.contains(junction.nodeId)
            ? firstExit : 2;
        for(int exit = firstExit; exit <= lastExit; exit++){
            if(junction.pins[exit] <= 0)
                continue;
            adjacency[junction.pins[0]].push_back(qMakePair(junction.pins[exit], junction.nodeId));
            adjacency[junction.pins[exit]].push_back(qMakePair(junction.pins[0], junction.nodeId));
        }
    }

    struct Connection { int junction; int vector; int mainVector; int depth; qreal routeOrder; };
    auto routeOrder = [this](const QPointF &point) {
        qreal accumulated = 0.0;
        qreal bestOrder = 0.0;
        qreal best = std::numeric_limits<qreal>::max();
        for(const QLineF &line : draftRouteLines){
            const QPointF delta = line.p2() - line.p1();
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = point - line.p1();
                t = qBound<qreal>(0.0, (relative.x() * delta.x() + relative.y() * delta.y()) /
                                        lengthSquared, 1.0);
            }
            const qreal error = QLineF(point, line.p1() + delta * t).length();
            if(error < best){
                best = error;
                bestOrder = accumulated + line.length() * t;
            }
            accumulated += line.length();
        }
        return bestOrder;
    };
    QVector<Connection> connections;
    QHash<int, int> parent;
    QHash<int, int> parentJunction;
    QHash<int, int> depth;
    std::queue<int> pending;
    parent.insert(clickedVector, -1);
    depth.insert(clickedVector, 0);
    pending.push(clickedVector);
    while(!pending.empty() && parent.size() < 10000){
        const int current = pending.front();
        pending.pop();
        for(const QPair<int, int> &edge : adjacency.value(current)){
            const bool currentIsMain = draftRouteVectors.contains(current);
            const bool nextIsMain = draftRouteVectors.contains(edge.first);
            if(!currentIsMain && nextIsMain){
                // Normally this is a valid reconnection candidate. When this
                // crossover was explicitly thrown, however, it is a transit
                // across/along the main path and the search must continue.
                if(switchRoutes.contains(edge.second)){
                    if(!parent.contains(edge.first)){
                        parent.insert(edge.first, current);
                        parentJunction.insert(edge.first, edge.second);
                        depth.insert(edge.first, depth.value(current) + 1);
                        pending.push(edge.first);
                    }
                    continue;
                }
                const QPointF connectionPoint = junctionPosition(edge.second);
                if(pointRouteDistance(connectionPoint) <= 6.0){
                    bool alreadyFound = false;
                    for(const Connection &existing : connections){
                        if(existing.junction == edge.second){
                            alreadyFound = true;
                            break;
                        }
                    }
                    if(!alreadyFound)
                        connections.push_back({edge.second, current, edge.first,
                                               depth.value(current), routeOrder(connectionPoint)});
                }
                continue;
            }
            if(!parent.contains(edge.first)){
                parent.insert(edge.first, current);
                parentJunction.insert(edge.first, edge.second);
                depth.insert(edge.first, depth.value(current) + 1);
                pending.push(edge.first);
            }
        }
    }
    if(connections.size() < 2){
        emit pathDraftStatus(tr("<b>Not a complete passing siding</b><br>This track does not reconnect to the current main path at two different switches."));
        return false;
    }
    auto rootPath = [&parent, &parentJunction, clickedVector](int node,
                                                              QVector<int> &vectors,
                                                              QVector<int> &junctionIds) {
        while(node != clickedVector){
            vectors.push_back(node);
            junctionIds.push_back(parentJunction.value(node));
            node = parent.value(node, -1);
            if(node < 0)
                return false;
        }
        vectors.push_back(clickedVector);
        std::reverse(vectors.begin(), vectors.end());
        std::reverse(junctionIds.begin(), junctionIds.end());
        return true;
    };

    struct ConnectionPair { int a; int b; qreal span; int depth; };
    QVector<ConnectionPair> pairs;
    for(int a = 0; a < connections.size(); a++){
        for(int b = a + 1; b < connections.size(); b++){
            if(connections[a].junction == connections[b].junction)
                continue;
            pairs.push_back({a, b,
                             qAbs(connections[a].routeOrder - connections[b].routeOrder),
                             connections[a].depth + connections[b].depth});
        }
    }
    std::sort(pairs.begin(), pairs.end(), [](const ConnectionPair &a, const ConnectionPair &b) {
        if(qAbs(a.span - b.span) > 0.01)
            return a.span > b.span;
        return a.depth > b.depth;
    });

    Connection connectionA;
    Connection connectionB;
    QVector<int> sidingVectors;
    QVector<int> internalJunctions;
    QHash<int, int> resolvedRoutes;
    bool foundPair = false;
    for(const ConnectionPair &pair : pairs){
        const Connection candidateA = connections[pair.a];
        const Connection candidateB = connections[pair.b];
        QVector<int> rootToA, rootToB, rootJunctionsA, rootJunctionsB;
        if(!rootPath(candidateA.vector, rootToA, rootJunctionsA) ||
           !rootPath(candidateB.vector, rootToB, rootJunctionsB))
            continue;

        // The selected route must pass through the clicked track exactly once.
        // Two reconnections reached through the same branch would double back.
        QSet<int> branchA;
        for(int i = 1; i < rootToA.size(); i++)
            branchA.insert(rootToA[i]);
        bool branchesOverlap = false;
        for(int i = 1; i < rootToB.size(); i++){
            if(branchA.contains(rootToB[i])){
                branchesOverlap = true;
                break;
            }
        }
        if(branchesOverlap)
            continue;

        QVector<int> candidateVectors;
        QVector<int> candidateJunctions;
        for(int i = rootToA.size() - 1; i >= 0; i--)
            candidateVectors.push_back(rootToA[i]);
        for(int i = rootJunctionsA.size() - 1; i >= 0; i--)
            candidateJunctions.push_back(rootJunctionsA[i]);
        for(int i = 1; i < rootToB.size(); i++)
            candidateVectors.push_back(rootToB[i]);
        for(int junctionId : rootJunctionsB)
            candidateJunctions.push_back(junctionId);

        // Verify every switch, including the two switches where orange leaves
        // and rejoins magenta. This also records all inferred alignments.
        QVector<int> verificationVectors;
        QVector<int> verificationJunctions;
        verificationVectors.push_back(candidateA.mainVector);
        verificationVectors += candidateVectors;
        verificationVectors.push_back(candidateB.mainVector);
        verificationJunctions.push_back(candidateA.junction);
        verificationJunctions += candidateJunctions;
        verificationJunctions.push_back(candidateB.junction);
        QHash<int, int> candidateRoutes = switchRoutes;
        if(!resolvePassingSwitchRoutes(verificationVectors, verificationJunctions, candidateRoutes))
            continue;

        connectionA = candidateA;
        connectionB = candidateB;
        sidingVectors = candidateVectors;
        internalJunctions = candidateJunctions;
        resolvedRoutes = candidateRoutes;
        foundPair = true;
        break;
    }
    if(!foundPair){
        emit pathDraftStatus(tr("<b>No consistent passing route</b><br>The reachable connections require conflicting switch positions."));
        return false;
    }

    siding = DraftPassingSiding();
    siding.seedVector = clickedVector;
    siding.vectors = sidingVectors;
    siding.junctionIds = internalJunctions;
    siding.switchRoutes = resolvedRoutes;
    siding.manualSwitchRoutes = switchRoutes;
    siding.startJunction = connectionA.junction;
    siding.endJunction = connectionB.junction;
    siding.start = junctionPosition(siding.startJunction);
    siding.end = junctionPosition(siding.endJunction);
    for(int i = 0; i < sidingVectors.size(); i++){
        const QPointF from = i == 0 ? siding.start : junctionPosition(internalJunctions[i - 1]);
        const QPointF to = i == sidingVectors.size() - 1 ? siding.end
                                                         : junctionPosition(internalJunctions[i]);
        appendVectorSlice(sidingVectors[i], from, to, siding.lines);
    }
    if(siding.lines.isEmpty())
        return false;
    if(routeOrder(siding.end) < routeOrder(siding.start)){
        std::swap(siding.start, siding.end);
        std::swap(siding.startJunction, siding.endJunction);
        std::reverse(siding.lines.begin(), siding.lines.end());
        std::reverse(siding.vectors.begin(), siding.vectors.end());
        std::reverse(siding.junctionIds.begin(), siding.junctionIds.end());
        for(QLineF &line : siding.lines)
            line = QLineF(line.p2(), line.p1());
    }
    return true;
}

bool ActivityTrackViewer::resolvePassingSwitchRoutes(const QVector<int> &vectors,
                                                      const QVector<int> &junctionIds,
                                                      QHash<int, int> &switchRoutes) const {
    if(vectors.size() != junctionIds.size() + 1)
        return false;
    for(int i = 0; i < junctionIds.size(); i++){
        const int junctionId = junctionIds[i];
        const int a = vectors[i];
        const int b = vectors[i + 1];
        int requiredExit = -1;
        for(const JunctionInfo &junction : junctions){
            if(junction.nodeId != junctionId)
                continue;
            if((junction.pins[0] == a && junction.pins[1] == b) ||
               (junction.pins[0] == b && junction.pins[1] == a))
                requiredExit = 1;
            else if((junction.pins[0] == a && junction.pins[2] == b) ||
                    (junction.pins[0] == b && junction.pins[2] == a))
                requiredExit = 2;
            break;
        }
        if(requiredExit < 0)
            return false;
        if(switchRoutes.contains(junctionId) && switchRoutes.value(junctionId) != requiredExit)
            return false;
        switchRoutes.insert(junctionId, requiredExit);
    }
    return true;
}

bool ActivityTrackViewer::recalculatePassingSiding(const DraftPassingSiding &original,
                                                    const QHash<int, int> &switchRoutes,
                                                    DraftPassingSiding &siding){
    const JunctionInfo *startInfo = NULL;
    const JunctionInfo *endInfo = NULL;
    for(const JunctionInfo &junction : junctions){
        if(junction.nodeId == original.startJunction)
            startInfo = &junction;
        if(junction.nodeId == original.endJunction)
            endInfo = &junction;
    }
    if(startInfo == NULL || endInfo == NULL)
        return false;

    QSet<int> startVectors;
    QSet<int> endVectors;
    for(int pin = 0; pin < 3; pin++){
        if(startInfo->pins[pin] > 0 && !draftRouteVectors.contains(startInfo->pins[pin]))
            startVectors.insert(startInfo->pins[pin]);
        if(endInfo->pins[pin] > 0 && !draftRouteVectors.contains(endInfo->pins[pin]))
            endVectors.insert(endInfo->pins[pin]);
    }
    if(startVectors.isEmpty() || endVectors.isEmpty())
        return false;

    QHash<int, QVector<QPair<int, int>>> adjacency;
    for(const JunctionInfo &junction : junctions){
        if(junction.nodeId == original.startJunction || junction.nodeId == original.endJunction)
            continue;
        if(junction.pins[0] <= 0)
            continue;
        const int firstExit = switchRoutes.contains(junction.nodeId)
            ? switchRoutes.value(junction.nodeId) : 1;
        const int lastExit = switchRoutes.contains(junction.nodeId)
            ? firstExit : 2;
        for(int exit = firstExit; exit <= lastExit; exit++){
            if(junction.pins[exit] <= 0)
                continue;
            const int trunkVector = junction.pins[0];
            const int exitVector = junction.pins[exit];
            // A fixed-end recalculation remains off magenta between boundaries.
            if(draftRouteVectors.contains(trunkVector) || draftRouteVectors.contains(exitVector))
                continue;
            adjacency[trunkVector].push_back(qMakePair(exitVector, junction.nodeId));
            adjacency[exitVector].push_back(qMakePair(trunkVector, junction.nodeId));
        }
    }

    QHash<int, int> parent;
    QHash<int, int> parentJunction;
    std::queue<int> pending;
    for(int vectorId : startVectors){
        parent.insert(vectorId, -1);
        pending.push(vectorId);
    }
    int reached = -1;
    while(!pending.empty() && parent.size() < 10000){
        const int current = pending.front();
        pending.pop();
        if(endVectors.contains(current)){
            reached = current;
            break;
        }
        for(const QPair<int, int> &edge : adjacency.value(current)){
            if(parent.contains(edge.first))
                continue;
            parent.insert(edge.first, current);
            parentJunction.insert(edge.first, edge.second);
            pending.push(edge.first);
        }
    }
    if(reached < 0)
        return false;

    QVector<int> vectors;
    QVector<int> internalJunctions;
    int current = reached;
    vectors.push_back(current);
    while(parent.value(current, -1) >= 0){
        internalJunctions.push_back(parentJunction.value(current));
        current = parent.value(current);
        vectors.push_back(current);
    }
    std::reverse(vectors.begin(), vectors.end());
    std::reverse(internalJunctions.begin(), internalJunctions.end());

    siding = DraftPassingSiding();
    siding.startJunction = original.startJunction;
    siding.endJunction = original.endJunction;
    siding.start = junctionPosition(siding.startJunction);
    siding.end = junctionPosition(siding.endJunction);
    siding.seedVector = vectors.first();
    siding.vectors = vectors;
    siding.junctionIds = internalJunctions;
    siding.switchRoutes = switchRoutes;
    siding.manualSwitchRoutes = switchRoutes;
    for(int i = 0; i < vectors.size(); i++){
        const QPointF from = i == 0 ? siding.start : junctionPosition(internalJunctions[i - 1]);
        const QPointF to = i == vectors.size() - 1 ? siding.end
                                                   : junctionPosition(internalJunctions[i]);
        appendVectorSlice(vectors[i], from, to, siding.lines);
    }
    int startMainVector = -1;
    int endMainVector = -1;
    for(int pin = 0; pin < 3; pin++){
        if(startInfo->pins[pin] > 0 && draftRouteVectors.contains(startInfo->pins[pin])){
            QHash<int, int> testRoutes;
            QVector<int> testVectors = {startInfo->pins[pin], vectors.first()};
            QVector<int> testJunctions = {original.startJunction};
            if(resolvePassingSwitchRoutes(testVectors, testJunctions, testRoutes)){
                startMainVector = startInfo->pins[pin];
                break;
            }
        }
    }
    for(int pin = 0; pin < 3; pin++){
        if(endInfo->pins[pin] > 0 && draftRouteVectors.contains(endInfo->pins[pin])){
            QHash<int, int> testRoutes;
            QVector<int> testVectors = {vectors.last(), endInfo->pins[pin]};
            QVector<int> testJunctions = {original.endJunction};
            if(resolvePassingSwitchRoutes(testVectors, testJunctions, testRoutes)){
                endMainVector = endInfo->pins[pin];
                break;
            }
        }
    }
    if(startMainVector < 0 || endMainVector < 0)
        return false;

    QVector<int> verificationVectors;
    QVector<int> verificationJunctions;
    verificationVectors.push_back(startMainVector);
    verificationVectors += vectors;
    verificationVectors.push_back(endMainVector);
    verificationJunctions.push_back(original.startJunction);
    verificationJunctions += internalJunctions;
    verificationJunctions.push_back(original.endJunction);
    QHash<int, int> resolvedRoutes = switchRoutes;
    if(!resolvePassingSwitchRoutes(verificationVectors, verificationJunctions, resolvedRoutes))
        return false;
    siding.switchRoutes = resolvedRoutes;
    siding.manualSwitchRoutes.clear();
    for(auto it = switchRoutes.constBegin(); it != switchRoutes.constEnd(); ++it){
        if(resolvedRoutes.contains(it.key()) && resolvedRoutes.value(it.key()) == it.value())
            siding.manualSwitchRoutes.insert(it.key(), it.value());
    }
    return !siding.lines.isEmpty();
}

bool ActivityTrackViewer::computeDraftRoute(){
    draftRouteLines.clear();
    draftRouteMedium.clear();
    draftRouteOverview.clear();
    draftRouteLineVectors.clear();
    draftRouteLineSegments.clear();
    draftRouteLineLegs.clear();
    draftRouteVectors.clear();
    draftOverlapLines.clear();
    draftOverlapMedium.clear();
    draftOverlapOverview.clear();
    if(!draftStartSelected)
        return false;

    QPointF currentPoint = draftStart;
    int currentVector = draftStartVector;
    while(draftLegSwitchRoutes.size() < draftReversePoints.size() + 1)
        draftLegSwitchRoutes.push_back(draftLegSwitchRoutes.isEmpty()
                                       ? QHash<int, int>() : draftLegSwitchRoutes.last());
    for(int reverseIndex = 0; reverseIndex < draftReversePoints.size(); reverseIndex++){
        const DraftReversePoint &reverse = draftReversePoints[reverseIndex];
        if(!computeRouteLeg(currentPoint, currentVector, reverse.position,
                            reverse.vectorNodeId, draftLegSwitchRoutes[reverseIndex],
                            draftRouteLines, NULL, NULL, reverseIndex))
            return false;
        currentPoint = reverse.position;
        currentVector = reverse.vectorNodeId;
    }

    bool valid = false;
    if(draftEndSelected){
        valid = computeRouteLeg(currentPoint, currentVector, draftEnd,
                                draftEndVector, draftLegSwitchRoutes[draftReversePoints.size()],
                                draftRouteLines, NULL, NULL, draftReversePoints.size());
    } else {
        QPointF direction;
        if(!draftRouteLines.isEmpty()){
            // A reverse point sends the train back in the opposite direction.
            // Do not derive that direction from only the final rendered line:
            // the last line can be a tiny snap/connector fragment whose heading
            // is numerically unstable. Walk back along the directed route until
            // we have a useful baseline on the TrackDB vector, then point from
            // the reverse location back toward it. The original start point is
            // deliberately not a boundary; natural flow may pass straight
            // through it on the return leg.
            QPointF priorPoint = currentPoint;
            qreal baselineLength = 0.0;
            for(int lineIndex = draftRouteLines.size() - 1; lineIndex >= 0; lineIndex--){
                const QLineF &line = draftRouteLines[lineIndex];
                if(line.length() <= 0.001)
                    continue;
                baselineLength += line.length();
                priorPoint = line.p1();
                if(baselineLength >= 2.0 && QLineF(currentPoint, priorPoint).length() >= 0.5)
                    break;
            }
            direction = priorPoint - currentPoint;
        } else {
            const QVector<QLineF> lines = vectorSegments.value(currentVector);
            if(lines.isEmpty())
                return false;
            // A reverse point placed exactly at the start has no incoming
            // rendered line, so use the opposite of the chosen start flow.
            direction = draftForward ? lines.first().p1() - currentPoint
                                     : lines.last().p2() - currentPoint;
        }
        valid = extendRouteToNaturalEnd(currentPoint, currentVector, direction,
                                        draftRouteLines, draftEnd, draftEndVector,
                                        draftLegSwitchRoutes[draftReversePoints.size()],
                                        draftReversePoints.size());
    }
    if(!valid)
        return false;
    draftRouteMedium = simplifiedLines(draftRouteLines, 8.0);
    draftRouteOverview = simplifiedLines(draftRouteLines, 48.0);
    rebuildOverlapLines();
    return !draftRouteLines.isEmpty();
}

bool ActivityTrackViewer::computeRouteLeg(const QPointF &from, int fromVector,
                                           const QPointF &to, int toVector,
                                           QHash<int, int> &switchRoutes,
                                           QVector<QLineF> &output,
                                           QVector<int> *orderedVectors,
                                           QVector<int> *orderedJunctions,
                                           int overlapLeg){
    QVector<int> *lineVectors = overlapLeg >= 0 ? &draftRouteLineVectors : NULL;
    QVector<int> *lineSegments = overlapLeg >= 0 ? &draftRouteLineSegments : NULL;
    QVector<int> *lineLegs = overlapLeg >= 0 ? &draftRouteLineLegs : NULL;
    if(fromVector == toVector){
        draftRouteVectors.insert(fromVector);
        appendVectorSlice(fromVector, from, to, output, lineVectors, lineSegments,
                          lineLegs, overlapLeg);
        if(orderedVectors != NULL){
            orderedVectors->clear();
            orderedVectors->push_back(fromVector);
        }
        if(orderedJunctions != NULL)
            orderedJunctions->clear();
        return true;
    }

    // Facing movement (trunk to exit) obeys the selected route. Trailing
    // movement (either exit to trunk) is always legal and self-aligns the
    // switch, matching how a path flows through spring/trailing points.
    QHash<int, QVector<QPair<int, int>>> adjacency;
    for(const JunctionInfo &junction : junctions){
        if(junction.pins[0] <= 0)
            continue;
        const int exit = switchRoutes.contains(junction.nodeId)
            ? switchRoutes.value(junction.nodeId)
            : qBound(1, junction.mainRoute + 1, 2);
        if(junction.pins[exit] <= 0)
            continue;
        adjacency[junction.pins[0]].push_back(qMakePair(junction.pins[exit], junction.nodeId));
        for(int trailingExit = 1; trailingExit <= 2; trailingExit++){
            if(junction.pins[trailingExit] > 0)
                adjacency[junction.pins[trailingExit]].push_back(
                    qMakePair(junction.pins[0], junction.nodeId));
        }
    }

    QHash<int, qreal> vectorLength;
    for(auto it = vectorSegments.constBegin(); it != vectorSegments.constEnd(); ++it){
        qreal length = 0.0;
        for(const QLineF &line : it.value())
            length += line.length();
        vectorLength.insert(it.key(), qMax<qreal>(1.0, length));
    }

    typedef QPair<qreal, int> QueueItem;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> queue;
    QHash<int, qreal> distance;
    QHash<int, int> previous;
    QHash<int, int> previousJunction;
    distance.insert(fromVector, 0.0);
    queue.push(qMakePair(0.0, fromVector));

    while(!queue.empty()){
        const QueueItem current = queue.top();
        queue.pop();
        if(current.first > distance.value(current.second, std::numeric_limits<qreal>::max()) + 0.001)
            continue;
        if(current.second == toVector)
            break;
        const QVector<QPair<int, int>> edges = adjacency.value(current.second);
        for(const QPair<int, int> &edge : edges){
            const qreal cost = (vectorLength.value(current.second, 1.0) +
                                vectorLength.value(edge.first, 1.0)) * 0.5;
            const qreal candidate = current.first + cost;
            if(candidate < distance.value(edge.first, std::numeric_limits<qreal>::max())){
                distance.insert(edge.first, candidate);
                previous.insert(edge.first, current.second);
                previousJunction.insert(edge.first, edge.second);
                queue.push(qMakePair(candidate, edge.first));
            }
        }
    }

    if(!previous.contains(toVector))
        return false;

    QVector<int> vectors;
    QVector<int> connectingJunctions;
    int current = toVector;
    vectors.push_back(current);
    while(current != fromVector){
        connectingJunctions.push_back(previousJunction.value(current));
        current = previous.value(current, -1);
        if(current < 0)
            return false;
        vectors.push_back(current);
    }
    std::reverse(vectors.begin(), vectors.end());
    std::reverse(connectingJunctions.begin(), connectingJunctions.end());

    if(orderedVectors != NULL)
        *orderedVectors = vectors;
    if(orderedJunctions != NULL)
        *orderedJunctions = connectingJunctions;

    // Reject an immediate reversal through two exits of the same switch.
    for(int i = 1; i < connectingJunctions.size(); i++){
        if(connectingJunctions[i] == connectingJunctions[i - 1])
            return false;
    }

    // Record every trailing alignment inferred by the chosen traversal.
    for(int i = 0; i < connectingJunctions.size(); i++){
        for(const JunctionInfo &junction : junctions){
            if(junction.nodeId != connectingJunctions[i])
                continue;
            if(vectors[i + 1] == junction.pins[0]){
                if(vectors[i] == junction.pins[1])
                    switchRoutes.insert(junction.nodeId, 1);
                else if(vectors[i] == junction.pins[2])
                    switchRoutes.insert(junction.nodeId, 2);
            }
            break;
        }
    }

    for(int i = 0; i < vectors.size(); i++){
        draftRouteVectors.insert(vectors[i]);
        const QPointF sliceFrom = i == 0 ? from : junctionPosition(connectingJunctions[i - 1]);
        const QPointF sliceTo = i == vectors.size() - 1 ? to : junctionPosition(connectingJunctions[i]);
        appendVectorSlice(vectors[i], sliceFrom, sliceTo, output,
                          lineVectors, lineSegments, lineLegs, overlapLeg);
    }
    return true;
}

bool ActivityTrackViewer::extendRouteToNaturalEnd(const QPointF &from, int fromVector,
                                                   const QPointF &travelDirection,
                                                   QVector<QLineF> &output,
                                                   QPointF &naturalEnd,
                                                   int &naturalEndVector,
                                                   QHash<int, int> &switchRoutes,
                                                   int overlapLeg) {
    QVector<int> *lineVectors = overlapLeg >= 0 ? &draftRouteLineVectors : NULL;
    QVector<int> *lineSegments = overlapLeg >= 0 ? &draftRouteLineSegments : NULL;
    QVector<int> *lineLegs = overlapLeg >= 0 ? &draftRouteLineLegs : NULL;
    QPointF point = from;
    int vectorId = fromVector;
    QPointF direction = travelDirection;
    int previousJunction = -1;
    QSet<QString> visited;

    for(int step = 0; step < 10000; step++){
        draftRouteVectors.insert(vectorId);
        const QVector<QLineF> lines = vectorSegments.value(vectorId);
        if(lines.isEmpty())
            return false;
        const QPointF first = lines.first().p1();
        const QPointF last = lines.last().p2();
        const QPointF toFirst = first - point;
        const QPointF toLast = last - point;
        const qreal firstDot = direction.x() * toFirst.x() + direction.y() * toFirst.y();
        const qreal lastDot = direction.x() * toLast.x() + direction.y() * toLast.y();
        const QPointF terminal = lastDot >= firstDot ? last : first;
        appendVectorSlice(vectorId, point, terminal, output,
                          lineVectors, lineSegments, lineLegs, overlapLeg);

        const JunctionInfo *nextJunction = NULL;
        int arrivalPin = -1;
        qreal bestDistance = std::numeric_limits<qreal>::max();
        for(const JunctionInfo &junction : junctions){
            if(junction.nodeId == previousJunction)
                continue;
            for(int pin = 0; pin < 3; pin++){
                if(junction.pins[pin] != vectorId)
                    continue;
                const qreal distance = QLineF(terminal, junction.position).length();
                if(distance < bestDistance){
                    bestDistance = distance;
                    nextJunction = &junction;
                    arrivalPin = pin;
                }
            }
        }
        if(nextJunction == NULL || bestDistance > 100.0){
            naturalEnd = terminal;
            naturalEndVector = vectorId;
            return true;
        }

        if(QLineF(terminal, nextJunction->position).length() > 0.001){
            output.push_back(QLineF(terminal, nextJunction->position));
            if(lineVectors != NULL)
                lineVectors->push_back(vectorId);
            if(lineSegments != NULL)
                lineSegments->push_back(terminal == last ? lines.size() - 1 : 0);
            if(lineLegs != NULL)
                lineLegs->push_back(overlapLeg);
        }
        int activeExit = switchRoutes.contains(nextJunction->nodeId)
            ? switchRoutes.value(nextJunction->nodeId)
            : qBound(1, nextJunction->mainRoute + 1, 2);
        int departurePin = -1;
        if(arrivalPin == 0)
            departurePin = activeExit;
        else {
            // Approaching from either exit is a trailing move: align the
            // switch to the incoming leg and continue to the trunk.
            switchRoutes.insert(nextJunction->nodeId, arrivalPin);
            departurePin = 0;
        }
        if(departurePin < 0 || nextJunction->pins[departurePin] <= 0){
            naturalEnd = nextJunction->position;
            naturalEndVector = vectorId;
            return true;
        }

        const int nextVector = nextJunction->pins[departurePin];
        const QString state = QString::number(vectorId) + QLatin1Char(':') +
                              QString::number(nextJunction->nodeId) + QLatin1Char(':') +
                              QString::number(nextVector);
        if(visited.contains(state)){
            naturalEnd = nextJunction->position;
            naturalEndVector = vectorId;
            return true;
        }
        visited.insert(state);
        previousJunction = nextJunction->nodeId;
        point = nextJunction->position;
        vectorId = nextVector;
        const QVector<QLineF> nextLines = vectorSegments.value(vectorId);
        if(nextLines.isEmpty())
            return false;
        const QPointF nextFirst = nextLines.first().p1();
        const QPointF nextLast = nextLines.last().p2();
        direction = QLineF(point, nextFirst).length() < QLineF(point, nextLast).length()
            ? nextLast - point : nextFirst - point;
    }
    return false;
}

void ActivityTrackViewer::rebuildOverlapLines(){
    draftOverlapLines.clear();
    QHash<QString, QSet<int>> segmentLegs;
    if(draftRouteLineVectors.size() == draftRouteLines.size() &&
       draftRouteLineSegments.size() == draftRouteLines.size() &&
       draftRouteLineLegs.size() == draftRouteLines.size()){
        for(int lineIndex = 0; lineIndex < draftRouteLines.size(); lineIndex++){
            const QLineF &line = draftRouteLines[lineIndex];
            const int vectorId = draftRouteLineVectors[lineIndex];
            const int segmentId = draftRouteLineSegments[lineIndex];
            const int legId = draftRouteLineLegs[lineIndex];
            const QString key = QString::number(vectorId) + QLatin1Char(':') +
                                QString::number(segmentId);
            QSet<int> &usedLegs = segmentLegs[key];
            if(!usedLegs.isEmpty() && !usedLegs.contains(legId))
                draftOverlapLines.push_back(line);
            usedLegs.insert(legId);
        }
    } else {
        // Defensive fallback for any future route producer which supplies
        // geometry without TrackDB segment metadata.
        QSet<QString> seen;
        for(const QLineF &line : draftRouteLines){
            qint64 ax = qRound64(line.p1().x() * 4.0);
            qint64 ay = qRound64(line.p1().y() * 4.0);
            qint64 bx = qRound64(line.p2().x() * 4.0);
            qint64 by = qRound64(line.p2().y() * 4.0);
            if(ax > bx || (ax == bx && ay > by)){
                std::swap(ax, bx);
                std::swap(ay, by);
            }
            const QString key = QString::number(ax) + QLatin1Char(',') + QString::number(ay) +
                                QLatin1Char(':') + QString::number(bx) + QLatin1Char(',') +
                                QString::number(by);
            if(seen.contains(key))
                draftOverlapLines.push_back(line);
            else
                seen.insert(key);
        }
    }
    draftOverlapMedium = simplifiedLines(draftOverlapLines, 8.0);
    draftOverlapOverview = simplifiedLines(draftOverlapLines, 48.0);
}

void ActivityTrackViewer::appendVectorSlice(int vectorNodeId, const QPointF &from, const QPointF &to,
                                            QVector<QLineF> &output, QVector<int> *vectorIds,
                                            QVector<int> *segmentIds, QVector<int> *legIds,
                                            int legId) const {
    const auto linesIt = vectorSegments.constFind(vectorNodeId);
    if(linesIt == vectorSegments.constEnd() || linesIt->isEmpty())
        return;
    const QVector<QLineF> &lines = linesIt.value();

    struct Location { int index; qreal t; qreal distance; QPointF point; };
    auto locate = [&lines](const QPointF &target) {
        Location best = {0, 0.0, 0.0, lines.first().p1()};
        qreal bestSquared = std::numeric_limits<qreal>::max();
        qreal accumulated = 0.0;
        for(int i = 0; i < lines.size(); i++){
            const QPointF a = lines[i].p1();
            const QPointF delta = lines[i].p2() - a;
            const qreal lengthSquared = delta.x() * delta.x() + delta.y() * delta.y();
            const qreal length = qSqrt(lengthSquared);
            qreal t = 0.0;
            if(lengthSquared > 0.000001){
                const QPointF relative = target - a;
                t = qBound<qreal>(0.0,
                    (relative.x() * delta.x() + relative.y() * delta.y()) / lengthSquared, 1.0);
            }
            const QPointF point = a + delta * t;
            const QPointF error = target - point;
            const qreal squared = error.x() * error.x() + error.y() * error.y();
            if(squared < bestSquared){
                bestSquared = squared;
                best = {i, t, accumulated + length * t, point};
            }
            accumulated += length;
        }
        return best;
    };
    auto addLine = [&output, vectorIds, segmentIds, legIds, vectorNodeId, legId]
                   (const QPointF &a, const QPointF &b, int segmentId) {
        if(QLineF(a, b).length() > 0.001){
            output.push_back(QLineF(a, b));
            if(vectorIds != NULL)
                vectorIds->push_back(vectorNodeId);
            if(segmentIds != NULL)
                segmentIds->push_back(segmentId);
            if(legIds != NULL)
                legIds->push_back(legId);
        }
    };

    const Location a = locate(from);
    const Location b = locate(to);
    addLine(from, a.point, a.index);
    if(a.index == b.index){
        addLine(a.point, b.point, a.index);
    } else if(a.distance < b.distance){
        addLine(a.point, lines[a.index].p2(), a.index);
        for(int i = a.index + 1; i < b.index; i++)
            addLine(lines[i].p1(), lines[i].p2(), i);
        addLine(lines[b.index].p1(), b.point, b.index);
    } else {
        addLine(a.point, lines[a.index].p1(), a.index);
        for(int i = a.index - 1; i > b.index; i--)
            addLine(lines[i].p2(), lines[i].p1(), i);
        addLine(lines[b.index].p2(), b.point, b.index);
    }
    addLine(b.point, to, b.index);
}

QVector<QLineF> ActivityTrackViewer::simplifiedLines(const QVector<QLineF> &source,
                                                     qreal minimumLength){
    QVector<QLineF> result;
    if(source.isEmpty())
        return result;
    result.reserve(qMax(1, source.size() / qMax(1, qRound(minimumLength))));
    QPointF start = source.first().p1();
    QPointF last = start;
    qreal accumulated = 0.0;
    for(int i = 0; i < source.size(); i++){
        // Do not bridge disconnected line groups (important for route-wide LOD).
        if(QLineF(last, source[i].p1()).length() > 0.1){
            if(QLineF(start, last).length() > 0.001)
                result.push_back(QLineF(start, last));
            start = source[i].p1();
            accumulated = 0.0;
        }
        accumulated += source[i].length();
        last = source[i].p2();
        if(accumulated >= minimumLength){
            if(QLineF(start, last).length() > 0.001)
                result.push_back(QLineF(start, last));
            start = last;
            accumulated = 0.0;
        }
    }
    if(QLineF(start, last).length() > 0.001)
        result.push_back(QLineF(start, last));
    return result;
}

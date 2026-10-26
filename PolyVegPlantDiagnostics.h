// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef POLYVEGPLANTDIAGNOSTICS_H
#define POLYVEGPLANTDIAGNOSTICS_H

#include "ForestGenerator.h"

struct PolyVegPlantDiagnostics {
    int tileX = 0, tileZ = 0;
    QString boundaryDescription;
    QString reason;
    QString osmMode;
    int osmPolygons = -1;
    bool tileBaked = false;
    bool generationEvaluated = false;
    double usableArea = 0.0;
    qint64 requested = 0, target = 0, attempts = 0, generated = 0;
    qint64 limitSkipped = 0;
    qint64 outside = 0, feather = 0, spacing = 0;
    int slope = 0, terrain = 0, track = 0, road = 0, water = 0;
    int tileEdge = 0, osm = 0, bake = 0;
    int placementTerrain = 0, placementFailed = 0, placementTileEdge = 0;

    void addGeneration(const ForestGenerationResult &result) {
        generationEvaluated = true;
        usableArea += result.usableAreaSquareMetres;
        requested += result.requestedCount;
        target += result.targetCount;
        attempts += result.attempts;
        generated += result.candidates.size();
        outside += result.rejectedOutside;
        feather += result.rejectedEdgeFeather;
        spacing += result.rejectedOccupied;
    }

    QString text() const {
        QString report = QString(
            "No vegetation was planted.\n\n"
            "Tile tested: %1, %2\n%3\n"
            "OSM mode: %4\nOSM polygons overlapping tile bounds: %5\n"
            "Existing bake detected on tested tile: %6\n\n"
            "Usable boundary area: %7 m2\n"
            "Requested / target positions: %8 / %9\n"
            "Generation attempts: %10\nGenerated candidates: %11\n\n"
            "Generation rejects (first failed check per attempt):\n"
            "Outside boundary / holes / footprint: %12\n"
            "Edge feathering: %13\nSpacing / occupied footprint: %14\n"
            "Unavailable terrain or slope samples: %15\nAbove slope limit: %16\n"
            "Track clearance: %17\nRoad clearance: %18\nWater clearance: %19\n"
            "Outside target tile: %20\n\n"
            "Generated candidates rejected afterward:\n"
            "OSM coverage / exclusions: %21\nBaked-tile protection: %22\n"
            "Terrain unavailable during placement: %23\n"
            "Object creation failed: %24")
            .arg(tileX).arg(tileZ).arg(boundaryDescription).arg(osmMode)
            .arg(osmPolygons < 0 ? QStringLiteral("Not evaluated")
                                : QString::number(osmPolygons))
            .arg(tileBaked ? "Yes" : "No")
            .arg(usableArea, 0, 'f', 1).arg(requested).arg(target)
            .arg(attempts).arg(generated).arg(outside).arg(feather).arg(spacing)
            .arg(terrain).arg(slope).arg(track).arg(road).arg(water).arg(tileEdge)
            .arg(osm).arg(bake).arg(placementTerrain).arg(placementFailed);
        report += QString("\nOutside target tile during placement: %1").arg(placementTileEdge);
        report += QString("\nGenerated candidates discarded by operation cap: %1").arg(limitSkipped);
        if(!reason.isEmpty())
            report += "\n\n" + reason;
        if(!generationEvaluated)
            report += "\n\nCandidate generation was not reached.";
        else if(target == 0)
            report += "\n\nThe boundary area and density/row settings produced a zero target.";
        report += "\n\nZero rejects can mean a later check was never reached. "
                  "An OSM image on the terrain does not establish planting-cache coverage.";
        return report;
    }
};

#endif

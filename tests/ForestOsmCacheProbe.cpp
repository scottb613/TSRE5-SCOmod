// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "ForestOsmCache.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

#include <cmath>
#include <iostream>

namespace {

bool check(bool condition, const char *message) {
    if(condition) return true;
    std::cerr << "FAILED: " << message << '\n';
    return false;
}

} // namespace

int main(int argc, char **argv) {
    if(argc == 2) {
        const ForestOsmCacheLoadResult routeResult =
            ForestOsmCache::loadFile(QString::fromLocal8Bit(argv[1]));
        bool passed = true;
        passed &= check(routeResult.isValid(), "real route cache should load");
        passed &= check(routeResult.schemaVersion == 2,
                        "real route cache schema version");
        passed &= check(routeResult.sourceFeatureCount > 0,
                        "real route cache should contain source features");
        passed &= check(!routeResult.polygons.isEmpty(),
                        "real route cache should contain polygon parts");
        for(const ForestOsmPolygon &polygon : routeResult.polygons) {
            passed &= check(!polygon.category.isEmpty(),
                            "every real route polygon needs a category");
            passed &= check(!polygon.featureId.isEmpty(),
                            "every real route polygon needs a feature id");
        }
        if(passed) {
            std::cout << "Real PolyVeg OSM cache passed: "
                      << routeResult.sourceFeatureCount << " features, "
                      << routeResult.polygons.size() << " polygon parts.\n";
        }
        return passed ? 0 : 1;
    }
    QTemporaryDir temporary;
    if(!temporary.isValid()) return 2;
    const QString path = temporary.filePath("polyveg-polygons.geojson");
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) return 2;
    QTextStream stream(&file);
    stream << R"JSON({
      "type":"FeatureCollection","schemaVersion":2,"features":[
        {"type":"Feature","id":"polygon-part","properties":{"category":"grassland","styleId":"natural=grassland","sourceId":"way/123","fillColor":"#C6E4B4","drawOrder":70,"natural":"grassland"},
         "geometry":{"type":"Polygon","coordinates":[
           [[-75.10,40.70],[-75.09,40.70],[-75.09,40.71],[-75.10,40.71],[-75.10,40.70]],
           [[-75.098,40.702],[-75.092,40.702],[-75.092,40.708],[-75.098,40.708],[-75.098,40.702]]]}},
        {"type":"Feature","id":"multi","properties":{"category":"woodland","landuse":"forest"},
         "geometry":{"type":"MultiPolygon","coordinates":[
           [[[-75.08,40.70],[-75.07,40.70],[-75.07,40.71],[-75.08,40.71],[-75.08,40.70]]],
           [[[-75.06,40.70],[-75.05,40.70],[-75.05,40.71],[-75.06,40.71],[-75.06,40.70]]]]}},
        {"type":"Feature","id":"collection","properties":{"category":"woodland","natural":"wood"},
         "geometry":{"type":"GeometryCollection","geometries":[
           {"type":"Polygon","coordinates":[[[-75.04,40.70],[-75.03,40.70],[-75.03,40.71],[-75.04,40.71],[-75.04,40.70]]]}]}},
        {"type":"Feature","id":"exclusion","properties":{"kind":"road"},
         "geometry":{"type":"MultiPolygon","coordinates":[
           [[[-75.02,40.70],[-75.01,40.70],[-75.01,40.71],[-75.02,40.71],[-75.02,40.70]]]]}}
      ]})JSON";
    file.close();

    const ForestOsmCacheLoadResult result = ForestOsmCache::loadFile(path);
    bool passed = true;
    passed &= check(result.isValid(), "schema-2 PolyVeg cache should load");
    passed &= check(result.sourceFeatureCount == 4, "source feature count");
    passed &= check(result.polygons.size() == 5,
                    "Polygon, MultiPolygon, GeometryCollection, and exclusion kind");
    passed &= check(result.polygons.first().boundary.holes.size() == 1,
                    "inner ring should be preserved");
    passed &= check(result.polygons.first().boundary.outer.size() == 4,
                    "duplicate GeoJSON closing point should be removed");
    passed &= check(result.polygons.first().category == "grassland",
                    "explicit PolyVeg category should be retained");
    passed &= check(result.polygons.first().featureId == "way/123",
                    "contract sourceId should be the stable feature identity");
    passed &= check(result.polygons.first().tags.value("styleid")
                        == "natural=grassland"
                    && result.polygons.first().tags.value("fillcolor")
                        == "#c6e4b4",
                    "contract style and fill metadata should be retained");
    passed &= check(result.polygons.first().drawOrder == 70,
                    "visible map draw order should be retained");
    passed &= check(result.polygons.at(1).category == "woodland",
                    "woodland category should be retained");
    passed &= check(result.polygons.last().category == "road"
                    && result.polygons.last().tags.value("kind") == "road",
                    "exclusion kind should be retained as its cache category");
    const ForestPlanPoint point = result.polygons.first().boundary.outer.first();
    passed &= check(std::isfinite(point.x) && std::isfinite(point.z),
                    "projected plan coordinate should be finite");
    passed &= check(point.x > -23000000.0 && point.x < -22000000.0
                    && point.z > -30000000.0 && point.z < -28000000.0,
                    "projected point should lie in the TOPOTEST MSTS region");

    QJsonObject obsoleteRoot;
    QFile source(path);
    if(source.open(QIODevice::ReadOnly))
        obsoleteRoot = QJsonDocument::fromJson(source.readAll()).object();
    obsoleteRoot.insert("schemaVersion", 1);
    const QString obsoletePath = temporary.filePath("polyveg-schema1.geojson");
    QFile obsoleteFile(obsoletePath);
    if(obsoleteFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        obsoleteFile.write(QJsonDocument(obsoleteRoot).toJson());
        obsoleteFile.close();
        const ForestOsmCacheLoadResult obsolete =
            ForestOsmCache::loadFile(obsoletePath);
        passed &= check(!obsolete.isValid()
                && obsolete.errors.join(' ').contains("schemaVersion 2"),
            "obsolete schema-1 PolyVeg cache should be rejected clearly");
    } else {
        passed &= check(false, "unable to create obsolete schema fixture");
    }
    if(passed) std::cout << "Forest OSM cache probe passed.\n";
    return passed ? 0 : 1;
}

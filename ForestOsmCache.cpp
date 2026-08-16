#include "ForestOsmCache.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>
#include <cmath>

namespace {

constexpr double EarthRadiusMetres = 6370997.0;
constexpr double UpperLeftGoodeX = -20013976.0;
constexpr double UpperLeftGoodeY = 8674024.0;
constexpr double TileSizeMetres = 2048.0;
constexpr double TileOrigin = 16384.0;
constexpr double Parallel41 = (40.0 + 44.0/60.0 + 11.8/3600.0) * M_PI/180.0;
constexpr double Epsilon = 0.0000001;

struct CachedForestOsmFile {
    qint64 size = -1;
    QDateTime modified;
    ForestOsmCacheLoadResult result;
};

QHash<QString, CachedForestOsmFile> loadedForestOsmFiles;

constexpr double LongitudeCenters[12] {
    -100.0*M_PI/180.0, -100.0*M_PI/180.0,
      30.0*M_PI/180.0,   30.0*M_PI/180.0,
    -160.0*M_PI/180.0,  -60.0*M_PI/180.0,
    -160.0*M_PI/180.0,  -60.0*M_PI/180.0,
      20.0*M_PI/180.0,  140.0*M_PI/180.0,
      20.0*M_PI/180.0,  140.0*M_PI/180.0
};

double adjustedLongitude(double value) {
    if(value >= M_PI) return value - 2.0*M_PI;
    if(value < -M_PI) return value + 2.0*M_PI;
    return value;
}

int regionFor(double latitude, double longitude) {
    if(latitude >= Parallel41)
        return longitude <= -40.0*M_PI/180.0 ? 0 : 2;
    if(latitude >= 0.0)
        return longitude <= -40.0*M_PI/180.0 ? 1 : 3;
    if(latitude >= -Parallel41) {
        if(longitude <= -100.0*M_PI/180.0) return 4;
        if(longitude <= -20.0*M_PI/180.0) return 5;
        return longitude <= 80.0*M_PI/180.0 ? 8 : 9;
    }
    if(longitude <= -100.0*M_PI/180.0) return 6;
    if(longitude <= -20.0*M_PI/180.0) return 7;
    return longitude <= 80.0*M_PI/180.0 ? 10 : 11;
}

bool lonLatToPlan(double longitudeDegrees, double latitudeDegrees,
                  ForestPlanPoint &point) {
    if(!std::isfinite(longitudeDegrees) || !std::isfinite(latitudeDegrees)
            || longitudeDegrees < -180.0 || longitudeDegrees > 180.0
            || latitudeDegrees < -90.0 || latitudeDegrees > 90.0)
        return false;

    const double latitude = latitudeDegrees*M_PI/180.0;
    const double longitude = longitudeDegrees*M_PI/180.0;
    const int region = regionFor(latitude, longitude);
    const double deltaLongitude = adjustedLongitude(
        longitude - LongitudeCenters[region]);
    double projectedX = 0.0;
    double projectedY = 0.0;
    if(region == 1 || region == 3 || region == 4 || region == 5
            || region == 8 || region == 9) {
        projectedY = latitude;
        projectedX = LongitudeCenters[region]
            + deltaLongitude*std::cos(latitude);
    } else {
        double theta = latitude;
        const double target = M_PI*std::sin(latitude);
        for(int iteration = 0; iteration < 31; ++iteration) {
            const double denominator = 1.0 + std::cos(theta);
            if(std::fabs(denominator) <= Epsilon) return false;
            const double delta = -(theta + std::sin(theta) - target)
                / denominator;
            theta += delta;
            if(std::fabs(delta) < 0.00000000001) break;
            if(iteration == 30) return false;
        }
        theta *= 0.5;
        projectedY = 1.4142135623731*std::sin(theta)
            - 0.0528035274542*(latitude < 0.0 ? -1.0 : 1.0);
        projectedX = LongitudeCenters[region]
            + 0.900316316158*deltaLongitude*std::cos(theta);
    }

    // The plan coordinate is the continuous metre coordinate used by the
    // generator.  Converting it to a TSRE tile later is simply floor((m+1024)/2048).
    point.x = projectedX*EarthRadiusMetres - UpperLeftGoodeX
        - TileOrigin*TileSizeMetres;
    point.z = -(projectedY*EarthRadiusMetres - UpperLeftGoodeY
        + TileOrigin*TileSizeMetres);
    return std::isfinite(point.x) && std::isfinite(point.z);
}

bool parseRing(const QJsonValue &value, ForestPlanRing &ring) {
    const QJsonArray coordinates = value.toArray();
    if(coordinates.size() < 4) return false;
    for(const QJsonValue &coordinateValue : coordinates) {
        const QJsonArray coordinate = coordinateValue.toArray();
        if(coordinate.size() < 2 || !coordinate.at(0).isDouble()
                || !coordinate.at(1).isDouble())
            return false;
        ForestPlanPoint point;
        if(!lonLatToPlan(coordinate.at(0).toDouble(),
                         coordinate.at(1).toDouble(), point))
            return false;
        ring.append(point);
    }
    if(ring.size() >= 2) {
        const ForestPlanPoint &first = ring.first();
        const ForestPlanPoint &last = ring.last();
        if(std::fabs(first.x-last.x) <= Epsilon
                && std::fabs(first.z-last.z) <= Epsilon)
            ring.removeLast();
    }
    return ring.size() >= 3;
}

void appendPolygon(const QJsonArray &rings, const QString &featureId,
                   const QString &category, int drawOrder,
                   const QHash<QString, QString> &tags,
                   ForestOsmCacheLoadResult &result) {
    if(rings.isEmpty()) return;
    ForestOsmPolygon polygon;
    polygon.featureId = featureId;
    polygon.category = category;
    polygon.drawOrder = drawOrder;
    polygon.tags = tags;
    if(!parseRing(rings.at(0), polygon.boundary.outer)) {
        result.warnings.append("Feature " + featureId
            + ": skipped polygon with an invalid outer ring.");
        return;
    }
    for(int ringIndex = 1; ringIndex < rings.size(); ++ringIndex) {
        ForestPlanRing hole;
        if(parseRing(rings.at(ringIndex), hole))
            polygon.boundary.holes.append(hole);
        else
            result.warnings.append("Feature " + featureId
                + ": skipped an invalid inner ring.");
    }
    polygon.minimumX = polygon.maximumX = polygon.boundary.outer.first().x;
    polygon.minimumZ = polygon.maximumZ = polygon.boundary.outer.first().z;
    for(const ForestPlanPoint &point : polygon.boundary.outer) {
        polygon.minimumX = std::min(polygon.minimumX, point.x);
        polygon.maximumX = std::max(polygon.maximumX, point.x);
        polygon.minimumZ = std::min(polygon.minimumZ, point.z);
        polygon.maximumZ = std::max(polygon.maximumZ, point.z);
    }
    result.polygons.append(polygon);
}

void appendGeometry(const QJsonObject &geometry, const QString &featureId,
                    const QString &category, int drawOrder,
                    const QHash<QString, QString> &tags,
                    ForestOsmCacheLoadResult &result) {
    const QString type = geometry.value("type").toString();
    if(type == "Polygon") {
        appendPolygon(geometry.value("coordinates").toArray(),
                      featureId, category, drawOrder, tags, result);
    } else if(type == "MultiPolygon") {
        for(const QJsonValue &polygon : geometry.value("coordinates").toArray())
            appendPolygon(polygon.toArray(), featureId, category, drawOrder,
                          tags, result);
    } else if(type == "GeometryCollection") {
        for(const QJsonValue &child : geometry.value("geometries").toArray())
            appendGeometry(child.toObject(), featureId, category, drawOrder,
                           tags, result);
    } else {
        result.warnings.append("Feature " + featureId
            + ": ignored unsupported geometry type '" + type + "'.");
    }
}

} // namespace

ForestOsmCacheLoadResult ForestOsmCache::loadRoute(const QString &routePath) {
    return loadFile(routePath + "/osm_data/polyveg-polygons.geojson");
}

ForestOsmCacheLoadResult ForestOsmCache::loadFile(const QString &filePath) {
    const QFileInfo fileInfo(filePath);
    const QString absolutePath = fileInfo.absoluteFilePath();
    const auto cached = loadedForestOsmFiles.constFind(absolutePath);
    if(fileInfo.exists() && cached != loadedForestOsmFiles.constEnd()
            && cached->size == fileInfo.size()
            && cached->modified == fileInfo.lastModified())
        return cached->result;

    ForestOsmCacheLoadResult result;
    result.filePath = absolutePath;
    QFile file(absolutePath);
    if(!file.open(QIODevice::ReadOnly)) {
        loadedForestOsmFiles.remove(absolutePath);
        result.errors.append(
            "No PolyVeg OSM data is available for this route.\n\n"
            "Run SCO LIDEX and use Create Map Tiles for this route to retrieve "
            "and prepare the route OSM data. SCO LIDEX must generate "
            "osm_data/polyveg-polygons.geojson.");
        return result;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.errors.append(QString("PolyVeg polygon cache JSON error at %1: %2")
            .arg(parseError.offset).arg(parseError.errorString()));
        return result;
    }
    const QJsonObject root = document.object();
    result.schemaVersion = root.value("schemaVersion").toInt();
    if(root.value("type").toString() != "FeatureCollection"
            || result.schemaVersion != 2) {
        result.errors.append(
            "PolyVeg polygon cache must be a schemaVersion 2 FeatureCollection.");
        return result;
    }
    const QJsonArray features = root.value("features").toArray();
    result.sourceFeatureCount = features.size();
    for(int index = 0; index < features.size(); ++index) {
        const QJsonObject feature = features.at(index).toObject();
        QString featureId = feature.value("id").toVariant().toString();
        QHash<QString, QString> tags;
        const QJsonObject properties = feature.value("properties").toObject();
        const QString sourceId = properties.value("sourceId").toString().trimmed();
        if(!sourceId.isEmpty()) featureId = sourceId;
        if(featureId.isEmpty()) featureId = QString::number(index);
        for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
            const QString value = it.value().toString().trimmed().toLower();
            if(!value.isEmpty()) tags.insert(it.key().toLower(), value);
        }
        QString category = properties.value("category").toString()
            .trimmed().toLower();
        if(category.isEmpty())
            category = properties.value("kind").toString()
                .trimmed().toLower();
        if(category.isEmpty()) {
            result.warnings.append("Feature " + featureId
                + ": skipped because its PolyVeg category/kind is missing.");
            continue;
        }
        const int drawOrder = properties.value("drawOrder").toInt(0);
        tags.insert(QStringLiteral("category"), category);
        if(properties.contains("kind"))
            tags.insert(QStringLiteral("kind"), category);
        appendGeometry(feature.value("geometry").toObject(),
                       featureId, category, drawOrder, tags, result);
    }
    if(result.polygons.isEmpty())
        result.errors.append("PolyVeg polygon cache contains no usable polygons.");
    if(result.isValid()) {
        CachedForestOsmFile entry;
        entry.size = fileInfo.size();
        entry.modified = fileInfo.lastModified();
        entry.result = result;
        loadedForestOsmFiles.insert(absolutePath, entry);
    }
    return result;
}

// Full-route diagnostics. No track placement, repair, deletion or saving.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Route.h"
#include "TrackOverlapScan.h"
#include "RejectedWorldFile.h"

#include "DynTrackObj.h"
#include "ErrorMessage.h"
#include "ErrorMessagesLib.h"
#include "Game.h"
#include "GeoCoordinates.h"
#include "GLMatrix.h"
#include "GuiFunct.h"
#include "TSectionDAT.h"
#include "TRnode.h"
#include "Tile.h"
#include "Vector3f.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <memory>
#include <set>
#include <tuple>

namespace {
using namespace TrackOverlapScan;
using ObjectKey = std::tuple<int,int,unsigned int>;
struct MessageDeleter {
    void operator()(ErrorMessage* message) const {
        if(message != nullptr) { delete message->coords; delete message; }
    }
};
using Message = std::unique_ptr<ErrorMessage,MessageDeleter>;
constexpr const char* scanPrefix = "[Route scan] ";

bool integer(float value, int& out) {
    if(!std::isfinite(value) || double(value) < std::numeric_limits<int>::min()
            || double(value) > std::numeric_limits<int>::max() || std::trunc(value) != value)
        return false;
    out = static_cast<int>(value);
    return true;
}
bool objectKey(const float* p, ObjectKey& key) {
    int x,z;
    if(!integer(p[0],x) || !integer(p[1],z) || !std::isfinite(p[2])
            || p[2] < 0 || double(p[2]) > std::numeric_limits<unsigned int>::max()
            || std::trunc(p[2]) != p[2]) return false;
    key = {x,z,static_cast<unsigned int>(p[2])};
    return true;
}
bool validSection(TSection* s) {
    if(s == nullptr) return false;
    // Straight sections do not initialize radius/angle; never read those fields.
    return s->type == 0 ? validSubsection(0,s->size,0,0)
        : s->type == 1 && validSubsection(1,0,s->radius,s->angle);
}
TSection* sectionAt(TSectionDAT* database, float id) {
    int index;
    if(database == nullptr || !integer(id,index)) return nullptr;
    const auto it = database->sekcja.find(index);
    return it == database->sekcja.end() ? nullptr : it->second;
}
// World shapes use the same path transform as the grade marker/rail renderer.
// Transform locally as floats, then add tile origins in double precision.
bool samplePath(WorldObj* obj, const QVector<TSection>& sections,
                const TrackShape::SectionIdx* path, std::vector<Point>& points) {
    if(sections.isEmpty()) return false;
    double total = 0;
    QVector<TSection> usable;
    for(TSection s : sections) {
        if(!validSection(&s)) return false;
        if(s.getDlugosc() == 0) continue; // dynamic track may begin directly with a curve
        total += s.getDlugosc();
        usable.push_back(s);
    }
    if(total < 0.01 || total > 100000) return false;

    float rotation[4], transform[16], origin[3] = {0,0,0};
    Quat::fill(rotation);
    if(path != nullptr) {
        origin[0] = -path->pos[0]; origin[1] = path->pos[1]; origin[2] = path->pos[2];
        Quat::rotateY(rotation,rotation,-path->rotDeg * 3.14159265358979323846 / 180.0);
    }
    Mat4::fromRotationTranslation(transform,rotation,origin);
    const int count = std::max(1,static_cast<int>(std::ceil(total)));
    points.reserve(count+1);
    for(int i = 0; i <= count; ++i) {
        float p[6] = {};
        // Same subsection walk as ComplexLine, without its rendering hash
        // (large-radius sections can overflow the legacy integer hash).
        float remaining = float(total * (double(i)/count));
        float heading = 0;
        Vector3f offset(0,0,0), local;
        for(TSection section : usable) {
            const float len = section.getDlugosc();
            const float distance = std::min(remaining,len);
            section.getDrawPosition(&local,distance);
            local.rotateY(heading,0);
            offset.x += local.x; offset.y += local.y; offset.z += local.z;
            if(remaining <= len) break;
            remaining -= len;
            heading += section.getDrawAngle(len);
        }
        p[0] = offset.x; p[1] = offset.y; p[2] = offset.z;
        Vec3::transformMat4(p,p,transform);
        Vec3::transformMat4(p,p,obj->matrix);
        const Point result{double(obj->x)*2048+p[0],p[1],double(obj->y)*2048+p[2]};
        if(!finite(result)) return false;
        points.push_back(result);
    }
    return true;
}
struct Track {
    WorldObj* object = nullptr;
    std::set<int> components;
    bool member = false;
    bool suspect = false;
    bool standalone = false;
    bool geometryComplete = true;
    double isolatedMetres = 0;
};
struct Endpoint { Point position; bool valid = false; };
struct NodeGeometry { Endpoint start,end; };

// TDB coordinates here are MSTS coordinates (external Z), matching UiD and
// Jump. Mirror the transform used by TDB::getDrawPositionOnTrNode, independently
// for each subsection so an internal gap cannot be hidden by distance lookup.
bool sectionPoint(const float* p, TSection* section, float metres, Point& point) {
    if(!validSection(section)) return false;
    for(int i = 8; i < 16; ++i) if(!std::isfinite(p[i])) return false;
    Vector3f local;
    section->getDrawPosition(&local,metres);
    float rotation[3] = {3.14159265358979323846f,p[14],0};
    float q[4], matrix[16], origin[3] = {p[10],p[11],p[12]};
    Quat::fromRotationXYZ(q,rotation);
    Mat4::fromRotationTranslation(matrix,q,origin);
    Mat4::rotate(matrix,matrix,-p[13],1,0,0);
    Mat4::rotate(matrix,matrix,-p[15],0,0,1);
    float v[3] = {local.x,local.y,-local.z};
    Vec3::transformMat4(v,v,matrix);
    point = {double(p[8])*2048+v[0],v[1],double(p[9])*2048+v[2]};
    return finite(point);
}
Point uidPoint(const TRnode* n) {
    return {double(n->UiD[4])*2048+n->UiD[6],n->UiD[7],double(n->UiD[5])*2048+n->UiD[8]};
}
void locate(ErrorMessage* message, Point p) {
    if(!finite(p)) return;
    const double tx = std::floor((p.x+1024)/2048), tz = std::floor((p.z+1024)/2048);
    message->setLocationXYZ(tx,tz,p.x-tx*2048,p.y,p.z-tz*2048);
}
Message warning(const QString& description, const QString& detail, Point location) {
    Message message(new ErrorMessage(ErrorMessage::Type_Warning,ErrorMessage::Source_TDB,
        QString::fromLatin1(scanPrefix)+description,detail));
    locate(message.get(),location);
    return message;
}

QString gapText(Point a, Point b) {
    return QString("Horizontal gap: %1 m; height difference: %2 m. Inspect this joint before repairing it.")
        .arg(std::hypot(a.x-b.x,a.z-b.z),0,'f',3).arg(std::abs(a.y-b.y),0,'f',3);
}
}

QString Route::scanAllWorldTiles(double maxOrphanLength, QWidget* parent,
                                double horizontalTolerance, double verticalTolerance) {
    using namespace TrackOverlapScan;
    if(!std::isfinite(horizontalTolerance) || !std::isfinite(verticalTolerance)
            || horizontalTolerance < 0.01 || horizontalTolerance > 5.0
            || verticalTolerance < 0.01 || verticalTolerance > 5.0)
        return tr("Joint-gap thresholds must be between 0.01 and 5.00 m.");
    if(!loaded || trackDB == nullptr || !trackDB->loaded || tsection == nullptr)
        return tr("Open a route before scanning.");
    if(Game::serverClient != nullptr)
        return tr("Full-route scanning requires a local route; remote tiles are not all available here.");
    if(!std::isfinite(maxOrphanLength) || maxOrphanLength < 1 || maxOrphanLength > 1000)
        return tr("Orphan length must be between 1 and 1000 m.");

    // Tile loading has legacy repair hooks. Suppress every configured repair
    // used by that path, and restore the user's settings even on cancellation.
    QScopedValueRollback<bool> autoFix(Game::autoFix,false);
    QScopedValueRollback<bool> positiveQuaternions(Game::useOnlyPositiveQuaternions,false);
    QScopedValueRollback<QStringList> removeObjects(Game::objectsToRemove,QStringList());
    QScopedValueRollback<bool> allWorld(Game::loadAllWFiles,false);
    QScopedValueRollback<bool> listFiles(Game::listFiles,false);

    QProgressDialog progress(tr("Loading world tiles..."),tr("Cancel"),0,1000,parent);
    progress.setWindowTitle(Game::AppName);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    GuiFunct::styleEditorDialog(&progress);
    progress.show();
    QElapsedTimer pulse;
    pulse.start();
    auto update = [&](const QString& text, int value, bool force = false) {
        if(force || pulse.elapsed() >= 40) {
            progress.setLabelText(text);
            progress.setValue(value);
            QApplication::processEvents(QEventLoop::AllEvents,20);
            pulse.restart();
        }
        return !progress.wasCanceled();
    };
    const auto cancelled = [&]() {
        return tr("Scan cancelled. Coverage is incomplete; loaded tiles remain available. Previous scan results were not replaced.");
    };
    const QDir world(Game::root+"/routes/"+Game::route+"/world");
    if(!world.exists()) return tr("Scan failed: the route's world directory is unavailable.");
    const QStringList files = world.entryList(QStringList()<<"*.w",QDir::Files,QDir::Name);
    if(files.isEmpty()) return tr("Scan failed: no world files were found.");
    const QRegularExpression filename("^w([+-][0-9]{6})([+-][0-9]{6})\\.w$",QRegularExpression::CaseInsensitiveOption);
    int failures = 0, loadedFiles = 0;
    std::vector<Message> rejectedFiles;
    for(int i = 0; i < files.size(); ++i) {
        if(!update(tr("Loading world tiles: %1 / %2").arg(i).arg(files.size()),i*200/files.size())) return cancelled();
        const auto match = filename.match(files[i]);
        bool ok = match.hasMatch();
        if(ok) {
            const int x = match.captured(1).toInt(), z = -match.captured(2).toInt();
            const qint64 wideKey = qint64(x)*10000+z;
            ok = wideKey >= std::numeric_limits<int>::min() && wideKey <= std::numeric_limits<int>::max();
            if(ok) {
                const int key = int(wideKey);
                Tile* current = tile.value(key,nullptr);
                if(current == nullptr) { current = new Tile(x,z); tile.insert(key,current); }
                // Never replace an existing in-memory tile (including edits).
                ok = current->x == x && current->z == z && current->loaded == 1;
            }
        }
        if(ok) ++loadedFiles;
        else {
            ++failures;
            Message rejected(new ErrorMessage(ErrorMessage::Type_Warning, ErrorMessage::Source_World,
                QString::fromLatin1(scanPrefix) + tr("Rejected world file: %1").arg(files[i]),
                match.hasMatch()
                    ? tr("The tile could not be loaded safely. It was skipped; coverage is incomplete. Investigate this file before rescanning.")
                    : tr("Unsupported world-tile filename. The file was skipped. After the scan, Delete can remove it from the active world list while preserving the original as a recovery .bak file. Nothing has been deleted.")));
            if(!match.hasMatch()){
                const QFileInfo file(world.filePath(files[i]));
                if(file.isFile() && !file.isSymLink()){
                    rejected->rejectedWorldRoot = world.canonicalPath();
                    rejected->rejectedWorldName = files[i];
                    rejected->rejectedWorldHash = RejectedWorldFile::digest(file.absoluteFilePath());
                }
            }
            rejectedFiles.push_back(std::move(rejected));
        }
    }

    // Refresh normal diagnostics for previously loaded tiles too. New tiles
    // were checked by Tile::load; repeated legacy messages remain in the log.
    const auto loadedTiles = tile.values();
    for(int i = 0; i < loadedTiles.size(); ++i) {
        if(!update(tr("Checking world objects..."),200+i*100/std::max(1,int(loadedTiles.size())))) return cancelled();
        Tile* current = loadedTiles[i];
        if(current != nullptr && current->loaded == 1) current->checkForErrors();
    }
    Game::loadAllWFiles = failures == 0;
    if(!update(tr("Checking route databases..."),300,true)) return cancelled();
    if(failures == 0) checkRouteDatabase();
    if(progress.wasCanceled()) return cancelled();

    // Event processing may let rendering populate sparse database maps.
    // Iterate a stable pointer snapshot while the modal dialog blocks editing.
    const auto databaseNodes = trackDB->trackNodes;
    std::vector<Node> nodes;
    std::map<ObjectKey,std::set<int>> membership;
    std::unordered_map<int,NodeGeometry> nodeGeometry;
    std::vector<Message> findings;
    auto addGap = [&](const QString& title, Point a, Point b) {
        findings.push_back(warning(title,gapText(a,b)
            + tr(" Thresholds: %1 m horizontal / %2 m vertical.")
                .arg(horizontalTolerance,0,'f',2).arg(verticalTolerance,0,'f',2),a));
    };
    int processed = 0, invalidGeometry = 0;
    for(const auto& entry : databaseNodes) {
        if(!update(tr("Checking TDB joints and connectivity..."),320+(processed++)*140/std::max(1,int(databaseNodes.size())))) return cancelled();
        TRnode* n = entry.second;
        if(n == nullptr || n->typ == -1) continue;
        Node node;
        node.id = entry.first; node.type = n->typ;
        const int pinCount = n->TrP1+n->TrP2;
        node.valid = n->TrP1 >= 0 && n->TrP2 >= 0 && pinCount >= 0 && pinCount <= 3;
        for(int pin = 0; pin < std::clamp(pinCount,0,3); ++pin) node.links.push_back(n->TrPinS[pin]);
        if(n->typ == 1) {
            node.valid = node.valid && n->iTrv > 0 && n->trVectorSection != nullptr;
            Point previousEnd{};
            bool previousValid = false;
            for(int j = 0; n->trVectorSection != nullptr && j < n->iTrv; ++j) {
                const float* p = n->trVectorSection[j].param;
                ObjectKey key;
                if(objectKey(p+2,key)) membership[key].insert(node.id);
                else node.valid = false;
                TSection* section = sectionAt(tsection,p[0]);
                Point a,b;
                const bool valid = validSection(section)
                    && sectionPoint(p,section,0,a)
                    && sectionPoint(p,section,section->getDlugosc(),b);
                if(!valid) {
                    node.valid = false; previousValid = false; ++invalidGeometry;
                    const Point location{double(p[8])*2048+p[10],p[11],double(p[9])*2048+p[12]};
                    findings.push_back(warning(tr("Uncheckable joint geometry: TDB node %1, section %2").arg(node.id).arg(j),
                        tr("This subsection has missing or invalid geometry. Its adjacent joints could not be verified."),location));
                    continue;
                }
                const float len = section->getDlugosc();
                node.metres += len;
                if(previousValid && jointSeparated(previousEnd,a,horizontalTolerance,verticalTolerance))
                    addGap(tr("Bad internal joint: TDB node %1, sections %2 / %3").arg(node.id).arg(j-1).arg(j),previousEnd,a);
                // A zero-length first/last subsection still defines the actual
                // endpoint location. Do not skip it or invent a broken joint.
                if(j == 0) nodeGeometry[node.id].start = {a,true};
                if(j == n->iTrv-1) nodeGeometry[node.id].end = {b,true};
                previousEnd = b; previousValid = true;
            }
        } else if(n->typ == 2) {
            ObjectKey key;
            if(objectKey(n->UiD,key)) membership[key].insert(node.id);
            else node.valid = false;
        }
        nodes.push_back(node);
    }
    const Connectivity connectivity = classify(nodes);
    std::unordered_map<int,std::set<ObjectKey>> componentOwners;
    for(const auto& entry : membership)
        for(int id : entry.second) {
            const auto component = connectivity.componentOf.find(id);
            if(component != connectivity.componentOf.end())
                componentOwners[component->second].insert(entry.first);
        }
    // Verify each vector endpoint against its linked endpoint/junction. For
    // vector-to-vector links use the reciprocal pin, not the nearest end.
    for(const Node& node : nodes) {
        if(!update(tr("Checking linked TDB joints..."),465)) return cancelled();
        TRnode* n = databaseNodes.at(node.id);
        const auto geometry = nodeGeometry.find(node.id);
        Point fallback = uidPoint(n);
        if(n->typ == 1) {
            const double unknown = std::numeric_limits<double>::quiet_NaN();
            fallback = {unknown,unknown,unknown};
            if(geometry != nodeGeometry.end() && geometry->second.start.valid)
                fallback = geometry->second.start.position;
            else if(n->iTrv > 0 && n->trVectorSection != nullptr) {
                const float* p = n->trVectorSection[0].param;
                fallback = {double(p[8])*2048+p[10],p[11],double(p[9])*2048+p[12]};
            }
        }
        const int expectedPins = node.type == 0 ? 1 : node.type == 1 ? 2 : node.type == 2 ? 3 : -1;
        if(int(node.links.size()) != expectedPins)
            findings.push_back(warning(tr("Bad pin count: TDB node %1").arg(node.id),tr("The node has an invalid number of links."),fallback));
        for(int pin = 0; pin < int(node.links.size()); ++pin) {
            const int targetId = node.links[pin];
            const auto targetIt = databaseNodes.find(targetId);
            Endpoint endpoint;
            if(node.type == 1 && geometry != nodeGeometry.end()) endpoint = pin == 0 ? geometry->second.start : geometry->second.end;
            Point location = endpoint.valid ? endpoint.position : fallback;
            if(targetId <= 0 || targetId == node.id || targetIt == databaseNodes.end()
                    || targetIt->second == nullptr || targetIt->second->typ == -1) {
                findings.push_back(warning(tr("Broken TDB joint: node %1, pin %2 -> %3").arg(node.id).arg(pin).arg(targetId),
                    tr("This pin does not reference a live, different TDB node."),location));
                continue;
            }
            TRnode* target = targetIt->second;
            std::vector<int> targetLinks, targetDirections;
            for(int j = 0; j < std::clamp(target->TrP1+target->TrP2,0,3); ++j) {
                targetLinks.push_back(target->TrPinS[j]);
                targetDirections.push_back(target->TrPinK[j]);
            }
            const int back = reciprocalPin(node.id,node.type,pin,n->TrPinK[pin],
                target->typ,targetLinks,targetDirections);
            if(back < 0) {
                findings.push_back(warning(tr("Non-reciprocal TDB joint: nodes %1 / %2").arg(node.id).arg(targetId),
                    tr("The return link is missing or references the wrong vector endpoint."),location));
                continue;
            }
            if(node.type != 1 || !endpoint.valid) continue;
            Point other;
            if(target->typ == 1) {
                if(node.id > targetId) continue;
                const auto otherGeometry = nodeGeometry.find(targetId);
                if(otherGeometry == nodeGeometry.end()) continue;
                const Endpoint& opposite = back == 0 ? otherGeometry->second.start : otherGeometry->second.end;
                if(!opposite.valid) continue;
                other = opposite.position;
            } else { other = uidPoint(target); if(!finite(other)) continue; }
            if(linkedJointSeparated(target->typ,location,other,horizontalTolerance,verticalTolerance))
                addGap(tr("Bad linked joint: TDB nodes %1 / %2").arg(node.id).arg(targetId),location,other);
        }
    }
    const int jointCount = int(findings.size());

    std::vector<Track> tracks;
    for(Tile* current : loadedTiles) {
        if(current == nullptr || current->loaded != 1) continue;
        for(const auto& item : current->obiekty) {
            WorldObj* obj = item.second;
            if(obj == nullptr || !obj->loaded || (obj->typeID != WorldObj::trackobj && obj->typeID != WorldObj::dyntrack)) continue;
            const auto shapeIt = tsection->shape.find(obj->sectionIdx);
            if(shapeIt != tsection->shape.end() && shapeIt->second != nullptr && shapeIt->second->roadshape) continue;
            Track track; track.object = obj;
            const auto member = membership.find({obj->x,-obj->y,obj->UiD});
            track.member = member != membership.end();
            track.suspect = !track.member;
            if(track.member) {
                track.suspect = true;
                for(int id : member->second) {
                    const auto component = connectivity.componentOf.find(id);
                    if(component == connectivity.componentOf.end()) { track.suspect = false; continue; }
                    track.components.insert(component->second);
                }
                for(int id : track.components) {
                    const Component& component = connectivity.components[id];
                    track.suspect = track.suspect && component.shortIsolated(maxOrphanLength);
                    track.isolatedMetres += component.metres;
                }
            }
            if(track.member && !track.components.empty()) {
                track.standalone = true;
                for(int id : track.components)
                    track.standalone = track.standalone
                        && connectivity.components[id].singlePiece(componentOwners[id].size());
            }
            tracks.push_back(track);
        }
    }
    Geometry geometry;
    int skippedPaths = 0;
    for(int i = 0; i < int(tracks.size()); ++i) {
        if(!update(tr("Sampling track geometry: %1 / %2").arg(i).arg(tracks.size()),500+i*200/std::max(1,int(tracks.size())))) return cancelled();
        WorldObj* obj = tracks[i].object;
        if(obj->typeID == WorldObj::dyntrack) {
            DynTrackObj* dynamic = static_cast<DynTrackObj*>(obj);
            QVector<TSection> sections;
            if(dynamic->sections != nullptr)
                for(int j = 0; j < 5; ++j) {
                    const auto& s = dynamic->sections[j];
                    if(s.sectIdx > 100000000) continue;
                    sections.push_back(TSection(0,s.type,s.a,s.r));
                }
            std::vector<Point> points;
            if(!samplePath(obj,sections,nullptr,points) || !geometry.addPath(i,points)) { ++skippedPaths; tracks[i].geometryComplete = false; }
        } else {
            const auto shapeIt = tsection->shape.find(obj->sectionIdx);
            TrackShape* shape = shapeIt == tsection->shape.end() ? nullptr : shapeIt->second;
            if(shape == nullptr || shape->path == nullptr || shape->numpaths <= 0 || shape->numpaths > 256) { ++skippedPaths; tracks[i].geometryComplete = false; continue; }
            for(int j = 0; j < shape->numpaths; ++j) {
                const auto& path = shape->path[j];
                if(path.n < 1 || path.n > 12) { ++skippedPaths; tracks[i].geometryComplete = false; continue; }
                QVector<TSection> sections;
                for(int s = 0; s < path.n; ++s) {
                    const auto section = tsection->sekcja.find(path.sect[s]);
                    if(section == tsection->sekcja.end() || !validSection(section->second)) { sections.clear(); break; }
                    sections.push_back(*section->second);
                }
                std::vector<Point> points;
                if(!samplePath(obj,sections,&path,points) || !geometry.addPath(i,points)) { ++skippedPaths; tracks[i].geometryComplete = false; }
            }
        }
    }
    std::map<int,Match> matches;
    const auto eligible = [&](int a, int b) {
        if(!tracks[a].suspect) return false;
        for(int component : tracks[a].components)
            if(tracks[b].components.count(component)) return false;
        return true;
    };
    if(!geometry.find(eligible,[&](std::size_t done,std::size_t total) {
        return update(tr("Checking stacked track: %1 / %2 samples").arg(done).arg(total),700+int(done*290/std::max(std::size_t(1),total)),true);
    },matches,[&](int owner) { return tracks[owner].suspect; })) return cancelled();
    for(const auto& entry : matches) {
        const Track& track = tracks[entry.first];
        const Track& other = tracks[entry.second.other];
        WorldObj* obj = track.object;
        const QString reason = !track.member ? tr("No TDB entry")
            : tr("Short isolated TDB component (%1 m)").arg(track.isolatedMetres,0,'f',2);
        const QString title = tr("Stacked track: tile %1 %2, UID %3 - %4")
            .arg(obj->x).arg(-obj->y).arg(obj->UiD).arg(reason);
        const QString detail = tr("%1\nShape: %2\nOverlaps tile %3 %4, UID %5 (%6).\n"
            "Shared alignment: %7 m (%8% of the shorter path).\n"
            "Orphan limit: %9 m; centre-line tolerance: 0.25 m horizontally / 0.20 m vertically.\n"
            "Inspect both pieces. Intentional overlays connected to the network are excluded; no track has been removed.")
            .arg(reason).arg(obj->fileName).arg(other.object->x).arg(-other.object->y)
            .arg(other.object->UiD).arg(other.object->fileName).arg(entry.second.metres,0,'f',2)
            .arg(entry.second.fraction*100,0,'f',0).arg(maxOrphanLength,0,'f',0);
        Message message(new ErrorMessage(ErrorMessage::Type_Warning,ErrorMessage::Source_World,
            QString::fromLatin1(scanPrefix)+title,detail));
        message->setObject(obj);
        message->setLocationXYZ(obj->x,-obj->y,obj->position[0],obj->position[1],-obj->position[2]);
        findings.push_back(std::move(message));
    }
    std::set<int> connectedEnds;
    if(!geometry.endpointConnections(connectedEnds,[&](std::size_t done,std::size_t total) {
        return update(tr("Checking standalone track pieces: %1 / %2 paths").arg(done).arg(total),995,true);
    })) return cancelled();
    int standaloneCount = 0;
    for(int i = 0; i < int(tracks.size()); ++i) {
        const Track& track = tracks[i];
        const bool isolated = track.member ? track.standalone
            : track.geometryComplete && !connectedEnds.count(i);
        if(failures != 0 || !isolated || matches.count(i)) continue;
        ++standaloneCount;
        WorldObj* obj = track.object;
        const QString reason = track.member
            ? tr("Its entire TDB component contains this track object only; it has no connection to any other track piece.")
            : tr("It has no TDB entry and none of its path endpoints meets another track piece (0.25 m horizontal / 0.10 m vertical tolerance).");
        Message message(new ErrorMessage(ErrorMessage::Type_Warning,ErrorMessage::Source_World,
            QString::fromLatin1(scanPrefix)+tr("Standalone track: tile %1 %2, UID %3%4")
                .arg(obj->x).arg(-obj->y).arg(obj->UiD).arg(track.member ? QString() : tr(" - no TDB entry")),
            tr("Shape: %1\n%2\nThis check has no length limit and does not require overlap. Inspect the piece; nothing has been removed.")
                .arg(obj->fileName).arg(reason)));
        message->setObject(obj);
        message->setLocationXYZ(obj->x,-obj->y,obj->position[0],obj->position[1],-obj->position[2]);
        findings.push_back(std::move(message));
    }
    // Replace only this scanner's results after a complete run. Preserve all
    // unrelated log entries and retain old results if cancelled or load failed.
    for(int i = int(ErrorMessagesLib::ErrorMessages.size())-1; i >= 0; --i) {
        ErrorMessage* message = ErrorMessagesLib::ErrorMessages[i];
        if(failures == 0 && message != nullptr && message->description.startsWith(QLatin1String(scanPrefix))) {
            ErrorMessagesLib::ErrorMessages.removeAt(i);
            delete message->coords;
            delete message;
        }
    }
    for(auto &rejected : rejectedFiles) findings.push_back(std::move(rejected));
    for(auto& message : findings){
        bool duplicate = false;
        if(failures != 0){
            for(const ErrorMessage *previous : ErrorMessagesLib::ErrorMessages)
                if(previous != nullptr && previous->description == message->description
                        && previous->action == message->action
                        && previous->rejectedWorldHash == message->rejectedWorldHash){
                    duplicate = true;
                    break;
                }
        }
        if(!duplicate) ErrorMessagesLib::PushErrorMessage(message.release());
    }
    QString result = tr("Scan complete: %1 world files, %2 track objects; %3 stacked-track suspects, %4 joint warnings, %5 additional standalone pieces.")
        .arg(loadedFiles).arg(tracks.size()).arg(matches.size()).arg(jointCount).arg(standaloneCount);
    if(skippedPaths != 0 || invalidGeometry != 0)
        result += tr(" Coverage limited: %1 world paths and %2 TDB sections could not be sampled.")
            .arg(skippedPaths).arg(invalidGeometry);
    if(failures != 0)
        result = tr("Scan finished with incomplete coverage: %1 of %2 files checked; %3 rejected. "
                    "See rejected-file records. Full-world database and standalone checks skipped; "
                    "previous findings retained. ").arg(loadedFiles).arg(files.size()).arg(failures)
                + result.replace(tr("Scan complete:"), tr("Checked tiles:"));
    ErrorMessagesLib::PushErrorMessage(new ErrorMessage(
        failures || skippedPaths || invalidGeometry ? ErrorMessage::Type_Warning : ErrorMessage::Type_Info,
        ErrorMessage::Source_Editor,QString::fromLatin1(scanPrefix)+result));
    return result;
}

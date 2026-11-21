// TSRE GenX modifications. Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "Route.h"
#include "Game.h"
#include "GuiFunct.h"
#include "ParserX.h"
#include "ReadFile.h"
#include "RouteSaveTransaction.h"
#include "RulerObj.h"
#include "TSectionDAT.h"
#include "TRnode.h"
#include "Undo.h"
#include "WorldCleanupValidation.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QMessageBox>
#include <QQuaternion>
#include <QRegularExpression>
#include <QSet>
#include <QStringConverter>
#include <QtMath>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <tuple>

namespace {
const QRegularExpression generatedName(QStringLiteral(R"(^DynaTrax-[0-9]+\.(?:s|sd)$)"),
                                       QRegularExpression::CaseInsensitiveOption);
const QRegularExpression worldName(QStringLiteral(R"(^w([+-][0-9]{6})([+-][0-9]{6})\.w$)"),
                                   QRegularExpression::CaseInsensitiveOption);
const QRegularExpression space(QStringLiteral(R"(\s+)"));
constexpr unsigned int unusedSection = std::numeric_limits<unsigned int>::max();
constexpr float geometryTolerance = 0.01f; // Metres from the existing TrackDB centreline.

struct Block {
    QString name;
    qsizetype start;
    qsizetype content;
    qsizetype end;
};

// Read direct child blocks only; quoted filenames and nested comments may
// contain tokens that look like world object or field names.
bool readBlocks(QStringView text, QVector<Block> &blocks) {
    qsizetype pos = 0;
    while(pos < text.size()) {
        if(text[pos].isSpace()) { ++pos; continue; }
        const qsizetype start = pos;
        while(pos < text.size() && !text[pos].isSpace()
                && text[pos] != '(' && text[pos] != ')') ++pos;
        if(pos == start) return false;
        const QString name = text.mid(start, pos - start).toString().toLower();
        while(pos < text.size() && text[pos].isSpace()) ++pos;
        if(pos == text.size() || text[pos++] != '(') return false;
        const qsizetype content = pos;
        int depth = 1;
        bool quoted = false;
        bool escaped = false;
        while(pos < text.size() && depth) {
            const QChar ch = text[pos++];
            if(quoted) {
                if(escaped) escaped = false;
                else if(ch == '\\') escaped = true;
                else if(ch == '"') quoted = false;
            } else if(ch == '"') quoted = true;
            else if(ch == '(') ++depth;
            else if(ch == ')') --depth;
        }
        if(depth) return false;
        blocks.push_back({name, start, content, pos});
    }
    return true;
}

QString value(QStringView text, const Block &field) {
    return text.mid(field.content, field.end - field.content - 1).trimmed().toString();
}

template<size_t N>
bool readNumbers(const QString &text, std::array<float, N> &out) {
    const auto parts = text.split(space, Qt::SkipEmptyParts);
    if(parts.size() != qsizetype(N)) return false;
    for(size_t i = 0; i < N; ++i) {
        bool ok = false;
        out[i] = parts[qsizetype(i)].toFloat(&ok);
        if(!ok || !std::isfinite(out[i])) return false;
    }
    return true;
}

using Owner = std::tuple<int, int, unsigned int>;
struct VectorRow {
    int node;
    int index;
    const float *data;
};
using TrackRows = std::map<Owner, QVector<VectorRow>>;

TrackRows indexTrackRows(const TDB &db) {
    TrackRows rows;
    for(const auto &[id, node] : db.trackNodes) {
        if(!node || node->typ != 1 || !node->trVectorSection) continue;
        for(int i = 0; i < node->iTrv; ++i) {
            const float *data = node->trVectorSection[i].param;
            // Tile coordinates and world UID are stored as floats in TrackDB.
            if(!std::isfinite(data[2]) || !std::isfinite(data[3]) || !std::isfinite(data[4])
                    || std::abs(data[2]) > 999999 || std::abs(data[3]) > 999999
                    || data[4] < 0 || double(data[4]) > std::numeric_limits<unsigned int>::max())
                continue;
            rows[{int(data[2]), int(data[3]), unsigned(data[4])}].push_back({id, i, data});
        }
    }
    return rows;
}

const TSection *sectionFor(const TSectionDAT &sections, int id) {
    const auto it = sections.sekcja.find(id);
    return it == sections.sekcja.end() ? nullptr : it->second;
}

bool usable(const TSection *section) {
    return section && ((section->type == 0 && std::isfinite(section->size) && section->size > 0
            && std::isfinite(section->val1))
        || (section->type == 1 && std::isfinite(section->angle) && section->angle != 0
            && std::isfinite(section->radius) && section->radius > 0));
}

float length(const TSection &section) {
    return section.type == 0 ? section.size : std::abs(section.angle * section.radius);
}

QVector3D pointAt(const TSection &section, float distance) {
    if(section.type == 0) return {0, 0, distance};
    const double theta = double(distance) / section.radius;
    const double side = section.angle > 0 ? -1 : 1;
    return {float(side * section.radius * (1 - std::cos(theta))), 0,
            float(section.radius * std::sin(theta))};
}

QQuaternion rotation(float radians, float x, float y, float z) {
    return QQuaternion::fromAxisAndAngle(x, y, z, qRadiansToDegrees(radians));
}

bool matchesTrackDb(const QVector<const TSection *> &path, const QVector<VectorRow> &rows,
                    const TSectionDAT &sections, int shapeId, int x, int z,
                    const QVector3D &position, const QQuaternion &orientation) {
    if(rows.size() != path.size()) return false;
    const bool forward = rows.first().data[5] == 0;
    QVector3D offset;
    float turn = 0;
    for(qsizetype i = 0; i < path.size(); ++i) {
        const VectorRow &row = rows[forward ? i : rows.size() - 1 - i];
        const float *data = row.data;
        if(row.node != rows.first().node || row.index != rows.first().index + (forward ? i : rows.size() - 1 - i)
                || data[1] != shapeId || data[5] != (forward ? 0 : 1)
                || !std::isfinite(data[0]) || data[0] < 0
                || double(data[0]) > std::numeric_limits<int>::max()) return false;
        for(int field = 8; field < 16; ++field)
            if(!std::isfinite(data[field])) return false;
        const TSection &section = *path[i];
        const TSection *dbSection = sectionFor(sections, int(data[0]));
        const float distance = length(section);
        if(!usable(dbSection) || !std::isfinite(distance)
                || std::abs(length(*dbSection) - distance) > geometryTolerance
                || dbSection->type != section.type
                || (section.type == 1 && (std::abs(dbSection->radius - section.radius) > geometryTolerance
                    || std::abs(dbSection->angle - (forward ? section.angle : -section.angle)) > 0.0001f)))
            return false;

        const auto worldCurve = rotation(-turn, 0, 1, 0);
        const auto dbRotation = rotation(-3.14159265358979323846f - data[14], 0, 1, 0)
            * rotation(data[13], 1, 0, 0) * rotation(-data[15], 0, 0, 1);
        const QVector3D dbOrigin((data[8] - x) * 2048.0f + data[10], data[11],
                                 (data[9] - z) * 2048.0f + data[12]);
        for(float fraction : {0.f, 0.25f, 0.5f, 0.75f, 1.f}) {
            const auto local = offset + worldCurve.rotatedVector(pointAt(section, distance * fraction));
            const auto posed = orientation.rotatedVector({-local.x(), local.y(), -local.z()});
            const auto dbPoint = dbRotation.rotatedVector(pointAt(*dbSection,
                length(*dbSection) * (forward ? fraction : 1 - fraction)));
            if(!((position + QVector3D(posed.x(), posed.y(), -posed.z())
                    - dbOrigin - QVector3D(dbPoint.x(), dbPoint.y(), -dbPoint.z())).length()
                    <= geometryTolerance)) return false;
        }
        offset += worldCurve.rotatedVector(pointAt(section, distance));
        if(section.type == 1) turn += section.angle;
    }
    return true;
}

struct Rewrite {
    qsizetype start;
    qsizetype end;
    QString text;
};

QString dynamicTrack(QStringView body, const QVector<Block> &fields, int x, int z,
                     const TSectionDAT &sections, const TrackRows &trackRows,
                     const TrackRows &savedRows) {
    static const QSet<QString> supported{"uid", "sectionidx", "elevation", "collideflags",
        "filename", "staticflags", "position", "qdirection", "staticdetaillevel",
        "vdbid", "jnodeposn"};
    QHash<QString, Block> lookup;
    for(const Block &field : fields) {
        if(!supported.contains(field.name) || lookup.contains(field.name)) return {};
        lookup.insert(field.name, field);
    }
    auto property = [&](const QString &name) {
        const auto field = lookup.constFind(name);
        return field == lookup.cend() ? QString() : value(body, *field);
    };
    bool uidOk = false, idOk = false;
    const auto uid = property("uid").toUInt(&uidOk);
    const int id = property("sectionidx").toInt(&idOk);
    std::array<float, 3> position{};
    std::array<float, 4> q{};
    if(!uidOk || !idOk || !readNumbers(property("position"), position)
            || !readNumbers(property("qdirection"), q)) return {};
    const QQuaternion attitude(q[3], q[0], q[1], -q[2]);
    if(std::abs(attitude.length() - 1.f) > 0.001f) return {};
    const auto path = sections.shape.find(id);
    if(path == sections.shape.end() || !path->second || !path->second->dyntrack
            || path->second->roadshape || path->second->numpaths != 1 || !path->second->path)
        return {};
    const auto &trackPath = path->second->path[0];
    if(trackPath.n < 1 || trackPath.n > 5 || trackPath.rotDeg != 0
            || trackPath.pos[0] != 0 || trackPath.pos[1] != 0 || trackPath.pos[2] != 0) return {};

    QVector<const TSection *> geometry;
    std::array<const TSection *, 5> nativeSections{};
    size_t slot = 0;
    for(int i = 0; i < trackPath.n; ++i) {
        const unsigned int sectionId = trackPath.sect[i];
        if(sectionId > 1000000) return {};
        const TSection *section = sectionFor(sections, int(sectionId));
        if(!usable(section)) return {};
        if(slot % 2 != size_t(section->type)) ++slot;
        if(slot >= nativeSections.size()) return {};
        nativeSections[slot++] = section;
        geometry.push_back(section);
    }

    const auto matches = [&](const TrackRows &index) {
        const auto rows = index.find({x, z, uid});
        return rows != index.end() && matchesTrackDb(geometry, rows->second, sections, id, x, z,
            {position[0], position[1], position[2]}, attitude.normalized());
    };
    if(!matches(trackRows) || !matches(savedRows)) return {};

    const QString newline = body.contains(u"\r\r\n") ? QStringLiteral("\r\r\n")
        : body.contains(u"\r\n") ? QStringLiteral("\r\n") : QStringLiteral("\n");
    QString result = "Dyntrack (" + newline + "\t\tTrackSections (" + newline;
    for(size_t i = 0; i < nativeSections.size(); ++i) {
        const auto *section = nativeSections[i];
        const bool curve = i % 2 != 0;
        result += QString("\t\t\tTrackSection ( SectionCurve ( %1 ) %2 %3 %4 )")
            .arg(curve ? 1 : 0).arg(section ? unsigned(section->id) : unusedSection)
            .arg(section ? (curve ? section->angle : section->size) : 0, 0, 'g', 9)
            .arg(section ? (curve ? section->radius : section->val1) : 0, 0, 'g', 9) + newline;
    }
    const float rise = -attitude.normalized().rotatedVector({0, 0, 1}).y();
    if(std::abs(rise) >= 1.f) return {};
    result += "\t\t)" + newline + QString("\t\tElevation ( %1 )")
        .arg(1000.f * std::tan(std::asin(rise)), 0, 'g', 9);
    QString preserved = body.toString();
    for(auto it = fields.crbegin(); it != fields.crend(); ++it)
        if(it->name == "filename" || it->name == "elevation")
            preserved.remove(it->start, it->end - it->start);
    return result + preserved + ')';
}

bool containsMarker(const QByteArray &data) {
    static const QByteArray ascii("dynatrax-");
    QByteArray folded = data.toLower();
    QByteArray little, big;
    for(char c : ascii) { little += c; little += '\0'; big += '\0'; big += c; }
    return folded.contains(ascii) || folded.contains(little) || folded.contains(big);
}

bool readBytes(const QFileInfo &info, QByteArray &bytes) {
    QFile file(info.absoluteFilePath());
    constexpr qint64 limit = 256LL * 1024 * 1024;
    if(info.isSymLink() || !file.open(QIODevice::ReadOnly) || file.size() > limit) return false;
    bytes = file.read(limit + 1);
    return bytes.size() <= limit && bytes.size() == file.size() && file.error() == QFileDevice::NoError;
}

QByteArray encodeWorld(const QString &text, const QByteArray &source) {
    const auto encoding = source.startsWith(QByteArray::fromHex("fffe")) ? QStringConverter::Utf16LE
        : source.startsWith(QByteArray::fromHex("feff")) ? QStringConverter::Utf16BE
        : QStringConverter::Utf8;
    const bool bom = encoding != QStringConverter::Utf8 || source.startsWith(QByteArray::fromHex("efbbbf"));
    QStringEncoder encoder(encoding, bom ? QStringConverter::Flag::WriteBom : QStringConverter::Flag::Default);
    return encoder(text);
}
}

void Route::removeDynaTrax(QWidget *parent) {
    const QString title = tr("Remove DYNATRAX");
    auto stop = [&](const QString &reason) { GuiFunct::showEditorStopped(parent, title, reason); };
    const auto remapped = [](const QHash<unsigned int, unsigned int> &ids) {
        for(unsigned int id : ids) if(id != 0) return true;
        return false;
    };
    if(!Game::writeEnabled || !trackDB || !trackDB->loaded || !tsection || tsection->dataOutOfSync
            || remapped(tsection->autoFixedSectionIds) || remapped(tsection->autoFixedShapeIds)) {
        stop(tr("Open a writable route with a synchronized track database and local tsection.dat."));
        return;
    }
    QVector<QString> unsaved;
    getUnsavedInfo(unsaved);
    if(!unsaved.isEmpty()) {
        stop(tr("Save the current route edits before removing DynaTrax."));
        return;
    }
    const QDir route(Game::root + "/routes/" + Game::route);
    const QDir world(route.filePath("world"));
    const QDir shapes(route.filePath("shapes"));
    const auto worldFiles = world.entryInfoList({"*.w"}, QDir::Files | QDir::Hidden);
    if(worldFiles.isEmpty() || !shapes.exists() || QFileInfo(world.absolutePath()).isSymLink()
            || QFileInfo(shapes.absolutePath()).isSymLink()) {
        stop(tr("The route's World and Shapes directories must be available for a complete scan."));
        return;
    }

    RouteSaveTransaction transaction(route.absolutePath(),
        Game::appDataDir() + "/atomicSaves/" + Game::routeAppDataKey());
    QHash<QString, QString> generatedAssets;
    QHash<QString, QByteArray> inspected;
    QString error;
    for(const auto &file : shapes.entryInfoList({"*.s", "*.sd"}, QDir::Files | QDir::Hidden)) {
        if(!generatedName.match(file.fileName()).hasMatch()) continue;
        generatedAssets.insert(file.fileName().toLower(), file.absoluteFilePath());
    }

    const auto trackRows = indexTrackRows(*trackDB);
    TDB savedDb(tsection, false);
    TrackRows savedRows;
    bool loadedSavedDb = false;
    QVector<QPair<int, int>> changedTiles;
    int converted = 0;
    for(const auto &file : worldFiles) {
        const auto coords = worldName.match(file.fileName());
        QByteArray raw, binary;
        QString text;
        if(!coords.hasMatch() || !readBytes(file, raw)
                || !WorldCleanupValidation::decodeWorldFile(raw, text, binary)) {
            stop(tr("Could not safely read world file %1. No conversion was committed.").arg(file.fileName()));
            return;
        }
        const auto originalHash = QCryptographicHash::hash(raw, QCryptographicHash::Sha256);
        inspected.insert(file.absoluteFilePath(), originalHash);
        if(containsMarker(binary)) {
            stop(tr("DynaTrax was found in binary world file %1. Convert that file to text before retrying.").arg(file.fileName()));
            return;
        }
        if(!text.contains("DynaTrax-", Qt::CaseInsensitive)) continue;
        if(!loadedSavedDb) {
            // TrackDB can have pending edits independent of the world tiles.
            // Validate against the persisted data as well as the editor state.
            QFile saved(route.filePath(Game::routeName + ".tdb"));
            if(!saved.open(QIODevice::ReadOnly)) {
                stop(tr("Cannot read the saved TrackDB.")); return;
            }
            const std::unique_ptr<FileBuffer> data(ReadFile::read(&saved));
            if(!data) { stop(tr("Cannot decode the saved TrackDB.")); return; }
            data->toUtf16();
            data->skipBOM();
            ParserX::NextLine(data.get());
            if(ParserX::NextTokenInside(data.get()).compare("TrackDB", Qt::CaseInsensitive) != 0
                    || !savedDb.loadUtf16Data(data.get())) {
                stop(tr("Cannot validate the saved TrackDB.")); return;
            }
            savedRows = indexTrackRows(savedDb);
            loadedSavedDb = true;
        }
        const int x = coords.captured(1).toInt(), z = coords.captured(2).toInt();
        const qsizetype rootStart = text.indexOf("Tr_Worldfile", 0, Qt::CaseInsensitive);
        QVector<Block> roots, objects;
        const QStringView source(text);
        if(rootStart < 0 || !readBlocks(source.mid(rootStart), roots) || roots.isEmpty()) {
            stop(tr("Could not scan %1.").arg(file.fileName())); return;
        }
        const auto &root = roots.first();
        const qsizetype worldStart = rootStart + root.content;
        const auto worldBody = source.mid(worldStart, root.end - root.content - 1);
        if(!readBlocks(worldBody, objects)) { stop(tr("Could not scan %1.").arg(file.fileName())); return; }
        QVector<Rewrite> rewrites;
        QSet<unsigned int> convertedIds;
        QHash<unsigned int, int> uidCounts;
        static const QRegularExpression uidField(QStringLiteral(R"(\bUiD\s*\(\s*([0-9]+)\s*\))"),
                                                 QRegularExpression::CaseInsensitiveOption);
        for(const auto &object : objects) {
            const auto body = worldBody.mid(object.content, object.end - object.content - 1);
            const auto uidMatch = uidField.matchView(body);
            bool validUid = false;
            const unsigned int uid = uidMatch.captured(1).toUInt(&validUid);
            if(validUid) ++uidCounts[uid];
            if(!body.contains(u"DynaTrax-", Qt::CaseInsensitive)) continue;
            QVector<Block> fields;
            if(!readBlocks(body, fields)) {
                stop(tr("Could not inspect an object in %1.").arg(file.fileName())); return;
            }
            QString fileName;
            for(const auto &field : fields) {
                if(field.name == "filename") fileName = value(body, field);
            }
            fileName.remove('"');
            fileName.replace('\\', '/');
            const auto name = QFileInfo(fileName).fileName();
            const auto replacement = validUid && object.name == "trackobj" && name.endsWith(".s", Qt::CaseInsensitive)
                    && generatedName.match(name).hasMatch()
                ? dynamicTrack(body, fields, x, z, *tsection, trackRows, savedRows) : QString();
            if(replacement.isEmpty()) {
                stop(tr("Cannot reconstruct DynaTrax in %1 (UID %2) from its local track path and TrackDB geometry. No conversion was committed.")
                    .arg(file.fileName()).arg(uid));
                return;
            }
            convertedIds.insert(uid);
            rewrites.push_back({worldStart + object.start, worldStart + object.end, replacement});
        }
        for(unsigned int uid : convertedIds) {
            if(uidCounts.value(uid) != 1) {
                stop(tr("Duplicate track UID %1 in %2.").arg(uid).arg(file.fileName())); return;
            }
        }
        if(rewrites.isEmpty()) {
            stop(tr("An unresolved DynaTrax reference remains in %1.").arg(file.fileName())); return;
        }
        for(auto it = rewrites.crbegin(); it != rewrites.crend(); ++it)
            text.replace(it->start, it->end - it->start, it->text);
        if(text.contains("DynaTrax-", Qt::CaseInsensitive)
                || !WorldCleanupValidation::completeTextWorld(text)) {
            stop(tr("An unresolved DynaTrax reference remains in %1.").arg(file.fileName())); return;
        }
        if(!transaction.addFile(file.absoluteFilePath(), encodeWorld(text, raw), &error, originalHash)) {
            stop(error); return;
        }
        converted += rewrites.size();
        changedTiles.push_back(qMakePair(x, -z));
    }
    if(converted == 0 && generatedAssets.isEmpty()) {
        QMessageBox::information(parent, title, tr("No DynaTrax track or generated shape files were found."));
        return;
    }

    // These files can still refer to generated assets even if every placement
    // has been converted. Shared textures and unrelated metadata stay in place.
    QDirIterator metadata(route.absolutePath(), {"*.ref", "*.dat", "*.trk", "*.tdb", "*.rdb", "*.ws",
        "*.sd", "*.xml", "*.ini", "*.json"}, QDir::Files, QDirIterator::Subdirectories);
    while(metadata.hasNext()) {
        const QFileInfo file(metadata.next());
        if(file.absolutePath().compare(shapes.absolutePath(), Qt::CaseInsensitive) == 0
                && generatedAssets.contains(file.fileName().toLower())) continue;
        QByteArray raw, expanded;
        if(!readBytes(file, raw) || !WorldCleanupValidation::expandMstsFile(raw, expanded)
                || containsMarker(expanded)) {
            stop(tr("Cannot verify that %1 is free of DynaTrax references. No conversion was committed.")
                .arg(route.relativeFilePath(file.absoluteFilePath())));
            return;
        }
        inspected.insert(file.absoluteFilePath(), QCryptographicHash::hash(raw, QCryptographicHash::Sha256));
    }
    for(const QString &asset : generatedAssets)
        if(!transaction.addRemoval(asset, &error)) { stop(error); return; }
    const QString summary = tr("Convert %1 track placement(s) in %2 world tile(s) to native Dynamic Track and remove %3 generated shape or descriptor file(s)?\n\n"
        "The TrackDB, section IDs and shared textures will be preserved. TSRE will save these changes immediately and back up affected files. This operation clears the current Undo history.")
        .arg(converted).arg(changedTiles.size()).arg(generatedAssets.size());
    if(!GuiFunct::confirmDestructiveAction(parent, title, summary)) return;
    if(Game::currentRoute != this) return;
    unsaved.clear();
    getUnsavedInfo(unsaved);
    if(!unsaved.isEmpty()
            || world.entryInfoList({"*.w"}, QDir::Files | QDir::Hidden) != worldFiles) {
        stop(tr("The route changed after the scan. Run Remove DYNATRAX again."));
        return;
    }
    for(auto it = inspected.cbegin(); it != inspected.cend(); ++it) {
        QByteArray current;
        if(!readBytes(QFileInfo(it.key()), current)
                || QCryptographicHash::hash(current, QCryptographicHash::Sha256) != it.value()) {
            stop(tr("A route file changed after the scan: %1").arg(route.relativeFilePath(it.key())));
            return;
        }
    }
    if(!transaction.commit(&error)) { stop(error); return; }

    emit sendMsg("unselect");
    Undo::Clear();
    auto clearRuler = [&](RulerObj *&ruler, int x, int z) {
        if(ruler && ruler->x == x && ruler->y == z) ruler = nullptr;
    };
    for(const auto &[x, z] : changedTiles) {
        if(!tile.value(x * 10000 + z, nullptr)) continue;
        clearRuler(waterRulerObj, x, z);
        clearRuler(vegetationRulerObj, x, z);
        clearRuler(gradeRulerObj, x, z);
        clearRuler(autoPlaceRulerObj, x, z);
        reloadTile(x, z);
    }
    QMessageBox::information(parent, title, tr("Converted %1 track placement(s) and removed %2 generated shape or descriptor file(s).")
        .arg(converted).arg(generatedAssets.size()));
}

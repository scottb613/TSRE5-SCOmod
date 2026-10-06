#include "AutoPlaceWire.h"

QSet<QString> AutoPlaceWire::connectedSpans(const QJsonObject &spans,
        const QString &support, int node, bool road) {
    auto identity=[](const QJsonObject &p) {
        return QString("%1,%2,%3").arg(p["x"].toInt()).arg(p["z"].toInt()).arg(p["uid"].toDouble(),0,'f',0);
    };
    QSet<QString> supports{support}, keys;
    bool changed=true;
    while(changed) {
        changed=false;
        for(const QString &key:spans.keys()) {
            const auto span=spans[key].toObject();
            if(keys.contains(key) || span["deleted"].toBool() || span["external"].toBool()
                    || span["node"].toInt(-1)!=node || span["road"].toBool()!=road) continue;
            const auto a=identity(span["poleA"].toObject()), b=identity(span["poleB"].toObject());
            if(!supports.contains(a) && !supports.contains(b)) continue;
            keys.insert(key); supports.insert(a); supports.insert(b); changed=true;
        }
    }
    return keys;
}
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStringConverter>
#include <QCryptographicHash>
#include <QDataStream>
#include <QLocale>
#include <cmath>
#include <QFileInfo>
#include <QRegularExpression>
#include "WireAssetScan.h"

namespace AutoPlaceWire {
bool isWireShape(const QString &name) {
    static const QRegularExpression pattern(
        "^(APW_[0-9a-f]{24}|SCO_TelephoneWire_OR_Test(?:_[0-9]{2})?)\\.s$",
        QRegularExpression::CaseInsensitiveOption);
    return QFileInfo(name).fileName()==name && pattern.match(name).hasMatch();
}

bool findWireWorldTiles(const QString &routePath, QVector<QPair<int, int>> &tiles,
                        QString &error, const CleanupProgress &progress) {
    tiles.clear(); error.clear();
    QDir world(routePath+"/WORLD");
    if(!world.exists() || !world.isReadable()) {
        error="Wire cleanup requires a readable WORLD folder."; return false;
    }
    const auto files=world.entryList({"*.w"},QDir::Files);
    QVector<QPair<int,int>> found;
    int done=0;
    for(const QString &file:files) {
        if(progress) progress("Finding wire world tiles...",done++,int(files.size()));
        QFile input(world.filePath(file));
        QString text; QByteArray binary;
        if(!input.open(QIODevice::ReadOnly) || input.size()>WireAssetScan::MaximumExpandedWorldBytes) {
            error="Cannot scan saved world file for wire cleanup: "+file; return false;
        }
        const auto data=input.readAll();
        if(input.error()!=QFileDevice::NoError || !WireAssetScan::decodedWorldFile(data,text,binary)) {
            error="Cannot decode saved world file for wire cleanup: "+file; return false;
        }
        // Conservative discovery only: exact shape-name ownership is checked
        // again on loaded objects. Include missing assets and untracked bakes.
        bool hasWire=false;
        binary=binary.toLower();
        for(const QString &prefix:{QString("apw_"),QString("sco_telephonewire_or_test")}) {
            QStringEncoder encoder(QStringEncoder::Utf16LE);
            const QByteArray utf16Prefix=encoder(prefix);
            hasWire=hasWire || text.contains(prefix,Qt::CaseInsensitive)
                || binary.contains(prefix.toLatin1()) || binary.contains(utf16Prefix);
        }
        if(!hasWire) continue;
        bool validX=false,validZ=false;
        const int x=file.mid(1,7).toInt(&validX),z=-file.mid(8,7).toInt(&validZ);
        if(file.size()!=17 || !file.startsWith('w',Qt::CaseInsensitive) || !validX || !validZ) {
            error="Invalid wire world filename: "+file; return false;
        }
        found.append({x,z});
    }
    tiles=found;
    return true;
}

bool pruneAssets(const QString &routePath, QJsonObject &spans, bool all, QString &error,
                 bool allowSavedReferences, const CleanupProgress &progress) {
    error.clear();
    QSet<QString> candidates, keep;
    for(const QString &key:spans.keys()) {
        const auto span=spans[key].toObject();
        const QString name=span["external"].toBool()?span["shape"].toString():shapeName(key);
        if(!isWireShape(name)) continue;
        if(span["deleted"].toBool()) candidates.insert(name.toLower());
        else keep.insert(name.toLower());
    }
    QDir shapes(routePath+"/SHAPES"), world(routePath+"/WORLD");
    if(!shapes.exists() || !world.exists() || !world.isReadable()) { error="Wire cleanup requires readable SHAPES and WORLD folders."; return false; }
    if(all) for(const QString &file:shapes.entryList(QDir::Files)) {
        const QString name=file.endsWith(".sd",Qt::CaseInsensitive)?file.left(file.size()-1):file;
        if(isWireShape(name)) candidates.insert(name.toLower());
    }
    // A reset may be followed by Commit/Bake before its cleanup is finished.
    // Those retained spans are not deletion candidates, even in a global sweep.
    // Their saved references must not keep the cleanup request pending forever.
    candidates.subtract(keep);
    if(candidates.isEmpty() && !all) return true;
    QSet<QString> referenced;
    struct EncodedName { QString name; QByteArray ascii,utf16; };
    QVector<EncodedName> encodedNames;
    for(const QString &name:candidates) {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        encodedNames.append({name,name.toLatin1(),encoder(name)});
    }
    const auto worldFiles=world.entryList({"*.w"},QDir::Files);
    int worldsDone=0;
    for(const QString &file:worldFiles) {
        if(progress) progress("Checking saved wire references...",worldsDone++,int(worldFiles.size()));
        QFile input(world.filePath(file));
        if(!input.open(QIODevice::ReadOnly) || input.size()>WireAssetScan::MaximumExpandedWorldBytes) {
            error="Cannot scan saved world file for wire cleanup: "+file; return false;
        }
        QString text; QByteArray binary;
        const QByteArray data = input.readAll();
        if(input.error() != QFileDevice::NoError
                || !WireAssetScan::decodedWorldFile(data,text,binary)) {
            error="Cannot decode saved world file for wire cleanup: "+file; return false;
        }
        text=text.toLower(); binary=binary.toLower();
        for(const auto &name:encodedNames) {
            if(referenced.contains(name.name)) continue;
            if(text.contains(name.name) || binary.contains(name.ascii) || binary.contains(name.utf16))
                referenced.insert(name.name);
        }
    }
    // Scan everything before deleting anything. Saved references and newly
    // committed raw pairs always win over pending cleanup.
    candidates.subtract(referenced); candidates.subtract(keep);
    for(const QString &file:shapes.entryList(QDir::Files)) {
        const QString name=file.endsWith(".sd",Qt::CaseInsensitive)?file.left(file.size()-1):file;
        if(!candidates.contains(name.toLower())) continue;
        if(!QFile::remove(shapes.filePath(file))) { error="Cannot remove unused wire asset: "+file; return false; }
    }
    // Shared textures may also be used by a surviving non-wire shape. Inspect
    // all remaining shape payloads before deleting these exact generated names.
    // Before world placements are saved, wire shapes still have saved
    // references. Retain shared textures until that save instead of scanning
    // every scenery shape during the initial Delete All operation.
    if(all && referenced.isEmpty()) {
        QDir textures(routePath+"/TEXTURES");
        QStringList unused;
        static const QRegularExpression texturePattern(
            "^(APWireCharcoal_v1|SCO_TelephoneWire_OR_Test(?:_[0-9]{2})?)\\.dds$",
            QRegularExpression::CaseInsensitiveOption);
        for(const QString &file:textures.entryList(QDir::Files))
            if(texturePattern.match(file).hasMatch()) unused.append(file);
        const auto shapeFiles=shapes.entryList({"*.s"},QDir::Files);
        int shapesDone=0;
        if(!unused.isEmpty()) for(const QString &file:shapeFiles) {
            if(progress) progress("Checking shared wire textures...",shapesDone++,int(shapeFiles.size()));
            QFile input(shapes.filePath(file)); QByteArray data;
            if(!input.open(QIODevice::ReadOnly) || input.size()>WireAssetScan::MaximumExpandedWorldBytes
                    || !WireAssetScan::expandedWorldFile(input.readAll(),data)) {
                error="Cannot check shared wire texture references in "+file; return false;
            }
            data=data.toLower();
            for(int i=int(unused.size())-1;i>=0;--i) {
                const QString name=unused[i].toLower();
                QStringEncoder le(QStringEncoder::Utf16LE), be(QStringEncoder::Utf16BE);
                const QByteArray utf16LeName=le(name), utf16BeName=be(name);
                if(data.contains(name.toLatin1()) || data.contains(utf16LeName) || data.contains(utf16BeName)) unused.removeAt(i);
            }
            if(unused.isEmpty()) break;
        }
        for(const QString &file:unused) if(!QFile::remove(textures.filePath(file))) {
            error="Cannot remove unused wire texture: "+file; return false;
        }
    }
    for(const QString &key:spans.keys()) {
        const auto span=spans[key].toObject();
        const QString name=span["external"].toBool()?span["shape"].toString():shapeName(key);
        if(span["deleted"].toBool() && !referenced.contains(name.toLower())) spans.remove(key);
    }
    if(!referenced.isEmpty() && !allowSavedReferences) { error="Some wire assets are still referenced by saved world objects; retained for retry."; return false; }
    return true;
}

bool discardBakes(const QString &routePath, QJsonObject &spans,
                  const QSet<QString> &keys, QString &error) {
    error.clear();
    if(keys.isEmpty()) return true;
    QJsonObject candidates;
    for(const QString &key:keys) {
        if(spans.value(key).toObject().value("external").toBool()) continue;
        candidates[key]=QJsonObject{{"deleted",true}};
    }
    const auto attempted=candidates.keys();
    // A failed route save may already have written some world tiles. Preserve
    // their assets and original registry records; discard only absent bakes.
    if(!pruneAssets(routePath,candidates,false,error,true)) return false;
    for(const QString &key:attempted)
        if(!candidates.contains(key)) spans.remove(key);
    return true;
}

QJsonObject reconcileReservation(QJsonObject span, bool present) {
    if(present || span.value("deleted").toBool()) return span;
    span["baked"]=false;
    span.remove("uid");
    if(span.value("external").toBool()) {
        span.remove("external");
        span.remove("shape");
        // Removing an external object does not authorize an automatic rebake.
        // Explicit Commit captures current supports and reactivates this pair.
        span["active"]=false;
    }
    return span;
}
QVector3D vector(const QJsonValue &v) {
    const auto a = v.toArray();
    return QVector3D(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
}
QJsonArray json(const QVector3D &v) { return {v.x(), v.y(), v.z()}; }
QString shapeName(const QString &key) {
    return "APW_" + QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(),
        QCryptographicHash::Sha256).toHex().left(24)) + ".s";
}
Mesh mesh(const QJsonObject &span, bool distant) {
    Mesh result;
    const float radius = span["width"].toDouble(20) * 0.0005f;
    auto face = [&](QVector3D a, QVector3D b, QVector3D c, QVector3D outward) {
        auto normal = QVector3D::crossProduct(b-a,c-a).normalized();
        if(QVector3D::dotProduct(normal,outward) < 0) { qSwap(b,c); normal = -normal; }
        result << Vertex{a,normal} << Vertex{b,normal} << Vertex{c,normal};
    };
    for(const auto &value : span["wires"].toArray()) {
        const auto wire = value.toObject();
        const auto a = vector(wire["a"]), b = vector(wire["b"]);
        auto horizontal = b-a; horizontal.setY(0);
        if(horizontal.length() < 0.1f) continue;
        const float sag = horizontal.length() * span["sag"].toDouble(1.5) * 0.01f;
        const auto side = QVector3D::crossProduct(horizontal.normalized(), {0,1,0});
        const int segments = distant ? 5 : 10;
        auto sample = [&](float t) { return a*(1-t)+b*t-QVector3D(0,4*sag*t*(1-t),0); };
        if(distant) {
            const QVector3D r(0,radius,0);
            for(int i=0;i<segments;++i) {
                auto p=sample(float(i)/segments), q=sample(float(i+1)/segments);
                for(float sign : {1.f,-1.f}) {
                    face(p-r,q-r,q+r,side*sign);
                    face(p-r,q+r,p+r,side*sign);
                }
            }
        } else {
            QVector<QVector<QVector3D>> rings;
            for(int i=0;i<=segments;++i) {
                float t=float(i)/segments;
                auto tangent=b-a-QVector3D(0,4*sag*(1-2*t),0);
                auto up=QVector3D::crossProduct(side,tangent).normalized();
                QVector<QVector3D> ring;
                for(int j=0;j<3;++j) {
                    float angle=6.28318530718f*j/3;
                    ring << sample(t)+(side*std::cos(angle)+up*std::sin(angle))*radius;
                }
                rings << ring;
            }
            for(int i=0;i<segments;++i) for(int j=0;j<3;++j) {
                int k=(j+1)%3;
                auto outward=(rings[i][j]+rings[i][k])*0.5f-sample(float(i)/segments);
                face(rings[i][j],rings[i+1][j],rings[i+1][k],outward);
                face(rings[i][j],rings[i+1][k],rings[i][k],outward);
            }
            face(rings[0][0],rings[0][1],rings[0][2],sample(0)-sample(0.1f));
            face(rings[segments][0],rings[segments][1],rings[segments][2],sample(1)-sample(0.9f));
        }
    }
    return result;
}
bool writeRegistry(const QString &path, const QJsonObject &registry, QString &error) {
    if(!registryWithinLimit(registry["spans"].toObject(), error)) return false;
    QSaveFile file(path);
    const auto bytes=QJsonDocument(registry).toJson();
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
        error="Cannot save AP wire definitions: "+file.errorString(); return false;
    }
    return true;
}
bool writeShape(const QString &routePath, const QJsonObject &span, QString &error) {
    const auto near=mesh(span), far=mesh(span,true);
    if(near.isEmpty() || far.isEmpty()) { error="Wire span has no valid geometry."; return false; }
    const QString name=shapeName(span["key"].toString());
    const QString shapes=routePath+"/SHAPES/", textures=routePath+"/TEXTURES/";
    if(!QDir().mkpath(shapes) || !QDir().mkpath(textures)) { error="Cannot create wire asset directories."; return false; }
    // Shared, versioned texture: explicit full mip chain is essential for OR loaders.
    const QString texture="APWireCharcoal_v1.dds";
    QByteArray dds;
    QDataStream data(&dds,QIODevice::WriteOnly); data.setByteOrder(QDataStream::LittleEndian);
    data.writeRawData("DDS ",4);
    for(quint32 n : {124u,0x2100fu,4u,4u,16u,0u,3u}) data << n;
    for(int i=0;i<11;++i) data << quint32(0);
    for(quint32 n : {32u,0x41u,0u,32u,0xff0000u,0xff00u,0xffu,0xff000000u,0x401008u,0u,0u,0u,0u}) data << n;
    for(int i=0;i<21;++i) { const char pixel[4]={64,62,60,char(255)}; data.writeRawData(pixel,4); }
    QFile existing(textures+texture);
    if(existing.exists()) {
        if(!existing.open(QIODevice::ReadOnly) || existing.readAll()!=dds) {
            error="The shared AP wire texture differs from the generated texture; it was not overwritten."; return false;
        }
    } else {
        QSaveFile file(textures+texture);
        if(!file.open(QIODevice::WriteOnly) || file.write(dds)!=dds.size() || !file.commit()) {
            error="Cannot write AP wire texture."; return false;
        }
    }
    QSaveFile file(shapes+name);
    if(!file.open(QIODevice::WriteOnly)) { error=file.errorString(); return false; }
    QTextStream out(&file); out.setEncoding(QStringConverter::Utf16LE); out.setGenerateByteOrderMark(true);
    out.setLocale(QLocale::c()); out.setRealNumberPrecision(9);
    Mesh all=near; all+=far;
    float radius=1; for(const auto &v : all) radius=qMax(radius,v.point.length()*1.05f);
    auto vec=[&](QVector3D v) { out<<v.x()<<' '<<v.y()<<' '<<-v.z(); };
    out<<"SIMISA@@@@@@@@@@JINX0s1t______\r\nshape (\r\n shape_header ( 00000000 00000000 )\r\n"
       <<" volumes ( 1 vol_sphere ( vector ( 0 0 0 ) "<<radius<<" ) )\r\n"
       <<" shader_names ( 1 named_shader ( TexDiff ) )\r\n texture_filter_names ( 1 named_filter_mode ( MipLinear ) )\r\n points ( "<<all.size()<<"\r\n";
    for(const auto &v:all) { out<<" point ( "; vec(v.point); out<<" )\r\n"; }
    out<<" ) uv_points ( 1 uv_point ( 0.5 0.5 ) )\r\n normals ( "<<all.size()<<"\r\n";
    for(const auto &v:all) { out<<" vector ( "; vec(v.normal); out<<" )\r\n"; }
    out<<" ) sort_vectors ( 1 vector ( 0 0 0 ) ) colours ( 0 )\r\n"
       <<" matrices ( 1 matrix MAIN ( 1 0 0 0 1 0 0 0 1 0 0 0 ) )\r\n"
       <<" images ( 1 image ( "<<texture<<" ) ) textures ( 1 texture ( 0 0 0 ff000000 ) ) light_materials ( 0 )\r\n"
       <<" light_model_cfgs ( 1 light_model_cfg ( 00000000 uv_ops ( 1 uv_op_copy ( 1 0 ) ) ) )\r\n"
       <<" vtx_states ( 1 vtx_state ( 00000000 0 -6 0 00000002 ) )\r\n"
       <<" prim_states ( 1 prim_state ap_wire ( 00000000 0 tex_idxs ( 1 0 ) 0 0 0 0 1 ) )\r\n"
       <<" lod_controls ( 1 lod_control ( distance_levels_header ( 0 ) distance_levels ( 3\r\n";
    for(int lod=0;lod<3;++lod) {
        int count=lod==0?near.size():lod==1?far.size():3;
        int offset=lod==1?near.size():0, triangles=count/3;
        out<<" distance_level ( distance_level_header ( dlevel_selection ( "<<(lod==0?300:lod==1?750:2000)
           <<" ) hierarchy ( 1 -1 ) ) sub_objects ( 1 sub_object (\r\n"
           <<" sub_object_header ( 00000400 -1 -1 000001d2 000001c4 geometry_info ( "<<triangles<<" 1 0 "<<count
           <<" 0 0 1 0 0 0 geometry_nodes ( 1 geometry_node ( 1 0 0 0 0 cullable_prims ( 1 "<<triangles<<' '<<count
           <<" ) ) ) geometry_node_map ( 1 0 ) ) subobject_shaders ( 1 0 ) subobject_light_cfgs ( 1 0 ) 0 )\r\n vertices ( "<<count<<"\r\n";
        for(int i=0;i<count;++i) { int index=lod==2?0:offset+i; out<<" vertex ( 00000000 "<<index<<' '<<index<<" FFFFFFFF FF000000 vertex_uvs ( 1 0 ) )\r\n"; }
        out<<" ) vertex_sets ( 1 vertex_set ( 0 0 "<<count<<" ) ) primitives ( 2 prim_state_idx ( 0 ) indexed_trilist ( vertex_idxs ( "<<count;
        for(int i=0;i<count;++i) out<<' '<<i;
        out<<" ) normal_idxs ( "<<triangles;
        for(int i=0;i<triangles;++i) out<<" 0 3";
        out<<" ) flags ( "<<triangles;
        for(int i=0;i<triangles;++i) out<<" 00000000";
        out<<" ) ) ) ) ) )\r\n";
    }
    out<<" ) ) ) )\r\n"; out.flush();
    if(out.status()!=QTextStream::Ok || !file.commit()) { error="Cannot publish wire shape."; return false; }
    QSaveFile sd(shapes+name+"d");
    if(!sd.open(QIODevice::WriteOnly)) { error=sd.errorString(); return false; }
    QTextStream descriptor(&sd); descriptor.setEncoding(QStringConverter::Utf16LE); descriptor.setGenerateByteOrderMark(true);
    descriptor<<"SIMISA@@@@@@@@@@JINX0t1t______\r\nshape ( "<<name<<" ESD_Detail_Level ( 0 ) ESD_Alternative_Texture ( 0 ) )\r\n";
    descriptor.flush();
    if(descriptor.status()!=QTextStream::Ok || !sd.commit()) { error="Cannot publish wire descriptor."; return false; }
    return true;
}
}

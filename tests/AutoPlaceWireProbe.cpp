#include "AutoPlaceWire.h"
#include "AutoPlaceRuler.h"
#include "WireAssetScan.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QtEndian>
#include <iostream>
#include <cstdlib>
#include <cmath>

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    auto require=[](bool ok,const char *message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } };
    {
        QString error;
        const QVector<QVector3D> line{{0,0,0},{12,6,0}};
        const auto straight=AutoPlaceRuler::plan(line,5,false,error);
        require(error.isEmpty() && straight.size()==4,"AP ruler must place both endpoints and ordinary spacing");
        require(straight[1].point==QVector3D(5,2.5,0) && straight[2].point==QVector3D(10,5,0)
            && straight.back().point==QVector3D(12,6,0),"AP ruler spacing must use horizontal length and interpolate height");
        const QVector<QVector3D> bend{{0,0,0},{7,0,0},{7,0,9}};
        const auto ordinary=AutoPlaceRuler::plan(bend,5,false,error);
        require(ordinary.size()==5 && ordinary[2].point==QVector3D(7,0,3),"normal spacing must carry through an unpreserved bend");
        require(ordinary[1].direction.x()>ordinary[1].direction.z() && ordinary[1].direction.z()>0
            && ordinary[2].direction.z()>ordinary[2].direction.x() && ordinary[2].direction.x()>0,
            "ordinary posts on both sides of a bend must turn toward their neighboring spans");
        const auto centred=AutoPlaceRuler::plan({{0,0,0},{10,0,0},{10,0,10}},5,false,error);
        require((centred[2].direction-QVector3D(float(std::sqrt(0.5)),0,float(std::sqrt(0.5)))).length()<1.e-5f,
            "ordinary post exactly on a direction change must bisect the meeting spans");
        require(centred.front().direction==QVector3D(1,0,0) && centred.back().direction==QVector3D(0,0,1),
            "end posts must face their sole adjacent span");
        const auto preserved=AutoPlaceRuler::plan(bend,5,true,error);
        require(preserved.size()==5 && preserved[2].corner && preserved[2].point==QVector3D(7,0,0)
            && preserved[3].point==QVector3D(7,0,5),"preserve corners must force the corner and restart spacing there");
        require(preserved[2].direction.x()>0.7f && preserved[2].direction.z()>0.7f,
            "corner post must use the angle bisector");
        const auto cornerCount=[&](const QVector<QVector3D> &points) {
            const auto plan=AutoPlaceRuler::plan(points,4,true,error);
            int count=0; for(const auto &station:plan) if(station.corner) ++count;
            return count;
        };
        require(cornerCount({{0,0,0},{10,0,0},{20,0,10}})==1,"45-degree turn must preserve its corner");
        require(cornerCount({{0,0,0},{10,0,0},{20,0,5}})==0,"shallow turn must retain ordinary spacing");
        require(cornerCount({{0,0,0},{10,0,0},{5,0,10}})==0,"turn beyond 90 degrees must not force a corner");
        const auto nearEnd=AutoPlaceRuler::plan({{0,0,0},{10.2f,0,0}},5,false,error);
        require(nearEnd.size()==3 && nearEnd.back().point.x()==10.2f,"exact endpoint must replace a nearly coincident regular station");
        const auto crossedTile=AutoPlaceRuler::plan({{1020,0,0},{1030,0,0}},5,false,error);
        require(crossedTile.size()==3 && crossedTile[1].point.x()==1025,"AP ruler must retain continuous positions across a tile edge");
        require(AutoPlaceRuler::plan({{0,0,0},{0,2,0}},5,true,error).isEmpty() && !error.isEmpty(),
            "vertical/duplicate guide must not create duplicate posts");
        require(AutoPlaceRuler::plan(line,0,true,error).isEmpty() && !error.isEmpty(),"invalid AP ruler spacing must be rejected");
        require(AutoPlaceRuler::plan({{0,0,0},{100000,0,0}},1,true,error).isEmpty()
            && !error.isEmpty(),"oversized AP ruler batches must be refused before placement");
    }
    require(AutoPlaceWire::matchesSupportPrefix("SCOsnapPole.s", "SCOsnapPole.s"),
        "selected pole excluded");
    require(AutoPlaceWire::matchesSupportPrefix("SCOsnapPole_U.s", "SCOsnapPole.s"),
        "manually placed underground variant excluded");
    require(AutoPlaceWire::matchesSupportPrefix("scosnappole_U.S", "SCOsnapPole.S"),
        "support prefix must ignore case and shape extension");
    require(AutoPlaceWire::matchesSupportPrefix("SCOsnapPoleExtra.s", "SCOsnapPole.s"),
        "support suffix must not require an underscore");
    require(!AutoPlaceWire::matchesSupportPrefix("SCOsnapFence.s", "SCOsnapPole.s"),
        "nearby fence must not enter the pole run");
    require(!AutoPlaceWire::matchesSupportPrefix("SCOsnapPole.s", "SCOsnapPole_U.s"),
        "variant selection must not widen to the base family");
    require(!AutoPlaceWire::matchesSupportPrefix("OtherSCOsnapPole.s", "SCOsnapPole.s"),
        "support family must match at the start");
    require(AutoPlaceWire::matchesSupportPrefix("SCOsnapPole.v2_U.s", "SCOsnapPole.v2.s"),
        "dots inside support names must be preserved");
    require(!AutoPlaceWire::matchesSupportPrefix("SCOsnapPole.s", "")
        && !AutoPlaceWire::matchesSupportPrefix("SCOsnapPole.s", ".s"),
        "empty selection must not match all supports");
    const QJsonObject rawPreview{{"active",true},{"baked",false}};
    require(AutoPlaceWire::isPending(rawPreview), "raw blue wire must prompt before exit");
    QJsonObject hiddenPreview = rawPreview; hiddenPreview["active"] = false;
    require(AutoPlaceWire::isRaw(hiddenPreview) && !AutoPlaceWire::isPending(hiddenPreview),
        "hidden raw wire must remain a discard target without being a blue preview");
    require(!AutoPlaceWire::isRaw(QJsonObject()), "missing registry record is not raw work");
    const QJsonObject savedBake{{"baked",true},{"active",true},{"uid",42}};
    QJsonObject deletedBake = savedBake; deletedBake["deleted"] = true;
    const QJsonObject states{{"saved",savedBake},{"hidden",hiddenPreview},
        {"external",QJsonObject{{"external",true},{"active",true}}},
        {"deleted",deletedBake},{"raw",rawPreview}};
    require(AutoPlaceWire::pendingBakeKeys(states) == QStringList{"raw"},
        "bake pass must select only active raw spans");
    QJsonObject afterBake = states; afterBake["raw"] = savedBake;
    require(AutoPlaceWire::pendingBakeKeys(afterBake).isEmpty(),
        "bake followed by terrain-only save must not schedule another bake");
    const auto reloadedBakes = QJsonDocument::fromJson(
        QJsonDocument(afterBake).toJson()).object();
    require(AutoPlaceWire::pendingBakeKeys(reloadedBakes).isEmpty(),
        "saved/reloaded bakes must not schedule another bake");
    afterBake["newRaw"] = rawPreview;
    require(AutoPlaceWire::pendingBakeKeys(afterBake) == QStringList{"newRaw"},
        "new work and failed-bake retry must exclude completed spans");
    require(!AutoPlaceWire::hasPendingCleanup(QJsonObject{{"saved",savedBake}},false),
        "unchanged saved wires must not start cleanup");
    require(AutoPlaceWire::hasPendingCleanup(states,false)
        && AutoPlaceWire::hasPendingCleanup({},true),
        "pending deletion and global cleanup must remain retryable");
    for(const QString &state : {QString("baked"),QString("external"),QString("deleted")}) {
        QJsonObject protectedRecord = rawPreview; protectedRecord[state] = true;
        require(!AutoPlaceWire::isRaw(protectedRecord) && !AutoPlaceWire::isPending(protectedRecord),
            "saved, external or deleted wires cannot become raw discard targets");
    }
    const QJsonObject reservation{{"external",true},{"baked",true},{"active",true},
        {"shape","LegacyWire.s"},{"uid",123},{"key","test-pair"}};
    require(AutoPlaceWire::reconcileReservation(reservation,true)==reservation,
        "existing external wire must keep duplicate reservation");
    const auto released=AutoPlaceWire::reconcileReservation(reservation,false);
    require(!released["baked"].toBool() && !released.contains("external")
        && !released.contains("shape") && !released.contains("uid") && !released["active"].toBool()
        && released["key"]==reservation["key"],"deleted external wire still blocks Commit or silently reactivates");
    require(AutoPlaceWire::reconcileReservation(released,false)==released,"reservation release is not idempotent");
    QJsonObject pending{{"baked",true},{"active",true},{"uid",124}};
    const auto recovered=AutoPlaceWire::reconcileReservation(pending,false);
    require(!recovered["baked"].toBool() && recovered["active"].toBool(),"pending native bake recovery lost");
    pending["deleted"]=true;
    require(AutoPlaceWire::reconcileReservation(pending,false)==pending,"deletion tombstone changed");
    auto connection=[](int a,int b,int node,bool road=false) {
        return QJsonObject{{"poleA",QJsonObject{{"x",0},{"z",0},{"uid",a}}},
            {"poleB",QJsonObject{{"x",0},{"z",0},{"uid",b}}},{"node",node},{"road",road}};
    };
    QJsonObject graph{{"ab",connection(1,2,7)},{"bc",connection(2,3,7)},
        {"cd",connection(3,4,8)},{"road",connection(3,5,7,true)},
        {"separate",connection(8,9,7)}};
    auto deleted=connection(3,6,7); deleted["deleted"]=true; graph["deleted"]=deleted;
    auto external=connection(3,7,7); external["external"]=true; graph["external"]=external;
    require(AutoPlaceWire::connectedSpans(graph,"0,0,2",7,false)==QSet<QString>{"ab","bc"},
        "selected run crossed a node, database, deletion, ownership or disconnected boundary");
    require(AutoPlaceWire::connectedSpans(graph,"0,0,42",7,false).isEmpty(),"unrelated pole selected wires");
    graph["rulerA"]=connection(20,21,-1); graph["rulerB"]=connection(21,22,-1);
    graph["otherRuler"]=connection(30,31,-1);
    graph["trackBesideRuler"]=connection(22,23,7);
    require(AutoPlaceWire::connectedSpans(graph,"0,0,21",-1,false)==QSet<QString>{"rulerA","rulerB"},
        "ruler wire deletion must follow its connected supports without entering unrelated runs or tracks");
    QJsonArray wires;
    for(int i=0;i<16;++i) wires.append(QJsonObject{{"a",QJsonArray{double(i)*0.2,8.,0.}},
        {"b",QJsonArray{double(i)*0.2,8.,50.}}});
    QJsonObject span{{"key","1,2,3|1,2,4"},{"width",20.},{"sag",1.5},{"wires",wires}};
    auto near=AutoPlaceWire::mesh(span), far=AutoPlaceWire::mesh(span,true);
    require(near.size()==992*3,"near triangle count");
    require(far.size()==320*3,"far triangle count");
    // Every near mesh edge belongs to exactly two faces, including the two end caps.
    QMap<QString,int> edges;
    auto pointKey=[](QVector3D p) { return QString("%1,%2,%3").arg(p.x(),0,'g',9).arg(p.y(),0,'g',9).arg(p.z(),0,'g',9); };
    for(int i=0;i<near.size();i+=3) {
        auto normal=QVector3D::crossProduct(near[i+1].point-near[i].point,near[i+2].point-near[i].point).normalized();
        require(QVector3D::dotProduct(normal,near[i].normal)>0.999f,"normal/winding disagreement");
        for(int j=0;j<3;++j) {
            auto a=pointKey(near[i+j].point), b=pointKey(near[i+(j+1)%3].point);
            if(b<a) qSwap(a,b);
            ++edges[a+'|'+b];
        }
    }
    for(int count:edges) require(count==2,"triangle wire is not closed");
    // All far normals are horizontal: one upright double-sided face, no cruciform.
    for(const auto &v:far) require(qAbs(v.normal.y())<0.0001f,"far ribbon is not vertical");
    QTemporaryDir dir; require(dir.isValid(),"temporary directory");
    QString error;
    require(AutoPlaceWire::writeShape(dir.path(),span,error),qPrintable(error));
    QFile dds(dir.path()+"/TEXTURES/APWireCharcoal_v1.dds"); require(dds.open(QIODevice::ReadOnly),"DDS missing");
    auto bytes=dds.readAll();
    require(bytes.size()==212 && bytes.left(4)=="DDS ","DDS size/header");
    require(qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()+28))==3,"DDS must declare all three mip levels");
    for(int i=128;i<bytes.size();i+=4) require(uchar(bytes[i])==64 && uchar(bytes[i+1])==62 && uchar(bytes[i+2])==60 && uchar(bytes[i+3])==255,"DDS mip pixel");
    QFile shape(dir.path()+"/SHAPES/"+AutoPlaceWire::shapeName(span["key"].toString()));
    require(shape.open(QIODevice::ReadOnly),"shape missing"); auto raw=shape.readAll();
    require(raw.startsWith(QByteArray::fromHex("fffe")),"UTF16 BOM");
    QString text=QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData()+2),(raw.size()-2)/2);
    int depth=0; for(QChar c:text) { if(c=='(') ++depth; if(c==')') --depth; require(depth>=0,"unbalanced shape"); }
    require(depth==0 && text.contains("dlevel_selection ( 300 )") && text.contains("dlevel_selection ( 750 )"),"LOD serialization");
    // Windows must be able to atomically replace the inspected file on retry.
    shape.close();
    dds.close();
    require(AutoPlaceWire::writeShape(dir.path(),span,error),qPrintable(error));
    require(AutoPlaceWire::writeRegistry(dir.path()+"/wires.json",{{"version",1},{"spans",QJsonObject{{span["key"].toString(),span}}}},error),"registry write");
    auto explicitPlain=span; explicitPlain["wireType"]="plain";
    const auto plainMesh=AutoPlaceWire::mesh(explicitPlain);
    require(plainMesh.size()==near.size(),"explicit plain style changed legacy geometry size");
    for(int i=0;i<near.size();++i)
        require(plainMesh[i].point==near[i].point && plainMesh[i].normal==near[i].normal
            && plainMesh[i].uv==near[i].uv,"missing type must preserve legacy plain geometry exactly");
    auto barbed=span;
    barbed["key"]="barbed-wire-style"; barbed["wireType"]="barbed";
    barbed["width"]=3.0; barbed["barbSpacing"]=0.25;
    barbed["wires"]=QJsonArray{wires.first()};
    const auto barbedNear=AutoPlaceWire::mesh(barbed), barbedFar=AutoPlaceWire::mesh(barbed,true);
    require(barbedNear.size()==24 && barbedFar.size()==24,"barbed wire must be two double-sided alpha strips, eight triangles per span");
    auto obsoleteSettings=barbed;
    obsoleteSettings["sag"]=10.; obsoleteSettings["barbSpacing"]=1.;
    const auto unchanged=AutoPlaceWire::mesh(obsoleteSettings);
    bool sidePlane=false, upPlane=false, varyingUv=false;
    for(int i=0;i<barbedNear.size();++i) {
        const auto &v=barbedNear[i];
        require(v.point==unchanged[i].point && v.normal==unchanged[i].normal && v.uv==unchanged[i].uv,
            "obsolete sag/spacing fields must not affect textured barbed wire");
        require(v.point.z()==0 || v.point.z()==50,"straight barbed strip must not contain sag samples");
        require(std::isfinite(v.point.x()) && std::isfinite(v.point.y()) && std::isfinite(v.point.z())
            && std::isfinite(v.uv.x()) && std::isfinite(v.uv.y()),"alpha strip has nonfinite geometry or UVs");
        sidePlane |= qAbs(v.point.x())>0.02f;
        upPlane |= qAbs(v.point.y()-8)>0.02f;
        varyingUv |= v.uv!=barbedNear.first().uv;
    }
    require(sidePlane && upPlane && varyingUv,"barbed wire requires perpendicular strips and repeated artwork UVs");
    auto thickerSettings=barbed, thinnerSettings=barbed;
    thickerSettings["width"]=6.0; thinnerSettings["width"]=1.0;
    const auto thicker=AutoPlaceWire::mesh(thickerSettings), thinner=AutoPlaceWire::mesh(thinnerSettings);
    require(thicker.size()==barbedNear.size() && thinner.size()==barbedNear.size(),
        "barbed width must not change the geometry budget");
    for(int i=0;i<barbedNear.size();++i) {
        const QVector3D centre(0,8,barbedNear[i].point.z());
        const auto offset=barbedNear[i].point-centre;
        require(((thicker[i].point-centre)-offset*2).length()<1.e-5f
            && ((thinner[i].point-centre)-offset/3).length()<1.e-5f,
            "barbed thickness must scale the silhouette around its unchanged centre line");
        require(thicker[i].uv==barbedNear[i].uv && thinner[i].uv==barbedNear[i].uv,
            "barbed thickness must preserve repeat spacing and endpoint UVs");
    }
    for(int i=0;i<barbedNear.size();i+=3) {
        const auto normal=QVector3D::crossProduct(barbedNear[i+1].point-barbedNear[i].point,
            barbedNear[i+2].point-barbedNear[i].point).normalized();
        require(QVector3D::dotProduct(normal,barbedNear[i].normal)>0.999f,"crossed alpha strip normal/winding disagreement");
    }
    const auto guide=AutoPlaceWire::previewMesh(barbed);
    require(guide.size()==62*3,"raw barbed definitions require a visible plain blue centre-line guide");
    auto longBarbed=barbed;
    longBarbed["wires"]=QJsonArray{QJsonObject{{"a",QJsonArray{0.,0.,0.}},{"b",QJsonArray{0.,0.,200.}}}};
    require(AutoPlaceWire::mesh(longBarbed).size()==24,"barbed geometry must stay constant for long spans");
    require(AutoPlaceWire::writeShape(dir.path(),barbed,error),qPrintable(error));
    QFile brown(dir.path()+"/TEXTURES/APWireBarbedBrown_v4.dds");
    require(brown.open(QIODevice::ReadOnly),"barbed alpha texture missing");
    const QByteArray brownBytes=brown.readAll(); brown.close();
    require(brownBytes.size()==128+43691*4 && brownBytes.left(4)=="DDS "
        && qFromLittleEndian<quint32>(brownBytes.constData()+12)==128
        && qFromLittleEndian<quint32>(brownBytes.constData()+16)==256
        && qFromLittleEndian<quint32>(brownBytes.constData()+28)==9
        && qFromLittleEndian<quint32>(brownBytes.constData()+88)==32
        && qFromLittleEndian<quint32>(brownBytes.constData()+104)==0xff000000u,
        "barbed artwork requires a full RGBA mip chain");
    QSet<quint32> steelColors;
    bool transparent=false, opaque=false, upperBarbs=false, lowerBarbs=false;
    int top=128,bottom=-1;
    for(int y=0;y<128;++y) for(int x=0;x<256;++x) {
        const int at=128+4*(y*256+x);
        const int b=uchar(brownBytes[at]), g=uchar(brownBytes[at+1]), r=uchar(brownBytes[at+2]), a=uchar(brownBytes[at+3]);
        transparent |= a==0; opaque |= a==255;
        upperBarbs |= a>128 && y<48; lowerBarbs |= a>128 && y>80;
        if(a>128) { top=qMin(top,y); bottom=qMax(bottom,y); }
        if(a>200) {
            require(r>=g && g>=b && r<130,"visible artwork must be native brown metal rather than bright orange rust");
            steelColors.insert(quint32(r<<16|g<<8|b));
        }
    }
    require(transparent && opaque && upperBarbs && lowerBarbs && steelColors.size()>32,
        "alpha artwork must contain transparent gaps, a visible wire, short barbs and varied brown metal");
    require(top>=34 && top<=37 && bottom>=90 && bottom<=93,
        "barb artwork must use half-length ends with doubled thickness");
    int narrowest=256,widest=0;
    for(int y:{42,47,52}) {
        int coverage=0;
        for(int x=0;x<128;++x) if(uchar(brownBytes[128+4*(y*256+x)+3])>128) ++coverage;
        narrowest=qMin(narrowest,coverage); widest=qMax(widest,coverage);
    }
    require(narrowest>=7 && widest<=10 && widest-narrowest<=2,
        "barb wire must keep its doubled diameter through to the cut end");
    int strandWidth=0;
    for(int y=0;y<128;++y) if(uchar(brownBytes[128+4*(y*256)+3])>128) ++strandWidth;
    require(strandWidth>=5 && strandWidth<=7,"main strand must retain its original thickness");
    const int centre=128+4*(64*256);
    require(uchar(brownBytes[centre+3])==255 && uchar(brownBytes[centre+255*4+3])==255,
        "wire artwork must meet seamlessly at both repeat edges");
    QFile barbedShape(dir.path()+"/SHAPES/"+AutoPlaceWire::shapeName(barbed["key"].toString()));
    require(barbedShape.open(QIODevice::ReadOnly),"barbed shape missing");
    const auto barbedRaw=barbedShape.readAll(); barbedShape.close();
    const auto barbedText=QString::fromUtf16(reinterpret_cast<const char16_t*>(barbedRaw.constData()+2),(barbedRaw.size()-2)/2);
    require(barbedText.contains("APWireBarbedBrown_v4.dds") && barbedText.contains("BlendATexDiff")
        && barbedText.contains("vtx_state ( 00000000 0 -9")
        && barbedText.contains("tex_idxs ( 1 0 ) 0 0 1 0 1")
        && barbedText.contains(QString("uv_points ( %1").arg(barbedNear.size()+barbedFar.size())),
        "barbed bake must serialize alpha shader/state, repeated artwork and per-vertex UVs");
    require(AutoPlaceWire::writeShape(dir.path(),barbed,error),"barbed retry must reuse immutable alpha texture");
    require(AutoPlaceWire::writeRegistry(dir.path()+"/barbed.json",{{"version",1},{"spans",QJsonObject{{"barbed",barbed}}}},error),
        "barbed registry write");
    QFile barbedRegistry(dir.path()+"/barbed.json"); require(barbedRegistry.open(QIODevice::ReadOnly),"barbed registry read");
    const auto loadedBarbed=QJsonDocument::fromJson(barbedRegistry.readAll()).object()["spans"].toObject()["barbed"].toObject();
    require(AutoPlaceWire::isBarbed(loadedBarbed),"barbed type must survive registry reload");
    QJsonObject maximumSpans;
    for(int i=0;i<AutoPlaceWire::MaximumSpanCount;++i)
        maximumSpans.insert(QString::number(i), QJsonObject{{"key",QString::number(i)}});
    const QString limitPath = dir.path()+"/registry-limit.json";
    require(AutoPlaceWire::writeRegistry(limitPath,
        {{"version",1},{"spans",maximumSpans}},error), "registry limit boundary rejected");
    QFile limitFile(limitPath);
    require(limitFile.open(QIODevice::ReadOnly), "limit registry read");
    const QByteArray acceptedRegistry = limitFile.readAll();
    limitFile.close();
    maximumSpans.insert("overflow", QJsonObject{{"key","overflow"}});
    require(!AutoPlaceWire::writeRegistry(limitPath,
        {{"version",1},{"spans",maximumSpans}},error) && !error.isEmpty(),
        "oversized registry published");
    require(limitFile.open(QIODevice::ReadOnly)
        && limitFile.readAll()==acceptedRegistry, "rejected registry changed accepted file");
    limitFile.close();
    maximumSpans.remove("overflow");
    require(AutoPlaceWire::writeRegistry(limitPath,
        {{"version",1},{"spans",maximumSpans}},error) && error.isEmpty(),
        "registry retry after rejected size failed");
    QJsonArray expandedWires = wires;
    for(int i=16;i<24;++i) expandedWires.append(QJsonObject{{"a",QJsonArray{double(i)*0.2,8.,0.}},
        {"b",QJsonArray{double(i)*0.2,8.,50.}},{"from",i+1},{"to",i+1}});
    auto expandedSpan = span;
    expandedSpan["key"] = "24-wire-regression";
    expandedSpan["wires"] = expandedWires;
    require(AutoPlaceWire::mesh(expandedSpan).size()==24*62*3, "24-wire near mesh truncated");
    require(AutoPlaceWire::mesh(expandedSpan,true).size()==24*20*3, "24-wire far mesh truncated");
    require(AutoPlaceWire::writeShape(dir.path(),expandedSpan,error),qPrintable(error));
    const QJsonObject expandedRegistry{{"version",1},{"spans",QJsonObject{{"24-wire-regression",expandedSpan}}}};
    require(AutoPlaceWire::writeRegistry(dir.path()+"/24-wires.json",expandedRegistry,error),"24-wire registry write");
    QFile expandedFile(dir.path()+"/24-wires.json");
    require(expandedFile.open(QIODevice::ReadOnly),"24-wire registry read");
    require(QJsonDocument::fromJson(expandedFile.readAll()).object()==expandedRegistry,"24-wire registry round trip");
    QTemporaryDir cleanup;
    require(QDir().mkpath(cleanup.path()+"/SHAPES") && QDir().mkpath(cleanup.path()+"/WORLD")
        && QDir().mkpath(cleanup.path()+"/TEXTURES"),"cleanup fixture directories");
    auto put=[&](const QString &path,const QByteArray &data) {
        QFile file(cleanup.path()+path); require(file.open(QIODevice::WriteOnly),"fixture open");
        require(file.write(data)==data.size(),"fixture write");
    };
    const QString first=AutoPlaceWire::shapeName("first"), second=AutoPlaceWire::shapeName("second");
    put("/SHAPES/"+first,"shape"); put("/SHAPES/"+first+"d","descriptor");
    put("/SHAPES/"+second,"shape"); put("/SHAPES/"+second+"d","descriptor");
    put("/SHAPES/SCO_TelephoneWire_OR_Test.s","legacy");
    put("/SHAPES/SCO_TelephoneWire_OR_Test.sd","legacy descriptor");
    put("/SHAPES/Pole.s","pole");
    put("/TEXTURES/APWireCharcoal_v1.dds","texture");
    put("/TEXTURES/SCO_TelephoneWire_OR_Test.dds","texture");
    QJsonObject cleanupSpans{{"first",QJsonObject{{"deleted",true}}}};
    put("/WORLD/test.w",("SIMISA@@@@@@@@@@JINX0w0t______\nTr_Worldfile ( FileName ( "+first+" ) )").toUtf8());
    require(!AutoPlaceWire::pruneAssets(cleanup.path(),cleanupSpans,false,error)
        && QFile::exists(cleanup.path()+"/SHAPES/"+first),"cleanup deleted a saved reference");
    put("/WORLD/test.w","SIMISA@@@@@@@@@@JINX0w0t______\nTr_Worldfile ( )");
    require(AutoPlaceWire::pruneAssets(cleanup.path(),cleanupSpans,false,error),"scoped cleanup failed");
    require(cleanupSpans.isEmpty() && !QFile::exists(cleanup.path()+"/SHAPES/"+first)
        && !QFile::exists(cleanup.path()+"/SHAPES/"+first+"d")
        && QFile::exists(cleanup.path()+"/SHAPES/"+second),"scoped cleanup crossed run boundary");
    // A surviving shape using the shared texture must protect it even during reset.
    put("/TEXTURES/APWireBrownSteel_v1.dds","shared brown steel texture");
    put("/TEXTURES/APWireBarbedBrown_v2.dds","legacy tapered barbed texture");
    put("/TEXTURES/APWireBarbedBrown_v3.dds","shared barbed alpha texture");
    put("/TEXTURES/APWireBarbedBrown_v4.dds","short thick barbed texture");
    put("/SHAPES/Pole.s","shape ( APWireCharcoal_v1.dds APWireBrownSteel_v1.dds APWireBarbedBrown_v2.dds APWireBarbedBrown_v3.dds APWireBarbedBrown_v4.dds )");
    require(AutoPlaceWire::pruneAssets(cleanup.path(),cleanupSpans,true,error),"global cleanup failed");
    require(!QFile::exists(cleanup.path()+"/SHAPES/"+second)
        && !QFile::exists(cleanup.path()+"/SHAPES/SCO_TelephoneWire_OR_Test.s")
        && !QFile::exists(cleanup.path()+"/TEXTURES/SCO_TelephoneWire_OR_Test.dds")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireCharcoal_v1.dds")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireBrownSteel_v1.dds")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v2.dds")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v3.dds")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v4.dds")
        && QFile::exists(cleanup.path()+"/SHAPES/Pole.s"),"global cleanup lost shared assets or left legacy files");
    put("/SHAPES/Pole.s","pole");
    require(AutoPlaceWire::pruneAssets(cleanup.path(),cleanupSpans,true,error)
        && !QFile::exists(cleanup.path()+"/TEXTURES/APWireCharcoal_v1.dds")
        && !QFile::exists(cleanup.path()+"/TEXTURES/APWireBrownSteel_v1.dds")
        && !QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v2.dds")
        && !QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v3.dds")
        && !QFile::exists(cleanup.path()+"/TEXTURES/APWireBarbedBrown_v4.dds"),"unused shared texture retained");
    require(!AutoPlaceWire::isWireShape("../APW_000000000000000000000000.s")
        && !AutoPlaceWire::isWireShape("APW_user_shape.s"),"unsafe cleanup name accepted");
    // Discard only this session's bakes, protecting even a partial save.
    put("/SHAPES/"+first,"shape"); put("/SHAPES/"+first+"d","descriptor");
    put("/SHAPES/"+second,"shape"); put("/SHAPES/"+second+"d","descriptor");
    const QJsonObject savedRecord{{"baked",true},{"active",true},{"uid",42}};
    QJsonObject discardSpans{{"first",savedRecord},{"second",savedRecord},
        {"raw",QJsonObject{{"active",true}}},{"external",reservation}};
    const auto originalDiscardSpans = discardSpans;
    const QList<QByteArray> invalidWorlds = {
        "invalid world", "", "Tr_Worldfile", "Tr_Worldfile (",
        "Tr_Worldfile ( Static ( )", "Tr_Worldfile ( ) )",
        "Tr_Worldfile ( FileName ( \"unterminated ) )",
        "Tr_Worldfile ( ) trailing", "Tr_WorldfileExtra ( )",
        "Tr_Worldfile ( ) Comment ( unfinished",
        "Tr_Worldfile ( ) Comment ( \"unfinished )",
        "Tr_Worldfile ( ) Comment ( complete ) trailing",
        "Tr_Worldfile ( ) Commentary ( not a comment token )",
        "Tr_Worldfile ( ) Comment ( complete ) Static ( )",
        "Tr_Worldfile ( ) Comment ( complete ) Tr_Worldfile ( )",
        "Tr_Worldfile ( ) Comment ( complete ) )",
        "SIMISA@@@@@@@@@@JINX0w0t______\nNotAWorld ( )",
        QByteArray("Tr_Worldfile ( \0 )", 18),
        QByteArray::fromHex("fffe5400ff")
    };
    // A valid earlier tile must not permit deletion before a later failure.
    put("/WORLD/aaa.w", "Tr_Worldfile ( )");
    put("/TEXTURES/APWireCharcoal_v1.dds", "shared texture");
    auto rejectWorld = [&](const QByteArray &invalid) {
        put("/WORLD/test.w", invalid);
        require(!AutoPlaceWire::discardBakes(cleanup.path(),discardSpans,{"first","second"},error)
            && !error.isEmpty() && discardSpans == originalDiscardSpans,
            "discard did not stop before deleting on unreadable world");
        QJsonObject deleted{{"first",QJsonObject{{"deleted",true}}}};
        const auto originalDeleted = deleted;
        require(!AutoPlaceWire::pruneAssets(cleanup.path(),deleted,true,error)
            && !error.isEmpty() && deleted == originalDeleted,
            "global cleanup accepted a malformed world");
        for(const QString &name : {first, first+"d", second, second+"d"})
            require(QFile::exists(cleanup.path()+"/SHAPES/"+name),
                "malformed world cleanup removed a generated asset");
        require(QFile::exists(cleanup.path()+"/TEXTURES/APWireCharcoal_v1.dds"),
            "malformed world cleanup removed shared texture");
    };
    for(const auto &invalid : invalidWorlds) rejectWorld(invalid);
    rejectWorld(QByteArray("Tr_Worldfile ( ) Comment ( complete )") + QByteArray(1, '\0'));
    rejectWorld(QByteArray("Tr_Worldfile ( )") + QByteArray::fromHex("e2"));
    // Accept complete worlds in supported text encodings, with quoted
    // parentheses/escaped quotes and balanced comment blocks.
    const QString validText = "SIMISA@@@@@@@@@@JINX0w0t______\nTr_Worldfile ( "
        "Comment ( \"quoted ) ( and \\\"quote\\\"\" ) Static ( FileName ( " + first + " ) ) )"
        "\nComment (Legacy utility - Version: 1.2 (build 3), Filetype='World')"
        "\ncOmMeNt ( \"quoted ) ( and \\\"quote\\\"\" )\n";
    QStringEncoder le(QStringEncoder::Utf16LE), be(QStringEncoder::Utf16BE);
    QList<QByteArray> validWorlds{validText.toUtf8(),
        QByteArray::fromHex("efbbbf") + validText.toUtf8(),
        QByteArray::fromHex("fffe") + le(validText),
        QByteArray::fromHex("feff") + be(validText)};
    const QByteArray payload = validText.toUtf8().mid(16);
    QByteArray compressed("SIMISA@F", 8);
    char sizeBytes[4];
    qToLittleEndian<quint32>(quint32(payload.size()), sizeBytes);
    compressed.append(sizeBytes, 4); compressed.append("@@@@", 4);
    compressed.append(qCompress(payload).mid(4));
    validWorlds.append(compressed);
    const QByteArray utf16Payload = le(validText.mid(16));
    QByteArray compressedUtf16 = QByteArray::fromHex("fffe") + le(u"SIMISA@@@@@@@@@@");
    compressedUtf16[16] = 'F';
    qToLittleEndian<quint32>(quint32(utf16Payload.size()), sizeBytes);
    compressedUtf16[13] = sizeBytes[0]; compressedUtf16[17] = sizeBytes[1];
    compressedUtf16[18] = sizeBytes[2]; compressedUtf16[19] = sizeBytes[3];
    compressedUtf16.append(qCompress(utf16Payload).mid(4));
    validWorlds.append(compressedUtf16);
    QByteArray binaryPayload(1, '\0');
    binaryPayload.append(le(first));
    QByteArray binaryWorld("SIMISA@@@@@@@@@@JINX0w0b______\r\n");
    qToLittleEndian<quint32>(261844u + 375u, sizeBytes);
    binaryWorld.append(sizeBytes, 4);
    qToLittleEndian<quint32>(quint32(binaryPayload.size()), sizeBytes);
    binaryWorld.append(sizeBytes, 4); binaryWorld.append(binaryPayload);
    validWorlds.append(binaryWorld);
    QByteArray compressedBinary("SIMISA@F", 8);
    qToLittleEndian<quint32>(quint32(binaryWorld.size()-16), sizeBytes);
    compressedBinary.append(sizeBytes, 4); compressedBinary.append("@@@@", 4);
    compressedBinary.append(qCompress(binaryWorld.mid(16)).mid(4));
    validWorlds.append(compressedBinary);
    // Discovery must find native/legacy bakes without loading unrelated
    // scenery tiles, including binary strings at odd byte offsets.
    QTemporaryDir discovery;
    require(QDir().mkpath(discovery.path()+"/WORLD"),"wire discovery directory");
    auto putDiscovery=[&](const QString &name,const QByteArray &data) {
        QFile file(discovery.path()+"/WORLD/"+name);
        require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"wire discovery fixture");
    };
    putDiscovery("w+000009+000009.w","Tr_Worldfile ( Static ( FileName ( Pole.s ) ) )");
    QVector<QPair<int,int>> wireTiles;
    int progressCalls=0;
    auto reportDiscovery=[&](const QString &,int,int) { ++progressCalls; };
    for(const auto &valid:validWorlds) {
        putDiscovery("w+000001-000002.w",valid);
        require(AutoPlaceWire::findWireWorldTiles(discovery.path(),wireTiles,error,reportDiscovery)
                && wireTiles==QVector<QPair<int,int>>{{1,2}},
                "wire discovery missed supported encoding or included unrelated scenery");
    }
    require(progressCalls==validWorlds.size()*2,"wire discovery did not report file progress");
    putDiscovery("w+000001-000002.w","Tr_Worldfile ( Static ( FileName ( SCO_TelephoneWire_OR_Test_09.s ) ) )");
    require(AutoPlaceWire::findWireWorldTiles(discovery.path(),wireTiles,error)
            && wireTiles==QVector<QPair<int,int>>{{1,2}},"legacy wire discovery missed");
    putDiscovery("w+000009+000009.w","Tr_Worldfile (");
    require(!AutoPlaceWire::findWireWorldTiles(discovery.path(),wireTiles,error)
            && wireTiles.isEmpty() && !error.isEmpty(),"discovery published partial results on malformed world");
    putDiscovery("w+000009+000009.w","Tr_Worldfile ( )");
    putDiscovery("invalid.w",validWorlds.first());
    require(!AutoPlaceWire::findWireWorldTiles(discovery.path(),wireTiles,error)
            && wireTiles.isEmpty(),"discovery accepted an invalid wire tile filename");
    QTemporaryDir deferredCleanup;
    require(QDir().mkpath(deferredCleanup.path()+"/WORLD")
            && QDir().mkpath(deferredCleanup.path()+"/SHAPES")
            && QDir().mkpath(deferredCleanup.path()+"/TEXTURES"),"deferred cleanup directories");
    auto putDeferred=[&](const QString &path,const QByteArray &data) {
        QFile file(deferredCleanup.path()+path);
        require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"deferred cleanup fixture");
    };
    putDeferred("/WORLD/w+000001-000002.w",validWorlds.first());
    putDeferred("/SHAPES/"+first,"shape");
    putDeferred("/SHAPES/Pole.s",compressed.chopped(4));
    putDeferred("/TEXTURES/APWireCharcoal_v1.dds","shared texture");
    QJsonObject deferredSpans{{"first",QJsonObject{{"deleted",true}}}};
    require(!AutoPlaceWire::pruneAssets(deferredCleanup.path(),deferredSpans,true,error)
            && error=="Some wire assets are still referenced by saved world objects; retained for retry."
            && QFile::exists(deferredCleanup.path()+"/TEXTURES/APWireCharcoal_v1.dds")
            && QFile::exists(deferredCleanup.path()+"/SHAPES/"+first),
            "pre-save cleanup scanned unrelated shapes or deleted saved wire assets");
    rejectWorld(binaryWorld.chopped(1));
    rejectWorld(compressed.chopped(4));
    rejectWorld(QByteArray::fromHex("fffe") + le(validText) + QByteArray(1, '\0'));
    for(const auto &valid : validWorlds) {
        put("/WORLD/test.w", valid);
        QJsonObject referenced{{"first",QJsonObject{{"deleted",true}}}};
        require(AutoPlaceWire::pruneAssets(cleanup.path(),referenced,false,error,true)
            && error.isEmpty() && referenced.contains("first")
            && QFile::exists(cleanup.path()+"/SHAPES/"+first),
            "valid world encoding rejected or saved wire reference lost");
    }
    put("/WORLD/test.w",("SIMISA@@@@@@@@@@JINX0w0t______\nTr_Worldfile ( FileName ( "+first+" ) )").toUtf8());
    require(AutoPlaceWire::discardBakes(cleanup.path(),discardSpans,{"first","second","external"},error),
        "discard failed with partially saved bakes");
    require(discardSpans.value("first").toObject()==savedRecord
        && discardSpans.contains("raw") && discardSpans.value("external").toObject()==reservation
        && !discardSpans.contains("second")
        && QFile::exists(cleanup.path()+"/SHAPES/"+first)
        && QFile::exists(cleanup.path()+"/SHAPES/"+first+"d")
        && !QFile::exists(cleanup.path()+"/SHAPES/"+second)
        && !QFile::exists(cleanup.path()+"/SHAPES/"+second+"d"),"discard lost saved assets or retained unsaved bake");
    require(AutoPlaceWire::discardBakes(cleanup.path(),discardSpans,{"second"},error),"discard retry failed");
    discardSpans["hidden"] = hiddenPreview;
    require(AutoPlaceWire::discardBakes(cleanup.path(),discardSpans,{"raw","hidden"},error),
        "discard failed for raw-only wire records without generated shapes");
    require(!discardSpans.contains("raw") && !discardSpans.contains("hidden")
        && discardSpans.value("first").toObject()==savedRecord
        && discardSpans.value("external").toObject()==reservation,
        "raw discard left JSON records or changed saved/external ownership");
    require(AutoPlaceWire::discardBakes(cleanup.path(),discardSpans,{"raw","hidden"},error),
        "raw discard retry was not idempotent");
    // Delete All followed by a new bake/save must not treat kept wires as
    // unresolved deletions merely because the saved world references them.
    put("/SHAPES/"+first,"shape ( APWireCharcoal_v1.dds )");
    put("/SHAPES/"+first+"d","descriptor");
    put("/TEXTURES/APWireCharcoal_v1.dds","shared texture");
    put("/SHAPES/"+second,"orphan"); put("/SHAPES/"+second+"d","descriptor");
    QJsonObject keptSpans{{"first",savedRecord}};
    error="previous failure";
    require(AutoPlaceWire::pruneAssets(cleanup.path(),keptSpans,true,error)
        && error.isEmpty() && keptSpans.value("first").toObject()==savedRecord
        && QFile::exists(cleanup.path()+"/SHAPES/"+first)
        && QFile::exists(cleanup.path()+"/SHAPES/"+first+"d")
        && QFile::exists(cleanup.path()+"/TEXTURES/APWireCharcoal_v1.dds")
        && !QFile::exists(cleanup.path()+"/SHAPES/"+second),
        "global cleanup mistook a kept saved bake for a pending deletion");
    require(AutoPlaceWire::pruneAssets(cleanup.path(),keptSpans,true,error),"kept bake cleanup retry failed");
    auto deletedSaved=savedRecord; deletedSaved["deleted"]=true;
    keptSpans["first"]=deletedSaved;
    require(!AutoPlaceWire::pruneAssets(cleanup.path(),keptSpans,true,error)
        && keptSpans.contains("first") && QFile::exists(cleanup.path()+"/SHAPES/"+first),
        "genuinely deleted but still referenced bake lost protection");
    // Optional local evidence is read-only and never becomes a public fixture.
    const QString sampleDirectory = qEnvironmentVariable("TSRE_TEST_WORLD_DIR");
    if(!sampleDirectory.isEmpty()) {
        const QDir samples(sampleDirectory);
        const QStringList worldFiles = samples.entryList({"*.w"}, QDir::Files);
        require(!worldFiles.isEmpty(), "local world evidence folder is empty");
        for(const QString &name : worldFiles) {
            QFile file(samples.filePath(name));
            require(file.open(QIODevice::ReadOnly), "local world evidence unreadable");
            QString text;
            QByteArray binary;
            require(WireAssetScan::decodedWorldFile(file.readAll(), text, binary),
                    "local world failed cleanup validation");
        }
        std::cout << "Local world cleanup validation passed: " << worldFiles.size() << " files\n";
    }
    std::cout<<"AP wire geometry, LOD, DDS, reservations, discard and scoped asset cleanup passed\n";
    return 0;
}

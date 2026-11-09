// TSRE GenX streaming terrain paint. GNU GPL v3 or later.
#include "RoutePaintBatch.h"
#include "Terrain.h"
#include "TerrainInfo.h"
#include "TerrainLib.h"
#include "TerrainLibQt.h"
#include "QuadTree.h"
#include "Route.h"
#include "TDB.h"
#include "Brush.h"
#include "Texture.h"
#include "AceLib.h"
#include "ace_codec/AceInputValidation.h"
#include "DdsLib.h"
#include "TerrainTextureFormat.h"
#include "TerrainGridMath.h"
#include "TexturePaintMath.h"
#include "RouteSaveTransaction.h"
#include "SaveProgress.h"
#include "Game.h"
#include <QImageReader>
#include <QDirIterator>
#include <QScopedValueRollback>
#include <QSet>
#include <QFileInfo>
#include <QFile>
#include <QCache>
#include <QTemporaryDir>
#include <QDataStream>
#include <QtEndian>
#include <memory>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace {
struct DeleteImage {
    void operator()(Texture *texture) const {
        if(!texture) return;
        delete[] texture->imageData;
        texture->imageData=nullptr;
        delete texture;
    }
};
using Image = std::unique_ptr<Texture,DeleteImage>;
constexpr qint64 ImageBudget = 64*1024*1024;

Image loadImage(const QString &path, QString &error) {
    if(QFileInfo(path).size()>128*1024*1024) {
        error="Encoded texture exceeds the batch input budget: "+path; return {};
    }
    Image texture(new Texture(path));
    const QString extension=QFileInfo(path).suffix().toLower();
    if(extension=="ace") {
        if(!AceInputValidation::preflight(path,AceInputValidation::Profile::RoutePaint,error)) {
            error="Invalid texture: "+path+"\n"+error; return {};
        }
        AceLib reader; reader.texture=texture.get(); reader.run();
    } else {
        // DDS header dimensions are checked before allocating decoded pixels.
        if(extension=="dds") {
            QFile file(path);
            if(!file.open(QIODevice::ReadOnly)) { error="Cannot read texture: "+path; return {}; }
            const QByteArray header=file.read(128);
            if(header.size()<128 || header.left(4)!="DDS ") { error="Invalid DDS: "+path; return {}; }
            const auto *raw=reinterpret_cast<const uchar *>(header.constData());
            const quint32 width=qFromLittleEndian<quint32>(raw+16), height=qFromLittleEndian<quint32>(raw+12);
            if(!width || !height || width>4096 || height>4096) { error="DDS exceeds the batch image budget: "+path; return {}; }
            DdsLib reader; reader.texture=texture.get(); reader.run();
        } else {
            QImageReader reader(path);
            const QSize size=reader.size();
            if(!size.isValid() || qint64(size.width())*size.height()*4>ImageBudget) {
                error="Invalid or oversized preset image: "+path; return {};
            }
            const QImage image=reader.read().convertToFormat(QImage::Format_RGB888);
            if(image.isNull()) { error="Cannot decode image: "+path; return {}; }
            texture->width=image.width(); texture->height=image.height();
            texture->bytesPerPixel=3; texture->bpp=24; texture->type=GL_RGB;
            texture->imageSize=image.width()*image.height()*3;
            texture->imageData=new unsigned char[texture->imageSize];
            for(int y=0;y<image.height();++y)
                std::memcpy(texture->imageData+y*image.width()*3,image.constScanLine(y),size_t(image.width())*3);
            texture->loaded=true;
        }
    }
    if(!texture->loaded || texture->error || texture->missing || !texture->imageData
            || (texture->bytesPerPixel!=3 && texture->bytesPerPixel!=4)) {
        error="Texture cannot be painted: "+path; return {};
    }
    texture->editable=true;
    return texture;
}
bool stageImage(RouteSaveTransaction &transaction, Texture *texture, const QString &path, QString *error) {
    if((texture->sourceWidth>0 && texture->width!=texture->sourceWidth)
            || (texture->sourceHeight>0 && texture->height!=texture->sourceHeight)) {
        if(error) *error="Refusing to save a texture whose decoded dimensions changed: "+path;
        return false;
    }
    QByteArray bytes;
    texture->saveMipmaps=false;
    const bool encoded=AceLib::serialize(texture,bytes,error);
    return encoded && transaction.addFile(path,bytes,error);
}
struct Source {
    QJsonObject preset;
    QString main, snow;
};
struct CachedImage { Image image; };
class SourceCache {
    QCache<QString,CachedImage> images{128*1024*1024};
public:
    Texture *get(const QString &path,QString &error) {
        if(auto *entry=images.object(path)) return entry->image.get();
        Image image=loadImage(path,error);
        if(!image) return nullptr;
        const int cost=image->width*image->height*image->bytesPerPixel;
        auto *entry=new CachedImage{std::move(image)};
        images.insert(path,entry,cost);
        return entry->image.get();
    }
};

// A private tile: no shared terrain pointers or TexLib entries are modified.
class PaintTile : public Terrain {
public:
    PaintTile(int x,int z) : Terrain(float(x),float(z)) {
        if(tfile) tfile->batchOwned=true;
    }
    bool waterPresent() const {
        for(int i=0;i<tfile->patchsetNpatches*tfile->patchsetNpatches;++i)
            if(tfile->flags[i]&0xc0) return true;
        return false;
    }
    bool paintAndSave(const RoutePaintPlan::Plan &plan, const QVector<Source> &sources,
                      SourceCache &cache,const QVector<QImage> &shapes, const QVector<float> &track,
                      const QVector<float> &roads, unsigned presence,
                      const std::function<void(int)> &report, QString &error) {
        const int patches=tfile->patchsetNpatches;
        return save(&error,[&](RouteSaveTransaction &transaction,QString *failure) {
            for(activePatch=0;activePatch<patches*patches;++activePatch) {
                batchPaintPatch=activePatch;
                main.reset(); snow.reset(); touched=false; snowTouched=false;
                originalName=*tfile->materials[int(tfile->tdata[activePatch*13+6])].tex[0];
                std::copy(tfile->tdata+activePatch*13+6,tfile->tdata+activePatch*13+13,originalCoords);
                if(!RoutePaintPlan::execute(plan,presence,[&](qsizetype row,const RoutePaintPlan::Step &step) {
                    report(int(row));
                    const auto &source=sources[row];
                    Texture *paint=cache.get(source.main,error);
                    if(!paint) return false;
                    Texture *paired=nullptr;
                    if(step.mirror) { paired=cache.get(source.snow,error); if(!paired) return false; }
                    Brush brush;
                    brush.tex=paint; brush.useTexture=true;
                    brush.size=std::clamp(source.preset["size"].toInt(10),1,99);
                    brush.alpha=float(std::clamp(source.preset["intensity"].toInt(99),1,99))/100;
                    brush.texScalePercent=TexturePaintMath::presetScalePercent(source.preset);
                    brush.texRotationDegrees=std::clamp(source.preset["rotation"].toInt(),0,360);
                    brush.brushshape=const_cast<QImage *>(&shapes[source.preset["brush"].toInt()]);
                    brush.ddsOutput=false; brush.createMipmaps=false;
                    brush.mirrorSeason=step.mirror;
                    mirrorSource=paired;
                    if(step.target==RoutePaintPlan::Water) paintWaterEdges(&brush);
                    else {
                        const auto &points=step.target==RoutePaintPlan::Track?track:roads;
                        for(qsizetype i=0;i+2<points.size();i+=3)
                            paintTexture(&brush,int(mojex),int(mojez),points[i],points[i+2]);
                    }
                    mirrorSource=nullptr;
                    if(!paintError.isEmpty()) error=paintError;
                    return error.isEmpty();
                })) { if(failure) *failure=error; return false; }
                if(!touched) continue;
                const QString stem=name+QString("_%1_%2").arg(activePatch/patches).arg(activePatch%patches);
                // Existing painted files may be shared after TERRTEX consolidation.
                // Publish a fresh compatible patch name, never overwrite shared input.
                QStringList folders{routeTerrtexPath()};
                QDirIterator directories(routeTerrtexPath(),QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks);
                while(directories.hasNext()) folders.append(directories.next()+'/');
                const QString output=RoutePaintPlan::freshPatchName(stem,false,folders);
                if(!stageImage(transaction,main.get(),routeTerrtexPath()+output,failure)) return false;
                if(snowTouched && !stageImage(transaction,snow.get(),routeTerrtexPath("snow")+output,
                                             failure)) return false;
                // Preserve every existing seasonal family under the new basename.
                for(const QString &folder:folders.mid(1)) {
                    if(snowTouched && QFileInfo(QDir::cleanPath(folder)).fileName().compare("snow",Qt::CaseInsensitive)==0) continue;
                    const QString old=resolveTexturePath(folder,QString(),originalName);
                    if(!QFile::exists(old)) continue;
                    Image variant=loadImage(old,error);
                    if(!variant) { if(failure) *failure=error; return false; }
                    if(!stageImage(transaction,variant.get(),folder+output,failure)) return false;
                }
                const int material=int(tfile->tdata[activePatch*13+6]);
                *tfile->materials[material].tex[0]=output;
                *tfile->amaterials[material].tex[0]=output;
                main.reset(); snow.reset();
            }
            return true;
        });
    }
private:
    int activePatch=-1;
    Image main,snow;
    Texture *mirrorSource=nullptr;
    bool touched=false,snowTouched=false;
    QString originalName;
    float originalCoords[7];
    QString paintError;
    void paintTextureOnTile(Brush *brush,int y,int u,float x,float z) override {
        if(y*tfile->patchsetNpatches+u!=activePatch || !paintError.isEmpty()) return;
        auto transformed=[&](const QString &path) {
            Image image=loadImage(path,paintError);
            if(!image) return image;
            const float scale=TerrainGridMath::insetPatchTextureScale(
                TerrainGridMath::patchSampleCount(*tfile->nsamples,tfile->patchsetNpatches));
            if(std::abs(originalCoords[1]-TerrainGridMath::TextureInset)+std::abs(originalCoords[2]-TerrainGridMath::TextureInset)
                    +std::abs(originalCoords[3]-scale)+std::abs(originalCoords[4])+std::abs(originalCoords[5])
                    +std::abs(originalCoords[6]-scale)>=0.01f) image->advancedCrop(originalCoords);
            return image;
        };
        if(!main) {
            main=transformed(resolveTexturePath(routeTerrtexPath(),QString(),originalName));
            if(!main) return;
            const int material=tfile->cloneMat(int(tfile->tdata[activePatch*13+6]));
            tfile->tdata[activePatch*13+6]=float(material);
            const float scale=TerrainGridMath::insetPatchTextureScale(
                TerrainGridMath::patchSampleCount(*tfile->nsamples,tfile->patchsetNpatches));
            const float coords[6]={TerrainGridMath::TextureInset,TerrainGridMath::TextureInset,scale,0,0,scale};
            std::copy(coords,coords+6,tfile->tdata+activePatch*13+7);
        }
        main->paint(brush,z,x); touched=true;
        if(brush->mirrorSeason) {
            if(!snow) {
                QString path=resolveTexturePath(routeTerrtexPath("snow"),QString(),originalName);
                if(!QFile::exists(path)) path=routeTerrtexPath("snow")+"terrain.ace";
                if(!QFile::exists(path)) path=routeTerrtexPath("snow")+"terrain.dds";
                snow=transformed(path);
                if(!snow) return;
            }
            Brush paired=*brush; paired.tex=mirrorSource;
            snow->paint(&paired,z,x); snowTouched=true;
        }
    }
};
}

int RoutePaintBatch::textureResolution(const QString &textureName,QString &error) {
    error.clear();
    const QString path=Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),QString(),textureName);
    Image texture=loadImage(path,error);
    return texture?qMax(texture->width,texture->height):0;
}

bool RoutePaintBatch::validateMirrorPreset(const QJsonObject &preset,QString &error) {
    error.clear();
    const QString name=preset["texture"].toString();
    if(name.isEmpty() || QFileInfo(name).fileName()!=name) {
        error="Preset must reference a route TERRTEX filename."; return false;
    }
    const QString main=Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),QString(),name);
    Image image=loadImage(main,error);
    if(!image) return false;
    image.reset();
    const QString snow=Terrain::routeTerrtexPath("snow")+QFileInfo(main).fileName();
    if(!loadImage(snow,error)) { error="Matching Snow texture is unavailable or invalid.\n"+error; return false; }
    return true;
}

bool RoutePaintBatch::run(Route *route,const RoutePaintPlan::Plan &plan,const QJsonArray &presets,
                         const QVector<QImage> &shapes,const Progress &progress,Result &result,QString &error) {
    result={}; error.clear();
    if(!Game::writeEnabled || Game::serverClient) { error="A writable local route is required."; return false; }
    if(!RoutePaintPlan::validate(plan,presets,error)) return false;
    auto *terrainLib=dynamic_cast<TerrainLibQt *>(Game::terrainLib);
    if(!route || !terrainLib || !terrainLib->getQuadTreeDetailed()) {
        error="A loaded detailed-terrain route is required."; return false;
    }
    QVector<Source> sources(plan.steps.size());
    QTemporaryDir sourceCopies;
    if(!sourceCopies.isValid()) { error="Cannot create temporary preset storage."; return false; }
    QHash<QString,QString> copiedSources;
    auto freezeSource=[&](QString &path) {
        if(copiedSources.contains(path)) { path=copiedSources[path]; return true; }
        if(QFileInfo(path).size()>128*1024*1024) {
            error="Preset exceeds the batch input budget: "+path; return false;
        }
        const QString copy=sourceCopies.filePath(QString::number(copiedSources.size())+'.'+QFileInfo(path).suffix());
        if(!QFile::copy(path,copy)) { error="Cannot snapshot preset texture: "+path; return false; }
        copiedSources.insert(path,copy); path=copy; return true;
    };
    // Validate every source before any terrain changes; retain paths, not pixels.
    for(qsizetype i=0;i<plan.steps.size();++i) {
        if(plan.steps[i].target==RoutePaintPlan::Unused) continue;
        for(const auto &value:presets) if(value.toObject()["name"].toString()==plan.steps[i].preset) sources[i].preset=value.toObject();
        auto &source=sources[i];
        const QString name=source.preset["texture"].toString();
        if(name.isEmpty() || QFileInfo(name).fileName()!=name) { error=QString("Row %1: preset must reference a route TERRTEX filename.").arg(i+1); return false; }
        source.main=Terrain::resolveTexturePath(Terrain::routeTerrtexPath(),QString(),name);
        source.snow=Terrain::routeTerrtexPath("snow")+QFileInfo(source.main).fileName();
        const int shape=source.preset["brush"].toInt();
        if(shape<0 || shape>=shapes.size() || shapes[shape].isNull()) { error=QString("Row %1: preset brush shape is unavailable.").arg(i+1); return false; }
        if(!freezeSource(source.main)) return false;
        Image check=loadImage(source.main,error);
        if(!check) { error=QString("Row %1: ").arg(i+1)+error; return false; }
        check.reset();
        if(plan.steps[i].mirror && (!freezeSource(source.snow) || !loadImage(source.snow,error))) {
            error=QString("Row %1: matching Snow texture is required. ").arg(i+1)+error; return false;
        }
    }
    QVector<QPair<int,int>> tiles;
    bool unsupportedTiles=false;
    auto *tree=Game::terrainLib->getQuadTreeDetailed();
    std::function<void(QuadTree::QuadTile *)> collect=[&](QuadTree::QuadTile *node) {
        if(!node) return;
        for(int x=0;x<2;++x) for(int y=0;y<2;++y) {
            if(node->tile[x][y]) collect(node->tile[x][y]);
            if(node->level==1 && node->populated[x][y]) tiles.append({node->x+x,-(node->y+y)});
            else if(node->populated[x][y] && !node->tile[x][y]) unsupportedTiles=true;
        }
    };
    for(auto *entry:tree->td) if(entry) collect(entry->qt);
    if(unsupportedTiles) { error="The detailed terrain index contains nonstandard tile sizes. No painting was started."; return false; }
    if(tiles.isEmpty()) { error="No detailed terrain tiles were found in the route index."; return false; }
    QScopedValueRollback<bool> season(Game::seasonalEditing,false);
    QScopedValueRollback<QString> mainSeason(Game::season,QString("Summer"));
    SourceCache cache;
    bool wantsTrack=false,wantsRoads=false;
    for(const auto &step:plan.steps) {
        wantsTrack|=step.target==RoutePaintPlan::Track;
        wantsRoads|=step.target==RoutePaintPlan::Roads;
    }
    for(const auto &coordinate:tiles) {
        result.tile=Terrain::getTileName(coordinate.first,-coordinate.second);
        if(!progress(result.scanned,tiles.size(),"Scanning tile "+result.tile)) { result.cancelled=true; return true; }
        PaintTile tile(coordinate.first,coordinate.second);
        if(!tile.loaded) { error="Cannot load terrain tile "+result.tile; return false; }
        QVector<float> track,roads;
        if(wantsTrack) route->collectTerrainPaintPoints(coordinate.first,coordinate.second,false,track);
        if(wantsRoads) route->collectTerrainPaintPoints(coordinate.first,coordinate.second,true,roads);
        const unsigned presence=(!track.isEmpty()?1u<<RoutePaintPlan::Track:0)
            |(!roads.isEmpty()?1u<<RoutePaintPlan::Roads:0)|(tile.waterPresent()?1u<<RoutePaintPlan::Water:0);
        bool applicable=false;
        RoutePaintPlan::execute(plan,presence,[&](qsizetype,const RoutePaintPlan::Step &){applicable=true;return true;});
        if(!applicable) { ++result.skipped; ++result.scanned; continue; }
        if(!tile.paintAndSave(plan,sources,cache,shapes,track,roads,presence,[&](int row) {
            // Cancellation is honored only at the next committed-tile boundary.
            progress(result.scanned,tiles.size(),QString("Tile %1 - row %2; %3 saved, %4 skipped")
                .arg(result.tile).arg(row+1).arg(result.saved).arg(result.skipped));
        },error)) { error="Tile "+result.tile+": "+error; return false; }
        ++result.saved; ++result.scanned;
        if(!terrainLib->refreshPaintedTile(coordinate.first,coordinate.second,error)) {
            result.refreshFailed=true; return false;
        }
    }
    progress(tiles.size(),tiles.size(),"Route painting complete.");
    return true;
}

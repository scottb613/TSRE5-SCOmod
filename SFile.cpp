/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "SFile.h"
#include "RouteFrameProfile.h"
#include "OpeningViewLoad.h"
#include "PolyVegObject.h"
#include <QOpenGLExtraFunctions>
#include <QThreadPool>
#include "WireAttachmentOrder.h"
#include <cmath>
#include "SFileC.h"
#include "SFileX.h"
#include "ReadFile.h"
#include "ParserX.h"
#include "TexLib.h"
#include "ShapeTexturePath.h"
#include "Texture.h"
#include <QDebug>
#include <QtCore>
#include <iostream>
#include <QOpenGLShaderProgram>
#include "GLUU.h"
#include "GLMatrix.h"
#include <QString>
#include "Game.h"
#include "TS.h"
#include "ShapeTextureInfo.h"
#include "ShapeHierarchyInfo.h"
#include "ContentHierarchyInfo.h"
#include "RenderItem.h"
#include "Renderer.h"
#include "Route.h"
#include "Terrain.h"

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <new>
#include <limits>
#include <utility>

struct SFileReadResult {
    std::unique_ptr<SFileGeometry> geometry;
    std::unique_ptr<FileBuffer> descriptor;
    bool shapeOpened = false;
    bool descriptorOpened = false;
    qint64 workerParseNs = 0;
};

namespace {
std::unique_ptr<FileBuffer> readShapeFile(const QString &path, bool &opened) {
    QFile file(path);
    opened = file.open(QIODevice::ReadOnly);
    return opened ? std::unique_ptr<FileBuffer>(ReadFile::read(&file)) : nullptr;
}

struct ShapeParseOptions {
    float alphaTest;
    bool debugOutput;
};

SFileReadResult readAndParseShape(const QString &path, bool readDescriptor,
                                ShapeParseOptions options, bool background) {
    SFileReadResult result;
    std::unique_ptr<FileBuffer> source;
    {
        RouteFrameProfile::Scope read(RouteFrameProfile::Stage::ShapeRead, &path);
        source = readShapeFile(path, result.shapeOpened);
        if (source && readDescriptor)
            result.descriptor = readShapeFile(path + "d", result.descriptorOpened);
    }
    if (!source)
        return result;

    QElapsedTimer timer;
    if (background)
        timer.start();
    RouteFrameProfile::Scope parse(RouteFrameProfile::Stage::ShapeParse, &path);
    auto geometry = std::make_unique<SFileGeometry>();
    if (geometry->parse(*source, options.alphaTest, options.debugOutput))
        result.geometry = std::move(geometry);
    if (background)
        result.workerParseNs = timer.nsecsElapsed();
    return result;
}

using ShapeReadClock = std::chrono::steady_clock;
struct PendingShapeRead {
    std::future<SFileReadResult> result;
    ShapeReadClock::time_point lastRequest;

    bool ready() const {
        return result.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
};

struct ShapeReadPool {
    std::map<QString, PendingShapeRead> pending;
    QThreadPool workers;

    ShapeReadPool() { workers.setMaxThreadCount(2); }

    bool takeOrStart(const QString &path, ShapeParseOptions options, bool allowUpload,
                     SFileReadResult &result) {
        const auto now = ShapeReadClock::now();
        const auto it = pending.find(path);
        if (it != pending.end()) {
            it->second.lastRequest = now;
            if (!allowUpload || !it->second.ready())
                return false;
            auto completed = std::move(it->second.result);
            pending.erase(it);
            try {
                result = completed.get();
            } catch (const std::bad_alloc &) {
                qWarning() << "Insufficient memory to read shape:" << path;
                result.shapeOpened = true;
            }
            RouteFrameProfile::count(RouteFrameProfile::Counter::ShapeReadCompleted);
            if (result.workerParseNs > 0)
                RouteFrameProfile::recordDuration(RouteFrameProfile::Stage::ShapeWorkerParse,
                                                  result.workerParseNs, path);
            return true;
        }

        // Bound the number of decoded sources waiting for the editor. Release
        // completed reads whose shapes are no longer being requested.
        constexpr std::size_t maxOutstanding = 4;
        if (pending.size() >= maxOutstanding) {
            for (auto stale = pending.begin(); stale != pending.end();) {
                if (now - stale->second.lastRequest > std::chrono::milliseconds(500)
                        && stale->second.ready())
                    stale = pending.erase(stale);
                else
                    ++stale;
            }
            if (pending.size() >= maxOutstanding)
                return false;
        }

        // The worker owns its source and parsed data. The live SFile and its
        // OpenGL resources stay on the render thread.
        if (workers.activeThreadCount() >= workers.maxThreadCount())
            return false;
        std::packaged_task<SFileReadResult()> task([path, options] {
            return readAndParseShape(path, true, options, true);
        });
        auto future = task.get_future();
        if (workers.tryStart(std::move(task))) {
            pending.emplace(path, PendingShapeRead{std::move(future), now});
            RouteFrameProfile::count(RouteFrameProfile::Counter::ShapeReadSubmitted);
        }
        return false;
    }
};

ShapeReadPool &shapeReads() {
    static ShapeReadPool pool;
    return pool;
}
} // namespace

SFile::SFile() {
    pathid = "";
    this->loaded = 2;
}

SFile::SFile(QString pathid, QString name, QString texp ) {
    this->pathid = pathid;
    this->nazwa = name;
    this->isinit = 1;
    this->loaded = 0;
    this->baseTexPath = texp;
    this->texPath = texp;
    state.push_back(State());
}

SFile::SFile(const SFile& orig) {
}

SFile::~SFile() {
}

QString SFile::evaluatePathId() {   //// EFO fixing relative paths which have folders between Trainset and /../    
    int trainsetPos = pathid.indexOf("trainset");
    int parentDirPos = pathid.indexOf("/../");
    if (trainsetPos != -1 && parentDirPos != -1 && trainsetPos < parentDirPos) {        
        if(Game::debugOutput) qDebug() << "Before APF:" << pathid;
        // Replace everything between "trainset" and "/../" with empty string
        pathid = pathid.mid(0, trainsetPos+8) + pathid.mid(parentDirPos + 3);                
        if(Game::debugOutput) qDebug() << "After AFP: " << pathid;
        return pathid;        
    } else {
        // Pathid doesn't contain the specific pattern or "trainset" comes after "/../"
        return pathid;
    }
}

void SFile::load() {
    RouteFrameProfile::Scope shapeLoad(RouteFrameProfile::Stage::ShapeLoad, &pathid);
    pathid = evaluatePathId();
    invalidatePendingRead();
    loaded = 2;
    loadDecoded(readAndParseShape(pathid, !loadedSd,
                                  {GLUU::get()->alphaTest, Game::debugOutput}, false));
}

void SFile::invalidatePendingRead() {
    shapeReads().pending.erase(pathid);
}

bool SFile::loadForRender() {
    if (!GLUU::get()->streamShapeReads) {
        if (Game::allowObjLag < 1)
            return false;
        Game::allowObjLag -= 2;
        load();
        return true;
    }

    pathid = evaluatePathId();
    SFileReadResult result;
    if (!shapeReads().takeOrStart(pathid, {GLUU::get()->alphaTest, Game::debugOutput},
                                  Game::allowObjLag >= 1, result))
        return false;
    Game::allowObjLag -= 2;
    loaded = 2;
    RouteFrameProfile::Scope shapeLoad(RouteFrameProfile::Stage::ShapeLoad, &pathid);
    loadDecoded(std::move(result));
    return true;
}

void SFile::loadDecoded(SFileReadResult result) {
    if (!result.shapeOpened) {
        if (Game::debugOutput) qDebug() << "S Shape: not exist" << pathid;
        Route::missingList.append(pathid.mid(pathid.lastIndexOf('/') + 1));
        return;
    }
    if (!result.geometry) {
        Route::missingList.append(QFileInfo(pathid).fileName());
        return;
    }
    static_cast<SFileGeometry &>(*this) = std::move(*result.geometry);
    loaded = uploadGeometry() ? 1 : 2;
    if (!loadedSd) {
        if (!result.descriptorOpened && Game::legacySupport)
            qDebug() << "Sd Shape: not exist" << pathid + "d";
        if (result.descriptor)
            loadSd(result.descriptor.get());
    }
}

bool SFile::uploadGeometry() {
    auto *context = QOpenGLContext::currentContext();
    if (!context)
        return false;
    auto *f = context->functions();
    for (int lod = 0; lod < iloscd; ++lod) {
        auto &level = distancelevel[lod];
        for (int i = 0; i < level.iloscs; ++i) {
            auto &object = level.subobiekty[i];
            RouteFrameProfile::Scope upload(RouteFrameProfile::Stage::ShapeGraphics, &pathid);
            if (object.vertices.size() > std::numeric_limits<int>::max() / sizeof(float))
                return false;
            auto graphics = std::make_unique<SubGraphics>();
            if (!graphics->VAO.create() || !graphics->VBO.create())
                return false;
            {
                QOpenGLVertexArrayObject::Binder vao(&graphics->VAO);
                if (!graphics->VBO.bind())
                    return false;
                graphics->VBO.allocate(object.vertices.data(), int(object.vertices.size() * sizeof(float)));
                constexpr int stride = 9 * sizeof(float);
                f->glEnableVertexAttribArray(0);
                f->glEnableVertexAttribArray(1);
                f->glEnableVertexAttribArray(2);
                f->glEnableVertexAttribArray(3);
                f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
                f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(float)));
                f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(6 * sizeof(float)));
                f->glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(8 * sizeof(float)));
                graphics->VBO.release();
            }
            object.graphics = std::move(graphics);
            std::vector<float>().swap(object.vertices);
        }
    }
    return true;
}

bool SFileGeometry::parse(FileBuffer &source, float alphaTest, bool debugOutput) {
    auto *data = &source;
    bool hasLods = false;
    int loadingCount = 0;
    //qDebug() << "--" << pathid << "--" << data->length;

    data->off = 32;
    if (data->length >= 36 && data->getInt() == 71) {
        int pozycja, offset, akto;
        data->off = 32;
        int val = data->getInt();
        
        //qDebug() << val << " plik binarny ";
        //wczytanie binarnego
        data->off += 5;
        for (;;) {
            pozycja = data->getInt();
            offset = data->getInt();
            akto = data->off;
            //qDebug() << "#SFile - token: "<< pozycja << TS::IdName[pozycja];
            switch (pozycja) {
                case 70:
                    break;
                case 68:
                    break;
                case 72:
                    loadingCount++;
                    SFileC::odczytajshaders(data, this);
                    break;
                case 7:
                    loadingCount++;
                    SFileC::odczytajpunktyc(data, this);
                    getSize();
                    break;
                case 9:
                    loadingCount++;
                    SFileC::odczytajuvpunktyc(data, this);
                    break;
                case 5:
                    loadingCount++;
                    SFileC::odczytajnormalnec(data, this);
                    break;
                case 66:
                    loadingCount++;
                    SFileC::odczytajmatricesc(data, this);
                    break;
                case 14:
                    loadingCount++;
                    SFileC::odczytajimagesc(data, this);
                    break;
                case 16:
                    loadingCount++;
                    SFileC::odczytajtexturesc(data, this);
                    break;
                case 47:
                    loadingCount++;
                    SFileC::odczytajvtx_statesc(data, this);
                    break;
                case 55:
                    loadingCount++;
                    SFileC::odczytajprim_statesc(data, this);
                    break;
                case 74:
                    break;
                case 76:
                    break;
                case 11:
                    break;
                case 18:
                    break;
                case 79:
                    break;
                case 31:
                    if(loadingCount < 9){
                        qDebug() << "#shape - loading error" << 31;
                        return false;
                    }
                    if (!SFileC::odczytajloddc(data, this, alphaTest))
                        return false;
                    hasLods = true;
                    break;
                case 29:
                    int pozycja1,offset1,akto1,some_val;
                    //some_val = data->getInt();
                    data->off+=5; // maybe read animation number instead?
                    //qDebug() << some_val;
                    //qDebug() << data->off << akto + offset;
                    //some_val = data->off+=4;
                    if(data->off >= akto + offset) break;
                    for (;;) {
                        pozycja1 = data->getInt();
                        offset1 = data->getInt();
                        akto1 = data->off;
                        switch (pozycja1) {
                            case 28:
                                animations.push_back(Animation());
                                animations.back().loadC(data, akto1 + offset1, debugOutput);
                                if(animations.size() > 0){
                                    //qDebug() << animations[0].node.size();
                                }
                                break;
                            default:
                                if(debugOutput) qDebug() << "#SFile Animations - unknown token: " << pozycja1;
                                break;
                        }
                        data->off = akto1 + offset1;
                        if(data->off >= akto + offset) break;
                    }
                    break;
                default:
                    if(debugOutput) qDebug() << "#SFile - unknown token: " << pozycja;
                    break;
            }
            data->off = akto + offset;
            if(data->off >= data->length) break;
        }
    } else {
        //qDebug() << "plik xml:";
        //wczytanie plku xml
        data->off = 0;
        ParserX::NextLine(data);

        QString sh = "";
        while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
            if(sh == "shape"){
                while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                    //qDebug() << sh;
                    if(sh == "shader_names"){
                        SFileX::odczytajshaders(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "points"){
                        SFileX::odczytajpunkty(data, this);
                        getSize();
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "uv_points"){
                        SFileX::odczytajuvpunkty(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "normals"){
                        SFileX::odczytajnormalne(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "matrices"){
                        SFileX::odczytajmatrices(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "images"){
                        SFileX::odczytajimages(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "textures"){
                        SFileX::odczytajtextures(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "vtx_states"){
                        SFileX::odczytajvtx_states(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "prim_states"){
                        SFileX::odczytajprim_states(data, this);
                        loadingCount++;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "shape_header"){
                        //qDebug() << "shape_header" << 
                        ParserX::GetAlternativeTokenName(data);
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "volumes"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "texture_filter_names"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "sort_vectors"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "colours"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "light_materials"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "light_model_cfgs"){
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "lod_controls"){
                        if(loadingCount < 9){
                            if(debugOutput) qDebug() << "#shape - loading error" << sh;
                            return false;
                        }
                        if (!SFileX::odczytajlodd(data, this, alphaTest))
                            return false;
                        hasLods = true;
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(sh == "animations"){
                        while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                            if(sh == "animation"){
                                animations.push_back(Animation());
                                animations.back().loadX(data);
                                if(animations.size() > 0){
                                    if(debugOutput) qDebug() << "Sfile306:" << animations[0].node.size();
                                           
                                    /*for(int i = 0; i < animations[0].node.size(); i++){
                                        qDebug() << animations[0].node[i].linearKey.size();
                                        qDebug() << animations[0].node[i].slerpRot.size();
                                        qDebug() << animations[0].node[i].tcbKey.size();
                                    }*/
                                }
                                ParserX::SkipToken(data);
                                continue;
                            }
                            ParserX::SkipToken(data);
                        }
                        ParserX::SkipToken(data);
                        continue;
                    }
                    if(debugOutput) qDebug() << "#shape - undefined token" << sh;
                    ParserX::SkipToken(data);
                }
                
                ParserX::SkipToken(data);
                continue;
            }
            if(debugOutput) qDebug() << "#SFile - undefined token" << sh;
            ParserX::SkipToken(data);
        }
    }
    if (hasLods)
        buildFrameIds();
    return hasLods && iloscd > 0;
}

void SFileGeometry::Animation::loadC(FileBuffer* data, int length, bool debugOutput){
    data->off++;
    frames = data->getInt();
    fps = data->getInt();
    int pozycja, offset, akto;
    int pozycja1, offset1, akto1;
    int pozycja2, offset2, akto2;
    int nodeCount, cCount, kCount;
    
    for (;;) {
        pozycja = data->getInt();
        offset = data->getInt();
        akto = data->off;

        switch (pozycja) {
            case 27:
                data->off++;
                nodeCount = data->getInt();
                for(int i = 0; i < nodeCount; i++){
                    node.push_back(AnimNode());
                    pozycja1 = data->getInt();
                    offset1 = data->getInt();
                    akto1 = data->off;
                    int temp = data->get();
                    data->off += temp*2;
                    data->off++;
                    data->off += 8;
                    cCount = data->getInt();
                    //qDebug() << cCount;
                    for(int j = 0; j < cCount; j++){
                        pozycja2 = data->getInt();
                        offset2 = data->getInt();
                        akto2 = data->off;
                        switch(pozycja2){
                            case 24:
                                data->off++;
                                kCount = data->getInt();
                                //qDebug() << "kCount"<<kCount;
                                for(int ii = 0; ii < kCount; ii++){
                                    int tcbid = data->getInt();
                                    data->getInt();
                                    data->off++;
                                    if(tcbid == 20){
                                        node.back().tcbKey.push_back(AnimNode::TcbKey());
                                        node.back().tcbKey.back().frame = data->getUint();
                                        node.back().tcbKey.back().quat[0] = -data->getFloat();
                                        node.back().tcbKey.back().quat[1] = data->getFloat();
                                        node.back().tcbKey.back().quat[2] = -data->getFloat();
                                        node.back().tcbKey.back().quat[3] = data->getFloat();
                                        for(int i = 0; i < 5; i++){
                                            node.back().tcbKey.back().param[i] = data->getFloat();
                                        }
                                    }
                                    if(tcbid == 23){
                                        node.back().slerpRot.push_back(AnimNode::SlerpRot());
                                        node.back().slerpRot.back().frame = data->getUint();
                                        node.back().slerpRot.back().quat[0] = -data->getFloat();
                                        node.back().slerpRot.back().quat[1] = data->getFloat();
                                        node.back().slerpRot.back().quat[2] = -data->getFloat();
                                        node.back().slerpRot.back().quat[3] = data->getFloat();
                                    }
                                }
                                break;
                            case 21:
                                data->off++;
                                kCount = data->getInt();
                                //qDebug() << "kCount"<<kCount;
                                for(int ii = 0; ii < kCount; ii++){
                                    //qDebug() << 
                                    data->getInt();
                                    data->getInt();
                                    data->off++;
                                    node.back().linearKey.push_back(AnimNode::LinearKey());
                                    node.back().linearKey.back().frame = data->getUint();
                                    for(int i = 0; i < 3; i++){
                                        node.back().linearKey.back().pos[i] = data->getFloat();
                                    }
                                }
                                break;
                            default:
                                if(debugOutput) qDebug() << "#controller" << pozycja2;
                                break;
                        }
                        data->off = akto2 + offset2;
                        if(data->off >= akto1+offset1) break;
                    }
                    //qDebug() << "pozycja" << pozycja1 << TS::IdName[pozycja1];
                    data->off = akto1+offset1;
                }
                break;
            default:
                if(debugOutput) qDebug() << "#SFile Animation - unknown token: " << pozycja;
                break;
        }
        data->off = akto + offset;
        if(data->off >= length) break;
    }
}

void SFileGeometry::Animation::loadX(FileBuffer* data){
    frames = ParserX::GetNumber(data);
    fps = ParserX::GetNumber(data);
    
    QString sh;
    int count;
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        if(sh == "anim_nodes"){
            count = ParserX::GetNumber(data);
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                if(sh == "anim_node"){
                    //qDebug() << 
                    ParserX::NextTokenInside(data);
                    node.push_back(AnimNode());
                    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                        if(sh == "controllers"){
                            count = ParserX::GetNumber(data);
                            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                                if(sh == "tcb_rot"){
                                    count =  ParserX::GetNumber(data);
                                    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                                        if(sh == "tcb_key"){
                                            node.back().tcbKey.push_back(AnimNode::TcbKey());
                                            node.back().tcbKey.back().frame = ParserX::GetUInt(data);
                                            node.back().tcbKey.back().quat[0] = -ParserX::GetNumber(data);
                                            node.back().tcbKey.back().quat[1] = ParserX::GetNumber(data);
                                            node.back().tcbKey.back().quat[2] = -ParserX::GetNumber(data);
                                            node.back().tcbKey.back().quat[3] = ParserX::GetNumber(data);
                                            for(int i = 0; i < 5; i++){
                                                node.back().tcbKey.back().param[i] = ParserX::GetNumber(data);
                                            }
                                            ParserX::SkipToken(data);
                                            continue;
                                        }
                                        if(sh == "slerp_rot"){
                                            node.back().slerpRot.push_back(AnimNode::SlerpRot());
                                            node.back().slerpRot.back().frame = ParserX::GetUInt(data);
                                            node.back().slerpRot.back().quat[0] = -ParserX::GetNumber(data);
                                            node.back().slerpRot.back().quat[1] = ParserX::GetNumber(data);
                                            node.back().slerpRot.back().quat[2] = -ParserX::GetNumber(data);
                                            node.back().slerpRot.back().quat[3] = ParserX::GetNumber(data);
                                            ParserX::SkipToken(data);
                                            continue;
                                        }
                                        ParserX::SkipToken(data);
                                    }
                                    ParserX::SkipToken(data);
                                    continue;
                                }
                                if(sh == "linear_pos"){
                                    count = ParserX::GetNumber(data);
                                    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                                        if(sh == "linear_key"){
                                            node.back().linearKey.push_back(AnimNode::LinearKey());
                                            node.back().linearKey.back().frame = ParserX::GetUInt(data);
                                            for(int i = 0; i < 3; i++){
                                                node.back().linearKey.back().pos[i] = ParserX::GetNumber(data);
                                            }
                                            ParserX::SkipToken(data);
                                            continue;
                                        }
                                        ParserX::SkipToken(data);
                                    }
                                    ParserX::SkipToken(data);
                                    continue;
                                }
                                ParserX::SkipToken(data);
                            }
                            ParserX::SkipToken(data);
                            continue;
                        }
                        ParserX::SkipToken(data);
                    }
                    ParserX::SkipToken(data);
                    continue;
                }
                ParserX::SkipToken(data);
            }
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
    }
}

void SFile::loadSd(FileBuffer *data) {
    data->toUtf16();
    data->skipBOM();
    QString sh;
    
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        //qDebug() << sh;
        if (sh == ("simisa@@@@@@@@@@jinx0t1t______")) {
            continue;
        }
        if (sh == ("shape")) {
            sdName = ParserX::GetString(data).trimmed();
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                if (sh == ("esd_detail_level")) {
                    esdDetailLevel = ParserX::GetNumber(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("esd_alternative_texture")) {
                    esdAlternativeTexture = ParserX::GetNumber(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("esd_bounding_box")) {
                    esdBoundingBox << EsdBoundingBox();
                    bool ok = false;
                    for(int i = 0; i < 6; i++){
                        esdBoundingBox.back().shape[i] = ParserX::GetNumberInside(data, &ok);
                        if(!ok) break;
                    }
                    if(ok == false)
                        esdBoundingBox.pop_back();
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("esd_complex")) {
                    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                        if (sh == ("esd_complex_box")) {
                            esdBoundingBox << EsdBoundingBox();
                            for(int i = 0; i < 3; i++){
                                esdBoundingBox.back().rotation[i] = ParserX::GetNumber(data);
                            }
                            for(int i = 0; i < 3; i++){
                                esdBoundingBox.back().translation[i] = ParserX::GetNumber(data);
                            }
                            for(int i = 0; i < 6; i++){
                                esdBoundingBox.back().shape[i] = ParserX::GetNumber(data);
                            }
                            ParserX::SkipToken(data);
                            continue;
                        }
                        ParserX::SkipToken(data);
                    }
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("esd_snapable")) {
                    snapable = true;
                    ParserX::SkipToken(data);
                    continue;
                }
                ParserX::SkipToken(data);
            }
            ParserX::SkipToken(data);
            continue;
        }
    }
    texPath = baseTexPath;
    loadedSd = true;
}

QString SFile::resolveTexturePath(QString textureName) const {
    if(rollingStock)
        return ShapeTexturePath::resolveRollingStock(
            baseTexPath, ShapeTexturePath::rollingStockSeason(esdAlternativeTexture, Game::season),
            textureName);
    return Terrain::resolveTexturePath(baseTexPath, Terrain::textureSubdirCandidatesForFlags(esdAlternativeTexture, Game::season), textureName, true);
}

int SFile::loadTexture(const QString &textureName) const {
    const QString path = resolveTexturePath(textureName);
    return path.isEmpty() ? -2 : TexLib::addTex(path, false, rollingStock);
}

void SFileGeometry::getSize() {
        float tx[2]{-9999, 9999}, ty[2]{-9999, 9999}, tz[2]{-9999, 9999};
        for (int i = 0; i < tpoints.ipoints ; i++) {
            if(tpoints.points[i].x < tx[1]) tx[1] = tpoints.points[i].x;
            if(tpoints.points[i].y < ty[1]) ty[1] = tpoints.points[i].y;
            if(tpoints.points[i].z < tz[1]) tz[1] = tpoints.points[i].z;
            if(tpoints.points[i].x > tx[0]) tx[0] = tpoints.points[i].x;
            if(tpoints.points[i].y > ty[0]) ty[0] = tpoints.points[i].y;
            if(tpoints.points[i].z > tz[0]) tz[0] = tpoints.points[i].z;
        }
        bound[0] = tx[0];
        bound[1] = tx[1];
        bound[2] = ty[0];
        bound[3] = ty[1];
        bound[4] = tz[0];
        bound[5] = tz[1];
        tx[0] = tx[0] - tx[1];
        ty[0] = ty[0] - ty[1];
        tz[0] = tz[0] - tz[1];
        size = sqrt(tx[0]*tx[0] + ty[0]*ty[0] + tz[0]*tz[0]);
        //console.log(this.size);
    }

bool SFile::getBoxPoints(QVector<float>& points){
    if(this->esdBoundingBox.size() == 0){
        for(int i=0; i<2; i++)
            for(int j=4; j<6; j++){
                points.push_back(-bound[i]);
                points.push_back(bound[2]);
                points.push_back(bound[j]);
                points.push_back(-bound[i]);
                points.push_back(bound[3]);
                points.push_back(bound[j]);
            }
        for(int i=0; i<2; i++)
            for(int j=2; j<4; j++){
                points.push_back(-bound[i]);
                points.push_back(bound[j]);
                points.push_back(bound[4]);
                points.push_back(-bound[i]);
                points.push_back(bound[j]);
                points.push_back(bound[5]);
            }
        for(int i=4; i<6; i++)
            for(int j=2; j<4; j++){
                points.push_back(-bound[0]);
                points.push_back(bound[j]);
                points.push_back(bound[i]);
                points.push_back(-bound[1]);
                points.push_back(bound[j]);
                points.push_back(bound[i]);
            }
    } else {
        float tbound[6];
        for(int u = 0; u < this->esdBoundingBox.size(); u++ ){
            tbound[0] = this->esdBoundingBox[u].shape[0];
            tbound[1] = this->esdBoundingBox[u].shape[3];
            tbound[2] = this->esdBoundingBox[u].shape[1];
            tbound[3] = this->esdBoundingBox[u].shape[4];
            tbound[4] = this->esdBoundingBox[u].shape[2];
            tbound[5] = this->esdBoundingBox[u].shape[5];
            
            //int pointsBeg = points.size();
            ///////////
            for(int i=0; i<2; i++)
                for(int j=4; j<6; j++){
                    points.push_back(-tbound[i]);
                    points.push_back(tbound[2]);
                    points.push_back(tbound[j]);
                    points.push_back(-tbound[i]);
                    points.push_back(tbound[3]);
                    points.push_back(tbound[j]);
                }
            for(int i=0; i<2; i++)
                for(int j=2; j<4; j++){
                    points.push_back(-tbound[i]);
                    points.push_back(tbound[j]);
                    points.push_back(tbound[4]);
                    points.push_back(-tbound[i]);
                    points.push_back(tbound[j]);
                    points.push_back(tbound[5]);
                }
            for(int i=4; i<6; i++)
                for(int j=2; j<4; j++){
                    points.push_back(-tbound[0]);
                    points.push_back(tbound[j]);
                    points.push_back(tbound[i]);
                    points.push_back(-tbound[1]);
                    points.push_back(tbound[j]);
                    points.push_back(tbound[i]);
                }
            //int pointsEnd = points.size();
            //for(int i = pointsBeg; i < pointsEnd; i++){
            //    //Vec3::add(&points[i], &points[i], this->esdBoundingBox[u].translation);
            //}
        }
    }
    return true;
}

void SFile::getFloorBorderLinePoints(float *&punkty){
    if(this->esdBoundingBox.size() == 0){
        float h = bound[2];
        if(bound[3] < bound[2])
            h = bound[3];
//0 1 / 4 5
        float vec1[3], vec2[3], dist;
        Vec3::set(vec1, bound[0], h, bound[4]);
        Vec3::set(vec2, bound[0], h, bound[5]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[0], h, bound[5]);
        Vec3::set(vec2, bound[1], h, bound[5]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[1], h, bound[4]);
        Vec3::set(vec2, bound[0], h, bound[4]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[1], h, bound[5]);
        Vec3::set(vec2, bound[1], h, bound[4]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
    } else {
        float tbound[6];
        for(int u = 0; u < this->esdBoundingBox.size(); u++ ){
            tbound[0] = this->esdBoundingBox[u].shape[0];
            tbound[1] = this->esdBoundingBox[u].shape[3];
            tbound[2] = this->esdBoundingBox[u].shape[1];
            tbound[3] = this->esdBoundingBox[u].shape[4];
            tbound[4] = this->esdBoundingBox[u].shape[2];
            tbound[5] = this->esdBoundingBox[u].shape[5];
            
        float h = tbound[2];
        if(tbound[3] < tbound[2])
            h = tbound[3];
        float vec1[3], vec2[3], dist;
        Vec3::set(vec1, bound[0], h, bound[4]);
        Vec3::set(vec2, bound[0], h, bound[5]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[0], h, bound[5]);
        Vec3::set(vec2, bound[1], h, bound[5]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[1], h, bound[4]);
        Vec3::set(vec2, bound[0], h, bound[4]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        Vec3::set(vec1, bound[1], h, bound[5]);
        Vec3::set(vec2, bound[1], h, bound[4]);
        dist = Vec3::distance(vec1, vec2);
        Vec3::sub(vec2, vec2, vec1);
        Vec3::normalize(vec2, vec2);
        for(float i = 0; i < dist + 2; i+=4 ){
            *punkty++ = vec1[0] + i*vec2[0];
            *punkty++ = h;
            *punkty++ = vec1[2] + i*vec2[2];
        }
        }
    }
}

bool SFile::isSnapable(){
    return snapable;
}

void SFile::addSnapablePoints(QVector<float> &out){
    //if(!this->loadedSd)
    //    loadSd();
    if(esdBoundingBox.size() == 0) return;
    float avgZ = (esdBoundingBox[0].shape[2] + esdBoundingBox[0].shape[5])/2.0;
    out.push_back(esdBoundingBox[0].shape[0]);
    out.push_back(0);
    out.push_back(avgZ);
    out.push_back(esdBoundingBox[0].shape[3]);
    out.push_back(0);
    out.push_back(avgZ);
}

void SFile::reload() {
    if(Game::debugOutput) qDebug() << "reload";
    invalidatePendingRead();
    // Use the same resolved path (including case and season) as rendering.
    // The path/name overload lowercases its cache key and can reload a
    // different entry; walking only LOD 0 also misses higher-LOD textures.
    QSet<QString> paths;
    if(loaded == 1 && image) {
        for(int i = 0; i < ilosci; i++)
            paths.insert(resolveTexturePath(image[i].name));
    }
    for(const QString &path : paths) {
        if(path.isEmpty())
            continue;
        if(Game::debugOutput) qDebug() << "Reload shape texture:" << path;
        TexLib::addTex(path, true, rollingStock);
    }
    renderItems.clear();
    requiresUpdate = true;
    loaded = 0;
}

void SFile::refreshSeasonTextures() {
    if (loaded != 1 || ilosci <= 0 || !image)
        return;

    texPath = baseTexPath;
    for (int i = 0; i < ilosci; i++) {
        image[i].texAddr = -1;
        image[i].tex = -1;
    }
    renderItems.clear();
    requiresUpdate = true;
}

unsigned int SFile::newState(){
    if(!releasedStateIds.isEmpty()) {
        const auto released = releasedStateIds.constBegin();
        const unsigned int stateId = *released;
        releasedStateIds.erase(released);
        state[static_cast<int>(stateId)] = State();
        return stateId;
    }
    state.push_back(State());
    return state.size() - 1;
}

void SFile::releaseState(unsigned int stateId){
    if(stateId == 0 || stateId >= static_cast<unsigned int>(state.size()))
        return;
    state[static_cast<int>(stateId)] = State();
    releasedStateIds.insert(stateId);
}

void SFile::compactReleasedStates(){
    while(state.size() > 1) {
        const unsigned int last = static_cast<unsigned int>(state.size()-1);
        if(!releasedStateIds.remove(last))
            break;
        state.removeLast();
    }
    state.squeeze();
}

void SFile::setAnimated(unsigned int stateId, bool animated){
    state[stateId].animated = animated;
}

void SFile::setEnabledSubObjs(unsigned int stateId, unsigned int enabledSubObjs){
    state[stateId].enabledSubObjs = enabledSubObjs;
}

void SFile::setCurrentDistanceLevel(unsigned int stateId, int level){
    if(level < 0)
        level = 0;
    if(level >= iloscd)
        level = 0;
    state[stateId].distanceLevel = level;
    if(Game::debugOutput) qDebug() << "SFile 884:" << state[stateId].distanceLevel;
}

void SFile::enableSubObjByNameQueue(unsigned int stateId, QString name, bool val){
    if(stateId > state.size() - 1) 
        return;
    state[stateId].enableSubObjQueue[name] = val;
}

void SFile::enableSubObjByName(unsigned int stateId, QString name, bool val){
    // Find matrix id
    int matrixId = -1;
    for(int i = 0; i < iloscm; i++) {
        if(macierz[i].name.toLower() == name.toLower() ) {
            matrixId = i;
            break;
        }
    }
    if(matrixId < 0)
        return;
    
    // check if can be disabled/enabled
    if(distancelevel[0].iloscs < 2)
        return;
    //qDebug() << matrixId << distancelevel[0].subobiekty[0].header.geometryNodeMap.size() ;
    if(matrixId >= distancelevel[0].subobiekty[0].header.geometryNodeMap.size())
        return;
    if(distancelevel[0].subobiekty[0].header.geometryNodeMap[matrixId] > 0 )
        return;
    
    // enable / disable subobjs
    for(int j = 1; j < distancelevel[0].iloscs; j++){
        if(distancelevel[0].subobiekty[j].header.geometryNodeMap[matrixId] == 0){
            if(val)
                state[stateId].enabledSubObjs = state[stateId].enabledSubObjs | (1 << j);
            else
                state[stateId].enabledSubObjs = state[stateId].enabledSubObjs & ~(1 << j);
        }
    }
}

void SFile::updateSim(float deltaTime, unsigned int stateId){
    if (isinit != 1 || loaded == 2)
        return;
    
    animated = false;

    if(state[stateId].animated && animations.size() > 0){
        if(animations[0].frames == 0)
            return;
        animated = true;
        if(deltaTime <= 0){
            state[stateId].frameCount = 0;
        } else {
            state[stateId].frameCount += animations[0].fps * deltaTime;
            if(state[stateId].frameCount > animations[0].frames)
                while(state[stateId].frameCount > animations[0].frames)
                    state[stateId].frameCount -= animations[0].frames;
            if(state[stateId].frameCount < 0)
                state[stateId].frameCount = 0;
        }
    }
}

void SFile::render() {
    render(0,0);
}

void SFile::pushRenderItem(){
    pushRenderItem(0,0);
}

void SFile::pushRenderItem(int selectionColor, unsigned int stateId){
    if (isinit != 1 || loaded == 2)
        return;
    if (loaded == 0) {
        loadForRender();
        return;
    }
    
    if(state[stateId].enableSubObjQueue.size() > 0){
        for (auto it = state[stateId].enableSubObjQueue.begin(); it != state[stateId].enableSubObjQueue.end();){
            enableSubObjByName(stateId, it.key(), it.value());
            it = state[stateId].enableSubObjQueue.erase(it);
        }
    }

    if(renderItems[stateId].size() == 0 || requiresUpdate){
        requiresUpdate = false;
        renderItems[stateId].clear();
        
        RenderItem * r;// = new RenderItem();
        float m[16];
        int currentDlevel = state[stateId].distanceLevel;
        for (int i = 0; i < distancelevel[currentDlevel].iloscs; i++) {

            if(((state[stateId].enabledSubObjs >> i) & 1) == 0)
                continue;

            for (int j = 0; j < distancelevel[currentDlevel].subobiekty[i].iloscc; j++) {
                r = new RenderItem();

                int prim_state = distancelevel[currentDlevel].subobiekty[i].czesci[j].prim_state_idx;
                int vtx_state = primstate[prim_state].vtx_state;
                int matrix = vtxstate[vtx_state].matrix;
                bool texEnabled = distancelevel[currentDlevel].subobiekty[i].czesci[j].enabled;

                if(animated){
                    Mat4::identity(m);
                    getPmatrixAnimated(currentDlevel, m, matrix, state[stateId].frameCount);
                    r->msMatrix = (float*)&m;
                } else {
                    if (!macierz[matrix].isFixed) {
                        Mat4::identity(m);
                        memcpy(macierz[matrix].fixed, getPmatrix(currentDlevel, m, matrix), sizeof (float) * 16);
                        macierz[matrix].isFixed = true;
                    }
                    r->msMatrix = (float*)&macierz[matrix].fixed;
                }

                if( vtxstate[vtx_state].arg2 < -7 )
                    r->normalsEnabled = 0;
                else
                    r->normalsEnabled = 1;

                if( vtxstate[vtx_state].arg2 == -12 )
                    r->brightness = 0.5;
                else
                    r->brightness = 1.0;

                if(primstate[prim_state].arg4 == -1 || !texEnabled || TexLib::disabledTextures[image[texture[primstate[prim_state].arg4].image].texAddr] == 1){
                    if(selectionColor == 0){
                        r->disableTextures(1.0, 0.0, 1.0, 1.0);
                    }
                } else if (image[texture[primstate[prim_state].arg4].image].texAddr >= 0) {
                    r->enableTextures(image[texture[primstate[prim_state].arg4].image].texAddr);
                } else if (image[texture[primstate[prim_state].arg4].image].tex == -2) {
                } else if (image[texture[primstate[prim_state].arg4].image].tex == -1) {
                    auto &materialImage = image[texture[primstate[prim_state].arg4].image];
                    materialImage.tex = loadTexture(materialImage.name);
                    requiresUpdate = true;
                } else if (TexLib::mtex[image[texture[primstate[prim_state].arg4].image].tex]->glLoaded) {
                    image[texture[primstate[prim_state].arg4].image].texAddr = TexLib::mtex[image[texture[primstate[prim_state].arg4].image].tex]->tex[0];
                    requiresUpdate = true;
                } else if (TexLib::mtex[image[texture[primstate[prim_state].arg4].image].tex]->loaded) {
                    TexLib::mtex[image[texture[primstate[prim_state].arg4].image].tex]->GLTextures();
                    requiresUpdate = true;
                } else {
                    //                    requiresUpdate = true;
                }

                r->VBO = &distancelevel[currentDlevel].subobiekty[i].graphics->VBO;
                r->VAO = &distancelevel[currentDlevel].subobiekty[i].graphics->VAO;
                //r->mvMatrix = Mat4::clone(Game::currentRenderer->mvMatrix);
                r->vertOffset = distancelevel[currentDlevel].subobiekty[i].czesci[j].offset;
                r->vertCount = distancelevel[currentDlevel].subobiekty[i].czesci[j].iloscv;
                r->itemType = GL_TRIANGLES;
                r->vertexAttr = RenderItem::VNTA;
                r->shared = true;
                renderItems[stateId].push_back(r);
                //Game::currentRenderer->pushItemVNTA(r);
            }
        }
    }
    
    if(renderItems[stateId].size() > 0){
        //for(int i = 0; i < renderItems[stateId].size(); i++){
        //    Mat4::identity(renderItems[stateId][i]->mvMatrix);
        //    RenderItem *r;
            
            //renderItems[stateId][i]->mvMatrix = 
        //            Mat4::copy(renderItems[stateId][i]->mvMatrix, Game::currentRenderer->mvMatrix);
        //    Game::currentRenderer->pushItemVNTA(r);
        //}
        //     Mat4::copy(Game::currentRenderer->mvMatrix, renderItems[stateId][i]->mvMatrix);
        
        Game::currentRenderer->pushItemsVNTA(renderItems[stateId], Game::currentRenderer->mvMatrix);
    }
}

bool SFile::staticBatchState(unsigned int stateId, int &lod, int &subObjects) const {
    if (isinit != 1 || loaded != 1 || animated || stateId >= unsigned(state.size()))
        return false;

    const State &placement = state[stateId];
    if (placement.animated || !placement.enableSubObjQueue.isEmpty())
        return false;
    lod = placement.distanceLevel;
    if (lod < 0 || lod >= iloscd)
        return false;
    subObjects = placement.enabledSubObjs;
    return distancelevel[lod].iloscs <= 32;
}

bool SFile::polyVegBatchState(int lod, int subObjects) const {
    const dist &level = distancelevel[lod];
    const czes *part = nullptr;
    for (int i = 0; i < level.iloscs; ++i) {
        if (!(unsigned(subObjects) & (1u << i)) || level.subobiekty[i].iloscc == 0)
            continue;
        if (part || level.subobiekty[i].iloscc != 1)
            return false;
        part = &level.subobiekty[i].czesci[0];
    }
    if (!part || !part->enabled || part->iloscv == 0)
        return false;

    // A first encounter may load a texture or draw a placeholder. Keep
    // that transition on the original single-placement path.
    const int slot = primstate[part->prim_state_idx].arg4;
    if (slot < 0 || slot >= ilosct)
        return false;
    const int id = texture[slot].image;
    return id >= 0 && id < ilosci && image[id].texAddr >= 0;
}

bool SFile::opaqueBatchState(int lod, int subObjects) const {
    const dist &level = distancelevel[lod];
    bool hasGeometry = false;
    for (int i = 0; i < level.iloscs; ++i) {
        if (!(unsigned(subObjects) & (1u << i)))
            continue;
        const auto &sub = level.subobiekty[i];
        for (int j = 0; j < sub.iloscc; ++j) {
            const auto &part = sub.czesci[j];
            if (!part.iloscv)
                continue;
            if (!part.enabled || part.prim_state_idx < 0 || part.prim_state_idx >= iloscps)
                return false;
            const auto &material = primstate[part.prim_state_idx];
            if (material.arg4 < 0 || material.arg4 >= ilosct)
                return false;
            const int id = texture[material.arg4].image;
            if (id < 0 || id >= ilosci || image[id].texAddr < 0
                    || TexLib::disabledTextures.value(image[id].texAddr) == 1)
                return false;
            // TexDiff forces fragment alpha to 1. A resident RGB texture is
            // also opaque under the same shader, regardless of its alpha test.
            if (material.arg2 < 0 || material.arg2 >= ishaders
                    || shader[material.arg2].alpha != 1.0f) {
                const auto found = TexLib::mtex.find(image[id].tex);
                if (found == TexLib::mtex.end() || !found->second
                        || !found->second->glLoaded || found->second->type != GL_RGB)
                    return false;
            }
            hasGeometry = true;
        }
    }
    return hasGeometry;
}

void StaticShapeRenderBatch::begin(bool vegetation, bool scenery) {
    vegetationEnabled = vegetation;
    sceneryEnabled = scenery;
    nonOpaque.clear();
}

bool StaticShapeRenderBatch::enqueue(SFile &candidate, unsigned int candidateState,
                                     float *tileMatrix, float *objectMatrix,
                                     const QString &fileName, bool preview) {
    int lod, subObjects;
    if (!candidate.staticBatchState(candidateState, lod, subObjects))
        return false;
    const Key key{&candidate, lod, subObjects};
    bool vegetation = false;
    {
        RouteFrameProfile::Scope candidateCheck(RouteFrameProfile::Stage::PolyVegCandidate);
        vegetation = candidate.polyVegBatchState(lod, subObjects)
            && (preview || (plants.key.shape == &candidate && !rawPreview)
                        || PolyVegObject::isRawShape(fileName));
    }

    Group *group = nullptr;
    if (vegetation) {
        if (!vegetationEnabled)
            return false;
        flushScenery();
        if (!(plants.key == key) || rawPreview != preview) {
            flushVegetation();
            plants.key = key;
            plants.state = candidateState;
            rawPreview = preview;
        }
        group = &plants;
    } else {
        if (!sceneryEnabled || preview || nonOpaque.contains(key))
            return false;
        const auto found = sceneryIndices.constFind(key);
        if (found == sceneryIndices.cend()) {
            if (!candidate.opaqueBatchState(lod, subObjects)) {
                nonOpaque.insert(key);
                return false;
            }
            flushVegetation();
            sceneryIndices.insert(key, sceneryGroups.size());
            sceneryGroups.append(Group{key, candidateState, {}});
            group = &sceneryGroups.back();
        } else {
            group = &sceneryGroups[*found];
        }
    }

    std::array<float, 16> transform;
    Mat4::multiply(transform.data(), tileMatrix, objectMatrix);
    group->matrices.append(transform);
    return true;
}

void StaticShapeRenderBatch::draw(const Group &group, QOpenGLExtraFunctions *f,
                                  bool preview, bool staticScenery) {
    GLUU *gluu = GLUU::get();
    Shader *shader = gluu->currentShader;
    gluu->enableTextures();
    shader->setUniformValue(shader->shaderRawPreview, preview ? 1.0f : 0.0f);

    const auto &matrices = group.matrices;
    const auto setMatrix = [shader](const auto &matrix) {
        shader->setUniformValue(shader->mvMatrixUniform,
            *reinterpret_cast<const float (*)[4][4]>(matrix.data()));
    };
    if (matrices.size() > 1 && f && shader->polyVegInstances >= 0 &&
            (buffer.isCreated() || buffer.create()) && buffer.bind()) {
        buffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
        buffer.allocate(matrices.constData(), int(matrices.size() * sizeof(matrices.front())));
        setMatrix(matrices.back());
        if (staticScenery)
            RouteFrameProfile::count(RouteFrameProfile::Counter::StaticShapePlacements, matrices.size());
        group.key.shape->render(0, group.state, f, int(matrices.size()), staticScenery);
        buffer.release();
    } else {
        for (const auto &matrix : matrices) {
            setMatrix(matrix);
            group.key.shape->render(0, group.state);
        }
    }
    shader->setUniformValue(shader->shaderRawPreview, 0.0f);
}

void StaticShapeRenderBatch::flushVegetation() {
    if (plants.matrices.isEmpty())
        return;
    RouteFrameProfile::Scope flushTiming(RouteFrameProfile::Stage::PolyVegFlush);
    draw(plants, GLUU::vegetationInstancing(), rawPreview, false);
    plants.matrices.clear();
    plants.key = {};
}

void StaticShapeRenderBatch::flushScenery() {
    if (sceneryGroups.isEmpty())
        return;
    RouteFrameProfile::Scope flushTiming(RouteFrameProfile::Stage::StaticShapeFlush);
    auto *functions = GLUU::staticShapeInstancing();
    for (const auto &group : sceneryGroups)
        draw(group, functions, false, true);
    sceneryGroups.clear();
    sceneryIndices.clear();
}

void StaticShapeRenderBatch::flush() {
    flushVegetation();
    flushScenery();
}

void SFile::applyPendingSubObjects(unsigned int stateId) {
    auto &queue = state[stateId].enableSubObjQueue;
    for (auto it = queue.begin(); it != queue.end();) {
        enableSubObjByName(stateId, it.key(), it.value());
        it = queue.erase(it);
    }
}

void SFile::prepareForOpeningView(unsigned int stateId, OpeningViewLoad &opening) {
    if (isinit != 1 || loaded == 2 || stateId >= unsigned(state.size()))
        return;
    if (loaded == 0) {
        loaded = 2;
        load();
    }
    if (loaded != 1)
        return;

    applyPendingSubObjects(stateId);
    const auto &placement = state[stateId];
    if (placement.distanceLevel < 0 || placement.distanceLevel >= iloscd)
        return;
    const auto &level = distancelevel[placement.distanceLevel];
    for (int i = 0; i < level.iloscs && i < 32; ++i) {
        if (!(unsigned(placement.enabledSubObjs) & (1u << i)))
            continue;
        const auto &subObject = level.subobiekty[i];
        for (int j = 0; j < subObject.iloscc; ++j) {
            const auto &part = subObject.czesci[j];
            if (!part.enabled)
                continue;
            const int slot = primstate[part.prim_state_idx].arg4;
            if (slot < 0 || slot >= ilosct)
                continue;
            const int id = texture[slot].image;
            if (id < 0 || id >= ilosci || image[id].texAddr >= 0)
                continue;
            if (image[id].tex == -1)
                image[id].tex = loadTexture(image[id].name);
            if (image[id].tex < 0)
                continue;
            const auto found = TexLib::mtex.find(image[id].tex);
            if (found == TexLib::mtex.end() || !found->second)
                continue;
            Texture *material = found->second;
            if (material->decodePending)
                opening.queueSceneryTexture(image[id].tex);
            else if (material->glLoaded)
                image[id].texAddr = material->tex[0];
            else if (material->loaded)
                opening.queueSceneryTexture(image[id].tex);
        }
    }
}

void SFile::render(int selectionColor, unsigned int stateId) {
    render(selectionColor, stateId, nullptr, 0);
}

void SFile::render(int selectionColor, unsigned int stateId,
                   QOpenGLExtraFunctions *instancing, int instanceCount, bool staticScenery) {

    if (isinit != 1 || loaded == 2)
        return;
    if (loaded == 0) {
        if (!loadForRender())
            RouteFrameProfile::count(RouteFrameProfile::Counter::ShapeLoadDeferred);
        return;
    }

    applyPendingSubObjects(stateId);

    int oldmatrix = -2;
    float m[16];
    //var tex;
    //var oldtex = -3;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    GLUU *gluu = GLUU::get();
    //gluu->enableTextures();
    //for(int iii = 0; iii < 200; iii++)
    //float talpha = gluu->alpha;
    //float talphatest = gluu->alphaTest;
    
    int currentDlevel = state[stateId].distanceLevel;
    for (int i = 0; i < distancelevel[currentDlevel].iloscs; i++) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&distancelevel[currentDlevel].subobiekty[i].graphics->VAO);

        if(((state[stateId].enabledSubObjs >> i) & 1) == 0)
            continue;
        
        for (int j = 0; j < distancelevel[currentDlevel].subobiekty[i].iloscc; j++) {

            int prim_state = distancelevel[currentDlevel].subobiekty[i].czesci[j].prim_state_idx;
            int vtx_state = primstate[prim_state].vtx_state;
            int matrix = vtxstate[vtx_state].matrix;
            bool texEnabled = distancelevel[currentDlevel].subobiekty[i].czesci[j].enabled;

            if(animated){
                Mat4::identity(m);
                getPmatrixAnimated(currentDlevel, m, matrix, state[stateId].frameCount);
                oldmatrix = -1;
                gluu->currentMsMatrinxHash = 0;
                gluu->currentShader->setUniformValue(gluu->currentShader->msMatrixUniform, *reinterpret_cast<float(*)[4][4]>(&m));
            } else {
                if (oldmatrix != matrix) {
                    oldmatrix = matrix;
                    if (!macierz[matrix].isFixed) {
                        Mat4::identity(m);
                        memcpy(macierz[matrix].fixed, getPmatrix(currentDlevel, m, matrix), sizeof (float) * 16);
                        macierz[matrix].isFixed = true;
                        macierz[matrix].hash = gluu->getMatrixHash(macierz[matrix].fixed);
                    }
                    if(macierz[matrix].hash != gluu->currentMsMatrinxHash){
                        gluu->currentMsMatrinxHash = macierz[matrix].hash;
                        gluu->currentShader->setUniformValue(gluu->currentShader->msMatrixUniform, *reinterpret_cast<float(*)[4][4]>(&macierz[matrix].fixed));
                    }
                }
            }
            
            if( vtxstate[vtx_state].arg2 < -7 )
                gluu->disableNormals();
            else
                gluu->enableNormals();
            
            if( vtxstate[vtx_state].arg2 == -12 )
                gluu->setBrightness(0.5);
            else
                gluu->setBrightness(1.0);
            
            /*if(primstate[prim_state].arg2 < this->ishaders)
                gluu->alpha = shader[primstate[prim_state].arg2].alpha;
            else 
                gluu->alpha = 0;
            gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlpha, gluu->alpha);

            if(primstate[prim_state].arg6 == 1)
                gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlphaTest, 0.51f);
            else 
                gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlphaTest, gluu->alphaTest);
            */
            const auto useUntexturedMaterial = [gluu, selectionColor]() {
                if(selectionColor == 0)
                    gluu->disableTextures(1.0, 0.0, 1.0, 1.0);
            };
            const int textureSlot = primstate[prim_state].arg4;
            const int imageId = textureSlot >= 0 && textureSlot < ilosct
                    ? texture[textureSlot].image
                    : -1;
            const bool validImage = imageId >= 0 && imageId < ilosci;

            if(!validImage || !texEnabled){
                useUntexturedMaterial();
            } else if(TexLib::disabledTextures.value(image[imageId].texAddr) == 1){
                useUntexturedMaterial();
            } else if(image[imageId].texAddr >= 0){
                if(selectionColor == 0){
                    gluu->enableTextures();
                    gluu->bindTexture(f, image[imageId].texAddr);
                }
            } else if(image[imageId].tex == -2){
                useUntexturedMaterial();
            } else if(image[imageId].tex == -1){
                image[imageId].tex = loadTexture(image[imageId].name);
                useUntexturedMaterial();
            } else {
                const auto textureIt = TexLib::mtex.find(image[imageId].tex);
                Texture *shapeTexture = textureIt == TexLib::mtex.end()
                        ? nullptr
                        : textureIt->second;
                if(shapeTexture == nullptr){
                    useUntexturedMaterial();
                } else {
                    if(!shapeTexture->glLoaded && shapeTexture->loaded)
                        shapeTexture->GLTextures();

                    if(shapeTexture->glLoaded){
                        image[imageId].texAddr = shapeTexture->tex[0];
                        if(selectionColor == 0){
                            gluu->enableTextures();
                            gluu->bindTexture(f, image[imageId].texAddr);
                        }
                    } else {
                        useUntexturedMaterial();
                    }
                }
            }
            
            const czes &part = distancelevel[currentDlevel].subobiekty[i].czesci[j];
            RouteFrameProfile::count(RouteFrameProfile::Counter::ShapeDraws);
            if (instancing) {
                RouteFrameProfile::count(staticScenery ? RouteFrameProfile::Counter::StaticShapeInstancedDraws
                                                       : RouteFrameProfile::Counter::PolyVegInstancedDraws);
                RouteFrameProfile::count(staticScenery ? RouteFrameProfile::Counter::StaticShapePrimitiveInstances
                                                       : RouteFrameProfile::Counter::PolyVegInstances, instanceCount);
                constexpr int firstColumn = 5;
                for (int column = 0; column < 4; ++column) {
                    const int attribute = firstColumn + column;
                    f->glEnableVertexAttribArray(attribute);
                    f->glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE,
                        sizeof(std::array<float, 16>),
                        reinterpret_cast<const void *>(column * 4 * sizeof(float)));
                    instancing->glVertexAttribDivisor(attribute, 1);
                }
                Shader *shader = gluu->currentShader;
                shader->setUniformValue(shader->polyVegInstances, GLint(1));
                instancing->glDrawArraysInstanced(GL_TRIANGLES, part.offset, part.iloscv, instanceCount);
                shader->setUniformValue(shader->polyVegInstances, GLint(0));
                for (int column = 0; column < 4; ++column) {
                    const int attribute = firstColumn + column;
                    instancing->glVertexAttribDivisor(attribute, 0);
                    f->glDisableVertexAttribArray(attribute);
                }
            } else {
                if (selectionColor == 0 && RouteFrameProfile::collectingShapeDraws()) {
                    static const QString untextured = QStringLiteral("<untextured>");
                    RouteFrameProfile::countShapeDraw(pathid,
                        validImage && texEnabled ? image[imageId].name : untextured);
                }
                f->glDrawArrays(GL_TRIANGLES, part.offset, part.iloscv);
            }
            
            if(selectionColor == 0)
                gluu->enableTextures();
        }
    }
    //gluu->currentShader->setUniformValue(gluu->currentShader->shaderAlphaTest, gluu->alphaTest);
    gluu->setBrightness(1.0);
}

void SFile::fillContentHierarchyInfo(QVector<ContentHierarchyInfo*>& list, int parent){
    if (isinit != 1 || loaded != 1)
        return;
    
    ContentHierarchyInfo *info = new ContentHierarchyInfo();
    info->parent = parent;
    info->name = nazwa;
    info->distanceLevelId = -1; // Default
    info->sfile = this;
    info->type = "shape";
    list.push_back(info);
    parent = list.size()-1;
    
    for (int i = 0; i < iloscd; i++) {
        ContentHierarchyInfo *info = new ContentHierarchyInfo();
        info->parent = parent;
        info->name = "Distance Level: " + QString::number(distancelevel[i].levelSelection) + " m";
        info->distanceLevelId = i;
        info->sfile = this;
        info->type = "shape";
        list.push_back(info);
    }
}

void SFile::enablePart(unsigned int uid, unsigned int stateId){
    if (isinit != 1 || loaded != 1)
        return;
    
    int currentDlevel = state[stateId].distanceLevel;

    unsigned int i = uid / 1000;
    unsigned int j = uid - i*1000;
    distancelevel[currentDlevel].subobiekty[i].czesci[j].enabled = true;
}

void SFile::disablePart(unsigned int uid, unsigned int stateId){
    if (isinit != 1 || loaded != 1)
        return;
    
    int currentDlevel = state[stateId].distanceLevel;

    unsigned int i = uid / 1000;
    unsigned int j = uid - i*1000;
    distancelevel[currentDlevel].subobiekty[i].czesci[j].enabled = false;
}

void SFile::fillShapeHierarchyInfo(ShapeHierarchyInfo* info, unsigned int stateId){
    if(info == NULL)
        return;
    
    if (isinit != 1 || loaded != 1)
        return;
    
    int currentDlevel = state[stateId].distanceLevel;
    
    for(int i = 0; i < distancelevel[currentDlevel].ilosch; i++){
        info->hierarchy.push_back(distancelevel[currentDlevel].hierarchia[i]);
    }

    for(int i = 0; i < iloscm; i++){
        info->matrices.push_back(macierz[i].name);
    }

    for (int i = 0; i < distancelevel[currentDlevel].iloscs; i++) {
        for (int j = 0; j < distancelevel[currentDlevel].subobiekty[i].iloscc; j++) {
            int prim_state = distancelevel[currentDlevel].subobiekty[i].czesci[j].prim_state_idx;
            int vtx_state = primstate[prim_state].vtx_state;
            int matrix = vtxstate[vtx_state].matrix;
            
            info->parts.push_back(ShapeHierarchyInfo::ShapePart());
            info->parts.last().matrixId = matrix;
            if(primstate[prim_state].arg4 >= 0)
                info->parts.last().textureName = image[texture[primstate[prim_state].arg4].image].name;
            info->parts.last().polyCount = distancelevel[currentDlevel].subobiekty[i].czesci[j].iloscv/3;
            info->parts.last().uid = i*1000 + j;
            info->parts.last().enabled = distancelevel[currentDlevel].subobiekty[i].czesci[j].enabled;
        }
    }
}

void SFile::fillShapeTextureInfo(QHash<int, ShapeTextureInfo*>& list, unsigned int stateId){
    if (isinit != 1 || loaded != 1)
        return;
    
    int currentDlevel = state[stateId].distanceLevel;
    
    for (int i = 0; i < distancelevel[currentDlevel].iloscs; i++) {
        for (int j = 0; j < distancelevel[currentDlevel].subobiekty[i].iloscc; j++) {
            int prim_state = distancelevel[currentDlevel].subobiekty[i].czesci[j].prim_state_idx;

            if(primstate[prim_state].arg4 == -1){
                continue;
            } 
            ShapeTextureInfo *tInfo = new ShapeTextureInfo();
            tInfo->enabled = true;
            tInfo->textureName = image[texture[primstate[prim_state].arg4].image].name;
            tInfo->loaded = "NONE";
            list[image[texture[primstate[prim_state].arg4].image].tex] = tInfo;
            
            if (image[texture[primstate[prim_state].arg4].image].tex == -2){
                continue;
            }
            if (image[texture[primstate[prim_state].arg4].image].tex == -1){
                tInfo->loading = true;
                continue;
            }

            Texture* ttex = TexLib::mtex[image[texture[primstate[prim_state].arg4].image].tex];
            if(ttex == NULL)
                continue;     
            
            if (!ttex->loaded && !ttex->missing && !ttex->error ) {
                tInfo->loading = true;
                continue;
            }
            
            if(ttex->missing){
                tInfo->loaded = "MISSING";
                qWarning() << "Missing Texture: " << tInfo->textureName;
                continue;
            }
            if(ttex->error){
                tInfo->loaded = "ERROR";
                qWarning() << "Texture Error: " << tInfo->textureName;                
                continue;
            }
            
            if (!ttex->glLoaded) {
                tInfo->loading = true;
                continue;
            }
       
            tInfo->loaded = "YES";
            tInfo->resolution = QString::number(ttex->width) + "x" + QString::number(ttex->height);
            tInfo->textureId = image[texture[primstate[prim_state].arg4].image].tex;
            
            Route::texturesList.append(tInfo->textureName);
            
        }
    }
}


/*======================================================
===== Przekształcenia takie tam
=======================================================*/
QMap<int, QVector3D> SFile::wireAttachmentOrigins() {
    QMap<int, QVector3D> origins;
    if(loaded != 1 || iloscd < 1) return origins;
    QSet<int> seen;
    for(int i = 1; i < iloscm; ++i) {
        const int id = WireAttachmentOrder::attachmentId(macierz[i].name);
        if(id == 0) continue;
        // Ambiguous duplicate names cannot identify a unique connection point.
        if(seen.contains(id)) { origins.remove(id); continue; }
        seen.insert(id);
        float point[3];
        if(attachmentOrigin(macierz[i].name, point)
                && std::isfinite(point[0]) && std::isfinite(point[1]) && std::isfinite(point[2]))
            origins.insert(id, QVector3D(point[0], point[1], point[2]));
    }
    return origins;
}

bool SFile::attachmentOrigin(const QString &name, float *origin) {
    if(loaded != 1 || iloscd < 1 || iloscm < 1) return false;
    for(int i = 1; i < iloscm; ++i) {
        if(macierz[i].name.compare(name, Qt::CaseInsensitive) != 0) continue;
        int parent = i;
        int depth = 0;
        while(parent != 0 && parent != -1) {
            if(parent < 0 || parent >= iloscm
                    || parent >= distancelevel[0].ilosch || ++depth > iloscm)
                return false;
            parent = distancelevel[0].hierarchia[parent];
        }
        float transform[16];
        getPmatrix(0, transform, i);
        for(int axis = 0; axis < 3; ++axis) origin[axis] = transform[12 + axis];
        return true;
    }
    return false;
}

float* SFile::getPmatrix(int currentDlevel, float* pmatrix, int matrix) {
    if (matrix == -1 || matrix == 0) {
        Mat4::identity(pmatrix);
        pmatrix[0] = -1;
        return pmatrix;
    } else {
        pmatrix = getPmatrix(currentDlevel, pmatrix, distancelevel[currentDlevel].hierarchia[matrix]);
    }
    Mat4::multiply(pmatrix, pmatrix, macierz[matrix].param);
    return pmatrix;
}

float* SFile::getPmatrixAnimated(int currentDlevel, float* pmatrix, int matrix, float frame){
    if (matrix == -1 || matrix == 0) {
        Mat4::identity(pmatrix);
        pmatrix[0] = -1;
        return pmatrix;
    } else {
        pmatrix = getPmatrixAnimated(currentDlevel, pmatrix, distancelevel[currentDlevel].hierarchia[matrix], frame);
    }
    float m[16];
    memcpy(m, macierz[matrix].param, sizeof (float) * 16);
    if(animations[0].node.size() > matrix){
        AnimNode *n = &animations[0].node[matrix];
        if(frame + 1 < n->tcbId.size()){
            float m1[16];
            float q1[4];
            float q2[4];
            float q3[4];
            Quat::slerp(q1, n->tcbKey[n->tcbId[(int)frame].id1].quat, n->tcbKey[n->tcbId[(int)frame].id2].quat, n->tcbId[(int)frame].offset);
            Quat::slerp(q2, n->tcbKey[n->tcbId[(int)frame+1].id1].quat, n->tcbKey[n->tcbId[(int)frame+1].id2].quat, n->tcbId[(int)frame+1].offset);
            Quat::slerp(q3, q1, q2, frame - (int)frame);
            Mat4::fromQuat(m1, q3);
            Mat4::multiply(m, m, m1);
        }
        if(frame + 1 < n->linearId.size()){
            float v1[3], v2[3], v3[3];
            Vec3::lerp(v1, n->linearKey[n->linearId[(int)frame].id1].pos, n->linearKey[n->linearId[(int)frame].id2].pos, n->linearId[(int)frame].offset);
            Vec3::lerp(v2, n->linearKey[n->linearId[(int)frame+1].id1].pos, n->linearKey[n->linearId[(int)frame+1].id2].pos, n->linearId[(int)frame+1].offset);
            Vec3::lerp(v3, v1, v2, frame - (int)frame);
            
            m[12] = v3[0];
            m[13] = v3[1];
            m[14] = v3[2];
        }
        //slerprot
    }
    Mat4::multiply(pmatrix, pmatrix, m);
    return pmatrix;
}

void SFileGeometry::buildFrameIds(){
    for(int i = 0; i < animations.size(); i++){
        for(int j = 0; j < animations[i].node.size(); j++){
            int frameCount = animations[i].frames;
            AnimNode *n = &animations[i].node[j];

            for(int y = 0; y < frameCount + 1; y++){
                if(n->tcbKey.size() > 0){
                    n->tcbId.push_back(AnimFrameId());
                    n->tcbId.back().id1 = 0;
                    n->tcbId.back().id2 = 0;
                    n->tcbId.back().offset = 0;
                    //qDebug() << "n->tcbKey.size()"<< n->tcbKey.size();
                    for(int u = 0; u < n->tcbKey.size() - 1; u++){
                        if(n->tcbKey[u+1].frame > y){
                            n->tcbId.back().id1 = u;
                            n->tcbId.back().id2 = u+1;
                            n->tcbId.back().offset = ((float)y - (float)n->tcbKey[u].frame)/((float)n->tcbKey[u+1].frame - (float)n->tcbKey[u].frame);
                            break;
                        }
                    }
                }
                if(n->linearKey.size() > 0){
                    n->linearId.push_back(AnimFrameId());
                    n->linearId.back().id1 = 0;
                    n->linearId.back().id2 = 0;
                    n->linearId.back().offset = 0;
                    //qDebug() << "n->tcbKey.size()"<< n->tcbKey.size();
                    for(int u = 0; u < n->linearKey.size() - 1; u++){
                        if(n->linearKey[u+1].frame > y){
                            n->linearId.back().id1 = u;
                            n->linearId.back().id2 = u+1;
                            n->linearId.back().offset = ((float)y - (float)n->linearKey[u].frame)/((float)n->linearKey[u+1].frame - (float)n->linearKey[u].frame);
                            break;
                        }
                    }
                }
                //qDebug() << y << n->tcbId.back().id1 << n->tcbId.back().id2 << n->tcbId.back().offset;
/*
                n->linearId.push_back(AnimFrameId());
                n->linearId.back().id = 0;
                n->linearId.back().offset = 0;
                for(int u = 0; u < n->linearKey.size() - 1; u++){
                    if(n->linearKey[u+1].frame > y){
                        n->linearId.back().id = u;
                        n->linearId.back().offset = (n->linearKey[u+1].frame - n->linearKey[u].frame)/(y - n->linearKey[u].frame);
                        break;
                    }
                }

                n->slerpbId.push_back(AnimFrameId());
                n->slerpbId.back().id = 0;
                n->slerpbId.back().offset = 0;
                for(int u = 0; u < n->slerpRot.size() - 1; u++){
                    if(n->slerpRot[u+1].frame > y){
                        n->slerpbId.back().id = u;
                        n->slerpbId.back().offset = (n->slerpRot[u+1].frame - n->slerpRot[u].frame)/(y - n->slerpRot[u].frame);
                        break;
                    }
                }*/
            }
        }
    }
}

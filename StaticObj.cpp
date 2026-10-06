/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "StaticObj.h"
#include "SFile.h"
#include "ShapeLib.h"
#include "GLMatrix.h"
#include <math.h>
#include <algorithm>
#include "ParserX.h"
#include "TS.h"
#include "TrackItemObj.h"
#include <QDebug>
#include "Game.h"
#include <QMenu>
#include "ErrorMessagesLib.h"
#include "ErrorMessage.h"
#include "Renderer.h"
#include "PolyVegObject.h"
#include "GltfPreview.h"
#include "GltfPlacementMath.h"

StaticObj::StaticObj() {
    this->shape = -1;
    this->loaded = false;
    this->modified = false;
}

bool StaticObj::allowNew(){
    return true;
}

StaticObj::StaticObj(const StaticObj& o) : WorldObj(o) {
    gltfShape = o.gltfShape;
    snapablePoints.append(o.snapablePoints);
    
}

WorldObj* StaticObj::clone(){
    return new StaticObj(*this);
}

StaticObj::~StaticObj() {
}

void StaticObj::loadingFixes(){
    if(Game::useOnlyPositiveQuaternions){
        if(qDirection[3] < 0){
            Quat::makePositive(qDirection);
            ErrorMessage *e = new ErrorMessage(
                    ErrorMessage::Type_Info, 
                    ErrorMessage::Source_Editor, 
                    QString("Fixed negative quaternion in tile ") + QString::number(x) + " " + QString::number(y) + " : " + QString::number(typeID) );
            ErrorMessagesLib::PushErrorMessage(e);
            setModified();
        }            
    }
}

ErrorMessage* StaticObj::checkForErrors(){
    if(gltfShape && !gltfShape->loadError.isEmpty()) {
        auto *e = new ErrorMessage(ErrorMessage::Type_Warning, ErrorMessage::Source_World,
                "Unable to display glTF scenery: " + fileName, gltfShape->loadError
                + "\nThe existing world reference is retained for saving and repair.");
        e->setObject((GameObj*)this);
        e->setLocationXYZ(x, -y, position[0], position[1], -position[2]);
        ErrorMessagesLib::PushErrorMessage(e);
        return e;
    }
    
    if(abs(position[0]) > 2047 || abs(position[2]) > 2047){
        ErrorMessage *e = new ErrorMessage(
                ErrorMessage::Type_Warning, 
                ErrorMessage::Source_World, 
                QString("StaticObj " + fileName + " seems to be located too far from it's Tile. ") + QString::number(x) + " " + QString::number(y) + " : " + QString::number(UiD),
                            "This location may cause unwanted behavior. \n"
                            "Jump to it's location or select it and check if it should be moved or removed. "
                            );
        e->setObject((GameObj*)this);
        e->setLocationXYZ(x, -y, position[0], position[1], -position[2]);
        ErrorMessagesLib::PushErrorMessage(e);
        return e;
    }
    
    /// EFO look for deep underground...        
    if(position[1] <= Game::deepUnderground ) {
                                  
        if(Game::debugOutput) qDebug() << "Warning: Object " << fileName << " at " << x << " " << y << " "  << position[0] << " " << position[1] << " " << position[2] << " is considerably underground.";
        
        ErrorMessage *e = new ErrorMessage(
                ErrorMessage::Type_Warning, 
                ErrorMessage::Source_World, 
                QString("StaticObj " + fileName + " is deep underground: ") + QString::number(position[1]),
                    fileName + " at " + 
                        " " + QString::number(x) + 
                        " " + QString::number(y) +
                        " " + QString::number(position[0]) + 
                        " " + QString::number(position[1]) + 
                        " " + QString::number(position[2]) + 
                        " might be a stray track piece or other object that got lost when moving without the terrain visible.\n\n To fix, you can use \"Transform\" to adjust the position values or possibly the \"Z\" and \"H\" key to try to bring it back to the surface."
                );
        e->setObject((GameObj*)this);
        e->setLocationXYZ(x, -y, position[0], position[1], -position[2]);
        ErrorMessagesLib::PushErrorMessage(e);
    }                        

    return NULL;
}

void StaticObj::load(int x, int y) {
    if(GltfModel::accepts(fileName)) {
        QString error;
        gltfShape = Game::currentShapeLib->getGltfShape(resPath + "/" + fileName, error);
        shape = -1;
        shapePointer = nullptr;
        shapeState = 0;
        gltfRenderFailed = false;
    } else {
        gltfShape.reset();
        this->shape = Game::currentShapeLib->addShape(resPath +"/"+ fileName);
        this->shapePointer = Game::currentShapeLib->shape[this->shape];
        this->shapeState = shapePointer->newState();
        shapePointer->setAnimated(shapeState, isAnimated());
    }
    this->x = x;
    this->y = y;
    this->position[2] = -this->position[2];
    this->qDirection[2] = -this->qDirection[2];
    this->loaded = true;
    this->size = -1;
    this->skipLevel = 1;
    this->box.loaded = false;
            
    setMartix();
}

void StaticObj::set(QString sh, QString val){
    if (sh == ("filename")) {
        fileName = val;
        return;
    }
    if (sh == ("ref_filename")) {
        fileName = val;
        return;
    }
    WorldObj::set(sh, val);
    return;
}

void StaticObj::set(int sh, FileBuffer* data) {
    if (sh == TS::FileName) {
        data->off++;
        int slen = data->getShort()*2;
        fileName = *data->getString(data->off, data->off + slen);
        data->off += slen;
        return;
    }
    if (sh == TS::NoDirLight) {
        //data->off++;
        return;
    }
    WorldObj::set(sh, data);
    return;
}

void StaticObj::set(QString sh, FileBuffer* data) {
    if (sh == ("filename")) {
        fileName = ParserX::GetString(data);
        return;
    }
    WorldObj::set(sh, data);
    return;
}

void StaticObj::updateSim(float deltaTime){
    if (!loaded) return;
    if (shape < 0) return;
    if (jestPQ < 2) return;
    
    if(shapePointer != NULL)
        shapePointer->updateSim(deltaTime, shapeState);
}

void StaticObj::pushRenderItems(float lod, float posx, float posz, float* playerW, float* target, float fov, int selectionColor){
    if(!Game::viewPolyVeg && PolyVegObject::isVegetationShape(fileName, polyVegRaw)) return;
    // The batched renderer is inactive in this editor. GLB is currently scoped
    // to the active immediate renderer; do not put it in the SFile-only queue.
    if(gltfShape) return;
    if (!loaded) return;
    if (shape < 0) return;
    if (jestPQ < 2) return;
    //if (renderMode == gluu->RENDER_SHADOWMAP && Game::mstsShadows) {
    //    if(this->getShadowType() != WorldObj::ShadowDynamic )
    //        return;
    //}
    //GLUU* gluu = GLUU::get();
    //if((this.position===undefined)||this.qDirection===undefined) return;
    
    if (size > 0) {
        if ((lod > size + 150)) {
            float v1[2];
            v1[0] = playerW[0] - (target[0]);
            v1[1] = playerW[2] - (target[2]);
            float v2[2];
            v2[0] = posx;
            v2[1] = posz;
            float iloczyn = v1[0] * v2[0] + v1[1] * v2[1];
            float d1 = sqrt(v1[0] * v1[0] + v1[1] * v1[1]);
            float d2 = sqrt(v2[0] * v2[0] + v2[1] * v2[1]);
            float zz = iloczyn / (d1 * d2);
            if (zz > 0) return;

            float ccos = cos(fov) + zz;
            float xxx = sqrt(2 * d2 * d2 * (1 - ccos));
            //if((ccos > 0) && (xxx > 200+50)) return;
            if ((ccos > 0) && (xxx > size) && (skipLevel == 1)) return;
        }
    } else {
        if (Game::currentShapeLib->shape[shape]->loaded){
            size = Game::currentShapeLib->shape[shape]->size
                    * std::max(matrixScale[0], std::max(matrixScale[1], matrixScale[2]));
            loadSnapablePoints();
        }
    }

    //if(Game::viewSnapable)
    //    if(snapablePoints.size() == 6)
    //        renderSnapableEndpoints(gluu);  
    
    Mat4::multiply(Game::currentRenderer->mvMatrix, Game::currentRenderer->mvMatrix, matrix);
    
    if(Game::showWorldObjPivotPoints){
        if(pointer3d == NULL){
            pointer3d = new TrackItemObj(1);  
            pointer3d->setMaterial(0.9,0.9,0.7);
        }
        //pointer3d->pushRenderItem(selectionColor);
    }
    
    /*if(selectionColor != 0){
        gluu->disableTextures(selectionColor);
    } else {
        gluu->enableTextures();
    }*/
    
    if(shapePointer != NULL){
        Game::currentRenderer->rawPreview = polyVegRaw && selectionColor == 0;
        shapePointer->pushRenderItem(selectionColor, shapeState);
        Game::currentRenderer->rawPreview = false;
    }

    if(selected){
        //drawBox();
    }
}

void StaticObj::render(GLUU* gluu, float lod, float posx, float posz, float* pos, float* target, float fov, int selectionColor, int renderMode) {
    if(!Game::viewPolyVeg && PolyVegObject::isVegetationShape(fileName, polyVegRaw)) return;
    if(gltfShape) {
        if(!loaded || jestPQ < 2 || gltfShape->model.primitives.empty()) return;
        if(renderMode == gluu->RENDER_SHADOWMAP && Game::mstsShadows
                && getShadowType() != WorldObj::ShadowDynamic) return;
        Mat4::multiply(gluu->mvMatrix, gluu->mvMatrix, matrix);
        const QMatrix4x4 world = GltfPlacementMath::matrix(gluu->mvMatrix);
        QMatrix4x4 projection;
        const float *projectionValues = renderMode == gluu->RENDER_SHADOWMAP
                ? gluu->pShadowMatrix : gluu->pMatrix;
        std::copy(projectionValues, projectionValues + 16, projection.data());
        QString error;
        if(!gltfShape->draw(projection * world, error, selectionColor, world.determinant() < 0)) {
            if(!gltfRenderFailed) qWarning() << "glTF draw failed:" << error;
            gltfRenderFailed = true;
            return;
        }
        gltfRenderFailed = false;
        size = (gltfShape->model.maximum - gltfShape->model.minimum).length()
                * std::max({matrixScale[0], matrixScale[1], matrixScale[2]});
        if(selected && renderMode == gluu->RENDER_DEFAULT) {
            if(gltfBoxRevision != gltfShape->revision) {
                box.deleteVBO();
                gltfBoxRevision = gltfShape->revision;
            }
            gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform,
                    *reinterpret_cast<float(*)[4][4]>(gluu->mvMatrix));
            drawBox();
        }
        return;
    }
    if (!loaded) return;
    if (shape < 0) return;
    if (jestPQ < 2) return;
    if (renderMode == gluu->RENDER_SHADOWMAP && Game::mstsShadows) {
        if(this->getShadowType() != WorldObj::ShadowDynamic )
            return;
    }
    //GLUU* gluu = GLUU::get();
    //if((this.position===undefined)||this.qDirection===undefined) return;
    
    if (size > 0) {
        if ((lod > size + 150)) {
            float v1[2];
            v1[0] = pos[0] - (target[0]);
            v1[1] = pos[2] - (target[2]);
            float v2[2];
            v2[0] = posx;
            v2[1] = posz;
            float iloczyn = v1[0] * v2[0] + v1[1] * v2[1];
            float d1 = sqrt(v1[0] * v1[0] + v1[1] * v1[1]);
            float d2 = sqrt(v2[0] * v2[0] + v2[1] * v2[1]);
            float zz = iloczyn / (d1 * d2);
            if (zz > 0) return;

            float ccos = cos(fov) + zz;
            float xxx = sqrt(2 * d2 * d2 * (1 - ccos));
            //if((ccos > 0) && (xxx > 200+50)) return;
            if ((ccos > 0) && (xxx > size) && (skipLevel == 1)) return;
        }
    } else {
        if (Game::currentShapeLib->shape[shape]->loaded){
            size = Game::currentShapeLib->shape[shape]->size
                    * std::max(matrixScale[0], std::max(matrixScale[1], matrixScale[2]));
            loadSnapablePoints();
        }
    }

    if(Game::viewSnapable)
        if(snapablePoints.size() == 6)
            renderSnapableEndpoints(gluu);  
    
    Mat4::multiply(gluu->mvMatrix, gluu->mvMatrix, matrix);
    gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
    
    if(Game::showWorldObjPivotPoints){
        if(pointer3d == NULL){
            pointer3d = new TrackItemObj(1);
            pointer3d->setMaterial(0.9,0.9,0.7);
        }
        pointer3d->render(selectionColor);
    }
    
    if(selectionColor != 0){
        gluu->disableTextures(selectionColor);
    } else {
        gluu->enableTextures();
    }
    
    if(shapePointer != NULL) {
        gluu->currentShader->setUniformValue(gluu->currentShader->shaderRawPreview,
            polyVegRaw && selectionColor == 0 && renderMode == gluu->RENDER_DEFAULT ? 1.0f : 0.0f);
        shapePointer->render(selectionColor, shapeState);
        gluu->currentShader->setUniformValue(gluu->currentShader->shaderRawPreview, 0.0f);
    }
    //Game::currentShapeLib->shape[shape]->render(isAnimated());
    
    if(selected){
        drawBox();
    }
};

void StaticObj::snapped(int side){
    if(side > -1){
        QVector<float> points;
        loadSnapablePoints();
        insertSnapablePoints(points);
        if(points.size() > 11){
            if(side%2 == 0)
                Vec3::sub(position, position, (float*)&points[11]);
            else
                Vec3::sub(position, position, (float*)&points[0]);
            setMartix();
        }
    }
}

bool StaticObj::hasLinePoints(){
    if(gltfShape) return false;
    return true;
}

void StaticObj::getLinePoints(float *&punkty){
    float *beg = punkty;
    if(shapePointer != NULL)
        shapePointer->getFloorBorderLinePoints(punkty);
    int len = punkty - beg;
    for(int i = 0; i < len; i+=3){
        beg[i+0] = -beg[i+0];
        beg[i+2] = beg[i+2];
        Vec3::transformMat4(&beg[i], &beg[i], matrix);
        qDebug() << beg[i] << beg[i+1] << beg[i+2];
    }
    return;
}

void StaticObj::loadSnapablePoints() {
    if(gltfShape || !shapePointer) return;
    if(snapable == true)
        return;
    snapable = this->shapePointer->isSnapable();
    if(snapable)
        this->shapePointer->addSnapablePoints(this->snapablePoints);
}

void StaticObj::renderSnapableEndpoints(GLUU* gluu) {
    if (snapableEndPoint == NULL) {
        snapableEndPoint = new OglObj();
        float *punkty = new float[3 * 2];
        int ptr = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 30;
        punkty[ptr++] = 0;
        snapableEndPoint->setMaterial(0.0, 1.0, 0.0);
        snapableEndPoint->init(punkty, ptr, RenderItem::V, GL_LINES);
        delete[] punkty;
    }
    
    float vec[3];
    for(int i = 0; i < 6; i+=3){
        Vec3::transformQuat(vec, (float*)&snapablePoints[i], qDirection);
        Vec3::add(vec, vec, position);
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, vec);
        gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        snapableEndPoint->render();
        gluu->mvPopMatrix();
    }
}

void StaticObj::insertSnapablePoints(QVector<float>& points){
    if(snapablePoints.size() == 6){
        float vec[3];
        for(int i = 0; i < 6; i+=3){
            Vec3::transformQuat(vec, (float*)&snapablePoints[i], qDirection);
            points.push_back(vec[0]);
            points.push_back(vec[1]);
            points.push_back(vec[2]);
            Vec3::add(vec, vec, position);
            points.push_back(vec[0]);
            points.push_back(vec[1]);
            points.push_back(vec[2]);
            
            points.push_back(qDirection[0]);
            points.push_back(qDirection[1]);
            points.push_back(qDirection[2]);
            points.push_back(qDirection[3]);
            points.push_back(UiD);
        }
    }
    return;
}

bool StaticObj::getSimpleBorder(float* border){
    if(gltfShape) {
        if(gltfShape->model.primitives.empty()) return false;
        for(int axis = 0; axis < 3; ++axis) {
            border[axis * 2] = gltfShape->model.minimum[axis];
            border[axis * 2 + 1] = gltfShape->model.maximum[axis];
        }
        return true;
    }
    if (shapePointer == 0) return false;
    if (!shapePointer->loaded)
        return false;
    float* bound = shapePointer->bound;
    border[0] = bound[0];
    border[1] = bound[1];
    border[2] = bound[2];
    border[3] = bound[3];
    border[4] = bound[4];
    border[5] = bound[5];
    return true;
}

bool StaticObj::getBoxPoints(QVector<float>& points){
    if(gltfShape) {
        if(gltfShape->model.primitives.empty()) return false;
        GltfPlacementMath::boxPoints(gltfShape->model.minimum, gltfShape->model.maximum, points);
        return true;
    }
    if (shapePointer == 0) return false;
    if (!shapePointer->loaded)
        return false;
    return shapePointer->getBoxPoints(points);
}

QString StaticObj::getName(){
    return PolyVegObject::labelForShape(fileName, polyVegRaw);
}

QString StaticObj::getShapePath(){
    if (!loaded) return "";
    if(gltfShape) return gltfShape->sourcePath;
    if (shapePointer == 0) return "";
    return shapePointer->pathid+"|"+shapePointer->texPath;
}

int StaticObj::getDefaultDetailLevel(){
    if (!loaded) return 0;
    if (shapePointer == 0) return 0;
    int esdDLevel = shapePointer->esdDetailLevel;
    if(esdDLevel >= 0) return esdDLevel;
    return 0;
}

int StaticObj::getCollisionType(){
    if(this->type == "collideobject"){
        bool enabled = ((this->collideFlags & 2) >> 1) == 0;
        if(enabled){
            if(this->collideFunction == 1 )
                return 2;
            else 
                return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void StaticObj::setCollisionType(int val){
    setModified();
    if(val < 0){
        if(this->type == "collideobject")
            collideFlags = collideFlags | (2);
        return;
    }
    if(this->type != "collideobject"){
        type = "collideobject";
        typeID = WorldObj::collideobject;
        collideFlags = 32;
    }
    collideFlags = collideFlags & (~2);
    collideFunction = val;
    return;
}

void StaticObj::removeCollisions(){
    if(type != "static"){
        type = "static";
        typeID = WorldObj::sstatic;
        collideFlags = 0;
        collideFunction = 0;
        setModified();
    }
}

void StaticObj::reload(){
    if(gltfShape) {
        QString error;
        if(!gltfShape->load(resPath + "/" + fileName, error)) qWarning() << "glTF reload failed:" << error;
        // The next draw refreshes the bounds in the owning OpenGL context.
        else { gltfRenderFailed = false; }
        return;
    }
    if(shapePointer != NULL)
        shapePointer->reload();
}

void StaticObj::save(QTextStream* out){
    if (!loaded) return;
    if (jestPQ < 2) return;
    if(Game::useOnlyPositiveQuaternions)
        Quat::makePositive(this->qDirection);
    
if(type == "static")
*(out) << "	Static (\n";
if(type == "gantry")
*(out) << "	Gantry (\n";
if(type == "collideobject")
*(out) << "	CollideObject (\n";

*(out) << "		UiD ( "<<this->UiD<<" )\n";
if(type == "collideobject"){
*(out) << "		CollideFlags ( "<<this->collideFlags<<" )\n";
if(this->collideFunction > 0 )
*(out) << "		CollideFunction ( "<<this->collideFunction<<" )\n";
}
*(out) << "		FileName ( "<<ParserX::AddComIfReq(this->fileName)<<" )\n";
if(this->staticFlags != 0)
*(out) << "		StaticFlags ( "<<ParserX::MakeFlagsString(this->staticFlags)<<" )\n";
*(out) << "		Position ( "<<this->position[0]<<" "<<this->position[1]<<" "<<-this->position[2]<<" )\n";
if(matrix3x3 != NULL || hasNonUnitMatrixScale())
    writeMatrix3x3(out);
else
    *(out) << "		QDirection ( "<<this->qDirection[0]<<" "<<this->qDirection[1]<<" "<<-this->qDirection[2]<<" "<<this->qDirection[3]<<" )\n";
if(Game::legacySupport)   *(out) << "		VDbId ( " << this->vDbId << " )\n";  // EFO 
if(this->staticDetailLevel > -1)
*(out) << "		StaticDetailLevel ( "<<this->staticDetailLevel<<" )\n";
*(out) << "	)\n";
}

bool StaticObj::isSimilar(WorldObj* obj){
    if(obj->typeID != this->typeID)
        return false;
    if(obj->fileName == this->fileName)
        return true;
    return false;
}

void StaticObj::pushContextMenuActions(QMenu *menu){
    /*if(contextMenuActions["Rot"] == NULL){
        contextMenuActions["Rot"] = new QAction(tr("&Rot")); 
        QObject::connect(contextMenuActions["Rot"], SIGNAL(triggered()), this, SLOT(menuRot()));
    }
    menu->addAction(contextMenuActions["Rot"]);*/

}

void StaticObj::menuRot(){
    qDebug() << "aaa";
}

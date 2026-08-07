/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "RulerObj.h"
#include "GLMatrix.h"
#include "TrackObj.h"
#include "GLMatrix.h"
#include "TrackItemObj.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <math.h>
#include "ParserX.h"
#include <QDebug>
#include "Game.h"
#include "DynTrackObj.h"
#include "TDB.h"
#include "TrackShape.h"
#include "TSectionDAT.h"
#include "Route.h"
#include "GeoCoordinates.h"
#include "ProceduralShape.h"
#include "Terrain.h"
#include "TerrainLib.h"

bool RulerObj::TwoPointRuler = false;
bool RulerObj::DrawPoints = false;

RulerObj::RulerObj() {
    this->internalLodControl = true;
    this->shape = -1;
    this->loaded = false;
    this->modified = false;
}

bool RulerObj::allowNew(){
    return true;
}

RulerObj::RulerObj(const RulerObj& o) : WorldObj(o){

    for(int i = 0; i < o.points.size(); i++){
        Point point;
        point.position[0] = o.points[i].position[0];
        point.position[1] = o.points[i].position[1];
        point.position[2] = o.points[i].position[2];
        points.push_back(point);
    }
    selectionValue = o.selectionValue;
    length = o.length;
    geoLength = o.geoLength;
    waterRuler = o.waterRuler;
    vegetationRuler = o.vegetationRuler;
    gradeRuler = o.gradeRuler;
    internalLodControl = o.internalLodControl;
}

WorldObj* RulerObj::clone(){
    return new RulerObj(*this);
}

RulerObj::~RulerObj() {
    delete point3d;
    delete line3d;
    delete point3dSelected;
    delete vegetationPost3d;
    delete vegetationPostSelected3d;
    delete vegetationBounds3d;
    delete vegetationHandle3d;
    delete specialHandlePick3d;
}

void RulerObj::load(int x, int y) {
    this->x = x;
    this->y = y;
    this->position[2] = -this->position[2];
    this->qDirection[2] = -this->qDirection[2];
    this->loaded = true;
    this->size = -1;
    this->skipLevel = 1;
    this->box.loaded = false;
    setMartix();
    //this->point3d = new TrackItemObj();
    //this->point3d->setMaterial(1,1,1);
    //this->point3dSelected = new TrackItemObj();
    //this->point3dSelected->setMaterial(0.5,0.5,0.5);

    if(this->points.size() == 0){
        Point point;
        
        Vec3::copy(point.position, this->position);
        this->points.push_back(point);
        // The legacy two-point preference applies only to the ordinary
        // measurement ruler. Special rulers build their control points from
        // explicit terrain clicks; the grade ruler in particular must wait
        // for its second endpoint instead of synthesizing one here.
        if(TwoPointRuler && !isSpecialRuler()){
            Point point2;
            Vec3::copy(point2.position, this->position);
            point2.position[2] += 1;
            this->points.push_back(point2);
            selectionValue = 1;
        }
    }
}

void RulerObj::set(QString sh, QString val){
    
    WorldObj::set(sh, val);
    return;
}

void RulerObj::set(QString sh, FileBuffer* data) {
    if (sh == ("points")) {
        int pointCount = ParserX::GetNumber(data);
        for(int i=0; i< pointCount; i++){
            Point point;
            point.position[0] = ParserX::GetNumber(data);
            point.position[1] = ParserX::GetNumber(data);
            point.position[2] = -ParserX::GetNumber(data);
            points.push_back(point);
        }
        ParserX::SkipToken(data);
        return;
    }
    if (sh == ("shapetemplate")) {
        shapeEnabled = true;
        templateName = ParserX::GetStringInside(data);
        return;
    }
    if (sh == ("waterruler")) {
        waterRuler = ParserX::GetNumber(data) != 0;
        if(waterRuler){
            vegetationRuler = false;
            gradeRuler = false;
        }
        return;
    }
    if (sh == ("vegetationruler")) {
        vegetationRuler = ParserX::GetNumber(data) != 0;
        if(vegetationRuler){
            waterRuler = false;
            gradeRuler = false;
        }
        return;
    }
    if (sh == ("graderuler")) {
        gradeRuler = ParserX::GetNumber(data) != 0;
        if(gradeRuler){
            waterRuler = false;
            vegetationRuler = false;
        }
        return;
    }
    
    WorldObj::set(sh, data);
    return;
}

void RulerObj::reload(){
    proceduralShapeInit = false;
    for(int j = 0; j < points.size() - 1; j++){
        points[j].procShape.clear();
    }
}

void RulerObj::setTemplate(QString name){
    templateName = name;
    shapeEnabled = true;
    setModified();
    reload();
}

void RulerObj::setPosition(int x, int z, float* p){
    if(isSpecialRuler() && selectionValue >= 0 && selectionValue < points.size()){
        float newX = -2048*(this->x-x) + p[0];
        float newY = p[1];
        float newZ = -2048*(this->y-z) + p[2];
        Point &point = points[selectionValue];
        if(std::fabs(point.position[0] - newX) < 0.001f
                && std::fabs(point.position[1] - newY) < 0.001f
                && std::fabs(point.position[2] - newZ) < 0.001f)
            return;
        point.position[0] = newX;
        point.position[1] = newY;
        point.position[2] = newZ;
        specialPointMoved = true;
        setModified();
        if(line3d != NULL)
            line3d->deleteVBO();
        return;
    }
    if(selectionValue > 0){
        points[selectionValue].position[0] = -2048*(this->x-x) + p[0];
        points[selectionValue].position[1] = p[1];
        points[selectionValue].position[2] = -2048*(this->y-z) + p[2];
    } else {
        Point point;
        point.position[0] = -2048*(this->x-x) + p[0];
        point.position[1] = p[1];
        point.position[2] = -2048*(this->y-z) + p[2];
        if(Vec3::dist(points.back().position, point.position) > 1)
            points.push_back(point);
    }
    setModified();
    if(line3d != NULL)
        line3d->deleteVBO();
}

void RulerObj::refreshLength(){
    length = 0;
    geoLength = 0;
    
    IghCoordinate igh1, igh2;
    LatitudeLongitudeCoordinate latlon1, latlon2;
    PreciseTileCoordinate coords1, coords2;
    
    for(int i = 0; i < points.size() - 1; i++){
        length += Vec3::distance(points[i].position, points[i+1].position);
        
        coords1.setTWxyz(x, -y, points[i].position[0], points[i].position[1], points[i].position[2]);
        coords2.setTWxyz(x, -y, points[i+1].position[0], points[i+1].position[1], points[i+1].position[2]);
        Game::GeoCoordConverter->ConvertToInternal(&coords1, &igh1);
        Game::GeoCoordConverter->ConvertToInternal(&coords2, &igh2);
        Game::GeoCoordConverter->ConvertToLatLon(&igh1, &latlon1);
        Game::GeoCoordConverter->ConvertToLatLon(&igh2, &latlon2);
        geoLength += latlon1.distanceTo(&latlon2);
        
    }
    
    
}

float RulerObj::getElevation(){
    if(points.size() < 2)
        return 0;
    const Point &first = points.first();
    const Point &last = points.last();
    const float dx = last.position[0] - first.position[0];
    const float dz = last.position[2] - first.position[2];
    const float horizontalRun = std::sqrt(dx*dx + dz*dz);
    if(horizontalRun < 0.001f)
        return 0;
    const float rise = last.position[1] - first.position[1];
    return std::atan2(rise, horizontalRun);
}

float RulerObj::getLength(){
    return length;
    
}float RulerObj::getGeoLength(){
    return geoLength;
}

int RulerObj::getPointCount() const {
    return points.size();
}

void RulerObj::getPointWorldPosition(int index, float *pos) const {
    if(pos == NULL || index < 0 || index >= points.size())
        return;
    pos[0] = x * 2048.0f + points[index].position[0];
    pos[1] = points[index].position[1];
    pos[2] = y * 2048.0f + points[index].position[2];
}

bool RulerObj::isWaterRuler() const {
    return waterRuler;
}

void RulerObj::setWaterRuler(bool enabled) {
    waterRuler = enabled;
    if(enabled){
        vegetationRuler = false;
        gradeRuler = false;
    }
    setModified();
    if(point3d != NULL)
        point3d->deleteVBO();
    if(point3dSelected != NULL)
        point3dSelected->deleteVBO();
    if(line3d != NULL)
        line3d->deleteVBO();
}

void RulerObj::appendWaterPoint(int px, int pz, float *p) {
    appendSpecialPoint(px, pz, p);
}

bool RulerObj::isVegetationRuler() const {
    return vegetationRuler;
}

void RulerObj::setVegetationRuler(bool enabled) {
    vegetationRuler = enabled;
    if(enabled){
        waterRuler = false;
        gradeRuler = false;
    }
    setModified();
    if(vegetationBounds3d != NULL)
        vegetationBounds3d->deleteVBO();
    if(line3d != NULL)
        line3d->deleteVBO();
}

void RulerObj::appendVegetationPoint(int px, int pz, float *p) {
    appendSpecialPoint(px, pz, p);
}

bool RulerObj::isGradeRuler() const {
    return gradeRuler;
}

void RulerObj::setGradeRuler(bool enabled) {
    gradeRuler = enabled;
    if(enabled){
        waterRuler = false;
        vegetationRuler = false;
    }
    setModified();
    if(line3d != NULL)
        line3d->deleteVBO();
}

void RulerObj::appendGradePoint(int px, int pz, float *p) {
    appendSpecialPoint(px, pz, p);
}

bool RulerObj::isSpecialRuler() const {
    return waterRuler || vegetationRuler || gradeRuler;
}

void RulerObj::appendSpecialPoint(int px, int pz, float *p) {
    if(p == NULL)
        return;
    if(gradeRuler && points.size() >= 2)
        return;
    Point point;
    point.position[0] = -2048*(this->x-px) + p[0];
    point.position[1] = p[1];
    point.position[2] = -2048*(this->y-pz) + p[2];
    if(points.size() > 0 && Vec3::dist(points.back().position, point.position) <= 1)
        return;
    points.push_back(point);
    selectionValue = points.size() - 1;
    setModified();
    if(line3d != NULL)
        line3d->deleteVBO();
    if(vegetationBounds3d != NULL)
        vegetationBounds3d->deleteVBO();
}

void RulerObj::selectSpecialPoint(
        int encodedIndex, int px, int pz, const float *p) {
    if(!isSpecialRuler() || points.isEmpty())
        return;
    QVector<int> candidates;
    for(int i = 0; i < points.size(); i++){
        if((i & 0xF) == (encodedIndex & 0xF))
            candidates.push_back(i);
    }
    if(candidates.isEmpty())
        return;
    if(candidates.size() == 1 || p == NULL){
        selectionValue = candidates.first();
        selected = true;
        return;
    }
    float localX = -2048*(this->x-px) + p[0];
    float localZ = -2048*(this->y-pz) + p[2];
    int nearestIndex = candidates.first();
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for(int i : candidates){
        float dx = points[i].position[0] - localX;
        float dz = points[i].position[2] - localZ;
        float distanceSquared = dx*dx + dz*dz;
        if(distanceSquared < nearestDistanceSquared){
            nearestDistanceSquared = distanceSquared;
            nearestIndex = i;
        }
    }
    selectionValue = nearestIndex;
    selected = true;
}

void RulerObj::snapSelectedSpecialPointToTerrain() {
    if(!isSpecialRuler() || !specialPointMoved
            || selectionValue < 0 || selectionValue >= points.size())
        return;

    specialPointMoved = false;

    // A point may have moved even when the terrain tile cannot be sampled at
    // release. Always discard the pre-drag corridor; terrain lookup below is
    // only responsible for correcting the point height.
    if(vegetationBounds3d != NULL)
        vegetationBounds3d->deleteVBO();

    int tileX = x;
    int tileZ = y;
    float localX = points[selectionValue].position[0];
    float localZ = points[selectionValue].position[2];
    Game::check_coords(tileX, tileZ, localX, localZ);
    Terrain *terrain = Game::terrainLib->getTerrainByXY(tileX, tileZ, true);
    if(terrain == NULL || !terrain->loaded)
        return;
    float height = Game::terrainLib->getHeight(tileX, tileZ, localX, localZ, false);
    if(height <= -10000.0f)
        return;
    points[selectionValue].position[1] = height;
    setModified();
    if(line3d != NULL)
        line3d->deleteVBO();
}

void RulerObj::createRoadPaths(){
    float tlength = 0;
    DynTrackObj* dobj = new DynTrackObj();
    dobj->load(x, y);
    float p[3];
    float q[4];
    for(int i = 0; i < points.size() - 1; i++){
        tlength = Vec3::distance(points[i].position, points[i+1].position);
        Vec3::copy(p, points[i].position);
        
        int someval = (((points[i+1].position[2]-points[i].position[2])+0.00001f)/fabs((points[i+1].position[2]-points[i].position[2])+0.00001f));
        float rotY = ((float)someval+1.0)*(M_PI/2)+(float)(atan((points[i].position[0]-points[i+1].position[0])/(points[i].position[2]-points[i+1].position[2]))); 
        float rotX = -(float)(asin((points[i].position[1]-points[i+1].position[1])/(tlength))); 

        Quat::fill(q);
        Quat::rotateY(q, q, rotY);
        Quat::rotateX(q, q, rotX);

        dobj->sections[0].a = floor((tlength * 10 ) + 0.5) / 10;
        Game::roadDB->fillDynTrack(dobj);
        Game::roadDB->placeTrack(x, y, (float*) &p, (float*) &q, dobj->sectionIdx, UiD);
        
    }
}

void RulerObj::removeRoadPaths(){
    bool ok;
    ok = Game::roadDB->removeTrackFromTDB(x, y, UiD);
    //if(ok)
}

void RulerObj::enableShape(){
    if(points.size() < 2)
        return;
    
    shapeEnabled = true;
    setModified();

}

void RulerObj::render(GLUU* gluu, float lod, float posx, float posz, float* pos, float* target, float fov, int selectionColor, int renderMode) {
    if (!loaded) return;
    if (jestPQ < 2) return;
    
    if(Game::showWorldObjPivotPoints){
        if(pointer3d == NULL){
            pointer3d = new TrackItemObj(1);
            pointer3d->setMaterial(0.9,0.9,0.7);
        }
        pointer3d->render(selectionColor);
    }
    
    int useSC = (float)selectionColor/(float)(selectionColor+0.000001);
    const bool specialRuler = isSpecialRuler();
    
    if(shapeEnabled){
        if(proceduralShapeInit){
            for(int j = 0; j < points.size() - 1; j++){
                gluu->mvPushMatrix();
                Mat4::multiply(gluu->mvMatrix, gluu->mvMatrix, points[j].matrix);
                gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                for(int i = 0; i < points[j].procShape.size(); i++){
                    points[j].procShape[i]->render(selectionColor | (i&0xF)*useSC);
                }
                gluu->mvPopMatrix();
            }
        } else {
            //templateName = "Siec1";
            qDebug() << templateName;
            for(int i = 0; i < points.size() - 1; i++){
                float tlength = Vec3::distance(points[i].position, points[i+1].position);
                int someval = (((points[i+1].position[2]-points[i].position[2])+0.00001f)/fabs((points[i+1].position[2]-points[i].position[2])+0.00001f));
                float rotY = ((float)someval+1.0)*(M_PI/2)+(float)(atan((points[i].position[0]-points[i+1].position[0])/(points[i].position[2]-points[i+1].position[2]))); 
                float rotX = (float)(asin((points[i].position[1]-points[i+1].position[1])/(tlength))); 

                Quat::fill(points[i].quat);
                Quat::rotateY(points[i].quat, points[i].quat, rotY + M_PI);
                Quat::rotateX(points[i].quat, points[i].quat, rotX);
                Mat4::fromRotationTranslation(points[i].matrix, points[i].quat, points[i].position);
                QVector<TSection> sections;
                sections.push_back(TSection());
                sections.back().size = floor((tlength * 10 ) + 0.5) / 10;
                ProceduralShape::GetShape(templateName, points[i].procShape, sections, i);
            }
            proceduralShapeInit = true;
        }
    }
    
    if (renderMode == GLUU::RENDER_SHADOWMAP) return;
    if(!Game::viewInteractives) 
        return;
    
    if((specialRuler && vegetationHandle3d == NULL)
            || (!specialRuler && point3d == NULL)){
        if(specialRuler){
            const float h = VegetationHandleSize * 0.5f;
            const float faceCorners[6][12] = {
                {-h,-h, h,  h,-h, h,  h, h, h, -h, h, h},
                { h,-h,-h, -h,-h,-h, -h, h,-h,  h, h,-h},
                {-h,-h,-h, -h,-h, h, -h, h, h, -h, h,-h},
                { h,-h, h,  h,-h,-h,  h, h,-h,  h, h, h},
                {-h, h, h,  h, h, h,  h, h,-h, -h, h,-h},
                {-h,-h,-h,  h,-h,-h,  h,-h, h, -h,-h, h}
            };
            QVector<float> handlePositions;
            handlePositions.reserve(36*3);
            for(int faceIndex = 0; faceIndex < 6; faceIndex++){
                const float *c = faceCorners[faceIndex];
                float faceVertices[18] = {
                    c[0],c[1],c[2], c[3],c[4],c[5], c[6],c[7],c[8],
                    c[0],c[1],c[2], c[6],c[7],c[8], c[9],c[10],c[11]
                };
                for(float value : faceVertices)
                    handlePositions.push_back(value);
            }
            vegetationHandle3d = new OglObj();
            vegetationHandle3d->setMaterial(1.0f, 0.35f, 0.0f);
            vegetationHandle3d->initLitTriangles(
                handlePositions.constData(), static_cast<int>(handlePositions.size()));
            QVector<float> pickPositions = handlePositions;
            const float pickScale = SpecialHandlePickSize / VegetationHandleSize;
            for(float &value : pickPositions)
                value *= pickScale;
            specialHandlePick3d = new OglObj();
            specialHandlePick3d->initLitTriangles(
                pickPositions.constData(), static_cast<int>(pickPositions.size()));
            vegetationPost3d = new OglObj();
            if(gradeRuler)
                vegetationPost3d->setMaterial(1.0f,0.0f,1.0f);
            else if(waterRuler)
                vegetationPost3d->setMaterial(0.10f,0.45f,1.0f);
            else
                vegetationPost3d->setMaterial(0.13f,0.55f,0.13f);
            vegetationPost3d->setLineWidth(8);
            float postPoints[6] = { 0, 0, 0, 0, VegetationPostHeight, 0 };
            vegetationPost3d->init(postPoints, 6, RenderItem::V, GL_LINES);
            vegetationPostSelected3d = new OglObj();
            if(gradeRuler)
                vegetationPostSelected3d->setMaterial(1.0f,0.0f,1.0f);
            else if(waterRuler)
                vegetationPostSelected3d->setMaterial(0.10f,0.45f,1.0f);
            else
                vegetationPostSelected3d->setMaterial(0.13f,0.55f,0.13f);
            vegetationPostSelected3d->setLineWidth(8);
            vegetationPostSelected3d->init(postPoints, 6, RenderItem::V, GL_LINES);
        } else {
        point3d = new OglObj();
        if(waterRuler)
            point3d->setMaterial(0.10f,0.45f,1.0f);
        else
            point3d->setMaterial(1,1,1);
        point3d->setLineWidth(8);
        float *punkty = new float[6];
        int ptr = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 10;
        punkty[ptr++] = 0;
        point3d->init(punkty, ptr, RenderItem::V, GL_LINES);
        
        point3dSelected = new OglObj();
        point3dSelected->setLineWidth(8);
        if(waterRuler)
            point3dSelected->setMaterial(0.35f,0.80f,1.0f);
        else
            point3dSelected->setMaterial(0.5,0.5,0.5);
        ptr = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 0;
        punkty[ptr++] = 10;
        punkty[ptr++] = 0;
        point3dSelected->init(punkty, ptr, RenderItem::V, GL_LINES);
        delete[] punkty;
        }
    }
    if(line3d == NULL){
        line3d = new OglObj();
        line3d->setLineWidth(2);
        if(gradeRuler)
            line3d->setMaterial(1.0f,0.0f,1.0f);
        else if(vegetationRuler)
            line3d->setMaterial(0.13f,0.55f,0.13f);
        else if(waterRuler)
            line3d->setMaterial(0.10f,0.45f,1.0f);
        else
            line3d->setMaterial(1,1,1);
    }
    if(!line3d->loaded){
        float *punkty = new float[points.size()*6*2]; 
        int ptr = 0;
        
        for(int i = 0; i < points.size() - 1; i++){
            punkty[ptr++] = points[i].position[0];
            punkty[ptr++] = points[i].position[1]+1;
            punkty[ptr++] = points[i].position[2];
            punkty[ptr++] = points[i+1].position[0];
            punkty[ptr++] = points[i+1].position[1]+1;
            punkty[ptr++] = points[i+1].position[2];
        }
        line3d->init(punkty, ptr, RenderItem::V, GL_LINES);
        delete[] punkty;
        refreshLength();
    }

    if(vegetationRuler && vegetationBounds3d == NULL){
        vegetationBounds3d = new OglObj();
        vegetationBounds3d->setMaterial(1.0f,0.0f,1.0f);
        vegetationBounds3d->setLineWidth(3);
    }
    if(vegetationRuler && vegetationBounds3d != NULL
            && !vegetationBounds3d->loaded && points.size() >= 2){
        struct BoundaryPoint {
            float x;
            float z;
        };

        const int pointCount = points.size();
        QVector<BoundaryPoint> directions(pointCount - 1);
        QVector<BoundaryPoint> normals(pointCount - 1);
        for(int i = 0; i < pointCount - 1; i++){
            float dx = points[i+1].position[0] - points[i].position[0];
            float dz = points[i+1].position[2] - points[i].position[2];
            float segmentLength = std::sqrt(dx*dx + dz*dz);
            if(segmentLength < 0.001f){
                if(i > 0)
                    directions[i] = directions[i-1];
                else
                    directions[i] = { 1.0f, 0.0f };
            } else {
                directions[i] = { dx/segmentLength, dz/segmentLength };
            }
            normals[i] = { -directions[i].z, directions[i].x };
        }

        QVector<BoundaryPoint> left(pointCount);
        QVector<BoundaryPoint> right(pointCount);
        left[0] = {
            points[0].position[0] + normals[0].x*VegetationHalfWidth,
            points[0].position[2] + normals[0].z*VegetationHalfWidth
        };
        right[0] = {
            points[0].position[0] - normals[0].x*VegetationHalfWidth,
            points[0].position[2] - normals[0].z*VegetationHalfWidth
        };

        for(int i = 1; i < pointCount - 1; i++){
            float miterX = normals[i-1].x + normals[i].x;
            float miterZ = normals[i-1].z + normals[i].z;
            float denominator = miterX*normals[i].x + miterZ*normals[i].z;
            float scale = VegetationHalfWidth;
            if(std::fabs(denominator) > 0.05f)
                scale = VegetationHalfWidth/denominator;

            // Near reversals do not have a useful finite intersection. Keep
            // the guide bounded with a conservative miter limit.
            if(std::fabs(scale) > VegetationHalfWidth*10.0f){
                miterX = normals[i].x;
                miterZ = normals[i].z;
                scale = VegetationHalfWidth;
            }

            left[i] = {
                points[i].position[0] + miterX*scale,
                points[i].position[2] + miterZ*scale
            };
            right[i] = {
                points[i].position[0] - miterX*scale,
                points[i].position[2] - miterZ*scale
            };
        }

        const int last = pointCount - 1;
        left[last] = {
            points[last].position[0] + normals[last-1].x*VegetationHalfWidth,
            points[last].position[2] + normals[last-1].z*VegetationHalfWidth
        };
        right[last] = {
            points[last].position[0] - normals[last-1].x*VegetationHalfWidth,
            points[last].position[2] - normals[last-1].z*VegetationHalfWidth
        };

        QVector<float> boundaryVertices;
        auto terrainHeight = [this](float localX, float localZ, float fallback) -> float {
            int tileX = x;
            int tileZ = y;
            Game::check_coords(tileX, tileZ, localX, localZ);
            Terrain *terrain = Game::terrainLib->getTerrainByXY(tileX, tileZ, true);
            if(terrain == NULL || !terrain->loaded)
                return fallback + 0.35f;
            float height = Game::terrainLib->getHeight(
                tileX, tileZ, localX, localZ, false);
            return height > -10000.0f ? height + 0.35f : fallback + 0.35f;
        };
        auto appendTerrainLine = [&boundaryVertices, &terrainHeight](
                const BoundaryPoint &a, const BoundaryPoint &b,
                float fallbackA, float fallbackB){
            float dx = b.x - a.x;
            float dz = b.z - a.z;
            float distance = std::sqrt(dx*dx + dz*dz);
            int steps = std::max(1, (int)std::ceil(distance/25.0f));
            for(int step = 0; step < steps; step++){
                float t0 = (float)step/(float)steps;
                float t1 = (float)(step + 1)/(float)steps;
                float x0 = a.x + dx*t0;
                float z0 = a.z + dz*t0;
                float x1 = a.x + dx*t1;
                float z1 = a.z + dz*t1;
                float fallback0 = fallbackA + (fallbackB-fallbackA)*t0;
                float fallback1 = fallbackA + (fallbackB-fallbackA)*t1;
                boundaryVertices.push_back(x0);
                boundaryVertices.push_back(terrainHeight(x0, z0, fallback0));
                boundaryVertices.push_back(z0);
                boundaryVertices.push_back(x1);
                boundaryVertices.push_back(terrainHeight(x1, z1, fallback1));
                boundaryVertices.push_back(z1);
            }
        };

        for(int i = 0; i < pointCount - 1; i++){
            appendTerrainLine(left[i], left[i+1],
                              points[i].position[1], points[i+1].position[1]);
            appendTerrainLine(right[i], right[i+1],
                              points[i].position[1], points[i+1].position[1]);
        }
        appendTerrainLine(left[0], right[0],
                          points[0].position[1], points[0].position[1]);
        appendTerrainLine(left[last], right[last],
                          points[last].position[1], points[last].position[1]);

        if(!boundaryVertices.isEmpty())
            vegetationBounds3d->init(boundaryVertices.data(), boundaryVertices.size(),
                                     RenderItem::V, GL_LINES);
    }
    
    gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
    line3d->render(selectionColor);
    if(vegetationRuler && vegetationBounds3d != NULL)
        vegetationBounds3d->render(selectionColor);

    for(int i = 0; i < points.size(); i++){
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, points[i].position[0], points[i].position[1], points[i].position[2]);
        gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        if(i == 0 || i == points.size() - 1 || DrawPoints || specialRuler){
            if(specialRuler && vegetationPost3d != NULL){
                if(this->selected && this->selectionValue == i)
                    vegetationPostSelected3d->render(selectionColor | (i&0xF)*useSC);
                else
                    vegetationPost3d->render(selectionColor | (i&0xF)*useSC);
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0,
                                VegetationPostHeight + VegetationHandleGap
                                + VegetationHandleSize*0.5f, 0);
                gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                if(selectionColor == 0){
                    if(this->selected && this->selectionValue == i)
                        vegetationHandle3d->setMaterial(1.0f, 0.62f, 0.08f);
                    else
                        vegetationHandle3d->setMaterial(1.0f, 0.35f, 0.0f);
                }
                OglObj *handle = selectionColor == 0
                    ? vegetationHandle3d : specialHandlePick3d;
                handle->render(selectionColor | (i&0xF)*useSC);
            } else if(this->selected && this->selectionValue == i) {
                point3dSelected->render(selectionColor | (i&0xF)*useSC);
            } else {
                point3d->render(selectionColor | (i&0xF)*useSC);
            }
        //    pointer3d->render(selectionColor + (i+1)*131072*8*useSC);
        }
        gluu->mvPopMatrix();
    }
};

bool RulerObj::hasLinePoints(){
    return true;
}

void RulerObj::getLinePoints(float *&punkty){
    
    float pos[3];
    for(float i = 0; i < this->length; i += 1){
        getPosition(i, pos);
        *punkty++ = pos[0];
        *punkty++ = pos[1];
        *punkty++ = pos[2];
    }
    //for(int i = 0; i < points.size(); i++){
    //    *punkty++ = points[i].position[0];
    //    *punkty++ = points[i].position[1];
    //    *punkty++ = points[i].position[2];
    //}
    return;
}

void RulerObj::getPosition(float len, float* pos){
    float slen = 0;
    float tlen;
    for(int i = 0; i < points.size()-1; i++){
        tlen = Vec3::dist(points[i].position, points[i+1].position);
        slen += tlen;
        if(slen > len){
            float len1 = len - slen + tlen;
            float len2 = slen - len;
            len1 = len1/tlen;
            len2 = len2/tlen;
            pos[0] = len1*points[i].position[0] + len2*points[i+1].position[0];
            pos[1] = len1*points[i].position[1] + len2*points[i+1].position[1];
            pos[2] = len1*points[i].position[2] + len2*points[i+1].position[2];
            return;
        }
    }
    pos[0] = points[points.size()-1].position[0];
    pos[1] = points[points.size()-1].position[1];
    pos[2] = points[points.size()-1].position[2];
    
}

bool RulerObj::select(int value){
    this->selectionValue = value;
    this->selected = true;
    return true;
}


void RulerObj::save(QTextStream* out){
    if (!loaded) return;
    if (jestPQ < 2) return;
    
*(out) << "	Ruler (\n";

*(out) << "		UiD ( "<<this->UiD<<" )\n";
*(out) << "		Position ( "<<this->position[0]<<" "<<this->position[1]<<" "<<-this->position[2]<<" )\n";
*(out) << "		QDirection ( "<<this->qDirection[0]<<" "<<this->qDirection[1]<<" "<<-this->qDirection[2]<<" "<<this->qDirection[3]<<" )\n";
*(out) << "		Points ( " << points.size()<<" \n";
for(int i = 0; i < points.size(); i++)
*(out) << "			Point ( "<<points[i].position[0]<<" "<<points[i].position[1]<<" "<<-points[i].position[2]<<" )\n";
*(out) << "		)\n";
if(shapeEnabled){
*(out) << "		ShapeTemplate ( "<<ParserX::AddComIfReq(templateName)<<" )\n";
}
if(waterRuler){
*(out) << "		WaterRuler ( 1 )\n";
}
if(vegetationRuler){
*(out) << "		VegetationRuler ( 1 )\n";
}
if(gradeRuler){
*(out) << "		GradeRuler ( 1 )\n";
}
*(out) << "	)\n";
}

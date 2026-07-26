/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "DynTrackObj.h"
#include "ParserX.h"
#include <QDebug>
#include "GLMatrix.h"
#include "TexLib.h"
#include "Vector2f.h"
#include "Vector3f.h"
#include <QOpenGLShaderProgram>
#include "GLUU.h"
#include "TS.h"
#include "TrackItemObj.h"
#include "Game.h"
#include "ProceduralMstsDyntrack.h"
#include "ProceduralShape.h"
#include "TSection.h"
#include "TDB.h"
#include "TSectionDAT.h"
#include "ComplexLine.h"
#include "OglObj.h"
#include <QCoreApplication>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DynTrackObj::DynTrackObj() {
    sectionIdx = -1;
    sections = NULL;
    collideFlags = 39;    
    staticFlags = 0x100000;
}

DynTrackObj::DynTrackObj(const DynTrackObj& o) : WorldObj(o) {
    if(o.sections != NULL){
        sections = new Section[5];
        for (int i = 0; i < 5; i++) {
            sections[i].type = o.sections[i].type;
            sections[i].sectIdx = o.sections[i].sectIdx;
            sections[i].a = o.sections[i].a;
            sections[i].r = o.sections[i].r;
        }
    }
    tex1 = o.tex1;
    tex2 = o.tex2;
    init = false;// o.init;
    elevation = o.elevation;
    if(o.jNodePosn!=NULL){
        jNodePosn = new float[5];
        memcpy(jNodePosn, o.jNodePosn, sizeof(float)*5);
    }
    sidxSelected = o.sidxSelected;
}

WorldObj* DynTrackObj::clone(){
    return new DynTrackObj(*this);
}

DynTrackObj::~DynTrackObj() {
    delete gradeMarker;
    delete gradeMarkerMirror;
}

bool DynTrackObj::allowNew(){
    return true;
}

float DynTrackObj::getElevation(){
    float vect[3];
    vect[0] = 0; vect[1] = 0; vect [2] = 1000;
    Vec3::transformQuat(vect, vect, qDirection);
    return this->endp[3]*asin(-vect[1]/1000.0);
}

void DynTrackObj::setElevation(float prom){
    // Keep the serialized Dyntrack elevation in sync with the rotation applied below.
    elevation = prom;
    float * q = qDirection;
    float vect[3];
    vect[0] = 0; vect[1] = 0; vect [2] = 1000;
    Vec3::transformQuat(vect, vect, q);
    rotate(asin((vect[1]*this->endp[3]+prom)/1000.0),0,0);
}

void DynTrackObj::rotate(float x, float y, float z){
    this->tRotation[0] += x;
    this->tRotation[1] += y;
    if(x != 0)
        ++Game::gradeOverlayRevision;
    if(matrix3x3 != NULL) matrix3x3 = NULL;

    if(Game::debugOutput) qDebug() << "rot" << x << y << z;
    float vect2[3];
    float vect[3];
    float quat[4];
    Quat::fill(quat);
    
    if(x!=0) Quat::rotateX(this->qDirection, this->qDirection, x*this->endp[3]);
    if(y!=0) Quat::rotateY(this->qDirection, this->qDirection, y*this->endp[3]);
    if(z!=0) Quat::rotateZ(this->qDirection, this->qDirection, z*this->endp[3]);    
    
    Vec3::set(vect, 0, 0, 10);
    Vec3::transformQuat(vect, vect, this->qDirection);
    
    Quat::fill(quat);
    Quat::rotateY(quat, quat, endp[4]);

    Vec3::transformQuat(reinterpret_cast<float*>(&vect), this->endp, this->qDirection);
    Vec3::transformQuat(reinterpret_cast<float*>(&vect2), this->endp, quat);

    if(Game::debugOutput) qDebug() << this->endp[0] << " "<< vect[0] << " " << vect2[0];
    if(Game::debugOutput) qDebug() << this->endp[2] << " "<< vect[2] << " " << vect2[2];
    
    vect[0] = (vect2[0] - vect[0]);
    vect[2] = (vect2[2] - vect[2]);
    
    this->position[0] = this->placedAtPosition[0] + vect[0];
    this->position[1] = this->placedAtPosition[1] - vect[1];
    this->position[2] = this->placedAtPosition[2] + vect[2];
    
    setModified();
    setMartix();
}

void DynTrackObj::deleteVBO(){
    //this->shape.deleteVBO();
    this->init = false;
    for(int i = 0; i < shape.size(); i++){
        shape[i]->deleteVBO();
        delete shape[i];
    }
    shape.clear();
    if(gradeMarker != NULL)
        gradeMarker->deleteVBO();
    if(gradeMarkerMirror != NULL)
        gradeMarkerMirror->deleteVBO();
    gradeMarkerDirection = 99;
    gradeMarkerTransition = -1;
}

void DynTrackObj::removedFromTDB(){
    this->sectionIdx = -1;
}

int DynTrackObj::updateTrackSectionInfo(QHash<unsigned int,unsigned int> shapes, QHash<unsigned int,unsigned int> sect){
    if(shapes[sectionIdx] > 0)
        sectionIdx = shapes[sectionIdx];
    if(sections == NULL)
        return 0;
    int count = 0;
    for(int i = 0; i < 5; i++){
        if(sections[i].sectIdx == 4294967295)
            continue;
        if(sect[sections[i].sectIdx] > 0){
            sections[i].sectIdx = sect[sections[i].sectIdx];
            count++;
        }
    }
    return count;
}

void DynTrackObj::load(int x, int y) {
    this->tex1 = -1;
    this->init = false;
    //this->shape = ShapeLib::addShape(path, fileName);
    this->x = x;
    this->y = y;
    this->position[2] = -this->position[2];
    this->qDirection[2] = -this->qDirection[2];
    Mat4::fromRotationTranslation(this->matrix, qDirection, position);
    Mat4::rotate(this->matrix, this->matrix, M_PI, 0, -1, 0);
    this->loaded = true;
    this->size = -1;
    this->skipLevel = 3;
    this->modified = false;
    this->endp = new float[5];
    this->endp[0] = 0;
    this->endp[1] = 0;
    this->endp[2] = 0;
    this->endp[3] = 1;
    this->endp[4] = 0;
    this->placedAtPosition[0] = this->position[0];
    this->placedAtPosition[1] = this->position[1];
    this->placedAtPosition[2] = this->position[2];
    
    if(sections == NULL){
        sections = new Section[5];
        sections[0].type = 0;
        sections[0].sectIdx = 0;
        sections[0].a = 10;
        sections[0].r = 0;
        for (int iii = 1; iii < 5; iii++) {
            sections[iii].type = iii%2;
            sections[iii].sectIdx = 4294967295;
            sections[iii].a = 0;
            sections[iii].r = 0;
        }
    }
    //sections[1].type = 1;
    //sections[1].sectIdx = 0;
    //sections[1].a = 0.1;
    //sections[1].r = 100;
}



void DynTrackObj::resize(float x, float y, float z){
    if(z < 0)
        this->sidxSelected--;
    if(z > 0)
        this->sidxSelected++;
    if(this->sidxSelected < 0 )
        this->sidxSelected = 0;
    if(this->sidxSelected > 4 )
        this->sidxSelected = 4;

    if(this->sections[this->sidxSelected].sectIdx > 1000000 && x == 0) return;
    
    if(this->sections[this->sidxSelected].sectIdx > 1000000){
        this->sections[this->sidxSelected].sectIdx = 0;
        this->sections[this->sidxSelected].a = 0.1;
        if(this->sections[this->sidxSelected].type == 1) 
            this->sections[this->sidxSelected].r = 100;
    }
    
    

    if(this->sections[this->sidxSelected].type == 1){
        if(x < 0)
            this->sections[this->sidxSelected].a -= 0.01;
        if(x > 0)
            this->sections[this->sidxSelected].a += 0.01;
        if(y < 0)
            this->sections[this->sidxSelected].r -= 5;
        if(y > 0)
            this->sections[this->sidxSelected].r += 5;
        if(this->sections[this->sidxSelected].a > 3.14) this->sections[this->sidxSelected].a = 3.14;
        if(this->sections[this->sidxSelected].a < -3.14) this->sections[this->sidxSelected].a = -3.14;
        if(this->sections[this->sidxSelected].r < 15) this->sections[this->sidxSelected].r = 15;
        
        if(x < 0 && this->sections[this->sidxSelected].a < 0.01 && this->sections[this->sidxSelected].a > -0.01) 
            this->sections[this->sidxSelected].a = -0.01;
        if(x > 0 && this->sections[this->sidxSelected].a > -0.01 && this->sections[this->sidxSelected].a < 0.01) 
            this->sections[this->sidxSelected].a = 0.01;
    } else {
        if(x < 0)
            this->sections[this->sidxSelected].a -= 0.1;
        if(x > 0)
            this->sections[this->sidxSelected].a += 0.1;
        if(this->sections[this->sidxSelected].a < 0) this->sections[this->sidxSelected].a = 0;
    }

    setModified();
    deleteVBO();
}

void DynTrackObj::set(int sh, FileBuffer* data) {
    //qDebug() << "dyntrack: "<<sh;
    if (sh == TS::SectionIdx) {
        data->off++;
        sectionIdx = data->getUint();
        return;
    }
    if (sh == TS::Elevation) {
        data->off++;
        elevation = data->getFloat();
        return;
    }
    if (sh == TS::JNodePosn) {
        data->off++;
        jNodePosn = new float[5];
        jNodePosn[0] = data->getFloat();
        jNodePosn[1] = data->getFloat();
        jNodePosn[2] = data->getFloat();
        jNodePosn[3] = data->getFloat();
        jNodePosn[4] = data->getFloat();
        return;
    }
    if (sh == TS::TrackSections) {
        if(sections == NULL) sections = new Section[5];
        data->off++;
        
        for (int iii = 0; iii < 5; iii++) {
            data->off+=18;
            sections[iii].type = data->getUint();
            sections[iii].sectIdx = data->getUint();
            sections[iii].a = data->getFloat();
            sections[iii].r = data->getFloat();
        }
        return;
    }
    //qDebug() <<"A";
    WorldObj::set(sh, data);
    return;
}

void DynTrackObj::set(QString sh, FileBuffer* data) {
    if (sh == ("sectionidx")) {
        //qDebug() << ParserX::GetNumber(data);
        sectionIdx = ParserX::GetNumber(data);
        return;
    }
    if (sh == ("elevation")) {
        elevation = ParserX::GetNumber(data);
        return;
    }
    if (sh == ("tracksections")) {
        if(sections == NULL) sections = new Section[5];
        for (int iii = 0; iii < 5; iii++) {
            sections[iii].type = ParserX::GetNumber(data);
            sections[iii].sectIdx = ParserX::GetUInt(data);
            sections[iii].a = ParserX::GetNumber(data);
            sections[iii].r = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
        }
        return;
    }
    if (sh == ("jnodeposn")) {
        jNodePosn = new float[5];
        jNodePosn[0] = ParserX::GetNumber(data);
        jNodePosn[1] = ParserX::GetNumber(data);
        jNodePosn[2] = ParserX::GetNumber(data);
        jNodePosn[3] = ParserX::GetNumber(data);
        jNodePosn[4] = ParserX::GetNumber(data);
        return;
    }    
    //qDebug() <<"A";
    WorldObj::set(sh, data);
    return;
}

void DynTrackObj::set(QString sh, float* val) {
    if(sh == "dyntrackdata"){
        for (int iii = 0; iii < 5; iii++) {
            sections[iii].a = val[iii*2];
            sections[iii].r = val[iii*2+1];
            if(iii%2 == 0){
                sections[iii].sectIdx = 0;
                if(sections[iii].a == 0 && iii > 0)
                    sections[iii].sectIdx = 4294967295;
            } else {
                sections[iii].sectIdx = 0;
                if(sections[iii].a < 0.01 && sections[iii].a > -0.01)
                    sections[iii].sectIdx = 4294967295;
                if(sections[iii].r < 0.1)
                    sections[iii].sectIdx = 4294967295;
            }
        }
    }
    //sections[0].sectIdx = 4294967295;
    //sections[0].a = 0;
    //sections[0].r = 0;
    setModified();
    deleteVBO();
}

void DynTrackObj::render(GLUU* gluu, float lod, float posx, float posz, float* pos, float* target, float fov, int selectionColor, int renderMode) {
    if (!loaded) 
        return;

    Mat4::multiply(gluu->mvMatrix, gluu->mvMatrix, matrix);

    gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
    
    if(Game::showWorldObjPivotPoints){
        if(pointer3d == NULL){
            pointer3d = new TrackItemObj(1);
            pointer3d->setMaterial(0.9,0.9,0.7);
        }
        pointer3d->render(selectionColor);
    }

    if (!init) {
        QVector<TSection> tsections;
        for(int i = 0; i < 5; i++){
            if(sections[i].sectIdx > 100000000)
                continue;
            tsections.push_back(TSection(0, sections[i].type, sections[i].a, sections[i].r));
        }
        if (Game::proceduralTracks) {
            TrackShape *tsh = Game::trackDB->tsection->shape[sectionIdx];
            QMap<int, float> angles;
            if(Game::useSuperelevation){
                Game::trackDB->fillTrackAngles(x, -y, UiD, angles);
                bool positiveAngles = false;
                for(int i = 0; i < tsections.size(); i++){
                    if(tsections[i].angle > 0)
                        positiveAngles = true;
                }
                if(positiveAngles){
                    QList<int> keys = angles.keys();
                    for(int j = 0; j < keys.size(); j++){
                        angles[keys[j]] = -angles[keys[j]];
                    }
                }
            }
            ProceduralShape::GetShape(templateName, shape, tsh, angles);
        } else {
            ProceduralMstsDyntrack::GenShape(shape, tsections);
        }
        init = true;
    } else {
        for(int i = 0; i < shape.size(); i++){
            shape[i]->render(selectionColor);
        }
    }
    
    if(selected){
        drawBox();
    }

    renderGradeMarker(lod, renderMode, selectionColor);
};

void DynTrackObj::rebuildGradeMarker(int direction, int transition){
    if(gradeMarker == NULL)
        gradeMarker = new OglObj();
    else
        gradeMarker->deleteVBO();
    if(gradeMarkerMirror == NULL)
        gradeMarkerMirror = new OglObj();
    else
        gradeMarkerMirror->deleteVBO();

    float border[6];
    float centerX = 0.0f;
    float centerY = 0.0f;
    float centerZ = 0.0f;
    if(getSimpleBorder(border))
        centerZ = (border[4] + border[5]) * 0.5f;

    float forwardX = 0.0f;
    float forwardZ = (float)direction;
    TDB *tdb = Game::trackDB;
    TrackShape *trackShape = NULL;
    if(tdb != NULL && tdb->tsection != NULL && sectionIdx >= 0)
        trackShape = tdb->tsection->shape[sectionIdx];
    if(trackShape != NULL && trackShape->numpaths > 0){
        int firstPath = 0;
        int pathCount = trackShape->numpaths;
        if(trackShape->mainroute >= 0 && trackShape->mainroute < trackShape->numpaths){
            firstPath = trackShape->mainroute;
            pathCount = firstPath + 1;
        }

        float centerSum[3] = { 0.0f, 0.0f, 0.0f };
        float tangentSum[3] = { 0.0f, 0.0f, 0.0f };
        int sampledPaths = 0;
        for(int pathId = firstPath; pathId < pathCount; ++pathId){
            TrackShape::SectionIdx &path = trackShape->path[pathId];
            QVector<TSection> markerSections;
            for(int sectionId = 0; sectionId < path.n; ++sectionId){
                TSection *section = tdb->tsection->sekcja[path.sect[sectionId]];
                if(section != NULL)
                    markerSections.push_back(*section);
            }
            if(markerSections.isEmpty())
                continue;

            ComplexLine line;
            line.init(markerSections);
            if(line.length <= 0.001f)
                continue;

            const float sampleStep = qMin(0.5f, line.length * 0.1f);
            float center[6];
            float before[6];
            float after[6];
            line.getDrawPosition(center, line.length * 0.5f);
            line.getDrawPosition(before, qMax(0.0f, line.length * 0.5f - sampleStep));
            line.getDrawPosition(after, qMin(line.length, line.length * 0.5f + sampleStep));

            float pathRotation[4];
            float pathMatrix[16];
            float pathPosition[3] = { -path.pos[0], path.pos[1], path.pos[2] };
            Quat::fill(pathRotation);
            Quat::rotateY(pathRotation, pathRotation, -path.rotDeg * M_PI / 180.0f);
            Mat4::fromRotationTranslation(pathMatrix, pathRotation, pathPosition);
            Vec3::transformMat4(center, center, pathMatrix);
            Vec3::transformMat4(before, before, pathMatrix);
            Vec3::transformMat4(after, after, pathMatrix);

            centerSum[0] += center[0];
            centerSum[1] += center[1];
            centerSum[2] += center[2];
            tangentSum[0] += after[0] - before[0];
            tangentSum[1] += after[1] - before[1];
            tangentSum[2] += after[2] - before[2];
            sampledPaths++;
        }

        if(sampledPaths > 0){
            centerX = centerSum[0] / sampledPaths;
            centerY = centerSum[1] / sampledPaths;
            centerZ = centerSum[2] / sampledPaths;
            const float tangentLength = std::sqrt(tangentSum[0] * tangentSum[0]
                                                 + tangentSum[2] * tangentSum[2]);
            if(tangentLength > 0.0001f){
                forwardX = tangentSum[0] / tangentLength * direction;
                forwardZ = tangentSum[2] / tangentLength * direction;
            }
        }
    }

    const float markerY = centerY + 0.58f;
    QVector<float> points;
    QVector<float> mirrorPoints;
    const float halfSize = 2.3f;
    const float rightX = forwardZ;
    const float rightZ = -forwardX;
    const float headLeftX = centerX - rightX * halfSize + forwardX * halfSize;
    const float headLeftZ = centerZ - rightZ * halfSize + forwardZ * halfSize;
    const float tailLeftX = centerX - rightX * halfSize - forwardX * halfSize;
    const float tailLeftZ = centerZ - rightZ * halfSize - forwardZ * halfSize;
    const float tailRightX = centerX + rightX * halfSize - forwardX * halfSize;
    const float tailRightZ = centerZ + rightZ * halfSize - forwardZ * halfSize;
    const float headRightX = centerX + rightX * halfSize + forwardX * halfSize;
    const float headRightZ = centerZ + rightZ * halfSize + forwardZ * halfSize;
    const float alpha = -GLUU::get()->alphaTest;
    auto addVertex = [&points, alpha](float px, float py, float pz, float u, float v){
        points << px << py << pz << u << v << alpha;
    };
    auto addMirrorVertex = [&mirrorPoints, alpha](float px, float py, float pz, float u, float v){
        mirrorPoints << px << py << pz << u << v << alpha;
    };

    addVertex(headLeftX, markerY, headLeftZ, 0.0f, 0.0f);
    addVertex(tailLeftX, markerY, tailLeftZ, 0.0f, 1.0f);
    addVertex(tailRightX, markerY, tailRightZ, 1.0f, 1.0f);
    addVertex(headLeftX, markerY, headLeftZ, 0.0f, 0.0f);
    addVertex(tailRightX, markerY, tailRightZ, 1.0f, 1.0f);
    addVertex(headRightX, markerY, headRightZ, 1.0f, 0.0f);

    addMirrorVertex(tailRightX, markerY, tailRightZ, 1.0f, 1.0f);
    addMirrorVertex(tailLeftX, markerY, tailLeftZ, 0.0f, 1.0f);
    addMirrorVertex(headLeftX, markerY, headLeftZ, 0.0f, 0.0f);
    addMirrorVertex(headRightX, markerY, headRightZ, 1.0f, 0.0f);
    addMirrorVertex(tailRightX, markerY, tailRightZ, 1.0f, 1.0f);
    addMirrorVertex(headLeftX, markerY, headLeftZ, 0.0f, 0.0f);

    static QString stableTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-stable.png";
    static QString transitionTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-increase.png";
    static QString warningTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-warning.png";
    static QString stableMirrorTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-stable-mirror.png";
    static QString transitionMirrorTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-increase-mirror.png";
    static QString warningMirrorTexture = QCoreApplication::applicationDirPath() + "/tsre_appdata/" + Game::AppDataVersion + "/grade-warning-mirror.png";
    QString *texture = &stableTexture;
    QString *mirrorTexture = &stableMirrorTexture;
    if(transition == TDB::GradeMarkerTransitory){
        texture = &transitionTexture;
        mirrorTexture = &transitionMirrorTexture;
    } else if(transition == TDB::GradeMarkerWarning){
        texture = &warningTexture;
        mirrorTexture = &warningMirrorTexture;
    }
    gradeMarker->setMaterial(texture);
    gradeMarker->init(points.data(), points.size(), RenderItem::VT, GL_TRIANGLES);
    gradeMarkerMirror->setMaterial(mirrorTexture);
    gradeMarkerMirror->init(mirrorPoints.data(), mirrorPoints.size(), RenderItem::VT, GL_TRIANGLES);
    gradeMarkerDirection = direction;
    gradeMarkerTransition = transition;
}

void DynTrackObj::renderGradeMarker(float lod, int renderMode, int selectionColor){
    if(!Game::gradeOverlayEnabled || renderMode != GLUU::RENDER_DEFAULT || selectionColor != 0)
        return;
    if(lod > 650.0f)
        return;

    const float gradePercent = std::tan(getElevation()) * 100.0f;
    if(qAbs(gradePercent) <= 0.01f)
        return;
    int direction = gradePercent > 0.0f ? 1 : -1;
    if(endp != NULL && endp[3] < 0.0f)
        direction = -direction;
    int transition = TDB::GradeMarkerStable;
    if(Game::trackDB != NULL)
        transition = Game::trackDB->getGradeMarkerTransition(x, y, UiD);
    if(gradeMarker == NULL || gradeMarkerDirection != direction || gradeMarkerTransition != transition)
        rebuildGradeMarker(direction, transition);
    if(gradeMarker != NULL)
        gradeMarker->render(0, lod);
    if(gradeMarkerMirror != NULL)
        gradeMarkerMirror->render(0, lod);
}

bool DynTrackObj::getSimpleBorder(float* border){
    if(shape.size() > 0){
        if(shape[0] != NULL)
            return shape[0]->getSimpleBorder(border);
    }
    return false;
}

bool DynTrackObj::getBoxPoints(QVector<float>& points){
        float bound[6];
        if (!getSimpleBorder((float*)&bound)) return false;
        
        for(int i=0; i<2; i++)
            for(int j=4; j<6; j++){
                points.push_back(bound[i]);
                points.push_back(bound[2]);
                points.push_back(bound[j]);
                points.push_back(bound[i]);
                points.push_back(bound[3]);
                points.push_back(bound[j]);
            }
        for(int i=0; i<2; i++)
            for(int j=2; j<4; j++){
                points.push_back(bound[i]);
                points.push_back(bound[j]);
                points.push_back(bound[4]);
                points.push_back(bound[i]);
                points.push_back(bound[j]);
                points.push_back(bound[5]);
            }
        for(int i=4; i<6; i++)
            for(int j=2; j<4; j++){
                points.push_back(bound[0]);
                points.push_back(bound[j]);
                points.push_back(bound[i]);
                points.push_back(bound[1]);
                points.push_back(bound[j]);
                points.push_back(bound[i]);
            }
        return true;
}

int DynTrackObj::getDefaultDetailLevel(){
    return -12;
}

void DynTrackObj::save(QTextStream* out){
    if (!loaded) return;
    if(Game::useOnlyPositiveQuaternions)
        Quat::makePositive(this->qDirection);
    
*(out) << "	Dyntrack (\n";
*(out) << "		UiD ( "<<this->UiD<<" )\n";
*(out) << "		TrackSections (\n";
for(int i = 0; i < 5; i++){
*(out) << "			TrackSection (\n";
*(out) << "				SectionCurve ( "<<this->sections[i].type<<" ) "<<this->sections[i].sectIdx<<" "<<this->sections[i].a<<" "<<this->sections[i].r<<"\n";
*(out) << "			)\n";
}
*(out) << "		)\n";
*(out) << "		SectionIdx ( "<<this->sectionIdx<<" )\n";
*(out) << "		Elevation ( "<<this->elevation<<" )\n";
if(this->jNodePosn!=NULL)
*(out) << "		JNodePosn ( "<<this->jNodePosn[0]<<" "<<this->jNodePosn[1]<<" "<<this->jNodePosn[2]<<" "<<this->jNodePosn[3]<<" "<<this->jNodePosn[4]<<" )\n";
*(out) << "		CollideFlags ( "<<this->collideFlags<<" )\n";
*(out) << "		StaticFlags ( "<<ParserX::MakeFlagsString(this->staticFlags)<<" )\n";
*(out) << "		Position ( "<<this->position[0]<<" "<<this->position[1]<<" "<<-this->position[2]<<" )\n";
*(out) << "		QDirection ( "<<this->qDirection[0]<<" "<<this->qDirection[1]<<" "<<-this->qDirection[2]<<" "<<this->qDirection[3]<<" )\n";
if(Game::legacySupport)   *(out) << "		VDbId ( " << this->vDbId << " )\n";  // EFO 
if(this->staticDetailLevel > -1)
*(out) << "		StaticDetailLevel ( "<<this->staticDetailLevel<<" )\n";
*(out) << "	)\n";
}

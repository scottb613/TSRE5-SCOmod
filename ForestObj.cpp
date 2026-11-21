/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include <QFile>
#include "RouteFrameProfile.h"
#include "ForestObj.h"
#include "GLMatrix.h"
#include <math.h>
#include "ParserX.h"
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <cstdlib>
#include "TexLib.h"
#include "Vector2f.h"
#include "TerrainLib.h"
#include "Terrain.h"
#include "TS.h"
#include "Game.h"
#include "FileFunctions.h"
#include "ReadFile.h"
#include "TDB.h"
#include "Vector4f.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QVector<ForestObj::ForestList> ForestObj::forestList;
float ForestObj::ForestClearDistance = 0;
int ForestObj::TextureRevision = 0;

ForestObj::ForestObj() {
    this->loaded = false;
}

ForestObj::ForestObj(const ForestObj& o) : WorldObj(o) {
    treeTexture = o.treeTexture;
    scaleRangeX = o.scaleRangeX;
    scaleRangeZ = o.scaleRangeZ;
    areaX = o.areaX;
    areaZ = o.areaZ;
    treeSizeX = o.treeSizeX;
    treeSizeZ = o.treeSizeZ;
    population = o.population;
    tex = o.tex;
    init = false;

}

WorldObj* ForestObj::clone(){
    return new ForestObj(*this);
}

ForestObj::~ForestObj() {
}

void ForestObj::LoadForestList(){
    ForestObj::forestList.clear();
    QString path = Game::root + "/routes/" + Game::route + "/forests.dat";
    path.replace("//", "/");
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    FileBuffer* data = ReadFile::read(&file);
    if (data == nullptr)
        return;
    ParserX::NextLine(data);

    QString sh = "";
    //ParserX::szukajsekcji1(sh, data);
    //ParserX::GetNumber(data);
    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        
        if (sh == ("forest")) {
            ForestObj::forestList.push_back(ForestObj::ForestList());
            ForestObj::forestList.back().name = ParserX::GetString(data);
            ForestObj::forestList.back().texture = ParserX::GetString(data);
            ForestObj::forestList.back().treeSizeX = ParserX::GetNumber(data);
            ForestObj::forestList.back().treeSizeZ = ParserX::GetNumber(data);
            ForestObj::forestList.back().scaleRangeX = ParserX::GetNumber(data);
            ForestObj::forestList.back().scaleRangeZ = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        ParserX::SkipToken(data);
    }
}

int ForestObj::GetListIdByTexture(QString texture){
    for(int i = 0; i < forestList.size(); i++){
        if(forestList[i].texture.toLower() == texture.toLower())
            return i;
    }
    return 0;
}

void ForestObj::load(int x, int y) {
    this->x = x;
    this->y = y;
    this->position[2] = -this->position[2];
    this->loaded = true;
    this->size = -1;
    this->tex = -1;
    this->init = false;
    this->skipLevel = 3;
    this->modified = false;
}

void ForestObj::set(QString sh, QString val){
    if (sh == ("ref_filename")) {
        treeTexture = val;
        return;
    }
    WorldObj::set(sh, val);
    return;
}

void ForestObj::set(QString sh, long long int val) {
    if (sh == ("ref_value")) {
        // if(Game::debugOutput) qDebug() << "FO126: "<< val;
        int val2        = val & 0xFFFF;
        int tareax      = (val >> 16) & 0xFFF;
        int tareaz      = (val >> 28) & 0xFFF;
        int tpopulation = (val >> 40) & 0xFFFFFF;    
        treeTexture = ForestObj::forestList[val2].texture;
        scaleRangeX = ForestObj::forestList[val2].scaleRangeX;
        scaleRangeZ = ForestObj::forestList[val2].scaleRangeZ;
        areaX = 100;
        if(tareax != 0)
            areaX = tareax;
        areaZ = 100;
        if(tareaz != 0)
            areaZ = tareaz;
        treeSizeX = ForestObj::forestList[val2].treeSizeX;
        treeSizeZ = ForestObj::forestList[val2].treeSizeZ;
        population = 10;
        if(tpopulation != 0)
            population = tpopulation;
        return;
    }
    if (sh == ("population")) {
        population = val;
        return;
    }
    WorldObj::set(sh, val);
    setModified();
}

void ForestObj::set(QString sh, float val) {
    if (sh == ("areaX")) {
        areaX = val;
        return;
    }
    if (sh == ("areaZ")) {
        areaZ = val;
        return;
    }
    WorldObj::set(sh, val);
    setModified();
}

void ForestObj::set(int sh, FileBuffer* data) {
    if (sh == TS::TreeTexture) {
        data->off++;
        int slen = data->getShort()*2;
        treeTexture = *data->getString(data->off, data->off + slen);
        data->off += slen;
        return;
    }
    if (sh == TS::ScaleRange) {
        data->off++;
        scaleRangeX = data->getFloat();
        scaleRangeZ = data->getFloat();
        return;
    }
    if (sh == TS::Area) {
        data->off++;
        areaX = data->getFloat();
        areaZ = data->getFloat();
        return;
    }
    if (sh == TS::TreeSize) {
        data->off++;
        treeSizeX = data->getFloat();
        treeSizeZ = data->getFloat();
        return;
    }
    if (sh == TS::Population) {
        data->off++;
        population = data->getUint();
        return;
    }
    WorldObj::set(sh, data);
    return;
}

void ForestObj::set(QString sh, FileBuffer* data) {
    if (sh == ("treetexture")) {
        treeTexture = ParserX::GetString(data);
        return;
    }
    if (sh == ("scalerange")) {
        scaleRangeX = ParserX::GetNumber(data);
        scaleRangeZ = ParserX::GetNumber(data);
        return;
    }
    if (sh == ("area")) {
        areaX = ParserX::GetNumber(data);
        areaZ = ParserX::GetNumber(data);
        return;
    }
    if (sh == ("treesize")) {
        treeSizeX = ParserX::GetNumber(data);
        treeSizeZ = ParserX::GetNumber(data);
        return;
    }
    if (sh == ("population")) {
        population = ParserX::GetNumber(data);
        return;
    }
    WorldObj::set(sh, data);
    return;
}

Ref::RefItem* ForestObj::getRefInfo(){
    Ref::RefItem* r = new Ref::RefItem();
    r->type = type;
    r->filename.push_back(treeTexture);
    
    r->value       = (long long int)this->GetListIdByTexture(treeTexture) & 0xFFFF;
    r->value      |= ((long long int)areaX & 0xFFF) << 16;
    r->value      |= ((long long int)areaZ & 0xFFF) << 28;
    r->value |= ((long long int)population & 0xFFFFFF) << 40;    
    return r;
}

void ForestObj::translate(float px, float py, float pz){
    this->position[0]+=px;
    //this->position[1]+=py;
    this->position[2]+=pz;
    setModified();
    deleteVBO();
}

void ForestObj::rotate(float x, float y, float z){
    if(matrix3x3 != NULL) matrix3x3 = NULL;
    if(x!=0) Quat::rotateX(this->qDirection, this->qDirection, x);
    if(y!=0) Quat::rotateY(this->qDirection, this->qDirection, y);
    if(z!=0) Quat::rotateZ(this->qDirection, this->qDirection, z);
    setModified();
    deleteVBO();
}

void ForestObj::resize(float x, float y, float z){
    if(x == 0 && y == 0) return;
    this->areaX += x;
    this->areaZ += y;
    setModified();
    deleteVBO();
}

void ForestObj::render(GLUU* gluu, float lod, float posx, float posz, float* pos, float* target, float fov, int selectionColor, int renderMode) {
    if (!loaded) return;
    if (!Game::viewForestRegions) return;
    //if (jestPQ < 2) return;
    //GLUU* gluu = GLUU::get();
    //if((this.position===undefined)||this.qDirection===undefined) return;

    /*if (size > 0) {
        if ((lod > size)) {
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
            if ((ccos > 0) && (xxx > size + 150) && (skipLevel == 1)) return;
        }
    } else {
        if (ShapeLib::shape[shape]->loaded)
            size = ShapeLib::shape[shape]->size;
    }
*/
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, position[0], 0, position[2]);
    //float scale = sqrt(qDirection[0] * qDirection[0] + qDirection[1] * qDirection[1] + qDirection[2] * qDirection[2]);
    //float angle = ((acos(qDirection[3])*360) / M_PI);
    //Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, gluu->degToRad(-angle), -qDirection[0] * scale, -qDirection[1] * scale, qDirection[2] * scale);
    //Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, gluu->degToRad(180), 0, -1, 0);

    //if(selected){
    //    selected = !selected;
    //    selectionColor = 155;
    //}
    //gluu.setMatrixUniforms();

    //
    //var z = this.position[0]*mmm[9] + this.position[1]*mmm[7] + this.position[2]*mmm[9];

    gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
   
    drawShape(selectionColor);
}

bool ForestObj::prepareGeometry(bool openingView) {
    if (textureRevision != ForestObj::TextureRevision) {
        init = false;
        shape.deleteVBO();
        textureRevision = ForestObj::TextureRevision;
    }

    if (!init) {
        if(!openingView && !Game::ignoreLoadLimits){
            int forestLoadCost = 2 + population / 200;
            if(forestLoadCost > Game::maxObjLag)
                forestLoadCost = Game::maxObjLag;
            if(forestLoadCost < 2)
                forestLoadCost = 2;
            if (Game::allowObjLag < forestLoadCost) {
                RouteFrameProfile::count(RouteFrameProfile::Counter::ForestBuildDeferred);
                return false;
            }
            Game::allowObjLag -= forestLoadCost;
        }
        
        RouteFrameProfile::Scope build(RouteFrameProfile::Stage::ForestBuild, &treeTexture);
        shape.deleteVBO();

        int seed = (int)(position[0] + position[1] + position[2]);
        std::srand(seed);
        float posT[2];
        posT[0] = x;
        posT[1] = y;
        float tposx, tposz;

        QVector<Vector4f> fpoints;
        float off = ((qDirection[1]+0.00001f)/fabs(qDirection[1]+0.00001f))*(float)-acos(qDirection[3])*2.0;
        for(int uu = 0; uu < population; uu++){

            tposx = ((float)((std::rand()%1000))/1000)*areaX-areaX/2.0;
            tposz = ((float)((std::rand()%1000))/1000)*areaZ-areaZ/2.0;

            Vector2f uuu(tposx,tposz);
            uuu.rotate(off, 0);
            tposx = uuu.x;
            tposz = uuu.y;

            fpoints.push_back(Vector4f(tposx+position[0], 0, tposz+position[2], 9999));
        }

        if(ForestClearDistance > 0){
            float bBox[4];
            bBox[0] = bBox[2] = 9999;
            bBox[1] = bBox[3] = -9999;
            Vector2f v;
            for(int u = -1; u < 2; u+=2)
                for(int y = -1; y < 2; y+=2){
                    v.set(areaX*u/2,areaZ*y/2);
                    v.rotate(off, 0);
                    if(v.x < bBox[0])
                        bBox[0] = v.x;
                    if(v.x > bBox[1])
                        bBox[1] = v.x;
                    if(v.y < bBox[2])
                        bBox[2] = v.y;
                    if(v.y > bBox[3])
                        bBox[3] = v.y;
                }
            bBox[0] += position[0];
            bBox[1] += position[0];
            bBox[2] += position[2];
            bBox[3] += position[2];

            if(Game::trackDB != NULL)
                Game::trackDB->fillNearestSquaredDistanceToTDBXZ(posT, fpoints, bBox);
            if(Game::roadDB != NULL)
                Game::roadDB->fillNearestSquaredDistanceToTDBXZ(posT, fpoints, bBox);
        }

        QVector<float> offsets;
        const float clearanceSquared = ForestClearDistance * ForestClearDistance;
        for (const Vector4f &point : fpoints) {
            if (ForestClearDistance > 0 && point.c < clearanceSquared)
                continue;
            const float height = Game::terrainLib->getHeight(x, y, point.x, point.z);
            offsets << point.x - position[0] << height << point.z - position[2];
        }

        RouteFrameProfile::count(RouteFrameProfile::Counter::ForestTreesBuilt, offsets.size() / 3);
        QVector<float> tree;
        tree.reserve(24 * RenderItem::VNTA);
        const float halfWidth = float(treeSizeX * 0.7) / 2.0f;
        const float alpha = -GLUU::get()->alphaTest;
        const auto addVertex = [&](float x, float y, float z, float u, float v) {
            tree << x << y << z << 0.0f << 1.0f << 0.0f << u << v << alpha;
        };
        // Keep both sides of each crossed quad and the original UV winding.
        for (int direction : {-1, 1}) {
            for (int side : {-1, 1}) {
                const float z = halfWidth * side;
                const float x = z * direction;
                addVertex(-x, treeSizeZ, -z, 0, 0);
                addVertex( x, treeSizeZ,  z, 1, 0);
                addVertex( x, 0,          z, 1, 1);
                addVertex(-x, 0,         -z, 0, 1);
                addVertex(-x, treeSizeZ, -z, 0, 0);
                addVertex( x, 0,          z, 1, 1);
            }
        }

        GLUU *gluu = GLUU::get();
        const auto supportsForest = [gluu](const char *name) {
            const Shader *shader = gluu->shaders.value(QString::fromLatin1(name));
            return shader && shader->forestOffsets >= 0;
        };
        bool instanced = false;
        if (offsets.size() > 3 && GLUU::vegetationInstancing()
                && supportsForest("StandardFog") && supportsForest("StandardBloom")
                && supportsForest("Shadows")) {
            shape.init(tree.data(), int(tree.size()), RenderItem::VNTA, GL_TRIANGLES);
            instanced = shape.setForestOffsets(offsets);
        }
        if (!instanced) {
            QVector<float> vertices;
            vertices.reserve(tree.size() * (offsets.size() / 3));
            for (qsizetype i = 0; i < offsets.size(); i += 3) {
                for (qsizetype j = 0; j < tree.size(); j += RenderItem::VNTA) {
                    vertices << tree[j] + offsets[i] << tree[j + 1] + offsets[i + 1]
                             << tree[j + 2] + offsets[i + 2];
                    for (int component = 3; component < RenderItem::VNTA; ++component)
                        vertices << tree[j + component];
                }
            }
            shape.init(vertices.data(), int(vertices.size()), RenderItem::VNTA, GL_TRIANGLES);
        }

        texturePath = Terrain::resolveTexturePath(resPath.toLower(),
            Terrain::textureSubdirCandidatesForSeason(Game::season), treeTexture.toLower(), true);
        init = true;
    }
    return true;
}

void ForestObj::prepareOpeningView(OpeningViewLoad &opening) {
    if (Game::viewForestRegions && prepareGeometry(true)) {
        shape.setMaterial(texturePath);
        shape.prepareTexture(opening);
    }
}

void ForestObj::drawShape(int selectionColor) {
    if (!prepareGeometry(false))
        return;
    if (selectionColor != 0)
        shape.setMaterial(((selectionColor >> 16) & 255) / 255.0f,
                          ((selectionColor >> 8) & 255) / 255.0f,
                          (selectionColor & 255) / 255.0f);
    else
        shape.setMaterial(texturePath);

    {
        RouteFrameProfile::Scope draw(RouteFrameProfile::Stage::ForestDraw, &treeTexture);
        shape.render();
    }
    if(selected){
        drawBox();
    }
}

bool ForestObj::getBoxPoints(QVector<float>& points){
    if (!loaded) 
        return false;
    float scale = (float) sqrt(qDirection[0] * qDirection[0] + qDirection[1] * qDirection[1] + qDirection[2] * qDirection[2]);
    float off = ((qDirection[1]+0.000001f)/fabs(scale+0.000001f))*(float)-acos(qDirection[3])*2;
        
            Vector2f x1y1(-areaX/2,-areaZ/2, off, 0);
            Vector2f x1y2(-areaX/2,areaZ/2, off, 0);
            Vector2f x2y1(areaX/2,-areaZ/2, off, 0);
            Vector2f x1y12 = x1y2.subv(x1y1);
            Vector2f x12y1 = x2y1.subv(x1y1);
            float x1y12d = x1y12.getDlugosc();
            float x12y1d = x12y1.getDlugosc();
            float wysokosc = 0;
            float step = 2;
            
            for(float i = 0; i <= x1y12d; i+=x1y12d){
                for(float j = 0; j < x12y1d; j+=step){
                    float jj = j+step;
                    if(jj>x12y1d) jj = x12y1d;          

                    wysokosc = Game::terrainLib->getHeight(x, y, x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/j).x + position[0], ( x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/j).y + position[2]));
                    points << x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/j).x;
                    points << wysokosc+0.5f;
                    points << x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/j).y;
                    wysokosc = Game::terrainLib->getHeight(x, y, x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/jj).x + position[0], ( x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/jj).y + position[2]));
                    points << x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/jj).x;
                    points << wysokosc+0.5f;
                    points << x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/jj).y;
                }
            }
            for(float j = 0; j <= x12y1d; j+=x12y1d){
                for(float i = 0; i < x1y12d; i+=step){
                    float ii = i+step;
                    if(ii>x1y12d) ii = x1y12d;          

                    wysokosc = Game::terrainLib->getHeight(x, y, x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/j).x + position[0], ( x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/j).y + position[2]));
                    points << x1y1.x + x1y12.divf(x1y12d/i).x + x12y1.divf(x12y1d/j).x;
                    points << wysokosc+0.5f;
                    points << x1y1.y + x1y12.divf(x1y12d/i).y + x12y1.divf(x12y1d/j).y;
                    wysokosc = Game::terrainLib->getHeight(x, y, x1y1.x + x1y12.divf(x1y12d/ii).x + x12y1.divf(x12y1d/j).x + position[0], ( x1y1.y + x1y12.divf(x1y12d/ii).y + x12y1.divf(x12y1d/j).y + position[2]));
                    points << x1y1.x + x1y12.divf(x1y12d/ii).x + x12y1.divf(x12y1d/j).x;
                    points << wysokosc+0.5f;
                    points << x1y1.y + x1y12.divf(x1y12d/ii).y + x12y1.divf(x12y1d/j).y;
                }
            }
            return true;
}

int ForestObj::getDefaultDetailLevel(){
    return -11;
}

void ForestObj::save(QTextStream* out){
    if (!loaded) return;
    if(Game::useOnlyPositiveQuaternions)
        Quat::makePositive(this->qDirection);
    
*(out) << "	Forest (\n";
*(out) << "		UiD ( "<<this->UiD<<" )\n";
if(this->treeTexture != "")
*(out) << "		TreeTexture ( "<<ParserX::AddComIfReq(this->treeTexture)<<" )\n";
*(out) << "		ScaleRange ( "<<this->scaleRangeX<<" "<<this->scaleRangeZ<<" )\n";
*(out) << "		Area ( "<<this->areaX<<" "<<this->areaZ<<" )\n";
*(out) << "		Population ( "<<this->population<<" )\n";
*(out) << "		TreeSize ( "<<this->treeSizeX<<" "<<this->treeSizeZ<<" )\n";
*(out) << "		StaticFlags ( "<<ParserX::MakeFlagsString(this->staticFlags)<<" )\n";
*(out) << "		Position ( "<<this->position[0]<<" "<<this->position[1]<<" "<<-this->position[2]<<" )\n";
*(out) << "		QDirection ( "<<this->qDirection[0]<<" "<<this->qDirection[1]<<" "<<this->qDirection[2]<<" "<<this->qDirection[3]<<" )\n";
if(Game::legacySupport)   *(out) << "		VDbId ( " << this->vDbId << " )\n";  // EFO 
if(this->staticDetailLevel >= 0)
*(out) << "		StaticDetailLevel ( "<<this->staticDetailLevel<<" )\n";
*(out) << "	)\n";
}

bool ForestObj::allowNew(){
    return true;
}

void ForestObj::deleteVBO(){
    //this->shape.deleteVBO();
    this->init = false;
    this->box.deleteVBO();
}

void ForestObj::InvalidateSeasonTextures(){
    ForestObj::TextureRevision++;
}

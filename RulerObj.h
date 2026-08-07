/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef RULEROBJ_H
#define	RULEROBJ_H

#include "WorldObj.h"
#include <QString>
#include "FileBuffer.h"
#include "ComplexLine.h"

class OglObj;

class RulerObj : public WorldObj {
public:
    static bool TwoPointRuler;
    static bool DrawPoints;
    
    RulerObj();
    RulerObj(const RulerObj& o);
    WorldObj* clone();
    virtual ~RulerObj();
    bool allowNew();
    void reload();
    void setTemplate(QString name);
    void load(int x, int y);
    void set(QString sh, QString val);
    void set(QString sh, FileBuffer* data);
    void setPosition(int x, int z, float* p);
    bool select(int value);
    void save(QTextStream* out);
    bool hasLinePoints();
    void getLinePoints(float *&punkty);
    void getPosition(float len, float* pos);
    float getLength();
    float getGeoLength();
    float getElevation();
    int getPointCount() const;
    void getPointWorldPosition(int index, float *pos) const;
    bool isWaterRuler() const;
    void setWaterRuler(bool enabled);
    void appendWaterPoint(int x, int z, float *p);
    bool isVegetationRuler() const;
    void setVegetationRuler(bool enabled);
    void appendVegetationPoint(int x, int z, float *p);
    void selectVegetationPointNear(int x, int z, const float *p);
    void snapSelectedVegetationPointToTerrain();
    void createRoadPaths();
    void removeRoadPaths();
    void enableShape();
    void render(GLUU* gluu, float lod, float posx, float posz, float* playerW, float* target, float fov, int selectionColor, int renderMode);

private:
    static constexpr float VegetationHalfWidth = 50.0f;
    static constexpr float VegetationPostHeight = 50.0f;
    static constexpr float VegetationHandleGap = 5.0f;
    static constexpr float VegetationHandleSize = 4.0f;

    struct Point {
        bool selected = false;
        int shapeType = 0;
        float position[3];
        QVector<OglObj*> procShape;
        float quat[4];
        float matrix[16];
    };
    
    QVector<Point> points;
    OglObj* point3d = NULL;
    OglObj* line3d = NULL;
    OglObj* point3dSelected = NULL;
    OglObj* vegetationPost3d = NULL;
    OglObj* vegetationPostSelected3d = NULL;
    OglObj* vegetationBounds3d = NULL;
    OglObj* vegetationHandle3d = NULL;
    int selectionValue = 0;
    float length = 0;
    float geoLength = 0;
    bool waterRuler = false;
    bool vegetationRuler = false;
    bool vegetationPointMoved = false;
    
    void refreshLength();
    bool shapeEnabled = false;
    bool proceduralShapeInit = false;

};

#endif	/* RULEROBJ_H */


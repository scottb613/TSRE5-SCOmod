/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "OglObj.h"
#include <QPainter>

#ifndef TEXTOBJ_H
#define	TEXTOBJ_H

#include <QString>

class TextObj : public OglObj{
public:
    bool inUse = false;
    float pos[3];
    TextObj(QString val, float s = 0, float sc = 0, int resm = 1);
    TextObj();
    TextObj(int val, float s = 0, float sc = 0, int resm = 1);
    TextObj(const TextObj& orig);
    virtual ~TextObj();
    void pushRenderItem() override;
    void pushRenderItem(int selectionColor, float lod) override;
    void pushRenderItem(float rot);
    void render() override;
    void render(int selectionColor, float lod) override;
    void render(float rot);
    void setColor(int r, int g, int b);
    void setOColor(int r, int g, int b);
    void setFontName(QString val);
    void setFontSize(int val);
    void setRotOffset(float val);
    void setHeight(float val);
private:
    QString text;
    QString fontName;
    int fontSize = 24;
    void init();
    QColor color;
    QColor ocolor;
    bool isOutline = false;
    bool isInit = false;
    float size = 4;
    float scale = 1;
    float rotOffset = 3.14;
    float textHeight = 0.0f;
    int resMult = 1;
};

#endif	/* TEXTOBJ_H */


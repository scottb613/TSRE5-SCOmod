/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "GuiGlCompass.h"
#include "GLMatrix.h"
#include "GLUU.h"
#include "FileBuffer.h"

GuiGlCompass::GuiGlCompass() {
    
    static QString texString = QString("tsre_appdata/")+Game::AppDataVersion+"/compass.png";
    
    GLUU* gluu = GLUU::get();
    float *punkty = new float[54];
    int ptr = 0;

    float alpha = -gluu->alphaTest;
    // Keep the tape clear of the viewport edge and large enough to read
    // without returning to the tall legacy presentation.
    const float top = 0.96;
    const float sizex = 0.425;
    // Match the 2048x256 texture's 8:1 aspect ratio so lettering and ticks
    // retain their native proportions on screen.
    const float sizey = sizex / 8.0;
    
    punkty[ptr++] = (-sizex / 2);
    punkty[ptr++] = top;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = alpha;

    punkty[ptr++] = (-sizex / 2);
    punkty[ptr++] = top-sizey;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = 1.0;
    punkty[ptr++] = alpha;

    punkty[ptr++] = (sizex / 2);
    punkty[ptr++] = top-sizey;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.4;
    punkty[ptr++] = 1.0;
    punkty[ptr++] = alpha;

    punkty[ptr++] = (sizex / 2);
    punkty[ptr++] = top;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.4;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = alpha;

    punkty[ptr++] = (-sizex / 2);
    punkty[ptr++] = top;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = 0.0;
    punkty[ptr++] = alpha;

    punkty[ptr++] = (sizex / 2);
    punkty[ptr++] = top-sizey;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.4;
    punkty[ptr++] = 1.0;
    punkty[ptr++] = alpha;
    
    this->setMaterial(&texString);
    OglObj::init(punkty, ptr, RenderItem::VT, GL_TRIANGLES);
    
    delete[] punkty;
}

GuiGlCompass::~GuiGlCompass() {
}

void GuiGlCompass::render(){
    render(0);
}

void GuiGlCompass::render(float a){
    
    float *data = mapBuffer();
    a = -a / (M_PI*2);
    data[3] = a-0.2;
    data[3+6] = a-0.2;
    data[3+12] = a+0.2;
    data[3+18] = a+0.2;
    data[3+24] = a-0.2;
    data[3+30] = a+0.2;

    unmapBuffer();
    
    OglObj::render();
}

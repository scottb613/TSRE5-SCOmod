/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "Skydome.h"
#include "GLUU.h"
#include "SFile.h"
#include "ShapeLib.h"
#include <QFileInfo>

Skydome::Skydome() {
    const QString shapePath =
            Game::root + "/routes/" + Game::route + "/shapes/skydome.s";
    // Open Rails renders its own sky.  skydome.s is only an optional TSRE
    // editor asset and must not be reported as missing route content.
    if(!QFileInfo::exists(shapePath))
        return;

    int shape = Game::currentShapeLib->addShape(shapePath);
    this->shapePointer = Game::currentShapeLib->shape[shape];
    if(this->shapePointer == NULL)
        return;
    loaded = true;
}

Skydome::Skydome(const Skydome& orig) {
}

Skydome::~Skydome() {
}

void Skydome::render(GLUU* gluu, int renderMode) {
    if (!loaded) return;
    if (renderMode == gluu->RENDER_SHADOWMAP) {
        return;
    }
    
    gluu->enableTextures();
    gluu->mvPushMatrix();
    if(shapePointer != NULL)
        shapePointer->render();
    gluu->mvPopMatrix();
};

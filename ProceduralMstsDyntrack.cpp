/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ProceduralMstsDyntrack.h"
#include "Game.h"
#include "Vector2f.h"

#include "Vector3f.h"
#include "OglObj.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QCoreApplication>
#include <QFileInfo>

namespace {

QString dyntrackTexturePath(const QString &fileName) {
    const QString routePath = (Game::root + "/routes/" + Game::route + "/textures/" + fileName).toLower();
    if (QFileInfo::exists(routePath))
        return routePath;

    const QString contentPath = QCoreApplication::applicationDirPath() + "/content/dyntrack/" + fileName.toLower();
    if (QFileInfo::exists(contentPath))
        return contentPath;

    const QString templatePath = QCoreApplication::applicationDirPath() + "/tsre_assets/templateRoute_0.6/textures/" + fileName.toLower();
    if (QFileInfo::exists(templatePath))
        return templatePath;

    return routePath;
}

}

void ProceduralMstsDyntrack::GenShape(QVector<OglObj*> &shape, QVector<TSection> &sections) {
    constexpr int kMaxFloats = 55000;
    constexpr int kPdPerStraight = 54;
    constexpr int kSkPerStraight = 270;
    constexpr int kPdPerCurveStep = 54;
    constexpr int kSkPerCurveStep = 324;
    constexpr float kDefaultCurveStep = 0.03f;
    constexpr float kMaxCenterlineLen = 2048.0f;

    float railtopInner = 0.7175f;
    float railtopOuter = 0.7895f;
    if ((Game::railProfile[0] > 0) && (Game::railProfile[1] > 0)) {
        railtopInner = Game::railProfile[0];
        railtopOuter = Game::railProfile[1];
    }

    // Trim incoming sections to a single-tile length budget to avoid generating runaway geometry.
    QVector<TSection> trimmed;
    trimmed.reserve(sections.size());
    float remaining = kMaxCenterlineLen;
    for (int i = 0; i < sections.size(); i++) {
        float segLen = sections[i].getDlugosc();
        if (segLen < 1e-4f)
            continue;
        if (remaining <= 0.0f)
            break;
        if (segLen <= remaining + 1e-4f) {
            trimmed.push_back(sections[i]);
            remaining -= segLen;
            continue;
        }

        TSection s = sections[i];
        if (s.type == 0) {
            s.size = remaining;
            s.angle = remaining;
        } else if (s.type == 1 && s.radius > 0.1f) {
            float sign = (s.angle >= 0.0f) ? 1.0f : -1.0f;
            float newAngle = sign * (remaining / s.radius);
            s.angle = newAngle;
            s.size = newAngle;
        } else {
            break;
        }
        trimmed.push_back(s);
        break;
    }

    // Choose a curve subdivision step that fits within the fixed VBO budget.
    int straightCount = 0;
    float totalAbsAngle = 0.0f;
    for (int i = 0; i < trimmed.size(); i++) {
        if (trimmed[i].type == 0) {
            straightCount++;
            continue;
        }
        if (trimmed[i].type == 1 && trimmed[i].radius > 0.1f && std::fabs(trimmed[i].angle) > 1e-6f)
            totalAbsAngle += std::fabs(trimmed[i].angle);
    }

    int nMax = (kMaxFloats - (kSkPerStraight * straightCount)) / kSkPerCurveStep;
    if (nMax < 0)
        nMax = 0;

    auto requiredSteps = [&](float step) {
        int total = 0;
        for (int i = 0; i < trimmed.size(); i++) {
            if (trimmed[i].type != 1 || trimmed[i].radius <= 0.1f)
                continue;
            float a = std::fabs(trimmed[i].angle);
            if (a < 1e-6f)
                continue;
            total += (int)std::ceil(a / step);
        }
        return total;
    };

    float curveStep = kDefaultCurveStep;
    if (nMax > 0) {
        float minStep = (totalAbsAngle > 0.0f) ? (totalAbsAngle / (float)nMax) : kDefaultCurveStep;
        curveStep = std::max(kDefaultCurveStep, minStep);
        // Compensate for ceil() and any numerical edge cases.
        for (int it = 0; it < 32; it++) {
            if (requiredSteps(curveStep) <= nMax)
                break;
            curveStep *= 1.05f;
        }
    }

    std::vector<float> pdBuf(kMaxFloats);
    std::vector<float> skBuf(kMaxFloats);
    float* pd = pdBuf.data();
    float* sk = skBuf.data();

    int ptr = 0;
    int str = 0;
    
    Vector2f tx;
    Vector2f ty;
    Vector2f offpos(0.0, 0.0);
    Vector2f b1;
    Vector2f a1;
    Vector2f a;
    Vector2f b;
    Vector2f dl;
    
    float offrot = 0;
    GLUU *gluu = GLUU::get();
    float alpha = -gluu->alphaTest;
    
    bool bufferTrimmed = false;
    for (int i = 0; i < trimmed.size(); i++) {
        //if (sections[i].sectIdx > 100000000) continue;
        //prosta
        if (trimmed[i].type == 0) {
            if ((ptr + kPdPerStraight) > kMaxFloats || (str + kSkPerStraight) > kMaxFloats) {
                bufferTrimmed = true;
                break;
            }
            //podklady

            b.set(2.5, 0.0);
            b.rotate(offrot, 0);
            a1.set(0.0, trimmed[i].angle);
            a1.rotate(offrot, 0);

            pd[ptr++] = offpos.x + b.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y + b.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = -0.139000;
            pd[ptr++] = -1.0;
            pd[ptr++] = alpha;

            pd[ptr++] = offpos.x - b.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y - b.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = 0.862000;
            pd[ptr++] = -1.0;
            pd[ptr++] = alpha;

            pd[ptr++] = offpos.x - b.x + a1.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y - b.y + a1.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = 0.862000;
            pd[ptr++] = -1.0 + 0.2 * trimmed[i].angle;
            pd[ptr++] = alpha;

            pd[ptr++] = offpos.x + b.x + a1.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y + b.y + a1.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = -0.139000;
            pd[ptr++] = -1.0 + 0.2 * trimmed[i].angle;
            pd[ptr++] = alpha;

            pd[ptr++] = offpos.x + b.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y + b.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = -0.139000;
            pd[ptr++] = -1.0;
            pd[ptr++] = alpha;

            pd[ptr++] = offpos.x - b.x + a1.x;
            pd[ptr++] = 0.2;
            pd[ptr++] = offpos.y - b.y + a1.y;
            pd[ptr++] = 0.0;
            pd[ptr++] = 1.0;
            pd[ptr++] = 0.0;
            pd[ptr++] = 0.862000;
            pd[ptr++] = -1.0 + 0.2 * trimmed[i].angle;
            pd[ptr++] = alpha;
            //szyny

            tx.set(-railtopInner, 0.0);
            ty.set(-railtopOuter, 0.0);

            
            for (int jj = 0; jj < 2; jj++) {
                tx.rotate(offrot, 0);
                ty.rotate(offrot, 0);

                sk[str++] = offpos.x + tx.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + tx.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.0;
                sk[str++] = 0.2270;
                sk[str++] = alpha;

                sk[str++] = offpos.x + ty.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + ty.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.0;
                sk[str++] = 0.1330;
                sk[str++] = alpha;

                sk[str++] = offpos.x + ty.x + a1.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + ty.y + a1.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.2;
                sk[str++] = 0.1330;
                sk[str++] = alpha;

                sk[str++] = offpos.x + tx.x + a1.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + tx.y + a1.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.2;
                sk[str++] = 0.2270;
                sk[str++] = alpha;

                sk[str++] = offpos.x + tx.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + tx.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.0;
                sk[str++] = 0.2270;
                sk[str++] = alpha;

                sk[str++] = offpos.x + ty.x + a1.x;
                sk[str++] = 0.325000;
                sk[str++] = offpos.y + ty.y + a1.y;
                sk[str++] = 0.0;
                sk[str++] = 1.0;
                sk[str++] = 0.0;
                sk[str++] = 0.2;
                sk[str++] = 0.1330;
                sk[str++] = alpha;
                
                tx.set(railtopOuter, 0.0);
                ty.set(railtopInner, 0.0);
            }
            ///
            tx.set(railtopInner, 0.0);
            tx.rotate(offrot, 0);

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;
            
            tx.set(-railtopInner, 0.0);
            tx.rotate(offrot, 0);

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;
            
            tx.set(-railtopOuter, 0.0);
            tx.rotate(offrot, 0);

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            tx.set(railtopOuter, 0.0);
            tx.rotate(offrot, 0);

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x;
            sk[str++] = 0.2;
            sk[str++] = offpos.y + tx.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.07;
            sk[str++] = alpha;

            sk[str++] = offpos.x + tx.x + a1.x;
            sk[str++] = 0.325;
            sk[str++] = offpos.y + tx.y + a1.y;
            sk[str++] = 1.0;
            sk[str++] = 0.0;
            sk[str++] = 0.0;
            sk[str++] = 0.069;
            sk[str++] = 0.002;
            sk[str++] = alpha;
            
            offpos.x += a1.x;
            offpos.y += a1.y;
        }
        //krzywa
        if(trimmed[i].type==1){
            float kierunek = 1;
            if(trimmed[i].angle > 0){
                kierunek = -1;
            }
            float aa = 0;
            float angle;
            float angle2;
            for(angle = 0, angle2 = 0; angle2>trimmed[i].angle*kierunek; angle-=curveStep*kierunek,angle2-=curveStep){
                if ((ptr + kPdPerCurveStep) > kMaxFloats || (str + kSkPerCurveStep) > kMaxFloats) {
                    bufferTrimmed = true;
                    break;
                }
                if(trimmed[i].angle*kierunek-angle2<-curveStep) 
                    aa = -curveStep*kierunek;
                else 
                    aa = (trimmed[i].angle*kierunek-angle2)*kierunek;
                //podklady
                b1.set(-2.5, 0.0);
                a1.set(2.5, 0.0);
                a.set(-2.5, 0.0);
                b.set(2.5, 0.0);
                a1.rotate(angle, trimmed[i].radius*kierunek);
                b1.rotate(angle, trimmed[i].radius*kierunek);           
                a.rotate(angle+aa, trimmed[i].radius*kierunek);
                b.rotate(angle+aa, trimmed[i].radius*kierunek);
                a1.rotate(offrot, 0);
                b1.rotate(offrot, 0);
                a.rotate(offrot, 0);
                b.rotate(offrot, 0);
                dl.set(0.0, 0.0);
                dl.rotate(aa, trimmed[i].radius*kierunek); 
                float dlugosc = dl.getDlugosc();
                    
                pd[ptr++] = offpos.x+a1.x;  pd[ptr++] = 0.2; pd[ptr++] = offpos.y+a1.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = -0.139000;      pd[ptr++] = -1.0;
                pd[ptr++] = alpha;
                    
                pd[ptr++] = offpos.x+b1.x;  pd[ptr++] = 0.2; pd[ptr++] = offpos.y+b1.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = 0.862000;       pd[ptr++] = -1.0;
                pd[ptr++] = alpha;
                    
                pd[ptr++] = offpos.x+a.x;   pd[ptr++] = 0.2; pd[ptr++] = offpos.y+a.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = 0.862000;       pd[ptr++] = -1.0 + 0.2*dlugosc;
                pd[ptr++] = alpha;
                  
                pd[ptr++] = offpos.x+b.x;   pd[ptr++] = 0.2; pd[ptr++] = offpos.y+b.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = -0.139000;      pd[ptr++] = -1.0 + 0.2*dlugosc;
                pd[ptr++] = alpha;

                pd[ptr++] = offpos.x+a1.x;  pd[ptr++] = 0.2; pd[ptr++] = offpos.y+a1.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = -0.139000;      pd[ptr++] = -1.0;
                pd[ptr++] = alpha;
                    
                pd[ptr++] = offpos.x+a.x;   pd[ptr++] = 0.2; pd[ptr++] = offpos.y+a.y;
                pd[ptr++] = 0.0;            pd[ptr++] = 1.0; pd[ptr++] = 0.0;
                pd[ptr++] = 0.862000;       pd[ptr++] = -1.0 + 0.2*dlugosc;
                pd[ptr++] = alpha;
                
                //szyny
                   
                tx.set(-railtopInner, 0.0);
                ty.set(-railtopOuter, 0.0);
              
                for( int jj = 0; jj < 2; jj++){
                    a.set(tx.x, 0.0);
                    b.set(ty.x, 0.0);
                    tx.rotate(angle, trimmed[i].radius*kierunek);
                    ty.rotate(angle, trimmed[i].radius*kierunek);
                    a.rotate(angle+aa, trimmed[i].radius*kierunek);
                    b.rotate(angle+aa, trimmed[i].radius*kierunek);
                    a.rotate(offrot, 0);
                    b.rotate(offrot, 0);      
                    tx.rotate(offrot, 0); 
                    ty.rotate(offrot, 0); 
                        sk[str++] = offpos.x+tx.x; sk[str++] = 0.325000; sk[str++] = offpos.y+tx.y;
                        sk[str++] = 0.0; sk[str++] = 1.0; sk[str++] = 0.0;
                        sk[str++] = 0.0; sk[str++] = 0.2270;
                        sk[str++] = alpha;
                        
                        sk[str++] = offpos.x+ty.x; sk[str++] = 0.325000; sk[str++] = offpos.y+ty.y;
                        sk[str++] = 0.0; sk[str++] = 1.0; sk[str++] = 0.0;
                        sk[str++] = 0.0; sk[str++] = 0.1330;
                        sk[str++] = alpha;
                        
                        sk[str++] = offpos.x+b.x; sk[str++] = 0.325000; sk[str++] = offpos.y+b.y;
                        sk[str++] = 0.0; sk[str++] = 1.0; sk[str++] = 0.0;
                        sk[str++] = 0.2; sk[str++] = 0.1330;
                        sk[str++] = alpha;
                        
                        sk[str++] = offpos.x+a.x; sk[str++] =  0.325000; sk[str++] =  offpos.y+a.y;
                        sk[str++] = 0.0; sk[str++] =  1.0; sk[str++] = 0.0;
                        sk[str++] = 0.2; sk[str++] =  0.2270;
                        sk[str++] = alpha;

                        sk[str++] = offpos.x+tx.x; sk[str++] = 0.325000; sk[str++] = offpos.y+tx.y;
                        sk[str++] = 0.0; sk[str++] = 1.0; sk[str++] = 0.0;
                        sk[str++] = 0.0; sk[str++] = 0.2270;
                        sk[str++] = alpha;
                        
                        sk[str++] = offpos.x+b.x; sk[str++] = 0.325000; sk[str++] = offpos.y+b.y;
                        sk[str++] = 0.0; sk[str++] = 1.0; sk[str++] = 0.0;
                        sk[str++] = 0.2; sk[str++] = 0.1330;
                        sk[str++] = alpha;
                    ty.set(railtopInner, 0.0);
                    tx.set(railtopOuter, 0.0);
                }
                ///
                tx.set(railtopInner, 0.0);
                a.set(tx.x, 0.0);
                tx.rotate(angle, trimmed[i].radius*kierunek);
                a.rotate(angle+aa, trimmed[i].radius*kierunek);
                a.rotate(offrot, 0); 
                tx.rotate(offrot, 0); 
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;

                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;
                tx.set(-railtopInner, 0.0);
                a.set(tx.x, 0.0);
                tx.rotate(angle, trimmed[i].radius*kierunek);
                a.rotate(angle+aa, trimmed[i].radius*kierunek);
                a.rotate(offrot, 0); 
                tx.rotate(offrot, 0); 
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;

                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;

                tx.set(-railtopOuter, 0.0);
                a.set(tx.x, 0.0);
                tx.rotate(angle, trimmed[i].radius*kierunek);
                a.rotate(angle+aa, trimmed[i].radius*kierunek);
                a.rotate(offrot, 0); 
                tx.rotate(offrot, 0); 

                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;

                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;                
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;

                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;                
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;

                tx.set(railtopOuter, 0.0);
                a.set(tx.x, 0.0);
                tx.rotate(angle, trimmed[i].radius*kierunek);
                a.rotate(angle+aa, trimmed[i].radius*kierunek);
                a.rotate(offrot, 0); 
                tx.rotate(offrot, 0); 

                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.325; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.002;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;

                    sk[str++] = offpos.x+a.x; sk[str++] = 0.2; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+tx.x; sk[str++] = 0.2; sk[str++] = offpos.y+tx.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.0; sk[str++] = 0.07;
                    sk[str++] = alpha;
                    
                    sk[str++] = offpos.x+a.x; sk[str++] = 0.325; sk[str++] = offpos.y+a.y;
                    sk[str++] = 1.0; sk[str++] = 0.0; sk[str++] = 0.0;
                    sk[str++] = 0.069; sk[str++] = 0.002;
                    sk[str++] = alpha;
            }
            if (bufferTrimmed)
                break;
            a.set(0.0, 0.0);
            a.rotate(trimmed[i].angle, trimmed[i].radius*kierunek);
            a.rotate(offrot, 0);
            offpos.x+=a.x; offpos.y+=a.y;
            offrot+=trimmed[i].angle;
        }
    }
    Q_UNUSED(bufferTrimmed);
    //qDebug() << ptr << "" << str;
    
    QString* texturePath = new QString(dyntrackTexturePath("acleantrack1.ace"));
    shape.push_back(new OglObj());
    shape.push_back(new OglObj());
    shape[0]->setMaterial(texturePath);
    texturePath = new QString(dyntrackTexturePath("acleantrack2.ace"));
    shape[1]->setMaterial(texturePath);
    shape[0]->init(pd, ptr, RenderItem::VNTA, GL_TRIANGLES );
    shape[1]->init(sk, str, RenderItem::VNTA, GL_TRIANGLES );
    
    float bound[6];
    bound[0] = -9999;
    bound[1] = 9999;
    bound[2] = -9999;
    bound[3] = 9999;
    bound[4] = -9999;
    bound[5] = 9999;
    for (int i = 0; i < ptr ; i+=9) {
        if(pd[i+0] < bound[1]) bound[1] = pd[i+0];
        if(pd[i+1] < bound[3]) bound[3] = pd[i+1];
        if(pd[i+2] < bound[5]) bound[5] = pd[i+2];
        if(pd[i+0] > bound[0]) bound[0] = pd[i+0];
        if(pd[i+1] > bound[2]) bound[2] = pd[i+1];
        if(pd[i+2] > bound[4]) bound[4] = pd[i+2];
    }
    for (int i = 0; i < str ; i+=9) {
        if(sk[i+0] < bound[1]) bound[1] = sk[i+0];
        if(sk[i+1] < bound[3]) bound[3] = sk[i+1];
        if(sk[i+2] < bound[5]) bound[5] = sk[i+2];
        if(sk[i+0] > bound[0]) bound[0] = sk[i+0];
        if(sk[i+1] > bound[2]) bound[2] = sk[i+1];
        if(sk[i+2] > bound[4]) bound[4] = sk[i+2];
    }
    
    shape[0]->setBound(bound);
    shape[1]->setBound(bound);
    
}

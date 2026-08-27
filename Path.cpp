/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "Path.h"
#include "TRnode.h"
#include <QString>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStringConverter>
#include "Game.h"
#include "FileBuffer.h"
#include "ReadFile.h"
#include "Ruch.h"
#include "Vector3f.h"
#include "ParserX.h"
#include "OglObj.h"
#include "TrackItemObj.h"
#include "GLMatrix.h"
#include "TRitem.h"

Path::Path() {
    typeObj = activitypath;
}

Path::Path(QString p, QString n, bool nowe) {
    typeObj = activitypath;
    pathid = p.toLower() + "/" + n.toLower();
    pathid.replace("//", "/");
    path = p;
    name = n;
    nameId = n.section(".", 0, -2);
    if(!nowe){
        loaded = -1;
        load();
        if(loaded == 1){
            metadataConfirmed = true;
            initRoute();
        }
    } else {
        loaded = 1;
        modified = true;
    }
}

bool Path::isModified(){
    return modified;
}

void Path::markSaved(){
    modified = false;
}

const QVector<QLineF>& Path::getMapLines(){
    init3dShapes(false);
    return mapLines;
}

void Path::load(){
    QString sh;
    pathid.replace("//", "/");
    // if(Game::debugOutput) qDebug() << pathid;
    QFile *file = new QFile(pathid);
    if (!file->open(QIODevice::ReadOnly)) {
        if(Game::debugOutput) qDebug() << pathid << "not exist";
        return;
    }

    FileBuffer* data = ReadFile::read(file);
    file->close();
    if (data == nullptr)
        return;
    data->toUtf16();
    data->skipBOM();

    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
        //qDebug() << sh;
        if (sh == ("simisa@@@@@@@@@@jinx0p0t______")) {
            continue;
        }
        
        if (sh == ("_comment")) 
        {   ParserX::SkipToken(data);
            continue;
        }

        if (sh == ("_skip")) 
        {   ParserX::SkipToken(data);
            continue;
        }
            
        if (sh == ("serial")) {
            serial = ParserX::GetNumber(data);
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("trackpdps")) {
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                if (sh == ("trackpdp")) {
                    trackPdp.push_back(new float[7]);
                    for(int i = 0; i < 7; i++)
                        trackPdp.back()[i] = ParserX::GetNumber(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if(Game::debugOutput) qDebug() << "#TrackPDPs - undefined token: " << sh;
                ParserX::SkipToken(data);
            }
            ParserX::SkipToken(data);
            continue;
        }
        if (sh == ("trackpath")) {
            while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                if (sh == ("trpathname")) {
                    trPathName = ParserX::GetString(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("name")) {
                    displayName = ParserX::GetString(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("trpathstart")) {
                    trPathStart = ParserX::GetString(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("trpathend")) {
                    trPathEnd = ParserX::GetString(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("trpathflags")) {
                    trPathFlags = ParserX::GetHex(data);
                    ParserX::SkipToken(data);
                    continue;
                }
                if (sh == ("trpathnodes")) {
                    while (!((sh = ParserX::NextTokenInside(data).toLower()) == "")) {
                        if (sh == ("trpathnode")) {
                            trPathNode.push_back(new unsigned int[4]);
                            trPathNode.back()[0] = ParserX::GetHex(data);
                            trPathNode.back()[1] = ParserX::GetUInt(data);
                            trPathNode.back()[2] = ParserX::GetUInt(data);
                            trPathNode.back()[3] = ParserX::GetUInt(data);
                            ParserX::SkipToken(data);
                            continue;
                        }
                        if(Game::debugOutput) qDebug() << "#TrPathNodes - undefined token: " << sh;
                        ParserX::SkipToken(data);
                    }
                    ParserX::SkipToken(data);
                    continue;
                }
                if(Game::debugOutput) qDebug() << "#TrackPath - undefined token: " << sh;
                ParserX::SkipToken(data);
            }
            ParserX::SkipToken(data);
            continue;
        }
        if(Game::debugOutput) qDebug() << "#PAT - undefined token: " << sh;
        ParserX::SkipToken(data);
    }
    
    loaded = 1;
}

QMap<int, int>* Path::getJunctionDirections(){
    return &junctionDirections;
}

int Path::getStartDirection(){
    init3dShapes(false);
    
    return startDirection;
}

float* Path::getStartPositionTXZ(float* out){
    init3dShapes(false);
    
    if(out == NULL)
        out = new float[4];
        
    float posT[2];
    float posW[3];
    float tpos1[3];
    TDB* tdb = Game::trackDB;
    
    posT[0] = node[0].tilex;
    posT[1] = node[0].tilez;
    Vec3::copy(posW, node[0].pos);
    tdb->findNearestPositionOnTDB(posT, posW, NULL, tpos1);
    out[0] = posT[0];
    out[1] = -posT[1];
    out[2] = posW[0];
    out[3] = -posW[2];
    //nodeId1 = tpos1[0];
    //currentDistance = tpos1[1];
    //currentNodeId = nodeId1;
    return out;
}

void Path::initRoute(){
    node.clear();
    junctionDirections.clear();
    mapLines.clear();
    linesX.clear();
    linesZ.clear();
    for(OglObj *line : lines)
        delete line;
    lines.clear();
    pathObjects.clear();
    for(auto it = pathObjectsMap.begin(); it != pathObjectsMap.end(); ++it)
        delete it.value();
    pathObjectsMap.clear();
    isinit1 = false;
    isinit2 = false;

    if(Game::trackDB == NULL)
        return;
    if(trPathNode.size() < 1)
        return;
    
    unsigned int current = 0;
    unsigned int trackPdpId = 0;
    
    while(current < 1000000) {
        trackPdpId = trPathNode[current][3];
        current = trPathNode[current][1];
        node.push_back(PathNode());
        node.back().next = current;
        node.back().tilex = trackPdp[trackPdpId][0];
        node.back().tilez = -trackPdp[trackPdpId][1];
        node.back().pos[0] = trackPdp[trackPdpId][2];
        node.back().pos[1] = trackPdp[trackPdpId][3];
        node.back().pos[2] = -trackPdp[trackPdpId][4];
        node.back().flag1 = trackPdp[trackPdpId][5];
        node.back().flag2 = trackPdp[trackPdpId][6];
        //qDebug() << trackPdp;
    }
    //init3dShapes();
}

void Path::init3dShapes(bool initShapes){
    if(isinit1 && !initShapes)
        return;
    if(isinit2 && initShapes)
        return;
    lines.clear();
    if(!isinit1)
        mapLines.clear();
    int fail = 0;
    TDB* tdb = Game::trackDB;
    if(tdb == NULL){
        qWarning() << "Unable to build path geometry without TrackDB:" << pathid;
        return;
    }
    const auto trackNode = [tdb](int id) -> TRnode* {
        const auto it = tdb->trackNodes.find(id);
        return it == tdb->trackNodes.end() ? NULL : it->second;
    };
    const auto abortInvalidPath = [this, initShapes](const QString &reason) {
        qWarning() << "Skipping invalid path geometry" << pathid << "-" << reason;
        mapLines.clear();
        for(OglObj *line : lines)
            delete line;
        lines.clear();
        linesX.clear();
        linesZ.clear();
        isinit1 = true;
        if(initShapes)
            isinit2 = true;
    };
    float posT[2];
    float posW[3];
    float tpos1[3] = {-1, 0, 0};
    int nodeId1, currentDistance;
    int currentNodeId = -1;
    int lastNodeId = -1;
    float lastDistance = 0;
    float distance1 = 0;
    float distance2 = 0;
    float tp1[3], tp2[3];
    
    float distanceDownPath = 0;
    for(int i = 0; i < node.size(); i++){
        if(node[i].flag1 == 1){
            if(Game::debugOutput) qDebug() << "point";
            posT[0] = node[i].tilex;
            posT[1] = node[i].tilez;
            Vec3::copy(posW, node[i].pos);
            if(tdb->findNearestPositionOnTDB(posT, posW, NULL, tpos1) < 0){
                abortInvalidPath(tr("path point %1 is not on the current TrackDB").arg(i));
                return;
            }
            nodeId1 = tpos1[0];
            TRnode *currentPathNode = trackNode(nodeId1);
            if(currentPathNode == NULL){
                abortInvalidPath(tr("path point %1 references missing track node %2")
                                 .arg(i).arg(nodeId1));
                return;
            }
            currentDistance = tpos1[1];
            currentNodeId = nodeId1;

            if(lastNodeId < 0){
                lastNodeId = nodeId1;
                lastDistance = currentDistance;
                continue;
            }
            
            TRnode *lastPathNode = trackNode(lastNodeId);
            if(lastPathNode == NULL){
                abortInvalidPath(tr("path point %1 follows missing track node %2")
                                 .arg(i).arg(lastNodeId));
                return;
            }
            if(lastPathNode->typ != 1){
                if(currentPathNode->TrPinS[0] == lastNodeId){
                    distance1 = 0;
                } else {
                    distance1 = tdb->getVectorSectionLength(currentNodeId);
                }
                distance2 = currentDistance;
            } else {
                distance1 = lastDistance;
                distance2 = currentDistance;
            }
            
            if(lastPathNode->typ == 2){
                if(lastPathNode->TrPinS[1] == currentNodeId)
                    junctionDirections[lastNodeId] = 0;
                else
                    junctionDirections[lastNodeId] = 1;
            }
            
            lastNodeId = nodeId1;
            lastDistance = currentDistance;
        } else if(node[i].flag1 == 2){
            if(Game::debugOutput) qDebug() << "junction";
            //qDebug() << node[i].tilex, node[i].tilez, node[i].pos[0], node[i].pos[1], node[i].pos[2];
            nodeId1 = tdb->findNearestNode(node[i].tilex, node[i].tilez, node[i].pos, NULL, 4, false);
            //qDebug() << nodeId1;
            if(lastNodeId < 0){
                if(Game::debugOutput) qDebug() << "fail";
                abortInvalidPath(tr("path starts with an unconnected junction at point %1")
                                 .arg(i));
                return;
            }

            TRnode *lastPathNode = trackNode(lastNodeId);
            TRnode *junctionNode = trackNode(nodeId1);
            if(lastPathNode == NULL || junctionNode == NULL){
                abortInvalidPath(tr("path point %1 references missing track node %2")
                                 .arg(i).arg(lastPathNode == NULL ? lastNodeId : nodeId1));
                return;
            }

            if(lastPathNode->typ != 1){
                currentNodeId = tdb->findVectorNodeBetweenTwoNodes(lastNodeId, nodeId1);
                TRnode *currentPathNode = trackNode(currentNodeId);
                if(currentPathNode == NULL){
                    abortInvalidPath(
                        tr("track nodes %1 and %2 have no connecting vector at point %3")
                            .arg(lastNodeId).arg(nodeId1).arg(i));
                    return;
                }
                if(currentPathNode->TrPinS[0] == nodeId1){
                    distance2 = 0;
                    distance1 = tdb->getVectorSectionLength(currentNodeId);
                } else {
                    distance1 = 0;
                    distance2 = tdb->getVectorSectionLength(currentNodeId);
                }
            } else {
                currentNodeId = lastNodeId;
                if(lastPathNode->TrPinS[0] == nodeId1){
                    /*&& tdb->trackNodes[currentNodeId]->TrPinK[0] == 1*/
                    distance2 = 0;
                } else {
                    distance2 = tdb->getVectorSectionLength(currentNodeId);
                }
                distance1 = lastDistance;
            }
            
            if(i == 1){
                if(trackNode(currentNodeId)->TrPinS[0] == nodeId1)
                    startDirection = 1;
                else 
                    startDirection = 0;
            }
            
            if(lastPathNode->typ == 2){
                if(lastPathNode->TrPinS[1] == currentNodeId)
                    junctionDirections[lastNodeId] = 0;
                else
                    junctionDirections[lastNodeId] = 1;
            }

            lastNodeId = nodeId1;
        } else {
            if(Game::debugOutput) qDebug() << "fail";
            fail++;
        }
        
        if(fail > 0)
            return;
        
        if(currentNodeId < 0){
            abortInvalidPath(tr("path point %1 did not resolve to a vector node").arg(i));
            return;
        }
        if(Game::debugOutput) qDebug() << "currentNodeId" << currentNodeId << distance1 << distance2 ;

        TRnode *currentPathNode = trackNode(currentNodeId);
        if(currentPathNode == NULL){
            abortInvalidPath(tr("path point %1 resolved to missing vector node %2")
                             .arg(i).arg(currentNodeId));
            return;
        }
        for(int ti = 0; ti < currentPathNode->iTri; ti++){
            int trid1 = currentPathNode->trItemRef[ti];
            const auto firstItemIt = tdb->trackItems.find(trid1);
            TRitem *firstItem = firstItemIt == tdb->trackItems.end()
                ? NULL : firstItemIt->second;
            if(firstItem != NULL){
                if(firstItem->type != "platformitem")
                    continue;
                int trid2 = firstItem->platformTrItemData[1];
                const auto secondItemIt = tdb->trackItems.find(trid2);
                TRitem *secondItem = secondItemIt == tdb->trackItems.end()
                    ? NULL : secondItemIt->second;
                if(secondItem != NULL){
                    float ddd1 = firstItem->getTrackPosition();
                    if(distance1 > distance2)
                        ddd1 = 2*tdb->getVectorSectionLength(currentNodeId) - ddd1 - distance1;
                    else 
                        ddd1 = ddd1 - distance1;
                    float ddd2 = secondItem->getTrackPosition();
                    if(distance1 > distance2)
                        ddd2 = 2*tdb->getVectorSectionLength(currentNodeId) - ddd2 - distance1;
                    else 
                        ddd2 = ddd2 - distance1;
                    
                    float dist = 0;
                    int trid = 0;
                    if(ddd1 > ddd2){
                        dist = ddd1;
                        trid = trid1;
                    }else{
                        dist = ddd2;
                        trid = trid2;
                    }
                    if(pathObjectsMap[distanceDownPath + dist] == NULL)
                        pathObjectsMap[distanceDownPath + dist] = new PathObject();
                    TRitem *platformItem = trid == trid1 ? firstItem : secondItem;
                    pathObjectsMap[distanceDownPath + dist]->name = platformItem->stationName;
                    pathObjectsMap[distanceDownPath + dist]->trItemId = trid;
                    pathObjectsMap[distanceDownPath + dist]->distanceDownPath = distanceDownPath + dist;
                    if(Game::debugOutput) qDebug() << "=" << platformItem->stationName
                                                   << distanceDownPath + dist;
                }
            }
        }
        
        if(distance1 > distance2){
            float temp = distance2;
            distance2 = distance1;
            distance1 = temp;
        }
        distanceDownPath += distance2 - distance1;
        
        QMapIterator<float, Path::PathObject*> it(pathObjectsMap);
        pathObjects.clear();
        while (it.hasNext()) {
            it.next();
            pathObjects.push_back(it.value());
        }
        
        float *ptr = NULL, *punkty = NULL;
        int length, len = 0;
        //qDebug() << "currentNodeId" << currentNodeId << distance1 << distance2 ;
        tdb->getVectorSectionLine(ptr, length, node[i].tilex, node[i].tilez, currentNodeId, true);
        if(!isinit1 && ptr != NULL){
            const QPointF tileOffset(node[i].tilex * 2048.0, node[i].tilez * 2048.0);
            for(int ii = 0; ii + 11 < length; ii += 12){
                if(ptr[ii + 5] < distance1)
                    continue;
                const bool endOfRange = ptr[ii + 11] > distance2;
                mapLines.push_back(QLineF(QPointF(ptr[ii], ptr[ii + 2]) + tileOffset,
                                          QPointF(ptr[ii + 6], ptr[ii + 8]) + tileOffset));
                if(endOfRange)
                    break;
            }
        }

        if(!initShapes){
            delete[] ptr;
            continue;
        }

        OglObj *line = new OglObj();
        punkty = new float[length+6];
        bool endd = false;
        
        for(int ii = 0; ii < length; ii+=12){
            if(ptr[ii+5] < distance1) continue;
            if(ii + 17 >= length || ptr[ii + 17] > distance2)
                endd = true;
            //if(len == 0)
            //    Vec3::set(tp1, posB[0], posB[1]+1, -posB[2]);
            //else 
                Vec3::set(tp1, ptr[ii+0], ptr[ii+1], ptr[ii+2]);
            //if(endd)
            //    Vec3::set(tp2, posE[0], posE[1]+1, -posE[2]); 
            // else
                Vec3::set(tp2, ptr[ii+6], ptr[ii+7], ptr[ii+8]);
            
            punkty[len++] = tp1[0];
            punkty[len++] = tp1[1];
            punkty[len++] = tp1[2];
            punkty[len++] = tp2[0];
            punkty[len++] = tp2[1];
            punkty[len++] = tp2[2];

            if(endd) break;
        }

        line->init(punkty, len, RenderItem::V, GL_LINES);
        delete[] ptr;
        delete[] punkty;
        
        line->setMaterial(0.0, 1.0, 0.0);
        line->setLineWidth(2);
        lines.push_back(line);
        linesX.push_back(node[i].tilex);
        linesZ.push_back(node[i].tilez);
    }
    isinit1 = true;
    if(initShapes)
        isinit2 = true;
}

void Path::render(GLUU* gluu, float * playerT, int selectionColor){
    if(pointer3d == NULL){
        pointer3d = new TrackItemObj(1);
        pointer3d->setMaterial(0.0,1.0,0.0);
    }
    
    if(!isinit2){
        init3dShapes();
    }
    
    if(!Game::viewInteractives)
        return;

    for(int i = 0; i < node.size(); i++){
        gluu->mvPushMatrix();
        //Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, node[i].pos[0], node[i].pos[1]+10.0, -node[i].pos[2]);
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, node[i].pos[0] + 2048 * (node[i].tilex - playerT[0]), node[i].pos[1]+2.2, node[i].pos[2] + 2048 * (node[i].tilez - playerT[1]));
        gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
    
        pointer3d->render();
        gluu->mvPopMatrix();
    }
    
    for(int i = 0; i < lines.size(); i++){
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048 * (linesX[i] - playerT[0]), 2.2, 2048 * (linesZ[i] - playerT[1]));
        gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        lines[i]->render();
        gluu->mvPopMatrix();
    }
}

Path::Path(const Path& orig) {
}

Path::~Path() {
}

void Path::CreatePaths(TDB * tdb){
    QString path;
    path = Game::root + "/routes/" + Game::route + "/paths";
    QDir dir(path);
    if(Game::debugOutput) qDebug() << path;
    dir.setNameFilters(QStringList() << "*.pat");
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList())
    {
        if(Game::debugOutput) qDebug() << dirFile;
        if(!QDir(dirFile).exists())
            dir.remove(dirFile);
    }
    
    QFile file;
    QTextStream out;
    QString filepath;
    
    Ruch ruch;
    Vector3f* pos;
    for (int i = 1; i <= tdb->iTRnodes; i++) {
        if (tdb->trackNodes[i] == NULL) continue;
        if (tdb->trackNodes[i]->typ == 0) {
            //tdb->trackNodes[i]->TrPinS[0];
            ruch.set(i, 0, 0);

            filepath = path+"/"+QString::number(i,10)+".pat";
            file.setFileName(filepath);
            //qDebug() << filepath;
            if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
                qWarning() << "Unable to create path file" << filepath << file.errorString();
                continue;
            }
            out.setDevice(&file);
            out.setRealNumberPrecision(8);
            out.setEncoding(QStringConverter::Utf16);
            out.setGenerateByteOrderMark(true);

            out << "SIMISA@@@@@@@@@@JINX0P0t______" << "\n\n";
            out << "Serial ( 1 )" << "\n";
            out << "TrackPDPs (" << "\n";
            //ruch.toNext(1);
            //pos = ruch.getPosition();
            float *dPos = ruch.getCurrentPosition();
            Game::check_coords(dPos[5],dPos[6],dPos[0],dPos[2]);
            if(Game::debugOutput) qDebug() << dPos[0]<<" "<<dPos[1]<<" "<<dPos[2];
            out << "	TrackPDP ( "<<dPos[5]<<" "<<dPos[6]<<" "<<dPos[0]<<" "<<dPos[1]<<" "<<dPos[2]<<" 1 1 )" << "\n";
            ruch.next(10);
            dPos = ruch.getCurrentPosition();
            Game::check_coords(dPos[5],dPos[6],dPos[0],dPos[2]);
            if(Game::debugOutput) qDebug() << dPos[0]<<" "<<dPos[1]<<" "<<dPos[2];
            out << "	TrackPDP ( "<<dPos[5]<<" "<<dPos[6]<<" "<<dPos[0]<<" "<<dPos[1]<<" "<<dPos[2]<<" 1 1 )" << "\n";
            out << ")" << "\n";
            out << "TrackPath (" << "\n";
            out << "	TrPathName ( "<<i<<" )" << "\n";
            out << "	Name ( endNode_"<<i<<" )" << "\n";
            out << "	TrPathStart ( endNode_"<<i<<" )" << "\n";
            out << "	TrPathEnd ( endNode_"<<i<<" )" << "\n";
            out << "	TrPathNodes ( 2" << "\n";
            out << "		TrPathNode ( 00000000 1 4294967295 0 )" << "\n";
            out << "		TrPathNode ( 00000000 4294967295 4294967295 1 )" << "\n";
            out << "	)" << "\n";
            out << ")" << "\n";
            
            out.flush();
            file.close();
        }
        
    }
}

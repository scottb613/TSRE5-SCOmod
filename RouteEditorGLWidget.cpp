/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "RouteEditorGLWidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QJsonObject>
#include <math.h>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif
#include "GLUU.h"
#include "SFile.h"
#include "ReadFile.h"
#include "FileBuffer.h"
#include "Route.h"
#include "GLMatrix.h"
#include "Eng.h"
#include "Tile.h"
#include "Game.h"
#include "GLH.h"
#include "Vector2f.h"
#include "TerrainLib.h"
#include "Brush.h"
#include "GeoCoordinates.h"
#include "MapWindow.h"
#include "TerrainTreeWindow.h"
#include "ShapeLib.h"
#include "EngLib.h"
#include "QOpenGLFunctions_3_3_Core"
#include "Undo.h"
#include "Environment.h"
#include "Terrain.h"
#include "ActivityObject.h"
#include "GuiFunct.h"
#include "GuiGlCompass.h"
#include "ActLib.h"
#include "Activity.h"
#include "PlayActivitySelectWindow.h"
#include "SoundManager.h"
#include "Skydome.h"
#include "OpenGL3Renderer.h"
#include <QDebug>
#include "RouteEditorClient.h"
#include "RouteClient.h"
#include "ClientInfo.h"
#include "StatusWindow.h"
#include "Texture.h"
#include "ObjTools.h"
#include "RouteMergeDialog.h"
#include "TerrainTools.h" // Include the dialog header
#include "TRitem.h"


RouteEditorGLWidget::RouteEditorGLWidget(QWidget *parent)
: QOpenGLWidget(parent),
m_xRot(0),
m_yRot(0),
m_zRot(0) {

    this->installEventFilter(this);
}





bool RouteEditorGLWidget::eventFilter(QObject *object, QEvent *event){
    if (event->type() == QEvent::FocusIn){
        //qDebug() << "aaaaa";
        bolckContextMenu = true;
    }
    return false;
}

RouteEditorGLWidget::~RouteEditorGLWidget() {
    cleanup();
}

QSize RouteEditorGLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize RouteEditorGLWidget::sizeHint() const {
    return QSize(1000, 700);
}

void RouteEditorGLWidget::cleanup() {
    makeCurrent();
    //delete gluu->m_program;
    //gluu->m_program = 0;
    doneCurrent();
}

void RouteEditorGLWidget::timerEvent(QTimerEvent * event) {
    Game::currentShapeLib = currentShapeLib;
    timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeNow - lastTime < 1)
        fps = 1;
    else
        fps = 1000.0 / (timeNow - lastTime);
    if (fps < 10) fps = 10;

    if (timeNow % 200 < lastTime % 200) {
        //qDebug() << "new second" << timeNow;
        if (selectedObj != NULL)
        {
          emit updateProperties(selectedObj);

            QString selectedType = "";
            Game::objSelected = true;
            if(selectedObj->typeObj == GameObj::terrainobj) {
                selectedType = "Terrain";
            } else if(selectedObj->typeObj == GameObj::tritemobj) {
                TRitem* trItem = (TRitem*)selectedObj;
                if(trItem->tdbId == 1)
                    selectedType = "Road";
                else if(trItem->tdbId == 0)
                    selectedType = "Track";
                else
                    selectedType = "Track Item";
            } else if(selectedObj->typeObj == GameObj::activityobj) {
                selectedType = "Activity";
            } else if(selectedObj->typeObj == GameObj::consistobj) {
                selectedType = "Consist";
            } else if(selectedObj->typeObj == GameObj::worldobj) {
                WorldObj* worldObj = (WorldObj*)selectedObj;
                if(worldObj->typeID == WorldObj::sstatic)
                    selectedType = "Static Object";
                else if(worldObj->typeID == WorldObj::trackobj || worldObj->typeID == WorldObj::dyntrack)
                    selectedType = "Track";
                else if(worldObj->typeID == WorldObj::platform || worldObj->typeID == WorldObj::siding ||
                        worldObj->typeID == WorldObj::carspawner || worldObj->typeID == WorldObj::pickup ||
                        worldObj->typeID == WorldObj::levelcr || worldObj->typeID == WorldObj::hazard)
                    selectedType = "Interactive";
                else if(worldObj->typeID == WorldObj::signal || worldObj->typeID == WorldObj::speedpost)
                    selectedType = "Track Item";
                else if(worldObj->typeID == WorldObj::forest || worldObj->typeID == WorldObj::polyforest)
                    selectedType = "Forest";
                else if(worldObj->typeID == WorldObj::transfer)
                    selectedType = "Transfer";
                else if(worldObj->typeID == WorldObj::soundsource || worldObj->typeID == WorldObj::soundregion)
                    selectedType = "Sound";
                else if(worldObj->typeID == WorldObj::ruler)
                    selectedType = "Ruler";
                else if(worldObj->typeID == WorldObj::groupobject)
                    selectedType = "Group";
                else
                    selectedType = "World Object";
            } else {
                selectedType = selectedObj->getName();
            }
            emit updStatus(QString("object"), selectedType);
        } else {
            Game::objSelected = false;
            emit updStatus(QString("object"), QString(""));
        }
        Undo::StateEndIfLongTime();
    }

    if (timeNow % 100 < lastTime % 100) {
        //qDebug() << "new second" << timeNow;
        if(Game::serverClient != NULL){
            Game::serverClient->updatePointerPosition((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);

        }


    }
    if (timeNow % 250 < lastTime % 250) {

        gluu->setMatrixUniforms();

       //// This is a fake signal mechanism using a Game value
       if(Game::resetTools == true)
       {
           // qDebug() << " fake signal ";
            emit enableTool("selectTool");
            Game::resetTools = false;
       }

        /// try to send camera status every half second or so?
        if (Game::lockCamera) emit updStatus(QString("camera"), QString("Camera LOCK")); else emit updStatus(QString("camera"), QString("Camera FREE"));
        if (Game::cameraStickToTerrain) emit updStatus(QString("camterr"), QString("Cam Terrain LOCK")); else emit updStatus(QString("camterr"), QString("Cam Terrain FREE"));

        if(autoAddToTDB == true) emit updStatus(QString("autotdb"), QString("AutoTDB: ON")); else emit updStatus(QString("autotdb"), QString("AutoTDB: OFF"));  /// EFO Added to
        if(Game::writeTDB == false) emit updStatus(QString("autotdb"), QString("WriteTDB: OFF"));   /// EFO Added to
        if(stickPointerToTerrain == true) emit updStatus(QString("stickterr"), QString("StickToTerrain: ON")); else emit updStatus(QString("stickterr"), QString("StickToTerrain: OFF"));  /// EFO Added to
        if(resizeTool == true) emit updStatus(QString("resize"), QString("Resize: ON")); else emit updStatus(QString("resize"), QString("Resize: OFF"));  /// EFO Added to
        if(translateTool == true) emit updStatus(QString("translate"), QString("Translate: ON")); else emit updStatus(QString("translate"), QString("Translate: OFF"));  /// EFO Added to
        if(rotateTool == true) emit updStatus(QString("rotate"), QString("Rotate: ON")); else emit updStatus(QString("rotate"), QString("Rotate: OFF"));  /// EFO Added to

        if(toolEnabled == "placeTool") emit updStatus(QString("place"), QString("Place: ON")); else emit updStatus(QString("place"), QString("Place: OFF"));  /// EFO Added to
        if(toolEnabled == "selectTool") emit updStatus(QString("select"), QString("Select: ON")); else emit updStatus(QString("select"), QString("Select: OFF"));  /// EFO Added to
        if(placeGuardEnabled) emit updStatus(QString("guard"), QString("Place Guard: ON")); else emit updStatus(QString("guard"), QString("Place Guard: OFF"));

        if(toolEnabled == "heightTool") {
            if(defaultPaintBrush->direction == 1) emit updStatus(QString("brushdir"), QString("Terrain Brush: +")); else emit updStatus(QString("brushdir"), QString("Terrain Brush: -"));  /// EFO Added to
        } else {
            emit updStatus(QString("brushdir"), QString("Terrain Brush: OFF"));
        }

        /// Try to capture player cam rotation


        //        if(resizeTool == true)  reloadRefFile updStatus(QString("resize"), QString("Resize: ON")); else emit updStatus(QString("resize"), QString("Resize: OFF"));  /// EFO Added to
        //        emit updStatus(QString("Stat3"), QString(""));
    }

    if (timeNow % 15000 < lastTime % 15000) {

        unsigned long long int worktime = (timeNow - timeSaved)/60000 ;
        emit updStatus(QString("timer"), QString( QString::number(worktime ))); /// EFO Added to
        emit updStatus(QString("camRot"), QString(QString::number(camera->getRotX())));
    }


    if(Game::soundEnabled){
        if (timeNow % 200 < lastTime % 200) {
            SoundManager::UpdateListenerPos((int)camera->pozT[0], (int)camera->pozT[1], camera->getPos(), camera->getTarget(), camera->getUp());
            SoundManager::UpdateAll();
        }

        if (timeNow % 50 < lastTime % 50) {
            SoundManager::UpdateAll();
        }
    }

    route->updateSim(camera->pozT, (float) (timeNow - lastTime) / 1000.0);

    lastTime = timeNow;

    if (Game::allowObjLag < Game::maxObjLag)
        Game::allowObjLag += 2;

    camera->update(fps);

    update();
}

bool RouteEditorGLWidget::initRoute(){
    // Init Shape and Trains libs
    currentShapeLib = new ShapeLib();
    Game::currentShapeLib = currentShapeLib;
    engLib = new EngLib();
    Game::currentEngLib = engLib;
    timeSaved = QDateTime::currentMSecsSinceEpoch();


    // Init Route
    if(Game::serverClient != NULL){
        if(Game::debugOutput) qDebug() << "RouteClient";
        route = new RouteClient();
        QObject::connect(route, SIGNAL(initDone()), this, SLOT(initRoute2()));
        route->load();
        return true;
    } else {
        route = new Route();
        route->load();
        if (!route->loaded){
            return false;
        }


        qDebug() << "Initializing 3D viewer";
        initRoute2();



        return true;
    }
    return false;
}

void RouteEditorGLWidget::initRoute2(){
    QObject::connect(route, SIGNAL(objectSelected(GameObj*)), this, SLOT(objectSelected(GameObj*)));
    QObject::connect(route, SIGNAL(objectSelected(QVector<GameObj*>)), this, SLOT(objectSelected(QVector<GameObj*>)));
    QObject::connect(route, SIGNAL(sendMsg(QString)), this, SLOT(msg(QString)));

    // Init Camera
    cameraInit();

    // Play?
    if(Game::ActivityToPlay.length() > 0){
        playInit();
    }
    emit routeLoaded(route);
    emit showWindow();
    emit preloadTexturesSignal();

    return;
}

void RouteEditorGLWidget::playInit(){
        int actId = ActLib::GetAct(Game::root + "/routes/" + Game::route + "/activities", Game::ActivityToPlay );
        if(Game::debugOutput) qDebug() << "======== actId" << actId << Game::ActivityToPlay;
        if(actId < 0){
            PlayActivitySelectWindow actWindow;
            actWindow.setRoute(route);
            actWindow.exec();
            actId = actWindow.actId;
        }
        if(actId >= 0){
            ActLib::Act[actId]->initToPlay();
            route->activitySelected(ActLib::Act[actId]);
            setSelectedObj((GameObj*)route->getActivityConsist(0));
            camera->setCameraObject((GameObj*)route->getActivityConsist(0));
        }
}

void RouteEditorGLWidget::cameraInit(){
    float * aaa = new float[2] { 0, 0 };
    cameraFree = new CameraFree(aaa);
    //cameraObj = new CameraConsist();
    camera = cameraFree;
    float spos[3];
    if (Game::start == 2) {
        camera->setPozT(Game::startTileX, -Game::startTileY);
    } else {
        camera->setPozT(route->getStartTileX(), -route->getStartTileZ());
        spos[0] = route->getStartpX();
        spos[2] = -route->getStartpZ();
    }
    if (Game::terrainLib->load(route->getStartTileX(), -route->getStartTileZ())) {
        spos[1] = 20 + Game::terrainLib->getHeight(route->getStartTileX(), -route->getStartTileZ(), route->getStartpX(), -route->getStartpZ());
    } else {
        spos[1] = 0;
    }
    camera->setPos((float*) &spos);
    if(Game::restoreLastSessionCamera){
        camera->setPozT(Game::restoreCameraTileX, Game::restoreCameraTileZ);
        camera->setPos(Game::restoreCameraX, Game::restoreCameraY, Game::restoreCameraZ);
        camera->setPlayerRot(Game::restoreCameraRotX, Game::restoreCameraRotY);
        Game::terrainLib->load(Game::restoreCameraTileX, Game::restoreCameraTileZ);
        Game::restoreLastSessionCamera = false;
    }
}

QJsonObject RouteEditorGLWidget::getSessionCameraState() const{
    QJsonObject state;
    if(camera == NULL)
        return state;

    float* pos = camera->getPos();
    state["tileX"] = (int)camera->pozT[0];
    state["tileZ"] = (int)camera->pozT[1];
    state["x"] = pos[0];
    state["y"] = pos[1];
    state["z"] = pos[2];
    state["rotX"] = camera->getRotX();
    state["rotY"] = camera->getRotY();
    return state;
}

void RouteEditorGLWidget::initializeGL() {

    if(Game::soundEnabled)
        SoundManager::InitAl();

    gluu = GLUU::get();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &RouteEditorGLWidget::cleanup);
    if(Game::debugOutput) qDebug() << "# InitializeOpenGLFunctions";

    initializeOpenGLFunctions();

    Game::currentRenderer = new OpenGL3Renderer();

    //funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
    //if (!funcs) {
    //    qWarning() << "Could not obtain required OpenGL context version";
    //    exit(1);
    //}
    //funcs->initializeOpenGLFunctions();/**/
    glClearColor(0, 0, 0, 1);
    //qDebug() << "gluu->initShader();";
    if(Game::debugOutput) qDebug() << "# InitShaders";
    gluu->initShader();
    if(Game::debugOutput) qDebug() << "# InitShaders finished";
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glLineWidth(Game::oglDefaultLineWidth);

    //sFile = new SFile("F:/TrainSim/trains/trainset/pkp_sp47/pkp_sp47-001.s", "F:/TrainSim/trains/trainset/pkp_sp47");
    //sFile = new SFile("f:/train simulator/routes/cmk/shapes/cottage3.s", "cottage3.s", "f:/train simulator/routes/cmk/textures");
    //eng = new Eng("F:/Train Simulator/trains/trainset/PKP-ST44-992/","PKP-ST44-992.eng",0);
    //sFile->Load("f:/train simulator/routes/cmk/shapes/cottage3.s");
    //tile = new Tile(-5303,-14963);
    //qDebug() << "route = new Route();";


    /*PlayActivitySelectWindow *actWindow = new PlayActivitySelectWindow();
    actWindow->setRoute(route);
    qDebug() << "=1a";
    actWindow->exec();
     qDebug() << "=1b";*/

    lastTime = QDateTime::currentMSecsSinceEpoch();


    int timerStep = 15;
    if (Game::fpsLimit > 0)
        timerStep = 1000 / Game::fpsLimit;
    timer.start(timerStep, this);
    setFocus();
    setMouseTracking(true);
    pointer3d = new Pointer3d();
    compass = new GuiGlCompass();
    compassPointer = new OglObj();
    float *punkty = new float[3 * 6];
    int ptr = 0;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0.98;
    punkty[ptr++] = 0;
    punkty[ptr++] = 0;
    punkty[ptr++] = 1.0;
    punkty[ptr++] = 0;
    compassPointer->setLineWidth(2);
    compassPointer->setMaterial(0.0, 0.0, 0.0);
    compassPointer->init(punkty, ptr, RenderItem::V, GL_LINES);
    delete[] punkty;

    //selectedObj = NULL;
    setSelectedObj(NULL);
    groupObj = new GroupObj();
    copyPasteGroupObj = new GroupObj();
    defaultPaintBrush = new Brush();
    mapWindow = new MapWindow();
    Quat::fill(this->placeRot);

    SoundManager::listenerX = camera->pozT[0];
    SoundManager::listenerZ = camera->pozT[1];

    //emit routeLoaded(route);
    emit mkrList(route->getMkrList());

    gluu->makeShadowFramebuffer(FramebufferName1, depthTexture1, Game::shadowMapSize, GL_TEXTURE2);
    gluu->makeShadowFramebuffer(FramebufferName2, depthTexture2, Game::shadowLowMapSize, GL_TEXTURE3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);


    moveStep = Game::DefaultMoveStep;
    moveMaxStep = Game::DefaultMoveStep;
    defaultMoveStep = moveStep;
}

void RouteEditorGLWidget::reloadRefFile(){
    route->loadAddons();
    //route->ref = new Ref((Game::root + "/routes/" + Game::route + "/" + Game::routeName + ".ref"));
    emit refreshObjLists();
}

void RouteEditorGLWidget::reloadMkrFiles(){
    if(Game::debugOutput) qDebug() << "route->loadMkrList;";
    route->loadMkrList();
    if(Game::debugOutput) qDebug() << "REGL->emit mkrList(route->getMkrList())";
    emit mkrList(route->getMkrList());
}

void RouteEditorGLWidget::setCameraObject(GameObj* obj){
    camera->setCameraObject(obj);
}

void RouteEditorGLWidget::setMoveStep(float val){
    moveStep = val;
    moveMaxStep = val;
}

void RouteEditorGLWidget::paintGL(){
    paintGL2();
    return;

    // Here is not finishes, future version of TSRE renderer.
    // Unlike old TSRE renderer, here collect all render items first
    // And then use renderer to render them

    Game::currentShapeLib = currentShapeLib;
    if (route == NULL) return;
    if (!route->loaded) return;

    // Render Shadows
    //if (Game::shadowsEnabled > 0)
    //    renderShadowMaps();

    // Render Scene
    //gluu->currentShader = gluu->shaders["StandardBloom"];
    gluu->currentShader = gluu->shaders["StandardFog"];
    gluu->currentShader->bind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int renderMode = GLUU::RENDER_DEFAULT;
    if (selection)
        renderMode = GLUU::RENDER_SELECTION;

    glClearColor(gluu->skyColor[0], gluu->skyColor[1], gluu->skyColor[2], 1.0);
    glViewport(0, 0, (float) this->width() * Game::PixelRatio, (float) this->height() * Game::PixelRatio);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);

    Mat4::perspective(gluu->fMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->fMatrix, gluu->fMatrix, camera->getMatrix());

    // Render Skydome
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 100.0f, 10000.0f);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, camera->getPos());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, -50, 0);
    Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, 2.0, 0, 1, 0);
    gluu->setMatrixUniforms();
    gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
    //route->skydome->render(gluu, renderMode);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render Low Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 600.0f, Game::distantLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    //gluu->currentShader->setUniformValue(gluu->currentShader->lod, -0.5f);
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, route->getDistantTerrainYOffset(), 0);
    //Game::terrainLib->renderLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    //for(int i = 0; i < route->env->waterCount; i++)
    //    Game::terrainLib->renderWaterLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(Game::currentRenderer->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render High Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    Game::terrainLib->pushRenderItems(camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);

    // Render World
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();

    if (stickPointerToTerrain && Game::viewTerrainShape)

        if (!selection && !Game::playerMode) pushRenderPointer();

    route->pushRenderItems(camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, renderMode);
    //if (!selection)
    //for(int i = 0; i < route->env->waterCount; i++)
    //    Game::terrainLib->renderWater(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);

    //if (!stickPointerToTerrain || !Game::viewTerrainShape)
    //    if (!selection && !Game::playerMode) pushRenderPointer();

    // render compass
    /*if (!selection && Game::viewCompass){
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, 1.0, 1.0 - 2*(float(this->height()) / this->width()), 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);

        compass->pushRenderItem(camera->getRotX()+M_PI);
        compassPointer->pushRenderItem();
    }*/


    // HUD
    /*if(Game::hudEnabled){
        int shadowsState = Game::shadowsEnabled;
        Game::shadowsEnabled = 0;
        float hudScale = Game::hudScale;
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, -1.0+2.0*hudScale, 1.0 - 2*(float(this->height()) / this->width())*hudScale, 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
        camera->renderHud(gluu);
        Game::shadowsEnabled = shadowsState;
        gluu->currentShader->release();
    }*/

    Game::currentRenderer->renderFrame();
    // Handle Selection
    //handleSelection();


    // Set Info
    //if (this->isActiveWindow()) {
    //    emit this->naviInfo(route->getTileObjCount((int) camera->pozT[0], (int) camera->pozT[1]), route->getTileHiddenObjCount((int) camera->pozT[0], (int) camera->pozT[1]));
    //    emit this->posInfo(camera->getCurrentPos());
    //    emit this->pointerInfo(aktPointerPos);
    //}
}

void RouteEditorGLWidget::paintGL2() {
    Game::currentShapeLib = currentShapeLib;
    if (route == NULL) return;
    if (!route->loaded) return;

    // Render Shadows
    if (Game::shadowsEnabled > 0)
       renderShadowMaps();

    // Render Scene
    //gluu->currentShader = gluu->shaders["StandardBloom"];
    gluu->currentShader = gluu->shaders["StandardFog"];
    gluu->currentShader->bind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int renderMode = GLUU::RENDER_DEFAULT;
    if (selection)
        renderMode = GLUU::RENDER_SELECTION;

    glClearColor(gluu->skyColor[0], gluu->skyColor[1], gluu->skyColor[2], 1.0);
    glViewport(0, 0, (float) this->width() * Game::PixelRatio, (float) this->height() * Game::PixelRatio);
    Mat4::identity(gluu->mvMatrix);

    Mat4::perspective(gluu->fMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->fMatrix, gluu->fMatrix, camera->getMatrix());

    // Render Skydome
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 100.0f, 10000.0f);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, camera->getPos());
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, -50, 0);
    Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, 2.0, 0, 1, 0);
    gluu->setMatrixUniforms();
    gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
    route->skydome->render(gluu, renderMode);
    Mat4::identity(gluu->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render Low Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 600.0f, Game::distantLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    //gluu->currentShader->setUniformValue(gluu->currentShader->lod, -0.5f);
    Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, route->getDistantTerrainYOffset(), 0);
    Game::terrainLib->renderLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    for(int i = 0; i < route->env->waterCount; i++)
        Game::terrainLib->renderWaterLo(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);
    Mat4::identity(gluu->mvMatrix);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render High Resolution Terrain
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();
    Game::terrainLib->render(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode);
    //glClear(GL_DEPTH_BUFFER_BIT);
    // Render World
    Mat4::perspective(gluu->pMatrix, Game::cameraFov * M_PI / 180, float(this->width()) / this->height(), 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, camera->getMatrix());
    gluu->setMatrixUniforms();

    if (stickPointerToTerrain && Game::viewTerrainShape)
        if (!selection && !Game::playerMode) drawPointer();

    route->render(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, renderMode);

    //if (!selection)
    for(int i = 0; i < route->env->waterCount; i++)
        Game::terrainLib->renderWater(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3, renderMode, i);

    if (!stickPointerToTerrain || !Game::viewTerrainShape)
        if (!selection && !Game::playerMode) drawPointer();

    // render compass
    if (!selection && Game::viewCompass){
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, 1.0, 1.0 - 2*(float(this->height()) / this->width()), 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);

        compass->render(camera->getRotX()+M_PI);
        compassPointer->render();
    }


    // HUD
    if(Game::hudEnabled){
        int shadowsState = Game::shadowsEnabled;
        Game::shadowsEnabled = 0;
        float hudScale = Game::hudScale;
        Mat4::identity(gluu->mvMatrix);
        Mat4::ortho(gluu->pMatrix, -1.0, -1.0+2.0*hudScale, 1.0 - 2*(float(this->height()) / this->width())*hudScale, 1.0, 0.0, 1.0);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        gluu->currentShader->setUniformValue(gluu->currentShader->lod, 0.0f);
        camera->renderHud(gluu);
        Game::shadowsEnabled = shadowsState;
        gluu->currentShader->release();
    }
    // Handle Selection
    handleSelection();

    // Set Info
    if (this->isActiveWindow()) {
        emit this->naviInfo(route->getTileObjCount((int) camera->pozT[0], (int) camera->pozT[1]), route->getTileHiddenObjCount((int) camera->pozT[0], (int) camera->pozT[1]));
        emit this->posInfo(camera->getCurrentPos());
        emit this->pointerInfo(aktPointerPos);

    }
}

void RouteEditorGLWidget::renderShadowMaps() {
    float* lookAt = Mat4::create();
    float* out1 = Vec3::create();
    Vec3::set(out1, 0, 1, 0);
    float *ld = Vec3::create();
    Vec3::set(ld, -1.0, 1.5, 1.0);
    float *aaa = camera->getPos();
    //float *lt = camera->getTarget();
    //Vec3::sub(lt, lt, aaa);
    //lt[0] = lt[0]*100;
    //lt[1] = 0;
    //lt[2] = lt[2]*100;
    //Vec3::add(aaa, aaa, lt);
    //aaa[2] = -aaa[2];
    //aaa[0] = -aaa[0];
    Vec3::add(ld, ld, aaa);
    Mat4::ortho(gluu->pShadowMatrix, -150, 150, -150, 150, -200, 200);
    Mat4::ortho(gluu->pShadowMatrix2, -700, 700, -700, 700, -700, 700);
    Mat4::lookAt(lookAt, ld, aaa, out1);
    Mat4::multiply(gluu->pShadowMatrix, gluu->pShadowMatrix, lookAt);
    Mat4::multiply(gluu->pShadowMatrix2, gluu->pShadowMatrix2, lookAt);

    gluu->currentShader = gluu->shaders["Shadows"];
    gluu->currentShader->bind();
    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(gluu->objStrMatrix);
    gluu->setMatrixUniforms();
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName1);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Game::shadowMapSize, Game::shadowMapSize);
    int tempLod = Game::objectLod;
    Game::objectLod = 600;
    Game::terrainLib->renderEmpty(gluu, camera->pozT, camera->getPos(), camera->getTarget(), 3.14f / 3);
    route->renderShadowMap(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, selection);

    Mat4::identity(gluu->mvMatrix);
    Mat4::identity(gluu->objStrMatrix);
    float *tmatrix = gluu->pShadowMatrix;
    gluu->pShadowMatrix = gluu->pShadowMatrix2;
    gluu->setMatrixUniforms();
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName2);
    glActiveTexture(GL_TEXTURE0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Game::shadowLowMapSize, Game::shadowLowMapSize);
    Game::objectLod = 1000;
    route->renderShadowMap(gluu, camera->pozT, camera->getPos(), camera->getTarget(), camera->getRotX(), 3.14f / 3, selection);
    gluu->pShadowMatrix2 = gluu->pShadowMatrix;
    gluu->pShadowMatrix = tmatrix;
    Game::objectLod = tempLod;
    gluu->currentShader->release();
}

void RouteEditorGLWidget::handleSelection() {
    if (selection) {
        int x = mousex;
        int y = mousey;

        float winZ[4];

        int* viewport = new int[4];
        float* mvmatrix = new float[16];
        float* projmatrix = new float[16];
        float* wcoord = new float[4];

        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
        glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);
        int realy = viewport[3] - (int) y - 1;
        glReadPixels(x, realy, 1, 1, GL_RGBA, GL_FLOAT, &winZ);

        // if(Game::debugOutput) qDebug() << "REGLW676:" << winZ[0] << " " << winZ[1] << " " << winZ[2] << " " << winZ[3];
        int colorHash = (int) (winZ[0]*255)*256 * 256 + (int) (winZ[1]*255)*256 + (int) (winZ[2]*255);
        // if(Game::debugOutput) qDebug() << "REGLW678:" << colorHash;
        int ww = (colorHash >> 20) & 0xF;
        // if(Game::debugOutput) qDebug() << "REGLW680:" << "ww"<< ww;

        // WorldObj Selected
        if(ww == 0){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    route->addToTDBIfNotExist((WorldObj*)selectedObj); if(Game::debugOutput) qDebug() << "REGLW 687";
                setSelectedObj(NULL);
            }
        } else if( ww >= 1 && ww <= 9 ){
            int UiD = (colorHash >> 4) & 0xFFFF;
            //if(UiD >= 50000)
            //    UiD += 50000;
            int cdata = colorHash & 0xF;
            int wx = 0;
            int wz = 0;
            if (ww == 1 || ww == 2 || ww == 3) wx = camera->pozT[0] - 1;
            if (ww == 4 || ww == 5 || ww == 6) wx = camera->pozT[0];
            if (ww == 7 || ww == 8 || ww == 9) wx = camera->pozT[0] + 1;
            if (ww == 1 || ww == 4 || ww == 7) wz = camera->pozT[1] - 1;
            if (ww == 2 || ww == 5 || ww == 8) wz = camera->pozT[1];
            if (ww == 3 || ww == 6 || ww == 9) wz = camera->pozT[1] + 1;
            // if(Game::debugOutput) qDebug() << "REGLW703:" << "color data: " << cdata;
            // if(Game::debugOutput) qDebug() << "REGLW704:" << wx << " " << wz << " " << UiD;
            WorldObj *selectedWorldObj = (WorldObj*) selectedObj;
            if (keyControlEnabled) {
                if (selectedWorldObj == NULL){
                    setSelectedObj(groupObj);
                    selectedWorldObj = (WorldObj*) selectedObj;
                } else if (selectedWorldObj->typeObj != GameObj::worldobj){
                    selectedWorldObj->unselect();
                    setSelectedObj(groupObj);
                } else if (selectedWorldObj->typeObj == GameObj::worldobj) {
                    groupObj->addObject(selectedWorldObj);
                    setSelectedObj(groupObj);
                }
                groupObj->addObject(route->getObj(wx, wz, UiD));
                if (groupObj->count() == 0) {
                    if(Game::debugOutput) qDebug() << "brak obiektu";
                    groupObj->unselect();
                    setSelectedObj(NULL);
                }
            } else {
                WorldObj* twobj = route->getObj(wx, wz, UiD);
                if (selectedWorldObj != NULL && twobj != selectedWorldObj) {
                    selectedWorldObj->unselect();
                    if (autoAddToTDB) {
                        route->addToTDBIfNotExist(selectedWorldObj); if(Game::debugOutput) qDebug() << "REGLW 728";
                    }
                }
                lastSelectedObj = selectedObj;
                setSelectedObj(twobj);
                if (selectedObj == NULL) {
                    if(Game::debugOutput) qDebug() << "brak obiektu";
                } else {
                    selectedObj->select(cdata);
                }
            }
        } else if( ww == 10 ){
            int wx = camera->pozT[0] - 1 + ((colorHash >> 10) & 0x3);
            int wz = camera->pozT[1] - 1 + ((colorHash >> 8) & 0x3);
            int UiD = (colorHash) & 0xFF;
            // if(Game::debugOutput) qDebug() << "REGLW743:" << wx << wz << UiD;
            if (selectedObj != NULL) {
                if ((keyControlEnabled || keyShiftEnabled) && selectedObj->typeObj == GameObj::terrainobj ) {
                    Terrain * tt = (Terrain*) selectedObj;
                    if(!tt->isXYinside(wx, wz)){// >mojex != wx || tt->mojez != wz){
                        selectedObj->unselect();
                        setSelectedObj(NULL);
                    }
                } else {
                    selectedObj->unselect();
                    if (autoAddToTDB)
                        route->addToTDBIfNotExist((WorldObj*)selectedObj); // if(Game::debugOutput) qDebug() << "REGLW 754";
                    setSelectedObj(NULL);
                }
            }
            Terrain *t = Game::terrainLib->getTerrainByXY(wx, wz);
            if (t == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                t->select(UiD, keyControlEnabled);
            }
            setSelectedObj((GameObj*)t);
        } else if( ww == 11 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    route->addToTDBIfNotExist((WorldObj*)selectedObj); if(Game::debugOutput) qDebug() << "REGLW 769";
                setSelectedObj(NULL);
            }
            int CID = ((colorHash) >> 8) & 0xFFF;
            int EID = ((colorHash)) & 0xFF;
            if(Game::debugOutput) qDebug() << "REGLW 774:"  << CID << EID;
            setSelectedObj((GameObj*)route->getActivityObject(CID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                //qDebug() << "eid"<<EID;
                selectedObj->select(EID);
                setSelectedObj(selectedObj);
            }
        } else if( ww == 12 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    route->addToTDBIfNotExist((WorldObj*)selectedObj); if(Game::debugOutput) qDebug() << "REGLW 787";
                setSelectedObj(NULL);
            }
            int TID = ((colorHash) >> 19) & 0x1;
            int UID = ((colorHash)) & 0xFFFF;
            if(Game::debugOutput) qDebug() << "REGL 792:" << TID << UID;
            setSelectedObj((GameObj*)route->getTrackItem(TID, UID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                selectedObj->select();
            }
        } else if( ww == 13 ){
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    route->addToTDBIfNotExist((WorldObj*)selectedObj); if(Game::debugOutput) qDebug() << "REGLW 803";
                setSelectedObj(NULL);
            }
            int CID = ((colorHash) >> 8) & 0xFFF;
            int EID = ((colorHash)) & 0xFF;
            if(Game::debugOutput) qDebug() << "REGLW808:" << CID << EID;
            setSelectedObj((GameObj*)route->getActivityConsist(CID));
            if (selectedObj == NULL) {
                if(Game::debugOutput) qDebug() << "brak obiektu";
            } else {
                //qDebug() << "eid"<<EID;
                selectedObj->select(EID);
                setSelectedObj(selectedObj);
            }
        } else {
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    route->addToTDBIfNotExist((WorldObj*)selectedObj); if(Game::debugOutput) qDebug() << "REGLW 821";
                setSelectedObj(NULL);
            }
        }

        //qDebug() << "selection" << selection;
        selection = false;// !selection;
        paintGL();
    }
}

void RouteEditorGLWidget::pushRenderPointer() {

    int x = mousex;
    int y = mousey;

    static unsigned long long int oldTime = 0;
    unsigned long long int newTime = QDateTime::currentMSecsSinceEpoch();
    static float winZ[4];
    int viewport[4];
    //float wcoord[4];


    glGetIntegerv(GL_VIEWPORT, viewport);
    //glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
    //glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);
    int realy = viewport[3] - (int) y - 1;
    if(newTime - oldTime > 200){
        glReadPixels(x, realy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        oldTime = newTime;
    }
    GLH::glhUnProjectf((float) x, (float) realy, winZ[0], //
            gluu->mvMatrix,
            gluu->pMatrix,
            viewport,
            aktPointerPos);
    return;
    //qDebug()<<aktPointerPos[0]<< aktPointerPos[1]<< aktPointerPos[2];
    /*if (Game::viewPointer3d) {
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        pointer3d->render();
        gluu->mvPopMatrix();
    }*/
}

void RouteEditorGLWidget::drawPointer() {
    int x = mousex;
    int y = mousey;

    static unsigned long long int oldTime = 0;
    unsigned long long int newTime = QDateTime::currentMSecsSinceEpoch();
    static float winZ[4];
    int viewport[4];
    //float wcoord[4];



    glGetIntegerv(GL_VIEWPORT, viewport);
    //glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
    //glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);
    int realy = viewport[3] - (int) y - 1;
    if(newTime - oldTime > 50){
        glReadPixels(x, realy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        oldTime = newTime;
    }
    GLH::glhUnProjectf((float) x, (float) realy, winZ[0], //
            gluu->mvMatrix,
            gluu->pMatrix,
            viewport,
            aktPointerPos);
    //qDebug()<<aktPointerPos[0]<< aktPointerPos[1]<< aktPointerPos[2];
    if (Game::viewPointer3d) {
        gluu->mvPushMatrix();
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, aktPointerPos[0], aktPointerPos[1], aktPointerPos[2]);
        Mat4::identity(gluu->objStrMatrix);
        gluu->setMatrixUniforms();
        //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
        pointer3d->render();
        gluu->mvPopMatrix();

        if(Game::serverClient != NULL){
            foreach(ClientInfo *info, Game::serverClient->clientUsersList){
                if(info == NULL)
                    continue;
                if(info->username == Game::serverClient->username)
                    continue;
                gluu->mvPushMatrix();
                //qDebug() << camera->pozT[0] << info->X;
                Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 2048*(info->X-camera->pozT[0])+info->x, info->y, 2048*(info->Z-camera->pozT[1])+info->z);
                Mat4::identity(gluu->objStrMatrix);
                gluu->setMatrixUniforms();
                //gluu->m_program->setUniformValue(gluu->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
                info->render(camera->getRotX());
                gluu->mvPopMatrix();
            }
        }
    }
}

bool RouteEditorGLWidget::pointerNearPlacementDb(Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ) {
    if (item == NULL || pointerPos == NULL)
        return true;

    if (WorldObj::isTrackObj(item->type) == 0)
        return true;

    float sample[3];
    Vec3::copy(sample, pointerPos);
    float dbHeight = 0;
    return route->findNearestDbHeight(pointerTileX, pointerTileZ, sample, 3.0f, dbHeight);
}

bool RouteEditorGLWidget::validatePlacement(WorldObj* obj, Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ) {
    if (!placeGuardEnabled || obj == NULL || camera == NULL)
        return true;

    if (qAbs(obj->x - (int)camera->pozT[0]) > 1 || qAbs(obj->y - (int)camera->pozT[1]) > 1)
        return false;

    if (!pointerNearPlacementDb(item, pointerPos, pointerTileX, pointerTileZ))
        return false;

    if (item != NULL && WorldObj::isTrackObj(item->type) != 0) {
        float sample[3];
        Vec3::copy(sample, obj->position);
        float dbHeight = 0;
        if (!route->findNearestDbHeight(obj->x, obj->y, sample, 10.0f, dbHeight))
            return false;
        return qAbs(obj->position[1] - dbHeight) <= 10.0f;
    }

    Terrain *terrain = Game::terrainLib->getTerrainByXY(obj->x, obj->y, false);
    if (terrain == NULL || !terrain->loaded)
        return false;

    float ground = Game::terrainLib->getHeight(obj->x, obj->y, obj->position[0], obj->position[2]);
    float delta = obj->position[1] - ground;

    if (obj->typeID == WorldObj::trackobj || obj->typeID == WorldObj::dyntrack)
        return delta <= 100.0f && delta >= -50.0f;

    return delta <= 1.0f && delta >= -1.0f;
}

void RouteEditorGLWidget::rejectPlacement() {
    if (selectedObj != NULL)
        selectedObj->unselect();

    setSelectedObj(NULL);
    Undo::StateEnd();
    Undo::UndoLast();
    mouseLPressed = false;
    showPlacementGuardError();
}

void RouteEditorGLWidget::playPlacementSound(QString fileName) {
    if(!Game::scoSoundEnabled)
        return;
#ifdef Q_OS_WIN
    QString soundPath = QCoreApplication::applicationDirPath() + "/content/" + fileName;
    if (QFile::exists(soundPath)) {
        ::PlaySoundW(NULL, NULL, 0);
        ::PlaySoundW(reinterpret_cast<const wchar_t*>(soundPath.utf16()), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
#else
    Q_UNUSED(fileName);
#endif
}

void RouteEditorGLWidget::showPlacementSuccess() {
    playPlacementSound("SCOclick.wav");
}

void RouteEditorGLWidget::showModeChange() {
    playPlacementSound("SCOpress.wav");
}

void RouteEditorGLWidget::userModeChangeSound() {
    showModeChange();
}

void RouteEditorGLWidget::showPlacementGuardError() {
    playPlacementSound("SCObuzz.wav");
    emit updStatus(QString("guarderror"), QString("ERROR"));
}

void RouteEditorGLWidget::flexResult(bool success) {
    if(success) {
        // Flex changes the generated sections, so rebuild this object's TDB
        // entry before reporting success. Normal track joins use this same
        // database path; leaving the old Dyntrack section cached produces
        // endpoint gaps and duplicate-looking TDB markers.
        if(route != NULL && selectedObj != NULL &&
           selectedObj->typeObj == GameObj::worldobj &&
           ((WorldObj*)selectedObj)->type == "dyntrack") {
            WorldObj* dyntrack = (WorldObj*)selectedObj;
            route->removeTrackFromTDB(dyntrack);
            route->addToTDB(dyntrack);
        }
        showPlacementSuccess();
    } else {
        showPlacementGuardError();
    }
}

void RouteEditorGLWidget::focusEditor() {
    setFocus();
}

void RouteEditorGLWidget::resizeGL(int w, int h) {
    //gluu->m_proj.setToIdentity();
    //gluu->m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void RouteEditorGLWidget::keyPressEvent(QKeyEvent * event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;

    if(event->key() == Qt::Key_A
            && event->modifiers().testFlag(Qt::AltModifier)){
        selectAllTerrainPatchesOnSelectedTile();
        event->accept();
        return;
    }

    if(event->modifiers() == Qt::NoModifier){
        if(event->isAutoRepeat()){
            switch(event->key()){
                case Qt::Key_E:
                case Qt::Key_Q:
                case Qt::Key_R:
                case Qt::Key_T:
                case Qt::Key_Y:
                    event->accept();
                    return;
                default:
                    break;
            }
        }

        switch(event->key()){
            case Qt::Key_E:
                statusPanelCommand("select");
                event->accept();
                return;
            case Qt::Key_Q:
                statusPanelCommand("place");
                event->accept();
                return;
            case Qt::Key_R:
                statusPanelCommand("rotate");
                event->accept();
                return;
            case Qt::Key_T:
                statusPanelCommand("translate");
                event->accept();
                return;
            case Qt::Key_Y:
                statusPanelCommand("resize");
                event->accept();
                return;
            default:
                break;
        }
    }

    camera->keyDown(event);

    Undo::StateBeginIfNotExist();

    // EFO Key events

    switch (event->key()) {
        case Qt::Key_Control:
            moveStep = moveMaxStep / 10.0;
            keyControlEnabled = true;
            break;
        case Qt::Key_Shift:
            keyShiftEnabled = true;
            break;
        case Qt::Key_Alt:
            moveStep = moveMaxStep * 10.0;
            keyAltEnabled = true;
            break;
            //case Qt::Key_N:
            //if(selectedObj != NULL)
            //    route->deleteTDBTree(selectedObj);
            /*if(this->selectedObj != NULL){
                this->selectedObj->unselect();
                this->selectedObj = NULL;
            }
            this->selectedObj = route->makeFlexTrack((int)camera->pozT[0], (int)camera->pozT[1], aktPointerPos);
            if(this->selectedObj != NULL){
                this->selectedObj->select();
                lastNewObjPosT[0] = this->selectedObj->x;
                lastNewObjPosT[1] = this->selectedObj->y;
                lastNewObjPos[0] = this->selectedObj->position[0];
                lastNewObjPos[1] = this->selectedObj->position[1];
                lastNewObjPos[2] = this->selectedObj->position[2];
            }*/
            //    break;
        case Qt::Key_B:
        {
            QMessageBox msgBox;
            msgBox.setText("Create new Tile here?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if (msgBox.exec() == QMessageBox::Yes){
                // New Tile != New Terrain. Need fix for distant terrain!
                int out = 0;
                out = route->newTile((int) camera->pozT[0], (int) camera->pozT[1]);
                if(out == 1){
                    msgBox.setText("Tile exist. Override?");
                    if (msgBox.exec() == QMessageBox::Yes)
                        route->newTile((int) camera->pozT[0], (int) camera->pozT[1], true);
                }
            }
        }
            break;
        /// EFO Added to
        case Qt::Key_Escape:
            resizeTool = false;
            translateTool = false;
            rotateTool = false;
            showModeChange();

            break;

        /// EFO Added to
        case Qt::Key_E:
            break;

        case Qt::Key_R:
            break;
        case Qt::Key_T:
            break;
        case Qt::Key_Y:
            break;
        case Qt::Key_Q:
            if (keyControlEnabled)
            {
                autoAddToTDB = !autoAddToTDB;
                showModeChange();
                break;
            }
            else if (keyShiftEnabled)
            {
                stickPointerToTerrain = !stickPointerToTerrain;
                showModeChange();
                break;
            }
            else
            {
            }
            break;
        case Qt::Key_Home:
            aktPointerPos[1] += 40;
            jumpTo(camera->pozT, aktPointerPos);
            aktPointerPos[1] -= 40;
            break;
        default:
            break;
    }
    if (toolEnabled == "heightTool" || toolEnabled == "waterTerrTool" || toolEnabled == "gapsTerrainTool") {
        switch (event->key()) {
            case Qt::Key_Z:
                if (!keyControlEnabled) {
                    this->defaultPaintBrush->direction = -this->defaultPaintBrush->direction;
                    if (this->defaultPaintBrush->direction == 1)
                    {
                        emit sendMsg(QString("brushDirection"), QString("+"));
                    }
                    else
                    {
                        emit sendMsg(QString("brushDirection"), QString("-"));
                    }
                }
                break;
            default:
                break;
        }
    }
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        Vector2f a;

        switch (event->key()) {
            case Qt::Key_Up:
                if (Game::usenNumPad)
                    break;
            case Qt::Key_8:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(moveStep, 0, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(moveStep / 10, 0, 0);
                } else if (selectedObj != NULL) {
                    a.y = moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Down:
                if (Game::usenNumPad)
                    break;
            case Qt::Key_2:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(-moveStep, 0, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(-moveStep / 10, 0, 0);
                } else if (selectedObj != NULL) {
                    a.y = -moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Left:
                if (Game::usenNumPad)
                    break;
            case Qt::Key_4:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, moveStep, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, -moveStep / 10, 0);
                } else if (selectedObj != NULL) {
                    a.x = moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_Right:
                if (Game::usenNumPad)
                    break;
            case Qt::Key_6:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, -moveStep, 0);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, moveStep / 10, 0);
                } else if (selectedObj != NULL) {
                    a.x = -moveStep;
                    a.rotate(-camera->getRotX(), 0);
                    selectedObj->translate(a.x, 0, a.y);
                }
                break;
            case Qt::Key_PageUp:
                //Game::cameraFov += 1;
                //qDebug() << Game::cameraFov;
            case Qt::Key_9:
                Undo::PushGameObjData(selectedObj);
                if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, 0, moveStep);
                } else if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, 0, moveStep / 10);
                } else if (selectedObj != NULL) {
                    selectedObj->translate(0, moveStep, 0);
                }
                break;
            case Qt::Key_PageDown:
                //Game::cameraFov -= 1;
                //qDebug() << Game::cameraFov;
            case Qt::Key_3:
            case Qt::Key_7:
                Undo::PushGameObjData(selectedObj);
                if (rotateTool && selectedObj != NULL) {
                    selectedObj->rotate(0, 0, -moveStep / 10);
                } else if (resizeTool && selectedObj != NULL) {
                    selectedObj->resize(0, 0, -moveStep);
                } else if (selectedObj != NULL) {
                    selectedObj->translate(0, -moveStep, 0);
                }
                break;
            case Qt::Key_F:
                if(event->modifiers() & Qt::ControlModifier)
                    setTerrainToSelectedObjTile();
                else if(event->modifiers() & Qt::ShiftModifier)
                    smoothTerrainToObj();
                else
                    setTerrainToObj();
                break;
            case Qt::Key_H:
                adjustObjPositionToTerrainMenu();
                break;
            case Qt::Key_N:
                adjustObjRotationToTerrainMenu();
                break;
            case Qt::Key_Delete:
                if (selectedObj != NULL) {
                    if(selectedObj->typeObj == GameObj::worldobj){
                        route->deleteObj((WorldObj*)selectedObj);
                        selectedObj->unselect();
                    }
                    if(selectedObj->typeObj == GameObj::tritemobj){
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Remove Track Item?");
                        msgBox.setStyleSheet("QLabel{min-width: 300px;}");
                        msgBox.setText("Warning!");
                        msgBox.setInformativeText("Do you want to remove this track item? It will damage your route if you don't know what you are doing!");
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                        msgBox.setDefaultButton(QMessageBox::No);
                        if(msgBox.exec() == QMessageBox::Yes){
                            route->deleteTrackItem((TRitem*)selectedObj);
                            selectedObj->unselect();
                        }
                    }
                    if(selectedObj->typeObj == GameObj::activityobj){
                        selectedObj->remove();
                        emit sendMsg("refreshActivityTools");
                    }
                    setSelectedObj(NULL);
                    lastSelectedObj = NULL;
                }
                break;
            case Qt::Key_C:
                if (selectedObj != NULL) {
                    selectedObj->unselect();
                    if(selectedObj->typeObj == GameObj::worldobj){
                        setSelectedObj(route->placeObject(((WorldObj*)selectedObj)->x, ((WorldObj*)selectedObj)->y, ((WorldObj*)selectedObj)->position, ((WorldObj*)selectedObj)->qDirection, 0, ((WorldObj*)selectedObj)->getRefInfo()));
                        if (selectedObj != NULL) {
                            selectedObj->select();
                        }
                    }
                }
                break;
            case Qt::Key_P:
                if (keyControlEnabled)
                    pickObjForPlacement();
                else if(keyShiftEnabled)
                    pickObjRotElevForPlacement();
                else
                    pickObjRotForPlacement();
                break;
            case Qt::Key_Z:
                //route->refreshObj(selectedWorldObj);
                //route->trackDB->setDefaultEnd(0);
                //route->addToTDB(selectedWorldObj, (float*)&lastNewObjPosT, (float*)&selectedWorldObj->position);
                Undo::StateBegin();
                Undo::PushTrackDB(Game::trackDB, false);
                Undo::PushTrackDB(Game::roadDB, true);
                route->toggleToTDB((WorldObj*)selectedObj);
                Undo::StateEnd();
                // EFO keep object selected
                // next three lines commented out
                 if (selectedObj != NULL) selectedObj->unselect();
                 lastSelectedObj = selectedObj;
                 setSelectedObj(NULL);
                break;
            case Qt::Key_X:
                if (selectedObj == NULL)
                    return;
                if (selectedObj->typeObj != WorldObj::worldobj)
                    return;
                //route->refreshObj(selectedWorldObj);
                route->flipObject((WorldObj*)selectedObj);
                if(placeElev != 0)
                    selectedObj->rotate(placeElev, 0, 0);
                //selectToolresetRot();
                break;
            default:
                break;
        }
    }
}

void RouteEditorGLWidget::keyReleaseEvent(QKeyEvent * event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    camera->keyUp(event);
    switch (event->key()) {
            //case Qt::Key_Alt:
        case Qt::Key_Control:
            moveStep = moveMaxStep;
            keyControlEnabled = false;
            break;
        case Qt::Key_Shift:
            keyShiftEnabled = false;
            break;
        case Qt::Key_Alt:
            moveStep = moveMaxStep;
            keyAltEnabled = false;
            break;
        default:
            break;
    }
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        switch (event->key()) {
            default:
                break;
        }
    }
}

void RouteEditorGLWidget::mousePressEvent(QMouseEvent *event) {
    Game::currentShapeLib = currentShapeLib;
    bolckContextMenu = false;
    if (!route->loaded) return;
    m_lastPos = event->pos();
    m_lastPos *= Game::PixelRatio;
    mousex = m_lastPos.x();
    mousey = m_lastPos.y();
    mouseClick = true;  // if(Game::debugOutput) qDebug() << "REGLW 1260";
    if ((event->button()) == Qt::RightButton) {
        mouseRPressed = true;
        camera->MouseDown(event);
    }
    if ((event->button()) == Qt::LeftButton) {
        Undo::StateBegin();
        mouseLPressed = true;
        lastMousePressTime = QDateTime::currentMSecsSinceEpoch();

        /// placing an item onto the world EFO

        if (toolEnabled == "placeTool") {

        //QString selectedFilename = selectedObj->objectName();
        //qDebug() << "placed objName: " << selectedFilename;


         // if(Game::debugOutput) qDebug() << "REGLW 1278 placed: " ;

            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    if (selectedObj->typeObj == GameObj::worldobj)
                        route->addToTDBIfNotExist((WorldObj*) selectedObj);  // if(Game::debugOutput) qDebug() << "REGLW 1284";
            }
            Undo::StateBeginIfNotExist();

            if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
             lastNewObjPos[2] = aktPointerPos[2];
            float *q = Quat::create();
            Quat::copy(q, this->placeRot);
            setSelectedObj(route->placeObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, q, placeElev)); if(Game::debugOutput) qDebug() << "REGLW 1294";
            if (selectedObj == NULL)
                showPlacementGuardError();
            if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, route->ref->selected, placementPointer, placementTileX, placementTileZ)) {
                rejectPlacement();
                return;
            }
            if (selectedObj != NULL) {
                showPlacementSuccess();
                selectedObj->select();
            }
        }
        if (toolEnabled == "autoPlaceSimpleTool") {
            if (selectedObj != NULL) {
                selectedObj->unselect();
                if (autoAddToTDB)
                    if (selectedObj->typeObj == GameObj::worldobj)
                        route->addToTDBIfNotExist((WorldObj*) selectedObj);
            }
            Undo::StateBeginIfNotExist();
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
            lastNewObjPos[2] = aktPointerPos[2];
            int mode = 0;
            if (keyControlEnabled)
                mode = 1;
            if (keyShiftEnabled)
                mode = 2;
            setSelectedObj(route->autoPlaceObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, mode));
            if (selectedObj == NULL)
                showPlacementGuardError();
            if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, route->ref->selected, placementPointer, placementTileX, placementTileZ)) {
                rejectPlacement();
                return;
            }
            if (selectedObj != NULL) {
                showPlacementSuccess();
                selectedObj->select();
            }
        }
        if (toolEnabled == "selectTool") {
            if (!translateTool && !rotateTool && !resizeTool)
                selection = true;
            if (selectedObj != NULL) {
                mouseLPressed = true;
                if (translateTool) {
                    if (selectedObj->typeObj == GameObj::worldobj) {
                        Undo::PushGameObjData((WorldObj*) selectedObj);
                        float tempPos[3];
                        int tx = camera->pozT[0];
                        int tz = camera->pozT[1];
                        route->getPointerPosition(tempPos, tx, tz, aktPointerPos);
                        ((WorldObj*) selectedObj)->setPosition(tx, tz, tempPos);
                        ((WorldObj*) selectedObj)->setMartix();
                    }
                }
                if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "signalLinkTool") {
            Undo::PushGameObjData((WorldObj*) selectedObj);
            Undo::PushTrackDB(Game::trackDB);
            route->linkSignal((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, (WorldObj*) selectedObj);
            enableTool("");
        }
        if (toolEnabled == "FlexTool") {
            emit flexData((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "heightTool") {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            route->paintHeightMap(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled.startsWith("paintTool")) {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            if (keyControlEnabled)
                route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 0);
            else if (keyShiftEnabled)
                route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, NULL);
            else
                Game::terrainLib->paintTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "pickTerrainTexTool") {
            // qDebug() << aktPointerPos[0] << " " << aktPointerPos[2];
            int textureId = Game::terrainLib->getTexture((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            emit setBrushTextureId(textureId);
        }
        if (toolEnabled == "putTerrainTexTool") {
            Game::terrainLib->setTerrainTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            lastPointerPos[0] = aktPointerPos[0];
            lastPointerPos[1] = aktPointerPos[1];
            lastPointerPos[2] = aktPointerPos[2];
        }
        if (toolEnabled == "waterTerrTool") {
            Game::terrainLib->toggleWaterDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
        }
        if (toolEnabled == "drawTerrTool") {
            Game::terrainLib->toggleDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "waterHeightTileTool") {
            Game::terrainLib->setWaterLevelGui((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "fixedTileTool") {
            Game::terrainLib->setFixedTileHeight(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "mapTileShowTool") {
            Game::terrainLib->setTileBlob((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "mapTileLoadTool") {
            int x = (int) camera->pozT[0];
            int z = (int) camera->pozT[1];
            float posx = aktPointerPos[0];
            float posz = aktPointerPos[2];
            Game::check_coords(x, z, posx, posz);
            Terrain *t = Game::terrainLib->getTerrainByXY(x, z);
            if(t == NULL)
                return;
            if(!t->loaded)
                return;
            t->getLowCornerTileXY(mapWindow->tileX, mapWindow->tileZ);
            mapWindow->tileSize = t->getSampleCount()*t->getSampleSize();
            mapWindow->exec();
        }
        if (toolEnabled == "heightTileLoadTool") {
            Game::terrainLib->setHeightFromGeoGui((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "lockTexTool") {
            Game::terrainLib->lockTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "gapsTerrainTool") {
            Game::terrainLib->toggleGaps((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
        }
        if (toolEnabled == "makeTileTextureTool") {
            Game::terrainLib->makeTextureFromMap((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "removeTileTextureTool") {
            Game::terrainLib->removeTileTextureFromMap((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
        }
        if (toolEnabled == "actNewLooseConsistTool") {
            route->actNewLooseConsist((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            emit sendMsg("refreshActivityTools");
        }
        if (toolEnabled == "actNewSpeedZoneTool") {
            route->actNewNewSpeedZone((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            emit sendMsg("refreshActivityTools");
        }
        if (toolEnabled == "pickNewEventLocationTool") {
            route->actPickNewEventLocation((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            enableTool("");
        }
        if (toolEnabled == "") {
            camera->MouseDown(event);
        }
    }
    setFocus();
}

void RouteEditorGLWidget::wheelEvent(QWheelEvent *event) {
    float numDegrees = 0.01 * event->delta();

    if (event->orientation() == Qt::Vertical) {
        if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
            /// Move the selected object up or down
            if (selectedObj != NULL) {
                if (selectedObj->typeObj == GameObj::worldobj) {
                    Undo::StateBeginIfNotExist();
                    Undo::PushGameObjData(selectedObj);
                    ((WorldObj*) selectedObj)->translate(0, numDegrees*moveStep, 0);
                }
                else
                {
                /// EFO Move the camera forward or backward if no object selected
                if(numDegrees != 0)  { camera->moveForward( (numDegrees*10) ); }
                }
            }
        } else {
            /// EFO Move the camera forward or backward if in free view
            if(numDegrees != 0)  { camera->moveForward( (numDegrees*10) ); }
        }
    } else {

    }
    event->accept();
    //qDebug() << "scrollwheel: " << numDegrees;
}

void RouteEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    camera->MouseUp(event);
    if ((event->button()) == Qt::RightButton) {
        mouseRPressed = false;
        if(mouseClick && !bolckContextMenu)
            showContextMenu(event->pos());
    }
    if ((event->button()) == Qt::LeftButton) {
        mouseLPressed = false;
        Undo::StateEnd();
    }
    mouseClick = false;
    bolckContextMenu = false;
}

void RouteEditorGLWidget::mouseMoveEvent(QMouseEvent *event) {
    mouseClick = false;
    bolckContextMenu = false;
    Game::currentShapeLib = currentShapeLib;
    if (!route->loaded) return;
    /*int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {

    } else if (event->buttons() & Qt::RightButton) {

    }*/
    mousex = event->x() * Game::PixelRatio;
    mousey = event->y() * Game::PixelRatio;

    if ((event->buttons() & 2) == Qt::RightButton) {
        camera->MouseMove(event);
    }
    if ((event->buttons() & 1) == Qt::LeftButton) {
        if (toolEnabled.startsWith("paintTool") && mouseLPressed == true) {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                Game::terrainLib->paintTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            }
        }
        if (toolEnabled == "heightTool" && mouseLPressed == true) {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                route->paintHeightMap(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
            }
        }
        if (toolEnabled == "waterTerrTool") {
            if (mousex != m_lastPos.x() || mousey != m_lastPos.y()) {
                Game::terrainLib->toggleWaterDraw((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush->direction);
            }
        }
        if (toolEnabled == "putTerrainTexTool" && mouseLPressed == true) { if(Game::debugOutput) qDebug() << "Put: " << defaultPaintBrush->tex->pathid;
            if (fabs(lastPointerPos[0] - aktPointerPos[0]) > 32 || fabs(lastPointerPos[2] - aktPointerPos[2]) > 32) {
                Game::terrainLib->setTerrainTexture(defaultPaintBrush, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "selectTool") {
            if (selectedObj != NULL && mouseLPressed) {
                if (!translateTool && !rotateTool && !resizeTool) {
                long long int ntime = QDateTime::currentMSecsSinceEpoch();
                if (ntime - lastMousePressTime > 200) {
                        if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                        Undo::PushGameObjData(selectedObj);
                        if (keyShiftEnabled) {
                            float val = mousex - m_lastPos.x();
                            selectedObj->rotate(0, val * moveStep * 0.1, 0);
                        } else {
                            if(selectedObj->typeObj == GameObj::worldobj)
                                route->dragWorldObject((WorldObj*)selectedObj, camera->pozT[0], camera->pozT[1], aktPointerPos);
                            if(selectedObj->typeObj == GameObj::activityobj)
                                selectedObj->setPosition((int)camera->pozT[0], (int)camera->pozT[1], aktPointerPos);
                        }
                    }
                }
                if (translateTool) {
                    Undo::PushGameObjData(selectedObj);
                    selectedObj->setPosition(camera->pozT[0], camera->pozT[1], aktPointerPos);
                    selectedObj->setMartix();
                }
                if (rotateTool) {
                    Undo::PushGameObjData(selectedObj);
                    float val = mousex - m_lastPos.x();
                    selectedObj->rotate(0, val * moveStep * 0.1, 0);
                }
                lastPointerPos[0] = aktPointerPos[0];
                lastPointerPos[1] = aktPointerPos[1];
                lastPointerPos[2] = aktPointerPos[2];
            }
        }
        if (toolEnabled == "placeTool") {
            if (selectedObj != NULL && mouseLPressed) {
                long long int ntime = QDateTime::currentMSecsSinceEpoch();
                if (ntime - lastMousePressTime > 200) {
                    if(Game::deepUnderground) aktPointerPos[1] = std::max(aktPointerPos[1],Game::deepUnderground);
                    Undo::PushGameObjData(selectedObj);
                    if (keyShiftEnabled) {
                        float val = mousex - m_lastPos.x();
                        selectedObj->rotate(0, val * moveStep * 0.1, 0);
                    } else {
                        route->dragWorldObject((WorldObj*)selectedObj, camera->pozT[0], camera->pozT[1], aktPointerPos);
                    }
                }
            }
        }
        if (toolEnabled == "") {
            camera->MouseMove(event);
        }
    }
    m_lastPos = event->pos();
    m_lastPos *= Game::PixelRatio;
}

void RouteEditorGLWidget::enableTool(QString name) {
    if(Game::debugOutput) qDebug() << name;
    toolEnabled = name;
    //if(toolEnabled == "placeTool" || toolEnabled == "selectTool" || toolEnabled == "autoPlaceSimpleTool"){
    resizeTool = false;
    translateTool = false;
    rotateTool = false;
    //}
    emit sendMsg("toolEnabled", name);
}

void RouteEditorGLWidget::statusPanelCommand(QString name) {
    if(name == "select"){
        if(toolEnabled == "selectTool" && !resizeTool && !rotateTool && !translateTool){
            enableTool("");
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolSelect();
        showModeChange();
        return;
    }
    if(name == "place"){
        if(toolEnabled == "placeTool"){
            enableTool("");
            showModeChange();
            return;
        }
        enableTool("placeTool");
        showModeChange();
        return;
    }
    if(name == "rotate"){
        if(toolEnabled == "selectTool" && rotateTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolRotate();
        showModeChange();
        return;
    }
    if(name == "translate"){
        if(toolEnabled == "selectTool" && translateTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolTranslate();
        showModeChange();
        return;
    }
    if(name == "resize"){
        if(toolEnabled == "selectTool" && resizeTool){
            selectToolSelect();
            showModeChange();
            return;
        }
        enableTool("selectTool");
        selectToolScale();
        showModeChange();
        return;
    }
    if(name == "autotdb"){
        if(Game::writeTDB)
            autoAddToTDB = !autoAddToTDB;
        showModeChange();
        return;
    }
    if(name == "stickterr"){
        if(stickPointerToTerrain)
            placeToolStickAll();
        else
            placeToolStickTerrain();
        showModeChange();
        return;
    }
    if(name == "brushdir"){
        if(defaultPaintBrush == NULL)
            return;
        if(defaultPaintBrush->direction == 1)
            toolBrushDirectionDown();
        else
            toolBrushDirectionUp();
        showModeChange();
        return;
    }
    if(name == "camera"){
        Game::lockCamera = !Game::lockCamera;
        if(camera != NULL)
            camera->setLockYaxis(Game::lockCamera);
        showModeChange();
        return;
    }
    if(name == "camterr"){
        Game::cameraStickToTerrain = !Game::cameraStickToTerrain;
        showModeChange();
        return;
    }
    if(name == "guard"){
        placeGuardEnabled = !placeGuardEnabled;
        showModeChange();
        return;
    }
    if(name == "clearselect"){
        if(selectedObj != NULL){
            selectedObj->unselect();
            setSelectedObj(NULL);
        }
        lastSelectedObj = NULL;
        return;
    }
}

void RouteEditorGLWidget::jumpTo(PreciseTileCoordinate* c) {
    jumpTo(c->TileX, -c->TileZ, c->wX, c->wY, -c->wZ);
}

void RouteEditorGLWidget::jumpTo(float *posT, float *pos) {
    int X = posT[0];
    int Z = posT[1];
    float x = pos[0];
    float z = pos[2];
    Game::check_coords(X, Z, x, z);
    jumpTo(X, Z, x, pos[1], z);
}

void RouteEditorGLWidget::jumpTo(int X, int Z, float x, float y, float z) {
    if(Game::debugOutput) qDebug() << "jump: " << X << " " << Z;
    Game::terrainLib->load(X, Z);
    float h = Game::terrainLib->getHeight(X, Z, x, z);
    //if(h == -1)
        y = y + 10;
    if ((y < h) || (y > h + 100))
        y = h + 20;

    camera->setPozT(X, Z);
    camera->setPos(x, y, z);

}

/// EFO Object Selected
void RouteEditorGLWidget::objectSelected(GameObj* obj){
    if (selectedObj != NULL) {
        selectedObj->unselect();
        setSelectedObj(NULL);
    }
    toolEnabled = "selectTool";   /// set the selected tool

    if(obj == NULL)
        return;
    obj->select();
    setSelectedObj(obj);
}

void RouteEditorGLWidget::objectSelected(QVector<GameObj*> obj){
    if (selectedObj != NULL) {
        selectedObj->unselect();
        setSelectedObj(NULL);
    }
    if(obj.size() == 0)
        return;
    for(int i = 0; i < obj.size(); i++){
        groupObj->addObject((WorldObj*)obj[i]);
        groupObj->select();
        setSelectedObj(groupObj);
    }
}

void RouteEditorGLWidget::setPaintBrush(Brush* brush) {
    this->defaultPaintBrush = brush;
    Terrain::DefaultBrush = brush;
}

void RouteEditorGLWidget::setSelectedObj(GameObj* o) {
    selectedObj = o;
    Game::currentSelectedGameObj = selectedObj;
    emit showProperties(selectedObj);
    if (o != NULL)
        if (o->typeObj == o->worldobj)
           emit sendMsg("showShape", ((WorldObj*) o)->getShapePath());
}

void RouteEditorGLWidget::editCopy() {
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        if (selectedObj != NULL) {
            if (selectedObj->typeObj == GameObj::worldobj) {
                WorldObj *selectedWorldObj = (WorldObj*) selectedObj;
                if (selectedWorldObj->typeID == WorldObj::groupobject) {
                    delete copyPasteGroupObj;
                    copyPasteGroupObj = new GroupObj(*groupObj);
                    copyPasteObj = copyPasteGroupObj;
                } else {
                    copyPasteObj = selectedWorldObj;
                }
            }
        }
    }
}

void RouteEditorGLWidget::editPaste() {
    if(Game::debugOutput) qDebug() << "EditPaste Start";
    Undo::StateBeginIfNotExist();
    if (toolEnabled == "selectTool" || toolEnabled == "placeTool") {
        if (copyPasteObj != NULL) {
            //qDebug() << "EditPaste selectedObj->unselect";
            if (selectedObj != NULL)
                selectedObj->unselect();
            int placementTileX = (int)camera->pozT[0];
            int placementTileZ = (int)camera->pozT[1];
            float placementPointer[3];
            Vec3::copy(placementPointer, aktPointerPos);
            lastNewObjPosT[0] = camera->pozT[0];
            lastNewObjPosT[1] = camera->pozT[1];
            lastNewObjPos[0] = aktPointerPos[0];
            lastNewObjPos[1] = aktPointerPos[1];
            lastNewObjPos[2] = aktPointerPos[2];
            //qDebug() << "EditPaste copyPasteObj->typeObj";
            if(copyPasteObj->typeObj == GameObj::worldobj) {
               // qDebug() << "EditPaste copyPasteObj->typeID";
                if (copyPasteObj->typeID == WorldObj::groupobject) {
                    //qDebug() << "EditPaste groupobject";
                    groupObj->fromNewObjects((GroupObj*) copyPasteObj, route, (int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos);
                    //qDebug() << "EditPaste setSelectedObj";
                    setSelectedObj(groupObj);
                } else {
                    Ref::RefItem* refInfo = copyPasteObj->getRefInfo();
                    //qDebug() << "EditPaste object";
                    float *q = Quat::create();
                    Quat::copy(q, copyPasteObj->qDirection);
                    setSelectedObj(route->placeObject((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, q, 0, refInfo));
                    if (selectedObj == NULL)
                        showPlacementGuardError();
                    //qDebug() << "EditPaste setSelectedObj";
                    if (selectedObj != NULL && !validatePlacement((WorldObj*)selectedObj, refInfo, placementPointer, placementTileX, placementTileZ)) {
                        rejectPlacement();
                        return;
                    }
                    if (selectedObj != NULL) {
                        showPlacementSuccess();
                        selectedObj->select();
                    }
                }
            }
        }
    }
    if(Game::debugOutput) qDebug() << "EditPaste End";
}

void RouteEditorGLWidget::editSelect() {
    enableTool("selectTool");
}

void RouteEditorGLWidget::selectToolresetMoveStep(){
    Game::DefaultMoveStep = defaultMoveStep;
    moveStep = defaultMoveStep;
    moveMaxStep = defaultMoveStep;
}

void RouteEditorGLWidget::selectToolresetRot(){
    Quat::fill(this->placeRot);
    placeElev = 0;
}

void RouteEditorGLWidget::selectToolresetVert(){

    WorldObj* obj = (WorldObj*)selectedObj;

    float nq[4];
    nq[0] = 0.0f;              // Set first value to 0
    nq[1] = obj->qDirection[1]; // Keep existing second value
    nq[2] = 0.0f;              // Set third value to 0
    nq[3] = obj->qDirection[3]; // Keep existing fourth value

    // 4. Update the object
    Undo::SinglePushWorldObjData(obj);

    // Pass the modified float array back to the object
    obj->setQdirection(nq);
    obj->setModified();
    obj->setMartix();
}


void RouteEditorGLWidget::selectToolSelect(){
    resizeTool = false;
    translateTool = false;
    rotateTool = false;
}

void RouteEditorGLWidget::selectToolRotate(){
    resizeTool = false;
    translateTool = false;
    rotateTool = true;
}

void RouteEditorGLWidget::selectToolTranslate(){
    resizeTool = false;
    translateTool = true;
    rotateTool = false;
}

void RouteEditorGLWidget::selectToolScale(){
    resizeTool = true;
    translateTool = false;
    rotateTool = false;
}

void RouteEditorGLWidget::toolBrushDirectionUp(){
    defaultPaintBrush->direction = 1;
    emit sendMsg(QString("brushDirection"), QString("+"));
}

void RouteEditorGLWidget::toolBrushDirectionDown(){
    defaultPaintBrush->direction = -1;
    emit sendMsg(QString("brushDirection"), QString("-"));
}
void RouteEditorGLWidget::putTerrainTexToolSelectRandom(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->RANDOM;
}

void RouteEditorGLWidget::putTerrainTexToolSelectPresent(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->PRESENT;
}

void RouteEditorGLWidget::putTerrainTexToolSelect0(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT0;
    defaultPaintBrush->texRotationDegrees = 0;
    emit sendMsg(QString("textureRotation"), QString("0"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect90(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT90;
    defaultPaintBrush->texRotationDegrees = 90;
    emit sendMsg(QString("textureRotation"), QString("90"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect180(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT180;
    defaultPaintBrush->texRotationDegrees = 180;
    emit sendMsg(QString("textureRotation"), QString("180"));
}

void RouteEditorGLWidget::putTerrainTexToolSelect270(){
    defaultPaintBrush->texTransformation = defaultPaintBrush->ROT270;
    defaultPaintBrush->texRotationDegrees = 270;
    emit sendMsg(QString("textureRotation"), QString("270"));
}

void RouteEditorGLWidget::placeToolStickTerrain(){
    stickPointerToTerrain = true;
}

void RouteEditorGLWidget::placeToolStickAll(){
    stickPointerToTerrain = false;
}

void RouteEditorGLWidget::paintToolObjSelected(){
    route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, (WorldObj*) selectedObj);
}

void RouteEditorGLWidget::paintToolObj(){
    route->setTerrainTextureToObj((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, NULL);
}

void RouteEditorGLWidget::paintToolTDB(){
    route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 0);
}

void RouteEditorGLWidget::paintToolTDBVector(){
    route->setTerrainTextureToTrack((int) camera->pozT[0], (int) camera->pozT[1], aktPointerPos, defaultPaintBrush, 1);
}

void RouteEditorGLWidget::paintToolTileTrack(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int sections = route->setTerrainTextureToTileTrack(tileX, tileZ, defaultPaintBrush);
    if(sections > 0)
        emit sendMsg(QString("msg"), QString("Autopainted track on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(sections) + QString(" sections."));
    else
        emit sendMsg(QString("msg"), QString("No track sections found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolTileRoad(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int sections = route->setTerrainTextureToTileRoad(tileX, tileZ, defaultPaintBrush);
    if(sections > 0)
        emit sendMsg(QString("msg"), QString("Autopainted roads on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(sections) + QString(" sections."));
    else
        emit sendMsg(QString("msg"), QString("No road sections found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolWaterEdges(){
    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    int paintedEdges = Game::terrainLib->paintWaterEdges(defaultPaintBrush, tileX, tileZ);
    if(paintedEdges > 0)
        emit sendMsg(QString("msg"), QString("Autopainted water edges on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(": ") + QString::number(paintedEdges));
    else
        emit sendMsg(QString("msg"), QString("No water edges found on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
}

void RouteEditorGLWidget::paintToolResetTile(){
    QVector<QString> unsavedItems;
    getUnsavedInfo(unsavedItems);
    if (!unsavedItems.isEmpty()) {
        QMessageBox::warning(this, "Reset Tile",
                QString("There are %1 pending route change(s).\n\nSave your changes, then retry Reset Tile.")
                .arg(unsavedItems.size()));
        return;
    }

    int tileX = (int) camera->pozT[0];
    int tileZ = (int) camera->pozT[1];
    float posx = aktPointerPos[0];
    float posz = aktPointerPos[2];
    Game::check_coords(tileX, tileZ, posx, posz);

    QString msg = QString("Reset terrain paint on tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("?");
    if (QMessageBox::question(this, "Reset Tile", msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    int filesDeleted = 0;
    int filesFailed = 0;
    if (!route->resetTerrainTextureOnTile(tileX, tileZ, filesDeleted, filesFailed)) {
        emit sendMsg(QString("msg"), QString("Reset Tile failed on ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString("."));
        return;
    }

    emit sendMsg(QString("msg"), QString("Reset tile ") + QString::number(tileX) + QString(", ") + QString::number(tileZ) + QString(". Deleted ") + QString::number(filesDeleted) + QString(" texture file(s), failed ") + QString::number(filesFailed) + QString("."));
}

void RouteEditorGLWidget::editFind1x1() {
    editFind(0);
}

void RouteEditorGLWidget::editFind3x3() {
    editFind(1);
}

void RouteEditorGLWidget::editFind(int radius) {
    if (selectedObj != NULL)
        if (selectedObj->typeObj == GameObj::worldobj){
            selectedObj->unselect();
            route->findSimilar((WorldObj*)selectedObj, groupObj, camera->pozT, radius);
            setSelectedObj(groupObj);
            if (groupObj->count() == 0) {
                groupObj->unselect();
                setSelectedObj(NULL);
            }
        }
}

void RouteEditorGLWidget::editUndo() {
    Undo::UndoLast();
}

void RouteEditorGLWidget::showTrkEditr() {
    if (route != NULL)
        route->showTrkEditr();
}




void RouteEditorGLWidget::setTerrainToObj(){
    Undo::StateBegin();
    if (selectedObj != NULL)
        route->setTerrainToTrackObj((WorldObj*)selectedObj, defaultPaintBrush);
    else
        route->setTerrainToTrackObj((WorldObj*)lastSelectedObj, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::smoothTerrainToObj(){
    Undo::StateBegin();
    if (selectedObj != NULL)
        route->smoothTerrainToTrackObj((WorldObj*)selectedObj, defaultPaintBrush);
    else
        route->smoothTerrainToTrackObj((WorldObj*)lastSelectedObj, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::setTerrainToNearestDbTile(){
    if(route == NULL)
        return;

    Undo::StateBegin();
    route->setTerrainToNearestDbTile((int)camera->pozT[0], (int)camera->pozT[1], aktPointerPos, defaultPaintBrush);
    Undo::StateEnd();
}

void RouteEditorGLWidget::setTerrainToSelectedObjTile(){
    if(route == NULL || selectedObj == NULL)
        return;
    if(selectedObj->typeObj != GameObj::worldobj)
        return;

    WorldObj *obj = (WorldObj*)selectedObj;
    if(obj->type != "trackobj" && obj->type != "dyntrack")
        return;

    Undo::StateBegin();
    route->setTerrainToTrackObjTile(obj, defaultPaintBrush, (int)camera->pozT[0], (int)camera->pozT[1]);
    Undo::StateEnd();
}

void RouteEditorGLWidget::selectAllTerrainPatchesOnSelectedTile(){
    if(selectedObj == NULL || selectedObj->typeObj != GameObj::terrainobj)
        return;

    ((Terrain*)selectedObj)->selectAllPatches();
}

void RouteEditorGLWidget::adjustObjPositionToTerrainMenu(){
    Undo::StateBeginIfNotExist();
    Undo::PushGameObjData(selectedObj);
    if (selectedObj != NULL)
        if(selectedObj->typeObj == GameObj::worldobj)
            ((WorldObj*)selectedObj)->adjustPositionToTerrain();
}

void RouteEditorGLWidget::adjustObjRotationToTerrainMenu(){
    Undo::StateBeginIfNotExist();
    Undo::PushGameObjData(selectedObj);
    if (selectedObj != NULL)
        if(selectedObj->typeObj == GameObj::worldobj)
            ((WorldObj*)selectedObj)->adjustRotationToTerrain();
}

void RouteEditorGLWidget::pickObjForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            Quat::copy(this->placeRot, ((WorldObj*)selectedObj)->qDirection);
            route->ref->selected = ((WorldObj*)selectedObj)->getRefInfo();
            emit itemSelected(route->ref->selected);
        }
    }
}

void RouteEditorGLWidget::pickObjRotForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            placeElev = 0;
            Quat::copy(this->placeRot, ((WorldObj*)selectedObj)->qDirection);
        }
    }
}

void RouteEditorGLWidget::pickObjRotForCamera(){
    double spinval = (M_PI );

    /*
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            int camobjtx = ((WorldObj*)selectedObj)->x;
            int camobjty = ((WorldObj*)selectedObj)->y;

            double camobjx = ((WorldObj*)selectedObj)->position[0];
            double camobjy = ((WorldObj*)selectedObj)->position[1]+10;
            double camobjz = ((WorldObj*)selectedObj)->position[2];
            double currcam = camera->getRotX();

            camera->setPozT(camobjtx,camobjty);
            camera->setPos(camobjx,camobjy,camobjz);

            double camrotw = ((WorldObj*)selectedObj)->qDirection[1];
            double camrotz = ((WorldObj*)selectedObj)->qDirection[3];
            double camrot = (2.0 * std::atan2(camrotw, camrotz));

            camera->setPlayerRot(camrot,NULL);
            selectedObj->unselect(); setSelectedObj(NULL);

         }
    }
    else
    {
     * */
            qDebug() << "trying repos cam to nearest object";

        ////if(CamObj == NULL)
            ////{
            CamObj = NULL;
            float campos[3];
            campos[0] = aktPointerPos[0];
            campos[1] = aktPointerPos[1];
            campos[2] = aktPointerPos[2];

            CamObj = route->findNearestObj((int)camera->pozT[0] , (int) camera->pozT[1],campos );
            //qDebug() << "CamObj:        " << CamObj->fileName;
            //qDebug() << "aktPointerPos: " << camera->pozT[0] << " " << camera->pozT[1] << " " <<  aktPointerPos[0] << " " <<  aktPointerPos[1] << " " <<  -aktPointerPos[2];
            ////}
            if(CamObj == NULL)
            {   qDebug() << "camObj still null";
                return;
            }
            int camobjtx = CamObj->x;
            int camobjty = CamObj->y;

            double camobjx = CamObj->position[0];
            double camobjy = CamObj->position[1]+10;
            double camobjz = CamObj->position[2];
            double currcam = camera->getRotX();

            camera->setPozT(camobjtx,camobjty);
            camera->setPos(camobjx,camobjy,camobjz);

            double camrotw = CamObj->qDirection[1];
            double camrotz = CamObj->qDirection[3];
            double camrot = (2.0 * std::atan2(camrotw, camrotz));

            while (camrot > 2.0 * M_PI)
                camrot -= 2.0 * M_PI;
            while (camrot < 0)
                camrot += 2.0 * M_PI;

            while (currcam > 2.0 * M_PI)
                currcam -= 2.0 * M_PI;
            while (currcam < 0)
                currcam += 2.0 * M_PI;


            if((currcam - camrot) > (M_PI/2)) camrot = camrot - M_PI;
            if((currcam - camrot) < (-M_PI/2)) camrot = camrot + M_PI;

            while (camrot > 2.0 * M_PI)
                camrot -= 2.0 * M_PI;
            while (camrot < 0)
                camrot += 2.0 * M_PI;

            qDebug() << "starting = " << currcam << " camrot = " << camrot;

            camera->setPlayerRot(camrot,NULL);
            CamObj = NULL;
            return;
  //      }
//    }
}

void RouteEditorGLWidget::pickObjRotForCameraFlip(){
            double camrot = (camera->getRotX()) + M_PI;
            camera->setPlayerRot(camrot,NULL);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamN() {
    double camrot = (M_PI);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;

    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);

}

void RouteEditorGLWidget::resetCamS() {
    double camrot = 0;
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamE() {
    double camrot = (M_PI/2);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}
void RouteEditorGLWidget::resetCamW() {
    double camrot = (M_PI/-2);
    double camelev = camera->getRotY();
    if(camelev < (M_PI/-4)) camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamD() {
    double camrot = 0;
    double camelev = (M_PI/-2);
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}

void RouteEditorGLWidget::resetCamZ() {
    double camrot = 0;
    double camelev = 0;
    camera->setPlayerRot(camrot,camelev);
            if(selectedObj != NULL) selectedObj->unselect();
            setSelectedObj(NULL);
}


// tangentTarget

void RouteEditorGLWidget::tangentOrigin(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){

            StartObject = selectedObj;
            //if(Game::debugOutput)
                if(Game::debugOutput) qDebug() << "Tangent Start: " << ((WorldObj*)StartObject)->x << " " << ((WorldObj*)StartObject)->y << " " << ((WorldObj*)StartObject)->position[0] << " " << ((WorldObj*)StartObject)->position[1] << " " << ((WorldObj*)StartObject)->position[2];


            if(EndObject != NULL) tangentMath();
         }
    }
}
void RouteEditorGLWidget::tangentTarget(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){

            EndObject = selectedObj;
            // if(Game::debugOutput)
                if(Game::debugOutput) qDebug() << "Tangent End: " << ((WorldObj*)EndObject)->x << " " << ((WorldObj*)EndObject)->y << " " << ((WorldObj*)EndObject)->position[0] << " " << ((WorldObj*)EndObject)->position[1] << " " << ((WorldObj*)EndObject)->position[2];

            if(StartObject != NULL) tangentMath();
         }
    }
}

void RouteEditorGLWidget::tangentMath(){


    int stx = ((WorldObj*)StartObject)->x;
    int etx = ((WorldObj*)EndObject)->x;
    int stz = ((WorldObj*)StartObject)->y;
    int etz = ((WorldObj*)EndObject)->y;
    float spx = ((WorldObj*)StartObject)->position[0];
    float epx = ((WorldObj*)EndObject)->position[0];
    float spy = ((WorldObj*)StartObject)->position[1];
    float epy = ((WorldObj*)EndObject)->position[1];
    float spz = ((WorldObj*)StartObject)->position[2];
    float epz = ((WorldObj*)EndObject)->position[2];



    float dx = (2048 * (etx - stx)) + epx - spx;
     if(Game::debugOutput) qDebug() << "DX: " << dx ;

    float dy = epy - spy;
     if(Game::debugOutput) qDebug() << "DY: " << dy ;

    float dz = -((2048 * (etz - stz)) + epz - spz);
     if(Game::debugOutput) qDebug() << "DZ: " << dz ;

    double heading; // d
    if (abs(dx) > 0)
    {
        if (dx > 0) { heading = 90 - 180 / M_PI * atan(dz / dx); }
        else { heading = -90 - 180 / M_PI * atan(dz / dx); }
    }
    else { heading = -90 + copysignf(1.0, dz) * 90; }

     if(Game::debugOutput) qDebug() << "HD: " << heading ;

    double distance = sqrt((dx * dx) + (dy * dy) + (dz * dz)); // d
     if(Game::debugOutput) qDebug() << "DI: " << distance ;

    double slope; // d
    if (abs(dy / distance) < 0.99999)
    { slope = atan(dy / sqrt(dx * dx + dz * dz)); }
    else { slope = copysignf(1.0, dy / distance) * 1000; }

     if(Game::debugOutput) qDebug() << "SL: " << slope ;

    double slopedeg = 0;  // d
    if (abs(dy / distance) < 0.99999)
    { slope = 180 / M_PI * atan(dy / sqrt(dx * dx + dz * dz)); }
    else { slope = copysignf(1.0, dy / distance) * 90; }

     if(Game::debugOutput) qDebug() << "SD: " << slopedeg ;

    double headingcos = cos(-0.5 * heading * M_PI / 180); // d
    double headingsin = sin(-0.5 * heading * M_PI / 180); // d
    double slopecos = 1;  // d
    double slopesin = 0; // d
    double slopedegcos = 1; // d
    double slopedegsin = 0; // d

    double QD4 = (headingcos * slopecos * slopedegcos) + (headingsin * slopesin * slopedegsin);  // d
    double QD2 = (headingsin * slopecos * slopedegcos) - (headingcos * slopesin * slopedegsin); // d
    double QD3 = (headingcos * slopesin * slopedegcos) + (headingsin * slopecos * slopedegsin); // d
    double QD1 = (headingcos * slopecos * slopedegsin) + (headingsin * slopesin * slopedegcos); // d

//    QD = Math.Round(QD1, 6).toString() + " " + Math.Round(QD2, 6).toString() + " " + Math.Round(QD3, 6).toString() + " " + Math.Round(QD4, 6).toString();
//    DQ = Math.Round(QD3, 6).toString() + " " + Math.Round(QD4, 6).toString() + " " + Math.Round(QD1, 6).toString() + " " + Math.Round(QD2 * -1, 6).toString();

    QClipboard *clipboard = QApplication::clipboard();
    QString qd = QString::number(QD1) + " " + QString::number(QD2) + " " + QString::number(QD3) + " " + QString::number(QD4) ;


    clipboard->setText(qd);
    if(Game::debugOutput) qDebug() << "Final QD:" << qd;

}

void RouteEditorGLWidget::TangentApplyRot(){
    if(selectedObj->typeObj != GameObj::worldobj)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    if(args.length() != 4)
        return;
    float nq[4];
    nq[0] = args[0].toFloat();
    nq[1] = args[1].toFloat();
    nq[2] = -args[2].toFloat();
    nq[3] = args[3].toFloat();

    Undo::SinglePushWorldObjData((WorldObj*)selectedObj);
    ((WorldObj*)selectedObj)->setQdirection((float*)&nq);
    ((WorldObj*)selectedObj)->setModified();
    ((WorldObj*)selectedObj)->setMartix();
    // quat.setText(clipboard->text());
}

void RouteEditorGLWidget::pickObjRotElevForPlacement(){
    if (selectedObj != NULL) {
        if(selectedObj->typeObj == GameObj::worldobj){
            Quat::fill(this->placeRot);
            placeElev = ((WorldObj*)selectedObj)->getElevation();
        }
    }
}

void RouteEditorGLWidget::showContextMenu(const QPoint & point) {
    if(defaultMenuActions["undo"] == NULL){
        defaultMenuActions["undo"] = new QAction(tr("&Undo"), this);
        QObject::connect(defaultMenuActions["undo"], SIGNAL(triggered()), this, SLOT(editUndo()));
    }
    if(defaultMenuActions["copy"] == NULL){
        defaultMenuActions["copy"] = new QAction(tr("&Copy"), this);
        QObject::connect(defaultMenuActions["copy"], SIGNAL(triggered()), this, SLOT(editCopy()));
    }
    if(defaultMenuActions["paste"] == NULL){
        defaultMenuActions["paste"] = new QAction(tr("&Paste"), this);
        QObject::connect(defaultMenuActions["paste"], SIGNAL(triggered()), this, SLOT(editPaste()));
    }
    if(defaultMenuActions["find1x1"] == NULL){
        defaultMenuActions["find1x1"] = new QAction(tr("&Select Similar 1x1"), this);
        QObject::connect(defaultMenuActions["find1x1"], SIGNAL(triggered()), this, SLOT(editFind1x1()));
    }
    if(defaultMenuActions["find3x3"] == NULL){
        defaultMenuActions["find3x3"] = new QAction(tr("&Select Similar 3x3"), this);
        QObject::connect(defaultMenuActions["find3x3"], SIGNAL(triggered()), this, SLOT(editFind3x3()));
    }
    if(defaultMenuActions["select"] == NULL){
        defaultMenuActions["select"] = new QAction(tr("&Select Tool"), this);
        QObject::connect(defaultMenuActions["select"], SIGNAL(triggered()), this, SLOT(editSelect()));
    }
    if(defaultMenuActions["setTerrToObj"] == NULL){
        defaultMenuActions["setTerrToObj"] = new QAction(tr("&Set Terrain to Object"));
        QObject::connect(defaultMenuActions["setTerrToObj"], SIGNAL(triggered()), this, SLOT(setTerrainToObj()));
    }
    if(defaultMenuActions["setPosToTerr"] == NULL){
        defaultMenuActions["setPosToTerr"] = new QAction(tr("&Set position to Terrain"));
        QObject::connect(defaultMenuActions["setPosToTerr"], SIGNAL(triggered()), this, SLOT(adjustObjPositionToTerrainMenu()));
    }
    if(defaultMenuActions["setRotToTerr"] == NULL){
        defaultMenuActions["setRotToTerr"] = new QAction(tr("&Set rotation to Terrain"));
        QObject::connect(defaultMenuActions["setRotToTerr"], SIGNAL(triggered()), this, SLOT(adjustObjRotationToTerrainMenu()));
    }
    if(defaultMenuActions["pickObj"] == NULL){
        defaultMenuActions["pickObj"] = new QAction(tr("&Pick for placement"));
        QObject::connect(defaultMenuActions["pickObj"], SIGNAL(triggered()), this, SLOT(pickObjForPlacement()));
    }
    if(defaultMenuActions["pickObjRot"] == NULL){
        defaultMenuActions["pickObjRot"] = new QAction(tr("&Pick rotation for placement"));
        QObject::connect(defaultMenuActions["pickObjRot"], SIGNAL(triggered()), this, SLOT(pickObjRotForPlacement()));
    }
    if(defaultMenuActions["pickObjElev"] == NULL){
        defaultMenuActions["pickObjElev"] = new QAction(tr("&Pick elevation for placement"));
        QObject::connect(defaultMenuActions["pickObjElev"], SIGNAL(triggered()), this, SLOT(pickObjRotElevForPlacement()));
    }

    if(defaultMenuActions["pickObjRotCam"] == NULL){
        defaultMenuActions["pickObjRotCam"] = new QAction(tr("&Reposition camera to object"));
        QObject::connect(defaultMenuActions["pickObjRotCam"], SIGNAL(triggered()), this, SLOT(pickObjRotForCamera()));
    }

    if(defaultMenuActions["pickObjRotCamFlip"] == NULL){
        defaultMenuActions["pickObjRotCamFlip"] = new QAction(tr("&Flip camera 180 degrees"));
        QObject::connect(defaultMenuActions["pickObjRotCamFlip"], SIGNAL(triggered()), this, SLOT(pickObjRotForCameraFlip()));
    }

    if(defaultMenuActions["resetCamN"] == NULL){
        defaultMenuActions["resetCamN"] = new QAction(tr("Face &North"));
        QObject::connect(defaultMenuActions["resetCamN"], SIGNAL(triggered()), this, SLOT(resetCamN()));
    }

    if(defaultMenuActions["resetCamS"] == NULL){
        defaultMenuActions["resetCamS"] = new QAction(tr("Face &South"));
        QObject::connect(defaultMenuActions["resetCamS"], SIGNAL(triggered()), this, SLOT(resetCamS()));
    }
    if(defaultMenuActions["resetCamE"] == NULL){
        defaultMenuActions["resetCamE"] = new QAction(tr("Face &East"));
        QObject::connect(defaultMenuActions["resetCamE"], SIGNAL(triggered()), this, SLOT(resetCamE()));
    }
    if(defaultMenuActions["resetCamW"] == NULL){
        defaultMenuActions["resetCamW"] = new QAction(tr("Face &West"));
        QObject::connect(defaultMenuActions["resetCamW"], SIGNAL(triggered()), this, SLOT(resetCamW()));
    }

    if(defaultMenuActions["resetCamD"] == NULL){
        defaultMenuActions["resetCamD"] = new QAction(tr("Face &Down"));
        QObject::connect(defaultMenuActions["resetCamD"], SIGNAL(triggered()), this, SLOT(resetCamD()));
    }

    if(defaultMenuActions["resetCamZ"] == NULL){
        defaultMenuActions["resetCamZ"] = new QAction(tr("Default"));
        QObject::connect(defaultMenuActions["resetCamZ"], SIGNAL(triggered()), this, SLOT(resetCamZ()));
    }

    if(defaultMenuActions["TangentOrigin"] == NULL){
        defaultMenuActions["TangentOrigin"] = new QAction(tr("Tangent &Origin"));
        QObject::connect(defaultMenuActions["TangentOrigin"], SIGNAL(triggered()), this, SLOT(tangentOrigin()));
    }

    if(defaultMenuActions["TangentTarget"] == NULL){
        defaultMenuActions["TangentTarget"] = new QAction(tr("Tangent &Target"));
        QObject::connect(defaultMenuActions["TangentTarget"], SIGNAL(triggered()), this, SLOT(tangentTarget()));
    }

    if(defaultMenuActions["TangentApply"] == NULL){
        defaultMenuActions["TangentApply"] = new QAction(tr("Tangent &Apply Rotation"));
        QObject::connect(defaultMenuActions["TangentApply"], SIGNAL(triggered()), this, SLOT(TangentApplyRot()));
    }



    QMenu menu;
    QMenu menuTool;
    QMenu menuCamera;
    QMenu menuPointer;
    QString menuStyle = QString(
        "QMenu::separator {\
          color: ")+Game::StyleMainLabel+";\
        }";
    menu.setStyleSheet(menuStyle);
    if(selectedObj != NULL){
        menu.addSection("Object: " + selectedObj->getName());
        selectedObj->pushContextMenuActions(&menu);

        if(selectedObj->typeObj == selectedObj->worldobj){
            menu.addAction(defaultMenuActions["setTerrToObj"]);
            menu.addAction(defaultMenuActions["setPosToTerr"]);
            menu.addAction(defaultMenuActions["setRotToTerr"]);
            menu.addAction(defaultMenuActions["pickObj"]);
            menu.addAction(defaultMenuActions["pickObjRot"]);
            menu.addAction(defaultMenuActions["pickObjElev"]);

            menu.addAction(defaultMenuActions["find1x1"]);
            menu.addAction(defaultMenuActions["find3x3"]);
        }




    }
    if(toolEnabled == ""){
        menu.addSection("No Tool");
    } else {
        QString toolName = toolEnabled;
        toolName[0] = toolName[0].toUpper();
        menu.addSection(toolName);
        if (toolEnabled == "selectTool"){
            menuTool.setTitle("Mode");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["selectToolSelect"] == NULL){
                defaultMenuActions["selectToolSelect"] = GuiFunct::newMenuCheckAction(tr("&Select"), this, !resizeTool|!rotateTool|!translateTool);
                QObject::connect(defaultMenuActions["selectToolSelect"], SIGNAL(triggered()), this, SLOT(selectToolSelect()));
            }
            defaultMenuActions["selectToolSelect"]->setChecked(!resizeTool&!rotateTool&!translateTool);
            if(defaultMenuActions["selectToolRotate"] == NULL){
                defaultMenuActions["selectToolRotate"] = GuiFunct::newMenuCheckAction(tr("&Rotate"), this, rotateTool);
                QObject::connect(defaultMenuActions["selectToolRotate"], SIGNAL(triggered()), this, SLOT(selectToolRotate()));
            }
            defaultMenuActions["selectToolRotate"]->setChecked(rotateTool);
            if(defaultMenuActions["selectToolTranslate"] == NULL){
                defaultMenuActions["selectToolTranslate"] = GuiFunct::newMenuCheckAction(tr("&Translate"), this, translateTool);
                QObject::connect(defaultMenuActions["selectToolTranslate"], SIGNAL(triggered()), this, SLOT(selectToolTranslate()));
            }
            defaultMenuActions["selectToolTranslate"]->setChecked(translateTool);
            if(defaultMenuActions["selectToolScale"] == NULL){
                defaultMenuActions["selectToolScale"] = GuiFunct::newMenuCheckAction(tr("&Custom"), this, resizeTool);
                QObject::connect(defaultMenuActions["selectToolScale"], SIGNAL(triggered()), this, SLOT(selectToolScale()));
            }
            defaultMenuActions["selectToolScale"]->setChecked(resizeTool);
            menuTool.addAction(defaultMenuActions["selectToolSelect"]);
            menuTool.addAction(defaultMenuActions["selectToolRotate"]);
            menuTool.addAction(defaultMenuActions["selectToolTranslate"]);
            menuTool.addAction(defaultMenuActions["selectToolScale"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            menuPointer.setTitle("Pointer");
            menu.addMenu(&menuPointer);
            if(defaultMenuActions["placeToolStickToTerrain"] == NULL){
                defaultMenuActions["placeToolStickToTerrain"] = GuiFunct::newMenuCheckAction(tr("&Stick to Terrain"), this, stickPointerToTerrain);
                QObject::connect(defaultMenuActions["placeToolStickToTerrain"], SIGNAL(triggered()), this, SLOT(placeToolStickTerrain()));
            }
            defaultMenuActions["placeToolStickToTerrain"]->setChecked(stickPointerToTerrain);
            if(defaultMenuActions["placeToolStickToAll"] == NULL){
                defaultMenuActions["placeToolStickToAll"] = GuiFunct::newMenuCheckAction(tr("&Stick to All"), this, !stickPointerToTerrain);
                QObject::connect(defaultMenuActions["placeToolStickToAll"], SIGNAL(triggered()), this, SLOT(placeToolStickAll()));
            }
            defaultMenuActions["placeToolStickToAll"]->setChecked(!stickPointerToTerrain);
            menuPointer.addAction(defaultMenuActions["placeToolStickToTerrain"]);
            menuPointer.addAction(defaultMenuActions["placeToolStickToAll"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetMoveStep"] == NULL){
                defaultMenuActions["resetMoveStep"] = new QAction(tr("&Reset MoveStep"), this);
                QObject::connect(defaultMenuActions["resetMoveStep"], SIGNAL(triggered()), this, SLOT(selectToolresetMoveStep()));
            }
            menu.addAction(defaultMenuActions["resetMoveStep"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetRot"] == NULL){
                defaultMenuActions["resetRot"] = new QAction(tr("&Reset Rotation"), this);
                QObject::connect(defaultMenuActions["resetRot"], SIGNAL(triggered()), this, SLOT(selectToolresetRot()));
            }
            menu.addAction(defaultMenuActions["resetRot"]);
        }
        if (toolEnabled == "placeTool" || toolEnabled == "selectTool"){
            if(defaultMenuActions["resetVert"] == NULL){
                defaultMenuActions["resetVert"] = new QAction(tr("Reset &Vertical"), this);
                QObject::connect(defaultMenuActions["resetVert"], SIGNAL(triggered()), this, SLOT(selectToolresetVert()));
            }
            menu.addAction(defaultMenuActions["resetVert"]);
        }


        if (toolEnabled == "heightTool" || toolEnabled == "waterTerrTool" || toolEnabled == "gapsTerrainTool"){
            menu.addMenu(&menuTool);
            if(defaultMenuActions["toolDirectionUp"] == NULL){
                defaultMenuActions["toolDirectionUp"] = GuiFunct::newMenuCheckAction(tr("&Up"), this, (defaultPaintBrush->direction+1));
                QObject::connect(defaultMenuActions["toolDirectionUp"], SIGNAL(triggered()), this, SLOT(toolBrushDirectionUp()));
            }
            defaultMenuActions["toolDirectionUp"]->setChecked((defaultPaintBrush->direction+1));
            if(defaultMenuActions["toolDirectionDown"] == NULL){
                defaultMenuActions["toolDirectionDown"] = GuiFunct::newMenuCheckAction(tr("&Down"), this, !((defaultPaintBrush->direction+1)));
                QObject::connect(defaultMenuActions["toolDirectionDown"], SIGNAL(triggered()), this, SLOT(toolBrushDirectionDown()));
            }
            defaultMenuActions["toolDirectionDown"]->setChecked(!((defaultPaintBrush->direction+1)));
            menuTool.addAction(defaultMenuActions["toolDirectionUp"]);
            menuTool.addAction(defaultMenuActions["toolDirectionDown"]);

            if (toolEnabled == "heightTool"){
                menuTool.setTitle("Paint Direction");
                defaultMenuActions["toolDirectionUp"]->setText("Up");
                defaultMenuActions["toolDirectionDown"]->setText("Down");
            }
            if (toolEnabled == "waterTerrTool"){
                menuTool.setTitle("Water");
                defaultMenuActions["toolDirectionUp"]->setText("Show");
                defaultMenuActions["toolDirectionDown"]->setText("Hide");
            }
            if (toolEnabled == "gapsTerrainTool"){
                menuTool.setTitle("Gaps");
                defaultMenuActions["toolDirectionUp"]->setText("Show");
                defaultMenuActions["toolDirectionDown"]->setText("Hide");
            }
        }
        if (toolEnabled == "putTerrainTexTool"){
            menuTool.setTitle("Default");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["putTerrainTexRandom"] == NULL){
                defaultMenuActions["putTerrainTexRandom"] = GuiFunct::newMenuCheckAction(tr("&Random"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->RANDOM);
                QObject::connect(defaultMenuActions["putTerrainTexRandom"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelectRandom()));
            }
            defaultMenuActions["putTerrainTexRandom"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->RANDOM);
            if(defaultMenuActions["putTerrainTexPresent"] == NULL){
                defaultMenuActions["putTerrainTexPresent"] = GuiFunct::newMenuCheckAction(tr("&Present"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->PRESENT);
                QObject::connect(defaultMenuActions["putTerrainTexPresent"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelectPresent()));
            }
            defaultMenuActions["putTerrainTexPresent"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->PRESENT);
            if(defaultMenuActions["putTerrainTex0"] == NULL){
                defaultMenuActions["putTerrainTex0"] = GuiFunct::newMenuCheckAction(tr("&Rotate 0°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT0);
                QObject::connect(defaultMenuActions["putTerrainTex0"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect0()));
            }
            defaultMenuActions["putTerrainTex0"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT0);
            if(defaultMenuActions["putTerrainTex90"] == NULL){
                defaultMenuActions["putTerrainTex90"] = GuiFunct::newMenuCheckAction(tr("&Rotate 90°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT90);
                QObject::connect(defaultMenuActions["putTerrainTex90"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect90()));
            }
            defaultMenuActions["putTerrainTex90"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT90);
            if(defaultMenuActions["putTerrainTex180"] == NULL){
                defaultMenuActions["putTerrainTex180"] = GuiFunct::newMenuCheckAction(tr("&Rotate 180°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT180);
                QObject::connect(defaultMenuActions["putTerrainTex180"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect180()));
            }
            defaultMenuActions["putTerrainTex180"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT180);
            if(defaultMenuActions["putTerrainTex270"] == NULL){
                defaultMenuActions["putTerrainTex270"] = GuiFunct::newMenuCheckAction(tr("&Rotate 270°"), this, defaultPaintBrush->texTransformation == defaultPaintBrush->ROT270);
                QObject::connect(defaultMenuActions["putTerrainTex270"], SIGNAL(triggered()), this, SLOT(putTerrainTexToolSelect270()));
            }
            defaultMenuActions["putTerrainTex270"]->setChecked(defaultPaintBrush->texTransformation == defaultPaintBrush->ROT270);
            menuTool.addAction(defaultMenuActions["putTerrainTexRandom"]);
            menuTool.addAction(defaultMenuActions["putTerrainTexPresent"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex0"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex90"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex180"]);
            menuTool.addAction(defaultMenuActions["putTerrainTex270"]);
        }
        if (toolEnabled.startsWith("paintTool")){
            menuTool.setTitle("Auto Paint");
            menu.addMenu(&menuTool);
            if(defaultMenuActions["paintToolObjSelected"] == NULL){
                defaultMenuActions["paintToolObjSelected"] = new QAction(tr("&Selected Object"), this);
                QObject::connect(defaultMenuActions["paintToolObjSelected"], SIGNAL(triggered()), this, SLOT(paintToolObjSelected()));
            }
            if(defaultMenuActions["paintToolObj"] == NULL){
                defaultMenuActions["paintToolObj"] = new QAction(tr("&Nearest Object"), this);
                QObject::connect(defaultMenuActions["paintToolObj"], SIGNAL(triggered()), this, SLOT(paintToolObj()));
            }
            if(defaultMenuActions["paintToolTDB"] == NULL){
                defaultMenuActions["paintToolTDB"] = new QAction(tr("&Nearest Track or Road"), this);
                QObject::connect(defaultMenuActions["paintToolTDB"], SIGNAL(triggered()), this, SLOT(paintToolTDB()));
            }
            if(defaultMenuActions["paintToolTDBVector"] == NULL){
                defaultMenuActions["paintToolTDBVector"] = new QAction(tr("&Nearest TDB/RDB Vector"), this);
                QObject::connect(defaultMenuActions["paintToolTDBVector"], SIGNAL(triggered()), this, SLOT(paintToolTDBVector()));
            }
            if(defaultMenuActions["paintToolTileTrack"] == NULL){
                defaultMenuActions["paintToolTileTrack"] = new QAction(tr("&Track on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolTileTrack"], SIGNAL(triggered()), this, SLOT(paintToolTileTrack()));
            }
            if(defaultMenuActions["paintToolTileRoad"] == NULL){
                defaultMenuActions["paintToolTileRoad"] = new QAction(tr("&Roads on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolTileRoad"], SIGNAL(triggered()), this, SLOT(paintToolTileRoad()));
            }
            if(defaultMenuActions["paintToolWaterEdges"] == NULL){
                defaultMenuActions["paintToolWaterEdges"] = new QAction(tr("&Water on Tile"), this);
                QObject::connect(defaultMenuActions["paintToolWaterEdges"], SIGNAL(triggered()), this, SLOT(paintToolWaterEdges()));
            }
            if(defaultMenuActions["paintToolResetTile"] == NULL){
                defaultMenuActions["paintToolResetTile"] = new QAction(tr("&Reset Tile Paint"), this);
                QObject::connect(defaultMenuActions["paintToolResetTile"], SIGNAL(triggered()), this, SLOT(paintToolResetTile()));
            }
            menuTool.addAction(defaultMenuActions["paintToolObjSelected"]);
            menuTool.addAction(defaultMenuActions["paintToolObj"]);
            menuTool.addAction(defaultMenuActions["paintToolTDB"]);
            menuTool.addAction(defaultMenuActions["paintToolTDBVector"]);
            menuTool.addSeparator();
            menuTool.addAction(defaultMenuActions["paintToolTileTrack"]);
            menuTool.addAction(defaultMenuActions["paintToolTileRoad"]);
            menuTool.addAction(defaultMenuActions["paintToolWaterEdges"]);
            menu.addAction(defaultMenuActions["paintToolResetTile"]);
        }

    }

    // EFO WFH tools
    if(selectedObj != NULL){
    menu.addSection("WFH Tools");
        menu.addAction(defaultMenuActions["TangentOrigin"]);
        menu.addAction(defaultMenuActions["TangentTarget"]);
        menu.addAction(defaultMenuActions["TangentApply"]);
    }

    // EFO new Camera menu
    menu.addSection("Camera");
    //if(selectedObj != NULL){
        menu.addAction(defaultMenuActions["pickObjRotCam"]);
    //}
    menu.addAction(defaultMenuActions["pickObjRotCamFlip"]);

    menuCamera.setTitle("Reset Camera");
            menu.addMenu(&menuCamera);
            menuCamera.addAction(defaultMenuActions["resetCamN"]);
            menuCamera.addAction(defaultMenuActions["resetCamS"]);
            menuCamera.addAction(defaultMenuActions["resetCamE"]);
            menuCamera.addAction(defaultMenuActions["resetCamW"]);
            menuCamera.addAction(defaultMenuActions["resetCamD"]);
            menuCamera.addAction(defaultMenuActions["resetCamZ"]);

    menu.addSeparator();
    menu.addSection("Edit");
    menu.addAction(defaultMenuActions["undo"]);
    menu.addAction(defaultMenuActions["copy"]);
    menu.addAction(defaultMenuActions["paste"]);
    menu.addSeparator();
    menu.addAction(defaultMenuActions["select"]);

    menu.exec(mapToGlobal(point));
}

void RouteEditorGLWidget::createNewTiles(QMap<int, QPair<int, int>*> list){
    int x, z;
    QMapIterator<int, QPair<int, int>*> i2(list);
    while (i2.hasNext()) {
        i2.next();
        if(i2.value() == NULL)
            continue;
        x = i2.value()->first;
        z = i2.value()->second;
        if(Game::debugOutput) qDebug() << x << z;
        route->newTile(x, -z);
    }
}

void RouteEditorGLWidget::createNewLoTiles(QMap<int, QPair<int, int>*> list){
    int x, z;
    QMapIterator<int, QPair<int, int>*> i2(list);
    if (!Game::writeEnabled) return;
    while (i2.hasNext()) {
        i2.next();
        if(i2.value() == NULL)
            continue;
        x = i2.value()->first;
        z = i2.value()->second;
        if(Game::debugOutput) qDebug() << x << z;
        Game::terrainLib->setLowTerrainAsCurrent();
        Game::terrainLib->saveEmpty(x, z);
        Game::terrainLib->reload(x, -z);
        if(Game::autoGeoTerrain){
            float pos[3];
            Vec3::set(pos, 0, 0, 0);
            Game::terrainLib->setHeightFromGeo(x, -z, (float*)&pos);
        }
        Game::terrainLib->setDetailedTerrainAsCurrent();
    }
}

void RouteEditorGLWidget::getUnsavedInfo(QVector<QString> &items) {
    if (this->route == NULL)
        return;
    route->getUnsavedInfo(items);
}

void RouteEditorGLWidget::msg(QString text) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "save") {
        route->save();
        timeSaved = timeNow;
        return;
    }
    if (text == "unselect") {
        setSelectedObj(NULL);
        lastSelectedObj = NULL;
        return;
    }
    if (text == "createPaths") {
        route->createNewPaths();
        return;
    }
    if (text == "resetPlaceRotation") {
        Quat::fill(this->placeRot);
        return;
    }
    if (text == "showTerrainTreeEditr") {
        TerrainTreeWindow ttWindow;
        ttWindow.exec();
        return;
    }
    if (text == "engItemSelected") {
        //QString pathid = ;
        //QString name = pathid.split("/").last();
        //QString texpath = pathid.left(pathid.length() - name.length());
        emit sendMsg("showShape", route->ref->selected->getShapePath());
    }
    if (text == "autoPlacementDeleteLast") {
        route->autoPlacementDeleteLast();
    }
    if (text == "editDetailedTerrain") {
        Game::terrainLib->setDetailedAsCurrent();
    }if (text == "editDistantTerrain") {
        Game::terrainLib->setDistantAsCurrent();
    }
}

void RouteEditorGLWidget::msg(QString text, bool val) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "stickToTDB") {
        this->route->placementStickToTarget = val;
        return;
    }
}

void RouteEditorGLWidget::msg(QString text, int val) {
}

void RouteEditorGLWidget::msg(QString text, float val) {
    if(Game::debugOutput) qDebug() << text;
    if (text == "autoPlacementLength") {
        this->route->placementAutoLength = val;
        return;
    }
}

void RouteEditorGLWidget::msg(QString text, QString val) {
    //qDebug() << text;
    if (text == "mkrFile") {
        this->route->setMkrFile(val);
        return;
    }
}

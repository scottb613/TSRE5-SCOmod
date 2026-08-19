/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "ShapeViewerGLWidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <math.h>
#include "GLUU.h"
#include "SFile.h"
#include "Eng.h"
#include "EngLib.h"
#include "Game.h"
#include "GLH.h"
#include "CameraFree.h"
#include "CameraConsist.h"
#include "GLMatrix.h"
#include "EngLib.h"
#include "ConLib.h"
#include "Consist.h" 
#include "ShapeLib.h"
#include "EngLib.h"
#include "ActLib.h"
#include "Activity.h"
#include "ShapeTextureInfo.h"

ShapeViewerGLWidget::ShapeViewerGLWidget(QWidget *parent)
: QOpenGLWidget(parent),
m_xRot(0),
m_yRot(0),
m_zRot(0) {
    backgroundGlColor[0] = -2;
    currentShapeLib = new ShapeLib();
}

ShapeViewerGLWidget::~ShapeViewerGLWidget() {

}

void ShapeViewerGLWidget::cleanup() {
    makeCurrent();
    //delete gluu->m_program;
    //gluu->m_program = 0;
    doneCurrent();
}

QSize ShapeViewerGLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize ShapeViewerGLWidget::sizeHint() const {
    return QSize(1500, 700);
}

void ShapeViewerGLWidget::timerEvent(QTimerEvent * event) {

    timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeNow - lastTime < 1)
        fps = 1;
    else
        fps = 1000.0 / (timeNow - lastTime);
    if (fps < 10) fps = 10;
    lastTime = timeNow;


    if (Game::allowObjLag < Game::maxObjLag)
        Game::allowObjLag += 2;

    camera->update(fps);
    update();
    
}

void ShapeViewerGLWidget::setCamera(Camera* cam){
    camera = cam;
}

void ShapeViewerGLWidget::initializeGL() {
    Game::currentShapeLib = currentShapeLib;
    /*if(currentEngLib == NULL){
         currentEngLib = new EngLib();
         Game::currentEngLib = currentEngLib;
    }*/
    //qDebug() << "GLUU::get();";
    gluu = GLUU::get();
    //context()->set
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ShapeViewerGLWidget::cleanup);
    //qDebug() << "initializeOpenGLFunctions();";
    initializeOpenGLFunctions();
    if(backgroundGlColor[0] == -2){
        backgroundGlColor[0] = 25.0/255;
        backgroundGlColor[1] = 25.0/255;
        backgroundGlColor[2] = 25.0/255;
        if(Game::systemTheme){
            backgroundGlColor[0] = 1;
            backgroundGlColor[1] = 1;
            backgroundGlColor[2] = 1;
        }
    }
    glClearColor(backgroundGlColor[0], backgroundGlColor[1], backgroundGlColor[2], 1);

    gluu->initShader();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);


    //sFile = new SFile("F:/TrainSim/trains/trainset/pkp_sp47/pkp_sp47-001.s", "F:/TrainSim/trains/trainset/pkp_sp47");
    //sFile = new SFile("f:/train simulator/routes/cmk/shapes/cottage3.s", "cottage3.s", "f:/train simulator/routes/cmk/textures");

    //eng = new Eng("F:/Train Simulator/trains/trainset/PKP-ST44-992/","PKP-ST44-992.eng");

    //qDebug() << eng->loaded;
    //sFile->Load("f:/train simulator/routes/cmk/shapes/cottage3.s");
    //tile = new Tile(-5303,-14963);
    //qDebug() << "route = new Route();";
    
    float * aaa = new float[2]{0,0};
    if(camera == NULL){
        camera = new CameraFree(aaa);
        float spos[3];
        spos[0] = 0; spos[1] = 0; spos[2] = 0;
        camera->setPos((float*)&spos);
    }
    
    lastTime = QDateTime::currentMSecsSinceEpoch();
    timer.start(15, this);
    setFocus();
    setMouseTracking(true);
}

void ShapeViewerGLWidget::setBackgroundGlColor(float r, float g, float b){
    backgroundGlColor[0] = r;
    backgroundGlColor[1] = g;
    backgroundGlColor[2] = b;
    if(renderItem == 3 && con != NULL){
        con->setTextColor(backgroundGlColor);
    }
}

void ShapeViewerGLWidget::fillCurrentShapeHierarchyInfo(ShapeHierarchyInfo* info){
    Game::currentShapeLib = currentShapeLib;
    if(renderItem == 4 && sFile != NULL){
        sFile->fillShapeHierarchyInfo(info);
    }
}

void ShapeViewerGLWidget::fillCurrentShapeTextureInfo(QHash<int, ShapeTextureInfo*>& list){
    Game::currentShapeLib = currentShapeLib;
    if(renderItem == 4 && sFile != NULL){
        sFile->fillShapeTextureInfo(list);
    }
}

void ShapeViewerGLWidget::fillCurrentContentHierarchyInfo(QVector<ContentHierarchyInfo*>& list){
    Game::currentShapeLib = currentShapeLib;
    if(renderItem == 4 && sFile != NULL){
        sFile->fillContentHierarchyInfo(list, -1);
    }
    if(renderItem == 2 && eng != NULL){
        eng->fillContentHierarchyInfo(list, -1);
    }
    if(renderItem == 5 && con != NULL){
        con->fillContentHierarchyInfo(list, -1);
    }
}

void ShapeViewerGLWidget::paintGL() {
    Game::currentShapeLib = currentShapeLib;
    //Game::currentEngLib = currentEngLib;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // disable shadows temporaily
    int shadowsState = Game::shadowsEnabled;
    Game::shadowsEnabled = 0;
    
    glClearColor(backgroundGlColor[0], backgroundGlColor[1], backgroundGlColor[2], 1);
    
    float aspect = float(this->width()) / float(this->height());
    float* lookAt = camera->getMatrix();
    Mat4::perspective(gluu->pMatrix, camera->fov*M_PI/180*(1/aspect), aspect, 0.2f, Game::objectLod);
    Mat4::multiply(gluu->pMatrix, gluu->pMatrix, lookAt);
    
    Mat4::perspective(gluu->fMatrix, camera->fov*M_PI/180*(1/aspect), aspect, 0.2f, Game::objectLod);
    Mat4::multiply(gluu->fMatrix, gluu->pMatrix, lookAt);
    
    Mat4::identity(gluu->mvMatrix);

    Mat4::identity(gluu->objStrMatrix);
    
    gluu->currentShader->bind();
    gluu->setMatrixUniforms();
    //sFile->render();
    if(mode == "rot"){
        Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, rotY, 0,1,0);
        Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, rotZ, 0,0,1);

    }

    if(selection && renderItem == 3 && con != NULL){
        // The consist is a one-dimensional roster. Build each unit's hit zone
        // from the same positions, lengths, camera and projection used for the
        // visible render. This avoids a second color-render/readback pass whose
        // alignment can drift progressively along a long consist.
        float consistProjection[16];
        Mat4::multiply(consistProjection, gluu->pMatrix, gluu->mvMatrix);
        const auto projectToWidget = [this, &consistProjection](
                                      float y, float z,
                                      QPointF &screenPoint) {
            const float *matrix = consistProjection;
            const float clipX = matrix[4] * y + matrix[8] * z + matrix[12];
            const float clipY = matrix[5] * y + matrix[9] * z + matrix[13];
            const float clipW = matrix[7] * y + matrix[11] * z + matrix[15];
            if(clipW <= 0.00001f)
                return false;

            screenPoint.setX((clipX / clipW + 1.0f) * 0.5f * width());
            screenPoint.setY((1.0f - clipY / clipW) * 0.5f * height());
            return true;
        };

        int selectedUnit = -1;
        qreal closestCenter = width() + 1.0;
        for(int i = 0; i < con->engItems.size(); ++i){
            const Consist::EngItem &item = con->engItems[i];
            const float unitLength = qMax(1.0f, item.previewWidth);
            float unitHeight = 4.5f;
            if(Game::currentEngLib != NULL){
                const auto engIt = Game::currentEngLib->eng.find(item.eng);
                if(!item.previewMissing
                        && engIt != Game::currentEngLib->eng.end()
                        && engIt->second != NULL
                        && engIt->second->sizey > 0.0f)
                    unitHeight = engIt->second->sizey;
            }

            QPointF firstEnd;
            QPointF secondEnd;
            QPointF bottom;
            QPointF top;
            QPointF center;
            const float centerY = unitHeight * 0.5f;
            if(!projectToWidget(centerY, item.pos - unitLength * 0.5f,
                                firstEnd)
                    || !projectToWidget(centerY,
                                        item.pos + unitLength * 0.5f,
                                        secondEnd)
                    || !projectToWidget(-0.25f, item.pos, bottom)
                    || !projectToWidget(unitHeight + 0.25f, item.pos, top)
                    || !projectToWidget(centerY, item.pos, center))
                continue;

            constexpr qreal hitTolerance = 3.0;
            const qreal left = qMin(firstEnd.x(), secondEnd.x())
                    - hitTolerance;
            const qreal right = qMax(firstEnd.x(), secondEnd.x())
                    + hitTolerance;
            const qreal upper = qMin(bottom.y(), top.y()) - hitTolerance;
            const qreal lower = qMax(bottom.y(), top.y()) + hitTolerance;
            if(mousex < left || mousex > right
                    || mousey < upper || mousey > lower)
                continue;

            const qreal centerDistance = qAbs(mousex - center.x());
            if(centerDistance < closestCenter){
                closestCenter = centerDistance;
                selectedUnit = i;
            }
        }

        selection = false;
        if(selectedUnit >= 0){
            con->select(selectedUnit);
            emit selected(selectedUnit);
        }
    }
    gluu->currentShader->setUniformValue(gluu->currentShader->mvMatrixUniform, *reinterpret_cast<float(*)[4][4]> (gluu->mvMatrix));
    
    if(renderItem == 2 && eng != NULL){
        eng->render((int)selection*65536);
    }
    if(renderItem == 3 && con != NULL){
        con->render((int)selection*65536, true);
    }
    if(renderItem == 5 && con != NULL){
        con->render((int)selection*65536, false);
    }
    if(renderItem == 2 && con != NULL){
        Mat4::rotate(gluu->mvMatrix, gluu->mvMatrix, M_PI, 0,1,0);
        Mat4::translate(gluu->mvMatrix, gluu->mvMatrix, 0, 0, -con->conLength/2);
        con->render((int)selection*65536);
    }
    if(renderItem == 4 && sFile != NULL){
        GLUU *gluu = GLUU::get();
        gluu->enableTextures();
        sFile->render();
        if(cameraInit){
            cameraInit = false;
            float max = 0;
            if(fabs(sFile->bound[0]-sFile->bound[1]) > max)
                max = fabs(sFile->bound[0]-sFile->bound[1]);
            if(fabs(sFile->bound[2]-sFile->bound[3]) > max)
                max = fabs(sFile->bound[2]-sFile->bound[3]);
            if(fabs(sFile->bound[4]-sFile->bound[5]) > max)
                max = fabs(sFile->bound[4]-sFile->bound[5]);
            //qDebug() << fabs(sFile->bound[0]-sFile->bound[1]);
            //qDebug() << fabs(sFile->bound[2]-sFile->bound[3]);
            //qDebug() << fabs(sFile->bound[4]-sFile->bound[5]);
            camera->setPos(-max*1.2,fabs(sFile->bound[2]-sFile->bound[3])/2.0,0.0);
        }
    }

    if(getImage){
        qDebug() << "get image";
        if(screenShot != NULL)
            delete screenShot;
        qDebug() << "new image";
        screenShot = NULL;
        getImage = false;
        int* viewport = new int[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        unsigned char* winZ = new unsigned char[viewport[2]*viewport[3]*4];
        glReadPixels(0, 0, viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, winZ);
        screenShot = new QImage(winZ, viewport[2], viewport[3], QImage::Format_RGBA8888 );
        
    }
    
    gluu->currentShader->release();
    Game::shadowsEnabled = shadowsState;
}

void ShapeViewerGLWidget::getImg() {
    getImage = true;
    return;
}

void ShapeViewerGLWidget::resizeGL(int w, int h) {
}

void ShapeViewerGLWidget::keyPressEvent(QKeyEvent * event) {
    camera->keyDown(event);
    Game::currentShapeLib = currentShapeLib;
    //Game::currentEngLib = currentEngLib;
    if(renderItem == 3 && con != NULL){
        switch (event->key()) {
            case Qt::Key_Delete:
                if(con != NULL)
                    con->deteleSelected();
                break;
            case Qt::Key_F:
                if(con != NULL)
                    con->flipSelected();
                break;
            case Qt::Key_Right:
                if(con != NULL)
                    con->moveRightSelected();
                break;
            case Qt::Key_Left:
                if(con != NULL)
                con->moveLeftSelected();
                break;
        }
        
        emit refreshItem();
    }
}

void ShapeViewerGLWidget::wheelEvent(QWheelEvent *event) {
    camera->MouseWheel(event);
    event->accept();
}

void ShapeViewerGLWidget::keyReleaseEvent(QKeyEvent * event) {
    camera->keyUp(event);
}

void ShapeViewerGLWidget::mousePressEvent(QMouseEvent *event) {
    m_lastPos = event->position().toPoint();
    mousex = m_lastPos.x();
    mousey = m_lastPos.y();
    mousePressed = true;
    selection = renderItem == 3 && con != NULL;
    if(event->button() == Qt::RightButton)
        mouseRPressed = true;
    if(event->button() == Qt::LeftButton)
        mouseLPressed = true;
    camera->MouseDown(event);
    setFocus();
    update();
}

void ShapeViewerGLWidget::mouseReleaseEvent(QMouseEvent* event) {
    camera->MouseUp(event);
    mousePressed = false;
    mouseRPressed = false;
    mouseLPressed = false;
    
    if ((event->button()) == Qt::RightButton) {
        // Complete the color-pick before building the menu so a quick
        // right-click always targets the card beneath the pointer.
        if(selection)
            repaint();
        showContextMenu(event->position().toPoint());
    }
}

void ShapeViewerGLWidget::mouseMoveEvent(QMouseEvent *event) {
    const QPoint currentPos = event->position().toPoint();
    mousex = currentPos.x();
    mousey = currentPos.y();
    
    if(mode == "rot"){
        if (mousePressed) {
            // Qt mouse positions are logical coordinates. Keep both ends of
            // the delta logical so display scaling cannot amplify rotation.
            // Normalize against the established 0.1 default so existing
            // users retain the intended speed while F12 can adjust it.
            constexpr float defaultMouseSpeed = 0.1f;
            const float rotationSpeedScale = Game::mouseSpeed / defaultMouseSpeed;
            if(mouseRPressed)
                rotZ += static_cast<float>(m_lastPos.y() - currentPos.y())
                        / 60.0f * (camera->fov / 45.0f) * rotationSpeedScale;
            if(mouseLPressed)
                rotY += static_cast<float>(m_lastPos.x() - currentPos.x())
                        / 60.0f * (camera->fov / 45.0f) * rotationSpeedScale;
            if (rotZ > 1.57)
                rotZ = (float) 1.57;
            if (rotZ < -1.57)
                rotZ = (float) - 1.57;
        }
    } else {
        camera->MouseMove(event);
    }
    m_lastPos = currentPos;
}

void ShapeViewerGLWidget::showContextMenu(const QPoint & point) {
    if(defaultMenuActions["flipSelected"] == NULL){
        defaultMenuActions["flipSelected"] = new QAction(tr("&Flip"), this); 
        QObject::connect(defaultMenuActions["flipSelected"], SIGNAL(triggered()), this, SLOT(flipConSelected()));
        defaultMenuActions["leftSelected"] = new QAction(tr("&Move Left"), this); 
        QObject::connect(defaultMenuActions["leftSelected"], SIGNAL(triggered()), this, SLOT(leftConSelected()));
        defaultMenuActions["rightSelected"] = new QAction(tr("&Move right"), this); 
        QObject::connect(defaultMenuActions["rightSelected"], SIGNAL(triggered()), this, SLOT(rightConSelected()));
        defaultMenuActions["deleteSelected"] = new QAction(tr("&Delete"), this); 
        QObject::connect(defaultMenuActions["deleteSelected"], SIGNAL(triggered()), this, SLOT(deleteConSelected()));
        defaultMenuActions["replaceSelected"] =
                new QAction(tr("&Swap Missing with Selected Stock"), this);
        QObject::connect(
            defaultMenuActions["replaceSelected"], SIGNAL(triggered()),
            this, SIGNAL(replaceSelectedUnitRequested()));
        defaultMenuActions["copyUnit"] = new QAction(tr("&Copy"), this); 
        QObject::connect(defaultMenuActions["copyUnit"], SIGNAL(triggered()), this, SLOT(copyUnitConSelected()));
        defaultMenuActions["pasteUnit"] = new QAction(tr("&Paste Right"), this); 
        QObject::connect(defaultMenuActions["pasteUnit"], SIGNAL(triggered()), this, SLOT(pasteUnitConSelected()));
    }
    
    if(renderItem == 3 && con != NULL){
        QMenu menu;

        QString menuStyle = QString(
            "QMenu::separator {\
              color: ")+Game::StyleMainLabel+";\
            }";
        menu.setStyleSheet(menuStyle);
        menu.addSection("Selected Unit");
        defaultMenuActions["replaceSelected"]->setEnabled(
            con->selectedIdx >= 0
            && con->selectedIdx < con->engItems.size()
            && con->engItems[con->selectedIdx].previewMissing);
        menu.addAction(defaultMenuActions["replaceSelected"]);
        menu.addSeparator();
        menu.addAction(defaultMenuActions["flipSelected"]);
        menu.addAction(defaultMenuActions["leftSelected"]);
        menu.addAction(defaultMenuActions["rightSelected"]);
        menu.addAction(defaultMenuActions["deleteSelected"]);
        menu.addAction(defaultMenuActions["copyUnit"]);
        menu.addAction(defaultMenuActions["pasteUnit"]);
        menu.exec(mapToGlobal(point));
    }
}

void ShapeViewerGLWidget::resetRot(){
    rotY = M_PI;
    rotZ = 0;
}


void ShapeViewerGLWidget::resetCamRoster(float profile) {
    rotY = (M_PI/profile);
    rotZ = 0;
}

void ShapeViewerGLWidget::showEng(Eng *e){
    // if(Game::debugOutput) qDebug() << "SVGLW 429: engObj " ;
    eng = e;    
    con = NULL;
    renderItem = 2;
}

void ShapeViewerGLWidget::showEng(QString path, QString name){
    // if(Game::debugOutput) qDebug() << "SVGLW 436:" << path << " " << name;
    int idx = Game::currentEngLib->addEng(path, name);
    // if(Game::debugOutput) qDebug() << "eng id "<< idx;
    eng = Game::currentEngLib->eng[idx];
    con = NULL;
    renderItem = 2;
}

void ShapeViewerGLWidget::showEngSet(int id){
    Game::currentShapeLib = currentShapeLib;
    // qDebug() << "eng set id "<< id;
    con = ConLib::con[id];
    con->setTextColor(backgroundGlColor);
    eng = NULL;
    renderItem = 2;
}

void ShapeViewerGLWidget::flipConSelected(){
    if(con != NULL){
        con->flipSelected();
        emit refreshItem();
    }
}

void ShapeViewerGLWidget::leftConSelected(){
    if(con != NULL){
        con->moveLeftSelected();
        emit refreshItem();
    }
}

void ShapeViewerGLWidget::rightConSelected(){
    if(con != NULL){
        con->moveRightSelected();
        emit refreshItem();
    }
}

void ShapeViewerGLWidget::deleteConSelected(){
    if(con != NULL){
        con->deteleSelected();
        emit refreshItem();
    }
}

void ShapeViewerGLWidget::copyUnitConSelected(){
    if(con == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString("ENGID ")+QString::number(con->getSelectedEngId()));
}

void ShapeViewerGLWidget::pasteUnitConSelected(){
    if(con == NULL)
        return;
    QClipboard *clipboard = QApplication::clipboard();
    QStringList args = clipboard->text().split(" ");
    if(args[0] != "ENGID")
        return;
    if(args.size() != 2)
        return;
    int val = args[1].toInt();
    con->appendEngItem(val, 1, false);
    
    emit refreshItem();
}

void ShapeViewerGLWidget::showCon(int id){
    if(id < 0){
        con = NULL;
        eng = NULL;
        sFile = NULL;
        renderItem = 3;
        return;
    }
    // qDebug() << "con id "<< id;
    con = ConLib::con[id];
    con->setTextColor(backgroundGlColor);
    eng = NULL;
    renderItem = 3;
}

void ShapeViewerGLWidget::showConSimple(Consist *currentCon){
    if(currentCon == NULL){
        con = NULL;
        eng = NULL;
        sFile = NULL;
        renderItem = 5;
        return;
    }
    con = currentCon;
    con->setTextColor(backgroundGlColor);
    eng = NULL;
    renderItem = 5;
}

void ShapeViewerGLWidget::showConSimple(int id){
    if(id < 0){
        con = NULL;
        eng = NULL;
        sFile = NULL;
        renderItem = 5;
        return;
    }
    // qDebug() << "con id "<< id;
    con = ConLib::con[id];
    con->setTextColor(backgroundGlColor);
    eng = NULL;
    renderItem = 5;
}

void ShapeViewerGLWidget::showCon(int aid, int id){
    // qDebug() << "con aid "<< aid<< " con id "<< id;
    con = ActLib::Act[aid]->activityObjects[id]->con;
    con->setTextColor(backgroundGlColor);
    renderItem = 3;
}

void ShapeViewerGLWidget::showShape(QString path, QString texPath, SFile **currentSFile){
    int shapeId;
    if(texPath.length() > 0)
        shapeId = currentShapeLib->addShape(path, texPath);
    else
        shapeId = currentShapeLib->addShape(path);
    if(shapeId < 0){
        sFile = NULL;
    } else {
        sFile = currentShapeLib->shape[shapeId];
        if(currentSFile != NULL)
            *currentSFile = sFile;
        cameraInit = true;
    }
    renderItem = 4;
    con = NULL;
    eng = NULL;
}

void ShapeViewerGLWidget::showShape(SFile *currentSFile){
    if(currentSFile == NULL)
        return;
    sFile = currentSFile;
    cameraInit = true;
    renderItem = 4;
    con = NULL;
    eng = NULL;
    resetCamRoster(2);
}

void ShapeViewerGLWidget::setMode(QString n){
    mode = n;
}


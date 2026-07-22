/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
//#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QBasicTimer>
#include <QJsonObject>
#include "CameraFree.h"
#include "CameraConsist.h"
#include "WorldObj.h"
#include "GroupObj.h"
#include "Pointer3d.h"
#include "Ref.h"
#include <unordered_map>

class Tile;
class SFile;
class Eng;
class GLUU;
class Route;
class Brush;
class PreciseTileCoordinate;
class Coords;
class MapWindow;
class ShapeLib;
class EngLib;
class QOpenGLFunctions_3_3_Core;
class QAction;
class GuiGlCompass;
class QFrame;

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class RouteEditorGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    RouteEditorGLWidget(QWidget *parent = 0);
    ~RouteEditorGLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
    
    bool initRoute();
    void cameraInit();
    QJsonObject getSessionCameraState() const;
    void playInit();
    
    void getUnsavedInfo(QVector<QString> &items);

public slots:
    void cleanup();
    void enableTool(QString name);
    void userModeChangeSound();
    void userJumpSound();
    void setPaintBrush(Brush* brush);
    void jumpTo(PreciseTileCoordinate*);
    void jumpTo(float *posT, float *pos);
    void jumpTo(int X, int Z, float x, float y, float z);
    
    void msg(QString text);
    void msg(QString name, bool val);
    void msg(QString name, int val);
    void msg(QString name, float val);
    void msg(QString name, QString val);
    
    void editCopy();
    void copySelectionInfo();
    void editPaste();
    void editSelect();
    void editFind1x1();
    void editFind3x3();
    void editFind(int radius = 0);
    void editUndo();
    void showTrkEditr();
    
    void showContextMenu(const QPoint & point);
    void createNewTiles(QMap<int, QPair<int, int>*> list);
    void createNewLoTiles(QMap<int, QPair<int, int>*> list);
    void objectSelected(GameObj* obj);
    void objectSelected(QVector<GameObj*> obj);
    
    void selectToolresetMoveStep();
    void selectToolresetRot();
    void selectToolresetVert();    
    void selectToolSelect();
    void selectToolRotate();
    void selectToolTranslate();
    void selectToolScale();
    void toolBrushDirectionUp();
    void toolBrushDirectionDown();
    void putTerrainTexToolSelectRandom();
    void putTerrainTexToolSelectPresent();
    void putTerrainTexToolSelect0();
    void putTerrainTexToolSelect90();
    void putTerrainTexToolSelect180();
    void putTerrainTexToolSelect270();
    void placeToolStickTerrain();
    void placeToolStickAll();
    void reloadRefFile();
    void reloadMkrFiles();
    void setCameraObject(GameObj* obj);
    void setMoveStep(float val);
    void statusPanelCommand(QString name);
    void flexResult(bool success);
    void focusEditor();
    void paintToolObj();
    void paintToolObjSelected();
    void paintToolTDB();
    void paintToolTDBVector();
    void paintToolTileTrack();
    void paintToolTileRoad();
    void paintToolWaterEdges();
    void paintToolResetTile();
    void setTerrainToObj();
    void smoothTerrainToObj();
    void setTerrainToNearestDbTile();
    void setTerrainToSelectedObjTile();
    void selectAllTerrainPatchesOnSelectedTile();
    void adjustObjPositionToTerrainMenu();
    void adjustObjRotationToTerrainMenu();
    void pickObjForPlacement();
    void pickObjRotForPlacement();
    void pickObjRotElevForPlacement();
    void pickObjRotForCamera();
    void pickObjRotForCameraFlip();
    void resetCamN(); void resetCamS(); void resetCamE(); void resetCamW(); void resetCamD(); void resetCamZ(); 
    void tangentOrigin(); void tangentTarget(); void tangentMath(); void TangentApplyRot();

    void initRoute2(); 
     
signals:
    void showWindow();
    void routeLoaded(Route * a);
    void itemSelected(Ref::RefItem* pointer);
    void naviInfo(int all, int hidden);
    void posInfo(PreciseTileCoordinate* pos);
    void pointerInfo(float* pos);
    void setToolbox(QString name);
    void setBrushTextureId(int val);
    void showProperties(GameObj* obj);
    void updateProperties(GameObj* obj);
    void flexData(int x, int z, float* p);
    void mkrList(QMap<QString, Coords*> list);
    void refreshObjLists();
    void reloadMkrLists();
    
    void sendMsg(QString name);
    void sendMsg(QString name, bool val);
    void sendMsg(QString name, int val);
    void sendMsg(QString name, float val);
    void sendMsg(QString name, QString val);
    
    // EFO Status Updates
    void updStatus(QString statName, QString statValue);
    void preloadTexturesSignal();
    void resetGradeHelperRequested();

    
protected:
    bool eventFilter(QObject *object, QEvent *event);
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void paintGL2();
    void renderShadowMaps();
    void handleSelection();
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
    void drawPointer();
    void pushRenderPointer();
private:
    void setupVertexAttribs();
    void setSelectedObj(GameObj* o);
    bool validatePlacement(WorldObj* obj, Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ);
    bool pointerNearPlacementDb(Ref::RefItem* item, const float* pointerPos, int pointerTileX, int pointerTileZ);
    void playPlacementSound(QString fileName);
    float trackGradePercent(GameObj *obj) const;
    bool applyGradeLockToPlacedTrack(bool previousTrackValid, int previousX, int previousY,
                                     unsigned int previousUid, float previousGrade, bool &gradeAchieved);
    void showModeChange();
    void showPlacementSuccess();
    void showPlacementGuardError();
    void rejectPlacement();
    QBasicTimer timer;
    unsigned long long int lastTime = 0;
    unsigned long long int timeNow = 0;
    unsigned long long int timeSaved = 0;
    unsigned long long int lastDisplayInfoUpdate = 0;
    unsigned long long int lastPointerDepthRead = 0;
    bool m_core;
    int m_xRot;
    int m_yRot;
    int m_zRot;
    int fps;
    QPoint m_lastPos;
    SFile* sFile;
    Eng* eng;
    Tile* tile;
    Route* route = NULL;
    GLUU* gluu;
    QOpenGLFunctions_3_3_Core* funcs = 0;
    unsigned int fbo[3];
    bool m_transparent;
    Camera* camera = NULL;
    CameraFree* cameraFree = NULL;
    CameraConsist* cameraObj = NULL;
    bool selection = false;
    int mousex, mousey;
    int pointerDepthMouseX = -1;
    int pointerDepthMouseY = -1;
    int pointerDepthTileX = 0;
    int pointerDepthTileZ = 0;
    float pointerDepthCameraPos[3] = {0.0f, 0.0f, 0.0f};
    float pointerDepthCameraTarget[3] = {0.0f, 0.0f, 0.0f};
    bool pointerDepthValid = false;
    GameObj* selectedObj = NULL;
    bool placeGuardEnabled = true;
    GameObj* StartObject = NULL;
    GameObj* EndObject = NULL;    
    GameObj* lastSelectedObj = NULL;
    WorldObj* CamObj = NULL;
    WorldObj* copyPasteObj = NULL;
    GroupObj* groupObj = NULL;
    GroupObj* copyPasteGroupObj = NULL;
    Pointer3d* pointer3d;
    float lastPointerPos[3];
    float aktPointerPos[3];
    bool mouseLPressed = false;
    bool mouseRPressed = false;
    bool mouseClick = false;
    QString toolEnabled = "";
    float defaultMoveStep = 0.25;
    float moveStep = 0.25;
    //float moveUltraStep = 2.0;
    float moveMaxStep = 0.25;
    //float moveMinStep = 0.01;
    
    bool resizeTool = false;
    bool rotateTool = false;
    bool translateTool = false;

    bool stickPointerToTerrain = true;
    bool autoAddToTDB = true;
    float lastNewObjPos[3];
    float lastNewObjPosT[2];
    int   placeTile[2];
    float placeRot[4];
    float placeElev = 0;
    long long int lastMousePressTime = 0;
    bool keyControlEnabled = false;
    bool keyShiftEnabled = false;
    int cameraMoveSpeedLock = 0;
    bool keyAltEnabled = false;
    GLuint FramebufferName1 = 0;
    GLuint depthTexture1 = 0;
    GLuint FramebufferName2 = 0;
    GLuint depthTexture2 = 0;
    Brush* defaultPaintBrush;
    MapWindow* mapWindow;
    ShapeLib *currentShapeLib = NULL;    
    EngLib *engLib = NULL;
    
    /*struct DefaultMenuActions {
        QAction *undo;
        QAction *copy;
        QAction *paste;
        QAction *find1x1;
        QAction *find3x3;
        QAction *select;
        void init(RouteEditorGLWidget *widget);
    };*/
    //DefaultMenuActions defaultMenuActions;
    QMap<QString, QAction*> defaultMenuActions;
    bool bolckContextMenu = false;
    
    GuiGlCompass * compass = NULL;
    OglObj * compassPointer = NULL;
    
    
};

#endif

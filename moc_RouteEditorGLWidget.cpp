/****************************************************************************
** Meta object code from reading C++ file 'RouteEditorGLWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.19)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "RouteEditorGLWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QVector>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RouteEditorGLWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.19. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RouteEditorGLWidget_t {
    QByteArrayData data[126];
    char stringdata0[1785];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RouteEditorGLWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RouteEditorGLWidget_t qt_meta_stringdata_RouteEditorGLWidget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "RouteEditorGLWidget"
QT_MOC_LITERAL(1, 20, 10), // "showWindow"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 11), // "routeLoaded"
QT_MOC_LITERAL(4, 44, 6), // "Route*"
QT_MOC_LITERAL(5, 51, 1), // "a"
QT_MOC_LITERAL(6, 53, 12), // "itemSelected"
QT_MOC_LITERAL(7, 66, 13), // "Ref::RefItem*"
QT_MOC_LITERAL(8, 80, 7), // "pointer"
QT_MOC_LITERAL(9, 88, 8), // "naviInfo"
QT_MOC_LITERAL(10, 97, 3), // "all"
QT_MOC_LITERAL(11, 101, 6), // "hidden"
QT_MOC_LITERAL(12, 108, 7), // "posInfo"
QT_MOC_LITERAL(13, 116, 22), // "PreciseTileCoordinate*"
QT_MOC_LITERAL(14, 139, 3), // "pos"
QT_MOC_LITERAL(15, 143, 11), // "pointerInfo"
QT_MOC_LITERAL(16, 155, 6), // "float*"
QT_MOC_LITERAL(17, 162, 10), // "setToolbox"
QT_MOC_LITERAL(18, 173, 4), // "name"
QT_MOC_LITERAL(19, 178, 17), // "setBrushTextureId"
QT_MOC_LITERAL(20, 196, 3), // "val"
QT_MOC_LITERAL(21, 200, 14), // "showProperties"
QT_MOC_LITERAL(22, 215, 8), // "GameObj*"
QT_MOC_LITERAL(23, 224, 3), // "obj"
QT_MOC_LITERAL(24, 228, 16), // "updateProperties"
QT_MOC_LITERAL(25, 245, 8), // "flexData"
QT_MOC_LITERAL(26, 254, 1), // "x"
QT_MOC_LITERAL(27, 256, 1), // "z"
QT_MOC_LITERAL(28, 258, 1), // "p"
QT_MOC_LITERAL(29, 260, 7), // "mkrList"
QT_MOC_LITERAL(30, 268, 21), // "QMap<QString,Coords*>"
QT_MOC_LITERAL(31, 290, 4), // "list"
QT_MOC_LITERAL(32, 295, 15), // "refreshObjLists"
QT_MOC_LITERAL(33, 311, 14), // "reloadMkrLists"
QT_MOC_LITERAL(34, 326, 7), // "sendMsg"
QT_MOC_LITERAL(35, 334, 9), // "updStatus"
QT_MOC_LITERAL(36, 344, 8), // "statName"
QT_MOC_LITERAL(37, 353, 9), // "statValue"
QT_MOC_LITERAL(38, 363, 21), // "preloadTexturesSignal"
QT_MOC_LITERAL(39, 385, 25), // "resetGradeHelperRequested"
QT_MOC_LITERAL(40, 411, 7), // "cleanup"
QT_MOC_LITERAL(41, 419, 10), // "enableTool"
QT_MOC_LITERAL(42, 430, 19), // "userModeChangeSound"
QT_MOC_LITERAL(43, 450, 13), // "userJumpSound"
QT_MOC_LITERAL(44, 464, 13), // "setPaintBrush"
QT_MOC_LITERAL(45, 478, 6), // "Brush*"
QT_MOC_LITERAL(46, 485, 5), // "brush"
QT_MOC_LITERAL(47, 491, 6), // "jumpTo"
QT_MOC_LITERAL(48, 498, 4), // "posT"
QT_MOC_LITERAL(49, 503, 1), // "X"
QT_MOC_LITERAL(50, 505, 1), // "Z"
QT_MOC_LITERAL(51, 507, 1), // "y"
QT_MOC_LITERAL(52, 509, 3), // "msg"
QT_MOC_LITERAL(53, 513, 4), // "text"
QT_MOC_LITERAL(54, 518, 8), // "editCopy"
QT_MOC_LITERAL(55, 527, 9), // "editPaste"
QT_MOC_LITERAL(56, 537, 10), // "editSelect"
QT_MOC_LITERAL(57, 548, 11), // "editFind1x1"
QT_MOC_LITERAL(58, 560, 11), // "editFind3x3"
QT_MOC_LITERAL(59, 572, 8), // "editFind"
QT_MOC_LITERAL(60, 581, 6), // "radius"
QT_MOC_LITERAL(61, 588, 8), // "editUndo"
QT_MOC_LITERAL(62, 597, 12), // "showTrkEditr"
QT_MOC_LITERAL(63, 610, 15), // "showContextMenu"
QT_MOC_LITERAL(64, 626, 5), // "point"
QT_MOC_LITERAL(65, 632, 14), // "createNewTiles"
QT_MOC_LITERAL(66, 647, 25), // "QMap<int,QPair<int,int>*>"
QT_MOC_LITERAL(67, 673, 16), // "createNewLoTiles"
QT_MOC_LITERAL(68, 690, 14), // "objectSelected"
QT_MOC_LITERAL(69, 705, 17), // "QVector<GameObj*>"
QT_MOC_LITERAL(70, 723, 23), // "selectToolresetMoveStep"
QT_MOC_LITERAL(71, 747, 18), // "selectToolresetRot"
QT_MOC_LITERAL(72, 766, 19), // "selectToolresetVert"
QT_MOC_LITERAL(73, 786, 16), // "selectToolSelect"
QT_MOC_LITERAL(74, 803, 16), // "selectToolRotate"
QT_MOC_LITERAL(75, 820, 19), // "selectToolTranslate"
QT_MOC_LITERAL(76, 840, 15), // "selectToolScale"
QT_MOC_LITERAL(77, 856, 20), // "toolBrushDirectionUp"
QT_MOC_LITERAL(78, 877, 22), // "toolBrushDirectionDown"
QT_MOC_LITERAL(79, 900, 29), // "putTerrainTexToolSelectRandom"
QT_MOC_LITERAL(80, 930, 30), // "putTerrainTexToolSelectPresent"
QT_MOC_LITERAL(81, 961, 24), // "putTerrainTexToolSelect0"
QT_MOC_LITERAL(82, 986, 25), // "putTerrainTexToolSelect90"
QT_MOC_LITERAL(83, 1012, 26), // "putTerrainTexToolSelect180"
QT_MOC_LITERAL(84, 1039, 26), // "putTerrainTexToolSelect270"
QT_MOC_LITERAL(85, 1066, 21), // "placeToolStickTerrain"
QT_MOC_LITERAL(86, 1088, 17), // "placeToolStickAll"
QT_MOC_LITERAL(87, 1106, 13), // "reloadRefFile"
QT_MOC_LITERAL(88, 1120, 14), // "reloadMkrFiles"
QT_MOC_LITERAL(89, 1135, 15), // "setCameraObject"
QT_MOC_LITERAL(90, 1151, 11), // "setMoveStep"
QT_MOC_LITERAL(91, 1163, 18), // "statusPanelCommand"
QT_MOC_LITERAL(92, 1182, 10), // "flexResult"
QT_MOC_LITERAL(93, 1193, 7), // "success"
QT_MOC_LITERAL(94, 1201, 11), // "focusEditor"
QT_MOC_LITERAL(95, 1213, 12), // "paintToolObj"
QT_MOC_LITERAL(96, 1226, 20), // "paintToolObjSelected"
QT_MOC_LITERAL(97, 1247, 12), // "paintToolTDB"
QT_MOC_LITERAL(98, 1260, 18), // "paintToolTDBVector"
QT_MOC_LITERAL(99, 1279, 18), // "paintToolTileTrack"
QT_MOC_LITERAL(100, 1298, 17), // "paintToolTileRoad"
QT_MOC_LITERAL(101, 1316, 19), // "paintToolWaterEdges"
QT_MOC_LITERAL(102, 1336, 18), // "paintToolResetTile"
QT_MOC_LITERAL(103, 1355, 15), // "setTerrainToObj"
QT_MOC_LITERAL(104, 1371, 18), // "smoothTerrainToObj"
QT_MOC_LITERAL(105, 1390, 25), // "setTerrainToNearestDbTile"
QT_MOC_LITERAL(106, 1416, 27), // "setTerrainToSelectedObjTile"
QT_MOC_LITERAL(107, 1444, 37), // "selectAllTerrainPatchesOnSele..."
QT_MOC_LITERAL(108, 1482, 30), // "adjustObjPositionToTerrainMenu"
QT_MOC_LITERAL(109, 1513, 30), // "adjustObjRotationToTerrainMenu"
QT_MOC_LITERAL(110, 1544, 19), // "pickObjForPlacement"
QT_MOC_LITERAL(111, 1564, 22), // "pickObjRotForPlacement"
QT_MOC_LITERAL(112, 1587, 26), // "pickObjRotElevForPlacement"
QT_MOC_LITERAL(113, 1614, 19), // "pickObjRotForCamera"
QT_MOC_LITERAL(114, 1634, 23), // "pickObjRotForCameraFlip"
QT_MOC_LITERAL(115, 1658, 9), // "resetCamN"
QT_MOC_LITERAL(116, 1668, 9), // "resetCamS"
QT_MOC_LITERAL(117, 1678, 9), // "resetCamE"
QT_MOC_LITERAL(118, 1688, 9), // "resetCamW"
QT_MOC_LITERAL(119, 1698, 9), // "resetCamD"
QT_MOC_LITERAL(120, 1708, 9), // "resetCamZ"
QT_MOC_LITERAL(121, 1718, 13), // "tangentOrigin"
QT_MOC_LITERAL(122, 1732, 13), // "tangentTarget"
QT_MOC_LITERAL(123, 1746, 11), // "tangentMath"
QT_MOC_LITERAL(124, 1758, 15), // "TangentApplyRot"
QT_MOC_LITERAL(125, 1774, 10) // "initRoute2"

    },
    "RouteEditorGLWidget\0showWindow\0\0"
    "routeLoaded\0Route*\0a\0itemSelected\0"
    "Ref::RefItem*\0pointer\0naviInfo\0all\0"
    "hidden\0posInfo\0PreciseTileCoordinate*\0"
    "pos\0pointerInfo\0float*\0setToolbox\0"
    "name\0setBrushTextureId\0val\0showProperties\0"
    "GameObj*\0obj\0updateProperties\0flexData\0"
    "x\0z\0p\0mkrList\0QMap<QString,Coords*>\0"
    "list\0refreshObjLists\0reloadMkrLists\0"
    "sendMsg\0updStatus\0statName\0statValue\0"
    "preloadTexturesSignal\0resetGradeHelperRequested\0"
    "cleanup\0enableTool\0userModeChangeSound\0"
    "userJumpSound\0setPaintBrush\0Brush*\0"
    "brush\0jumpTo\0posT\0X\0Z\0y\0msg\0text\0"
    "editCopy\0editPaste\0editSelect\0editFind1x1\0"
    "editFind3x3\0editFind\0radius\0editUndo\0"
    "showTrkEditr\0showContextMenu\0point\0"
    "createNewTiles\0QMap<int,QPair<int,int>*>\0"
    "createNewLoTiles\0objectSelected\0"
    "QVector<GameObj*>\0selectToolresetMoveStep\0"
    "selectToolresetRot\0selectToolresetVert\0"
    "selectToolSelect\0selectToolRotate\0"
    "selectToolTranslate\0selectToolScale\0"
    "toolBrushDirectionUp\0toolBrushDirectionDown\0"
    "putTerrainTexToolSelectRandom\0"
    "putTerrainTexToolSelectPresent\0"
    "putTerrainTexToolSelect0\0"
    "putTerrainTexToolSelect90\0"
    "putTerrainTexToolSelect180\0"
    "putTerrainTexToolSelect270\0"
    "placeToolStickTerrain\0placeToolStickAll\0"
    "reloadRefFile\0reloadMkrFiles\0"
    "setCameraObject\0setMoveStep\0"
    "statusPanelCommand\0flexResult\0success\0"
    "focusEditor\0paintToolObj\0paintToolObjSelected\0"
    "paintToolTDB\0paintToolTDBVector\0"
    "paintToolTileTrack\0paintToolTileRoad\0"
    "paintToolWaterEdges\0paintToolResetTile\0"
    "setTerrainToObj\0smoothTerrainToObj\0"
    "setTerrainToNearestDbTile\0"
    "setTerrainToSelectedObjTile\0"
    "selectAllTerrainPatchesOnSelectedTile\0"
    "adjustObjPositionToTerrainMenu\0"
    "adjustObjRotationToTerrainMenu\0"
    "pickObjForPlacement\0pickObjRotForPlacement\0"
    "pickObjRotElevForPlacement\0"
    "pickObjRotForCamera\0pickObjRotForCameraFlip\0"
    "resetCamN\0resetCamS\0resetCamE\0resetCamW\0"
    "resetCamD\0resetCamZ\0tangentOrigin\0"
    "tangentTarget\0tangentMath\0TangentApplyRot\0"
    "initRoute2"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RouteEditorGLWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
     104,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      22,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  534,    2, 0x06 /* Public */,
       3,    1,  535,    2, 0x06 /* Public */,
       6,    1,  538,    2, 0x06 /* Public */,
       9,    2,  541,    2, 0x06 /* Public */,
      12,    1,  546,    2, 0x06 /* Public */,
      15,    1,  549,    2, 0x06 /* Public */,
      17,    1,  552,    2, 0x06 /* Public */,
      19,    1,  555,    2, 0x06 /* Public */,
      21,    1,  558,    2, 0x06 /* Public */,
      24,    1,  561,    2, 0x06 /* Public */,
      25,    3,  564,    2, 0x06 /* Public */,
      29,    1,  571,    2, 0x06 /* Public */,
      32,    0,  574,    2, 0x06 /* Public */,
      33,    0,  575,    2, 0x06 /* Public */,
      34,    1,  576,    2, 0x06 /* Public */,
      34,    2,  579,    2, 0x06 /* Public */,
      34,    2,  584,    2, 0x06 /* Public */,
      34,    2,  589,    2, 0x06 /* Public */,
      34,    2,  594,    2, 0x06 /* Public */,
      35,    2,  599,    2, 0x06 /* Public */,
      38,    0,  604,    2, 0x06 /* Public */,
      39,    0,  605,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      40,    0,  606,    2, 0x0a /* Public */,
      41,    1,  607,    2, 0x0a /* Public */,
      42,    0,  610,    2, 0x0a /* Public */,
      43,    0,  611,    2, 0x0a /* Public */,
      44,    1,  612,    2, 0x0a /* Public */,
      47,    1,  615,    2, 0x0a /* Public */,
      47,    2,  618,    2, 0x0a /* Public */,
      47,    5,  623,    2, 0x0a /* Public */,
      52,    1,  634,    2, 0x0a /* Public */,
      52,    2,  637,    2, 0x0a /* Public */,
      52,    2,  642,    2, 0x0a /* Public */,
      52,    2,  647,    2, 0x0a /* Public */,
      52,    2,  652,    2, 0x0a /* Public */,
      54,    0,  657,    2, 0x0a /* Public */,
      55,    0,  658,    2, 0x0a /* Public */,
      56,    0,  659,    2, 0x0a /* Public */,
      57,    0,  660,    2, 0x0a /* Public */,
      58,    0,  661,    2, 0x0a /* Public */,
      59,    1,  662,    2, 0x0a /* Public */,
      59,    0,  665,    2, 0x2a /* Public | MethodCloned */,
      61,    0,  666,    2, 0x0a /* Public */,
      62,    0,  667,    2, 0x0a /* Public */,
      63,    1,  668,    2, 0x0a /* Public */,
      65,    1,  671,    2, 0x0a /* Public */,
      67,    1,  674,    2, 0x0a /* Public */,
      68,    1,  677,    2, 0x0a /* Public */,
      68,    1,  680,    2, 0x0a /* Public */,
      70,    0,  683,    2, 0x0a /* Public */,
      71,    0,  684,    2, 0x0a /* Public */,
      72,    0,  685,    2, 0x0a /* Public */,
      73,    0,  686,    2, 0x0a /* Public */,
      74,    0,  687,    2, 0x0a /* Public */,
      75,    0,  688,    2, 0x0a /* Public */,
      76,    0,  689,    2, 0x0a /* Public */,
      77,    0,  690,    2, 0x0a /* Public */,
      78,    0,  691,    2, 0x0a /* Public */,
      79,    0,  692,    2, 0x0a /* Public */,
      80,    0,  693,    2, 0x0a /* Public */,
      81,    0,  694,    2, 0x0a /* Public */,
      82,    0,  695,    2, 0x0a /* Public */,
      83,    0,  696,    2, 0x0a /* Public */,
      84,    0,  697,    2, 0x0a /* Public */,
      85,    0,  698,    2, 0x0a /* Public */,
      86,    0,  699,    2, 0x0a /* Public */,
      87,    0,  700,    2, 0x0a /* Public */,
      88,    0,  701,    2, 0x0a /* Public */,
      89,    1,  702,    2, 0x0a /* Public */,
      90,    1,  705,    2, 0x0a /* Public */,
      91,    1,  708,    2, 0x0a /* Public */,
      92,    1,  711,    2, 0x0a /* Public */,
      94,    0,  714,    2, 0x0a /* Public */,
      95,    0,  715,    2, 0x0a /* Public */,
      96,    0,  716,    2, 0x0a /* Public */,
      97,    0,  717,    2, 0x0a /* Public */,
      98,    0,  718,    2, 0x0a /* Public */,
      99,    0,  719,    2, 0x0a /* Public */,
     100,    0,  720,    2, 0x0a /* Public */,
     101,    0,  721,    2, 0x0a /* Public */,
     102,    0,  722,    2, 0x0a /* Public */,
     103,    0,  723,    2, 0x0a /* Public */,
     104,    0,  724,    2, 0x0a /* Public */,
     105,    0,  725,    2, 0x0a /* Public */,
     106,    0,  726,    2, 0x0a /* Public */,
     107,    0,  727,    2, 0x0a /* Public */,
     108,    0,  728,    2, 0x0a /* Public */,
     109,    0,  729,    2, 0x0a /* Public */,
     110,    0,  730,    2, 0x0a /* Public */,
     111,    0,  731,    2, 0x0a /* Public */,
     112,    0,  732,    2, 0x0a /* Public */,
     113,    0,  733,    2, 0x0a /* Public */,
     114,    0,  734,    2, 0x0a /* Public */,
     115,    0,  735,    2, 0x0a /* Public */,
     116,    0,  736,    2, 0x0a /* Public */,
     117,    0,  737,    2, 0x0a /* Public */,
     118,    0,  738,    2, 0x0a /* Public */,
     119,    0,  739,    2, 0x0a /* Public */,
     120,    0,  740,    2, 0x0a /* Public */,
     121,    0,  741,    2, 0x0a /* Public */,
     122,    0,  742,    2, 0x0a /* Public */,
     123,    0,  743,    2, 0x0a /* Public */,
     124,    0,  744,    2, 0x0a /* Public */,
     125,    0,  745,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   10,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 16,   14,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void, QMetaType::Int,   20,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, 0x80000000 | 16,   26,   27,   28,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Float,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   36,   37,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 45,   46,
    QMetaType::Void, 0x80000000 | 13,    2,
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 16,   48,   14,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Float, QMetaType::Float, QMetaType::Float,   49,   50,   26,   51,   27,
    QMetaType::Void, QMetaType::QString,   53,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Float,   18,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   18,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   60,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   64,
    QMetaType::Void, 0x80000000 | 66,   31,
    QMetaType::Void, 0x80000000 | 66,   31,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, 0x80000000 | 69,   23,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, QMetaType::Float,   20,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void, QMetaType::Bool,   93,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void RouteEditorGLWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RouteEditorGLWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->showWindow(); break;
        case 1: _t->routeLoaded((*reinterpret_cast< Route*(*)>(_a[1]))); break;
        case 2: _t->itemSelected((*reinterpret_cast< Ref::RefItem*(*)>(_a[1]))); break;
        case 3: _t->naviInfo((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->posInfo((*reinterpret_cast< PreciseTileCoordinate*(*)>(_a[1]))); break;
        case 5: _t->pointerInfo((*reinterpret_cast< float*(*)>(_a[1]))); break;
        case 6: _t->setToolbox((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->setBrushTextureId((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->showProperties((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 9: _t->updateProperties((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 10: _t->flexData((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< float*(*)>(_a[3]))); break;
        case 11: _t->mkrList((*reinterpret_cast< QMap<QString,Coords*>(*)>(_a[1]))); break;
        case 12: _t->refreshObjLists(); break;
        case 13: _t->reloadMkrLists(); break;
        case 14: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 15: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 16: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 17: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 18: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 19: _t->updStatus((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 20: _t->preloadTexturesSignal(); break;
        case 21: _t->resetGradeHelperRequested(); break;
        case 22: _t->cleanup(); break;
        case 23: _t->enableTool((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 24: _t->userModeChangeSound(); break;
        case 25: _t->userJumpSound(); break;
        case 26: _t->setPaintBrush((*reinterpret_cast< Brush*(*)>(_a[1]))); break;
        case 27: _t->jumpTo((*reinterpret_cast< PreciseTileCoordinate*(*)>(_a[1]))); break;
        case 28: _t->jumpTo((*reinterpret_cast< float*(*)>(_a[1])),(*reinterpret_cast< float*(*)>(_a[2]))); break;
        case 29: _t->jumpTo((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5]))); break;
        case 30: _t->msg((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 31: _t->msg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 32: _t->msg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 33: _t->msg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 34: _t->msg((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 35: _t->editCopy(); break;
        case 36: _t->editPaste(); break;
        case 37: _t->editSelect(); break;
        case 38: _t->editFind1x1(); break;
        case 39: _t->editFind3x3(); break;
        case 40: _t->editFind((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 41: _t->editFind(); break;
        case 42: _t->editUndo(); break;
        case 43: _t->showTrkEditr(); break;
        case 44: _t->showContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 45: _t->createNewTiles((*reinterpret_cast< QMap<int,QPair<int,int>*>(*)>(_a[1]))); break;
        case 46: _t->createNewLoTiles((*reinterpret_cast< QMap<int,QPair<int,int>*>(*)>(_a[1]))); break;
        case 47: _t->objectSelected((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 48: _t->objectSelected((*reinterpret_cast< QVector<GameObj*>(*)>(_a[1]))); break;
        case 49: _t->selectToolresetMoveStep(); break;
        case 50: _t->selectToolresetRot(); break;
        case 51: _t->selectToolresetVert(); break;
        case 52: _t->selectToolSelect(); break;
        case 53: _t->selectToolRotate(); break;
        case 54: _t->selectToolTranslate(); break;
        case 55: _t->selectToolScale(); break;
        case 56: _t->toolBrushDirectionUp(); break;
        case 57: _t->toolBrushDirectionDown(); break;
        case 58: _t->putTerrainTexToolSelectRandom(); break;
        case 59: _t->putTerrainTexToolSelectPresent(); break;
        case 60: _t->putTerrainTexToolSelect0(); break;
        case 61: _t->putTerrainTexToolSelect90(); break;
        case 62: _t->putTerrainTexToolSelect180(); break;
        case 63: _t->putTerrainTexToolSelect270(); break;
        case 64: _t->placeToolStickTerrain(); break;
        case 65: _t->placeToolStickAll(); break;
        case 66: _t->reloadRefFile(); break;
        case 67: _t->reloadMkrFiles(); break;
        case 68: _t->setCameraObject((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 69: _t->setMoveStep((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 70: _t->statusPanelCommand((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 71: _t->flexResult((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 72: _t->focusEditor(); break;
        case 73: _t->paintToolObj(); break;
        case 74: _t->paintToolObjSelected(); break;
        case 75: _t->paintToolTDB(); break;
        case 76: _t->paintToolTDBVector(); break;
        case 77: _t->paintToolTileTrack(); break;
        case 78: _t->paintToolTileRoad(); break;
        case 79: _t->paintToolWaterEdges(); break;
        case 80: _t->paintToolResetTile(); break;
        case 81: _t->setTerrainToObj(); break;
        case 82: _t->smoothTerrainToObj(); break;
        case 83: _t->setTerrainToNearestDbTile(); break;
        case 84: _t->setTerrainToSelectedObjTile(); break;
        case 85: _t->selectAllTerrainPatchesOnSelectedTile(); break;
        case 86: _t->adjustObjPositionToTerrainMenu(); break;
        case 87: _t->adjustObjRotationToTerrainMenu(); break;
        case 88: _t->pickObjForPlacement(); break;
        case 89: _t->pickObjRotForPlacement(); break;
        case 90: _t->pickObjRotElevForPlacement(); break;
        case 91: _t->pickObjRotForCamera(); break;
        case 92: _t->pickObjRotForCameraFlip(); break;
        case 93: _t->resetCamN(); break;
        case 94: _t->resetCamS(); break;
        case 95: _t->resetCamE(); break;
        case 96: _t->resetCamW(); break;
        case 97: _t->resetCamD(); break;
        case 98: _t->resetCamZ(); break;
        case 99: _t->tangentOrigin(); break;
        case 100: _t->tangentTarget(); break;
        case 101: _t->tangentMath(); break;
        case 102: _t->TangentApplyRot(); break;
        case 103: _t->initRoute2(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RouteEditorGLWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::showWindow)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(Route * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::routeLoaded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(Ref::RefItem * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::itemSelected)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::naviInfo)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(PreciseTileCoordinate * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::posInfo)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(float * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::pointerInfo)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::setToolbox)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::setBrushTextureId)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(GameObj * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::showProperties)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(GameObj * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::updateProperties)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(int , int , float * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::flexData)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QMap<QString,Coords*> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::mkrList)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::refreshObjLists)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::reloadMkrLists)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::sendMsg)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::sendMsg)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::sendMsg)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::sendMsg)) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::sendMsg)) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::updStatus)) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::preloadTexturesSignal)) {
                *result = 20;
                return;
            }
        }
        {
            using _t = void (RouteEditorGLWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorGLWidget::resetGradeHelperRequested)) {
                *result = 21;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RouteEditorGLWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_RouteEditorGLWidget.data,
    qt_meta_data_RouteEditorGLWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RouteEditorGLWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RouteEditorGLWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RouteEditorGLWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QOpenGLFunctions"))
        return static_cast< QOpenGLFunctions*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int RouteEditorGLWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 104)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 104;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 104)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 104;
    }
    return _id;
}

// SIGNAL 0
void RouteEditorGLWidget::showWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RouteEditorGLWidget::routeLoaded(Route * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RouteEditorGLWidget::itemSelected(Ref::RefItem * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void RouteEditorGLWidget::naviInfo(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RouteEditorGLWidget::posInfo(PreciseTileCoordinate * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RouteEditorGLWidget::pointerInfo(float * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void RouteEditorGLWidget::setToolbox(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void RouteEditorGLWidget::setBrushTextureId(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void RouteEditorGLWidget::showProperties(GameObj * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void RouteEditorGLWidget::updateProperties(GameObj * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void RouteEditorGLWidget::flexData(int _t1, int _t2, float * _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void RouteEditorGLWidget::mkrList(QMap<QString,Coords*> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void RouteEditorGLWidget::refreshObjLists()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void RouteEditorGLWidget::reloadMkrLists()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void RouteEditorGLWidget::sendMsg(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void RouteEditorGLWidget::sendMsg(QString _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void RouteEditorGLWidget::sendMsg(QString _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void RouteEditorGLWidget::sendMsg(QString _t1, float _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void RouteEditorGLWidget::sendMsg(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void RouteEditorGLWidget::updStatus(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void RouteEditorGLWidget::preloadTexturesSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void RouteEditorGLWidget::resetGradeHelperRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

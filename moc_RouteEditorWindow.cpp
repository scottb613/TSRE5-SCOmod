/****************************************************************************
** Meta object code from reading C++ file 'RouteEditorWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.19)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "RouteEditorWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RouteEditorWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.19. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RouteEditorWindow_t {
    QByteArrayData data[69];
    char stringdata0[1060];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RouteEditorWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RouteEditorWindow_t qt_meta_stringdata_RouteEditorWindow = {
    {
QT_MOC_LITERAL(0, 0, 17), // "RouteEditorWindow"
QT_MOC_LITERAL(1, 18, 7), // "exitNow"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 7), // "sendMsg"
QT_MOC_LITERAL(4, 35, 4), // "text"
QT_MOC_LITERAL(5, 40, 13), // "reloadRefFile"
QT_MOC_LITERAL(6, 54, 13), // "reloadMkrFile"
QT_MOC_LITERAL(7, 68, 14), // "reloadMkrLists"
QT_MOC_LITERAL(8, 83, 16), // "refreshErrorList"
QT_MOC_LITERAL(9, 100, 9), // "updStatus"
QT_MOC_LITERAL(10, 110, 8), // "statName"
QT_MOC_LITERAL(11, 119, 9), // "statValue"
QT_MOC_LITERAL(12, 129, 4), // "save"
QT_MOC_LITERAL(13, 134, 15), // "openRouteFolder"
QT_MOC_LITERAL(14, 150, 9), // "showRoute"
QT_MOC_LITERAL(15, 160, 4), // "show"
QT_MOC_LITERAL(16, 165, 11), // "createPaths"
QT_MOC_LITERAL(17, 177, 9), // "reloadRef"
QT_MOC_LITERAL(18, 187, 9), // "reloadMkr"
QT_MOC_LITERAL(19, 197, 14), // "reloadSettings"
QT_MOC_LITERAL(20, 212, 13), // "refreshErrors"
QT_MOC_LITERAL(21, 226, 5), // "about"
QT_MOC_LITERAL(22, 232, 13), // "terrainCamera"
QT_MOC_LITERAL(23, 246, 3), // "val"
QT_MOC_LITERAL(24, 250, 11), // "mstsShadows"
QT_MOC_LITERAL(25, 262, 22), // "detailedTerrainEnabled"
QT_MOC_LITERAL(26, 285, 21), // "distantTerrainEnabled"
QT_MOC_LITERAL(27, 307, 10), // "setToolbox"
QT_MOC_LITERAL(28, 318, 4), // "name"
QT_MOC_LITERAL(29, 323, 14), // "showProperties"
QT_MOC_LITERAL(30, 338, 8), // "GameObj*"
QT_MOC_LITERAL(31, 347, 3), // "obj"
QT_MOC_LITERAL(32, 351, 16), // "updateProperties"
QT_MOC_LITERAL(33, 368, 18), // "hideShowToolWidget"
QT_MOC_LITERAL(34, 387, 24), // "hideShowPropertiesWidget"
QT_MOC_LITERAL(35, 412, 18), // "hideShowStatWidget"
QT_MOC_LITERAL(36, 431, 22), // "hideShowSettingsDialog"
QT_MOC_LITERAL(37, 454, 23), // "hideShowShapeViewWidget"
QT_MOC_LITERAL(38, 478, 22), // "hideShowErrorMsgWidget"
QT_MOC_LITERAL(39, 501, 13), // "viewWorldGrid"
QT_MOC_LITERAL(40, 515, 12), // "viewTileGrid"
QT_MOC_LITERAL(41, 528, 16), // "viewTerrainShape"
QT_MOC_LITERAL(42, 545, 15), // "viewTerrainGrid"
QT_MOC_LITERAL(43, 561, 16), // "viewInteractives"
QT_MOC_LITERAL(44, 578, 17), // "viewForestRegions"
QT_MOC_LITERAL(45, 596, 16), // "viewTrackDbLines"
QT_MOC_LITERAL(46, 613, 17), // "viewTsectionLines"
QT_MOC_LITERAL(47, 631, 16), // "viewGradeSymbols"
QT_MOC_LITERAL(48, 648, 14), // "viewTrackItems"
QT_MOC_LITERAL(49, 663, 13), // "viewPointer3d"
QT_MOC_LITERAL(50, 677, 11), // "viewMarkers"
QT_MOC_LITERAL(51, 689, 12), // "viewSnapable"
QT_MOC_LITERAL(52, 702, 11), // "viewCompass"
QT_MOC_LITERAL(53, 714, 25), // "showToolsObjectAndTerrain"
QT_MOC_LITERAL(54, 740, 15), // "showToolsObject"
QT_MOC_LITERAL(55, 756, 16), // "showToolsTerrain"
QT_MOC_LITERAL(56, 773, 12), // "showToolsGeo"
QT_MOC_LITERAL(57, 786, 17), // "showToolsActivity"
QT_MOC_LITERAL(58, 804, 20), // "showTerrainTreeEditr"
QT_MOC_LITERAL(59, 825, 30), // "showWorldObjPivotPointsEnabled"
QT_MOC_LITERAL(60, 856, 18), // "statusWindowClosed"
QT_MOC_LITERAL(61, 875, 25), // "errorMessagesWindowClosed"
QT_MOC_LITERAL(62, 901, 21), // "shapeVeiwWindowClosed"
QT_MOC_LITERAL(63, 923, 15), // "viewUnselectAll"
QT_MOC_LITERAL(64, 939, 23), // "showActivityEventEditor"
QT_MOC_LITERAL(65, 963, 25), // "showActivityServiceEditor"
QT_MOC_LITERAL(66, 989, 25), // "showActivityTrafficEditor"
QT_MOC_LITERAL(67, 1015, 27), // "showActivityTimetableEditor"
QT_MOC_LITERAL(68, 1043, 16) // "exitToLoadWindow"

    },
    "RouteEditorWindow\0exitNow\0\0sendMsg\0"
    "text\0reloadRefFile\0reloadMkrFile\0"
    "reloadMkrLists\0refreshErrorList\0"
    "updStatus\0statName\0statValue\0save\0"
    "openRouteFolder\0showRoute\0show\0"
    "createPaths\0reloadRef\0reloadMkr\0"
    "reloadSettings\0refreshErrors\0about\0"
    "terrainCamera\0val\0mstsShadows\0"
    "detailedTerrainEnabled\0distantTerrainEnabled\0"
    "setToolbox\0name\0showProperties\0GameObj*\0"
    "obj\0updateProperties\0hideShowToolWidget\0"
    "hideShowPropertiesWidget\0hideShowStatWidget\0"
    "hideShowSettingsDialog\0hideShowShapeViewWidget\0"
    "hideShowErrorMsgWidget\0viewWorldGrid\0"
    "viewTileGrid\0viewTerrainShape\0"
    "viewTerrainGrid\0viewInteractives\0"
    "viewForestRegions\0viewTrackDbLines\0"
    "viewTsectionLines\0viewGradeSymbols\0"
    "viewTrackItems\0viewPointer3d\0viewMarkers\0"
    "viewSnapable\0viewCompass\0"
    "showToolsObjectAndTerrain\0showToolsObject\0"
    "showToolsTerrain\0showToolsGeo\0"
    "showToolsActivity\0showTerrainTreeEditr\0"
    "showWorldObjPivotPointsEnabled\0"
    "statusWindowClosed\0errorMessagesWindowClosed\0"
    "shapeVeiwWindowClosed\0viewUnselectAll\0"
    "showActivityEventEditor\0"
    "showActivityServiceEditor\0"
    "showActivityTrafficEditor\0"
    "showActivityTimetableEditor\0"
    "exitToLoadWindow"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RouteEditorWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      60,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  314,    2, 0x06 /* Public */,
       3,    1,  315,    2, 0x06 /* Public */,
       5,    0,  318,    2, 0x06 /* Public */,
       6,    0,  319,    2, 0x06 /* Public */,
       7,    0,  320,    2, 0x06 /* Public */,
       8,    0,  321,    2, 0x06 /* Public */,
       9,    2,  322,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,  327,    2, 0x0a /* Public */,
      13,    0,  328,    2, 0x0a /* Public */,
      14,    0,  329,    2, 0x0a /* Public */,
      15,    0,  330,    2, 0x0a /* Public */,
      16,    0,  331,    2, 0x0a /* Public */,
      17,    0,  332,    2, 0x0a /* Public */,
      18,    0,  333,    2, 0x0a /* Public */,
      19,    0,  334,    2, 0x0a /* Public */,
      20,    0,  335,    2, 0x0a /* Public */,
      21,    0,  336,    2, 0x0a /* Public */,
      22,    1,  337,    2, 0x0a /* Public */,
      24,    1,  340,    2, 0x0a /* Public */,
      25,    0,  343,    2, 0x0a /* Public */,
      26,    0,  344,    2, 0x0a /* Public */,
      27,    1,  345,    2, 0x0a /* Public */,
      29,    1,  348,    2, 0x0a /* Public */,
      32,    1,  351,    2, 0x0a /* Public */,
      33,    1,  354,    2, 0x0a /* Public */,
      34,    1,  357,    2, 0x0a /* Public */,
      35,    1,  360,    2, 0x0a /* Public */,
      36,    1,  363,    2, 0x0a /* Public */,
      37,    1,  366,    2, 0x0a /* Public */,
      38,    1,  369,    2, 0x0a /* Public */,
      39,    1,  372,    2, 0x0a /* Public */,
      40,    1,  375,    2, 0x0a /* Public */,
      41,    1,  378,    2, 0x0a /* Public */,
      42,    1,  381,    2, 0x0a /* Public */,
      43,    1,  384,    2, 0x0a /* Public */,
      44,    1,  387,    2, 0x0a /* Public */,
      45,    1,  390,    2, 0x0a /* Public */,
      46,    1,  393,    2, 0x0a /* Public */,
      47,    1,  396,    2, 0x0a /* Public */,
      48,    1,  399,    2, 0x0a /* Public */,
      49,    1,  402,    2, 0x0a /* Public */,
      50,    1,  405,    2, 0x0a /* Public */,
      51,    1,  408,    2, 0x0a /* Public */,
      52,    1,  411,    2, 0x0a /* Public */,
      53,    1,  414,    2, 0x0a /* Public */,
      54,    1,  417,    2, 0x0a /* Public */,
      55,    1,  420,    2, 0x0a /* Public */,
      56,    1,  423,    2, 0x0a /* Public */,
      57,    1,  426,    2, 0x0a /* Public */,
      58,    0,  429,    2, 0x0a /* Public */,
      59,    1,  430,    2, 0x0a /* Public */,
      60,    0,  433,    2, 0x0a /* Public */,
      61,    0,  434,    2, 0x0a /* Public */,
      62,    0,  435,    2, 0x0a /* Public */,
      63,    0,  436,    2, 0x0a /* Public */,
      64,    0,  437,    2, 0x0a /* Public */,
      65,    0,  438,    2, 0x0a /* Public */,
      66,    0,  439,    2, 0x0a /* Public */,
      67,    0,  440,    2, 0x0a /* Public */,
      68,    0,  441,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,

 // slots: parameters
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
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   28,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   15,
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

void RouteEditorWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RouteEditorWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->exitNow(); break;
        case 1: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->reloadRefFile(); break;
        case 3: _t->reloadMkrFile(); break;
        case 4: _t->reloadMkrLists(); break;
        case 5: _t->refreshErrorList(); break;
        case 6: _t->updStatus((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 7: _t->save(); break;
        case 8: _t->openRouteFolder(); break;
        case 9: _t->showRoute(); break;
        case 10: _t->show(); break;
        case 11: _t->createPaths(); break;
        case 12: _t->reloadRef(); break;
        case 13: _t->reloadMkr(); break;
        case 14: _t->reloadSettings(); break;
        case 15: _t->refreshErrors(); break;
        case 16: _t->about(); break;
        case 17: _t->terrainCamera((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->mstsShadows((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 19: _t->detailedTerrainEnabled(); break;
        case 20: _t->distantTerrainEnabled(); break;
        case 21: _t->setToolbox((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 22: _t->showProperties((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 23: _t->updateProperties((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 24: _t->hideShowToolWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 25: _t->hideShowPropertiesWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 26: _t->hideShowStatWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 27: _t->hideShowSettingsDialog((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 28: _t->hideShowShapeViewWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 29: _t->hideShowErrorMsgWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 30: _t->viewWorldGrid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 31: _t->viewTileGrid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 32: _t->viewTerrainShape((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 33: _t->viewTerrainGrid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 34: _t->viewInteractives((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 35: _t->viewForestRegions((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 36: _t->viewTrackDbLines((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 37: _t->viewTsectionLines((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 38: _t->viewGradeSymbols((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 39: _t->viewTrackItems((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 40: _t->viewPointer3d((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 41: _t->viewMarkers((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 42: _t->viewSnapable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 43: _t->viewCompass((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 44: _t->showToolsObjectAndTerrain((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 45: _t->showToolsObject((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 46: _t->showToolsTerrain((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 47: _t->showToolsGeo((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 48: _t->showToolsActivity((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 49: _t->showTerrainTreeEditr(); break;
        case 50: _t->showWorldObjPivotPointsEnabled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 51: _t->statusWindowClosed(); break;
        case 52: _t->errorMessagesWindowClosed(); break;
        case 53: _t->shapeVeiwWindowClosed(); break;
        case 54: _t->viewUnselectAll(); break;
        case 55: _t->showActivityEventEditor(); break;
        case 56: _t->showActivityServiceEditor(); break;
        case 57: _t->showActivityTrafficEditor(); break;
        case 58: _t->showActivityTimetableEditor(); break;
        case 59: _t->exitToLoadWindow(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RouteEditorWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::exitNow)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::sendMsg)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::reloadRefFile)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::reloadMkrFile)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::reloadMkrLists)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::refreshErrorList)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RouteEditorWindow::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RouteEditorWindow::updStatus)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RouteEditorWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_RouteEditorWindow.data,
    qt_meta_data_RouteEditorWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RouteEditorWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RouteEditorWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RouteEditorWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int RouteEditorWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 60)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 60;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 60)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 60;
    }
    return _id;
}

// SIGNAL 0
void RouteEditorWindow::exitNow()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RouteEditorWindow::sendMsg(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RouteEditorWindow::reloadRefFile()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void RouteEditorWindow::reloadMkrFile()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void RouteEditorWindow::reloadMkrLists()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void RouteEditorWindow::refreshErrorList()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void RouteEditorWindow::updStatus(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

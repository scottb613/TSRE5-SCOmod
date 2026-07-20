/****************************************************************************
** Meta object code from reading C++ file 'Route.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.19)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "Route.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QVector>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Route.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.19. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Route_t {
    QByteArrayData data[8];
    char stringdata0[65];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Route_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Route_t qt_meta_stringdata_Route = {
    {
QT_MOC_LITERAL(0, 0, 5), // "Route"
QT_MOC_LITERAL(1, 6, 14), // "objectSelected"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 8), // "GameObj*"
QT_MOC_LITERAL(4, 31, 3), // "obj"
QT_MOC_LITERAL(5, 35, 17), // "QVector<GameObj*>"
QT_MOC_LITERAL(6, 53, 7), // "sendMsg"
QT_MOC_LITERAL(7, 61, 3) // "val"

    },
    "Route\0objectSelected\0\0GameObj*\0obj\0"
    "QVector<GameObj*>\0sendMsg\0val"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Route[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       1,    1,   32,    2, 0x06 /* Public */,
       6,    1,   35,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 5,    4,
    QMetaType::Void, QMetaType::QString,    7,

       0        // eod
};

void Route::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Route *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->objectSelected((*reinterpret_cast< GameObj*(*)>(_a[1]))); break;
        case 1: _t->objectSelected((*reinterpret_cast< QVector<GameObj*>(*)>(_a[1]))); break;
        case 2: _t->sendMsg((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Route::*)(GameObj * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Route::objectSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Route::*)(QVector<GameObj*> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Route::objectSelected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Route::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Route::sendMsg)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Route::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Route.data,
    qt_meta_data_Route,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Route::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Route::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Route.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Route::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void Route::objectSelected(GameObj * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Route::objectSelected(QVector<GameObj*> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Route::sendMsg(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

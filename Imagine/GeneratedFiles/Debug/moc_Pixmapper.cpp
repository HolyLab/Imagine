/****************************************************************************
** Meta object code from reading C++ file 'Pixmapper.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Pixmapper.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Pixmapper.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_Pixmapper_t {
    QByteArrayData data[22];
    char stringdata0[161];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Pixmapper_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Pixmapper_t qt_meta_stringdata_Pixmapper = {
    {
QT_MOC_LITERAL(0, 0, 9), // "Pixmapper"
QT_MOC_LITERAL(1, 10, 11), // "pixmapReady"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 4), // "pxmp"
QT_MOC_LITERAL(4, 28, 3), // "img"
QT_MOC_LITERAL(5, 32, 9), // "handleImg"
QT_MOC_LITERAL(6, 42, 2), // "ba"
QT_MOC_LITERAL(7, 45, 6), // "imageW"
QT_MOC_LITERAL(8, 52, 6), // "imageH"
QT_MOC_LITERAL(9, 59, 11), // "scaleFactor"
QT_MOC_LITERAL(10, 71, 9), // "dAreaSize"
QT_MOC_LITERAL(11, 81, 5), // "dLeft"
QT_MOC_LITERAL(12, 87, 4), // "dTop"
QT_MOC_LITERAL(13, 92, 6), // "dWidth"
QT_MOC_LITERAL(14, 99, 7), // "dHeight"
QT_MOC_LITERAL(15, 107, 5), // "xDown"
QT_MOC_LITERAL(16, 113, 4), // "xCur"
QT_MOC_LITERAL(17, 118, 5), // "yDown"
QT_MOC_LITERAL(18, 124, 4), // "yCur"
QT_MOC_LITERAL(19, 129, 9), // "minPixVal"
QT_MOC_LITERAL(20, 139, 9), // "maxPixVal"
QT_MOC_LITERAL(21, 149, 11) // "colorizeSat"

    },
    "Pixmapper\0pixmapReady\0\0pxmp\0img\0"
    "handleImg\0ba\0imageW\0imageH\0scaleFactor\0"
    "dAreaSize\0dLeft\0dTop\0dWidth\0dHeight\0"
    "xDown\0xCur\0yDown\0yCur\0minPixVal\0"
    "maxPixVal\0colorizeSat"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Pixmapper[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,   16,   29,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QPixmap, QMetaType::QImage,    3,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::QByteArray, QMetaType::Int, QMetaType::Int, QMetaType::Double, QMetaType::Double, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Bool,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,

       0        // eod
};

void Pixmapper::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Pixmapper *_t = static_cast<Pixmapper *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->pixmapReady((*reinterpret_cast< const QPixmap(*)>(_a[1])),(*reinterpret_cast< const QImage(*)>(_a[2]))); break;
        case 1: _t->handleImg((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< const int(*)>(_a[2])),(*reinterpret_cast< const int(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4])),(*reinterpret_cast< const double(*)>(_a[5])),(*reinterpret_cast< const int(*)>(_a[6])),(*reinterpret_cast< const int(*)>(_a[7])),(*reinterpret_cast< const int(*)>(_a[8])),(*reinterpret_cast< const int(*)>(_a[9])),(*reinterpret_cast< const int(*)>(_a[10])),(*reinterpret_cast< const int(*)>(_a[11])),(*reinterpret_cast< const int(*)>(_a[12])),(*reinterpret_cast< const int(*)>(_a[13])),(*reinterpret_cast< const int(*)>(_a[14])),(*reinterpret_cast< const int(*)>(_a[15])),(*reinterpret_cast< bool(*)>(_a[16]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (Pixmapper::*_t)(const QPixmap & , const QImage & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&Pixmapper::pixmapReady)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject Pixmapper::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pixmapper.data,
      qt_meta_data_Pixmapper,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *Pixmapper::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Pixmapper::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_Pixmapper.stringdata0))
        return static_cast<void*>(const_cast< Pixmapper*>(this));
    return QObject::qt_metacast(_clname);
}

int Pixmapper::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void Pixmapper::pixmapReady(const QPixmap & _t1, const QImage & _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE

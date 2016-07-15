/****************************************************************************
** Meta object code from reading C++ file 'ImgApplication.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ImgApplication.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ImgApplication.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ImgApplication_t {
    QByteArrayData data[8];
    char stringdata0[98];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ImgApplication_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ImgApplication_t qt_meta_stringdata_ImgApplication = {
    {
QT_MOC_LITERAL(0, 0, 14), // "ImgApplication"
QT_MOC_LITERAL(1, 15, 13), // "msgForArduino"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 11), // "const char*"
QT_MOC_LITERAL(4, 42, 7), // "message"
QT_MOC_LITERAL(5, 50, 6), // "length"
QT_MOC_LITERAL(6, 57, 17), // "ardThreadFinished"
QT_MOC_LITERAL(7, 75, 22) // "incomingArduinoMessage"

    },
    "ImgApplication\0msgForArduino\0\0const char*\0"
    "message\0length\0ardThreadFinished\0"
    "incomingArduinoMessage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ImgApplication[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   34,    2, 0x0a /* Public */,
       7,    1,   35,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,

       0        // eod
};

void ImgApplication::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ImgApplication *_t = static_cast<ImgApplication *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->msgForArduino((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->ardThreadFinished(); break;
        case 2: _t->incomingArduinoMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ImgApplication::*_t)(const char * , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ImgApplication::msgForArduino)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject ImgApplication::staticMetaObject = {
    { &QApplication::staticMetaObject, qt_meta_stringdata_ImgApplication.data,
      qt_meta_data_ImgApplication,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ImgApplication::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImgApplication::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ImgApplication.stringdata0))
        return static_cast<void*>(const_cast< ImgApplication*>(this));
    return QApplication::qt_metacast(_clname);
}

int ImgApplication::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QApplication::qt_metacall(_c, _id, _a);
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
void ImgApplication::msgForArduino(const char * _t1, int _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE

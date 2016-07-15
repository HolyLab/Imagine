/****************************************************************************
** Meta object code from reading C++ file 'ArduinoThread.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ArduinoThread.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ArduinoThread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ArduinoThread_t {
    QByteArrayData data[7];
    char stringdata0[70];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ArduinoThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ArduinoThread_t qt_meta_stringdata_ArduinoThread = {
    {
QT_MOC_LITERAL(0, 0, 13), // "ArduinoThread"
QT_MOC_LITERAL(1, 14, 15), // "incomingArduino"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 7), // "message"
QT_MOC_LITERAL(4, 39, 11), // "sendMessage"
QT_MOC_LITERAL(5, 51, 11), // "const char*"
QT_MOC_LITERAL(6, 63, 6) // "length"

    },
    "ArduinoThread\0incomingArduino\0\0message\0"
    "sendMessage\0const char*\0length"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ArduinoThread[] = {

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
       1,    1,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   27,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5, QMetaType::Int,    3,    6,

       0        // eod
};

void ArduinoThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ArduinoThread *_t = static_cast<ArduinoThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->incomingArduino((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->sendMessage((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ArduinoThread::*_t)(QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ArduinoThread::incomingArduino)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject ArduinoThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_ArduinoThread.data,
      qt_meta_data_ArduinoThread,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ArduinoThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArduinoThread::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ArduinoThread.stringdata0))
        return static_cast<void*>(const_cast< ArduinoThread*>(this));
    return QThread::qt_metacast(_clname);
}

int ArduinoThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
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
void ArduinoThread::incomingArduino(QString _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE

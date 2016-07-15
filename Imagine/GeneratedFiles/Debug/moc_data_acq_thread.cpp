/****************************************************************************
** Meta object code from reading C++ file 'data_acq_thread.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../data_acq_thread.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'data_acq_thread.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DataAcqThread_t {
    QByteArrayData data[13];
    char stringdata0[138];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DataAcqThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DataAcqThread_t qt_meta_stringdata_DataAcqThread = {
    {
QT_MOC_LITERAL(0, 0, 13), // "DataAcqThread"
QT_MOC_LITERAL(1, 14, 17), // "imageDisplayReady"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 5), // "image"
QT_MOC_LITERAL(4, 39, 3), // "idx"
QT_MOC_LITERAL(5, 43, 17), // "newStatusMsgReady"
QT_MOC_LITERAL(6, 61, 3), // "str"
QT_MOC_LITERAL(7, 65, 14), // "newLogMsgReady"
QT_MOC_LITERAL(8, 80, 14), // "imageDataReady"
QT_MOC_LITERAL(9, 95, 6), // "data16"
QT_MOC_LITERAL(10, 102, 6), // "imageW"
QT_MOC_LITERAL(11, 109, 6), // "imageH"
QT_MOC_LITERAL(12, 116, 21) // "resetActuatorPosReady"

    },
    "DataAcqThread\0imageDisplayReady\0\0image\0"
    "idx\0newStatusMsgReady\0str\0newLogMsgReady\0"
    "imageDataReady\0data16\0imageW\0imageH\0"
    "resetActuatorPosReady"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DataAcqThread[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,
       5,    1,   44,    2, 0x06 /* Public */,
       7,    1,   47,    2, 0x06 /* Public */,
       8,    4,   50,    2, 0x06 /* Public */,
      12,    0,   59,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage, QMetaType::Long,    3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::Long, QMetaType::Int, QMetaType::Int,    9,    4,   10,   11,
    QMetaType::Void,

       0        // eod
};

void DataAcqThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DataAcqThread *_t = static_cast<DataAcqThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->imageDisplayReady((*reinterpret_cast< const QImage(*)>(_a[1])),(*reinterpret_cast< long(*)>(_a[2]))); break;
        case 1: _t->newStatusMsgReady((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->newLogMsgReady((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->imageDataReady((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< long(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 4: _t->resetActuatorPosReady(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (DataAcqThread::*_t)(const QImage & , long );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DataAcqThread::imageDisplayReady)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (DataAcqThread::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DataAcqThread::newStatusMsgReady)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (DataAcqThread::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DataAcqThread::newLogMsgReady)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (DataAcqThread::*_t)(const QByteArray & , long , int , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DataAcqThread::imageDataReady)) {
                *result = 3;
                return;
            }
        }
        {
            typedef void (DataAcqThread::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DataAcqThread::resetActuatorPosReady)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject DataAcqThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_DataAcqThread.data,
      qt_meta_data_DataAcqThread,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DataAcqThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DataAcqThread::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DataAcqThread.stringdata0))
        return static_cast<void*>(const_cast< DataAcqThread*>(this));
    return QThread::qt_metacast(_clname);
}

int DataAcqThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void DataAcqThread::imageDisplayReady(const QImage & _t1, long _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DataAcqThread::newStatusMsgReady(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DataAcqThread::newLogMsgReady(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DataAcqThread::imageDataReady(const QByteArray & _t1, long _t2, int _t3, int _t4)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DataAcqThread::resetActuatorPosReady()
{
    QMetaObject::activate(this, &staticMetaObject, 4, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE

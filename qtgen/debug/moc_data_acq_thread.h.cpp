/****************************************************************************
** Meta object code from reading C++ file 'data_acq_thread.hpp'
**
** Created: Mon Sep 27 17:15:12 2010
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../data_acq_thread.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'data_acq_thread.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_DataAcqThread[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      25,   15,   14,   14, 0x05,
      60,   56,   14,   14, 0x05,
      87,   56,   14,   14, 0x05,
     136,  111,   14,   14, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_DataAcqThread[] = {
    "DataAcqThread\0\0image,idx\0"
    "imageDisplayReady(QImage,long)\0str\0"
    "newStatusMsgReady(QString)\0"
    "newLogMsgReady(QString)\0"
    "data16,idx,imageW,imageH\0"
    "imageDataReady(QByteArray,long,int,int)\0"
};

const QMetaObject DataAcqThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_DataAcqThread,
      qt_meta_data_DataAcqThread, 0 }
};

const QMetaObject *DataAcqThread::metaObject() const
{
    return &staticMetaObject;
}

void *DataAcqThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DataAcqThread))
	return static_cast<void*>(const_cast< DataAcqThread*>(this));
    return QThread::qt_metacast(_clname);
}

int DataAcqThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: imageDisplayReady((*reinterpret_cast< const QImage(*)>(_a[1])),(*reinterpret_cast< long(*)>(_a[2]))); break;
        case 1: newStatusMsgReady((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: newLogMsgReady((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: imageDataReady((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< long(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        }
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void DataAcqThread::imageDisplayReady(const QImage & _t1, long _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DataAcqThread::newStatusMsgReady(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DataAcqThread::newLogMsgReady(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DataAcqThread::imageDataReady(const QByteArray & _t1, long _t2, int _t3, int _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
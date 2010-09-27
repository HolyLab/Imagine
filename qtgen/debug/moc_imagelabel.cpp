/****************************************************************************
** Meta object code from reading C++ file 'imagelabel.h'
**
** Created: Mon Sep 27 17:15:12 2010
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../imagelabel.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imagelabel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_ImageLabel[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      18,   12,   11,   11, 0x05,
      45,   12,   11,   11, 0x05,
      70,   12,   11,   11, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_ImageLabel[] = {
    "ImageLabel\0\0event\0mousePressed(QMouseEvent*)\0"
    "mouseMoved(QMouseEvent*)\0"
    "mouseReleased(QMouseEvent*)\0"
};

const QMetaObject ImageLabel::staticMetaObject = {
    { &QLabel::staticMetaObject, qt_meta_stringdata_ImageLabel,
      qt_meta_data_ImageLabel, 0 }
};

const QMetaObject *ImageLabel::metaObject() const
{
    return &staticMetaObject;
}

void *ImageLabel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ImageLabel))
	return static_cast<void*>(const_cast< ImageLabel*>(this));
    return QLabel::qt_metacast(_clname);
}

int ImageLabel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: mousePressed((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 1: mouseMoved((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 2: mouseReleased((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        }
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ImageLabel::mousePressed(QMouseEvent * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ImageLabel::mouseMoved(QMouseEvent * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ImageLabel::mouseReleased(QMouseEvent * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

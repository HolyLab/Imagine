/****************************************************************************
** Meta object code from reading C++ file 'ai_thread.hpp'
**
** Created: Mon Sep 27 17:15:13 2010
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ai_thread.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ai_thread.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_AiThread[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets

       0        // eod
};

static const char qt_meta_stringdata_AiThread[] = {
    "AiThread\0"
};

const QMetaObject AiThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_AiThread,
      qt_meta_data_AiThread, 0 }
};

const QMetaObject *AiThread::metaObject() const
{
    return &staticMetaObject;
}

void *AiThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AiThread))
	return static_cast<void*>(const_cast< AiThread*>(this));
    return QThread::qt_metacast(_clname);
}

int AiThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
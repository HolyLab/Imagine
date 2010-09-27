/****************************************************************************
** Meta object code from reading C++ file 'imagine.h'
**
** Created: Mon Sep 27 17:15:11 2010
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../imagine.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imagine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Imagine[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      35,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x08,
      42,    8,    8,    8, 0x08,
      68,    8,    8,    8, 0x08,
      99,    8,    8,    8, 0x08,
     133,    8,    8,    8, 0x08,
     163,    8,    8,    8, 0x08,
     193,    8,    8,    8, 0x08,
     227,    8,    8,    8, 0x08,
     253,    8,    8,    8, 0x08,
     285,    8,    8,    8, 0x08,
     318,    8,    8,    8, 0x08,
     345,    8,    8,    8, 0x08,
     374,    8,    8,    8, 0x08,
     396,    8,    8,    8, 0x08,
     425,    8,    8,    8, 0x08,
     455,    8,    8,    8, 0x08,
     492,    8,    8,    8, 0x08,
     523,    8,    8,    8, 0x08,
     549,    8,    8,    8, 0x08,
     582,    8,    8,    8, 0x08,
     616,    8,    8,    8, 0x08,
     649,    8,    8,    8, 0x08,
     692,    8,    8,    8, 0x08,
     721,    8,    8,    8, 0x08,
     749,    8,    8,    8, 0x08,
     774,    8,    8,    8, 0x08,
     818,  812,    8,    8, 0x08,
     863,  854,    8,    8, 0x08,
     907,    8,    8,    8, 0x08,
     929,  925,    8,    8, 0x08,
     955,  951,    8,    8, 0x08,
     999,  974,    8,    8, 0x08,
    1038,    8,    8,    8, 0x08,
    1072,    8,    8,    8, 0x08,
    1104,    8,    8,    8, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Imagine[] = {
    "Imagine\0\0on_actionHeatsinkFan_triggered()\0"
    "on_actionExit_triggered()\0"
    "on_btnGetPiezoCurPos_clicked()\0"
    "on_btnFastDecPosAndMove_clicked()\0"
    "on_btnDecPosAndMove_clicked()\0"
    "on_btnIncPosAndMove_clicked()\0"
    "on_btnFastIncPosAndMove_clicked()\0"
    "on_btnMovePiezo_clicked()\0"
    "on_btnSetCurPosAsStop_clicked()\0"
    "on_btnSetCurPosAsStart_clicked()\0"
    "on_btnSelectFile_clicked()\0"
    "on_btnOpenStimFile_clicked()\0"
    "on_btnApply_clicked()\0"
    "on_btnFullChipSize_clicked()\0"
    "on_btnUseZoomWindow_clicked()\0"
    "on_actionStartAcqAndSave_triggered()\0"
    "on_actionStartLive_triggered()\0"
    "on_actionStop_triggered()\0"
    "on_actionOpenShutter_triggered()\0"
    "on_actionCloseShutter_triggered()\0"
    "on_actionTemperature_triggered()\0"
    "on_actionAutoScaleOnFirstFrame_triggered()\0"
    "on_actionStimuli_triggered()\0"
    "on_actionConfig_triggered()\0"
    "on_actionLog_triggered()\0"
    "on_actionDisplayFullImage_triggered()\0"
    "index\0on_tabWidgetCfg_currentChanged(int)\0"
    "newValue\0on_doubleSpinBoxCurPos_valueChanged(double)\0"
    "readPiezoCurPos()\0str\0updateStatus(QString)\0"
    "msg\0appendLog(QString)\0data16,idx,imageW,imageH\0"
    "updateDisplay(QByteArray,long,int,int)\0"
    "zoom_onMousePressed(QMouseEvent*)\0"
    "zoom_onMouseMoved(QMouseEvent*)\0"
    "zoom_onMouseReleased(QMouseEvent*)\0"
};

const QMetaObject Imagine::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_Imagine,
      qt_meta_data_Imagine, 0 }
};

const QMetaObject *Imagine::metaObject() const
{
    return &staticMetaObject;
}

void *Imagine::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Imagine))
	return static_cast<void*>(const_cast< Imagine*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int Imagine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_actionHeatsinkFan_triggered(); break;
        case 1: on_actionExit_triggered(); break;
        case 2: on_btnGetPiezoCurPos_clicked(); break;
        case 3: on_btnFastDecPosAndMove_clicked(); break;
        case 4: on_btnDecPosAndMove_clicked(); break;
        case 5: on_btnIncPosAndMove_clicked(); break;
        case 6: on_btnFastIncPosAndMove_clicked(); break;
        case 7: on_btnMovePiezo_clicked(); break;
        case 8: on_btnSetCurPosAsStop_clicked(); break;
        case 9: on_btnSetCurPosAsStart_clicked(); break;
        case 10: on_btnSelectFile_clicked(); break;
        case 11: on_btnOpenStimFile_clicked(); break;
        case 12: on_btnApply_clicked(); break;
        case 13: on_btnFullChipSize_clicked(); break;
        case 14: on_btnUseZoomWindow_clicked(); break;
        case 15: on_actionStartAcqAndSave_triggered(); break;
        case 16: on_actionStartLive_triggered(); break;
        case 17: on_actionStop_triggered(); break;
        case 18: on_actionOpenShutter_triggered(); break;
        case 19: on_actionCloseShutter_triggered(); break;
        case 20: on_actionTemperature_triggered(); break;
        case 21: on_actionAutoScaleOnFirstFrame_triggered(); break;
        case 22: on_actionStimuli_triggered(); break;
        case 23: on_actionConfig_triggered(); break;
        case 24: on_actionLog_triggered(); break;
        case 25: on_actionDisplayFullImage_triggered(); break;
        case 26: on_tabWidgetCfg_currentChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 27: on_doubleSpinBoxCurPos_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 28: readPiezoCurPos(); break;
        case 29: updateStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 30: appendLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 31: updateDisplay((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< long(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 32: zoom_onMousePressed((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 33: zoom_onMouseMoved((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 34: zoom_onMouseReleased((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        }
        _id -= 35;
    }
    return _id;
}

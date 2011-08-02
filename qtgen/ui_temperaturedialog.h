/********************************************************************************
** Form generated from reading UI file 'temperaturedialog.ui'
**
** Created: Tue Aug 2 14:57:54 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEMPERATUREDIALOG_H
#define UI_TEMPERATUREDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSlider>

QT_BEGIN_NAMESPACE

class Ui_TemperatureDialogClass
{
public:
    QLabel *label;
    QGroupBox *groupBox;
    QRadioButton *radioButtonOn;
    QRadioButton *radioButtonOff;
    QLabel *labelTempToSet;
    QSlider *verticalSliderTempToSet;
    QPushButton *btnSet;
    QLabel *labelCurTemp;
    QPushButton *btnClose;
    QLabel *labelTempMsg;

    void setupUi(QDialog *TemperatureDialogClass)
    {
        if (TemperatureDialogClass->objectName().isEmpty())
            TemperatureDialogClass->setObjectName(QString::fromUtf8("TemperatureDialogClass"));
        TemperatureDialogClass->resize(273, 300);
        label = new QLabel(TemperatureDialogClass);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 20, 111, 16));
        groupBox = new QGroupBox(TemperatureDialogClass);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(130, 30, 131, 131));
        radioButtonOn = new QRadioButton(groupBox);
        radioButtonOn->setObjectName(QString::fromUtf8("radioButtonOn"));
        radioButtonOn->setGeometry(QRect(30, 40, 84, 19));
        radioButtonOff = new QRadioButton(groupBox);
        radioButtonOff->setObjectName(QString::fromUtf8("radioButtonOff"));
        radioButtonOff->setGeometry(QRect(30, 80, 84, 19));
        radioButtonOff->setChecked(true);
        labelTempToSet = new QLabel(TemperatureDialogClass);
        labelTempToSet->setObjectName(QString::fromUtf8("labelTempToSet"));
        labelTempToSet->setGeometry(QRect(30, 50, 47, 14));
        verticalSliderTempToSet = new QSlider(TemperatureDialogClass);
        verticalSliderTempToSet->setObjectName(QString::fromUtf8("verticalSliderTempToSet"));
        verticalSliderTempToSet->setGeometry(QRect(40, 80, 21, 161));
        verticalSliderTempToSet->setOrientation(Qt::Vertical);
        btnSet = new QPushButton(TemperatureDialogClass);
        btnSet->setObjectName(QString::fromUtf8("btnSet"));
        btnSet->setEnabled(false);
        btnSet->setGeometry(QRect(30, 260, 75, 23));
        labelCurTemp = new QLabel(TemperatureDialogClass);
        labelCurTemp->setObjectName(QString::fromUtf8("labelCurTemp"));
        labelCurTemp->setGeometry(QRect(170, 210, 47, 14));
        btnClose = new QPushButton(TemperatureDialogClass);
        btnClose->setObjectName(QString::fromUtf8("btnClose"));
        btnClose->setGeometry(QRect(170, 260, 75, 23));
        labelTempMsg = new QLabel(TemperatureDialogClass);
        labelTempMsg->setObjectName(QString::fromUtf8("labelTempMsg"));
        labelTempMsg->setGeometry(QRect(100, 180, 141, 16));

        retranslateUi(TemperatureDialogClass);
        QObject::connect(verticalSliderTempToSet, SIGNAL(valueChanged(int)), labelTempToSet, SLOT(setNum(int)));
        QObject::connect(btnClose, SIGNAL(clicked()), TemperatureDialogClass, SLOT(accept()));

        QMetaObject::connectSlotsByName(TemperatureDialogClass);
    } // setupUi

    void retranslateUi(QDialog *TemperatureDialogClass)
    {
        TemperatureDialogClass->setWindowTitle(QApplication::translate("TemperatureDialogClass", "Temperature Control", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("TemperatureDialogClass", "Temperature to set:", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("TemperatureDialogClass", "Cooler", 0, QApplication::UnicodeUTF8));
        radioButtonOn->setText(QApplication::translate("TemperatureDialogClass", "ON", 0, QApplication::UnicodeUTF8));
        radioButtonOff->setText(QApplication::translate("TemperatureDialogClass", "OFF", 0, QApplication::UnicodeUTF8));
        labelTempToSet->setText(QApplication::translate("TemperatureDialogClass", "20", 0, QApplication::UnicodeUTF8));
        btnSet->setText(QApplication::translate("TemperatureDialogClass", "Set", 0, QApplication::UnicodeUTF8));
        labelCurTemp->setText(QApplication::translate("TemperatureDialogClass", "20", 0, QApplication::UnicodeUTF8));
        btnClose->setText(QApplication::translate("TemperatureDialogClass", "Close", 0, QApplication::UnicodeUTF8));
        labelTempMsg->setText(QApplication::translate("TemperatureDialogClass", "Current Temperature:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class TemperatureDialogClass: public Ui_TemperatureDialogClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEMPERATUREDIALOG_H

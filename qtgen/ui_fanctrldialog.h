/********************************************************************************
** Form generated from reading ui file 'fanctrldialog.ui'
**
** Created: Mon Sep 27 17:15:13 2010
**      by: Qt User Interface Compiler version 4.3.1
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_FANCTRLDIALOG_H
#define UI_FANCTRLDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QRadioButton>
#include <QtGui/QVBoxLayout>

class Ui_FanCtrlDialogClass
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QRadioButton *radioButtonHigh;
    QRadioButton *radioButtonLow;
    QRadioButton *radioButtonOff;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *FanCtrlDialogClass)
    {
    if (FanCtrlDialogClass->objectName().isEmpty())
        FanCtrlDialogClass->setObjectName(QString::fromUtf8("FanCtrlDialogClass"));
    FanCtrlDialogClass->resize(269, 159);
    vboxLayout = new QVBoxLayout(FanCtrlDialogClass);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(11);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    groupBox = new QGroupBox(FanCtrlDialogClass);
    groupBox->setObjectName(QString::fromUtf8("groupBox"));
    vboxLayout1 = new QVBoxLayout(groupBox);
    vboxLayout1->setSpacing(6);
    vboxLayout1->setMargin(11);
    vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
    radioButtonHigh = new QRadioButton(groupBox);
    radioButtonHigh->setObjectName(QString::fromUtf8("radioButtonHigh"));
    radioButtonHigh->setChecked(true);

    vboxLayout1->addWidget(radioButtonHigh);

    radioButtonLow = new QRadioButton(groupBox);
    radioButtonLow->setObjectName(QString::fromUtf8("radioButtonLow"));

    vboxLayout1->addWidget(radioButtonLow);

    radioButtonOff = new QRadioButton(groupBox);
    radioButtonOff->setObjectName(QString::fromUtf8("radioButtonOff"));

    vboxLayout1->addWidget(radioButtonOff);


    vboxLayout->addWidget(groupBox);

    buttonBox = new QDialogButtonBox(FanCtrlDialogClass);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    buttonBox->setCenterButtons(true);

    vboxLayout->addWidget(buttonBox);


    retranslateUi(FanCtrlDialogClass);

    QMetaObject::connectSlotsByName(FanCtrlDialogClass);
    } // setupUi

    void retranslateUi(QDialog *FanCtrlDialogClass)
    {
    FanCtrlDialogClass->setWindowTitle(QApplication::translate("FanCtrlDialogClass", "Heatsink Fan Control", 0, QApplication::UnicodeUTF8));
    groupBox->setTitle(QApplication::translate("FanCtrlDialogClass", "Speed", 0, QApplication::UnicodeUTF8));
    radioButtonHigh->setText(QApplication::translate("FanCtrlDialogClass", "high", 0, QApplication::UnicodeUTF8));
    radioButtonLow->setText(QApplication::translate("FanCtrlDialogClass", "low", 0, QApplication::UnicodeUTF8));
    radioButtonOff->setText(QApplication::translate("FanCtrlDialogClass", "off", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(FanCtrlDialogClass);
    } // retranslateUi

};

namespace Ui {
    class FanCtrlDialogClass: public Ui_FanCtrlDialogClass {};
} // namespace Ui

#endif // UI_FANCTRLDIALOG_H

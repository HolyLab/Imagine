/********************************************************************************
** Form generated from reading UI file 'imagine.ui'
**
** Created: Thu Sep 22 12:42:01 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGINE_H
#define UI_IMAGINE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QTabWidget>
#include <QtGui/QTableWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "imagelabel.h"

QT_BEGIN_NAMESPACE

class Ui_ImagineClass
{
public:
    QAction *actionStartAcqAndSave;
    QAction *actionStop;
    QAction *actionOpenShutter;
    QAction *actionCloseShutter;
    QAction *action;
    QAction *actionTemperature;
    QAction *actionStartLive;
    QAction *actionNoAutoScale;
    QAction *actionAutoScaleOnFirstFrame;
    QAction *actionAutoScaleOnAllFrames;
    QAction *actionStimuli;
    QAction *actionConfig;
    QAction *actionLog;
    QAction *actionColorizeSaturatedPixels;
    QAction *actionDisplayFullImage;
    QAction *actionExit;
    QAction *actionHeatsinkFan;
    QWidget *centralWidget;
    QDockWidget *dwStim;
    QWidget *dockWidgetContents;
    QHBoxLayout *hboxLayout;
    QTableWidget *tableWidgetStimDisplay;
    QDockWidget *dwLog;
    QWidget *dockWidgetContents1;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout1;
    QLabel *label_22;
    QSpacerItem *spacerItem;
    QSpinBox *spinBoxMaxNumOfLinesInLog;
    QTextEdit *textEditLog;
    QDockWidget *dwCfg;
    QWidget *dockWidgetContents2;
    QVBoxLayout *vboxLayout1;
    QTabWidget *tabWidgetCfg;
    QWidget *tabGeneral;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout2;
    QLabel *label;
    QSpacerItem *spacerItem1;
    QPushButton *btnSelectFile;
    QLineEdit *lineEditFilename;
    QLabel *label_2;
    QTextEdit *textEditComment;
    QWidget *tabCamera;
    QVBoxLayout *vboxLayout3;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QComboBox *comboBoxTriggerMode;
    QCheckBox *cbBaseLineClamp;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_5_2;
    QDoubleSpinBox *doubleSpinBoxExpTime;
    QSpinBox *spinBoxNumOfStacks;
    QSpinBox *spinBoxFramesPerStack;
    QDoubleSpinBox *doubleSpinBoxBoxIdleTimeBtwnStacks;
    QHBoxLayout *hboxLayout3;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout1;
    QComboBox *comboBoxPreAmpGains;
    QLabel *label_14;
    QSpinBox *spinBoxEmGain;
    QLabel *label_16;
    QComboBox *comboBoxHorReadoutRate;
    QLabel *label_15;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout2;
    QLabel *label_18;
    QComboBox *comboBoxVertClockVolAmp;
    QLabel *label_17;
    QComboBox *comboBoxVertShiftSpeed;
    QWidget *tabBinning;
    QLabel *label_26;
    QLabel *label_21;
    QLabel *label_25;
    QLabel *label_24;
    QSpinBox *spinBoxHstart;
    QSpinBox *spinBoxHend;
    QSpinBox *spinBoxVstart;
    QSpinBox *spinBoxVend;
    QPushButton *btnUseZoomWindow;
    QPushButton *btnFullChipSize;
    QWidget *tabStim;
    QVBoxLayout *vboxLayout4;
    QCheckBox *cbStim;
    QHBoxLayout *hboxLayout4;
    QLabel *label_13;
    QSpacerItem *spacerItem2;
    QPushButton *btnOpenStimFile;
    QLineEdit *lineEditStimFile;
    QTextEdit *textEditStimFileContent;
    QWidget *tabPiezo;
    QLabel *label_19;
    QDoubleSpinBox *doubleSpinBoxMaxDistance;
    QLabel *label_8;
    QLabel *label_9;
    QDoubleSpinBox *doubleSpinBoxStartPos;
    QDoubleSpinBox *doubleSpinBoxStopPos;
    QPushButton *btnSetCurPosAsStart;
    QPushButton *btnSetCurPosAsStop;
    QLabel *label_20;
    QDoubleSpinBox *doubleSpinBoxCurPos;
    QPushButton *btnMovePiezo;
    QPushButton *btnFastIncPosAndMove;
    QPushButton *btnIncPosAndMove;
    QPushButton *btnDecPosAndMove;
    QPushButton *btnFastDecPosAndMove;
    QWidget *tabShutter;
    QLabel *label_11;
    QLabel *label_12;
    QWidget *tabAI;
    QLabel *label_6;
    QLabel *label_7;
    QPushButton *btnApply;
    ImageLabel *labelImage;
    QDockWidget *dwHist;
    QWidget *dockWidgetContents_2;
    QVBoxLayout *vboxLayout5;
    QDockWidget *dwIntenCurve;
    QWidget *dockWidgetContents_3;
    QVBoxLayout *vboxLayout6;
    QFrame *frameIntenCurve;
    QMenuBar *menuBar;
    QMenu *menu_File;
    QMenu *menuShutter;
    QMenu *menu_Acquisition;
    QMenu *menuView;
    QToolBar *toolBar;
    QToolBar *toolBar_2;
    QToolBar *toolBar_3;

    void setupUi(QMainWindow *ImagineClass)
    {
        if (ImagineClass->objectName().isEmpty())
            ImagineClass->setObjectName(QString::fromUtf8("ImagineClass"));
        ImagineClass->resize(763, 643);
        actionStartAcqAndSave = new QAction(ImagineClass);
        actionStartAcqAndSave->setObjectName(QString::fromUtf8("actionStartAcqAndSave"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/Resources/start_acq.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStartAcqAndSave->setIcon(icon);
        actionStop = new QAction(ImagineClass);
        actionStop->setObjectName(QString::fromUtf8("actionStop"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/Resources/stop.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop->setIcon(icon1);
        actionOpenShutter = new QAction(ImagineClass);
        actionOpenShutter->setObjectName(QString::fromUtf8("actionOpenShutter"));
        actionCloseShutter = new QAction(ImagineClass);
        actionCloseShutter->setObjectName(QString::fromUtf8("actionCloseShutter"));
        action = new QAction(ImagineClass);
        action->setObjectName(QString::fromUtf8("action"));
        actionTemperature = new QAction(ImagineClass);
        actionTemperature->setObjectName(QString::fromUtf8("actionTemperature"));
        actionStartLive = new QAction(ImagineClass);
        actionStartLive->setObjectName(QString::fromUtf8("actionStartLive"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/Resources/start_live.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStartLive->setIcon(icon2);
        actionNoAutoScale = new QAction(ImagineClass);
        actionNoAutoScale->setObjectName(QString::fromUtf8("actionNoAutoScale"));
        actionAutoScaleOnFirstFrame = new QAction(ImagineClass);
        actionAutoScaleOnFirstFrame->setObjectName(QString::fromUtf8("actionAutoScaleOnFirstFrame"));
        actionAutoScaleOnAllFrames = new QAction(ImagineClass);
        actionAutoScaleOnAllFrames->setObjectName(QString::fromUtf8("actionAutoScaleOnAllFrames"));
        actionStimuli = new QAction(ImagineClass);
        actionStimuli->setObjectName(QString::fromUtf8("actionStimuli"));
        actionConfig = new QAction(ImagineClass);
        actionConfig->setObjectName(QString::fromUtf8("actionConfig"));
        actionLog = new QAction(ImagineClass);
        actionLog->setObjectName(QString::fromUtf8("actionLog"));
        actionColorizeSaturatedPixels = new QAction(ImagineClass);
        actionColorizeSaturatedPixels->setObjectName(QString::fromUtf8("actionColorizeSaturatedPixels"));
        actionDisplayFullImage = new QAction(ImagineClass);
        actionDisplayFullImage->setObjectName(QString::fromUtf8("actionDisplayFullImage"));
        actionExit = new QAction(ImagineClass);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionHeatsinkFan = new QAction(ImagineClass);
        actionHeatsinkFan->setObjectName(QString::fromUtf8("actionHeatsinkFan"));
        centralWidget = new QWidget(ImagineClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        dwStim = new QDockWidget(centralWidget);
        dwStim->setObjectName(QString::fromUtf8("dwStim"));
        dwStim->setGeometry(QRect(0, 0, 421, 120));
        dwStim->setMinimumSize(QSize(89, 111));
        dwStim->setMaximumSize(QSize(524287, 524287));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        hboxLayout = new QHBoxLayout(dockWidgetContents);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        tableWidgetStimDisplay = new QTableWidget(dockWidgetContents);
        if (tableWidgetStimDisplay->columnCount() < 3)
            tableWidgetStimDisplay->setColumnCount(3);
        if (tableWidgetStimDisplay->rowCount() < 1)
            tableWidgetStimDisplay->setRowCount(1);
        tableWidgetStimDisplay->setObjectName(QString::fromUtf8("tableWidgetStimDisplay"));
        tableWidgetStimDisplay->setMaximumSize(QSize(16777215, 75));
        tableWidgetStimDisplay->setRowCount(1);
        tableWidgetStimDisplay->setColumnCount(3);

        hboxLayout->addWidget(tableWidgetStimDisplay);

        dwStim->setWidget(dockWidgetContents);
        dwLog = new QDockWidget(centralWidget);
        dwLog->setObjectName(QString::fromUtf8("dwLog"));
        dwLog->setGeometry(QRect(370, 420, 351, 141));
        dockWidgetContents1 = new QWidget();
        dockWidgetContents1->setObjectName(QString::fromUtf8("dockWidgetContents1"));
        vboxLayout = new QVBoxLayout(dockWidgetContents1);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(9, 9, 9, 9);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        label_22 = new QLabel(dockWidgetContents1);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        hboxLayout1->addWidget(label_22);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem);

        spinBoxMaxNumOfLinesInLog = new QSpinBox(dockWidgetContents1);
        spinBoxMaxNumOfLinesInLog->setObjectName(QString::fromUtf8("spinBoxMaxNumOfLinesInLog"));
        spinBoxMaxNumOfLinesInLog->setMinimum(1);
        spinBoxMaxNumOfLinesInLog->setMaximum(9999);
        spinBoxMaxNumOfLinesInLog->setValue(1000);

        hboxLayout1->addWidget(spinBoxMaxNumOfLinesInLog);


        vboxLayout->addLayout(hboxLayout1);

        textEditLog = new QTextEdit(dockWidgetContents1);
        textEditLog->setObjectName(QString::fromUtf8("textEditLog"));

        vboxLayout->addWidget(textEditLog);

        dwLog->setWidget(dockWidgetContents1);
        dwCfg = new QDockWidget(centralWidget);
        dwCfg->setObjectName(QString::fromUtf8("dwCfg"));
        dwCfg->setGeometry(QRect(20, 130, 362, 395));
        dwCfg->setMinimumSize(QSize(362, 395));
        dockWidgetContents2 = new QWidget();
        dockWidgetContents2->setObjectName(QString::fromUtf8("dockWidgetContents2"));
        vboxLayout1 = new QVBoxLayout(dockWidgetContents2);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        tabWidgetCfg = new QTabWidget(dockWidgetContents2);
        tabWidgetCfg->setObjectName(QString::fromUtf8("tabWidgetCfg"));
        tabGeneral = new QWidget();
        tabGeneral->setObjectName(QString::fromUtf8("tabGeneral"));
        vboxLayout2 = new QVBoxLayout(tabGeneral);
        vboxLayout2->setSpacing(6);
        vboxLayout2->setContentsMargins(9, 9, 9, 9);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        label = new QLabel(tabGeneral);
        label->setObjectName(QString::fromUtf8("label"));

        hboxLayout2->addWidget(label);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem1);

        btnSelectFile = new QPushButton(tabGeneral);
        btnSelectFile->setObjectName(QString::fromUtf8("btnSelectFile"));

        hboxLayout2->addWidget(btnSelectFile);


        vboxLayout2->addLayout(hboxLayout2);

        lineEditFilename = new QLineEdit(tabGeneral);
        lineEditFilename->setObjectName(QString::fromUtf8("lineEditFilename"));

        vboxLayout2->addWidget(lineEditFilename);

        label_2 = new QLabel(tabGeneral);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        vboxLayout2->addWidget(label_2);

        textEditComment = new QTextEdit(tabGeneral);
        textEditComment->setObjectName(QString::fromUtf8("textEditComment"));

        vboxLayout2->addWidget(textEditComment);

        tabWidgetCfg->addTab(tabGeneral, QString());
        tabCamera = new QWidget();
        tabCamera->setObjectName(QString::fromUtf8("tabCamera"));
        vboxLayout3 = new QVBoxLayout(tabCamera);
        vboxLayout3->setSpacing(6);
        vboxLayout3->setContentsMargins(9, 9, 9, 9);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        groupBox = new QGroupBox(tabCamera);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(9, 9, 9, 9);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        comboBoxTriggerMode = new QComboBox(groupBox);
        comboBoxTriggerMode->setObjectName(QString::fromUtf8("comboBoxTriggerMode"));

        gridLayout->addWidget(comboBoxTriggerMode, 4, 0, 1, 1);

        cbBaseLineClamp = new QCheckBox(groupBox);
        cbBaseLineClamp->setObjectName(QString::fromUtf8("cbBaseLineClamp"));
        cbBaseLineClamp->setChecked(true);

        gridLayout->addWidget(cbBaseLineClamp, 4, 1, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 2, 0, 1, 1);

        label_5_2 = new QLabel(groupBox);
        label_5_2->setObjectName(QString::fromUtf8("label_5_2"));

        gridLayout->addWidget(label_5_2, 3, 0, 1, 1);

        doubleSpinBoxExpTime = new QDoubleSpinBox(groupBox);
        doubleSpinBoxExpTime->setObjectName(QString::fromUtf8("doubleSpinBoxExpTime"));
        doubleSpinBoxExpTime->setDecimals(3);
        doubleSpinBoxExpTime->setSingleStep(0.001);
        doubleSpinBoxExpTime->setValue(0.05);

        gridLayout->addWidget(doubleSpinBoxExpTime, 2, 1, 1, 1);

        spinBoxNumOfStacks = new QSpinBox(groupBox);
        spinBoxNumOfStacks->setObjectName(QString::fromUtf8("spinBoxNumOfStacks"));
        spinBoxNumOfStacks->setMinimum(1);
        spinBoxNumOfStacks->setMaximum(9999);
        spinBoxNumOfStacks->setValue(2);

        gridLayout->addWidget(spinBoxNumOfStacks, 0, 1, 1, 1);

        spinBoxFramesPerStack = new QSpinBox(groupBox);
        spinBoxFramesPerStack->setObjectName(QString::fromUtf8("spinBoxFramesPerStack"));
        spinBoxFramesPerStack->setMinimum(1);
        spinBoxFramesPerStack->setMaximum(9999);
        spinBoxFramesPerStack->setValue(20);

        gridLayout->addWidget(spinBoxFramesPerStack, 1, 1, 1, 1);

        doubleSpinBoxBoxIdleTimeBtwnStacks = new QDoubleSpinBox(groupBox);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setObjectName(QString::fromUtf8("doubleSpinBoxBoxIdleTimeBtwnStacks"));
        doubleSpinBoxBoxIdleTimeBtwnStacks->setDecimals(3);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setMaximum(10000);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(3);

        gridLayout->addWidget(doubleSpinBoxBoxIdleTimeBtwnStacks, 3, 1, 1, 1);


        vboxLayout3->addWidget(groupBox);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        groupBox_2 = new QGroupBox(tabCamera);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout1 = new QGridLayout(groupBox_2);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(9, 9, 9, 9);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        comboBoxPreAmpGains = new QComboBox(groupBox_2);
        comboBoxPreAmpGains->setObjectName(QString::fromUtf8("comboBoxPreAmpGains"));

        gridLayout1->addWidget(comboBoxPreAmpGains, 1, 1, 1, 1);

        label_14 = new QLabel(groupBox_2);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        gridLayout1->addWidget(label_14, 2, 0, 1, 1);

        spinBoxEmGain = new QSpinBox(groupBox_2);
        spinBoxEmGain->setObjectName(QString::fromUtf8("spinBoxEmGain"));
        spinBoxEmGain->setMaximum(255);

        gridLayout1->addWidget(spinBoxEmGain, 2, 1, 1, 1);

        label_16 = new QLabel(groupBox_2);
        label_16->setObjectName(QString::fromUtf8("label_16"));

        gridLayout1->addWidget(label_16, 0, 0, 1, 1);

        comboBoxHorReadoutRate = new QComboBox(groupBox_2);
        comboBoxHorReadoutRate->setObjectName(QString::fromUtf8("comboBoxHorReadoutRate"));

        gridLayout1->addWidget(comboBoxHorReadoutRate, 0, 1, 1, 1);

        label_15 = new QLabel(groupBox_2);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        gridLayout1->addWidget(label_15, 1, 0, 1, 1);


        hboxLayout3->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(tabCamera);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        gridLayout2 = new QGridLayout(groupBox_3);
        gridLayout2->setSpacing(6);
        gridLayout2->setContentsMargins(9, 9, 9, 9);
        gridLayout2->setObjectName(QString::fromUtf8("gridLayout2"));
        label_18 = new QLabel(groupBox_3);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setWordWrap(true);

        gridLayout2->addWidget(label_18, 1, 0, 1, 2);

        comboBoxVertClockVolAmp = new QComboBox(groupBox_3);
        comboBoxVertClockVolAmp->setObjectName(QString::fromUtf8("comboBoxVertClockVolAmp"));

        gridLayout2->addWidget(comboBoxVertClockVolAmp, 2, 1, 1, 1);

        label_17 = new QLabel(groupBox_3);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        gridLayout2->addWidget(label_17, 0, 0, 1, 1);

        comboBoxVertShiftSpeed = new QComboBox(groupBox_3);
        comboBoxVertShiftSpeed->setObjectName(QString::fromUtf8("comboBoxVertShiftSpeed"));

        gridLayout2->addWidget(comboBoxVertShiftSpeed, 0, 1, 1, 1);


        hboxLayout3->addWidget(groupBox_3);


        vboxLayout3->addLayout(hboxLayout3);

        tabWidgetCfg->addTab(tabCamera, QString());
        tabBinning = new QWidget();
        tabBinning->setObjectName(QString::fromUtf8("tabBinning"));
        label_26 = new QLabel(tabBinning);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setGeometry(QRect(20, 20, 47, 14));
        label_21 = new QLabel(tabBinning);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        label_21->setGeometry(QRect(140, 20, 47, 14));
        label_25 = new QLabel(tabBinning);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setGeometry(QRect(140, 60, 47, 14));
        label_24 = new QLabel(tabBinning);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setGeometry(QRect(20, 60, 47, 14));
        spinBoxHstart = new QSpinBox(tabBinning);
        spinBoxHstart->setObjectName(QString::fromUtf8("spinBoxHstart"));
        spinBoxHstart->setGeometry(QRect(80, 20, 51, 22));
        spinBoxHstart->setMinimum(1);
        spinBoxHstart->setMaximum(1004);
        spinBoxHend = new QSpinBox(tabBinning);
        spinBoxHend->setObjectName(QString::fromUtf8("spinBoxHend"));
        spinBoxHend->setGeometry(QRect(201, 20, 61, 22));
        spinBoxHend->setMinimum(1);
        spinBoxHend->setMaximum(2560);
        spinBoxHend->setValue(1);
        spinBoxVstart = new QSpinBox(tabBinning);
        spinBoxVstart->setObjectName(QString::fromUtf8("spinBoxVstart"));
        spinBoxVstart->setGeometry(QRect(80, 60, 51, 22));
        spinBoxVstart->setMinimum(1);
        spinBoxVstart->setMaximum(1002);
        spinBoxVend = new QSpinBox(tabBinning);
        spinBoxVend->setObjectName(QString::fromUtf8("spinBoxVend"));
        spinBoxVend->setGeometry(QRect(200, 60, 61, 22));
        spinBoxVend->setMinimum(1);
        spinBoxVend->setMaximum(2160);
        spinBoxVend->setValue(2160);
        btnUseZoomWindow = new QPushButton(tabBinning);
        btnUseZoomWindow->setObjectName(QString::fromUtf8("btnUseZoomWindow"));
        btnUseZoomWindow->setGeometry(QRect(24, 100, 241, 23));
        btnFullChipSize = new QPushButton(tabBinning);
        btnFullChipSize->setObjectName(QString::fromUtf8("btnFullChipSize"));
        btnFullChipSize->setGeometry(QRect(24, 130, 241, 23));
        tabWidgetCfg->addTab(tabBinning, QString());
        tabStim = new QWidget();
        tabStim->setObjectName(QString::fromUtf8("tabStim"));
        vboxLayout4 = new QVBoxLayout(tabStim);
        vboxLayout4->setSpacing(6);
        vboxLayout4->setContentsMargins(9, 9, 9, 9);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        cbStim = new QCheckBox(tabStim);
        cbStim->setObjectName(QString::fromUtf8("cbStim"));

        vboxLayout4->addWidget(cbStim);

        hboxLayout4 = new QHBoxLayout();
        hboxLayout4->setSpacing(6);
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        label_13 = new QLabel(tabStim);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        hboxLayout4->addWidget(label_13);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem2);

        btnOpenStimFile = new QPushButton(tabStim);
        btnOpenStimFile->setObjectName(QString::fromUtf8("btnOpenStimFile"));

        hboxLayout4->addWidget(btnOpenStimFile);


        vboxLayout4->addLayout(hboxLayout4);

        lineEditStimFile = new QLineEdit(tabStim);
        lineEditStimFile->setObjectName(QString::fromUtf8("lineEditStimFile"));
        lineEditStimFile->setReadOnly(true);

        vboxLayout4->addWidget(lineEditStimFile);

        textEditStimFileContent = new QTextEdit(tabStim);
        textEditStimFileContent->setObjectName(QString::fromUtf8("textEditStimFileContent"));
        textEditStimFileContent->setLineWrapMode(QTextEdit::NoWrap);
        textEditStimFileContent->setReadOnly(true);

        vboxLayout4->addWidget(textEditStimFileContent);

        tabWidgetCfg->addTab(tabStim, QString());
        tabPiezo = new QWidget();
        tabPiezo->setObjectName(QString::fromUtf8("tabPiezo"));
        label_19 = new QLabel(tabPiezo);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setGeometry(QRect(10, 10, 111, 20));
        doubleSpinBoxMaxDistance = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxMaxDistance->setObjectName(QString::fromUtf8("doubleSpinBoxMaxDistance"));
        doubleSpinBoxMaxDistance->setGeometry(QRect(120, 10, 62, 22));
        doubleSpinBoxMaxDistance->setReadOnly(true);
        doubleSpinBoxMaxDistance->setMaximum(1000);
        doubleSpinBoxMaxDistance->setValue(400);
        label_8 = new QLabel(tabPiezo);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(10, 40, 81, 16));
        label_9 = new QLabel(tabPiezo);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(10, 160, 81, 16));
        doubleSpinBoxStartPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxStartPos->setObjectName(QString::fromUtf8("doubleSpinBoxStartPos"));
        doubleSpinBoxStartPos->setGeometry(QRect(120, 40, 62, 22));
        doubleSpinBoxStartPos->setMaximum(400);
        doubleSpinBoxStopPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxStopPos->setObjectName(QString::fromUtf8("doubleSpinBoxStopPos"));
        doubleSpinBoxStopPos->setGeometry(QRect(120, 160, 62, 22));
        doubleSpinBoxStopPos->setMaximum(400);
        doubleSpinBoxStopPos->setValue(400);
        btnSetCurPosAsStart = new QPushButton(tabPiezo);
        btnSetCurPosAsStart->setObjectName(QString::fromUtf8("btnSetCurPosAsStart"));
        btnSetCurPosAsStart->setGeometry(QRect(190, 40, 51, 23));
        btnSetCurPosAsStop = new QPushButton(tabPiezo);
        btnSetCurPosAsStop->setObjectName(QString::fromUtf8("btnSetCurPosAsStop"));
        btnSetCurPosAsStop->setGeometry(QRect(190, 160, 51, 23));
        label_20 = new QLabel(tabPiezo);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setGeometry(QRect(10, 102, 91, 16));
        doubleSpinBoxCurPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxCurPos->setObjectName(QString::fromUtf8("doubleSpinBoxCurPos"));
        doubleSpinBoxCurPos->setGeometry(QRect(120, 101, 62, 22));
        doubleSpinBoxCurPos->setMaximum(400);
        doubleSpinBoxCurPos->setValue(200);
        btnMovePiezo = new QPushButton(tabPiezo);
        btnMovePiezo->setObjectName(QString::fromUtf8("btnMovePiezo"));
        btnMovePiezo->setGeometry(QRect(190, 101, 51, 23));
        btnFastIncPosAndMove = new QPushButton(tabPiezo);
        btnFastIncPosAndMove->setObjectName(QString::fromUtf8("btnFastIncPosAndMove"));
        btnFastIncPosAndMove->setGeometry(QRect(250, 70, 21, 23));
        btnIncPosAndMove = new QPushButton(tabPiezo);
        btnIncPosAndMove->setObjectName(QString::fromUtf8("btnIncPosAndMove"));
        btnIncPosAndMove->setGeometry(QRect(250, 90, 21, 23));
        btnDecPosAndMove = new QPushButton(tabPiezo);
        btnDecPosAndMove->setObjectName(QString::fromUtf8("btnDecPosAndMove"));
        btnDecPosAndMove->setGeometry(QRect(250, 110, 21, 23));
        btnFastDecPosAndMove = new QPushButton(tabPiezo);
        btnFastDecPosAndMove->setObjectName(QString::fromUtf8("btnFastDecPosAndMove"));
        btnFastDecPosAndMove->setGeometry(QRect(250, 130, 21, 23));
        tabWidgetCfg->addTab(tabPiezo, QString());
        tabShutter = new QWidget();
        tabShutter->setObjectName(QString::fromUtf8("tabShutter"));
        label_11 = new QLabel(tabShutter);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(60, 70, 131, 16));
        label_12 = new QLabel(tabShutter);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setGeometry(QRect(60, 170, 161, 16));
        tabWidgetCfg->addTab(tabShutter, QString());
        tabAI = new QWidget();
        tabAI->setObjectName(QString::fromUtf8("tabAI"));
        label_6 = new QLabel(tabAI);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(40, 40, 91, 16));
        label_7 = new QLabel(tabAI);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(40, 120, 81, 16));
        tabWidgetCfg->addTab(tabAI, QString());

        vboxLayout1->addWidget(tabWidgetCfg);

        btnApply = new QPushButton(dockWidgetContents2);
        btnApply->setObjectName(QString::fromUtf8("btnApply"));

        vboxLayout1->addWidget(btnApply);

        dwCfg->setWidget(dockWidgetContents2);
        labelImage = new ImageLabel(centralWidget);
        labelImage->setObjectName(QString::fromUtf8("labelImage"));
        labelImage->setGeometry(QRect(400, 300, 121, 101));
        dwHist = new QDockWidget(centralWidget);
        dwHist->setObjectName(QString::fromUtf8("dwHist"));
        dwHist->setGeometry(QRect(400, 240, 274, 51));
        dwHist->setMinimumSize(QSize(80, 40));
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName(QString::fromUtf8("dockWidgetContents_2"));
        vboxLayout5 = new QVBoxLayout(dockWidgetContents_2);
        vboxLayout5->setSpacing(6);
        vboxLayout5->setContentsMargins(11, 11, 11, 11);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        dwHist->setWidget(dockWidgetContents_2);
        dwIntenCurve = new QDockWidget(centralWidget);
        dwIntenCurve->setObjectName(QString::fromUtf8("dwIntenCurve"));
        dwIntenCurve->setGeometry(QRect(450, 10, 241, 121));
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName(QString::fromUtf8("dockWidgetContents_3"));
        vboxLayout6 = new QVBoxLayout(dockWidgetContents_3);
        vboxLayout6->setSpacing(6);
        vboxLayout6->setContentsMargins(11, 11, 11, 11);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        frameIntenCurve = new QFrame(dockWidgetContents_3);
        frameIntenCurve->setObjectName(QString::fromUtf8("frameIntenCurve"));
        frameIntenCurve->setFrameShape(QFrame::StyledPanel);
        frameIntenCurve->setFrameShadow(QFrame::Raised);

        vboxLayout6->addWidget(frameIntenCurve);

        dwIntenCurve->setWidget(dockWidgetContents_3);
        ImagineClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(ImagineClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 763, 21));
        menu_File = new QMenu(menuBar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menuShutter = new QMenu(menuBar);
        menuShutter->setObjectName(QString::fromUtf8("menuShutter"));
        menu_Acquisition = new QMenu(menuBar);
        menu_Acquisition->setObjectName(QString::fromUtf8("menu_Acquisition"));
        menuView = new QMenu(menuBar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        ImagineClass->setMenuBar(menuBar);
        toolBar = new QToolBar(ImagineClass);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setOrientation(Qt::Horizontal);
        ImagineClass->addToolBar(Qt::TopToolBarArea, toolBar);
        toolBar_2 = new QToolBar(ImagineClass);
        toolBar_2->setObjectName(QString::fromUtf8("toolBar_2"));
        toolBar_2->setOrientation(Qt::Horizontal);
        ImagineClass->addToolBar(Qt::TopToolBarArea, toolBar_2);
        toolBar_3 = new QToolBar(ImagineClass);
        toolBar_3->setObjectName(QString::fromUtf8("toolBar_3"));
        toolBar_3->setOrientation(Qt::Horizontal);
        ImagineClass->addToolBar(Qt::TopToolBarArea, toolBar_3);

        menuBar->addAction(menu_File->menuAction());
        menuBar->addAction(menu_Acquisition->menuAction());
        menuBar->addAction(menuShutter->menuAction());
        menuBar->addAction(menuView->menuAction());
        menu_File->addAction(actionExit);
        menuShutter->addAction(actionOpenShutter);
        menuShutter->addAction(actionCloseShutter);
        menuShutter->addSeparator();
        menuShutter->addAction(actionTemperature);
        menuShutter->addAction(actionHeatsinkFan);
        menu_Acquisition->addAction(actionStartAcqAndSave);
        menu_Acquisition->addAction(actionStartLive);
        menu_Acquisition->addAction(actionStop);
        menuView->addAction(actionDisplayFullImage);
        menuView->addSeparator();
        menuView->addAction(actionNoAutoScale);
        menuView->addAction(actionAutoScaleOnFirstFrame);
        menuView->addAction(actionAutoScaleOnAllFrames);
        menuView->addSeparator();
        menuView->addAction(actionStimuli);
        menuView->addAction(actionConfig);
        menuView->addAction(actionLog);
        menuView->addSeparator();
        menuView->addAction(actionColorizeSaturatedPixels);
        toolBar->addAction(actionStartAcqAndSave);
        toolBar->addAction(actionStartLive);
        toolBar->addAction(actionStop);
        toolBar_2->addAction(actionOpenShutter);
        toolBar_2->addAction(actionCloseShutter);
        toolBar_3->addAction(actionDisplayFullImage);

        retranslateUi(ImagineClass);

        tabWidgetCfg->setCurrentIndex(4);


        QMetaObject::connectSlotsByName(ImagineClass);
    } // setupUi

    void retranslateUi(QMainWindow *ImagineClass)
    {
        ImagineClass->setWindowTitle(QApplication::translate("ImagineClass", "Imagine", 0, QApplication::UnicodeUTF8));
        actionStartAcqAndSave->setText(QApplication::translate("ImagineClass", "&Start acq and save", 0, QApplication::UnicodeUTF8));
        actionStop->setText(QApplication::translate("ImagineClass", "S&top", 0, QApplication::UnicodeUTF8));
        actionOpenShutter->setText(QApplication::translate("ImagineClass", "Open Shutter", 0, QApplication::UnicodeUTF8));
        actionCloseShutter->setText(QApplication::translate("ImagineClass", "Close Shutter", 0, QApplication::UnicodeUTF8));
        action->setText(QApplication::translate("ImagineClass", "-", 0, QApplication::UnicodeUTF8));
        actionTemperature->setText(QApplication::translate("ImagineClass", "Temperature", 0, QApplication::UnicodeUTF8));
        actionStartLive->setText(QApplication::translate("ImagineClass", "Start live", 0, QApplication::UnicodeUTF8));
        actionNoAutoScale->setText(QApplication::translate("ImagineClass", "No Auto Scale", 0, QApplication::UnicodeUTF8));
        actionAutoScaleOnFirstFrame->setText(QApplication::translate("ImagineClass", "Auto Scale On First Frame", 0, QApplication::UnicodeUTF8));
        actionAutoScaleOnAllFrames->setText(QApplication::translate("ImagineClass", "Auto Scale On All Frames", 0, QApplication::UnicodeUTF8));
        actionStimuli->setText(QApplication::translate("ImagineClass", "Stimuli", 0, QApplication::UnicodeUTF8));
        actionConfig->setText(QApplication::translate("ImagineClass", "Config", 0, QApplication::UnicodeUTF8));
        actionLog->setText(QApplication::translate("ImagineClass", "Log", 0, QApplication::UnicodeUTF8));
        actionColorizeSaturatedPixels->setText(QApplication::translate("ImagineClass", "Colorize Saturated Pixels", 0, QApplication::UnicodeUTF8));
        actionDisplayFullImage->setText(QApplication::translate("ImagineClass", "Display Full Image", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("ImagineClass", "Exit", 0, QApplication::UnicodeUTF8));
        actionHeatsinkFan->setText(QApplication::translate("ImagineClass", "Heatsink Fan", 0, QApplication::UnicodeUTF8));
        dwLog->setWindowTitle(QApplication::translate("ImagineClass", "Log", 0, QApplication::UnicodeUTF8));
        label_22->setText(QApplication::translate("ImagineClass", "Max number of lines:", 0, QApplication::UnicodeUTF8));
        dwCfg->setWindowTitle(QApplication::translate("ImagineClass", "Config", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("ImagineClass", "File name:", 0, QApplication::UnicodeUTF8));
        btnSelectFile->setText(QApplication::translate("ImagineClass", "...", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("ImagineClass", "Comment:", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabGeneral), QApplication::translate("ImagineClass", "general", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("ImagineClass", "time/duration", 0, QApplication::UnicodeUTF8));
        cbBaseLineClamp->setText(QApplication::translate("ImagineClass", "Baseline Clamp", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("ImagineClass", "#stacks:", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("ImagineClass", "#frames/stack:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_5->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_5->setText(QApplication::translate("ImagineClass", "exposure time(s):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_5_2->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_5_2->setText(QApplication::translate("ImagineClass", "idle time between stacks(s):", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("ImagineClass", "horizontal pixel shift", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("ImagineClass", "EM gain:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_16->setToolTip(QApplication::translate("ImagineClass", "Horizontal shift speed", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_16->setText(QApplication::translate("ImagineClass", "readout rate:", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("ImagineClass", "pre-amp gain", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("ImagineClass", "vertical pixel shift", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("ImagineClass", "vertical clock voltage amplitude:", 0, QApplication::UnicodeUTF8));
        label_17->setText(QApplication::translate("ImagineClass", "shift speed:", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabCamera), QApplication::translate("ImagineClass", "camera", 0, QApplication::UnicodeUTF8));
        label_26->setText(QApplication::translate("ImagineClass", "hor. start:", 0, QApplication::UnicodeUTF8));
        label_21->setText(QApplication::translate("ImagineClass", "hor. end:", 0, QApplication::UnicodeUTF8));
        label_25->setText(QApplication::translate("ImagineClass", "ver. end:", 0, QApplication::UnicodeUTF8));
        label_24->setText(QApplication::translate("ImagineClass", "ver. start:", 0, QApplication::UnicodeUTF8));
        btnUseZoomWindow->setText(QApplication::translate("ImagineClass", "use values from zoom window", 0, QApplication::UnicodeUTF8));
        btnFullChipSize->setText(QApplication::translate("ImagineClass", "full chip size", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabBinning), QApplication::translate("ImagineClass", "Binning", 0, QApplication::UnicodeUTF8));
        cbStim->setText(QApplication::translate("ImagineClass", "Apply stimuli", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("ImagineClass", "Stimulus file:", 0, QApplication::UnicodeUTF8));
        btnOpenStimFile->setText(QApplication::translate("ImagineClass", "...", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabStim), QApplication::translate("ImagineClass", "stimuli", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("ImagineClass", "min-max distance(um):", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("ImagineClass", "start position:", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("ImagineClass", "stop position:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnSetCurPosAsStart->setToolTip(QApplication::translate("ImagineClass", "set as start position", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnSetCurPosAsStart->setText(QApplication::translate("ImagineClass", "Set", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnSetCurPosAsStop->setToolTip(QApplication::translate("ImagineClass", "set as stop position", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnSetCurPosAsStop->setText(QApplication::translate("ImagineClass", "Set", 0, QApplication::UnicodeUTF8));
        label_20->setText(QApplication::translate("ImagineClass", "move to position:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        doubleSpinBoxCurPos->setToolTip(QApplication::translate("ImagineClass", "Use page up/dow key for fast change", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnMovePiezo->setToolTip(QApplication::translate("ImagineClass", "Move piezo according to user's input", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnMovePiezo->setText(QApplication::translate("ImagineClass", "Move", 0, QApplication::UnicodeUTF8));
        btnFastIncPosAndMove->setText(QApplication::translate("ImagineClass", "^^", 0, QApplication::UnicodeUTF8));
        btnIncPosAndMove->setText(QApplication::translate("ImagineClass", "^", 0, QApplication::UnicodeUTF8));
        btnDecPosAndMove->setText(QApplication::translate("ImagineClass", "v", 0, QApplication::UnicodeUTF8));
        btnFastDecPosAndMove->setText(QApplication::translate("ImagineClass", "vv", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabPiezo), QApplication::translate("ImagineClass", "piezo", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("ImagineClass", "open time(offset 2):", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("ImagineClass", "close time(offset 4):", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabShutter), QApplication::translate("ImagineClass", "shutter", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("ImagineClass", "input rate:", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("ImagineClass", "input range:", 0, QApplication::UnicodeUTF8));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabAI), QApplication::translate("ImagineClass", "AI", 0, QApplication::UnicodeUTF8));
        btnApply->setText(QApplication::translate("ImagineClass", "Apply", 0, QApplication::UnicodeUTF8));
        labelImage->setText(QApplication::translate("ImagineClass", "labelImage", 0, QApplication::UnicodeUTF8));
        dwHist->setWindowTitle(QApplication::translate("ImagineClass", "Histogram", 0, QApplication::UnicodeUTF8));
        dwIntenCurve->setWindowTitle(QApplication::translate("ImagineClass", "Intensity", 0, QApplication::UnicodeUTF8));
        menu_File->setTitle(QApplication::translate("ImagineClass", "&File", 0, QApplication::UnicodeUTF8));
        menuShutter->setTitle(QApplication::translate("ImagineClass", "Hardware", 0, QApplication::UnicodeUTF8));
        menu_Acquisition->setTitle(QApplication::translate("ImagineClass", "&Acquisition", 0, QApplication::UnicodeUTF8));
        menuView->setTitle(QApplication::translate("ImagineClass", "View", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ImagineClass: public Ui_ImagineClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGINE_H

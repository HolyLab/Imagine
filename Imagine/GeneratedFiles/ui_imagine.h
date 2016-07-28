/********************************************************************************
** Form generated from reading UI file 'imagine.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGINE_H
#define UI_IMAGINE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
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
    QAction *actionContrastMin;
    QAction *actionContrastMax;
    QAction *actionManualContrast;
    QAction *actionFlickerControl;
    QAction *actionHistogram;
    QAction *actionIntensity;
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
    QLabel *label_32;
    QLabel *label_3;
    QLabel *label_5;
    QLabel *label_33;
    QLabel *label_5_2;
    QLabel *label_4;
    QSpinBox *spinBoxFramesPerStack;
    QDoubleSpinBox *doubleSpinBoxExpTime;
    QSpinBox *spinBoxNumOfStacks;
    QSpinBox *spinBoxAngle;
    QDoubleSpinBox *doubleSpinBoxUmPerPxlXy;
    QDoubleSpinBox *doubleSpinBoxBoxIdleTimeBtwnStacks;
    QLabel *label_5_3;
    QComboBox *comboBoxTriggerMode;
    QLabel *label_5_4;
    QComboBox *comboBoxTriggerMode_2;
    QHBoxLayout *hboxLayout3;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout1;
    QComboBox *comboBoxPreAmpGains;
    QLabel *label_14;
    QSpinBox *spinBoxGain;
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
    QDoubleSpinBox *doubleSpinBoxMinDistance;
    QLabel *label_23;
    QLabel *label_10;
    QDoubleSpinBox *doubleSpinBoxPiezoTravelBackTime;
    QCheckBox *cbAutoSetPiezoTravelBackTime;
    QLabel *label_27;
    QLabel *labelCurPos;
    QPushButton *btnRefreshPos;
    QLabel *label_28;
    QComboBox *comboBoxAxis;
    QPushButton *btnMoveToStartPos;
    QPushButton *btnMoveToStopPos;
    QCheckBox *cbBidirectionalImaging;
    QLabel *label_30;
    QSpinBox *spinBoxSpinboxSteps;
    QLabel *label_31;
    QSpinBox *spinBoxNumOfDecimalDigits;
    QWidget *tabShutter;
    QLabel *label_11;
    QLabel *label_12;
    QWidget *tabAI;
    QLabel *label_6;
    QLabel *label_7;
    QWidget *tabDisplay;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QLabel *label_29;
    QSpinBox *spinBoxDisplayAreaSize;
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
            ImagineClass->setObjectName(QStringLiteral("ImagineClass"));
        ImagineClass->resize(849, 716);
        actionStartAcqAndSave = new QAction(ImagineClass);
        actionStartAcqAndSave->setObjectName(QStringLiteral("actionStartAcqAndSave"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/Resources/start_acq.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStartAcqAndSave->setIcon(icon);
        actionStop = new QAction(ImagineClass);
        actionStop->setObjectName(QStringLiteral("actionStop"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/Resources/stop.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop->setIcon(icon1);
        actionOpenShutter = new QAction(ImagineClass);
        actionOpenShutter->setObjectName(QStringLiteral("actionOpenShutter"));
        actionCloseShutter = new QAction(ImagineClass);
        actionCloseShutter->setObjectName(QStringLiteral("actionCloseShutter"));
        action = new QAction(ImagineClass);
        action->setObjectName(QStringLiteral("action"));
        actionTemperature = new QAction(ImagineClass);
        actionTemperature->setObjectName(QStringLiteral("actionTemperature"));
        actionStartLive = new QAction(ImagineClass);
        actionStartLive->setObjectName(QStringLiteral("actionStartLive"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/Resources/start_live.bmp"), QSize(), QIcon::Normal, QIcon::Off);
        actionStartLive->setIcon(icon2);
        actionNoAutoScale = new QAction(ImagineClass);
        actionNoAutoScale->setObjectName(QStringLiteral("actionNoAutoScale"));
        actionAutoScaleOnFirstFrame = new QAction(ImagineClass);
        actionAutoScaleOnFirstFrame->setObjectName(QStringLiteral("actionAutoScaleOnFirstFrame"));
        actionAutoScaleOnAllFrames = new QAction(ImagineClass);
        actionAutoScaleOnAllFrames->setObjectName(QStringLiteral("actionAutoScaleOnAllFrames"));
        actionStimuli = new QAction(ImagineClass);
        actionStimuli->setObjectName(QStringLiteral("actionStimuli"));
        actionConfig = new QAction(ImagineClass);
        actionConfig->setObjectName(QStringLiteral("actionConfig"));
        actionLog = new QAction(ImagineClass);
        actionLog->setObjectName(QStringLiteral("actionLog"));
        actionColorizeSaturatedPixels = new QAction(ImagineClass);
        actionColorizeSaturatedPixels->setObjectName(QStringLiteral("actionColorizeSaturatedPixels"));
        actionDisplayFullImage = new QAction(ImagineClass);
        actionDisplayFullImage->setObjectName(QStringLiteral("actionDisplayFullImage"));
        actionExit = new QAction(ImagineClass);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        actionHeatsinkFan = new QAction(ImagineClass);
        actionHeatsinkFan->setObjectName(QStringLiteral("actionHeatsinkFan"));
        actionContrastMin = new QAction(ImagineClass);
        actionContrastMin->setObjectName(QStringLiteral("actionContrastMin"));
        actionContrastMax = new QAction(ImagineClass);
        actionContrastMax->setObjectName(QStringLiteral("actionContrastMax"));
        actionManualContrast = new QAction(ImagineClass);
        actionManualContrast->setObjectName(QStringLiteral("actionManualContrast"));
        actionManualContrast->setCheckable(true);
        actionFlickerControl = new QAction(ImagineClass);
        actionFlickerControl->setObjectName(QStringLiteral("actionFlickerControl"));
        actionFlickerControl->setCheckable(true);
        actionHistogram = new QAction(ImagineClass);
        actionHistogram->setObjectName(QStringLiteral("actionHistogram"));
        actionHistogram->setCheckable(true);
        actionIntensity = new QAction(ImagineClass);
        actionIntensity->setObjectName(QStringLiteral("actionIntensity"));
        actionIntensity->setCheckable(true);
        centralWidget = new QWidget(ImagineClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        dwStim = new QDockWidget(centralWidget);
        dwStim->setObjectName(QStringLiteral("dwStim"));
        dwStim->setGeometry(QRect(0, 0, 421, 120));
        dwStim->setMinimumSize(QSize(89, 111));
        dwStim->setMaximumSize(QSize(524287, 524287));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
        hboxLayout = new QHBoxLayout(dockWidgetContents);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(11, 11, 11, 11);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        tableWidgetStimDisplay = new QTableWidget(dockWidgetContents);
        if (tableWidgetStimDisplay->columnCount() < 3)
            tableWidgetStimDisplay->setColumnCount(3);
        if (tableWidgetStimDisplay->rowCount() < 1)
            tableWidgetStimDisplay->setRowCount(1);
        tableWidgetStimDisplay->setObjectName(QStringLiteral("tableWidgetStimDisplay"));
        tableWidgetStimDisplay->setMaximumSize(QSize(16777215, 75));
        tableWidgetStimDisplay->setRowCount(1);
        tableWidgetStimDisplay->setColumnCount(3);

        hboxLayout->addWidget(tableWidgetStimDisplay);

        dwStim->setWidget(dockWidgetContents);
        dwLog = new QDockWidget(centralWidget);
        dwLog->setObjectName(QStringLiteral("dwLog"));
        dwLog->setGeometry(QRect(400, 420, 351, 141));
        dockWidgetContents1 = new QWidget();
        dockWidgetContents1->setObjectName(QStringLiteral("dockWidgetContents1"));
        vboxLayout = new QVBoxLayout(dockWidgetContents1);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setContentsMargins(9, 9, 9, 9);
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        label_22 = new QLabel(dockWidgetContents1);
        label_22->setObjectName(QStringLiteral("label_22"));

        hboxLayout1->addWidget(label_22);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem);

        spinBoxMaxNumOfLinesInLog = new QSpinBox(dockWidgetContents1);
        spinBoxMaxNumOfLinesInLog->setObjectName(QStringLiteral("spinBoxMaxNumOfLinesInLog"));
        spinBoxMaxNumOfLinesInLog->setMinimum(1);
        spinBoxMaxNumOfLinesInLog->setMaximum(999999);
        spinBoxMaxNumOfLinesInLog->setValue(9000);

        hboxLayout1->addWidget(spinBoxMaxNumOfLinesInLog);


        vboxLayout->addLayout(hboxLayout1);

        textEditLog = new QTextEdit(dockWidgetContents1);
        textEditLog->setObjectName(QStringLiteral("textEditLog"));

        vboxLayout->addWidget(textEditLog);

        dwLog->setWidget(dockWidgetContents1);
        dwCfg = new QDockWidget(centralWidget);
        dwCfg->setObjectName(QStringLiteral("dwCfg"));
        dwCfg->setGeometry(QRect(20, 130, 362, 481));
        dwCfg->setMinimumSize(QSize(362, 479));
        dockWidgetContents2 = new QWidget();
        dockWidgetContents2->setObjectName(QStringLiteral("dockWidgetContents2"));
        vboxLayout1 = new QVBoxLayout(dockWidgetContents2);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
        tabWidgetCfg = new QTabWidget(dockWidgetContents2);
        tabWidgetCfg->setObjectName(QStringLiteral("tabWidgetCfg"));
        tabWidgetCfg->setMinimumSize(QSize(0, 410));
        tabGeneral = new QWidget();
        tabGeneral->setObjectName(QStringLiteral("tabGeneral"));
        vboxLayout2 = new QVBoxLayout(tabGeneral);
        vboxLayout2->setSpacing(6);
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        vboxLayout2->setContentsMargins(9, 9, 9, 9);
        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(tabGeneral);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout2->addWidget(label);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem1);

        btnSelectFile = new QPushButton(tabGeneral);
        btnSelectFile->setObjectName(QStringLiteral("btnSelectFile"));

        hboxLayout2->addWidget(btnSelectFile);


        vboxLayout2->addLayout(hboxLayout2);

        lineEditFilename = new QLineEdit(tabGeneral);
        lineEditFilename->setObjectName(QStringLiteral("lineEditFilename"));

        vboxLayout2->addWidget(lineEditFilename);

        label_2 = new QLabel(tabGeneral);
        label_2->setObjectName(QStringLiteral("label_2"));

        vboxLayout2->addWidget(label_2);

        textEditComment = new QTextEdit(tabGeneral);
        textEditComment->setObjectName(QStringLiteral("textEditComment"));

        vboxLayout2->addWidget(textEditComment);

        tabWidgetCfg->addTab(tabGeneral, QString());
        tabCamera = new QWidget();
        tabCamera->setObjectName(QStringLiteral("tabCamera"));
        vboxLayout3 = new QVBoxLayout(tabCamera);
        vboxLayout3->setSpacing(6);
        vboxLayout3->setContentsMargins(11, 11, 11, 11);
        vboxLayout3->setObjectName(QStringLiteral("vboxLayout3"));
        vboxLayout3->setContentsMargins(9, 9, 9, 9);
        groupBox = new QGroupBox(tabCamera);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(9, 9, 9, 9);
        label_32 = new QLabel(groupBox);
        label_32->setObjectName(QStringLiteral("label_32"));

        gridLayout->addWidget(label_32, 7, 0, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 2, 0, 1, 1);

        label_33 = new QLabel(groupBox);
        label_33->setObjectName(QStringLiteral("label_33"));

        gridLayout->addWidget(label_33, 8, 0, 1, 1);

        label_5_2 = new QLabel(groupBox);
        label_5_2->setObjectName(QStringLiteral("label_5_2"));

        gridLayout->addWidget(label_5_2, 3, 0, 1, 1);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        spinBoxFramesPerStack = new QSpinBox(groupBox);
        spinBoxFramesPerStack->setObjectName(QStringLiteral("spinBoxFramesPerStack"));
        spinBoxFramesPerStack->setMinimum(1);
        spinBoxFramesPerStack->setMaximum(999999999);
        spinBoxFramesPerStack->setValue(100);

        gridLayout->addWidget(spinBoxFramesPerStack, 1, 1, 1, 1);

        doubleSpinBoxExpTime = new QDoubleSpinBox(groupBox);
        doubleSpinBoxExpTime->setObjectName(QStringLiteral("doubleSpinBoxExpTime"));
        doubleSpinBoxExpTime->setDecimals(4);
        doubleSpinBoxExpTime->setMaximum(1e+9);
        doubleSpinBoxExpTime->setSingleStep(0.001);
        doubleSpinBoxExpTime->setValue(0.0107);

        gridLayout->addWidget(doubleSpinBoxExpTime, 2, 1, 1, 1);

        spinBoxNumOfStacks = new QSpinBox(groupBox);
        spinBoxNumOfStacks->setObjectName(QStringLiteral("spinBoxNumOfStacks"));
        spinBoxNumOfStacks->setMinimum(1);
        spinBoxNumOfStacks->setMaximum(999999999);
        spinBoxNumOfStacks->setValue(3);

        gridLayout->addWidget(spinBoxNumOfStacks, 0, 1, 1, 1);

        spinBoxAngle = new QSpinBox(groupBox);
        spinBoxAngle->setObjectName(QStringLiteral("spinBoxAngle"));
        spinBoxAngle->setMinimum(-1);
        spinBoxAngle->setMaximum(360);
        spinBoxAngle->setValue(-1);

        gridLayout->addWidget(spinBoxAngle, 7, 1, 1, 1);

        doubleSpinBoxUmPerPxlXy = new QDoubleSpinBox(groupBox);
        doubleSpinBoxUmPerPxlXy->setObjectName(QStringLiteral("doubleSpinBoxUmPerPxlXy"));
        doubleSpinBoxUmPerPxlXy->setDecimals(4);
        doubleSpinBoxUmPerPxlXy->setMinimum(-1);
        doubleSpinBoxUmPerPxlXy->setValue(-1);

        gridLayout->addWidget(doubleSpinBoxUmPerPxlXy, 8, 1, 1, 1);

        doubleSpinBoxBoxIdleTimeBtwnStacks = new QDoubleSpinBox(groupBox);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setObjectName(QStringLiteral("doubleSpinBoxBoxIdleTimeBtwnStacks"));
        doubleSpinBoxBoxIdleTimeBtwnStacks->setDecimals(3);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setMaximum(1e+9);
        doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(0.7);

        gridLayout->addWidget(doubleSpinBoxBoxIdleTimeBtwnStacks, 3, 1, 1, 1);

        label_5_3 = new QLabel(groupBox);
        label_5_3->setObjectName(QStringLiteral("label_5_3"));

        gridLayout->addWidget(label_5_3, 4, 0, 1, 1);

        comboBoxTriggerMode = new QComboBox(groupBox);
        comboBoxTriggerMode->setObjectName(QStringLiteral("comboBoxTriggerMode"));

        gridLayout->addWidget(comboBoxTriggerMode, 4, 1, 1, 1);

        label_5_4 = new QLabel(groupBox);
        label_5_4->setObjectName(QStringLiteral("label_5_4"));

        gridLayout->addWidget(label_5_4, 6, 0, 1, 1);

        comboBoxTriggerMode_2 = new QComboBox(groupBox);
        comboBoxTriggerMode_2->setObjectName(QStringLiteral("comboBoxTriggerMode_2"));

        gridLayout->addWidget(comboBoxTriggerMode_2, 6, 1, 1, 1);


        vboxLayout3->addWidget(groupBox);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setObjectName(QStringLiteral("hboxLayout3"));
        hboxLayout3->setContentsMargins(0, 0, 0, 0);
        groupBox_2 = new QGroupBox(tabCamera);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        gridLayout1 = new QGridLayout(groupBox_2);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName(QStringLiteral("gridLayout1"));
        gridLayout1->setContentsMargins(9, 9, 9, 9);
        comboBoxPreAmpGains = new QComboBox(groupBox_2);
        comboBoxPreAmpGains->setObjectName(QStringLiteral("comboBoxPreAmpGains"));

        gridLayout1->addWidget(comboBoxPreAmpGains, 1, 1, 1, 1);

        label_14 = new QLabel(groupBox_2);
        label_14->setObjectName(QStringLiteral("label_14"));

        gridLayout1->addWidget(label_14, 2, 0, 1, 1);

        spinBoxGain = new QSpinBox(groupBox_2);
        spinBoxGain->setObjectName(QStringLiteral("spinBoxGain"));
        spinBoxGain->setMaximum(255);

        gridLayout1->addWidget(spinBoxGain, 2, 1, 1, 1);

        label_16 = new QLabel(groupBox_2);
        label_16->setObjectName(QStringLiteral("label_16"));

        gridLayout1->addWidget(label_16, 0, 0, 1, 1);

        comboBoxHorReadoutRate = new QComboBox(groupBox_2);
        comboBoxHorReadoutRate->setObjectName(QStringLiteral("comboBoxHorReadoutRate"));

        gridLayout1->addWidget(comboBoxHorReadoutRate, 0, 1, 1, 1);

        label_15 = new QLabel(groupBox_2);
        label_15->setObjectName(QStringLiteral("label_15"));

        gridLayout1->addWidget(label_15, 1, 0, 1, 1);


        hboxLayout3->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(tabCamera);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        gridLayout2 = new QGridLayout(groupBox_3);
        gridLayout2->setSpacing(6);
        gridLayout2->setContentsMargins(11, 11, 11, 11);
        gridLayout2->setObjectName(QStringLiteral("gridLayout2"));
        gridLayout2->setContentsMargins(9, 9, 9, 9);
        label_18 = new QLabel(groupBox_3);
        label_18->setObjectName(QStringLiteral("label_18"));
        label_18->setWordWrap(true);

        gridLayout2->addWidget(label_18, 1, 0, 1, 2);

        comboBoxVertClockVolAmp = new QComboBox(groupBox_3);
        comboBoxVertClockVolAmp->setObjectName(QStringLiteral("comboBoxVertClockVolAmp"));

        gridLayout2->addWidget(comboBoxVertClockVolAmp, 2, 1, 1, 1);

        label_17 = new QLabel(groupBox_3);
        label_17->setObjectName(QStringLiteral("label_17"));

        gridLayout2->addWidget(label_17, 0, 0, 1, 1);

        comboBoxVertShiftSpeed = new QComboBox(groupBox_3);
        comboBoxVertShiftSpeed->setObjectName(QStringLiteral("comboBoxVertShiftSpeed"));

        gridLayout2->addWidget(comboBoxVertShiftSpeed, 0, 1, 1, 1);


        hboxLayout3->addWidget(groupBox_3);


        vboxLayout3->addLayout(hboxLayout3);

        tabWidgetCfg->addTab(tabCamera, QString());
        tabBinning = new QWidget();
        tabBinning->setObjectName(QStringLiteral("tabBinning"));
        label_26 = new QLabel(tabBinning);
        label_26->setObjectName(QStringLiteral("label_26"));
        label_26->setGeometry(QRect(20, 20, 47, 14));
        label_21 = new QLabel(tabBinning);
        label_21->setObjectName(QStringLiteral("label_21"));
        label_21->setGeometry(QRect(140, 20, 47, 14));
        label_25 = new QLabel(tabBinning);
        label_25->setObjectName(QStringLiteral("label_25"));
        label_25->setGeometry(QRect(140, 60, 47, 14));
        label_24 = new QLabel(tabBinning);
        label_24->setObjectName(QStringLiteral("label_24"));
        label_24->setGeometry(QRect(20, 60, 47, 14));
        spinBoxHstart = new QSpinBox(tabBinning);
        spinBoxHstart->setObjectName(QStringLiteral("spinBoxHstart"));
        spinBoxHstart->setGeometry(QRect(80, 20, 51, 22));
        spinBoxHstart->setMinimum(1);
        spinBoxHstart->setMaximum(1004);
        spinBoxHend = new QSpinBox(tabBinning);
        spinBoxHend->setObjectName(QStringLiteral("spinBoxHend"));
        spinBoxHend->setGeometry(QRect(201, 20, 61, 22));
        spinBoxHend->setMinimum(1);
        spinBoxHend->setMaximum(2560);
        spinBoxHend->setValue(8);
        spinBoxVstart = new QSpinBox(tabBinning);
        spinBoxVstart->setObjectName(QStringLiteral("spinBoxVstart"));
        spinBoxVstart->setGeometry(QRect(80, 60, 51, 22));
        spinBoxVstart->setMinimum(1);
        spinBoxVstart->setMaximum(1002);
        spinBoxVend = new QSpinBox(tabBinning);
        spinBoxVend->setObjectName(QStringLiteral("spinBoxVend"));
        spinBoxVend->setGeometry(QRect(200, 60, 61, 22));
        spinBoxVend->setMinimum(1);
        spinBoxVend->setMaximum(2160);
        spinBoxVend->setValue(8);
        btnUseZoomWindow = new QPushButton(tabBinning);
        btnUseZoomWindow->setObjectName(QStringLiteral("btnUseZoomWindow"));
        btnUseZoomWindow->setGeometry(QRect(24, 100, 241, 23));
        btnFullChipSize = new QPushButton(tabBinning);
        btnFullChipSize->setObjectName(QStringLiteral("btnFullChipSize"));
        btnFullChipSize->setGeometry(QRect(24, 130, 241, 23));
        tabWidgetCfg->addTab(tabBinning, QString());
        tabStim = new QWidget();
        tabStim->setObjectName(QStringLiteral("tabStim"));
        vboxLayout4 = new QVBoxLayout(tabStim);
        vboxLayout4->setSpacing(6);
        vboxLayout4->setContentsMargins(11, 11, 11, 11);
        vboxLayout4->setObjectName(QStringLiteral("vboxLayout4"));
        vboxLayout4->setContentsMargins(9, 9, 9, 9);
        cbStim = new QCheckBox(tabStim);
        cbStim->setObjectName(QStringLiteral("cbStim"));

        vboxLayout4->addWidget(cbStim);

        hboxLayout4 = new QHBoxLayout();
        hboxLayout4->setSpacing(6);
        hboxLayout4->setObjectName(QStringLiteral("hboxLayout4"));
        hboxLayout4->setContentsMargins(0, 0, 0, 0);
        label_13 = new QLabel(tabStim);
        label_13->setObjectName(QStringLiteral("label_13"));

        hboxLayout4->addWidget(label_13);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem2);

        btnOpenStimFile = new QPushButton(tabStim);
        btnOpenStimFile->setObjectName(QStringLiteral("btnOpenStimFile"));

        hboxLayout4->addWidget(btnOpenStimFile);


        vboxLayout4->addLayout(hboxLayout4);

        lineEditStimFile = new QLineEdit(tabStim);
        lineEditStimFile->setObjectName(QStringLiteral("lineEditStimFile"));
        lineEditStimFile->setReadOnly(true);

        vboxLayout4->addWidget(lineEditStimFile);

        textEditStimFileContent = new QTextEdit(tabStim);
        textEditStimFileContent->setObjectName(QStringLiteral("textEditStimFileContent"));
        textEditStimFileContent->setLineWrapMode(QTextEdit::NoWrap);
        textEditStimFileContent->setReadOnly(true);

        vboxLayout4->addWidget(textEditStimFileContent);

        tabWidgetCfg->addTab(tabStim, QString());
        tabPiezo = new QWidget();
        tabPiezo->setObjectName(QStringLiteral("tabPiezo"));
        label_19 = new QLabel(tabPiezo);
        label_19->setObjectName(QStringLiteral("label_19"));
        label_19->setGeometry(QRect(10, 70, 111, 20));
        doubleSpinBoxMaxDistance = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxMaxDistance->setObjectName(QStringLiteral("doubleSpinBoxMaxDistance"));
        doubleSpinBoxMaxDistance->setEnabled(false);
        doubleSpinBoxMaxDistance->setGeometry(QRect(130, 70, 100, 22));
        doubleSpinBoxMaxDistance->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxMaxDistance->setReadOnly(true);
        doubleSpinBoxMaxDistance->setMaximum(1e+6);
        doubleSpinBoxMaxDistance->setValue(400);
        label_8 = new QLabel(tabPiezo);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(10, 100, 91, 16));
        label_9 = new QLabel(tabPiezo);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(10, 210, 91, 16));
        doubleSpinBoxStartPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxStartPos->setObjectName(QStringLiteral("doubleSpinBoxStartPos"));
        doubleSpinBoxStartPos->setGeometry(QRect(130, 100, 100, 22));
        doubleSpinBoxStartPos->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxStartPos->setAccelerated(true);
        doubleSpinBoxStartPos->setDecimals(0);
        doubleSpinBoxStartPos->setMaximum(1e+6);
        doubleSpinBoxStopPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxStopPos->setObjectName(QStringLiteral("doubleSpinBoxStopPos"));
        doubleSpinBoxStopPos->setGeometry(QRect(130, 210, 100, 22));
        doubleSpinBoxStopPos->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxStopPos->setAccelerated(true);
        doubleSpinBoxStopPos->setDecimals(0);
        doubleSpinBoxStopPos->setMaximum(1e+6);
        doubleSpinBoxStopPos->setValue(400);
        btnSetCurPosAsStart = new QPushButton(tabPiezo);
        btnSetCurPosAsStart->setObjectName(QStringLiteral("btnSetCurPosAsStart"));
        btnSetCurPosAsStart->setGeometry(QRect(240, 129, 70, 23));
        btnSetCurPosAsStop = new QPushButton(tabPiezo);
        btnSetCurPosAsStop->setObjectName(QStringLiteral("btnSetCurPosAsStop"));
        btnSetCurPosAsStop->setGeometry(QRect(240, 173, 70, 23));
        label_20 = new QLabel(tabPiezo);
        label_20->setObjectName(QStringLiteral("label_20"));
        label_20->setGeometry(QRect(10, 152, 111, 16));
        doubleSpinBoxCurPos = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxCurPos->setObjectName(QStringLiteral("doubleSpinBoxCurPos"));
        doubleSpinBoxCurPos->setGeometry(QRect(130, 151, 100, 22));
        doubleSpinBoxCurPos->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxCurPos->setAccelerated(true);
        doubleSpinBoxCurPos->setDecimals(0);
        doubleSpinBoxCurPos->setMinimum(-1);
        doubleSpinBoxCurPos->setMaximum(1e+6);
        doubleSpinBoxCurPos->setValue(200);
        btnMovePiezo = new QPushButton(tabPiezo);
        btnMovePiezo->setObjectName(QStringLiteral("btnMovePiezo"));
        btnMovePiezo->setGeometry(QRect(240, 151, 70, 23));
        btnFastIncPosAndMove = new QPushButton(tabPiezo);
        btnFastIncPosAndMove->setObjectName(QStringLiteral("btnFastIncPosAndMove"));
        btnFastIncPosAndMove->setGeometry(QRect(310, 120, 21, 23));
        btnIncPosAndMove = new QPushButton(tabPiezo);
        btnIncPosAndMove->setObjectName(QStringLiteral("btnIncPosAndMove"));
        btnIncPosAndMove->setGeometry(QRect(310, 140, 21, 23));
        btnDecPosAndMove = new QPushButton(tabPiezo);
        btnDecPosAndMove->setObjectName(QStringLiteral("btnDecPosAndMove"));
        btnDecPosAndMove->setGeometry(QRect(310, 160, 21, 23));
        btnFastDecPosAndMove = new QPushButton(tabPiezo);
        btnFastDecPosAndMove->setObjectName(QStringLiteral("btnFastDecPosAndMove"));
        btnFastDecPosAndMove->setGeometry(QRect(310, 180, 21, 23));
        doubleSpinBoxMinDistance = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxMinDistance->setObjectName(QStringLiteral("doubleSpinBoxMinDistance"));
        doubleSpinBoxMinDistance->setEnabled(false);
        doubleSpinBoxMinDistance->setGeometry(QRect(130, 40, 100, 22));
        doubleSpinBoxMinDistance->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxMinDistance->setReadOnly(true);
        doubleSpinBoxMinDistance->setMaximum(1e+6);
        doubleSpinBoxMinDistance->setValue(0);
        label_23 = new QLabel(tabPiezo);
        label_23->setObjectName(QStringLiteral("label_23"));
        label_23->setGeometry(QRect(10, 40, 111, 20));
        label_10 = new QLabel(tabPiezo);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(10, 340, 111, 21));
        label_10->setLineWidth(3);
        doubleSpinBoxPiezoTravelBackTime = new QDoubleSpinBox(tabPiezo);
        doubleSpinBoxPiezoTravelBackTime->setObjectName(QStringLiteral("doubleSpinBoxPiezoTravelBackTime"));
        doubleSpinBoxPiezoTravelBackTime->setEnabled(true);
        doubleSpinBoxPiezoTravelBackTime->setGeometry(QRect(130, 340, 100, 20));
        doubleSpinBoxPiezoTravelBackTime->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        doubleSpinBoxPiezoTravelBackTime->setAccelerated(true);
        doubleSpinBoxPiezoTravelBackTime->setDecimals(3);
        doubleSpinBoxPiezoTravelBackTime->setMaximum(10000);
        doubleSpinBoxPiezoTravelBackTime->setValue(0);
        cbAutoSetPiezoTravelBackTime = new QCheckBox(tabPiezo);
        cbAutoSetPiezoTravelBackTime->setObjectName(QStringLiteral("cbAutoSetPiezoTravelBackTime"));
        cbAutoSetPiezoTravelBackTime->setGeometry(QRect(240, 340, 70, 17));
        cbAutoSetPiezoTravelBackTime->setChecked(false);
        label_27 = new QLabel(tabPiezo);
        label_27->setObjectName(QStringLiteral("label_27"));
        label_27->setGeometry(QRect(10, 305, 101, 16));
        labelCurPos = new QLabel(tabPiezo);
        labelCurPos->setObjectName(QStringLiteral("labelCurPos"));
        labelCurPos->setGeometry(QRect(130, 305, 70, 16));
        btnRefreshPos = new QPushButton(tabPiezo);
        btnRefreshPos->setObjectName(QStringLiteral("btnRefreshPos"));
        btnRefreshPos->setGeometry(QRect(240, 300, 70, 23));
        label_28 = new QLabel(tabPiezo);
        label_28->setObjectName(QStringLiteral("label_28"));
        label_28->setGeometry(QRect(10, 10, 111, 21));
        label_28->setLineWidth(3);
        comboBoxAxis = new QComboBox(tabPiezo);
        comboBoxAxis->setObjectName(QStringLiteral("comboBoxAxis"));
        comboBoxAxis->setGeometry(QRect(130, 10, 101, 22));
        btnMoveToStartPos = new QPushButton(tabPiezo);
        btnMoveToStartPos->setObjectName(QStringLiteral("btnMoveToStartPos"));
        btnMoveToStartPos->setGeometry(QRect(240, 100, 70, 23));
        btnMoveToStopPos = new QPushButton(tabPiezo);
        btnMoveToStopPos->setObjectName(QStringLiteral("btnMoveToStopPos"));
        btnMoveToStopPos->setGeometry(QRect(240, 210, 70, 23));
        cbBidirectionalImaging = new QCheckBox(tabPiezo);
        cbBidirectionalImaging->setObjectName(QStringLiteral("cbBidirectionalImaging"));
        cbBidirectionalImaging->setGeometry(QRect(130, 370, 141, 17));
        label_30 = new QLabel(tabPiezo);
        label_30->setObjectName(QStringLiteral("label_30"));
        label_30->setGeometry(QRect(10, 240, 91, 16));
        spinBoxSpinboxSteps = new QSpinBox(tabPiezo);
        spinBoxSpinboxSteps->setObjectName(QStringLiteral("spinBoxSpinboxSteps"));
        spinBoxSpinboxSteps->setGeometry(QRect(130, 240, 100, 22));
        spinBoxSpinboxSteps->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        spinBoxSpinboxSteps->setAccelerated(true);
        spinBoxSpinboxSteps->setMinimum(1);
        spinBoxSpinboxSteps->setMaximum(9999999);
        spinBoxSpinboxSteps->setSingleStep(10);
        spinBoxSpinboxSteps->setValue(10);
        label_31 = new QLabel(tabPiezo);
        label_31->setObjectName(QStringLiteral("label_31"));
        label_31->setGeometry(QRect(10, 270, 91, 16));
        spinBoxNumOfDecimalDigits = new QSpinBox(tabPiezo);
        spinBoxNumOfDecimalDigits->setObjectName(QStringLiteral("spinBoxNumOfDecimalDigits"));
        spinBoxNumOfDecimalDigits->setGeometry(QRect(130, 270, 100, 22));
        spinBoxNumOfDecimalDigits->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        spinBoxNumOfDecimalDigits->setAccelerated(true);
        spinBoxNumOfDecimalDigits->setMinimum(0);
        spinBoxNumOfDecimalDigits->setMaximum(5);
        spinBoxNumOfDecimalDigits->setSingleStep(1);
        spinBoxNumOfDecimalDigits->setValue(0);
        tabWidgetCfg->addTab(tabPiezo, QString());
        tabShutter = new QWidget();
        tabShutter->setObjectName(QStringLiteral("tabShutter"));
        label_11 = new QLabel(tabShutter);
        label_11->setObjectName(QStringLiteral("label_11"));
        label_11->setGeometry(QRect(60, 70, 131, 16));
        label_12 = new QLabel(tabShutter);
        label_12->setObjectName(QStringLiteral("label_12"));
        label_12->setGeometry(QRect(60, 170, 161, 16));
        tabWidgetCfg->addTab(tabShutter, QString());
        tabAI = new QWidget();
        tabAI->setObjectName(QStringLiteral("tabAI"));
        label_6 = new QLabel(tabAI);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(40, 40, 91, 16));
        label_7 = new QLabel(tabAI);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(40, 120, 81, 16));
        tabWidgetCfg->addTab(tabAI, QString());
        tabDisplay = new QWidget();
        tabDisplay->setObjectName(QStringLiteral("tabDisplay"));
        layoutWidget = new QWidget(tabDisplay);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 20, 323, 22));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_29 = new QLabel(layoutWidget);
        label_29->setObjectName(QStringLiteral("label_29"));

        horizontalLayout->addWidget(label_29);

        spinBoxDisplayAreaSize = new QSpinBox(layoutWidget);
        spinBoxDisplayAreaSize->setObjectName(QStringLiteral("spinBoxDisplayAreaSize"));
        spinBoxDisplayAreaSize->setMinimumSize(QSize(60, 0));
        spinBoxDisplayAreaSize->setMinimum(1);
        spinBoxDisplayAreaSize->setMaximum(800);
        spinBoxDisplayAreaSize->setValue(100);

        horizontalLayout->addWidget(spinBoxDisplayAreaSize);

        tabWidgetCfg->addTab(tabDisplay, QString());

        vboxLayout1->addWidget(tabWidgetCfg);

        btnApply = new QPushButton(dockWidgetContents2);
        btnApply->setObjectName(QStringLiteral("btnApply"));

        vboxLayout1->addWidget(btnApply);

        dwCfg->setWidget(dockWidgetContents2);
        labelImage = new ImageLabel(centralWidget);
        labelImage->setObjectName(QStringLiteral("labelImage"));
        labelImage->setGeometry(QRect(400, 300, 121, 101));
        dwHist = new QDockWidget(centralWidget);
        dwHist->setObjectName(QStringLiteral("dwHist"));
        dwHist->setGeometry(QRect(400, 240, 274, 51));
        dwHist->setMinimumSize(QSize(80, 40));
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName(QStringLiteral("dockWidgetContents_2"));
        vboxLayout5 = new QVBoxLayout(dockWidgetContents_2);
        vboxLayout5->setSpacing(6);
        vboxLayout5->setContentsMargins(11, 11, 11, 11);
        vboxLayout5->setObjectName(QStringLiteral("vboxLayout5"));
        dwHist->setWidget(dockWidgetContents_2);
        dwIntenCurve = new QDockWidget(centralWidget);
        dwIntenCurve->setObjectName(QStringLiteral("dwIntenCurve"));
        dwIntenCurve->setGeometry(QRect(450, 10, 241, 121));
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName(QStringLiteral("dockWidgetContents_3"));
        vboxLayout6 = new QVBoxLayout(dockWidgetContents_3);
        vboxLayout6->setSpacing(6);
        vboxLayout6->setContentsMargins(11, 11, 11, 11);
        vboxLayout6->setObjectName(QStringLiteral("vboxLayout6"));
        frameIntenCurve = new QFrame(dockWidgetContents_3);
        frameIntenCurve->setObjectName(QStringLiteral("frameIntenCurve"));
        frameIntenCurve->setFrameShape(QFrame::StyledPanel);
        frameIntenCurve->setFrameShadow(QFrame::Raised);

        vboxLayout6->addWidget(frameIntenCurve);

        dwIntenCurve->setWidget(dockWidgetContents_3);
        ImagineClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(ImagineClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 849, 21));
        menu_File = new QMenu(menuBar);
        menu_File->setObjectName(QStringLiteral("menu_File"));
        menuShutter = new QMenu(menuBar);
        menuShutter->setObjectName(QStringLiteral("menuShutter"));
        menu_Acquisition = new QMenu(menuBar);
        menu_Acquisition->setObjectName(QStringLiteral("menu_Acquisition"));
        menuView = new QMenu(menuBar);
        menuView->setObjectName(QStringLiteral("menuView"));
        ImagineClass->setMenuBar(menuBar);
        toolBar = new QToolBar(ImagineClass);
        toolBar->setObjectName(QStringLiteral("toolBar"));
        toolBar->setOrientation(Qt::Horizontal);
        ImagineClass->addToolBar(Qt::TopToolBarArea, toolBar);
        toolBar_2 = new QToolBar(ImagineClass);
        toolBar_2->setObjectName(QStringLiteral("toolBar_2"));
        toolBar_2->setOrientation(Qt::Horizontal);
        ImagineClass->addToolBar(Qt::TopToolBarArea, toolBar_2);
        toolBar_3 = new QToolBar(ImagineClass);
        toolBar_3->setObjectName(QStringLiteral("toolBar_3"));
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
        menuShutter->addAction(actionHeatsinkFan);
        menuShutter->addAction(actionTemperature);
        menu_Acquisition->addAction(actionStartAcqAndSave);
        menu_Acquisition->addAction(actionStartLive);
        menu_Acquisition->addAction(actionStop);
        menuView->addAction(actionDisplayFullImage);
        menuView->addSeparator();
        menuView->addAction(actionNoAutoScale);
        menuView->addAction(actionAutoScaleOnFirstFrame);
        menuView->addAction(actionAutoScaleOnAllFrames);
        menuView->addAction(actionManualContrast);
        menuView->addAction(actionContrastMin);
        menuView->addAction(actionContrastMax);
        menuView->addSeparator();
        menuView->addAction(actionConfig);
        menuView->addAction(actionHistogram);
        menuView->addAction(actionIntensity);
        menuView->addAction(actionLog);
        menuView->addAction(actionStimuli);
        menuView->addSeparator();
        menuView->addAction(actionColorizeSaturatedPixels);
        menuView->addSeparator();
        menuView->addAction(actionFlickerControl);
        toolBar->addAction(actionStartAcqAndSave);
        toolBar->addAction(actionStartLive);
        toolBar->addAction(actionStop);
        toolBar_2->addAction(actionOpenShutter);
        toolBar_2->addAction(actionCloseShutter);
        toolBar_3->addAction(actionDisplayFullImage);

        retranslateUi(ImagineClass);
        QObject::connect(dwStim, SIGNAL(visibilityChanged(bool)), actionStimuli, SLOT(setChecked(bool)));
        QObject::connect(dwCfg, SIGNAL(visibilityChanged(bool)), actionConfig, SLOT(setChecked(bool)));
        QObject::connect(dwLog, SIGNAL(visibilityChanged(bool)), actionLog, SLOT(setChecked(bool)));
        QObject::connect(dwHist, SIGNAL(visibilityChanged(bool)), actionHistogram, SLOT(setChecked(bool)));
        QObject::connect(dwIntenCurve, SIGNAL(visibilityChanged(bool)), actionIntensity, SLOT(setChecked(bool)));

        tabWidgetCfg->setCurrentIndex(2);
        comboBoxTriggerMode->setCurrentIndex(0);
        comboBoxTriggerMode_2->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ImagineClass);
    } // setupUi

    void retranslateUi(QMainWindow *ImagineClass)
    {
        ImagineClass->setWindowTitle(QApplication::translate("ImagineClass", "Imagine", 0));
        actionStartAcqAndSave->setText(QApplication::translate("ImagineClass", "&Start acq and save", 0));
        actionStop->setText(QApplication::translate("ImagineClass", "S&top", 0));
        actionOpenShutter->setText(QApplication::translate("ImagineClass", "Open Shutter", 0));
        actionCloseShutter->setText(QApplication::translate("ImagineClass", "Close Shutter", 0));
        action->setText(QApplication::translate("ImagineClass", "-", 0));
        actionTemperature->setText(QApplication::translate("ImagineClass", "Temperature", 0));
        actionStartLive->setText(QApplication::translate("ImagineClass", "Start live", 0));
        actionNoAutoScale->setText(QApplication::translate("ImagineClass", "No Auto Scale", 0));
        actionAutoScaleOnFirstFrame->setText(QApplication::translate("ImagineClass", "Auto Scale On First Frame", 0));
        actionAutoScaleOnAllFrames->setText(QApplication::translate("ImagineClass", "Auto Scale On All Frames", 0));
        actionStimuli->setText(QApplication::translate("ImagineClass", "Stimuli", 0));
        actionConfig->setText(QApplication::translate("ImagineClass", "Config", 0));
        actionLog->setText(QApplication::translate("ImagineClass", "Log", 0));
        actionColorizeSaturatedPixels->setText(QApplication::translate("ImagineClass", "Colorize Saturated Pixels", 0));
        actionDisplayFullImage->setText(QApplication::translate("ImagineClass", "Display Full Image", 0));
        actionExit->setText(QApplication::translate("ImagineClass", "Exit", 0));
        actionHeatsinkFan->setText(QApplication::translate("ImagineClass", "Heatsink Fan", 0));
        actionContrastMin->setText(QApplication::translate("ImagineClass", "                 min", 0));
        actionContrastMax->setText(QApplication::translate("ImagineClass", "                 max", 0));
        actionManualContrast->setText(QApplication::translate("ImagineClass", "Manual", 0));
        actionFlickerControl->setText(QApplication::translate("ImagineClass", "Flicker control", 0));
        actionHistogram->setText(QApplication::translate("ImagineClass", "Histogram", 0));
        actionIntensity->setText(QApplication::translate("ImagineClass", "Intensity", 0));
        dwStim->setWindowTitle(QApplication::translate("ImagineClass", "Stimuli", 0));
        dwLog->setWindowTitle(QApplication::translate("ImagineClass", "Log", 0));
        label_22->setText(QApplication::translate("ImagineClass", "Max number of lines:", 0));
        dwCfg->setWindowTitle(QApplication::translate("ImagineClass", "Config", 0));
        label->setText(QApplication::translate("ImagineClass", "File name:", 0));
        btnSelectFile->setText(QApplication::translate("ImagineClass", "...", 0));
        lineEditFilename->setText(QApplication::translate("ImagineClass", "d:/test/t.imagine", 0));
        label_2->setText(QApplication::translate("ImagineClass", "Comment:", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabGeneral), QApplication::translate("ImagineClass", "General", 0));
        groupBox->setTitle(QApplication::translate("ImagineClass", "time/duration", 0));
        label_32->setText(QApplication::translate("ImagineClass", "Angle from horizontal (deg):", 0));
        label_3->setText(QApplication::translate("ImagineClass", "#stacks:", 0));
#ifndef QT_NO_TOOLTIP
        label_5->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0));
#endif // QT_NO_TOOLTIP
        label_5->setText(QApplication::translate("ImagineClass", "exposure time(s):", 0));
        label_33->setText(QApplication::translate("ImagineClass", "um per pixle (xy plane):", 0));
#ifndef QT_NO_TOOLTIP
        label_5_2->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0));
#endif // QT_NO_TOOLTIP
        label_5_2->setText(QApplication::translate("ImagineClass", "idle time between stacks(s):", 0));
        label_4->setText(QApplication::translate("ImagineClass", "#frames/stack:", 0));
#ifndef QT_NO_TOOLTIP
        spinBoxAngle->setToolTip(QApplication::translate("ImagineClass", "-1 means n/a", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        doubleSpinBoxUmPerPxlXy->setToolTip(QApplication::translate("ImagineClass", "-1 means n/a", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_5_3->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0));
#endif // QT_NO_TOOLTIP
        label_5_3->setText(QApplication::translate("ImagineClass", "Acquisition trigger mode:", 0));
        comboBoxTriggerMode->clear();
        comboBoxTriggerMode->insertItems(0, QStringList()
         << QApplication::translate("ImagineClass", "External", 0)
         << QApplication::translate("ImagineClass", "Internal", 0)
        );
#ifndef QT_NO_TOOLTIP
        label_5_4->setToolTip(QApplication::translate("ImagineClass", "That is, msec/frame", 0));
#endif // QT_NO_TOOLTIP
        label_5_4->setText(QApplication::translate("ImagineClass", "Exposure trigger mode:", 0));
        comboBoxTriggerMode_2->clear();
        comboBoxTriggerMode_2->insertItems(0, QStringList()
         << QApplication::translate("ImagineClass", "Auto", 0)
         << QApplication::translate("ImagineClass", "External Start", 0)
         << QApplication::translate("ImagineClass", "External Control", 0)
        );
        groupBox_2->setTitle(QApplication::translate("ImagineClass", "horizontal pixel shift", 0));
        label_14->setText(QApplication::translate("ImagineClass", "Gain:", 0));
#ifndef QT_NO_TOOLTIP
        label_16->setToolTip(QApplication::translate("ImagineClass", "Horizontal shift speed", 0));
#endif // QT_NO_TOOLTIP
        label_16->setText(QApplication::translate("ImagineClass", "readout rate:", 0));
        label_15->setText(QApplication::translate("ImagineClass", "pre-amp gain", 0));
        groupBox_3->setTitle(QApplication::translate("ImagineClass", "vertical pixel shift", 0));
        label_18->setText(QApplication::translate("ImagineClass", "vertical clock voltage amplitude:", 0));
        label_17->setText(QApplication::translate("ImagineClass", "shift speed:", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabCamera), QApplication::translate("ImagineClass", "Camera", 0));
        label_26->setText(QApplication::translate("ImagineClass", "hor. start:", 0));
        label_21->setText(QApplication::translate("ImagineClass", "hor. end:", 0));
        label_25->setText(QApplication::translate("ImagineClass", "ver. end:", 0));
        label_24->setText(QApplication::translate("ImagineClass", "ver. start:", 0));
        btnUseZoomWindow->setText(QApplication::translate("ImagineClass", "use values from zoom window", 0));
        btnFullChipSize->setText(QApplication::translate("ImagineClass", "full chip size", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabBinning), QApplication::translate("ImagineClass", "Binning", 0));
        cbStim->setText(QApplication::translate("ImagineClass", "Apply stimuli", 0));
        label_13->setText(QApplication::translate("ImagineClass", "Stimulus file:", 0));
        btnOpenStimFile->setText(QApplication::translate("ImagineClass", "...", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabStim), QApplication::translate("ImagineClass", "Stimuli", 0));
        label_19->setText(QApplication::translate("ImagineClass", "max distance(um):", 0));
        label_8->setText(QApplication::translate("ImagineClass", "start position(um):", 0));
        label_9->setText(QApplication::translate("ImagineClass", "stop position(um):", 0));
#ifndef QT_NO_TOOLTIP
        btnSetCurPosAsStart->setToolTip(QApplication::translate("ImagineClass", "set as start position", 0));
#endif // QT_NO_TOOLTIP
        btnSetCurPosAsStart->setText(QApplication::translate("ImagineClass", "Set as start", 0));
#ifndef QT_NO_TOOLTIP
        btnSetCurPosAsStop->setToolTip(QApplication::translate("ImagineClass", "set as stop position", 0));
#endif // QT_NO_TOOLTIP
        btnSetCurPosAsStop->setText(QApplication::translate("ImagineClass", "Set as stop", 0));
        label_20->setText(QApplication::translate("ImagineClass", "move to position(um):", 0));
#ifndef QT_NO_TOOLTIP
        doubleSpinBoxCurPos->setToolTip(QApplication::translate("ImagineClass", "Use page up/dow key for fast change", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnMovePiezo->setToolTip(QApplication::translate("ImagineClass", "Move piezo according to user's input", 0));
#endif // QT_NO_TOOLTIP
        btnMovePiezo->setText(QApplication::translate("ImagineClass", "Move", 0));
        btnFastIncPosAndMove->setText(QApplication::translate("ImagineClass", "^^", 0));
        btnIncPosAndMove->setText(QApplication::translate("ImagineClass", "^", 0));
        btnDecPosAndMove->setText(QApplication::translate("ImagineClass", "v", 0));
        btnFastDecPosAndMove->setText(QApplication::translate("ImagineClass", "vv", 0));
        label_23->setText(QApplication::translate("ImagineClass", "min distance(um):", 0));
        label_10->setText(QApplication::translate("ImagineClass", "time for moving back(s):", 0));
        cbAutoSetPiezoTravelBackTime->setText(QApplication::translate("ImagineClass", "Auto", 0));
        label_27->setText(QApplication::translate("ImagineClass", "current position(um):", 0));
        labelCurPos->setText(QString());
#ifndef QT_NO_TOOLTIP
        btnRefreshPos->setToolTip(QApplication::translate("ImagineClass", "refresh current position", 0));
#endif // QT_NO_TOOLTIP
        btnRefreshPos->setText(QApplication::translate("ImagineClass", "Refresh", 0));
        label_28->setText(QApplication::translate("ImagineClass", "Axis:", 0));
        comboBoxAxis->clear();
        comboBoxAxis->insertItems(0, QStringList()
         << QApplication::translate("ImagineClass", "X", 0)
         << QApplication::translate("ImagineClass", "Y", 0)
        );
#ifndef QT_NO_TOOLTIP
        btnMoveToStartPos->setToolTip(QApplication::translate("ImagineClass", "Move to the start position", 0));
#endif // QT_NO_TOOLTIP
        btnMoveToStartPos->setText(QApplication::translate("ImagineClass", "Move", 0));
#ifndef QT_NO_TOOLTIP
        btnMoveToStopPos->setToolTip(QApplication::translate("ImagineClass", "Move to the stop position", 0));
#endif // QT_NO_TOOLTIP
        btnMoveToStopPos->setText(QApplication::translate("ImagineClass", "Move", 0));
        cbBidirectionalImaging->setText(QApplication::translate("ImagineClass", "Bi-directional Imaging", 0));
        label_30->setText(QApplication::translate("ImagineClass", "spin box step:", 0));
#ifndef QT_NO_TOOLTIP
        spinBoxSpinboxSteps->setToolTip(QApplication::translate("ImagineClass", "This specifies how much to increase/decrease when click up/down arrows", 0));
#endif // QT_NO_TOOLTIP
        label_31->setText(QApplication::translate("ImagineClass", "#decimals:", 0));
#ifndef QT_NO_TOOLTIP
        spinBoxNumOfDecimalDigits->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabPiezo), QApplication::translate("ImagineClass", "Positioner", 0));
        label_11->setText(QApplication::translate("ImagineClass", "open time(offset 2):", 0));
        label_12->setText(QApplication::translate("ImagineClass", "close time(offset 4):", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabShutter), QApplication::translate("ImagineClass", "Shutter", 0));
        label_6->setText(QApplication::translate("ImagineClass", "input rate:", 0));
        label_7->setText(QApplication::translate("ImagineClass", "input range:", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabAI), QApplication::translate("ImagineClass", "AI", 0));
#ifndef QT_NO_TOOLTIP
        label_29->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        label_29->setText(QApplication::translate("ImagineClass", "Display area size (% of acquired image size):", 0));
        tabWidgetCfg->setTabText(tabWidgetCfg->indexOf(tabDisplay), QApplication::translate("ImagineClass", "Display", 0));
        btnApply->setText(QApplication::translate("ImagineClass", "Apply all", 0));
        labelImage->setText(QApplication::translate("ImagineClass", "labelImage", 0));
        dwHist->setWindowTitle(QApplication::translate("ImagineClass", "Histogram", 0));
        dwIntenCurve->setWindowTitle(QApplication::translate("ImagineClass", "Intensity", 0));
        menu_File->setTitle(QApplication::translate("ImagineClass", "&File", 0));
        menuShutter->setTitle(QApplication::translate("ImagineClass", "Hardware", 0));
        menu_Acquisition->setTitle(QApplication::translate("ImagineClass", "&Acquisition", 0));
        menuView->setTitle(QApplication::translate("ImagineClass", "View", 0));
    } // retranslateUi

};

namespace Ui {
    class ImagineClass: public Ui_ImagineClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGINE_H

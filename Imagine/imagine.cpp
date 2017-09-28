/*-------------------------------------------------------------------------
** Copyright (C) 2005-2012 Timothy E. Holy and Zhongsheng Guo
**    All rights reserved.
** Author: All code authored by Zhongsheng Guo.
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/

#include "imagine.h"

#include <QScrollArea>
#include <QStatusBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QPainter>
#include <QVBoxLayout>
#include <QDate>
#include <QScriptEngine>
#include <QScriptProgram>
#include <QApplication>
#include <QInputDialog>
#include <QColorDialog>
#include <QTextBrowser>

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_histogram.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>
#include <QDesktopWidget>

//#include "histogram_item.h"
#include "curvedata.h"

#include <vector>
#include <utility>
#include <string>
#include <iostream>

using namespace std;

#include "ni_daq_g.hpp"
#include "positioner.hpp"

#include "spoolthread.h"
#include <bitset>
#include "misc.hpp"
using std::bitset;

ImagineStatus curStatus;
ImagineAction curAction;

extern QScriptEngine* se;
extern DigitalControls* digOut;

// misc vars
bool zoom_isMouseDown = false;
QPoint zoom_downPos, zoom_curPos; //in the unit of displayed image
int nUpdateImage;
ControlWaveform *conWaveData = NULL;
ControlWaveform *conWaveDataUser = NULL;
ControlWaveform *conWaveDataParam = NULL;

#pragma region LIFECYCLE

Imagine::Imagine(QString rig, Camera *cam, Positioner *pos, Laser *laser,
    Imagine *mImagine, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    this->rig = rig;
    m_OpenDialogLastDirectory = QDir::homePath();
    m_OpenConWaveDialogLastDirectory = QDir::homePath();
    masterImagine = mImagine;
    dataAcqThread = new DataAcqThread(QThread::NormalPriority, cam, pos, this, parent);

    qDebug() << QString("created data acq thread");

    // set up the pixmap making thread (see dtor for cleanup)
    pixmapper = new Pixmapper();
    pixmapper->moveToThread(&pixmapperThread);
    connect(&pixmapperThread, &QThread::finished, pixmapper, &QObject::deleteLater);
    connect(this, &Imagine::makePixmap, pixmapper, &Pixmapper::handleImg);
    connect(this, &Imagine::makeColorPixmap, pixmapper, &Pixmapper::handleColorImg);
    connect(pixmapper, &Pixmapper::pixmapReady, this, &Imagine::handlePixmap);
    connect(pixmapper, &Pixmapper::pixmapColorReady, this, &Imagine::handleColorPixmap);
    pixmapperThread.start(QThread::IdlePriority);
    imagePlay = new ImagePlay();
    imagePlay->moveToThread(&imagePlayThread);
    connect(&imagePlayThread, &QThread::finished, imagePlay, &QObject::deleteLater);
    imagePlayThread.start(QThread::IdlePriority);
    connect(this, &Imagine::startIndexRunning, imagePlay, &ImagePlay::indexRun);
    connect(imagePlay, &ImagePlay::nextIndexIsReady, this, &Imagine::readNextCamImages);

    minPixelValueByUser = 0;
    maxPixelValueByUser = 1 << 16;
    minPixelValueByUser2 = 0;
    maxPixelValueByUser2 = 1 << 16;

    ui.setupUi(this);
    //load user's preference/preset from js
    loadPreset();
    pixmapper->param.isAuto = false;
    pixmapper->param.isOk = false;
    pixmapper->param.theta = ui.dsbRotationAngle->value();
    pixmapper->param.tx = ui.sbXTranslation->value();
    pixmapper->param.ty = ui.sbYTranslation->value();

    //to overcome qt designer's incapability
    addDockWidget(Qt::TopDockWidgetArea, ui.dwStim);
    addDockWidget(Qt::LeftDockWidgetArea, ui.dwCfg);
    addDockWidget(Qt::LeftDockWidgetArea, ui.dwLog);
    addDockWidget(Qt::LeftDockWidgetArea, ui.dwHist);
    addDockWidget(Qt::LeftDockWidgetArea, ui.dwIntenCurve);
    ui.dwCfg->setWindowTitle("Config and control");
    tabifyDockWidget(ui.dwCfg, ui.dwHist);
    tabifyDockWidget(ui.dwHist, ui.dwIntenCurve);
    setDockOptions(dockOptions() | QMainWindow::VerticalTabs);

    // monitoring input (Ai, Di) and control waveform (Ao, Do)
    // checkBox list and comboBox list
    QCheckBox* cb1[] = { ui.cbAO0Wav, ui.cbAO1Wav, ui.cbDO0Wav,
        ui.cbDO1Wav, ui.cbDO2Wav, ui.cbDO3Wav, ui.cbDO4Wav, ui.cbDO5Wav };
    QComboBox* combo1[] = { ui.comboBoxAO0, ui.comboBoxAO1, ui.comboBoxDO0,
        ui.comboBoxDO1, ui.comboBoxDO2, ui.comboBoxDO3, ui.comboBoxDO4, ui.comboBoxDO5 };
    for (int i = 0; i < sizeof(cb1) / sizeof(QCheckBox*); i++) {
        cbAoDo.push_back(cb1[i]);
        comboBoxAoDo.push_back(combo1[i]);
    }
    ui.lableConWavRigname->setText(rig);
    QCheckBox* cb2[] = { ui.cbAI0Wav, ui.cbAI1Wav, ui.cbDI0Wav, ui.cbDI1Wav,
        ui.cbDI2Wav, ui.cbDI3Wav, ui.cbDI4Wav, ui.cbDI5Wav, ui.cbDI6Wav };
    QComboBox* combo2[] = { ui.comboBoxAI0, ui.comboBoxAI1, ui.comboBoxDI0, ui.comboBoxDI1,
        ui.comboBoxDI2, ui.comboBoxDI3, ui.comboBoxDI4, ui.comboBoxDI5, ui.comboBoxDI6 };
    for (int i = 0; i < sizeof(cb2) / sizeof(QCheckBox*); i++) {
        cbAiDi.push_back(cb2[i]);
        comboBoxAiDi.push_back(combo2[i]);
    }

    //ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabAI));
    //remove positioner tab from slave window
    //TODO: Give stimulus tab the same treatment (It's in position #3)
    if (masterImagine != NULL) { // Imagine(2)
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabPiezo));
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabWaveform));
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabAI));
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabStim));
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabScript));
        ui.gbDisplaySourceSelect->hide();
        ui.groupBoxPlayCamImage->hide();
        ui.groupBoxBlendingOption->hide();
        ui.cbCam1Enable->setChecked(false);
        ui.cbCam2Enable->setChecked(true);
        ui.rbImgCameraEnable->setChecked(true);
        ui.lineEditFilename->setText("d:/test/t2.imagine");
        ui.actionContrastMin2->setVisible(false);
        ui.actionContrastMax2->setVisible(false);
    }
    else {  // Imagine(1)
        conWaveDataParam = new ControlWaveform(rig);
        conWaveDataUser = new ControlWaveform(rig);
        conWaveData = conWaveDataParam;
        ui.cbWaveformEnable->setEnabled(false);
        ui.cbCam1Enable->setChecked(true);
        ui.cbCam2Enable->setChecked(false);
        ui.rbImgCameraEnable->setChecked(true);
        ui.lineEditFilename->setText("f:/test/t1.imagine");
    }
    ui.doubleSpinBoxExpTimeWav1->setEnabled(false);
    ui.doubleSpinBoxExpTimeWav2->setEnabled(false);

    //adjust size
    QRect tRect = geometry();
    tRect.setHeight(tRect.height() + 150);
    tRect.setWidth(tRect.width() + 125);
    this->setGeometry(tRect);

    //group menuitems on View:
    ui.actionNoAutoScale->setCheckable(true);  //note: MUST
    ui.actionAutoScaleOnFirstFrame->setCheckable(true);
    ui.actionAutoScaleOnAllFrames->setCheckable(true);
    QActionGroup* autoScaleActionGroup = new QActionGroup(this);
    autoScaleActionGroup->addAction(ui.actionNoAutoScale);  //OR: ui.actionNoAutoScale->setActionGroup(autoScaleActionGroup);
    autoScaleActionGroup->addAction(ui.actionAutoScaleOnFirstFrame);
    autoScaleActionGroup->addAction(ui.actionAutoScaleOnAllFrames);
    autoScaleActionGroup->addAction(ui.actionManualContrast);
    ui.actionAutoScaleOnFirstFrame->setChecked(true);

    ui.actionStimuli->setCheckable(true);
    ui.actionStimuli->setChecked(true);
    ui.actionConfig->setCheckable(true);
    ui.actionConfig->setChecked(true);
    ui.actionLog->setCheckable(true);
    ui.actionLog->setChecked(true);

    ui.actionColorizeSaturatedPixels->setCheckable(true);
    ui.actionColorizeSaturatedPixels->setChecked(true);

    preparePlots();

    //see: QT demo -> widgets -> ImageViewer Example
    ui.labelImage->setBackgroundRole(QPalette::Base);
    ui.labelImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui.labelImage->setScaledContents(true);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(ui.labelImage);
    setCentralWidget(scrollArea);

    //
    //addDockWidget(Qt::LeftDockWidgetArea, ui.dwCfg);
    ui.dwCfg->show();
    ui.dwCfg->raise();

    //show logo
    QImage tImage(":/images/Resources/logo.jpg"); //todo: put logo here
    if (!tImage.isNull()) {
        ui.labelImage->setPixmap(QPixmap::fromImage(tImage));
        ui.labelImage->adjustSize();
    }

    //trigger mode combo box
//    if (pos != NULL && pos->posType == ActuatorPositioner) {
//        ui.comboBoxAcqTriggerMode->setEnabled(false);
//        ui.comboBoxAcqTriggerMode->setCurrentIndex(1);
//    }

    Camera& camera = *dataAcqThread->pCamera;

    ui.comboBoxHorReadoutRate->setEnabled(false);
    ui.comboBoxPreAmpGains->setEnabled(false);
    ui.comboBoxVertShiftSpeed->setEnabled(false);
    ui.comboBoxVertClockVolAmp->setEnabled(false);

    maxROIHSize = camera.getChipWidth();
    maxROIVSize = camera.getChipHeight();
 //   roiStepsHor = camera.getROIStepsHor(); // OCPI-2 return 20
    roiStepsHor = 160;

    if (maxROIHSize == 0) {// this is for GUI test at dummy HW
        maxROIHSize = 2048;
        maxROIVSize = 2048;
        roiStepsHor = 160;
    }
    setROIMinMaxSize();
    ui.spinBoxHend->setValue(maxROIHSize);
    ui.spinBoxVend->setValue(maxROIVSize);

    updateStatus(eIdle, eNoAction);

    //apply the camera setting:
    on_btnApply_clicked();

    ///piezo stuff
    if (pos != NULL) {
        bool isActPos = pos->posType == ActuatorPositioner;
        ui.doubleSpinBoxMinDistance->setValue(pos->minPos());
        ui.doubleSpinBoxMaxDistance->setValue(pos->maxPos());
        ui.comboBoxAxis->setEnabled(isActPos);
        if (isActPos){
            pos->setDim(1);
            ui.comboBoxAxis->setCurrentIndex(1);
        }
        //move piezo position to preset position
        on_btnMovePiezo_clicked();
        piezoUiParams.resize(3);//3 axes at most. TODO: support querying #dims
    }
    QString buildDateStr = __DATE__;
    QDate date = QDate::fromString(buildDateStr, "MMM d yyyy");
    if (!date.isValid()) {
        date = QDate::fromString(buildDateStr, "MMM  d yyyy");
        assert(date.isValid());
    }
    QString ver = QString("%1%2%3")
        .arg(date.year() - 2000, 1, 16)
        .arg(date.month(), 1, 16)
        .arg(date.day(), 2, 10, QChar('0'));
    QString copyright = "(c) 2005-2013";
    this->statusBar()->showMessage(
        QString("Ready. Imagine Version 2.0 (Build %1). %2").arg(ver)
        .arg(copyright));

    //to enable pass QImage in signal/slot
    qRegisterMetaType<QImage>("QImage");

    connect(dataAcqThread, SIGNAL(newStatusMsgReady(const QString &)),
        this, SLOT(updateStatus(const QString &)));
    connect(dataAcqThread, SIGNAL(newLogMsgReady(const QString &)),
        this, SLOT(appendLog(const QString &)));
    connect(dataAcqThread, SIGNAL(imageDataReady(const QByteArray &, long, int, int)),
        this, SLOT(updateLiveImagePlay(const QByteArray &, long, int, int)));
        // this, SLOT(updateDisplay(const QByteArray &, long, int, int)));
    connect(dataAcqThread, SIGNAL(resetActuatorPosReady()),
        this, SLOT(on_btnMoveToStartPos_clicked()));

    //mouse events on image
    connect(ui.labelImage, SIGNAL(mousePressed(QMouseEvent*)),
        this, SLOT(zoom_onMousePressed(QMouseEvent*)));

    connect(ui.labelImage, SIGNAL(mouseMoved(QMouseEvent*)),
        this, SLOT(zoom_onMouseMoved(QMouseEvent*)));

    connect(ui.labelImage, SIGNAL(mouseReleased(QMouseEvent*)),
        this, SLOT(zoom_onMouseReleased(QMouseEvent*)));

    //for detect param changes
    auto lineedits = ui.tabWidgetCfg->findChildren<QLineEdit*>();
    for (int i = 0; i < lineedits.size(); ++i){
        if (lineedits[i]->accessibleDescription() != "no apply")
            connect(lineedits[i], SIGNAL(textChanged(const QString&)),
                this, SLOT(onModified()));
    }

    auto spinboxes = ui.tabWidgetCfg->findChildren<QSpinBox*>();
    for (int i = 0; i < spinboxes.size(); ++i){
        if (spinboxes[i]->accessibleDescription() != "no apply")
            connect(spinboxes[i], SIGNAL(valueChanged(const QString&)),
                this, SLOT(onModified()));
//        else
//            disconnect(((QAbstractSpinBox*)(spinboxes[i]))->lineEdit(), SIGNAL(textChanged(const QString&)),
//                this, SLOT(onModified()));
    }

    auto doublespinboxes = ui.tabWidgetCfg->findChildren<QDoubleSpinBox*>();
    for (int i = 0; i < doublespinboxes.size(); ++i){
        if (doublespinboxes[i]->accessibleDescription() != "no apply")
            connect(doublespinboxes[i], SIGNAL(valueChanged(const QString&)),
                this, SLOT(onModified()));
    }

    auto comboboxes = ui.tabWidgetCfg->findChildren<QComboBox*>();
    for (int i = 0; i < comboboxes.size(); ++i){
        if (comboboxes[i]->accessibleDescription() != "no apply")
            connect(comboboxes[i], SIGNAL(currentIndexChanged(const QString&)),
                this, SLOT(onModified()));
    }

    auto checkboxes = ui.tabWidgetCfg->findChildren<QCheckBox*>();
    for (auto cb : checkboxes){
        if (cb->accessibleDescription() != "no apply")
            connect(cb, SIGNAL(stateChanged(int)),
                this, SLOT(onModified()));
    }

    on_spinBoxSpinboxSteps_valueChanged(ui.spinBoxSpinboxSteps->value());

    /* for laser control from this line */
    // pass laser object to a new thread
    if ((masterImagine == NULL)&&(laser->getDeviceName() != "nidaq")) {
        QString portName;
        maxLaserFreq = laser->getMaxFreq();
        if (laser->getDeviceName() == "COM")
            portName = QString("COM%1").arg(ui.spinBoxPortNum->value());
        else
            portName = "DummyPort";
        laserCtrlSerial = new LaserCtrlSerial(portName);
        laserCtrlSerial->moveToThread(&laserCtrlThread);
        // connect signals
        connect(&laserCtrlThread, SIGNAL(finished()), laserCtrlSerial, SLOT(deleteLater()));
        connect(this, SIGNAL(openLaserSerialPort(QString)), laserCtrlSerial, SLOT(openSerialPort(QString)));
        connect(this, SIGNAL(closeLaserSerialPort(void)), laserCtrlSerial, SLOT(closeSerialPort(void)));
        connect(this, SIGNAL(getLaserShutterStatus(void)), laserCtrlSerial, SLOT(getShutterStatus(void)));
        connect(this, SIGNAL(setLaserShutters(int)), laserCtrlSerial, SLOT(setShutters(int)));
        connect(this, SIGNAL(getLaserTransStatus(bool, int)), laserCtrlSerial, SLOT(getTransStatus(bool, int)));
        connect(this, SIGNAL(setLaserTrans(bool, int, int)), laserCtrlSerial, SLOT(setTrans(bool, int, int)));
        connect(this, SIGNAL(getLaserLineSetupStatus(void)), laserCtrlSerial, SLOT(getLaserLineSetup(void)));
        //        connect(laserCtrlSerial, SIGNAL(getShutterStatusReady(int)), this, SLOT(displayShutterStatus(int)));
        //        connect(laserCtrlSerial, SIGNAL(getTransStatusReady(bool, int, int)), this, SLOT(displayTransStatus(bool, int, int)));
        connect(laserCtrlSerial, SIGNAL(getLaserLineSetupReady(int, int *, int *)), this, SLOT(displayLaserGUI(int, int *, int *)));
        // run event handler
        laserCtrlThread.start(QThread::IdlePriority);
        if (laserCtrlSerial) {
            emit openLaserSerialPort(portName);
            emit getLaserLineSetupStatus();
            emit setLaserShutters(0);
        }
        ui.labelAnalogTTL->setVisible(false);
        ui.cbLine1->setVisible(false);
        ui.cbLine2->setVisible(false);
        ui.cbLine3->setVisible(false);
        ui.cbLine4->setVisible(false);
        ui.cbLine5->setVisible(false);
        ui.cbLineTTL6->setVisible(false);
        ui.cbLineTTL7->setVisible(false);
        ui.cbLineTTL8->setVisible(false);
        ui.cbDualOutSW1->setVisible(false);
        ui.cbDualOutSW2->setVisible(false);
    }
    else {
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabLaser));
        ui.actionOpenShutter->setEnabled(true);
        ui.actionCloseShutter->setEnabled(false);
    }
    /* for laser control until this line */
    /* for piezo setup from this line */
    // pass piezo setup object to a new thread
    if ((masterImagine == NULL) && (pos->getCtrlrSetupType() != "none")) {
        QString portName;
        if (pos->getCtrlrSetupType().left(3) == "COM")
            portName = pos->getCtrlrSetupType();
        else
            portName = "DummyPort";
        piezoCtrlSerial = new PiezoCtrlSerial(portName);
        piezoCtrlSerial->moveToThread(&piezoCtrlThread);
        // connect signals
        connect(&piezoCtrlThread, SIGNAL(finished()), piezoCtrlSerial, SLOT(deleteLater()));
        connect(this, SIGNAL(openPiezoCtrlSerialPort(QString)), piezoCtrlSerial, SLOT(openSerialPort(QString)));
        connect(this, SIGNAL(closePiezoCtrlSerialPort(void)), piezoCtrlSerial, SLOT(closeSerialPort(void)));
        connect(this, SIGNAL(sendPiezoCtrlCmd(QString)), piezoCtrlSerial, SLOT(sendCommand(QString)));
        connect(piezoCtrlSerial, SIGNAL(sendStatusToGUI(QByteArray)), this, SLOT(displayPiezoCtrlStatus(QByteArray)));
        connect(piezoCtrlSerial, SIGNAL(sendValueToGUI(QByteArray)), this, SLOT(displayPiezoCtrlValue(QByteArray)));
        // run event handler
        piezoCtrlThread.start();
        if (piezoCtrlSerial) {
            emit openPiezoCtrlSerialPort(portName);
            emit sendPiezoCtrlCmd("cl,1"); // closed loop on
            emit sendPiezoCtrlCmd("stat"); // read status
            ui.btnPzOpenPort->setEnabled(false);
            ui.btnPzClosePort->setEnabled(true);
        }
    }
    else {
        ui.btnPzOpenPort->setVisible(false);
        ui.btnPzClosePort->setVisible(false);
        ui.labelSerialCommand->setVisible(false);
        ui.lineEditSerialCmd->setVisible(false);
        ui.btnSendSerialCmd->setVisible(false);
    }
    /* for piezo setup until this line */

    //center window:
    QRect rect = QApplication::desktop()->screenGeometry();
    int x = (rect.width() - this->width()) / 2;
    int y = (rect.height() - this->height()) / 2;
    this->move(x, y);

    ui.groupBoxPlayCamImage->setEnabled(false);
    ui.groupBoxBlendingOption->setEnabled(false);

    img1Color = QColor(255, 0, 0);
    img2Color = QColor(0, 255, 0);
    applyImgColor(ui.widgetImg1Color, img1Color);
    applyImgColor(ui.widgetImg2Color, img2Color);
    alpha = 50;
    ui.hsBlending->setValue(alpha);

    ui.gbDontShowMe1->setVisible(false);
    ui.gbDontShowMe2->setVisible(false);
    ui.gbDontShowMe3->setVisible(false);
    ui.gbHorizontalShift->setVisible(false);
    ui.gbVerticalShift->setVisible(false);
    //    ui.cbEnableMismatch->setVisible(false);
    ui.pbTestButton->setVisible(false);
}

Imagine::~Imagine()
{
    if (conWaveDataParam) {
        delete conWaveDataParam;
        conWaveDataParam = NULL;
    }
    if (conWaveDataUser) {
        delete conWaveDataUser;
        conWaveDataUser = NULL;
    }
    if (aiWaveData) {
        delete aiWaveData;
        aiWaveData = NULL;
    }
    if (diWaveData) {
        delete diWaveData;
        diWaveData = NULL;
    }
    if (imagineScript) {
        delete imagineScript;
        imagineScript = NULL;
    }
    // clean up the pixmapper nonsense
    pixmapperThread.quit();
    pixmapperThread.wait();
    //delete(pixmapper);

    // for laser control from this line
    laserCtrlThread.quit();
    laserCtrlThread.wait();
    // for laser control until this line

    // for piezo control from this line
    piezoCtrlThread.quit();
    piezoCtrlThread.wait();
    // for piezo control until this line

    stopDisplayCamFile();
}

void Imagine::setSlaveWindow(Imagine *sImagine)
{
    slaveImagine = sImagine;
    if ((masterImagine == NULL) && (slaveImagine == NULL)) { // single Imagine window system
        ui.cbBothCamera->setChecked(false);
        ui.cbBothCamera->setVisible(false);
        if (slaveImagine == NULL) {
            ui.cbCam2Enable->setChecked(false);
            ui.cbCam2Enable->setVisible(false);
        }
    }
}

void Imagine::setROIMinMaxSize()
{
    if (isUsingSoftROI) {
        ui.spinBoxHstart->setMaximum(maxROIHSize);
        ui.spinBoxVstart->setMaximum(maxROIVSize);
        ui.spinBoxHend->setMaximum(maxROIHSize);
        ui.spinBoxVend->setMinimum(1);
        ui.spinBoxVend->setMaximum(maxROIVSize);
        ui.spinBoxHend->setSingleStep(1);
        ui.spinBoxHstart->setSingleStep(1);
    }
    else {
        ui.spinBoxHstart->setMaximum(maxROIHSize);
        ui.spinBoxVstart->setMaximum(maxROIVSize / 2);
        ui.spinBoxHend->setMaximum(maxROIHSize);
        ui.spinBoxVend->setMinimum(maxROIVSize / 2 + 1);
        ui.spinBoxVend->setMaximum(maxROIVSize);
        ui.spinBoxHend->setSingleStep(roiStepsHor);
        ui.spinBoxHstart->setSingleStep(roiStepsHor);
    }
}
#pragma endregion


#pragma region DRAWING

void Imagine::updateImage(bool isColor, bool isForce)
{
    if (isColor) {
        if (lastRawImg.isNull()|| lastRawImg2.isNull()) return;
    }
    else {
        if (lastRawImg.isNull()) return;
    }
    if (isPixmapping && !isForce) return;
    isPixmapping = true;

    // making the pixmap is expensive - push it to another thread
    // see handlePixmap() for continuation
    int xd = 0, xc = 0, yd = 0, yc = 0;
    if (zoom_isMouseDown) {
        xd = zoom_downPos.x();
        xc = zoom_curPos.x();
        yd = zoom_downPos.y();
        yc = zoom_curPos.y();
    }
    bool colSat = ui.actionColorizeSaturatedPixels->isChecked();
    double displayAreaSize = ui.spinBoxDisplayAreaSize->value() / 100.0;
    if (L == -1) {
        L = T = 0; H = lastImgH; W = lastImgW;
    }

    if(isColor)
        emit makeColorPixmap(lastRawImg, lastRawImg2, img1Color, img2Color, alpha
            , lastImgW, lastImgH, factor, factor2, displayAreaSize,
            L, T, W, H, xd, xc, yd, yc, minPixelValue, maxPixelValue,
            minPixelValue2, maxPixelValue2, colSat);
    else
        emit makePixmap(lastRawImg, lastImgW, lastImgH, factor, displayAreaSize,
            L, T, W, H, xd, xc, yd, yc, minPixelValue, maxPixelValue, colSat);
}

void Imagine::handlePixmap(const QPixmap &pxmp, const QImage &img) {
    // pop the new pixmap into the label, and hey-presto
    pixmap = pxmp;
    image = img;
    ui.labelImage->setPixmap(pxmp);
    ui.labelImage->adjustSize();
    isPixmapping = false;
}

void Imagine::handleColorPixmap(const QPixmap &pxmp, const QImage &img) {
    pixmap = pxmp;
    image = img;
    ui.labelImage->setPixmap(pxmp);
    ui.labelImage->adjustSize();
    isPixmapping = false;
}

int counts[1 << 16];
void Imagine::updateHist(QwtPlotHistogram *histogram, const Camera::PixelValue * frame,
    const int imageW, const int imageH, double &maxcount, unsigned int &maxintensity)
{
    //initialize the counts to all 0's
    int ncounts = sizeof(counts) / sizeof(counts[0]);
    for (unsigned int i = 0; i < ncounts; ++i) counts[i] = 0;

    for (unsigned int i = 0; i < imageW*imageH; ++i){
        counts[*frame++]++;
    }

    // Autoscale
    for (maxintensity = ncounts - 1; maxintensity >= 0; maxintensity--)
        if (counts[maxintensity])
            break;

    //now binning
	int nBins = histPlot->width() / 3; //3 display pixels per bin?
    int totalWidth = maxintensity; //1<<14;
    double binWidth = double(totalWidth) / nBins;

    QVector<QwtIntervalSample> intervals(nBins);

    for (unsigned int i = 0; i < nBins; i++) {
        double start = i*binWidth;                            //inclusive
        double end = (i == nBins - 1) ? totalWidth : start + binWidth;

        int count = 1;  // since log scale, plot count+1
        int width = 0;
        int istart = ceil(start);
        for (; istart < end;){
            count += counts[istart++];
            width++;
        }
        double value = (width > 0) ? (double(count) / width)*binWidth : 1;
        if (value > maxcount)
            maxcount = value;
        intervals[i] = QwtIntervalSample(value, start, end);
    }//for, each inten
    //cout << "Last interval: (" << intervals[nBins-1].minValue() << "," << intervals[nBins-1].maxValue() << ")";
    //cout << "; Last value: " << values[nBins-1] << endl;

    histogram->setSamples(intervals);
    histogram->attach(histPlot);
}//updateHist(),

void Imagine::updateIntenCurve(const Camera::PixelValue * frame,
    const int imageW, const int imageH, const int frameIdx)
{
    //TODO: if not visible, just return
    double sum=0;
    int nPixels=imageW*imageH;
    for(unsigned int i=0; i<nPixels; ++i){
    sum+=*frame++;
    }

    double value=sum/nPixels;

    intenCurveData->append(frameIdx, value);
    intenCurve->setData(intenCurveData);
    intenPlot->setAxisScale(QwtPlot::xBottom, intenCurveData->left(), intenCurveData->right());
    intenPlot->replot();
}//updateIntenCurve(),

void prepareCurve(QwtPlotCurve *curve, QwtPlot *plot,
    QwtText &xTitle, QwtText &yTitle, QColor color)
{
    xTitle.setFont(QFont("Helvetica", 10));
    yTitle.setFont(QFont("Helvetica", 10));
    plot->setAxisTitle(QwtPlot::xBottom, xTitle); //TODO: stack number too
    plot->setAxisTitle(QwtPlot::yLeft, yTitle);
    plot->setAxisScale(QwtPlot::xBottom, 0, 400, 100);
    plot->setAxisScale(QwtPlot::yLeft, 0, 300, 100);
    plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());
    plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
    plot->setMinimumWidth(200);
    plot->setBaseSize(200,100);

    QwtSymbol *sym = new QwtSymbol;
    sym->setStyle(QwtSymbol::NoSymbol);
    sym->setPen((Qt::red));
    sym->setSize(5);
    curve->setSymbol(sym);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setPen(QPen(color));
    curve->attach(plot);
}

void Imagine::preparePlots()
{
    //the histgram
    histPlot = new QwtPlot();
    histPlot->setCanvasBackground(QColor(Qt::white));
//    ui.dwHist->setWidget(histPlot); //setWidget() causes the widget using all space
    ui.histogramLayout->addWidget(histPlot); //setWidget() causes the widget using all space
    histPlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine(10));
    histPlot->setMinimumWidth(200);
    //histPlot->setBaseSize(200, 100);

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(QPen(Qt::black, 0, Qt::DotLine));
    grid->setMinorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(histPlot);

    histogram = new QwtPlotHistogram(); // HistogramItem();
    histogram->setTitle("camera 1 or image 1");
    histogram->setPen(Qt::darkCyan);
    histogram->setBrush(QBrush(Qt::darkCyan));
    histogram->setBaseline(1.0); //baseline will be subtracted from pixel intensities
    histogram->setItemAttribute(QwtPlotItem::AutoScale, true);
    histogram->setItemAttribute(QwtPlotItem::Legend, true);
    histogram->setZ(20.0);
    histogram->setStyle(QwtPlotHistogram::Columns);
//    histogram->attach(histPlot);

    histogram2 = new QwtPlotHistogram(); // HistogramItem();
    histogram2->setTitle("camera 2 or image 2");
    histogram2->setPen(Qt::darkYellow);
    histogram2->setBrush(QBrush(Qt::darkYellow));
    histogram2->setBaseline(1.0); //baseline will be subtracted from pixel intensities
    histogram2->setItemAttribute(QwtPlotItem::AutoScale, true);
    histogram2->setItemAttribute(QwtPlotItem::Legend, true);
    histogram2->setZ(20.0);
    histogram2->setStyle(QwtPlotHistogram::Columns);
//    histogram2->attach(histPlot);

    ///todo: make it more robust by query Camera class
    if (dataAcqThread->pCamera->vendor == "andor"){
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 14);
    }
    else if (dataAcqThread->pCamera->vendor == "cooke"){
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 5000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 16);
    }
    else {
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 14);
    }

    //the intensity plot
    int curveWidth = 500;
    intenPlot = new QwtPlot();
//    ui.dwIntenCurve->setWidget(intenPlot);
    ui.intensityLayout->addWidget(intenPlot);
    intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number"); //TODO: stack number too
    intenPlot->setAxisTitle(QwtPlot::yLeft, "avg intensity");
    intenCurve = new QwtPlotCurve("avg intensity");
    intenPlot->setMinimumWidth(200);
    //intenPlot->setBaseSize(200, 100);

    QwtSymbol *sym = new QwtSymbol;
    sym->setStyle(QwtSymbol::Cross);
    sym->setPen(QColor(Qt::black));
    sym->setSize(5);
    intenCurve->setSymbol(sym);

    intenCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    intenCurve->setPen(QPen(Qt::red));
    intenCurve->attach(intenPlot);

    intenCurveData = new CurveData(curveWidth);
    intenCurve->setData(intenCurveData);

    // control waveform and ai and di waveform
    QColor curveColor[9] = { Qt::red,  Qt::green,  Qt::blue,  Qt::black,
                Qt::darkGreen,  Qt::magenta,  Qt::darkYellow, Qt::darkMagenta, Qt::cyan };
    QwtText xTitle("Sample index");
    QwtText yTitle("Position");
    QwtText aiDiyTitle("Voltage(mV)");
    //the control waveform
    conWavPlot = new QwtPlot;// (ui.frameConWav);
    for (int i = 0; i < numAoCurve + numDoCurve; i++) {
        outCurve.push_back(new QwtPlotCurve(QString("Signal out curve %1").arg(i)));
        prepareCurve(outCurve[i], conWavPlot, xTitle, yTitle, curveColor[i]);
    }
    piezoSpeedCurve = new QwtPlotCurve("Positioner max speed");
    prepareCurve(piezoSpeedCurve, conWavPlot, xTitle, yTitle, Qt::cyan);
    ui.conWavLayout->addWidget(conWavPlot);

    //the read out waveform
    conReadWavPlot = new QwtPlot;// (ui.frameConWav);
    for (int i = 0; i < numAiCurve + numDiCurve; i++) {
        inCurve.push_back(new QwtPlotCurve(QString("Signal in curve %1").arg(i)));
        prepareCurve(inCurve[i], conReadWavPlot, xTitle, aiDiyTitle, curveColor[i]);
    }
    ui.readWavLayout->addWidget(conReadWavPlot);
}

//convert position in the unit of pixmap to position in the unit of orig img
QPoint Imagine::calcPos(const QPoint& pos)
{
    QPoint result;
    result.rx() = L + double(W) / pixmap.width()*pos.x();
    result.ry() = T + double(H) / pixmap.height()*pos.y();

    return result;
}

//note: idx is 0-based
void Imagine::updateDisplay(const QByteArray &data16, long idx, int imageW, int imageH)
{
    // If something breaks... probably these units are wrong.
    lastRawImg = data16;
    lastImgH = imageH;
    lastImgW = imageW;

    Camera::PixelValue * frame = (Camera::PixelValue *)data16.constData();
    if (ui.actionFlickerControl->isChecked()){
        int oldMax = maxPixelValue;
        calcMinMaxValues(frame, imageW, imageH);
        if (maxPixelValue == 0){
            if (++nContinousBlackFrames < 3){
                maxPixelValue = oldMax;
                goto done;
            }
            else nContinousBlackFrames = 0;
        }//if, black frame
        else {
            nContinousBlackFrames = 0; //reset counter
        }
        maxPixelValue = oldMax;
    }

    //update histogram plot
    int histSamplingRate = 3; //that is, calc histgram every 3 updating
    if (nUpdateImage%histSamplingRate == 0){
        unsigned int maxintensity;
        double maxcount = 0;
        if ((ui.rbImgCameraEnable->isChecked()&& ui.cbCam1Enable->isChecked()) ||
            (ui.rbImgFileEnable->isChecked()&ui.cbImg1Enable->isChecked())) {
            updateHist(histogram, frame, imageW, imageH, maxcount, maxintensity);
            histogram2->detach();
        }
        else {
            updateHist(histogram2, frame, imageW, imageH, maxcount, maxintensity);
            histogram->detach();
        }
        histPlot->setAxisScale(QwtPlot::yLeft, 1, maxcount);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, maxintensity);
        histPlot->replot();
    }

    //update intensity vs stack (or frame) curve
    //  --- live mode:
    if (nUpdateImage%histSamplingRate == 0 && curStatus == eRunning && curAction == eLive){
        updateIntenCurve(frame, imageW, imageH, idx);
    }
    //  --- acq mode:
    if (curStatus == eRunning && curAction == eAcqAndSave
        && idx == dataAcqThread->nFramesPerStack - 1){
        updateIntenCurve(frame, imageW, imageH, dataAcqThread->idxCurStack);
    }

    //copy and scale data
    if (ui.actionNoAutoScale->isChecked()){
        minPixelValue = 0;
        maxPixelValue = (1 << 14)-1;
        factor = 1.0 / (1 << 6); //i.e. /(2^14)*(2^8). note: not >>8 b/c it's 14-bit camera. TODO: take care of 16 bit camera?
    }//if, no contrast adjustment
    else {
        if (ui.actionAutoScaleOnFirstFrame->isChecked()){
            //user can change the setting when acq. is running
            if (maxPixelValue == -1 || maxPixelValue == 0){
                calcMinMaxValues(frame, imageW, imageH);
            }//if, need update min/max values.
        }//if, min/max are from the first frame
        else if (ui.actionAutoScaleOnAllFrames->isChecked()){
            calcMinMaxValues(frame, imageW, imageH);
        }//else if, min/max are from each frame's own data
        else {
            minPixelValue = minPixelValueByUser;
            maxPixelValue = maxPixelValueByUser;
        }//else, user supplied min/max

        factor = 255.0 / (maxPixelValue - minPixelValue);
        if (ui.actionColorizeSaturatedPixels->isChecked()) factor *= 254 / 255.0;
    }//else, adjust contrast(i.e. scale data by min/max values)

    isUpdateImgColor = false;
    updateImage(isUpdateImgColor, false);

    nUpdateImage++;
    appendLog(QString().setNum(nUpdateImage)
        + "-th updated frame(0-based)=" + QString().setNum(idx));

done:
    return;
}//updateDisplay(),

void Imagine::updateDisplayColor(const QByteArray &data1, const QByteArray &data2, long idx, int imageW, int imageH)
{
    lastRawImg = data1;
    lastRawImg2 = data2;
    lastImgH = imageH;
    lastImgW = imageW;

    Camera::PixelValue * frame1 = (Camera::PixelValue *)data1.constData();
    Camera::PixelValue * frame2 = (Camera::PixelValue *)data2.constData();
    if (ui.actionFlickerControl->isChecked()) {
        int oldMax = maxPixelValue;
        calcMinMaxValues(frame1, frame2, imageW, imageH);
        if (maxPixelValue == 0) {
            if (++nContinousBlackFrames < 3) {
                maxPixelValue = oldMax;
                goto done;
            }
            else nContinousBlackFrames = 0;
        }//if, black frame
        else {
            nContinousBlackFrames = 0; //reset counter
        }
        maxPixelValue = oldMax; //?? do nothing?
    }

    //update histogram plot
    int histSamplingRate = 3; //that is, calc histgram every 3 updating
    if (nUpdateImage%histSamplingRate == 0) {
        unsigned int maxintensity, maxintensity2;
        double maxcount = 0, maxcount2 = 0;
        updateHist(histogram, frame1, imageW, imageH, maxcount, maxintensity);
        updateHist(histogram2, frame2, imageW, imageH, maxcount2, maxintensity2);
        histPlot->setAxisScale(QwtPlot::yLeft, 1, max(maxcount, maxcount2));
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, max(maxintensity, maxintensity2));
        histPlot->replot();
        histPlot->legend();
    }

    //update intensity vs stack (or frame) curve
    //  --- live mode:
    if (nUpdateImage%histSamplingRate == 0 && curStatus == eRunning && curAction == eLive) {
        updateIntenCurve(frame1, imageW, imageH, idx);
    }
    //  --- acq mode:
    if (curStatus == eRunning && curAction == eAcqAndSave
        && idx == dataAcqThread->nFramesPerStack - 1) {
        updateIntenCurve(frame1, imageW, imageH, dataAcqThread->idxCurStack);
    }

    //copy and scale data
    if (ui.actionNoAutoScale->isChecked()) {
        minPixelValue = 0;
        maxPixelValue = (1 << 14) - 1;
        minPixelValue2 = 0;
        maxPixelValue2 = (1 << 14) - 1;
        factor = 1.0 / (1 << 4); //i.e. /(2^14)*(2^10). note: not >>8 b/c it's 14-bit camera. TODO: take care of 16 bit camera?
    }//if, no contrast adjustment
    else {
        if (ui.actionAutoScaleOnFirstFrame->isChecked()) {
            //user can change the setting when acq. is running
            if (maxPixelValue == -1 || maxPixelValue == 0
                || maxPixelValue2 == -1 || maxPixelValue2 == 0) {
                calcMinMaxValues(frame1, frame2, imageW, imageH);
            }//if, need update min/max values.
        }//if, min/max are from the first frame
        else if (ui.actionAutoScaleOnAllFrames->isChecked()) {
            calcMinMaxValues(frame1, frame2, imageW, imageH);
        }//else if, min/max are from each frame's own data
        else {
            minPixelValue = minPixelValueByUser;
            maxPixelValue = maxPixelValueByUser;
            minPixelValue2 = minPixelValueByUser2;
            maxPixelValue2 = maxPixelValueByUser2;
        }//else, user supplied min/max

        factor = 1023.0 / (maxPixelValue - minPixelValue);
        factor2 = 1023.0 / (maxPixelValue2 - minPixelValue2);
        if (ui.actionColorizeSaturatedPixels->isChecked()) {
            factor *= 1024 / 1023;
            factor2 *= 1024 / 1023;
        }
    }//else, adjust contrast(i.e. scale data by min/max values)

    isUpdateImgColor = true;
    updateImage(isUpdateImgColor, false);

    nUpdateImage++;
    appendLog(QString().setNum(nUpdateImage)
        + "-th updated frame(0-based)=" + QString().setNum(idx));

done:
    return;
}//updateDisplayColor(),

void Imagine::updateStatus(const QString &str)
{
    if (str == "acq-thread-finish"){
        updateStatus(eIdle, eNoAction);
        if (reqFromScript) {
            reqFromScript = false;
            imagineScript->retVal = true;
            imagineScript->shouldWait = false;
        }
        return;
    }

    if (str.startsWith("valve", Qt::CaseInsensitive)){
        ui.tableWidgetStimDisplay->setCurrentCell(0, dataAcqThread->curStimIndex);
    }//if, update stimulus display

    if (ui.rbImgFileEnable->isChecked())
        return;

    this->statusBar()->showMessage(str);
    appendLog(str);

}

//TODO: move this func to misc.cpp
QString addExtNameIfAbsent(QString filename, QString newExtname)
{
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if (ext.isEmpty()){
        return filename + "." + newExtname;
    }
    else {
        return filename;
    }
}//addExtNameIfAbsent()

#pragma endregion


#pragma region UNSORTED

void Imagine::appendLog(const QString& msg)
{
    static int nAppendingLog = 0;
    //if(nAppendingLog>=1000) {
    if (nAppendingLog >= ui.spinBoxMaxNumOfLinesInLog->value()) {
        ui.textEditLog->clear();
        nAppendingLog = 0;
    }

    ui.textEditLog->append(msg);
    nAppendingLog++;
}

void Imagine::calcMinMaxValues(Camera::PixelValue * frame, int imageW, int imageH)
{
    minPixelValue = maxPixelValue = frame[0];
    for (int i = 1; i<imageW*imageH; i++){
        if (frame[i] == (1 << 16) - 1) continue; //todo: this is a tmp fix for dead pixels
        if (frame[i]>maxPixelValue) maxPixelValue = frame[i];
        else if (frame[i] < minPixelValue) minPixelValue = frame[i];
    }//for, each pixel
}

void Imagine::calcMinMaxValues(Camera::PixelValue * frame1, Camera::PixelValue * frame2, int imageW, int imageH)
{
    minPixelValue = maxPixelValue = frame1[0];
    for (int i = 1; i<imageW*imageH; i++) {
        if (frame1[i] == (1 << 16) - 1) continue; //todo: this is a tmp fix for dead pixels
        if (frame1[i] > maxPixelValue) maxPixelValue = frame1[i];
        else if (frame1[i] < minPixelValue) minPixelValue = frame1[i];
    }//for, each pixel
    minPixelValue2 = maxPixelValue2 = frame2[0];
    for (int i = 1; i<imageW*imageH; i++) {
        if (frame2[i] == (1 << 16) - 1) continue; //todo: this is a tmp fix for dead pixels
        if (frame2[i] > maxPixelValue2) maxPixelValue2 = frame2[i];
        else if (frame2[i] < minPixelValue2) minPixelValue2 = frame2[i];
    }//for, each pixel
}

//TODO: put this func in misc.cpp
bool loadTextFile(string filename, vector<string> & result)
{
    QFile file(filename.c_str());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        return false;
    }

    result.clear();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        result.push_back(line.toStdString());
    }

    return true;
}//loadTextFile(),

void setDockwigetByAction(QAction* act, QDockWidget* widget)
{
    bool visible = act->isChecked();
    widget->setVisible(visible);
    if (visible) widget->raise();
}

bool Imagine::loadPreset()
{
    QScriptValue preset = se->globalObject().property("preset");
    //piezo
    QScriptValue sv = preset.property("startPosition");
    if (sv.isValid()) ui.doubleSpinBoxStartPos->setValue(sv.toNumber());
    sv = preset.property("stopPosition");
    if (sv.isValid()) ui.doubleSpinBoxStopPos->setValue(sv.toNumber());
    sv = preset.property("initialPosition");
    if (sv.isValid()) ui.doubleSpinBoxCurPos->setValue(sv.toNumber());
    sv = preset.property("travelBackTime");
    if (sv.isValid()) ui.doubleSpinBoxPiezoTravelBackTime->setValue(sv.toNumber());
    ///camera
    sv = preset.property("numOfStacks");
    if (sv.isValid()) ui.spinBoxNumOfStacks->setValue(sv.toNumber());
    sv = preset.property("framesPerStack");
    if (sv.isValid()) ui.spinBoxFramesPerStack->setValue(sv.toNumber());
    sv = preset.property("exposureTime");
    if (sv.isValid()) ui.doubleSpinBoxExpTime->setValue(sv.toNumber());
    sv = preset.property("idleTime");
    if (sv.isValid()) ui.doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(sv.toNumber());
    sv = preset.property("acqTriggerMode");
    if (sv.isValid()){
        QString ttStr = sv.toString();
//        ui.comboBoxAcqTriggerMode->setCurrentIndex(ttStr == "internal");
    }
	//TODO: add expTriggerMode as a preset
    sv = preset.property("gain");
    if (sv.isValid()) ui.spinBoxGain->setValue(sv.toNumber());

    return true;
}

void Imagine::updateStatus(ImagineStatus newStatus, ImagineAction newAction)
{
    curStatus = newStatus;
    curAction = newAction;

    vector<QWidget*> enabledWidgets, disabledWidgets;
    vector<QAction*> enabledActions, disabledActions;
    if (curStatus == eIdle){
        //in the order of from left to right, from top to bottom:
        enabledActions.push_back(ui.actionStartAcqAndSave);
        enabledActions.push_back(ui.actionStartLive);
        enabledActions.push_back(ui.actionOpenShutter);
        enabledActions.push_back(ui.actionCloseShutter);

        disabledActions.push_back(ui.actionStop);

        //no widgets to disable

    }//if, idle
    else if (curStatus == eRunning){
        enabledActions.push_back(ui.actionStop);

        //no widget to enable

        disabledActions.push_back(ui.actionStartAcqAndSave);
        disabledActions.push_back(ui.actionStartLive);

        //change intensity plot xlabel
        if (curAction == eLive){
            intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number");
        }
        else {
            intenPlot->setAxisTitle(QwtPlot::xBottom, "stack number");
        }

    }//else if, running
    else {
        disabledActions.push_back(ui.actionStop);

    }//else, stopping 

    for (int i = 0; i < enabledWidgets.size(); ++i){
        enabledWidgets[i]->setEnabled(true);
    }

    for (int i = 0; i < enabledActions.size(); ++i){
        enabledActions[i]->setEnabled(true);
    }

    for (int i = 0; i < disabledWidgets.size(); ++i){
        disabledWidgets[i]->setEnabled(false);
    }

    for (int i = 0; i < disabledActions.size(); ++i){
        disabledActions[i]->setEnabled(false);
    }

}//Imagine::updateUiStatus()

void Imagine::readPiezoCurPos()
{
    double um;
    Positioner *pos = dataAcqThread->pPositioner;
    if (pos != NULL && pos->curPos(&um)) {
        ui.labelCurPos->setText(QString::number(um, 'f', 2));
    }
    else {
        ui.labelCurPos->setText("Unknown");
    }
}

#pragma endregion


#pragma region UI_CALLBACKS

void Imagine::zoom_onMouseReleased(QMouseEvent* event)
{
    if ((event->button() == Qt::LeftButton)&& zoom_isMouseDown) {
        appendLog("released");
        zoom_curPos = event->pos();
        zoom_isMouseDown = false;

        appendLog(QString("last pos: (%1, %2); cur pos: (%3, %4)")
            .arg(zoom_downPos.x()).arg(zoom_downPos.y())
            .arg(zoom_curPos.x()).arg(zoom_curPos.y()));

        if (pixmap.isNull()) return; //NOTE: this is for not zooming logo image

        //update L,T,W,H
        QPoint LT = calcPos(zoom_downPos);
        QPoint RB = calcPos(zoom_curPos);

        if (LT.x() > RB.x() && LT.y() > RB.y()) {
            L = -1;
            goto done;
        }//if, should show full image

        if (!(LT.x() < RB.x() && LT.y() < RB.y())) return; //unknown action

        L = LT.x(); T = LT.y();
        W = RB.x() - L; H = RB.y() - T;
    done:
        //refresh the image
        updateImage(isUpdateImgColor, true);
    }//if, dragging
}

void Imagine::on_actionDisplayFullImage_triggered()
{
    L = -1;
    updateImage(isUpdateImgColor, true);

}

void Imagine::zoom_onMousePressed(QMouseEvent* event)
{
    appendLog("pressed");
    if (event->button() == Qt::LeftButton) {
        zoom_downPos = event->pos();
        zoom_isMouseDown = true;
    }
}

void Imagine::zoom_onMouseMoved(QMouseEvent* event)
{
    //appendLog("moved");
    if ((event->buttons() & Qt::LeftButton) && zoom_isMouseDown){
        zoom_curPos = event->pos();

        if (!pixmap.isNull()){ //TODO: if(pixmap && idle)
            updateImage(isUpdateImgColor, false);
        }
    }
}

void Imagine::startAcqAndSave()
{
    if ((!paramOK) && (dataAcqThread->pCamera->getModel() != "dummy")) {
        QMessageBox::information(this, "Wrong parameters --- Imagine",
            "Please correct the parameters.");
        return;
    }

    if (dataAcqThread->headerFilename == "") {
        if (ui.lineEditFilename->text() != "") {
            QMessageBox::information(this, "Forget to apply configuration --- Imagine",
                "Please apply the configuration by clicking Apply button");
            return;
        }

        QMessageBox::information(this, "Need file name --- Imagine",
            "Please specify file name to save");
        return;
    }

    //warn user if overwrite file:
    if (QFile::exists(dataAcqThread->headerFilename) &&
        QMessageBox::question(
            this,
            tr("Overwrite File? -- Imagine"),
            tr("The file called \n   %1\n already exists. "
                "Do you want to overwrite it?").arg(dataAcqThread->headerFilename),
            tr("&Yes"), tr("&No"),
            QString(), 0, 1)) {
        return;
    }//if, file exists and user don't want to overwrite it

    //warn user if there are params changes pending:
    if (modified &&
        QMessageBox::question(
            this,
            "Apply pending change(s)? -- Imagine",
            "Do you want to go ahead without applying them?",
            "&Yes", "&No",
            QString(), 0, 1)) {
        return;
    }//if, 

    if (!CheckAndMakeFilePath(dataAcqThread->headerFilename)) {
        QMessageBox::information(this, "File creation error.",
            "Unable to create the directory");
    }

    nUpdateImage = 0;
    minPixelValue = maxPixelValue = -1;

    dataAcqThread->applyStim = ui.cbStim->isChecked();
    if (dataAcqThread->applyStim) {
        dataAcqThread->curStimIndex = 0;
    }

    holdDisplayCamFile();

    intenCurveData->clear();

    if (isRecordCommander)
        dataAcqThread->ownPos = true;
    else
        dataAcqThread->ownPos = false;
    dataAcqThread->isLive = false;
    dataAcqThread->startAcq();
    updateStatus(eRunning, eAcqAndSave);
}

void Imagine::on_actionStartAcqAndSave_triggered()
{
    Imagine *otherImagine = NULL;
    if (ui.cbBothCamera->isChecked()) {
        if (masterImagine != NULL)
            otherImagine = masterImagine;
        else
            otherImagine = slaveImagine;
    }
    if (otherImagine != NULL) {
        otherImagine->startAcqAndSave();
    }
    isRecordCommander = true;
    startAcqAndSave();
    isRecordCommander = false;
}

void Imagine::on_actionStartLive_triggered()
{
    if ((!paramOK) && (dataAcqThread->pCamera->getModel() != "dummy")) {
        QMessageBox::information(this, "Wrong parameters --- Imagine",
            "Please correct the parameters.");
        return;
    }

    //warn user if there are params changes pending:
    if (modified &&
        QMessageBox::question(
        this,
        "Apply pending change(s)? -- Imagine",
        "Do you want to go ahead without applying them?",
        "&Yes", "&No",
        QString(), 0, 1)){
        return;
    }//if, 

    nUpdateImage = 0;
    minPixelValue = maxPixelValue = -1;

    holdDisplayCamFile();
    
    intenCurveData->clear();

    dataAcqThread->isLive = true;
    dataAcqThread->startAcq();
    updateStatus(eRunning, eLive);
}

void Imagine::on_actionAutoScaleOnFirstFrame_triggered()
{
    minPixelValue = maxPixelValue = -1;
}

void Imagine::on_actionContrastMin1_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the min pixel value to cut off", "min", minPixelValueByUser, 0, 1 << 16, 10, &ok);
    if (ok) minPixelValueByUser = value;
    displayImageUpdate();
}

void Imagine::on_actionContrastMax1_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the max pixel value to cut off", "max", maxPixelValueByUser, 0, 1 << 16, 10, &ok);
    if (ok) maxPixelValueByUser = value;
    displayImageUpdate();
}

void Imagine::on_actionContrastMin2_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the min pixel value to cut off", "min", minPixelValueByUser2, 0, 1 << 16, 10, &ok);
    if (ok) minPixelValueByUser2 = value;
    displayImageUpdate();
}

void Imagine::on_actionContrastMax2_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the max pixel value to cut off", "max", maxPixelValueByUser2, 0, 1 << 16, 10, &ok);
    if (ok) maxPixelValueByUser2 = value;
    displayImageUpdate();
}

void Imagine::on_actionStop_triggered()
{
    dataAcqThread->stopAcq();
    updateStatus(eStopping, curAction);
}

void Imagine::on_actionViewHelp_triggered()
{
    if (!proc)
        proc = new QProcess();

    if (proc->state() != QProcess::Running) {
        QString app = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QDir::separator();
        app += QLatin1String("assistant");

        QStringList args;
        args << QLatin1String("-collectionFile")
            << QLatin1String("./documentation/imagineHelp.qhc")
            << QLatin1String("-enableRemoteControl");

        proc->start(app, args);

        if (!proc->waitForStarted()) {
            QMessageBox::critical(0, QObject::tr("Simple Text Viewer"),
                QObject::tr("Unable to launch Qt Assistant (%1)").arg(app));
            return;
        }
    }
    return;
}

void Imagine::on_actionOpenShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //open laser shutter
    bool retVal = digOut->singleOut(4, true);
/*
    QString portName;
    if (!(laserCtrlSerial->isPortOpen())) {
       emit openLaserSerialPort(portName);
    }

    changeLaserShutters();
    for (int i = 1; i <= 4; i++)
        changeLaserTrans(true, i);
*/
    QString str;
    if(retVal == true)
        str = QString("Open laser shutter");
    else
        str = QString("Open laser shutter error");
    appendLog(str);

    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(true);
}

void Imagine::on_actionCloseShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //open laser shutter
    bool retVal = digOut->singleOut(4, false);
/*
    emit setLaserShutters(0);
*/
    QString str;
    if (retVal == true)
        str = QString("Close laser shutter");
    else
        str = QString("Close laser shutter error");
    appendLog(str);
    ui.actionOpenShutter->setEnabled(true);
    ui.actionCloseShutter->setEnabled(false);
}

void Imagine::closeEvent(QCloseEvent *event)
{
    //see: QCloseEvent Class Reference
    /*
    if(maybeSave()) {
    writeSettings();
    event->accept();
    } else {
    event->ignore();
    }
    */

    Camera& camera = *dataAcqThread->pCamera;

    //shutdown camera
    //see: QMessageBox Class Reference
    QMessageBox *mb = new QMessageBox("Imagine",    //caption
        "Shutting down camera, \nplease wait ...", //text
        QMessageBox::Information,                  //icon
        QMessageBox::NoButton,                     //btn 0
        QMessageBox::NoButton,                     //btn 1
        QMessageBox::NoButton,                     //btn 2
        this                                       //parent
        //wflag. use the default
        );
    mb->show();
    camera.fini();
    delete mb;

    if (slaveImagine != NULL) {
        Ui_ImagineClass* slaveUi = &slaveImagine->ui;    // master window
        QWidget *masterTab[5] = { ui.tabStim, ui.tabPiezo, ui.tabLaser, ui.tabWaveform, ui.tabAI };
        QWidget *slaveTab[5] = { (*slaveUi).tabStim, (*slaveUi).tabPiezo, (*slaveUi).tabLaser,
                                (*slaveUi).tabWaveform, (*slaveUi).tabAI };
        for (int i = 0; i < 5; i++) {
            int index = ui.tabWidgetCfg->indexOf(masterTab[i]);
            QString tabName = ui.tabWidgetCfg->tabText(index);
            slaveImagine->ui.tabWidgetCfg->insertTab(index, slaveTab[i], tabName);
        }
        (*slaveUi).cbBothCamera->setChecked(false);
        (*slaveUi).cbBothCamera->setVisible(false);
        (*slaveUi).gbDisplaySourceSelect->show();
        (*slaveUi).groupBoxPlayCamImage->show();
        (*slaveUi).groupBoxBlendingOption->show();
        (*slaveUi).cbCam2Enable->setChecked(false);
        (*slaveUi).cbCam2Enable->setVisible(false);
        slaveImagine->masterImagine == NULL;
    }
    if (masterImagine != NULL) {
        masterImagine->ui.cbBothCamera->setChecked(false);
        masterImagine->ui.cbBothCamera->setVisible(false);
        masterImagine->slaveImagine == NULL;
    }

    event->accept();
}

void Imagine::on_btnFullChipSize_clicked()
{
    isUsingSoftROI = false;
    setROIMinMaxSize();
    ui.spinBoxHstart->setValue(1);
    ui.spinBoxHend->setValue(maxROIHSize);
    ui.spinBoxVstart->setValue(1);
    ui.spinBoxVend->setValue(maxROIVSize);
}

void Imagine::on_btnUseZoomWindow_clicked()
{
    if (image.isNull()){
        QMessageBox::warning(this, tr("Imagine"),
            tr("Please acquire one full chip image first"));
        return;
    }

//    if (image.width() != 1004 || image.height() != 1002){
//        QMessageBox::warning(this, tr("Imagine"),
//            tr("Please acquire one full chip image first"));
//        return;
//    }

    if (L == -1) {
        on_btnFullChipSize_clicked();
        return;
    }//if, full image
    else {
        isUsingSoftROI = true;
        setROIMinMaxSize();
        ui.spinBoxHstart->setValue(L + 1);  //NOTE: hstart is 1-based
        ui.spinBoxHend->setValue(L + W);
        ui.spinBoxVstart->setValue(T + 1);
        ui.spinBoxVend->setValue(T + H);

    }//else,

}

bool Imagine::checkRoi()
{
    Camera& camera = *dataAcqThread->pCamera;

    //set the roi def
    se->globalObject().setProperty("hstart", ui.spinBoxHstart->value());
    se->globalObject().setProperty("hend", ui.spinBoxHend->value());
    se->globalObject().setProperty("vstart", ui.spinBoxVstart->value());
    se->globalObject().setProperty("vend", ui.spinBoxVend->value());

    //set the window reference
    QScriptValue qtwin = se->newQObject(this);
    se->globalObject().setProperty("window", qtwin);

    QScriptValue checkFunc = se->globalObject().property("checkRoi");

    bool result = true;
    if (checkFunc.isFunction()) result = checkFunc.call().toBool();

    return result;
}

void Imagine::onModified()
{
    modified = true;
    ui.btnApply->setEnabled(modified);
}

void Imagine::on_spinBoxHstart_editingFinished()
{
    int hEndValue = ui.spinBoxHend->value();
    int newValue = ui.spinBoxHstart->value();

    if ((newValue - 1) % roiStepsHor) {
        newValue = (int)(newValue / roiStepsHor + 0.5) * roiStepsHor + 1;
        if (newValue < hEndValue) {
            appendLog(QString("Invalid number. This value should be n*%1+1").arg(roiStepsHor));
        }
        else {
            newValue = hEndValue - (hEndValue - 2) % roiStepsHor - 1;
            appendLog(QString("Invalid number. This value should be The value should be n*%1+1 and less than hor. end").arg(roiStepsHor));
        }
    }
    else {
        if (newValue < hEndValue) {
            goto setvalue;
        }
        else {
            newValue = hEndValue - (hEndValue - 2) % roiStepsHor - 1;
            appendLog("Invalid number. This value should be less than hor. end");
        }
    }
    appendLog(QString("The value is corrected as the closest valid number %1").arg(newValue));
setvalue:
    ui.spinBoxHstart->setValue(newValue);

}

void Imagine::on_spinBoxHend_editingFinished()
{
    int hStartValue = ui.spinBoxHstart->value();
    int newValue = ui.spinBoxHend->value();

    newValue = maxROIHSize - newValue;
    if (newValue % roiStepsHor) {
        newValue = (int)(newValue / roiStepsHor + 0.5) * roiStepsHor;
        newValue = maxROIHSize - newValue;
        if (newValue > hStartValue) {
            appendLog(QString("Invalid number. This value should be (%1 - n*%2)").arg(maxROIHSize).arg(roiStepsHor));
        }
        else {
            newValue = hStartValue + (maxROIHSize - hStartValue) % roiStepsHor;
            appendLog(QString("Invalid number. This value should be (%1 - n*%2) and grater than hor. start").arg(maxROIHSize).arg(roiStepsHor));
        }
    }
    else {
        newValue = maxROIHSize - newValue;
        if (newValue > hStartValue) {
            goto setvalue;
        }
        else {
            newValue = hStartValue + (maxROIHSize - hStartValue) % roiStepsHor;
            appendLog(QString("Invalid number. This value should be grater than hor. start").arg(maxROIHSize));
        }
    }
    appendLog(QString("The value is corrected as the closest valid number %1").arg(newValue));
setvalue:
    ui.spinBoxHend->setValue(newValue);

}


void Imagine::on_spinBoxVstart_valueChanged(int newValue)
{
    if(!isUsingSoftROI)
        ui.spinBoxVend->setValue(maxROIVSize + 1 - newValue);
}


void Imagine::on_spinBoxVend_valueChanged(int newValue)
{
    if (!isUsingSoftROI)
        ui.spinBoxVstart->setValue(maxROIVSize + 1 - newValue);
}


bool Imagine::waveformValidityCheck(void)
{
    CFErrorCode err = conWaveData->waveformValidityCheck();
    if (conWaveData->getErrorMsg() != "") {
        QMessageBox::critical(this, "Imagine", conWaveData->getErrorMsg(), QMessageBox::Ok, QMessageBox::NoButton);
    }
    if (err == NO_CF_ERROR)
        return true;
    else
        return false;
}

void Imagine::on_sbObjectiveLens_valueChanged(int newValue)
{
    int magnificationFactor = se->globalObject().property("magnificationFactor").toNumber();
    double adjusted_mag = (double)newValue * (double)magnificationFactor;
    ui.doubleSpinBoxUmPerPxlXy->setValue(6.5/ adjusted_mag); // PCO.Edge pixel size = 6.5um
}

void Imagine::duplicateParameters(Ui_ImagineClass* destUi)
{
    if (destUi != NULL) {
        if (ui.cbBothCamera->isChecked()) {
            destUi->cbBothCamera->setChecked(true);
            destUi->spinBoxNumOfStacks->setValue(ui.spinBoxNumOfStacks->value());
            destUi->spinBoxFramesPerStack->setValue(ui.spinBoxFramesPerStack->value());
            destUi->doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value());
            destUi->comboBoxExpTriggerMode->setCurrentIndex(ui.comboBoxExpTriggerMode->currentIndex());
            destUi->spinBoxAngle->setValue(ui.spinBoxAngle->value());
            destUi->doubleSpinBoxUmPerPxlXy->setValue(ui.doubleSpinBoxUmPerPxlXy->value());
            destUi->comboBoxHorReadoutRate->setCurrentIndex(ui.comboBoxHorReadoutRate->currentIndex());
            destUi->comboBoxPreAmpGains->setCurrentIndex(ui.comboBoxPreAmpGains->currentIndex());
            destUi->spinBoxGain->setValue(ui.spinBoxGain->value());
            destUi->comboBoxVertShiftSpeed->setCurrentIndex(ui.comboBoxVertShiftSpeed->currentIndex());
            destUi->comboBoxVertClockVolAmp->setCurrentIndex(ui.comboBoxVertClockVolAmp->currentIndex());
            destUi->spinBoxHstart->setValue(ui.spinBoxHstart->value());
            destUi->spinBoxHend->setValue(ui.spinBoxHend->value());
            destUi->spinBoxVstart->setValue(ui.spinBoxVstart->value());
            destUi->spinBoxVend->setValue(ui.spinBoxVend->value());
        }
        else {
            destUi->cbBothCamera->setChecked(false);
        }
    }
}

void Imagine::on_btnApply_clicked()
{
    Ui_ImagineClass* destUi = NULL;
    if ((masterImagine == NULL) && (slaveImagine != NULL)) { // camera1
        destUi = &slaveImagine->ui;
    }
    else if ((masterImagine != NULL) && (slaveImagine == NULL)) { // camera2
        destUi = &masterImagine->ui;
    }

    // If both camera capturing, duplicate camera parameters to the other camera setting
    duplicateParameters(destUi);

    isApplyCommander = true;
    applySetting();
    isApplyCommander = false;
}

bool Imagine::applySetting()
{
    if (curStatus != eIdle){
        QMessageBox::information(this, "Please wait --- Imagine",
            "Please click 'apply' when the camera is idle.");
        return false;
    }

    QString headerFilename;
    Ui_ImagineClass* masterUi = NULL;    // master window
    Camera* camera = dataAcqThread->pCamera;
    QString expTriggerModeStr;
    Camera::AcqTriggerMode acqTriggerMode;
	Camera::ExpTriggerMode expTriggerMode;
    DataAcqThread *masterDataAcqTh;
    Positioner *pos = dataAcqThread->pPositioner;
    Imagine *otherImagine = NULL;

    if (masterImagine == NULL) { // Imagine (1)
        masterUi = &ui;
        masterDataAcqTh = dataAcqThread;
        if (slaveImagine != NULL)
            otherImagine = slaveImagine;
    }
    else {                       // Imagine (2)
        masterUi = &masterImagine->ui;
        masterDataAcqTh = masterImagine->dataAcqThread;
// TMP        masterControlWav = masterImagine->conWaveData;
        otherImagine = masterImagine;
    }

    acqTriggerMode = Camera::eInternalTrigger;
    dataAcqThread->isUsingWav = true;   // Both parameter and waveform controls
                                        // use waveform control backend
    dataAcqThread->horShiftSpeedIdx = ui.comboBoxHorReadoutRate->currentIndex();
    dataAcqThread->preAmpGainIdx = ui.comboBoxPreAmpGains->currentIndex();
    dataAcqThread->gain = ui.spinBoxGain->value();
    dataAcqThread->verShiftSpeedIdx = ui.comboBoxVertShiftSpeed->currentIndex();
    dataAcqThread->verClockVolAmp = ui.comboBoxVertClockVolAmp->currentIndex();
    dataAcqThread->preAmpGain = ui.comboBoxPreAmpGains->currentText();
    dataAcqThread->horShiftSpeed = ui.comboBoxHorReadoutRate->currentText();
    dataAcqThread->verShiftSpeed = ui.comboBoxVertShiftSpeed->currentText();
    dataAcqThread->angle = ui.spinBoxAngle->value();

    if ((*masterUi).cbWaveformEnable->isChecked()) { // waveform enabled
        dataAcqThread->sampleRate = conWaveDataUser->sampleRate;
        dataAcqThread->isBiDirectionalImaging = conWaveDataUser->bidirection;
        dataAcqThread->conWaveData = conWaveDataUser;
        if (camera->getCameraID() == 1) {
            dataAcqThread->exposureTime = (*masterUi).doubleSpinBoxExpTimeWav1->value();
            dataAcqThread->nStacksUser = conWaveDataUser->nStacks1;
            dataAcqThread->nFramesPerStackUser = conWaveDataUser->nFrames1;
            expTriggerModeStr = (*masterUi).comboBoxExpTriggerModeWav1->currentText();
        }
        else {
            dataAcqThread->exposureTime = (*masterUi).doubleSpinBoxExpTimeWav2->value();
            dataAcqThread->nStacksUser = conWaveDataUser->nStacks2;
            dataAcqThread->nFramesPerStackUser = conWaveDataUser->nFrames2;
            expTriggerModeStr = (*masterUi).comboBoxExpTriggerModeWav2->currentText();
        }
    }
    else {
        expTriggerModeStr = ui.comboBoxExpTriggerMode->currentText();
        dataAcqThread->sampleRate = 50000; // Hard coded (ocpi-1 cannot work with 100000)
        dataAcqThread->nStacksUser = ui.spinBoxNumOfStacks->value();
        dataAcqThread->nFramesPerStackUser = ui.spinBoxFramesPerStack->value();
        dataAcqThread->idleTimeBwtnStacks = ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();
        dataAcqThread->piezoStartPosUm = (*masterUi).doubleSpinBoxStartPos->value();
        dataAcqThread->piezoStopPosUm = (*masterUi).doubleSpinBoxStopPos->value();
        dataAcqThread->piezoTravelBackTime = (*masterUi).doubleSpinBoxPiezoTravelBackTime->value();
        dataAcqThread->isBiDirectionalImaging = (*masterUi).cbBidirectionalImaging->isChecked();
        dataAcqThread->exposureTime = ui.doubleSpinBoxExpTime->value();
    }
    dataAcqThread->nFramesPerStack = dataAcqThread->nStacksUser*dataAcqThread->nFramesPerStackUser;
    dataAcqThread->nStacks = 1;

    if (expTriggerModeStr == "External Start") expTriggerMode = Camera::eExternalStart;
    else if (expTriggerModeStr == "External Control")  expTriggerMode = Camera::eExternalControl;
    else if (expTriggerModeStr == "Fast External Control")  expTriggerMode = Camera::eFastExternalControl;
    else {
        assert(0); //if code goes here, it is a bug
    }
    dataAcqThread->acqTriggerMode = acqTriggerMode;
    dataAcqThread->expTriggerMode = expTriggerMode;

    dataAcqThread->pPositioner->setScanRateAo(dataAcqThread->sampleRate);

    //params for binning
    camera->isUsingSoftROI = isUsingSoftROI;
    dataAcqThread->hstart = camera->hstart = ui.spinBoxHstart->value();
    dataAcqThread->hend = camera->hend = ui.spinBoxHend->value();
    dataAcqThread->vstart = camera->vstart = ui.spinBoxVstart->value();
    dataAcqThread->vend = camera->vend = ui.spinBoxVend->value();
    dataAcqThread->umPerPxlXy = ui.doubleSpinBoxUmPerPxlXy->value();

    camera->updateImageParams(dataAcqThread->nStacks, dataAcqThread->nFramesPerStack); //transfer image params to camera object

    if (!isUsingSoftROI) {
        //enforce #imageSizeBytes is x times of 16
        if (camera->imageSizeBytes % 16) {
            QMessageBox::critical(this, "Imagine", "ROI spec is wrong (#pixels per frame is not x times of 8)."
                , QMessageBox::Ok, QMessageBox::NoButton);

            goto skip;
        }

        if (!checkRoi()) {
            QMessageBox::critical(this, "Imagine", "ROI spec is wrong."
                , QMessageBox::Ok, QMessageBox::NoButton);

            goto skip;
        }
    }

    if (camera->vendor == "avt"){
        if (dataAcqThread->exposureTime < 1 / 57.0){
            QMessageBox::critical(this, "Imagine", "exposure time is too small."
                , QMessageBox::Ok, QMessageBox::NoButton);

            goto skip;
        }
    }

    L = -1;

    for (int i = 0; i < 2; ++i){
        paramOK = camera->setAcqParams(dataAcqThread->gain,
            dataAcqThread->preAmpGainIdx,
            dataAcqThread->horShiftSpeedIdx,
            dataAcqThread->verShiftSpeedIdx,
            dataAcqThread->verClockVolAmp
            );
        if (!paramOK) {
            updateStatus(QString("Camera: applied params: ") + camera->getErrorMsg().c_str());
            goto skip;
        }

        paramOK = camera->setAcqModeAndTime(Camera::eLive,
            dataAcqThread->exposureTime,
            dataAcqThread->nFramesPerStack,
            dataAcqThread->acqTriggerMode,
            dataAcqThread->expTriggerMode
            );
        updateStatus(QString("Camera: applied params: ") + camera->getErrorMsg().c_str());
        if (!paramOK) goto skip;
    }

    //get the real params used by the camera:
    if ((dataAcqThread->expTriggerMode == Camera::eExternalControl) ||
        (dataAcqThread->expTriggerMode == Camera::eFastExternalControl)) {
        double readoutTime = camera->getCycleTime();
        dataAcqThread->cycleTime = dataAcqThread->exposureTime + readoutTime;
    }
    else
        dataAcqThread->cycleTime = camera->getCycleTime();
    cout << "cycle time is: " << dataAcqThread->cycleTime << endl;

    //set filenames:
    headerFilename = ui.lineEditFilename->text();
    dataAcqThread->headerFilename = headerFilename;
    if (!headerFilename.isEmpty()){
        dataAcqThread->aiFilename = replaceExtName(headerFilename, "ai");
        dataAcqThread->diFilename = replaceExtName(headerFilename, "di");
        dataAcqThread->camFilename = replaceExtName(headerFilename, "cam");
        dataAcqThread->commandFilename = ui.lineEditConWaveFile->text();
        dataAcqThread->sifFileBasename = replaceExtName(headerFilename, "");
        if (!CheckFileExtention(headerFilename)) { // check file extention
            if (!QMessageBox::question(this, "File name error.",
                "File extention is not 'imagine'. Do you want to change it?",
                "&Yes", "&No",
                QString(), 0, 1)){
                dataAcqThread->headerFilename = replaceExtName(headerFilename, "imagine");
                ui.lineEditFilename->clear();
                ui.lineEditFilename->insert(dataAcqThread->headerFilename);
            }
        }
    }//else, save data

    dataAcqThread->stimFileContent = ui.textEditStimFileContent->toPlainText();
    dataAcqThread->comment = ui.textEditComment->toPlainText();

    // if same filename .cam file is open, close it
    if (!dataAcqThread->camFilename.compare(img1.camFile.fileName(),Qt::CaseInsensitive))
        if (img1.camFile.isOpen())
            img1.camFile.close();
    if (!dataAcqThread->camFilename.compare(img2.camFile.fileName(),Qt::CaseInsensitive))
        if (img2.camFile.isOpen())
            img2.camFile.close();

    if (isApplyCommander) {
        if (otherImagine != NULL) {
            otherImagine->applySetting();
        }
        if (!(*masterUi).cbWaveformEnable->isChecked()) { // waveform disabled
            if (conWaveDataParam)
                delete conWaveDataParam;
            conWaveDataParam = new ControlWaveform(rig);
            conWaveDataParam->sampleRate = dataAcqThread->sampleRate;
            if (ui.cbBothCamera->isChecked()) { // Both camera
                DataAcqThread *dataAcqThreadCam1, *dataAcqThreadCam2;
                if (dataAcqThread->pCamera->getCameraID() == 1) {
                    dataAcqThreadCam1 = dataAcqThread;
                    dataAcqThreadCam2 = otherImagine->dataAcqThread;
                }
                else {
                    dataAcqThreadCam1 = otherImagine->dataAcqThread;
                    dataAcqThreadCam2 = dataAcqThread;
                }
                conWaveDataParam->nStacks1 = dataAcqThreadCam1->nStacksUser;
                conWaveDataParam->nFrames1 = dataAcqThreadCam1->nFramesPerStackUser;
                conWaveDataParam->expTriggerModeStr1 = expTriggerModeStr;
                conWaveDataParam->exposureTime1 = dataAcqThreadCam1->exposureTime;
                conWaveDataParam->nStacks2 = dataAcqThreadCam2->nStacksUser;
                conWaveDataParam->nFrames2 = dataAcqThreadCam2->nFramesPerStackUser;
                conWaveDataParam->expTriggerModeStr2 = expTriggerModeStr;
                conWaveDataParam->exposureTime2 = dataAcqThreadCam2->exposureTime;
                conWaveDataParam->cycleTime = max(dataAcqThreadCam1->cycleTime, dataAcqThreadCam2->cycleTime);
                conWaveDataParam->enableCam1 = true;
                conWaveDataParam->enableCam2 = true;
            }
            else {
                if (dataAcqThread->pCamera->getCameraID() == 1) { // If camera1
                    conWaveDataParam->exposureTime1 = dataAcqThread->exposureTime;
                    conWaveDataParam->exposureTime2 = 0;
                    conWaveDataParam->nStacks1 = dataAcqThread->nStacksUser;
                    conWaveDataParam->nFrames1 = dataAcqThread->nFramesPerStackUser;
                    conWaveDataParam->nStacks2 = 0;
                    conWaveDataParam->nFrames2 = 0;
                    conWaveDataParam->enableCam1 = true;
                    conWaveDataParam->enableCam2 = false;
                }
                else {
                    conWaveDataParam->exposureTime1 = 0;
                    conWaveDataParam->exposureTime2 = dataAcqThread->exposureTime;
                    conWaveDataParam->nStacks1 = 0;
                    conWaveDataParam->nFrames1 = 0;
                    conWaveDataParam->nStacks2 = dataAcqThread->nStacksUser;
                    conWaveDataParam->nFrames2 = dataAcqThread->nFramesPerStackUser;
                    conWaveDataParam->enableCam1 = false;
                    conWaveDataParam->enableCam2 = true;
                }
                conWaveDataParam->expTriggerModeStr1 = expTriggerModeStr;
                conWaveDataParam->expTriggerModeStr2 = expTriggerModeStr;
                conWaveDataParam->cycleTime = dataAcqThread->cycleTime;
            }

            conWaveDataParam->bidirection = dataAcqThread->isBiDirectionalImaging;
            if (dataAcqThread->isBiDirectionalImaging && (dataAcqThread->nStacksUser % 2)) {
                QMessageBox::critical(this, "Imagine", "Stack number should be even number in bi-dirctional acquisition mode"
                    , QMessageBox::Ok, QMessageBox::NoButton);
                goto skip;
            }
            conWaveDataParam->idleTimeBwtnStacks = dataAcqThread->idleTimeBwtnStacks;
            conWaveDataParam->piezoStartPosUm = dataAcqThread->piezoStartPosUm;
            conWaveDataParam->piezoStopPosUm = dataAcqThread->piezoStopPosUm;
//            if (masterImagine->ui.cbAutoSetPiezoTravelBackTime->isChecked())
//                conWaveData->piezoTravelBackTime = abs(conWaveData->piezoStartPosUm - conWaveData->piezoStopPosUm)/
//                                                    conWaveData->maxPiezoSpeed; // + 0.005sec
//            else
                conWaveDataParam->piezoTravelBackTime = dataAcqThread->piezoTravelBackTime;
//            dataAcqThread->applyStim = masterImagine->ui.cbStim->isChecked();
            conWaveDataParam->applyStim = masterUi->cbStim->isChecked();
            conWaveDataParam->stimuli = &(masterDataAcqTh->stimuli);
            conWaveDataParam->genDefaultControl(headerFilename);
            dataAcqThread->conWaveData = conWaveDataParam;
            conWaveData = conWaveDataParam;
            if(masterImagine != NULL)
                masterImagine->displayConWaveData();
            else
                displayConWaveData();
            if (conWaveData->getErrorMsg() != "") {
                QMessageBox::critical(this, "Imagine", conWaveData->getErrorMsg(),
                    QMessageBox::Ok, QMessageBox::NoButton);
            }
        }
        else {
            conWaveData = conWaveDataUser;
        }
        if (!waveformValidityCheck()) // waveform is not valid
            goto skip;
        // setup default laser TTL output value
        conWaveData->laserTTLSig = laserTTLSig;
        conWaveData->setLaserIntensityValue(1, ui.doubleSpinBox_aotfLine1->value());
        conWaveData->setLaserIntensityValue(2, ui.doubleSpinBox_aotfLine2->value());
        conWaveData->setLaserIntensityValue(3, ui.doubleSpinBox_aotfLine3->value());
        conWaveData->setLaserIntensityValue(4, ui.doubleSpinBox_aotfLine4->value());
        conWaveData->setLaserIntensityValue(5, ui.doubleSpinBox_aotfLine5->value());
        // move positioner to start position
        int chIdx = conWaveData->getChannelIdxFromSig(STR_axial_piezo);
        double um;
        bool ok = conWaveData->getCtrlSampleValue(chIdx, 0, um, PDT_Z_POSITION);
        if (ok) pos->moveTo(um);
        // clear all digital output as low
        digOut->clearAllOutput();

        // if applicable, make sure positioner preparation went well...
        if (pos != NULL && !dataAcqThread->prepareDaqBuffered())
            paramOK = false;
        if (!paramOK) {
            QString msg = QString("Positioner: applied params failed: ") + pos->getLastErrorMsg().c_str();
            updateStatus(msg);
            QMessageBox::critical(0, "Imagine: Failed to setup piezo/stage.", msg
                , QMessageBox::Ok, QMessageBox::NoButton);
            goto skip;
        }
    }

    modified = false;
    ui.btnApply->setEnabled(modified);
    return true;

skip:
    return false;
}

void Imagine::on_doubleSpinBoxExpTime_valueChanged()
{
    if (masterImagine != NULL) {
        masterImagine->modified = true;
        masterImagine->ui.btnApply->setEnabled(modified);
    }
}

void Imagine::on_btnOpenStimFile_clicked()
{
    QString stimFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a stimulus file to open",
        "",
        "Stimulus files (*.stim);;All files(*.*)");
    if (stimFilename.isEmpty()) {
        clearStimulus();
        return;
    }

    ui.lineEditStimFile->setText(stimFilename);
    readAndApplyStimulusFile(stimFilename);
}

void Imagine::readAndApplyStimulusFile(QString stimFilename)
{
    QFile file(stimFilename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(stimFilename)
            .arg(file.errorString()));
        return;
    }
    QTextStream in(&file);
    ui.textEditStimFileContent->setPlainText(in.readAll());


    vector<string> lines;
    if (!loadTextFile(stimFilename.toStdString(), lines)){
        //TODO: warn user file open failed.
        return;
    }

    //TODO: remove Comments and handle other fields

    int headerSize;
    QTextStream tt(lines[1].substr(string("header size=").size()).c_str());
    tt >> headerSize;

    int dataStartIdx = headerSize;
    dataAcqThread->stimuli.clear();

    for (int i = dataStartIdx; i < lines.size(); ++i){
        int valve, stack;
        QString line = lines[i].c_str();
        if (line.trimmed().isEmpty()) continue;
        QTextStream tt(&line);
        tt >> valve >> stack;
        dataAcqThread->stimuli.push_back(pair<int, int>(valve, stack));
    }

    //fill in entries in table:
    ui.tableWidgetStimDisplay->setColumnCount(dataAcqThread->stimuli.size());
    QStringList tableHeader;
    for (int i = 0; i < dataAcqThread->stimuli.size(); ++i){
        QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(dataAcqThread->stimuli[i].first));
        ui.tableWidgetStimDisplay->setItem(0, i, newItem);
        tableHeader << QString().setNum(dataAcqThread->stimuli[i].second);
    }//for,
    ui.tableWidgetStimDisplay->setHorizontalHeaderLabels(tableHeader);
    tableHeader.clear();
    tableHeader << "valve";
    ui.tableWidgetStimDisplay->setVerticalHeaderLabels(tableHeader);
}

void Imagine::clearStimulus()
{
    ui.textEditStimFileContent->clear();
    dataAcqThread->stimuli.clear();
    ui.tableWidgetStimDisplay->clear();
}

void Imagine::on_actionStimuli_triggered()
{
    setDockwigetByAction(ui.actionStimuli, ui.dwStim);
}

void Imagine::on_actionConfig_triggered()
{
    setDockwigetByAction(ui.actionConfig, ui.dwCfg);
}

void Imagine::on_actionLog_triggered()
{
    setDockwigetByAction(ui.actionLog, ui.dwLog);
}

void Imagine::on_actionIntensity_triggered()
{
    setDockwigetByAction(ui.actionIntensity, ui.dwIntenCurve);
}

void Imagine::on_actionHistogram_triggered()
{
    setDockwigetByAction(ui.actionHistogram, ui.dwHist);
}

void Imagine::on_btnSelectFile_clicked()
{
    QString imagineFilename = QFileDialog::getSaveFileName(
        this,
        "Choose an Imagine file name to save as",
        "",
        "Imagine files (*.imagine);;All files(*.*)");
    if (imagineFilename.isEmpty()) return;

    imagineFilename = addExtNameIfAbsent(imagineFilename, "imagine");
    ui.lineEditFilename->setText(imagineFilename);
    QFileInfo fi(imagineFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();
}

void Imagine::on_tabWidgetCfg_currentChanged(int index)
{
    static bool hasRead = false;

    if (ui.tabWidgetCfg->currentWidget() == ui.tabPiezo){
        if (!hasRead){
            readPiezoCurPos();
            hasRead = true;
        }
    }
}

void Imagine::on_doubleSpinBoxCurPos_valueChanged(double newValue)
{
    //do nothing for now
}

void Imagine::on_spinBoxSpinboxSteps_valueChanged(int newValue)
{
    QDoubleSpinBox* spinboxes[] = { ui.doubleSpinBoxStartPos, ui.doubleSpinBoxCurPos,
        ui.doubleSpinBoxStopPos };
    for (auto box : spinboxes){
        box->setSingleStep(newValue);
    }
}

void Imagine::on_spinBoxNumOfDecimalDigits_valueChanged(int newValue)
{
    QDoubleSpinBox* spinboxes[] = { ui.doubleSpinBoxStartPos, ui.doubleSpinBoxCurPos,
        ui.doubleSpinBoxStopPos };
    for_each(begin(spinboxes), end(spinboxes), [=](QDoubleSpinBox* box){
        box->setDecimals(newValue); });
}

#define MIN_IDLETIME_BIDIRECTION 0 // 0.04
void Imagine::set_safe_piezo_params()
{
    //check whether settings are within the speed limits of the piezo
    //TODO: also check whether the speed during an imaging scan is within this speed limit
    Positioner *pos = dataAcqThread->pPositioner;
    double cur_travel_back, start, stop;
    double max_speed = pos->maxSpeed();
    if (masterImagine == NULL) {
        start = (double)ui.doubleSpinBoxStartPos->value();
        stop = (double)ui.doubleSpinBoxStopPos->value();
    }
    else {
        start = (double)masterImagine->ui.doubleSpinBoxStartPos->value();
        stop = (double)masterImagine->ui.doubleSpinBoxStopPos->value();
    }

    bool biDirection;
    double min_s_needed = (stop - start) / max_speed;
    double cur_idle_time = (double)ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();
    if (masterImagine == NULL) {
        cur_travel_back = (double)ui.doubleSpinBoxPiezoTravelBackTime->value();
        biDirection = ui.cbBidirectionalImaging->isChecked();
    }
    else {
        cur_travel_back = (double)masterImagine->ui.doubleSpinBoxPiezoTravelBackTime->value();
        biDirection = masterImagine->ui.cbBidirectionalImaging->isChecked();
    }
    if (!biDirection){
        //reset to respect maximum speed limit if necessary
        double safe_idle_time = max(min_s_needed, cur_idle_time);
        double safe_travel_back = max(min_s_needed, cur_travel_back);
        ui.doubleSpinBoxPiezoTravelBackTime->setValue(safe_travel_back);
        ui.doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(safe_idle_time);
    }
    else {
        double safe_idle_time = max(MIN_IDLETIME_BIDIRECTION, cur_idle_time);
        ui.doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(safe_idle_time);
    }
}

void Imagine::on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue)
{
	if (!(ui.cbAutoSetPiezoTravelBackTime->isChecked())) {
		set_safe_piezo_params();
	}
	else{// can probably ditch this
        ui.doubleSpinBoxPiezoTravelBackTime->setValue(newValue / 2); //this may trigger safe piezo params function
		set_safe_piezo_params();
    }

}

void Imagine::on_doubleSpinBoxPiezoTravelBackTime_valueChanged(double newValue)
{
	set_safe_piezo_params();
}

void Imagine::on_doubleSpinBoxStartPos_valueChanged(double newValue)
{
	set_safe_piezo_params();
}

void Imagine::on_doubleSpinBoxStopPos_valueChanged(double newValue)
{
	set_safe_piezo_params();
}

void Imagine::on_cbAutoSetPiezoTravelBackTime_stateChanged(int /* state */)
{
    ui.doubleSpinBoxPiezoTravelBackTime->setEnabled(!ui.cbAutoSetPiezoTravelBackTime->isChecked());

}

void Imagine::on_btnSetCurPosAsStart_clicked()
{
    ui.doubleSpinBoxStartPos->setValue(ui.doubleSpinBoxCurPos->value());
}

void Imagine::on_btnSetCurPosAsStop_clicked()
{
    ui.doubleSpinBoxStopPos->setValue(ui.doubleSpinBoxCurPos->value());
}

void Imagine::on_btnMovePiezo_clicked()
{
    Positioner *pos = dataAcqThread->pPositioner;
    if (pos == NULL) return;
    double um = ui.doubleSpinBoxCurPos->value();
    pos->setDim(ui.comboBoxAxis->currentIndex());

    this->statusBar()->showMessage("Moving the actuator. Please wait ...");

    pos->moveTo(um);

    on_btnRefreshPos_clicked();

    this->statusBar()->showMessage("Moving the actuator. Please wait ... Done");
}

void Imagine::on_btnMoveToStartPos_clicked()
{
    ui.doubleSpinBoxCurPos->setValue(ui.doubleSpinBoxStartPos->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_btnMoveToStopPos_clicked()
{
    ui.doubleSpinBoxCurPos->setValue(ui.doubleSpinBoxStopPos->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_comboBoxAxis_currentIndexChanged(int index)
{
    Positioner *pos = dataAcqThread->pPositioner;
    if (pos == NULL) return;
    int oldDim = pos->getDim();
    pos->setDim(index);
    ui.doubleSpinBoxMinDistance->setValue(pos->minPos());
    ui.doubleSpinBoxMaxDistance->setValue(pos->maxPos());
    on_btnRefreshPos_clicked();

    int tmpIdx = max(oldDim, index);
    if (piezoUiParams.size() <= tmpIdx) piezoUiParams.resize(tmpIdx + 1);

    PiezoUiParam& oldP = piezoUiParams[oldDim];
    PiezoUiParam& newP = piezoUiParams[index];
    oldP.valid = true;
    oldP.start = ui.doubleSpinBoxStartPos->value();
    oldP.stop = ui.doubleSpinBoxStopPos->value();
    oldP.moveto = ui.doubleSpinBoxCurPos->value();
    if (newP.valid){
        ui.doubleSpinBoxStartPos->setValue(newP.start);
        ui.doubleSpinBoxStopPos->setValue(newP.stop);
        ui.doubleSpinBoxCurPos->setValue(newP.moveto);
    }
}

void Imagine::on_btnRefreshPos_clicked()
{
    readPiezoCurPos();
}

void Imagine::on_btnFastIncPosAndMove_clicked()
{
    ui.doubleSpinBoxCurPos->stepBy(10 * ui.spinBoxSpinboxSteps->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_btnIncPosAndMove_clicked()
{
    ui.doubleSpinBoxCurPos->stepBy(ui.spinBoxSpinboxSteps->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_btnDecPosAndMove_clicked()
{
    ui.doubleSpinBoxCurPos->stepBy(-1 * ui.spinBoxSpinboxSteps->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_btnFastDecPosAndMove_clicked()
{
    ui.doubleSpinBoxCurPos->stepBy(-10 * ui.spinBoxSpinboxSteps->value());
    on_btnMovePiezo_clicked();
}

void Imagine::on_actionExit_triggered()
{
    close();
}

/*
void Imagine::on_actionHeatsinkFan_triggered()
{
    if (!fanCtrlDialog){
        fanCtrlDialog = new FanCtrlDialog(this);
    }
    fanCtrlDialog->exec();
}
*/

// -----------------------------------------------------------------------------
// Laser control
// -----------------------------------------------------------------------------
void Imagine::displayShutterStatus(int status)
{
    for (int i = 1; i <= 4; i++) {
        QCheckBox *cb = ui.groupBoxLaser->findChild<QCheckBox *>(QString("cbLine%1").arg(i));
        cb->setChecked(status % 2);
        status /= 2;
    }
}

void Imagine::displayTransStatus(bool isAotf, int line, int status)
{
    QString prefix = isAotf ? "aotf" : "wheel";
    QSlider *slider = ui.groupBoxLaser->findChild<QSlider *>(QString("%1Line%2").arg(prefix).arg(line));
    slider->setValue(status);
    slider->setToolTip(QString::number(status / 10.0));
}


void Imagine::displayLaserGUI(int numLines, int *laserIndex, int *wavelength)
{
    numLaserShutters = numLines;

    for (int i = 1; i <= numLines; i++) {
        laserShutterIndex[i - 1] = laserIndex[i - 1];
        QCheckBox *checkBox = ui.groupBoxLaser->findChild<QCheckBox *>(QString("cbLine%1").arg(i));
        QString wl= QString("%1 nm").arg(wavelength[i-1]);
        if(checkBox) checkBox->setText(wl);
    }
    for (int i = numLines + 1; i <= 8; i++) {
        QCheckBox *checkBox = ui.groupBoxLaser->findChild<QCheckBox *>(QString("cbLine%1").arg(i));
        QSlider *slider = ui.groupBoxLaser->findChild<QSlider *>(QString("aotfLine%1").arg(i));
        QDoubleSpinBox *spinBox = ui.groupBoxLaser->findChild<QDoubleSpinBox *>(QString("doubleSpinBox_aotfLine%1").arg(i));
        if(checkBox) checkBox->setVisible(false);
        if(slider) slider->setVisible(false);
        if(spinBox) spinBox->setVisible(false);
    }
}

void Imagine::on_btnOpenPort_clicked()
{
    QString portName;
    if (laserCtrlSerial) {
        portName = laserCtrlSerial->getPortName();
        emit openLaserSerialPort(portName);
        emit getLaserShutterStatus();
        for (int i = 1; i <= 4; i++) {
	        emit getLaserTransStatus(true, i);
        }
        QString str = QString("%1 port is opened").arg(portName);
        appendLog(str);
    }
    else {
        QString str = QString("laserCtrlSerial object is not defined");
        appendLog(str);
    }

    ui.btnOpenPort->setEnabled(false);
    ui.btnClosePort->setEnabled(true);
    if (ui.groupBoxLaser->isChecked()) {
        ui.actionOpenShutter->setEnabled(false);
        ui.actionCloseShutter->setEnabled(false);
    }
    else {
        ui.actionOpenShutter->setEnabled(true);
        ui.actionCloseShutter->setEnabled(false);
    }
    ui.groupBoxLaser->setEnabled(true);

    qDebug() << "Open laser control port " ;
}

void Imagine::on_btnClosePort_clicked()
{
    QString portName;
    if (laserCtrlSerial) {
        portName = laserCtrlSerial->getPortName();
        emit closeLaserSerialPort();
        QString str = QString("%1 port is closed").arg(portName);
        appendLog(str);
    }
    else {
        QString str = QString("laserCtrlSerial object is not defined");
        appendLog(str);
    }

    ui.btnOpenPort->setEnabled(true);
    ui.btnClosePort->setEnabled(false);
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);
    ui.groupBoxLaser->setEnabled(false);
}


void Imagine::changeLaserShutters(void)
{
    if (laserCtrlSerial) {
        bitset<8> bs(0);
        for (int i = 1; i <= numLaserShutters; i++) {
            QCheckBox *checkBox = ui.groupBoxLaser->findChild<QCheckBox *>(QString("cbLine%1").arg(i));
            bs.set(laserShutterIndex[i - 1], checkBox->isChecked());
        }
        int status = bs.to_ulong();
        emit setLaserShutters(status);
    }
    else {
        QString str = QString("laserCtrlSerial object is not defined");
        appendLog(str);
    }
}

void Imagine::changeLaserTrans(bool isAotf, int line)
{
    QString prefix = isAotf ? "aotf" : "wheel";
    QSlider *slider = ui.groupBoxLaser->findChild<QSlider *>(QString("%1Line%2").arg(prefix).arg(line));
    slider->setToolTip(QString::number(slider->value() / 10.0));

    if (laserCtrlSerial) {
        QString str;
        if (isAotf) {
            emit setLaserTrans(true, line, slider->value());
            str = QString("Set laser AOTF value as %1").arg(slider->value());
        }
        else{
            emit setLaserTrans(false, line, slider->value());
            str = QString("Set laser ND wheel value as %1").arg(slider->value());
        }
        appendLog(str);
    }
    else {
        QString str = QString("laserCtrlSerial object is not defined");
        appendLog(str);
    }
}

void Imagine::on_groupBoxLaser_clicked(bool checked)
{
    if (checked) {
        on_actionOpenShutter_triggered();
        ui.actionOpenShutter->setEnabled(false);
        ui.actionCloseShutter->setEnabled(false);
        QString str = QString("Laser setup mode");
        appendLog(str);
    }
    else {
        ui.actionOpenShutter->setEnabled(false);
        ui.actionCloseShutter->setEnabled(true);
        QString str = QString("Exit laser setup mode");
        appendLog(str);
    }
}

void Imagine::on_cbLine1_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 1");
    else
        str = QString("Close laser shutter 1");
    appendLog(str);
}

void Imagine::on_cbLine2_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 2");
    else
        str = QString("Close laser shutter 2");
    appendLog(str);
}

void Imagine::on_cbLine3_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 3");
    else
        str = QString("Close laser shutter 3");
    appendLog(str);
}

void Imagine::on_cbLine4_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 4");
    else
        str = QString("Close laser shutter 4");
    appendLog(str);
}


void Imagine::on_cbLine5_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 5");
    else
        str = QString("Close laser shutter 5");
    appendLog(str);
}

void Imagine::on_cbLine6_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 6");
    else
        str = QString("Close laser shutter 6");
    appendLog(str);
}

void Imagine::on_cbLine7_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 7");
    else
        str = QString("Close laser shutter 7");
    appendLog(str);
}

void Imagine::on_cbLine8_clicked(bool checked)
{
    QString str;
    changeLaserShutters();
    if (checked)
        str = QString("Open laser shutter 8");
    else
        str = QString("Close laser shutter 8");
    appendLog(str);
}

void Imagine::on_aotfLine1_valueChanged()
{
    ui.doubleSpinBox_aotfLine1->setValue((double)(ui.aotfLine1->value()) / 10.);
//    changeLaserTrans(true, 1);
}

void Imagine::on_aotfLine2_valueChanged()
{
    ui.doubleSpinBox_aotfLine2->setValue((double)(ui.aotfLine2->value()) / 10.);
//    changeLaserTrans(true, 2);
}

void Imagine::on_aotfLine3_valueChanged()
{
    ui.doubleSpinBox_aotfLine3->setValue((double)(ui.aotfLine3->value()) / 10.);
//    changeLaserTrans(true, 3);
}

void Imagine::on_aotfLine4_valueChanged()
{
    ui.doubleSpinBox_aotfLine4->setValue((double)(ui.aotfLine4->value()) / 10.);
//    changeLaserTrans(true, 4);
}

void Imagine::on_aotfLine5_valueChanged()
{
    ui.doubleSpinBox_aotfLine5->setValue((double)(ui.aotfLine5->value()) / 10.);
//    changeLaserTrans(true, 5);
}

void Imagine::on_aotfLine6_valueChanged()
{
    ui.doubleSpinBox_aotfLine6->setValue((double)(ui.aotfLine6->value()) / 10.);
//    changeLaserTrans(true, 6);
}

void Imagine::on_aotfLine7_valueChanged()
{
    ui.doubleSpinBox_aotfLine7->setValue((double)(ui.aotfLine7->value()) / 10.);
//    changeLaserTrans(true, 7);
}

void Imagine::on_aotfLine8_valueChanged()
{
    ui.doubleSpinBox_aotfLine8->setValue((double)(ui.aotfLine8->value()) / 10.);
//    changeLaserTrans(true, 8);
}

void Imagine::on_doubleSpinBox_aotfLine1_valueChanged()
{
    ui.aotfLine1->setValue((int)(ui.doubleSpinBox_aotfLine1->value()*10.0));
    changeLaserTrans(true, 1);
}

void Imagine::on_doubleSpinBox_aotfLine2_valueChanged()
{
    ui.aotfLine2->setValue((int)(ui.doubleSpinBox_aotfLine2->value()*10.0));
    changeLaserTrans(true, 2);
}

void Imagine::on_doubleSpinBox_aotfLine3_valueChanged()
{
    ui.aotfLine3->setValue((int)(ui.doubleSpinBox_aotfLine3->value()*10.0));
    changeLaserTrans(true, 3);
}

void Imagine::on_doubleSpinBox_aotfLine4_valueChanged()
{
    ui.aotfLine4->setValue((int)(ui.doubleSpinBox_aotfLine4->value()*10.0));
    changeLaserTrans(true, 4);
}

void Imagine::on_doubleSpinBox_aotfLine5_valueChanged()
{
    ui.aotfLine5->setValue((int)(ui.doubleSpinBox_aotfLine5->value()*10.0));
    changeLaserTrans(true, 5);
}

void Imagine::on_doubleSpinBox_aotfLine6_valueChanged()
{
    ui.aotfLine6->setValue((int)(ui.doubleSpinBox_aotfLine6->value()*10.0));
    changeLaserTrans(true, 6);
}

void Imagine::on_doubleSpinBox_aotfLine7_valueChanged()
{
    ui.aotfLine7->setValue((int)(ui.doubleSpinBox_aotfLine7->value()*10.0));
    changeLaserTrans(true, 7);
}

void Imagine::on_doubleSpinBox_aotfLine8_valueChanged()
{
    ui.aotfLine8->setValue((int)(ui.doubleSpinBox_aotfLine8->value()*10.0));
    changeLaserTrans(true, 8);
}

void Imagine::changeLaserShuttersTTL(int line, bool onOff)
{
    QString str;
    int lineNum = line + 7;
    bool returnVal = digOut->singleOut(lineNum, onOff); // P0.8~P0.12
    if (onOff)
        laserTTLSig |= (0x01 << lineNum);
    else
        laserTTLSig &= ~(0x01 << lineNum);

    if (onOff)
        str = QString("Open laser shutter 1 with TTL pulse");
    else
        str = QString("Close laser shutter 1 with TTL pulse");
    appendLog(str);
}

void Imagine::on_cbLineTTL1_clicked(bool checked)
{
    changeLaserShuttersTTL(3, checked);
}

void Imagine::on_cbLineTTL2_clicked(bool checked)
{
    changeLaserShuttersTTL(5, checked);
//    bool returnVal = digOut->singleOut(14, checked); // P0.8~P0.12
}

void Imagine::on_cbLineTTL3_clicked(bool checked)
{
    changeLaserShuttersTTL(1, checked);
}

void Imagine::on_cbLineTTL4_clicked(bool checked)
{
    changeLaserShuttersTTL(2, checked);
}

void Imagine::on_cbLineTTL5_clicked(bool checked)
{
    changeLaserShuttersTTL(4, checked);
}

void Imagine::on_cbLineTTL6_clicked(bool checked)
{
    changeLaserShuttersTTL(8, checked);
}

void Imagine::on_cbLineTTL7_clicked(bool checked)
{
    changeLaserShuttersTTL(9, checked);
}

void Imagine::on_cbLineTTL8_clicked(bool checked)
{
    changeLaserShuttersTTL(10, checked);
}

void Imagine::on_cbDualOutSW1_clicked(bool checked)
{
    changeLaserShuttersTTL(6, checked);
}

void Imagine::on_cbDualOutSW2_clicked(bool checked)
{
    changeLaserShuttersTTL(7, checked);
}


// -----------------------------------------------------------------------------
// Load and save configuration
// -----------------------------------------------------------------------------
void Imagine::writeSettings(QString file)
{
    QSettings prefs(file, QSettings::IniFormat, this);

    prefs.beginGroup("General");
    prefs.setValue("m_OpenDialogLastDirectory", m_OpenDialogLastDirectory);
    WRITE_STRING_SETTING(prefs, lineEditFilename);
    prefs.endGroup();

    prefs.beginGroup("Camera");
    WRITE_CHECKBOX_SETTING(prefs, cbBothCamera);
    WRITE_SETTING(prefs, spinBoxNumOfStacks);
    WRITE_SETTING(prefs, spinBoxFramesPerStack);
    WRITE_SETTING(prefs, doubleSpinBoxExpTime);
    WRITE_SETTING(prefs, doubleSpinBoxBoxIdleTimeBtwnStacks);
//    WRITE_COMBO_SETTING(prefs, comboBoxAcqTriggerMode);
    WRITE_COMBO_SETTING(prefs, comboBoxExpTriggerMode);
    WRITE_SETTING(prefs, spinBoxAngle);
    WRITE_SETTING(prefs, sbObjectiveLens);
    WRITE_SETTING(prefs, doubleSpinBoxUmPerPxlXy);
    WRITE_COMBO_SETTING(prefs, comboBoxHorReadoutRate);
    WRITE_COMBO_SETTING(prefs, comboBoxPreAmpGains);
    WRITE_SETTING(prefs, spinBoxGain);
    WRITE_COMBO_SETTING(prefs, comboBoxVertShiftSpeed);
    WRITE_COMBO_SETTING(prefs, comboBoxVertClockVolAmp);
    WRITE_BOOL_SETTING(prefs, isUsingSoftROI);
    WRITE_SETTING(prefs, spinBoxHstart);
    WRITE_SETTING(prefs, spinBoxHend);
    WRITE_SETTING(prefs, spinBoxVstart);
    WRITE_SETTING(prefs, spinBoxVend);
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Positioner");
        WRITE_COMBO_SETTING(prefs, comboBoxAxis);
        WRITE_SETTING(prefs, doubleSpinBoxMinDistance);
        WRITE_SETTING(prefs, doubleSpinBoxMaxDistance);
        WRITE_SETTING(prefs, doubleSpinBoxStartPos);
        WRITE_SETTING(prefs, doubleSpinBoxCurPos);
        WRITE_SETTING(prefs, doubleSpinBoxStopPos);
        WRITE_SETTING(prefs, spinBoxSpinboxSteps);
        WRITE_SETTING(prefs, spinBoxNumOfDecimalDigits);
        WRITE_SETTING(prefs, doubleSpinBoxPiezoTravelBackTime);
        WRITE_CHECKBOX_SETTING(prefs, cbAutoSetPiezoTravelBackTime);
        WRITE_CHECKBOX_SETTING(prefs, cbBidirectionalImaging);
        prefs.endGroup();

        prefs.beginGroup("Stimuli");
        WRITE_CHECKBOX_SETTING(prefs, cbStim);
        WRITE_STRING_SETTING(prefs, lineEditStimFile);
        prefs.endGroup();
    }

    prefs.beginGroup("Display");
    WRITE_SETTING(prefs, spinBoxDisplayAreaSize);
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Waveform");
        WRITE_CHECKBOX_SETTING(prefs, cbWaveformEnable);
//        WRITE_COMBO_SETTING(prefs, comboBoxExpTriggerModeWav);
//        WRITE_SETTING(prefs, doubleSpinBoxExpTimeWav);
        WRITE_STRING_SETTING(prefs, lineEditConWaveFile);
        prefs.endGroup();

        prefs.beginGroup("Laser");
        WRITE_CHECKBOX_SETTING(prefs, cbLineTTL1);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine1);
        WRITE_CHECKBOX_SETTING(prefs, cbLineTTL2);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine2);
        WRITE_CHECKBOX_SETTING(prefs, cbLineTTL3);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine3);
        WRITE_CHECKBOX_SETTING(prefs, cbLineTTL4);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine4);
        WRITE_CHECKBOX_SETTING(prefs, cbLineTTL5);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine5);
        prefs.endGroup();
    }
}

void Imagine::readSettings(QString file)
{
    QString val;
    bool ok;
    qint32 i;
    double d;

    QSettings prefs(file, QSettings::IniFormat, this);

    prefs.beginGroup("General");
    prefs.setValue("m_OpenDialogLastDirectory", m_OpenDialogLastDirectory);
    val = prefs.value("m_OpenDialogLastDirectory").toString();
    if (val == "")
        m_OpenDialogLastDirectory = QDir::homePath();
    else
        m_OpenDialogLastDirectory = val;
    READ_STRING_SETTING(prefs, lineEditFilename, "");
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Positioner");
        READ_COMBO_SETTING(prefs, comboBoxAxis, 0);
        READ_SETTING(prefs, doubleSpinBoxMinDistance, ok, d, 0., Double);
        READ_SETTING(prefs, doubleSpinBoxMaxDistance, ok, d, 400.0, Double);
        READ_SETTING(prefs, doubleSpinBoxStartPos, ok, d, 1.0, Double);
        READ_SETTING(prefs, doubleSpinBoxCurPos, ok, d, 200.0, Double);
        READ_SETTING(prefs, doubleSpinBoxStopPos, ok, d, 400.0, Double);
        READ_SETTING(prefs, spinBoxSpinboxSteps, ok, i, 10, Int);
        READ_SETTING(prefs, spinBoxNumOfDecimalDigits, ok, i, 0, Int);
        READ_SETTING(prefs, doubleSpinBoxPiezoTravelBackTime, ok, d, 0., Double);
        READ_CHECKBOX_SETTING(prefs, cbAutoSetPiezoTravelBackTime, false); // READ_CHECKBOX_SETTING
        READ_CHECKBOX_SETTING(prefs, cbBidirectionalImaging, false); // READ_CHECKBOX_SETTING
        READ_COMBO_SETTING(prefs, comboBoxPositionerOwner, 0);
        prefs.endGroup();

        prefs.beginGroup("Stimuli");
        READ_CHECKBOX_SETTING(prefs, cbStim, false); // READ_CHECKBOX_SETTING
        READ_STRING_SETTING(prefs, lineEditStimFile, "");
        prefs.endGroup();
    }

    prefs.beginGroup("Camera");
    READ_CHECKBOX_SETTING(prefs, cbBothCamera, false);
    READ_SETTING(prefs, spinBoxNumOfStacks, ok, i, 3, Int);
    READ_SETTING(prefs, spinBoxFramesPerStack, ok, i, 100, Int);
    READ_SETTING(prefs, doubleSpinBoxExpTime, ok, d, 0.0107, Double);
    READ_SETTING(prefs, doubleSpinBoxBoxIdleTimeBtwnStacks, ok, d, 0.700, Double); // this should be loaded after doubleSpinBoxPiezoTravelBackTime
    //    READ_COMBO_SETTING(prefs, comboBoxAcqTriggerMode, 1);
    READ_COMBO_SETTING(prefs, comboBoxExpTriggerMode, 0);
    READ_SETTING(prefs, spinBoxAngle, ok, i, 0, Int);
    READ_SETTING(prefs, sbObjectiveLens, ok, i, 20, Int);
    READ_SETTING(prefs, doubleSpinBoxUmPerPxlXy, ok, d, -1.0000, Double);
    READ_COMBO_SETTING(prefs, comboBoxHorReadoutRate, 0);
    READ_COMBO_SETTING(prefs, comboBoxPreAmpGains, 0);
    READ_SETTING(prefs, spinBoxGain, ok, i, 0, Int);
    READ_COMBO_SETTING(prefs, comboBoxVertShiftSpeed, 0);
    READ_COMBO_SETTING(prefs, comboBoxVertClockVolAmp, 0);
    READ_BOOL_SETTING(prefs, isUsingSoftROI, false);
    READ_SETTING(prefs, spinBoxHstart, ok, i, 1, Int);
    READ_SETTING(prefs, spinBoxHend, ok, i, maxROIHSize, Int);
    READ_SETTING(prefs, spinBoxVstart, ok, i, 1, Int);
    READ_SETTING(prefs, spinBoxVend, ok, i, maxROIVSize, Int);
    prefs.endGroup();

    prefs.beginGroup("Display");
    READ_SETTING(prefs, spinBoxDisplayAreaSize, ok, i, 0, Int);
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Waveform");
        READ_CHECKBOX_SETTING(prefs, cbWaveformEnable, false);
//        READ_COMBO_SETTING(prefs, comboBoxExpTriggerModeWav, 0);
//        READ_SETTING(prefs, doubleSpinBoxExpTimeWav, ok, d, 0.0107, Double);
        READ_STRING_SETTING(prefs, lineEditConWaveFile, "");
        prefs.endGroup();

        prefs.beginGroup("Laser");
        READ_CHECKBOX_SETTING(prefs, cbLineTTL1, true); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine1, ok, d, 50.0, Double);
        READ_CHECKBOX_SETTING(prefs, cbLineTTL2, true); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine2, ok, d, 50.0, Double);
        READ_CHECKBOX_SETTING(prefs, cbLineTTL3, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine3, ok, d, 50.0, Double);
        READ_CHECKBOX_SETTING(prefs, cbLineTTL4, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine4, ok, d, 50.0, Double);
        READ_CHECKBOX_SETTING(prefs, cbLineTTL5, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine5, ok, d, 50.0, Double);
        prefs.endGroup();
    }

    if (masterImagine == NULL) {
        // stimulus file read
        QString stimulusFile = ui.lineEditStimFile->text();
        if (stimulusFile.isEmpty())
            clearStimulus();
        else
            readAndApplyStimulusFile(stimulusFile);

        // waveform file read
        QString conFileName = ui.lineEditConWaveFile->text();
        if (conFileName != "")
            readControlWaveformFile(conFileName);
        if (!ui.cbWaveformEnable->isChecked())
            on_cbWaveformEnable_clicked(false);

        // laser apply
        ui.groupBoxLaser->setChecked(false);
        on_actionCloseShutter_triggered();
        for (int i = 1; i <= 8; i++) changeLaserTrans(true, i);
        //changeLaserShutters();
        on_cbLineTTL1_clicked(ui.cbLineTTL1->isChecked());
        on_cbLineTTL2_clicked(ui.cbLineTTL2->isChecked());
        on_cbLineTTL3_clicked(ui.cbLineTTL3->isChecked());
        on_cbLineTTL4_clicked(ui.cbLineTTL4->isChecked());
        on_cbLineTTL5_clicked(ui.cbLineTTL5->isChecked());
    }
//    if ((rig == "ocpi-1") || (rig == "ocpi-lsk"))
//        ui.cbBothCamera->setChecked(false);
}

void Imagine::writeComments(QString file)
{
    QFile fp(file);
    if (!fp.open(QIODevice::ReadWrite)) {
        qDebug("File open error\n");
    }
    else {
        QTextStream stream(&fp);
        stream.readAll();
        stream << "\n[Comment]\n" << ui.textEditComment->toPlainText();
        stream.flush();
        fp.close();
    }
}

void Imagine::readComments(QString file)
{
    QFile fp(file);
    if (!fp.open(QIODevice::ReadOnly)) {
        qDebug("File open error\n");
        return;
    }
    else {
        QTextStream in(&fp);
        QString line;
        bool enableDisplay = false;
        do {
            line = in.readLine();
            if (line.contains("[Comment]", Qt::CaseSensitive)) {
                enableDisplay = true;
                break;
            }
        } while (!line.isNull());
        ui.textEditComment->setText(in.readAll());

        fp.close();
    }
}

void Imagine::on_actionSave_Configuration_triggered()
{
    QString proposedFile;
    if (masterImagine == NULL) {
        if (file == "")
            proposedFile = m_OpenDialogLastDirectory + QDir::separator() + "OCPI_cfg1.txt";
        else
            proposedFile = file;
        file = QFileDialog::getSaveFileName(this, tr("Save OCPI Imagine(1) Configuration"), proposedFile, tr("*.txt"));
    }
    else {
        if (file == "")
            proposedFile = m_OpenDialogLastDirectory + QDir::separator() + "OCPI_cfg2.txt";
        else
            proposedFile = file;
        file = QFileDialog::getSaveFileName(this, tr("Save OCPI Imagine(2) Configuration"), proposedFile, tr("*.txt"));
    }
    if (true == file.isEmpty()) { return; }
    QFileInfo fi(file);
    m_OpenDialogLastDirectory = fi.absolutePath();

    writeSettings(file);
    writeComments(file);
}

void Imagine::on_actionLoad_Configuration_triggered()
{
    file = QFileDialog::getOpenFileName(this, tr("Select Configuration File"),
        m_OpenDialogLastDirectory,
        tr("Configuration File (*.txt)"));
    if (true == file.isEmpty()) { return; }
    QFileInfo fi(file);
    m_OpenDialogLastDirectory = fi.absolutePath();

    readSettings(file);
    readComments(file);
    on_btnApply_clicked();
}

void Imagine::displayPiezoCtrlStatus(QByteArray rx)
{
    bool ok;
    bitset<16> bs(rx.mid(5, rx.size() - 7).toInt(&ok, 10)); // ex) rx = "stat,8389\r\n"
    vector <int> status(9);
    status[0] = bs[0];                          // 0: acturator not plugged, 1: acturator plugged
    status[1] = bs[1] + bs[2] * 2;              // 0: no measuring system,
                                                // 1: strain gauge measuring system,
                                                // 2: capacitive measuring system
    status[2] = bs[4];                          // 0: closed loop system, 1: open loop system
    status[3] = bs[6];                          // 0: piezo voltage not enabled, 1: piezo voltage enabled
    status[4] = bs[7];                          // 0: open loop, 1: closed loop
    status[5] = bs[9] + bs[10] * 2 + bs[11] * 4;// 0: generator off,
                                                // 1: sine on
                                                // 2: triangle on
                                                // 3: rectangle on
                                                // 4: noise on
                                                // 5: sweep on
    status[6] = bs[12];                         // 0: notch filter off, 1: notch filter on
    status[7] = bs[13];                         // 0: low pass filter off, 1: low pass filter on
    status[8] = bs[15];                         // 0: fan off, 1: fan on

    vector <vector<QString>> statMsg = {
        {   "Actuator not plugged",
            "Actuator plugged"
        },
        {   "No measuring system",
            "Strain gauge Measuring system",
            "capacitive measuring system",
            "undefined"
        },
        {   "Closed loop system",
            "Open loop system"
        },
        {   "piezo voltage not enabled",
            "piezo voltage enabled"
        },
        {   "Open loop",
            "Closed loop"
        },
        {   "Generator off",
            "Generator sine on",
            "Generator triangle on",
            "Generator rectangle on",
            "Generator noise on",
            "Generator sweep on",
            "undefined",
            "undefined"
        },
        {   "Notch filter off",
            "Notch filter on"
        },
        {   "Low pass filter off",
            "Low pass filter on"
        },
        {   "Fan off",
            "Fan on"
        }
    };
    for (int i = 0; i < statMsg.size(); i++) {
        appendLog(statMsg[i][status[i]]);
    }
}

void Imagine::displayPiezoCtrlValue(QByteArray rx)
{
    appendLog(rx);
}

void Imagine::on_btnSendSerialCmd_clicked()
{
    std::vector<QString> helptbl= {
        "dprpon : switch on the cyclic output of the current actuator position value",
        "dprpof : switch off the cyclic output of the current actuator position value",
        "dprson : switch on the automatic output of the status register",
        "dprsof : switch off the automatic output of the status register",
        "s : shows all available commands request",
        "stat : request content of status register"
        "mess : position value request [um or mrad]",
        "ktemp : amplifier temperature value [degree Celsius]",
        "rohm : operation time of actuator since shipping [minutes]",
        "rgver : version number of loop-controller request",
        "fan : <cmd>,<value> switches the fan on/off: 0=off,1=on",
        "sr : <cmd>,<value> slew rate: 0.0000002 to 500.0[V/ms]",
        "modon : <cmd>,<value> modulation input(MOD plug): 0=off,1=on",
        "monsrc : <cmd>,<value> monitor output: 0=pos.CL,1=cmd value,2=ctrl out,3=CL devi.,4=ACL devi.,5=actuator vol.,6=pos.OL",
        "cl : <cmd>,<value> loop: 0=open,1=closed",
        "kp : <cmd>,<value> proportional term: 0 to 999.0",
        "ki : <cmd>,<value> integral term: 0 to 999.0",
        "kd : <cmd>,<value> differential term: 0 to 999.0",
        "sstd : set default values",
        "notchon : <cmd>,<value> notch filter: 0=off,1=on",
        "notchf : <cmd>,<value> notch filter frequency: 0 to 20000 [Hz]",
        "notchb : <cmd>,<value> notch filter bandwidth (-3dB): 0 to 20000 (max. 2 * notch_fr) [Hz]",
        "lpon : <cmd>,<value> low pass filter: 0=off,1=on",
        "lpf : <cmd>,<value> low pass cut frequency: 1 to 20000[Hz]",
        "gfkt : <cmd>,<value> internal function generator: 0=off,1=sine,2=triangle,3=rectangle,4=noise,5=sweep",
        "sct : <cmd>,<value> scan type: 0=off,1=sine,2=triangle",
        "ss : <cmd>,<value> start scan: 0=complete,1=start,2=running",
        "trgedge : <cmd>,<value> trigger generation edge: 0=off,1=rising,2=falling,3=both",
        "help : help <command>, ex) help dprpon"
    };

    if (ui.lineEditSerialCmd->text().left(4) == "help"){
        bool found = 0;
        QString cmd = ui.lineEditSerialCmd->text().mid(5).append(" ");
        for (int i = 0; i < helptbl.size(); i++) {
            if (cmd == helptbl[i].left(cmd.size())) {
                appendLog(helptbl[i]);
                found = 1;
                break;
            }
        }
        if (!found)
            appendLog(helptbl.back());
    }
    else
        emit sendPiezoCtrlCmd(ui.lineEditSerialCmd->text());
}


void Imagine::on_btnPzOpenPort_clicked()
{
    QString portName;
    if (piezoCtrlSerial) {
        portName = piezoCtrlSerial->getPortName();
        emit openPiezoCtrlSerialPort(portName);
        QString str = QString("%1 port for piezo controller is opened").arg(portName);
        appendLog(str);
    }
    else {
        QString str = QString("piezoCtrlSerial object is not defined");
        appendLog(str);
    }

    ui.btnPzOpenPort->setEnabled(false);
    ui.btnPzClosePort->setEnabled(true);
    ui.labelSerialCommand->setEnabled(true);
    ui.lineEditSerialCmd->setEnabled(true);
    ui.btnSendSerialCmd->setEnabled(true);

    qDebug() << "Open piezo controller port ";
}

void Imagine::on_btnPzClosePort_clicked()
{
    QString portName;
    if (piezoCtrlSerial) {
        portName = piezoCtrlSerial->getPortName();
        emit closePiezoCtrlSerialPort();
        QString str = QString("%1 port for piezo controller is closed").arg(portName);
        appendLog(str);
    }
    else {
        QString str = QString("piezoCtrlSerial object is not defined");
        appendLog(str);
    }

    ui.btnPzOpenPort->setEnabled(true);
    ui.btnPzClosePort->setEnabled(false);
    ui.labelSerialCommand->setEnabled(false);
    ui.lineEditSerialCmd->setEnabled(false);
    ui.btnSendSerialCmd->setEnabled(false);
}

/**************************** AI and DI read ***********************/
#define MIN_Y   -50
bool seekSection(QString section, QTextStream &stream)
{
    bool found = false;
    QString line;
    stream.seek(0);
    do {
        line = stream.readLine();
        if (line.contains(section, Qt::CaseSensitive)) {
            found = true;
            break;
        }
    } while (!line.isNull());
    return found;
}

bool seekField(QString section, QString key, QTextStream &stream, QString &container)
{
    bool found = false;
    QString line;
    if (!seekSection(section, stream))
        return false;
    do {
        line = stream.readLine();
        if (line.contains(QString(key).append("="), Qt::CaseSensitive)) {
            container = line.remove(0, QString(key).size() + 1);
            found = true;
            break;
        }
    } while (!line.isNull());
    return found;
}

void Imagine::on_btnReadAiWavOpen_clicked()
{
    QString imagineFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a Imagine file to open",
        m_OpenDialogLastDirectory, 
        "Imagine files (*.imagine);;All files(*.*)");
    if (imagineFilename.isEmpty()) return;

    ui.lineEditReadWaveformFile->setText(imagineFilename);
    QFileInfo fi(imagineFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();

    QFile file(imagineFilename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(imagineFilename)
            .arg(file.errorString()));
        return;
    }

    for (int i = 0; i < numAiCurve + numDiCurve; i++)
        inCurve[i]->setData(0);
    // read ai filename and di filename
    // read channel number and signal names
    QTextStream in(&file);
    QString aiFilename, diFilename="NA";
    QString container;
    QVector<QString> aiName, diName;
    QVector<int> diChNumList;
    int diChBase;
    bool valid, preSection = false, aiSection = false, diSection = false;
    numAiCurveData = 0;
    numDiCurveData = 0;
    if (seekField("[general]", "rig", in, container)) {
        ui.lableAiDiWavRigname->setText(container);
    }
    if (seekField("[ai]", "scan rate", in, container)) {
        ui.labelAiDiSampleRate->setText(container);
    }
    if (seekField("[misc params]", "ai data file", in, container)) {
        if (container != "NA")
            aiFilename = m_OpenDialogLastDirectory + '/' + container.section("/", -1, -1);
        else
            aiFilename = "NA";
    }
    if (seekField("[misc params]", "di data file", in, container)) {
        if (container != "NA")
            diFilename = m_OpenDialogLastDirectory + '/' + container.section("/", -1, -1);
        else
            diFilename = "NA";
    }
    if (seekField("[ai]", "label list", in, container)) {
        numAiCurveData = container.count("$")+1;
        for (int i = 0; i < numAiCurveData; i++)
            aiName.push_back(container.section("$", i, i));
    }
    if (seekField("[di]", "di channel list base", in, container)) {
        diChBase = container.toInt();
    }
    QVector<QString> list;
    if (seekField("[di]", "di label list", in, container)) {
        numDiCurveData = container.count("$") + 1;
        for (int i = 0; i < numDiCurveData; i++) {
            list.push_back(container.section("$", i, i));
        }
    }
    if (seekField("[di]", "di channel list", in, container)) {
        numDiCurveData = container.count(" ") + 1;
        for (int i = 0; i < numDiCurveData; i++) {
            int chList = container.section(" ", i, i).toInt();
            if (i == 0) diChBase = chList;
            if (list[i] != "unused") {
                diName.push_back(list[i]);
                diChNumList.push_back(chList - diChBase);
            }
        }
        numDiCurveData = diChNumList.size();
    }
    file.close();
    QByteArray data;
    /*
    // read ai file
    if (aiFilename != "NA") {
        file.setFileName(aiFilename);
        if (!file.open(QFile::ReadOnly)) {// if we are to use iostream, QIODevice
            QMessageBox::warning(this, tr("Imagine"),
                tr("Cannot read file %1:\n%2.")
                .arg(aiFilename)
                .arg(file.errorString()));
            numAiCurveData = 0;
        }
        else {
            //data.resize(1000000);
            //QDataStream in(&file);
            //in.readRawData(data.data(), 1000000);// skipRawData()
            data = file.readAll();
            file.close();
        }
    }
    */
    if (aiFilename != "NA") {
        if (aiWaveData)
            delete aiWaveData;
        aiWaveData = new AiWaveform(aiFilename, numAiCurveData); // load data to aiWaveData
        if (aiWaveData->getErrorMsg() != "") {
            QMessageBox::warning(this, "Imagine", aiWaveData->getErrorMsg());
            numAiCurveData = 0;
        }
    }

    // read di file
    if (diFilename != "NA") {
        if (diWaveData)
            delete diWaveData;
        diWaveData = new DiWaveform(diFilename, diChNumList); // load data to diWaveData
        if (diWaveData->getErrorMsg() != "") {
            QMessageBox::warning(this, "Imagine", diWaveData->getErrorMsg());
            numDiCurveData = 0;
        }
    }

    int err = NO_CF_ERROR; // for later use
    if (err == NO_CF_ERROR) {
        int aiSampleNum = aiWaveData->totalSampleNum;
        int diSampleNum = diWaveData->totalSampleNum;
        if (aiSampleNum && diSampleNum)
            dsplyTotalSampleNum = min(aiSampleNum, diSampleNum);
        else if (aiSampleNum)
            dsplyTotalSampleNum = aiSampleNum;
        else
            dsplyTotalSampleNum = diSampleNum;
        ui.labelAiSampleNum->setText(QString("%1").arg(dsplyTotalSampleNum));

        // Waveform selection combobox setup
        QStringList aiList, diList;
        aiList.push_back("empty");
        diList.push_back("empty");
        for (int i = 0; i < numAiCurveData; i++) {
            aiList.push_back(aiName[i]);
        }
        for (int i = 0; i < numDiCurveData; i++) {
            diList.push_back(diName[i]);
        }
        for (int i = 0; i < numAiCurve; i++) {
            comboBoxAiDi[i]->blockSignals(true);
            comboBoxAiDi[i]->clear();
            comboBoxAiDi[i]->addItems(aiList);
            if (numAiCurveData > i)
                comboBoxAiDi[i]->setCurrentIndex(i + 1);
            comboBoxAiDi[i]->blockSignals(false);
        }
        for (int i = 0; i < numDiCurve; i++) {
            comboBoxAiDi[i + numAiCurve]->blockSignals(true);
            comboBoxAiDi[i + numAiCurve]->clear();
            comboBoxAiDi[i + numAiCurve]->addItems(diList);
            if (numDiCurveData > i)
                comboBoxAiDi[i + numAiCurve]->setCurrentIndex(i + 1);
            comboBoxAiDi[i + numAiCurve]->blockSignals(false);
        }
        for (int i = 0; i < numAiCurve + numDiCurve; i++) {
            if (comboBoxAiDi[i]->currentIndex())
                cbAiDi[i]->setChecked(true);
            else
                cbAiDi[i]->setChecked(false);
        }

        // GUI waveform y axis display value adjusting box setup
        if (numAiCurveData) {
            int maxy = aiWaveData->getMaxyValue();
            int miny = aiWaveData->getMinyValue();
            ui.sbAiDiDsplyTop->setValue(maxy + 50);
            ui.sbAiDiDsplyBottom->setValue(miny - 50);
            ui.sbAiDiDsplyTop->setMinimum(-1500);
            ui.sbAiDiDsplyTop->setMaximum(1500);
            ui.sbAiDiDsplyBottom->setMinimum(-1500);
            ui.sbAiDiDsplyBottom->setMaximum(1500);
        }
        else {
            ui.sbAiDiDsplyTop->setValue(200);
            ui.sbAiDiDsplyBottom->setValue(0);
        }
        // When current sbAiDiDsplyRight value is bigger than new maximum value
        // QT change current value as the new maximum value and this activate
        // sbAiDiDsplyRight_valueChanged signal. The slot of this signal calls
        // updateAiDiWaveform. Therefore, we need to set comboBoxAiDi[]->currentIndex
        // before calling the updateAiDiWaveform.
        if (dsplyTotalSampleNum > 0)
            ui.sbAiDiDsplyRight->setMaximum(dsplyTotalSampleNum - 1);

        // GUI waveform display
        updateAiDiWaveform(0, dsplyTotalSampleNum - 1);

        // Even though there is no error, if there are some warnings, we show them.
        if (aiWaveData->getErrorMsg() != "") {
            QMessageBox::critical(this, "Imagine", aiWaveData->getErrorMsg(),
                QMessageBox::Ok, QMessageBox::NoButton);
        }
        if (diWaveData->getErrorMsg() != "") {
            QMessageBox::critical(this, "Imagine", diWaveData->getErrorMsg(),
                QMessageBox::Ok, QMessageBox::NoButton);
        }
        //for (int i = 0; i < numAiCurve; i++) {
        //    comboBoxAiDi[i]->blockSignals(false);
        //}
        //for (int i = 0; i < numDiCurve; i++) {
        //    comboBoxAiDi[i + numAiCurve]->blockSignals(false);
        //}
    }
    else {
        if (err & ERR_READ_AI) {
            QMessageBox::critical(this, "Imagine", aiWaveData->getErrorMsg(),
                QMessageBox::Ok, QMessageBox::NoButton);
        }
        if (err & ERR_READ_DI) {
            QMessageBox::critical(this, "Imagine", diWaveData->getErrorMsg(),
                QMessageBox::Ok, QMessageBox::NoButton);
        }
    }
}

bool Imagine::loadAiDiWavDataAndPlot(int leftEnd, int rightEnd, int curveIdx)
{
    int retVal;
    int curveDataSampleNum = 50000;
    int sampleNum = rightEnd - leftEnd + 1;
    int factor = sampleNum / curveDataSampleNum;
    int amplitude, yoffset;
    QVector<int> dest;
    int ctrlIdx;
    CurveData *curveData = NULL;
    QwtPlotCurve *curve;
    QCheckBox *cBox;

    if (factor == 0) {
        curveDataSampleNum = sampleNum;
        factor = 1;
    }
    else
        curveDataSampleNum = sampleNum / factor;

    int comboIdx = comboBoxAiDi[curveIdx]->currentIndex();
    if (comboIdx == 0) {// "empty"
        inCurve[curveIdx]->setData(0);
        return false;
    }

    ctrlIdx = comboIdx - 1;
    curve = inCurve[curveIdx];
    curve->setData(0);
    cBox = cbAiDi[curveIdx];
    dest.clear();
    // set curve position in the plot area
    if (curveIdx < numAiCurve) {
        amplitude = 1;
        yoffset = 0;
        if (!aiWaveData)
            return false;
        retVal = aiWaveData->readWaveform(dest, ctrlIdx, leftEnd, rightEnd, factor);
        if (retVal) {
            curveData = new CurveData(aiWaveData->totalSampleNum); // parameter should not be xpoint.size()
        }
    }
    else if (curveIdx < numAiCurve + numDiCurve) {
        amplitude = 10;
        yoffset = (curveIdx- numAiCurve) * 15;
        if (!diWaveData)
            return false;
        retVal = diWaveData->readWaveform(dest, ctrlIdx, leftEnd, rightEnd, factor);
        if (retVal) {
            curveData = new CurveData(diWaveData->totalSampleNum); // parameter should not be xpoint.size()
        }
    }

    if (retVal) {
        setCurveData(curveData, curve, dest, leftEnd, rightEnd, factor, amplitude, yoffset);
        conReadWavPlot->setAxisScale(QwtPlot::xBottom, curveData->left(), curveData->right());
        if (cBox->isChecked())
            curve->show();
        else
            curve->hide();
        return true;
    }
    else {
        curve->setData(NULL);
        cBox->setChecked(false);
        curve->hide();
        return false;
    }
}

// read jason data to waveform data
void Imagine::updateAiDiWaveform(int leftEnd, int rightEnd)
{
    for (int i = 0; i < cbAiDi.size(); i++) {
//        if(cbAiDi[i]->isChecked())
            loadAiDiWavDataAndPlot(leftEnd, rightEnd, i);
    }

    // y axis setup and replot
    conReadWavPlot->setAxisScale(QwtPlot::yLeft, MIN_Y, ui.sbAiDiDsplyTop->value());
    conReadWavPlot->replot();

    // ui update
    ui.sbAiDiDsplyLeft->setValue(leftEnd);
    ui.sbAiDiDsplyRight->setValue(rightEnd);
    ui.sbAiDiDsplyLeft->setMinimum(0);
    ui.sbAiDiDsplyLeft->setMaximum(rightEnd);
    ui.sbAiDiDsplyRight->setMinimum(leftEnd);
}

void Imagine::showInCurve(int idx, bool checked)
{
    if (inCurve[idx]) {
        if (checked)
            inCurve[idx]->show();
        else
            inCurve[idx]->hide();
        conReadWavPlot->replot();
    }
}

void Imagine::on_cbAI0Wav_clicked(bool checked)
{
    showInCurve(0, checked);
}

void Imagine::on_cbAI1Wav_clicked(bool checked)
{
    showInCurve(1, checked);
}

void Imagine::on_cbDI0Wav_clicked(bool checked)
{
    showInCurve(2, checked);
}

void Imagine::on_cbDI1Wav_clicked(bool checked)
{
    showInCurve(3, checked);
}

void Imagine::on_cbDI2Wav_clicked(bool checked)
{
    showInCurve(4, checked);
}

void Imagine::on_cbDI3Wav_clicked(bool checked)
{
    showInCurve(5, checked);
}

void Imagine::on_cbDI4Wav_clicked(bool checked)
{
    showInCurve(6, checked);
}

void Imagine::on_cbDI5Wav_clicked(bool checked)
{
    showInCurve(7, checked);
}

void Imagine::on_cbDI6Wav_clicked(bool checked)
{
    showInCurve(8, checked);
}

void Imagine::on_comboBoxAI0_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 0);
    on_cbAI0Wav_clicked(ui.cbAI0Wav->isChecked());
}

void Imagine::on_comboBoxAI1_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 1);
    on_cbAI1Wav_clicked(ui.cbAI1Wav->isChecked());
}

void Imagine::on_comboBoxDI0_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 2);
    on_cbDI0Wav_clicked(ui.cbDI0Wav->isChecked());
}

void Imagine::on_comboBoxDI1_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 3);
    on_cbDI1Wav_clicked(ui.cbDI1Wav->isChecked());
}

void Imagine::on_comboBoxDI2_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 4);
    on_cbDI2Wav_clicked(ui.cbDI2Wav->isChecked());
}

void Imagine::on_comboBoxDI3_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 5);
    on_cbDI3Wav_clicked(ui.cbDI3Wav->isChecked());
}

void Imagine::on_comboBoxDI4_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 6);
    on_cbDI4Wav_clicked(ui.cbDI4Wav->isChecked());
}

void Imagine::on_comboBoxDI5_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 7);
    on_cbDI5Wav_clicked(ui.cbDI5Wav->isChecked());
}

void Imagine::on_comboBoxDI6_currentIndexChanged(int index)
{
    loadAiDiWavDataAndPlot(ui.sbAiDiDsplyLeft->value(),
        ui.sbAiDiDsplyRight->value(), 8);
    on_cbDI6Wav_clicked(ui.cbDI6Wav->isChecked());
}

void Imagine::on_sbAiDiDsplyRight_valueChanged(int value)
{
    int left = ui.sbAiDiDsplyLeft->value();
    ui.sbAiDiDsplyLeft->setMaximum(value);
}

void Imagine::on_sbAiDiDsplyLeft_valueChanged(int value)
{
    int right = ui.sbAiDiDsplyRight->value();
    ui.sbAiDiDsplyRight->setMinimum(value);
}

void Imagine::on_sbAiDiDsplyTop_valueChanged(int value)
{
    int bottom = ui.sbAiDiDsplyBottom->value();
    if (value < bottom)
        return;
    conReadWavPlot->setAxisScale(QwtPlot::yLeft, bottom, value);
    conReadWavPlot->replot();
}

void Imagine::on_sbAiDiDsplyBottom_valueChanged(int value)
{
    int top = ui.sbAiDiDsplyTop->value();
    if (value > top)
        return;
    conReadWavPlot->setAxisScale(QwtPlot::yLeft, value, top);
    conReadWavPlot->replot();
}

void Imagine::on_btnAiDiDsplyReload_clicked()
{
    /*
    ui.sbAiDiDsplyLeft->setValue(0);
    ui.sbAiDiDsplyRight->setValue(dsplyTotalSampleNum - 1);
    if (numAiCurveData)
        ui.sbAiDiDsplyTop->setValue(aiWaveData->getMaxyValue() + 50);
    else
        ui.sbAiDiDsplyTop->setValue(200);
        */
    int left = ui.sbAiDiDsplyLeft->value();
    int right = ui.sbAiDiDsplyRight->value();
    updateAiDiWaveform(left, right);
    conReadWavPlot->replot();
}

/***************** Waveform control *************************/

void Imagine::updataSpeedData(CurveData *curveData, int newValue, int start, int end)
{
    double sampleRate = static_cast<double>(newValue);
    double dStart = static_cast<double>(start);
    double dEnd = static_cast<double>(end);
    double width = (dEnd - dStart);
    Positioner *pos = dataAcqThread->pPositioner;
    double maxSpeed = pos->maxSpeed();
    double piezoSpeedCurveMax = maxSpeed / sampleRate * width;
    curveData->clear();
    curveData->append(dStart, 0.);
    curveData->append(dEnd, piezoSpeedCurveMax);
}

template<class Type> void Imagine::setCurveData(CurveData *curveData, QwtPlotCurve *curve,
    QVector<Type> &wave, int start, int end, int factor, int amplitude, int yoffset)
{
    for (int i = start; i <= end; i += factor)
        curveData->append(static_cast<double>(i), wave[(i - start) / factor] * amplitude + yoffset);
    curve->setData(curveData);
}

void Imagine::readControlWaveformFile(QString fn)
{
    enum SaveFormat {
        Json, Binary
    };
    SaveFormat saveFormat;
    QJsonObject alldata;
    QJsonObject metadata;
    QJsonObject analog;
    QJsonObject digital;
    QJsonArray piezo1Wave;
    QJsonArray camera1Pulse;
    QJsonArray laser1Pulse;
    QJsonArray stimulus1Pulse;
    QJsonArray stimulus2Pulse;
    double exposureTime;
    int nStacks;
    int nFrames;
    QFileInfo fi(fn);

    if (conWaveDataUser)
        delete conWaveDataUser;
    conWaveDataUser = new ControlWaveform(rig);
    conWaveData = conWaveDataUser;

    m_OpenConWaveDialogLastDirectory = fi.absolutePath();
    QString ext = fi.suffix();
    if (ext == "json")
        saveFormat = Json;
    else if (ext == "bin")
        saveFormat = Binary;
    else {
        QMessageBox::warning(this, tr("File name error."),
            tr("File extention is not 'json' nor 'bin'."));
        return;
    }
    QFile loadFile(fn);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    QByteArray loadData = loadFile.readAll();
    QJsonDocument loadDoc(saveFormat == Json
        ? QJsonDocument::fromJson(loadData)
        : QJsonDocument::fromBinaryData(loadData));

    CFErrorCode err = conWaveData->loadJsonDocument(loadDoc);

    if (err == NO_CF_ERROR) {
        ui.cbWaveformEnable->setChecked(true);
        ui.cbWaveformEnable->setEnabled(true);
        rearrangeTabWindow();
        displayConWaveData();
    }
    else {
        ui.cbWaveformEnable->setChecked(false);
        ui.cbWaveformEnable->setEnabled(false);
        rearrangeTabWindow();
        clearConWavPlot();
        if (err & ERR_INVALID_VERSION) {
            QMessageBox::critical(this, "Imagine", "Invalid command file version. Please check the version.",
                QMessageBox::Ok, QMessageBox::NoButton);
        }
        if (err & ERR_RIG_MISMATCHED) {
            QMessageBox::critical(this, "Imagine", "Invalid rig name. Please check the name.",
                QMessageBox::Ok, QMessageBox::NoButton);
        }
        if ((err & ERR_INVALID_PORT_ASSIGNMENT) || (err & ERR_WAVEFORM_MISSING)) {
            if (conWaveData->getErrorMsg() != "") {
                QMessageBox::critical(this, "Imagine", conWaveData->getErrorMsg(),
                    QMessageBox::Ok, QMessageBox::NoButton);
            }
        }
    }
}

void Imagine::clearConWavPlot()
{
    // clear the previous plot
    for (int i = 0; i < numAoCurve + numDoCurve; i++)
        outCurve[i]->setData(0);
    conWavPlot->replot();
}

void Imagine::displayConWaveData()
{
    // GUI metadata display
    ui.lableConWavRigname->setText(QString("rig : %1").arg(conWaveData->rig));
    ui.labelPiezoSampleRate->setText(QString("%1").arg(conWaveData->sampleRate));
    int sec = conWaveData->totalSampleNum / conWaveData->sampleRate;
    int min = sec / 60; sec -= min * 60;
    int hour = min / 60; min -= hour * 60;
    ui.labelSampleNumber->setText(QString("%1 (%2h %3m %4s)")
        .arg(conWaveData->totalSampleNum).arg(hour).arg(min).arg(sec));
    ui.doubleSpinBoxExpTimeWav1->setValue(conWaveData->exposureTime1);
    ui.labelNumOfStacksWav1->setText(QString("%1").arg(conWaveData->nStacks1));
    ui.labelFramesPerStackWav1->setText(QString("%1").arg(conWaveData->nFrames1));
    ui.comboBoxExpTriggerModeWav1->setCurrentText(conWaveData->expTriggerModeStr1);
    if ((rig == "ocpi-2") || (rig == "dummy")) {
        ui.doubleSpinBoxExpTimeWav2->setValue(conWaveData->exposureTime2);
        ui.labelNumOfStacksWav2->setText(QString("%1").arg(conWaveData->nStacks2));
        ui.labelFramesPerStackWav2->setText(QString("%1").arg(conWaveData->nFrames2));
        ui.comboBoxExpTriggerModeWav2->setCurrentText(conWaveData->expTriggerModeStr2);
    }
    if (conWaveData->bidirection)
        ui.labelBidirection->setText("on");
    else
        ui.labelBidirection->setText("off");

    // Waveform selection combobox setup
    QStringList aoList, doList;
    int numNonAOEmpty = 0, numNonDOEmpty = 0;
    aoList.push_back("empty");
    doList.push_back("empty");
    for (int i = 0; i < conWaveData->getNumChannel(); i++) {
        if (!conWaveData->isEmpty(i)) {
            if (i < conWaveData->getAIBegin()) {
                aoList.push_back(conWaveData->getSignalName(i));
                numNonAOEmpty++;
            }
            else if ((i >= conWaveData->getP0Begin()) && (i < conWaveData->getP0InBegin())) {
                doList.push_back(conWaveData->getSignalName(i));
                numNonDOEmpty++;
            }
        }
    }
    for (int i = 0; i < numAoCurve; i++) {
        comboBoxAoDo[i]->blockSignals(true);
        comboBoxAoDo[i]->clear();
        comboBoxAoDo[i]->addItems(aoList);
        if (numNonAOEmpty > i)
            comboBoxAoDo[i]->setCurrentIndex(i + 1);
        comboBoxAoDo[i]->blockSignals(false);
    }
    for (int i = 0; i < numDoCurve; i++) {
        comboBoxAoDo[i + numAoCurve]->blockSignals(true);
        comboBoxAoDo[i + numAoCurve]->clear();
        comboBoxAoDo[i + numAoCurve]->addItems(doList);
        if (numNonDOEmpty > i)
            comboBoxAoDo[i + numAoCurve]->setCurrentIndex(i + 1);
        comboBoxAoDo[i + numAoCurve]->blockSignals(false);
    }
    for (int i = 0; i < numAoCurve + numDoCurve; i++) {
        if (comboBoxAoDo[i]->currentIndex())
            cbAoDo[i]->setChecked(true);
        else
            cbAoDo[i]->setChecked(false);
    }

    // GUI waveform y axis display value adjusting box setup
    Positioner *pos = dataAcqThread->pPositioner;
    ui.sbWavDsplyTop->setValue(pos->maxPos() + 50);
    ui.sbWavDsplyTop->setMinimum(0);
    ui.sbWavDsplyTop->setMaximum(33000); // more than 32768
    if (conWaveData->totalSampleNum>0)
        ui.sbWavDsplyRight->setMaximum(conWaveData->totalSampleNum - 1);

    // GUI waveform display
    updateControlWaveform(0, conWaveData->totalSampleNum - 1);
    // Even though there is no error, if there are some warnings, we show them.
    if (conWaveData->getErrorMsg() != "") {
        QMessageBox::critical(this, "Imagine", conWaveData->getErrorMsg(),
            QMessageBox::Ok, QMessageBox::NoButton);
    }
}

bool Imagine::loadConWavDataAndPlot(int leftEnd, int rightEnd, int curveIdx)
{
    int retVal;
    int curveDataSampleNum = 50000; //100000
    int sampleNum = rightEnd - leftEnd + 1;
    int factor = sampleNum / curveDataSampleNum;
    int amplitude, yoffset;
    QVector<int> dest;
    int ctrlIdx;
    CurveData *curveData = NULL;
    QwtPlotCurve *curve;
    QCheckBox *cBox;
    QString sigName;
    int p0Begin = conWaveData->getP0Begin();

    if (factor == 0) {
        curveDataSampleNum = sampleNum;
        factor = 1;
    }
    else
        curveDataSampleNum = sampleNum / factor;

    sigName = comboBoxAoDo[curveIdx]->currentText();
    curve = outCurve[curveIdx];
    curve->setData(0);
    cBox = cbAoDo[curveIdx];
    // set curve position in the plot area
    if (curveIdx < numAoCurve) {
        amplitude = 1;
        yoffset = 0;
    }
    else if (curveIdx < numAoCurve + numDoCurve) {
        amplitude = 10;
        yoffset = (curveIdx - numAoCurve) * 15;
    }
    if ((sigName == "") || (sigName == "empty"))
        return false;

    dest.clear();
    ctrlIdx = conWaveData->getChannelIdxFromSig(sigName);
    if (conWaveData->getSignalName(ctrlIdx).right(5) == "piezo")
        retVal = conWaveData->readControlWaveform(dest, ctrlIdx, leftEnd, rightEnd, factor, PDT_Z_POSITION);
    else
        retVal = conWaveData->readControlWaveform(dest, ctrlIdx, leftEnd, rightEnd, factor); // TODO: for other analog signal, we need to display voltage?
    if (retVal) {
        curveData = new CurveData(conWaveData->totalSampleNum); // parameter should not be xpoint.size()
        setCurveData(curveData, curve, dest, leftEnd, rightEnd, factor, amplitude, yoffset);
        conWavPlot->setAxisScale(QwtPlot::xBottom, curveData->left(), curveData->right());
        if(cBox->isChecked())
            curve->show();
        else
            curve->hide();
        return true;
    }
    else {
        curve->setData(NULL);
        cBox->setChecked(false);
        curve->hide();
        return false;
    }
}

// read jason data to waveform data
void Imagine::updateControlWaveform(int leftEnd, int rightEnd)
{
    for (int i = 0; i < cbAoDo.size(); i++) {
//        if (cbAoDo[i]->isChecked())
            loadConWavDataAndPlot(leftEnd, rightEnd, i);
    }

    // graph data for piezo speed
    CurveData *curveData = NULL;
    curveData = new CurveData(conWaveData->totalSampleNum);
    updataSpeedData(curveData, conWaveData->sampleRate, leftEnd, rightEnd);
    piezoSpeedCurve->setData(curveData);
    conWavPlot->setAxisScale(QwtPlot::yLeft, 0, ui.sbWavDsplyTop->value());
    conWavPlot->replot();

    // ui update
    ui.sbWavDsplyLeft->setValue(leftEnd);
    ui.sbWavDsplyRight->setValue(rightEnd);
    ui.sbWavDsplyLeft->setMinimum(0);
    ui.sbWavDsplyLeft->setMaximum(rightEnd);
    ui.sbWavDsplyRight->setMinimum(leftEnd);
}

void Imagine::on_btnConWavOpen_clicked()
{
    wavFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a control waveform file to open",
        m_OpenConWaveDialogLastDirectory,
        "Waveform files (*.json;*.bin);;All files(*.*)");
    if (wavFilename.isEmpty()) return;

    ui.lineEditConWaveFile->setText(wavFilename);

    readControlWaveformFile(wavFilename);
}

void Imagine::rearrangeTabWindow()
{
    if (ui.cbWaveformEnable->isChecked()) {
        ui.lineEditConWaveFile->setText(wavFilename);
        ui.tabPiezo->setEnabled(false);
        ui.tabStim->setEnabled(false);
        ui.cbBothCamera->setEnabled(false);
        ui.groupBoxTiming->setEnabled(false);
        if (slaveImagine != NULL)
            slaveImagine->ui.tabCamera->setEnabled(false);
    }
    else {
        ui.lineEditConWaveFile->setText("");
        on_btnWavDsplyReset_clicked();
        ui.tabPiezo->setEnabled(true);
        ui.tabStim->setEnabled(true);
        ui.cbBothCamera->setEnabled(true);
        ui.groupBoxTiming->setEnabled(true);
        if (slaveImagine != NULL)
            slaveImagine->ui.tabCamera->setEnabled(true);
    }
}

void Imagine::on_cbWaveformEnable_clicked(bool state)
{
    if (state)
        conWaveData = conWaveDataUser;
    else
        conWaveData = conWaveDataParam;
    rearrangeTabWindow();
    displayConWaveData();
}

void Imagine::showOutCurve(int idx, bool checked)
{
    if (outCurve[idx]) {
        if (checked)
            outCurve[idx]->show();
        else
            outCurve[idx]->hide();
        conWavPlot->replot();
    }
}

void Imagine::on_cbAO0Wav_clicked(bool checked)
{
    showOutCurve(0, checked);
}

void Imagine::on_cbAO1Wav_clicked(bool checked)
{
    showOutCurve(1, checked);
}

void Imagine::on_cbDO0Wav_clicked(bool checked)
{
    showOutCurve(2, checked);
}

void Imagine::on_cbDO1Wav_clicked(bool checked)
{
    showOutCurve(3, checked);
}

void Imagine::on_cbDO2Wav_clicked(bool checked)
{
    showOutCurve(4, checked);
}

void Imagine::on_cbDO3Wav_clicked(bool checked)
{
    showOutCurve(5, checked);
}

void Imagine::on_cbDO4Wav_clicked(bool checked)
{
    showOutCurve(6, checked);
}

void Imagine::on_cbDO5Wav_clicked(bool checked)
{
    showOutCurve(7, checked);
}

void Imagine::on_comboBoxAO0_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 0);
    on_cbAO0Wav_clicked(ui.cbAO0Wav->isChecked());
}

void Imagine::on_comboBoxAO1_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 1);
    on_cbAO1Wav_clicked(ui.cbAO1Wav->isChecked());
}

void Imagine::on_comboBoxDO0_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 2);
    on_cbDO0Wav_clicked(ui.cbDO0Wav->isChecked());
}

void Imagine::on_comboBoxDO1_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 3);
    on_cbDO1Wav_clicked(ui.cbDO1Wav->isChecked());
}

void Imagine::on_comboBoxDO2_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 4);
    on_cbDO2Wav_clicked(ui.cbDO2Wav->isChecked());
}

void Imagine::on_comboBoxDO3_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 5);
    on_cbDO3Wav_clicked(ui.cbDO3Wav->isChecked());
}

void Imagine::on_comboBoxDO4_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 6);
    on_cbDO4Wav_clicked(ui.cbDO4Wav->isChecked());
}

void Imagine::on_comboBoxDO5_currentIndexChanged(int index)
{
    loadConWavDataAndPlot(ui.sbWavDsplyLeft->value(),
        ui.sbWavDsplyRight->value(), 7);
    on_cbDO5Wav_clicked(ui.cbDO4Wav->isChecked());
}

void Imagine::on_sbWavDsplyRight_valueChanged(int value)
{
    int left = ui.sbWavDsplyLeft->value();
    ui.sbWavDsplyLeft->setMaximum(value);
    updateControlWaveform(left, value);
    conWavPlot->replot();
}

void Imagine::on_sbWavDsplyLeft_valueChanged(int value)
{
    int right = ui.sbWavDsplyRight->value();
    ui.sbWavDsplyRight->setMinimum(value);
    updateControlWaveform(value, right);
    conWavPlot->replot();
}

void Imagine::on_sbWavDsplyTop_valueChanged(int value)
{
    conWavPlot->setAxisScale(QwtPlot::yLeft, 0, value);
    conWavPlot->replot();
}

void Imagine::on_btnWavDsplyReset_clicked()
{
    ui.sbWavDsplyLeft->setValue(0);
    ui.sbWavDsplyRight->setValue(conWaveData->totalSampleNum - 1);
    Positioner *pos = dataAcqThread->pPositioner;
    ui.sbWavDsplyTop->setValue(pos->maxPos() + 50);
}

void Imagine::on_btnConWavList_clicked()
{
    QString msg = "";
    msg.append(QString("version : %1\n")
        .arg(conWaveData->version));
    msg.append(QString("generated from : %1\n\n")
        .arg(conWaveData->generatedFrom));
    for (int i = 0; i < conWaveData->getNumChannel(); i++) {
        QString sigName = conWaveData->getSignalName(i);
        if (sigName != "") {
            msg.append(QString("%1 : %2\n")
                .arg(conWaveData->getChannelName(i)).arg(sigName));
        }
    }
    QMessageBox::information(this, "Control signal list", msg, QMessageBox::Ok);

}
/****** Image Display *****************************************************/
#define IMAGINE_VALID_CHECK(container)\
        if(line.contains("IMAGINE", Qt::CaseSensitive)){\
            bool ok; container = true;}
#define READ_IMAGINE_INT(key,container)\
        else if (line.contains(QString(key).append("="), Qt::CaseSensitive)){\
            bool ok; container = line.remove(0, QString(key).size()+1).section(' ',0,0).toInt(&ok);}
#define READ_IMAGINE_DOUBLE(key,container)\
        else if (line.contains(QString(key).append("="), Qt::CaseSensitive)){\
            bool ok; container = line.remove(0, QString(key).size()+1).section(' ',0,0).toDouble(&ok);}
#define READ_IMAGINE_STRING(key,container)\
        else if (line.contains(QString(key).append("="), Qt::CaseSensitive)){\
            bool ok; container = line.remove(0, QString(key).size()+1);}

bool Imagine::readImagineFile(QString filename, ImagineData &img)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(filename)
            .arg(file.errorString()));
        return false;
    }

    QTextStream in(&file);
    QString line;
    do {
        line = in.readLine();
        IMAGINE_VALID_CHECK(img.valid) // should not use ;
        READ_IMAGINE_INT("[ai]", img.version)
        READ_IMAGINE_STRING("app version", img.appVersion)
        READ_IMAGINE_STRING("date and time", img.dateNtime)
        READ_IMAGINE_STRING("rig", img.rig)
        READ_IMAGINE_STRING("binning", img.binning) // TODO: need more parsing
        READ_IMAGINE_STRING("label list", img.stimlabelList) // TODO: need more parsing
        READ_IMAGINE_INT("image width", img.imgWidth)
        READ_IMAGINE_INT("image height", img.imgHeight)
        READ_IMAGINE_INT("nStacks", img.nStacks)
        READ_IMAGINE_INT("frames per stack", img.framesPerStack)
        READ_IMAGINE_DOUBLE("exposure time", img.exposure)
        READ_IMAGINE_STRING("pixel order", img.pixelOrder)
        READ_IMAGINE_STRING("pixel data type", img.dataType)
    } while (!line.isNull());

    if (img.dataType == "uint16")
        img.bytesPerPixel = 2;
    else
        img.bytesPerPixel = 1;
    img.imageSizePixels = img.imgHeight * img.imgWidth;
    img.imageSizeBytes = img.imageSizePixels * img.bytesPerPixel;

    file.close();

    if (!img.valid) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("%1 file is not valid\n")
            .arg(filename));
        return false;
    }
    else
        return true;
}

void Imagine::holdDisplayCamFile()
{
    ui.rbImgCameraEnable->setChecked(true);
    ui.cbImg1Enable->setChecked(true);
    img1.enable = false;
    img2.enable = false;
    reconfigDisplayTab();
}

void Imagine::stopDisplayCamFile()
{
//    ui.cbImg1Enable->setChecked(false);
//    ui.cbImg2Enable->setChecked(false);
    img1.valid = false;
    img2.valid = false;
    img1.enable = false;
    img2.enable = false;
    img1.camValid = false;
    img2.camValid = false;
    img1.camImage.clear();
    img2.camImage.clear();
    if (img1.camFile.isOpen())
        img1.camFile.close();
    if (img2.camFile.isOpen())
        img2.camFile.close();
}

void Imagine::on_btnImg1LoadFile_clicked()
{
    // read .imagine file
    QString filename = QFileDialog::getOpenFileName(this, tr("Select an Imagine file 1"),
        m_OpenDialogLastDirectory,
        tr("Imagine File (*.imagine)"));
    if (true == filename.isEmpty()) { return; }
    QFileInfo fi(filename);
    m_OpenDialogLastDirectory = fi.absolutePath();
    ui.rbImgFileEnable->setChecked(true);

    bool succeed = readImagineFile(filename, img1);
    if (succeed == false) {
        ui.cbImg1Enable->setChecked(false);
        ui.lineEditImg1Filename->setText("");
        img1.enable = false;
        return;
    }
    else {
        ui.cbImg1Enable->setChecked(true);
        ui.lineEditImg1Filename->setText(filename);
        img1.enable = true;
    }
    setupPlayCamParam(img1);
    QString camFilename = replaceExtName(filename, "cam");
    openAndReadCamFile(camFilename, img1);
    ui.sbFrameIdx->setValue(imgFrameIdx);
    ui.sbStackIdx->setValue(imgStackIdx);

    reconfigDisplayTab();
}

void Imagine::on_btnImg2LoadFile_clicked()
{
    // read .imagine file
    QString filename = QFileDialog::getOpenFileName(this, tr("Select an Imagine file 2"),
        m_OpenDialogLastDirectory,
        tr("Imagine File (*.imagine)"));
    if (true == filename.isEmpty()) { return; }
    QFileInfo fi(filename);
    m_OpenDialogLastDirectory = fi.absolutePath();
    ui.rbImgFileEnable->setChecked(true);

    bool succeed = readImagineFile(filename, img2);
    if (succeed == false) {
        ui.cbImg2Enable->setChecked(false);
        ui.lineEditImg2Filename->setText("");
        img2.enable = false;
        return;
    }
    else {
        ui.cbImg2Enable->setChecked(true);
        ui.lineEditImg2Filename->setText(filename);
        img2.enable = true;
    }
    setupPlayCamParam(img2);
    QString camFilename = replaceExtName(filename, "cam");
    openAndReadCamFile(camFilename, img2);
    ui.sbFrameIdx->setValue(imgFrameIdx);
    ui.sbStackIdx->setValue(imgStackIdx);

    reconfigDisplayTab();
}

bool Imagine::compareDimensions()
{
    if ((img1.nStacks != img2.nStacks) || (img1.framesPerStack != img2.framesPerStack) ||
        (img1.imgWidth != img2.imgWidth) || (img1.imgHeight != img2.imgHeight)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Number of stacks, frames per stack, image width and height of image 1 an image 2 should be same."));
        return false;
    }
    else
        return true;
}

bool Imagine::setupPlayCamParam(ImagineData &img)
{
    // setup palyback parameter
    if (img1.valid && img1.enable && img2.valid && img2.enable) {
        if (!compareDimensions()) {
            img.valid = false;
            return false;
        }
        else
            setupDimensions(img.nStacks, img.framesPerStack, img.imgWidth, img.imgHeight);
    }
    else if (img.valid) {
        setupDimensions(img.nStacks, img.framesPerStack, img.imgWidth, img.imgHeight);
    }
    else {
        setupDimensions(0, 0, 0, 0);
    }
    ui.sbStackIdx->setMaximum(imgNStacks - 1);
    ui.sbFrameIdx->setMaximum(imgFramesPerStack - 1);
    ui.hsStackIdx->setMaximum(imgNStacks - 1);
    ui.hsFrameIdx->setMaximum(imgFramesPerStack - 1);
    ui.labelNumverOfStack->setText(QString("%1").arg(imgNStacks));
    ui.labelFramesPerStack->setText(QString("%1").arg(imgFramesPerStack));

    // Reset stack index, frame index and zoom
    imgFrameIdx = 0; // 0-based
    imgStackIdx = 0; // 0-based
    L = -1;
    return true;
}

void Imagine::setupDimensions(int stacks, int frames, int width, int height)
{
    imgNStacks = stacks;
    imgFramesPerStack = frames;
    imgWidth = width;
    imgHeight = height;
}

bool Imagine::openAndReadCamFile(QString filename, ImagineData &img)
{
    img.camFile.setFileName(filename);
    if (!img.camFile.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(filename)
            .arg(img.camFile.errorString()));
        return false;
    }

    // read camfile and update with current index and zoom setting
    readCamFileImagesAndUpdate();

    return true;
}

void Imagine::applyImgColor(QWidget *widget, QColor color)
{
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, color);
    widget->setPalette(pal);

}

void Imagine::on_pbImg1Color_clicked()
{
    img1Color = QColorDialog::getColor(img1Color, this);
    applyImgColor(ui.widgetImg1Color, img1Color);
    readCamFileImagesAndUpdate();
}

void Imagine::on_pbImg2Color_clicked()
{
    img2Color = QColorDialog::getColor(img2Color, this);
    applyImgColor(ui.widgetImg2Color, img2Color);
    readCamFileImagesAndUpdate();
}

// read camfile with current imgStackIdx and imgFrameIdx
bool Imagine::readCamFileImages()
{
    bool found;
    qint64 totalImgIdx;
    // TODO: merge below two blocks
    if (img1.valid&&img1.enable) {
        img1.currentFrameIdx = imgFrameIdx;
        img1.currentStackIdx = imgStackIdx;
        totalImgIdx = img1.currentStackIdx*img1.framesPerStack + img1.currentFrameIdx;
        img1.camValid = img1.camFile.seek(totalImgIdx*img1.imageSizeBytes);
        if (img1.camValid) {
            img1.camImage = img1.camFile.read(img1.imageSizeBytes);
        }
        else {
            appendLog("Image 1 : invalid stack and frame numbers");
            return false;
        }
    }
    if (img2.valid&&img2.enable) {
        img2.currentFrameIdx = imgFrameIdx;
        img2.currentStackIdx = imgStackIdx;
        totalImgIdx = img2.currentStackIdx*img2.framesPerStack + img2.currentFrameIdx;
        img2.camValid = img2.camFile.seek(totalImgIdx*img2.imageSizeBytes);
        if (img2.camValid) {
            img2.camImage = img2.camFile.read(img2.imageSizeBytes);
        }
        else {
            appendLog("Image 2 : invalid stack and frame numbers");
            return false;
        }
    }
    return true;
}

void Imagine::readCamFileImagesAndUpdate()
{
    isUpdatingDisplay = true; // updating cam file image (this is meaningless)
    bool succeed = readCamFileImages();

    if (succeed) {
        // update images with current zoom setting
        nUpdateImage = 0;
        minPixelValue = maxPixelValue = -1;
        minPixelValue2 = maxPixelValue2 = -1;
        if (img1.enable && img1.camValid && img2.enable && img2.camValid)
            updateDisplayColor(img1.camImage, img2.camImage, imgFrameIdx, imgWidth, imgHeight);
        else if (img1.enable && img1.camValid)
            updateDisplay(img1.camImage, imgFrameIdx, img1.imgWidth, img1.imgHeight);
        else if (img2.enable && img2.valid)
            updateDisplay(img2.camImage, imgFrameIdx, img2.imgWidth, img2.imgHeight);
    }
    isUpdatingDisplay = false;
}

void Imagine::updateLiveImagePlay(const QByteArray &data16, long idx, int imageW, int imageH)
{
    if (masterImagine != NULL) {
        if (masterImagine->ui.rbImgCameraEnable->isChecked()) {
            if (masterImagine->ui.cbCam2Enable->isChecked()) {
                masterImageReady = false;
                return;
            }
            masterImageReady = true;
            readCameraImagesAndUpdate();
        }
    }
    else {
        if (ui.rbImgCameraEnable->isChecked()) {
            masterImageReady = true;
            readCameraImagesAndUpdate();
        }
    }
}

void Imagine::updateSlaveLiveImagePlay(const QByteArray &data16, long idx, int imageW, int imageH)
{
    if (ui.rbImgCameraEnable->isChecked()) {
        slaveImageReady = true;
        readCameraImagesAndUpdate();
    }
}

void Imagine::readCameraImagesAndUpdate()
{
    QByteArray live1, live2;
    int width1, height1, width2, height2;
    int updateMethod = 0;
    if ((masterImagine != NULL) && masterImageReady) // slave Imagine
    {
//        dataAcqThread->isUpdatingImage = true;
        live1 = dataAcqThread->pCamera->getLiveImage();
        width1 = dataAcqThread->pCamera->getImageWidth();
        height1 = dataAcqThread->pCamera->getImageHeight();
        if (live1.size() != 0) updateMethod = 1;
    }
    else {
        if (ui.cbCam1Enable->isChecked()) {
            live1 = dataAcqThread->pCamera->getLiveImage();
            width1 = dataAcqThread->pCamera->getImageWidth();
            height1 = dataAcqThread->pCamera->getImageHeight();
            if (live1.size() != 0) updateMethod += 1;
        }
        if (ui.cbCam2Enable->isChecked() && slaveImagine) {
            live2 = slaveImagine->dataAcqThread->pCamera->getLiveImage();
            width2 = slaveImagine->dataAcqThread->pCamera->getImageWidth();
            height2 = slaveImagine->dataAcqThread->pCamera->getImageHeight();
            if (live2.size() != 0) updateMethod += 2;
        }
        if (updateMethod == 3) {
            if ((width1 != width2) || (height1 != height2)) {
                QMessageBox::warning(this, tr("Imagine"),
                    tr("Image size is different betwen camera 1 and camera 2"));
                // only display camera1
                ui.cbCam2Enable->setChecked(false);
                updateMethod = 1;
            }
        }
    }

    if (updateMethod) {
        maxPixelValue = -1;
        if (updateMethod == 1) {
            dataAcqThread->isUpdatingImage = true;
            updateDisplay(live1, 0, width1, height1);
            masterImageReady = false;
            slaveImageReady = false;
            dataAcqThread->isUpdatingImage = false;
        }
        else if (updateMethod == 2) {
            slaveImagine->dataAcqThread->isUpdatingImage = true;
            updateDisplay(live2, 0, width2, height2);
            masterImageReady = false;
            slaveImageReady = false;
            slaveImagine->dataAcqThread->isUpdatingImage = false;
        }
        else {// updateMethod == 3
            if (masterImageReady && slaveImageReady) { // ignore until both images are ready
                updateDisplayColor(live1, live2, 0, width1, height1);
                masterImageReady = false;
                slaveImageReady = false;
                dataAcqThread->isUpdatingImage = false;
                slaveImagine->dataAcqThread->isUpdatingImage = false;
            }
            else if (masterImageReady)
            {
                dataAcqThread->isUpdatingImage = true;
            }
            else if (slaveImageReady)
            {
                slaveImagine->dataAcqThread->isUpdatingImage = true;
            }
        }
    }
    else {// show logo
        QImage tImage(":/images/Resources/logo.jpg"); //todo: put logo here
        if (!tImage.isNull()) {
            ui.labelImage->setPixmap(QPixmap::fromImage(tImage));
            ui.labelImage->adjustSize();
        }
    }
}

void Imagine::readNextCamImages(int stack1, int frameIdx1, int stack2, int frameIdx2)
{
    if (ui.rbImgCameraEnable->isChecked())
        return;
    if (imgStackIdx != stack1) {
        imgStackIdx = stack1;
        ui.sbStackIdx->setValue(imgStackIdx);
    }
    if (imgFrameIdx != frameIdx1) {
        imgFrameIdx = frameIdx1;
        ui.sbFrameIdx->setValue(imgFrameIdx);
    }
}

void Imagine::setFrameIndexSpeed(int speed1, int speed2)
{
    if (img1.camValid) {
        img1.framePlaySpeed = speed1;
        imagePlay->framePlaySpeed1 = img1.framePlaySpeed;
        img1.stackPlaySpeed = 0;
        img2.stackPlaySpeed = 0;
        imagePlay->stackPlaySpeed1 = 0;
        imagePlay->stackPlaySpeed2 = 0;
    }
    if (img2.camValid) {
        img2.framePlaySpeed = speed2;
        imagePlay->framePlaySpeed2 = img2.framePlaySpeed;
        img1.stackPlaySpeed = 0;
        img2.stackPlaySpeed = 0;
        imagePlay->stackPlaySpeed1 = 0;
        imagePlay->stackPlaySpeed2 = 0;
    }
    if (!imagePlay->isRunning)
        emit startIndexRunning(imgStackIdx, imgFrameIdx, img1.nStacks, img1.framesPerStack,
            imgStackIdx, imgFrameIdx, img2.nStacks, img2.framesPerStack);
}

void Imagine::on_pbFramePlayback_clicked()
{
    setFrameIndexSpeed(-1, -1);
}

void Imagine::on_pbFramePlay_clicked()
{
    setFrameIndexSpeed(1, 1);
}

void Imagine::on_pbFrameFastBackward_clicked()
{
    setFrameIndexSpeed(-5, -5);
}

void Imagine::on_pbFrameFastForward_clicked()
{
    setFrameIndexSpeed(5, 5);
}

void Imagine::on_pbFramePause_clicked()
{
    setFrameIndexSpeed(0, 0);
}

void Imagine::setStackIndexSpeed(int speed1, int speed2)
{
    if (img1.camValid) {
        img1.stackPlaySpeed = speed1;
        imagePlay->stackPlaySpeed1 = img1.stackPlaySpeed;
        img1.framePlaySpeed = 0;
        img2.framePlaySpeed = 0;
        imagePlay->framePlaySpeed1 = 0;
        imagePlay->framePlaySpeed2 = 0;
    }
    if (img2.camValid) {
        img2.stackPlaySpeed = speed2;
        imagePlay->stackPlaySpeed2 = img2.stackPlaySpeed;
        img1.framePlaySpeed = 0;
        img2.framePlaySpeed = 0;
        imagePlay->framePlaySpeed1 = 0;
        imagePlay->framePlaySpeed2 = 0;
    }
    if(!imagePlay->isRunning)
        emit startIndexRunning(imgStackIdx, imgFrameIdx, img1.nStacks, img1.framesPerStack,
            imgStackIdx, imgFrameIdx, img2.nStacks, img2.framesPerStack);
}

void Imagine::on_pbStackPlayback_clicked()
{
    setStackIndexSpeed(-1, -1);
}

void Imagine::on_pbStackPlay_clicked()
{
    setStackIndexSpeed(1, 1);
}

void Imagine::on_pbStackFastBackward_clicked()
{
    setStackIndexSpeed(-5, -5);
}

void Imagine::on_pbStackFastForward_clicked()
{
    setStackIndexSpeed(5, 5);
}

void Imagine::on_pbStackPause_clicked()
{
    setStackIndexSpeed(0, 0);
}

void Imagine::on_sbFrameIdx_valueChanged(int newValue)
{
    imgFrameIdx = newValue;
    ui.hsFrameIdx->setValue(newValue);
    imagePlay->frameIdx1 = newValue;
    imagePlay->frameIdx2 = newValue;
    if(!isUpdatingDisplay)
        readCamFileImagesAndUpdate();
}

void Imagine::on_sbStackIdx_valueChanged(int newValue)
{
    imgStackIdx = newValue;
    ui.hsStackIdx->setValue(newValue);
    imagePlay->stackIdx1 = newValue;
    imagePlay->stackIdx2 = newValue;
    if (!isUpdatingDisplay)
        readCamFileImagesAndUpdate();
}

void Imagine::on_hsFrameIdx_valueChanged(int newValue)
{
    ui.sbFrameIdx->setValue(newValue);
}

void Imagine::on_hsStackIdx_valueChanged(int newValue)
{
    ui.sbStackIdx->setValue(newValue);
}

void Imagine::reconfigDisplayTab()
{
    if (ui.rbImgCameraEnable->isChecked()) {
        // stop playing cam file images
        img1.stackPlaySpeed = 0;
        img2.stackPlaySpeed = 0;
        imagePlay->stackPlaySpeed1 = 0;
        imagePlay->stackPlaySpeed2 = 0;
        img1.framePlaySpeed = 0;
        img2.framePlaySpeed = 0;
        imagePlay->framePlaySpeed1 = 0;
        imagePlay->framePlaySpeed2 = 0;
        // reconfigurate some GUI items
        ui.groupBoxPlayCamImage->setEnabled(false);
        if (ui.cbCam1Enable->isChecked() && ui.cbCam2Enable->isChecked()) {
            ui.groupBoxBlendingOption->setEnabled(true);
//            ui.actionContrastMin2->setVisible(true);
//            ui.actionContrastMax2->setVisible(true);
        }
        else {
            ui.groupBoxBlendingOption->setEnabled(false);
//            ui.actionContrastMin2->setVisible(false);
//            ui.actionContrastMax2->setVisible(false);
        }
        if (slaveImagine) {
            if (ui.cbCam2Enable->isChecked()) {
                connect(slaveImagine->dataAcqThread, SIGNAL(imageDataReady(const QByteArray &, long, int, int)),
                    this, SLOT(updateSlaveLiveImagePlay(const QByteArray &, long, int, int)));
                if (ui.cbCam1Enable->isChecked()) {
                    dataAcqThread->isUpdatingImage = false;
                    slaveImagine->dataAcqThread->isUpdatingImage = false;
                }
            }
            else {
                disconnect(slaveImagine->dataAcqThread, SIGNAL(imageDataReady(const QByteArray &, long, int, int)),
                    this, SLOT(updateSlaveLiveImagePlay(const QByteArray &, long, int, int)));
                slaveImagine->dataAcqThread->isUpdatingImage = false;
            }
        }
    }
    else {
        if ((img1.enable && img1.camValid) || (img2.enable && img2.camValid)) {
            ui.groupBoxPlayCamImage->setEnabled(true);
//            ui.actionContrastMin2->setVisible(true);
//            ui.actionContrastMax2->setVisible(true);
        }
        else {
            ui.groupBoxPlayCamImage->setEnabled(false);
//            ui.actionContrastMin2->setVisible(false);
//            ui.actionContrastMax2->setVisible(false);
        }
        if ((img1.enable && img1.camValid) && (img2.enable && img2.camValid))
            ui.groupBoxBlendingOption->setEnabled(true);
        else
            ui.groupBoxBlendingOption->setEnabled(false);
        if (slaveImagine) {
            disconnect(slaveImagine->dataAcqThread, SIGNAL(imageDataReady(const QByteArray &, long, int, int)),
                this, SLOT(updateSlaveLiveImagePlay(const QByteArray &, long, int, int)));
        }
    }
}

void Imagine::on_cbImg1Enable_clicked(bool checked)
{
    if (ui.rbImgFileEnable->isChecked()) {
        img1.enable = checked;
        reconfigDisplayTab();
        readCamFileImagesAndUpdate();
    }
}

void Imagine::on_cbImg2Enable_clicked(bool checked)
{
    if (ui.rbImgFileEnable->isChecked()) {
        img2.enable = checked;
        reconfigDisplayTab();
        readCamFileImagesAndUpdate();
    }
}

void Imagine::on_cbCam1Enable_clicked(bool checked)
{
    if (ui.rbImgCameraEnable->isChecked()) {
        reconfigDisplayTab();
        readCameraImagesAndUpdate();
    }
}

void Imagine::on_cbCam2Enable_clicked(bool checked)
{
    if (ui.rbImgCameraEnable->isChecked()) {
        reconfigDisplayTab();
        readCameraImagesAndUpdate();
    }
}

void Imagine::on_rbImgCameraEnable_toggled(bool checked)
{
    L = -1; // full image
    if (checked) { // camera
        reconfigDisplayTab();
        readCameraImagesAndUpdate();
    }
    else { // imagine file
        img1.enable = ui.cbImg1Enable->isChecked();
        img2.enable = ui.cbImg2Enable->isChecked();
        reconfigDisplayTab();
        readCamFileImagesAndUpdate();
    }
}

void Imagine::on_rbImgFileEnable_toggled(bool checked)
{
    return;
}

void Imagine::on_hsBlending_valueChanged()
{
    alpha = ui.hsBlending->value();
    displayImageUpdate();
}

void Imagine::on_hsBlending_sliderReleased()
{
    updateImage(isUpdateImgColor, true);
}

void Imagine::on_hsFrameIdx_sliderReleased()
{
    updateImage(isUpdateImgColor, true);
}

void Imagine::on_hsStackIdx_sliderReleased()
{
    updateImage(isUpdateImgColor, true);
}

bool Imagine::autoFindMismatchParameters(QByteArray &img1, QByteArray &img2, int width, int height)
{
    // TODO: Auto find mismatch
    return true;
}

void Imagine::calHomogeneousTramsformMatrix()
{
    int width, height;
    TransformParam &param = (pixmapper->param);
    if (ui.rbImgCameraEnable->isChecked()) {
        width = dataAcqThread->pCamera->getImageWidth();
        height = dataAcqThread->pCamera->getImageHeight();
    }
    else {
        width = img1.imgWidth;
        height = img1.imgHeight;
    }
    double cx = (double)width / 2.;
    double cy = (double)height / 2.;
    double radian = param.theta / 180.*M_PI;
    double c = cos(radian);
    double s = sin(radian);
    double tcx = cx + param.tx;
    double tcy = cy + param.ty;
    param.a[0][0] = c;
    param.a[0][1] = s;
    param.a[0][2] = -c*tcx - s*tcy + cx;
    param.a[1][0] = s;
    param.a[1][1] = -c;
    param.a[1][2] = -s*tcx + c*tcy + cy;
}

// Currently we just apply translation and rotation parallel to the focal plain.
void Imagine::enableMismatchCorrection(QByteArray &img1, QByteArray &img2, int width, int height)
{
    if (pixmapper != NULL) {
        Camera::PixelValue * tp = (Camera::PixelValue *)img1.constData();

        // TODO: Auto mismatch will be implemented later.
        // Now we just use manual mode
        pixmapper->param.isAuto = false;
        if (pixmapper->param.isAuto) {
            pixmapper->param.isOk = autoFindMismatchParameters(img1, img2, width, height);
            ui.sbXTranslation->setValue(pixmapper->param.tx);
            ui.sbYTranslation->setValue(pixmapper->param.ty);
            ui.dsbRotationAngle->setValue(pixmapper->param.theta);
        }
        else {
            pixmapper->param.tx = (double)ui.sbXTranslation->value();
            pixmapper->param.ty = (double)ui.sbYTranslation->value();
            pixmapper->param.theta = ui.dsbRotationAngle->value();
            calHomogeneousTramsformMatrix();
            pixmapper->param.isOk = true;
        }

        if (!pixmapper->param.isOk) {
            // TODO: parameter finding error warning message
            ui.cbEnableMismatch->setChecked(false);
        }
    }
    else {
        // TODO: no pixmapper error warning message
        ui.cbEnableMismatch->setChecked(false);
    }
}

void Imagine::on_pbSaveImage_clicked()
{
    QString proposedFile;
    if (imageFilename == "")
        proposedFile = m_OpenDialogLastDirectory + QDir::separator() + "image.jpg";
    else
        proposedFile = imageFilename;
    imageFilename = QFileDialog::getSaveFileName(this, tr("Save current image"), proposedFile, tr("JPG file(*.jpg)"));
    if (true == imageFilename.isEmpty()) { return; }
    QFileInfo fi(imageFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();

    bool result = pixmap.save(imageFilename, "JPG", 80);
    if (!result) {
        QMessageBox::critical(this, "Imagine", "Fail to save image.",
            QMessageBox::Ok, QMessageBox::NoButton);
    }
}

void Imagine::on_cbEnableMismatch_clicked(bool checked)
{
    if (checked) {
        if (ui.rbImgCameraEnable->isChecked()) {
            enableMismatchCorrection(dataAcqThread->pCamera->getLiveImage(),
                slaveImagine->dataAcqThread->pCamera->getLiveImage(),
                dataAcqThread->pCamera->getImageWidth(),
                dataAcqThread->pCamera->getImageHeight());
        }
        else
            enableMismatchCorrection(img1.camImage, img2.camImage, img1.imgWidth, img1.imgHeight);
    }
    else {
        if (pixmapper != NULL) pixmapper->param.isOk = false;
    }
    displayImageUpdate();
}

void Imagine::displayImageUpdate()
{
    if (ui.rbImgCameraEnable->isChecked()) {
        if(!dataAcqThread->isUpdatingImage)
            readCameraImagesAndUpdate();
    }
    else {
        if (!isUpdatingDisplay)
            readCamFileImagesAndUpdate();
    }
}

void Imagine::on_sbXTranslation_valueChanged(int newValue)
{
    pixmapper->param.tx = newValue;
    calHomogeneousTramsformMatrix();
    displayImageUpdate();
}

void Imagine::on_sbXTranslation_editingFinished()
{
    updateImage(isUpdateImgColor, true);
}

void Imagine::on_sbYTranslation_valueChanged(int newValue)
{
    pixmapper->param.ty = newValue;
    calHomogeneousTramsformMatrix();
    displayImageUpdate();
}

void Imagine::on_sbYTranslation_editingFinished()
{
    updateImage(isUpdateImgColor, true);
}

void Imagine::on_dsbRotationAngle_valueChanged(double newValue)
{
    pixmapper->param.theta = newValue;
    calHomogeneousTramsformMatrix();
    displayImageUpdate();
}

void Imagine::on_dsbRotationAngle_editingFinished()
{
    updateImage(isUpdateImgColor, true);
}

/****** Script ************************************************************/
void Imagine::on_btnOpenScriptFile_clicked()
{
    QString scriptFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a script file to open",
        "",
        "Script files (*.js);;All files(*.*)");
    if (scriptFilename.isEmpty()) return;

    ui.lineEditReadScriptFle->setText(scriptFilename);

    QFile file(scriptFilename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(scriptFilename)
            .arg(file.errorString()));
        return;
    }
    QTextStream in(&file);
    ui.textEditScriptFileContent->setPlainText(in.readAll());
    file.close();
}

void Imagine::on_btnScriptExecute_clicked()
{
    if (imagineScript != NULL)
        delete imagineScript;
    imagineScript = new ImagineScript(rig);
    imagineScript->moveToThread(&scriptThread);
    connect(&scriptThread, &QThread::finished, imagineScript, &QObject::deleteLater);
    connect(this, &Imagine::evaluateScript, imagineScript, &ImagineScript::scriptProgramEvaluate);
//    connect(this, &Imagine::stopEvaluating, imagineScript, &ImagineScript::scriptAbortEvaluation);
    connect(imagineScript, &ImagineScript::newMsgReady, this, &Imagine::appendLog);
    connect(imagineScript, &ImagineScript::requestValidityCheck, this, &Imagine::configValidityCheck);
    connect(imagineScript, &ImagineScript::requestRecord, this, &Imagine::configRecord);
    connect(imagineScript, &ImagineScript::requestLoadConfig, this, &Imagine::loadConfig);
    connect(imagineScript, &ImagineScript::requestLoadWaveform, this, &Imagine::loadWaveform);
    connect(imagineScript, &ImagineScript::requestSetFilename, this, &Imagine::setFilename);
    connect(imagineScript, &ImagineScript::requestStopRecord, this, &Imagine::scriptStopRecord);
    scriptThread.start(QThread::IdlePriority);

    imagineScript->loadImagineScript(ui.textEditScriptFileContent->toPlainText());
    emit evaluateScript();
}

void Imagine::on_btnScriptStop_clicked()
{
    if (imagineScript != NULL)
        imagineScript->scriptAbortEvaluation();
}

void Imagine::on_textEditScriptFileContent_cursorPositionChanged()
{
    QTextCursor cursor = ui.textEditScriptFileContent->textCursor();
    int block = cursor.blockNumber();// ui.textEditScriptFileContent->document()->blockCount();
    ui.labelScriptLineNum->setText(QString("%1").arg(block + 1));
    ui.labelScriptColumnNum->setText(QString("%1").arg(cursor.columnNumber() + 1));
}

void Imagine::on_btnScriptUndo_clicked()
{
    QTextCursor cursor = ui.textEditScriptFileContent->textCursor();
    ui.textEditScriptFileContent->document()->undo(&cursor);
}

void Imagine::on_btnScriptRedo_clicked()
{
    QTextCursor cursor = ui.textEditScriptFileContent->textCursor();
    ui.textEditScriptFileContent->document()->redo(&cursor);
}

void Imagine::on_btnScriptSave_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save OCPI Script file"),
        ui.lineEditReadScriptFle->text(), "*.js");
    if (true == filename.isEmpty()) { return; }
    QFileInfo fi(filename);
    m_OpenDialogLastDirectory = fi.absolutePath();
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug("File open error\n");
    }
    else {
        QTextStream out(&file);
        out << ui.textEditScriptFileContent->toPlainText();
        out.flush();
        file.close();
        ui.lineEditReadScriptFle->setText(filename);
    }
}

void Imagine::scriptStopRecord()
{
    if (curStatus == eRunning) {
        dataAcqThread->stopAcq();
        if (slaveImagine) slaveImagine->dataAcqThread->stopAcq();
        updateStatus(eStopping, curAction);
    }
    else {
        imagineScript->shouldWait = false;
        imagineScript->retVal = true;
    }
}

void Imagine::scriptApplyAndReport(bool preRetVal)
{
    isApplyCommander = true;
    bool retVal = applySetting();
    isApplyCommander = false;
    imagineScript->estimatedTime = conWaveData->totalSampleNum / conWaveData->sampleRate;
    imagineScript->retVal = preRetVal&retVal;
    imagineScript->shouldWait = false;
}

void Imagine::scriptJustReport(bool preRetVal)
{
    imagineScript->retVal = preRetVal;
    imagineScript->shouldWait = false;
}

void Imagine::readConfigFiles(const QString &file1, const QString &file2)
{
    if (!file1.isEmpty()) {
        readSettings(file1);
        readComments(file1);
    }
    if (slaveImagine != NULL) {
        if (!file2.isEmpty()) {
            slaveImagine->readSettings(file2);
            slaveImagine->readComments(file2);
        }
        duplicateParameters(&slaveImagine->ui);
    }
    else {
        if (ui.cbBothCamera->isChecked())
            ui.cbBothCamera->setChecked(false);
    }
}

bool Imagine::outputFileValidCheck()
{
    //warn user if overwrite file:
    if (QFile::exists(ui.lineEditFilename->text())) {
        if (QMessageBox::question( this,
            tr("Overwrite File? -- Imagine"),
            tr("The file called \n   %1\n already exists. "
                "Do you want to overwrite it?").arg(dataAcqThread->headerFilename),
            tr("&Yes"), tr("&No"),
            QString(), 0, 1)) {
            return false;
        }
        else { // want overwrite
            QFile::remove(ui.lineEditFilename->text());
        }
    }
    if (!CheckAndMakeFilePath(ui.lineEditFilename->text())) {
        QMessageBox::information(this, "File creation error.",
            "Unable to create the directory");
        return false;
    }
    return true;
}

void Imagine::configValidityCheck(const QString &file1, const QString &file2)
{
    readConfigFiles(file1, file2);
    bool retVal = outputFileValidCheck();
    if(slaveImagine != NULL)
        retVal &= slaveImagine->outputFileValidCheck();
    scriptApplyAndReport(retVal);
}

void Imagine::loadConfig(const QString &file1, const QString &file2)
{
    readConfigFiles(file1, file2);
    scriptJustReport(true);
}

void Imagine::loadWaveform(const QString &file)
{
    readControlWaveformFile(file);
    scriptJustReport(true);
}

void Imagine::setFilename(const QString &file1, const QString &file2)
{
    ui.lineEditFilename->setText(file1);
    if(slaveImagine != NULL)
        slaveImagine->ui.lineEditFilename->setText(file2);
    scriptJustReport(true);
}

void Imagine::configRecord()
{
    reqFromScript = true;
    on_actionStartAcqAndSave_triggered();
}
#pragma endregion

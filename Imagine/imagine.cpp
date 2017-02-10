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
extern DaqDo* digOut;

// misc vars
bool zoom_isMouseDown = false;
QPoint zoom_downPos, zoom_curPos; //in the unit of displayed image
int nUpdateImage;


#pragma region LIFECYCLE

Imagine::Imagine(Camera *cam, Positioner *pos, Laser *laser, Imagine *mImagine, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    m_OpenDialogLastDirectory = QDir::homePath();
    masterImagine = mImagine;
    dataAcqThread = new DataAcqThread(QThread::NormalPriority, cam, pos, this, parent);

    qDebug() << QString("created data acq thread");

    // set up the pixmap making thread (see dtor for cleanup)
    pixmapper = new Pixmapper();
    pixmapper->moveToThread(&pixmapperThread);
    connect(&pixmapperThread, &QThread::finished, pixmapper, &QObject::deleteLater);
    connect(this, &Imagine::makePixmap, pixmapper, &Pixmapper::handleImg);
    connect(pixmapper, &Pixmapper::pixmapReady, this, &Imagine::handlePixmap);
    pixmapperThread.start(QThread::IdlePriority);


    minPixelValueByUser = 0;
    maxPixelValueByUser = 1 << 16;

    ui.setupUi(this);
    //load user's preference/preset from js
    loadPreset();
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


    ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabAI));
    //remove positioner tab from slave window
    //TODO: Give stimulus tab the same treatment (It's in position #3)
    if (masterImagine != NULL) {
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabPiezo));
    }

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
    if (pos != NULL && pos->posType == ActuatorPositioner) {
        ui.comboBoxAcqTriggerMode->setEnabled(false);
        ui.comboBoxAcqTriggerMode->setCurrentIndex(1);
    }

    Camera& camera = *dataAcqThread->pCamera;

    ui.comboBoxHorReadoutRate->setEnabled(false);
    ui.comboBoxPreAmpGains->setEnabled(false);
    ui.comboBoxVertShiftSpeed->setEnabled(false);
    ui.comboBoxVertClockVolAmp->setEnabled(false);

    maxROIHSize = camera.getChipWidth();
    maxROIVSize = camera.getChipHeight();
 //   roiStepsHor = camera.getROIStepsHor(); // OCPI-II return 20
    roiStepsHor = 160;

    if (maxROIHSize == 0) {// this is for GUI test at dummy HW
        maxROIHSize = 2048;
        maxROIVSize = 2048;
        roiStepsHor = 160;
    }
    ui.spinBoxHend->setValue(maxROIHSize);
    ui.spinBoxVend->setValue(maxROIVSize);
    ui.spinBoxHstart->setMaximum(maxROIHSize);
    ui.spinBoxVstart->setMaximum(maxROIVSize/2);
    ui.spinBoxVend->setMinimum(maxROIVSize/2 + 1);
    ui.spinBoxHend->setMaximum(maxROIHSize);
    ui.spinBoxVend->setMaximum(maxROIVSize);
    ui.spinBoxHend->setValue(maxROIHSize);
    ui.spinBoxVend->setValue(maxROIVSize);
    ui.spinBoxHend->setSingleStep(roiStepsHor);
    ui.spinBoxHstart->setSingleStep(roiStepsHor);
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
        this, SLOT(updateDisplay(const QByteArray &, long, int, int)));
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
        connect(lineedits[i], SIGNAL(textChanged(const QString&)),
            this, SLOT(onModified()));
    }

    auto spinboxes = ui.tabWidgetCfg->findChildren<QSpinBox*>();
    for (int i = 0; i < spinboxes.size(); ++i){
        connect(spinboxes[i], SIGNAL(valueChanged(const QString&)),
            this, SLOT(onModified()));
    }

    auto doublespinboxes = ui.tabWidgetCfg->findChildren<QDoubleSpinBox*>();
    for (int i = 0; i < doublespinboxes.size(); ++i){
        connect(doublespinboxes[i], SIGNAL(valueChanged(const QString&)),
            this, SLOT(onModified()));
    }

    auto comboboxes = ui.tabWidgetCfg->findChildren<QComboBox*>();
    for (int i = 0; i < comboboxes.size(); ++i){
        connect(comboboxes[i], SIGNAL(currentIndexChanged(const QString&)),
            this, SLOT(onModified()));
    }

    auto checkboxes = ui.tabWidgetCfg->findChildren<QCheckBox*>();
    for (auto cb : checkboxes){
        connect(cb, SIGNAL(stateChanged(int)),
            this, SLOT(onModified()));
    }

    on_spinBoxSpinboxSteps_valueChanged(ui.spinBoxSpinboxSteps->value());

    /* for laser control from this line */
    // pass laser object to a new thread
    if ((masterImagine == NULL)&&(laser->getDeviceName() != "nidaq")) {
        QString portName;
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
    }
    else {
        ui.tabWidgetCfg->removeTab(ui.tabWidgetCfg->indexOf(ui.tabLaser));
        ui.actionOpenShutter->setEnabled(false);
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
}

Imagine::~Imagine()
{
    // this isn't really the logical place to do this...
    if (digOut != NULL)
    delete digOut;
    digOut = NULL;

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
}

#pragma endregion


#pragma region DRAWING

void Imagine::updateImage()
{
    if (lastRawImg.isNull() || isPixmapping) return;
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

int counts[1 << 16];
void Imagine::updateHist(const Camera::PixelValue * frame,
    const int imageW, const int imageH)
{
    //initialize the counts to all 0's
    int ncounts = sizeof(counts) / sizeof(counts[0]);
    for (unsigned int i = 0; i < ncounts; ++i) counts[i] = 0;

    for (unsigned int i = 0; i < imageW*imageH; ++i){
        counts[*frame++]++;
    }

    // Autoscale
    unsigned int maxintensity;
    for (maxintensity = ncounts - 1; maxintensity >= 0; maxintensity--)
        if (counts[maxintensity])
            break;

    //now binning
	int nBins = histPlot->width() / 3; //3 display pixels per bin?
    int totalWidth = maxintensity; //1<<14;
    double binWidth = double(totalWidth) / nBins;

    QVector<QwtIntervalSample> intervals(nBins);

    double maxcount = 0;
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

    histPlot->setAxisScale(QwtPlot::yLeft, 1, maxcount);
    histPlot->setAxisScale(QwtPlot::xBottom, 0.0, maxintensity);
    histPlot->replot();

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

/*
void Imagine::updateConWave(const int frameIdx, const int value)
{
    //TODO: if not visible, just return
    conPiezoCurveData->append(frameIdx, value);
    conPiezoCurve->setData(conPiezoCurveData);
    conWavPlot->setAxisScale(QwtPlot::xBottom, conPiezoCurveData->left(), conPiezoCurveData->right());
    conWavPlot->replot();
}//updateConWave()
*/

void prepareCurve(QwtPlotCurve *curve, QwtPlot *plot,
    QwtText &xTitle, QwtText &yTitle, QColor color)
{
    xTitle.setFont(QFont("Helvetica", 10));
    yTitle.setFont(QFont("Helvetica", 10));
    plot->setAxisTitle(QwtPlot::xBottom, xTitle); //TODO: stack number too
    plot->setAxisTitle(QwtPlot::yLeft, yTitle);
    plot->setAxisScale(QwtPlot::xBottom, 0, 1000, 100);
    plot->setAxisScale(QwtPlot::yLeft, 0, 400, 100);
    plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());
    plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());

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
    ui.dwHist->setWidget(histPlot); //setWidget() causes the widget using all space
    histPlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine(10));

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(QPen(Qt::black, 0, Qt::DotLine));
    grid->setMinorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(histPlot);

    histogram = new QwtPlotHistogram(); // HistogramItem();
    histogram->setPen(Qt::darkCyan);
    histogram->setBrush(QBrush(Qt::darkCyan));
    histogram->setBaseline(1.0); //baseline will be subtracted from pixel intensities
    histogram->setItemAttribute(QwtPlotItem::AutoScale, true);
    histogram->setItemAttribute(QwtPlotItem::Legend, true);
    histogram->setZ(20.0);
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->attach(histPlot);

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
    ui.dwIntenCurve->setWidget(intenPlot);
    intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number"); //TODO: stack number too
    intenPlot->setAxisTitle(QwtPlot::yLeft, "avg intensity");
    intenCurve = new QwtPlotCurve("avg intensity");

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

    //the control waveform
    QwtText xTitle("Sample index");
    QwtText yTitle("Position");
    conWavPlot = new QwtPlot;// (ui.frameConWav);
    conPiezoCurve = new QwtPlotCurve("Positioner waveform");
    conShutterCurve = new QwtPlotCurve("Shutter waveform");
    piezoSpeedCurve = new QwtPlotCurve("Positioner max speed");
    prepareCurve(conPiezoCurve, conWavPlot, xTitle, yTitle, Qt::red);
    prepareCurve(conShutterCurve, conWavPlot, xTitle, yTitle, Qt::blue);
    prepareCurve(piezoSpeedCurve, conWavPlot, xTitle, yTitle, Qt::green);
    ui.conWavLayout->addWidget(conWavPlot);

    //the read out waveform
    conReadWavPlot = new QwtPlot;// (ui.frameConWav);
    conReadPiezoCurve = new QwtPlotCurve("Piezo waveform");
    conReadStimuliCurve = new QwtPlotCurve("Stimuli waveform");
    conReadCameraCurve = new QwtPlotCurve("Camera waveform");
    conReadHeartbeatCurve = new QwtPlotCurve("Hearbeat waveform");
    prepareCurve(conReadPiezoCurve, conReadWavPlot, xTitle, yTitle, Qt::red);
    prepareCurve(conReadStimuliCurve, conReadWavPlot, xTitle, yTitle, Qt::green);
    prepareCurve(conReadCameraCurve, conReadWavPlot, xTitle, yTitle, Qt::blue);
    prepareCurve(conReadHeartbeatCurve, conReadWavPlot, xTitle, yTitle, Qt::black);
    ui.conWavLayout->addWidget(conWavPlot);
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
    dataAcqThread->isUpdatingImage = true;

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
        updateHist(frame, imageW, imageH);
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
        minPixelValue = maxPixelValue = 0;
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

    updateImage();

    nUpdateImage++;
    appendLog(QString().setNum(nUpdateImage)
        + "-th updated frame(0-based)=" + QString().setNum(idx));

done:
    dataAcqThread->isUpdatingImage = false;
}//updateDisplay(),

void Imagine::updateStatus(const QString &str)
{
    if (str == "acq-thread-finish"){
        updateStatus(eIdle, eNoAction);
        return;
    }

    this->statusBar()->showMessage(str);
    appendLog(str);

    if (str.startsWith("valve", Qt::CaseInsensitive)){
        ui.tableWidgetStimDisplay->setCurrentCell(0, dataAcqThread->curStimIndex);
    }//if, update stimulus display
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
        ui.comboBoxAcqTriggerMode->setCurrentIndex(ttStr == "internal");
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
    appendLog("released");
    if (event->button() == Qt::LeftButton && zoom_isMouseDown) {
        zoom_curPos = event->pos();
        zoom_isMouseDown = false;

        appendLog(QString("last pos: (%1, %2); cur pos: (%3, %4)")
            .arg(zoom_downPos.x()).arg(zoom_downPos.y())
            .arg(zoom_curPos.x()).arg(zoom_curPos.y()));

        if (pixmap.isNull()) return; //NOTE: this is for not zooming logo image

        //update L,T,W,H
        QPoint LT = calcPos(zoom_downPos);
        QPoint RB = calcPos(zoom_curPos);

        if (LT.x() > RB.x() && LT.y() > RB.y()){
            L = -1;
            goto done;
        }//if, should show full image

        if (!(LT.x() < RB.x() && LT.y() < RB.y())) return; //unknown action

        L = LT.x(); T = LT.y();
        W = RB.x() - L; H = RB.y() - T;

    done:
        //refresh the image
        updateImage();
    }//if, dragging

}

void Imagine::on_actionDisplayFullImage_triggered()
{
    L = -1;
    updateImage();

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
            updateImage();
        }
    }
}

void Imagine::on_actionStartAcqAndSave_triggered()
{
    if (!paramOK){
        QMessageBox::information(this, "Wrong parameters --- Imagine",
            "Please correct the parameters.");
        return;
    }

    if (dataAcqThread->headerFilename == ""){
        if (ui.lineEditFilename->text() != ""){
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
        QString(), 0, 1)){
        return;
    }//if, file exists and user don't want to overwrite it

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

    if (!CheckAndMakeFilePath(dataAcqThread->headerFilename)) {
        QMessageBox::information(this, "File creation error.",
            "Unable to create the directory");
    }

    nUpdateImage = 0;
    minPixelValue = maxPixelValue = -1;

    dataAcqThread->applyStim = ui.cbStim->isChecked();
    if (dataAcqThread->applyStim){
        dataAcqThread->curStimIndex = 0;
    }

    intenCurveData->clear();

    dataAcqThread->isLive = false;
    dataAcqThread->startAcq();
    updateStatus(eRunning, eAcqAndSave);
}

void Imagine::on_actionStartLive_triggered()
{
    if (!paramOK){
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

    intenCurveData->clear();

    dataAcqThread->isLive = true;
    dataAcqThread->startAcq();
    updateStatus(eRunning, eLive);
}

void Imagine::on_actionAutoScaleOnFirstFrame_triggered()
{
    minPixelValue = maxPixelValue = -1;
}

void Imagine::on_actionContrastMin_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the min pixel value to cut off", "min", minPixelValueByUser, 0, 1 << 16, 10, &ok);
    if (ok) minPixelValueByUser = value;
}

void Imagine::on_actionContrastMax_triggered()
{
    bool ok;
    int value = QInputDialog::getInt(this, "please specify the max pixel value to cut off", "max", maxPixelValueByUser, 0, 1 << 16, 10, &ok);
    if (ok) maxPixelValueByUser = value;
}

void Imagine::on_actionStop_triggered()
{
    dataAcqThread->stopAcq();
    updateStatus(eStopping, curAction);
}

void Imagine::on_actionOpenShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //open laser shutter
    digOut->updateOutputBuf(4, true);
    digOut->write();
/*
    QString portName;
    if (!(laserCtrlSerial->isPortOpen())) {
       emit openLaserSerialPort(portName);
    }

    changeLaserShutters();
    for (int i = 1; i <= 4; i++)
        changeLaserTrans(true, i);
*/
    QString str = QString("Open laser shutter");
    appendLog(str);

    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(true);
}

void Imagine::on_actionCloseShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //open laser shutter
    digOut->updateOutputBuf(4, false);
    digOut->write();
/*
    emit setLaserShutters(0);
*/
    QString str = QString("Close laser shutter");
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

    event->accept();
}

void Imagine::on_btnFullChipSize_clicked()
{
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

    if (image.width() != 1004 || image.height() != 1002){
        QMessageBox::warning(this, tr("Imagine"),
            tr("Please acquire one full chip image first"));
        return;
    }

    if (L == -1) {
        on_btnFullChipSize_clicked();
        return;
    }//if, full image
    else {
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
    ui.spinBoxVend->setValue(maxROIVSize + 1 - newValue);
}


void Imagine::on_spinBoxVend_valueChanged(int newValue)
{
    ui.spinBoxVstart->setValue(maxROIVSize + 1 - newValue);
}

void Imagine::on_btnApply_clicked()
{
    if (curStatus != eIdle){
        QMessageBox::information(this, "Please wait --- Imagine",
            "Please click 'apply' when the camera is idle.");

        return;
    }

    Camera* camera = dataAcqThread->pCamera;

    QString acqTriggerModeStr = ui.comboBoxAcqTriggerMode->currentText();
 	QString expTriggerModeStr = ui.comboBoxExpTriggerMode->currentText();
    Camera::AcqTriggerMode acqTriggerMode;
	Camera::ExpTriggerMode expTriggerMode;
    if (acqTriggerModeStr == "External") acqTriggerMode = Camera::eExternal;
	else if (acqTriggerModeStr == "Internal")  acqTriggerMode = Camera::eInternalTrigger;
    else {
        assert(0); //if code goes here, it is a bug
    }
	if (expTriggerModeStr == "External Start") expTriggerMode = Camera::eExternalStart;
	else if (expTriggerModeStr == "Auto")  expTriggerMode = Camera::eAuto;
	else if (expTriggerModeStr == "External Control")  expTriggerMode = Camera::eExternalControl;
	else {
		assert(0); //if code goes here, it is a bug
	}
    dataAcqThread->acqTriggerMode = acqTriggerMode;
	dataAcqThread->expTriggerMode = expTriggerMode;

    //TODO: fix this hack

    //QPointer<Ui_ImagineClass>
    Ui_ImagineClass* temp_ui = NULL;
    if (masterImagine == NULL) {
        temp_ui = &ui;
    }
    else {
        temp_ui = &masterImagine->ui;
    }

    dataAcqThread->piezoStartPosUm = (*temp_ui).doubleSpinBoxStartPos->value();
    dataAcqThread->piezoStopPosUm = (*temp_ui).doubleSpinBoxStopPos->value();
    dataAcqThread->piezoTravelBackTime = (*temp_ui).doubleSpinBoxPiezoTravelBackTime->value();

    dataAcqThread->isBiDirectionalImaging = ui.cbBidirectionalImaging->isChecked();

    dataAcqThread->nStacks = ui.spinBoxNumOfStacks->value();
    dataAcqThread->nFramesPerStack = ui.spinBoxFramesPerStack->value();
    dataAcqThread->exposureTime = ui.doubleSpinBoxExpTime->value();
    dataAcqThread->idleTimeBwtnStacks = ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();

    dataAcqThread->horShiftSpeedIdx = ui.comboBoxHorReadoutRate->currentIndex();
    dataAcqThread->preAmpGainIdx = ui.comboBoxPreAmpGains->currentIndex();
    dataAcqThread->gain = ui.spinBoxGain->value();
    dataAcqThread->verShiftSpeedIdx = ui.comboBoxVertShiftSpeed->currentIndex();
    dataAcqThread->verClockVolAmp = ui.comboBoxVertClockVolAmp->currentIndex();

    dataAcqThread->preAmpGain = ui.comboBoxPreAmpGains->currentText();
    dataAcqThread->horShiftSpeed = ui.comboBoxHorReadoutRate->currentText();
    dataAcqThread->verShiftSpeed = ui.comboBoxVertShiftSpeed->currentText();

    if (ui.cbPositionerWav->isChecked()) {
        if (conPiezoCurveData&&conShutterCurveData) {
            dataAcqThread->isUsingWav = true;
            dataAcqThread->conPiezoWavData = conPiezoCurveData;
            dataAcqThread->conShutterWavData = conShutterCurveData;
            dataAcqThread->sampleRate = ui.spinBoxPiezoSampleRate->value();
            dataAcqThread->acqTriggerMode = Camera::eExternal;
            if (dataAcqThread->isUsingWav) { // check conPiezoCurveData
                Positioner *pos = dataAcqThread->pPositioner;
                double distance, time, speed;
                int dataSize = conPiezoCurveData->size();
                for (int i = 0; i < dataSize - 5; i++) {
                    int ii = i%dataSize;
                    distance = (conPiezoCurveData->y(ii + 5) - conPiezoCurveData->y(ii));
                    time = (conPiezoCurveData->x(ii + 5) - conPiezoCurveData->x(ii)) / dataAcqThread->sampleRate; // sec
                    speed = distance / time; // piezostep/sec
                    if ((distance != 1) && (abs(speed) > pos->maxSpeed())) {
                        QMessageBox::critical(this, "Imagine", "Positioner control is too fast"
                            , QMessageBox::Ok, QMessageBox::NoButton);
                        return;
                    }
                }
                dataAcqThread->acqTriggerMode = Camera::eInternalTrigger;
                dataAcqThread->expTriggerMode = Camera::eExternalStart;
                dataAcqThread->nStacks = ui.spinBoxNumOfStacksWav->value();
                dataAcqThread->nFramesPerStack = ui.spinBoxFramesPerStackWav->value();
                dataAcqThread->pPositioner->setScanRateAo(ui.spinBoxPiezoSampleRate->value());
                dataAcqThread->exposureTime = ui.doubleSpinBoxExpTimeWav->value();
            }
        }
        else {
            QMessageBox::critical(this, "Imagine", "Positioner and shutter waveforms are not correctly loaded"
                , QMessageBox::Ok, QMessageBox::NoButton);
            return;
        }
    }
    else {
        dataAcqThread->isUsingWav = false;
        dataAcqThread->pPositioner->setScanRateAo(10000); // Hard coded
    }

    //params for binning
    dataAcqThread->hstart = camera->hstart = ui.spinBoxHstart->value();
    dataAcqThread->hend = camera->hend = ui.spinBoxHend->value();
    dataAcqThread->vstart = camera->vstart = ui.spinBoxVstart->value();
    dataAcqThread->vend = camera->vend = ui.spinBoxVend->value();
    dataAcqThread->angle = ui.spinBoxAngle->value();

    dataAcqThread->umPerPxlXy = ui.doubleSpinBoxUmPerPxlXy->value();

    camera->updateImageParams(dataAcqThread->nStacks, dataAcqThread->nFramesPerStack); //transfer image params to camera object

    //enforce #imageSizeBytes is x times of 16
    if (camera->imageSizeBytes % 16) {
        QMessageBox::critical(this, "Imagine", "ROI spec is wrong (#pixels per frame is not x times of 8)."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return;
    }

    if (!checkRoi()){
        QMessageBox::critical(this, "Imagine", "ROI spec is wrong."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return;
    }

    if (camera->vendor == "avt"){
        if (dataAcqThread->exposureTime < 1 / 57.0){
            QMessageBox::critical(this, "Imagine", "exposure time is too small."
                , QMessageBox::Ok, QMessageBox::NoButton);

            return;
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
            dataAcqThread->acqTriggerMode, //CookeCamera::eInternalTrigger  //use internal trigger
			dataAcqThread->expTriggerMode
            );
        dataAcqThread->cycleTime = camera->getCycleTime();
        updateStatus(QString("Camera: applied params: ") + camera->getErrorMsg().c_str());
        if (!paramOK) goto skip;
    }

    //get the real params used by the camera:
    dataAcqThread->cycleTime = camera->getCycleTime();
    cout << "cycle time is: " << dataAcqThread->cycleTime << endl;

    // if applicable, make sure positioner preparation went well...
    Positioner *pos = dataAcqThread->pPositioner;
    bool useTrig = true;
    if (pos != NULL && !dataAcqThread->preparePositioner(true, useTrig)){
        paramOK = false;
        QString msg = QString("Positioner: applied params failed: ") + pos->getLastErrorMsg().c_str();
        updateStatus(msg);
        QMessageBox::critical(0, "Imagine: Failed to setup piezo/stage.", msg
            , QMessageBox::Ok, QMessageBox::NoButton);
    }

skip:

    //set filenames:
    QString headerFilename = ui.lineEditFilename->text();
    dataAcqThread->headerFilename = headerFilename;
    if (!headerFilename.isEmpty()){
        dataAcqThread->aiFilename = replaceExtName(headerFilename, "ai");
        dataAcqThread->camFilename = replaceExtName(headerFilename, "cam");
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

    modified = false;
    ui.btnApply->setEnabled(modified);
}

void Imagine::on_btnOpenStimFile_clicked()
{
    QString stimFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a stimulus file to open",
        "",
        "Stimulus files (*.stim);;All files(*.*)");
    if (stimFilename.isEmpty()) return;

    ui.lineEditStimFile->setText(stimFilename);

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

	double min_s_needed = (stop - start) / max_speed;
	double cur_idle_time = (double)ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();
    if (masterImagine == NULL) {
        cur_travel_back = (double)ui.doubleSpinBoxPiezoTravelBackTime->value();
    }
    else {
        cur_travel_back = (double)masterImagine->ui.doubleSpinBoxPiezoTravelBackTime->value();
    }
	//reset to respect maximum speed limit if necessary
	double safe_idle_time = max(min_s_needed, cur_idle_time);
	double safe_travel_back = max(min_s_needed, cur_travel_back);
	ui.doubleSpinBoxPiezoTravelBackTime->setValue(safe_travel_back);
	ui.doubleSpinBoxBoxIdleTimeBtwnStacks->setValue(safe_idle_time);
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
        checkBox->setText(wl);
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
    changeLaserTrans(true, 1);
}

void Imagine::on_aotfLine2_valueChanged()
{
    ui.doubleSpinBox_aotfLine2->setValue((double)(ui.aotfLine2->value()) / 10.);
    changeLaserTrans(true, 2);
}

void Imagine::on_aotfLine3_valueChanged()
{
    ui.doubleSpinBox_aotfLine3->setValue((double)(ui.aotfLine3->value()) / 10.);
    changeLaserTrans(true, 3);
}

void Imagine::on_aotfLine4_valueChanged()
{
    ui.doubleSpinBox_aotfLine4->setValue((double)(ui.aotfLine4->value()) / 10.);
    changeLaserTrans(true, 4);
}

void Imagine::on_aotfLine5_valueChanged()
{
    ui.doubleSpinBox_aotfLine5->setValue((double)(ui.aotfLine5->value()) / 10.);
    changeLaserTrans(true, 5);
}

void Imagine::on_aotfLine6_valueChanged()
{
    ui.doubleSpinBox_aotfLine6->setValue((double)(ui.aotfLine6->value()) / 10.);
    changeLaserTrans(true, 6);
}

void Imagine::on_aotfLine7_valueChanged()
{
    ui.doubleSpinBox_aotfLine7->setValue((double)(ui.aotfLine7->value()) / 10.);
    changeLaserTrans(true, 7);
}

void Imagine::on_aotfLine8_valueChanged()
{
    ui.doubleSpinBox_aotfLine8->setValue((double)(ui.aotfLine8->value()) / 10.);
    changeLaserTrans(true, 8);
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
    WRITE_SETTING(prefs, spinBoxNumOfStacks);
    WRITE_SETTING(prefs, spinBoxFramesPerStack);
    WRITE_SETTING(prefs, doubleSpinBoxExpTime);
    WRITE_SETTING(prefs, doubleSpinBoxBoxIdleTimeBtwnStacks);
    WRITE_COMBO_SETTING(prefs, comboBoxAcqTriggerMode);
    WRITE_COMBO_SETTING(prefs, comboBoxExpTriggerMode);
    WRITE_SETTING(prefs, spinBoxAngle);
    WRITE_SETTING(prefs, doubleSpinBoxUmPerPxlXy);
    WRITE_COMBO_SETTING(prefs, comboBoxHorReadoutRate);
    WRITE_COMBO_SETTING(prefs, comboBoxPreAmpGains);
    WRITE_SETTING(prefs, spinBoxGain);
    WRITE_COMBO_SETTING(prefs, comboBoxVertShiftSpeed);
    WRITE_COMBO_SETTING(prefs, comboBoxVertClockVolAmp);
    prefs.endGroup();

    prefs.beginGroup("Binning");
    WRITE_SETTING(prefs, spinBoxHstart);
    WRITE_SETTING(prefs, spinBoxHend);
    WRITE_SETTING(prefs, spinBoxVstart);
    WRITE_SETTING(prefs, spinBoxVend);
    prefs.endGroup();

    prefs.beginGroup("Stimuli");
    WRITE_CHECKBOX_SETTING(prefs, cbStim);
    WRITE_STRING_SETTING(prefs, lineEditStimFile);
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
        WRITE_COMBO_SETTING(prefs, comboBoxPositionerOwner);
        prefs.endGroup();
    }

    prefs.beginGroup("Display");
    WRITE_SETTING(prefs, spinBoxDisplayAreaSize);
    prefs.endGroup();

    prefs.beginGroup("Advanced");
    WRITE_CHECKBOX_SETTING(prefs, cbPositionerWav);
    WRITE_SETTING(prefs, spinBoxPiezoSampleRate);
    WRITE_SETTING(prefs, spinBoxNumOfStacks);
    WRITE_SETTING(prefs, spinBoxFramesPerStack);
    WRITE_STRING_SETTING(prefs, lineEditPiezoWaveFile);
    WRITE_STRING_SETTING(prefs, lineEditShutterWaveFile);
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Laser");
        WRITE_CHECKBOX_SETTING(prefs, cbLine1);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine1);
        WRITE_CHECKBOX_SETTING(prefs, cbLine2);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine2);
        WRITE_CHECKBOX_SETTING(prefs, cbLine3);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine3);
        WRITE_CHECKBOX_SETTING(prefs, cbLine4);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine4);
        WRITE_CHECKBOX_SETTING(prefs, cbLine5);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine5);
        WRITE_CHECKBOX_SETTING(prefs, cbLine6);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine6);
        WRITE_CHECKBOX_SETTING(prefs, cbLine7);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine7);
        WRITE_CHECKBOX_SETTING(prefs, cbLine8);
        WRITE_SETTING(prefs, doubleSpinBox_aotfLine8);
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

    prefs.beginGroup("Camera");
    READ_SETTING(prefs, spinBoxNumOfStacks, ok, i, 3, Int);
    READ_SETTING(prefs, spinBoxFramesPerStack, ok, i, 100, Int);
    READ_SETTING(prefs, doubleSpinBoxExpTime, ok, d, 0.0107, Double);
    READ_SETTING(prefs, doubleSpinBoxBoxIdleTimeBtwnStacks, ok, d, 0.700, Double);
    READ_COMBO_SETTING(prefs, comboBoxAcqTriggerMode, 1);
    READ_COMBO_SETTING(prefs, comboBoxExpTriggerMode, 0);
    READ_SETTING(prefs, spinBoxAngle, ok, i, 0, Int);
    READ_SETTING(prefs, doubleSpinBoxUmPerPxlXy, ok, d, -1.0000, Double);
    READ_COMBO_SETTING(prefs, comboBoxHorReadoutRate, 0);
    READ_COMBO_SETTING(prefs, comboBoxPreAmpGains, 0);
    READ_SETTING(prefs, spinBoxGain, ok, i, 0, Int);
    READ_COMBO_SETTING(prefs, comboBoxVertShiftSpeed, 0);
    READ_COMBO_SETTING(prefs, comboBoxVertClockVolAmp, 0);
    prefs.endGroup();

    prefs.beginGroup("Binning");
    READ_SETTING(prefs, spinBoxHstart, ok, i, 1, Int);
    READ_SETTING(prefs, spinBoxHend, ok, i, 2048, Int);
    READ_SETTING(prefs, spinBoxVstart, ok, i, 1, Int);
    READ_SETTING(prefs, spinBoxVend, ok, i, 2048, Int);
    prefs.endGroup();

    prefs.beginGroup("Stimuli");
    READ_BOOL_SETTING(prefs, cbStim, false); // READ_CHECKBOX_SETTING
    READ_STRING_SETTING(prefs, lineEditStimFile,"");
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
        READ_BOOL_SETTING(prefs, cbAutoSetPiezoTravelBackTime, false); // READ_CHECKBOX_SETTING
        READ_BOOL_SETTING(prefs, cbBidirectionalImaging, false); // READ_CHECKBOX_SETTING
        READ_COMBO_SETTING(prefs, comboBoxPositionerOwner, 0);
        prefs.endGroup();
    }

    prefs.beginGroup("Display");
    READ_SETTING(prefs, spinBoxDisplayAreaSize, ok, i, 0, Int);
    prefs.endGroup();

    prefs.beginGroup("Advanced");
    READ_BOOL_SETTING(prefs, cbPositionerWav, false);
    READ_SETTING(prefs, spinBoxPiezoSampleRate, ok, i, 10000, Int);
    WRITE_SETTING(prefs, spinBoxNumOfStacks, ok, i, 5, Int);
    WRITE_SETTING(prefs, spinBoxFramesPerStack, ok, i, 20, Int);
    READ_STRING_SETTING(prefs, lineEditPiezoWaveFile, "");
    READ_STRING_SETTING(prefs, lineEditShutterWaveFile, "");
    prefs.endGroup();

    if (masterImagine == NULL) {
        prefs.beginGroup("Laser");
        READ_BOOL_SETTING(prefs, cbLine1, true); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine1, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine2, true); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine2, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine3, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine3, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine4, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine4, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine5, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine5, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine6, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine6, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine7, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine7, ok, d, 50.0, Double);
        READ_BOOL_SETTING(prefs, cbLine8, false); // READ_CHECKBOX_SETTING
        READ_SETTING(prefs, doubleSpinBox_aotfLine8, ok, d, 50.0, Double);
        prefs.endGroup();
        ui.groupBoxLaser->setChecked(false);
        on_actionCloseShutter_triggered();
    }
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

    QString piezoFileName = ui.lineEditPiezoWaveFile->text();
    QString shutterFileName = ui.lineEditShutterWaveFile->text();
    updateControlWavefrom(piezoFileName, shutterFileName);

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

void Imagine::ControlFileLoad(QByteArray &data1, QByteArray &data2)
{
    BOOL data1_valid = (data1.size() != 0) ? true : false;
    BOOL data2_valid = (data2.size() != 0) ? true : false;
    QDataStream *dStream1, *dStream2;
    unsigned short us;
    vector <int> xpoint1;
    vector <int> xpoint2;
    vector <double> ypoint1;
    vector <double> ypoint2;
    double y1=0, y2=0, oldy1=0, oldy2=0, maxy=0;
    int width;
    int datasize;

    if(data1_valid){
        width = data1.size() / 2;
        conPiezoCurveData = new CurveData(width); // parameter should not be xpoint.size()
        dStream1 = new QDataStream(data1);
        dStream1->setByteOrder(QDataStream::LittleEndian);//BigEndian, LittleEndian
    }
    if (data2_valid) {
        width = data2.size() / 2;
        conShutterCurveData = new CurveData(width); // parameter should not be xpoint.size()
        dStream2 = new QDataStream(data2);
        dStream2->setByteOrder(QDataStream::LittleEndian);//BigEndian, LittleEndian
    }
    piezoSpeedCurveData = new CurveData(width);
    // first point
    if(data1_valid){
        (*dStream1) >> us; // piezo
        y1 = static_cast<double>(us);
        oldy1 = y1;
        if(y1 > maxy) maxy = y1;
        xpoint1.push_back(0);
        ypoint1.push_back(y1);
    }
    if(data2_valid){
        (*dStream2) >> us; // shutter
        y2 = static_cast<double>(us);
        oldy2 = y2;
        if (y2 > maxy) maxy = y2;
        xpoint2.push_back(0);
        ypoint2.push_back(y2);
    }
    // next points except the last one
    for (int i = 1; i < width-1; i++)
    {
        if (data1_valid) {
            (*dStream1) >> us; // piezo (.ai file)
            y1 = static_cast<double>(us);
            if (y1 > maxy) maxy = y1;
        }
        if (data2_valid) {
            (*dStream2) >> us; // piezo (.ai file)
            y2 = static_cast<double>(us);
            if (y2 > maxy) maxy = y2;
        }
        if ((oldy1 != y1) || (oldy2 != y2)) { // only save different one from previous point
            if (data1_valid) {
                if (xpoint1.back() != i - 1) {
                    xpoint1.push_back(i - 1);
                    ypoint1.push_back(y1);
                }
                xpoint1.push_back(i);
                ypoint1.push_back(y1);
            }
            if (data2_valid) {
                if (xpoint2.back() != i - 1) {
                    xpoint2.push_back(i - 1);
                    ypoint2.push_back(oldy2);
                }
                xpoint2.push_back(i);
                ypoint2.push_back(y2);
            }
            oldy1 = y1;
            oldy2 = y2;
        }
    }
    // last point is always saved
    if (data1_valid) {
        (*dStream1) >> us;
        y1 = static_cast<double>(us);
        if (y1 > maxy) maxy = y1;
        xpoint1.push_back(width-1);
        ypoint1.push_back(y1);
        datasize = xpoint1.size();
    }
    if (data2_valid) {
        (*dStream2) >> us;
        y2 = static_cast<double>(us);
        if (y2 > maxy) maxy = y2;
        xpoint2.push_back(width-1);
        ypoint2.push_back(y2);
        datasize = xpoint2.size();
    }

    for (int i = 0; i < datasize; i++)
    {
        if ((conPiezoCurveData) && (data1_valid))
            conPiezoCurveData->append(static_cast<double>(xpoint1[i]), ypoint1[i]);
        if ((conShutterCurveData) && (data2_valid))
            conShutterCurveData->append(static_cast<double>(xpoint2[i]), ypoint2[i]);
    }

    if ((conPiezoCurveData) && (data1_valid)) {
        conPiezoCurve->setData(conPiezoCurveData);
        conWavPlot->setAxisScale(QwtPlot::xBottom, conPiezoCurveData->left(), conPiezoCurveData->right());
    }
    if ((conShutterCurveData) && (data2_valid)) {
        conShutterCurve->setData(conShutterCurveData);
        conWavPlot->setAxisScale(QwtPlot::xBottom, conShutterCurveData->left(), conShutterCurveData->right());
    }
    updataSpeedData(ui.spinBoxPiezoSampleRate->value());
    piezoSpeedCurve->setData(piezoSpeedCurveData);
    conWavPlot->setAxisScale(QwtPlot::yLeft, 0, maxy);
    conWavPlot->replot();
}

void Imagine::updataSpeedData(int newValue)
{
    double xMax = conPiezoCurveData->right();
    Positioner *pos = dataAcqThread.pPositioner;
    double maxSpeed = pos->maxSpeed();
    double sampleRate = static_cast<double>(newValue);
    double piezoSpeedCurveMax = maxSpeed / sampleRate * xMax;
    piezoSpeedCurveData->clear();
    piezoSpeedCurveData->append(0., 0.);
    piezoSpeedCurveData->append(xMax, piezoSpeedCurveMax);
    conWavPlot->replot();
}

bool Imagine::updateControlWavefrom(QString fn1, QString fn2)
{
    QByteArray data1, data2;
    QFile file1(fn1);
    if (file1.open(QFile::ReadOnly)) {
        data1 = file1.readAll();
        file1.close();
        QFile file2(fn2);
        if (file2.open(QFile::ReadOnly)) {
            data2 = file2.readAll();
            file2.close();
            if (data1.size() == data2.size()) {
                ControlFileLoad(data1, data2);
                return true;
            }
        }
    }
    return false;
}

void Imagine::on_btnShutterWavOpen_clicked()
{
    // Read shutter file
    QString wavFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a shutter control waveform file to open",
        m_OpenDialogLastDirectory,
        "Waveform files (*.wvf);;All files(*.*)");
    if (wavFilename.isEmpty()) return;

    ui.lineEditShutterWaveFile->setText(wavFilename);
    QFileInfo fi(wavFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();

    QFile file2(wavFilename);
    if (!file2.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(wavFilename)
            .arg(file2.errorString()));
        return;
    }
    QByteArray data1, data2;
    data2 = file2.readAll();
    file2.close();
    
    // Read piezo file
    QFile file1(ui.lineEditPiezoWaveFile->text());
    if (file1.open(QFile::ReadOnly)) {
        data1 = file1.readAll();
        if (data1.size() != data2.size()) {
            QMessageBox::critical(this, "Imagine", "Piezo waveform and shtter waveform are different in their sample size"
                , QMessageBox::Ok, QMessageBox::NoButton);
            return;
        }
    }

    ControlFileLoad(data1, data2);
}

void Imagine::on_btnPiezoWavOpen_clicked()
{
    // Read piezo file
    QString wavFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a piezo control waveform file to open",
        m_OpenDialogLastDirectory,
        "Waveform files (*.wvf);;All files(*.*)");
    if (wavFilename.isEmpty()) return;

    ui.lineEditPiezoWaveFile->setText(wavFilename);
    QFileInfo fi(wavFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();

    QFile file1(wavFilename);
    if (!file1.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(wavFilename)
            .arg(file1.errorString()));
        return;
    }
    QByteArray data1, data2;
    data1 = file1.readAll();
    file1.close();

    // Read shutter file
    QFile file2(ui.lineEditShutterWaveFile->text());
    if (file2.open(QFile::ReadOnly)) {
        data2 = file2.readAll();
        if (data1.size() != data2.size()) {
            QMessageBox::critical(this, "Imagine", "Piezo waveform and shtter waveform are different in their sample size"
                , QMessageBox::Ok, QMessageBox::NoButton);
            return;
        }
    }

    ControlFileLoad(data1, data2);
}

void Imagine::on_btnReadWavOpen_clicked()
{
    QString wavFilename = QFileDialog::getOpenFileName(
        this,
        "Choose a read out waveform file to open",
        m_OpenDialogLastDirectory, 
        "Waveform files (*.ai);;All files(*.*)");
    if (wavFilename.isEmpty()) return;

    ui.lineEditReadWaveformFile->setText(wavFilename);
    QFileInfo fi(wavFilename);
    m_OpenDialogLastDirectory = fi.absolutePath();

    QFile file(wavFilename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Imagine"),
            tr("Cannot read file %1:\n%2.")
            .arg(wavFilename)
            .arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    QDataStream dStream(data);
    dStream.setByteOrder(QDataStream::LittleEndian);//BigEndian, LittleEndian
    conReadPiezoCurveData = new CurveData(data.size() / 8);
    conReadStimuliCurveData = new CurveData(data.size() / 8);
    conReadCameraCurveData = new CurveData(data.size() / 8);
    conReadHeartbeatCurveData = new CurveData(data.size() / 8);
    double maxy = 0, miny = INFINITY;
    for (int i = 0; i < data.size() / 8; i++)
    {
        unsigned short us;
        dStream >> us; // piezo (.ai file)
        double y = static_cast<double>(us)/100;
        if (y > maxy) maxy = y;
        if (y < miny) miny = y;
        conReadPiezoCurveData->append(static_cast<double>(i), y);
        dStream >> us; // stimuli (.ai file)
        y = static_cast<double>(us)/400;
        if (y > maxy) maxy = y;
        if (y < miny) miny = y;
        conReadStimuliCurveData->append(static_cast<double>(i), y);
        dStream >> us; // camera frame TTL (.ai file)
        y = static_cast<double>(us)/400;
        if (y > maxy) maxy = y;
        if (y < miny) miny = y;
        conReadCameraCurveData->append(static_cast<double>(i), y);
        dStream >> us; // heartbeat (.ai file)
        y = static_cast<double>(us)/400;
        if (y > maxy) maxy = y;
        if (y < miny) miny = y;
        conReadHeartbeatCurveData->append(static_cast<double>(i), y);
    }
    //QRectF bound = conPiezoCurveData->boundingRect();
    conReadPiezoCurve->setData(conReadPiezoCurveData);
    conReadStimuliCurve->setData(conReadStimuliCurveData);
    conReadCameraCurve->setData(conReadCameraCurveData);
    conReadHeartbeatCurve->setData(conReadHeartbeatCurveData);
    conReadWavPlot->setAxisScale(QwtPlot::yLeft, miny, maxy);
    conReadWavPlot->setAxisScale(QwtPlot::xBottom, conReadPiezoCurveData->left(), conReadPiezoCurveData->right());
    ui.cbPiezoReadWav->setChecked(true);
    ui.cbStimuliReadWav->setChecked(true);
    ui.cbCameraReadWav->setChecked(true);
    ui.cbHeartReadWav->setChecked(true);
    conReadPiezoCurve->show();
    conReadStimuliCurve->show();
    conReadCameraCurve->show();
    conReadHeartbeatCurve->show();
    conReadWavPlot->replot();

    //To make test control data 
    QFile file2("d:/piezo.wvf");
    QFile file3("d:/shutter.wvf");
    qint16 piezo, shutter;
    double piezom1,piezo0,piezop1,piezoavg=0.;

    if (file2.open(QFile::WriteOnly | QFile::Truncate)&& file3.open(QFile::WriteOnly | QFile::Truncate))
    {
        if (0) {
            piezop1 = (conReadPiezoCurveData->y(0) / 40 * 100);;
            piezo0 = piezop1;
            for (int i = 0; i < 70000; i++)//conReadPiezoCurveData->size()
            {
                int ii = i % 14000;
                piezom1 = piezo0;
                piezo0 = piezop1;
                piezop1 = (conReadPiezoCurveData->y(ii + 1) / 40 * 100);
                piezoavg = (piezom1 + piezo0 + piezop1) / 3.;
                piezo = static_cast<qint16>(piezoavg + 0.5);
                if ((ii >= 5000) && (ii <= 10000)) {
                    int j = ii % 50;
                    if ((j >= 0) && (j <= 30))
                        shutter = 150;
                    else
                        shutter = 0;
                }
                else
                    shutter = 0;
                file2.write((char *)(&piezo), sizeof(qint16));
                file3.write((char *)(&shutter), sizeof(qint16));
            }
        }
        else {
            int sampleRate = 6000; // 2000
            int totalSamples = 70000;
            int nStacks = 9; // 5
            int perStackSamples = totalSamples / nStacks;
            int piezoDelaySamples = 100; // 4000
            int piezoRigingSamples = 6000.;
            int piezoFallingSamples = 1000.;
            double piezoStartPos = 100.;
            double piezoStopPos = 400.;
            int nFrames = 20;
            int shutterControlMarginSamples = 500; // shutter control begin and end from piezo start and stop
            int stakShutterCtrlSamples = piezoRigingSamples - 2 * shutterControlMarginSamples;
            int frameShutterCtrlSamples = stakShutterCtrlSamples / nFrames;
            double exposureTime = 0.01; // sec : 0.008 ~ 0.012 is optimal
            int exposureSamples = static_cast<int>(exposureTime*static_cast<double>(sampleRate));
            if(exposureSamples>frameShutterCtrlSamples-50)
                appendLog("exposureSamples > frameShutterCtrlSamples-50");
            piezoavg = piezoStartPos;
            for (int i = 0; i < totalSamples; i++)
            {
                int ii = i % perStackSamples;
                if ((ii >= piezoDelaySamples) && (ii < piezoDelaySamples+ piezoRigingSamples)) {
                    piezoavg += (piezoStopPos- piezoStartPos)/ static_cast<double>(piezoRigingSamples);
                }
                if ((ii >= piezoDelaySamples + piezoRigingSamples) &&
                    (ii < piezoDelaySamples + piezoRigingSamples + piezoFallingSamples)) {
                    piezoavg -= (piezoStopPos - piezoStartPos) / static_cast<double>(piezoFallingSamples);
                }
                piezo = static_cast<qint16>(piezoavg + 0.5);
                if ((ii >= piezoDelaySamples + shutterControlMarginSamples) &&
                    (ii <= piezoDelaySamples + piezoRigingSamples - shutterControlMarginSamples)) {
                    int j = (ii- piezoDelaySamples + shutterControlMarginSamples) % frameShutterCtrlSamples;
                    if ((j > 0) && (j <= exposureSamples)) // (0.008~0.012sec)*(sampling rate) is optimal
                        shutter = 150;
                    else
                        shutter = 0;
                }
                else
                    shutter = 0;
                file2.write((char *)(&piezo), sizeof(qint16));
                file3.write((char *)(&shutter), sizeof(qint16));
            }
        }
        file2.close();
        file3.close();
    }
}

void Imagine::on_cbPiezoReadWav_clicked(bool checked)
{
    if (conReadPiezoCurve) {
        if (checked)
            conReadPiezoCurve->show();
        else
            conReadPiezoCurve->hide();
        conReadWavPlot->replot();
    }
}

void Imagine::on_cbStimuliReadWav_clicked(bool checked)
{
    if (conReadStimuliCurve) {
        if (checked)
            conReadStimuliCurve->show();
        else
            conReadStimuliCurve->hide();
        conReadWavPlot->replot();
    }
}

void Imagine::on_cbCameraReadWav_clicked(bool checked)
{
    if (conReadCameraCurve) {
        if (checked)
            conReadCameraCurve->show();
        else
            conReadCameraCurve->hide();
        conReadWavPlot->replot();
    }
}

void Imagine::on_cbHeartReadWav_clicked(bool checked)
{
    if (conReadHeartbeatCurve) {
        if (checked)
            conReadHeartbeatCurve->show();
        else
            conReadHeartbeatCurve->hide();
        conReadWavPlot->replot();
    }
}

void Imagine::on_spinBoxPiezoSampleRate_valueChanged(int newValue)
{
    if (piezoSpeedCurveData&&conPiezoCurveData) updataSpeedData(newValue);
}
#pragma endregion

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
//#include "curvedata.h"

#include <vector>
#include <utility>
#include <string>
#include <iostream>

using namespace std;

//#include "andor_g.hpp"
#include "ni_daq_g.hpp"
//#include "temperaturedialog.h"
//#include "fanctrldialog.h" //only for Andor camera
#include "positioner.hpp"
#include "timer_g.hpp"
#include "spoolthread.h"

//TemperatureDialog * temperatureDialog = NULL;
//FanCtrlDialog* fanCtrlDialog = NULL;
ImagineStatus curStatus;
ImagineAction curAction;

extern QScriptEngine* se;
extern DaqDo* digOut;

// misc vars
bool zoom_isMouseDown = false;
QPoint zoom_downPos, zoom_curPos; //in the unit of displayed image
int nUpdateImage;


#pragma region LIFECYCLE

Imagine::Imagine(Camera *cam, Positioner *pos, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    // pass the camera and positioner to the dataAcqThread
    dataAcqThread.setCamera(cam);
    dataAcqThread.setPositioner(pos);
    dataAcqThread.parentImagine = this;

    // set up the pixmap making thread (see dtor for cleanup)
    pixmapper = new Pixmapper();
    pixmapper->moveToThread(&pixmapperThread);
    connect(&pixmapperThread, &QThread::finished, pixmapper, &QObject::deleteLater);
    connect(this, &Imagine::makePixmap, pixmapper, &Pixmapper::handleImg);
    connect(pixmapper, &Pixmapper::pixmapReady, this, &Imagine::handlePixmap);
    pixmapperThread.start();
    pixmapperThread.setPriority(QThread::LowPriority);

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

    //TODO: temp hide shutter and AI tabs
    //ui.tabAI->hide();  //doesn't work
    //ui.tabWidgetCfg->setTabEnabled(4,false); //may confuse user
    ui.tabWidgetCfg->removeTab(5); //shutter
    ui.tabWidgetCfg->removeTab(5); //now ai becomes 5,  so tab ai is removed as well.

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
    if (pos != NULL && pos->posType == ActuatorPositioner){
        ui.comboBoxTriggerMode->setEnabled(false);
        ui.comboBoxTriggerMode->setCurrentIndex(1);
    }

    Camera& camera = *dataAcqThread.pCamera;

    bool isAndor = camera.vendor == "andor";
    bool isAvt = camera.vendor == "avt";

    if (isAvt){
        ui.actionFlickerControl->setChecked(true);
    }

    ui.actionHeatsinkFan->setEnabled(isAndor);
    ui.actionTemperature->setEnabled(isAndor);

    //camera's gain:
    auto gainRange = camera.getGainRange();
    if (gainRange.second == gainRange.first) ui.spinBoxGain->setEnabled(false);
    else {
        ui.spinBoxGain->setMinimum(gainRange.first);
        ui.spinBoxGain->setMaximum(gainRange.second);
    }

/*    if (isAndor){
        //fill in horizontal shift speed (i.e. read out rate):
        vector<float> horSpeeds = ((AndorCamera*)dataAcqThread.pCamera)->getHorShiftSpeeds();
        for (int i = 0; i < horSpeeds.size(); ++i){
            ui.comboBoxHorReadoutRate->addItem(QString().setNum(horSpeeds[i])
                + " MHz");
        }
        ui.comboBoxHorReadoutRate->setCurrentIndex(0);

        //fill in pre-amp gains:
        vector<float> preAmpGains = ((AndorCamera*)dataAcqThread.pCamera)->getPreAmpGains();
        for (int i = 0; i < preAmpGains.size(); ++i){
            ui.comboBoxPreAmpGains->addItem(QString().setNum(preAmpGains[i]));
        }
        ui.comboBoxPreAmpGains->setCurrentIndex(preAmpGains.size() - 1);

        //verify all pre-amp gains available on all hor. shift speeds:
        for (int horShiftSpeedIdx = 0; horShiftSpeedIdx < horSpeeds.size(); ++horShiftSpeedIdx){
            for (int preAmpGainIdx = 0; preAmpGainIdx < preAmpGains.size(); ++preAmpGainIdx){
                int isAvail;
                //todo: put next func in class AndorCamera
                IsPreAmpGainAvailable(0, 0, horShiftSpeedIdx, preAmpGainIdx, &isAvail);
                if (!isAvail){
                    appendLog("WARNING: not all pre-amp gains available for all readout rates");
                }
            }//for, each pre amp gain
        }//for, each hor. shift speed

        //fill in vert. shift speed combo box
        vector<float> verSpeeds = ((AndorCamera*)dataAcqThread.pCamera)->getVerShiftSpeeds();
        for (int i = 0; i < verSpeeds.size(); ++i){
            ui.comboBoxVertShiftSpeed->addItem(QString().setNum(verSpeeds[i])
                + " us");
        }
        ui.comboBoxVertShiftSpeed->setCurrentIndex(2);

        //fill in vert. clock amplitude combo box:
        for (int i = 0; i < 5; ++i){
            QString tstr;
            if (i == 0) tstr = "0 - Normal";
            else tstr = QString("+%1").arg(i);
            ui.comboBoxVertClockVolAmp->addItem(tstr);
        }
        ui.comboBoxVertClockVolAmp->setCurrentIndex(0);

    }//if, is andor camera
*/
//    else{
        ui.comboBoxHorReadoutRate->setEnabled(false);
        ui.comboBoxPreAmpGains->setEnabled(false);
        ui.comboBoxVertShiftSpeed->setEnabled(false);
        ui.comboBoxVertClockVolAmp->setEnabled(false);
//    }

    ui.spinBoxHend->setValue(camera.getChipWidth());
    ui.spinBoxVend->setValue(camera.getChipHeight());

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

    connect(&dataAcqThread, SIGNAL(newStatusMsgReady(const QString &)),
        this, SLOT(updateStatus(const QString &)));

    connect(&dataAcqThread, SIGNAL(newLogMsgReady(const QString &)),
        this, SLOT(appendLog(const QString &)));

    connect(&dataAcqThread, SIGNAL(imageDataReady(const QByteArray &, long, int, int)),
        this, SLOT(updateDisplay(const QByteArray &, long, int, int)));

    connect(&dataAcqThread, SIGNAL(resetActuatorPosReady()),
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

    //center window:
    QRect rect = QApplication::desktop()->screenGeometry();
    int x = (rect.width() - this->width()) / 2;
    int y = (rect.height() - this->height()) / 2;
    this->move(x, y);

    if (dataAcqThread.pCamera->vendor == "cooke"){
        SpoolThread::allocMemPool(-1); //use default size
    }
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
    /*
    double sum=0;
    int nPixels=imageW*imageH;
    for(unsigned int i=0; i<nPixels; ++i){
    sum+=*frame++;
    }

    double value=sum/nPixels;

    intenCurveData->append(frameIdx, value);
    intenCurve->setData(*intenCurveData);
    intenPlot->setAxisScale(QwtPlot::xBottom, intenCurveData->left(), intenCurveData->right());
    intenPlot->replot();
    */
}//updateIntenCurve(),

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
    if (dataAcqThread.pCamera->vendor == "andor"){
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 14);
    }
    else if (dataAcqThread.pCamera->vendor == "cooke"){
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 5000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 16);
    }
    else {
        histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
        histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1 << 14);
    }

    //intensity curve
    //TODO: make it ui aware
    /*
    int curveWidth=500;
    intenPlot=new QwtPlot();
    ui.dwIntenCurve->setWidget(intenPlot);
    intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number"); //TODO: stack number too
    intenPlot->setAxisTitle(QwtPlot::yLeft, "avg intensity");
    intenCurve = new QwtPlotCurve("avg intensity");

    QwtSymbol sym;
    sym.setStyle(QwtSymbol::Cross);
    sym.setPen(QColor(Qt::black));
    sym.setSize(5);
    intenCurve->setSymbol(&sym);

    intenCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    intenCurve->setPen(QPen(Qt::red));
    intenCurve->attach(intenPlot);

    //intenCurveData=new CurveData(curveWidth);
    intenCurve->setData(*intenCurveData);
    */
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
    dataAcqThread.isUpdatingImage = true;

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
        && idx == dataAcqThread.nFramesPerStack - 1){
        updateIntenCurve(frame, imageW, imageH, dataAcqThread.idxCurStack);
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
    dataAcqThread.isUpdatingImage = false;
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
        ui.tableWidgetStimDisplay->setCurrentCell(0, dataAcqThread.curStimIndex);
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
        ui.comboBoxTriggerMode->setCurrentIndex(ttStr == "internal");
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
        //disabledActions.push_back(ui.actionOpenShutter);
        //disabledActions.push_back(ui.actionCloseShutter);
        //disabledActions.push_back(ui.actionTemperature);

        //disabledWidgets.push_back(ui.btnApply);

        //change intensity plot xlabel
        if (curAction == eLive){
            //intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number");
        }
        else {
            //intenPlot->setAxisTitle(QwtPlot::xBottom, "stack number");
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
    Positioner *pos = dataAcqThread.pPositioner;
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

    if (dataAcqThread.headerFilename == ""){
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
    if (QFile::exists(dataAcqThread.headerFilename) &&
        QMessageBox::question(
        this,
        tr("Overwrite File? -- Imagine"),
        tr("The file called \n   %1\n already exists. "
        "Do you want to overwrite it?").arg(dataAcqThread.headerFilename),
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

    nUpdateImage = 0;
    minPixelValue = maxPixelValue = -1;

    dataAcqThread.applyStim = ui.cbStim->isChecked();
    if (dataAcqThread.applyStim){
        dataAcqThread.curStimIndex = 0;
    }

    //   intenCurveData->clear();

    dataAcqThread.isLive = false;
    dataAcqThread.startAcq();
	//TODO:  Here is where we should synchronize recording from two cameras (if user wants)
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

    //   intenCurveData->clear();

    dataAcqThread.isLive = true;
    dataAcqThread.startAcq();
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
    dataAcqThread.stopAcq();
    updateStatus(eStopping, curAction);
}

void Imagine::on_actionOpenShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //open laser shutter
    digOut->updateOutputBuf(4, true);
    digOut->write();

    {
        QScriptValue jsFunc = se->globalObject().property("onShutterInit");
        if (jsFunc.isFunction()) jsFunc.call();
    }

   {
       QScriptValue jsFunc = se->globalObject().property("onShutterOpen");
       if (jsFunc.isFunction()) jsFunc.call();
   }

   {
       QScriptValue jsFunc = se->globalObject().property("onShutterFini");
       if (jsFunc.isFunction()) jsFunc.call();
   }

   ui.actionOpenShutter->setEnabled(true);
   ui.actionCloseShutter->setEnabled(true);
}

void Imagine::on_actionCloseShutter_triggered()
{
    ui.actionOpenShutter->setEnabled(false);
    ui.actionCloseShutter->setEnabled(false);

    //close laser shutter
    digOut->updateOutputBuf(4, false);
    digOut->write();
    {
        QScriptValue jsFunc = se->globalObject().property("onShutterInit");
        if (jsFunc.isFunction()) jsFunc.call();
    }

   {
       QScriptValue jsFunc = se->globalObject().property("onShutterClose");
       if (jsFunc.isFunction()) jsFunc.call();
   }

   {
       QScriptValue jsFunc = se->globalObject().property("onShutterFini");
       if (jsFunc.isFunction()) jsFunc.call();
   }

   ui.actionOpenShutter->setEnabled(true);
   ui.actionCloseShutter->setEnabled(true);
}

/* void Imagine::on_actionTemperature_triggered()  //only for Andor camera
{
    //
    if (!temperatureDialog){
        temperatureDialog = new TemperatureDialog(this);
        //initialize some value in the dialog:
        int minTemp, maxTemp;
        GetTemperatureRange(&minTemp, &maxTemp); //TODO: wrap this func
        temperatureDialog->ui.verticalSliderTempToSet->setRange(minTemp, maxTemp);
        temperatureDialog->ui.verticalSliderTempToSet->setValue(maxTemp);
        temperatureDialog->updateTemperature();
    }
    temperatureDialog->exec();
}
*/

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

    Camera& camera = *dataAcqThread.pCamera;

    bool isAndor = camera.vendor == "andor";

/*    if (isAndor){
        //TODO: ask user wait for temperature rising.
        //   like andor mcd, if temperature is too low,  event->ignore();
        int temperature = 20; //assume room temperature
        if (temperatureDialog){
            temperature = temperatureDialog->updateTemperature();
        }//if, user might adjust temperature
        if (temperature <= 0){
            QMessageBox::information(0, "Imagine",
                "Camera's temperature is below 0.\nPlease raise it to above 0");
            event->ignore();
            return;
        }
        ((AndorCamera*)dataAcqThread.pCamera)->switchCooler(false); //switch off cooler
    }
*/

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
    ui.spinBoxHend->setValue(1004); //TODO: hard coded
    ui.spinBoxVstart->setValue(1);
    ui.spinBoxVend->setValue(1002); //TODO: hard coded

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
    Camera& camera = *dataAcqThread.pCamera;

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

void Imagine::on_btnApply_clicked()
{
    if (curStatus != eIdle){
        QMessageBox::information(this, "Please wait --- Imagine",
            "Please click 'apply' when the camera is idle.");

        return;
    }

    Camera& camera = *dataAcqThread.pCamera;

    QString acqTriggerModeStr = ui.comboBoxTriggerMode->currentText();
	QString expTriggerModeStr = "Auto";
    Camera::AcqTriggerMode acqTriggerMode;
	Camera::ExpTriggerMode expTriggerMode;
    if (acqTriggerModeStr == "External Start") acqTriggerMode = Camera::eExternal;
	else if (acqTriggerModeStr == "Internal")  acqTriggerMode = Camera::eInternalTrigger;
    else {
        assert(0); //if code goes here, it is a bug
    }
	//TODO: add exposure trigger mode to UI
	if (expTriggerModeStr == "External Start") expTriggerMode = Camera::eExternalStart;
	else if (expTriggerModeStr == "Auto")  expTriggerMode = Camera::eAuto;
	else if (expTriggerModeStr == "External Control")  expTriggerMode = Camera::eExternalControl;
	else {
		assert(0); //if code goes here, it is a bug
	}
    dataAcqThread.acqTriggerMode = acqTriggerMode;
	dataAcqThread.expTriggerMode = expTriggerMode;

    dataAcqThread.piezoStartPosUm = ui.doubleSpinBoxStartPos->value();
    dataAcqThread.piezoStopPosUm = ui.doubleSpinBoxStopPos->value();
    dataAcqThread.piezoTravelBackTime = ui.doubleSpinBoxPiezoTravelBackTime->value();
    dataAcqThread.isBiDirectionalImaging = ui.cbBidirectionalImaging->isChecked();

    dataAcqThread.nStacks = ui.spinBoxNumOfStacks->value();
    dataAcqThread.nFramesPerStack = ui.spinBoxFramesPerStack->value();
    dataAcqThread.exposureTime = ui.doubleSpinBoxExpTime->value();
    dataAcqThread.idleTimeBwtnStacks = ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();

    dataAcqThread.horShiftSpeedIdx = ui.comboBoxHorReadoutRate->currentIndex();
    dataAcqThread.preAmpGainIdx = ui.comboBoxPreAmpGains->currentIndex();
    dataAcqThread.gain = ui.spinBoxGain->value();
    dataAcqThread.verShiftSpeedIdx = ui.comboBoxVertShiftSpeed->currentIndex();
    dataAcqThread.verClockVolAmp = ui.comboBoxVertClockVolAmp->currentIndex();

    dataAcqThread.preAmpGain = ui.comboBoxPreAmpGains->currentText();
    dataAcqThread.horShiftSpeed = ui.comboBoxHorReadoutRate->currentText();
    dataAcqThread.verShiftSpeed = ui.comboBoxVertShiftSpeed->currentText();

    //params for binning
    dataAcqThread.hstart = camera.hstart = ui.spinBoxHstart->value();
    dataAcqThread.hend = camera.hend = ui.spinBoxHend->value();
    dataAcqThread.vstart = camera.vstart = ui.spinBoxVstart->value();
    dataAcqThread.vend = camera.vend = ui.spinBoxVend->value();

    dataAcqThread.angle = ui.spinBoxAngle->value();

    dataAcqThread.umPerPxlXy = ui.doubleSpinBoxUmPerPxlXy->value();

    //enforce #nBytesPerFrame is x times of 16
    int nBytesPerFrame = camera.getImageWidth()*camera.getImageHeight() * 2; //todo: hardcoded 2
    if (nBytesPerFrame % 16) {
        QMessageBox::critical(this, "Imagine", "ROI spec is wrong (#pixels per frame is not x times of 8)."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return;
    }

    if (!checkRoi()){
        QMessageBox::critical(this, "Imagine", "ROI spec is wrong."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return;
    }

    if (camera.vendor == "avt"){
        if (dataAcqThread.exposureTime < 1 / 57.0){
            QMessageBox::critical(this, "Imagine", "exposure time is too small."
                , QMessageBox::Ok, QMessageBox::NoButton);

            return;
        }
    }

    L = -1;

    for (int i = 0; i < 2; ++i){
        paramOK = camera.setAcqParams(dataAcqThread.gain,
            dataAcqThread.preAmpGainIdx,
            dataAcqThread.horShiftSpeedIdx,
            dataAcqThread.verShiftSpeedIdx,
            dataAcqThread.verClockVolAmp
            );
        if (!paramOK) {
            updateStatus(QString("Camera: applied params: ") + camera.getErrorMsg().c_str());
            goto skip;
        }

        paramOK = camera.setAcqModeAndTime(Camera::eLive,
            dataAcqThread.exposureTime,
            dataAcqThread.nFramesPerStack,
            dataAcqThread.acqTriggerMode, //Camera::eInternalTrigger  //use internal trigger
			dataAcqThread.expTriggerMode
            );
        dataAcqThread.cycleTime = camera.getCycleTime();
        updateStatus(QString("Camera: applied params: ") + camera.getErrorMsg().c_str());
        if (!paramOK) goto skip;
    }

    //get the real params used by the camera:
    dataAcqThread.cycleTime = camera.getCycleTime();
    cout << "cycle time is: " << dataAcqThread.cycleTime << endl;

    // if applicable, make sure positioner preparation went well...
    Positioner *pos = dataAcqThread.pPositioner;
    if (pos != NULL && !dataAcqThread.preparePositioner()){
        paramOK = false;
        QString msg = QString("Positioner: applied params failed: ") + pos->getLastErrorMsg().c_str();
        updateStatus(msg);
        QMessageBox::critical(0, "Imagine: Failed to setup piezo/stage.", msg
            , QMessageBox::Ok, QMessageBox::NoButton);
    }

skip:

    //set filenames:
    QString headerFilename = ui.lineEditFilename->text();
    dataAcqThread.headerFilename = headerFilename;
    if (!headerFilename.isEmpty()){
        dataAcqThread.aiFilename = replaceExtName(headerFilename, "ai");
        dataAcqThread.camFilename = replaceExtName(headerFilename, "cam");
        dataAcqThread.sifFileBasename = replaceExtName(headerFilename, "");
    }//else, save data

    dataAcqThread.stimFileContent = ui.textEditStimFileContent->toPlainText();
    dataAcqThread.comment = ui.textEditComment->toPlainText();

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
    dataAcqThread.stimuli.clear();

    for (int i = dataStartIdx; i < lines.size(); ++i){
        int valve, stack;
        QString line = lines[i].c_str();
        if (line.trimmed().isEmpty()) continue;
        QTextStream tt(&line);
        tt >> valve >> stack;
        dataAcqThread.stimuli.push_back(pair<int, int>(valve, stack));
    }

    //fill in entries in table:
    ui.tableWidgetStimDisplay->setColumnCount(dataAcqThread.stimuli.size());
    QStringList tableHeader;
    for (int i = 0; i < dataAcqThread.stimuli.size(); ++i){
        QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(dataAcqThread.stimuli[i].first));
        ui.tableWidgetStimDisplay->setItem(0, i, newItem);
        tableHeader << QString().setNum(dataAcqThread.stimuli[i].second);
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

void Imagine::on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue)
{
    if (ui.cbAutoSetPiezoTravelBackTime->isChecked()){
        ui.doubleSpinBoxPiezoTravelBackTime->setValue(newValue / 2);
    }

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
    Positioner *pos = dataAcqThread.pPositioner;
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
    Positioner *pos = dataAcqThread.pPositioner;
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

#pragma endregion

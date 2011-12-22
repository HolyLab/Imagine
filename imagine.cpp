/*-------------------------------------------------------------------------
** Copyright (C) 2005-2008 Timothy E. Holy and Zhongsheng Guo
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
#include <QTimer>
#include <QPainter>
#include <QVBoxLayout>
#include <QDate>
#include <QScriptEngine>
#include <QScriptProgram>
#include <QApplication>

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_interval_data.h>
#include <qwt_scale_engine.h>
#include <qwt_data.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>
#include <QDesktopWidget>

#include "histogram_item.h"

#include <vector>
#include <utility>
#include <string>
#include <iostream>

using namespace std;

#include "andor_g.hpp"
#include "ni_daq_g.hpp"
#include "temperaturedialog.h"
#include "fanctrldialog.h"
#include "positioner.hpp"


vector<pair<int,int> > stimuli; //first: stim (valve), second: time (stack#)
int curStimIndex;
TemperatureDialog * temperatureDialog=NULL;
FanCtrlDialog* fanCtrlDialog=NULL;
ImagineStatus curStatus;
ImagineAction curAction;

//todo: 
extern Camera* pCamera;
extern Positioner* pPositioner;
extern QScriptEngine* se;


class CurveData: public QwtData
{
   QList<QPointF> data;
   double width; //the length of x axis
public:
   CurveData(double width) {
      this->width=width;
   }

   virtual QwtData *copy() const  {
      CurveData* result= new CurveData(width);
      result->data=data;

      return result;
   }

   virtual size_t size() const {
      return data.size();
   }

   virtual double x(size_t i) const {
      return data[i].x();
   }

   virtual double y(size_t i) const {
      return data[i].y();
   }

   //above are required for the virtual parent

   double left(){ return data[0].x(); }
   
   double right(){
      return data.last().x();
   }

   void clear(){
      data.clear();
   }

   void append(double x, double y){
      data.push_back(QPointF(x,y));
      while(x-this->x(0)>width) data.pop_front();
   }
};//class, CurveData


Imagine::Imagine(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
   ui.setupUi(this);

   //to overcome qt designer's incapability
   addDockWidget(Qt::TopDockWidgetArea, ui.dwStim);
   addDockWidget(Qt::LeftDockWidgetArea, ui.dwCfg);
   addDockWidget(Qt::LeftDockWidgetArea, ui.dwLog);
   addDockWidget(Qt::LeftDockWidgetArea, ui.dwHist);
   addDockWidget(Qt::LeftDockWidgetArea, ui.dwIntenCurve);

   //ui.dwIntenCurve->setFloating(true);
   //ui.dwIntenCurve->show();

   ui.dwCfg->setWindowTitle("Config and control");
   tabifyDockWidget(ui.dwCfg, ui.dwHist);
   tabifyDockWidget(ui.dwHist, ui.dwIntenCurve);
   setDockOptions(dockOptions() | QMainWindow::VerticalTabs);

   /*
   //make the hist window float
   ui.dwHist->setFloating(true);
   ui.dwHist->show();
   */

   //TODO: temp hide stim windows
   //ui.dwStim->hide();

   //TODO: temp hide shutter and AI tabs
   //ui.tabAI->hide();  //doesn't work
   //ui.tabWidgetCfg->setTabEnabled(4,false); //may confuse user
   ui.tabWidgetCfg->removeTab(5); //shutter
   ui.tabWidgetCfg->removeTab(5); //now ai becomes 5,  so tab ai is removed as well.

   //adjust size and 
   //todo: center the window
   QRect tRect=geometry();
   tRect.setHeight(tRect.height()+100);
   tRect.setWidth(tRect.width()+125);
   this->setGeometry(tRect);
   
   //group menuitems on View:
   ui.actionNoAutoScale->setCheckable(true);  //note: MUST
   ui.actionAutoScaleOnFirstFrame->setCheckable(true);
   ui.actionAutoScaleOnAllFrames->setCheckable(true);
   QActionGroup* autoScaleActionGroup = new QActionGroup(this);
   autoScaleActionGroup->addAction(ui.actionNoAutoScale);  //OR: ui.actionNoAutoScale->setActionGroup(autoScaleActionGroup);
   autoScaleActionGroup->addAction(ui.actionAutoScaleOnFirstFrame);
   autoScaleActionGroup->addAction(ui.actionAutoScaleOnAllFrames);
   ui.actionAutoScaleOnFirstFrame->setChecked(true);

   ui.actionStimuli->setCheckable(true);
   ui.actionStimuli->setChecked(true);
   ui.actionConfig->setCheckable(true);
   ui.actionConfig->setChecked(true);
   ui.actionLog->setCheckable(true);
   ui.actionLog->setChecked(true);

   ui.actionColorizeSaturatedPixels->setCheckable(true);
   ui.actionColorizeSaturatedPixels->setChecked(true);

   //the histgram
   histPlot=new QwtPlot();
   histPlot->setCanvasBackground(QColor(Qt::white));
   ui.dwHist->setWidget(histPlot); //setWidget() causes the widget using all space
   histPlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);

   QwtPlotGrid *grid = new QwtPlotGrid;
   grid->enableXMin(true);
   grid->enableYMin(true);
   grid->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
   grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
   grid->attach(histPlot);

   histogram = new HistogramItem();
   histogram->setColor(Qt::darkCyan);
   histogram->attach(histPlot);

   ///todo: make it more robust by query Camera class
   if(pCamera->vendor=="andor"){
      histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
      histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1<<14);
   }
   else if(pCamera->vendor=="cooke"){
      histPlot->setAxisScale(QwtPlot::yLeft, 1, 5000000.0);
      histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1<<16);
   }
   else {
      histPlot->setAxisScale(QwtPlot::yLeft, 1, 1000000.0);
      histPlot->setAxisScale(QwtPlot::xBottom, 0.0, 1<<14);
   }

   //intensity curve
   //TODO: make it ui aware
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
   intenCurve->setSymbol(sym);

   intenCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
   intenCurve->setPen(QPen(Qt::red));
   intenCurve->attach(intenPlot);

   intenCurveData=new CurveData(curveWidth);
   intenCurve->setData(*intenCurveData);

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
   QImage tImage("logo.jpg"); //todo: put logo here
   if(!tImage.isNull()) {
      ui.labelImage->setPixmap(QPixmap::fromImage(tImage));
      ui.labelImage->adjustSize();
   }

   //fill in trigger mode combo box
   ui.comboBoxTriggerMode->addItem("External Start");
   ui.comboBoxTriggerMode->addItem("Internal");

   //todo: even on cooke, use external start
   ui.comboBoxTriggerMode->setCurrentIndex(pCamera->vendor=="cooke");

   Camera& camera=*pCamera;

   bool isAndor=camera.vendor=="andor";

   if(isAndor){
      //TODO: get pre-amp gain and fill in the cfg window
      //fill in horizontal shift speed (i.e. read out rate):
      vector<float> horSpeeds=((AndorCamera*)pCamera)->getHorShiftSpeeds();
      for(int i=0; i<horSpeeds.size(); ++i){
         ui.comboBoxHorReadoutRate->addItem(QString().setNum(horSpeeds[i])
            +" MHz");
      }
      ui.comboBoxHorReadoutRate->setCurrentIndex(0);

      //fill in pre-amp gains:
      vector<float> preAmpGains=((AndorCamera*)pCamera)->getPreAmpGains();
      for(int i=0; i<preAmpGains.size(); ++i){
         ui.comboBoxPreAmpGains->addItem(QString().setNum(preAmpGains[i]));
      }
      ui.comboBoxPreAmpGains->setCurrentIndex(preAmpGains.size()-1);

      //verify all pre-amp gains available on all hor. shift speeds:
      for(int horShiftSpeedIdx=0; horShiftSpeedIdx<horSpeeds.size(); ++horShiftSpeedIdx){
         for(int preAmpGainIdx=0; preAmpGainIdx<preAmpGains.size(); ++preAmpGainIdx){
            int isAvail;
            //todo: put next func in class AndorCamera
            IsPreAmpGainAvailable(0,0,horShiftSpeedIdx, preAmpGainIdx, &isAvail);
            if(!isAvail){
               appendLog("WARNING: not all pre-amp gains available for all readout rates");
            }
         }//for, each pre amp gain
      }//for, each hor. shift speed

      /* for this specific camera (ixon 885), nADChannels=nOutputAmplifiers=1
         so this chunk code is not necessary.
      int nADChannels=0, nOutputAmplifiers=0;
      GetNumberADChannels(&nADChannels); //TODO: wrap it
      GetNumberAmp(&nOutputAmplifiers); //TODO: wrap it
      */

      //fill in vert. shift speed combo box
      vector<float> verSpeeds=((AndorCamera*)pCamera)->getVerShiftSpeeds();
      for(int i=0; i<verSpeeds.size(); ++i){
         ui.comboBoxVertShiftSpeed->addItem(QString().setNum(verSpeeds[i])
            +" us");
      }
      ui.comboBoxVertShiftSpeed->setCurrentIndex(2);

      //fill in vert. clock amplitude combo box:
      for(int i=0; i<5; ++i){
         QString tstr;
         if(i==0) tstr="0 - Normal";
         else tstr=QString("+%1").arg(i);
         ui.comboBoxVertClockVolAmp->addItem(tstr);
      }
      ui.comboBoxVertClockVolAmp->setCurrentIndex(0);

   }//if, is andor camera
   else{
      ui.comboBoxHorReadoutRate->setEnabled(false);
      ui.comboBoxPreAmpGains->setEnabled(false);
      ui.comboBoxVertShiftSpeed->setEnabled(false);
      ui.comboBoxVertClockVolAmp->setEnabled(false);

      ui.actionTemperature->setEnabled(false);

   }

   ui.spinBoxHend->setValue(camera.getChipWidth());
   ui.spinBoxVend->setValue(camera.getChipHeight());

   //apply the camera setting:
   on_btnApply_clicked();

   /*
   //pre-allocate space for camera buffer:
   int nFrameToHold=60; //TODO: make this ui aware
   if(!camera.allocImageArray(nFrameToHold,true)){
      appendLog(QString("reserve %1-frame memory failed")
         .arg(nFrameToHold));
   }
   */

   //prepare for DO:
   digOut=new NiDaqDo();
   //TODO: check error of digOut

   updateStatus(eIdle, eNoAction);

   ///piezo stuff
   ui.doubleSpinBoxMinDistance->setValue(pPositioner->minPos());
   ui.doubleSpinBoxMaxDistance->setValue(pPositioner->maxPos());

   //set piezo position to 0 um
   QMessageBox::information(this, "Imagine", 
         "Please raise microscope.\nPiezo position is about to be set 0 um");
   ui.doubleSpinBoxCurPos->setValue(pPositioner->minPos());
   on_btnMovePiezo_clicked();

   //

   QString buildDateStr=__DATE__;
   QDate date=QDate::fromString(buildDateStr, "MMM d yyyy");
   QString ver=QString("%1%2%3")
      .arg(date.year()-2000, 1, 16)
      .arg(date.month(), 1, 16)
      .arg(date.day(), 2, 10, QChar('0'));
   QString copyright="(c) 2005-2011 Tim Holy & Zhongsheng Guo";
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

   
   //mouse events on image
   connect(ui.labelImage, SIGNAL(mousePressed(QMouseEvent*)),
               this, SLOT(zoom_onMousePressed(QMouseEvent*)));

   connect(ui.labelImage, SIGNAL(mouseMoved(QMouseEvent*)),
               this, SLOT(zoom_onMouseMoved(QMouseEvent*)));

   connect(ui.labelImage, SIGNAL(mouseReleased(QMouseEvent*)),
               this, SLOT(zoom_onMouseReleased(QMouseEvent*)));

   QRect rect = QApplication::desktop()->screenGeometry();
   int x = (rect.width()-this->width()) / 2;
   int y = (rect.height()-this->height()) / 2;
   this->move(x, y);

}

Imagine::~Imagine()
{
   delete digOut;
   delete pPositioner;
   delete pCamera;
}

bool zoom_isMouseDown=false;
QPoint zoom_downPos, zoom_curPos; //in the unit of displayed image
int L=-1, W, T, H; //in the unit of original image
QImage * image=0;   //TODO: free it in dtor. The acquired image.
QPixmap * pixmap=0; //the displayed.

void Imagine::zoom_onMousePressed(QMouseEvent* event)
{
   appendLog("pressed");
   if(event->button() == Qt::LeftButton) {
      zoom_downPos = event->pos();
      zoom_isMouseDown = true;
   }
}

void Imagine::zoom_onMouseMoved(QMouseEvent* event)
{
   //appendLog("moved");
   if((event->buttons() & Qt::LeftButton) && zoom_isMouseDown){
      zoom_curPos=event->pos();

      if(pixmap){ //TODO: if(pixmap && idle)
         updateImage();
      }
   }
}

//convert position in the unit of pixmap to position in the unit of orig img
QPoint calcPos(const QPoint& pos)
{
   QPoint result;
   result.rx()=L+double(W)/pixmap->width()*pos.x();
   result.ry()=T+double(H)/pixmap->height()*pos.y();

   return result;
}

void Imagine::zoom_onMouseReleased(QMouseEvent* event)
{
   appendLog("released");
   if(event->button() == Qt::LeftButton && zoom_isMouseDown) {
      zoom_curPos=event->pos();
      zoom_isMouseDown = false;

      appendLog(QString("last pos: (%1, %2); cur pos: (%3, %4)")
         .arg(zoom_downPos.x()).arg(zoom_downPos.y())
         .arg(zoom_curPos.x()).arg(zoom_curPos.y()));

      if(!pixmap) return; //NOTE: this is for not zooming logo image

      //update L,T,W,H
      QPoint LT=calcPos(zoom_downPos);
      QPoint RB=calcPos(zoom_curPos);
      
      if(LT.x()>RB.x() && LT.y()>RB.y()){
         L=-1;
         goto done;
      }//if, should show full image

      if(!(LT.x()<RB.x() && LT.y()<RB.y())) return; //unknown action
      
      L=LT.x(); T=LT.y();
      W=RB.x()-L; H=RB.y()-T;

done:
      //refresh the image
      updateImage();
   }//if, dragging

}


void Imagine::on_actionDisplayFullImage_triggered()
{
   L=-1;
   updateImage();

}


volatile bool isUpdatingImage=false;
int nUpdateImage;

void Imagine::appendLog(const QString& msg)
{
   static int nAppendingLog=0;
   //if(nAppendingLog>=1000) {
   if(nAppendingLog>=ui.spinBoxMaxNumOfLinesInLog->value()) {
      ui.textEditLog->clear();
      nAppendingLog=0;
   }

   ui.textEditLog->append(msg);
   nAppendingLog++;
}

void Imagine::calcMinMaxValues(Camera::PixelValue * frame, int imageW, int imageH)
{
   minPixelValue=maxPixelValue=frame[0];
   for(int i=1; i<imageW*imageH; i++){
      if(frame[i]==(1<<16)-1) continue; //todo: this is a tmp fix for dead pixels
      if(frame[i]>maxPixelValue) maxPixelValue=frame[i];
      else if(frame[i]<minPixelValue) minPixelValue=frame[i];
   }//for, each pixel
}

void Imagine::updateImage()
{
   if(!image) return;

   //TODO: user should be able to adjust this param or fit to width/height
   double maxWidth=500, maxHeight=500;
   if(L==-1){
      L=T=0; H=image->height(); W=image->width();
   }//if, uninitialized edges
   //crop the image from the original
   QImage cropedImage=image->copy(L, T, W, H);
   //zoom
   double zoomFactor=min(maxWidth/W, maxHeight/H);
   //double ttLiveZoomFactor=0.5;
   QImage scaledImage=cropedImage.scaledToHeight(cropedImage.height()*zoomFactor);
   //QImage scaledImage=*image; //TODO: temp

   //NOTE: comment out next 2 lines, you will know that the display framerate's bottleneck
   //   is QT's signal-slot queuing and what in data_acq_thread.cpp
   if(!pixmap) pixmap=new QPixmap();
   *pixmap= QPixmap::fromImage(scaledImage);

   {
      QPainter painter(pixmap); //interesting that it's not working w/ QImage
      QPen pen(Qt::green, 2);
      painter.setPen(pen);
      //int width=nUpdateImage%100+20;
      if(zoom_isMouseDown){
         QPoint lt(min(zoom_downPos.x(), zoom_curPos.x()),
                   min(zoom_downPos.y(), zoom_curPos.y()) );
         QPoint rb(max(zoom_downPos.x(), zoom_curPos.x()),
                   max(zoom_downPos.y(), zoom_curPos.y()) );
         painter.drawRect(QRect(lt, rb));
         //painter.drawText(rect(), Qt::AlignCenter, "The Text");
      }
   }//scope for QPainter obj

   ui.labelImage->setPixmap(*pixmap);
   //ui.labelImage->setPixmap(QPixmap::fromImage(scaledImage));
   ui.labelImage->adjustSize();

}

int counts[1<<16];
void Imagine::updateHist(const Camera::PixelValue * frame, 
                         const int imageW, const int imageH)
{
   //initialize the counts to all 0's
   for(unsigned int i=0; i<sizeof(counts)/sizeof(counts[0]); ++i) counts[i]=0;

   for(unsigned int i=0; i<imageW*imageH; ++i){
      counts[*frame++]++;
   }

   //now binning
   int nBins=histPlot->width()/2;
   int totalWidth=1<<14;
   int binWidth=totalWidth/nBins;

   QwtArray<QwtDoubleInterval> intervals(nBins);
   QwtArray<double> values(nBins);

   for(unsigned int i = 0; i < nBins; i++ ) {
      int start=i*binWidth;                            //inclusive
      int end  = i==nBins-1?totalWidth:start+binWidth; //exclusive
      int width=end-start;
      intervals[i] = QwtDoubleInterval(start, end);

      int count=0;
      for(; start<end;){
         count+=counts[start++];
      }
      values[i] = count?count:1; 

   }//for, each inten

   histogram->setData(QwtIntervalData(intervals, values));
   
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
   intenCurve->setData(*intenCurveData);
   intenPlot->setAxisScale(QwtPlot::xBottom, intenCurveData->left(), intenCurveData->right());
   intenPlot->replot();
}//updateIntenCurve(),


//note: idx is 0-based
void Imagine::updateDisplay(const QByteArray &data16, long idx, int imageW, int imageH)
{
   isUpdatingImage=true;

   //NOTE: about colormap/contrast/autoscale:
   //  Because Qt doesn't support 16bit grayscale image, we have to use 8bit index
   //  image. Then changing colormap (which is much faster than scaling the real data)
   //  is not a good option because when we map 14 bit value to 8 bit, 
   //  we map 64 possible values into one value and if original values are in small
   //  range, we've lost the contrast so that changing colormap won't help. 
   //  So I have to scale the real data.

   if(image==0 || image->width()!=imageW || image->height()!=imageH){
      delete image; image=0;
      image=new QImage(imageW, imageH, QImage::Format_Indexed8);
      //set to 256 level grayscale
      image->setNumColors(256);
      for(int i=0; i<256; i++){
         image->setColor(i, qRgb(i, i, i)); //set color table entry
      }
   }//if, need realloc the Image object

   //user may changed the colorizing option during acq.
   bool isColorizeSaturatedPixels=ui.actionColorizeSaturatedPixels->isChecked();
   if(isColorizeSaturatedPixels){
      image->setColor(0, qRgb(0, 0, 255));
      image->setColor(255, qRgb(255, 0, 0));
   }
   else{
      image->setColor(0, qRgb(0, 0, 0));
      image->setColor(255, qRgb(255, 255, 255));
   }

   Camera::PixelValue * frame=(Camera::PixelValue * )data16.constData();

   //upate histogram plot
   int histSamplingRate=3; //that is, calc histgram every 3 updating
   if(nUpdateImage%histSamplingRate==0){
      updateHist(frame, imageW, imageH);
   }

   //update intensity vs stack (or frame) curve
   //  --- live mode:
   if(nUpdateImage%histSamplingRate==0 && curStatus==eRunning && curAction==eLive){
      updateIntenCurve(frame, imageW, imageH, idx);
   }
   //  --- acq mode:
   if(curStatus==eRunning && curAction==eAcqAndSave 
        && idx==dataAcqThread.nFramesPerStack-1){
      updateIntenCurve(frame, imageW, imageH, dataAcqThread.idxCurStack);
   }

   //copy and scale data
   unsigned char * imageData=image->bits();
   double factor;
   if(ui.actionNoAutoScale->isChecked()){
      minPixelValue=maxPixelValue=0;
      factor=1.0/(1<<6); //i.e. /(2^14)*(2^8). note: not >>8 b/c it's 14-bit camera. TODO: take care of 16 bit camera?
   }//if, no contrast adjustment
   else {
      if(ui.actionAutoScaleOnFirstFrame->isChecked()){
         //user can change the setting when acq. is running
         if(maxPixelValue==-1 || maxPixelValue==0){
            calcMinMaxValues(frame, imageW, imageH);
         }//if, need update min/max values. 
      }//if, min/max are from the first frame
      else {
         calcMinMaxValues(frame, imageW, imageH);
      }//else, min/max are from each frame's own data

      factor=255.0/(maxPixelValue-minPixelValue);
      if(isColorizeSaturatedPixels) factor*=254/255.0;
   }//else, adjust contrast(i.e. scale data by min/max values)

   AndorCamera::PixelValue * tp=frame;
   for(int row=0; row<imageH; ++row){
      unsigned char * rowData=image->scanLine(row);
      for(int col=0; col<imageW; ++col){
         AndorCamera::PixelValue inten= *tp++;
         int index;
         if(inten>=16383 && isColorizeSaturatedPixels) index=255;
         else {
            index=(inten-minPixelValue)*factor;
            //image->setPixel(i%imageW, i/imageW,(frame[i]-minPixelValue)*factor);
         }

         rowData[col]= index;
         //image->setPixel(col, row, index);
      }//for, each scan line
   }//for, each row

   updateImage();

   nUpdateImage++;
   appendLog(QString().setNum(nUpdateImage)
      +"-th updated frame(0-based)="+QString().setNum(idx));
   isUpdatingImage=false;
}//updateDisplay(),


void Imagine::updateStatus(const QString &str)
{
   if(str=="acq-thread-finish"){
      updateStatus(eIdle, eNoAction);
      return;
   }

   this->statusBar()->showMessage(str);
   appendLog(str);

   if(str.startsWith("valve", Qt::CaseInsensitive)){
      ui.tableWidgetStimDisplay->setCurrentCell(0, curStimIndex);
   }//if, update stimulus display
}


void Imagine::on_actionStartAcqAndSave_triggered()
{
   if(dataAcqThread.headerFilename==""){
      if(ui.lineEditFilename->text()!=""){
         QMessageBox::information(this, "Forget to apply configuration --- Imagine",
            "Please apply the configuration by clicking Apply button");
         return;
      }

      QMessageBox::information(this, "Need file name --- Imagine",
            "Please specify file name to save");
      return;
   }

   //warn user if overwrite file:
   //QFileInfo fi(dataAcqThread.headerFilename);
   //if(fi.exists()){ }
   if(QFile::exists(dataAcqThread.headerFilename) &&
      QMessageBox::question(
      this,
      tr("Overwrite File? -- Imagine"),
      tr("The file called \n   %1\n already exists. "
         "Do you want to overwrite it?").arg(dataAcqThread.headerFilename),
      tr("&Yes"), tr("&No"),
      QString(), 0, 1)){
         return;
   }//if, file exists and user don't want to overwrite it
   
   nUpdateImage=0;
   minPixelValue=maxPixelValue=-1;

   dataAcqThread.applyStim=ui.cbStim->isChecked();
   if(dataAcqThread.applyStim){
      curStimIndex=0;
   }

   intenCurveData->clear();

   dataAcqThread.isLive=false;
   dataAcqThread.startAcq();
   updateStatus(eRunning, eAcqAndSave);
}

void Imagine::on_actionStartLive_triggered()
{
   nUpdateImage=0;
   minPixelValue=maxPixelValue=-1;

   intenCurveData->clear();

   dataAcqThread.isLive=true;
   dataAcqThread.startAcq();
   updateStatus(eRunning, eLive);
}

void Imagine::on_actionAutoScaleOnFirstFrame_triggered()
{
   minPixelValue=maxPixelValue=-1;
}


void Imagine::on_actionStop_triggered()
{
   dataAcqThread.stopAcq();
   updateStatus(eStopping, curAction);
}

void Imagine::on_actionOpenShutter_triggered()
{
   //open laser shutter
   digOut->updateOutputBuf(4,true);
   digOut->write();
   {
   QScriptValue jsFunc=se->globalObject().property("onShutterOpen");
   if(jsFunc.isFunction()) jsFunc.call();
   }

}

void Imagine::on_actionCloseShutter_triggered()
{
   //close laser shutter
   digOut->updateOutputBuf(4,false);
   digOut->write();
   {
   QScriptValue jsFunc=se->globalObject().property("onShutterClose");
   if(jsFunc.isFunction()) jsFunc.call();
   }

}


void Imagine::on_actionTemperature_triggered()
{
   //
   if(!temperatureDialog){
      temperatureDialog=new TemperatureDialog(this);
      //initialize some value in the dialog:
      int minTemp, maxTemp;
      GetTemperatureRange(&minTemp, &maxTemp); //TODO: wrap this func
      temperatureDialog->ui.verticalSliderTempToSet->setRange(minTemp, maxTemp);
      temperatureDialog->ui.verticalSliderTempToSet->setValue(maxTemp);
      temperatureDialog->updateTemperature();
   }
   temperatureDialog->exec();
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

   Camera& camera=*pCamera;

   bool isAndor=camera.vendor=="andor";

   if(isAndor){
      //TODO: ask user wait for temperature rising.
      //   like andor mcd, if temperature is too low,  event->ignore();
      int temperature=20; //assume room temperature
      if(temperatureDialog){
         temperature=temperatureDialog->updateTemperature();
      }//if, user might adjust temperature
      if(temperature<=0){
         QMessageBox::information(0, "Imagine", 
            "Camera's temperature is below 0.\nPlease raise it to above 0");
         event->ignore();
         return;
      }
      ((AndorCamera*)pCamera)->switchCooler(false); //switch off cooler
   }

   //shutdown camera
   //see: QMessageBox Class Reference
   QMessageBox *mb=new QMessageBox("Imagine",    //caption
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
   if(!image){
      QMessageBox::warning(this, tr("Imagine"),
         tr("Please acquire one full chip image first"));
      return;
   }

   if(image->width()!=1004 || image->height()!=1002){
      QMessageBox::warning(this, tr("Imagine"),
         tr("Please acquire one full chip image first"));
      return;
   }

   if(L==-1) {
      on_btnFullChipSize_clicked();
      return;
   }//if, full image
   else {
      ui.spinBoxHstart->setValue(L+1);  //NOTE: hstart is 1-based
      ui.spinBoxHend->setValue(L+W);
      ui.spinBoxVstart->setValue(T+1);
      ui.spinBoxVend->setValue(T+H);

   }//else,

}


bool Imagine::checkRoi()
{
   Camera& camera=*pCamera;

   //set the roi def
   se->globalObject().setProperty("hstart",ui.spinBoxHstart->value());
   se->globalObject().setProperty("hend",ui.spinBoxHend->value());
   se->globalObject().setProperty("vstart",ui.spinBoxVstart->value());
   se->globalObject().setProperty("vend",ui.spinBoxVend->value());

   //set the window reference
   QScriptValue qtwin=se->newQObject(this);
   se->globalObject().setProperty("window", qtwin);

   QScriptValue checkFunc=se->globalObject().property("checkRoi");

   bool result=true;
   if(checkFunc.isFunction()) result=checkFunc.call().toBool();

   return result;
}


void Imagine::on_btnApply_clicked()
{
   Camera& camera=*pCamera;

   if(!checkRoi()){
      QMessageBox::critical(this, "Imagine", "ROI spec is wrong."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return;
   }

   QString triggerModeStr=ui.comboBoxTriggerMode->currentText();
   AndorCamera::TriggerMode triggerMode;
   if(triggerModeStr=="External Start") triggerMode=AndorCamera::eExternalStart;
   else if(triggerModeStr=="Internal")  triggerMode=AndorCamera::eInternalTrigger;
   else {
      assert(0); //if code goes here, it is a bug
   }
   dataAcqThread.triggerMode=triggerMode;

   dataAcqThread.piezoStartPosUm=ui.doubleSpinBoxStartPos->value();
   dataAcqThread.piezoStopPosUm =ui.doubleSpinBoxStopPos->value();
   dataAcqThread.piezoTravelBackTime=ui.doubleSpinBoxPiezoTravelBackTime->value();

   dataAcqThread.nStacks=ui.spinBoxNumOfStacks->value();
   dataAcqThread.nFramesPerStack=ui.spinBoxFramesPerStack->value();
   dataAcqThread.exposureTime=ui.doubleSpinBoxExpTime->value();
   dataAcqThread.idleTimeBwtnStacks=ui.doubleSpinBoxBoxIdleTimeBtwnStacks->value();
   dataAcqThread.isBaselineClamp=ui.cbBaseLineClamp->isChecked();

   dataAcqThread.horShiftSpeedIdx=ui.comboBoxHorReadoutRate->currentIndex();
   dataAcqThread.preAmpGainIdx=ui.comboBoxPreAmpGains->currentIndex();
   dataAcqThread.emGain=ui.spinBoxEmGain->value();
   dataAcqThread.verShiftSpeedIdx=ui.comboBoxVertShiftSpeed->currentIndex();
   dataAcqThread.verClockVolAmp=ui.comboBoxVertClockVolAmp->currentIndex();

   dataAcqThread.preAmpGain=ui.comboBoxPreAmpGains->currentText();
   dataAcqThread.horShiftSpeed=ui.comboBoxHorReadoutRate->currentText();
   dataAcqThread.verShiftSpeed=ui.comboBoxVertShiftSpeed->currentText();

   //params for binning
   dataAcqThread.hstart=camera.hstart=ui.spinBoxHstart->value();
   dataAcqThread.hend  =camera.hend  =ui.spinBoxHend  ->value();
   dataAcqThread.vstart=camera.vstart=ui.spinBoxVstart->value();
   dataAcqThread.vend  =camera.vend  =ui.spinBoxVend  ->value();

   //TODO: temp
   L=-1;

   camera.setAcqParams(dataAcqThread.emGain,
                       dataAcqThread.preAmpGainIdx,
                       dataAcqThread.horShiftSpeedIdx,
                       dataAcqThread.verShiftSpeedIdx,
                       dataAcqThread.verClockVolAmp,
                       dataAcqThread.isBaselineClamp
                       );
   updateStatus(QString("Camera: applied params: ")+camera.getErrorMsg().c_str());

   //set filenames:
   QString headerFilename=ui.lineEditFilename->text();
   dataAcqThread.headerFilename=headerFilename; 
   if(!headerFilename.isEmpty()){
      dataAcqThread.aiFilename=replaceExtName(headerFilename, "ai");
      dataAcqThread.camFilename=replaceExtName(headerFilename, "cam");
      dataAcqThread.sifFileBasename=replaceExtName(headerFilename, "");
   }//else, save data

   dataAcqThread.stimFileContent=ui.textEditStimFileContent->toPlainText();
   dataAcqThread.comment=ui.textEditComment->toPlainText();
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

//TODO: make this func clearer
void Imagine::on_btnOpenStimFile_clicked()
{
   //
   QString stimFilename = QFileDialog::getOpenFileName(
                    this,
                    "Choose a stimulus file to open",
                    "",
                    "Stimulus files (*.stim);;All files(*.*)");
   if(stimFilename.isEmpty()) return;

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
   //QApplication::setOverrideCursor(Qt::WaitCursor);
   ui.textEditStimFileContent->setPlainText(in.readAll());


   vector<string> lines;
   if(!loadTextFile(stimFilename.toStdString(), lines)){
      //TODO: warn user file open failed.
      return;
   }

   //TODO: remove Comments and handle other fields

   int headerSize;
   QTextStream tt(lines[1].substr(string("header size=").size()).c_str());
   tt>>headerSize;

   int dataStartIdx=headerSize;
   stimuli.clear();

   for(int i=dataStartIdx; i<lines.size(); ++i){
      int valve, stack;
      QString line=lines[i].c_str();
      if(line.trimmed().isEmpty()) continue;
      QTextStream tt(&line);
      tt>>valve>>stack;
      stimuli.push_back(pair<int,int>(valve, stack) );
   }
   
   //fill in entries in table:
   ui.tableWidgetStimDisplay->setColumnCount(stimuli.size());
   QStringList tableHeader;
   for(int i=0; i<stimuli.size(); ++i){
      QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(stimuli[i].first ));
      ui.tableWidgetStimDisplay->setItem(0, i, newItem);
      tableHeader<<QString().setNum(stimuli[i].second);
   }//for,
   ui.tableWidgetStimDisplay->setHorizontalHeaderLabels(tableHeader);
   tableHeader.clear();
   tableHeader<<"valve";
   ui.tableWidgetStimDisplay->setVerticalHeaderLabels(tableHeader);
}

void Imagine::on_actionStimuli_triggered()
{
   ui.dwStim->setVisible(ui.actionStimuli->isChecked());
}

void Imagine::on_actionConfig_triggered()
{
   ui.dwCfg->setVisible(ui.actionConfig->isChecked());
}

void Imagine::on_actionLog_triggered()
{
   ui.dwLog->setVisible(ui.actionLog->isChecked());
}


//TODO: move this func to misc.cpp
QString addExtNameIfAbsent(QString filename, QString newExtname)
{
   QFileInfo fi(filename);
   QString ext = fi.suffix();
   if(ext.isEmpty()){
      return filename+"."+newExtname;
   }
   else {
      return filename;
   }
}//addExtNameIfAbsent()


void Imagine::on_btnSelectFile_clicked()
{
   QString imagineFilename = QFileDialog::getSaveFileName(
                    this,
                    "Choose an Imagine file name to save as",
                    "",
                    "Imagine files (*.imagine);;All files(*.*)");
   if(imagineFilename.isEmpty()) return;

   imagineFilename=addExtNameIfAbsent(imagineFilename, "imagine");
   ui.lineEditFilename->setText(imagineFilename);
}

void Imagine::updateStatus(ImagineStatus newStatus, ImagineAction newAction)
{
   curStatus=newStatus;
   curAction=newAction;

   vector<QWidget*> enabledWidgets, disabledWidgets;
   vector<QAction*> enabledActions, disabledActions;
   if(curStatus==eIdle){
      //in the order of from left to right, from top to bottom:
      enabledActions.push_back(ui.actionStartAcqAndSave);
      enabledActions.push_back(ui.actionStartLive);
      enabledActions.push_back(ui.actionOpenShutter);
      enabledActions.push_back(ui.actionCloseShutter);
      enabledActions.push_back(ui.actionTemperature);

      enabledWidgets.push_back(ui.btnApply);

      disabledActions.push_back(ui.actionStop);

      //no widgets to disable

   }//if, idle
   else if(curStatus==eRunning){
      enabledActions.push_back(ui.actionStop);

      //no widget to enable

      disabledActions.push_back(ui.actionStartAcqAndSave);
      disabledActions.push_back(ui.actionStartLive);
      //disabledActions.push_back(ui.actionOpenShutter);
      //disabledActions.push_back(ui.actionCloseShutter);
      //disabledActions.push_back(ui.actionTemperature);

      disabledWidgets.push_back(ui.btnApply);

      //change intensity plot xlabel
      if(curAction==eLive){
         intenPlot->setAxisTitle(QwtPlot::xBottom, "frame number");
      }
      else {
         intenPlot->setAxisTitle(QwtPlot::xBottom, "stack number");
      }

   }//else if, running
   else {
      disabledActions.push_back(ui.actionStop);
      
   }//else, stopping 

   for(int i=0; i<enabledWidgets.size(); ++i){
      enabledWidgets[i]->setEnabled(true);
   }

   for(int i=0; i<enabledActions.size(); ++i){
      enabledActions[i]->setEnabled(true);
   }

   for(int i=0; i<disabledWidgets.size(); ++i){
      disabledWidgets[i]->setEnabled(false);
   }

   for(int i=0; i<disabledActions.size(); ++i){
      disabledActions[i]->setEnabled(false);
   }

}//Imagine::updateUiStatus()

void Imagine::readPiezoCurPos()
{
   NiDaqAiReadOne ai(0); //channel 0: piezo
   int16 reading;
   ai.readOne(reading); //TODO: check error
   double vol=ai.toPhyUnit(reading);
   double um=vol/10*ui.doubleSpinBoxMaxDistance->value();
   ui.doubleSpinBoxCurPos->setValue(um);
}

void Imagine::on_tabWidgetCfg_currentChanged(int index)
{
   static bool hasRead=false;

   if(ui.tabWidgetCfg->currentWidget()==ui.tabPiezo){
      //QMessageBox::information(this, "test", "test");
      if(!hasRead){
         readPiezoCurPos();
         hasRead=true;
      }
   }
}


void Imagine::on_doubleSpinBoxCurPos_valueChanged(double newValue)
{
   //do nothing for now
}


void Imagine::on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue)
{
   if(ui.cbAutoSetPiezoTravelBackTime->isChecked()){
      ui.doubleSpinBoxPiezoTravelBackTime->setValue(newValue/2);
   }

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
   double um=ui.doubleSpinBoxCurPos->value();
   pPositioner->moveTo(um);
}


void Imagine::on_btnFastIncPosAndMove_clicked()
{
   ui.doubleSpinBoxCurPos->stepBy(10);
   on_btnMovePiezo_clicked();
}

void Imagine::on_btnIncPosAndMove_clicked()
{
   ui.doubleSpinBoxCurPos->stepBy(1);
   on_btnMovePiezo_clicked();
}

void Imagine::on_btnDecPosAndMove_clicked()
{
   ui.doubleSpinBoxCurPos->stepBy(-1);
   on_btnMovePiezo_clicked();
}

void Imagine::on_btnFastDecPosAndMove_clicked()
{
   ui.doubleSpinBoxCurPos->stepBy(-10);
   on_btnMovePiezo_clicked();
}


void Imagine::on_btnGetPiezoCurPos_clicked()
{
   readPiezoCurPos();
}

void Imagine::on_actionExit_triggered()
{
   close();
}

void Imagine::on_actionHeatsinkFan_triggered()
{
   if(!fanCtrlDialog){
      fanCtrlDialog=new FanCtrlDialog(this);
   }
   fanCtrlDialog->exec();
}
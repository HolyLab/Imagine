/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
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

#include <QtGui>
#include <QDir>

#include <math.h>
#include <fstream>
#include <vector>
#include <utility>

using namespace std;

#include <QDateTime>

#include "data_acq_thread.hpp"

#include "andor_g.hpp"
#include "timer_g.hpp"
#include "ni_daq_g.hpp"
#include "ai_thread.hpp"
#include "scoped_ptr_g.hpp"


Camera* pCamera;
NiDaqDo * digOut=NULL;
NiDaqAoWriteOne * aoOnce=NULL;

//defined in imagine.cpp:
extern vector<pair<int,int> > stimuli; //first: stim (valve), second: time (stack#)
extern int curStimIndex;

QString replaceExtName(QString filename, QString newExtname);


DataAcqThread::DataAcqThread(QObject *parent)
: QThread(parent)
{
   restart = false;
   abort = false;
   isLive=true;
}

DataAcqThread::~DataAcqThread()
{
   mutex.lock();
   abort = true;
   condition.wakeOne();
   mutex.unlock();

   wait();
}

void DataAcqThread::startAcq()
{
   stopRequested=false;
   this->start();
}

void DataAcqThread::stopAcq()
{
   stopRequested=true;
}

//join several lines into one line
QString linize(QString lines)
{
   lines.replace("\n", "\\n");
   return lines;
}

const string headerMagic="IMAGINE";
bool DataAcqThread::saveHeader(QString filename, NiDaqAi* ai)
{
   Camera& camera=*pCamera;

   ofstream header(filename.toStdString().c_str(), 
         ios::out|ios::trunc );
   header<<headerMagic<<endl;
   header<<"[general]"<<endl
      <<"header version=5.1"<<endl
      <<"app version=2.0, build ("<<__DATE__<<", "<<__TIME__<<")"<<endl
      <<"date and time="<<QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()<<endl
      <<"byte order=l"<<endl<<endl;

   header<<"[misc params]"<<endl
      <<"stimulus file content="
      <<(applyStim?linize(stimFileContent).toStdString():"")
      <<endl; 

   header<<"comment="<<linize(comment).toStdString()<<endl; //TODO: may need encode into one line

   header<<"ai data file="<<aiFilename.toStdString()<<endl
      <<"image data file="<<camFilename.toStdString()<<endl;

   //TODO: output shutter params
   //header<<"shutter=open time: ;"<<endl; 

   header<<"piezo=start position: "<<piezoStartPosUm<<" um"
      <<";stop position: "<<piezoStopPosUm<<" um"
      <<";output scan rate: "<<scanRateAo
      <<endl<<endl; 

   //ai related:
   header<<"[ai]"<<endl
      <<"nscans="<<-1<<endl //TODO: output nscans at the end
         <<"channel list=0 1 2 3"<<endl
         <<"label list=piezo$stimuli$camera frame TTL$heartbeat"<<endl
         <<"scan rate="<<ai->scanRate<<endl  
         <<"min sample="<<ai->minDigitalValue<<endl
         <<"max sample="<<ai->maxDigitalValue<<endl 
         <<"min input="<<ai->minPhyValue<<endl 
         <<"max input="<<ai->maxPhyValue<<endl<<endl; 

   //camera related:
   header<<"[camera]"<<endl
      <<"original image depth=14"<<endl    //effective bits per pixel
      <<"saved image depth=14"<<endl       //effective bits per pixel
      <<"image width="<<camera.getImageWidth()<<endl
      <<"image height="<<camera.getImageHeight()<<endl
      <<"number of frames requested="<<nStacks*nFramesPerStack<<endl
      <<"nStacks="<<nStacks<<endl
      <<"idle time between stacks="<<idleTimeBwtnStacks<<" s"<<endl;
   header<<"pre amp gain="<<preAmpGain.toStdString()<<endl
      <<"EM gain="<<emGain<<endl
      <<"exposure time="<<exposureTime<<" s" <<endl
      <<"vertical shift speed="<<verShiftSpeed.toStdString()<<endl
      <<"vertical clock vol amp="<<verClockVolAmp<<endl
      <<"readout rate="<<horShiftSpeed.toStdString()<<endl; 
   header<<"pixel order=x y z"<<endl
      <<"frame index offset=0"<<endl
      <<"frames per stack="<<nFramesPerStack<<endl
      <<"pixel data type=uint16"<<endl    //bits per pixel
      <<"camera="<<camera.getModel()<<endl 
      <<"um per pixel=-1"<<endl //TODO:
      <<"binning="<<"hbin:"<<camera.hbin
      <<";vbin:"<<camera.vbin
      <<";hstart:"<<camera.hstart
      <<";hend:"<<camera.hend
      <<";vstart:"<<camera.vstart
      <<";vend:"<<camera.vend<<endl; 

   header.close();

   return true;
}//DataAcqThread::saveHeader()


extern volatile bool isUpdatingImage;

void DataAcqThread::fireStimulus(int valve)
{
   emit newStatusMsgReady(QString("valve is open: %1").arg(valve));
   for(int i=0; i<4; ++i){
      digOut->updateOutputBuf(i, valve&1);
      valve>>=1;
   }
   digOut->write();
}//fireStimulus(),

void DataAcqThread::run()
{
   if(isLive) run_live();
   else run_acq_and_save();

   //this is for update ui status only:
   emit newStatusMsgReady(QString("acq-thread-finish"));
}//run()


void DataAcqThread::run_live()
{
   Camera& camera=*pCamera;

   //prepare for camera:
   camera.setAcqModeAndTime(Camera::eLive,
                            this->exposureTime, 
                            this->nFramesPerStack,
                            Camera::eInternalTrigger  //use internal trigger
                            );
   emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
      .arg(camera.getErrorMsg().c_str()));

   isUpdatingImage=false;

   int imageW=camera.getImageWidth();
   int imageH=camera.getImageHeight();
   int nPixels=imageW*imageH;
   Camera::PixelValue * frame=new Camera::PixelValue[nPixels];

   Timer_g timer;
   timer.start();

   camera.startAcq();
   emit newStatusMsgReady(QString("Camera: started acq: %1")
      .arg(camera.getErrorMsg().c_str()));


   long nDisplayUpdating=0;
   long nFramesGot=0;
   long nFramesGotCur;
   while(!stopRequested){
      nFramesGotCur=camera.getAcquiredFrameCount();

      if(nFramesGot!=nFramesGotCur && !isUpdatingImage){
         nFramesGot=nFramesGotCur;
         
         //get the last frame:
         camera.getLatestLiveImage(frame);

         //copy data
         QByteArray data16((const char*)frame, imageW*imageH*2);
         emit imageDataReady(data16, nFramesGot-1, imageW, imageH); //-1: due to 0-based indexing

         nDisplayUpdating++;
         if(nDisplayUpdating%10==0){
            emit newStatusMsgReady(QString("Screen update frame rate: %1")
               .arg(nDisplayUpdating/timer.read(), 5, 'f', 2) //width=5, fixed point, 2 decimal digits
               );
         }

         //spare some time for gui thread's event handling:
         QApplication::instance()->processEvents();
         double timeToWait=0.05;
         QThread::msleep(timeToWait*1000); // *1000: sec -> ms
      }
   }//while, user did not requested stop

   camera.stopAcq(); //TODO: check return value

   delete frame;  //TODO: use scoped ptr

   QString ttMsg="Live mode is stopped";
   emit newStatusMsgReady(ttMsg);
}//run_live()


void DataAcqThread::run_acq_and_save()
{
   Camera& camera=*pCamera;

   bool isAndor=camera.vendor=="andor";
   
   if(isAndor){
      isUseSpool=true;  //TODO: make ui aware this
   }
   else {
      isUseSpool=false;
   }

   isCreateFilePerStack=true; //TODO: make ui aware this


   ////prepare for AO AI and camera:
   
   ///prepare for camera:
   ///camera.setAcqModeAndTime(AndorCamera::eKineticSeries,
   camera.setAcqModeAndTime(Camera::eAcqAndSave,
                            this->exposureTime, 
                            this->nFramesPerStack,
                            this->triggerMode);
   emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
      .arg(camera.getErrorMsg().c_str()));

   //get the real params used by the camera:
   cycleTime=camera.getCycleTime();

   //prepare for AO:
   if(aoOnce){
      delete aoOnce;
      aoOnce=nullptr;
   }

   int aoChannelForPiezo=0; //TODO: put this configurable
   vector<int> aoChannels;
   aoChannels.push_back(aoChannelForPiezo);
   if(triggerMode==Camera::eExternalStart){
      int aoChannelForTrigger=1;
      aoChannels.push_back(aoChannelForTrigger);
   }
   NiDaqAo* ao=new NiDaqAo(aoChannels);

   double durationAo=nFramesPerStack*cycleTime; //in sec
   if(triggerMode==Camera::eInternalTrigger){
      durationAo+=0.1; 
   }
   else if(triggerMode==Camera::eExternalStart){
      durationAo+=piezoPreTriggerTime;
   }

   double durationResetPosition=idleTimeBwtnStacks/2;
   int nScansForReset=int(scanRateAo*durationResetPosition);
   ao->cfgTiming(scanRateAo, int(scanRateAo*durationAo)+nScansForReset); //+(): for reset position
   if(ao->isError()) {
      delete ao;
      emit newStatusMsgReady("error when configure AO timing");
      return;
   }

   //get buffer address and generate waveform:
   int aoStartValue=ao->toDigUnit(piezoStartPos);
   int aoStopValue=ao->toDigUnit(piezoStopPos);

   uInt16 * bufAo=ao->getOutputBuf();
   for(int i=0;i<ao->nScans-nScansForReset;i++){
      bufAo[i] = uInt16( double(i)/(ao->nScans-nScansForReset)*(aoStopValue-aoStartValue)+aoStartValue );
   }
   int firstScanNumberForReset=ao->nScans-nScansForReset;
   for(int i=firstScanNumberForReset; i<ao->nScans; ++i){
      bufAo[i] = uInt16( double(i-firstScanNumberForReset)/(nScansForReset)*(aoStartValue-aoStopValue)+aoStopValue );
   }
   bufAo[ao->nScans-1]=bufAo[0]; //the last sample resets piezo's position exactly

   if(triggerMode==Camera::eExternalStart){
      bufAo+=ao->nScans;
      int aoTTLHigh=ao->toDigUnit(5.0);            
      int aoTTLLow=ao->toDigUnit(0);
      for(int i=0;i<ao->nScans;i++){
         bufAo[i] = aoTTLLow; 
      }
      bufAo[int(piezoPreTriggerTime*scanRateAo)]=aoTTLHigh; //the sample that starts camera
   }

   ao->updateOutputBuf();
   //todo: check error like
   // if(ao->isError()) goto exitpoint;

   //prepare for AI:
   AiThread * aiThread=new AiThread(); //TODO: set buf size and scan rate here
   ScopedPtr_g<AiThread> ttScopedPtrAiThread(aiThread,false);

   //after all devices are prepared, now we can save the file header:
   camFilename=replaceExtName(headerFilename, "cam"); //NOTE: necessary if user overwrite files
   QString stackDir=replaceExtName(camFilename, "stacks");
   if(isUseSpool || isCreateFilePerStack){
      QDir::current().mkpath(stackDir);
   }
   if(isUseSpool){
      camFilename=stackDir+"\\stack_%1_";
   }
   else if(isCreateFilePerStack){
      camFilename=stackDir+"\\stack_%1";
   }

   ofstream *ofsAi=NULL, *ofsCam=NULL;
   if(isSaveData){
      saveHeader(headerFilename, aiThread->ai);
      ofsAi =new ofstream(aiFilename.toStdString().c_str(), 
         ios::binary|ios::out|ios::trunc );
      if(!isUseSpool && !isCreateFilePerStack){
         ofsCam=new ofstream(camFilename.toStdString().c_str(), 
            ios::binary|ios::out|ios::trunc );
      }
   }//if, save data to file

   isUpdatingImage=false;

   int imageW=camera.getImageWidth();
   int imageH=camera.getImageHeight();

   int nPixels=imageW*imageH;
   Camera::PixelValue * frame=new Camera::PixelValue[nPixels];

   idxCurStack=0;
   Timer_g timer;
   timer.start();

   //start AI
   aiThread->startAcq();

   //start stimuli[0] if necessary
   if(applyStim && stimuli[0].second==0){
      curStimIndex=0;
      fireStimulus(stimuli[curStimIndex].first);
   }//if, first stimulus should be fired before stack_0

nextStack:
   //open laser shutter
   digOut->updateOutputBuf(4,true);
   digOut->write();

   //TODO: may need delay for shutter open time

   emit newLogMsgReady(QString("Acquiring stack (0-based)=%1 @time=%2 s")
      .arg(idxCurStack)
      .arg(timer.read(), 10, 'f', 4) //width=10, fixed point, 4 decimal digits 
      );

   if(isUseSpool) {
      QString stemName=camFilename.arg(idxCurStack,4,10,QLatin1Char('0'));
      //enable spool
      SetSpool(1,2,(char*)(stemName.toStdString().c_str()),10); //Enabled, Mode 2, aPath stem and buffer size of 10
   }
   camera.startAcq();

   //for external start, put 100ms delay here to wait camera ready for trigger
   if(triggerMode==Camera::eExternalStart){
      double timeToWait=0.1;
      //TODO: maybe I should use busy waiting?
      QThread::msleep(timeToWait*1000); // *1000: sec -> ms
   }

   //start piezo movement (and may also trigger the camera):
   ao->start(); //todo: check error

   emit newStatusMsgReady(QString("Camera: started acq: %1")
      .arg(camera.getErrorMsg().c_str()));


   //bool isResize=false;
   long nFramesGotForStack=0;
   long nFramesGotForStackCur;
   while(true){
      nFramesGotForStackCur=camera.getAcquiredFrameCount();
      if(nFramesGotForStackCur==nFramesPerStack){
         break;
      }

      if(nFramesGotForStack!=nFramesGotForStackCur && !isUpdatingImage){
         nFramesGotForStack=nFramesGotForStackCur;

         //get the latest frame:
         camera.getLatestLiveImage(frame);
         
         //copy data
         QByteArray data16((const char*)frame, imageW*imageH*2);
         emit imageDataReady(data16, nFramesGotForStack-1, imageW, imageH); //-1: due to 0-based indexing
      }
   }//while, camera is not idle

   //get the last frame if nec
   if(nFramesGotForStack<nFramesPerStack){
      camera.getLatestLiveImage(frame);
      QByteArray data16((const char*)frame, imageW*imageH*2);
      emit imageDataReady(data16, nFramesPerStack-1, imageW, imageH); //-1: due to 0-based indexing
   }

   //close laser shutter
   digOut->updateOutputBuf(4,false);
   digOut->write();

   //update stimulus if necessary:  idxCurStack
   if(applyStim 
        && curStimIndex+1<stimuli.size() 
        && stimuli[curStimIndex+1].second==idxCurStack+1){
      curStimIndex++;
      fireStimulus(stimuli[curStimIndex].first);
   }//if, should update stimulus

   if(!isUseSpool){
      //get camera's data:
      bool result=camera.transferData();
      if(!result){
         emit newStatusMsgReady(QString(camera.getErrorMsg().c_str())
            +QString("(errCode=%1)").arg(camera.getErrorCode()));
         return;
      }
   }

   //save data to files:
   //save camera's data:
   if(isSaveData && !isUseSpool){
      if(isCreateFilePerStack){
         ofsCam=new ofstream(
            camFilename.arg(idxCurStack,4,10,QLatin1Char('0')).toStdString().c_str(), 
            ios::binary|ios::out|ios::trunc );
      }
      Camera::PixelValue * imageArray=camera.getImageArray();
      ofsCam->write((const char*)imageArray, 
         sizeof(AndorCamera::PixelValue)*nFramesPerStack*imageW*imageH );
      if(!*ofsCam){
         //TODO: deal with the error
      }//if, error occurs when write camera data
      ofsCam->flush();
      if(isCreateFilePerStack){
         ofsCam->close();
         delete ofsCam;
         ofsCam=nullptr;
      }

   }//if, save data to file and not using spool

   //save ai data:
   if(isSaveData){
      aiThread->save(*ofsAi);
      ofsAi->flush();
   }

   //wait until piezo's orig position is reset
   ao->wait(-1); //wait forever
   //stop ao task:
   ao->stop();

   idxCurStack++;  //post: it is #stacks we got so far
   if(idxCurStack<this->nStacks && !stopRequested){
      double timePerStack=nFramesPerStack*cycleTime+idleTimeBwtnStacks;
      double timeToWait=timePerStack*idxCurStack-timer.read();
      if(timeToWait<0){
         emit newLogMsgReady("WARNING: overrun: idle time is too short.");
      }
      if(timeToWait>0.01){
         QThread::msleep(timeToWait*1000); // *1000: sec -> ms
      }//if, need wait more than 10ms

      goto nextStack;
   }//if, there're more stacks and no stop requested


   //fire post-seq stimulus:
   if(applyStim){
      int postSeqStim=0; //TODO: get this value from stim file header
      fireStimulus(postSeqStim);  
   }

   delete frame;  //TODO: use scoped ptr

   delete ao;     //TODO: use scoped ptr

   //wait 200ms to have Piezo settled (so ai thread can record its final position)
   QThread::msleep(0.2*1000); // *1000: sec -> ms

   aiThread->stopAcq();
   if(isSaveData){
      aiThread->save(*ofsAi);

      //TODO: do we really need these close()?
      if(!isUseSpool && !isCreateFilePerStack){
         ofsCam->close();
         delete ofsCam;   //TODO: use scoped ptr
         ofsCam=0;
      }

      ofsAi->close();
      delete ofsAi;    //TODO: use scoped ptr
   }

   if(isUseSpool){
      //disable spool
      SetSpool(0,2,NULL,10); //disable spooling
   }

   camera.stopAcq();

   QString ttMsg="Acquisition is done";
   if(stopRequested) ttMsg+=" (User requested STOP)";
   emit newStatusMsgReady(ttMsg);
}//run_acq_and_save()

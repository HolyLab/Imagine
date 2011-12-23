#ifndef COOKEWORKERTHREAD_H
#define COOKEWORKERTHREAD_H

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <assert.h>

#include "cooke.hpp"
#include "lockguard.h"
#include "spoolthread.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


class CookeCamera::WorkerThread: public QThread {
   Q_OBJECT
private:
   CookeCamera* camera;
   SpoolThread* spoolingThread;

   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   WorkerThread(CookeCamera * camera, QObject *parent = 0)
      : QThread(parent){
      this->camera=camera;
      
      if(camera->isSpooling()){
         spoolingThread=new SpoolThread(camera->ofsSpooling, 
            camera->getImageWidth()*camera->getImageHeight()*sizeof(CookeCamera::PixelValue));
         spoolingThread->start();
      }
      else {
         spoolingThread=nullptr;
      }

      shouldStop=false;
   }
   ~WorkerThread(){
      if(spoolingThread){
         spoolingThread->wait();
         delete spoolingThread;
         spoolingThread=nullptr;
      }
   }

   void requestStop(){
      shouldStop=true;
      if(spoolingThread)spoolingThread->requestStop();
   }

   void append2seq(CookeCamera::PixelValue* frame, int frameIdx, int nPixelsPerFrame){
      //if(spoolingThread) assert(sizeof(Camera::PixelValue)*nPixelsPerFrame==spoolThread->itemSize);
      if(spoolingThread){
         spoolingThread->appendItem((char*)frame);
      }
      else {
         memcpy(camera->pImageArray+frameIdx*nPixelsPerFrame, frame, sizeof(Camera::PixelValue)*nPixelsPerFrame);

      }
   }

   void run(){
#if defined(_DEBUG)
      cerr<<"enter cooke worker thread run()"<<endl;
#endif
      while(true){
         if(shouldStop) break;
         ///wait for events
         int waitResult=WaitForMultipleObjects(2, camera->mEvent, false, 5000);
         switch(waitResult){
            case WAIT_OBJECT_0 + 0:
            case WAIT_OBJECT_0 + 1:
               break; //break the switch
            default: //WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
               //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
               shouldStop=true; //or goto
         }//switch,
         if(shouldStop)break; //break the while
         int eventIdx=waitResult-WAIT_OBJECT_0;
         //todo: should we call GetBufferStatus() to double-check the status about transferring? SEE: demo.cpp
         
         CLockGuard tGuard(camera->mpLock);

         PixelValue* rawData=camera->mRingBuf[eventIdx];
         long counter=camera->extractFrameCounter(rawData);
         if(camera->nAcquiredFrames==0) {
            camera->firstFrameCounter=counter;
            cout<<"first frame's counter is "<<counter<<endl;
         }
         int curFrameIdx=counter-camera->firstFrameCounter;

         //todo: tmp
         //curFrameIdx=camera->nAcquiredFrames;


         assert(curFrameIdx>=0);
         long nPixelsPerFrame=camera->getImageHeight()*camera->getImageWidth();

         //fill the gap w/ black images
         int gapWidth=0;
         for(int frameIdx=camera->nAcquiredFrames; frameIdx<min(camera->nFrames, curFrameIdx); ++frameIdx){
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               append2seq(camera->pBlackImage, frameIdx, nPixelsPerFrame);
            }
            gapWidth++;
         }
         if(gapWidth) cout<<"fill "<<gapWidth<<" frames with black images (start at frame idx="<<camera->nAcquiredFrames<<")"<<endl;
         camera->totalGap+=gapWidth;

         //fill the frame array and live image
         if(curFrameIdx<camera->nFrames){
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               append2seq(rawData, curFrameIdx, nPixelsPerFrame);
            }
            memcpy(camera->pLiveImage, rawData, sizeof(CookeCamera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=curFrameIdx+1;
         }
         else {
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               memcpy(camera->pLiveImage, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
               camera->nAcquiredFrames=camera->nFrames;
            }
            else {
               memcpy(camera->pLiveImage, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
               camera->nAcquiredFrames=curFrameIdx+1;
            }
         }

         //reset event
         ResetEvent(camera->mEvent[eventIdx]);
      
         ///then add back the buffer
         if(camera->nAcquiredFrames<camera->nFrames || camera->genericAcqMode==Camera::eLive){
            //in fifo mode, frameIdxInCamRam are 0 for both buffers?
            int frameIdxInCamRam=0;
            camera->errorCode = PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]);// Add buffer to the driver queue
            if(camera->errorCode!=PCO_NOERROR) {
               camera->errorMsg="failed to add buffer";
               break; //break the while
            }
         }
         else {
            break;
         }
      }//while,
#if defined(_DEBUG)
      cerr<<"leave cooke worker thread run()"<<endl;
#endif
   }//run(),
};//class, CookeCamera::WorkerThread


#endif // COOKEWORKERTHREAD_H

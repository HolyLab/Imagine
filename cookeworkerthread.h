#ifndef COOKEWORKERTHREAD_H
#define COOKEWORKERTHREAD_H

#include <iostream>
using std::cout;
using std::endl;

#include <assert.h>

#include "cooke.hpp"
#include "lockguard.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


class CookeCamera::WorkerThread: public QThread {
   Q_OBJECT
private:
   CookeCamera* camera;
   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   WorkerThread(CookeCamera * camera, QObject *parent = 0)
      : QThread(parent){
      this->camera=camera;
      shouldStop=false;
   }
   ~WorkerThread(){}

   void requestStop(){
      shouldStop=true;
   }

   void run(){
      while(true){
         if(shouldStop) break;
         ///wait for events
         int waitResult=WaitForMultipleObjects(2, camera->mEvent, false, 500);
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
         curFrameIdx=camera->nAcquiredFrames;


         assert(curFrameIdx>=0);
         long nPixelsPerFrame=camera->getImageHeight()*camera->getImageWidth();

         //fill the gap w/ black images
         int gapWidth=0;
         for(int frameIdx=camera->nAcquiredFrames; frameIdx<min(camera->nFrames, curFrameIdx); ++frameIdx){
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               memcpy(camera->pImageArray+frameIdx*nPixelsPerFrame, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            }
            gapWidth++;
         }
         if(gapWidth) cout<<"fill "<<gapWidth<<" frames with black images (start at frame idx="<<camera->nAcquiredFrames<<")"<<endl;

         //fill the frame array and live image
         if(curFrameIdx<camera->nFrames){
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               memcpy(camera->pImageArray+curFrameIdx*nPixelsPerFrame, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            }
            memcpy(camera->pLiveImage, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=curFrameIdx+1;
         }
         else {
            memcpy(camera->pLiveImage, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=camera->nFrames;
         }

         //reset event then add back the bufffer
         ResetEvent(camera->mEvent[eventIdx]);
         if(camera->nAcquiredFrames<camera->nFrames){
            //in fifo mode, frameIdxInCamRam are 0 for both buffers?
            int frameIdxInCamRam=0;
            camera->errorCode = PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]);// Add buffer to the driver queue
            if(camera->errorCode!=PCO_NOERROR) {
               camera->errorMsg="failed to add buffer";
               break; //break the while
            }
         }
         else {
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               break;
            }
         }
      }//while,
   }//run(),
};//class, CookeCamera::WorkerThread


#endif // COOKEWORKERTHREAD_H

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

extern Timer_g gTimer;

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
         cout<<"after new spoolthread: "<<gTimer.read()<<endl;

         spoolingThread->start();
      }
      else {
         spoolingThread=nullptr;
      }

      shouldStop=false;

      cout<<"at the end of workerThread->ctor(): "<<gTimer.read()<<endl;
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
      vector<int> nBlackFrames, blackFrameStartIndices, curFrameIndices;
      curFrameIndices.reserve(camera->nFrames);
      cerr<<"enter cooke worker thread run()"<<endl;
#endif
      while(true){
         //if(shouldStop) break;
         ///wait for events
         int waitResult=WaitForMultipleObjects(camera->nBufs, camera->mEvent, false, 500000);
         if(waitResult<WAIT_OBJECT_0 || waitResult>=WAIT_OBJECT_0+camera->nBufs) {
            break; //break the while
            //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
         }//if, WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
         int eventIdx=waitResult-WAIT_OBJECT_0;
         //todo: should we call GetBufferStatus() to double-check the status about transferring? SEE: demo.cpp
         
         CLockGuard tGuard(camera->mpLock);

         PixelValue* rawData=camera->mRingBuf[eventIdx];
         long counter=camera->extractFrameCounter(rawData);
         if(camera->nAcquiredFrames==0) {
            camera->firstFrameCounter=counter;
            assert(counter>=0);
            cout<<"first frame's counter is "<<counter<<endl;
         }
         int curFrameIdx=counter-camera->firstFrameCounter;

         //todo: tmp
         //curFrameIdx=camera->nAcquiredFrames;

#ifdef _DEBUG
         curFrameIndices.push_back(curFrameIdx);
#endif


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
         if(gapWidth) {
            //tmp: __debugbreak();
            cout<<"fill "<<gapWidth<<" frames with black images (start at frame idx="<<camera->nAcquiredFrames<<")"<<endl;
            camera->totalGap+=gapWidth;
#ifdef _DEBUG
            nBlackFrames.push_back(gapWidth);
            blackFrameStartIndices.push_back(camera->nAcquiredFrames);
#endif
         }

         //fill the frame array and live image
         if(curFrameIdx<camera->nFrames){
            if(camera->genericAcqMode==Camera::eAcqAndSave){
               append2seq(rawData, curFrameIdx, nPixelsPerFrame);
            }

            // CLockGuard tGuard(camera->mpLock); //need the lock on the camera only for live img & nAcquiredFrames
            //tmp: 
            memcpy_g(camera->pLiveImage, rawData, sizeof(CookeCamera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=max(curFrameIdx+1, camera->nAcquiredFrames); //don't got back
         }
         else {
            // CLockGuard tGuard(camera->mpLock); //need the lock on the camera only for live img & nAcquiredFrames

            if(camera->genericAcqMode==Camera::eAcqAndSave){
               memcpy_g(camera->pLiveImage, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
               camera->nAcquiredFrames=camera->nFrames;
            }
            else {
               memcpy_g(camera->pLiveImage, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
               camera->nAcquiredFrames=curFrameIdx+1;
            }
         }

         //reset event
         ResetEvent(camera->mEvent[eventIdx]);
      
         if(shouldStop) break;

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
      cerr<<"black frame info:"<<endl;
      for(unsigned i=0; i<nBlackFrames.size(); ++i){
         cerr<<nBlackFrames[i]<<"@"<<blackFrameStartIndices[i]<<" \t";
         if(i%8==7) cerr<<endl;
      }
      cerr<<"end of black frame info"<<endl;
#endif
   }//run(),
};//class, CookeCamera::WorkerThread


#endif // COOKEWORKERTHREAD_H

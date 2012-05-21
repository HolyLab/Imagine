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

#include <Windows.h>
#include <assert.h>


#include "avt_g.hpp"
#include "lockguard.h"

#include <iostream>
using std::cout;
using std::endl;

// wait for a camera to be plugged
bool wait4Camera()
{
    printf("waiting for a camera ...\n");
    for(int i=0; i<3; ++i){
      if(PvCameraCount() ) //&& !GCamera.Abort)
           return true;
      else {
         printf("... wait 3 more seconds\n");
         Sleep(3000);
      }
    }
    printf("... give up.\n");

    return false;
}


bool AvtCamera::init()
{
   errorMsg="Camera Initialization failed when: ";

   errorCode=PvInitialize();
   if(errorCode!=ePvErrSuccess){
      errorMsg+="call PvInitialize";
      return false;
   }

   // wait for a camera to be plugged
   if(!wait4Camera()){
      PvUnInitialize();
      errorMsg+="no camera found";
      return false;
   }

   tPvUint32 count,connected;
   tPvCameraInfo list;

   count = PvCameraList(&list,1,&connected);
   if(count == 1)    {
        cameraID = list.UniqueId;
        model=list.DisplayName; //TODO: null-terminated?
   }
   else {
      errorMsg+="can't find one and only one AVT camera"; 
      return false;
   }

   errorCode=PvCameraOpen(cameraID,ePvAccessMaster,&cameraHandle);   

   if(errorCode!=ePvErrSuccess){
      errorMsg+="open camera";
      return false;
   }

   unsigned long sWidth,sHeight;
   errorCode = PvAttrUint32Get(cameraHandle,"SensorWidth",&sWidth);

   if(errorCode!=ePvErrSuccess){
      errorMsg+="get camera chip width";
      return false;
   }
   
   errorCode = PvAttrUint32Get(cameraHandle,"SensorHeight",&sHeight);

   if(errorCode!=ePvErrSuccess){
      errorMsg+="get camera chip height";
      return false;
   }

   this->chipWidth=sWidth;
   this->chipHeight=sHeight;



   //todo: maybe we should make it a param. 
   //For avt, it should be <=100.
   circBufSize=100;

   //alloc circular buf
   pRealCircBuf=new PixelValue[chipWidth*chipHeight*circBufSize+31];

   if(!pRealCircBuf){
      errorCode=getExtraErrorCode(eOutOfMem);
      errorMsg+="no enough mem";
      return false;
   }

   //todo: align it
   pCircBuf=pRealCircBuf; //tmp:


   //the latest live image
   pLiveImage=new PixelValue[chipWidth*chipHeight+31];


   //the black image
   //todo: may need align too
   int size=chipWidth*chipHeight+31;
   pBlackImage=new PixelValue[size];
   memset(pBlackImage, 0, size*sizeof(PixelValue)); //all zeros



   //prepare the tPvFrame structure
   pFrames=new tPvFrame[circBufSize];
   if(!pFrames){
      errorCode=getExtraErrorCode(eOutOfMem);
      errorMsg+="not enough mem";
      return false;
   }

   //NOTE: now we can start camera

   return true;

}

bool AvtCamera::fini()
{
   //unsetup the camera

   //printf("clearing the queue\n");  
   // dequeue all the frame still queued (this will block until they all have been dequeued)
   PvCaptureQueueClear(cameraHandle);
   // then close the camera
   // printf("and closing the camera\n");  
   PvCameraClose(cameraHandle);

   // delete all the allocated buffers
   /* TODO
    for(int i=0;i<FRAMESCOUNT;i++){
        delete [] (char*)GCamera.Frames[i].ImageBuffer;
    }
    */
    cameraHandle = NULL;

    // TODO  uninitialise the API
    PvUnInitialize();

    return true;
}


bool AvtCamera::setAcqParams(int gain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) 
{
   //TODO: the gain settings

   ///pixel format
   errorCode=PvAttrEnumSet(cameraHandle,"PixelFormat","Mono16");
   if(errorCode!=ePvErrSuccess) return false;

   ///gain:
   errorCode=PvAttrEnumSet(cameraHandle,"GainMode","Manual");
   if(errorCode!=ePvErrSuccess) return false;
   errorCode=PvAttrUint32Set(cameraHandle,"GainValue",gain);
   if(errorCode!=ePvErrSuccess) return false;


   ///set image dim
   ///TODO: check error code and make sure the start/end/etc are in bin-ed unit
   errorCode=PvAttrUint32Set(cameraHandle,"BinningX",hbin);
   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvAttrUint32Set(cameraHandle,"BinningY",vbin);
   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvAttrUint32Set(cameraHandle,"RegionX",hstart);
   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvAttrUint32Set(cameraHandle,"RegionY",vstart);
   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvAttrUint32Set(cameraHandle,"Width",getImageWidth());
   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvAttrUint32Set(cameraHandle,"Height",getImageHeight());
   if(errorCode!=ePvErrSuccess) return false;


   //
    unsigned long sWidth,sHeight;
    unsigned long rWidth,rHeight;
    unsigned long CenterX,CenterY;
    PvAttrUint32Get(cameraHandle,"Width",&rWidth);
    PvAttrUint32Get(cameraHandle,"Height",&rHeight);
    cout<<"new width/height: "<<rWidth<<"/"<<rHeight<<endl;


   return true;
}//setAcqParams(),

/*
// callback called when a frame is done
void _STDCALL FrameDoneCB(tPvFrame* pFrame)
{
    // if the frame was completed (or if data were missing/lost) we re-enqueue it
    if(pFrame->Status == ePvErrSuccess  || 
       pFrame->Status == ePvErrDataLost ||
       pFrame->Status == ePvErrDataMissing)
        PvCaptureQueueFrame(GCamera.Handle,pFrame,FrameDoneCB);  
}
*/

// callback called when a frame is done
void  __stdcall onFrameDone(tPvFrame* pFrame)
{
   // if the frame was completed we re-enqueue it
   cout<<"pFrame->FrameCount=="<<pFrame->FrameCount<<endl;
   AvtCamera* pCamera=(AvtCamera*)pFrame->Context[0];
   int frameIdx=(int)pFrame->Context[1];
   int nPixels=pCamera->getImageHeight()*pCamera->getImageWidth();

   CLockGuard tGuard(pCamera->mpLock);
   
   //if suc: the image; otherwise all zeros
   void *src=pFrame->Status == ePvErrSuccess?pFrame->ImageBuffer:pCamera->pBlackImage;

   ///the live image
   memcpy(pCamera->pLiveImage, 
      src, 
      sizeof(Camera::PixelValue)*nPixels
      );

   //if(pCamera->genericAcqMode==Camera::eAcqAndSave){
   //	   if(pCamera->nAcquiredFrames>=pCamera->nFrames) return;
   //}

   //if(pFrame->Status == ePvErrSuccess) pCamera->nAcquiredFrames=pFrame->FrameCount;
   //else {
      pCamera->nAcquiredFrames++;
   //}

   ///the saved buf/file
   if(pCamera->genericAcqMode==Camera::eAcqAndSave){
      assert((pCamera->nAcquiredFrames-1)%pCamera->circBufSize==frameIdx);

      if(pCamera->nAcquiredFrames<=pCamera->nFrames){
         if(pCamera->isSpooling()){
            pCamera->ofsSpooling->write((const char*)src, sizeof(Camera::PixelValue)*nPixels);
         }
         else {
            memcpy(pCamera->pImageArray+(pCamera->nAcquiredFrames-1)*nPixels, 
               src, 
               sizeof(Camera::PixelValue)*nPixels
               );
         }
      }
   }

   if(//(pCamera->nAcquiredFrames<pCamera->nFrames || pCamera->genericAcqMode==Camera::eLive)
        //&& 
		pFrame->Status != ePvErrUnplugged 
        && pFrame->Status != ePvErrCancelled
      ){
      PvCaptureQueueFrame(pCamera->cameraHandle,pFrame, onFrameDone);
   }
}//onFrameDone(),


double AvtCamera::getCycleTime()
{
   tPvUint32 time;

   PvAttrUint32Get(cameraHandle, "ExposureValue", &time);

   return time/1000.0/1000;
}


//params different for live from for save mode
bool AvtCamera::setAcqModeAndTime(GenericAcqMode genericAcqMode,
                          float exposure,
                          int anFrames,  //used only in kinetic-series mode
                          TriggerMode triggerMode
                          )
{
   this->genericAcqMode=genericAcqMode;

   this->nFrames=anFrames;
   this->triggerMode=triggerMode;

   errorCode=ePvErrSuccess;


   //fill the remaining fields of tPvFrame struct
   for(int frameIdx=0; frameIdx<circBufSize; ++frameIdx){
      pFrames[frameIdx].ImageBuffer=pCircBuf+frameIdx*getImageWidth()*getImageHeight(); 
      pFrames[frameIdx].ImageBufferSize=getImageWidth()*getImageHeight()*sizeof(PixelValue); 
      pFrames[frameIdx].Context[0]=this;
      pFrames[frameIdx].Context[1]=(void*)frameIdx;
   }

   ////set trigger mode

   ///trigger for sequence/acquisition
   if(triggerMode==eInternalTrigger){
      errorCode=PvAttrEnumSet(cameraHandle,"AcqStartTriggerMode","Disabled");
   }
   else {
      errorCode=PvAttrEnumSet(cameraHandle,"AcqStartTriggerMode","SyncIn1");
   }
   if(errorCode!=ePvErrSuccess){
      errorMsg="error when set acq trigger mode";
      return false;
   }

   ///trigger for frame
   errorCode=PvAttrEnumSet(cameraHandle,"FrameStartTriggerMode","Freerun");
   if(errorCode!=ePvErrSuccess){
      errorMsg="error when set frame trigger mode";
      return false;
   }

   ///set exp mode 
   errorCode=PvAttrEnumSet(cameraHandle,"ExposureMode","Manual");

   /// and time
   errorCode=PvAttrUint32Set(cameraHandle, "ExposureValue", exposure*1000*1000);

   ///set acq mode
   if(genericAcqMode==eLive){
      errorCode=PvAttrEnumSet(cameraHandle,"AcquisitionMode","Continuous");
   }
   else {
      //set up #frames to acq
      errorCode=PvAttrUint32Set(cameraHandle, "AcquisitionFrameCount", nFrames);

      //todo: maybe should be "Continuous"
      errorCode=PvAttrEnumSet(cameraHandle,"AcquisitionMode","MultiFrame"); 

      //PvAttrEnumSet(cameraHandle,"AcquisitionMode","Continuous");
   }

   tPvFloat32 frameRate=-1;
   errorCode=PvAttrFloat32Get(cameraHandle, "FrameRate", &frameRate);
   cout<<"frame rate is: "<<frameRate<<endl;

   ///take care of image array saving (in mem only)
   if(genericAcqMode==Camera::eAcqAndSave && !isSpooling()){
	   if(!allocImageArray(nFrames,false)){
		   return false;
	   }//if, fail to alloc enough mem
   }

   return errorCode==ePvErrSuccess;
}


long AvtCamera::getAcquiredFrameCount()
{
   CLockGuard tGuard(mpLock);
   return nAcquiredFrames;
}


bool AvtCamera::startAcq()
{
   CLockGuard tGuard(mpLock);
   nAcquiredFrames=0; //todo: maybe too late if external trigger?

   //init the capture stream
   errorCode=PvCaptureStart(cameraHandle);
   if(errorCode!=ePvErrSuccess){
      errorMsg="error when PvCaptureStart";
      return false;
   }

   //enqueue the frames
   for(int frameIdx=0; frameIdx<circBufSize; ++frameIdx){
      errorCode=PvCaptureQueueFrame(cameraHandle,&pFrames[frameIdx], onFrameDone);
      if(errorCode!=ePvErrSuccess){
         errorMsg="error when queue frames";
         return false;
      }
   }


   if(triggerMode==eInternalTrigger){
      errorCode=PvCommandRun(cameraHandle,"AcquisitionStart");
   }
   else {
      //todo: external triggered, no nec?
      errorCode=PvCommandRun(cameraHandle,"AcquisitionStart");
   }

   return errorCode==ePvErrSuccess;
}


bool AvtCamera::stopAcq()
{
   errorCode=PvCommandRun(cameraHandle,"AcquisitionStop");

   if(errorCode!=ePvErrSuccess) return false;

   errorCode=PvCaptureEnd(cameraHandle) ;

   if(errorCode!=ePvErrSuccess) return false;

   /// dequeue all the frame still queued (this will block until they all have been dequeued)
   
   //CLockGuard tGuard(mpLock); //SEE: PvCaptureQueueClear()

   errorCode=PvCaptureQueueClear(cameraHandle);

   if(errorCode!=ePvErrSuccess) return false;

   return errorCode==ePvErrSuccess;
}


bool AvtCamera::getLatestLiveImage(PixelValue * frame)
{
   CLockGuard tGuard(mpLock);

   if(nAcquiredFrames<=0)   return false;

   int nPixels=getImageHeight()*getImageWidth();
   memcpy(frame, pLiveImage, nPixels*sizeof(PixelValue));

   return true;
}

/*
bool AvtCamera::isIdle()
{
   CLockGuard tGuard(mpLock);

   return mIsIdle;
}
*/

bool AvtCamera::setSpooling(string filename)
{
   if(ofsSpooling){
      delete ofsSpooling; //the file is closed too
      ofsSpooling=nullptr;
   }

   Camera::setSpooling(filename);

   if(isSpooling()){
      int bufsize_in_4kb=64*1024*1024/(4*1024); //64M
      ofsSpooling=new FastOfstream(filename.c_str(), bufsize_in_4kb);
      return *ofsSpooling;
   }
   else return true;

}


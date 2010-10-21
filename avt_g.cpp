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

#include <Windows.h>

#include "avt_g.hpp"
#include "lockguard.h"


// wait for a camera to be plugged
void WaitForCamera()
{
    //printf("waiting for a camera ...\n");
    while(!PvCameraCount() ) //&& !GCamera.Abort)
        Sleep(250);
    //printf("\n");
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
   WaitForCamera();

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


bool AvtCamera::setAcqParams(int emGain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) 
{
   //TODO: the gain settings

   ///set image dim
   ///TODO: check error code and make sure the start/end/etc are in bin-ed unit
   PvAttrUint32Set(cameraHandle,"BinningX",hbin);

   PvAttrUint32Set(cameraHandle,"BinningY",vbin);

   PvAttrUint32Set(cameraHandle,"RegionX",hstart);

   PvAttrUint32Set(cameraHandle,"RegionY",vstart);

   PvAttrUint32Set(cameraHandle,"width",getImageWidth());

   PvAttrUint32Set(cameraHandle,"height",getImageHeight());


   //


   return true;
}

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
   //cout<<"frame #"<<pFrame->FrameCount<<endl;
   AvtCamera* pCamera=(AvtCamera*)pFrame->Context[0];
   int frameIdx=(int)pFrame->Context[1];
   int nPixels=pCamera->getImageHeight()*pCamera->getImageWidth();

   CLockGuard tGuard(pCamera->mpLock);

   //update live image; save to buf/file
   //update counter
   //if nec, re-enqueue

   //if suc: the image; otherwise all zeros
   void *src=pFrame->Status == ePvErrSuccess?pFrame->ImageBuffer:pCamera->pBlackImage;

   ///the live image
   memcpy(pCamera->pLiveImage, 
      src, 
      sizeof(Camera::PixelValue)*nPixels
      );

   ///the saved buf

   if(){//pFrame->Status != ePvErrUnplugged && pFrame->Status != ePvErrCancelled
   }
   
      PvCaptureQueueFrame(pCamera->cameraHandle,pFrame, onFrameDone);
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


   //init the capture stream
   PvCaptureStart(cameraHandle);

   //fill the remaining fields of tPvFrame struct
   for(int frameIdx=0; frameIdx<circBufSize; ++frameIdx){
      pFrames[frameIdx].ImageBuffer=pCircBuf+frameIdx*getImageWidth()*getImageHeight(); 
      pFrames[frameIdx].ImageBufferSize=getImageWidth()*getImageHeight()*sizeof(PixelValue); 
      pFrames[frameIdx].Context[0]=this;
      pFrames[frameIdx].Context[1]=(void*)frameIdx;
   }

   //enqueue the frames
   for(int frameIdx=0; frameIdx<circBufSize; ++frameIdx){
      PvCaptureQueueFrame(cameraHandle,&pFrames[frameIdx], onFrameDone);
   }

   ////set trigger mode

   ///trigger for sequence/acquisition
   if(triggerMode==eInternalTrigger){
      PvAttrEnumSet(cameraHandle,"AcqStartTriggerMode","Disabled");
   }
   else {
      PvAttrEnumSet(cameraHandle,"AcqStartTriggerMode","SyncIn1");
   }

   ///trigger for frame
   PvAttrEnumSet(cameraHandle,"FrameStartTriggerMode","Freerun");

   ///set exp mode 
   PvAttrEnumSet(cameraHandle,"ExposureMode","Manual");

   /// and time
   PvAttrUint32Set(cameraHandle, "ExposureValue", exposure*1000*1000);

   ///set acq mode
   if(genericAcqMode==eLive){
      PvAttrEnumSet(cameraHandle,"AcquisitionMode","Continuous");
   }
   else {
      //set up #frames to acq
      PvAttrUint32Set(cameraHandle, "AcquisitionFrameCount", nFrames);

      //todo: maybe should be "Continuous"
      PvAttrEnumSet(cameraHandle,"AcquisitionMode","MultiFrame"); 

      //PvAttrEnumSet(cameraHandle,"AcquisitionMode","Continuous");
   }


   return true;
}


long AvtCamera::getAcquiredFrameCount()
{
   CLockGuard tGuard(mpLock);
   return nAcquiredFrames;
}


bool AvtCamera::startAcq()
{
   CLockGuard tGuard(mpLock);
   nAcquiredFrames=0;

   if(triggerMode==eInternalTrigger){
      PvCommandRun(cameraHandle,"AcquisitionStart");
   }
   else {
      //todo: external triggered, no nec?
      PvCommandRun(cameraHandle,"AcquisitionStart");
   }

   return true;
}


bool AvtCamera::stopAcq()
{
   //todo: do we need to clear the queue?

   PvCommandRun(cameraHandle,"AcquisitionStop");

   PvCaptureEnd(cameraHandle) ;

   return true;
}


bool AvtCamera::getLatestLiveImage(PixelValue * frame)
{
   CLockGuard tGuard(mpLock);

   if(nAcquiredFrames<=0)   return false;

   int nPixels=getImageHeight()*getImageWidth();
   memcpy(frame, pLiveImage, nPixels*sizeof(PixelValue));

   return true;
}


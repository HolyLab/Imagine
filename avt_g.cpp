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

   //todo: set trigger mode

   ///todo: set acq mode
   PvAttrEnumSet(cameraHandle,"FrameStartTriggerMode","Freerun");

   ///todo: set exp time

   ///todo: set up #frames to acq



   return true;
}




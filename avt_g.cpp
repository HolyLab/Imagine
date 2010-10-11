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
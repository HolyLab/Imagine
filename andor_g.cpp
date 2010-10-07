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


#include "andor_g.hpp"

bool AndorCamera::init()
{
      char     tBuf[256];

      GetCurrentDirectory(256,tBuf);// Look in current working directory for driver files
      errorCode=Initialize(tBuf);  // Initialize driver in current directory
      errorMsg="Camera Initialization failed when: ";

      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Call Initialize()";
         return false;
      }

      // Get camera capabilities
      caps.ulSize = sizeof(AndorCapabilities);
      errorCode=GetCapabilities(&caps);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Get Andor Capabilities information";
         return false;
      }

      // Get Head Model
      char tModel[32];                // headmodel
      errorCode=GetHeadModel(tModel);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Get Head Model information";
         return false;
      }
      model=tModel;

      // Get detector information
      errorCode=GetDetector(&chipWidth,&chipHeight);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Get Detector information";
         return false;
      }

      if(hend<0) hend=chipWidth;
      if(vend<0) vend=chipHeight;

      // Set frame transfer mode:
      errorCode=SetFrameTransferMode(1);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Set Frame Transfer Mode";
         return false;
      }

      // Set read mode to required setting specified in xxxxWndw.c
      errorCode=SetReadMode(readMode);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Set Read Mode";
         return false;
      }

      // Set Shutter is made up of ttl level, shutter and open close time
      errorCode=SetShutter(1, 1, 0, 0); //ttl high to open, always open, 0ms closing time, 0 ms opening time
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="open shutter";
         return false;
      }

      // Set AD channel to the first available
      errorCode=SetADChannel(0);
      if(errorCode!=DRV_SUCCESS){
         errorMsg+="Set AD Channel";
         return false;
      }

      // TODO: set output amplifier to be the first available


      // Wait for 2 seconds to allow MCD to calibrate fully before allowing an
      // acquisition to begin
      DWORD tickStart =GetTickCount();
      while(GetTickCount()-tickStart<2000){;}

      return true;
}


bool AndorCamera::fini()
{
      //close camera's (not Laser's) shutter:
      errorCode=SetShutter(1, 2, 0, 0); //ttl high to open, always close, 0ms closing time, 0 ms opening time
      if(errorCode!=DRV_SUCCESS){
         errorMsg="error when close shutter";
         return false;
      }

      // shut down system
      errorCode=ShutDown();
      if(errorCode!=DRV_SUCCESS){
         errorMsg="error to shut down the camera";
         return false;
      }

      return true;
}



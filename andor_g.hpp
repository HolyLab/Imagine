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

#ifndef ANDOR_G_HPP
#define ANDOR_G_HPP

#include <string>
#include <vector>
#include <iostream>
using std::string;
using std::vector;
using std::cerr;
using std::endl;

#include "atmcd32d.h"        		// Andor functions

#include "camera_g.hpp"

class AndorCamera: public Camera {
public:
   enum AndorAcqMode {eSingleScan=1, eAccumulate, eKineticSeries, eRunTillAbort=5,
         eUndocumentedFrameTransfer=6,
   };

private:
   AndorCapabilities caps;                     // AndorCapabilities structure

   //todo: move next two to local
   int  idxVirticalSpeed;                 // Vertical Speed Index
   int  idxHorizontalSpeed;                 // Horizontal Speed Index

   //AcqMode acquisitionMode;
   int     readMode;

public:
   AndorCamera(){
      //acquisitionMode=eUndocumentedFrameTransfer; //6
      readMode=4;
      triggerMode=eInternalTrigger;   //internal mode

      vendor="andor";
   }//ctor

   ~AndorCamera(){
   }

   string getErrorMsg(){
      if(errorCode==DRV_SUCCESS){
         return "no error";
      }
      else return errorMsg;
   }

   int getExtraErrorCode(ExtraErrorCodeType type) {
      switch(type){
      case eOutOfMem: return DRV_ERROR_CODES-100;
      //default: 
      }
   }

   long getAcquiredFrameCount()
   {
      long nFramesGotCur;
      GetTotalNumberImagesAcquired(&nFramesGotCur);
      return nFramesGotCur;
   }

   bool getLatestLiveImage(PixelValue * frame)
   {
      long nPixels=getImageWidth()*getImageHeight();

      //todo: check return value
      GetMostRecentImage16(frame, nPixels);

      return true;
   }

   vector<float> getHorShiftSpeeds()
   {
      vector<float> horSpeeds;

      int nSpeeds;
      float speed;
      GetNumberHSSpeeds(0,0,&nSpeeds); //todo: error checking. func can be: bool getHorShiftSpeeds(vector<float> &)
      for(int i=0; i<nSpeeds; i++){
         GetHSSpeed(0, 0, i, &speed); //AD channel=0; type=0, i.e. electron multiplication.
         horSpeeds.push_back(speed);
      }

      return horSpeeds;
   }//getHorShiftSpeeds()

   vector<float> getVerShiftSpeeds()
   {
      vector<float> verSpeeds;

      int nSpeeds;
      float speed;
      GetNumberVSSpeeds(&nSpeeds);   //todo: error checking
      for(int i=0; i<nSpeeds; i++){
         GetVSSpeed(i, &speed);
         verSpeeds.push_back(speed);
      }

      return verSpeeds;
   }//getVerShiftSpeeds()

   vector<float> getPreAmpGains()
   {
      vector<float> preAmpGains;

      int nPreAmpGains=0;
      float gain;
      GetNumberPreAmpGains(&nPreAmpGains);
      for(int i=0; i<nPreAmpGains; ++i){
         GetPreAmpGain(i, &gain);
         preAmpGains.push_back(gain);
      }

      return preAmpGains;
   }//getPreAmpGains(),


   //return true if success
   bool init();

   //switch on or off cooler
   bool switchCooler(bool toON){
      // cooler:
      errorCode=toON?CoolerON():CoolerOFF();        // Switch on or off cooler
      if(errorCode!=DRV_SUCCESS){
         errorMsg=string("Error to switch ")+(toON?"on":"off")+" cooler";
         return false;
      }

      return true;
   }//switchCooler(),

   //set heat sink fan speed
   enum FanSpeed {fsHigh=0, fsLow, fsOff};
   bool setHeatsinkFanSpeed(FanSpeed speed){
      errorCode=SetFanMode(speed);
      if(errorCode!=DRV_SUCCESS){
         errorMsg=string("Error when change heatsink fan speed");
         return false;
      }

      return true;
   }//setHeatsinkFanSpeed()

   //shut down the camera
   bool fini();


   //------------------------------------------------------------------------------
   //  RETURNS: 	true: Image data acquired successfully
   //  DESCRIPTION:    This function gets the acquired data from the card and
   //		stores it in the global buffer pImageArray. 
   // the data layout: e.g.
   //     // get the requested spooled image
   //     start = (scanNo-1)*imageWidth*imageHeight;  // scanNo is 1,2,3...
   //     for(i=0;i<(imageWidth*imageHeight);i++){
   //        MaxValue=pImageArray[i+start];
   //     }
   //note: before call this func, make sure camera is idle. Otherwise
   //   this func will be ganranteed return false.
   bool getData(void)   {
      int nPixels=getImageWidth()*getImageHeight()*nFrames;

      if(!allocImageArray(nFrames,false)){
         return false;
      }//if, fail to alloc enough mem

      /*
      //here check if camera is idle, i.e. data is ready
      int status;
      GetStatus(&status);  //todo: check return value
      while(status!=DRV_IDLE){
         GetStatus(&status);  //todo: check return value
      }
      */

      errorCode=GetAcquiredData16(pImageArray, nPixels);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="get acquisition data error";
         return false;
      }

      return true;
   }//getData(),

   //abort acquisition
   bool abortAcq(){
      int status;
      // abort acquisition if in progress
      GetStatus(&status);
      if(status==DRV_ACQUIRING){
         errorCode=AbortAcquisition();
         if(errorCode!=DRV_SUCCESS){
            errorMsg="Error aborting acquistion";
            return false;
         }
      }
      else{
         //todo: should set some errorCode, like eAbortNonAcq?
      }//else, let user know none is in progress

      return true;
   }//abortAcq(),


   bool setAcqModeAndTime(GenericAcqMode genericAcqMode,
                          float exposure,
                          int anFrames,  //used only in kinetic-series mode
                          TriggerMode triggerMode
                          );

   bool setAcqParams(int emGain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) ;

   //start acquisition. Return true if successful
   bool startAcq(){
      //todo: is this necessary?
      int status;
      GetStatus(&status);  //todo: check return value
      if(status!=DRV_IDLE){
         errorMsg="start acquisition error: camera is not idle yet";
         return false;
      }

      errorCode=StartAcquisition();
      //if(trig==1) strcat(tBuf,"Waiting for external trigger\r\n");
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Start acquisition error";
         AbortAcquisition();
         return false;
      }

      return true;
   }//startAcq(),


   bool stopAcq()
   {
      if(genericAcqMode==eLive){
         //todo: check return value
         AbortAcquisition();
      }
      //else  do  nothing

      return true;
   }


   double getCycleTime()
   {
   float tExp, tAccumTime, tKineticTime;
   //todo: check return value
   GetAcquisitionTimings(&tExp,&tAccumTime,&tKineticTime);

   return tKineticTime;
   }

};//class, AndorCamera

//extern AndorCamera camera;


#endif //ANDOR_G_HPP


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
using std::string;
using std::vector;

#include "atmcd32d.h"        		// Andor functions

#include "camera_g.hpp"

class AndorCamera: public Camera {
public:
   enum AcqMode {eSingleScan=1, eAccumulate, eKineticSeries, eRunTillAbort=5,
         eUndocumentedFrameTransfer=6,
   };
   enum TriggerMode {eInternalTrigger=0, 
      eExternalStart=6,
   };

private:
   AndorCapabilities caps;                     // AndorCapabilities structure

   //todo: move next two to local
   int  idxVirticalSpeed;                 // Vertical Speed Index
   int  idxHorizontalSpeed;                 // Horizontal Speed Index

   //AcqMode acquisitionMode;
   int     readMode;
   int     triggerMode;

public:
   AndorCamera(){
      //acquisitionMode=eUndocumentedFrameTransfer; //6
      readMode=4;
      triggerMode=eInternalTrigger;   //internal mode

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
   bool init(){
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
   }//init(),

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
   bool fini(){
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
   }//fini(),


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

   //TODO: change this func's name 
   //set up trigger mode, acq. mode and time
   bool setAcqModeAndTime(AcqMode acqMode,
                          float exposure,
                          int anFrames,  //used only in kinetic-series mode
                          TriggerMode triggerMode
                          )
   {
      this->nFrames=anFrames;
      this->triggerMode=triggerMode;

      //set trigger mode:
      errorCode=SetTriggerMode(triggerMode);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set Trigger Mode Error";
         return false;
      }

      // Set acquisition mode:
      errorCode=SetAcquisitionMode(acqMode);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set Acquisition Mode error";
         return false;
      }

      //TODO: after change acqMode from runTillAbort to Kinetic, 
      //   you must call SetNumberKinetics(nFrames);
      //   but I don't know whether I should call SetExposureTime() also.
      //   To be safe, here I also call SetExposureTime().

      // Set Exposure Time
      errorCode = SetExposureTime(exposure);
      if (errorCode != DRV_SUCCESS) {
         errorMsg="set exposure time error";
         return false;
      }

      // set # of kinetic scans(i.e. frames) to be taken
//      if(acqMode==eKineticSeries){
      if(acqMode==eKineticSeries || acqMode==eUndocumentedFrameTransfer){
         errorCode=SetNumberKinetics(nFrames);
         if(errorCode!=DRV_SUCCESS) {
            errorMsg="Set number Frames error";
            return false;
         }
      }//if, in kinetic-series mode

      // It is necessary to get the actual times as the system will calculate the
      // nearest possible time. eg if you set exposure time to be 0, the system
      // will use the closest value (around 0.01s)
      //    float fAccumTime,fKineticTime;
      //    GetAcquisitionTimings(&exposure,&fAccumTime,&fKineticTime);

      return true;
   }//setAcqModeAndTime(),

   // This function sets up the acquisition settings exposure time
   //		shutter, trigger and starts an acquisition. It also starts a
   //	timer to check when the acquisition has finished.
   //   exposure: time in sec
   //TODO: may need set up trigger mode as well so that it is verified that the trigger mode is valid
   bool setAcqParams(int emGain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) 
   {
      //todo: is this necessary?
      int status;
      GetStatus(&status);  //todo: check return value
      if(status!=DRV_IDLE){
         errorMsg="set camera parameter error: camera is not idle now";
         return false;
      }

      //set horizontal shift speed, i.e. readout rate
      errorCode=SetHSSpeed(0, horShiftSpeedIdx); 
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set Horizontal Speed error";
         return false;
      }

      //set preamp gain:
      errorCode =  SetPreAmpGain(preAmpGainIdx);
      if(errorCode != DRV_SUCCESS) {
         errorMsg="set pre-amp gain error";
         return false;
      }

      //set vertical shift speed
      errorCode=SetVSSpeed(verShiftSpeedIdx);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set Vertical Speed error";
         return false;
      }

      //set vert. clock voltage amplitude:
      errorCode=SetVSAmplitude(verClockVolAmp);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set Vertical clock voltage amplitude error";
         return false;
      }

      //set em gain
      errorCode =  SetEMCCDGain(emGain);
      if(errorCode != DRV_SUCCESS) {
         errorMsg="set EM gain error";
         return false;
      }
      Sleep(250);  // a delay is required for gain level to change

      // This function only needs to be called when acquiring an image. It sets
      // the horizontal and vertical binning and the area of the image to be
      // captured. In this example it is set to 1x1 binning and is acquiring the
      // whole image
      SetImage(hbin, vbin, hstart, hend, vstart, vend);

      //turn on/off SDK's baseline clamp:
      errorCode=SetBaselineClamp(isBaselineClamp);
      if(errorCode!=DRV_SUCCESS){
         errorMsg="Set baseline clamp error";
         return false;
      }

      return true;
   }//setAcqParams(),

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


};//class, AndorCamera

extern AndorCamera camera;


#endif //ANDOR_G_HPP


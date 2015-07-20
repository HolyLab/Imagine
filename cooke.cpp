#include "cooke.hpp"
#include "cookeworkerthread.h"

#include <assert.h>

#include "SC2_SDKAddendum.h"

#include "pco_errt.h"
#include <iostream>
#include <QtDebug>

#define PCO_ERRT_H_CREATE_OBJECT

int const COOKE_EXCEPTION = 47;

bool CookeCamera::init()
{
   errorMsg="Camera Initialization failed when: ";

   errorCode=PCO_OpenCamera(&hCamera, 0);

   if(errorCode!=PCO_NOERROR){
      char msg[16384];
      PCO_GetErrorText(errorCode, msg, 16384);
      cout << msg << endl;
      errorMsg+="call PCO_OpenCamera";
      return false;
   }

   errorCode = PCO_GetGeneral(hCamera, &strGeneral);// Get infos from camera
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetGeneral";
      return false;
   }

   errorCode = PCO_GetCameraType(hCamera, &strCamType);
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetCameraType";
      return false;
   }

   errorCode = PCO_GetSensorStruct(hCamera, &strSensor);
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetSensorStruct";
      return false;
   }

   errorCode = PCO_GetCameraDescription(hCamera, &strDescription);
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetCameraDescription";
      return false;
   }

   errorCode = PCO_GetTimingStruct(hCamera, &strTiming);
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetTimingStruct";
      return false;
   }

   errorCode = PCO_GetRecordingStruct(hCamera, &strRecording);
   if (errorCode != PCO_NOERROR)
   {
      errorMsg+="PCO_GetRecordingStruct";
      return false;
   }

   //sensor width/height
   this->chipHeight=strSensor.strDescription.wMaxVertResStdDESC;
   this->chipWidth=strSensor.strDescription.wMaxHorzResStdDESC;

   //model
   this->model=strCamType.strHardwareVersion.Board[0].szName;

   int nPixels=chipWidth*chipHeight;
   
   //pLiveImage=new PixelValue[nPixels];
   pLiveImage=(PixelValue*)_aligned_malloc(sizeof(PixelValue)*nPixels, 4*1024);

   //pBlackImage=new PixelValue[nPixels];
   pBlackImage=(PixelValue*)_aligned_malloc(sizeof(PixelValue)*nPixels, 4*1024);
   memset(pBlackImage, 0, nPixels*sizeof(PixelValue)); //all zeros


   return true;
}//init(),

bool CookeCamera::fini()
{
   PCO_CloseCamera(hCamera);

   return true;
}//fini(),

bool CookeCamera::setAcqParams(int emGain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) 
{
   // if you need to ask the camera about available settings, this is the place to do it.

   // failure of a config call will raise an exception so we can try/catch for cleanliness
   try {
	   // stop recording - necessary for changing camera settings
	   safe_pco(PCO_SetRecordingState(hCamera, 0), "failed to stop camera");
	   // start with default settings
	   safe_pco(PCO_ResetSettingsToDefault(hCamera), "failed to reset camera settings");
	   // skip SetBinning... not applicable to cooke cameras
	   // set roi
	   safe_pco(PCO_SetROI(hCamera, hstart, vstart, hend, vend), "failed to set ROI");
	   // frame trigger mode (0 for auto)
	   safe_pco(PCO_SetTriggerMode(hCamera, 0), "failed to set frame trigger mode");
	   // storage mode (1 for FIFO)
	   safe_pco(PCO_SetStorageMode(hCamera, 1), "failed to set storage mode");
	   // acquisition mode (0 for auto)
	   safe_pco(PCO_SetAcquireMode(hCamera, 0), "failed to set acquisition mode");
	   // timestamp mode (1: bcd; 2: bcd+ascii)
	   safe_pco(PCO_SetTimestampMode(hCamera, 1), "failed to set timestamp mode");
	   // bit alignment (0: MSB aligned; 1: LSB aligned)
	   safe_pco(PCO_SetBitAlignment(hCamera, 1), "failed to set bit alignment mode");
	  
	   // pixel rate (note that the viable settings might change between models)
	   //DWORD dwPixelRate=286000000;
	   DWORD dwPixelRate = 272250000;
	   //DWORD dwPixelRate = 95333333;
	   safe_pco(PCO_SetPixelRate(hCamera, dwPixelRate), "failed to call PCO_SetPixelRate()");

       // arm the camera to apply the image size settings... needed for the config calls that follow
       safe_pco(PCO_ArmCamera(hCamera), "failed to arm the camera");

       // get set roi size, make sure it matches what we meant to set
       WORD wXResAct = -1, wYResAct = -1, wXResMax, wYResMax;
       safe_pco(PCO_GetSizes(hCamera, &wXResAct, &wYResAct, &wXResMax, &wYResMax), "failed to get roi size");
       assert(wXResAct == hend - hstart + 1 && wYResAct == vend - vstart + 1);

       // interface output format (interface 2 for DVI)
       // note that the wFormat passed here is copied from the code before I arrived... seems weird.
       safe_pco(PCO_SetInterfaceOutputFormat(hCamera, 2, SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER, 0, 0),
           "failed to call PCO_SetInterfaceOutputFormat()");

       // auto-set the transfer params
       PCO_SetTransferParametersAuto(hCamera, NULL, 0);


       // TODO: explicitly set the transfer params here


       // TODO: explicitly set the active lookup table here


       // arm the camera to validate the settings
       safe_pco(PCO_ArmCamera(hCamera), "failed to arm the camera");

       // set the image buffer parameters
       safe_pco(PCO_CamLinkSetImageParameters(hCamera, wXResAct, wYResAct),
           "failed to call PCO_CamLinkSetImageParameters()");
   }
   catch (int e) {
	   if (e != COOKE_EXCEPTION) throw;
	   return false;
   }

   /*
   So if you find yourself having problems with PCO_CamLinkSetImageParameters(), it
   might be because of a mismatch between your camera settings (specifically the pixel
   rate and ROI dimensions, see doco FMI), the image parameters set in
   PCO_SetTransferParameter(), and the active lookup table set in PCO_SetActiveLookupTable().
   One approach to debugging is to comment out the explicit transfer params and lookup calls
   and uncomment PCO_SetTransferParametersAuto, see what settings were applied, then plug
   those value back into the explicit methods (if they suit your needs). We don't just
   leave auto on because "auto" isn't necessarily garunteed to do the same thing.
   
   For reference, here are the values that auto-set chose last time we checked:
   clparams:
        baudrate: 38400
        ClockFrequency: 85000000
        CCline: 0
        DataFormat: 261
        Transmit: 1

    Lookuptable: 0 (disabled)
    Lookuptable params: 0 (disabled)
   */
   

   //clparams.DataFormat=PCO_CL_DATAFORMAT_5x12L | wFormat; // 12L requires lookup table 0x1612
   ////clparams.ClockFrequency = 85000000; // see SC2_SDKAddendum.h for valid values - NOT same as pixel freq.
   ////clparams.Transmit = 0; // single image transmission mode
   //errorCode=PCO_SetTransferParameter(hCamera, &clparams,sizeof(PCO_SC2_CL_TRANSFER_PARAM)); 
   //if(errorCode!=PCO_NOERROR) {
   //   errorMsg="failed to call PCO_SetTransferParameter()";
   //   return false;
   //}



   // set the active lookup table 
   //WORD wParameter=0;
   //WORD wIdentifier=0x1612; // 0x1612 for high speed, with x resolution > 1920
   //errorCode =PCO_SetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
   //if(errorCode!=PCO_NOERROR) {
   //   errorMsg="failed to call PCO_SetActiveLookupTable()";
   //   return false;
   //}

   

   ////////////////////////////////////////
   // EVERYTHING BELOW THIS POINT IS FOR DEBUGGING ONLY... COMMENT OUT FOR RELEASE BUILD.
   ////////////////////////////////////////


   // take a look at the transfer parameters that got set...
   PCO_SC2_CL_TRANSFER_PARAM clparams;
   PCO_GetTransferParameter(hCamera, &clparams, sizeof(PCO_SC2_CL_TRANSFER_PARAM));

   // take a look at the lookup table that got activated
   WORD wIdentifier=-1, wParameter=-1;
   PCO_GetActiveLookupTable(hCamera, &wIdentifier, &wParameter);

   // take a look at the available lookup tables
   WORD wFormat2;
   errorCode = PCO_GetDoubleImageMode(hCamera, &wFormat2);
   char Description[256]; BYTE bInputWidth, bOutputWidth;
   WORD wNumberOfLuts, wDescLen;
   PCO_GetLookupTableInfo(hCamera, 0, &wNumberOfLuts, 0, 0, 0, 0, 0, 0);
   for (int i = 0; i < (int)wNumberOfLuts; ++i){
       PCO_GetLookupTableInfo(hCamera, i, &wNumberOfLuts, Description,
           256, &wIdentifier, &bInputWidth, &bOutputWidth, &wFormat2);
       qDebug() << QString("\nlut: %1\n\tdesc: %2\n\tid: %3").arg(i).arg(Description).arg(wIdentifier);
   }


   return true;
}//setAcqParams(),

double CookeCamera::getCycleTime()
{
   DWORD sec, nsec;
   errorCode=PCO_GetCOCRuntime(hCamera, &sec, &nsec);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetCOCRuntime()";
      return -1;
   }

   return sec+nsec*1e-9;
}


//params different for live from for save mode
bool CookeCamera::setAcqModeAndTime(GenericAcqMode genericAcqMode,
                          float exposure,
                          int anFrames,  //used only in kinetic-series mode
                          TriggerMode triggerMode
                          )
{
   this->genericAcqMode=genericAcqMode;

   this->nFrames=anFrames;
   this->triggerMode=triggerMode;

   errorCode=PCO_NOERROR;

   ///set acqusition trigger mode
   if(triggerMode==eInternalTrigger){
      errorCode=PCO_SetAcquireMode(hCamera, 0); //0: auto
   }
   else {
      errorCode=PCO_SetAcquireMode(hCamera, 1); //1: external

   }
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set acquisition trigger mode";
      return false;
   }

   ///exposure time
   errorCode = PCO_SetDelayExposureTime(hCamera, // Timebase: 0-ns; 1-us; 2-ms  
      (DWORD)(exposure*10000),		// DWORD dwDelay. Make long enough that TTL pulse is recorded (here we use 1%)
      (DWORD)(exposure*990000),
      1,		// WORD wTimeBaseDelay,
      1);	// WORD wTimeBaseExposure
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set exposure time";
      return false;
   }

   ///arm the camera again
   errorCode = PCO_ArmCamera(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to arm the camera";
      return false;
   }

   cout<<"frame rate is: "<<1/getCycleTime()<<endl;

   ///take care of image array saving (in mem only)
   if(genericAcqMode==eAcqAndSave){
      if(!isSpooling() && !allocImageArray(nFrames,false)){
         return false;
      }//if, fail to alloc enough mem
   }


   return true;
}


long CookeCamera::getAcquiredFrameCount()
{
   if(!mpLock->tryLock()) return -1;
   long result=nAcquiredFrames;
   mpLock->unlock();
   return result;
}

bool CookeCamera::startAcq()
{
   CLockGuard tGuard(mpLock);
   nAcquiredFrames=0; 

   //camera's internal frame counter can be reset by ARM which is too costly we maintain our own counter
   firstFrameCounter=-1;

   totalGap=0;

   ///alloc the ring buffer for data transfer from card to pc
   for(int i=0; i<nBufs; ++i){
      mBufIndex[i]= -1;
      mEvent[i]= NULL;
      mRingBuf[i] = NULL;

      errorCode = PCO_AllocateBuffer(hCamera, (SHORT*)&mBufIndex[i], chipWidth * chipHeight * sizeof(DWORD), &mRingBuf[i], &mEvent[i]);
      if(errorCode!=PCO_NOERROR) {
         errorMsg="failed to allocate buffer";
         return false;
      }

      //in fifo mode, frameIdxInCamRam are 0 for all buffers?
      int frameIdxInCamRam=0;
      errorCode = PCO_AddBuffer(hCamera, frameIdxInCamRam, frameIdxInCamRam, mBufIndex[i]);// Add buffer to the driver queue
      if(errorCode!=PCO_NOERROR) {
         errorMsg="failed to add buffer";
         return false;
      }
   }

   cout<<"b4 new WorkerThread(): "<<gTimer.read()<<endl;

   workerThread=new WorkerThread(this);
   cout<<"after new WorkerThread(): "<<gTimer.read()<<endl;
   workerThread->start();

   cout<<"after workerThread->start(): "<<gTimer.read()<<endl;

   errorCode = PCO_SetRecordingState(hCamera, 1); //1: run
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to start camera";
      return false;
   }

   cout<<"after set rec state to 1/run: "<<gTimer.read()<<endl;

   return true;
}//startAcq(),

bool CookeCamera::stopAcq()
{
   //not rely on the "wait abandon"!!!
   workerThread->requestStop();
   workerThread->wait();

   errorCode = PCO_SetRecordingState(hCamera, 0);// stop recording
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to stop camera";
      return false;
   }


   cout<<"b4 PCO_RemoveBuffer: "<<gTimer.read()<<endl;

   //reverse of PCO_AddBuffer()
   errorCode=PCO_RemoveBuffer(hCamera);   // If there's still a buffer in the driver queue, remove it
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to stop camera";
      return false;
   }

   cout<<"after PCO_RemoveBuffer: "<<gTimer.read()<<endl;

   for(int i=0; i<nBufs; ++i){
      //reverse of PCO_AllocateBuffer()
      //ResetEvent(mEvent[0]);
      //ResetEvent(mEvent[1]);

      //TODO: what about the events associated w/ the buffers
      PCO_FreeBuffer(hCamera, mBufIndex[i]);    // Frees the memory that was allocated for the buffer
   }

   delete workerThread;
   workerThread=nullptr;

   cout<<"total # of black frames: "<<totalGap<<endl;

   return true;
}//stopAcq(),

long CookeCamera::extractFrameCounter(PixelValue* rawData)
{
   unsigned long result=0;
   for(int i=0; i<4; ++i){
      unsigned hex=rawData[i];
      result=result*100+hex/16*10+hex%16;
   }

   return result;
}

bool CookeCamera::getLatestLiveImage(PixelValue * frame)
{
   if(!mpLock->tryLock()) return false;

   if(nAcquiredFrames<=0)  {
      mpLock->unlock();  
      return false;
   }

   int nPixels=getImageHeight()*getImageWidth();
   memcpy_g(frame, pLiveImage, nPixels*sizeof(PixelValue));

   mpLock->unlock();
   return true;
}

//return false if, say, can't open the spooling file to save
//NOTE: have to call this b4 setAcqModeAndTime() to avoid out-of-mem
bool CookeCamera::setSpooling(string filename)
{
   if(ofsSpooling){
      delete ofsSpooling; //the file is closed too
      ofsSpooling=nullptr;
   }

   Camera::setSpooling(filename);

   if(isSpooling()){
      int bufsize_in_4kb=5529600*2*8/(4*1024); //8 frames, about 80M in bytes
#ifdef _WIN64
      bufsize_in_4kb*=1;
#endif
      ofsSpooling=new FastOfstream(filename.c_str(), bufsize_in_4kb);
      return *ofsSpooling;
   }
   else return true;

}

void CookeCamera::safe_pco(int errCode, string errMsg)
{
    // set errorMsg and throw an exception if you get a not-ok error code
    if (errCode != PCO_NOERROR && (errCode & PCO_ERROR_CODE_MASK) != 0) {
    //if (errCode != PCO_NOERROR) {
        
        // it might be nice to use this to give slightly more informative debug messages
        char msg[16384];
        PCO_GetErrorText(errCode, msg, 16384);
        cout << msg << endl;
		errorMsg = errMsg;
		throw COOKE_EXCEPTION; // could throw an exception class w/informative message... but meh.
	}
}

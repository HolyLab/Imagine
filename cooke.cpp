#include "cooke.hpp"
#include "cookeworkerthread.h"

#include <assert.h>

#include "SC2_SDKAddendum.h"


bool CookeCamera::init()
{
   errorMsg="Camera Initialization failed when: ";

   errorCode=PCO_OpenCamera(&hCamera, 0);

   if(errorCode!=PCO_NOERROR){
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

   //todo: alignment
   int nPixels=chipWidth*chipHeight;
   
   pLiveImage=new PixelValue[nPixels];

   pBlackImage=new PixelValue[nPixels];
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
   //TODO: the gain settings (SetConversionFactor())


   //NOTE: the camera should be in idle state
   errorCode = PCO_SetRecordingState(hCamera, 0);// stop recording
   if (errorCode & PCO_WARNING_FIRMWARE_FUNC_ALREADY_OFF) errorCode = PCO_NOERROR;

   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to stop camera";
      return false;
   }

   // /*
   errorCode=PCO_ResetSettingsToDefault(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to reset camera settings";
      return false;
   }
   // */

   //for this cooke camera no binning is possible
   //so skip SetBinning

   //set roi
   errorCode = PCO_SetROI(hCamera, hstart, vstart, hend, vend);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set ROI";
      return false;
   }

   //frame trigger mode
   errorCode = PCO_SetTriggerMode(hCamera, 0);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set frame trigger mode";
      return false;
   }

   // /*
   errorCode=PCO_SetStorageMode(hCamera, 1); //1: fifo mode
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set storage mode";
      return false;
   }
   // */

   errorCode=PCO_SetAcquireMode(hCamera, 0); //0: auto
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set acquisition mode";
      return false;
   }

   errorCode=PCO_SetTimestampMode(hCamera, 2); //2: bcd+ascii
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set timestamp mode";
      return false;
   }

   errorCode=PCO_SetBitAlignment(hCamera, 1);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set bit alignment mode";
      return false;
   }

   DWORD dwPixelRate=286000000;
   errorCode=PCO_SetPixelRate(hCamera, dwPixelRate); 
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_SetPixelRate()";
      return false;
   }

   errorCode=PCO_GetPixelRate(hCamera, &dwPixelRate);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetPixelRate()";
      return false;
   }

   errorCode = PCO_ArmCamera(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to arm the camera";
      return false;
   }

   //todo: should we set Ram Segment settings. SEE: the api's "storage ctrl" section?
   // should we clear the active segment?

   PCO_SC2_CL_TRANSFER_PARAM clparams;

   errorCode = PCO_GetTransferParameter(hCamera, &clparams, sizeof(PCO_SC2_CL_TRANSFER_PARAM));
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetTransferParameter()";
      return false;
   }


   //get size
   WORD wXResAct=-1; // Actual X Resolution
   WORD wYResAct = -1; // Actual Y Resolution
   WORD wXResMax; // Maximum X Resolution
   WORD wYResMax; // Maximum Y 

   errorCode = PCO_GetSizes(hCamera, &wXResAct, &wYResAct, &wXResMax, &wYResMax);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to get roi size";
      return false;
   }
   assert(wXResAct==hend-hstart+1 && wYResAct==vend-vstart+1);

   //
   errorCode=PCO_GetPixelRate(hCamera, &dwPixelRate);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetPixelRate()";
      return false;
   }


   WORD wDestInterface=2,wFormat, wRes1, wRes2;
   errorCode=PCO_GetInterfaceOutputFormat(hCamera, &wDestInterface, &wFormat, &wRes1, &wRes2);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetInterfaceOutputFormat()";
      return false;
   }

   wRes1=wRes2=0;
   wFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
   errorCode=PCO_SetInterfaceOutputFormat(hCamera, wDestInterface, wFormat, wRes1, wRes2);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_SetInterfaceOutputFormat()";
      return false;
   }

   clparams.DataFormat=PCO_CL_DATAFORMAT_5x12L | wFormat;
   errorCode=PCO_SetTransferParameter(hCamera, &clparams,sizeof(PCO_SC2_CL_TRANSFER_PARAM)); 
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_SetTransferParameter()";
      return false;
   }

   errorCode = PCO_GetTransferParameter(hCamera, &clparams, sizeof(PCO_SC2_CL_TRANSFER_PARAM));
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetTransferParameter()";
      return false;
   }

   //PCO_SetInterfaceOutputFormat, p6, 20
   WORD wFormat2;
   errorCode=PCO_GetDoubleImageMode(hCamera, &wFormat2);

   char Description[256]; BYTE bInputWidth,bOutputWidth; 
   WORD wNumberOfLuts, wDescLen, wIdentifier;
   errorCode=PCO_GetLookupTableInfo(hCamera, 0, &wNumberOfLuts, 0, 0, 0, 0, 0, 0);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to get #LUTs";
      return false;
   }
   int nLuts=wNumberOfLuts;
   for(int i=0; i<nLuts; ++i){
      errorCode=PCO_GetLookupTableInfo(hCamera, i, &wNumberOfLuts, Description, 256, &wIdentifier, &bInputWidth, &bOutputWidth, &wFormat2);
      if(errorCode!=PCO_NOERROR) cout<<"error when get lut "<<i<<"'s info"<<endl;
      //else cout<<"lut "<<i<<": '"<<Description<<"'"<<
   }


   WORD wParameter=0;
   wIdentifier=5650; 
   errorCode =PCO_SetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_SetActiveLookupTable()";
      return false;
   }

   errorCode =PCO_GetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetActiveLookupTable()";
      return false;
   }

   //arm the camera to validate the settings
   //NOTE: you have to arm the camera again in setAcqModeAndTime() due to the trigger setting (i.e. set PCO_SetAcquireMode() again)
   errorCode = PCO_ArmCamera(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to arm the camera";
      return false;
   }

   errorCode = PCO_CamLinkSetImageParameters(hCamera, wXResAct, wYResAct);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_CamLinkSetImageParameters()";
      return false;
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
      0,		// DWORD dwDelay
      (DWORD)(exposure*1000),
      0,		// WORD wTimeBaseDelay,
      2);	// WORD wTimeBaseExposure (2: ms)
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
   CLockGuard tGuard(mpLock);
   return nAcquiredFrames;
}




bool CookeCamera::startAcq()
{
   CLockGuard tGuard(mpLock);
   nAcquiredFrames=0; //todo: maybe too late if external trigger?

   //camera's internal frame counter can be reset by ARM which is too costly we maintain our own counter
   firstFrameCounter=-1;

   //
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

   workerThread=new WorkerThread(this);
   workerThread->start();

   errorCode = PCO_SetRecordingState(hCamera, 1); //1: run
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to start camera";
      return false;
   }


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

   //reverse of PCO_AddBuffer()
   errorCode=PCO_RemoveBuffer(hCamera);   // If there's still a buffer in the driver queue, remove it
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to stop camera";
      return false;
   }

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
   CLockGuard tGuard(mpLock);

   if(nAcquiredFrames<=0)   return false;

   int nPixels=getImageHeight()*getImageWidth();
   memcpy(frame, pLiveImage, nPixels*sizeof(PixelValue));

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
      int bufsize_in_kb=65536; //64M in bytes
#ifdef _WIN64
      bufsize_in_kb*=3;
#endif
      ofsSpooling=new FastOfstream(filename.c_str(), bufsize_in_kb);
      return *ofsSpooling;
   }
   else return true;

}

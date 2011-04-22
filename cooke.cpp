#include "cooke.hpp"

#include <assert.h>
#include "lockguard.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <iostream>
using std::cout;
using std::endl;

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

   errorCode=PCO_SetStorageMode(hCamera, 1); //1: fifo mode
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to set storage mode";
      return false;
   }

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

   //todo: should we set Ram Segment settings. SEE: the api's "storage ctrl" section?
   // should we clear the active segment?

   //arm the camera to validate the settings
   //NOTE: you have to arm the camera again in setAcqModeAndTime() due to the trigger setting (i.e. set PCO_SetAcquireMode() again)
   errorCode = PCO_ArmCamera(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to arm the camera";
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

   return sec+nsec/1e-9;
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
      if(!allocImageArray(nFrames,false)){
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


class CookeCamera::WorkerThread: public QThread {
   Q_OBJECT
private:
   CookeCamera* camera;
   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   WorkerThread(CookeCamera * camera, QObject *parent = 0)
      : QThread(parent){
      this->camera=camera;
      shouldStop=false;
   }
   ~WorkerThread(){}

   void requestStop(){
      shouldStop=true;
   }

   void run(){
      while(true){
         if(shouldStop) break;
         ///wait for events
         int waitResult=WaitForMultipleObjects(2, camera->mEvent, false, 500);
         switch(waitResult){
            case WAIT_OBJECT_0 + 0:
            case WAIT_OBJECT_0 + 1:
               break; //break the switch
            default: //WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
               //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
               shouldStop=true; //or goto
         }//switch,
         if(shouldStop)break; //break the while
         int eventIdx=waitResult-WAIT_OBJECT_0;
         //todo: should we call GetBufferStatus() to double-check the status about transferring? SEE: demo.cpp
         
         CLockGuard tGuard(camera->mpLock);

         PixelValue* rawData=camera->mRingBuf[eventIdx];
         long counter=camera->extractFrameCounter(rawData);
         if(camera->nAcquiredFrames==0) {
            camera->firstFrameCounter=counter;
            cout<<"first frame's counter is "<<counter<<endl;
         }
         int curFrameIdx=counter-camera->firstFrameCounter;
         long nPixelsPerFrame=camera->getImageHeight()*camera->getImageWidth();

         //fill the gap w/ black images
         int gapWidth=0;
         for(int frameIdx=camera->nAcquiredFrames; frameIdx<min(camera->nFrames, curFrameIdx); ++frameIdx){
            memcpy(camera->pImageArray+frameIdx*nPixelsPerFrame, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            gapWidth++;
         }
         if(gapWidth) cout<<"fill "<<gapWidth<<" frames with black images (start at frame idx="<<camera->nAcquiredFrames<<")"<<endl;

         //fill the frame array and live image
         if(curFrameIdx<camera->nFrames){
            memcpy(camera->pImageArray+curFrameIdx*nPixelsPerFrame, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            memcpy(camera->pLiveImage, rawData, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=curFrameIdx+1;
         }
         else {
            memcpy(camera->pLiveImage, camera->pBlackImage, sizeof(Camera::PixelValue)*nPixelsPerFrame);
            camera->nAcquiredFrames=camera->nFrames;
         }

         //reset event then add back the bufffer
         ResetEvent(camera->mEvent[eventIdx]);
         if(camera->nAcquiredFrames<camera->nFrames){
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
   }//run(),
};//class, CookeCamera::WorkerThread


bool CookeCamera::startAcq()
{
   CLockGuard tGuard(mpLock);
   nAcquiredFrames=0; //todo: maybe too late if external trigger?

   //camera's internal frame counter can be reset by ARM which is too costly we maintain our own counter
   firstFrameCounter=-1;

   ///alloc the ring buffer for data transfer from card to pc
   mBufIndex[0]=mBufIndex[1] = -1;
   mEvent[0]= mEvent[1]= NULL;
   mRingBuf[0]=mRingBuf[1] = NULL;

   errorCode = PCO_AllocateBuffer(hCamera, (SHORT*)&mBufIndex[0], chipWidth * chipHeight * sizeof(DWORD), &mRingBuf[0], &mEvent[0]);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to allocate buffer";
      return false;
   }

   errorCode = PCO_AllocateBuffer(hCamera, (SHORT*)&mBufIndex[1], chipWidth * chipHeight * sizeof(DWORD), &mRingBuf[1], &mEvent[1]);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to allocate buffer";
      return false;
   }

   workerThread=new WorkerThread(this);

   errorCode = PCO_SetRecordingState(hCamera, 1); //1: run
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to start camera";
      return false;
   }

   //in fifo mode, frameIdxInCamRam are 0 for both buffers?
   int frameIdxInCamRam=0;
   errorCode = PCO_AddBuffer(hCamera, frameIdxInCamRam, frameIdxInCamRam, mBufIndex[0]);// Add buffer to the driver queue
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to add buffer";
      return false;
   }

   errorCode = PCO_AddBuffer(hCamera, frameIdxInCamRam, frameIdxInCamRam, mBufIndex[1]);// Add another buffer to the driver queue
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to add buffer";
      return false;
   }

   workerThread->start();

   return true;
}//startAcq(),


bool CookeCamera::stopAcq()
{
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

   workerThread->requestStop();
   workerThread->wait();

   //reverse of PCO_AllocateBuffer()
   //TODO: what about the events associated w/ the 2 buffers
   PCO_FreeBuffer(hCamera, mBufIndex[0]);    // Frees the memory that was allocated for the buffer
   PCO_FreeBuffer(hCamera, mBufIndex[1]);    // Frees the memory that was allocated for the buffer

   delete workerThread;
   workerThread=nullptr;


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


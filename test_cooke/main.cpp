//test cooke camera

#include <windows.h>
#include <assert.h>

#include <iostream>
#include <string>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
#include "PCO_err.h"
#include "SC2_SDKAddendum.h"

//class Camera:
   int	errorCode;    //these two are paired. For andor's, errorCode meanings are defined by 
   string errorMsg;   //   atmcd32d.h. 
   bool fake_camera = true;

//class CookeCamera:
HANDLE hCamera;
PCO_General strGeneral;
PCO_CameraType strCamType;
PCO_Sensor strSensor;
PCO_Description strDescription;
PCO_Timing strTiming;
PCO_Storage strStorage;
PCO_Recording strRecording;
PCO_Image strImage;

typedef unsigned short PixelValue;

WORD mBufIndex[2]; 
HANDLE mEvent[2];
PixelValue* mRingBuf[2]; 
long firstFrameCounter; //first frame's counter value

//CookeCamera::getCycleTime():
double getCycleTime()
{
   DWORD sec, nsec;
   errorCode=PCO_GetCOCRuntime(hCamera, &sec, &nsec);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetCOCRuntime()";
      return -1;
   }

   return sec+nsec*1e-9;
}

//CookeCamera::extractFrameCounter():
long extractFrameCounter(PixelValue* rawData)
{
   unsigned long result=0;
   for(int i=0; i<4; ++i){
      unsigned hex=rawData[i];
      result=result*100+hex/16*10+hex%16;
   }

   return result;
}

//
bool test(int nFrames, double exposure, int nRounds, void* buf)
{
   cout<<"#frames is "<<nFrames<<", exposure is "<<exposure<<", #rounds is "<<nRounds<<endl;

   ///ctor:
   strGeneral.wSize = sizeof(strGeneral);// initialize all structure size members
   strGeneral.strCamType.wSize = sizeof(strGeneral.strCamType);
   strCamType.wSize = sizeof(strCamType);
   strSensor.wSize = sizeof(strSensor);
   strSensor.strDescription.wSize = sizeof(strSensor.strDescription);
   strDescription.wSize = sizeof(strDescription);
   strTiming.wSize = sizeof(strTiming);
   strStorage.wSize = sizeof(strStorage);
   strRecording.wSize = sizeof(strRecording);
   strImage.wSize = sizeof(strImage);
   strImage.strSegment[0].wSize = sizeof(strImage.strSegment[0]);
   strImage.strSegment[1].wSize = sizeof(strImage.strSegment[0]);
   strImage.strSegment[2].wSize = sizeof(strImage.strSegment[0]);
   strImage.strSegment[3].wSize = sizeof(strImage.strSegment[0]);

   ///init():
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
   int chipHeight=strSensor.strDescription.wMaxVertResStdDESC;
   int chipWidth=strSensor.strDescription.wMaxHorzResStdDESC;

   errorMsg="";

   ///setAcqParams():
   errorCode = PCO_SetRecordingState(hCamera, 0);// stop recording
   if (errorCode & PCO_WARNING_FIRMWARE_FUNC_ALREADY_OFF) errorCode = PCO_NOERROR;
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to stop camera";
      return false;
   }

   errorCode=PCO_ResetSettingsToDefault(hCamera);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to reset camera settings";
      return false;
   }

   //set roi
   errorCode = PCO_SetROI(hCamera, 1, 1, 2560, 2160);
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

   clparams.DataFormat=PCO_CL_DATAFORMAT_5x12L | SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
   clparams.baudrate=115200;
   clparams.Transmit=1;
   errorCode=PCO_SetTransferParameter(hCamera, &clparams,sizeof(PCO_SC2_CL_TRANSFER_PARAM)); 
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetTransferParameter()";
      return false;
   }

   errorCode = PCO_GetTransferParameter(hCamera, &clparams, sizeof(PCO_SC2_CL_TRANSFER_PARAM));
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_GetTransferParameter()";
      return false;
   }

   WORD wParameter=0;
   WORD wIdentifier=5650; //sqrt(256*x)v(x>256)
   errorCode =PCO_SetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to call PCO_SetActiveLookupTable()";
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
   ///END: setAcqParams(),

   int round=1;
nextRound:
   cout<<"start a new round = "<<round++<<endl;
   ///setAcqModeAndTime():

   ///set acqusition trigger mode
   errorCode=PCO_SetAcquireMode(hCamera, 0); //0: auto
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

   ///startAcq():
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


   errorCode = PCO_SetRecordingState(hCamera, 1); //1: run
   if(errorCode!=PCO_NOERROR) {
      errorMsg="failed to start camera";
      return false;
   }

   //TODO: in fifo mode, frameIdxInCamRam are 0 for both buffers?
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
   ///END startAcq(),

   ///CookeCamera::WorkerThread::run():
   int nAcquiredFrames=0; //#frames so far
   long totalGap=0;

   bool shouldStop=false;
   while(true){
      if(shouldStop) break;
      ///wait for events
      int waitResult=WaitForMultipleObjects(2, mEvent, false, 30);
      switch(waitResult){
      case WAIT_OBJECT_0 + 0:
      case WAIT_OBJECT_0 + 1:
         break; //break the switch
      default: //WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
         shouldStop=true; 
      }//switch,
      if(shouldStop)break; //break the while
      int eventIdx=waitResult-WAIT_OBJECT_0;
      //todo: should we call GetBufferStatus() to double-check the status about transferring? SEE: demo.cpp

      PixelValue* rawData=mRingBuf[eventIdx];
      long counter=extractFrameCounter(rawData);
      if(nAcquiredFrames==0) {
         firstFrameCounter=counter;
         cout<<"first frame's counter is "<<counter<<endl;
      }
      int curFrameIdx=counter-firstFrameCounter;

      assert(curFrameIdx>=0);
      int gapWidth=0;
      for(int frameIdx=nAcquiredFrames; frameIdx<min(nFrames, curFrameIdx); ++frameIdx){
         memcpy(buf, rawData, 2560*2160*2);
         gapWidth++;
      }
      if(gapWidth) cout<<"fill "<<gapWidth<<" frames with black images (start at frame idx="<<nAcquiredFrames<<")"<<endl;
      totalGap+=gapWidth;
      nAcquiredFrames=curFrameIdx+1;

      memcpy(buf, rawData, 2560*2160*2);

      //reset event
      ResetEvent(mEvent[eventIdx]);

      memcpy(buf, rawData, 2560*2160*2);

      ///then add back the buffer
      if(nAcquiredFrames<nFrames){
         //todo: in fifo mode, frameIdxInCamRam are 0 for both buffers?
         int frameIdxInCamRam=0;
         errorCode = PCO_AddBuffer(hCamera, frameIdxInCamRam, frameIdxInCamRam, mBufIndex[eventIdx]);// Add buffer to the driver queue
         if(errorCode!=PCO_NOERROR) {
            errorMsg="failed to add buffer";
            break; //break the while
         }
      }
      else {
         break;
      }
   }//while,


   ///stopAcq():
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

   //reverse of PCO_AllocateBuffer()
   PCO_FreeBuffer(hCamera, mBufIndex[0]);    // Frees the memory that was allocated for the buffer
   PCO_FreeBuffer(hCamera, mBufIndex[1]);    // Frees the memory that was allocated for the buffer

   cout<<"total # of black frames in this round: "<<totalGap<<endl;

   if(--nRounds) goto nextRound;

   ///fini():
   PCO_CloseCamera(hCamera);


   return true;
}//test(),

int main(int argc, char* argv[])
{
	if (!fake_camera) {
		int nFrames;
		double exposure;
		int nRounds;
		if (argc != 4) {
			cerr << "usage: " << argv[0] << " #frames exposure #rounds" << endl;
			cerr << "Now use the default: 1000 frames w/ 0.05s exposure and 3 rounds" << endl;
			exposure = 0.05; nFrames = 1000; nRounds = 3;
		}
		else {
			nFrames = atoi(argv[1]);
			exposure = atof(argv[2]);
			nRounds = atoi(argv[3]);
		}

		void* buf = _aligned_malloc(2560 * 2160 * 2, 4 * 1024);

		if (!test(nFrames, exposure, nRounds, buf)) {
			cerr << "test failed: " << errorMsg << endl;
			return 1;
		}

		cout << "test succeeded!" << endl;
	}
   //getc(stdin);

   return 0;
}

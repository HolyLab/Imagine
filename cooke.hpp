#ifndef COOKE_HPP
#define COOKE_HPP

#include <windows.h>

#include <QMutex>

#include "camera_g.hpp"

#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
//#include "sc2_defs.h"
#include "PCO_err.h"
//#define PCO_ERRT_H_CREATE_OBJECT
//#include "PCO_errt.h"

#include "fast_ofstream.hpp"


class CookeCamera: public Camera {
   class WorkerThread;
   friend class WorkerThread;

   HANDLE hCamera;
   PCO_General strGeneral;
   PCO_CameraType strCamType;
   PCO_Sensor strSensor;
   PCO_Description strDescription;
   PCO_Timing strTiming;
   PCO_Storage strStorage;
   PCO_Recording strRecording;
   PCO_Image strImage;

   //the buf for the live image
   //todo: we might also want it to be aligned
   PixelValue* pLiveImage;

   //for speed reason: a black frame used for copying
   //todo: alignment
   PixelValue* pBlackImage;

   long firstFrameCounter; //first first frame's counter value
   long long totalGap;

   long nAcquiredFrames;
   //lock used to coordinate accessing to nAcquiredFrames & pLiveImage
   QMutex* mpLock; 

   WORD mBufIndex[2]; //m_wBufferNr
   HANDLE mEvent[2];//m_hEvent
   PixelValue* mRingBuf[2]; //m_pic12

   WorkerThread* workerThread;

   FastOfstream *ofsSpooling;

public:
   CookeCamera(){
      pLiveImage=nullptr;

      pBlackImage=nullptr;

      firstFrameCounter=-1;
      nAcquiredFrames=0;
      mpLock=new QMutex;

      workerThread=nullptr;
      
      ofsSpooling=nullptr;

      vendor="cooke";

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

   }

   ~CookeCamera(){
      delete[] pLiveImage;
      delete[] pBlackImage;
      delete mpLock;
   }

   string getErrorMsg(){
      if(errorCode==PCO_NOERROR){
         return "no error";
      }
      else return errorMsg;
   }

   int getExtraErrorCode(ExtraErrorCodeType type) {
      switch(type){
      case eOutOfMem: return PCO_ERROR_NOMEMORY; //NOTE: this might be an issue
      //default: 
      }

      return -1;
   }

   bool init();
   bool fini();

   bool setAcqParams(int emGain,
                     int preAmpGainIdx,
                     int horShiftSpeedIdx,
                     int verShiftSpeedIdx,
                     int verClockVolAmp,
                     bool isBaselineClamp
                     ) ;

   //params different for live from for save mode
   bool setAcqModeAndTime(GenericAcqMode genericAcqMode,
                          float exposure,
                          int anFrames,  //used only in kinetic-series mode
                          TriggerMode triggerMode
                          );
 

   long getAcquiredFrameCount();

   bool getLatestLiveImage(PixelValue * frame);

   bool startAcq();

   bool stopAcq();

   double getCycleTime();

   //bool isIdle();

   //transfer data from card
   bool transferData()
   {
      //do nothing, since the data already in the buffer

      return true;
   }

   long extractFrameCounter(PixelValue* rawData);

   bool setSpooling(string filename);

};//class, CookeCamera


//extern CookeCamera camera;


#endif //COOKE_HPP

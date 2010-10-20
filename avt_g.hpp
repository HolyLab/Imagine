#ifndef AVT_G_HPP
#define AVT_G_HPP

#include <QMutex>


#include "camera_g.hpp"

#include "PvApi.h"

class AvtCamera: public Camera {
    unsigned long   cameraID;
    tPvHandle       cameraHandle;

    int circBufSize; //in frames
    tPvFrame *  pFrames;
    PixelValue* pCircBuf;  //the buffer for circular buffer
                     //NOTE: it's word-aligned
    PixelValue* pRealCircBuf; //the ptr from operator new;

    //the buf for the live image
    //todo: we might also want it to be aligned
    PixelValue* pLiveImage;

    long nAcquiredFrames;
    //lock used to coordinate accessing to nAcquiredFrames
    QMutex* mpLock; 


public:
   AvtCamera(){
      //
      pCircBuf=pRealCircBuf=nullptr;

      pLiveImage=nullptr;

      nAcquiredFrames=0;
      mpLock=new QMutex;
   }

   ~AvtCamera(){
      //
      delete pLiveImage;
      
      delete pRealCircBuf;
   }

   string getErrorMsg(){
      if(errorCode==ePvErrSuccess){
         return "no error";
      }
      else return errorMsg;
   }

   int getExtraErrorCode(ExtraErrorCodeType type) {
      switch(type){
      case eOutOfMem: return __ePvErr_force_32-100; //NOTE: this might be an issue
      //default: 
      }
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

};


//extern AvtCamera camera;


#endif //AVT_G_HPP

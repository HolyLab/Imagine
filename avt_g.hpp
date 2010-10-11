#ifndef AVT_G_HPP
#define AVT_G_HPP

#include "camera_g.hpp"

#include "PvApi.h"

class AvtCamera: public Camera {
    unsigned long   cameraID;
    tPvHandle       cameraHandle;

public:
   AvtCamera(){
      //
   }

   ~AvtCamera(){
      //
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

};


//extern AvtCamera camera;


#endif //AVT_G_HPP

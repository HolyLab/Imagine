#ifndef COOKE_HPP
#define COOKE_HPP

#include "camera_g.hpp"

#include "SC2_CamExport.h"
#include "sc2_SDKStructures.h"
#include "sc2_defs.h"
#include "PCO_err.h"
#define PCO_ERRT_H_CREATE_OBJECT
#include "PCO_errt.h"


class CookeCamera: public Camera {

public:
   CookeCamera(){
      vendor="cooke";
   }

   ~CookeCamera(){

   }


};//class, CookeCamera

#endif //COOKE_HPP

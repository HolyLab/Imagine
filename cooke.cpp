#include "cooke.hpp"

bool CookeCamera::init()
{
   errorMsg="Camera Initialization failed when: ";

   errorCode=PCO_OpenCamera(&hCamera, 0);

   if(errorCode!=PCO_NOERROR){
      errorMsg+="call PCO_OpenCamera";
      return false;
   }


}//init(),
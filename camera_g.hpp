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

#ifndef CAMERA_G_HPP
#define CAMERA_G_HPP

#include <string>
#include <vector>
using std::string;
using std::vector;


class Camera {
public:
   typedef unsigned short PixelValue;
   enum ExtraErrorCodeType {
      eOutOfMem,
   };
   int  hbin, vbin,  hstart,  hend,  vstart,  vend; //image binning params. 1-based.
                                //for hend and vend: <0 means chip width (or height)

protected:
   string model;

   //camera specific error code and msg
   int	errorCode;    //these two are paired. For andor's, errorCode meanings are defined by 
   string errorMsg;   //   atmcd32d.h. 

   int  nFrames;

   int 	chipWidth;       				// dims of
   int	chipHeight;       				// CCD chip

   // image Buffers
   PixelValue * pImageArray;	 // main image buffer read from card
   int imageArraySize; //in pixel

public:
   Camera(){
      hbin=vbin=1;
      hstart=vstart=1;
      hend=vend= -1; //

      pImageArray=NULL;
      imageArraySize=0;

      nFrames=100;  //todo: make this as a param

      model="Unknown";

   }
   virtual ~Camera(){
      // free all allocated memory
      if(pImageArray){
         delete []pImageArray;
         pImageArray = NULL;
      }
   }

   virtual string getErrorMsg()=0;

   int getErrorCode(){
      return errorCode;
   }

   virtual long getAcquiredFrameCount()=0; // #frames acquired so far

   PixelValue * getImageArray(){
      return pImageArray;
   }

   //todo: to verify
   int getImageWidth(){
      return (hend-hstart+1)/hbin;
   }

   //todo: to verify
   int getImageHeight(){
      return (vend-vstart+1)/vbin;
   }

   string getModel()
   {
      return model;
   }//getModel()

   virtual bool init()=0;

   virtual bool fini()=0;

   virtual int getExtraErrorCode(ExtraErrorCodeType type)=0;

   //allocate the image array space
   bool allocImageArray(int nFrames, bool shouldReallocAnyway){
      int nPixels=getImageWidth()*getImageHeight()*nFrames;

      if(shouldReallocAnyway || nPixels>imageArraySize){
         if(pImageArray){
            delete []pImageArray;
            pImageArray = NULL;
            imageArraySize=0;
         }
      }//if, should delete old allocation

      //here if pImageArray!=NULL, 
      //     then !shouldReallocAnyway && nPixels<=imageArraySize.

      if(!pImageArray){
         pImageArray=new PixelValue[nPixels];
         if(!pImageArray){
            errorCode=getExtraErrorCode(eOutOfMem);
            errorMsg="no enough memory";
            return false;
         }//if, fail to alloc enough mem
         imageArraySize=nPixels;
      }//if, no allocated space before

      return true;
   }//allocImageArray(),


};//class, Camera




#endif //CAMERA_G_HPP

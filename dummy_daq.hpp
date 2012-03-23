/*-------------------------------------------------------------------------
** Copyright (C) 2005-2008 Timothy E. Holy and Zhongsheng Guo
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

#ifndef DUMMY_DAQ_HPP
#define DUMMY_DAQ_HPP

#include <assert.h>
#include <math.h>

#include <string>
#include <vector>
#include <iostream>

#include <Windows.h>


#include "daq.hpp"

class DummyDaqDo: public DaqDo {
public:
   bool write(){
      return true;
   }
};//class, DummyDaqDo


class DummyDaqAi : public DaqAi {
public:
   DummyDaqAi(const vector<int>& chs):Daq(chs), DaqAi(chs){
      maxDigitalValue=2047; 
      minDigitalValue=-2048;
      maxPhyValue=10;
      minPhyValue=-10;
   }

   bool read(int nScans, sample_t* buf){
      //do nothing
      double time2wait=nScans/(double)scanRate;
      Sleep(time2wait*1000);

      return true;
   }

   bool start(){return true;}
   bool stop(){return true;}
   bool isError(){return false;}
   bool isSuc(){return true;}
   string getErrorMsg(){return "no error";}

   bool cfgTiming(int scanRate, int size){
      //this->scanRate=scanRate;
      this->scanRate=1000;

      return true;
   }
};//class, DummyDaqAi
#endif //DUMMY_DAQ_HPP

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

#ifndef DAQ_HPP
#define DAQ_HPP

#include <assert.h>
#include <math.h>

#include <string>
#include <vector>
#include <iostream>

using std::string;
using std::vector;
using std::cout;
using std::endl;

#include "misc.hpp"

class Daq {
protected:
   int maxDigitalValue;
   int minDigitalValue;
   double minPhyValue, maxPhyValue; //min/max value in vol
   vector<int> channels;

public:
   typedef unsigned short sample_t;

   unsigned int  sampleSize; //in bits
   int scanRate;
   int nScans;

   //exception classes:

   class EInitDevice {
   public:
      string msg;

      EInitDevice(string msg){
         this->msg=msg;
      }
   };//class, EInitDevice 

   class ENoEnoughMem {};

   //ctor: create task
   Daq(vector<int> chs){
      this->channels=chs;
      if(channels.size()==0){
         throw Daq::EInitDevice("exception: no channel");
      }
      nScans=0;

   }//ctor,

   //dtor: clear task (i.e. release driver-allocated resources) 
   virtual ~Daq(){
   }//dtor,

   double toPhyUnit(double digValue){ //NOTE: cast int to double here
      return (digValue-minDigitalValue)/(maxDigitalValue-minDigitalValue)
         *(maxPhyValue-minPhyValue)+minPhyValue;
   }//toPhyUnit()

   int toDigUnit(double phyValue){
      return (phyValue-minPhyValue)/(maxPhyValue-minPhyValue)
         *(maxDigitalValue-minDigitalValue)+minDigitalValue;
   }//toDigUnit()
   
   //for ai: size is input buf size,
   //for ao: size is #scans to acquire
   //todo: for DIO: size is ...
   virtual bool cfgTiming(int scanRate, int size)=0;  

   //start task
   virtual bool start()=0;

   //stop task (i.e. abort if running & reset the task)
   virtual bool stop()=0;

   //check if an error code means error.
   //note: an error code may mean success/warning/error, like compiler's result.
   //return true if errorCode means error,
   //return false if errorCode means warning or success.
   virtual bool isError()=0;

   //similar to isError(). 
   //return true if errorCode means success, 
   //return false if it means warning or error.
   virtual bool isSuc()=0;

   //get error msg
   virtual string getErrorMsg()=0;
};//class, Daq

//class: DAQ Analog Output
//note: this class manages its output buffer itself.
class DaqAo {
public:
   //get output buffer address
   //NOTE: data is grouped by channel, i.e. all samples for a channel are close to each other
   virtual Daq::sample_t * getOutputBuf()=0;

   //write waveform to driver's buffer
   virtual bool updateOutputBuf()=0;

   //wait task until done
   virtual bool wait(double timeToWait)=0;

   //query if task is done
   virtual bool isDone()=0;

};//class, NiDaqAo


//unlike AO, user need supply read buffer for AI
class DaqAi {
public:
   //read input from driver
   virtual bool read(int nScans, Daq::sample_t * buf)=0;
};//class, DaqAi

//dig-out 
class DaqDo {
protected:
   unsigned char       data[8];
public:
   //create DO channel and start the task
   DaqDo(){
      for(int i=0; i<sizeof(data)/sizeof(data[0]); ++i) data[i]=0;
   }//ctor, DaqDo()

   virtual ~DaqDo(){
   }//dtor, ~DaqDo()

   //set bool value to output
   void updateOutputBuf(int lineIndex, bool newValue){
      assert(lineIndex>=0 && lineIndex<=7);
         
      data[lineIndex]=newValue;
   }//updateOutputBuf(),

   //output data to hardware
   virtual bool write()=0;
};//class, NiDaqDo

//read one sample
class DaqAiReadOne {
protected:
   int channel;
public:
   DaqAiReadOne(int channel){
      this->channel=channel;
   }//ctor,

   virtual ~DaqAiReadOne(){
   }//dtor,

   //return true if success
   virtual bool readOne(int & reading)=0;

   virtual double toPhyUnit(double digValue)=0;
};//class, NiDaqAiReadOne

//write one sample
class DaqAoWriteOne {
protected:
   int channel;
public:
   DaqAoWriteOne(int channel){
      this->channel=channel;
   }//ctor,

   virtual ~DaqAoWriteOne(){
   }

   //return true if success
   virtual bool writeOne(int valueToOutput)=0;
   virtual int toDigUnit(double phyValue)=0;
};//class, DaqAoWriteOne 


#endif //DAQ_HPP

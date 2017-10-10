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

#ifndef NI_DAQ_G_HPP
#define NI_DAQ_G_HPP

#include <assert.h>
#include <math.h>

#include <string>
#include <vector>
#include <iostream>

using std::string;
using std::vector;
using std::cout;
using std::endl;

#include "NIDAQmx.h"

#include "misc.hpp"
#include "daq.hpp"



typedef void(*DAQ_CALLBACK_FP)(void *);

class NiDaq : public virtual Daq {
protected:
   TaskHandle  taskHandle;
   int errorCode;  
   char    errBuff[2048] = { '\0' };
public:
   //ctor: create task
   NiDaq(const vector<int> & chs): Daq(chs){
      taskHandle=0;
      errorCode=DAQmxCreateTask("",&taskHandle);
      if(isError()){
         throw EInitDevice("exception when call DAQmxCreateTask()");
      }
   }//ctor,

   //dtor: clear task (i.e. release driver-allocated resources) 
   virtual ~NiDaq(){
      DAQmxClearTask(taskHandle);
   }//dtor,


   //start task
   bool start(){
      errorCode=DAQmxStartTask(taskHandle);
      return !isError();
   }//start(),

   //stop task (i.e. abort if running & reset the task)
   bool stop(){
      errorCode=DAQmxStopTask(taskHandle);
      return !isError();
   }//stop(),

   //check if an error code means error. This func wraps the macro.
   //note: an error code may mean success/warning/error, like compiler's result.
   //return true if errorCode means error,
   //return false if errorCode means warning or success.
   bool isError(){
      return DAQmxFailed(errorCode); 
   }//isError(),

   //similar to isError(). This func is a wrapper to a macro.
   //return true if errorCode means success, 
   //return false if it means warning or error.
   bool isSuc(){
      return DAQmxSuccess==errorCode;
   }//isSuc(),

   //get error msg
   string getErrorMsg(){
      static char errBuf[2048*2]={'\0'};
      string errorMsg;

      if(isSuc()){
         errorMsg="no error/warning";
      }//if, no error & no warning
      else{
         DAQmxGetExtendedErrorInfo(errBuf, sizeof(errBuf));
         if(isError()) errorMsg="Error msg:\n";
         else errorMsg="Warning msg:\n";
         errorMsg+=errBuf;
      }//else, there is error or warning

      return errorMsg;
   }//getErrorMsg(),

    //wait task until done
   bool wait(double timeToWait) {
       errorCode = DAQmxWaitUntilTaskDone(taskHandle, timeToWait);
       return !isError();
   }//wait(),

    //query if task is done
   bool isDone() {
       bool32 result;
       errorCode = DAQmxIsTaskDone(taskHandle, &result);

       return result != 0;
   }//isDone(),

};//class, NiDaq

//class: NI DAQ Analog Output
//note: this class manages its output buffer itself.
class NiDaqAo: public NiDaq, public DaqAo {
   sample_t *    dataU16; 
   float64 *    dataF64;
   string   dev;
   string trigOut;
   string clkOut;
   int blockSize;

public:
   static DAQ_CALLBACK_FP nSampleCallback;
   static DAQ_CALLBACK_FP doneCallback;
   static void *instance;
   static void *doneInstance;

   //create ao channel and add the channel to task
   NiDaqAo(QString devstring, const vector<int> & chs): Daq(chs), NiDaq(chs), DaqAo(chs){
	  dataF64 = 0;
      dev = devstring.toStdString();
//      trigOut.assign(dev).append("/StartTrigger");
      trigOut="/"+dev+"/StartTrigger";
      clkOut = "/" + dev + "/SampleClock";
      cout << "About to initialize AO device " << dev << endl;
      //string dev="Dev1/ao";
      string chanList=dev+toString(channels[0]);
      for(unsigned int i=1; i<channels.size(); ++i){
         chanList+=", "+dev+toString(channels[i]);
      }

      errorCode=DAQmxCreateAOVoltageChan(taskHandle,
         chanList.c_str(), //channels to acquire
         "",  //nameToAssignToChannel
         -10.0,   //min value to output
         10.0,//max value to output
         DAQmx_Val_Volts,  //unit
         NULL              //customScaleName
         );
      if(isError()){
         throw EInitDevice("exception when call DAQmxCreateAOVoltageChan()");
      }

      //get the raw sample size, min/max digital values:
      float64 tt;
      errorCode=DAQmxGetAOResolution(taskHandle,(dev+"0").c_str(), &tt);
      if(isError()){
         throw EInitDevice("exception when call DAQmxGetAOResolution()");
      }
      sampleSize=(uInt32)tt;
      maxDigitalValue=pow(2.0,int(sampleSize)-1)-1;
      minDigitalValue=-pow(2.0,int(sampleSize)-1);
      maxPhyValue=10;
      minPhyValue=-10;

   }//ctor,

   //release mem
   virtual ~NiDaqAo(){
	  if (dataF64) delete[] dataF64;
   }//dtor,

   //set rate and duration, and allocate output buffer
   //return true if success or warning;
   //return false if error
   //note: you can check for error by checking return value OR call .isError() 
   bool cfgTiming(int scanRate, int nScans, string clkName = ""){ //nScans: #scans to acquire
      this->scanRate=scanRate;
      this->nScans=nScans;

      errorCode=DAQmxCfgSampClkTiming(taskHandle,clkName.c_str(),
         scanRate,
         DAQmx_Val_Rising,
         DAQmx_Val_FiniteSamps,
         nScans
         );

      //allocate output buffer
      dataU16=new uInt16[nScans*channels.size()];
	  if (dataF64)delete[] dataF64;
	  dataF64 = 0;
	  dataF64 = new float64[nScans*channels.size()];
      if(!dataF64){
         throw ENoEnoughMem();
      }

      return !isError();
   }//cfgTiming(),

   bool setTrigger(string trigName) {
       errorCode=DAQmxCfgDigEdgeStartTrig(taskHandle, trigName.c_str(), DAQmx_Val_Rising);
       return !isError();
   }//cfgTrigger

   //get output buffer address
   //NOTE: data is grouped by channel, i.e. all samples for a channel are close to each other
   float64 * getOutputBuf(){ //uInt16 * getOutputBuf(){
      //return dataU16;
	  return dataF64;
   }//getOutputBuf()

   //write waveform to driver's buffer
   bool updateOutputBuf(){
//    cout<<"in  updateOutputBuf()"<<endl;
      int32 nScanWritten;
	  char  errBuff[2048] = { '\0' };
	  /*
	  float64* dataF64 = new float64[nScans];
	  for (int i = 0; i < nScans; ++i) {
		  dataF64[i] = 1.25;
	  }
	  */
	  if (DAQmxFailed(errorCode = (DAQmxWriteAnalogF64(taskHandle,
		  nScans,
		  false,
		  DAQmx_Val_WaitInfinitely,
		  DAQmx_Val_GroupByChannel, //data layout, all samples for a channel then for another channel
		  dataF64,
		  &nScanWritten,
		  NULL
		  )))) goto Error;
      goto Done;

	  Error:
		if (DAQmxFailed(errorCode))
			DAQmxGetExtendedErrorInfo(errBuff, 2048);
      Done:
//      cout<<"out updateOutputBuf()"<<endl;
      return !isError();
   }//updateOutputBuf(),

   string getTrigOut() { return trigOut; }
   string getClkOut() { return clkOut; }

   /* Continuously buffered output mode
   In this mode, buffer will be filled with a block of new data in every n sample event.
   With this mode, we can output infinite numbers of data to the analog port.
   We set the number n arbitrarily as scanRate.
   1. cfgTimingBuffered()
   2. setNSampleCallback()
   3. copy initial two block of n samle data to dataU32[]
   4. updateOutputBuf(blockSize*2)
   5. Task start
   */
   bool cfgTimingBuffered(int scanRate, int nScans, string clkName = "") {
       this->scanRate = scanRate;
       this->nScans = nScans;
       // sample number of the third block should be greater than 4
       this->blockSize = (nScans-4 < scanRate * 8) ? nScans / 2 -2 : scanRate * 4;

       errorCode = DAQmxCfgSampClkTiming(taskHandle, clkName.c_str(),
           scanRate,
           DAQmx_Val_Rising,
           DAQmx_Val_FiniteSamps,
           nScans
       );

       //allocate output buffer
       dataU16 = new uInt16[nScans*channels.size()];
       if (dataF64)delete[] dataF64;
       dataF64 = 0;
       dataF64 = new float64[blockSize * 2 *channels.size()];
       if (!dataF64) {
           throw ENoEnoughMem();
       }

       return !isError();
   }//cfgTimingBuffered(),

    // aoEveryNCallback : Callback function for every n sample event.
    // These callback will work after registration using DAQmxRegisterEveryNSamplesEvent().
    // To make this callback function be called for the first time,
    // DAQmxWriteDigitalU32 should be called from outside first.
    // When this DAQmxWriteDigitalU32 is called, two blocks of n samples should be written to the buffer.
    // Once this callback is called, this function should call DAQmxWriteDigitalU32 repeatedly.
    // In these calls, one next block of n samples should be written to the buffer each time.
    // Actually, this is a wrapper for customized callback function (nSampleCallback).
    // This provide a required function format of this callback.
   static int32 CVICALLBACK aoEveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType,
       uInt32 nSamples, void *callbackData)
   {
       if (nSampleCallback) {
           nSampleCallback(instance);
       }
       return 0;
   }

   // This registers aoEveryNCallback to every N sample event.
   bool setNSampleCallback(DAQ_CALLBACK_FP callback, void* ins) {
       nSampleCallback = callback;
       instance = ins;
       errorCode = DAQmxRegisterEveryNSamplesEvent(taskHandle,
           DAQmx_Val_Transferred_From_Buffer,
           blockSize, 0, aoEveryNCallback, dataF64);
       return !isError();
   }//setNSampleCallback

   static int32 CVICALLBACK aoTaskDoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
   {
       if (doneCallback) {
           doneCallback(doneInstance);
       }
       return 0;
   }

   // This registers aoEveryNCallback to every N sample event.
   bool setTaskDoneCallback(DAQ_CALLBACK_FP callback, void* ins) {
       doneCallback = callback;
       doneInstance = ins;
       errorCode = DAQmxRegisterDoneEvent(taskHandle, 0, aoTaskDoneCallback, dataF64);
       return !isError();
   }//setNSampleCallback

   bool updateOutputBuf(int numSample) {
//       cout << "in AO updateOutputBuf()" << endl;
       int32 nScanWritten;
       char  errBuff[2048] = { '\0' };
       if (DAQmxFailed(errorCode = (DAQmxWriteAnalogF64(taskHandle,
           numSample,
           false,
           DAQmx_Val_WaitInfinitely,
           DAQmx_Val_GroupByChannel, //data layout, all samples for a channel then for another channel
           dataF64,
           &nScanWritten,
           NULL
       )))) goto Error;
       goto Done;

   Error:
       if (DAQmxFailed(errorCode))
           DAQmxGetExtendedErrorInfo(errBuff, 2048);
   Done:
//       cout << "out AO updateOutputBuf()" << endl;
       return !isError();
   }//updateOutputBuf(int numSample),

   int getBlockSize(void) { return blockSize; }

};//class, NiDaqAo


//unlike AO, user need supply read buffer for AI
class NiDaqAi: public NiDaq, public DaqAi {
public:
	int32 numScalingCoeffs = 0;
	float64* scalingCoeffs;
   //create AI channels and add the channels to the task
   NiDaqAi(QString devstring,const vector<int> & chs): Daq(chs), NiDaq(chs), DaqAi(chs){
      //todo: next line is unnecessary?
      //DAQmxErrChk(DAQmxCfgInputBuffer(taskHandle, buf_size) ); //jason: this change DEFAULT(?) input buffer size
      string dev = devstring.toStdString();
      cout << "About to initialize AI device " << dev << endl;
      //string dev="Dev1/ai";
      string chanList=dev+toString(channels[0]);
      for(unsigned int i=1; i<channels.size(); ++i){
         chanList+=", "+dev+toString(channels[i]);
      }
      errorCode=DAQmxCreateAIVoltageChan(taskHandle,
         chanList.c_str(),  //channels to acquire
         "",          //nameToAssignToChannel
         DAQmx_Val_RSE, //RSE, //Diff, //NRSE, //terminalConfig. todo: should be DAQmx_Val_NRSE?
         -10.0,   //input range: minVal 
         10.0,    //input range: maxVal
         DAQmx_Val_Volts, //unit
         NULL     //reserved
         );  
      if(isError()){
         throw EInitDevice("exception when call DAQmxCreateAIVoltageChan()");
      }
      //get the raw sample size:
      errorCode=DAQmxGetAIRawSampSize(taskHandle, (dev+toString(channels[0])).c_str(), (uInt32*)&sampleSize);
      if(isError()){
         throw EInitDevice("exception when call DAQmxGetAIRawSampSize()");
      }
	  //TODO: above func set sampleSize to 16, which is incorrect. 
	  //      should be 12. a Temp fix: (or not?)
	  //sampleSize=12;

	  maxDigitalValue = pow(2.0, int(sampleSize) - 1) - 1;
	  minDigitalValue = -pow(2.0, int(sampleSize) - 1);

      maxPhyValue=10;
      minPhyValue=-10;

	  // Obtain the number of coefficients.  All channels in a single task will have the same number of
	  // coefficients so we will just look at the first channel.
	  // Passing a NULL and a 0 as the array and number of samples respectively
	  // causes the function to return the number of samples instead of an error.
	  // Positive number errors are not considered errors and will not halt execution
	  numScalingCoeffs = DAQmxGetAIDevScalingCoeff(taskHandle, "", NULL, 0);
	  // Allocate Memory for coefficient array
	  scalingCoeffs = (float64*)(malloc(sizeof(float64)*numScalingCoeffs));
	  // Get Coefficients
	  DAQmxGetAIDevScalingCoeff(taskHandle, "", scalingCoeffs, numScalingCoeffs);
   }//ctor, NiDaqAi

   //dtor:
   virtual ~NiDaqAi(){
	   delete[] scalingCoeffs;
   }//dtor, NiDaqAi

   //set scan rate and driver's input buffer size in scans
   bool cfgTiming(int scanRate, int bufSize, string clkName = ""){
      this->scanRate=scanRate;
      errorCode=DAQmxCfgSampClkTiming(taskHandle, clkName.c_str(),
         scanRate,
         DAQmx_Val_Rising,
         DAQmx_Val_ContSamps,
         bufSize //the unit is scan
         ); 
      //TODO: this doesn't seem to show a warning or an error when asking for an impossibly high scan rate.  should find a way to check that.
      return isSuc();
   }//cfgTiming(),

   bool setTrigger(string trigName) {
       errorCode = DAQmxCfgDigEdgeStartTrig(taskHandle, trigName.c_str(), DAQmx_Val_Rising);
       return !isError();
   }//cfgTrigger

   //read input from driver
   bool read(int nScans, float64 * buf) {
	  int32 nScansRead;
	  errorCode=DAQmxReadAnalogF64(taskHandle,
		  nScans, 
		  5,// timeToWait 5sec,  or DAQmx_Val_WaitInfinitely
		  DAQmx_Val_GroupByScanNumber, //data layout, here save all data in scan by scan
		  buf, 
		  nScans*channels.size(), 
		  &nScansRead, 
		  NULL //reserved
		  );

#if defined(DEBUG_AI)
      if(isError()){
         getErrorMsg();
      }
#endif

      assert(nScans==nScansRead || isError());

      return !isError();
   }//readInput(),

   double toPhyUnit(int digValue) {
		// Set offset as initial value
		float64 scaledData = scalingCoeffs[0]; //first element of coefficients is offset
		// Calculate gain error with the remaining coefficients (for M Series
		// There are 3.  For E-Series there is only 1.
		for (int32 k = 1; k<numScalingCoeffs; k++) {
			scaledData +=
				scalingCoeffs[k] * pow(digValue, (float)k);
		}
		scaledData = (digValue / pow(2, 15))*10.0;
		return scaledData;
   }

};//class, NiDaqAi


class NiDaqDo: public NiDaq, public DaqDo {
    uInt32*    dataU32; // burst out buffer
    int blockSize;
    uInt32 numP0Channel;

public:
    static DAQ_CALLBACK_FP nSampleCallback;
    static void *instance;
    //create DO channel and start the task
   NiDaqDo(QString devstring): Daq(vector<int>()), NiDaq(vector<int>()){
      dataU32 = nullptr;
      string dev = devstring.toStdString();
      cout << "About to initialize DO device with " << dev << endl;
      errorCode=DAQmxCreateDOChan(taskHandle,dev.c_str(),"",DAQmx_Val_ChanForAllLines);
      if(isError()){
         throw EInitDevice("exception when call DAQmxCreateDOChan()");
      }
      // count a number of DO channels
      //DAQmxGetTaskAttribute(taskHandle, DAQmx_Task_NumChans, &numP0Channel);
      char ch[256];
      DAQmxGetTaskChannels(taskHandle, ch, sizeof(ch));
      DAQmxGetDONumLines(taskHandle, ch, &numP0Channel);
      //      if(!start()){
//         throw EInitDevice(string("exception when call DAQmxCreateDOChan():\n")+getErrorMsg());
//      }
   }//ctor, NiDaqDo()

   //stop the task
   ~NiDaqDo(){
      stop();
   }//dtor, ~NiDaqDo()


   // single output
   bool outputChannelOnce(int lineIndex, bool newValue) {
       assert(lineIndex >= 0 && lineIndex < numP0Channel);
       //errorCode = DAQmxResetSampTimingType(taskHandle); // reset to DAQmx_Val_OnDemand
       uInt32 readData;
       int32 nScansRead, numBytesPerSamp;
       errorCode = DAQmxReadDigitalLines(taskHandle,
           1, //numSampsPerChan
           10.0, //timeout: 10s
           DAQmx_Val_GroupByChannel,//
           data,
           numP0Channel,
           &nScansRead,
           &numBytesPerSamp,
           NULL); //reserved
       if (errorCode != 0) {
           DAQmxGetExtendedErrorInfo(errBuff, 2048);
           return !isError();
       }
       data[lineIndex] = newValue;
       errorCode = DAQmxWriteDigitalLines(taskHandle,
           1, //numSampsPerChan
           true, //autoStart
           10.0, //timeout: 10s
           DAQmx_Val_GroupByChannel,//
           data,
           NULL, //sampsPerChanWritten
           NULL);//reserved
       if(errorCode!=0)
            DAQmxGetExtendedErrorInfo(errBuff, 2048);

       return !isError();
   }//write(),

   //output data to hardware
   bool write(){
      errorCode=DAQmxWriteDigitalLines(taskHandle,
         1, //numSampsPerChan
         1, //autoStart
         10.0, //timeout: 10s
         DAQmx_Val_GroupByChannel,//
         data,
         NULL, //sampsPerChanWritten
         NULL);//reserved

      return !isError();
   }//write(),

    //output data to hardware
   bool clearAllOutput() {
       for(int i = 0; i< numP0Channel;i++) data[i] = 0;
       errorCode = DAQmxWriteDigitalLines(taskHandle,
           1, //numSampsPerChan
           1, //autoStart
           10.0, //timeout: 10s
           DAQmx_Val_GroupByChannel,//
           data,
           NULL, //sampsPerChanWritten
           NULL);//reserved

       return !isError();
   }//write(),

    //   bool cfgTiming(int, int){ return true;} //do noting for Dig-out
   bool cfgTiming(int scanRate, int nScans, string clkName = "") {
       this->scanRate = scanRate;
       this->nScans = nScans;

       errorCode = DAQmxCfgSampClkTiming(taskHandle, clkName.c_str(),
           scanRate,
           DAQmx_Val_Rising,
           DAQmx_Val_FiniteSamps,
           nScans
       );

       //allocate output buffer
       if (dataU32) delete[] dataU32;
       dataU32 = new uInt32[nScans];
       if (!dataU32) {
           throw ENoEnoughMem();
       }

       return !isError();
   }//cfgTiming(),

   bool setTrigger(string trigName) {
       if (trigName != "")
           errorCode = DAQmxCfgDigEdgeStartTrig(taskHandle, trigName.c_str(), DAQmx_Val_Rising);
       else
           return false;
       return !isError();
   }//setTrigger

   uInt32* getOutputBuf() {
       return dataU32;
   }//getOutputBuf()

   bool updateOutputBuf() {
//       cout << "in DO updateOutputBuf()" << endl;
       int32 nScanWritten;
       char  errBuff[2048] = { '\0' };
       if (DAQmxFailed(errorCode = (DAQmxWriteDigitalU32(taskHandle,
           nScans,
           false,
           DAQmx_Val_WaitInfinitely,
           DAQmx_Val_GroupByChannel, //data layout, all samples for a channel then for another channel
           dataU32,
           &nScanWritten,
           NULL
       )))) goto Error;
       goto Done;
   Error:
       if (DAQmxFailed(errorCode))
           DAQmxGetExtendedErrorInfo(errBuff, 2048);
   Done:
//       cout << "out DO updateOutputBuf()" << endl;
       return !isError();
   }//updateOutputBuf(),

   /* Continuously buffered output mode
    In this mode, buffer will be filled with a block of new data in every n sample event.
    With this mode, we can output infinite numbers of data to the digital port.
    We set the number n arbitrarily as scanRate.
    1. cfgTimingBuffered()
    2. setNSampleCallback()
    3. copy initial two block of n samle data to dataU32[]
    4. updateOutputBuf(blockSize*2)
    5. Task start
    */
   bool cfgTimingBuffered(int scanRate, int nScans, string clkName = "") {
       this->scanRate = scanRate;
       this->nScans = nScans;
       // sample number of the third block should be greater than 4
       this->blockSize = (nScans - 4 < scanRate * 8) ? nScans / 2 - 2 : scanRate * 4;
       errorCode = DAQmxCfgSampClkTiming(taskHandle, clkName.c_str(),
           scanRate,
           DAQmx_Val_Rising,
           DAQmx_Val_FiniteSamps,
           nScans
       );

       //allocate output buffer
       if (dataU32) delete[] dataU32;
       dataU32 = new uInt32[blockSize * 2];
       if (!dataU32) {
           throw ENoEnoughMem();
       }

       return !isError();
   }//cfgTimingBuffered(),

    // doEveryNCallback : Callback function for every n sample event.
    // These callback will work after registration using DAQmxRegisterEveryNSamplesEvent().
    // To make this callback function be called for the first time,
    // DAQmxWriteDigitalU32 should be called from outside first.
    // When this DAQmxWriteDigitalU32 is called, two blocks of n samples should be written to the buffer.
    // Once this callback is called, this function should call DAQmxWriteDigitalU32 repeatly.
    // In these calls, one next block of n samples should be written to the buffer each time.
    // Actually, this is a wrapper for customized callback function (nSampleCallback).
    // This provide a required function format of this callback.
   static int32 CVICALLBACK doEveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType,
       uInt32 nSamples, void *callbackData)
   {
       if (nSampleCallback) {
           nSampleCallback(instance);
       }
       return 0;
   }

   // This registers aoEveryNCallback to every N sample event.
   bool setNSampleCallback(DAQ_CALLBACK_FP callback, void* ins) {
       nSampleCallback = callback;
       instance = ins;
       errorCode = DAQmxRegisterEveryNSamplesEvent(taskHandle,
           DAQmx_Val_Transferred_From_Buffer,
           blockSize, 0, doEveryNCallback, dataU32);
       return !isError();
   }//setNSampleCallback

   bool updateOutputBuf(int numSample) {
//       cout << "in DO updateOutputBuf()" << endl;
       int32 nScanWritten;
       char  errBuff[2048] = { '\0' };
       if (DAQmxFailed(errorCode = (DAQmxWriteDigitalU32(taskHandle,
           numSample,
           false,
           DAQmx_Val_WaitInfinitely,
           DAQmx_Val_GroupByChannel, //data layout, all samples for a channel then for another channel
           dataU32,
           &nScanWritten,
           NULL
       )))) goto Error;
       goto Done;

   Error:
       if (DAQmxFailed(errorCode))
           DAQmxGetExtendedErrorInfo(errBuff, 2048);
   Done:
//       cout << "out DO updateOutputBuf()" << endl;
       return !isError();
   }//updateOutputBuf(int numSample),

   int getBlockSize(void) { return blockSize; }

};//class, NiDaqDo

//unlike DO, user need supply read buffer for DI
class NiDaqDi : public NiDaq, public DaqDi {
public:
    int32 numScalingCoeffs = 0;
    float64* scalingCoeffs;
    //create DI channels and add the channels to the task
    NiDaqDi(QString devstring, const vector<int> & chs) : Daq(chs), NiDaq(chs), DaqDi(chs) {
        string dev = devstring.toStdString();
        cout << "About to initialize DI device with " << dev << endl;
        errorCode = DAQmxCreateDIChan(taskHandle,
            dev.c_str(),  //channels to acquire
            "",          //nameToAssignToChannel
            DAQmx_Val_ChanForAllLines
        );
        if (isError()) {
            throw EInitDevice("exception when call DAQmxCreateDIChan()");
        }
    }// NiDaqDi

    virtual ~NiDaqDi() {
    }// ~NiDaqDi

     //set scan rate and driver's input buffer size in scans
    bool cfgTiming(int scanRate, int bufSize, string clkName = "") {
        this->scanRate = scanRate;
        errorCode = DAQmxCfgSampClkTiming(taskHandle, clkName.c_str(),
            scanRate,
            DAQmx_Val_Rising,
            DAQmx_Val_ContSamps,
            bufSize //the unit is scan
        );
        //TODO: this doesn't seem to show a warning or an error when asking for an impossibly high scan rate.  should find a way to check that.
        return isSuc();
    }//cfgTiming(),

    bool setTrigger(string trigName) {
        errorCode = DAQmxCfgDigEdgeStartTrig(taskHandle, trigName.c_str(), DAQmx_Val_Rising);
        return !isError();
    }//setTrigger

     //read input from driver
    bool read(int nScans, uInt32 * buf) {
        int32 nScansRead;
        errorCode = DAQmxReadDigitalU32(taskHandle,
            nScans,
            5,// timeToWait 5sec,  or DAQmx_Val_WaitInfinitely
            DAQmx_Val_GroupByChannel, //data layout, here save all data in scan by scan
            buf,
            nScans,
            &nScansRead,
            NULL //reserved
        );

#if defined(DEBUG_DI)
        if (isError()) {
            getErrorMsg();
        }
#endif

        assert(nScans == nScansRead || isError());

        return !isError();
    }//readInput(),
};//class, NiDaqDi

//read one sample
class NiDaqAiReadOne :public DaqAiReadOne {
   NiDaqAi * ai;
public:
   NiDaqAiReadOne(QString ainame, int channel):DaqAiReadOne(channel){
      vector<int> chs;
      chs.push_back(channel);
      ai=new NiDaqAi(ainame, chs);

      if(!ai->cfgTiming(10000, 10000)){
         throw Daq::EInitDevice("exception when cfgTiming()");
      }
   }//ctor,

   ~NiDaqAiReadOne(){
      delete ai;
   }//dtor,

   //return true if success
   bool readOne(float64 & reading) {
	   if (!ai->start()) return false;
	   if (!ai->read(1, &reading)) return false;
	   if (!ai->stop()) return false;
	   //TODO: convert negative voltages
	   /*
	   if (*tReading < pow(2, 15)) {
		   reading = -1 * (int)(pow(2, 16) - *tReading);
	   }
	   else {
	   reading = (int)tReading;
		}
		*/
      return true;
   }//readOne()

   double toPhyUnit(int digValue){
      return ai->toPhyUnit(digValue);
   }
};//class, NiDaqAiReadOne

//write one sample
class NiDaqAoWriteOne : public DaqAoWriteOne{
   NiDaqAo* ao;
public:
   NiDaqAoWriteOne(QString aoname,int channel): DaqAoWriteOne(channel){
      vector<int> channels;
      channels.push_back(channel);
      ao=new NiDaqAo(aoname, channels);

      if(!ao->cfgTiming(10000, 2)){ //for buffered writing, 2 samples at least. //SEE: DAQmxWriteBinaryU16()'s online help.
         throw Daq::EInitDevice("exception when cfgTiming()");
      }
   }//ctor,

   ~NiDaqAoWriteOne(){
      delete ao;
   }

   //return true if success
   bool writeOne(int valueToOutput){
      cout<<"in  writeOne()"<<endl;
      //uInt16* buf=ao->getOutputBuf();
	  float64* buf = ao->getOutputBuf();
      buf[0]=float64(valueToOutput);
      buf[1]=buf[0];
      if(!ao->updateOutputBuf()) {
         cout<<ao->getErrorMsg()<<endl;
         return false;
      }
      if(!ao->start()) return false;
      if(!ao->wait(-1)) return false; //wait forever
      if(!ao->stop()) return false;

      cout<<"out writeOne()"<<endl;
      return true;
   }//writeOne(),

   int toDigUnit(double phyValue){
      return ao->toDigUnit(phyValue);
   }//toDigUnit(),

};//class, NiDaqAoWriteOne 

#endif //NI_DAQ_G_HPP

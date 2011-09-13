#include "volpiezo.hpp"

#include <Windows.h>

#include "ni_daq_g.hpp"

NiDaqAoWriteOne * aoOnce=NULL; //todo: move it inside the class
NiDaqAo* ao=nullptr;

double VolPiezo::zpos2voltage(double um)
{
    return um/this->max()*10; // *10: 0-10vol range for piezo
}


bool VolPiezo::moveTo(double to)
{
   double vol=zpos2voltage(to);
   if(aoOnce==NULL){
      int aoChannelForPiezo=0; //TODO: put this configurable
      aoOnce=new NiDaqAoWriteOne(aoChannelForPiezo);
      Sleep(0.5*1000); //sleep 0.5s 
   }
   uInt16 sampleValue=aoOnce->toDigUnit(vol);
   //temp commented: 
   try{
      assert(aoOnce->writeOne(sampleValue));
   }
   catch(...){
      
   }

}


bool VolPiezo::prepareCmd()
{
   //prepare for AO:
   if(aoOnce){
      delete aoOnce;
      aoOnce=nullptr;
   }

   int aoChannelForPiezo=0; //TODO: put this configurable
   vector<int> aoChannels;
   aoChannels.push_back(aoChannelForPiezo);
   int aoChannelForTrigger=1;
   aoChannels.push_back(aoChannelForTrigger);

   if(ao) cleanup();
   ao=new NiDaqAo(aoChannels);

   double durationAo=nFramesPerStack*cycleTime; //in sec
   if(triggerMode==Camera::eInternalTrigger){
      durationAo+=0.1; 
   }
   else if(triggerMode==Camera::eExternalStart){
      durationAo+=piezoPreTriggerTime;
   }

   double durationResetPosition=idleTimeBwtnStacks/2;
   int nScansForReset=int(scanRateAo*durationResetPosition);
   ao->cfgTiming(scanRateAo, int(scanRateAo*durationAo)+nScansForReset); //+(): for reset position
   if(ao->isError()) {
      delete ao;
      emit newStatusMsgReady("error when configure AO timing");
      return;
   }

   //get buffer address and generate waveform:
   int aoStartValue=ao->toDigUnit(piezoStartPos);
   int aoStopValue=ao->toDigUnit(piezoStopPos);

   uInt16 * bufAo=ao->getOutputBuf();
   for(int i=0;i<ao->nScans-nScansForReset;i++){
      bufAo[i] = uInt16( double(i)/(ao->nScans-nScansForReset)*(aoStopValue-aoStartValue)+aoStartValue );
   }
   int firstScanNumberForReset=ao->nScans-nScansForReset;
   for(int i=firstScanNumberForReset; i<ao->nScans; ++i){
      bufAo[i] = uInt16( double(i-firstScanNumberForReset)/(nScansForReset)*(aoStartValue-aoStopValue)+aoStopValue );
   }
   bufAo[ao->nScans-1]=bufAo[0]; //the last sample resets piezo's position exactly

   if(triggerMode==Camera::eExternalStart){
      bufAo+=ao->nScans;
      int aoTTLHigh=ao->toDigUnit(5.0);            
      int aoTTLLow=ao->toDigUnit(0);
      for(int i=0;i<ao->nScans;i++){
         bufAo[i] = aoTTLLow; 
      }
      bufAo[int(piezoPreTriggerTime*scanRateAo)]=aoTTLHigh; //the sample that starts camera
   }

   ao->updateOutputBuf();
   //todo: check error like
   // if(ao->isError()) goto exitpoint;

}//prepareCmd(),


bool VolPiezo::runCmd()
{
   //start piezo movement (and may also trigger the camera):
   ao->start(); //todo: check error

}


bool VolPiezo::waitCmd()
{
   //wait until piezo's orig position is reset
   ao->wait(-1); //wait forever
   //stop ao task:
   ao->stop();
   cleanup();
}


bool VolPiezo::abortCmd()
{
   //stop ao task:
   ao->stop();
   cleanup();
}


void VolPiezo::cleanup()
{
   delete ao;
   ao=nullptr;

   //wait 200ms to have Piezo settled (so ai thread can record its final position)
   Sleep(0.2*1000); // *1000: sec -> ms
}


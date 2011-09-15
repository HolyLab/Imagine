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

}//moveTo(),


bool VolPiezo::addMovement(double to, double duration, int trigger)
{
   bool result=Positioner::addMovement(to, duration, trigger);
   if(!result) return result;

   if(movements.size()==1){
      Movement& m=movements[0];
      m.duration+=0.06;
   }
   return result;
}//addMovement(),


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

   double totalTime=0;
   for(unsigned idx=0; idx<movements.size(); ++idx) totalTime+=movements[idx].duration/1e6; //macrosec to sec

   double durationAo=; //in sec
   double durationResetPosition=;
   int nScansForReset=int(scanRateAo*durationResetPosition);

   ao->cfgTiming(scanRateAo, int(scanRateAo*totalTime));
   if(ao->isError()) {
      cleanup();
      return false;
   }

   double piezoStartPos, piezoStopPos;
   for(unsigned idx=0; idx<movements.size(); ++idx){
      Movement& m=movements[idx];
      if(idx==0) piezoStartPos=from;
      else piezoStartPos=piezoStopPos;//todo: what if last "to" is nan
   }//for, each movement

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


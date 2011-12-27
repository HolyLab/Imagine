#include "volpiezo.hpp"

#include <Windows.h>

#include "volpiezo.hpp"

double VolPiezo::zpos2voltage(double um)
{
    return um/this->maxPos()*10; // *10: 0-10vol range for piezo
}


bool VolPiezo::curPos(double* pos)
{
   NiDaqAiReadOne ai(0); //channel 0: piezo
   int16 reading;
   ai.readOne(reading); //TODO: check error
   double vol=ai.toPhyUnit(reading);
   *pos=vol/10*this->maxPos();

   return true;
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
      return false;
   }

   return true;
}//moveTo(),


bool VolPiezo::addMovement(double from, double to, double duration, int trigger)
{
   bool result=Positioner::addMovement(from, to, duration, trigger);
   if(!result) return result;

   if(movements.size()==1){
      Movement& m=*movements[0];
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
   for(unsigned idx=0; idx<movements.size(); ++idx) totalTime+=movements[idx]->duration/1e6; //macrosec to sec

   int scanRateAo=10000; //TODO: hard coded

   ao->cfgTiming(scanRateAo, int(scanRateAo*totalTime));
   if(ao->isError()) {
      cleanup();
      lastErrorMsg="prepareCmd: failed to configure timing";
      return false;
   }

   double piezoStartPos, piezoStopPos, preStop;
   uInt16 * bufAo=ao->getOutputBuf();
   uInt16 * buf=bufAo-1;
   for(unsigned idx=0; idx<movements.size(); ++idx){
      const Movement& m=*movements[idx];
      if(_isnan(m.from)) piezoStartPos=preStop;
      else piezoStartPos=zpos2voltage(m.from);
      if(_isnan(m.to)) piezoStopPos=piezoStartPos;
      else piezoStopPos=zpos2voltage(m.to);
      
      preStop=piezoStopPos;

      //get buffer address and generate waveform:
      int aoStartValue=ao->toDigUnit(piezoStartPos);
      int aoStopValue=ao->toDigUnit(piezoStopPos);

      int nScansNow=int(scanRateAo*m.duration/1e6);
      if(idx==movements.size()-1){
         nScansNow=ao->nScans-(buf-bufAo+1);
      }//if, need roundup correction
      for(int i=0;i<nScansNow;i++){
         *++buf = uInt16( double(i)/nScansNow*(aoStopValue-aoStartValue)+aoStartValue );
      }
      if(nScansNow>0 || (nScansNow==0 && idx!=0)){//todo: if nScansNow==0 && idx==0, do this at end of for loop
         *buf=aoStopValue; //precisely set the last sample; or if move in 0 sec, rewrite last move's last sample
      }
   }//for, each movement

   /// now the trigger
   int aoTTLHigh=ao->toDigUnit(5.0);            
   int aoTTLLow=ao->toDigUnit(0);
   bufAo+=ao->nScans;
   buf=bufAo;
   for(int i=0;i<ao->nScans;i++){
      bufAo[i] = aoTTLLow; 
   }
   for(unsigned idx=0; idx<movements.size(); ++idx){
      const Movement& m=*movements[idx];
      int nScansNow=int(scanRateAo*m.duration/1e6);

      if(m.trigger==1){
         buf[0]=aoTTLHigh; //the sample that starts camera
      }
      buf+=nScansNow;
   }

   ao->updateOutputBuf();
   //todo: check error like
   // if(ao->isError()) goto exitpoint;

   return true;
}//prepareCmd(),


bool VolPiezo::runCmd()
{
   //start piezo movement (and may also trigger the camera):
   ao->start(); //todo: check error

   return true;
}


bool VolPiezo::waitCmd()
{
   //wait until piezo's orig position is reset
   ao->wait(-1); //wait forever
   //stop ao task:
   ao->stop();

   //wait 200ms to have Piezo settled (so ai thread can record its final position)
   Sleep(0.2*1000); // *1000: sec -> ms

   return true;
}


bool VolPiezo::abortCmd()
{
   //stop ao task:
   ao->stop();

   return true;
}

void VolPiezo::cleanup()
{
   delete ao;
   ao=nullptr;

}


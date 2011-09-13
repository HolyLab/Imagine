#include "volpiezo.hpp"

#include "ni_daq_g.hpp"

NiDaqAoWriteOne * aoOnce=NULL;

double VolPiezo::zpos2voltage(double um)
{
    return um/max()*10; // *10: 0-10vol range for piezo
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

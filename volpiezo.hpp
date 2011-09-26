#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

#include "ni_daq_g.hpp"

class VolPiezo: public Positioner {

public:
   VolPiezo(){aoOnce=nullptr; ao=nullptr;}
   ~VolPiezo(){delete aoOnce; delete ao;}

   double minPos(){return 0;}
   double maxPos(){return 400; }

   bool moveTo(double to);

   bool addMovement(double from, double to, double duration, int trigger);

   bool prepareCmd();
   bool runCmd();
   bool waitCmd();
   bool abortCmd();

private:
   NiDaqAoWriteOne * aoOnce;
   NiDaqAo* ao;

   double zpos2voltage(double um);
   void cleanup();
};//class, VolPiezo

#endif

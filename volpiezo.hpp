#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

class VolPiezo: public Positioner {

public:
   VolPiezo(){}
   ~VolPiezo(){}

   double min(){return 0;}
   double max(){return 400; }

   bool moveTo(double to);
   Status status();

   bool prepareCmd();
   bool runCmd();
   bool waitCmd();
   bool abortCmd();

};//class, VolPiezo

#endif

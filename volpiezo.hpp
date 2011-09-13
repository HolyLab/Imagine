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

   bool addMovement(double to, double duration, int trigger);

   bool prepareCmd();
   bool runCmd();
   bool waitCmd();
   bool abortCmd();

private:

   double zpos2voltage(double um);
   void cleanup();
};//class, VolPiezo

#endif

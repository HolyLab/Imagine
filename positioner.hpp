#ifndef POSITIONER_HPP
#define POSITIONER_HPP

#include <vector>
using std::vector;

//this is the base class of the specific positioners' classes

class Positioner {
protected:
   struct Movement {
      double to,
         duration;
      int trigger;
      Movement(double to, double duration, int trigger){
         this->to=to; this->duration=duration; this->trigger=trigger;
      }
   };
   double from;
   vector<Movement> movements;

public:
   Positioner(){}
   virtual ~Positioner(){}

   virtual double min()=0; // the min value of the position
   virtual double max()=0; // the max value of the position. NOTE: the unit is macro
   virtual bool moveTo(double to)=0; //move as quick as possible

   //// cmd related:
   virtual bool setFrom(double from); // set the initial position of the movement sequence
   virtual bool addMovement(double to, double duration, int trigger); //to: when NaN, don't move; trigger: 1 on, 0 off, otherwise no change
   virtual void clearCmd(); // clear the movement sequence

   virtual bool prepareCmd()=0; // after prepare, runCmd() will move it in real
   virtual bool runCmd()=0;
   virtual bool waitCmd()=0; // wait forever until the movement sequence finishes
   virtual bool abortCmd()=0;// stop it as soon as possible
};//class, Positioner

#endif //POSITIONER_HPP
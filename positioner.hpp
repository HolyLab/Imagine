#ifndef POSITIONER_HPP
#define POSITIONER_HPP

#include <vector>
using std::vector;

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

   virtual double min()=0;
   virtual double max()=0; //the unit is macro
   virtual bool moveTo(double to)=0; //move as quick as possible
   enum Status {idle, running, preparing};
   virtual Status status()=0;

   //// cmd related:
   virtual bool setFrom(double from);
   virtual bool addMovement(double to, double duration, int trigger); //to: when NaN, don't move; trigger: 1 on, 0 off, otherwise no change
   virtual void clearCmd();

   virtual bool prepareCmd()=0;
   virtual bool runCmd()=0;
   virtual bool waitCmd()=0;
   virtual bool abortCmd()=0;
};//class, Positioner

#endif //POSITIONER_HPP

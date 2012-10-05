#ifndef POSITIONER_HPP
#define POSITIONER_HPP

#include <vector>
#include <string>
using std::vector;
using std::string;

//this is the base class of the specific positioners' classes

class Positioner {
protected:
   struct Movement {
      double from, to, //in um
         duration;     //in us
      int trigger;
      Movement(double from, double to, double duration, int trigger){
         this->from=from; this->to=to; this->duration=duration; this->trigger=trigger;
      }
      virtual ~Movement(){}
   };
   vector<Movement* > movements;
   string lastErrorMsg;
   int dim; //dim that minPos, maxPos, curPos, moveTo, addMovement work on. 0 is x-axis.

public:
   Positioner(){setDim(0);}
   virtual ~Positioner(){ clearCmd(); }

   virtual string getLastErrorMsg(){return lastErrorMsg;}

   virtual int getDim(){return dim;}               //get cur dimension
   virtual bool setDim(int d){dim=d; return true;} //this will affect the dimension next 5 methods work on.

   virtual double minPos()=0; // the min value of the position
   virtual double maxPos()=0; // the max value of the position. NOTE: the unit is macro
   virtual bool curPos(double* pos)=0; // current position in um
   virtual bool moveTo(double to)=0; //move as quick as possible

   //// cmd related:
   virtual bool addMovement(double from, double to, double duration, int trigger); 
      //from:
      //to:
      //duration: 0 means asap;
      //trigger: 1 on, 0 off, otherwise no change
   virtual void clearCmd(); // clear the movement sequence
   //todo: for now, testCmd() and optimizeCmd() are not pure virtual so compiler is happy
   //   but they should be pure virtual once the changes to subclasses are done
   virtual bool testCmd(){return true;}  // check if the movement sequence is valid (e.g., the timing)

   virtual bool prepareCmd()=0; // after prepare, runCmd() will move it in real
   virtual void optimizeCmd(){} // reduce the delay between runCmd() and the time when the positioner reaches the start position
   virtual bool runCmd()=0;  //NOTE: this is repeatable (i.e. once prepared, you can run same cmd more than once)
   virtual bool waitCmd()=0; // wait forever until the movement sequence finishes
   virtual bool abortCmd()=0;// stop it as soon as possible
   virtual void setScanType(const bool b);
   virtual bool getScanType();
};//class, Positioner

#endif //POSITIONER_HPP

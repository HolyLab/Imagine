#ifndef DUMMYPIEZO_HPP
#define DUMMYPIEZO_HPP

#include "positioner.hpp"

class DummyPiezo: public Positioner {
protected:
    vector<Movement* > movements;

public:
    DummyPiezo(QString ctrlrsetup){
        posType = DummyPositioner;
        setuptype = ctrlrsetup;
    }
   ~DummyPiezo(){}

   double minPos(){return 0;}
   double maxPos(){return 400; }
   double maxSpeed() { return 2000; }
   QString getCtrlrSetupType() { return setuptype; }
   bool curPos(double* pos){*pos=500; return true;}
   bool moveTo(double to){ return true;}

   bool addMovement(double from, double to, double duration, int trigger)
   {
       if ((_isnan(from) && _isnan(to)) || (to >= minPos() && to <= maxPos() && from >= minPos() && from <= maxPos())) {
           movements.push_back(new Movement(from, to, duration, trigger));
           return true;
       }
       else {
           lastErrorMsg = "addMovment: out of range";
           return false;
       }
   }

   bool prepareCmd(bool useTrig){ return true;}
   bool runCmd(){ return true;}
   bool waitCmd(){ return true;}
   bool abortCmd(){ return true;}

private:
    QString setuptype;

};//class, DummyPiezo

#endif //DUMMYPIEZO_HPP

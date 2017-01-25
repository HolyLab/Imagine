#ifndef DUMMYPIEZO_HPP
#define DUMMYPIEZO_HPP

#include "positioner.hpp"

class DummyPiezo: public Positioner {

public:
    DummyPiezo(QString ctrlrsetup){
        posType = DummyPositioner;
        setuptype = ctrlrsetup;
    }
   ~DummyPiezo(){}

   double minPos(){return 0;}
   double maxPos(){return 1000; }
   double maxSpeed() { return 4000; }
   QString getCtrlrSetupType() { return setuptype; }
   bool curPos(double* pos){*pos=500; return true;}
   bool moveTo(double to){ return true;}

   bool addMovement(double from, double to, double duration, int trigger){return true;}

   bool prepareCmd(){ return true;}
   bool runCmd(){ return true;}
   bool waitCmd(){ return true;}
   bool abortCmd(){ return true;}

private:
    QString setuptype;

};//class, DummyPiezo

#endif //DUMMYPIEZO_HPP

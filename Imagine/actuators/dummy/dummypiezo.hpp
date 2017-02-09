#ifndef DUMMYPIEZO_HPP
#define DUMMYPIEZO_HPP

#include "positioner.hpp"
#include "curvedata.h"

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

   bool prepareCmd(bool useTrig);// { return true; }
   double zpos2voltage(double um);
   bool runCmd(){ return true;}
   bool waitCmd(){ return true;}
   bool abortCmd(){ return true;}

private:
    QString setuptype;

};//class, DummyPiezo


int toDigUnit(double phyValue) {
    return (phyValue - (-10)) / (10 - (-10))
        *(0x00007fff - 0xffff8000) + 0xffff8000;
}//toDigUnit()

double DummyPiezo::zpos2voltage(double um)
{
    return um / this->maxPos() * 10; // *10: 0-10vol range for piezo
}


bool DummyPiezo::prepareCmd(bool useTrig)
{
    int aoChannelForPiezo = 0; //TODO: put this configurable
    vector<int> aoChannels;
    aoChannels.push_back(aoChannelForPiezo);
    int aoChannelForTrigger = 1;
    aoChannels.push_back(aoChannelForTrigger);

    double totalTime = 0;
    for (unsigned idx = 0; idx < movements.size(); ++idx) totalTime += movements[idx]->duration / 1e6; //microsec to sec

    int scanRateAo = 10000; //TODO: hard coded

    int nScans = scanRateAo*totalTime;
    double piezoStartPos, piezoStopPos, preStop;
    unsigned int * bufAo = new unsigned int[nScans*aoChannels.size()];
    unsigned int * buf = bufAo - 1;

    for (unsigned idx = 0; idx < movements.size(); ++idx) {
        const Movement& m = *movements[idx];
        if (_isnan(m.from)) piezoStartPos = preStop;  // ??? preStop value is not initialized yet
        else piezoStartPos = zpos2voltage(m.from);
        if (_isnan(m.to)) piezoStopPos = piezoStartPos;
        else piezoStopPos = zpos2voltage(m.to);

        preStop = piezoStopPos;

        //get buffer address and generate waveform:
        int aoStartValue = toDigUnit(piezoStartPos);
        int aoStopValue = toDigUnit(piezoStopPos);

        int nScansNow = int(scanRateAo*m.duration / 1e6);
        if (idx == movements.size() - 1) {
            nScansNow = nScans - (buf - bufAo + 1);
        }//if, need roundup correction
        for (int i = 0; i < nScansNow; i++) {
            *++buf = unsigned int(double(i) / nScansNow*(aoStopValue - aoStartValue) + aoStartValue);
        }
        if (nScansNow>0 || (nScansNow == 0 && idx != 0)) {//todo: if nScansNow==0 && idx==0, do this at end of for loop
            *buf = aoStopValue; //precisely set the last sample; or if move in 0 sec, rewrite last move's last sample
        }
    }//for, each movement

     /// now the trigger
    int aoTTLHigh = toDigUnit(5.0);
    int aoTTLLow = toDigUnit(0);
    int test[1000] = { 0, }, mm = 0, mmm;
    bufAo += nScans;
    buf = bufAo;
    for (int i = 0; i < nScans; i++) {
        bufAo[i] = aoTTLLow;
    }
    for (unsigned idx = 0; idx < movements.size(); ++idx) {
        const Movement& m = *movements[idx];
        int nScansNow = int(scanRateAo*m.duration / 1e6);

        // Some cameras trigger on a pulse, others use an "enable" gate
        // that requires TTL high throughout the whole acquisition period.
        // For generality, implement the latter behavior.
        if (m.trigger == 1) {
            //for (int k = 0; k < nScansNow - 1; k++) {  // set low on very last sample
            mmm = 0;
            for (int k = 0; k < nScansNow; k++) {
                buf[k] = aoTTLHigh;
                if ((mm + mmm >= 4900) && (mm + mmm<5900))
                    test[mm + mmm - 4900] = 1;
                mmm++;
            }
        }
        else {
            mm = mm;
        }
        buf += nScansNow;
        mm += nScansNow;
    }
    return true;
}

#endif //DUMMYPIEZO_HPP

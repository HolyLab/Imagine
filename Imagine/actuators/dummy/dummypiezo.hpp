#ifndef DUMMYPIEZO_HPP
#define DUMMYPIEZO_HPP

#include "positioner.hpp"
#include "curvedata.h"

class DummyPiezo: public Positioner {
public:
    DummyPiezo(int maxposition, int maxspeed, QString ctrlrsetup){
        maxpos = maxposition;
        maxspd = maxspeed;
        posType = DummyPositioner;
        setuptype = ctrlrsetup;
    }
   ~DummyPiezo(){}

   int minPos(){return 0;}
   int maxPos(){return maxpos; }
   int maxSpeed() { return maxspd; }
   QString getCtrlrSetupType() { return setuptype; }
   bool curPos(double* pos){*pos=500; return true;}
   bool moveTo(double to);// { return true; }

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
   bool prepareCmdBuffered(ControlWaveform *conWaveData) { return true; }
   double zpos2voltage(double um);
   bool runCmd(){ return true;}
   bool waitCmd(){ return true;}
   bool abortCmd(){ return true;}
   string getSyncOut() { return "";};
   string getClkOut() { return ""; };
   bool resetDAQ() { return true; }

private:
    int maxpos = 0;
    int maxspd = 0;
    QString setuptype;
    void cleanup() { return; }

};//class, DummyPiezo


int toDigUnit(double phyValue) {
    return (phyValue - (-10)) / (10 - (-10))
        *(0x00007fff - 0xffff8000) + 0xffff8000;
}//toDigUnit()

double DummyPiezo::zpos2voltage(double um)
{
    return um / this->maxPos() * 10; // *10: 0-10vol range for piezo
}

bool DummyPiezo::moveTo(double to)
{
    double cur_pos = 0;
    double duration;
    curPos(&cur_pos);
    duration = (abs(to - cur_pos) / maxSpeed()) * 1e6;
    vector<Movement* > orig_movements = movements;
    vector<Movement* > empty_movements;
    movements = empty_movements;  //a hack, shouldn't have to hide prepared movements

    addMovement(max(cur_pos, 0), to, duration, -1);
    prepareCmd(false);
    runCmd();
    Sleep(200); //otherwise it seems to be cleared before nidaq can use it.
    clearCmd();
    cleanup();
    movements = orig_movements;
    prepareCmd(false); //prepare again, leaving the movement command list as we found it.

    return true;
}//moveTo(),

bool DummyPiezo::prepareCmd(bool useTrig)
{
//    return true; // block the below code

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
    buf = bufAo + nScans;
    for (int i = 0; i < nScans; i++) {
        buf[i] = aoTTLLow;
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

    delete bufAo; // This is just dummy piezo. So, delete bufAo.
    return true;
}


class DummyDigitalOut : public DigitalControls {

public:
    bool prepareCmdBuffered(ControlWaveform *conWaveData, string clkName) { return true; }
    bool runCmd() { return true; }
    bool waitCmd() { return true; }
    bool abortCmd() { return true; }
    bool singleOut(int lineIndex, bool newValue) { return true; }
    bool clearAllOutput() { return true; }
    bool resetDAQ() { return true; }

private:
    void cleanup() { return; }

};//class, DummyDigitalOut

#endif //DUMMYPIEZO_HPP

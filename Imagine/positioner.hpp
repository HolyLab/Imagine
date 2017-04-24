#ifndef POSITIONER_HPP
#define POSITIONER_HPP

#include <vector>
#include <string>
#include <QString>
#include "curvedata.h"
#include "waveform.h"

using std::vector;
using std::string;

class DataAcqThread;

// enum for the different types of positioners
typedef enum e_PositionerType {
    NullPositioner,
    DummyPositioner,
    VolPiezoPositioner,
    PiezoControlPositioner,
    ActuatorPositioner
} PositionerType;

//this is the base class of the specific positioners' classes
class Positioner {
protected:
    struct Movement {
        double from, to, //in um
            duration;     //in us
        int trigger;
        Movement(double from, double to, double duration, int trigger){
            this->from = from; this->to = to; this->duration = duration; this->trigger = trigger;
        }
        virtual ~Movement(){}
    };
    vector<Movement* > movements;
    string lastErrorMsg;
    int dim, scanRateAo; //dim that minPos, maxPos, curPos, moveTo, addMovement work on. 0 is x-axis, 1 is y-axis.

public:
    Positioner(){ setDim(0); }
    virtual ~Positioner(){ clearCmd(); }

	PositionerType posType = VolPiezoPositioner; // NullPositioner;
    DataAcqThread *parentAcqThread = nullptr;

    virtual string getLastErrorMsg(){ return lastErrorMsg; }

    virtual int getDim(){ return dim; }               //get cur dimension
    virtual bool setDim(int d){ dim = d; return true; } //this will affect the dimension next 5 methods work on.

    virtual double minPos() = 0; // the min value of the position
    virtual double maxPos() = 0; // the max value of the position. NOTE: the unit is macro
	virtual double maxSpeed() = 0; // maximum speed to allow the piezo to travel, in um per second
    virtual QString getCtrlrSetupType() { return 0; }
    virtual bool curPos(double* pos) = 0; // current position in um
    virtual bool moveTo(double to) = 0; //move at maximum safe speed
    virtual void setScanRateAo(int rate) { scanRateAo = rate; }

    //// cmd related:
    virtual bool addMovement(double from, double to, double duration, int trigger);
    //from:
    //to:
    //duration: 0 means asap;
    //trigger: 1 on, 0 off, otherwise no change
    virtual void clearCmd(); // clear the movement sequence
    //todo: for now, testCmd() and optimizeCmd() are not pure virtual so compiler is happy
    //   but they should be pure virtual once the changes to subclasses are done
    virtual bool testCmd(){ return true; }  // check if the movement sequence is valid (e.g., the timing)

    virtual bool prepareCmd(bool useTrigger) = 0; // after prepare, runCmd() will move it in real
    virtual bool prepareCmdBuffered(ControlWaveform *waveData) = 0;
    virtual void optimizeCmd(){} // reduce the delay between runCmd() and the time when the positioner reaches the start position
    virtual bool runCmd() = 0;  //NOTE: this is repeatable (i.e. once prepared, you can run same cmd more than once)
    virtual bool waitCmd() = 0; // wait forever until the movement sequence finishes
    virtual bool abortCmd() = 0;// stop it as soon as possible
    virtual void setScanType(const bool b) {};
    virtual bool getScanType() { return false; }
    virtual void setPCount() {};
    virtual string getSyncOut() = 0;
    virtual string getClkOut() = 0;
};//class, Positioner

class DigitalControls { // camera shutter, laser on/off, stimuli on/off
protected:
    string lastErrorMsg;
    int scanRateDo;

public:
    DigitalControls() {}
    virtual ~DigitalControls() {}

    virtual string getLastErrorMsg() { return lastErrorMsg; }
    virtual bool prepareCmdBuffered(ControlWaveform *waveData, string clkName) = 0;
    virtual bool runCmd() = 0;
    virtual bool waitCmd() = 0;
    virtual bool abortCmd() = 0;
    virtual bool singleOut(int lineIndex, bool newValue) = 0;

};//class, DigitalControls
#endif //POSITIONER_HPP

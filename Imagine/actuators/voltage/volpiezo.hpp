#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

#include "ni_daq_g.hpp"
#include "curvedata.h"
#include "waveform.h"

class VolPiezo : public Positioner {
    string clkName;// use AO as trigger for DO and AI
public:
    VolPiezo(QString aistring, QString aostring, int maxposition, int maxspeed, QString ctrlrsetup){
        ainame = aistring;
        aoname = aostring;
        aoOnce = nullptr;
        ao = nullptr;
        maxpos = maxposition;
        maxspd = maxspeed;
        posType = VolPiezoPositioner;
        setuptype = ctrlrsetup; // piezo controller setup method (none, COM port, ...)
    }
    ~VolPiezo() { delete aoOnce; delete ao; }

	//TODO: make these configurable elsewhere.  may be better to subclass VolPiezo for individual positioner models
    int minPos(){ return 0; }
    int maxPos(){ return maxpos; }
    int maxSpeed(){ return maxspd; } //microns per second
    QString getCtrlrSetupType() { return setuptype; }
    bool curPos(double* pos);
    bool moveTo(double to);

    bool addMovement(double from, double to, double duration, int trigger);

    bool prepareCmd(bool useTrigger);
    bool prepareCmdBuffered(ControlWaveform *conWaveData);
    bool runCmd();
    bool waitCmd();
    bool abortCmd();
    string getSyncOut();
    string getClkOut();
    int readConWaveToBuffer(int num);
    void writeNextSamples(void);
    static void CallbackWrapper(void* context) {
        static_cast<VolPiezo*>(context)->writeNextSamples();
    }

private:
    QString ainame;
    QString aoname;
    int numAOChannel;
    NiDaqAoWriteOne * aoOnce;
    NiDaqAo* ao;
    int maxpos = 0;
    int maxspd = 0;
    QString setuptype;
    int idx;
    int blockSize;
    ControlWaveform *conWaveData = NULL;
    int totalSample;

    double zpos2voltage(double um);
    void cleanup();
};//class, VolPiezo

/*
typedef enum  {
    doChannelForcamera1 = 5,
    doChannelForcamera2 = 6,
    doChannelForlaser1 = 4,
    doChannelForlaser2 = 8,
    doChannelForlaser3 = 9,
    doChannelForlaser4 = 10,
    doChannelForlaser5 = 11,
    doChannelForStimulus1 = 0,// NI DAQ Digital port P0.0
    doChannelForStimulus2 = 1,
    doChannelForStimulus3 = 2,
    doChannelForStimulus4 = 3,
    doChannelForStimulus5 = 7,
    doChannelForStimulus6 = 12,
    doChannelForStimulus7 = 13,
    doChannelForStimulus8 = 14,
    doChannelForStimulus9 = 15,// P0.15
}PortLayout;
*/

class DigitalOut : public DigitalControls {
public:
    DigitalOut(QString dostring) {
        doname = dostring;
        dout = new NiDaqDo(doname);
    }
    ~DigitalOut() { delete dout; dout = nullptr; }

    bool prepareCmdBuffered(ControlWaveform *conWaveData, string clkName);
    bool runCmd();
    bool waitCmd();
    bool abortCmd();
    bool singleOut(int lineIndex, bool newValue);
    int readConPulseToBuffer(int num);
    void writeNextSamples(void);
    static void CallbackWrapper(void* context) {
        static_cast<DigitalOut*>(context)->writeNextSamples();
    }
private:
    QString doname;
    NiDaqDo* dout;
    void cleanup();
    int idx;
    int blockSize;
    int totalSample;
    ControlWaveform *conWaveData = NULL;
};//class, DigitalBurstOut

#endif

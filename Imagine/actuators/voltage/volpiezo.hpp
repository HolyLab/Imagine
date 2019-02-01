#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

#include "ni_daq_g.hpp"
#include "curvedata.h"
#include "waveform.h"

class VolPiezo : public Positioner {
    string clkName;// use AO as trigger for DO and AI
public:
    VolPiezo(QString aistring, QString aostring, int maxposition, int maxspeed, double fres, QString ctrlrsetup){
        ainame = aistring;
        aoname = aostring;
        aoOnce = nullptr;
        ao = nullptr;
        maxpos = maxposition;
        maxspd = maxspeed;
        resonanceFreq = fres;
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
    void writeCompleted(void);
    bool resetDAQ();
    QTime getDAQEndTime(void);
    QTime getDAQStartTime(void);
    static void CallbackWrapper(void* context) {
        static_cast<VolPiezo*>(context)->writeNextSamples();
    }
    static void taskDoneCallbackWrapper(void* context) {
        static_cast<VolPiezo*>(context)->writeCompleted();
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
    SampleIdx idx;
    int blockSize;
    ControlWaveform *conWaveData = NULL;
    SampleIdx totalSample;
    QTime daqStartTime; // to calculate overhead time around DAQ task
    QTime daqDoneTime; // to calculate overhead time after DAQ task is done
    double resonanceFreq; // resonsance frequency

    double zpos2voltage(double um);
    void cleanup();
};//class, VolPiezo

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
    bool clearAllOutput();
    int readConPulseToBuffer(int num);
    void writeNextSamples(void);
    bool resetDAQ();
    static void CallbackWrapper(void* context) {
        static_cast<DigitalOut*>(context)->writeNextSamples();
    }
private:
    QString doname;
    NiDaqDo* dout;
    void cleanup();
    SampleIdx idx;
    int blockSize;
    SampleIdx totalSample;
    ControlWaveform *conWaveData = NULL;
    uInt32 laserTTLSig;
};//class, DigitalBurstOut

#endif

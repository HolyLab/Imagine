#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

#include "ni_daq_g.hpp"
#include "curvedata.h"
#include "waveform.h"

struct WaveData;

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
    double minPos(){ return 0; }
    double maxPos(){ return maxpos; }
    double maxSpeed(){ return maxspd; } //microns per second
    QString getCtrlrSetupType() { return setuptype; }
    bool curPos(double* pos);
    bool moveTo(double to);

    bool addMovement(double from, double to, double duration, int trigger);

    bool prepareCmd(bool useTrigger);
    bool prepareCmd(WaveData *waveData);
    bool prepareCmdBuffered(WaveData *waveData);
    bool prepareCmdBuffered(ControlWaveform *conWaveData);
    bool runCmd();
    bool waitCmd();
    bool abortCmd();
    string getSyncOut();
    string getClkOut();
    int readWaveToBuffer(int num);
    int readConWaveToBuffer(int num);
    void writeNextSamples(void);
    static void CallbackWrapper(void* context) {
        static_cast<VolPiezo*>(context)->writeNextSamples();
    }

private:
    QString ainame;
    QString aoname;
    NiDaqAoWriteOne * aoOnce;
    NiDaqAo* ao;
    int maxpos = 0;
    int maxspd = 0;
    QString setuptype;
    int idx;
    int blockSize;
    WaveData *waveData;
    ControlWaveform *conWaveData = NULL;
    int totalSample;

    double zpos2voltage(double um);
    void cleanup();
};//class, VolPiezo

typedef enum  {
    // 0~3:stimuli, 4:camera1, 5:camera2, 6:laser1, 7:laser2
    doChannelForcamera1 = 5,//TODO: put this configurable
    doChannelForcamera2 = 7,
    doChannelForlaser1 = 4,
    doChannelForlaser2 = 6,
    doChannelForStimulus1 = 0,
    doChannelForStimulus2 = 1,
    doChannelForStimulus3 = 2,
    doChannelForStimulus4 = 3
}PortLayout;

class DigitalOut : public DigitalControls {
public:
    DigitalOut(QString dostring) {
        doname = dostring;
        dout = new NiDaqDo(doname);
    }
    ~DigitalOut() { delete dout; dout = nullptr; }

    bool prepareCmd(WaveData *waveData, string clkName);
    bool prepareCmdBuffered(WaveData *waveData, string clkName);
    bool prepareCmdBuffered(ControlWaveform *conWaveData, string clkName);
    bool runCmd();
    bool waitCmd();
    bool abortCmd();
    bool singleOut(int lineIndex, bool newValue);
    int readPulseToBuffer(int num);
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
    WaveData *waveData;
    ControlWaveform *conWaveData = NULL;
};//class, DigitalBurstOut

#endif

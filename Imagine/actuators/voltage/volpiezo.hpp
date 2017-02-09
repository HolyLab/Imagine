#ifndef VOLPIEZO_HPP
#define VOLPIEZO_HPP

#include "positioner.hpp"

#include "ni_daq_g.hpp"
#include "curvedata.h"

class VolPiezo : public Positioner {

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
    ~VolPiezo(){ delete aoOnce; delete ao; }

	//TODO: make these configurable elsewhere.  may be better to subclass VolPiezo for individual positioner models
    double minPos(){ return 0; }
    double maxPos(){ return maxpos; }
    double maxSpeed(){ return maxspd; } //microns per second
    QString getCtrlrSetupType() { return setuptype; }
    bool curPos(double* pos);
    bool moveTo(double to);

    bool addMovement(double from, double to, double duration, int trigger);

    bool prepareCmd(bool useTrigger);
    bool runCmd();
    bool waitCmd();
    bool abortCmd();

private:
    QString ainame;
    QString aoname;
    NiDaqAoWriteOne * aoOnce;
    NiDaqAo* ao;
    int maxpos = 0;
    int maxspd = 0;
    QString setuptype;

    double zpos2voltage(double um);
    void cleanup();
};//class, VolPiezo

#endif

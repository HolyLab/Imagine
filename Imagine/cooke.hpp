#ifndef COOKE_HPP
#define COOKE_HPP

#include <windows.h>

#include <QMutex>

#include "camera_g.hpp"

#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
#include "PCO_err.h"
#include "timer_g.hpp"


// TODO: this is upside down, but cookeworkerthread.h is a mess...
// in the future it would be nice to make use of cookworkerthread.cpp,
// then forward declare this class in that one, rather than this.
//class SpoolThread;
class CookeWorkerThread;

class CookeCamera : public Camera {
public:
    friend class CookeWorkerThread;
    //friend class SpoolThread;

private:
    HANDLE hCamera;
    PCO_General strGeneral;
    PCO_CameraType strCamType;
    PCO_Sensor strSensor;
    PCO_Description strDescription;
    PCO_Timing strTiming;
    PCO_Storage strStorage;
    PCO_Recording strRecording;
    PCO_Image strImage;

    long firstFrameCounter; //first first frame's counter value
    long long totalGap;

    // nBufs is defined in cpp file - these arrays should have the same length as nBufs
    static const int nBufs;
    WORD mBufIndex[2]; //m_wBufferNr
    HANDLE mEvent[2];//m_hEvent
    DWORD driverStatus[2];
    char* mRingBuf[2]; //m_pic12

    //CookeWorkerThread* workerThread;

public:
    CookeCamera();

    ~CookeCamera();

    string getErrorMsg();

    int getExtraErrorCode(ExtraErrorCodeType type);

    int getROIStepsHor(void);

    bool init();
    bool fini();

    bool setAcqParams(int emGain,
        int preAmpGainIdx,
        int horShiftSpeedIdx,
        int verShiftSpeedIdx,
        int verClockVolAmp
        );

    //params different for live from for save mode
    bool setAcqModeAndTime(GenericAcqMode genericAcqMode,
        float exposure,
        int anFrames,  //used only in kinetic-series mode
        AcqTriggerMode acqTriggerMode,
        ExpTriggerMode expTriggerMode
        );

    bool prepCameraOnce();
    bool nextStack();

    double getCycleTime();

    long extractFrameCounter(PixelValue* rawData);

    void safe_pco(int errCode, string errMsg);
    void printPcoError(int errCode);
};//class, CookeCamera

#endif //COOKE_HPP



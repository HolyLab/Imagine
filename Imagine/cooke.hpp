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


class DummyCamera : public Camera {
public:
    friend class CameraWorkerThread;

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
    DummyCamera() { model = "dummy"; } // should be reimplemented? using CookCamera() and change vender name

    ~DummyCamera() { } // this is enough

    bool init() {
        return true;
    } // should be reimplemented
    bool fini() {
        return true;
    } // should be reimplemented

    bool setAcqParams(int emGain,  // should be reimplemented
        int preAmpGainIdx,
        int horShiftSpeedIdx,
        int verShiftSpeedIdx,
        int verClockVolAmp
    ) {
        return true;
    }

    //params different for live from for save mode
    bool setAcqModeAndTime(GenericAcqMode genericAcqMode,  // should be reimplemented
        float exposure,
        int anFrames,  //used only in kinetic-series mode
        AcqTriggerMode acqTriggerMode,
        ExpTriggerMode expTriggerMode
    ) {
        return true;
    }

    bool prepCameraOnce();
    bool nextStack() {
        return true;
    }  // should be reimplemented

    double getCycleTime() { return 1000; } // should be reimplemented

    void printPcoError(int errCode) {} // should be reimplemented
    bool stopAcqFinal();

    string getErrorMsg() { return "dummy getErrorMsg()"; };

    virtual int getErrorCode() {
        return -1;
    }

    virtual int getExtraErrorCode(ExtraErrorCodeType type) {
        return -1;
    }

    int getROIStepsHor(void) { return 160; };

};//class, DummyCamera

#endif //COOKE_HPP



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
    PCO_ImageTiming strImageTiming;

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


class DummyCameraThread : public WorkerThread {
    Q_OBJECT
private:
    int chipWidth;
    int chipHeight;
    int bytesPerPixel = 2;
    int framesPerStack;
    long long dummyImgDataSize;
    long imageSizePixels;
    long imageSizeBytes;
    char *dummyImg = nullptr;
    char *buff[2];
    HANDLE hEvent[2];
    DWORD *pStatus[2];
    long long dummyImgIdx;
    long frameNum;
    char *test;
    volatile int put_idx, get_idx;
    volatile bool recording;
    volatile bool cancel;
    QMutex mutex;

public:
    DummyCameraThread(QThread::Priority defaultPriority, QString fn, QObject *parent = 0);
    ~DummyCameraThread();
    unsigned long extractFrameCounter(unsigned short* rawData);
    bool getImageDataSize(QString filename);
    void SetRecordingState(bool bValue);
    void CancelImages();
    void AddBufferExtern(HANDLE evnt, char* ringBuf, long size, DWORD *status);
    void resetDummyCamera(void);
    bool getRecording(void) { return recording;}
    void run(void);

    int getChipHeight(void) { return chipHeight;}
    int getChipWidth(void) { return chipWidth;}
    int getBytesPerPixel(void) { return bytesPerPixel;}
    bool getCancel(void) { return cancel;}
    int getPut_idx(void) { return put_idx;}
    int getGet_idx(void) { return get_idx;}

 };//class, DummyCameraThread


class DummyCamera : public Camera {
public:
    friend class DummyWorkerThread;
    friend class DummyCameraThread;

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

    int bufferIdx;
    DummyCameraThread *sudoCamera = nullptr;
    float cycleTime;

public:
    DummyCamera(QString filename) {
        vendor = "dummy";
        sudoCamera = new DummyCameraThread(QThread::TimeCriticalPriority, filename);
    } // should be reimplemented? using CookCamera() and change vender name

    ~DummyCamera() {
        if (sudoCamera) {
            sudoCamera->quit();
            sudoCamera->wait();
            delete sudoCamera;
            sudoCamera = nullptr;
        }
    }

    bool init();
    bool fini() {
        return true;
    }

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
        this->genericAcqMode = genericAcqMode;
        this->cycleTime = exposure;
        this->nFrames = anFrames;
        this->acqTriggerMode = acqTriggerMode;
        this->expTriggerMode = expTriggerMode;
        return true;
    }

    bool prepCameraOnce();
    bool nextStack();

    double getCycleTime() { return cycleTime; }

    void printPcoError(int errCode) {} // should be reimplemented

    string getErrorMsg() { return "dummy getErrorMsg()"; };

    virtual int getErrorCode() {
        return -1;
    }

    virtual int getExtraErrorCode(ExtraErrorCodeType type) {
        return -1;
    }

    int getROIStepsHor(void) { return 160; };
    long extractFrameCounter(PixelValue* rawData);

};//class, DummyCamera

#endif //COOKE_HPP



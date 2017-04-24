#include "cooke.hpp"
#include "cookeworkerthread.h"
#include <assert.h>

#include "SC2_SDKAddendum.h"

#include "pco_errt.h"
#include <iostream>
#include <QtDebug>
#include "data_acq_thread.hpp"
#include "imagine.h"
#include "spoolthread.h"
#include "fast_ofstream.hpp"

#define PCO_ERRT_H_CREATE_OBJECT

int const COOKE_EXCEPTION = 47;
const int CookeCamera::nBufs = 2;

CookeCamera::CookeCamera() : Camera()
{

    firstFrameCounter = -1;
    totalGap = 0;

    bytesPerPixel = 2; //TODO: hardcoded
    vendor = "cooke";

    strGeneral.wSize = sizeof(strGeneral);// initialize all structure size members
    strGeneral.strCamType.wSize = sizeof(strGeneral.strCamType);
    strCamType.wSize = sizeof(strCamType);
    strSensor.wSize = sizeof(strSensor);
    strSensor.strDescription.wSize = sizeof(strSensor.strDescription);
    strDescription.wSize = sizeof(strDescription);
    strTiming.wSize = sizeof(strTiming);
    strStorage.wSize = sizeof(strStorage);
    strRecording.wSize = sizeof(strRecording);
    strImage.wSize = sizeof(strImage);
    strImage.strSegment[0].wSize = sizeof(strImage.strSegment[0]);
    strImage.strSegment[1].wSize = sizeof(strImage.strSegment[0]);
    strImage.strSegment[2].wSize = sizeof(strImage.strSegment[0]);
    strImage.strSegment[3].wSize = sizeof(strImage.strSegment[0]);

}

CookeCamera::~CookeCamera() {}

string CookeCamera::getErrorMsg() {
    if (errorCode == PCO_NOERROR) {
        return "no error";
    }
    else return errorMsg;
}

int CookeCamera::getExtraErrorCode(ExtraErrorCodeType type) {
    switch (type) {
    case eOutOfMem: return PCO_ERROR_NOMEMORY; //NOTE: this might be an issue
                                               //default: 
    }
    return -1;
}

int CookeCamera::getROIStepsHor(void) {
    return strDescription.wRoiHorStepsDESC;
};

bool CookeCamera::init()
{

    errorMsg = "Camera Initialization failed when: ";

    errorCode = PCO_OpenCamera(&hCamera, 0);

    if (errorCode != PCO_NOERROR){
        char msg[16384];
        PCO_GetErrorText(errorCode, msg, 16384);
        cout << msg << endl;
        errorMsg += "call PCO_OpenCamera";
        return false;
    }

    errorCode = PCO_GetGeneral(hCamera, &strGeneral);// Get infos from camera
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetGeneral";
        return false;
    }

    errorCode = PCO_GetCameraType(hCamera, &strCamType);
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetCameraType";
        return false;
    }

    errorCode = PCO_GetSensorStruct(hCamera, &strSensor);
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetSensorStruct";
        return false;
    }

    errorCode = PCO_GetCameraDescription(hCamera, &strDescription);
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetCameraDescription";
        return false;
    }

    errorCode = PCO_GetTimingStruct(hCamera, &strTiming);
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetTimingStruct";
        return false;
    }

    errorCode = PCO_GetRecordingStruct(hCamera, &strRecording);
    if (errorCode != PCO_NOERROR)
    {
        errorMsg += "PCO_GetRecordingStruct";
        return false;
    }

    //sensor width/height
    this->chipHeight = strSensor.strDescription.wMaxVertResStdDESC;
    this->chipWidth = strSensor.strDescription.wMaxHorzResStdDESC;

    this->bytesPerPixel = 2;
    this->imageSizePixels = chipHeight * chipWidth;
    this->imageSizeBytes = imageSizePixels*bytesPerPixel;

    //model
    this->model = strCamType.strHardwareVersion.Board[0].szName;

    allocBlackImage();
    allocMemPool(-1);

    //these get initialized and deleted by prepCameraOnce() and stopCameraFinal()
    this->circBuf = nullptr;
    this->circBufLock = nullptr;

    return true;
}//init(),

bool CookeCamera::fini()
{
    PCO_CloseCamera(hCamera);

    return true;
}//fini(),

bool CookeCamera::setAcqParams(int emGain,
    int preAmpGainIdx,
    int horShiftSpeedIdx,
    int verShiftSpeedIdx,
    int verClockVolAmp
    )
{
    // failure of an safe_pco() call will raise an exception, so we can try/catch for cleanliness
    try {
        // get camera description
        PCO_Description pcDesc;
        pcDesc.wSize = (ushort)sizeof(PCO_Description);
        safe_pco(PCO_GetCameraDescription(hCamera, &pcDesc), "failed to get camera description");
        // for now, we'll just always use the fastest available pixel rate
        DWORD maxPixRate = 0;
        for (int i = 0; i < sizeof(pcDesc.dwPixelRateDESC) / sizeof(DWORD); i++) {
            if (pcDesc.dwPixelRateDESC[i] > maxPixRate) maxPixRate = pcDesc.dwPixelRateDESC[i];
        }
        if (maxPixRate) {
            qDebug() << QString("setting pixel rate to: %1 Hz").arg(maxPixRate);
        }
        else {
            errorMsg = "could not find non-zero pixel rate";
            throw COOKE_EXCEPTION;
        }

        // stop recording - necessary for changing camera settings
        safe_pco(PCO_SetRecordingState(hCamera, 0), "failed to stop camera");
        // start with default settings
        safe_pco(PCO_ResetSettingsToDefault(hCamera), "failed to reset camera settings");
        // skip SetBinning... not applicable to cooke cameras
        // set roi
        if(isUsingSoftROI)
            safe_pco(PCO_EnableSoftROI(hCamera, 1, NULL, 0), "failed to set SoftROI enable");
        else
            safe_pco(PCO_EnableSoftROI(hCamera, 0, NULL, 0), "failed to set SoftROI disable");
        safe_pco(PCO_SetROI(hCamera, hstart, vstart, hend, vend), "failed to set ROI");
        // frame trigger mode (0 for auto)
        safe_pco(PCO_SetTriggerMode(hCamera, 0), "failed to set frame trigger mode");
        // storage mode (1 for FIFO)
        safe_pco(PCO_SetStorageMode(hCamera, 1), "failed to set storage mode");
        // acquisition mode (0 for auto)
        safe_pco(PCO_SetAcquireMode(hCamera, 0), "failed to set acquisition mode");
        // timestamp mode (1: bcd; 2: bcd+ascii)
        safe_pco(PCO_SetTimestampMode(hCamera, 1), "failed to set timestamp mode");
        // bit alignment (0: MSB aligned; 1: LSB aligned)
        safe_pco(PCO_SetBitAlignment(hCamera, 1), "failed to set bit alignment mode");
        // pixel rate (for now just use the fastest rate listed in the cam description)
        DWORD dwPixelRate = maxPixRate;
        safe_pco(PCO_SetPixelRate(hCamera, dwPixelRate), "failed to call PCO_SetPixelRate()");
        // arm the camera to apply the image size settings... needed for the config calls that follow
        safe_pco(PCO_ArmCamera(hCamera), "failed to arm the camera");
        // get set roi size, make sure it matches what we meant to set
        WORD wXResAct = -1, wYResAct = -1, wXResMax, wYResMax;
        safe_pco(PCO_GetSizes(hCamera, &wXResAct, &wYResAct, &wXResMax, &wYResMax), "failed to get roi size");
        assert(wXResAct == hend - hstart + 1 && wYResAct == vend - vstart + 1);
        // interface output format (interface 2 for DVI)
        // note that the wFormat passed here is copied from the code before I arrived... seems weird.
       // safe_pco(PCO_SetInterfaceOutputFormat(hCamera, 2, SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER, 0, 0),
       //     "failed to call PCO_SetInterfaceOutputFormat()");

        // auto-set the transfer params
        safe_pco(PCO_SetTransferParametersAuto(hCamera, NULL, 0), "failed to set transfer parameters");
        // TODO: explicitly set the transfer params here
        // TODO: explicitly set the active lookup table here
        // Do we really care to do the above, or are we okay as
        //   long as ROI and pix rate are what we want?

        // arm the camera to validate the settings
        safe_pco(PCO_ArmCamera(hCamera), "failed to arm the camera");
        // set the image buffer parameters
        safe_pco(PCO_CamLinkSetImageParameters(hCamera, wXResAct, wYResAct),
            "failed to call PCO_CamLinkSetImageParameters()");

        qDebug() << QString("Finished setting camera acq parameters");
    }
    catch (int e) {
        if (e != COOKE_EXCEPTION) throw;
        if (errorCode == PCO_NOERROR) errorCode = -1;
        return false;
    }

    /*
    So if you find yourself having problems with PCO_CamLinkSetImageParameters(), it
    might be because of a mismatch between your camera settings (specifically the pixel
    rate and ROI dimensions, see doco FMI), the image parameters set in
    PCO_SetTransferParameter(), and the active lookup table set in PCO_SetActiveLookupTable().
    One approach to debugging is to comment out the explicit transfer params and lookup calls
    and uncomment PCO_SetTransferParametersAuto, see what settings were applied, then plug
    those value back into the explicit methods (if they suit your needs). We don't just
    leave auto on because "auto" isn't necessarily garunteed to do the same thing.

    For reference, here are the values that auto-set chose last time we checked:
    clparams:
    baudrate: 38400
    ClockFrequency: 85000000
    CCline: 0
    DataFormat: 261
    Transmit: 1

    Lookuptable: 0 (disabled)
    Lookuptable params: 0 (disabled)
    */

    //clparams.DataFormat=PCO_CL_DATAFORMAT_5x12L | wFormat; // 12L requires lookup table 0x1612
    ////clparams.ClockFrequency = 85000000; // see SC2_SDKAddendum.h for valid values - NOT same as pixel freq.
    ////clparams.Transmit = 0; // single image transmission mode
    //errorCode=PCO_SetTransferParameter(hCamera, &clparams,sizeof(PCO_SC2_CL_TRANSFER_PARAM)); 
    //if(errorCode!=PCO_NOERROR) {
    //   errorMsg="failed to call PCO_SetTransferParameter()";
    //   return false;
    //}

    // set the active lookup table 
    //WORD wParameter=0;
    //WORD wIdentifier=0x1612; // 0x1612 for high speed, with x resolution > 1920
    //errorCode =PCO_SetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
    //if(errorCode!=PCO_NOERROR) {
    //   errorMsg="failed to call PCO_SetActiveLookupTable()";
    //   return false;
    //}

    ////////////////////////////////////////
    // EVERYTHING BELOW THIS POINT IS FOR DEBUGGING ONLY... COMMENT OUT FOR RELEASE BUILD.
    ////////////////////////////////////////
    //// take a look at the transfer parameters that got set...
    //PCO_SC2_CL_TRANSFER_PARAM clparams;
    //PCO_GetTransferParameter(hCamera, &clparams, sizeof(PCO_SC2_CL_TRANSFER_PARAM))
    //// take a look at the lookup table that got activated
    //WORD wIdentifier=-1, wParameter=-1;
    //PCO_GetActiveLookupTable(hCamera, &wIdentifier, &wParameter);
    //// take a look at the available lookup tables
    //WORD wFormat2;
    //errorCode = PCO_GetDoubleImageMode(hCamera, &wFormat2);
    //char Description[256]; BYTE bInputWidth, bOutputWidth;
    //WORD wNumberOfLuts, wDescLen;
    //PCO_GetLookupTableInfo(hCamera, 0, &wNumberOfLuts, 0, 0, 0, 0, 0, 0);
    //for (int i = 0; i < (int)wNumberOfLuts; ++i){
    //    PCO_GetLookupTableInfo(hCamera, i, &wNumberOfLuts, Description,
    //        256, &wIdentifier, &bInputWidth, &bOutputWidth, &wFormat2);
    //    qDebug() << QString("\nlut: %1\n\tdesc: %2\n\tid: %3").arg(i).arg(Description).arg(wIdentifier);
    //}

    return true;
}//setAcqParams(),

double CookeCamera::getCycleTime()
{
    DWORD sec, nsec;
    //TODO: this is probably inaccurate when using external exposure triggering (returns only exposure time?)
    errorCode = PCO_GetCOCRuntime(hCamera, &sec, &nsec);
    if (errorCode != PCO_NOERROR) {
        errorMsg = "failed to call PCO_GetCOCRuntime()";
        return -1;
    }

    return sec + nsec*1e-9;
}


//params different for live from for save mode
bool CookeCamera::setAcqModeAndTime(GenericAcqMode genericAcqMode,
    float exposure,
    int anFrames,  //used only in kinetic-series mode
    AcqTriggerMode acqTriggerMode,
	ExpTriggerMode expTriggerMode
    )
{
    this->genericAcqMode = genericAcqMode;

    this->nFrames = anFrames;
    this->acqTriggerMode = acqTriggerMode;
	this->expTriggerMode = expTriggerMode;

    errorCode = PCO_NOERROR;

    ///set acquisition trigger mode
	//Note that there are also additional modes accessible via PCO_SetAcquireModeEx.  They may require a firmware upgrade.
    if (acqTriggerMode == eInternalTrigger){
        errorCode = PCO_SetAcquireMode(hCamera, 0); //0: auto, all triggered images are read out
    }
	else if (acqTriggerMode == eExternal) {
		errorCode = PCO_SetAcquireMode(hCamera, 1); //1: acquisition only occurs for trigger signals received
													// while a HIGH signal is detected at SMA #2
													//The camera's frame counter may reset, not sure
	}
    if (errorCode != PCO_NOERROR) {
        errorMsg = "failed to set acquisition trigger mode";
        return false;
    }

	///set exposure trigger mode
	if (expTriggerMode == eAuto) {
		errorCode = PCO_SetTriggerMode(hCamera, 0); //0: auto, camera is in charge of timing
	}
	else if (expTriggerMode == eSoftwareTrigger) {
		errorCode = PCO_SetTriggerMode(hCamera, 1); //1: trigger only by a "force trigger" command, good for single images
	}
	else if (expTriggerMode == eExternalStart) {
		errorCode = PCO_SetTriggerMode(hCamera, 2); //2: external start via SMA input #1, uses pre-set exposure time
													//Busy status at SMA #3 determines if new trigger will be accepted
													//(HIGH signal means busy).  LOW is < 0.8V and HIGH is > 2.0V
	}
	else {
		errorCode = PCO_SetTriggerMode(hCamera, 3); //3: external exposure control, pulse length dictates exposure
													//HiGH means expose.  Busy status is available as above.
													//10s is maximum exposure time
	}
	if (errorCode != PCO_NOERROR) {
		errorMsg = "failed to set exposure trigger mode";
		return false;
	}


    ///exposure time
    errorCode = PCO_SetDelayExposureTime(hCamera, // Timebase: 0-ns; 1-us; 2-ms  
		(DWORD)(0),
		(DWORD)(exposure * 1000000),
        1,		// WORD wTimeBaseDelay,
        1);	// WORD wTimeBaseExposure
    if (errorCode != PCO_NOERROR) {
        errorMsg = "failed to set exposure time";
        return false;
    }

    ///arm the camera again
    errorCode = PCO_ArmCamera(hCamera);
    if (errorCode != PCO_NOERROR) {
        errorMsg = "failed to arm the camera";
        return false;
    }

    cout << "frame rate is: " << 1 / getCycleTime() << endl;

    return true;
}

bool CookeCamera::prepCameraOnce()
{
    int circBufCap = memPoolSize / imageSizeBytes;  //memPoolSize is set in spoolthread.h
    circBuf = new CircularBuf(circBufCap);
    circBufLock = new QMutex;
    nAcquiredStacks = 0;

    ///set pointers to slots in the ring buffer for data transfer from card to pc
    for (int i = 0; i < nBufs; ++i) {
        mBufIndex[i] = -1;
        mEvent[i] = CreateEvent(0, FALSE, FALSE, NULL);
        mRingBuf[i] = memPool + (circBuf->peekPut() + i) * size_t(imageSizeBytes);
    }
    SpoolThread * s = new SpoolThread(QThread::HighestPriority, ofsSpooling, this);
    setSpoolThread(s);
    s->start(QThread::HighestPriority);
    CookeWorkerThread * w = new CookeWorkerThread(QThread::TimeCriticalPriority, this);
    setWorkerThread(w);
    w->start(QThread::TimeCriticalPriority);
    return true;
}

bool CookeCamera::nextStack()
{
    bool workerPaused = workerThread->getIsPaused();
    while (!workerPaused && !stopRequested)
        QThread::msleep(5); //could hinder performance

    if (!stopRequested) {
        workerThread->setIsPaused(false);
        (workerThread->getUnPaused())->wakeAll();
        safe_pco(PCO_SetRecordingState(hCamera, 1), "failed to start camera recording"); //1: run
    }
    return true;
}//startAcq(),

long CookeCamera::extractFrameCounter(PixelValue* rawData)
{
    unsigned long result = 0;
    for (int i = 0; i < 4; ++i){
        unsigned hex = rawData[i];
        result = result * 100 + hex / 16 * 10 + hex % 16;
    }

    return result;
}

void CookeCamera::safe_pco(int errCode, string errMsg)
{
    // set errorMsg and throw an exception if you get a not-ok error code
    if (errCode != PCO_NOERROR && (errCode & PCO_ERROR_CODE_MASK) != 0) {
        printPcoError(errCode);
        errorCode = errCode;
        errorMsg = errMsg;
        throw COOKE_EXCEPTION; // could throw an exception class w/informative message... but meh.
    }
}

void CookeCamera::printPcoError(int errCode) {
    char msg[16384];
    PCO_GetErrorText(errCode, msg, 16384);
    std::string msgs = string(msg);
    std::wstring msgw;
    msgw.assign(msgs.begin(), msgs.end());
    cout << msg << endl;
    OutputDebugStringW((wstring(L"\nPCO error text: ") + msgw).c_str());
}

const int DummyCamera::nBufs = 2;
bool DummyCamera::prepCameraOnce()
{
    int circBufCap = memPoolSize / imageSizeBytes;  //memPoolSize is set in spoolthread.h
    circBuf = new CircularBuf(circBufCap);
    circBufLock = new QMutex;
    nAcquiredStacks = 0;

    ///set pointers to slots in the ring buffer for data transfer from card to pc
    for (int i = 0; i < nBufs; ++i) {
        mBufIndex[i] = -1;
        mEvent[i] = CreateEvent(0, FALSE, FALSE, NULL);
        mRingBuf[i] = memPool + (circBuf->peekPut() + i) * size_t(imageSizeBytes);
    }
    SpoolThread * s = new SpoolThread(QThread::HighestPriority, ofsSpooling, this);
    setSpoolThread(s);
    s->start(QThread::HighestPriority);
    CameraWorkerThread * w = new CameraWorkerThread(QThread::TimeCriticalPriority, this);
    setWorkerThread(w);
    w->start(QThread::TimeCriticalPriority);
    return true;
}

bool DummyCamera::stopAcqFinal()
{
    //not rely on the "wait abandon"!!!
    WorkerThread * w = getWorkerThread();
    SpoolThread * s = getSpoolThread();
    w->quit();
    s->requestStop();
    w->wait();
    s->wait();

    delete circBuf;
    delete getWorkerThread();
    delete getSpoolThread();
    spoolThread = nullptr;
    workerThread = nullptr;

    delete ofsSpooling; //the file is closed too
    ofsSpooling = nullptr;

    return true;
}

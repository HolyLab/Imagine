/*-------------------------------------------------------------------------
** Copyright (C) 2005-2012 Timothy E. Holy and Zhongsheng Guo
**    All rights reserved.
** Author: All code authored by Zhongsheng Guo.
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/

#ifndef CAMERA_G_HPP
#define CAMERA_G_HPP

#include <string>
#include <vector>
#include <utility>
#include <QtCore>
#include <atomic>
#include "circbuf.hpp"
#include "spoolthread.h"
#include "fast_ofstream.hpp"
#include "workerthread.h"
#include "timer_g.hpp"

using std::string;
using std::vector;
using std::pair;
using std::make_pair;

class  Camera {
public:
    typedef unsigned short PixelValue; //16-bit pixels for PCO camera. TODO: make generic
    enum ExtraErrorCodeType {
        eOutOfMem,
    };
    enum AcqTriggerMode { //see p. 117 of PCO sdk manual. (also, maybe we should move this to CookeCamera)
        eInternalTrigger = 0,
        eExternal = 6,
    };
	enum ExpTriggerMode {
		eAuto = 7,
		eSoftwareTrigger = 8, //an exposure can only be started by a force trigger command
		eExternalStart = 9,
		eExternalControl = 10,
        eFastExternalControl = 11
    };
    enum GenericAcqMode{
        eLive,
        eAcqAndSave,
    };

    GenericAcqMode  genericAcqMode;
    AcqTriggerMode  acqTriggerMode;
	ExpTriggerMode  expTriggerMode;
    string vendor;

    int  hbin, vbin, hstart, hend, vstart, vend; //image binning params. 1-based.
                                                 //for hend and vend: <0 means chip width (or height)

    int nStacks, nFramesPerStack, bytesPerPixel;
    long imageSizePixels, imageSizeBytes;
    bool isUsingSoftROI = false;
    bool stopRequested = false;

protected:
    string model;

    int cameraID;
    //camera specific error code and msg
    int	errorCode;    //these two are paired. For andor's, errorCode meanings are defined by 
    string errorMsg;   //   atmcd32d.h. 

    int  nFrames;

    int chipWidth = 0;       				// dims of
    int	chipHeight = 0;       				// CCD chip

    string spoolingFilename;
    //the buf for the live image
    //todo: we might also want it to be aligned
    QByteArray liveImage;
    //for streaming images to disk
    long long memPoolSize;
    //for speed reason: a black frame used for copying
    //todo: alignment
    PixelValue* pBlackImage;
    SpoolThread *spoolThread;
    FastOfstream *ofsSpooling;
    Timer_g * timer;
    WorkerThread *workerThread;

    std::atomic_int32_t nAcquiredFrames;
    std::atomic_int32_t nAcquiredStacks;

public:
    char * memPool;
    CircularBuf *circBuf;
    QMutex *circBufLock;

    Camera();
    virtual ~Camera();
    virtual WorkerThread* getWorkerThread();
    virtual SpoolThread* getSpoolThread();
    virtual int getNAcquiredFrames();
    virtual int getNAcquiredStacks();

    virtual void setWorkerThread(WorkerThread * w);
    virtual void setSpoolThread(SpoolThread * s);
    virtual void setNAcquiredFrames(int n);
    virtual void setNAcquiredStacks(int n);
    virtual void incrementNAcquiredFrames();
    virtual void incrementNAcquiredStacks();

    virtual bool allocMemPool(long long sz);
    virtual void freeMemPool();
    //allocate the black image array for missed frames
    virtual bool allocBlackImage();
    virtual bool updateLiveImage();
    virtual QByteArray &getLiveImage();
    //string getSpoolingFilename(){return spoolingFilename;}
    virtual bool setSpooling(string filename);
    //virtual void setTimer(Timer_g * timer);
    virtual void updateImageParams(int nstacks, int nframesperstack);
    virtual bool stopAcqFinal();


    virtual string getErrorMsg()=0;

    virtual int getErrorCode(){
        return errorCode;
    }

    virtual long getAcquiredFrameCount(); // #frames acquired for the current stack so far

    //virtual bool isIdle()=0;

    virtual bool prepCameraOnce() { return false; }
    virtual bool nextStack() { return false; }
    virtual bool init() { return false; }
    virtual bool fini() { return false; }

    virtual int getChipWidth(){
        return chipWidth;
    }

    virtual int getChipHeight(){
        return chipHeight;
    }

    //todo: to verify
    virtual int getImageWidth(){
        return (hend - hstart + 1) / hbin;
    }

    //todo: to verify
    virtual int getImageHeight(){
        return (vend - vstart + 1) / vbin;
    }

    virtual string getModel()
    {
        return model;
    }//getModel()

    virtual int getExtraErrorCode(ExtraErrorCodeType type)=0;


    //params common to both live- and save-modes
    ///NOTE: roi/binning are also set here
    virtual bool setAcqParams(int gain, //emGain for Andor
        int preAmpGainIdx,
        int horShiftSpeedIdx,
        int verShiftSpeedIdx,
        int verClockVolAmp
        ) = 0;

    //params different for live from for save mode
    virtual bool setAcqModeAndTime(GenericAcqMode genericAcqMode,
        float exposure,
        int anFrames,  //used only in kinetic-series mode
        AcqTriggerMode acqTriggerMode,
		ExpTriggerMode expTriggerMode
        ) = 0;


    virtual double getCycleTime()=0;

    virtual bool isSpooling() { return spoolingFilename != ""; }

    virtual int getROIStepsHor(void) = 0;

    virtual pair<int, int> getGainRange() { return make_pair(0, 0); }//by default, no gain
    virtual DWORD getCameraID() { return cameraID; }

};//class, Camera




#endif //CAMERA_G_HPP

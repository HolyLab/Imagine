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

using std::string;
using std::vector;
using std::pair;
using std::make_pair;

class DataAcqThread;

class Camera {
public:
    typedef unsigned short PixelValue;
    enum ExtraErrorCodeType {
        eOutOfMem,
    };
    enum AcqTriggerMode { //see p. 117 of PCO sdk manual
        eInternalTrigger = 0,
        eExternal = 6,
    };
	enum ExpTriggerMode {
		eAuto = 7,
		eSoftwareTrigger = 8, //an exposure can only be started by a force trigger command
		eExternalStart = 9,
		eExternalControl = 10
	};
    enum GenericAcqMode{
        eLive,
        eAcqAndSave,
    };

    GenericAcqMode  genericAcqMode;
    AcqTriggerMode     acqTriggerMode;
	ExpTriggerMode  expTriggerMode;
    string vendor;

    // dataAcqThread that owns this camera
    DataAcqThread *parentAcqThread = nullptr;

    int  hbin, vbin, hstart, hend, vstart, vend; //image binning params. 1-based.
    //for hend and vend: <0 means chip width (or height)

protected:
    string model;

    //camera specific error code and msg
    int	errorCode;    //these two are paired. For andor's, errorCode meanings are defined by 
    string errorMsg;   //   atmcd32d.h. 

    int  nFrames;

    int 	chipWidth;       				// dims of
    int	chipHeight;       				// CCD chip

    // image Buffers
    PixelValue * pImageArray;	 // main image buffer read from card
    int imageArraySize; //in pixel

    string spoolingFilename;

public:
    Camera();
    virtual ~Camera();

    virtual string getErrorMsg() = 0;

    int getErrorCode(){
        return errorCode;
    }

    virtual long getAcquiredFrameCount() = 0; // #frames acquired so far

    virtual bool getLatestLiveImage(PixelValue * frame) = 0;

    //virtual bool isIdle()=0;

    virtual bool startAcq() = 0;
    virtual bool stopAcq() = 0;

    PixelValue * getImageArray(){
        return pImageArray;
    }

    int getChipWidth(){
        return chipWidth;
    }

    int getChipHeight(){
        return chipHeight;
    }

    //todo: to verify
    int getImageWidth(){
        return (hend - hstart + 1) / hbin;
    }

    //todo: to verify
    int getImageHeight(){
        return (vend - vstart + 1) / vbin;
    }

    string getModel()
    {
        return model;
    }//getModel()

    virtual bool init() = 0;

    virtual bool fini() = 0;

    virtual int getExtraErrorCode(ExtraErrorCodeType type) = 0;

    //allocate the image array space
    bool allocImageArray(int nFrames, bool shouldReallocAnyway);

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


    virtual double getCycleTime() = 0;

    virtual bool transferData() = 0;

    bool isSpooling() { return spoolingFilename != ""; }
    //string getSpoolingFilename(){return spoolingFilename;}
    virtual bool setSpooling(string filename); //when filename is empty, disable the spooling

    virtual pair<int, int> getGainRange(){ return make_pair(0, 0); }//by default, no gain

private:
    void   freeImageArray();
};//class, Camera




#endif //CAMERA_G_HPP
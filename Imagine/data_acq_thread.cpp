/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
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

#include <QtGui>
#include <QApplication>
#include <QDir>

#include <math.h>
#include <fstream>
#include <vector>
#include <utility>
#include <memory>

using namespace std;

#include <QDateTime>
#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptable>
#include <QMetaType>

//#include "cooke.hpp"
#include "data_acq_thread.hpp"

//#include "andor_g.hpp"
#include "timer_g.hpp"
#include "ni_daq_g.hpp"
#include "ai_thread.hpp"
#include "fast_ofstream.hpp"
#include "positioner.hpp"
//#include "Piezo_Controller.hpp"
#include "imagine.h"
#include "curvedata.h"

QScriptEngine* se;
DigitalControls * digOut = nullptr;
QString daq;
string rig;
const string headerMagic = "IMAGINE";

#pragma region Lifecycle

DataAcqThread::DataAcqThread(QThread::Priority defaultPriority, Camera *cam, Positioner *pos, Imagine* parentImag, QObject *parent)
    : WorkerThread(defaultPriority, parent)
{
    restart = false;
    abort = false;
    isLive = true;
    parentImagine = (Imagine*)parentImag;
    setCamera(cam);
    // not that this was allocated with 'new', so you need to delete when you die
    pPositioner = pos;
}

DataAcqThread::~DataAcqThread()
{
	delete pCamera;
    if ((parentImagine->masterImagine) == NULL) delete pPositioner;
    wait();
}

#pragma endregion

#pragma region Utility

void DataAcqThread::genSquareSpike(int duration)
{
    digOut->singleOut(5, true);
    Sleep(duration);
    digOut->singleOut(5, false);
}

QString linize(QString lines)
{
    //join several lines into one line
    lines.replace("\n", "\\n");
    return lines;
}

#pragma endregion

#pragma region Camera

void DataAcqThread::setCamera(Camera *cam) {
    pCamera = cam;
}

#pragma endregion

#pragma region Positioner

void DataAcqThread::setPositioner(Positioner *pos) {
    // make sure that the camera knows who its parent is.
    pPositioner = pos;
    if (pos != NULL) pos->parentAcqThread = this;
}

bool DataAcqThread::preparePositioner(bool isForward, bool useTrigger)
{
    // if this thread has no positioner, it's always prepared
    if (pPositioner == NULL) {
        return true;
    }

    pPositioner->clearCmd();
    int oldAxis = pPositioner->getDim();

    //y axis. Todo: make it a param in cfg file
    if (pPositioner->posType == ActuatorPositioner) pPositioner->setDim(1);

    // the piezo code only supports the following three lines calling pattern
    if (isBiDirectionalImaging){
        pPositioner->setScanType(isBiDirectionalImaging);
        if (isForward){
            pPositioner->addMovement(piezoStartPosUm, piezoStopPosUm, nFramesPerStack*cycleTime*1e6, 1);
        }
        else {
            pPositioner->addMovement(piezoStopPosUm, piezoStartPosUm, nFramesPerStack*cycleTime*1e6, 1);
        }
        pPositioner->addMovement(numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN(), 0.05*1e6, -1); //no movement, no trigger change, just block 0.05s so camera can get its last frame
    }//if, record on both direction
    else {
        pPositioner->addMovement(piezoStartPosUm, piezoStopPosUm, nFramesPerStack*cycleTime*1e6, 1);
        pPositioner->addMovement(piezoStopPosUm, piezoStartPosUm, piezoTravelBackTime*1e6, -1); //move back directly without prepare
    }//else, uni-directional recording
    
    if (pPositioner->posType == ActuatorPositioner) pPositioner->setDim(oldAxis);

    if (pPositioner->testCmd())  pPositioner->prepareCmd(useTrigger);
    else return false;
    
    return true;

}

bool DataAcqThread::prepareDaqBuffered()
{
    string clkName;
    bool isOK;

    if (pPositioner == NULL) {
        return true;
    }
    // TODO: move positioner to the position of 1st element
    isOK = pPositioner->prepareCmdBuffered(conWaveData);
    if (isOK) {
        isOK = digOut->prepareCmdBuffered(conWaveData, pPositioner->getClkOut());
        if (isOK)
            return true;
        else
            return false;
    }
    else
        return false;
}

#pragma endregion

#pragma region Acquisition
void DataAcqThread::setIsUpdatingImage(bool value)
{
    mutex.lock();
    isUpdatingImage = value;
    mutex.unlock();
}

void DataAcqThread::startAcq()
{
    stopRequested = false;
    pCamera->stopRequested = false;
    this->start(getDefaultPriority());
}

void DataAcqThread::stopAcq()
{
    stopRequested = true;
    pCamera->stopRequested = true;
}

void DataAcqThread::run()
{
    if (isLive)
        run_live();
    else
        run_acq_and_save();

    //this is for update ui status only:
    emit newStatusMsgReady(QString("acq-thread-finish"));
}//run()

void DataAcqThread::run_live()
{
    Camera* camera = pCamera;

    //prepare for camera:
    bool suc = camera->setAcqModeAndTime(Camera::eLive,
        this->exposureTime,
        this->nFramesPerStack,
        Camera::eInternalTrigger,  //use internal trigger
		Camera::eAuto //use auto exposure triggering.  Could be nice to use the user-specified trigger mode instead.
        );
    emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
        .arg(camera->getErrorMsg().c_str()));
    if (!suc) return;

    isUpdatingImage = false;

    camera->prepCameraOnce();
    Timer_g gt;
    gt.start();

    camera->nextStack();

    emit newStatusMsgReady(QString("Camera: started acq: %1")
        .arg(camera->getErrorMsg().c_str()));

    long nDisplayUpdating = 0;
    long nFramesGot = 0;
    long nFramesGotCur = 0;
    while (!stopRequested){
        nFramesGotCur = camera->getAcquiredFrameCount();
        if (nFramesGotCur == nFramesGot){
            QThread::msleep(20);
            continue;
        }

        if (!isUpdatingImage){
            nFramesGot = nFramesGotCur;

            //get the last frame:
            if (!camera->updateLiveImage()){
                QThread::msleep(20);
                continue;
            }
            //incEmittedSignal();
            setIsUpdatingImage(true);
            emit imageDataReady(camera->getLiveImage(), nFramesGot - 1, camera->getImageWidth(), camera->getImageHeight(), 0); //-1: due to 0-based indexing
            nDisplayUpdating++;
            if (nDisplayUpdating % 10 == 0){
                emit newStatusMsgReady(QString("Screen update frame rate: %1")
                    .arg(nDisplayUpdating / gt.read(), 5, 'f', 2) //width=5, fixed point, 2 decimal digits
                    );
            }

            //spare some time for gui thread's event handling:
            QApplication::instance()->processEvents();
            double timeToWait = 0.05;
            QThread::msleep(timeToWait * 1000); // *1000: sec -> ms
        }
    }//while, user did not requested stop

    camera->stopAcqFinal();

    QString ttMsg = "Live mode is stopped";
    emit newStatusMsgReady(ttMsg);
}//run_live()

void DataAcqThread::run_acq_and_save()
{
    Camera* camera = pCamera;
    Timer_g gt;
    bool hasPos = pPositioner != NULL;
    bool useCam = true;
    if (this->nStacks == 0 && this->nStacksUser ==0) useCam = false;

    ///piezo feedback data file (only for PI piezo)
    string positionerFeedbackFile = replaceExtName(headerFilename, "pos").toStdString();
    ///prepare for camera:
    if (useCam) {
        camFilename = replaceExtName(headerFilename, "cam"); //NOTE: necessary if user overwrite files
        camera->setSpooling(camFilename.toStdString());

        camera->setAcqModeAndTime(Camera::eAcqAndSave,
            this->exposureTime,
            this->nFramesPerStack*this->nStacks,
            this->acqTriggerMode,
            this->expTriggerMode);
        emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
            .arg(camera->getErrorMsg().c_str()));

        //get the real params used by the camera:
        cycleTime = camera->getCycleTime();
        double timePerStack = nFramesPerStack*cycleTime;
    }

    if (hasPos && ownPos) pPositioner->setPCount();

    bool isAiEnable = false;
    bool isDiEnable = false;
    ofstream *ofsAi, *ofsDi;
    SampleIdx sampleNum;
    vector<int> aiChanList;
    if (ownPos) {
        prepareDaqBuffered(); //piezo, camera1 exp, camera2 exp, laser shutter, stimuli (waveform version of 'preparePositioner')
        //prepare for AI:
        QString ainame = se->globalObject().property("ainame").toString();
        //TODO: make channel recording list depend on who owns the piezo
        //chanList.push_back(0);
        int aiBegin = conWaveData->getAIBegin();
        int p0Begin = conWaveData->getP0Begin();
        int p0InBegin = conWaveData->getP0InBegin();
        int scanRate = conWaveData->sampleRate;
        for (int i = aiBegin; i < p0Begin; i++) {
            if (conWaveData->getSignalName(i) != "") {
                aiChanList.push_back(i - aiBegin);
            }
        }
        sampleNum = conWaveData->totalSampleNum;
        if (aiChanList.size() != 0) {
            isAiEnable = true;
            aiThread = new AiThread(ainame, 10000, 50000, scanRate, aiChanList,
                    sampleNum, pPositioner->getClkOut()); // TODO: set scanRate as the Maximum rate of external clock
            ofsAi = new ofstream(aiFilename.toStdString(), ios::binary | ios::out | ios::trunc);
            if (ofsAi->fail()) {
                emit newLogMsgReady("WARNING: unable to write ai file\n");
            }
        }
        else {
            isAiEnable = false;
            aiThread = nullptr;
            ofsAi = nullptr;
        }
        //prepare for DI:
        QString diname = se->globalObject().property("diname").toString();
        vector<int> diChanList; // this list is not used for di channels
        int diBegin = p0InBegin - p0Begin;
        isDiEnable = true;
        diThread = new DiThread(diname, 10000, 50000, scanRate, diChanList, diBegin,
                    sampleNum, pPositioner->getClkOut()); // TODO: set scanRate as the Maximum rate of external clock
        ofsDi = new ofstream(diFilename.toStdString(), ios::binary | ios::out | ios::trunc);
        if (ofsDi->fail()) {
            emit newLogMsgReady("WARNING: unable to write di file\n");
        }
    }
    else {
        aiThread = nullptr;
        ofsAi = nullptr;
        diThread = nullptr;
        ofsDi = nullptr;
    }
    unique_ptr<AiThread> uniPtrAiThread(aiThread);
    unique_ptr<ofstream> uniPtrOfsAi(ofsAi);
    unique_ptr<DiThread> uniPtrDiThread(diThread);
    unique_ptr<ofstream> uniPtrOfsDi(ofsDi);
    if (ownPos) {
        diThread->setOfstream(ofsDi);
        //start DI
        diThread->startAcq();
        if (isAiEnable) {
            aiThread->setOfstream(ofsAi);
            //start AI
            aiThread->startAcq();
            //after all devices are prepared, now we can save the file header:
            saveHeader(headerFilename, aiThread->ai, diThread->di, conWaveData);
        }
        else {
            saveHeader(headerFilename, NULL, diThread->di, conWaveData);
        }
    }
    else {
        saveHeader(headerFilename, NULL, NULL, conWaveData);
    }
    isUpdatingImage = false;

    if (useCam) {
        Camera::PixelValue * frame = (Camera::PixelValue*)_aligned_malloc(sizeof(Camera::PixelValue*)*(camera->imageSizePixels), 4 * 1024);
        idxCurStack = 0; //stack we are currently working on.  We will keep this in sync with camera->nAcquiredStacks
        camera->prepCameraOnce();
    }
    // start
//    camera->nextStack(); // wait until next stack and begin to capture

    int nOverrunStacks = 0;

    gt.start(); //seq's start time is 0, the new ref pt

    if (useCam) camera->nextStack();
    double curTime;
    double lastTime;
    double stackStartTime;
    bool isPiezo = hasPos && pPositioner->posType == PiezoControlPositioner;
    if (hasPos && ownPos) {
        digOut->runCmd();
        pPositioner->runCmd();
    }
nextStack:  //code below is repeated every stack
    stackStartTime = gt.read();

    //raise priority here to ensure that the camera and piezo begin (nearly) simultaneously
    setPriority(QThread::TimeCriticalPriority);
    //lower priority back to default
    setPriority(getDefaultPriority());

    long nFramesGotForStack = 0;
    long tempNumStacks = 0;
    long tempNumFrames = 0;

    if (useCam) {
        while (!stopRequested) {
            //to avoid "priority inversion" we temporarily elevate the priority
            setPriority(QThread::TimeCriticalPriority);
            if (pCamera->getModel() == "dummy") {
                if (!((++nFramesGotForStack) % nFramesPerStack))
                    tempNumStacks++;
            }
            else {
                nFramesGotForStack = camera->getNAcquiredFrames();
                tempNumStacks = camera->getNAcquiredStacks();
            }
            setPriority(getDefaultPriority());
            if (idxCurStack < tempNumStacks) {
                idxCurStack = tempNumStacks;
                OutputDebugStringW((wstring(L"Data acq thread finished stack #") + to_wstring(idxCurStack) + wstring(L"\n")).c_str());
                break;
            }
            if (!isUpdatingImage && tempNumFrames != nFramesGotForStack) {
                tempNumFrames = nFramesGotForStack;
                QThread::msleep(10);
                //copy latest frame to display buffer
                if (!camera->updateLiveImage()) {
                    QThread::msleep(10);
                    continue;
                }
                setIsUpdatingImage(true);
                int progress = aiThread->writeSampleNum / aiChanList.size() / (sampleNum / 100.);
                emit imageDataReady(camera->getLiveImage(), nFramesGotForStack - 1,
                    camera->getImageWidth(), camera->getImageHeight(), progress); //-1: due to 0-based indexing
            }
            else {
                QThread::msleep(10);
            }
        }//while, camera is not idle
    }

    if (useCam && (idxCurStack < this->nStacks) && !stopRequested) {
        if (!stopRequested) goto nextStack;
    }//if, there're more stacks and no stop requested

    if (useCam) {
        this->setPriority(QThread::TimeCriticalPriority);
        camera->stopAcqFinal(); //priority inversion delays stopping, so we haave to elevate priority
        this->setPriority(getDefaultPriority());
    }
    if (hasPos && ownPos) {
        if (!stopRequested && aiThread) { // wait until all daq samples are read
            while(aiThread->isLeftToReadSamples())
                QThread::msleep(100);
        }
        if (isAiEnable && aiThread) {
            aiThread->stopAcq();
            aiThread->save(*ofsAi);
            ofsAi->flush();
        }
        if (isDiEnable && diThread) {
            diThread->stopAcq();
            diThread->save(*ofsDi);
            ofsDi->flush();
        }
        pPositioner->abortCmd();
        //        pPositioner->resetDAQ();
        digOut->abortCmd();
        // reset for On Demend digital output
        // digOut->resetDAQ(); // need to do this every time we output single value to dout
        emit newStatusMsgReady("Now resetting the actuator to its exact starting pos ...");
        //reset the actuator to its exact starting pos
        emit resetActuatorPosReady();
        emit reapplyLaserSettings();
    }

    QString ttMsg = "Acquisition is done";
    if (stopRequested) ttMsg += " (User requested STOP)";
    emit newStatusMsgReady(ttMsg);

    if (!nOverrunStacks) ttMsg = "Idle time is OK (NO overrun).";
    else {
        ttMsg = QString("# of overrun stacks: %1").arg(nOverrunStacks);
    }
    emit newStatusMsgReady(ttMsg);
    ownPos = false;
}//run_acq_and_save()

#pragma endregion

#pragma region Stimulus

void DataAcqThread::fireStimulus(int valve)
{
    emit newStatusMsgReady(QString("valve is open: %1").arg(valve));
    for (int i = 0; i < 4; ++i){
        digOut->singleOut(i, valve & 1);
        valve >>= 1;
    }
}//fireStimulus(),

#pragma endregion

#pragma region File IO

QString replaceExtName(QString filename, QString newExtname);

bool DataAcqThread::saveHeader(QString filename, DaqAi* ai, DaqDi* di, ControlWaveform *conWaveData)
{
    Camera& camera = *pCamera;

    ofstream header(filename.toStdString().c_str(),
        ios::out | ios::trunc);
    header << headerMagic << endl;
    header << "[general]" << endl
        << "header version=7" << endl
        << "app version=2.0, build (" << __DATE__ << ", " << __TIME__ << ")" << endl
        << "date and time=" << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << endl
        << "byte order=l" << endl
        << "rig=" << rig << endl
        << endl;

    header << "[misc params]" << endl
        << "stimulus file content="
        << (applyStim ? linize(stimFileContent).toStdString() : "")
        << endl;

    header << "comment=" << linize(comment).toStdString() << endl; //TODO: may need encode into one line

    header << "ai data file=" << aiFilename.toStdString() << endl
        << "image data file=" << camFilename.toStdString() << endl;
    if (conWaveData != NULL) {
        header << "command file=" << commandFilename.toStdString() << endl
            << "di data file=" << diFilename.toStdString() << endl;
    }
    else {
        header << "command file=" << "NA" << endl
            << "di data file=" << "NA" << endl;
    }
    header << "piezo=start position: " << conWaveData->piezoStartPosUm << " um"
        << ";stop position: " << conWaveData->piezoStopPosUm << " um"
        << ";output scan rate: " << conWaveData->sampleRate
        << endl << endl;

    //ai,di related:
    int aiBegin, p0Begin, p0InBegin, numChannel;
    if (conWaveData != NULL) {
        aiBegin = conWaveData->getAIBegin();
        p0Begin = conWaveData->getP0Begin();
        p0InBegin = conWaveData->getP0InBegin();
        numChannel = conWaveData->getNumChannel();
    }

    if (ai != NULL) {
        header << "[ai]" << endl
            << "nscans=" << -1 << endl; //TODO: output nscans at the end
        if (conWaveData != NULL) {
            header << "channel list=";
            for (int i = aiBegin; i < p0Begin; i++) {
                if (conWaveData->getSignalName(i) != "") {
                    header << i - aiBegin << " ";
                }
            }
            header.seekp(-1, header.cur);
            header << endl << "label list=";
            for (int i = aiBegin; i < p0Begin; i++) {
                if (conWaveData->getSignalName(i) != "") {
                    header << conWaveData->getSignalName(i).toStdString() << "$";
                }
            }
            header.seekp(-1, header.cur);
            header << endl;
        }
        header << "scan rate=" << ai->scanRate << endl
            << "min sample=" << ai->minDigitalValue << endl
            << "max sample=" << ai->maxDigitalValue << endl
            << "min input=" << ai->minPhyValue << endl
            << "max input=" << ai->maxPhyValue << endl << endl;
    }
    if (di != NULL) {
        if (conWaveData != NULL) {
            header << "[di]" << endl
                << "di nscans=" << -1 << endl; //TODO: output nscans at the end
            header << "di channel list=";
            for (int i = p0InBegin; i < numChannel; i++) {
                header << i - p0Begin << " ";
            }
            header.seekp(-1, header.cur);
            //                header << endl << "di channel list base="
            //                    << conWaveData->getP0InBegin() - conWaveData->getP0Begin() << endl;
            header << endl << "di label list=";
            for (int i = p0InBegin; i < numChannel; i++) {
                if (conWaveData->getSignalName(i) != "") {
                    header << conWaveData->getSignalName(i).toStdString() << "$";
                }
                else
                    header << "unused" << "$";
            }
            header.seekp(-1, header.cur);
            header << endl;
            header << "di scan rate=" << di->scanRate << endl << endl;
        }
    }

    //camera related:
    header << "[camera]" << endl
        << "original image depth=16" << endl    //effective bits per pixel
        << "saved image depth=16" << endl       //effective bits per pixel
        << "image width=" << camera.getImageWidth() << endl
        << "image height=" << camera.getImageHeight() << endl
        << "number of frames requested=" << nStacks*nFramesPerStack << endl
        << "nStacks=" << nStacksUser << endl
        << "idle time between stacks=" << idleTimeBwtnStacks << " s" << endl;
    header << "pre amp gain=" << preAmpGain.toStdString() << endl
        << "gain=" << gain << endl
        << "exposure time=" << exposureTime << " s" << endl
        << "vertical shift speed=" << verShiftSpeed.toStdString() << endl
        << "vertical clock vol amp=" << verClockVolAmp << endl
        << "readout rate=" << horShiftSpeed.toStdString() << endl;
    header << "pixel order=x y z" << endl
        << "frame index offset=0" << endl
        << "frames per stack=" << nFramesPerStackUser << endl
        << "pixel data type=uint16" << endl    //bits per pixel
        << "camera=" << camera.getModel() << endl
        << "um per pixel=" << umPerPxlXy << endl
        << "binning=" << "hbin:" << camera.hbin
        << ";vbin:" << camera.vbin
        << ";hstart:" << camera.hstart
        << ";hend:" << camera.hend
        << ";vstart:" << camera.vstart
        << ";vend:" << camera.vend << endl
        << "angle from horizontal (deg)=" << angle << endl
        << "bidirectional=" << isBiDirectionalImaging
        << endl << endl;

    header << "[laser]" << endl
        << "405nm on=" << conWaveData->getLaserDefaultTTL(1) << endl
        << "445nm on=" << conWaveData->getLaserDefaultTTL(2) << endl
        << "488nm on=" << conWaveData->getLaserDefaultTTL(3) << endl
        << "514nm on=" << conWaveData->getLaserDefaultTTL(4) << endl
        << "561nm on=" << conWaveData->getLaserDefaultTTL(5) << endl
        << "405nm percent intensity=" << conWaveData->laserIntensity[3] << endl
        << "445nm percent intensity=" << conWaveData->laserIntensity[4] << endl
        << "488nm percent intensity=" << conWaveData->laserIntensity[1] << endl
        << "514nm percent intensity=" << conWaveData->laserIntensity[5] << endl
        << "561nm percent intensity=" << conWaveData->laserIntensity[2] << endl << endl;

    header << "[mismatch correction]" << endl
        << "x translation in pixels=" << mismatchXTranslation << endl
        << "y translation in pixels=" << mismatchYTranslation << endl
        << "rotation angle in degree=" << mismatchRotation << endl;

    header.close();

    return true;
}//DataAcqThread::saveHeader()

#pragma endregion



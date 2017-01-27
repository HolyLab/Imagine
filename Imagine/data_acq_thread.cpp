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

QScriptEngine* se;
DaqDo * digOut = nullptr;
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
    digOut->updateOutputBuf(5, true);
    digOut->write();
    Sleep(duration);
    digOut->updateOutputBuf(5, false);
    digOut->write();
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

#pragma endregion

#pragma region Acquisition

void DataAcqThread::startAcq()
{
    stopRequested = false;
    this->start(getDefaultPriority());
}

void DataAcqThread::stopAcq()
{
    stopRequested = true;
}

void DataAcqThread::run()
{
    if (isLive) run_live();
    else run_acq_and_save();

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

            emit imageDataReady(camera->getLiveImage(), nFramesGot - 1, camera->getImageWidth(), camera->getImageHeight()); //-1: due to 0-based indexing

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

    ////prepare for AO AI and camera:

    ///prepare for camera:

    camFilename = replaceExtName(headerFilename, "cam"); //NOTE: necessary if user overwrite files

    ///piezo feedback data file (only for PI piezo)
    string positionerFeedbackFile = replaceExtName(headerFilename, "pos").toStdString();

    bool startCameraOnce = (acqTriggerMode == Camera::eExternal);

    //int framefactor = startCameraOnce ? this->nStacks : 1;

    camera->setSpooling(camFilename.toStdString());

    ///camera->setAcqModeAndTime(AndorCamera::eKineticSeries,
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

    //TODO: fix this ugliness
    string wTitle = (*parentImagine).windowTitle().toStdString();
    string posOwner = "";
    if ((*parentImagine).masterImagine != NULL) {
        posOwner = (*parentImagine).masterImagine->ui.comboBoxPositionerOwner->currentText().toStdString();
    }
    else {
        posOwner = (*parentImagine).ui.comboBoxPositionerOwner->currentText().toStdString();
    }

    bool ownPos = false;
    if (!wTitle.compare("Imagine (2)") && !posOwner.compare("Camera 2")) {
        ownPos = true;
    }
    else if (!wTitle.compare("Imagine (1)") && !posOwner.compare("Camera 1")) {
        ownPos = true;
    }
    else if (!posOwner.compare("Either")) {
        ownPos = true;
    }
    else if (!wTitle.compare("Imagine")) {
        ownPos = true;
    }

    if (hasPos && ownPos) pPositioner->setPCount();
    bool useTrig = true;
    if (ownPos) preparePositioner(true, useTrig); //nec for volpiezo

    //prepare for AI:
    QString ainame = se->globalObject().property("ainame").toString();
    //TODO: make channel recording list depend on who owns the piezo
    vector<int> aiChanList;
    //chanList.push_back(0);
    for (int i = 0; i < 3; ++i) {
        aiChanList.push_back(i);
    }
    aiThread = new AiThread(ainame, 10000, 50000, 10000, aiChanList);
    unique_ptr<AiThread> uniPtrAiThread(aiThread);

    //after all devices are prepared, now we can save the file header:
    QString stackDir = replaceExtName(camFilename, "stacks");

    saveHeader(headerFilename, aiThread->ai);

    ofstream *ofsAi = new ofstream(aiFilename.toStdString(), ios::binary | ios::out | ios::trunc);
    unique_ptr<ofstream> uniPtrOfsAi(ofsAi);
    aiThread->setOfstream(ofsAi);

    isUpdatingImage = false;


    Camera::PixelValue * frame = (Camera::PixelValue*)_aligned_malloc(sizeof(Camera::PixelValue*)*(camera->imageSizePixels), 4 * 1024);

    idxCurStack = 0; //stack we are currently working on.  We will keep this in sync with camera->nAcquiredStacks

    {
        QScriptValue jsFunc = se->globalObject().property("onShutterInit");
        if (jsFunc.isFunction()) jsFunc.call();
    }

    //start AI
    if (ownPos) aiThread->startAcq();

    camera->prepCameraOnce();

    if (startCameraOnce)
        camera->nextStack();

    //start stimuli[0] if necessary
    if (applyStim && stimuli[0].second == 0) {
        curStimIndex = 0;
        fireStimulus(stimuli[curStimIndex].first);
    }//if, first stimulus should be fired before stack_0

    int nOverrunStacks = 0;

    gt.start(); //seq's start time is 0, the new ref pt

    double curTime;
    double lastTime;
    double stackStartTime;

nextStack:  //code below is repeated every stack
    stackStartTime = gt.read();

    //   below code caused an intermittent access violation write error
    /*
    {

        QScriptValue jsFunc = se->globalObject().property("onShutterOpen");
        if (jsFunc.isFunction()) {
            
               se->globalObject().setProperty("currentStackIndex", idxCurStack);
               jsFunc.call();
        }
    }*/

    //log messages like the below may bog down the GUI and cause overruns when taking stacks at a very high rate
    /*
    emit newLogMsgReady(QString("Acquiring stack (0-based)=%1 @time=%2 s")
        .arg(idxCurStack)
        .arg(stackStartTime, 10, 'f', 4) //width=10, fixed point, 4 decimal digits 
        );
    */
    if (hasPos && ownPos) pPositioner->optimizeCmd(); //This function currently does nothing for voltage positioner.  Is this okay?

    bool isPiezo = hasPos && pPositioner->posType == PiezoControlPositioner;
    if (hasPos && ownPos) pPositioner->runCmd(); //will wait on trigger pulse from camera
    if (acqTriggerMode == Camera::eExternal){
        if (!startCameraOnce && !isPiezo){
            camera.startAcq();
            double timeToWait = 0.1;
            //TODO: maybe I should use busy waiting?
        }
        if (hasPos && ownPos) {
            pPositioner->runCmd();
        }

        if (!startCameraOnce && isPiezo) {
            camera->nextStack();
        }
    }
    else {
        //Camera takes a while to start, so do this before starting positioner
        if (!startCameraOnce) {
            camera->nextStack();
        }
    }

    //lower priority back to default
    QThread::setPriority(getDefaultPriority());

    //log messages like the below may bog down the GUI and cause overruns when taking stacks at a very high rate
    /*
    emit newStatusMsgReady(QString("Camera: started acq: %1")
        .arg(camera->getErrorMsg().c_str()));
    */

    long nFramesGotForStack=0;
    long tempNumStacks=0;
    long tempNumFrames=0;

    while (!stopRequested) {
        //to avoid "priority inversion" we temporarily elevate the priority
        this->setPriority(QThread::TimeCriticalPriority);
        nFramesGotForStack = camera->getNAcquiredFrames();
        tempNumStacks = camera->getNAcquiredStacks();

        this->setPriority(getDefaultPriority());
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
            emit imageDataReady(camera->getLiveImage(), nFramesGotForStack - 1,  camera->getImageWidth(), camera->getImageHeight()); //-1: due to 0-based indexing
            
        }
        else {
            QThread::msleep(10);
        }
    }//while, camera is not idle

    //close laser shutter
    digOut->updateOutputBuf(4, false);
    digOut->write();

    //below code also causes intermittent crashes
    /*
    {
        QScriptValue jsFunc = se->globalObject().property("onShutterClose");
        if (jsFunc.isFunction()) {
            se->globalObject().setProperty("currentStackIndex", camera->nAcquiredStacks.load());
            jsFunc.call();
        }
    }
    */

    //update stimulus if necessary:  idxCurStack
    if (applyStim
        && curStimIndex + 1 < stimuli.size()
        && stimuli[curStimIndex + 1].second == idxCurStack + 1) {
        curStimIndex++;
        fireStimulus(stimuli[curStimIndex].first);
    }//if, should update stimulus

    if (hasPos && ownPos) {
        pPositioner->waitCmd();
    }


    idxCurStack++;  //post: it is #stacks we got so far
    if (idxCurStack < this->nStacks && !stopRequested){
        if (isBiDirectionalImaging && ownPos){
            preparePositioner(idxCurStack % 2 == 0, useTrig);
        }

        double stackEndingTime = gt.read();
        double timeToWait;

        if (expTriggerMode != Camera::eAuto && !ownPos) timeToWait = 0;
        else timeToWait = (timePerStack + idleTimeBwtnStacks)*idxCurStack - stackEndingTime;

        OutputDebugStringW((wstring(L"Waiting for ") + to_wstring(int(timeToWait*1000)) + wstring(L" milliseconds to start next stack\n")).c_str());

        if ((timeToWait < 0) && ownPos) {
            emit newLogMsgReady("WARNING: overrun(overall progress): idle time is too short.");
        }
        //TODO: warn user when the cycle time of the camera doesn't match the exposure time they ask for.
        //the camera always returns the minimum cycle time when the user requests a shorter exposure than it can produce.
        if ((stackEndingTime - stackStartTime > (timePerStack+idleTimeBwtnStacks)) && (expTriggerMode == Camera::eAuto || ownPos)) {
            nOverrunStacks++;
            emit newLogMsgReady("WARNING: overrun(current stack): Probably the positioner cannot keep up:\n");
            QString temp = QString::fromStdString("      The stack was acquired " + to_string(int((stackEndingTime - stackStartTime- (timePerStack+idleTimeBwtnStacks)) * 1000000)) + " microseconds too slowly.\n");
            emit newLogMsgReady(temp);
        }

        double maxWaitTime = 2; //in seconds. The frequency to check stop signal
    repeatWait:
        timeToWait = (timePerStack+idleTimeBwtnStacks)*idxCurStack - gt.read();
        if (timeToWait > 0.01) {
            QThread::msleep(min(maxWaitTime, timeToWait) * 1000); // *1000: sec -> ms
        }//if, need wait more than 10ms
        if (timeToWait > maxWaitTime && !stopRequested) goto repeatWait;

        if (!stopRequested) goto nextStack;
    }//if, there're more stacks and no stop requested

    this->setPriority(QThread::TimeCriticalPriority);
    camera->stopAcqFinal(); //priority inversion delays stopping, so we haave to elevate priority
    this->setPriority(getDefaultPriority());


    ///save ai data:
    if (ownPos) {
        aiThread->save(*ofsAi);
        ofsAi->flush();
    }

    //fire post-seq stimulus:
    if (applyStim) {
        int postSeqStim = 0; //TODO: get this value from stim file header
        fireStimulus(postSeqStim);
    }

    if (ownPos) {
        aiThread->stopAcq();
        aiThread->save(*ofsAi);
    }   

    {
        QScriptValue jsFunc = se->globalObject().property("onShutterFini");
        if (jsFunc.isFunction()) jsFunc.call();
    }

    ///reset the actuator to its exact starting pos
    if (hasPos && ownPos) {
        emit newStatusMsgReady("Now resetting the actuator to its exact starting pos ...");
        emit resetActuatorPosReady();
    }

    QString ttMsg = "Acquisition is done";
    if (stopRequested) ttMsg += " (User requested STOP)";
    emit newStatusMsgReady(ttMsg);

    if (!nOverrunStacks) ttMsg = "Idle time is OK (NO overrun).";
    else {
        ttMsg = QString("# of overrun stacks: %1").arg(nOverrunStacks);
    }
    emit newStatusMsgReady(ttMsg);

}//run_acq_and_save()

#pragma endregion

#pragma region Stimulus

void DataAcqThread::fireStimulus(int valve)
{
    emit newStatusMsgReady(QString("valve is open: %1").arg(valve));
    for (int i = 0; i < 4; ++i){
        digOut->updateOutputBuf(i, valve & 1);
        valve >>= 1;
    }
    digOut->write();
}//fireStimulus(),

#pragma endregion

#pragma region File IO

QString replaceExtName(QString filename, QString newExtname);

bool DataAcqThread::saveHeader(QString filename, DaqAi* ai)
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

    //TODO: output shutter params
    //header<<"shutter=open time: ;"<<endl; 

    header << "piezo=start position: " << piezoStartPosUm << " um"
        << ";stop position: " << piezoStopPosUm << " um"
        << ";output scan rate: " << 10000 //todo: hard coded
        << ";bidirection: " << isBiDirectionalImaging
        << endl << endl;

    //ai related:
    header << "[ai]" << endl
        << "nscans=" << -1 << endl //TODO: output nscans at the end
        << "channel list=0 1 2" << endl //TODO: this and label list shouldn't be hardcoded
        << "label list=piezo$stimuli$camera frame TTL" << endl
        << "scan rate=" << ai->scanRate << endl
        << "min sample=" << ai->minDigitalValue << endl
        << "max sample=" << ai->maxDigitalValue << endl
        << "min input=" << ai->minPhyValue << endl
        << "max input=" << ai->maxPhyValue << endl << endl;

    //camera related:
    header << "[camera]" << endl
        << "original image depth=14" << endl    //effective bits per pixel
        << "saved image depth=14" << endl       //effective bits per pixel
        << "image width=" << camera.getImageWidth() << endl
        << "image height=" << camera.getImageHeight() << endl
        << "number of frames requested=" << nStacks*nFramesPerStack << endl
        << "nStacks=" << nStacks << endl
        << "idle time between stacks=" << idleTimeBwtnStacks << " s" << endl;
    header << "pre amp gain=" << preAmpGain.toStdString() << endl
        << "gain=" << gain << endl
        << "exposure time=" << exposureTime << " s" << endl
        << "vertical shift speed=" << verShiftSpeed.toStdString() << endl
        << "vertical clock vol amp=" << verClockVolAmp << endl
        << "readout rate=" << horShiftSpeed.toStdString() << endl;
    header << "pixel order=x y z" << endl
        << "frame index offset=0" << endl
        << "frames per stack=" << nFramesPerStack << endl
        << "pixel data type=uint16" << endl    //bits per pixel
        << "camera=" << camera.getModel() << endl
        << "um per pixel=" << umPerPxlXy << endl
        << "binning=" << "hbin:" << camera.hbin
        << ";vbin:" << camera.vbin
        << ";hstart:" << camera.hstart
        << ";hend:" << camera.hend
        << ";vstart:" << camera.vstart
        << ";vend:" << camera.vend << endl
        << "angle from horizontal (deg)=" << angle << endl;

    header.close();

    return true;
}//DataAcqThread::saveHeader()

#pragma endregion



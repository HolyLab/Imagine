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

#include "data_acq_thread.hpp"

#include "andor_g.hpp"
#include "timer_g.hpp"
#include "ni_daq_g.hpp"
#include "ai_thread.hpp"
#include "fast_ofstream.hpp"
#include "positioner.hpp"
#include "Piezo_Controller.hpp"
#include "spoolthread.h"
#include "imagine.h"

QScriptEngine* se;
DaqDo * digOut = nullptr;
QString daq;
string rig;

//defined in imagine.cpp:
extern vector<pair<int, int> > stimuli; //first: stim (valve), second: time (stack#)

QString replaceExtName(QString filename, QString newExtname);

DataAcqThread::DataAcqThread(Camera *cam, Positioner *pos, QObject *parent)
    : QThread(parent)
{
    restart = false;
    abort = false;
    isLive = true;
    parentImagine = (Imagine*)parent;
    setCamera(cam);
    // not that this was allocated with 'new', so you need to delete when you die
    pPositioner = pos;
}

DataAcqThread::~DataAcqThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    // TODO: clean up the camera, if needed
    if (pCamera->vendor == "cooke"){
        SpoolThread::freeMemPool();
    }

    delete pCamera;
    delete pPositioner;

    wait();
}

void DataAcqThread::startAcq()
{
    stopRequested = false;
    this->start();
}

void DataAcqThread::stopAcq()
{
    stopRequested = true;
}

//join several lines into one line
QString linize(QString lines)
{
    lines.replace("\n", "\\n");
    return lines;
}

const string headerMagic = "IMAGINE";

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
        << "channel list=0 1 2 3" << endl
        << "label list=piezo$stimuli$camera frame TTL$heartbeat" << endl
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

extern volatile bool isUpdatingImage;

void DataAcqThread::fireStimulus(int valve)
{
    emit newStatusMsgReady(QString("valve is open: %1").arg(valve));
    for (int i = 0; i < 4; ++i){
        digOut->updateOutputBuf(i, valve & 1);
        valve >>= 1;
    }
    digOut->write();
}//fireStimulus(),

bool DataAcqThread::preparePositioner(bool isForward)
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
    pPositioner->addMovement(numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN(), 0, 0); //no movement, only stop the trigger

    if (pPositioner->posType == ActuatorPositioner) pPositioner->setDim(oldAxis);

    if (pPositioner->testCmd())  pPositioner->prepareCmd();
    else return false;

    return true;

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
    Camera& camera = *pCamera;

    //prepare for camera:
    bool suc = camera.setAcqModeAndTime(Camera::eLive,
        this->exposureTime,
        this->nFramesPerStack,
        Camera::eInternalTrigger  //use internal trigger
        );
    emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
        .arg(camera.getErrorMsg().c_str()));
    if (!suc) return;

    isUpdatingImage = false;

    int imageW = camera.getImageWidth();
    int imageH = camera.getImageHeight();
    int nPixels = imageW*imageH;
    Camera::PixelValue * frame = new Camera::PixelValue[nPixels];
    unique_ptr<Camera::PixelValue[]> uniPtrFrame(frame);

    Timer_g timer;
    timer.start();

    camera.startAcq();
    emit newStatusMsgReady(QString("Camera: started acq: %1")
        .arg(camera.getErrorMsg().c_str()));


    long nDisplayUpdating = 0;
    long nFramesGot = 0;
    long nFramesGotCur;
    while (!stopRequested){
        nFramesGotCur = camera.getAcquiredFrameCount();
        if (nFramesGotCur == -1){
            Sleep(20);
            continue;
        }

        if (nFramesGot != nFramesGotCur && !isUpdatingImage){
            nFramesGot = nFramesGotCur;

            //get the last frame:
            if (!camera.getLatestLiveImage(frame)){
                Sleep(20);
                continue;
            }

            //copy data
            QByteArray data16((const char*)frame, imageW*imageH * 2);
            emit imageDataReady(data16, nFramesGot - 1, imageW, imageH); //-1: due to 0-based indexing

            nDisplayUpdating++;
            if (nDisplayUpdating % 10 == 0){
                emit newStatusMsgReady(QString("Screen update frame rate: %1")
                    .arg(nDisplayUpdating / timer.read(), 5, 'f', 2) //width=5, fixed point, 2 decimal digits
                    );
            }

            //spare some time for gui thread's event handling:
            QApplication::instance()->processEvents();
            double timeToWait = 0.05;
            QThread::msleep(timeToWait * 1000); // *1000: sec -> ms
        }
    }//while, user did not requested stop

    camera.stopAcq(); //TODO: check return value


    QString ttMsg = "Live mode is stopped";
    emit newStatusMsgReady(ttMsg);
}//run_live()

void genSquareSpike(int duration)
{
    //return;

    //cout<<"enter gen spike @"<<gTimer.read()<<endl;
    digOut->updateOutputBuf(5, true);
    digOut->write();
    Sleep(duration);
    digOut->updateOutputBuf(5, false);
    digOut->write();
    //cout<<"leave gen spike @"<<gTimer.read()<<endl;
}

void DataAcqThread::run_acq_and_save()
{
    Camera& camera = *pCamera;
    Timer_g gt = parentImagine->gTimer;

    bool isAndor = camera.vendor == "andor";
    bool isCooke = camera.vendor == "cooke";
    bool isAvt = camera.vendor == "avt";
    bool hasPos = pPositioner != NULL;

    if (isAndor){
        isUseSpool = true;  //TODO: make ui aware this
    }
    else if (isCooke){
        isUseSpool = true;
    }
    else if (isAvt){
        isUseSpool = true;
    }
    else {
        isUseSpool = false;
    }

    ////prepare for AO AI and camera:

    ///prepare for camera:

    camFilename = replaceExtName(headerFilename, "cam"); //NOTE: necessary if user overwrite files

    ///piezo feedback data file (only for PI piezo)
    string positionerFeedbackFile = replaceExtName(headerFilename, "pos").toStdString();

    //if cooke/avt, setSpooling() here!!! (to avoid out-of-mem when setAcqModeAndTime())
    if ((isCooke || isAvt) && isUseSpool){
        camera.setSpooling(camFilename.toStdString());
    }

    bool startCameraOnce = isCooke && triggerMode == Camera::eExternalStart;

    int framefactor = startCameraOnce ? this->nStacks : 1;

    ///camera.setAcqModeAndTime(AndorCamera::eKineticSeries,
    camera.setAcqModeAndTime(Camera::eAcqAndSave,
        this->exposureTime,
        this->nFramesPerStack*framefactor,
        this->triggerMode);
    emit newStatusMsgReady(QString("Camera: set acq mode and time: %1")
        .arg(camera.getErrorMsg().c_str()));

    //get the real params used by the camera:
    cycleTime = camera.getCycleTime();

    if (hasPos) pPositioner->setPCount();

    preparePositioner(); //nec for volpiezo

    //prepare for AI:
    QString ainame = se->globalObject().property("ainame").toString();
    AiThread * aiThread = new AiThread(0, ainame, 10000, 50000, 10000);
    unique_ptr<AiThread> uniPtrAiThread(aiThread);

    //after all devices are prepared, now we can save the file header:
    QString stackDir = replaceExtName(camFilename, "stacks");
    if (isUseSpool && isAndor){
        QDir::current().mkpath(stackDir);
        camFilename = stackDir + "\\stack_%1_";
    }

    saveHeader(headerFilename, aiThread->ai);

    ofstream *ofsAi = new ofstream(aiFilename.toStdString(), ios::binary | ios::out | ios::trunc);
    unique_ptr<ofstream> uniPtrOfsAi(ofsAi);
    aiThread->setOfstream(ofsAi);

    FastOfstream *ofsCam = NULL;
    if (!isUseSpool){
        ofsCam = new FastOfstream(camFilename.toStdString());
        assert(*ofsCam);
    }

    isUpdatingImage = false;

    int imageW = camera.getImageWidth();
    int imageH = camera.getImageHeight();

    int nPixels = imageW*imageH;
    //Camera::PixelValue * frame=new Camera::PixelValue[nPixels];
    Camera::PixelValue * frame = (Camera::PixelValue*)_aligned_malloc(sizeof(Camera::PixelValue)*nPixels, 4 * 1024);
    unique_ptr<Camera::PixelValue, decltype(_aligned_free)*> uniPtrFrame(frame, _aligned_free);

    idxCurStack = 0;

    {
        QScriptValue jsFunc = se->globalObject().property("onShutterInit");
        if (jsFunc.isFunction()) jsFunc.call();
    }

    //start AI
    aiThread->startAcq();

    if (startCameraOnce)
        camera.startAcq();

    //start stimuli[0] if necessary
    if (applyStim && stimuli[0].second == 0){
        curStimIndex = 0;
        fireStimulus(stimuli[curStimIndex].first);
    }//if, first stimulus should be fired before stack_0

    int nOverrunStacks = 0;
    long nFramesDoneStack = this->nFramesPerStack;

    gt.start(); //seq's start time is 0, the new ref pt

nextStack:
    double stackStartTime = gt.read();
    cout << "b4 open laser: " << stackStartTime << endl;

    //open laser shutter
    digOut->updateOutputBuf(4, true);
    digOut->write();
    //cout<<"b4 open laser(js call): "<<gTimer.read()<<endl;
    {
        QScriptValue jsFunc = se->globalObject().property("onShutterOpen");
        if (jsFunc.isFunction()) {
            se->globalObject().setProperty("currentStackIndex", idxCurStack);
            jsFunc.call();
        }
    }

    cout << "after open laser: " << gt.read() << endl;
    //TODO: may need delay for shutter open time

    emit newLogMsgReady(QString("Acquiring stack (0-based)=%1 @time=%2 s")
        .arg(idxCurStack)
        .arg(gt.read(), 10, 'f', 4) //width=10, fixed point, 4 decimal digits 
        );

    if (isAndor && isUseSpool) {
        QString stemName = camFilename.arg(idxCurStack, 4, 10, QLatin1Char('0'));
        //enable spool
        ((AndorCamera*)(&camera))->enableSpool((char*)(stemName.toStdString().c_str()), 10);
    }

    if (hasPos) pPositioner->optimizeCmd();

    cout << "b4 start camera & piezo: " << gt.read() << endl;

    bool isPiezo = hasPos && pPositioner->posType == PiezoControlPositioner;
    if (triggerMode == Camera::eExternalStart){
        if (!startCameraOnce && !isPiezo){
            camera.startAcq();
            double timeToWait = 0.1;
            //TODO: maybe I should use busy waiting?
            QThread::msleep(timeToWait * 1000); // *1000: sec -> ms
        }
        //genSquareSpike(10);
        if (hasPos) {
            cout << "b4 pPositioner->runCmd: " << gt.read() << endl;
            pPositioner->runCmd();
            cout << "after pPositioner->runCmd: " << gt.read() << endl;
        }

        if (!startCameraOnce && isPiezo){
            camera.startAcq();
        }
    }
    else {
        if (hasPos) pPositioner->runCmd();
        if (!startCameraOnce) {
            cout << "b4 camera.startacq: " << gt.read() << endl;
            camera.startAcq();
            cout << "after camera.startacq: " << gt.read() << endl;
        }
    }

    cout << "after start camera & piezo: " << gt.read() << endl;

    //TMP: trigger camera
    //digOut->updateOutputBuf(5,true);
    //digOut->write();

    emit newStatusMsgReady(QString("Camera: started acq: %1")
        .arg(camera.getErrorMsg().c_str()));


    long nFramesGotForStack = 0;
    long nFramesGotForStackCur;
    while (!stopRequested || this->nStacks > 1){
        nFramesGotForStackCur = camera.getAcquiredFrameCount();
        if (nFramesGotForStackCur == -1){
            Sleep(30);
            continue;
        }
        if (nFramesGotForStackCur >= nFramesDoneStack){
            break;
        }

        if (nFramesGotForStack != nFramesGotForStackCur && !isUpdatingImage){
            nFramesGotForStack = nFramesGotForStackCur;

            //get the latest frame:
            if (!camera.getLatestLiveImage(frame)) {
                Sleep(30);
                continue;
            }
            //copy data
            QByteArray data16((const char*)frame, imageW*imageH * 2);
            emit imageDataReady(data16, nFramesGotForStack - 1 - (nFramesDoneStack - nFramesPerStack), imageW, imageH); //-1: due to 0-based indexing
            Sleep(50);
        }
    }//while, camera is not idle

    //get the last frame if nec
    if (nFramesGotForStack < nFramesDoneStack){
        camera.getLatestLiveImage(frame);
        QByteArray data16((const char*)frame, imageW*imageH * 2);
        emit imageDataReady(data16, nFramesPerStack - 1, imageW, imageH); //-1: due to 0-based indexing
    }

    nFramesDoneStack += startCameraOnce ? nFramesPerStack : 0;

    cout << "b4 close laser: " << gt.read() << endl;

    //close laser shutter
    digOut->updateOutputBuf(4, false);
    digOut->write();
    {
        QScriptValue jsFunc = se->globalObject().property("onShutterClose");
        if (jsFunc.isFunction()){
            se->globalObject().setProperty("currentStackIndex", idxCurStack);
            jsFunc.call();
        }
    }

    //update stimulus if necessary:  idxCurStack
    if (applyStim
        && curStimIndex + 1 < stimuli.size()
        && stimuli[curStimIndex + 1].second == idxCurStack + 1){
        curStimIndex++;
        fireStimulus(stimuli[curStimIndex].first);
    }//if, should update stimulus

    if (!isUseSpool){
        //get camera's data:
        bool result = camera.transferData();
        if (!result){
            emit newStatusMsgReady(QString(camera.getErrorMsg().c_str())
                + QString("(errCode=%1)").arg(camera.getErrorCode()));
            return;
        }
    }

    ////save data to files:

    ///save camera's data:
    if (!isUseSpool){
        Camera::PixelValue * imageArray = camera.getImageArray();
        double timerValue = gt.read();
        ofsCam->write((const char*)imageArray,
            sizeof(Camera::PixelValue)*nFramesPerStack*imageW*imageH);
        if (!*ofsCam){
            //TODO: deal with the error
        }//if, error occurs when write camera data
        cout << "time for writing the stack: " << gt.read() - timerValue << endl;
        //ofsCam->flush();
    }//if, save data to file and not using spool

    cout << "b4 flush ai data: " << gt.read() << endl;

    ///save ai data:
    aiThread->save(*ofsAi);
    ofsAi->flush();

    if (!startCameraOnce) {
        cout << "b4 stop camera: " << gt.read() << endl;
        camera.stopAcq();
        cout << "after stop camera: " << gt.read() << endl;
    }

    if (hasPos) {
        cout << "b4 wait piezo: " << gt.read() << endl;
        //genSquareSpike(50);
        pPositioner->waitCmd();
        cout << "after wait piezo: " << gt.read() << endl;
    }
    genSquareSpike(70);

    //TMP: trigger off camera
    //digOut->updateOutputBuf(5,false);
    //digOut->write();

    idxCurStack++;  //post: it is #stacks we got so far
    if (idxCurStack < this->nStacks && !stopRequested){
        if (isBiDirectionalImaging){
            cout << "b4 preparePositioner: " << gt.read() << endl;
            preparePositioner(idxCurStack % 2 == 0);
            cout << "after preparePositioner: " << gt.read() << endl;
        }
        double timePerStack = nFramesPerStack*cycleTime + idleTimeBwtnStacks;
        double stackEndingTime = gt.read();
        double timeToWait = timePerStack*idxCurStack - stackEndingTime;
        if (timeToWait < 0){
            emit newLogMsgReady("WARNING: overrun(overall progress): idle time is too short.");
        }
        if (stackEndingTime - stackStartTime > timePerStack){
            nOverrunStacks++;
            emit newLogMsgReady("WARNING: overrun(current stack): idle time is too short.");
        }

        double maxWaitTime = 2; //in seconds. The frequency to check stop signal
    repeatWait:
        timeToWait = timePerStack*idxCurStack - gt.read();
        if (timeToWait > 0.01){
            QThread::msleep(min(maxWaitTime, timeToWait) * 1000); // *1000: sec -> ms
        }//if, need wait more than 10ms
        if (timeToWait > maxWaitTime && !stopRequested) goto repeatWait;

        if (!stopRequested) goto nextStack;
    }//if, there're more stacks and no stop requested

    //fire post-seq stimulus:
    if (applyStim){
        int postSeqStim = 0; //TODO: get this value from stim file header
        fireStimulus(postSeqStim);
    }

    aiThread->stopAcq();
    aiThread->save(*ofsAi);

    if (startCameraOnce)
        camera.stopAcq();

    if (!isUseSpool){
        ofsCam->close();
        delete ofsCam;
        ofsCam = nullptr;
    }

    ofsAi->close();

    {
        QScriptValue jsFunc = se->globalObject().property("onShutterFini");
        if (jsFunc.isFunction()) jsFunc.call();
    }

    ///disable spool
    if (isUseSpool){
        if (isAndor) ((AndorCamera*)(&camera))->enableSpool(NULL, 10); //disable spooling
        else if (isCooke || isAvt) camera.setSpooling(""); //disable spooling which also closes file
    }

    if (isPiezo){
        Piezo_Controller* p = dynamic_cast<Piezo_Controller*>(pPositioner);
        if (p && !p->dumpFeedbackData(positionerFeedbackFile)) {
            emit newStatusMsgReady("Failed to dump piezo feedback data");
        }
    }

    ///reset the actuator to its exact starting pos
    if (hasPos) {
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

void DataAcqThread::setCamera(Camera *cam) {
    // make sure that the camera knows who its parent is.
    pCamera = cam;
    if (cam != NULL) cam->parentAcqThread = this;
}

void DataAcqThread::setPositioner(Positioner *pos) {
    // make sure that the camera knows who its parent is.
    pPositioner = pos;
    if (pos != NULL) pos->parentAcqThread = this;
}

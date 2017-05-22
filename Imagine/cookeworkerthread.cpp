#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <assert.h>

#include "data_acq_thread.hpp"
#include "imagine.h"
#include "pco_errt.h"

#include <QtCore>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "cookeworkerthread.h"

CookeWorkerThread::CookeWorkerThread(QThread::Priority defaultPriority, CookeCamera * camera, QObject *parent)
    : CameraWorkerThread(defaultPriority, camera, parent) {
    this->camera = camera; //necessary to replace the generic Camera pointer assigned by the base class constructor
    prepareNextStack(-1);
}

CookeWorkerThread::~CookeWorkerThread() {}

void CookeWorkerThread::prepareNextStack(int lastBufferIdx) {
    for (int i = 0; i < camera->nBufs; i++) {
        int nextBufferIdx = (lastBufferIdx + i + 1) % camera->circBuf->capacity();
        camera->mRingBuf[i] = camera->memPool + nextBufferIdx*size_t(camera->imageSizeBytes);
        //in fifo mode, frameIdxInCamRam is 0 for all buffers
        //printPcoError(driverStatus[i]);
        camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, camera->mEvent[i], 0, 0, 0, 0, static_cast<void*>(camera->mRingBuf[i]), camera->imageSizeBytes, &((camera->driverStatus)[i])), "failed to add external buffer");// Add buffer to the driver queue
    }
}

void CookeWorkerThread::run() {
    long nEvents = 0;
    #if defined(_DEBUG)
        vector<int> nBlackFrames, blackFrameStartIndices, curFrameIndices;
        curFrameIndices.reserve(camera->nFrames);
        cerr << "enter cooke worker thread run()" << endl;
    #endif
    WORD wActSeg = 0;
    int negIndices = 0;
    int eventIdx = 1;
    bool usingSwappedEvents = false; //stores whether we are currently using a event list with inverted priority
    HANDLE currentEvents[2] = { camera->mEvent[0], camera->mEvent[1] };  //the current event sequence, highest priority first
    DWORD currentDriverStatus[2] = { camera->driverStatus[0], camera->driverStatus[1] }; //the current status sequence, highest priority first
    int waitResult;
    int nextSlot; //the index of the next empty slot in the circular buffer
    int curSlot; //the index of the next empty slot in the circular buffer
    int slotsLeft; //the number of remaining empty slots (images) in the circular buffer
    int curFrameIdx = 0; //the index of the current frame as read from the encoding in the pixel data (see camera manual)
    int lastFrameIdx = -1; //previous value of curFrameIdx.  used to determine gaps of missed frames.
    long curStackIdx = 0; //stack currently being acquired
    int nwaits = 0; //the  number of wait results that didn't match an event or time out, for debugging
    vector<long> droppedFrameIdxs, droppedFrameStackIdxs; //list of frame, stack indices of missed frames.  currently just for debugging. TODO: save this information somewhere (.imagine file?)
    double startTime;
    double lastTime;
    double curTime;
    double handlingTime = 0.0; //max handling time in seconds
    long framesSoFar = 0; //frames acquired so far (if we've missed frames, this may be less than curFrameIdx
    long stacksSoFar = 0; //stacks acquired so far
    int gapWidth=0; //missed frame gap width
    int nDroppedSpool=0; //number of frames we had to drop because spool thread could not keep up
    long counter; //frame counter of camera (extracted from image pixels)
    Timer_g gt;


    camera->totalGap = 0;
    setIsPaused(true);
    //wait until we begin recording the first stack
    unPauseLock.lock();
    getUnPaused()->wait(&unPauseLock);
    unPauseLock.unlock();

    while (true) {
        if (getShouldStop()) {
            camera->setNAcquiredFrames(0);
            camera->setNAcquiredStacks(0);
            break;
        }
        ///wait for frame ready events
        waitResult = WaitForMultipleObjects(camera->nBufs, currentEvents, false, 2000);
        while ((waitResult < WAIT_OBJECT_0 || waitResult >= WAIT_OBJECT_0 + camera->nBufs) && !getShouldStop()) {
            if (!(waitResult == 258)) nwaits += 1;
            //wait again
            waitResult = WaitForMultipleObjects(camera->nBufs, currentEvents, false, 2000);
        }
         //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
        
        startTime = gt.read();
        
        if (getShouldStop()) {
            camera->setNAcquiredFrames(0);
            camera->setNAcquiredStacks(0);
            break;
        }

        nEvents++;
        eventIdx = waitResult - WAIT_OBJECT_0;
        OutputDebugStringW((wstring(L"Event index:") + to_wstring(eventIdx) + wstring(L" (0-based)\n")).c_str());
        ///work around sdk bug
        if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal) {
            goto prepareNextEvent;
        }

        counter = camera->extractFrameCounter((Camera::PixelValue*)(camera->mRingBuf[eventIdx]));
        if (framesSoFar == 0) {
            camera->firstFrameCounter = counter;
            lastFrameIdx = -1;
            assert(counter >= 0);
        }
        else lastFrameIdx = curFrameIdx;

        curFrameIdx = counter - camera->firstFrameCounter;
        gapWidth = curFrameIdx - lastFrameIdx-1;

        if (curFrameIdx > (lastFrameIdx + 1)) {
            OutputDebugStringW((wstring(L"Fill ") + to_wstring(gapWidth) + wstring(L" frames with black images\n")).c_str());
            camera->totalGap += gapWidth;
#ifdef _DEBUG
            nBlackFrames.push_back(gapWidth);
            blackFrameStartIndices.push_back(framesSoFar);
#endif
            for (int i = lastFrameIdx; i < curFrameIdx; i++) {
                droppedFrameIdxs.push_back(i);
                droppedFrameStackIdxs.push_back(curStackIdx);
            }
        }

    #ifdef _DEBUG
        curFrameIndices.push_back(curFrameIdx);
    #endif

        if (curFrameIdx < 0) {
            OutputDebugStringW((wstring(L"detected a garbage frame\n")).c_str());
            if (++negIndices < 16) goto prepareNextEvent;//NOTE: hard coded 16
            else {
                __debugbreak();
            }
        }
        assert(curFrameIdx >= 0);

        //update circular buffer and framecount to include the frame being handled
        framesSoFar += 1;
        camera->circBufLock->lock();
        slotsLeft = camera->circBuf->capacity() - camera->circBuf->size();
        curSlot = camera->circBuf->put();
        nextSlot = (curSlot + 2) % camera->circBuf->capacity();
        OutputDebugStringW((wstring(L"Event handled for frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());

        //if we are finished with this stack, update counters, pause, and continue the loop
        if (framesSoFar == camera->nFramesPerStack && camera->genericAcqMode != Camera::eLive) {
            curStackIdx += 1;
            camera->setNAcquiredStacks(curStackIdx); //assumes we will never miss a whole stack
            framesSoFar = 0;
            camera->setNAcquiredFrames(framesSoFar);
            camera->circBufLock->unlock();
            camera->safe_pco(PCO_CancelImages(camera->hCamera), "failed to stop camera");
            camera->safe_pco(PCO_SetRecordingState(camera->hCamera, 0), "failed to stop camera recording");// stop recording
            prepareNextStack(curSlot);
            for (int i = 0; i < 2; i++) {
                currentEvents[i] = camera->mEvent[i];
                currentDriverStatus[i] = camera->driverStatus[i];
            }
            usingSwappedEvents = false;
            pauseUntilWake();
            continue;
        }
        else {
            camera->setNAcquiredFrames(framesSoFar);
        }

        //never fill the last two slots.  drop the frame instead.
        if (slotsLeft <= 2) {
            OutputDebugStringW((wstring(L"Dropping frame #") + to_wstring(curFrameIdx) + wstring(L"because buffer is full\n")).c_str());
            nextSlot = camera->circBuf->peekPut();
            camera->circBufLock->unlock();
            nDroppedSpool++;
            goto prepareNextEvent;
        }
        camera->circBufLock->unlock();

        
        /*curTime = gt.read();
        OutputDebugStringW((wstring(L"Time to update circbuf (microseconds): ") + to_wstring((curTime-lastTime) * 1000000) + wstring(L"\n")).c_str());
        lastTime = gt.read();*/

        //fill the gap w/ black images
        /*for (int frameIdx = camera->camera->nAcquiredFrames; frameIdx < min(camera->nFrames, curFrameIdx); ++frameIdx){
        if (camera->genericAcqMode == Camera::eAcqAndSave){
        append2seq(camera->pBlackImage, frameIdx, nPixelsPerFrame);
        }
        gapWidth++;
        }*/

        //if we've made it this far, we know that we have more frames left to handle for the current stack
    prepareNextEvent:

        ///then add back the buffer if we need it to finish the stack, and raise the priority of the other frame event
        //(WaitForMultipleObjects prioritizes based on order in the event array when two events occure nearly-simultaneously)
        if (framesSoFar < (camera->nFramesPerStack - 1) || camera->genericAcqMode == Camera::eLive) {
            camera->mRingBuf[eventIdx] = camera->memPool + nextSlot*size_t(camera->imageSizeBytes);
            camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, currentEvents[eventIdx], wActSeg, 0, 0, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &(currentDriverStatus[eventIdx])), "failed to add external buffer");// Add buffer to the driver queue
            if (eventIdx == 0) { //if the first-priority frame event was handled (should be most of the time)
                if (usingSwappedEvents) {
                    for (int i = 0; i < 2; i++) {
                        currentEvents[i] = camera->mEvent[i];
                        currentDriverStatus[i] = camera->driverStatus[i];
                    }
                }
                else {
                    for (int i = 0; i < 2; i++) {
                        currentEvents[i] = camera->mEvent[!i];
                        currentDriverStatus[i] = camera->driverStatus[!i];
                    }
                }
                //and swap the ring buf entries
                char * temp = camera->mRingBuf[0];
                camera->mRingBuf[0] = camera->mRingBuf[1];
                camera->mRingBuf[1] = temp;
                usingSwappedEvents = !usingSwappedEvents;
            }

            if (camera->errorCode != PCO_NOERROR) {
                camera->errorMsg = "failed to add buffer";
                break; //break the while
            }
            if (framesSoFar > 0)  handlingTime = gt.read() - startTime; // note that first frame is always slow due to pause
            OutputDebugStringW((wstring(L"\nFrame event handling time (microseconds): ") + to_wstring(handlingTime*1000000) + wstring(L"\n")).c_str());
        }
        else {
            continue; // break;
        }
    }//while,

     //finished recording now, cancel images and stop camera
    camera->safe_pco(PCO_CancelImages(camera->hCamera), "failed to stop camera");
    camera->safe_pco(PCO_SetRecordingState(camera->hCamera, 0), "failed to stop camera recording");// stop recording
    OutputDebugStringW((wstring(L"Dropped ") + to_wstring(camera->totalGap) + wstring(L" frames total because workerthread could not keep up\n")).c_str());
    OutputDebugStringW((wstring(L"Dropped ") + to_wstring(nDroppedSpool) + wstring(L" frames total because spoolthread could not keep up\n")).c_str());

    #if defined(_DEBUG)
    DataAcqThread::genSquareSpike(30);
        cerr << "black frame info:" << endl;
        for (unsigned i = 0; i < nBlackFrames.size(); ++i) {
            cerr << nBlackFrames[i] << "@" << blackFrameStartIndices[i] << " \t";
            if (i % 8 == 7) cerr << endl;
        }
        cerr << "end of black frame info" << endl;
    #endif
}//run(),


DummyWorkerThread::DummyWorkerThread(QThread::Priority defaultPriority, DummyCamera * camera, QObject *parent)
    : CameraWorkerThread(defaultPriority, camera, parent) {
    this->camera = camera; //necessary to replace the generic Camera pointer assigned by the base class constructor
    prepareNextStack(-1);
}

DummyWorkerThread::~DummyWorkerThread() {}

void DummyWorkerThread::prepareNextStack(int lastBufferIdx) {
    for (int i = 0; i < camera->nBufs; i++) {
        int nextBufferIdx = (lastBufferIdx + i + 1) % camera->circBuf->capacity();
        camera->mRingBuf[i] = camera->memPool + nextBufferIdx*size_t(camera->imageSizeBytes);
        // dummy: camera->dummyCamera->AddBufferExtern(camera->hCamera, camera->mEvent[i], 0, 0, 0, 0, static_cast<void*>(camera->mRingBuf[i]), camera->imageSizeBytes, &((camera->driverStatus)[i])), "failed to add external buffer");// Add buffer to the driver queue
        camera->sudoCamera->AddBufferExtern(camera->mEvent[i], camera->mRingBuf[i], camera->imageSizeBytes, &((camera->driverStatus)[i]));
    }
}

void DummyWorkerThread::run() {
    long nEvents = 0;
#if defined(_DEBUG)
    vector<int> nBlackFrames, blackFrameStartIndices, curFrameIndices;
    curFrameIndices.reserve(camera->nFrames);
    cerr << "enter dummy worker thread run()" << endl;
#endif
    int negIndices = 0;
    int eventIdx = 1;
    bool usingSwappedEvents = false; //stores whether we are currently using a event list with inverted priority
    HANDLE currentEvents[2] = { camera->mEvent[0], camera->mEvent[1] };  //the current event sequence, highest priority first
    DWORD currentDriverStatus[2] = { camera->driverStatus[0], camera->driverStatus[1] }; //the current status sequence, highest priority first
    int waitResult;
    int nextSlot; //the index of the next empty slot in the circular buffer
    int curSlot; //the index of the next empty slot in the circular buffer
    int slotsLeft; //the number of remaining empty slots (images) in the circular buffer
    int curFrameIdx = 0; //the index of the current frame as read from the encoding in the pixel data (see camera manual)
    int lastFrameIdx = -1; //previous value of curFrameIdx.  used to determine gaps of missed frames.
    long curStackIdx = 0; //stack currently being acquired
    int nwaits = 0; //the  number of wait results that didn't match an event or time out, for debugging
    vector<long> droppedFrameIdxs, droppedFrameStackIdxs; //list of frame, stack indices of missed frames.  currently just for debugging. TODO: save this information somewhere (.imagine file?)
    double startTime;
    double lastTime;
    double curTime;
    double handlingTime = 0.0; //max handling time in seconds
    long framesSoFar = 0; //frames acquired so far (if we've missed frames, this may be less than curFrameIdx
    long stacksSoFar = 0; //stacks acquired so far
    int gapWidth = 0; //missed frame gap width
    int nDroppedSpool = 0; //number of frames we had to drop because spool thread could not keep up
    long counter; //frame counter of camera (extracted from image pixels)
    Timer_g gt;


    camera->totalGap = 0;
    setIsPaused(true);
    //wait until we begin recording the first stack
    unPauseLock.lock();
    getUnPaused()->wait(&unPauseLock);
    unPauseLock.unlock();

    while (true) {
        if (getShouldStop()) {
            camera->setNAcquiredFrames(0);
            camera->setNAcquiredStacks(0);
            break;
        }

        ///wait for frame ready events
        waitResult = WaitForMultipleObjects(camera->nBufs, currentEvents, false, 2000);
        while ((waitResult < WAIT_OBJECT_0 || waitResult >= WAIT_OBJECT_0 + camera->nBufs) && !getShouldStop()) {
            if (!(waitResult == 258)) nwaits += 1;
            //wait again
            waitResult = WaitForMultipleObjects(camera->nBufs, currentEvents, false, 2000);
        }
        //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()

        startTime = gt.read();

        if (getShouldStop()) {
            camera->setNAcquiredFrames(0);
            camera->setNAcquiredStacks(0);
            break;
        }

        nEvents++;
        eventIdx = waitResult - WAIT_OBJECT_0;
        OutputDebugStringW((wstring(L"Event index:") + to_wstring(eventIdx) + wstring(L" (0-based)\n")).c_str());
        ///work around sdk bug
        if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal) {
            goto prepareNextEvent;
        }

        counter = camera->extractFrameCounter((Camera::PixelValue*)(camera->mRingBuf[eventIdx]));
        if (framesSoFar == 0) {
            camera->firstFrameCounter = counter;
            lastFrameIdx = -1;
            assert(counter >= 0);
        }
        else lastFrameIdx = curFrameIdx;

        curFrameIdx = counter - camera->firstFrameCounter;
        gapWidth = curFrameIdx - lastFrameIdx - 1;

        if (curFrameIdx > (lastFrameIdx + 1)) {
            OutputDebugStringW((wstring(L"Fill ") + to_wstring(gapWidth) + wstring(L" frames with black images\n")).c_str());
            camera->totalGap += gapWidth;
#ifdef _DEBUG
            nBlackFrames.push_back(gapWidth);
            blackFrameStartIndices.push_back(framesSoFar);
#endif
            for (int i = lastFrameIdx; i < curFrameIdx; i++) {
                droppedFrameIdxs.push_back(i);
                droppedFrameStackIdxs.push_back(curStackIdx);
            }
        }

#ifdef _DEBUG
        curFrameIndices.push_back(curFrameIdx);
#endif

        if (curFrameIdx < 0) {
            OutputDebugStringW((wstring(L"detected a garbage frame\n")).c_str());
            if (++negIndices < 16) goto prepareNextEvent;//NOTE: hard coded 16
            else {
                __debugbreak();
            }
        }
        assert(curFrameIdx >= 0);

        //update circular buffer and framecount to include the frame being handled
        framesSoFar += 1;
        camera->circBufLock->lock();
        slotsLeft = camera->circBuf->capacity() - camera->circBuf->size();
        curSlot = camera->circBuf->put();
        nextSlot = (curSlot + 2) % camera->circBuf->capacity();
        OutputDebugStringW((wstring(L"Event handled for frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());

        //if we are finished with this stack, update counters, pause, and continue the loop
        if (framesSoFar == camera->nFramesPerStack && camera->genericAcqMode != Camera::eLive) {
            curStackIdx += 1;
            camera->setNAcquiredStacks(curStackIdx); //assumes we will never miss a whole stack
            framesSoFar = 0;
            camera->setNAcquiredFrames(framesSoFar);
            camera->circBufLock->unlock();
            camera->sudoCamera->CancelImages();
            camera->sudoCamera->SetRecordingState(false);// stop recording
            prepareNextStack(curSlot);
            for (int i = 0; i < 2; i++) {
                currentEvents[i] = camera->mEvent[i];
                currentDriverStatus[i] = camera->driverStatus[i];
            }
            usingSwappedEvents = false;
            pauseUntilWake();
            continue;
        }
        else {
            camera->setNAcquiredFrames(framesSoFar);
        }

        //never fill the last two slots.  drop the frame instead.
        if (slotsLeft <= 2) {
            OutputDebugStringW((wstring(L"Dropping frame #") + to_wstring(curFrameIdx) + wstring(L"because buffer is full\n")).c_str());
            nextSlot = camera->circBuf->peekPut();
            camera->circBufLock->unlock();
            nDroppedSpool++;
            goto prepareNextEvent;
        }
        camera->circBufLock->unlock();


        //if we've made it this far, we know that we have more frames left to handle for the current stack
    prepareNextEvent:

        ///then add back the buffer if we need it to finish the stack, and raise the priority of the other frame event
        //(WaitForMultipleObjects prioritizes based on order in the event array when two events occure nearly-simultaneously)
        if (framesSoFar < (camera->nFramesPerStack - 1) || camera->genericAcqMode == Camera::eLive) {
            camera->mRingBuf[eventIdx] = camera->memPool + nextSlot*size_t(camera->imageSizeBytes);
            // camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, currentEvents[eventIdx], wActSeg, 0, 0, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &(currentDriverStatus[eventIdx])), "failed to add external buffer");// Add buffer to the driver queue
            camera->sudoCamera->AddBufferExtern(currentEvents[eventIdx], camera->mRingBuf[eventIdx], camera->imageSizeBytes, &(currentDriverStatus[eventIdx]));
            if (eventIdx == 0) { //if the first-priority frame event was handled (should be most of the time)
                if (usingSwappedEvents) {
                    for (int i = 0; i < 2; i++) {
                        currentEvents[i] = camera->mEvent[i];
                        currentDriverStatus[i] = camera->driverStatus[i];
                    }
                }
                else {
                    for (int i = 0; i < 2; i++) {
                        currentEvents[i] = camera->mEvent[!i];
                        currentDriverStatus[i] = camera->driverStatus[!i];
                    }
                }
                //and swap the ring buf entries
                char * temp = camera->mRingBuf[0];
                camera->mRingBuf[0] = camera->mRingBuf[1];
                camera->mRingBuf[1] = temp;
                usingSwappedEvents = !usingSwappedEvents;
            }

            if (camera->errorCode != PCO_NOERROR) {
                camera->errorMsg = "failed to add buffer";
                break; //break the while
            }
            if (framesSoFar > 0)  handlingTime = gt.read() - startTime; // note that first frame is always slow due to pause
            OutputDebugStringW((wstring(L"\nFrame event handling time (microseconds): ") + to_wstring(handlingTime * 1000000) + wstring(L"\n")).c_str());
        }
        else {
            continue; // break;
        }
    }//while,

     //finished recording now, cancel images and stop camera
    camera->sudoCamera->CancelImages();
    camera->sudoCamera->SetRecordingState(false);// stop recording
    OutputDebugStringW((wstring(L"Dropped ") + to_wstring(camera->totalGap) + wstring(L" frames total because workerthread could not keep up\n")).c_str());
    OutputDebugStringW((wstring(L"Dropped ") + to_wstring(nDroppedSpool) + wstring(L" frames total because spoolthread could not keep up\n")).c_str());

#if defined(_DEBUG)
    DataAcqThread::genSquareSpike(30);
    cerr << "black frame info:" << endl;
    for (unsigned i = 0; i < nBlackFrames.size(); ++i) {
        cerr << nBlackFrames[i] << "@" << blackFrameStartIndices[i] << " \t";
        if (i % 8 == 7) cerr << endl;
    }
    cerr << "end of black frame info" << endl;
#endif
}//run(),
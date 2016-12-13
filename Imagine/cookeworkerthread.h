#ifndef COOKEWORKERTHREAD_H
#define COOKEWORKERTHREAD_H

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <assert.h>

#include "cooke.hpp"
#include "lockguard.h"
#include "spoolthread.h"
#include "data_acq_thread.hpp"
#include "imagine.h"
#include "pco_errt.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

void genSquareSpike(int);

class CookeWorkerThread : public QThread {
    Q_OBJECT
private:
    CookeCamera* camera;

    //camera->isRecording is modified when camera buffers are removed (i.e. between stacks)
    //shouldStop is modified when the acquisition is complete (i.e. after a sequence of stacks)
    volatile bool shouldStop;
    std::atomic_bool shouldPause;
    std::atomic_bool isPaused;

public:
    CookeWorkerThread(CookeCamera * camera, QObject *parent = 0)
        : QThread(parent){
        this->camera = camera;

        Timer_g gt = this->camera->parentAcqThread->parentImagine->gTimer;

        shouldStop = false;
        //it's important that the thread begin in the paused state, otherwise the camera driver throws unintelligible errors
        //this seems to be because the driver doesn't like receiving an event that we are already waiting for
        shouldPause = true;
        isPaused = true;

        //cout<<"after _aligned_malloc: "<<gt.read()<<endl;
#ifdef _WIN64
        assert((unsigned long long)(camera->memPool) % (1024 * 64) == 0);
#else
        assert((unsigned long)(camera->memPool) % (1024 * 64) == 0);
#endif


        cout << "at the end of workerThread->ctor(): " << gt.read() << endl;
    }

    ~CookeWorkerThread(){}

    void requestStop(){shouldStop = true;}

    void pause() {
        shouldPause = true;
        while (!isPaused) {
            QThread::msleep(10); //milliseconds
        }
    }

    //resume acquisition.  This is called before every stack (except the first) in a multi-stack acquisitoin
    //before calling this make sure that buffers have been added back via PCO_AddBufferExtern
    void resume() {
        if (isPaused) {
            shouldPause = false;
            isPaused = false;
        }
    }

    void run(){
        long nEvents = 0;
#if defined(_DEBUG)
        vector<int> nBlackFrames, blackFrameStartIndices, curFrameIndices;
        curFrameIndices.reserve(camera->nFrames);
        cerr << "enter cooke worker thread run()" << endl;
#endif
        WORD wActSeg = 0;
        int negIndices = 0;
        Timer_g gt = this->camera->parentAcqThread->parentImagine->gTimer;
        int eventIdx = 1;
        HANDLE mEventSwapped[2] = {camera->mEvent[1], camera-> mEvent[0]};
        int waitResult;
        int nextSlot; //the index of the next empty slot in the circular buffer
        int slotsLeft; //the number of remaining empty slots (images) in the circular buffer
        int curFrameIdx = 0; //the index of the current frame as read from the encoding in the pixel data (see camera manual)
        long curStackIdx = 0; //stack currently being acquired
        int nwaits = 0; //the  number of wait results that didn't match an event or time out
        long droppedFrameCount = 0; // number of dropped frames this stack
        vector<long> droppedFrameIdxs, droppedFrameStackIdxs;
        double startTime;
        double handlingTime = 0.0; //max handling time in seconds
        long framesSoFar=0; //frames acquired so far (if we've missed frames, this may be less than curFrameIdx
        long stacksSoFar = 0; //stacks acquired so far
        int gapWidth; //missed frame gap width
        long counter; //frame counter of camera (extracted from image pixels)
        //CookeCamera::PixelValue *rawPointers[] = {(Camera::PixelValue*)camera->mRingBuf[0], (Camera::PixelValue*)camera->mRingBuf[1] };
        //CookeCamera::PixelValue *rawData; //pointer to currently handled frame buffer

        while (true) {
            if (isPaused) { //Wait until resumed.
                //TODO: wait for a Qt event instead
                if (shouldStop) break;
                QThread::msleep(1); //milliseconds
                continue;
            }
            //if(shouldStop) break;
            ///wait for events
            if (eventIdx == 1) { // if the previous frame was from event 1, set event 0 as first priority
                waitResult = WaitForMultipleObjects(camera->nBufs, camera->mEvent, false, 2000);
            }
            else { //if the previous frame was from event 0, set event 1 as first priority
                waitResult = WaitForMultipleObjects(camera->nBufs, mEventSwapped, false, 2000);
                if (waitResult == 0 || waitResult == 1) waitResult = (int)!(bool(waitResult)); //keep indices relative to mEvent
            }
            //TODO: the logic block below is what causes external acquisitions to time out. maybe fixed?
            while ((waitResult < WAIT_OBJECT_0 || waitResult >= WAIT_OBJECT_0 + camera->nBufs) && !shouldStop && !shouldPause) {
                if (!(waitResult == 258)) nwaits += 1;
                //wait again
                if (eventIdx == 1) {
                    waitResult = WaitForMultipleObjects(camera->nBufs, camera->mEvent, false, 2000);
                }
                else {
                    waitResult = WaitForMultipleObjects(camera->nBufs, mEventSwapped, false, 2000);
                    if (waitResult == 0 || waitResult == 1) waitResult = (int)!(bool(waitResult)); //keep indices relative to mEvent
                }
            }//if, WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
           //OutputDebugStringW((wstring(L"Begin processing frame: ") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
            startTime = gt.read();
            //TODO: the two conditionals below are repeated in prepareNextEvent.  A bit ugly, could improve.
            if (shouldStop) break;
            if (shouldPause) {
                camera->safe_pco(PCO_CancelImages(camera->hCamera), "failed to stop camera");
                //ResetEvent(camera->mEvent[0]);
                //ResetEvent(camera->mEvent[1]);
                isPaused = true; //TODO: maybe better to use a Qt wait event to avoid the sleep statement in beginning of the while loop above?
                continue;
            }

            nEvents++;
            eventIdx = waitResult - WAIT_OBJECT_0;
            ///work around sdk bug
            if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal) {
                goto prepareNextEvent;
            }

            if (camera->isRecording) {
                //rawData = (Camera::PixelValue*)(camera->mRingBuf[eventIdx]);
                //OutputDebugStringW((wstring(L"Time before extract frame counter:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //wow, this takes as much as 260 microseconds sometimes -- but usually about 90 microseconds
                counter = camera->extractFrameCounter((Camera::PixelValue*)(camera->mRingBuf[eventIdx]));
                //OutputDebugStringW((wstring(L"Time after extract frame counter:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the two cases below can come up since we don't stop this thread between stacks
                if (counter == 0 || (counter > camera->nFrames && camera->genericAcqMode != Camera::eLive)) {
                    //reset event
                    //ResetEvent(camera->mEvent[eventIdx]);
                    continue;
                }
                if (framesSoFar == 0) {
                    camera->firstFrameCounter = counter;
                    assert(counter >= 0);
                    //cout << "first frame's counter is " << counter << ", at time: " << gt.read() << endl;
                    genSquareSpike(20);
                }
                curFrameIdx = counter - camera->firstFrameCounter;
                gapWidth = curFrameIdx - framesSoFar;

                if (curFrameIdx > (framesSoFar + 1)) {
                    for (int i = framesSoFar; i < curFrameIdx; i++) {
                        //curStack = camera->nAcquiredStacks.load();
                        //OutputDebugStringW((wstring(L"Missed frame #") + to_wstring(i) + wstring(L"(0-based) ")).c_str());
                        //OutputDebugStringW((wstring(L"of stack #") + to_wstring(curStack) + wstring(L"(0-based)\n")).c_str());
                        droppedFrameIdxs.push_back(i);
                        droppedFrameStackIdxs.push_back(curStackIdx);
                        droppedFrameCount += 1;
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
                nextSlot = (camera->circBuf->put() + 2) % camera->circBuf->capacity();
                //OutputDebugStringW((wstring(L"Event handled for frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());
                //OutputDebugStringW((wstring(L"Handled ") + to_wstring(camera->nAcquiredFrames) + wstring(L" frames total for this stack\n")).c_str());

                //if we are finished with this stack, update counters, reset event, pause, and continue the loop
                if (framesSoFar == camera->nFramesPerStack && camera->genericAcqMode != Camera::eLive) {
                    curStackIdx += 1;
                    camera->nAcquiredStacks = curStackIdx; //assumes we will never miss a whole stack
                    framesSoFar = 0;
                    camera->nAcquiredFrames = framesSoFar;
                    camera->circBufLock->unlock();
                    //in theory we should only have to reset the event at eventIdx, but just to be safe...
                    //ResetEvent(camera->mEvent[0]);
                    //ResetEvent(camera->mEvent[1]);
                    camera->safe_pco(PCO_CancelImages(camera->hCamera), "failed to stop camera");
                    isPaused = true;
                    continue;
                }
                else {
                    camera->nAcquiredFrames = framesSoFar;
                }

                //never fill the last two slots.  drop the frame instead.
                if (slotsLeft <= 2) {
                    OutputDebugStringW((wstring(L"Dropping frame #") + to_wstring(curFrameIdx) + wstring(L"because buffer is full\n")).c_str());
                    nextSlot = camera->circBuf->peekPut();
                    camera->circBufLock->unlock();
                    goto prepareNextEvent;
                }
                camera->circBufLock->unlock();

                //fill the gap w/ black images
                //int gapWidth = 0;
                //TODO: use a timer to decide when we've missed a frame.
                /*
                for (int frameIdx = camera->camera->nAcquiredFrames; frameIdx < min(camera->nFrames, curFrameIdx); ++frameIdx){
                    if (camera->genericAcqMode == Camera::eAcqAndSave){
                        append2seq(camera->pBlackImage, frameIdx, nPixelsPerFrame);
                    }
                    gapWidth++;
                }
                */

                if (gapWidth) {
                    //tmp: __debugbreak();
                    OutputDebugStringW((wstring(L"Fill ") + to_wstring(gapWidth) + wstring(L" frames with black images\n")).c_str());
                    //cout << "fill " << gapWidth << " frames with black images (start at frame idx=" << camera->nAcquiredFrames << ")" << endl;
                    camera->totalGap += gapWidth;
#ifdef _DEBUG
                    nBlackFrames.push_back(gapWidth);
                    blackFrameStartIndices.push_back(framesSoFar);
#endif
                }
            } // if(!shouldStop)

        //if we've made it this far, we know that we have more frames left to handle for the current stack
        prepareNextEvent:

            //reset event
            //ResetEvent(camera->mEvent[eventIdx]);

            if (shouldStop) break;
            if (shouldPause) {
                camera->safe_pco(PCO_CancelImages(camera->hCamera), "failed to stop camera");
                //in theory we should only have to reset the event at eventIdx, but just to be safe...
                //ResetEvent(camera->mEvent[0]);
                //ResetEvent(camera->mEvent[1]);
                isPaused = true; //TODO: maybe better to use a Qt wait event to avoid the sleep statement in beginning of the while loop above?
                continue;
            }

            ///then add back the buffer if we need it to finish the stack
            if (framesSoFar < (camera->nFramesPerStack- 1) || camera->genericAcqMode == Camera::eLive){
                //int temp_ = nextSlot*size_t(camera->imageSizeBytes);
                camera->mRingBuf[eventIdx] = camera->memPool + nextSlot*size_t(camera->imageSizeBytes);
                //rawPointers[eventIdx] = (Camera::PixelValue*)(camera->memPool + nextSlot*size_t(camera->imageSizeBytes));
                //in fifo mode, frameIdxInCamRam are 0 for both buffers?
                //int frameIdxInCamRam = 0;
                //OutputDebugStringW((wstring(L"Next frame pointer:") + to_wstring((unsigned)static_cast<void*>(camera->mRingBuf[eventIdx]))).c_str());
                //OutputDebugStringW((wstring(L"Time before add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the line below seeems to take at most 74 microseconds to execute on OCPI2 (assuming this doesn't depend on image size, which it shouldn't)
                camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, camera->mEvent[eventIdx], wActSeg, 0, 0, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &(camera->driverStatus[eventIdx])), "failed to add external buffer");// Add buffer to the driver queue

                //OutputDebugStringW((wstring(L"Time after add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //TODO: switch to PCO_AddBufferExtern to improve performance
                //camera->safe_pco(PCO_AddBufferEx(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx], camera->getImageWidth(), camera->getImageHeight(), bytesPerPixel), "failed to add buffer");// Add buffer to the driver queue
                //camera->safe_pco(PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]), "failed to add buffer");// Add buffer to the driver queue
                if (camera->errorCode != PCO_NOERROR) {
                    camera->errorMsg = "failed to add buffer";
                    break; //break the while
                }
                //OutputDebugStringW((wstring(L"End processing frame: ") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                if (framesSoFar >0)  handlingTime = max(gt.read() - startTime, handlingTime); //first is always slow due to pause
                //OutputDebugStringW((wstring(L"Frame event handling time (microseconds): ") + to_wstring(handlingTime*1000000) + wstring(L"\n")).c_str());
            }
            else {
                continue; // break;
            }
        }//while,
#if defined(_DEBUG)
        cerr << "leave cooke worker thread run() @ time " << gt.read() << endl;
        genSquareSpike(30);
        cerr << "black frame info:" << endl;
        for (unsigned i = 0; i < nBlackFrames.size(); ++i){
            cerr << nBlackFrames[i] << "@" << blackFrameStartIndices[i] << " \t";
            if (i % 8 == 7) cerr << endl;
        }
        cerr << "end of black frame info" << endl;
#endif
    }//run(),
};//class, CookeWorkerThread


#endif // COOKEWORKERTHREAD_H

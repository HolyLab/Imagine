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

public:
    CookeWorkerThread(CookeCamera * camera, QObject *parent = 0)
        : QThread(parent){
        this->camera = camera;

        Timer_g gt = this->camera->parentAcqThread->parentImagine->gTimer;

        shouldStop = false;

        //cout<<"after _aligned_malloc: "<<gt.read()<<endl;
#ifdef _WIN64
        assert((unsigned long long)circBufData % (1024 * 64) == 0);
#else
        assert((unsigned long)circBufData % (1024 * 64) == 0);
#endif


        cout << "at the end of workerThread->ctor(): " << gt.read() << endl;
    }

    ~CookeWorkerThread(){}

    void requestStop(){shouldStop = true;}

    void run(){
        long nEvents = 0;
#if defined(_DEBUG)
        vector<int> nBlackFrames, blackFrameStartIndices, curFrameIndices;
        curFrameIndices.reserve(camera->nFrames);
        cerr << "enter cooke worker thread run()" << endl;
#endif
        DWORD dwStatusDll=0;
        DWORD dwStatusDrv=0;
        WORD wActSeg=0;
        int negIndices = 0;
        Timer_g gt = this->camera->parentAcqThread->parentImagine->gTimer;
        int eventIdx = 1;
        HANDLE mEventSwapped[2] = {camera->mEvent[1], camera-> mEvent[0]};
        int waitResult;
        int currentSlot;
        long frameCount = 0;
        int curFrameIdx = 0;

        while (true){
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
            while ((waitResult < WAIT_OBJECT_0 || waitResult >= WAIT_OBJECT_0 + camera->nBufs) && !shouldStop) {
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

            if (shouldStop) break;
            nEvents++;
            eventIdx = waitResult - WAIT_OBJECT_0;
            ///work around sdk bug
            if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal){
                goto prepareNextEvent;
            }

            if (camera->isRecording) {
                CookeCamera::PixelValue* rawData = camera->mRingBuf[eventIdx];
                //OutputDebugStringW((wstring(L"Time before extract frame counter:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //wow, this takes as much as 260 microseconds sometimes -- but usually about 90 microseconds
                long counter = camera->extractFrameCounter(rawData);
                //OutputDebugStringW((wstring(L"Time after extract frame counter:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the two cases below can come up since we don't stop this thread between stacks
                if (counter == 0 || (counter > camera->nFrames && camera->genericAcqMode != Camera::eLive)) {
                    //reset event
                    ResetEvent(camera->mEvent[eventIdx]);
                    continue;
                }
                if (camera->nAcquiredFrames == 0) {
                    camera->firstFrameCounter = counter;
                    assert(counter >= 0);
                    cout << "first frame's counter is " << counter << ", at time: " << gt.read() << endl;
                    genSquareSpike(20);
                }
                curFrameIdx = counter - camera->firstFrameCounter;

#ifdef _DEBUG
                curFrameIndices.push_back(curFrameIdx);
#endif

                if (curFrameIdx < 0) {
                    if (++negIndices < 16) goto prepareNextEvent;//NOTE: hard coded 16
                    else {
                        __debugbreak();
                    }
                }
                assert(curFrameIdx >= 0);

                camera->circBufLock->lock();
                int slotsLeft = camera->circBuf->capacity() - camera->circBuf->size();
                //never fill the last slot.  drop the frame instead.
                if (slotsLeft == 1) {
                    OutputDebugStringW((wstring(L"Dropping frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());
                    camera->circBufLock->unlock();
                    goto prepareNextEvent;
                }
                camera->circBuf->put();
                camera->circBufLock->unlock();

                //fill the gap w/ black images
                int gapWidth = 0;
                //TODO: use a timer to decide when we've missed a frame.
                /*
                for (int frameIdx = camera->nAcquiredFrames; frameIdx < min(camera->nFrames, curFrameIdx); ++frameIdx){
                    if (camera->genericAcqMode == Camera::eAcqAndSave){
                        append2seq(camera->pBlackImage, frameIdx, nPixelsPerFrame);
                    }
                    gapWidth++;
                }
                */

                if (gapWidth) {
                    //tmp: __debugbreak();
                    OutputDebugStringW((wstring(L"Fill ") + to_wstring(gapWidth) + wstring(L" frames with black images")).c_str());
                    //cout << "fill " << gapWidth << " frames with black images (start at frame idx=" << camera->nAcquiredFrames << ")" << endl;
                    camera->totalGap += gapWidth;
#ifdef _DEBUG
                    nBlackFrames.push_back(gapWidth);
                    blackFrameStartIndices.push_back(camera->nAcquiredFrames);
#endif
                }
            } // if(!shouldStop)
        prepareNextEvent:

            //reset event
            ResetEvent(camera->mEvent[eventIdx]);

            if (shouldStop) break;

            ///then add back the buffer
            if (camera->nAcquiredFrames < camera->nFrames || camera->genericAcqMode == Camera::eLive){
                currentSlot = camera->circBuf->peekPut();
                camera->mRingBuf[eventIdx] = (CookeCamera::PixelValue*)(camera->memPool + currentSlot*size_t(camera->imageSizeBytes));
                //in fifo mode, frameIdxInCamRam are 0 for both buffers?
                int frameIdxInCamRam = 0;
                //OutputDebugStringW((wstring(L"Time before add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the line below seeems to take at most 74 microseconds to execute on OCPI2 (assuming this doesn't depend on image size, which it shouldn't)
                camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, camera->mEvent[eventIdx], 0, frameIdxInCamRam, frameIdxInCamRam, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &dwStatusDrv), "failed to add external buffer");// Add buffer to the driver queue
                OutputDebugStringW((wstring(L"Event handled for frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());
                frameCount += 1;
                OutputDebugStringW((wstring(L"Handled ") + to_wstring(frameCount) + wstring(L" frames total\n")).c_str());
                //OutputDebugStringW((wstring(L"Time after add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //TODO: switch to PCO_AddBufferExtern to improve performance
                //camera->safe_pco(PCO_AddBufferEx(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx], camera->getImageWidth(), camera->getImageHeight(), bytesPerPixel), "failed to add buffer");// Add buffer to the driver queue
                //camera->safe_pco(PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]), "failed to add buffer");// Add buffer to the driver queue
                if (camera->errorCode != PCO_NOERROR) {
                    camera->errorMsg = "failed to add buffer";
                    break; //break the while
                }
                //OutputDebugStringW((wstring(L"End processing frame: ") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
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

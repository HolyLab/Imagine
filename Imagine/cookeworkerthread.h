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
        friend class SpoolThread;
private:
    CookeCamera* camera;
    SpoolThread* spoolingThread;

    //camera->isRecording is modified when camera buffers are removed (i.e. between stacks)
    //shouldStop is modified when the acquisition is complete (i.e. after a sequence of stacks)
    volatile bool shouldStop;

public:
    CookeWorkerThread(CookeCamera * camera, QObject *parent = 0)
        : QThread(parent){
        this->camera = camera;

        Timer_g gt = this->camera->parentAcqThread->parentImagine->gTimer;

        if (camera->isSpooling()){
            spoolingThread = new SpoolThread(camera->ofsSpooling,
                camera->getImageWidth()*camera->getImageHeight()*sizeof(CookeCamera::PixelValue),
                this);
            cout << "after new spoolthread: " << gt.read() << endl;

            spoolingThread->start(QThread::NormalPriority);
        }
        else {
            spoolingThread = nullptr;
        }

        shouldStop = false;

        cout << "at the end of workerThread->ctor(): " << gt.read() << endl;
    }

    ~CookeWorkerThread(){
        if (spoolingThread){
            spoolingThread->wait();
            delete spoolingThread;
            spoolingThread = nullptr;
        }
    }

    void requestStop(){
        shouldStop = true;
        if (spoolingThread) {
            spoolingThread->requestStop();
            spoolingThread->wait();
        }
    }

    void append2seq(CookeCamera::PixelValue* frame, int frameIdx){
        if (spoolingThread){
            spoolingThread->appendItem((char*)frame);
        }
        else {
            memcpy(camera->pImageArray + frameIdx*camera->imageSizePixels, frame, sizeof(Camera::PixelValue)*camera->imageSizePixels);
        }
    }

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
        long frameCount = 0;
        int eventIdx = 1;
        HANDLE mEventSwapped[2] = {camera->mEvent[1], camera-> mEvent[0]};
        int waitResult;

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
            CLockGuard tGuard(camera->mpLock); //wraps and acquires the lock

            /*
            camera->safe_pco(PCO_GetBufferStatus(camera->hCamera, camera->mBufIndex[eventIdx], &dwStatusDll, &dwStatusDrv), "buffer status returned error");
            if (dwStatusDrv != 0) {
                char msg[16384];
                PCO_GetErrorText(dwStatusDrv, msg, 16384);
                cout << msg << endl;
            }
            */
            /*
            if (err != 0) {
                cerr << "PCO_GetBufferStatus error: " << hex << err << endl;
                break;
            }

            if (dwStatusDrv != 0) {
                cerr << "PCO_GetBufferStatus dwStatusDrv(hex): " << hex << dwStatusDrv << endl;
                break;
            }
            */



            ///work around sdk bug
            if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal){
                goto skipSponEvent;
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
                    tGuard.unlock();
                    continue;
                }
                if (camera->nAcquiredFrames == 0) {
                    camera->firstFrameCounter = counter;
                    assert(counter >= 0);
                    cout << "first frame's counter is " << counter << ", at time: " << gt.read() << endl;
                    genSquareSpike(20);
                }
                int curFrameIdx = counter - camera->firstFrameCounter;

#ifdef _DEBUG
                curFrameIndices.push_back(curFrameIdx);
#endif


                if (curFrameIdx < 0) {
                    if (++negIndices < 16) goto skipSponEvent;//NOTE: hard coded 16
                    else {
                        __debugbreak();
                    }
                }
                assert(curFrameIdx >= 0);


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

                //fill the frame array and live image
                if (curFrameIdx < camera->nFrames) { //nFrames is the number of frames per stack
                    if (camera->genericAcqMode == Camera::eAcqAndSave) {
                        //OutputDebugStringW((wstring(L"Time before append2seq:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                        //the line below takes ~1000 microseconds on a 2060 x 512 image
                        append2seq(rawData, curFrameIdx);
                        //OutputDebugStringW((wstring(L"Time after append2seq:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                        OutputDebugStringW((wstring(L"Event handled for frame #") + to_wstring(curFrameIdx) + wstring(L"\n")).c_str());
                        frameCount += 1;
                        OutputDebugStringW((wstring(L"Handled ") + to_wstring(frameCount) + wstring(L" frames total\n")).c_str());
                    }
                    //OutputDebugStringW((wstring(L"Time before copy:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                    //the line below takes ~600 microseconds on a 2060 x 512 image
                    //memcpy_g(camera->pLiveImage, rawData, sizeof(CookeCamera::PixelValue)*camera->imageSizePixels);
                    //OutputDebugStringW((wstring(L"Time after copy:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                    camera->nAcquiredFrames = max(curFrameIdx + 1, camera->nAcquiredFrames); //don't got back
                }
                else {
                    if (camera->genericAcqMode == Camera::eAcqAndSave) {
                        memcpy_g(camera->pLiveImage, camera->pBlackImage, sizeof(Camera::PixelValue)*camera->imageSizePixels);
                        camera->nAcquiredFrames = camera->nFrames;
                    }
                    else {
                        memcpy_g(camera->pLiveImage, rawData, sizeof(CookeCamera::PixelValue)*camera->imageSizePixels);
                        camera->nAcquiredFrames = curFrameIdx + 1;
                    }
                }
            } // if(!shouldStop)
        skipSponEvent:

            //reset event
            ResetEvent(camera->mEvent[eventIdx]);

            if (shouldStop) {
                tGuard.unlock();
                break;
            }

            ///then add back the buffer
            if (camera->nAcquiredFrames < camera->nFrames || camera->genericAcqMode == Camera::eLive){
                //in fifo mode, frameIdxInCamRam are 0 for both buffers?
                int frameIdxInCamRam = 0;
                //OutputDebugStringW((wstring(L"Time before add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the line below seeems to take at most 74 microseconds to execute on OCPI2 (assuming this doesn't depend on image size, which it shouldn't)
                camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, camera->mEvent[eventIdx], 0, frameIdxInCamRam, frameIdxInCamRam, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &dwStatusDrv), "failed to add external buffer");// Add buffer to the driver queue
                //OutputDebugStringW((wstring(L"Time after add buffer:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //TODO: switch to PCO_AddBufferExtern to improve performance
                //camera->safe_pco(PCO_AddBufferEx(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx], camera->getImageWidth(), camera->getImageHeight(), bytesPerPixel), "failed to add buffer");// Add buffer to the driver queue
                //camera->safe_pco(PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]), "failed to add buffer");// Add buffer to the driver queue
                tGuard.unlock();
                if (camera->errorCode != PCO_NOERROR) {
                    camera->errorMsg = "failed to add buffer";
                    break; //break the while
                }
                //OutputDebugStringW((wstring(L"End processing frame: ") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
            }
            else {
                tGuard.unlock();
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

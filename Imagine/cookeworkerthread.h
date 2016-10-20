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

            spoolingThread->start();
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


        while (true){
            //if(shouldStop) break;
            ///wait for events
            int waitResult = WaitForMultipleObjects(camera->nBufs, camera->mEvent, false, 2000);
			//TODO: the logic block below is what causes external acquisitions to time out. maybe fixed?
            while ((waitResult < WAIT_OBJECT_0 || waitResult >= WAIT_OBJECT_0 + camera->nBufs) && !shouldStop) {
                int waitResult = WaitForMultipleObjects(camera->nBufs, camera->mEvent, false, 2000); //wait again
            }//if, WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED
                //todo: should we try to keep going? SEE: CSC2Class::SC2Thread()
            if (shouldStop) break;
            nEvents++;
            int eventIdx = waitResult - WAIT_OBJECT_0;
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

            CLockGuard tGuard(camera->mpLock); //wraps and acquires the lock

            ///work around sdk bug
            if (nEvents == 1 && camera->acqTriggerMode == Camera::eExternal){
                goto skipSponEvent;
            }

            if (camera->isRecording) {
                CookeCamera::PixelValue* rawData = camera->mRingBuf[eventIdx];
                long counter = camera->extractFrameCounter(rawData);
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
                    string("hello");

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
                        append2seq(rawData, curFrameIdx);
                    }

                    memcpy_g(camera->pLiveImage, rawData, sizeof(CookeCamera::PixelValue)*camera->imageSizePixels);
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
                PCO_GetActiveRamSegment(camera->hCamera, &wActSeg);
                camera->safe_pco(PCO_AddBufferExtern(camera->hCamera, camera->mEvent[eventIdx], wActSeg, frameIdxInCamRam, frameIdxInCamRam, 0, camera->mRingBuf[eventIdx], camera->imageSizeBytes, &dwStatusDrv), "failed to add external buffer");// Add buffer to the driver queue
                //TODO: switch to PCO_AddBufferExtern to improve performance
                //camera->safe_pco(PCO_AddBufferEx(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx], camera->getImageWidth(), camera->getImageHeight(), bytesPerPixel), "failed to add buffer");// Add buffer to the driver queue
                //camera->safe_pco(PCO_AddBuffer(camera->hCamera, frameIdxInCamRam, frameIdxInCamRam, camera->mBufIndex[eventIdx]), "failed to add buffer");// Add buffer to the driver queue
                tGuard.unlock();
                if (camera->errorCode != PCO_NOERROR) {
                    camera->errorMsg = "failed to add buffer";
                    break; //break the while
                }
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

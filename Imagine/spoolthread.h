#ifndef SPOOLTHREAD_H
#define SPOOLTHREAD_H

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <assert.h>

#include "lockguard.h"

#include "fast_ofstream.hpp"
#include "circbuf.hpp"
#include "cooke.hpp"

#include "memcpy_g.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "timer_g.hpp"

class SpoolThread : public QThread {
    Q_OBJECT
private:
    FastOfstream *ofsSpooling; //NOTE: SpoolThread is not the owner
    CookeCamera* camera;

    QThread::Priority defaultPriority = QThread::HighestPriority;
    //QWaitCondition bufNotEmpty;
    QWaitCondition bufNotFull;
    Timer_g timer;

    volatile bool shouldStop; //todo: do we need a lock to guard it?

public:

    // ctor and dtor
    SpoolThread::SpoolThread(FastOfstream *ofsSpooling, CookeCamera* camera, QObject *parent = 0);
    SpoolThread::~SpoolThread();

    void requestStop(){
        shouldStop = true;
        //bufNotFull.wakeAll();
        //bufNotEmpty.wakeAll();
    }

    void run(){
#if defined(_DEBUG)
        cerr << "enter cooke spooling thread run()" << endl;
#endif

        long long timerReading;
        int curSize;
        int idx;

        //NOTE: may improve performace by moving live mode image update code to here
        //      This is becuase updateLiveImage() currently acquires a lock.
        //      Could optimize even further by creating the QByteArray from the raw circular buf data.
        while (true){
            //TODO: use QWaitCondition to wait until another frame is ready
            //to avoid "priority inversion" we temporarily elevate the priority
            this->setPriority(QThread::TimeCriticalPriority);
            camera->circBufLock->lock();
            if (shouldStop) goto finishup;
            curSize = camera->circBuf->size();
            if (curSize > 2) {
                //OutputDebugStringW((wstring(L"Current size of circ buf:") + to_wstring(curSize)+ wstring(L"\n")).c_str());
                idx = camera->circBuf->get();
                //camera->nAcquiredFrames += 1;
                //camera->nAcquiredFrames = camera->nAcquiredFrames; // max(curFrameIdx + 1, camera->nAcquiredFrames); //don't got back
                camera->circBufLock->unlock();
                this->setPriority(defaultPriority);
                if (camera->genericAcqMode != Camera::eLive)
                    this->ofsSpooling->write(camera->memPool + idx*size_t(camera->imageSizeBytes), camera->imageSizeBytes);
                //bufNotFull.wakeAll();
                //fill the live image
                //OutputDebugStringW((wstring(L"Time before copy:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
                //the line below takes ~600 microseconds on a 2060 x 512 image
                //memcpy_g(camera->pLiveImage, camera->memPool + idx*size_t(camera->imageSizeBytes), camera->imageSizeBytes);
                //OutputDebugStringW((wstring(L"Time after copy:") + to_wstring(gt.read()) + wstring(L"\n")).c_str()); 
            }
            else {
                camera->circBufLock->unlock();
                this->setPriority(defaultPriority);
                //OutputDebugStringW((wstring(L"Circ buf is empty: ") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
                QThread::msleep(10); //let the circular buffer fill a little
                continue;
                // while (circBuf->empty() && !shouldStop) {
                        //OutputDebugStringW((wstring(L"Begin wait for circbuf to fill:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
                //        bufNotEmpty.wait(circBufLock); //wait 4 not empty
                        //OutputDebugStringW((wstring(L"Fast ofstream thread awoke:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
                        //OutputDebugStringW((wstring(L"End wait for circbuf to fill:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
            }
        }//while,
    finishup:
        while (!camera->circBuf->empty()) {
            int idx = camera->circBuf->get();
            if (camera->genericAcqMode != Camera::eLive) {
                this->ofsSpooling->write(camera->memPool + idx*size_t(camera->imageSizeBytes), camera->imageSizeBytes);
            }
        }
        camera->circBufLock->unlock();
        this->setPriority(defaultPriority);

#if defined(_DEBUG)
        cerr << "leave cooke spooling thread run()" << endl;
#endif
    }//run(),
};//class, SpoolThread


#endif // SPOOLTHREAD_H

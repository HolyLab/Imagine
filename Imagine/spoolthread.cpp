#include "imagine.h"

#include <iostream>

#include "fast_ofstream.hpp"
#include "circbuf.hpp"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "timer_g.hpp"
#include "spoolthread.h"

//SpoolThread and CookeWorkerThread share access to 3 items:  a circular buffer object, a pool of memory, and a lock
SpoolThread::SpoolThread(QThread::Priority defaultPriority, FastOfstream *ofsSpooling, Camera * camera, QObject *parent)
    : WorkerThread(defaultPriority, parent)
{
    this->ofsSpooling = ofsSpooling;
    this->camera = camera;
}//ctor,

SpoolThread::~SpoolThread(){}//dtor

void SpoolThread::run() {
    #if defined(_DEBUG)
        std::cerr << "enter cooke spooling thread run()" << std::endl;
    #endif

    long long timerReading;
    int curSize;
    int idx;

    //NOTE: may improve performance by moving live mode image update code to here
    //      This is because updateLiveImage() currently acquires a lock.
    //      Could optimize even further by creating the QByteArray from the raw circular buf data.
    while (true) {
        //NOTE: can improve performance by using QWaitCondition to wait until another frame is ready
        //to avoid "priority inversion" we temporarily elevate the priority
        this->setPriority(QThread::TimeCriticalPriority);
        camera->circBufLock->lock();
        if (getShouldStop()) goto finishup;
        curSize = camera->circBuf->size();
        if (curSize > 2) {
            OutputDebugStringW((wstring(L"Current size of circ buf:") + to_wstring(curSize)+ wstring(L"\n")).c_str());
            idx = camera->circBuf->get();
            camera->circBufLock->unlock();
            if (camera->genericAcqMode != Camera::eLive)
                this->ofsSpooling->write(camera->memPool + idx*size_t(camera->imageSizeBytes), camera->imageSizeBytes);
            this->setPriority(getDefaultPriority());
        }
        else {
            camera->circBufLock->unlock();
            this->setPriority(getDefaultPriority());
            QThread::msleep(10); //let the circular buffer fill a little
            continue;
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
    this->setPriority(getDefaultPriority());

#if defined(_DEBUG)
    std::cerr << "leave cooke spooling thread run()" << std::endl;
#endif
}//run(),
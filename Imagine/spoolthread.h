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
#include "memcpy_g.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "timer_g.hpp"

class CookeWorkerThread;

class SpoolThread : public QThread {
    Q_OBJECT
private:
    FastOfstream *ofsSpooling; //NOTE: SpoolThread is not the owner

    CircularBuf * circBuf;
    long long itemSize;
    char * circBufData;

    char * memPool;
    long long memPoolSize;

    QWaitCondition bufNotEmpty;
    QWaitCondition bufNotFull;
    QMutex* spoolingLock;

    Timer_g timer;

    volatile bool shouldStop; //todo: do we need a lock to guard it?

    // this is only here so we can get access to the damn gTimer...
    CookeWorkerThread *parentThread;

public:
    bool allocMemPool(long long sz){
        if (sz < 0){
#ifdef _WIN64
            memPoolSize = (long long)4194304 * 2 * 100; //100 full frames. 5529600 for PCO.Edge 5.5 TODO: make this user-configuable
#else
            memPoolSize=5529600*2*30; //30 full frames
#endif
        }//if, use default
        else memPoolSize = sz;
        memPool = (char*)_aligned_malloc(memPoolSize, 1024 * 64);
        return memPool;
    }
    void freeMemPool(){
        _aligned_free(memPool);
        memPool = nullptr;
        memPoolSize = 0;
    }

    // ctor and dtor
    SpoolThread::SpoolThread(FastOfstream *ofsSpooling, long long itemSize, QObject *parent = 0);
    SpoolThread::~SpoolThread();

    void requestStop(){
        shouldStop = true;
        bufNotFull.wakeAll();
        bufNotEmpty.wakeAll();
    }

    //add one item to the ring buf. Blocked when full.
    void appendItem(char* item){
        spoolingLock->lock();
        while (circBuf->full() && !shouldStop){
            OutputDebugStringW((wstring(L"Begin wait for circbuf to empty:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
            bufNotFull.wait(spoolingLock);
            OutputDebugStringW((wstring(L"End wait for circbuf to empty:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
        }
        if (shouldStop)goto finishup;
        int idx = circBuf->put();
        memcpy_g(circBufData + idx*size_t(itemSize), item, itemSize);
        bufNotEmpty.wakeAll();
    finishup:
        spoolingLock->unlock();
    }//appendItem(),


    void run(){
#if defined(_DEBUG)
        cerr << "enter cooke spooling thread run()" << endl;
#endif

        long long timerReading;

        while (true){
        lockAgain:
            spoolingLock->lock();
            /*            if (!spoolingLock->tryLock()){
                timerReading = timer.readInNanoSec();
                while (timer.readInNanoSec() - timerReading < 1000 * 1000 * 7); //busy wait for 7ms
                goto lockAgain;
            }
            */
            while (circBuf->empty() && !shouldStop){
                OutputDebugStringW((wstring(L"Begin wait for circbuf to fill:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
                bufNotEmpty.wait(spoolingLock); //wait 4 not empty
                OutputDebugStringW((wstring(L"End wait for circbuf to fill:") + to_wstring(timer.read()) + wstring(L"\n")).c_str());
            }
            if (shouldStop) goto finishup;
        getAgain:
            int nEmptySlots = circBuf->capacity() - circBuf->size();
            int idx = circBuf->get();
            this->ofsSpooling->write(circBufData + idx*size_t(itemSize), itemSize);
            if (false && nEmptySlots < 64){
                goto getAgain;
            }
            bufNotFull.wakeAll();
            spoolingLock->unlock();

            //now flush if nec. 
            //todo: take care aggressive "get" above ( ... nEmptySlots < 64 ... )
            //(see below) shouldn't the ofstream be able to decide for itself when to flush?
            /*
            if (this->ofsSpooling->remainingBufSize() < itemSize){
                OutputDebugStringW(L"Flushing output file stream from spoolthread run\n");
                this->ofsSpooling->flush();
            }
            */
        }//while,
    finishup:
        while (!circBuf->empty()){
            int idx = circBuf->get();
            this->ofsSpooling->write(circBufData + idx*size_t(itemSize), itemSize);
        }
        spoolingLock->unlock();

#if defined(_DEBUG)
        cerr << "leave cooke spooling thread run()" << endl;
#endif
    }//run(),
};//class, SpoolThread


#endif // SPOOLTHREAD_H

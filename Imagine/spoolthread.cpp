#include "spoolthread.h"
#include "cooke.hpp"
#include "cookeworkerthread.h"
#include "imagine.h"

char * SpoolThread::memPool = nullptr;
long long SpoolThread::memPoolSize = 0;

// Moved methods that refer to workerThread into here...
// Eventually it might be nice to bring this class in line with the
// style used elsewhere in this codebase, with most code in the .cpp.

//PRE: itemsize: the size (in bytes) of each item in the circ buf
SpoolThread::SpoolThread(FastOfstream *ofsSpooling, int itemSize, QObject *parent)
    : QThread(parent){
    this->ofsSpooling = ofsSpooling;
    circBuf = nullptr;
    tmpItem = nullptr;
    parentThread = (CookeWorkerThread*)parent;

    // grab the timer...
    Timer_g gt = parentThread->camera->parentAcqThread->parentImagine->gTimer;

    //NOTE: to make sure fast_ofstream works, we enforce
    // (the precise cond: #bytes2write_in_total is an integer multiple of phy sector size.
    //  #bytes2write is itemSize * #items2write
    // assert(itemSize%4096==0);
    //NOTE: do it in upstream code instead

    this->itemSize = itemSize;

    int circBufCap = memPoolSize / itemSize;
    cout << "b4 new CircularBuf: " << gt.read() << endl;

    circBuf = new CircularBuf(circBufCap);
    cout << "after new CircularBuf: " << gt.read() << endl;

    long long circBufDataSize = size_t(itemSize)*circBuf->capacity();
    if (circBufDataSize > memPoolSize){
        freeMemPool();
        allocMemPool(circBufDataSize);
    }
    circBufData = memPool;

    //cout<<"after _aligned_malloc: "<<gt.read()<<endl;
#ifdef _WIN64
    assert((unsigned long long)circBufData % (1024 * 64) == 0);
#else
    assert((unsigned long)circBufData % (1024 * 64) == 0);
#endif
    tmpItem = new char[itemSize]; //todo: align

    //todo: provide way to check out-of-mem etc.. e.g., if(circBufData==nullptr) isInGoodState=false;
    //          If FastOfstream obj fails (i.e. write error), isInGoodState is set to false too.

    mpLock = new QMutex;

    shouldStop = false;

    cout << "b4 exit spoolthread->ctor: " << gt.read() << endl;

    timer.start();

}//ctor,

SpoolThread::~SpoolThread(){
    delete circBuf;
    delete[] tmpItem;
    delete mpLock;
}//dtor
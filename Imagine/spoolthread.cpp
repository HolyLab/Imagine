#include "spoolthread.h"
#include "cooke.hpp"
#include "cookeworkerthread.h"
#include "imagine.h"


// Moved methods that refer to workerThread into here...
// Eventually it might be nice to bring this class in line with the
// style used elsewhere in this codebase, with most code in the .cpp.

//PRE: itemsize: the size (in bytes) of each item in the circ buf
//SpoolThread and CookeWorkerThread share access to 3 items:  a circular buffer, a pool of memory, and a lock
SpoolThread::SpoolThread(FastOfstream *ofsSpooling, CookeCamera * camera, QObject *parent)
    : QThread(parent){
    this->ofsSpooling = ofsSpooling;

    parentThread = (CookeWorkerThread*)parent;


    // grab the timer...
    Timer_g gt = parentThread->camera->parentAcqThread->parentImagine->gTimer;

    //NOTE: to make sure fast_ofstream works, we enforce
    // (the precise cond: #bytes2write_in_total is an integer multiple of phy sector size.
    //  #bytes2write is itemSize * #items2write
    // assert(itemSize%4096==0);
    //NOTE: do it in upstream code instead

    //todo: provide way to check out-of-mem etc.. e.g., if(circBufData==nullptr) isInGoodState=false;
    //          If FastOfstream obj fails (i.e. write error), isInGoodState is set to false too.

    shouldStop = false;

    cout << "b4 exit spoolthread->ctor: " << gt.read() << endl;

    timer.start();

}//ctor,

SpoolThread::~SpoolThread(){}//dtor
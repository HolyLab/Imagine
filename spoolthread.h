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


class SpoolThread: public QThread {
   Q_OBJECT
private:
   FastOfstream *ofsSpooling; //NOTE: SpoolThread is not the owner

   CircularBuf * circBuf;
   int itemSize; //todo: change to long long
   char * tmpItem;
   char * circBufData;

   QWaitCondition bufNotEmpty;
   QWaitCondition bufNotFull;
   QMutex* mpLock;

   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   //PRE: itemsize: the size (in bytes) of each item in the circ buf
   SpoolThread(FastOfstream *ofsSpooling, int itemSize, QObject *parent = 0)
      : QThread(parent){
      this->ofsSpooling=ofsSpooling;
      circBuf=nullptr;
      tmpItem=nullptr;

      //NOTE: to make sure fast_ofstream works, we enforce
      // (the precise cond: #bytes2write_in_total is an integer multiple of phy sector size.
      //  #bytes2write is itemSize * #items2write
      // assert(itemSize%4096==0);
      //NOTE: do it in upstream code instead

      this->itemSize=itemSize;

      int circBufCap=16;//todo: hard coded 16
#ifdef _WIN64
      circBufCap*=64;
#endif
      circBuf=new CircularBuf(circBufCap); 
      circBufData=(char*)_aligned_malloc(size_t(itemSize)*circBuf->capacity(), 1024*64);
#ifdef _WIN64
      assert((unsigned long long)circBufData%(1024*64)==0);
#else
      assert((unsigned long)circBufData%(1024*64)==0);
#endif
      tmpItem=new char[itemSize]; //todo: align

      //todo: provide way to check out-of-mem etc.. e.g., if(circBufData==nullptr) isInGoodState=false;
      //          If FastOfstream obj fails (i.e. write error), isInGoodState is set to false too.

      mpLock=new QMutex;
 
      shouldStop=false;
   }//ctor,


   ~SpoolThread(){
      delete circBuf;
      _aligned_free(circBufData);
      delete[] tmpItem;
      delete mpLock;

   }//dtor,

   void requestStop(){
      shouldStop=true;
      bufNotFull.wakeAll();
      bufNotEmpty.wakeAll();
   }

   //add one item to the ring buf. Blocked when full.
   void appendItem(char* item){
      mpLock->lock();
      while(circBuf->full() && !shouldStop){
         bufNotFull.wait(mpLock);
      }
      if(shouldStop)goto finishup;
      int idx=circBuf->put();
      memcpy_g(circBufData+idx*size_t(itemSize), item, itemSize);
      bufNotEmpty.wakeAll();
finishup:
      mpLock->unlock();
   }//appendItem(),


   void run(){
#if defined(_DEBUG)
      cerr<<"enter cooke spooling thread run()"<<endl;
#endif

      while(true){
         mpLock->lock();
         while(circBuf->empty() && !shouldStop){
            bufNotEmpty.wait(mpLock); //wait 4 not empty
         }
         if(shouldStop) goto finishup;
getAgain:
         int nEmptySlots=circBuf->capacity()-circBuf->size();
         int idx=circBuf->get();
         this->ofsSpooling->write(circBufData+idx*size_t(itemSize), itemSize);
         if(false && nEmptySlots<64){
            goto getAgain;
         }
         bufNotFull.wakeAll();
         mpLock->unlock();

         //now flush if nec. 
         //todo: take care aggressive "get" above ( ... nEmptySlots < 64 ... )
         if(this->ofsSpooling->remainingBufSize()<itemSize){
            this->ofsSpooling->flush();
         }
      }//while,
finishup:
      while(!circBuf->empty()){
         int idx=circBuf->get();
         this->ofsSpooling->write(circBufData+idx*size_t(itemSize), itemSize);
      }
      mpLock->unlock();

#if defined(_DEBUG)
      cerr<<"leave cooke spooling thread run()"<<endl;
#endif
   }//run(),
};//class, SpoolThread


#endif // SPOOLTHREAD_H

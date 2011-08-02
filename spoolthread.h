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


#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


class SpoolThread: public QThread {
   Q_OBJECT
private:
   FastOfstream *ofsSpooling; //NOTE: SpoolThread is not the owner

   CircularBuf * circBuf;
   int itemSize;
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
      this->itemSize=itemSize;

      int circBufCap=16;//todo: hard coded 16
      circBuf=new CircularBuf(circBufCap); 
      circBufData=new char[itemSize*circBuf->capacity()]; //todo: alignment
      tmpItem=new char[itemSize]; //todo: align

      //todo: provide way to check out-of-mem etc.. e.g., if(circBufData==nullptr) isInGoodState=false;
      //          If FastOfstream obj fails (i.e. write error), isInGoodState is set to false too.

      mpLock=new QMutex;
 
      shouldStop=false;
   }//ctor,


   ~SpoolThread(){
      delete circBuf;
      delete[] circBufData;
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
      memcpy(circBufData+idx*itemSize, item, itemSize);
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
         int idx=circBuf->get();
         memcpy(tmpItem, circBufData+idx*itemSize, itemSize);
         bufNotFull.wakeAll();
         mpLock->unlock();

         //now save tmpItem
         this->ofsSpooling->write(tmpItem, itemSize);
      }//while,
finishup:
      while(!circBuf->empty()){
         int idx=circBuf->get();
         memcpy(tmpItem, circBufData+idx*itemSize, itemSize);
         this->ofsSpooling->write(tmpItem, itemSize);
      }
      mpLock->unlock();

#if defined(_DEBUG)
      cerr<<"leave cooke spooling thread run()"<<endl;
#endif
   }//run(),
};//class, SpoolThread


#endif // SPOOLTHREAD_H

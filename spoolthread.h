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
   char * circBufData;

   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   //PRE: itemsize: the size of each item in the circ buf
   SpoolThread(FastOfstream *ofsSpooling, int itemSize, QObject *parent = 0)
      : QThread(parent){
      this->ofsSpooling=ofsSpooling;
      circBuf=nullptr;
      this->itemSize=itemSize;

      int circBufCap=16;//todo: hard coded 16
      circBuf=new CircularBuf(circBufCap); 
      circBufData=new char[itemSize*circBuf->capacity()]; //todo: alignment

      shouldStop=false;
   }
   ~SpoolThread(){
      delete circBuf;
      delete[] circBufData;
   }

   void requestStop(){
      shouldStop=true;
   }

   void run(){
#if defined(_DEBUG)
      cerr<<"enter cooke spooling thread run()"<<endl;
#endif

      while(true){
         if(shouldStop) break;

      }//while,

#if defined(_DEBUG)
      cerr<<"leave cooke spooling thread run()"<<endl;
#endif
   }//run(),
};//class, SpoolThread


#endif // SPOOLTHREAD_H

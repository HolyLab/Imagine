#ifndef COOKESPOOLTHREAD_H
#define COOKESPOOLTHREAD_H

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <assert.h>

#include "cooke.hpp"
#include "lockguard.h"

#include <QThread>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


class CookeCamera::SpoolThread: public QThread {
   Q_OBJECT
private:
   CookeCamera* camera;
   volatile bool shouldStop; //todo: do we need a lock to guard it?

public:
   SpoolThread(CookeCamera * camera, QObject *parent = 0)
      : QThread(parent){
      this->camera=camera;
      shouldStop=false;
   }
   ~SpoolThread(){}

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
};//class, CookeCamera::SpoolThread


#endif // COOKESPOOLTHREAD_H

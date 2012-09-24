/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
**    All rights reserved.
** Author: All code authored by Zhongsheng Guo.
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/

#include <QtGui>

#include <math.h>
#include <fstream>
 
using namespace std;

#include "ai_thread.hpp"
#include "scoped_ptr_g.hpp"
#include "dummy_daq.hpp"
#include "ni_daq_g.hpp"

extern QString daq;


AiThread::AiThread(QObject *parent, int readBufSize, int driverBufSize, int scanrate)
: QThread(parent)
{
   this->readBufSize=readBufSize; 
   this->driverBufSize=driverBufSize; 

   //TODO: error checking for ai
   vector<int> chanList; 
   //chanList.push_back(0);
   for(int i=0; i<4; ++i){
      chanList.push_back(i);
   }
   this->chanList=chanList;

   ai=nullptr;

   if(daq=="ni") ai=new NiDaqAi(chanList);
   else if(daq=="dummy") ai=new DummyDaqAi(chanList);
   else {
      throw Daq::EInitDevice("exception: the AI device is unsupported");
   }


   ai->cfgTiming(scanrate, driverBufSize);

   //reserve space:
   //int tnSamplesToReserve=scanrate*chanList.size()*100; //100sec data
   //data.reserve(tnSamplesToReserve);
}

AiThread::~AiThread()
{
   if(ai)delete ai;
}

//return false if ai's start() failed.
bool AiThread::startAcq()
{
   stopRequested=false;
   ai->start();   //TODO: check return value
   this->start();

   return true;
}

void AiThread::stopAcq()
{
   stopRequested=true;
   wait(); //block calling thread until AiThread associated thread is done.
}

void AiThread::run()
{
   uInt16* readBuf=new uInt16[readBufSize*chanList.size()];
   ScopedPtr_g<uInt16> ttScopedPtr(readBuf, true);
   //ScopedPtr_g<uInt16>(readBuf, true); //note: unamed temp var will be free at cur line

   while(!stopRequested){
      ai->read(readBufSize, readBuf);
      {//local scope to make QMutexLocker work
         QMutexLocker locker(&mutex);
         for(int i=0; i<readBufSize*chanList.size(); ++i){
            data.push_back(readBuf[i]);
            //todo: save periodically to avoid data becomes too big
         }//for,
      }//local scope to make QMutexLocker work
   }//while, user not requested stop
}//run()

void AiThread::save(ofstream& ofsAi)
{
   QMutexLocker locker(&mutex);
   
   if(data.size()==0) return;

   ofsAi.write((const char*)&data[0], //note: take advantage that items in a vector are stored contiguously
      sizeof(uInt16)*data.size());
   data.clear();
   data.shrink_to_fit();
}

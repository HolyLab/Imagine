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
#include <memory>
#include <functional>

using namespace std;

#include "ai_thread.hpp"
#include "dummy_daq.hpp"
#include "ni_daq_g.hpp"

extern QString daq;


AiThread::AiThread(QObject *parent, QString ainame, int readBufSize, int driverBufSize, int scanrate)
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

   ofs=nullptr;

   ai=nullptr;

   if(daq=="ni") ai=new NiDaqAi(ainame, chanList);
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
   //ScopedPtr_g<uInt16>(readBuf, true); //note: unamed temp var will be free at cur line
   unique_ptr<uInt16[]> ttScopedPtr(readBuf);

   while(!stopRequested){
      ai->read(readBufSize, readBuf);
      {//local scope to make auto-unlock work
         unique_ptr<QMutex, void(*)(QMutex*)> locker(&mutex, [](QMutex* m){m->unlock();});
         mutex.lock();

         for(int i=0; i<readBufSize*chanList.size(); ++i){
            data.push_back(readBuf[i]);
         }//for,
	 if(ofs) mSave(*ofs);
      }//local scope to make auto-unlock work
   }//while, user not requested stop
}//run()

//NOTE: lockless
bool AiThread::mSave(ofstream& ofsAi)
{
   if(data.size()==0) return true;

   ofsAi.write((const char*)&data[0], //note: take advantage that items in a vector are stored contiguously
   sizeof(uInt16)*data.size());
   data.clear();
   data.shrink_to_fit();

   return ofsAi.good();
}

//same as mSave() but w/ lock
bool AiThread::save(ofstream& ofsAi)
{
   //QMutexLocker locker(&mutex);
   unique_ptr<QMutex, std::function<void(QMutex*)>> locker(&mutex, [](QMutex* m){m->unlock();});
   mutex.lock();
   
   return mSave(ofsAi);
}

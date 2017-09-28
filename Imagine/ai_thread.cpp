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
#include <QtCore>

#include <math.h>
#include <fstream>
#include <memory>
#include <functional>

using namespace std;

#include "ai_thread.hpp"
#include "dummy_daq.hpp"
#include "ni_daq_g.hpp"

extern QString daq;

AiThread::AiThread(QString ainame, int readBufSize, int driverBufSize, int scanrate,
                vector<int> chanList, int num, string clkName, QObject *parent)
    : QThread(parent)
{
    this->readBufSize = readBufSize;
    this->driverBufSize = driverBufSize;

    this->chanList = chanList;
    totalSampleNum = num*chanList.size();
    writeSampleNum = 0;

    ofs = nullptr;

    ai = nullptr;

    if (daq == "ni") ai = new NiDaqAi(ainame, chanList);
    else if (daq == "dummy") ai = new DummyDaqAi(chanList);
    else {
        throw Daq::EInitDevice("exception: the AI device is unsupported");
    }

    //NOTE: the success return value may be unreliable
    int success = ai->cfgTiming(scanrate, driverBufSize, clkName);
    if(!success) {
        throw Daq::EInitDevice("exception: failed to configure AI device timing");
    }
    //reserve space:
    //int tnSamplesToReserve=scanrate*chanList.size()*100; //100sec data
    //data.reserve(tnSamplesToReserve);
}

AiThread::~AiThread()
{
    if (ai)delete ai;
}

//return false if ai's start() failed.
bool AiThread::startAcq()
{
    stopRequested = false;
    ai->start();   //TODO: check return value
    this->start(QThread::IdlePriority);

    return true;
}

void AiThread::stopAcq()
{
    stopRequested = true;
    wait(); //block calling thread until AiThread associated thread is done.
}

bool AiThread::setTrigger(string clkName)
{
    return ai->setTrigger(clkName);
}

void AiThread::run()
{
    //uInt16* readBuf = new uInt16[readBufSize*chanList.size()];
	float64* readBuf = new float64[readBufSize*chanList.size()];
    //ScopedPtr_g<uInt16>(readBuf, true); //note: unamed temp var will be free at cur line
    //unique_ptr<uInt16[]> ttScopedPtr(readBuf);
	unique_ptr<float64[]> ttScopedPtr(readBuf);

    while (!stopRequested){
        ai->read(readBufSize, readBuf);
        {//local scope to make auto-unlock work
            unique_ptr<QMutex, void(*)(QMutex*)> locker(&mutex, [](QMutex* m){m->unlock(); });
            mutex.lock();

            for (int i = 0; i < readBufSize*chanList.size(); ++i){
                //rescale the float value (+-10.0) to a 16-bit integer
                data.push_back(static_cast<signed short>(std::round(readBuf[i] / 10.0 * 32767)));  //TODO: hardcoded +-10V range of DAQ and 16-bit A/D value
            }//for,
            if (ofs) mSave(*ofs);
        }//local scope to make auto-unlock work
    }//while, user not requested stop
}//run()

//NOTE: lockless
bool AiThread::mSave(ofstream& ofsAi)
{
    if (data.size() == 0) return true;

    long long size;
    if (totalSampleNum)
        size = min((long long)data.size(), totalSampleNum - writeSampleNum);
    else
        size = (long long)data.size();
    ofsAi.write((const char*)&data[0],
        sizeof(uInt16)*size);
    writeSampleNum += size;
    data.clear();
    data.shrink_to_fit();

    return ofsAi.good();
}

bool AiThread::isLeftToReadSamples()
{
    return (totalSampleNum - writeSampleNum);
}

//same as mSave() but w/ lock
bool AiThread::save(ofstream& ofsAi)
{
    //QMutexLocker locker(&mutex);
    unique_ptr<QMutex, std::function<void(QMutex*)>> locker(&mutex, [](QMutex* m){m->unlock(); });
    mutex.lock();

    return mSave(ofsAi);
}
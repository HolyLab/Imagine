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

#include "di_thread.hpp"
#include "dummy_daq.hpp"
#include "ni_daq_g.hpp"

extern QString daq;

DiThread::DiThread(QString diname, int readBufSize, int driverBufSize, int scanrate,
                vector<int> chanList, int diBegin, string clkName, QObject *parent)
    : QThread(parent)
{
    this->readBufSize = readBufSize;
    this->driverBufSize = driverBufSize;

    this->chanList = chanList;
    this->diBegin = diBegin;

    ofs = nullptr;

    di = nullptr;
    if (daq == "ni") di = new NiDaqDi(diname, chanList);
    else if (daq == "dummy") di = new DummyDaqDi(chanList);
    else {
        throw Daq::EInitDevice("exception: the DI device is unsupported");
    }

    //NOTE: the success return value may be unreliable
    int success = di->cfgTiming(scanrate, driverBufSize, clkName);
    if(!success) {
        throw Daq::EInitDevice("exception: failed to configure DI device timing");
    }

}

DiThread::~DiThread()
{
    if (di)delete di;
}

//return false if di's start() failed.
bool DiThread::startAcq()
{
    stopRequested = false;
    di->start();   //TODO: check return value
    this->start(QThread::IdlePriority);

    return true;
}

void DiThread::stopAcq()
{
    stopRequested = true;
    wait(); //block calling thread until AiThread associated thread is done.
}

bool DiThread::setTrigger(string clkName)
{
    return di->setTrigger(clkName);
}

void DiThread::run()
{
	uInt32* readBuf = new uInt32[readBufSize];
	unique_ptr<uInt32[]> ttScopedPtr(readBuf);

    while (!stopRequested){
        di->read(readBufSize, readBuf);
        {//local scope to make auto-unlock work
            unique_ptr<QMutex, void(*)(QMutex*)> locker(&mutex, [](QMutex* m){m->unlock(); });
            mutex.lock();

            for (int i = 0; i < readBufSize; ++i){
                data.push_back((uInt8)(readBuf[i]>> diBegin));
            }//for,
            if (ofs) mSave(*ofs);
        }//local scope to make auto-unlock work
    }//while, user not requested stop
}//run()

//NOTE: lockless
bool DiThread::mSave(ofstream& ofsDi)
{
    if (data.size() == 0) return true;

    ofsDi.write((const char*)&data[0],
        sizeof(uInt8)*data.size());
    data.clear();
    data.shrink_to_fit();

    return ofsDi.good();
}

//same as mSave() but w/ lock
bool DiThread::save(ofstream& ofsDi)
{
    //QMutexLocker locker(&mutex);
    unique_ptr<QMutex, std::function<void(QMutex*)>> locker(&mutex, [](QMutex* m){m->unlock(); });
    mutex.lock();

    return mSave(ofsDi);
}

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

#ifndef DI_THREAD_HPP
#define DI_THREAD_HPP

#include <vector>
#include <fstream>

using std::vector;
using std::ofstream;

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QObject>

#include "daq.hpp"
#include "ni_daq_g.hpp"


class DiThread : public QThread
{
    Q_OBJECT

public:
    DaqDi* di;

    //NOTE: readBufSize is in scan.
    DiThread(QString diname, int readBufSize, int driverBufSize, int scanrate,
            vector<int> chanList, int diBegin, int num, string clkName = "", QObject *parent = 0);
    ~DiThread();

    bool startAcq();
    void stopAcq(); //note: this func call is blocking.
    bool save(ofstream& ofsDi);
    void setOfstream(ofstream* ofs){ this->ofs = ofs; }
    bool setTrigger(string clkName);

protected:
    void run();
    bool mSave(ofstream& ofsDi);

private:
    vector<uInt8> data;  //TODO: may reserve space beforehand to speed up
    int readBufSize;  //note: not driver's input buffer size
    int driverBufSize;//driver's input buffer size
    volatile bool stopRequested;
    vector<int> chanList;
    ofstream* ofs;
    int diBegin;
    long long totalSampleNum;
    long long writeSampleNum;

    QMutex mutex;
    QWaitCondition condition;
};

#endif  //DI_THREAD_HPP

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

#ifndef AI_THREAD_HPP
#define AI_THREAD_HPP

#include <vector>
#include <fstream>

using std::vector;
using std::ofstream;

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "daq.hpp"
//#include "ni_daq_g.hpp"


class AiThread : public QThread
{
   Q_OBJECT

public:
   DaqAi* ai;

   //NOTE: readBufSize is in scan.
   AiThread(QObject *parent = 0, 
      int readBufSize=1000, int driverBufSize=10000, int scanrate=10000);
   ~AiThread();

   bool startAcq();
   void stopAcq(); //note: this func call is blocking.
   bool save(ofstream& ofsAi);
   void setOfstream(ofstream* ofs){this->ofs=ofs;}

protected:
   void run();
   bool mSave(ofstream& ofsAi);

private:
   vector<Daq::sample_t> data;  //TODO: may reserve space beforehand to speed up
   int readBufSize;  //note: not driver's input buffer size
   int driverBufSize;//driver's input buffer size
   volatile bool stopRequested;
   vector<int> chanList;
   ofstream* ofs;

   QMutex mutex;
   QWaitCondition condition;
};

#endif  //AI_THREAD_HPP

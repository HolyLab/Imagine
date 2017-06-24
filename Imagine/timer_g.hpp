/*-------------------------------------------------------------------------
** Copyright (C) 2005-2008 Timothy E. Holy and Zhongsheng Guo
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

#ifndef TIMER_G_HPP
#define TIMER_G_HPP

#include <QElapsedTimer>
#include <assert.h>

class Timer_g {
   QElapsedTimer mTimer;

public:
   Timer_g(){
      assert(mTimer.clockType()==QElapsedTimer::PerformanceCounter);
      mTimer.start();
   }

   ~Timer_g(){
   }

   void start(){
      mTimer.restart();   
   }

   //return time in sec
   double read() const{
      return mTimer.nsecsElapsed()/1e9;
   }

   long long readInNanoSec() const{
      return mTimer.nsecsElapsed();
   }

}; //class, Timer_g

#endif //TIMER_G_HPP

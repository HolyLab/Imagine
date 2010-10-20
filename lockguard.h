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


#ifndef lockguard_h
#define lockguard_h

#include <QMutex>

class CLockGuard {
public:
   CLockGuard(QMutex* apLock):mpLock(apLock),mOwned(false){
      mpLock->lock();
      mOwned=true;
   }
   ~CLockGuard(){
      if(mOwned)mpLock->unlock();
   }

private:
   QMutex * mpLock;
   bool mOwned;

   //forbid copy assignment and initialization
   CLockGuard& operator= (const CLockGuard &);
   CLockGuard(const CLockGuard &);

};//class


#endif //#ifndef
 

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

#ifndef SCOPED_PTR_G_HPP
#define SCOPED_PTR_G_HPP

template<typename T> class ScopedPtr_g {
   T* ptr;
   bool isArray;
public:
   ScopedPtr_g(T* ptr, bool isArray){
      this->ptr=ptr;
      this->isArray=isArray;
   }

   ~ScopedPtr_g(){
      if(isArray) delete[] ptr;
      else delete ptr;
   }

private:
   //hide copy ctor and assign operator
   ScopedPtr_g(ScopedPtr_g&);
   ScopedPtr_g& operator=(ScopedPtr_g&);
};//template class, ScopedPtr_g

#endif //SCOPED_PTR_G_HPP

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

#ifndef MISC_HPP
#define MISC_HPP

#include <sstream>
#include <string>

using std::ostringstream;
using std::string;

#include <QString>


template<typename T> string toString(T aValue) { //don't define as toString(T& rValue) b/c 
   ostringstream result;                        //I want toString(1) is legal
   result<<aValue;

   return result.str();
}


QString replaceExtName(QString filename, QString newExtname);


#endif //MISC_HPP

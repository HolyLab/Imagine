/*-------------------------------------------------------------------------
** Copyright (C) 2005-2012 Timothy E. Holy and Zhongsheng Guo
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

#ifndef CURVEDATA_H
#define CURVEDATA_H

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
//#include <qwt_interval_data.h>
#include <qwt_scale_engine.h>
//#include <qwt_data.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>


class CurveData: public QwtData
{
   QList<QPointF> data;
   double width; //the length of x axis
public:
   CurveData(double width) {
      this->width=width;
   }

   virtual QwtData *copy() const  {
      CurveData* result= new CurveData(width);
      result->data=data;

      return result;
   }

   virtual size_t size() const {
      return data.size();
   }

   virtual double x(size_t i) const {
      return data[i].x();
   }

   virtual double y(size_t i) const {
      return data[i].y();
   }

   //above are required for the virtual parent

   double left(){ return data[0].x(); }
   
   double right(){
      return data.last().x();
   }

   void clear(){
      data.clear();
   }

   void append(double x, double y){
      data.push_back(QPointF(x,y));
      while(x-this->x(0)>width) data.pop_front();
   }
};//class, CurveData

#endif


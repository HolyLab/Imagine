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

#include "temperaturedialog.h"
#include "imagine.h"
#include <QTimer>
#include "andor_g.hpp"

TemperatureDialog::TemperatureDialog(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);

        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(updateTemperature()));
        timer->start(1000);
        imagineParent = (Imagine*)parent;
}

TemperatureDialog::~TemperatureDialog()
{

}


void TemperatureDialog::on_radioButtonOn_toggled(bool isChecked)
{
   ui.btnSet->setEnabled(isChecked);
   ((AndorCamera*)imagineParent->dataAcqThread.pCamera)->switchCooler(isChecked);
}

void TemperatureDialog::on_btnSet_clicked()
{
   SetTemperature(ui.verticalSliderTempToSet->value()); //TODO: wrap this func
}


int TemperatureDialog::updateTemperature()
{
   int 	temperature;
   int errorValue=GetTemperature(&temperature);  //TODO: wrapp this func
   ui.labelCurTemp->setNum(temperature);

   QString tempMsg;
   if(DRV_TEMPERATURE_STABILIZED==errorValue){
      tempMsg="Current temperature stabled at: ";
   }
   else {
      tempMsg="Current temperature: ";
   }
   ui.labelTempMsg->setText(tempMsg);

   return temperature;
}

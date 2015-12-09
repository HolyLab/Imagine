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

#include "fanctrldialog.h"
#include "imagine.h"
#include "andor_g.hpp"

FanCtrlDialog::FanCtrlDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    parentImagine = (Imagine*)parent;
}

FanCtrlDialog::~FanCtrlDialog()
{

}


void FanCtrlDialog::on_buttonBox_accepted()
{
    AndorCamera::FanSpeed speed = AndorCamera::fsHigh;
    if (ui.radioButtonLow->isChecked()) speed = AndorCamera::fsLow;
    else if (ui.radioButtonOff->isChecked()) speed = AndorCamera::fsOff;

    AndorCamera *cam = (AndorCamera *)parentImagine->dataAcqThread.pCamera;
    cam->setHeatsinkFanSpeed(speed); //TODO: check return value

    this->hide();
}

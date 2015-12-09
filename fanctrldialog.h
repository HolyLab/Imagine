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

#ifndef FANCTRLDIALOG_H
#define FANCTRLDIALOG_H

#include <QDialog>
#include "ui_fanctrldialog.h"

class Imagine;

class FanCtrlDialog : public QDialog
{
    Q_OBJECT

public:
    FanCtrlDialog(QWidget *parent = 0);
    ~FanCtrlDialog();

private:
    Ui::FanCtrlDialogClass ui;
    Imagine *parentImagine;
    private slots:
    void on_buttonBox_accepted();
};

#endif // FANCTRLDIALOG_H

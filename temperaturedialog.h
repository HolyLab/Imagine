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

#ifndef TEMPERATUREDIALOG_H
#define TEMPERATUREDIALOG_H

#include <QDialog>
#include "ui_temperaturedialog.h"

class TemperatureDialog : public QDialog
{
    Q_OBJECT

public:
    TemperatureDialog(QWidget *parent = 0);
    ~TemperatureDialog();


    Ui::TemperatureDialogClass ui;
public slots:
    int updateTemperature();
private:

private slots:
        void on_btnSet_clicked();
        void on_radioButtonOn_toggled(bool);

};

#endif // TEMPERATUREDIALOG_H

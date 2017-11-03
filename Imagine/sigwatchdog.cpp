/*-------------------------------------------------------------------------
** Copyright (C) 2017-2022 Dae Woo Kim
**    All rights reserved.
** Author: Dae Woo Kim
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact kdw503@wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/
#include "sigwatchdog.h"
#include <QDebug>

SigWatchdog::SigWatchdog(int periodms)
{
    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        szName);               // name of mapping object

    if (hMapFile == NULL)
    {
        qDebug() << "Watchdog error:\n" << QString("%1").arg(GetLastError());
        errorMsg.append(QString("Can not open shared memory\n"));
        error = true;
        return;
    }

    pBuf = (LPTSTR)MapViewOfFile(hMapFile, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBuf == NULL)
    {
        qDebug() << "Watchdog error:\n" << QString("%1").arg(GetLastError());
        errorMsg.append(QString("Can not map shared memory file\n"));
        CloseHandle(hMapFile);
        error = true;
    }
    else {
        sd = (SharedData*)pBuf;
        period = periodms;
        qDebug() << sd->id;
        error = false;
    }
}

SigWatchdog::~SigWatchdog()
{
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
}

void SigWatchdog::signalToWatchdog()
{
    while (1)
    {
        QThread::msleep(period);
        if(sd)
            sd->isRunning = 1;
    }
}

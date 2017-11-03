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

#ifndef SIGWATCHDOG_H
#define SIGWATCHDOG_H

#include <QtWidgets/QApplication>
#include <QTextStream>
#include <QFile>
#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptable>
#include <QThread>
#include <QTime>
#include <string>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUF_SIZE 256
typedef struct {
    char id[100];
    int isRunning;
} SharedData;

class SigWatchdog : public QObject
{
    Q_OBJECT
private:
    QString errorMsg = "";
    HANDLE hMapFile;
    LPCTSTR pBuf;
    int period;
    TCHAR szName[100] = TEXT("Local\\ImagineWatchdogObject");
    SharedData *sd;

public:
    volatile bool error;
    SigWatchdog(int periodms);
    ~SigWatchdog();

    QString getErrorMsg(void) {
        return errorMsg;
    };
public slots:
    void signalToWatchdog(void);
};
#endif // SIGWATCHDOG_H
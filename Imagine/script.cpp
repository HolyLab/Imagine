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
#include "script.h"
#include "imagine.h"

void* ImagineScript::instance = nullptr;
extern string rig;

bool ImagineScript::print(const QString &str)
{
    emit newMsgReady(str);
    return true;
}

bool ImagineScript::validityCheck(const QString &file1, const QString &file2)
{
    int sleepTime = 100; // 1000msec

    shouldWait = true;
    emit requestValidityCheck(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

bool ImagineScript::record(const QString &file1, const QString &file2, long int timeout)
{
    long int elaspTime = 0;
    int sleepTimeInSec = 1;
    bool retVal;

    if (file1.isEmpty() && file2.isEmpty())
        retVal = true; // config files were already loaded in Imagine
    else
        retVal = validityCheck(file1, file2);

    if (retVal == true) { 
        shouldWait = true;
        emit requestRecord();

        while ((shouldWait)&&(elaspTime<timeout)) {
            Sleep(sleepTimeInSec*1000);
            elaspTime += sleepTimeInSec;
        }

        if (shouldWait) {
            shouldWait = false;
            // This can detect only masterImagine acq. error
            // TODO: Even when masterImagine acq. is ok, we need to detect
            // slaveImagine acq. and if it stuck we need to send stop recording signal to
            // slaveImagine. Othewise, acq_and_save itself has to have timeout stop routine
            return false;
        }
        return true;
    }
    else
        return false;
}

bool ImagineScript::loadConfig(const QString &file1, const QString &file2)
{
    int sleepTime = 100; // 100msec

    shouldWait = true;
    emit requestLoadConfig(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

bool ImagineScript::loadWaveform(const QString &file)
{
    int sleepTime = 100; // 100msec

    shouldWait = true;
    emit requestLoadWaveform(file);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}


bool ImagineScript::stopRecord()
{
    int sleepTime = 100; // 100msec

    shouldWait = true;
    emit requestStopRecord();

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

bool ImagineScript::sleep(long int time)
{
    Sleep(time);
    return true;
}

long int ImagineScript::getEstimatedRunTime()
{
    return estimatedTime;
}

bool ImagineScript::setFilename(const QString &file1, const QString &file2)
{
    int sleepTime = 100;

    shouldWait = true;
    emit requestSetFilename(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}


void readFilenames(QScriptContext *context, QString &file1, QString &file2)
{
    int numArg = context->argumentCount();

    if (numArg != 0) {
        file1 = context->argument(0).toString();
        if (numArg == 2) {
            file2 = context->argument(1).toString();
        }
    }
}

void readFilenamesNTimeout(QScriptContext *context, QString &file1, QString &file2, long int &timeout)
{
    int numArg = context->argumentCount();

    if (numArg > 1) { // 2,3
        file1 = context->argument(0).toString();
        if (numArg == 3) { // 3
            file2 = context->argument(1).toString();
            timeout = context->argument(2).toUInt32();
        }
        else { // 2
            timeout = context->argument(1).toUInt32();
        }
    }
    else {
        timeout = context->argument(0).toUInt32();
    }
}

QScriptValue ImagineScript::printWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString x = context->argument(0).toString();
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->print(x));
}

QScriptValue ImagineScript::validityCheckWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString file1 = "", file2 = "";

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->validityCheck(file1, file2));
}

QScriptValue ImagineScript::recordWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString file1 = "", file2 = "";
    long int timeout;

    readFilenamesNTimeout(context, file1, file2, timeout);
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->record(file1, file2, timeout));
}

QScriptValue ImagineScript::loadConfigWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString file1, file2;
    int numArg = context->argumentCount();

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->loadConfig(file1, file2));
}

QScriptValue ImagineScript::loadWaveformWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString file;
    int numArg = context->argumentCount();

    if (numArg)
        file = context->argument(0).toString();

    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->loadWaveform(file));
}

QScriptValue ImagineScript::sleepWrapper(QScriptContext *context, QScriptEngine *engine)
{
    long int sleepTime;
    int numArg = context->argumentCount();

    if(numArg)
        sleepTime = context->argument(0).toUInt32();

    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->sleep(sleepTime));
}

QScriptValue ImagineScript::setFilenameWrapper(QScriptContext *context, QScriptEngine *engine)
{
    QString file1 = "", file2 = "";

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->setFilename(file1, file2));
}

QScriptValue ImagineScript::getEstimatedRunTimeWrapper(QScriptContext *context, QScriptEngine *engine)
{
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->getEstimatedRunTime());
}

QScriptValue ImagineScript::stopRecordWrapper(QScriptContext *context, QScriptEngine *engine)
{
    if (instance)
        return QScriptValue(engine, static_cast<ImagineScript*>(instance)->stopRecord());
}
ImagineScript::ImagineScript(QString rig)
{
    this->rig = rig;
    sEngine = new QScriptEngine;
    instance = this;
    QScriptValue svPrint = sEngine->newFunction(printWrapper);
    sEngine->globalObject().setProperty("print", svPrint);
    QScriptValue svValid = sEngine->newFunction(validityCheckWrapper);
    sEngine->globalObject().setProperty("validityCheck", svValid);
    sEngine->globalObject().setProperty("applyConfiguration", svValid);
    QScriptValue svRecord = sEngine->newFunction(recordWrapper);
    sEngine->globalObject().setProperty("record", svRecord);
    QScriptValue svLoadCon = sEngine->newFunction(loadConfigWrapper);
    sEngine->globalObject().setProperty("loadConfig", svLoadCon);
    QScriptValue svLoadWav = sEngine->newFunction(loadWaveformWrapper);
    sEngine->globalObject().setProperty("loadWaveform", svLoadWav);
    QScriptValue svSleep = sEngine->newFunction(sleepWrapper);
    sEngine->globalObject().setProperty("sleep", svSleep);
    QScriptValue svSetFile = sEngine->newFunction(setFilenameWrapper);
    sEngine->globalObject().setProperty("setOutputFilename", svSetFile);
    QScriptValue svEstimatedRunTime = sEngine->newFunction(getEstimatedRunTimeWrapper);
    sEngine->globalObject().setProperty("getEstimatedRunTime", svEstimatedRunTime);
    QScriptValue svStopRecord = sEngine->newFunction(stopRecordWrapper);
    sEngine->globalObject().setProperty("stopRecord", svStopRecord);
}

ImagineScript::~ImagineScript()
{
    if(sEngine)
        delete sEngine;
    instance = nullptr;
    if (scriptProgram != NULL)
        delete scriptProgram;
}

void ImagineScript::loadImagineScript(QString code)
{
    if (scriptProgram != NULL)
        delete scriptProgram;
    scriptProgram = new QScriptProgram(code);
}

bool ImagineScript::scriptProgramEvaluate()
{
    if (sEngine == NULL)
        return false;
    QScriptValue jsobj = sEngine->evaluate(*scriptProgram);
    if (sEngine->hasUncaughtException()) {
        errorMsg.append(
            QString("There's problem when evaluating script:\n%1\n%2.")
            .arg(QString("   ... at line %1").arg(sEngine->uncaughtExceptionLineNumber()))
            .arg(QString("   ... error msg: %1").arg(sEngine->uncaughtException().toString())));
        newMsgReady(errorMsg);
        return false;
    }
    newMsgReady("Enecution end\n");
    return true;
}

bool ImagineScript::scriptAbortEvaluation()
{
    if (sEngine == NULL)
        return false;
    if (sEngine->isEvaluating()) {
        sEngine->abortEvaluation();
        newMsgReady("Enecution aborted\n");
    }
    return true;
}


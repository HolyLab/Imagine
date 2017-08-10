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
    newMsgReady(str);
    return true;
}

bool ImagineScript::validityCheck(const QString &file1, const QString &file2)
{
    int sleepTime = 100; // 1000msec

    shouldWait = true;
    requestValidityCheck(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

bool ImagineScript::record(const QString &file1, const QString &file2, long int timeout)
{
    long int elaspTime = 0;
    int sleepTime = 1000; // 1000msec
    bool retVal;

    if (file1.isEmpty() && file2.isEmpty())
        retVal = true;
    else
        retVal = validityCheck(file1, file2);

    if (retVal == true) {
        shouldWait = true;
        requestRecord();

        while ((shouldWait)&&(elaspTime<timeout)) {
            Sleep(sleepTime);
            elaspTime += sleepTime;
        }

        if (shouldWait) {
            shouldWait = false;
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
    requestLoadConfig(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

bool ImagineScript::loadWaveform(const QString &file)
{
    int sleepTime = 100; // 100msec

    shouldWait = true;
    requestLoadWaveform(file);

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

bool ImagineScript::setFilename(const QString &file1, const QString &file2)
{
    int sleepTime = 100;

    shouldWait = true;
    requestSetFilename(file1, file2);

    while (shouldWait) {
        Sleep(sleepTime);
    }

    return retVal;
}

QScriptValue ImagineScript::printWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString x = context->argument(0).toString();
    if(instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->print(x));
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

QScriptValue ImagineScript::validityCheckWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString file1 = "", file2 = "";

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->validityCheck(file1, file2));
}

QScriptValue ImagineScript::recordWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString file1 = "", file2 = "";
    long int timeout;

    readFilenamesNTimeout(context, file1, file2, timeout);
    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->record(file1, file2, timeout));
}

QScriptValue ImagineScript::loadConfigWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString file1, file2;
    int numArg = context->argumentCount();

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->loadConfig(file1, file2));
}

QScriptValue ImagineScript::loadWaveformWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString file;
    int numArg = context->argumentCount();

    if (numArg)
        file = context->argument(0).toString();

    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->loadWaveform(file));
}

QScriptValue ImagineScript::sleepWrapper(QScriptContext *context, QScriptEngine *se)
{
    long int sleepTime;
    int numArg = context->argumentCount();

    if(numArg)
        sleepTime = context->argument(0).toUInt32();

    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->sleep(sleepTime));
}

QScriptValue ImagineScript::setFilenameWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString file1 = "", file2 = "";

    readFilenames(context, file1, file2);
    if (instance)
        return QScriptValue(se, static_cast<ImagineScript*>(instance)->setFilename(file1, file2));
}

ImagineScript::ImagineScript(QString rig)
{
    this->rig = rig;
    se = new QScriptEngine;
    instance = this;
    QScriptValue svPrint = se->newFunction(printWrapper);
    se->globalObject().setProperty("print", svPrint);
    QScriptValue svValid = se->newFunction(validityCheckWrapper);
    se->globalObject().setProperty("validityCheck", svValid);
    se->globalObject().setProperty("applyConfiguration", svValid);
    QScriptValue svRecord = se->newFunction(recordWrapper);
    se->globalObject().setProperty("record", svRecord);
    QScriptValue svLoadCon = se->newFunction(loadConfigWrapper);
    se->globalObject().setProperty("loadConfig", svLoadCon);
    QScriptValue svLoadWav = se->newFunction(loadWaveformWrapper);
    se->globalObject().setProperty("loadWaveform", svLoadWav);
    QScriptValue svSleep = se->newFunction(sleepWrapper);
    se->globalObject().setProperty("sleep", svSleep);
    QScriptValue svSetFile = se->newFunction(setFilenameWrapper);
    se->globalObject().setProperty("setOutputFilename", svSetFile);
}

ImagineScript::~ImagineScript()
{
    delete se;
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
    if (se == NULL)
        return false;
    QScriptValue jsobj = se->evaluate(*scriptProgram);
    if (se->hasUncaughtException()) {
        errorMsg.append(
            QString("There's problem when evaluating script:\n%1\n%2.")
            .arg(QString("   ... at line %1").arg(se->uncaughtExceptionLineNumber()))
            .arg(QString("   ... error msg: %1").arg(se->uncaughtException().toString())));
        newMsgReady(errorMsg);
        return false;
    }
    newMsgReady("Enecution succeeded\n");
    return true;
}

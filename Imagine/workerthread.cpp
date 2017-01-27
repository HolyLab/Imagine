#include <QtCore>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "workerthread.h"

WorkerThread::WorkerThread(QThread::Priority defaultPriority, QObject *parent)
    : QThread(parent)
{
    this->defaultPriority = defaultPriority;
    shouldStop = false;
    isPaused = false;
//    gt = nullptr;
}

WorkerThread::~WorkerThread() {}
/*
void WorkerThread::setTimer(Timer_g* t)
{
    gt = t;
}

Timer_g * WorkerThread::getTimer()
{
    return gt;
}
*/

void WorkerThread::requestStop() {
    setShouldStop(true);
    setIsPaused(false);
    QThread::msleep(200);
    getUnPaused()->wakeAll();
}

void WorkerThread::pauseUntilWake() {
    setIsPaused(true); //there is potential for a race condition here, because the nextStack() of CookeCamera can try to wake the wait condition as soon as this function is called
                        //in practice this doesn't seem to happen, but could become an issue in the future
    unPauseLock.lock();
    getUnPaused()->wait(&unPauseLock);
    unPauseLock.unlock();
}

bool WorkerThread::getIsPaused() {return isPaused.load();}
bool WorkerThread::getShouldStop() {return shouldStop;}
QWaitCondition * WorkerThread::getUnPaused() { return &unPaused; }
QThread::Priority WorkerThread::getDefaultPriority() { return defaultPriority; }

void WorkerThread::setIsPaused(bool val) { isPaused = val; }
void WorkerThread::setShouldStop(bool val) { shouldStop = val; };
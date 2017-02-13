#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>
//#include <timer_g.hpp>

class WorkerThread : public QThread 
{
    Q_OBJECT
protected:

    QThread::Priority defaultPriority = QThread::IdlePriority;
    QMutex unPauseLock;
    //Timer_g * gt;
    //shouldStop is modified when the acquisition is complete (i.e. after a sequence of stacks)
    volatile bool shouldStop;
    QWaitCondition unPaused;
    //isPaused is modified when camera buffers are removed (i.e. between stacks)
    std::atomic_bool isPaused;

public:

    WorkerThread(QThread::Priority defaultPriority, QObject *parent = 0);

    virtual ~WorkerThread();

    //virtual void setTimer(Timer_g * t);
    //virtual Timer_g * getTimer();

    virtual void requestStop();

    virtual void pauseUntilWake();

    virtual bool getIsPaused();
    virtual bool getShouldStop();
    virtual QWaitCondition * getUnPaused();
    virtual QThread::Priority getDefaultPriority();

    virtual void setIsPaused(bool val);
    virtual void setShouldStop(bool val);

};//class, WorkerThread


#endif // WORKERTHREAD_H

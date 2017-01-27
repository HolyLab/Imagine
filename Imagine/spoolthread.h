#ifndef SPOOLTHREAD_H
#define SPOOLTHREAD_H

#include "fast_ofstream.hpp"
#include "workerthread.h"

#include <QThread>
#include <QWaitCondition>

class Camera;

class SpoolThread : public WorkerThread {
    Q_OBJECT
private:
    FastOfstream *ofsSpooling; //NOTE: SpoolThread is not the owner
    Camera* camera;

    QWaitCondition bufNotFull;

    volatile bool shouldStop; //todo: do we need a lock to guard it?

public:

    // ctor and dtor
    SpoolThread::SpoolThread(QThread::Priority defaultPriority, FastOfstream *ofsSpooling, Camera* camera, QObject *parent = 0);
    SpoolThread::~SpoolThread();

    void run();
};//class, SpoolThread


#endif // SPOOLTHREAD_H

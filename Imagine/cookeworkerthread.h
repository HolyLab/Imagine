#ifndef COOKEWORKERTHREAD_H
#define COOKEWORKERTHREAD_H

#include "cooke.hpp"
#include "cameraworkerthread.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

class CookeWorkerThread : public CameraWorkerThread {
    Q_OBJECT
private:
    CookeCamera* camera;

    //Note: This should only be called when already paused, pending images are cancelled, and recording is stopped.
    void prepareNextStack(int lastBufferIdx);


public:

    CookeWorkerThread(QThread::Priority defaultPriority, CookeCamera * camera, QObject *parent = 0);

    ~CookeWorkerThread();

    void run();

};//class, CookeWorkerThread


#endif // COOKEWORKERTHREAD_H

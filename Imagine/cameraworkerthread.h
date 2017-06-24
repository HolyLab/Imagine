#ifndef CAMERAWORKERTHREAD_H
#define CAMERAWORKERTHREAD_H

#include <QThread>
#include "camera_g.hpp"
#include "workerthread.h"

class CameraWorkerThread : public WorkerThread 
{
    Q_OBJECT
protected:
    Camera* camera;

    //Note: This should only be called when already paused, pending images are cancelled, and recording is stopped.
    virtual void prepareNextStack(int lastBufferIdx) {}


public:

    CameraWorkerThread(QThread::Priority defaultPriority, Camera * camera, QObject *parent = 0);

    virtual ~CameraWorkerThread();

    virtual void nextStack() {};

};//class, CameraWorkerThread


#endif // CAMERAWORKERTHREAD_H

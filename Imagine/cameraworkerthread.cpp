#include <QThread>

#include "cameraworkerthread.h"

CameraWorkerThread::CameraWorkerThread(QThread::Priority defaultPriority, Camera * camera, QObject *parent)
    : WorkerThread(defaultPriority, parent)
{
    this->camera = camera;
}

CameraWorkerThread::~CameraWorkerThread() {}
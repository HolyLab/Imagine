/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
**    All rights reserved.
** Author: All code authored by Zhongsheng Guo.
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/


#include "camera_g.hpp"

Camera::Camera()
{
    hbin = vbin = 1;
    hstart = vstart = 1;
	hend = 8;
    vend = 8; //
    timer = nullptr;


    pBlackImage = NULL;
    imageSizePixels = 0;

    circBufLock = new QMutex;

    nFrames = 100;  //todo: make this as a param
    nAcquiredFrames = 0;
    nAcquiredStacks = 0;
    model = "Unknown";

    spoolingFilename = "";
    spoolThread = nullptr;
    ofsSpooling = nullptr;
    workerThread = nullptr;
}

Camera::~Camera()
{
    freeMemPool();
    _aligned_free(pBlackImage);
    delete spoolThread;
    delete ofsSpooling;
    delete workerThread;
}

WorkerThread* Camera::getWorkerThread() { return workerThread; }
SpoolThread* Camera::getSpoolThread() { return spoolThread; }
int Camera::getNAcquiredFrames() { return nAcquiredFrames.load(); }
int Camera::getNAcquiredStacks() {return nAcquiredStacks.load();}

void Camera::setWorkerThread(WorkerThread* w) { workerThread = w; }
void Camera::setSpoolThread(SpoolThread* s) { spoolThread = s; }
void Camera::setNAcquiredFrames(int n) {nAcquiredFrames = n; }
void Camera::setNAcquiredStacks(int n) { nAcquiredStacks = n; }
void Camera::incrementNAcquiredFrames() {nAcquiredFrames += 1;}
void Camera::incrementNAcquiredStacks() { nAcquiredStacks += 1; }

bool Camera::allocMemPool(long long sz) {
    if (sz < 0) {
#ifdef _WIN64
        memPoolSize = (long long)4218880 * 2 * 400; //400 full frames (~4GB). 5529600 per frame for PCO.Edge 5.5 TODO: make this user-configuable
#else
        memPoolSize = 5529600 * 2 * 30; //30 full frames
#endif
    }//if, use default
    else memPoolSize = sz;
    memPool = (char*)_aligned_malloc(memPoolSize, 1024 * 64);
#ifdef _WIN64
    assert((unsigned long long)(memPool) % (1024 * 64) == 0);
#else
    assert((unsigned long)(camera->memPool) % (1024 * 64) == 0);
#endif

    return memPool;
}
void Camera::freeMemPool() {
    _aligned_free(memPool);
    memPool = nullptr;
    memPoolSize = 0;
}

bool Camera::allocBlackImage()
{
    pBlackImage = (PixelValue*)_aligned_malloc(imageSizeBytes, 4 * 1024);
    memset(pBlackImage, 0, imageSizeBytes); //all zeros
    return true;
}

//Note:  this function will fail (possibly in an ugly way) if the circular buffer overflows
bool Camera::updateLiveImage()
{
    if (!circBufLock->tryLock()) return false;

    long nFrames = nAcquiredFrames.load();

    if (nFrames <= 0 || workerThread->getIsPaused()) {
        circBufLock->unlock();
        return false;
    }
    long idx = circBuf->peekPut() - 1;
    if (idx == -1) {
        idx = circBuf->capacity() - 1; //if the latest image resides at the end of the circular buffer
    }
    circBufLock->unlock();

    const char * temp = memPool + idx*size_t(imageSizeBytes);
    //copies data
    //for optimization purposes we could also try QByteArray::fromRawData
    liveImage = QByteArray(temp, imageSizeBytes);
    return true;

    return true;
}

QByteArray &Camera::getLiveImage()
{
    return liveImage;
}

 //return false if, say, can't open the spooling file to save
 //NOTE: have to call this b4 setAcqModeAndTime() to avoid out-of-mem
bool Camera::setSpooling(string filename)
{
    if (ofsSpooling) {
        delete ofsSpooling; //the file is closed too
        ofsSpooling = nullptr;
    }

    spoolingFilename = filename;

    //the RAID controller for OCPI2 natively uses an 8kb buffer size
    __int64 bufsize_in_8kb = __int64(imageSizeBytes) * 64 / (8 * 1024); //64 frames
    __int64 total_in_bytes = __int64(imageSizeBytes) * nFramesPerStack * nStacks;
#ifdef _WIN64
    bufsize_in_8kb *= 1;
#endif
    ofsSpooling = new FastOfstream(filename.c_str(), total_in_bytes, bufsize_in_8kb);
    return *ofsSpooling;
}

//void Camera::setTimer(Timer_g * timer) { this->timer = timer; }

void Camera::updateImageParams(int nstacks, int nframesperstack) {
    imageSizePixels = getImageWidth() * getImageHeight();
    nStacks = nstacks;
    nFramesPerStack = nframesperstack;
    imageSizeBytes = imageSizePixels * 2; //TODO: hardcoded 2
}


bool Camera::stopAcqFinal()
{
    //not rely on the "wait abandon"!!!
    WorkerThread * w = getWorkerThread();
    SpoolThread * s = getSpoolThread();
    w->requestStop();
    s->requestStop();
    w->wait();
    s->wait();

    delete circBuf;
    delete getWorkerThread();
    delete getSpoolThread();
    spoolThread = nullptr;
    workerThread = nullptr;

    delete ofsSpooling; //the file is closed too
    ofsSpooling = nullptr;

    return true;
}

long Camera::getAcquiredFrameCount()
{
    return nAcquiredFrames.load();
}

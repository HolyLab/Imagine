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

#ifndef DATA_ACQ_THREAD_HPP
#define DATA_ACQ_THREAD_HPP

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "andor_g.hpp"

class QImage;
class NiDaqAi;

class DataAcqThread : public QThread
{
    Q_OBJECT

public:
    DataAcqThread(QObject *parent = 0);
    ~DataAcqThread();

    void startAcq();
    void stopAcq(); //note: this func call is non-blocking

    //intended camera params:
    int nStacks, 
       nFramesPerStack;
    double exposureTime, //in sec
       idleTimeBwtnStacks; //in sec
    int emGain;
    int preAmpGainIdx;
    int horShiftSpeedIdx;
    int verShiftSpeedIdx;
    int verClockVolAmp;
    bool isBaselineClamp;
    AndorCamera::TriggerMode triggerMode;
    int hstart, hend, vstart, vend; //binning params

    QString preAmpGain, horShiftSpeed, verShiftSpeed; //for the purpose of saving in header only

    //real params used by the camera:
    double cycleTime; //in sec

    //ao(piezo) params:
    int scanRateAo;
    double piezoPreTriggerTime; //in sec
    double piezoStartPos, piezoStopPos; //NOTE: voltage
    double piezoStartPosUm, piezoStopPosUm; //NOTE: in um. For the purpose of saving in the header only

    //stimulus:
    QString stimFileContent;
    bool applyStim;

    //file saving params:
    bool isSaveData;
    QString headerFilename, aiFilename, camFilename, sifFileBasename;
    //bool isCreateFilePerStack;
    bool isUseSpool;

    //comment:
    QString comment;

    bool isLive;
    int idxCurStack; 

signals:
    void imageDisplayReady(const QImage &image, long idx);
    void newStatusMsgReady(const QString &str);
    void newLogMsgReady(const QString &str);
    void imageDataReady(const QByteArray &data16, long idx, int imageW, int imageH);

protected:
    void run();
    void run_acq_and_save();
    void run_live();

private:
    bool saveHeader(QString filename, NiDaqAi* ai);
    void fireStimulus(int valve);

    QMutex mutex;
    QWaitCondition condition;
    bool restart;
    bool abort;
    volatile bool stopRequested;
};

#endif  //DATA_ACQ_THREAD_HPP

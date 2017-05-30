#include "waveform.h"
#include <iostream>
#include <QDebug>

using namespace std;

ControlWaveform::ControlWaveform() 
{
}

ControlWaveform::~ControlWaveform()
{
    for (int i = 0; i < waveList.size(); i++)
    {
        if (waveList[i] != nullptr)
            delete waveList[i];
    }
    for (int i = 0; i < controlList.size(); i++)
    {
        if (controlList[i] != nullptr)
            delete controlList[i];
    }
}

int ControlWaveform::lookUpWave(QString wn, QVector <QString> &wavName, QJsonObject wavelist)
{
    int i;

    for (i = 0; i < wavName.size(); i++)
    {
        if (wn.compare(wavName[i]) == 0)
            return i;
    }

    // If there is no matched string, find this wave from wavelist JSON object
    // and add this new array to waveList vector 
    QJsonArray jsonWave = wavelist[wn].toArray();
    QVector <int> *wav = new QVector<int>;
    int sum = 0;
    for (int j = 0; j < jsonWave.size(); j+=2) { 
        wav->push_back(sum);    // change run length to sample index number and save
        wav->push_back(jsonWave[j+1].toInt());
        sum += jsonWave[j].toInt();
    }
    wav->push_back(sum); // save total sample number to the last element
    waveList.push_back(wav);
    wavName.push_back(wn);
    return i;
}

bool ControlWaveform::loadJsonDocument(QJsonDocument &loadDoc)
{
    QJsonObject alldata;
    QJsonObject metadata;
    QJsonObject analog;
    QJsonObject digital;
    QJsonObject wavelist;

    QJsonArray piezo1, piezo2;
    QJsonArray camera1, camera2;
    QJsonArray laser1, laser2, laser3, laser4, laser5;
    QJsonArray stimulus1, stimulus2, stimulus3, stimulus4;
    QJsonArray stimulus5, stimulus6, stimulus7, stimulus8;

    // parsing
    alldata = loadDoc.object();
    version = alldata["version"].toString();
    if (version != "v1.0")
        return false;
    metadata = alldata["metadata"].toObject();
    sampleRate = metadata["sample rate"].toInt();
    totalSampleNum = metadata["sample num"].toInt();
    exposureTime = metadata["exposure"].toDouble();
    nStacks = metadata["stacks"].toInt();
    nFrames = metadata["frames"].toInt();
    analog = alldata["analog waveform"].toObject();
    digital = alldata["digital pulse"].toObject();
    wavelist = alldata["wave list"].toObject();
    bidirection = metadata["bi-direction"].toBool();

    // The order of this should be match with ControlSignal
    QString conName[] = { "positioner1", "positioner2", "camera1", "camera2",
        "laser1", "laser2", "laser3", "laser4", "laser5",
        "stimulus1", "stimulus2", "stimulus3", "stimulus4",
        "stimulus5", "stimulus6", "stimulus7", "stimulus8"}; 
    QVector<QString> wavName;
    QJsonArray control;
    for (int i = 0; i<sizeof(conName)/sizeof(QString); i++) {
//        qDebug() << "[" + conName[i] + "]\n";
        QVector<int> *cwf = new QVector<int>;
        if ( i < 2 )
            control = analog[conName[i]].toArray();
        else
            control = digital[conName[i]].toArray();
        if (!control.isEmpty()) {
            for (int i = 0; i < control.size(); i += 2) {
                int repeat = control[i].toInt();
                QString wn = control[i + 1].toString();
                int waveIdx = lookUpWave(wn, wavName, wavelist);
                cwf->push_back(repeat);
                cwf->push_back(waveIdx);
//                qDebug() << wn + "(" << waveIdx << ") " << repeat << "\n";
            }
        }
        controlList.push_back(cwf);
    }
    return true;
}

bool ControlWaveform::isEmpty(ControlSignal name)
{
    if (controlList.isEmpty())
        return true;
    return controlList[name]->isEmpty();
}

int ControlWaveform::getWaveSampleNum(int waveIdx)
{
    if (waveList.isEmpty())
        return 0;
    int  sampleNum = waveList[waveIdx]->last();
    return sampleNum;
}

int ControlWaveform::getCtrlSampleNum(ControlSignal name)
{
    if (controlList.isEmpty())
        return 0;
    int sum = 0;
    int controlIdx, repeat, waveIdx;

    for (controlIdx = 0; controlIdx < controlList[name]->size(); controlIdx += 2)
    {
        repeat = controlList[name]->at(controlIdx);
        waveIdx = controlList[name]->at(controlIdx + 1);
        sum += repeat*waveList[waveIdx]->last();
    }
    return sum;
}

bool ControlWaveform::getCtrlSampleValue(ControlSignal name, int idx, int &value)
{
    if (controlList.isEmpty())
        return false;
    int controlIdx, repeat, waveIdx;
    int sampleIdx = 0, prevSampleIdx, sampleIdxInWave;
    bool found = false;
    bool isValid;
    // find beginning point
    for (controlIdx = 0; controlIdx < controlList[name]->size(); controlIdx += 2)
    {
        repeat = controlList[name]->at(controlIdx);
        waveIdx = controlList[name]->at(controlIdx + 1);
        for (int j = 0; j < repeat; j++)
        {
            prevSampleIdx = sampleIdx;
            sampleIdx += getWaveSampleNum(waveIdx);
            if (idx < sampleIdx) {
                found = true;
                goto read_data;
            }
        }
    }
idx_error:
    return false;

read_data:
    sampleIdxInWave = idx - prevSampleIdx;
    isValid = getWaveSampleValue(waveIdx, sampleIdxInWave, value);
    if (!isValid)
        goto idx_error;
done:
    return true;
}

// binary search recursive function
int findValue(QVector<int> *wav, int begin, int end, int sampleIdx)
{
    int mid =  (begin + end) / 2;
    int midIdx = mid*2;
    int x0, x1;

    if (sampleIdx >= wav->at(midIdx)) {
        if (sampleIdx < wav->at(midIdx + 2)) {
            return wav->at(midIdx + 1);
        }
        else {
            x0 = mid;
            x1 = end;
            return findValue(wav, x0, x1, sampleIdx);
        }
    }
    else {
        if (sampleIdx >= wav->at(midIdx - 2)) {
            return wav->at(midIdx - 1);
        }
        else {
            x0 = begin;
            x1 = mid;
            return findValue(wav, x0, x1, sampleIdx);
        }
    }
}

bool ControlWaveform::getWaveSampleValue(int waveIdx, int sampleIdx, int &value)
{
    if (waveList.isEmpty())
        return false;

    int sum = 0;

    if (sampleIdx >= waveList[waveIdx]->last()) {
        return false;
    }
    int begin = 0;
    int end = waveList[waveIdx]->size() / 2;
    value = findValue(waveList[waveIdx], begin, end, sampleIdx);
    return true;
}

bool ControlWaveform::readControlWaveform(QVector<int> &dest, ControlSignal name,
    int begin, int end, int downSampleRate)
{
    if (controlList.isEmpty())
        return false;

    int controlIdx = 0, repeat, waveIdx, repeatLeft;
    int sampleIdx = begin, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;

    while (1) {
        for (; controlIdx < controlList[name]->size(); controlIdx += 2) {
            repeat = controlList[name]->at(controlIdx);
            repeatLeft = repeat;
            waveIdx = controlList[name]->at(controlIdx + 1);
            for (int j = 0; j < repeat; j++) {
                prevStartSampleIdx = startSampleIdx;
                startSampleIdx += getWaveSampleNum(waveIdx);
                if (sampleIdx < startSampleIdx) {
                    for (; sampleIdx < startSampleIdx; sampleIdx += downSampleRate) {
                        sampleIdxInWave = sampleIdx - prevStartSampleIdx;
                        int value;
                        bool isValid = getWaveSampleValue(waveIdx, sampleIdxInWave, value);
                        if (isValid)
                            dest.push_back(value);
                        else
                            goto idx_error;
                        if (sampleIdx >= end - downSampleRate + 1)
                            goto done;
                    }
                }
            }
        }
        break;
    }

idx_error:
    return false;
done:
    return true;
}

int ControlWaveform::positionerSpeedCheck(int maxSpeed, ControlSignal name, int userSampleRate, int &dataSize)
{
    dataSize = getCtrlSampleNum(name);
    if (dataSize == 0)
        return 0;
    int interval = 100;
    double distance, time, speed;
    int controlIdx, repeat, waveIdx;
    int sampleIdx, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;
    QVector<bool> usedWaveIdx(waveList.size(),false);
    int wave0, wave1, margin;

    time = static_cast<double>(interval) / static_cast<double>(userSampleRate); // sec

    for (controlIdx = 0; controlIdx < controlList[name]->size(); controlIdx += 2) {
        repeat = controlList[name]->at(controlIdx);
        waveIdx = controlList[name]->at(controlIdx + 1);
        if (usedWaveIdx[waveIdx] == false)
        {
            if (repeat > 1)
                margin = 0; // wraparound check
            else
                margin = interval; // only internal check
            for (int i = 0; i < getWaveSampleNum(waveIdx) - margin; i++) {
                getWaveSampleValue(waveIdx, i, wave0);
                getWaveSampleValue(waveIdx, (i + interval)%getWaveSampleNum(waveIdx), wave1);
                distance = abs(wave1 - wave0) - 1;
                if (distance > 1) {
                    speed = distance / time; // piezostep/sec
                    if (speed > maxSpeed) {
                        return -1;
                    }
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
        if (controlIdx >= 2) { // inter-connection point check
            int oldWaveIdx = controlList[name]->at(controlIdx - 1);
            for (int i = getWaveSampleNum(waveIdx) - interval, j = 0; i < getWaveSampleNum(waveIdx); i++, j++) {
                getWaveSampleValue(oldWaveIdx, i, wave0);
                getWaveSampleValue(waveIdx, j, wave1);
                distance = abs(wave1 - wave0) - 1;
                if (distance > 1) {
                    speed = distance / time; // piezostep/sec
                    if (speed > maxSpeed) {
                        return -1;
                    }
                }
            }
        }
    }
    return 1;
}

int ControlWaveform::laserSpeedCheck(double maxFreq, ControlSignal name, int &dataSize)
{
    dataSize = getCtrlSampleNum(name);
    if (dataSize == 0)
        return 0;
    int waveIdx, controlIdx;
    double freq, durationInSamples;
    QVector<bool> usedWaveIdx(waveList.size(), false);

    for (controlIdx = 0; controlIdx < controlList[name]->size(); controlIdx += 2) {
        waveIdx = controlList[name]->at(controlIdx + 1);
        if (usedWaveIdx[waveIdx] == false)
        {
            int wave0, wave1, lastIdx = -1;
            for (int i = 0; i < getWaveSampleNum(waveIdx)-1; i++) {
                getWaveSampleValue(waveIdx, i, wave0);
                getWaveSampleValue(waveIdx, i+1, wave1);
                if ((wave0 == 0) && (wave1 != 0)) { // check rising edge
                    durationInSamples = static_cast<double>(i - lastIdx);
                    freq = static_cast<double>(sampleRate) / durationInSamples;
                    if ((freq > maxFreq)&&(lastIdx > 0))
                        return -1;
                    lastIdx = i;
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
    }
    return 1;
}

/******* Generate waveform *******************************************/
bool conRunLengthToWav(QVariantList rlbuf, QVariantList &buf)
{
    buf.clear();
    for (int i = 0; i < rlbuf.size(); i +=2 ) {
        for (int j = 0; j < rlbuf.at(i).toInt(); j++) {
            buf.push_back(rlbuf.at(i + 1));
        }
    }
    return true;
}

bool conWavToRunLength(QVariantList buf, QVariantList &rlbuf)
{
    int value = 0, preValue;
    int j = 1;

    rlbuf.clear();
    for (int i = 0; i < buf.size(); i++,j++) {
        preValue = value;
        value = buf.at(i).toInt();
        if (i&&(value != preValue)) {
            rlbuf.push_back(j);
            rlbuf.push_back(preValue);
            j = 0;
        }
    }
    rlbuf.push_back(j);
    rlbuf.push_back(preValue);
    return true;
}

int getRunLengthSize(QVariantList rlbuf)
{
    int sum = 0;
    for (int i = 0; i < rlbuf.size(); i += 2) {
        sum += rlbuf.at(i).toInt();
    }
    return sum;
}

void genWaveform(QJsonArray &jarray, double startPos, double stopPos, int nDelay, int nRising, int nFalling, int nTotal)
{
    QVariantList* list = new QVariantList;
    qint16 wav, wavPrev;
    int runLength = 0;
    double  avg = startPos;

    for (int i = 0; i < nTotal; i++)
    {
        int ii = i % nTotal;
        if ((ii >= nDelay) && (ii < nDelay + nRising)) {
            avg += (stopPos - startPos) / static_cast<double>(nRising);
        }
        else if ((ii >= nDelay + nRising) &&
            (ii < nDelay + nRising + nFalling)) {
            avg -= (stopPos - startPos) / static_cast<double>(nFalling);
        }
        wav = static_cast<qint16>(avg + 0.5);
        if ((wav != wavPrev) && i) {
            list->append(runLength);
            list->append(wavPrev);
            runLength = 1;
        }
        else {
            runLength++;
        }
        wavPrev = wav;
    }
    list->append(runLength);
    list->append(wavPrev);
    // QJsonArray::append() is too slow
    // So, we store data to QVariantList container first.
    // Then, we convert it using QJsonArray::fromVariantList
    // to contain the data to QJsonArray.
    jarray = QJsonArray::fromVariantList(*list);
    delete list;
}

void genPulse(QJsonArray &jarray, int nDelay, int nRising, int nFalling, int nTotal, int nMargin, int nShutter, int nExposure)
{
    QVariantList* list = new QVariantList;
    int pulse, pulsePrev;
    int runLength = 0;

    for (int i = 0; i < nTotal; i++)
    {
        int ii = i % nTotal;
        if ((ii >= nDelay + nMargin) &&
            (ii < nDelay + nRising - nMargin)) {
            int j = (ii - nDelay - nMargin) % nShutter;
            if ((j >= 0) && (j <= nExposure)) {
                pulse = 1;
            }
            else {
                pulse = 0;
            }
        }
        else if ((ii >= nDelay + nRising - nMargin) &&
            (ii < nDelay + nRising + nFalling)) {
            pulse = 0;
        }
        else {
            pulse = 0;
        }
        if ((pulse != pulsePrev) && i) {
            list->append(runLength);
            list->append(pulsePrev);
            runLength = 1;
        }
        else {
            runLength++;
        }
        pulsePrev = pulse;
    }
    list->append(runLength);
    list->append(pulsePrev);
    jarray = QJsonArray::fromVariantList(*list);
    delete list;
}

void genBiDirection(QVariantList &vlarray, QVariantList refArray, int nDelay, int nRising, int nIdleTime)
{
    int i;
    QVariantList buf1, buf2;
    conRunLengthToWav(refArray, buf1);

    for (i = 0; i < nDelay + nRising; i++) {
        int value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    i--;
    for (int j = nDelay; j < nDelay + nRising; j++, i--) {
        int value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    for (int j = 0; j < nIdleTime; j++) {
        int value = buf1.at(i).toInt(0);
        buf2.push_back(value);
    }
    conWavToRunLength(buf2, vlarray);
}

void genCosineWave(QVariantList &vlarray, QVariantList &vlparray, QVariantList &vlparray2, double amplitude, int offset, int freq, int pulseNum)
{
    double value;
    double damp = static_cast<double>(amplitude);
    double dfreq = static_cast<double>(freq);
    dfreq = M_PI / dfreq;
    int margin = 100;
    int level = offset - amplitude + margin / 2;
    int levelStep = (2 * amplitude - 100) / pulseNum;
    int j = 0;
    QVariantList buf1, buf2, buf3;
    int initLevel = level;

    for (int i = 0; i < freq; i++) {
        value = -damp*cos(dfreq*static_cast<double>(i));
        int ival = static_cast<int>(value) + offset;
        buf1.push_back(ival);
        if (ival >= level) {
            if (j < 60) {
                buf2.push_back(1);
                j++;
            }
            else {
                buf2.push_back(0);
                level += levelStep;
                j = 0;
            }
        }
        else {
            buf2.push_back(0);
        }
        if (ival >= initLevel)
            buf3.push_back(1);
        else
            buf3.push_back(0);
    }
    conWavToRunLength(buf1, vlarray);
    conWavToRunLength(buf2, vlparray);
    conWavToRunLength(buf3, vlparray2);
}

void genMoveFromToWave(QVariantList &vlarray, int startPos, int stopPos, int duration)
{
    QVariantList buf;
    double slope = static_cast<double>(stopPos - startPos) / static_cast<double>(duration);
    double value = static_cast<double>(startPos);

    for (int i = 0; i < duration; i++) {
        buf.push_back(static_cast<int>(value));
        value += slope;
    }
    conWavToRunLength(buf, vlarray);
}

void genConstantWave(QVariantList &vlarray, int value, int duration)
{
    vlarray.clear();
    vlarray.push_back(duration);
    vlarray.push_back(value);
}

int genControlFileJSON(void)
{
    enum SaveFormat {
        Json, Binary
    };
    QString version = "v1.0";
    QJsonObject alldata;
    QJsonObject metadata;
    QJsonObject analog;
    QJsonObject digital;
    QJsonObject wavelist;
    QJsonArray piezo1_001;
    QJsonArray camera1_001;
    QJsonArray laser1_001;
    QJsonArray valve_on, valve_off;
    QJsonArray piezo1, camera1, laser1, stimulus1, stimulus2;
    QJsonArray piezo2, camera2, laser2, stimulus3, stimulus4;

    qint16 piezo;
    bool shutter, laser, ttl2;
    double piezom1, piezo0, piezop1, piezoavg = 0.;

    /* For metadata                     */
    int sampleRate = 10000;     // This can be configurable in the GUI
    long totalSamples = 0;      // this will be calculated later;
    int nStacks = 10;           // number of stack
    int nFrames = 20;           // frames per stack
    double exposureTime = 0.01; // This can be configurable in the GUI
    bool bidirection = false;   // Bi-directional capturing mode

    /* Generating waveforms and pulses                            */
    /* Refer to help menu in Imagine                              */

    ////////  Generating subsequences of waveforms and pulses  ////////

    ///  Piezo waveform  ///
    //  paramters specified by user
    int piezoDelaySamples = 100;
    int piezoRisingSamples = 6000;
    int piezoFallingSamples = 4000;
    int piezoWaitSamples = 800;
    double piezoStartPos = 100.;
    double piezoStopPos = 800.;
    // generating piezo waveform
    int perStackSamples = piezoRisingSamples + piezoFallingSamples + piezoWaitSamples;
    genWaveform(piezo1_001, piezoStartPos, piezoStopPos, piezoDelaySamples,
        piezoRisingSamples, piezoFallingSamples, perStackSamples);

    /// initial delay ///
    //  paramters specified by user
    int initDelaySamples = 500;
    // generating delay waveform
    QVariantList buf1;
    genConstantWave(buf1, piezoStartPos, initDelaySamples);
    QJsonArray initDelay_wav = QJsonArray::fromVariantList(buf1);
    // generating delay pulse
    genConstantWave(buf1, 0, initDelaySamples);
    QJsonArray initDelay_pulse = QJsonArray::fromVariantList(buf1);

    /// camera shutter pulse ///
    // paramters specified by user
    int shutterControlMarginSamples = 500; // shutter control begin after piezo start and stop before piezo stop
    // generating camera shutter pulse
    int stakShutterCtrlSamples = piezoRisingSamples - 2 * shutterControlMarginSamples; // full stack period sample number
    int frameShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    int exposureSamples = static_cast<int>(exposureTime*static_cast<double>(sampleRate));  // (0.008~0.012sec)*(sampling rate) is optimal
    if (exposureSamples > frameShutterCtrlSamples - 50 * (sampleRate / 10000))
        return 1;
    genPulse(camera1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, frameShutterCtrlSamples, exposureSamples);

    /// laser shutter pulse ///
    // paramters specified by user
    int laserControlMarginSamples = 400; // laser control begin after piezo start and stop before piezo stop
    // generating laser shutter pulse
    stakShutterCtrlSamples = piezoRisingSamples - 2 * laserControlMarginSamples; // full stack period sample number
    int laserShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    genPulse(laser1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, laserShutterCtrlSamples, laserShutterCtrlSamples);

    /// stimulus valve pulse ///
    // paramters specified by user
    int valveOffSamples = 100;
    int valveOnSamples = 6000;
    // generating stimulus valve pulse
    genConstantWave(buf1, 0, perStackSamples);
    valve_off = QJsonArray::fromVariantList(buf1);
    genPulse(valve_on, 0, valveOnSamples + 2*valveOffSamples, piezoFallingSamples, perStackSamples,
        valveOffSamples, valveOnSamples, valveOnSamples);

    // these values are calculated from user specified parameters
    piezoavg = piezoStartPos;
    ttl2 = false;

    ///  Register subsequences to waveform list  ///
    wavelist["delay_wave"] = initDelay_wav;
    wavelist["delay_pulse"] = initDelay_pulse;
    wavelist["positioner1_001"] = piezo1_001;
    wavelist["camera1_001"] = camera1_001;
    wavelist["laser1_001"] = laser1_001;
    wavelist["valve_on"] = valve_on;
    wavelist["valve_off"] = valve_off;


    ////////  Generating full sequences of waveforms and pulses  ////////
    piezo1 = { 1, "delay_wave", nStacks, "positioner1_001"};
    camera1 = { 1, "delay_pulse", nStacks, "camera1_001" };
    laser1 = { 1, "delay_pulse", nStacks, "laser1_001" };
    stimulus1.append(1);
    stimulus1.append("delay_pulse");
    stimulus2.append(1);
    stimulus2.append("delay_pulse");
    stimulus1.append(2); // stack 1, 2
    stimulus1.append("valve_off");
    stimulus2.append(2);
    stimulus2.append("valve_off");
    for (int i = 3; i <= nStacks; i++) { // stack 3 ~ nStack
        int j = i % 4;
        if (j == 0) {
            stimulus1.append(2);
            stimulus1.append("valve_on");
            stimulus2.append(2);
            stimulus2.append("valve_off");
        }
        else if(j == 2){
            stimulus1.append(2);
            stimulus1.append("valve_off");
            stimulus2.append(2);
            stimulus2.append("valve_on");
        }
    }
    totalSamples = initDelaySamples + nStacks * perStackSamples;

    analog["positioner1"] = piezo1;
    digital["camera1"] = camera1;
    digital["camera2"] = camera2;
    digital["laser1"] = laser1;
    digital["stimulus1"] = stimulus1;
    digital["stimulus2"] = stimulus2;

    metadata["sample rate"] = sampleRate;
    metadata["sample num"] = totalSamples;
    metadata["exposure"] = exposureTime;
    metadata["bi-direction"] = bidirection;
    metadata["stacks"] = nStacks;
    metadata["frames"] = nFrames;

    alldata["metadata"] = metadata;
    alldata["analog waveform"] = analog;
    alldata["digital pulse"] = digital;
    alldata["wave list"] = wavelist;
    alldata["version"] = version;
    // save data
    SaveFormat saveFormat = Json;// Binary;Json
    QFile saveFile(saveFormat == Json
        ? QStringLiteral("controls.json")
        : QStringLiteral("controls.bin"));
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }
    QJsonDocument saveDoc(alldata);
    saveFile.write(saveFormat == Json
        ? saveDoc.toJson()
        : saveDoc.toBinaryData());
    return 0;
}

int genControlFileJSON_Old(void)
{
    enum SaveFormat {
        Json, Binary
    };
    QString version = "v1.0";
    QJsonObject alldata;
    QJsonObject metadata;
    QJsonObject analog;
    QJsonObject digital;
    QJsonObject wavelist;
    QJsonArray piezo1_001, piezo1_002;
    QJsonArray camera1_001, camera1_002;
    QJsonArray laser1_001, laser1_002;
    QJsonArray stimulus1_001, stimulus1_002;
    QJsonArray stimulus2_001, stimulus2_002;
    QJsonArray piezo1, camera1, laser1, stimulus1, stimulus2;
    QJsonArray piezo2, camera2, laser2, stimulus3, stimulus4;

    qint16 piezo;
    bool shutter, laser, ttl2;
    double piezom1, piezo0, piezop1, piezoavg = 0.;
    // Parameter
    // ampleRate, tatalSamples, nStacks, nFrames
    // 1000, 16800000, 19200, 20 (16800000/1000 = 16800sec = 4.6hr)
    // 10000, 7000000, 800, 20 (7000000/10000 = 700sec = 11.6min)
    int sampleRate = 10000; // 2000
    long totalSamples = 70000;// 70000;
    int nStacks = 8;// 8; // 5
    int nFrames = 20;
    double exposureTime = 0.01; // sec : 0.008 ~ 0.012 is optimal
    bool bidirection = false;
    // prepare data

    int perStackSamples = totalSamples / nStacks;
    int piezoDelaySamples = sampleRate / 100; // 4000
    int piezoRisingSamples = 60 * (sampleRate / 100);
    int piezoFallingSamples = 20 * (sampleRate / 100);
    double piezoStartPos = 100.;
    double piezoStopPos = 400.;
    int shutterControlMarginSamples = 5 * (sampleRate / 100); // shutter control begin and end from piezo start and stop
    int stakShutterCtrlSamples = piezoRisingSamples - 2 * shutterControlMarginSamples; // full stack period sample number
    int frameShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    int exposureSamples = static_cast<int>(exposureTime*static_cast<double>(sampleRate));  // (0.008~0.012sec)*(sampling rate) is optimal

    if (exposureSamples>frameShutterCtrlSamples - 50 * (sampleRate / 10000))
        return 1;
    piezoavg = piezoStartPos;
    ttl2 = false;

    genWaveform(piezo1_001, piezoStartPos, piezoStopPos, piezoDelaySamples,
        piezoRisingSamples, piezoFallingSamples, perStackSamples);
    genWaveform(piezo1_002, piezoStartPos, piezoStopPos, piezoDelaySamples,
        piezoFallingSamples, piezoRisingSamples, perStackSamples);
    genPulse(camera1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, frameShutterCtrlSamples, exposureSamples);
    genPulse(camera1_002, piezoDelaySamples, piezoFallingSamples, piezoRisingSamples, perStackSamples,
        shutterControlMarginSamples, frameShutterCtrlSamples, exposureSamples);
    genPulse(laser1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, frameShutterCtrlSamples, frameShutterCtrlSamples);
    genPulse(laser1_002, piezoDelaySamples, piezoFallingSamples, piezoRisingSamples, perStackSamples,
        shutterControlMarginSamples, frameShutterCtrlSamples, frameShutterCtrlSamples);
    genPulse(stimulus1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        0, frameShutterCtrlSamples, frameShutterCtrlSamples);
    genPulse(stimulus1_002, piezoDelaySamples, piezoFallingSamples, piezoRisingSamples, perStackSamples,
        0, frameShutterCtrlSamples, frameShutterCtrlSamples);
    wavelist["positioner1_001"] = piezo1_001;
    wavelist["positioner1_002"] = piezo1_002;
    wavelist["camera1_001"] = camera1_001;
    wavelist["camera1_002"] = camera1_002;
    wavelist["laser1_001"] = laser1_001;
    wavelist["laser1_002"] = laser1_002;
    wavelist["stimulus1_001"] = stimulus1_001;
    wavelist["stimulus1_002"] = stimulus1_002;
    wavelist["stimulus2_001"] = stimulus2_001;
    wavelist["stimulus2_002"] = stimulus2_002;

    // This is for bi-directional capturing
    QVariantList buf1, buf2;
    genBiDirection(buf1, piezo1_001.toVariantList(), piezoDelaySamples, piezoRisingSamples, 300);
    QJsonArray piezo_bidirection = QJsonArray::fromVariantList(buf1);
    genBiDirection(buf1, camera1_001.toVariantList(), piezoDelaySamples, piezoRisingSamples, 300); // TODO: pulse should not be just mirrored but needs some delay
    QJsonArray camera_bidirection = QJsonArray::fromVariantList(buf1);
    genBiDirection(buf1, laser1_001.toVariantList(), piezoDelaySamples, piezoRisingSamples, 300);
    QJsonArray laser_bidirection = QJsonArray::fromVariantList(buf1);
    genBiDirection(buf1, stimulus1_001.toVariantList(), piezoDelaySamples, piezoRisingSamples, 300);
    QJsonArray stimulus_bidirection = QJsonArray::fromVariantList(buf1);
    wavelist["piezo_bidirection"] = piezo_bidirection;
    wavelist["camera_bidirection"] = camera_bidirection;
    wavelist["laser_bidirection"] = laser_bidirection;
    wavelist["stimulus_bidirection"] = stimulus_bidirection;

    // This is for sine wave
    QVariantList buf3;
    int amplitude = 150;
    int offset = 250;
    int freqInSampleNum = 10000;
    piezoStartPos = 100;
    piezoStopPos = offset;
    int durationInSample = (piezoStopPos - piezoStartPos)*sampleRate / 2000 + 50; // this should longer than (piezoStopPos- piezoStartPos)*sampleRate/piezo_maxspeed
                                                                                  // buf1: wave, buf2: pulse
    genCosineWave(buf1, buf2, buf3, amplitude, offset, freqInSampleNum, nFrames);
    QVariantList buf11, buf22, buf33;
    // buf11: bi-directional wave
    genBiDirection(buf11, buf1, 0, getRunLengthSize(buf1), 0);
    QJsonArray piezo_cos = QJsonArray::fromVariantList(buf11);
    // buf22: bi-directional pulse
    genBiDirection(buf22, buf2, 0, getRunLengthSize(buf2), 0);
    QJsonArray pulse_cos = QJsonArray::fromVariantList(buf22);
    // buf22: bi-directional pulse
    genBiDirection(buf33, buf3, 0, getRunLengthSize(buf3), 0);
    QJsonArray pulse2_cos = QJsonArray::fromVariantList(buf33);
    //    genMoveFromToWave(buf1, piezoStartPos, piezoStopPos, durationInSample);
    //    QJsonArray piezo_moveto = QJsonArray::fromVariantList(buf1);
    //    genConstantWave(buf1, 0, durationInSample);
    //    QJsonArray zero_wav = QJsonArray::fromVariantList(buf1);
    wavelist["piezo_cos"] = piezo_cos;
    wavelist["pulse_cos"] = pulse_cos;
    wavelist["pulse2_cos"] = pulse2_cos;
    //    wavelist["piezo_moveto"] = piezo_moveto;
    //    wavelist["zero_wav"] = zero_wav;

    /* Triangle pulses
    piezo1 = { 3, "positioner1_001", 2, "positioner1_001", nStacks - 3 - 2, "positioner1_001" };
    camera1 = { 3, "camera1_001", 2, "camera1_001", nStacks - 3 - 2, "camera1_001" };
    camera2 = { 3, "camera1_001", 2, "camera1_001", nStacks - 3 - 2, "camera1_001" };
    laser1 = { 3, "laser1_001", 2, "laser1_001", nStacks - 3 - 2, "laser1_001" };
    stimulus1 = { 3, "stimulus1_001", 2, "stimulus1_001", nStacks - 3 - 2, "stimulus1_001" };
    bidirection = false;
    totalSamples = 70000;
    */

    /* cosine */
    piezo1 = { nStacks, "piezo_cos" };
    camera1 = { nStacks, "pulse_cos" };
    //    camera2 = { 1, "zero_wav", nStacks, "pulse_cos" };
    laser1 = { nStacks, "pulse2_cos" };
    //    stimulus1 = { 1, "zero_wav", nStacks, "pulse_cos" };
    bidirection = true;
    totalSamples = nStacks*getRunLengthSize(piezo_cos.toVariantList());

    /* Bi-directional waveform
    piezo1 = { nStacks, "piezo_bidirection" };
    camera1 = { nStacks, "camera_bidirection" };
    camera2 = { nStacks, "camera_bidirection" };
    laser1 = { nStacks, "laser_bidirection" };
    stimulus1 = { nStacks, "stimulus_bidirection" };
    bidirection = true;
    totalSamples = nStacks*getRunLengthSize(piezo_bidirection.toVariantList());
    */

    analog["positioner1"] = piezo1;
    digital["camera1"] = camera1;
    digital["camera2"] = camera2;
    digital["laser1"] = laser1;
    digital["stimulus1"] = stimulus1;
    digital["stimulus2"] = stimulus2;

    metadata["sample rate"] = sampleRate;
    metadata["sample num"] = totalSamples;
    metadata["exposure"] = exposureTime;
    metadata["bi-direction"] = bidirection;
    metadata["stacks"] = nStacks;
    metadata["frames"] = nFrames;

    alldata["metadata"] = metadata;
    alldata["analog waveform"] = analog;
    alldata["digital pulse"] = digital;
    alldata["wave list"] = wavelist;
    alldata["version"] = version;
    // save data
    SaveFormat saveFormat = Json;// Binary;Json
    QFile saveFile(saveFormat == Json
        ? QStringLiteral("controls.json")
        : QStringLiteral("controls.bin"));
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }
    QJsonDocument saveDoc(alldata);
    saveFile.write(saveFormat == Json
        ? saveDoc.toJson()
        : saveDoc.toBinaryData());
    return 0;
}

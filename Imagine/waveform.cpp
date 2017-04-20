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
        qDebug() << "[" + conName[i] + "]\n";
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
                qDebug() << wn + "(" << waveIdx << ") " << repeat << "\n";
            }
        }
        controlList.push_back(cwf);
    }
    cout << endl;

    return true;
}

bool ControlWaveform::isEmpty(ControlSignal name)
{
    return controlList[name]->isEmpty();
}

int ControlWaveform::getWaveSampleNum(int waveIdx)
{
    int  sampleNum = waveList[waveIdx]->last();
    return sampleNum;
}

int ControlWaveform::getCtrlSampleNum(ControlSignal name)
{
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

/*
int ControlWaveform::positionerSpeedCheck(int maxSpeed, ControlSignal name)
{
    int interval = 5;
    double distance, time, speed;
    int dataSize = getCtrlSampleNum(name);
    for (int i = 0; i < dataSize - interval; i++) {
        int wave1, wave0;
        int isValid = getCtrlSampleValue(name, i + interval, wave1);
        if (isValid) {
            isValid = getCtrlSampleValue(name, i, wave0);
            if (isValid) {
                distance = (wave1 - wave0);
                time = static_cast<double>(interval) / static_cast<double>(sampleRate); // sec
                speed = distance / time; // piezostep/sec
                if ((distance != 1) && (abs(speed) >maxSpeed)) {
                    return -1;
                }
            }
            else
                return -2;
        }
        else
            return -2;
    }
    return dataSize;
}

int ControlWaveform::laserSpeedCheck(double maxFreq, ControlSignal name)
{
    int lastIdx = -1;
    double freq, durationInSamples;
    int dataSize = getCtrlSampleNum(name);
    for (int i = 0; i < dataSize - 1; i++) {
        int wave1, wave0;
        int isValid = getCtrlSampleValue(name, i + 1, wave1);
        if (isValid) {
            isValid = getCtrlSampleValue(name, i, wave0);
            if (isValid) {
                if ((wave0 == 0) && (wave1 != 0)) { // check rising edge
                    durationInSamples = static_cast<double>(i - lastIdx);
                    freq = static_cast<double>(sampleRate) / durationInSamples;
                    if (freq > maxFreq)
                        return -1;
                    lastIdx = i;
                }
            }
            else
                return -2;
        }
        else
            return -2;
    }
    return dataSize;
}
*/

int ControlWaveform::positionerSpeedCheck(int maxSpeed, ControlSignal name)
{
    int interval = 5;
    double distance, time, speed;
    int controlIdx, repeat, waveIdx;
    int sampleIdx, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;
    int dataSize = getCtrlSampleNum(name);
    QVector<bool> usedWaveIdx(waveList.size(),false);
    int wave0, wave1, margin;

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
                distance = (wave1 - wave0);
                time = static_cast<double>(interval) / static_cast<double>(sampleRate); // sec
                speed = distance / time; // piezostep/sec
                if ((distance != 1) && (abs(speed) >maxSpeed)) {
                    return -1;
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
        if (controlIdx >= 2) { // inter-connection point check
            int oldWaveIdx = controlList[name]->at(controlIdx - 1);
            for (int i = getWaveSampleNum(waveIdx) - interval, j = 0; i < getWaveSampleNum(waveIdx); i++, j++) {
                getWaveSampleValue(oldWaveIdx, i, wave0);
                getWaveSampleValue(waveIdx, j, wave1);
                distance = (wave1 - wave0);
                time = static_cast<double>(interval) / static_cast<double>(sampleRate); // sec
                speed = distance / time; // piezostep/sec
                if ((distance != 1) && (abs(speed) >maxSpeed)) {
                    return -1;
                }
            }
        }
    }
    return dataSize;
}

int ControlWaveform::laserSpeedCheck(double maxFreq, ControlSignal name)
{
    int waveIdx, controlIdx;
    double freq, durationInSamples;
    int dataSize = getCtrlSampleNum(name);
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
    return dataSize;
}

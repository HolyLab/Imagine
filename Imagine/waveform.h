#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QString>
#include <QObject>
#include <QtCore>
#include <vector>

using namespace std;

#define MAX_NUM_WAVEFORM 100

typedef enum {
    positioner1,
    positioner2,
    camera1,
    camera2,
    laser1,
    laser2,
    laser3,
    laser4,
    laser5,
    stimulus1,
    stimulus2,
    stimulus3,
    stimulus4,
    stimulus5,
    stimulus6,
    stimulus7,
    stimulus8
} ControlSignal;

class ControlWaveform : public QObject {
    Q_OBJECT

private:
    int retVal;
    QVector<QVector<int> *> controlList;
    QVector<QVector<int> *> waveList;

    int lookUpWave(QString wn, QVector <QString> &wavName, QJsonObject wavelist);
    int getWaveSampleNum(int waveIdx);
    bool getWaveSampleValue(int waveIdx, int sampleIdx, int &value);

public:
    QString version;
    int sampleRate;
    int totalSampleNum;
    int nStacks;
    int nFrames;
    double exposureTime;
    bool bidirection;

    ControlWaveform();
    ~ControlWaveform();

    bool loadJsonDocument(QJsonDocument &loadDoc);
    bool readControlWaveform(QVector<int> &dest, ControlSignal name,
            int begin, int end, int downSampleRate);
    bool isEmpty(ControlSignal name);
    int getCtrlSampleNum(ControlSignal name);
    bool getCtrlSampleValue(ControlSignal name, int idx, int &value);
    int positionerSpeedCheck(int maxSpeed, ControlSignal name);
    int laserSpeedCheck(double maxFreq, ControlSignal name);
}; // ControlWaveform

class WaveData : public QObject {
    Q_OBJECT

public:
    int sampleNum;
    int sampleRate;
    vector<double> piezo1;
    vector<double> piezo2;
    vector<int> camera1;
    vector<int> camera2;
    vector<int> laser1;
    vector<int> laser2;
    vector<int> laser3;
    vector<int> laser4;
    vector<int> laser5;
    vector<int> stimulus1;
    vector<int> stimulus2;
    vector<int> stimulus3;
    vector<int> stimulus4;
    vector<int> stimulus5;
    vector<int> stimulus6;
    vector<int> stimulus7;
    vector<int> stimulus8;
    vector<vector<double>> waveList;
    vector<vector<int>> pulseList;

    WaveData(void) {
        waveList.push_back(piezo1);
        waveList.push_back(piezo2);
        pulseList.push_back(camera1);
        pulseList.push_back(camera2);
        pulseList.push_back(laser1);
        pulseList.push_back(laser2);
        pulseList.push_back(laser3);
        pulseList.push_back(laser4);
        pulseList.push_back(laser5);
        pulseList.push_back(stimulus1);
        pulseList.push_back(stimulus2);
        pulseList.push_back(stimulus3);
        pulseList.push_back(stimulus4);
        pulseList.push_back(stimulus5);
        pulseList.push_back(stimulus6);
        pulseList.push_back(stimulus7);
        pulseList.push_back(stimulus8);
    }
    ~WaveData() {}

    bool isWavIndexValid(int index)
    {
        int listStart = static_cast<int>(ControlSignal::positioner1);
        int listEnd = static_cast<int>(ControlSignal::positioner2);
        if ((index < listStart) || (index > listEnd)) {
            if (waveList[index].empty())
                return false;
        }
        else
            return false;
    }

    bool isPulseIndexValid(int index)
    {
        int listStart = static_cast<int>(ControlSignal::camera1);
        int listEnd = static_cast<int>(ControlSignal::stimulus8);
        if ((index < listStart) || (index > listEnd)) {
            index -= listStart;
            if (pulseList[index].empty())
                return false;
        }
        else
            return false;
    }

    bool readWaveform(qint16 *dest, ControlSignal idx, int begin, int end, int inc)
    {
        int index = static_cast<int>(idx);
        if(!isWavIndexValid(index))
            return false;

        for (int i = begin; i <= end; i += inc)
            if (i<waveList[index].size())
                *dest++ = waveList[index][i];
            else
                *dest++ = waveList[index][waveList[index].size()-1];
    }

    bool readPulse(int *dest, ControlSignal idx, int begin, int end, int inc)
    {
        int index = static_cast<int>(idx);
        if (!isPulseIndexValid(index))
            return false;

        index -= static_cast<int>(ControlSignal::camera1);
        for (int i = begin; i <= end; i += inc)
            if(i<pulseList[index].size())
                *dest++ = pulseList[index][i];
            else
                *dest++ = pulseList[index][pulseList[index].size()-1];
    }
};
#endif //WAVEFORM_H

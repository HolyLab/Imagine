/**** JSON Data Structure of control waveform for OCPI ****
{
    "version": "v1.0",
    "analog waveform":{
        "positioner1": [5, "positioner1_001", 2, "positioner1_002",,,]
    },
    "digital pulse":{
        "camera1": [5, "camera1_001",,,],   "_comment": "[repeat time1, wave name1, repeat time2, wave name2,,,]",
        "laser1": [5, "laser1_001",,,],
        "stimulus1": [5, "stimulus1_001",,,]
    }
    "metadata":{
        "frames": 20
        "bi-direction": false,
        "exposure": 0.01,
        "frames": 20,
        "sample num": 600000,
        "stacks": 9
    },
    "wave list":{ "_comment": "We use run length code [runs,value,runs,value,,,]",
        "camera1_001":[20, 1,,,],
        "camera1_002":[20, 1,,,],
        "laser1_001":[20, 1,,,],
        "laser1_002":[20, 1,,,],
        "positioner1_001":[20, 100,,,],
        "positioner1_002":[20, 100,,,]
    }
}
**********************************************************/
#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QString>
#include <QObject>
#include <QtCore>
#include <vector>

using namespace std;

#define MAX_NUM_WAVEFORM 100

int genControlFileJSON(void);

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
    int sampleRate = 10000;
    int totalSampleNum = 0;
    int nStacks = 0;
    int nFrames = 0;
    double exposureTime = 0;
    bool bidirection = false;

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

#endif //WAVEFORM_H

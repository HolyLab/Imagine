/**** JSON Data Structure of control waveform for OCPI ****
{
    // This comment syntex is not allowed in JSON format
    // If you really want a comment, make a dummy field such as
    "_comment": "This is a comment",

    "version": "v1.0",
    "metadata":{
        "bi-direction": false,  // bi-directional acquisition
        "exposure": 0.01,       // Camera exposure. Currently, this value is not used.
        "sample num": 600000,   // Total sample number
        "stacks": 9,
        "frames": 20            // frames per stack
        // Default sample rate is 10000 but this value can be changed at the OCPI Imagein GUI
    },
    // postioner1, positioner2 are connected to positioners in the microscope
    "analog waveform":{
        "positioner1": [5, "triangle_001", 2, "triangle_002"] // [repeat_time, wavefrom name,,,]
    },
    // camera1, camera2, laser1, laser2, stimulus1, stimulus2, stimulus3, stimulus4
    // are connected to camera exposures, laser shutters and stimulus ports of the microscope
    "digital pulse":{
        "camera1": [5, "shutter_long", 2, "shutter_short"],
        "laser1": [5, "laser_001", 2, "laser_002",],
        "stimulus1": [7, "valve1_001"]
    }
    "wave list":{ "_comment": "We use run length code [runs,value,runs,value,,,]",
        "shutter_long":[10, 0, 20, 1], // If this pulse is used to control DAQ digital output
        "shutter_short":[12, 0, 18, 1], // zero mean 0V, not zero means 5V out.
        "laser_001":[20, 1],
        "laser_002":[20, 1],
        "valve1_001":[10,0, 200, 1],
        "triangle_001":[20, 100,,,], // If this pulse is used to control positioner
        "triangle_002":[20, 100,,,]  // the maximum value is 400(OCPI-1) and 800(OCPI-2)
                                     // And, the value type is integer.
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
    int positionerSpeedCheck(int maxSpeed, ControlSignal name, int userSampleRate, int &dataSize);
    int laserSpeedCheck(double maxFreq, ControlSignal name, int &dataSize);

}; // ControlWaveform

#endif //WAVEFORM_H

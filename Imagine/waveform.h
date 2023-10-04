/**** JSON Data Structure of control waveform for OCPI ****
{
    // This("//") comment syntex is not allowed in JSON format
    // If you really want a comment, make a dummy field such as
    "_comment": "This is a comment",

    "version": "v1.1",

    "metadata":{
        "camera1": {
            "bidirectional": false,
            "exposure time in seconds": 0.03,
            "exposure trigger mode": "External Start",
            "frames per stack": 20,
            "stacks": 6
        },
        "generated from": "Imagine",
        "sample num": 600000,               // Total sample number
        "samples per second": 10000,
        "rig": "ocpi-2"
    },

    // Analog waveform : AO0 ~ AO3(output), AI0 ~ AI31(input)
    "analog waveform":{
        // secured controls : port assignment to the control name is fixed
        // Output : axial piezo(AO0), horizontal piezo(AO1)
        "axial piezo": {
            "daq channel": "AO0",
            "sequence": [5, "triangle_001", 2, "triangle_002"] // [repeat_time, wavefrom name,,,]
        },
        // Input : axial piezo monitor(AI0), horizontal piezo monitor(AI1)
        "axial piezo monitor": {
            "daq channel": "AI0"
        },

        // custom controls : can use custom name and assign any non-secured port
        // non-secured port : AO2, AO3(output), AI2 ~ AI31(input)
        // Output
        "mirror control": { // This name is an example
            "daq channel": "AO2",
            "sequence": [5, "mirror_001", 2, "mirror_002"]
        },
        // Input
        "mirror position": { // This name is an example
            "daq channel": "AI2"
        }
    },

    // Digital pulse : P0.0 ~ P0.23(output), P0.24 ~ P0.31(input)
    "digital pulse":{
        // secured controls : port assignment to the control name is fixed
        // Output : all lasers(P0.4), camera1(P0.5), camera2(P0.6), reserved(P0.7),
        //          488nm laser(P0.8), 561nm laser(P0.9), 405nm laser(P0.10),
        //          445nm laser(P0.11), 514nm laser(P0.12)
        "camera1": {
            "daq channel": "P0.5",
            "sequence": [5, "shutter_long", 2, "shutter_short"]
        },
        "all lasers": {
            "daq channel": "P0.4",
            "sequence": [5, "laser_001", 2, "laser_002",]
        },
        // Input : camera1 frame TTL(P0.24), camera2 frame TTL(P0.25)
        "camera1 frame monitor": {
            "daq channel": "P0.24"
        },
        // custom controls : can use custom name and assign any non-secured port
        // non-secured port : P0.0 ~ P0.3(output), P0.13 ~ P0.23(output), P0.26 ~ P0.31(input)
        // Output
       "stimulus1": { // This name is an example
            "daq channel": "P0.0",
            "sequence": [7, "valve1_001"]
        },
        // Input
        "heartbeat": { // This name is an example
            "daq channel": "P0.1",
        }
    }

    "wave list":{ "_comment": "We use run length code [runs,value,runs,value,,,]",
        "shutter_long":[10, 0, 20, 1], // If this pulse is used to control DAQ digital output
        "shutter_short":[12, 0, 18, 1], // zero mean 0V, non-zero means High TTL out.
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
#include <fftw3.h>
#include "ni_daq_g.hpp"

using namespace std;

#define INFINITE_SAMPLE         0
#define PIEZO_10V_UINT16_VALUE  32767.   // max value of uint16 (double constant)
#define FFT_SAMPLE_NUM          8192 // res. = sampleRate/FFT_SAMPLE_NUM will be a resolution in frequency domain.
                                     // So, res. should be small enough.

#define CF_V1P0                 "v1.0"
#define CF_V1P1                 "v1.1"
#define CF_VERSION              CF_V1P1

#define STR_Rig                 "rig"
#define STR_SampleRate          "samples per second"
#define STR_TotalSamples        "sample num"
#define STR_Exposure            "exposure time in seconds"
#define STR_ExpTrigMode         "exposure trigger mode"
//#define STR_ExpStart            "Exposure Start"
//#define STR_ExpControl          "Exposure Control"
//#define STR_FastExpControl      "Fast Exposure Control"
#define STR_Cam1Metadata        "camera1"
#define STR_Cam2Metadata        "camera2"
#define STR_Exposure1           "exposure time in seconds for camera1"
#define STR_Exposure2           "exposure time in seconds for camera2"
#define STR_BiDirV1P0           "bi-direction"
#define STR_BiDir               "bidirectional"
#define STR_Stacks              "stacks"
#define STR_Frames              "frames per stack"
#define STR_GeneratedFrom       "generated from"
#define STR_ImagineInterface    "ImagineInterface"
#define STR_Imagine             "Imagine"
#define STR_Metadata            "metadata"
#define STR_Analog              "analog waveform"
#define STR_Digital             "digital pulse"
#define STR_WaveList            "wave list"
#define STR_Version             "version"
#define STR_Seq                 "sequence"
#define STR_Channel             "daq channel"
#define STR_AOHEADER            "AO"
#define STR_AIHEADER            "AI"
#define STR_P0HEADER            "P0."

#define STR_piezo               "piezo"
#define STR_piezo_mon           "piezo monitor"
#define STR_laser               "laser"
#define STR_camera              "camera"
#define STR_axial_piezo         "axial piezo"
#define STR_hor_piezo           "horizontal piezo"
#define STR_axial_piezo_mon     "axial piezo monitor"
#define STR_hor_piezo_mon       "horizontal piezo monitor"
#define STR_galvo_mon           "galvo monitor"
#define STR_all_lasers          "all lasers"
#define STR_camera1             "camera1"
#define STR_camera2             "camera2"
#define STR_488nm_laser         "488nm laser"
#define STR_561nm_laser         "561nm laser"
#define STR_405nm_laser         "405nm laser"
#define STR_445nm_laser         "445nm laser"
#define STR_514nm_laser         "514nm laser"
#define STR_488nm_laser_str     "488nm laser shutter"
#define STR_camera1_mon         "camera1 frame monitor"
#define STR_camera2_mon         "camera2 frame monitor"
#define STR_camera_mon          "camera frame monitor"
#define STR_stimuli_mon         "stimuli"
#define STR_galvo               "galvo"
#define STR_galvo1              "galvo1"
#define STR_galvo2              "galvo2"
#define STR_galvo1_mon          "galvo1 monitor"
#define STR_galvo2_mon          "galvo2 monitor"

typedef long long SampleIdx;

enum CFErrorCode : unsigned int
{
    NO_CF_ERROR                 = 0,
    // load waveform data error
    ERR_INVALID_VERSION         = 1 << 0,
    ERR_RIG_MISMATCHED          = 1 << 1,
    ERR_INVALID_PORT_ASSIGNMENT = 1 << 2,
    ERR_SHORT_CONTROL_SEQ       = 1 << 3,
    ERR_WAVEFORM_MISSING        = 1 << 4,
    // waveform validity error
    ERR_CONTROL_SEQ_MISSING     = 1 << 5,
    ERR_SAMPLE_NUM_MISMATCHED   = 1 << 6,
    ERR_PIEZO_VALUE_INVALID     = 1 << 7,
    ERR_PIEZO_SPEED_FAST        = 1 << 8,
    ERR_PIEZO_INSTANT_CHANGE    = 1 << 9,
    ERR_CAMERA_PULSE_NUM_ERR    = 1 << 10,
    ERR_LASER_SPEED_FAST        = 1 << 11,
    ERR_LASER_INSTANT_CHANGE    = 1 << 12,
    ERR_GALVO_VALUE_INVALID     = 1 << 13,
    ERR_GALVO_FREQ_FAST         = 1 << 14,
    ERR_SHORT_WAVEFORM          = 1 << 15,
    // load ai di data error
    ERR_READ_AI                 = 1 << 16,
    ERR_READ_DI                 = 1 << 17,
    // waveform generation error
    ERR_TRAVELBACKTIME_SHORT    = 1 << 18,
    ERR_IDLETIME_SHORT          = 1 << 19,
    ERR_FILE_OPEN               = 1 << 20,
    // frequency analysis error
    ERR_FREQUENCY_ANALYSIS_ERR  = 1 << 21
}; // ai, di and command file error code

enum PiezoDataType
{
    PDT_RAW         = 0,
    PDT_VOLTAGE     = 1,
    PDT_Z_POSITION  = 2
};

class Waveform : public QObject {
    Q_OBJECT
protected:

public:
    QString errorMsg = "";
    int sampleRate = 10000;
    SampleIdx totalSampleNum = 0;

    Waveform() {};
    ~Waveform() {};

    bool isEmpty()
    {
        if (totalSampleNum) return false;
        else return true;
    };

    QString getErrorMsg(void) {
        return errorMsg;
    };
};

class ControlWaveform : public Waveform {
    Q_OBJECT

private:
    int numAOChannel, numAIChannel, numP0Channel, numP0InChannel, numChannel;
    double piezo10Vint16Value;
    QJsonObject alldata;

    QVector<QVector<int> *>     controlList;    // [ctlIdx][0]:repeat,[ctlIdx][1]:wave,[ctlIdx][2]:repeat,,,
    QVector<QVector<int> *>     waveList_Raw;   // positioner waveform (raw format)
                                                // analog waveform + digital waveform
    QVector<QVector<int> *>     waveList_Pos;   // analog waveform (piezo position format)
    QVector<QVector<float64> *> waveList_Vol;   // analog waveform (voltage format) 

    QVector<QVector<QString>> channelSignalList; // [ctrlIdx][0]:port ID, [ctrlIdx][1]:signal name

    QVector<QVector<QString>> ocpi1Secured = { // secured signal name for OCPI-1
        // Analog output (AO0 ~ AO1)                                // ctrlIdx
        { QString(STR_AOHEADER).append("0"), STR_axial_piezo },     // 0
        // Analog input (AI0 ~ AI15)
        { QString(STR_AIHEADER).append("0"), STR_axial_piezo_mon }, // 2
        { QString(STR_AIHEADER).append("1"), STR_stimuli_mon }, // 3
        // Digital output (P0.0 ~ P0.6)
        { QString(STR_P0HEADER).append("4"), STR_488nm_laser_str }, // 18
        { QString(STR_P0HEADER).append("5"), STR_camera1 },
        // Digital input (P0.7)                                     // 25
        { QString(STR_P0HEADER).append("7"), STR_camera1_mon }
    };

    QVector<QVector<QString>> ocpilskSecured = { // secured signal name for OCPI-lsk
        // Analog output (AO0 ~ AO3)                                // ctrlIdx
        { QString(STR_AOHEADER).append("0"), STR_axial_piezo },     // 0
        // Analog input (AI0 ~ AI31)
        { QString(STR_AIHEADER).append("0"), STR_axial_piezo_mon }, // 4
        // Digital output (P0.0 ~ P0.23)
        { QString(STR_P0HEADER).append("4"), STR_all_lasers },      // 40
        { QString(STR_P0HEADER).append("5"), STR_camera1 },
        // Digital input (P0.24 ~ P0.31)
        { QString(STR_P0HEADER).append("24"), STR_camera1_mon }     // 68
    };

    QVector<QVector<QString>> ocpi2Secured = { // secured signal name for OCPI-2
        // Analog output (AO0 ~ AO3)                                // ctrlIdx
        { QString(STR_AOHEADER).append("0"), STR_axial_piezo },     // 0
        { QString(STR_AOHEADER).append("1"), STR_hor_piezo },
        { QString(STR_AOHEADER).append("2"), STR_galvo1 },          // 2
        { QString(STR_AOHEADER).append("3"), STR_galvo2 },
        // Analog input (AI0 ~ AI31)
        { QString(STR_AIHEADER).append("0"), STR_axial_piezo_mon }, // 4
        { QString(STR_AIHEADER).append("1"), STR_hor_piezo_mon },
        { QString(STR_AIHEADER).append("2"), STR_galvo1_mon },
        { QString(STR_AIHEADER).append("3"), STR_galvo2_mon },
        // Digital output (P0.0 ~ P0.23)
        { QString(STR_P0HEADER).append("4"), STR_all_lasers },      // 40
        { QString(STR_P0HEADER).append("5"), STR_camera1 },
        { QString(STR_P0HEADER).append("6"), STR_camera2 },
        { QString(STR_P0HEADER).append("8"), STR_405nm_laser },     // 44
        { QString(STR_P0HEADER).append("9"), STR_445nm_laser },
        { QString(STR_P0HEADER).append("10"), STR_488nm_laser },
        { QString(STR_P0HEADER).append("11"), STR_514nm_laser },
        { QString(STR_P0HEADER).append("12"), STR_561nm_laser },
        // Digital input (P0.24 ~ P0.31)
        { QString(STR_P0HEADER).append("24"), STR_camera1_mon },     // 68
        { QString(STR_P0HEADER).append("25"), STR_camera2_mon }
    };

	QVector<QVector<QString>> realmSecured = { // secured signal name for realm
		// Analog output (AO0 ~ AO1)								// ctrlIdx
		{ QString(STR_AOHEADER).append("0"), STR_axial_piezo },     // 0
        { QString(STR_AOHEADER).append("1"), STR_galvo },           // 1
        // Analog input (AI0 ~ AI15)
		{ QString(STR_AIHEADER).append("0"), STR_axial_piezo_mon }, // 2
		{ QString(STR_AIHEADER).append("1"), STR_galvo_mon },       // 3
        { QString(STR_AIHEADER).append("2"), STR_stimuli_mon },     // 4
        // Digital output (P0.0 ~ P0.6)
		{ QString(STR_P0HEADER).append("4"), STR_488nm_laser_str }, // 18
		{ QString(STR_P0HEADER).append("5"), STR_camera1 },
		// Digital input (P0.7)                                     // 25
		{ QString(STR_P0HEADER).append("7"), STR_camera1_mon }
	};

    QVector<QVector<double>> waveList_FC;   // Frequency components
    QVector<QVector<double>> waveList_ratio;
    QVector<QVector<double>> waveList_AC_PWR;
    CFErrorCode lookUpWave(QString wn, QVector <QString> &wavName,
        QJsonObject wavelist, int &waveIdx, int dataType);
    int getWaveSampleNum(int waveIdx);
    template<class Typ> bool getWaveSampleValue(int waveIdx, SampleIdx sampleIdx,
            Typ &value, PiezoDataType dataType = PDT_RAW);
    CFErrorCode parsing(void);
    // Waveform validity check
    CFErrorCode freqAnalysisOfAnalogSignal(int ctrlIdx, bool periodic);
    CFErrorCode analogResonanceFreqCheck(int ctrlIdx, double &ratio, SampleIdx &strt, SampleIdx &stop);
    double resonanceFreq_ratio(QVector<double> fc, int dataSize, double *ac_power);
    CFErrorCode positionerSpeedCheck(int maxPosSpeed, int minPos, int maxPos, int ctrlIdx,
        SampleIdx &dataSize, SampleIdx &strt, SampleIdx &stop);
    CFErrorCode galvoSpeedCheck(double maxVolSpeed, int minVol, int maxVol, int ctrlIdx,
        SampleIdx &dataSize, SampleIdx &strt, SampleIdx &stop);
    CFErrorCode ControlWaveform::galvoFreqCheck(double maxFreq, int minVol, int maxVol, int ctrlIdx,
        SampleIdx& dataSize, SampleIdx& strt, SampleIdx& stop);
    CFErrorCode ControlWaveform::analogFreqCheck(double maxFreq, int minRaw, int maxRaw, int ctrlIdx,
        SampleIdx& dataSize, SampleIdx& strt, SampleIdx& stop);
    CFErrorCode analogSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx, SampleIdx &dataSize,
        SampleIdx &strt, SampleIdx &stop);
    CFErrorCode fullSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx,
        QVector <SampleIdx>&swstrt, QVector <SampleIdx>&swstop, SampleIdx &dataSize, SampleIdx &strt, SampleIdx &stop);
    CFErrorCode fastSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx, SampleIdx &dataSize);
    CFErrorCode laserSpeedCheck(double maxFreq, int ctrlIdx, SampleIdx &dataSize);
    CFErrorCode cameraPulseNumCheck(int nTotalFrames, int ctrlIdx, int &nPulses, SampleIdx &dataSize);

public:
    QString version;
    QString rig;
    QString generatedFrom = "NA";
    int nStacks1 = 0;
    int nStacks2 = 0;
    int nFrames1 = 0;
    int nFrames2 = 0;
    double exposureTime1 = 0;
    double exposureTime2 = 0;
    QString expTriggerModeStr1 = "External Start";
    QString expTriggerModeStr2 = "External Start";
    bool bidirection1 = false;
    bool bidirection2 = false;
    int minPiezoPos;
    int maxPiezoPos;
    int maxPiezoSpeed;
    int maxLaserFreq;
    int minGalvoVol = 0;
    int maxGalvoVol = 0;
    int maxGalvoSpeed = 0;
    int maxGalvoFreq = 0;
    double resonanceFreq;  // this value comes from imagine.js
    double bandwidth = 20.; // bandwidth around resonance frequency to check safety
    double threshold = 0.02; // threshold(%) for power around resonance frequency
    double min_pwr_th = 100; // minimum AC power threshold
    uInt32 laserTTLSig = 0;
    double laserIntensity[6] = {0, };
    int perStackSamples;
    CFErrorCode validity = NO_CF_ERROR;
    int fftSampleLength;
    int freqCompLength;
    double freqResolution;

    // for default control
    bool enableCam1, enableCam2;
    double cycleTime;
    double piezoTravelBackTime;
//    bool piezoTravelBackTimeAuto;
    double idleTimeBwtnStacks;
    double piezoStartPosUm;
    double piezoStopPosUm;
    int minAoRaw, maxAoRaw;
    double maxAoFreqAmplitude;
    bool applyStim;
    vector<pair<int, int> > *stimuli;

    ControlWaveform(QString rig);
    ~ControlWaveform();

    void initControlWaveform(QString rig);
    CFErrorCode loadJsonDocument(QJsonDocument &loadDoc);
    // Read block
    template<class Typ> bool readControlWaveform(QVector<Typ> &dest, int ctrlIdx,
        SampleIdx begin, SampleIdx end, int downSampleRate, PiezoDataType dataType = PDT_RAW);
    bool readControlFrequency(QVector<double> &dest, int ctrlIdx, SampleIdx begin,
        SampleIdx end, int downSampleRate);
    // Read single value
    template<class Typ> bool getCtrlSampleValue(int ctrlIdx, SampleIdx idx, Typ &value,
            PiezoDataType dataType = PDT_RAW);
    bool isEmpty(int ctrlIdx);
    bool isEmpty(QString signalName);
    SampleIdx getCtrlSampleNum(int ctrlIdx);
    QString getSignalName(int ctrlIdx);
    QString getChannelName(int ctrlIdx);
    QString getSignalName(QString channelName);
    int getChannelIdxFromSig(QString signalName);
    int getChannelIdxFromCh(QString channelName);
    int getAIBegin(void);
    int getP0Begin(void);
    int getP0InBegin(void);
    int getNumAOChannel(void);
    int getNumAIChannel(void);
    int getNumP0Channel(void);
    int getNumP0InChannel(void);
    int getNumChannel(void);
    bool isPiezoWaveEmpty(void);
    bool isCameraPulseEmpty(void);
    bool isLaserPulseEmpty(void);
    double raw2Zpos(int raw);
    int zpos2Raw(double pos);
    float64 raw2Voltage(int raw);
    int voltage2Raw(float64 vol);
    // Waveform validity check
    CFErrorCode waveformValidityCheck();
    // Generate control waveform and command file
    CFErrorCode genDefaultControl(QString filename = "");
    int genSinusoidal(bool bidir);
    // Laser default output
    void setLaserIntensityValue(int line, double data);
    bool getLaserDefaultTTL(int line);
    int getMinAoRaw();
    int getMaxAoRaw();
    double getMaxAoFreqAmplitude();
    CFErrorCode freqAnalysis(void);
}; // ControlWaveform


#define MAX_AI_DI_SAMPLE_NUM    100000000
class AiWaveform : public Waveform {
    Q_OBJECT

private:
    QFile file;
    QVector<QVector<int>> aiData;
    int numAiCurveData;
    int maxRaw = -(PIEZO_10V_UINT16_VALUE + 1);
    int minRaw = PIEZO_10V_UINT16_VALUE;
    QDataStream stream;
    bool isReadFromFile = false;
    double convertRawToVoltage(short us);
    int removeSomeWrapAround(short us);

public:
    QVector<int> dest;

    AiWaveform(QString filename, int num);
    ~AiWaveform();

    // Read file
    bool readWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate);
    bool readFile(QString filename);
    // Read stream to waveforms
    bool readStreamToWaveforms();
    // Read single value
    bool getSampleValue(int ctrlIdx, SampleIdx idx, int &value);
    int getMaxAiRaw();
    int getMinAiRaw();
    // Close open file
    bool fileClose();
    // Read block
};


class DiWaveform : public Waveform {
    Q_OBJECT

private:
    QFile file;
    QVector<QVector<int>> diData;
    QVector<QVector<SampleIdx>> diCRLData;
    int numDiCurveData;
    QVector<int> chNumList;
    QDataStream stream;
    bool isCRLEncoded = false;

public:
    DiWaveform(QString filename, QVector<int> diChNumList);
    ~DiWaveform();
    bool readStreamToWaveforms();
    bool readStreamToCRLWaveforms();
    bool readFile(QString filename);

    // Read block
    bool readWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate);
    bool readCRLWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate);
    // Read single value
    bool getSampleValue(int ctrlIdx, SampleIdx idx, int &value);
    // Close open file
    bool fileClose();
};


/******* Template functions *********/
// To use these template functions from the outside of this class
// These funtion definitions should be located in this header file and the caller shoud include this header file.

int findValue(QVector<int> *wav, SampleIdx begin, SampleIdx end, int sampleIdx);
int findValuePos(QVector<int> *wav, QVector<int> *wav_p, SampleIdx begin, SampleIdx end, int sampleIdx);
float64 findValueVol(QVector<int> *wav, QVector<float64> *wav_v, SampleIdx begin, SampleIdx end, int sampleIdx);

// Read control signal data from index 'begin' to 'end' with downsample rate 'downSampleRate' to 'dest' vector
template<class Typ>
bool ControlWaveform::readControlWaveform(QVector<Typ> &dest, int ctrlIdx,
    SampleIdx begin, SampleIdx end, int downSampleRate, PiezoDataType dataType)
{
    if (controlList.isEmpty())
        return false;

    int controlIdx = 0, repeat, waveIdx, repeatLeft;
    SampleIdx sampleIdx = begin, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;

    while (1) {
        for (; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
            repeat = controlList[ctrlIdx]->at(controlIdx);
            repeatLeft = repeat;
            waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
            for (int j = 0; j < repeat; j++) {
                prevStartSampleIdx = startSampleIdx;
                startSampleIdx += getWaveSampleNum(waveIdx);
                if (sampleIdx < startSampleIdx) {
                    for (; sampleIdx < startSampleIdx; sampleIdx += downSampleRate) {
                        sampleIdxInWave = sampleIdx - prevStartSampleIdx;
                        Typ value;
                        bool isValid = getWaveSampleValue(waveIdx, sampleIdxInWave, value, dataType);
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

// Read wave signal data at index 'sampleIdx' to variable 'value'
template<class Typ>
bool ControlWaveform::getWaveSampleValue(int waveIdx, SampleIdx sampleIdx, Typ &value, PiezoDataType dataType)
{
    if (waveList_Raw.isEmpty())
        return false;

    int sum = 0;

    if (sampleIdx >= waveList_Raw[waveIdx]->last()) {
        return false;
    }
    SampleIdx begin = 0;
    SampleIdx end = waveList_Raw[waveIdx]->size() / 2;
    if (dataType == PDT_RAW)
        value = findValue(waveList_Raw[waveIdx], begin, end, sampleIdx);
    else if (dataType == PDT_VOLTAGE)
        if (waveList_Vol.isEmpty())
            value = findValue(waveList_Raw[waveIdx], begin, end, sampleIdx);
        else
            value = findValueVol(waveList_Raw[waveIdx], waveList_Vol[waveIdx], begin, end, sampleIdx);
    else if (dataType == PDT_Z_POSITION)
        if (waveList_Pos.isEmpty())
            value = findValue(waveList_Raw[waveIdx], begin, end, sampleIdx);
        else
            value = findValuePos(waveList_Raw[waveIdx], waveList_Pos[waveIdx], begin, end, sampleIdx);
    return true;
}

// Read control signal data at index 'idx' to variable 'value'
template<class Typ>
bool ControlWaveform::getCtrlSampleValue(int ctrlIdx, SampleIdx idx, Typ &value, PiezoDataType dataType)
{
    if (controlList.isEmpty())
        return false;
    int controlIdx, repeat, waveIdx;
    SampleIdx sampleIdx = 0, prevSampleIdx, sampleIdxInWave;
    bool found = false;
    bool isValid;
    // find beginning point
    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2)
    {
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
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
    isValid = getWaveSampleValue(waveIdx, sampleIdxInWave, value, dataType);
    if (!isValid)
        goto idx_error;
done:
    return true;
}

#endif //WAVEFORM_H

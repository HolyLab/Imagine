/*-------------------------------------------------------------------------
** Copyright (C) 2017-2022 Dae Woo Kim
**    All rights reserved.
** Author: Dae Woo Kim
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact kdw503@wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/
#include "waveform.h"
#include "misc.hpp"
#include <iostream>
#include <QDebug>
#include <QScriptEngine>

using namespace std;

extern QScriptEngine* se;

constexpr CFErrorCode operator|(CFErrorCode X, CFErrorCode Y) {
    return static_cast<CFErrorCode>(static_cast<unsigned int>(X) | static_cast<unsigned int>(Y));
}

constexpr CFErrorCode operator&(CFErrorCode X, CFErrorCode Y) {
    return static_cast<CFErrorCode>(static_cast<unsigned int>(X) & static_cast<unsigned int>(Y));
}

CFErrorCode& operator|=(CFErrorCode& X, CFErrorCode Y) {
    X = X | Y; return X;
}

CFErrorCode& operator&=(CFErrorCode& X, CFErrorCode Y) {
    X = X & Y; return X;
}

/***** ControlWaveform class : Read command file ****************************/
ControlWaveform::ControlWaveform(QString rig)
{
    this->rig = rig;
    initControlWaveform(rig);
}

void ControlWaveform::initControlWaveform(QString rig)
{
    QScriptEngine* seTmp = new QScriptEngine();
    seTmp->globalObject().setProperty("rig", rig); // to read parameters of .json's rig
    QFile file("imagine.js");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&file);
    QString jscode = in.readAll();
    QScriptProgram sp(jscode);
    QScriptValue jsobj = seTmp->evaluate(sp);
    maxPiezoPos = seTmp->globalObject().property("maxposition").toNumber();
    maxPiezoSpeed = seTmp->globalObject().property("maxspeed").toNumber();
    maxLaserFreq = seTmp->globalObject().property("maxlaserfreq").toNumber();
    if ((rig == "ocpi-2") || (rig == "dummy")) {
        minGalvoVol = seTmp->globalObject().property("mingalvovoltage").toNumber();
        maxGalvoVol = seTmp->globalObject().property("maxgalvovoltage").toNumber();
        maxGalvoSpeed = seTmp->globalObject().property("maxgalvospeed").toNumber();
    }
    file.close();
    delete seTmp;

    // Count all control channels
    if (rig != "dummy") {
        if (rig == "ocpi-1") {
            numAOChannel = 2;
            numAIChannel = 16;
            numP0Channel = 8;
            numP0InChannel = 1;
        }
        else { // ocpi-2, ocpi-lsk
            numAOChannel = 4;
            numAIChannel = 32;
            numP0Channel = 32;
            numP0InChannel = 8;
        }
    }
    else {
        numAOChannel = 4;
        numAIChannel = 32;
        numP0Channel = 32;
        numP0InChannel = 8;
    }

    // Setting secured channel name
    channelSignalList.clear();
    QVector<QVector<QString>> *secured;
    if (rig == "ocpi-1") {
        secured = &ocpi1Secured;
        piezo10Vint16Value = PIEZO_10V_UINT16_VALUE;
    }
    else if (rig == "ocpi-lsk") {
        secured = &ocpilskSecured;
        piezo10Vint16Value = PIEZO_10V_UINT16_VALUE;
    }
    else {// including "ocpi-2" and "dummy" ocpi
        secured = &ocpi2Secured;
        piezo10Vint16Value = PIEZO_10V_UINT16_VALUE;
    }
    numChannel = numAOChannel + numAIChannel + numP0Channel;
    for (int i = 0; i < numChannel; i++) {
        QVector<QString> portSignal = { "", "" };
        if (i < getAIBegin()) {
            portSignal[0] = QString(STR_AOHEADER).append(QString("%1").arg(i));
        }
        else if (i < getP0Begin())
            portSignal[0] = QString(STR_AIHEADER).append(QString("%1").arg(i - getAIBegin()));
        else
            portSignal[0] = QString(STR_P0HEADER).append(QString("%1").arg(i - getP0Begin()));
        channelSignalList.push_back(portSignal);
        for (int j = 0; j < secured->size(); j++) {
            if ((*secured)[j][0] == channelSignalList[i][0])
                channelSignalList[i][1] = (*secured)[j][1];
        }
    }
}

ControlWaveform::~ControlWaveform()
{
    for (int i = 0; i < waveList_Raw.size(); i++)
    {
        if (waveList_Raw[i] != nullptr)
            delete waveList_Raw[i];
    }
    for (int i = 0; i < waveList_Pos.size(); i++)
    {
        if (waveList_Pos[i] != nullptr)
            delete waveList_Pos[i];
    }
    for (int i = 0; i < waveList_Vol.size(); i++)
    {
        if (waveList_Vol[i] != nullptr)
            delete waveList_Vol[i];
    }
    for (int i = 0; i < controlList.size(); i++)
    {
        if (controlList[i] != nullptr)
            delete controlList[i];
    }
}

double ControlWaveform::raw2Zpos(int raw)
{
    double vol = static_cast<double>(raw) / piezo10Vint16Value* maxPiezoPos;
//    return static_cast<int>(vol + 0.5);
    return vol;
}

int ControlWaveform::zpos2Raw(double pos)
{
    int raw = pos / static_cast<double>(maxPiezoPos) * piezo10Vint16Value;
    return raw;
}

float64 ControlWaveform::raw2Voltage(int raw) // unit : voltage
{
    double vol = static_cast<double>(raw) / piezo10Vint16Value * 10.; // *10: 0-10vol range for piezo
    return static_cast<float64>(vol);
}


int ControlWaveform::voltage2Raw(float64 vol)
{
    int raw = vol * piezo10Vint16Value / 10.; // *10: 0-10vol range for piezo
    return raw;
}

CFErrorCode ControlWaveform::lookUpWave(QString wn, QVector <QString> &wavName,
        QJsonObject wavelist, int &waveIdx, int dataType)
{
    int i;

    for (i = 0; i < wavName.size(); i++)
    {
        if (wn.compare(wavName[i]) == 0) {
            waveIdx = i;
            return NO_CF_ERROR;
        }
    }

    // If there is no matched string, find this wave from wavelist json object
    // and add this new array to waveList_Raw vector
    QJsonArray jsonWave = wavelist[wn].toArray();
    QVector <int> *wav = new QVector<int>;
    QVector <int> *wav_p = new QVector<int>;
    QVector <float64> *wav_v = new QVector<float64>;
    int sum = 0;
    if (jsonWave.size() == 0) {
        errorMsg.append(QString("Can not find '%1' waveform\n").arg(wn));
        return ERR_WAVEFORM_MISSING;
    }
    for (int j = 0; j < jsonWave.size(); j+=2) { 
        wav->push_back(sum);    // change run length to sample index number and save
        int value = jsonWave[j + 1].toInt();
        wav->push_back(value); // This is a raw data format
        if (dataType == 1) {// For the piezo data type, we will keep another waveform data as a voltage
                            // format(max 10.0V) and position format(max maxPiezoPos) for later use.
                            // This is just for display
            wav_p->push_back(raw2Zpos(value));
            wav_v->push_back(raw2Voltage(value));
        }
        sum += jsonWave[j].toInt();
    }
    wav->push_back(sum); // save total sample number to the last element
    waveList_Raw.push_back(wav); // add this wav to waveList_Raw
    if (dataType != 0) { // analog
        waveList_Pos.push_back(wav_p);
        waveList_Vol.push_back(wav_v);
    }
    wavName.push_back(wn); // also add waveform name(wn) to wavName
    waveIdx = i;
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::loadJsonDocument(QJsonDocument &loadDoc)
{
    alldata = loadDoc.object();
    return parsing();
}

CFErrorCode ControlWaveform::parsing(void)
{
    QJsonObject metadata;
    QJsonObject cam1metadata;
    QJsonObject cam2metadata;
    QJsonObject analog;
    QJsonObject digital;
    QJsonObject wavelist;

    QJsonArray piezo1, piezo2;
    QJsonArray camera1, camera2;
    QJsonArray laser1, laser2, laser3, laser4, laser5;
    QJsonArray stimulus1, stimulus2, stimulus3, stimulus4;
    QJsonArray stimulus5, stimulus6, stimulus7, stimulus8;

    errorMsg.clear();
    // Parsing
    // 1. Metadata
    version = alldata[STR_Version].toString();
    if ((version != CF_VERSION)&& (version != CF_V1P0))
        return ERR_INVALID_VERSION;

    metadata = alldata[STR_Metadata].toObject();
    QString rig = metadata[STR_Rig].toString();
    if (this->rig == "dummy") { // dummy ocpi can read .json for any rig
        this->rig = rig;
        initControlWaveform(rig);
    }
    else if (rig == this->rig) {
        initControlWaveform(this->rig);
    }
    else
        return ERR_RIG_MISMATCHED;
    sampleRate = metadata[STR_SampleRate].toInt();
    totalSampleNum = metadata[STR_TotalSamples].toInt();
    if (version == CF_VERSION) {
        generatedFrom = metadata[STR_GeneratedFrom].toString();
        if (metadata.contains(STR_Cam1Metadata)) {
            enableCam1 = true;
            cam1metadata = metadata[STR_Cam1Metadata].toObject();
            exposureTime1 = cam1metadata[STR_Exposure].toDouble();
            nStacks1 = cam1metadata[STR_Stacks].toInt();
            nFrames1 = cam1metadata[STR_Frames].toInt();
            expTriggerModeStr1 = cam1metadata[STR_ExpTrigMode].toString();
            bidirection1 = cam1metadata[STR_BiDir].toBool();
            if ((expTriggerModeStr1 == "External Start") && !exposureTime1)
                errorMsg.append(QString("Exposure for camera1 should not be zero in the 'External Start' trigger mode\n"));
        }
        else
            enableCam1 = false;
        if ((rig == "ocpi-2") || (rig == "dummy")) {
            if (metadata.contains(STR_Cam2Metadata)) {
                enableCam2 = true;
                cam2metadata = metadata[STR_Cam2Metadata].toObject();
                exposureTime2 = cam2metadata[STR_Exposure].toDouble();
                nStacks2 = cam2metadata[STR_Stacks].toInt();
                nFrames2 = cam2metadata[STR_Frames].toInt();
                expTriggerModeStr2 = cam2metadata[STR_ExpTrigMode].toString();
                bidirection2 = cam2metadata[STR_BiDir].toBool();
                if (metadata.contains(STR_Cam2Metadata))
                    enableCam2 = true;
                else
                    enableCam2 = false;
                if ((expTriggerModeStr2 == "External Start") && !exposureTime2)
                    errorMsg.append(QString("Exposure for camera2 should not be zero in the 'External Start' trigger mode\n"));
            }
            else
                enableCam2 = false;
        }
    }
    else if (version == CF_V1P0) {
        generatedFrom = "NA";
        exposureTime1 = metadata[STR_Exposure1].toDouble();
        nStacks1 = metadata[STR_Stacks].toInt();
        nFrames1 = metadata[STR_Frames].toInt();
        expTriggerModeStr1 = "External Start";
        bidirection1 = metadata[STR_BiDirV1P0].toBool();
        if ((rig == "ocpi-2") || (rig == "dummy")) {
            exposureTime2 = metadata[STR_Exposure2].toDouble();
            nStacks2 = nStacks1;
            nFrames2 = nFrames1;
            expTriggerModeStr2 = "External Start";
            bidirection2 = bidirection1;
        }
    }
    analog = alldata[STR_Analog].toObject();
    digital = alldata[STR_Digital].toObject();
    wavelist = alldata[STR_WaveList].toObject();
    QStringList analogKeys = analog.keys();
    QStringList digitalKeys = digital.keys();
    QJsonObject obj;
    CFErrorCode err = NO_CF_ERROR;
    QJsonArray control;

    // 2. Read signal names and position them accoding to their channel names
    // - Analog signals (Output/Input)
    for (int i = 0; i < analogKeys.size(); i++) {
        obj = analog[analogKeys[i]].toObject();
        QString port = obj[STR_Channel].toString();
        // If the channel's category is analog output
        if (port.left(2) == STR_AOHEADER) {
            int j = port.mid(2).toInt();
            QString reservedName = channelSignalList[j][1]; // if not empty, secured channel
            // If the channel is secured but sinal name is not matched with the secured name
            if ((reservedName != "")&&(reservedName != analogKeys[i])) {
                errorMsg.append(QString("'%1' channel is already reserved as a '%2' signal\n")
                    .arg(channelSignalList[j][0])
                    .arg(channelSignalList[j][1]));
                err |= ERR_INVALID_PORT_ASSIGNMENT;
            }
            else {
                channelSignalList[j][1] = analogKeys[i];
                control = (analog[channelSignalList[j][1]].toObject())[STR_Seq].toArray();
                // If the signal does not have sequence field
                if (control.isEmpty()) {
                    err |= ERR_WAVEFORM_MISSING;
                    errorMsg.append(QString("Signal '%1' has no sequence field\n")
                        .arg(channelSignalList[j][1]));
                }
            }
        }
        // else if the channel's category is analog input
        else if (port.left(2) == STR_AIHEADER) {
            int j = port.mid(2).toInt();
            QString reservedName = channelSignalList[j + getAIBegin()][1];
            if (reservedName != "") {
                if (reservedName != analogKeys[i]) {
                    errorMsg.append(QString("'%1' channel is already reserved as a '%2' signal\n")
                        .arg(channelSignalList[j + getAIBegin()][0])
                        .arg(channelSignalList[j + getAIBegin()][1]));
                    err |= ERR_INVALID_PORT_ASSIGNMENT;
                }
            }
            else {
                channelSignalList[j + getAIBegin()][1] = analogKeys[i];
                control = (analog[channelSignalList[j + getAIBegin()][1]].toObject())[STR_Seq].toArray();
                // If the signal does have sequence field (to remind this is input channel)
                if (!control.isEmpty()) {
                    err |= NO_CF_ERROR; // This is not an error. We can ignore the sequence field
                    errorMsg.append(QString("Signal '%1' is input but has sequence field\n")
                        .arg(channelSignalList[j + getAIBegin()][1]));
                }
            }
        }
        else {
            errorMsg.append(QString("'%1' channel can not be used as an 'analog waveform'\n")
                .arg(port));
            err |= ERR_INVALID_PORT_ASSIGNMENT;
        }
    }
    // If a secured analog signal is not specified in command file,
    // we will not follow that signal(erase the name from channelSignalList)
    bool isAiSignal = false;
    for (int i = 0; i < getP0Begin(); i++)
    {
        bool found = false;
        for (int j = 0; j < analogKeys.size(); j++) {
            if (channelSignalList[i][1] == analogKeys[j]) {
                found = true;
                if(i >= getAIBegin())
                    isAiSignal = true;
            }
        }
        if (!found)
            channelSignalList[i][1] = "";
    }
    if (!isAiSignal) {
        err |= NO_CF_ERROR; // This is not an error. We just want to remind user.
        errorMsg.append(QString("There is no description of any analog input signal\n"));
    }
    // - Digital signals
    for (int i = 0; i < digitalKeys.size(); i++) {
        obj = digital[digitalKeys[i]].toObject();
        QString port = obj[STR_Channel].toString();
        if (port.left(3) == STR_P0HEADER) {
            int j = port.mid(3).toInt();
            if (j > getNumP0Channel()) {
                errorMsg.append(QString("'%1' channel number is out of range\n")
                    .arg(digitalKeys[i]));
                err |= ERR_INVALID_PORT_ASSIGNMENT;
            }
            else {
                QString reservedName = channelSignalList[j + getP0Begin()][1];
                if ((reservedName != "") && (reservedName != digitalKeys[i])) {
                    errorMsg.append(QString("'%1' channel is already reserved as a '%2' signal\n")
                        .arg(channelSignalList[j + getP0Begin()][0])
                        .arg(channelSignalList[j + getP0Begin()][1]));
                    err |= ERR_INVALID_PORT_ASSIGNMENT;
                }
                else {
                    channelSignalList[j + getP0Begin()][1] = digitalKeys[i];
                    control = (digital[channelSignalList[j + getP0Begin()][1]].toObject())[STR_Seq].toArray();
                    if ((j + getP0Begin()<getP0InBegin())&&control.isEmpty()) {
                        err |= ERR_WAVEFORM_MISSING;
                        errorMsg.append(QString("Signal '%1' has no sequence field\n")
                            .arg(channelSignalList[j + getP0Begin()][1]));
                    }
                }
            }
        }
        else {
            errorMsg.append(QString("'%1' channel can not be used as a 'digital pulse'\n")
                .arg(port));
            err |= ERR_INVALID_PORT_ASSIGNMENT;
        }
    }
    // If a secured digital signal is not specified in command file,
    // we will not follow that signal(erase the name from channelSignalList)
    for (int i = getP0Begin(); i < getNumChannel(); i++)
    {
        bool found = false;
        for (int j = 0; j < digitalKeys.size(); j++) {
            if (channelSignalList[i][1] == digitalKeys[j])
                found = true;
        }
        if (!found)
            channelSignalList[i][1] = "";
    }

    // 3. Read sequence field of each sinals and construct waveform list
    // The order of this should be match with ControlSignal
    QVector<QString> wavName;
    for (int i = 0; i < channelSignalList.size(); i++) {
//        qDebug() << "[" + conName[i] + "]\n";
        QVector<int> *cwf = new QVector<int>;
        int dataType;
        if (i < getP0Begin()) {
            control = (analog[channelSignalList[i][1]].toObject())[STR_Seq].toArray();
            if (channelSignalList[i][0] == QString(STR_AOHEADER).append("0")) {// If the signal is for piezo
                dataType = 1; // piezo raw data
            }
            else
                dataType = 2; // other analog data which might need some conversions
        }
        else {
            control = (digital[channelSignalList[i][1]].toObject())[STR_Seq].toArray();
            dataType = 0; // digital data which does not need an additional conversion
        }
        if (!control.isEmpty()) {
            for (int j = 0; j < control.size(); j += 2) {
                int waveIdx = 0;
                int repeat = control[j].toInt();
                QString wn = control[j + 1].toString();
                // Look for a waveform(wn) in the existing waveform list(wavName).
                // If not, read the waveform from the 'wave list' json object(wavelist) and register
                // wavform data to waveList_Raw object and the waveform name(wn) to wavName
                err |= lookUpWave(wn, wavName, wavelist, waveIdx, dataType);
                cwf->push_back(repeat);
                cwf->push_back(waveIdx);
//                qDebug() << wn + "(" << waveIdx << ") " << repeat << "\n";
            }
        }
        controlList.push_back(cwf); // empty signal is also pushed back
    }
    //caluclate min max piezo value
    piezoStartPosUm = 1000000;
    piezoStopPosUm = -1000000;
    minAnalogRaw = PIEZO_10V_UINT16_VALUE;
    maxAnalogRaw = - (PIEZO_10V_UINT16_VALUE+1);
    for (int i = 0; i < getAIBegin(); i++) {
        for (int j = 0; j < controlList[i]->size(); j += 2) {
            int waveIdx = controlList[i]->at(j + 1);
            if (getSignalName(i) == STR_axial_piezo) {
                for (int sampleIdx = 0; sampleIdx < waveList_Pos[waveIdx]->size(); sampleIdx++) {
                    int piezoPos = waveList_Pos[waveIdx]->at(sampleIdx);
                    if (piezoStartPosUm > piezoPos) piezoStartPosUm = piezoPos;
                    if (piezoStopPosUm < piezoPos) piezoStopPosUm = piezoPos;
                }
            }
            for (int sampleIdx = 0; sampleIdx < waveList_Raw[waveIdx]->size(); sampleIdx+=2) {
                int analogRaw = waveList_Raw[waveIdx]->at(sampleIdx);
                if (minAnalogRaw > analogRaw) minAnalogRaw = analogRaw;
                if (maxAnalogRaw < analogRaw) maxAnalogRaw = analogRaw;
            }
        }
    }

    return err;
}

bool ControlWaveform::isEmpty(int ctrlIdx)
{
    if (controlList.isEmpty())
        return true;
    return controlList[ctrlIdx]->isEmpty();
}

bool ControlWaveform::isEmpty(QString signalName)
{
    if (controlList.isEmpty())
        return true;
    return controlList[getChannelIdxFromSig(signalName)]->isEmpty();
}

int ControlWaveform::getWaveSampleNum(int waveIdx)
{
    if (waveList_Raw.isEmpty())
        return 0;
    int  sampleNum = waveList_Raw[waveIdx]->last();
    return sampleNum;
}

int ControlWaveform::getCtrlSampleNum(int ctrlIdx)
{
    if (controlList.isEmpty())
        return 0;
    int sum = 0;
    int controlIdx, repeat, waveIdx;

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2)
    {
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
        sum += repeat*waveList_Raw[waveIdx]->last();
    }
    return sum;
}

QString ControlWaveform::getSignalName(int ctrlIdx)
{
    return channelSignalList[ctrlIdx][1];
}

QString ControlWaveform::getChannelName(int ctrlIdx)
{
    return channelSignalList[ctrlIdx][0];
}

QString ControlWaveform::getSignalName(QString channelName)
{
    return channelSignalList[getChannelIdxFromCh(channelName)][1];
}

int ControlWaveform::getChannelIdxFromSig(QString signalName)
{
    for (int i = 0; i < channelSignalList.size(); i++)
        if (channelSignalList[i][1] == signalName)
            return i;
}

int ControlWaveform::getChannelIdxFromCh(QString channelName)
{
    for (int i = 0; i < channelSignalList.size(); i++)
        if (channelSignalList[i][0] == channelName)
            return i;
}

int ControlWaveform::getAIBegin(void)
{
    return numAOChannel;
}

int ControlWaveform::getP0Begin(void)
{
    return numAOChannel + numAIChannel;
}

int ControlWaveform::getP0InBegin(void)
{
    return numChannel - numP0InChannel;
}

int ControlWaveform::getNumAOChannel(void)
{
    return numAOChannel;
}

int ControlWaveform::getNumAIChannel(void)
{
    return numAIChannel;
}

int ControlWaveform::getNumP0Channel(void)
{
    return numP0Channel;
}

int ControlWaveform::getNumP0InChannel(void)
{
    return numP0InChannel;
}

int ControlWaveform::getNumChannel(void)
{
    return numChannel;
}

void ControlWaveform::setLaserIntensityValue(int line, double data)
{
    laserIntensity[line] = data;
}

bool ControlWaveform::getLaserDefaultTTL(int line)
{
    return (laserTTLSig&(0x01<<(line+7)));
}

// binary search recursive function
int findValue(QVector<int> *wav, SampleIdx begin, SampleIdx end, int sampleIdx)
{
    SampleIdx mid =  (begin + end) / 2;
    SampleIdx midIdx = mid*2;
    SampleIdx x0, x1;

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

int findValuePos(QVector<int> *wav, QVector<int> *wav_p, SampleIdx begin, SampleIdx end, int sampleIdx)
{
    SampleIdx mid = (begin + end) / 2;
    SampleIdx midIdx = mid * 2;
    SampleIdx x0, x1;

    if (sampleIdx >= wav->at(midIdx)) {
        if (sampleIdx < wav->at(midIdx + 2)) {
            return wav_p->at(midIdx / 2);
        }
        else {
            x0 = mid;
            x1 = end;
            return findValuePos(wav, wav_p, x0, x1, sampleIdx);
        }
    }
    else {
        if (sampleIdx >= wav->at(midIdx - 2)) {
            return wav_p->at(midIdx / 2 - 1);
        }
        else {
            x0 = begin;
            x1 = mid;
            return findValuePos(wav, wav_p, x0, x1, sampleIdx);
        }
    }
}

float64 findValueVol(QVector<int> *wav, QVector<float64> *wav_v, SampleIdx begin, SampleIdx end, int sampleIdx)
{
    SampleIdx mid = (begin + end) / 2;
    SampleIdx midIdx = mid * 2;
    SampleIdx x0, x1;

    if (sampleIdx >= wav->at(midIdx)) {
        if (sampleIdx < wav->at(midIdx + 2)) {
            return wav_v->at(midIdx/2);
        }
        else {
            x0 = mid;
            x1 = end;
            return findValueVol(wav, wav_v, x0, x1, sampleIdx);
        }
    }
    else {
        if (sampleIdx >= wav->at(midIdx - 2)) {
            return wav_v->at(midIdx/2 - 1);
        }
        else {
            x0 = begin;
            x1 = mid;
            return findValueVol(wav, wav_v, x0, x1, sampleIdx);
        }
    }
}

bool ControlWaveform::isPiezoWaveEmpty(void)
{
    bool empty = true;
    if((rig=="ocpi-1")|| (rig == "ocpi-lsk")) {
        empty &= isEmpty(STR_axial_piezo);
    }
    else {
        empty &= isEmpty(STR_axial_piezo);
        empty &= isEmpty(STR_hor_piezo_mon);
    }
    return empty;
}

bool ControlWaveform::isCameraPulseEmpty(void)
{
    bool empty = true;
    if ((rig == "ocpi-1") || (rig == "ocpi-lsk")) {
        empty &= isEmpty(STR_camera1);
    }
    else {
        empty &= isEmpty(STR_camera1);
        empty &= isEmpty(STR_camera2);
    }
    return empty;
}

bool ControlWaveform::isLaserPulseEmpty(void)
{
    bool empty = true;
    if (rig == "ocpi-1") {
        empty &= isEmpty(STR_488nm_laser_str);
    }
    else if (rig == "ocpi-lsk") {
        empty &= isEmpty(STR_all_lasers);
    }
    else {
        empty &= isEmpty(STR_488nm_laser);
        empty &= isEmpty(STR_561nm_laser);
        empty &= isEmpty(STR_405nm_laser);
        empty &= isEmpty(STR_445nm_laser);
        empty &= isEmpty(STR_514nm_laser);
        //empty |= isEmpty(STR_all_lasers); // should have this signal
        empty &= isEmpty(STR_all_lasers); // can be empty this signal

    }
    return empty;
}

#define  SPEED_CHECK_INTERVAL 100
CFErrorCode ControlWaveform::positionerSpeedCheck(int maxPosSpeed, int minPos, int maxPos, int ctrlIdx, int &dataSize)
{
    int minRaw = zpos2Raw(minPos);
    int maxRaw = zpos2Raw(maxPos);
    int maxRawSpeed = zpos2Raw(maxPosSpeed); // change position speed to raw data speed
    CFErrorCode retVal = analogSpeedCheck(maxRawSpeed, minRaw, maxRaw, ctrlIdx, dataSize);
    return retVal;
}

CFErrorCode ControlWaveform::galvoSpeedCheck(double maxVolSpeed, int minVol, int maxVol, int ctrlIdx, int &dataSize)
{
    int minRaw = voltage2Raw(minVol);
    int maxRaw = voltage2Raw(maxVol);
    int maxRawSpeed = voltage2Raw(maxVolSpeed); // change voltage speed to raw data speed
    CFErrorCode retVal = analogSpeedCheck(maxRawSpeed, minRaw, maxRaw, ctrlIdx, dataSize);
    return retVal;
}

CFErrorCode ControlWaveform::analogSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx, int &dataSize)
{
    int controlIdx, repeat, waveIdx;
    SampleIdx idx = 0, idxStrt, idxStop;
    bool isFullTest = false;
    QVector <SampleIdx> shortWaveIdxStrt, shortWaveIdxStop;

    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
        int waveLength = getWaveSampleNum(waveIdx);
        if (waveLength < SPEED_CHECK_INTERVAL) {
            idxStrt = idx - SPEED_CHECK_INTERVAL;
            if (idxStrt < 0) idxStrt = 0;
            idxStop = idx + repeat*waveLength;
            shortWaveIdxStrt.push_back(idxStrt);
            shortWaveIdxStop.push_back(idxStop);
        }
        idx += repeat*waveLength;
    }

    CFErrorCode retVal = fastSpeedCheck(maxSpeed, minRaw, maxRaw, ctrlIdx, dataSize); // this is too slow
    if ((retVal == NO_CF_ERROR) && !shortWaveIdxStrt.isEmpty())
        return fullSpeedCheck(maxSpeed, minRaw, maxRaw, ctrlIdx, shortWaveIdxStrt, shortWaveIdxStop, dataSize);
    else
        return retVal;
}

CFErrorCode ControlWaveform::fullSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx,
    QVector <SampleIdx>&strt, QVector <SampleIdx>&stop, int &dataSize)
{
    qint16 rawWave0, rawWave1;
    double distance, time, speed;
    int wave0, wave1;
    int instantChangeTh;

    instantChangeTh = maxSpeed / sampleRate + 1;
    time = static_cast<double>(SPEED_CHECK_INTERVAL) / static_cast<double>(sampleRate); // sec

    for (int i = 0; i < strt.size(); i++) {
        // 1 Speed check : But, this cannot filter out periodic signal which has same period as 'interval'
        SampleIdx end = (stop[i] < getCtrlSampleNum(ctrlIdx) - SPEED_CHECK_INTERVAL) ?
            stop[i] : getCtrlSampleNum(ctrlIdx) - SPEED_CHECK_INTERVAL - 1;
        for (SampleIdx j = strt[i]; j < end; j++) {
            getCtrlSampleValue(ctrlIdx, j, rawWave0, PDT_RAW);
            if ((rawWave0 < minRaw)||(rawWave0 > maxRaw)) // Invalid value
                return ERR_PIEZO_VALUE_INVALID;
            getCtrlSampleValue(ctrlIdx, j + SPEED_CHECK_INTERVAL, rawWave1, PDT_RAW); //PDT_RAW
            distance = abs(rawWave1 - rawWave0);
            if (distance > 1) {
                speed = (distance - 1) / time;// -1 -> to account quantization noise
                if (speed > maxSpeed) {
                    return ERR_PIEZO_SPEED_FAST;
                }
            }
        }
        // 2 Instantaneous change check : This will improve a defect of 1.
        end = (stop[i] < getCtrlSampleNum(ctrlIdx) - 1) ?
            stop[i] : getCtrlSampleNum(ctrlIdx) - 2;
        getCtrlSampleValue(ctrlIdx, 0, wave0, PDT_RAW);
        for (SampleIdx j = strt[i]; j < end; j++) {
            getCtrlSampleValue(ctrlIdx, j, wave1, PDT_RAW);
            if (abs(wave1 - wave0) > instantChangeTh)
                return ERR_PIEZO_INSTANT_CHANGE;
            wave0 = wave1;
        }
    }
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::fastSpeedCheck(int maxSpeed, int minRaw, int maxRaw, int ctrlIdx, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    double distance, time, speed;
    int controlIdx, repeat, waveIdx;
    int sampleIdx, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;
    QVector<bool> usedWaveIdx(waveList_Raw.size(), false);
    int wave0, wave1;
    qint16 rawWave0, rawWave1;
    int margin_interval, margin_instant;
    int instantChangeTh;
    
    instantChangeTh  = maxSpeed / sampleRate + 1;
    time = static_cast<double>(SPEED_CHECK_INTERVAL) / static_cast<double>(sampleRate); // sec

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
        int waveLength = getWaveSampleNum(waveIdx);
        if (waveLength < SPEED_CHECK_INTERVAL) // we skip short waveform
            continue;
        if (usedWaveIdx[waveIdx] == false)
        {
            // 1. Check individual waveforms
            if (repeat > 1) { // if repeat, wraparound check
                margin_interval = 0; // For speed check
                margin_instant = 0; // For intantaneous change check
            }
            else { // if no repeat, only internal check
                margin_interval = SPEED_CHECK_INTERVAL;
                margin_instant = 1;
            }
            for (int i = 0; i < getWaveSampleNum(waveIdx); i++) {
                getWaveSampleValue(waveIdx, i, rawWave0, PDT_RAW);
                if ((rawWave0 < minRaw) || (rawWave0 > maxRaw)) // Invalid value
                    return ERR_PIEZO_VALUE_INVALID;
                // 1.1 Speed check : But, this cannot filter out periodic signal which has same period as 'interval'
                if (i < getWaveSampleNum(waveIdx) - margin_interval) { // If margin == interval, no wraparound check
                    getWaveSampleValue(waveIdx, (i + SPEED_CHECK_INTERVAL) % getWaveSampleNum(waveIdx), rawWave1, PDT_RAW);
                    distance = abs(rawWave1 - rawWave0);
                    if (distance > 1) {
                        speed = (distance-1) / time;// raw2Zpos((distance - 1) / time + 0.5); // piezostep/sec
                        if (speed > maxSpeed) {
                            return ERR_PIEZO_SPEED_FAST;
                        }
                    }
                }
                // 1.2 Instantaneous change check : This will improve a defect of 1.1
                getWaveSampleValue(waveIdx, i, wave0, PDT_RAW);
                if (i < getWaveSampleNum(waveIdx) - margin_instant) {
                    getWaveSampleValue(waveIdx, (i + 1) % getWaveSampleNum(waveIdx), wave1, PDT_RAW);
                    if (abs(wave1 - wave0) > instantChangeTh)
                        return ERR_PIEZO_INSTANT_CHANGE;
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
        // 2. If control signal is compoused of several waveforms, we need to check connection points.
        if (controlIdx >= 2) {
            // 2.1 Speed check
            int oldWaveIdx = controlList[ctrlIdx]->at(controlIdx - 1);
            int oldWaveLength = getWaveSampleNum(oldWaveIdx);
            if (oldWaveLength < SPEED_CHECK_INTERVAL) // we skip short waveform
                continue;
            for (int i = getWaveSampleNum(oldWaveIdx) - SPEED_CHECK_INTERVAL, j = 0; i < getWaveSampleNum(oldWaveIdx); i++, j++) {
                getWaveSampleValue(oldWaveIdx, i, rawWave0, PDT_RAW);
                getWaveSampleValue(waveIdx, j, rawWave1, PDT_RAW);
                distance = abs(rawWave1 - rawWave0);
                if (distance > 1) {
                    speed = (distance - 1) / time;
                    if (speed > maxSpeed) {
                        return ERR_PIEZO_SPEED_FAST;
                    }
                }
            }
            // 2.2 Instantaneous change check
            getWaveSampleValue(oldWaveIdx, getWaveSampleNum(oldWaveIdx) - 1, wave0, PDT_RAW);
            getWaveSampleValue(waveIdx, 0, wave1, PDT_RAW);
            if (abs(wave1 - wave0) > instantChangeTh)
                return ERR_PIEZO_INSTANT_CHANGE;
        }
    }
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::cameraPulseNumCheck(int nTotalFrames, int ctrlIdx, int &nPulses, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    int repeat, waveIdx, lastWaveIdx, controlIdx;
    nPulses = 0;

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
        lastWaveIdx = waveIdx;
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
        // count transition inside waveform
        int nPulsesInStack = 0;
        int wave0, wave1, lastIdx = -1;
        for (int i = 0; i < getWaveSampleNum(waveIdx) - 1; i++) {
            getWaveSampleValue(waveIdx, i, wave0);
            getWaveSampleValue(waveIdx, i + 1, wave1);
            if ((wave0 == 0) && (wave1 != 0)) { // check rising edge
                nPulsesInStack++;
            }
        }
        nPulses += repeat*nPulsesInStack;
        // count transition between same waveform
        if (repeat > 1) {
            getWaveSampleValue(waveIdx, getWaveSampleNum(waveIdx)-1, wave0);
            getWaveSampleValue(waveIdx, 0, wave1);
            if ((wave0 == 0) && (wave1 != 0)) { // check rising edge
                nPulses += repeat-1;
            }
        }
        // count transition between different waveform
        if (controlIdx == 0) wave0 = 0; // every channel is low initially
        else getWaveSampleValue(lastWaveIdx, getWaveSampleNum(lastWaveIdx) - 1, wave0);
        getWaveSampleValue(waveIdx, 0, wave1);
        if ((wave0 == 0) && (wave1 != 0)) { // check rising edge
            nPulses++;
        }
    }
    if (nPulses != nTotalFrames)
        return ERR_CAMERA_PULSE_NUM_ERR;
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::laserSpeedCheck(double maxFreq, int ctrlIdx, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    int waveIdx, controlIdx;
    double freq, durationInSamples;
    QVector<bool> usedWaveIdx(waveList_Raw.size(), false);

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
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
                        return ERR_LASER_SPEED_FAST;
                    lastIdx = i;
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
    }
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::waveformValidityCheck()
{
    int sampleNum;
    bool piezoWaveEmpty;
    bool cameraWaveEmpty;
    bool laserWaveEmpty;
    CFErrorCode errorCode = NO_CF_ERROR;
    errorMsg.clear();

    piezoWaveEmpty  = isPiezoWaveEmpty();
    cameraWaveEmpty = isCameraPulseEmpty();
    laserWaveEmpty  = isLaserPulseEmpty();
    if (piezoWaveEmpty || cameraWaveEmpty || laserWaveEmpty) {
        errorMsg.append("At least one positioner, one camera and one laser control waveform are needed\n");
        return ERR_CONTROL_SEQ_MISSING;
    }
    else {
        for (int i = 0; i < channelSignalList.size(); i++) {
            // piezo speed check
            if (channelSignalList[i][1].right(5) == STR_piezo) {
                CFErrorCode err = positionerSpeedCheck(maxPiezoSpeed, 0, maxPiezoPos, i, sampleNum);
                if (err & (ERR_PIEZO_SPEED_FAST | ERR_PIEZO_INSTANT_CHANGE)) {
                    errorMsg.append(QString("'%1' control is too fast\n").arg(channelSignalList[i][1]));
                }
                if (err & (ERR_PIEZO_VALUE_INVALID)) {
                    errorMsg.append(QString("'%1' control value should not be negative\n")
                    .arg(channelSignalList[i][1]));//.arg(maxPos));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
                if (err & (ERR_SHORT_WAVEFORM)) {
                    errorMsg.append(QString("'%1' control includes too short waveform\nWaveform should be at least %2 samples")
                        .arg(channelSignalList[i][1]).arg(SPEED_CHECK_INTERVAL));
                }
                errorCode |= err;
            }
            // camera pulse number check
            else if (channelSignalList[i][1] == STR_camera1) {
                int nPulses;
                CFErrorCode err = cameraPulseNumCheck(nStacks1*nFrames1, i, nPulses, sampleNum);
                if (err & ERR_CAMERA_PULSE_NUM_ERR) {
                    errorMsg.append(QString("'%1' pulse number %2 is not consistent with stack and frame number\n")
                        .arg(channelSignalList[i][1]).arg(nPulses));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
                errorCode |= err;
            }
            else if (channelSignalList[i][1] == STR_camera2) {
                int nPulses;
                CFErrorCode err = cameraPulseNumCheck(nStacks2*nFrames2, i, nPulses, sampleNum);
                if (err & ERR_CAMERA_PULSE_NUM_ERR) {
                    errorMsg.append(QString("'%1' pulse number %2 is not consistent with stack and frame number\n")
                        .arg(channelSignalList[i][1]).arg(nPulses));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
                errorCode |= err;
            }
            // laser speed check
            else if ((channelSignalList[i][1].right(5) == STR_laser)|| (channelSignalList[i][1] == STR_all_lasers)) {
                CFErrorCode err = laserSpeedCheck(maxLaserFreq, i, sampleNum); // OCPI-1 should be less then 25Hz
                if (err & ERR_LASER_SPEED_FAST) {
                    errorMsg.append(QString("'%1' control is too fast\n").arg(channelSignalList[i][1]));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
                errorCode |= err;
            }
            // galvo control speed check
            if (channelSignalList[i][1].left(5) == STR_galvo) {
                CFErrorCode err = galvoSpeedCheck(maxGalvoSpeed, minGalvoVol, maxGalvoVol, i, sampleNum);
                if (err & (ERR_PIEZO_SPEED_FAST | ERR_PIEZO_INSTANT_CHANGE)) {
                    errorMsg.append(QString("'%1' control is too fast\n").arg(channelSignalList[i][1]));
                }
                if (err & (ERR_PIEZO_VALUE_INVALID)) {
                    errorMsg.append(QString("'%1' control value should not be negative\n")
                        .arg(channelSignalList[i][1]));//.arg(maxPos));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
                if (err & (ERR_SHORT_WAVEFORM)) {
                    errorMsg.append(QString("'%1' control includes too short waveform\nWaveform should be at least %2 samples")
                        .arg(channelSignalList[i][1]).arg(SPEED_CHECK_INTERVAL));
                }
                errorCode |= err;
            }
            // For other signals, just check sample numbers.
            else {
                sampleNum = getCtrlSampleNum(i);
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    errorCode |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("'%1' sample number is different from total sample number\n").arg(channelSignalList[i][1]));
                }
            }
        }
    }
    return errorCode;
}

/******* ControlWaveform class : Generate waveform *********************/
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
    int j = 0;

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
    rlbuf.push_back(value);
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

void genWaveform(QVariantList &array, double startPos, double stopPos, int nDelay, int nRising, int nFalling, int nTotal)
{
    qint16 wav, wavPrev;
    int runLength = 0;
    double  avg = startPos;

    array.clear();
    for (int i = 0; i < nTotal; i++)
    {
        int ii = i % nTotal;
        if ((ii >= nDelay) && (ii < nDelay + nRising)) {
            avg += (stopPos - startPos) / static_cast<double>(nRising);
        }
        else if ((ii >= nDelay + nRising) &&
            (ii < nDelay + nRising + nFalling)) {
            double tmp = (stopPos - startPos) / static_cast<double>(nFalling);
            avg -= tmp;
        }
        wav = static_cast<qint16>(avg + 0.5);
        if ((wav != wavPrev) && i) {
            array.append(runLength);
            array.append(wavPrev);
            runLength = 1;
        }
        else {
            runLength++;
        }
        wavPrev = wav;
    }
    array.append(runLength);
    array.append(wavPrev);
}

void genWaveform(QJsonArray &jarray, double startPos, double stopPos, int nDelay, int nRising, int nFalling, int nTotal)
{
    QVariantList list;
    genWaveform(list, startPos, stopPos, nDelay, nRising, nFalling, nTotal);
    // QJsonArray::append() is too slow
    // So, we store data to QVariantList container first.
    // Then, we convert it using QJsonArray::fromVariantList
    // to contain the data to QJsonArray.
    jarray = QJsonArray::fromVariantList(list);
}

void genPulse(QVariantList &array, int nDelay, int nRising, int nFalling, int nTotal, int nMargin, int nShutter, int nExposure)
{
    int pulse, pulsePrev;
    int runLength = 0;

    array.clear();
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
            array.append(runLength);
            array.append(pulsePrev);
            runLength = 1;
        }
        else {
            runLength++;
        }
        pulsePrev = pulse;
    }
    array.append(runLength);
    array.append(pulsePrev);
}

void genPulse(QJsonArray &jarray, int nDelay, int nRising, int nFalling, int nTotal, int nMargin, int nShutter, int nExposure)
{
    QVariantList list;
    genPulse(list, nDelay, nRising, nFalling, nTotal, nMargin, nShutter, nExposure);
    jarray = QJsonArray::fromVariantList(list);
}

void genWavBiDirection(QVariantList &vlarray, QVariantList refArray, int nDelay, int nRising, int nIdleTime)
{
    int i, value;
    QVariantList buf1, buf2;
    conRunLengthToWav(refArray, buf1);

    for (i = 0; i < nDelay + nRising; i++) {
        value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    i--;
    for (int j = 0; j < nIdleTime; j++) {
        value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    for (int j = 0; j < nRising; j++, i--) {
        value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    i++;
    for (int j = 0; j < nIdleTime; j++) {
        value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    conWavToRunLength(buf2, vlarray);
}

void genBiDirection(QVariantList &vlarray, QVariantList refArray, int nDelay, int nRising,
            int nIdleTime, int numOnSample, bool bidirection)
{
    int i;
    QVariantList buf1, buf2;
    conRunLengthToWav(refArray, buf1);

    int pre = 0, cur, next;
    for (i = 0; i < nDelay + nRising; i++) {
        cur = buf1.at(i).toInt();
        if((pre==0)&&(cur==1)) // riging edge
            buf2.push_back(1);
        else
            buf2.push_back(0);
        pre = cur;
    }
    i--;
    for (int j = 0; j < nIdleTime; j++) {
        int value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    for (int j = 0; j < nRising-1; j++, i--) {
        next = buf1.at(i-1).toInt();
        if ((cur == 0) && (next == 1)) // riging edge
        //if ((cur == 1) && (next == 0)) // falling edge
            if(bidirection)
                buf2.push_back(1);
            else
                buf2.push_back(0);
        else
            buf2.push_back(0);
        cur = next;
    }
    buf2.push_back(0);
    for (int j = 0; j < nIdleTime; j++) {
        buf2.push_back(0);
    }
    pre = 0;
    for (int j = 0; j < buf2.size(); j++) {
        cur = buf2.at(j).toInt();
        if ((pre == 0) && (cur == 1)) {
            for (int k = 0; k < numOnSample; k++)
                buf2.replace(j + k, 1);
        }
        pre = cur;
    }
    conWavToRunLength(buf2, vlarray);
}

void mirror(QVariantList &vlarray, QVariantList refArray)
{
    QVariantList buf1, buf2;
    conRunLengthToWav(refArray, buf1);
    for (int i = buf1.size() - 1; i >= 0; i--) {
        int value = buf1.at(i).toInt();
        buf2.push_back(value);
    }
    conWavToRunLength(buf2, vlarray);
}

int genCosineWave(QVariantList &vlarray, QVariantList &vlparray1, QVariantList &vlparray2, double piezoMin, double piezoMax, int delay, int freq, int pulseNum)
{
    int amplitude = static_cast<int>((piezoMax - piezoMin) / 2.);
    int offset = static_cast<int>((piezoMax + piezoMin) / 2.);
    // digital pulse start and stop level margin
    int margin = 50;
    // pulseLevelRange = [levelBegin, levelEnd] = [piezoMin + margin, piezoMax - margin]
    int levelStart = static_cast<int>(piezoMin) + margin;
    int levelEnd = static_cast<int>(piezoMax) - margin;
    int pulseLevelRange = levelEnd - levelStart;
    int levelStep = pulseLevelRange / pulseNum;

    QVariantList buf1, buf2, buf3;
    double damp = static_cast<double>(amplitude);
    double dfreq = static_cast<double>(freq);
    dfreq = M_PI / dfreq;

    for (int i = 0; i < delay; i++) {
        buf1.push_back(static_cast<int>(piezoMin));
        buf2.push_back(0);
        buf3.push_back(0);
    }
    int level = levelStart;
    int iOld = 0, di, diMin = freq;
    for (int i = 0, j = 0; i < freq; i++) {
        double value = -damp*cos(dfreq*static_cast<double>(i));
        int ival = static_cast<int>(value) + offset;
        buf1.push_back(ival); // cosine piezo value
        if ((ival >= level)&&(ival< levelEnd)) {
            if (j == 0) {
                int di = i - iOld;
                if (di < diMin)
                    diMin = di;
                iOld = i;
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
        if ((ival >= levelStart - levelStep) && (ival <= levelEnd + levelStep))
            buf3.push_back(1);
        else
            buf3.push_back(0);
    }
    conWavToRunLength(buf1, vlarray);
    conWavToRunLength(buf2, vlparray1);
    conWavToRunLength(buf3, vlparray2);
    return diMin;
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

CFErrorCode ControlWaveform::genDefaultControl(QString filename)
{
    int nFrames, nStacks;
    if ((enableCam1)&& (enableCam2)) {
        nFrames = max(nFrames1, nFrames2);
        nStacks = max(nStacks1, nStacks2);
    }
    else if (enableCam1) {
        nFrames = nFrames1;
        nStacks = nStacks1;
    }
    else {
        nFrames = nFrames2;
        nStacks = nStacks2;
    }
    CFErrorCode err = NO_CF_ERROR;
    errorMsg.clear();
    double shutterControlMargin = 0.01; // (sec) shutter control begin after piezo start and stop before piezo stop
    double laserControlMargin = 0.005;  // (sec) laser control begin after piezo start and stop before piezo stop
                                        // This value should be less than shutterControlMargin
    double valveControlMargin = 0.0;    // (sec) stimulus control begin after piezo start and stop before piezo stop
                                        // This value should be less than shutterControlMargin
    double valveDelay = 0.0;            // Initial delay

    QVariantList buf1, buf2, buf3, buf4;
    QVariantList buf11, buf22, buf33, buf44;
    double minPiezoTravelBackTime = (double)abs(piezoStopPosUm - piezoStartPosUm) / (double)maxPiezoSpeed;
    double frameTime = cycleTime + 0.001; // give a little more time to make fall down transition after exposure time (0.001)
    int eachShutterCtrlSamples = (int)(frameTime*sampleRate + 0.5);

    int piezoDelaySamples = 0;
    int piezoRisingSamples = nFrames*eachShutterCtrlSamples + 2*shutterControlMargin*sampleRate;
    if (piezoRisingSamples < minPiezoTravelBackTime*sampleRate)
    {
        double piezoRisingTime = minPiezoTravelBackTime;
        double cameraTime = piezoRisingTime - 2 * shutterControlMargin;
        eachShutterCtrlSamples = (int)(cameraTime*sampleRate / nFrames + 0.5);
        piezoRisingSamples = nFrames*eachShutterCtrlSamples + 2 * shutterControlMargin*sampleRate;
    }
    //int piezoRisingSamples = (nFrames*frameTime + 2 * shutterControlMargin)*sampleRate;
    if (!bidirection1 && !bidirection2 && (piezoTravelBackTime < minPiezoTravelBackTime)) {
        errorMsg.append(QString("Piezo travel back time is too short\n"));
        err |= ERR_TRAVELBACKTIME_SHORT;
        return err;
    }
    int piezoFallingSamples = piezoTravelBackTime*sampleRate;
    int idleTimeSamples = idleTimeBwtnStacks*sampleRate;
    int piezoWaitSamples = (idleTimeBwtnStacks - piezoTravelBackTime)*sampleRate;
    if (idleTimeBwtnStacks < piezoTravelBackTime)
        err |= ERR_IDLETIME_SHORT;
    perStackSamples = piezoRisingSamples + piezoFallingSamples + piezoWaitSamples;
    genWaveform(buf1, zpos2Raw(piezoStartPosUm), zpos2Raw(piezoStopPosUm), piezoDelaySamples,
        piezoRisingSamples, piezoFallingSamples, perStackSamples);

    /// camera shutter pulse ///
    // paramters specified by user
    int shutterControlMarginSamples = shutterControlMargin*sampleRate;
    // generating camera shutter pulse
    int stakShutterCtrlSamples = piezoRisingSamples - 2 * shutterControlMarginSamples; // full stack period sample number
    //int eachShutterCtrlSamples = (int)((double)stakShutterCtrlSamples / (double)nFrames + 0.5);     // one frame sample number
    // camera1 pulse
    int exposureSamples1 = static_cast<int>(exposureTime1*static_cast<double>(sampleRate));
//    if (exposureSamples > eachShutterCtrlSamples - 50 * (sampleRate / 10000))
//        return ERR_EXPOSURE_TIME;
    int eachShutterOnSamples = exposureSamples1;
    genPulse(buf2, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);
    // camera2 pulse
    int exposureSamples2 = static_cast<int>(exposureTime2*static_cast<double>(sampleRate));
    eachShutterOnSamples = exposureSamples2;
    genPulse(buf4, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);

    /// laser shutter pulse ///
    // paramters specified by user
    int laserControlMarginSamples = laserControlMargin*sampleRate; // laser control begin after piezo start and stop before piezo stop
    // generating laser shutter pulse
    stakShutterCtrlSamples = piezoRisingSamples - 2 * laserControlMarginSamples; // full stack period sample number
    eachShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    eachShutterOnSamples = eachShutterCtrlSamples; // all high (means just one pulse)
    genPulse(buf3, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        laserControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);

    QJsonArray piezo1_001;
    QJsonArray camera1_001;
    QJsonArray camera2_001;
    QJsonArray laser1_001;

    if (bidirection1 || bidirection2) {
        // bi-directional wave
        genWavBiDirection(buf11, buf1, piezoDelaySamples, piezoRisingSamples, idleTimeSamples); // piezo
        eachShutterOnSamples = exposureSamples1;
        genBiDirection(buf22, buf2, piezoDelaySamples, piezoRisingSamples, idleTimeSamples, eachShutterOnSamples, bidirection1); // camera1
        eachShutterOnSamples = exposureSamples2;
        genBiDirection(buf44, buf4, piezoDelaySamples, piezoRisingSamples, idleTimeSamples, eachShutterOnSamples, bidirection2); // camera2
        eachShutterOnSamples = stakShutterCtrlSamples;
        genBiDirection(buf33, buf3, piezoDelaySamples, piezoRisingSamples, idleTimeSamples, eachShutterOnSamples, true); // laser
        piezo1_001 = QJsonArray::fromVariantList(buf11);
        camera1_001 = QJsonArray::fromVariantList(buf22);
        camera2_001 = QJsonArray::fromVariantList(buf44);
        laser1_001 = QJsonArray::fromVariantList(buf33);
        perStackSamples = piezoRisingSamples + idleTimeSamples;
    }
    else {
        piezo1_001 = QJsonArray::fromVariantList(buf1);
        camera1_001 = QJsonArray::fromVariantList(buf2);
        camera2_001 = QJsonArray::fromVariantList(buf4);
        laser1_001 = QJsonArray::fromVariantList(buf3);
    }

    ///  Register subsequences to waveform list  ///
    QJsonObject wavelist;
    wavelist["positioner1_001"] = piezo1_001;
    if (enableCam1)
        wavelist["camera1_001"] = camera1_001;
    if (enableCam2)
        wavelist["camera2_001"] = camera2_001;
    wavelist["laser1_001"] = laser1_001;

    /// stimulus valve pulse ///
    QVector<int> valve;
    QVector<QString> stimuliPortMap;// = { "P0.0", "P0.1", "P0.2", "P0.3" }; // for standard
    QVector<QString> stimulusName;// = { "stimulus1", "stimulus2", "stimulus3", "stimulus4" };
    if (applyStim) {
        QJsonArray valve_on, valve_off;
        // paramters specified by user
        int valveDelaySamples = valveDelay*sampleRate; // no delay
        int valveOnSamples = perStackSamples;// piezoRisingSamples + 2 * piezoDelaySamples;
        int valveOffSamples = 0;// piezoFallingSamples - piezoDelaySamples;
        int valveControlMarginSamples = valveControlMargin*sampleRate;
        eachShutterCtrlSamples = valveOnSamples; // just one pulse
        eachShutterOnSamples = valveOnSamples; // all high
                                               // generating stimulus valve pulse
        if (bidirection1 || bidirection2) {
            QJsonArray valve_on_mirror;
            valveOnSamples = piezoRisingSamples + piezoDelaySamples + idleTimeSamples;
            valveOffSamples = 0;
            eachShutterCtrlSamples = valveOnSamples; // just one pulse
            eachShutterOnSamples = valveOnSamples; // all high
            genConstantWave(buf1, 0, valveOnSamples);
            valve_off = QJsonArray::fromVariantList(buf1); // valve_off
            genPulse(buf1, valveDelaySamples, valveOnSamples, valveOffSamples, valveOnSamples,
                valveControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);
            valve_on = QJsonArray::fromVariantList(buf1); // valve_on
            mirror(buf11, buf1);
            valve_on_mirror = QJsonArray::fromVariantList(buf11); // valve_on_mirror
            wavelist["valve_on_mirror"] = valve_on_mirror;
        }
        else {
            genConstantWave(buf1, 0, perStackSamples);
            valve_off = QJsonArray::fromVariantList(buf1); // valve_off
            genPulse(valve_on, valveDelaySamples, valveOnSamples, valveOffSamples, perStackSamples,
                valveControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples); // valve_on
        }
        wavelist["valve_on"] = valve_on;
        wavelist["valve_off"] = valve_off;

        int idx, value;
        bool found;
        // For Haoyang 5bits {A0, A1, A2, A3, Mux en} are mapped to {P0.0, P0.1, P0.2, P0.3, P0.18}
        //QVector<int> valueConv = { 0,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
        // For standard Imagine 4bits {stimulus1, stimulus2, stimulus3, stimulus4 } are mapped to
        // {P0.0, P0.1, P0.2, P0.3}
        QVector<int> valueConv = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        if (valueConv.size() == 16) {// 4bits
            stimuliPortMap.append({ "P0.0", "P0.1", "P0.2", "P0.3" });
            stimulusName.append({ "stimulus1", "stimulus2", "stimulus3", "stimulus4" });
        }
        else { // for Haoyang (5bits)
            if(rig=="ocpi-1")
                stimuliPortMap.append({ "P0.0", "P0.1", "P0.2", "P0.3", "P0.6" });
            else
                stimuliPortMap.append({ "P0.0", "P0.1", "P0.2", "P0.3", "P0.18" });
            stimulusName.append({ "A0", "A1", "A2", "A3", "Mux en" });
        }
        for (int i = 0; i < nStacks; ++i) {
            found = false;
            for (idx = 0; idx < stimuli->size(); idx++) {
                if (stimuli->at(idx).second == i) {
                    found = true;
                    break;
                }
            }
            if (found) {
                value = stimuli->at(idx).first;
                if (value < valueConv.size())
                    valve.push_back(valueConv[value]);
                else
                    valve.push_back(0);
            }
            else { // use last value
                if (value < valueConv.size())
                    valve.push_back(valueConv[value]);
                else
                    valve.push_back(0);
            }
        }
    }

    ////////  Generating full sequences of waveforms and pulses  ////////
    QJsonObject analog, digital;
    QJsonArray piezo1Seq, camera1Seq, camera2Seq, laser1Seq;
    QJsonObject piezo1, camera1, camera2, laser1;
    QJsonObject stimulus1, stimulus2, stimulus3, stimulus4;
    QJsonObject piezo1Mon, piezo2Mon, ai1Mon, camera1Mon, camera2Mon;
    QJsonObject ai4Mon, ai5Mon;
    QJsonObject laser1Mon, stimulus1Mon, stimulus2Mon;
    int repeat;
    if (bidirection1 || bidirection2) {
        repeat = nStacks / 2;
    }
    else {
        repeat = nStacks;
    }
    for (int i = 0; i < channelSignalList.size(); i++) {
        QString port = channelSignalList[i][0];
        QString sig = channelSignalList[i][1];
        if (port == QString(STR_AOHEADER).append("0")) { // AO0
            piezo1Seq = { repeat, "positioner1_001" };
            piezo1[STR_Channel] = port;
            piezo1[STR_Seq] = piezo1Seq;
            analog[sig] = piezo1; // secured port for piezo
        }

        if (port == QString(STR_AIHEADER).append("0")) { // AI0
            piezo1Mon[STR_Channel] = port;
            analog[sig] = piezo1Mon; // secured port for piezo monitor
        }
        if ((rig == "ocpi-1") || (rig == "ocpi-lsk")) {
            if (port == QString(STR_AIHEADER).append("1")) { // AI1
                stimulus1Mon[STR_Channel] = port;
                analog[sig] = stimulus1Mon; // secured port for stimulus monitor
            }
        }
        else {
            if (port == QString(STR_AIHEADER).append("1")) { // AI1
                piezo2Mon[STR_Channel] = port;
                analog[sig] = piezo2Mon; // horizontal piezo monitor
            }
        }

        if (port == QString(STR_AIHEADER).append("4")) { // AI4
            ai4Mon[STR_Channel] = port;
            analog["AI4"] = ai4Mon; // for compatibility with old Imainge HW configuration
        }
        if (rig != "ocpi-1") {
            if (port == QString(STR_AIHEADER).append("5")) { // AI5
                ai5Mon[STR_Channel] = port;
                analog["AI5"] = ai5Mon; // for compatibility with old Imainge HW configuration
            }
        }
        if (enableCam1) {
            if (port == QString(STR_P0HEADER).append("5")) { // P0.5
                camera1Seq = { repeat, "camera1_001" };
                camera1[STR_Channel] = port;
                camera1[STR_Seq] = camera1Seq;
                digital[sig] = camera1; // secured port for camera1
            }
        }

        if (enableCam2) {
            if (port == QString(STR_P0HEADER).append("6")) { // P0.6
                camera2Seq = { repeat, "camera2_001" };
                camera2[STR_Channel] = port;
                camera2[STR_Seq] = camera2Seq;
                digital[sig] = camera2; // secured port for camera2
            }
        }

        if (port == QString(STR_P0HEADER).append("4")) { // P0.4
            laser1Seq = { repeat, "laser1_001" };
            laser1[STR_Channel] = port;
            laser1[STR_Seq] = laser1Seq;
            digital[sig] = laser1; // secured port for laser
        }

        QString camera1InPort, camera2InPort;
        if (rig == "ocpi-1") {
            camera1InPort = QString(STR_P0HEADER).append("7"); // P0.7
            if (port == camera1InPort) {
                camera1Mon[STR_Channel] = port;
                digital[sig] = camera1Mon; // secured port for camera1 mon
            }
        }
        else if (rig == "ocpi-lsk") {
            camera1InPort = QString(STR_P0HEADER).append("24"); // P0.24
            if (port == camera1InPort) {
                camera1Mon[STR_Channel] = port;
                digital[sig] = camera1Mon; // secured port for camera1 mon
            }
        }
        else {
            if (enableCam1) {
                camera1InPort = QString(STR_P0HEADER).append("24"); // P0.24
                if (port == camera1InPort) {
                    camera1Mon[STR_Channel] = port;
                    digital[sig] = camera1Mon; // secured port for camera1 mon
                }
            }
            if (enableCam2) {
                camera2InPort = QString(STR_P0HEADER).append("25"); // P0.25
                if (port == camera2InPort) {
                    camera2Mon[STR_Channel] = port;
                    digital[sig] = camera2Mon; // secured port for camera2 mon
                }
            }
        }

        if (applyStim && valve.size()) {
            for (int j = 0; j < stimuliPortMap.size(); j++) {
                if (port == stimuliPortMap[j]) {
                    QJsonArray stimulusSeq;
                    QJsonObject stimulus;
                    int count = 1;
                    int pre_val;
                    int cur_val = (valve[0] >> j) & 1;
                    for (int k = 1; k < valve.size(); k++) {
                        pre_val = cur_val;
                        if (bidirection1)
                            cur_val = (k % 2) ? ((valve[k] >> j) & 1) * 2 : // valve_on_mirror
                                                 (valve[k] >> j) & 1;       // valve_on
                        else
                            cur_val = (valve[k] >> j) & 1;
                        if (cur_val != pre_val) {
                            stimulusSeq.append(count);
                            if (pre_val == 1)
                                stimulusSeq.append("valve_on");
                            else if (pre_val == 2)
                                stimulusSeq.append("valve_on_mirror");
                            else
                                stimulusSeq.append("valve_off");
                            count = 1;
                        }
                        else {
                            count++;
                        }
                    }
                    stimulusSeq.append(count);
                    if (cur_val == 1)
                        stimulusSeq.append("valve_on");
                    else if (cur_val == 2)
                        stimulusSeq.append("valve_on_mirror");
                    else
                        stimulusSeq.append("valve_off");
                    stimulus[STR_Channel] = port;
                    stimulus[STR_Seq] = stimulusSeq;
                    if (sig == "")  // custom port
                        digital[stimulusName[j]] = stimulus;
                    else {
                        errorMsg.append(QString("'%1' channel is already reserved as a '%2' signal\n")
                            .arg(port).arg(sig));
                        err |= ERR_INVALID_PORT_ASSIGNMENT;
                        return err;
                    }
                }
            }
        }
    }
    SampleIdx totalSamples = nStacks * perStackSamples;
    // packing all
    QJsonObject metadata;
    QJsonObject cam1metadata;
    QJsonObject cam2metadata;

    if (enableCam1) {
        cam1metadata[STR_Exposure] = exposureTime1;
        cam1metadata[STR_ExpTrigMode] = expTriggerModeStr1;
        cam1metadata[STR_Frames] = nFrames1;
        cam1metadata[STR_BiDir] = bidirection1;
        cam1metadata[STR_Stacks] = nStacks1;
        metadata[STR_Cam1Metadata] = cam1metadata;
    }
    if (enableCam2) {
        cam2metadata[STR_Exposure] = exposureTime2;
        cam2metadata[STR_ExpTrigMode] = expTriggerModeStr2;
        cam2metadata[STR_Frames] = nFrames2;
        cam2metadata[STR_BiDir] = bidirection2;
        cam2metadata[STR_Stacks] = nStacks2;
        metadata[STR_Cam2Metadata] = cam2metadata;
    }
    metadata[STR_SampleRate] = sampleRate;
    metadata[STR_TotalSamples] = totalSamples;
    metadata[STR_Rig] = rig;
    metadata[STR_GeneratedFrom] = STR_Imagine;

    alldata[STR_Metadata] = metadata;
    alldata[STR_Analog] = analog;
    alldata[STR_Digital] = digital;
    alldata[STR_WaveList] = wavelist;
    alldata[STR_Version] = CF_VERSION;

    // save data
    enum SaveFormat {
        Json, Binary
    };
    if (filename != "") {
        SaveFormat saveFormat = Json;// Binary;Json
        QString newName = addSuffixToBaseName(filename, "_gui");
        QFile saveFile(saveFormat == Json
            ? replaceExtName(newName, "json")
            : replaceExtName(newName, "bin"));
        if (saveFile.open(QIODevice::WriteOnly)) {
            QJsonDocument saveDoc(alldata);
            saveFile.write(saveFormat == Json
                ? saveDoc.toJson()
                : saveDoc.toBinaryData());
        }
        else {
            qWarning("Couldn't open save file.");
            err |= ERR_FILE_OPEN;
        }
    }

    // parsing
    err |= parsing();
    return err;
}

int ControlWaveform::genSinusoidal(bool bidir)
{
    enum SaveFormat {
        Json, Binary
    };
    enum WaveType {
        Triangle, Triangle_bidirection, Sinusoidal
    } wtype;

    qint16 piezo;
    bool shutter, laser, ttl2;
    double piezom1, piezo0, piezop1, piezoavg = 0.;

    /* For metadata */
    int sampleRate = 10000;     // This can be configurable in the GUI
    SampleIdx totalSamples = 0; // this will be calculated later;
    int nStacks = 10;           // number of stack (must be even!!)
    int nFrames = 20;           // frames per stack
    double exposureTime = 0.01; // This can be configurable in the GUI
    bool bidirection = bidir;   // Bi-directional capturing mode

    /* Generating waveforms and pulses                            */
    /* Refer to help menu in Imagine                              */

    ////////  Generating subsequences of waveforms and pulses  ////////

    ///  Piezo waveform  ///
    QJsonArray piezo1_001;
    //  paramters specified by user
    int piezoDelaySamples = 100;
    int freqInSampleNum = 10000;
    int piezoWaitSamples = 800;
    double piezoStartPos = PIEZO_10V_UINT16_VALUE/4.; // 100
    double piezoStopPos = PIEZO_10V_UINT16_VALUE; // 400

    /// initial delay ///
    //  paramters specified by user
    int initDelaySamples = 500;
    int idleTime = 200;
    // generating delay waveform
    QVariantList buf1;
    genConstantWave(buf1, piezoStartPos, initDelaySamples);
    QJsonArray initDelay_wav = QJsonArray::fromVariantList(buf1);
    // generating delay pulse
    genConstantWave(buf1, 0, initDelaySamples);
    QJsonArray initDelay_pulse = QJsonArray::fromVariantList(buf1);

    // generating piezo waveform
    // This is for sine wave
    QVariantList buf2, buf3;
    int minShutterCtrlSamples = genCosineWave(buf1, buf2, buf3, piezoStartPos, piezoStopPos,
                                                piezoDelaySamples, freqInSampleNum, nFrames);
    int exposureSamples = static_cast<int>(exposureTime*static_cast<double>(sampleRate));  // (0.008~0.012sec)*(sampling rate) is optimal
    if (exposureSamples > minShutterCtrlSamples - 50 * (sampleRate / 10000))
        return 1;
    int eachShutterOnSamples = exposureSamples;
    QVariantList buf11, buf22, buf33;
    // buf11: bi-directional wave
    genBiDirection(buf11, buf1, piezoDelaySamples, getRunLengthSize(buf1) - piezoDelaySamples, idleTime, eachShutterOnSamples, true);
    QJsonArray piezo_cos = QJsonArray::fromVariantList(buf11);
    // buf22: bi-directional pulse
    genBiDirection(buf22, buf2, piezoDelaySamples, getRunLengthSize(buf2) - piezoDelaySamples, idleTime, eachShutterOnSamples, true);
    QJsonArray pulse1_cos = QJsonArray::fromVariantList(buf22);
    // buf22: bi-directional pulse
    genBiDirection(buf33, buf3, piezoDelaySamples, getRunLengthSize(buf3) - piezoDelaySamples, idleTime, eachShutterOnSamples, true);
    QJsonArray pulse2_cos = QJsonArray::fromVariantList(buf33);

    /// stimulus valve pulse ///
    QJsonArray valve_on, valve_off;
    // paramters specified by user
    int per2StacksSamples = getRunLengthSize(buf11);
    int valveOffSamples = 0;
    int valveOnSamples = per2StacksSamples - idleTime;
    int valveControlMarginSamples = 50;
    // generating stimulus valve pulse
    QVariantList buf4;
    genConstantWave(buf4, 0, per2StacksSamples);
    valve_off = QJsonArray::fromVariantList(buf4);
    genPulse(valve_on, 0, valveOnSamples, idleTime, per2StacksSamples,
        valveControlMarginSamples, valveOnSamples, valveOnSamples);

    // these values are calculated from user specified parameters
    piezoavg = piezoStartPos;
    ttl2 = false;

    ///  Register subsequences to waveform list  ///
    QJsonObject wavelist;
    wavelist["delay_wave"] = initDelay_wav;
    wavelist["delay_pulse"] = initDelay_pulse;
    wavelist["piezo_cos"] = piezo_cos;
    wavelist["camera1_001"] = pulse1_cos;
    wavelist["laser1_001"] = pulse2_cos;
    wavelist["valve_on"] = valve_on;
    wavelist["valve_off"] = valve_off;

    ////////  Generating full sequences of waveforms and pulses  ////////
    QJsonObject analog, digital;
    QJsonArray piezo1Seq, camera1Seq, laser1Seq, stimulus1Seq, stimulus2Seq;
    QJsonObject piezo1, camera1, laser1, stimulus1, stimulus2;

    for (int i = 0; i < channelSignalList.size(); i++) {
        QString port = channelSignalList[i][0];
        QString sig = channelSignalList[i][1];

        if (port == QString(STR_AOHEADER).append("0")) { // AO0
            piezo1Seq = { 1, "delay_wave", nStacks/2, "piezo_cos" };
            piezo1[STR_Channel] = port;
            piezo1[STR_Seq] = piezo1Seq;
            analog[sig] = piezo1; // secured port for piezo
        }

        if (port == QString(STR_P0HEADER).append("5")) { // P0.5
            camera1Seq = { 1, "delay_pulse", nStacks/2, "camera1_001" };
            camera1[STR_Channel] = port;
            camera1[STR_Seq] = camera1Seq;
            digital[sig] = camera1; // secured port for camera1
        }

        if (port == QString(STR_P0HEADER).append("4")) { // P0.4
            laser1Seq = { 1, "delay_pulse", nStacks/2, "laser1_001" };
            laser1[STR_Channel] = port;
            laser1[STR_Seq] = laser1Seq;
            digital[sig] = laser1; // secured port for laser
        }

        if (port == QString(STR_P0HEADER).append("0")) { // P0.0
            stimulus1Seq.append(1);
            stimulus1Seq.append("delay_pulse");
            stimulus1Seq.append(1); // stack 1, 2
            stimulus1Seq.append("valve_off");
            for (int i = 1; i < nStacks/2; i++) { // stack 3 ~ nStack
                int j = i % 2;
                if (j == 0) {
                    stimulus1Seq.append(1);
                    stimulus1Seq.append("valve_on");
                }
                else {
                    stimulus1Seq.append(1);
                    stimulus1Seq.append("valve_off");
                }
            }
            stimulus1[STR_Channel] = port;
            stimulus1[STR_Seq] = stimulus1Seq;
            if (sig == "")  // custom port
                digital["stimulus1"] = stimulus1;
            else
                qDebug("The port is already used as sequred signal\n");
        }

        if (port == QString(STR_P0HEADER).append("1")) { // P0.1
            stimulus2Seq.append(1);
            stimulus2Seq.append("delay_pulse");
            stimulus2Seq.append(1);
            stimulus2Seq.append("valve_off");
            for (int i = 1; i < nStacks/2; i++) { // stack 3 ~ nStack
                int j = i % 2;
                if (j == 0) {
                    stimulus2Seq.append(1);
                    stimulus2Seq.append("valve_off");
                }
                else{
                    stimulus2Seq.append(1);
                    stimulus2Seq.append("valve_on");
                }
            }
            stimulus2[STR_Channel] = port;
            stimulus2[STR_Seq] = stimulus2Seq;
            if (sig == "")  // custom port
                digital["stimulus2"] = stimulus2;
            else
                qDebug("The port is already used as sequred signal\n");
        }
    }
    totalSamples = initDelaySamples + nStacks/2 * per2StacksSamples;

    // packing all
    QJsonObject alldata;
    QJsonObject metadata;

    metadata[STR_SampleRate] = sampleRate;
    metadata[STR_TotalSamples] = totalSamples;
    metadata[STR_Exposure] = exposureTime;
    metadata[STR_BiDir] = bidirection;
    metadata[STR_Stacks] = nStacks;
    metadata[STR_Frames] = nFrames;
    metadata[STR_Rig] = rig;

    alldata[STR_Metadata] = metadata;
    alldata[STR_Analog] = analog;
    alldata[STR_Digital] = digital;
    alldata[STR_WaveList] = wavelist;
    alldata[STR_Version] = CF_VERSION;

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

/***** AiWaveform class ******************************************************/
#define MIN_VALUE (-PIEZO_10V_UINT16_VALUE + 8192.) // (-10+2.5V) = -7.5V
AiWaveform::AiWaveform(QString filename, int num)
{
    errorMsg.clear();
    numAiCurveData = num;
    if (!numAiCurveData)
        return;
    readFile(filename);
}

AiWaveform::~AiWaveform()
{
    fileClose();
}

bool AiWaveform::fileClose()
{
    if (file.isOpen())
        file.close();
    totalSampleNum = 0;
    return true;
}

bool AiWaveform::readFile(QString filename)
{
    // read ai file
    file.setFileName(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg.append(QString(tr("Cannot read file %1:\n%2.")
            .arg(filename)
            .arg(file.errorString())));
        return false;
    }
    else {
        stream.setDevice(&file);
        stream.setByteOrder(QDataStream::LittleEndian);//BigEndian, LittleEndian
        QFileInfo fi(filename);
        int perSampleSize = numAiCurveData * 2; // number of ai channel * 2 bytes
        totalSampleNum = fi.size() / perSampleSize;
        if (totalSampleNum*numAiCurveData < MAX_AI_DI_SAMPLE_NUM) {
            if (!readStreamToWaveforms())
                return false;
            isReadFromFile = false;
            file.close();
        }
        else {
            isReadFromFile = true;
            maxy = 1000;
            miny = -100;
        }
    }
    return true;
}

double AiWaveform::convertRawToVoltage(short us)
{
    int wus; // warp arounded value
    if (us < (int)MIN_VALUE) // if 'us' is too negative value then probably it was positive
                             // value overflowed the maximum value so we wrap around
        wus = (int)us + 2 * (PIEZO_10V_UINT16_VALUE + 1);
    else
        wus = (int)us;
    return static_cast<double>(wus)*1000. / PIEZO_10V_UINT16_VALUE; // int value to double(mV)
}

bool AiWaveform::readStreamToWaveforms()
{
    for (int i = 0; i < numAiCurveData; i++) {
        aiData.push_back(QVector<int>()); // add empty vectors to rows
    }
    for (int i = 0; i < totalSampleNum; i++)
    {
        short us;
        int wus; // warp arounded value
        for (int j = 0; j < numAiCurveData; j++) {
            stream >> us;
            double y = convertRawToVoltage(us);
            if (y > maxy) maxy = y;
            if (y < miny) miny = y;
            aiData[j].push_back(y);
        }
    }
    return true;
}

bool AiWaveform::readWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate)
{
    if (!numAiCurveData)
        return true;
    if (isReadFromFile) {
        if (end >= totalSampleNum)
            return false;
        file.seek(0);
        stream.skipRawData(sizeof(short)*(begin*numAiCurveData + ctrlIdx));
        for (SampleIdx i = begin; i <= end; i += downSampleRate) {
            short us;
            stream >> us;
            double y = convertRawToVoltage(us);
            dest.push_back(y);
            if (i >= end - downSampleRate + 1)
                return true;
            else
                stream.skipRawData(sizeof(short)*(downSampleRate*numAiCurveData-1));
        }
    }
    else {
        if (end >= aiData[ctrlIdx].size())
            return false;
        for (SampleIdx i = begin; i <= end; i += downSampleRate) {
            dest.push_back(aiData[ctrlIdx][i]);
//            if (i >= end - downSampleRate + 1)
//                return true;
        }
        return true;
    }
    return false;
}

bool AiWaveform::getSampleValue(int ctrlIdx, SampleIdx idx, int &value)
{
    if (!numAiCurveData)
        return true;
    if (isReadFromFile) {
        if (idx >= totalSampleNum)
            return false;
        file.seek(0);
        stream.skipRawData(sizeof(short)*(idx*numAiCurveData + ctrlIdx));
        short us;
        stream >> us;
        value = us; // short to int
    }
    else {
        if (idx >= aiData[ctrlIdx].size())
            return false;
        else {
            value = aiData[ctrlIdx][idx];
            return true;
        }
    }
}

int AiWaveform::getMaxyValue()
{
    return maxy;
}

int AiWaveform::getMinyValue()
{
    return miny;
}

/***** DiWaveform class ******************************************************/
DiWaveform::DiWaveform(QString filename, QVector<int> diChNumList)
{
    errorMsg.clear();
    numDiCurveData = diChNumList.size();
    if (!numDiCurveData)
        return;
    chNumList = diChNumList;
    readFile(filename);
}

DiWaveform::~DiWaveform()
{
    fileClose();
}

bool DiWaveform::fileClose()
{
    if (file.isOpen())
        file.close();
    totalSampleNum = 0;
    return true;
}

bool DiWaveform::readFile(QString filename)
{
    // read di file
    file.setFileName(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg.append(QString(tr("Cannot read file %1:\n%2.")
            .arg(filename)
            .arg(file.errorString())));
        return false;
    }
    else {
        stream.setDevice(&file);
        QFileInfo fi(filename);
        int perSampleSize = 1;
        totalSampleNum = fi.size() / perSampleSize;
        if (totalSampleNum < MAX_AI_DI_SAMPLE_NUM) {
            if (!readStreamToWaveforms())
                return false;
            isCRLEncoded = false;
        }
        else {
            if (!readStreamToCRLWaveforms())
                return false;
            isCRLEncoded = true;
        }
        file.close();
    }
    return true;
}

bool DiWaveform::readStreamToWaveforms()
{
    for (int i = 0; i < numDiCurveData; i++) {
        diData.push_back(QVector<int>()); // add empty vectors to rows
    }
    for (SampleIdx i = 0; i < totalSampleNum; i++)
    {
        uInt8 us;
        stream >> us;
        for (int j = 0; j < numDiCurveData; j++) {
            if (us &(1 << chNumList[j]))
                diData[j].push_back(1);
            else
                diData[j].push_back(0);
        }
    }
    return true;
}

bool DiWaveform::readStreamToCRLWaveforms()
{
    uInt8 us;
    QVector<SampleIdx> runLength(numDiCurveData,1);
    QVector<int> lastValue(numDiCurveData,0);
    int currentValue;

    for (int i = 0; i < numDiCurveData; i++) {
        diCRLData.push_back(QVector<SampleIdx>()); // add empty vectors to rows
        diData.push_back(QVector<int>()); // add empty vectors to rows
    }

    stream >> us;
    for (int j = 0; j < numDiCurveData; j++) {
        currentValue = (us &(1 << chNumList[j]))? 1 : 0;
        lastValue[j] = currentValue;
    }
    for (SampleIdx i = 0; i < totalSampleNum-1; i++)
    {
        stream >> us;
        for (int j = 0; j < numDiCurveData; j++) {
            currentValue = (us &(1 << chNumList[j])) ? 1 : 0;
            if (lastValue[j] == currentValue) {
                runLength[j]++;
            }
            else {
                diCRLData[j].push_back(runLength[j]);
                diData[j].push_back(lastValue[j]);
                lastValue[j] = currentValue;
                runLength[j]++;
            }
        }
    }
    for (int j = 0; j < numDiCurveData; j++) {
        diCRLData[j].push_back(runLength[j]);
        diData[j].push_back(lastValue[j]);
    }
    return true;
}

bool DiWaveform::readWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate)
{
    if (!numDiCurveData)
        return true;
    if (end >= totalSampleNum)
        return false;
    if (isCRLEncoded) {
        if (readCRLWaveform(dest, ctrlIdx, begin, end, downSampleRate))
            return true;
        else
            return false;
    }
    else {
        for (SampleIdx i = begin; i <= end; i += downSampleRate) {
            dest.push_back(diData[ctrlIdx][i]);
//            if (i >= end - downSampleRate + 1)
//                return true;
        }
        return true;
    }
    return false;
}

bool DiWaveform::readCRLWaveform(QVector<int> &dest, int ctrlIdx, SampleIdx begin, SampleIdx end, int downSampleRate)
{
    if (!numDiCurveData)
        return true;
    if (end >= totalSampleNum)
        return false;
    SampleIdx i = begin;
    SampleIdx j;
    for(j = 0; j < diCRLData[ctrlIdx].size(); j++) {
        while (i < diCRLData[ctrlIdx][j]) {
            dest.push_back(diData[ctrlIdx][j]);
            i += downSampleRate;
            if (i > end)
                return true;
        }
    }
    return false;
}

bool DiWaveform::getSampleValue(int ctrlIdx, SampleIdx idx, int &value)
{
    if (!numDiCurveData)
        return true;
    if (isCRLEncoded) {
        if (idx >= totalSampleNum)
            return false;
        SampleIdx j = 0;
        while (idx < diCRLData[ctrlIdx][j++]);
        value = diData[ctrlIdx][j--];
        return true;
    }
    else {
        if (idx >= diData[ctrlIdx].size())
            return false;
        value = diData[ctrlIdx][idx];
    }
    return true;
}
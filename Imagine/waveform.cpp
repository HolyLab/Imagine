#include "waveform.h"
#include <iostream>
#include <QDebug>

using namespace std;

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

ControlWaveform::ControlWaveform(QString rig)
{
    this->rig = rig;

    // Count all control channels
    if (rig != "dummy") {
        char deviceName[100];
        char names[2000];
        strcpy(deviceName, "Dev2");
        DAQmxGetDevAIPhysicalChans(deviceName, names, 2000);
        QString nameAIChannel = QString(names);
        DAQmxGetDevAOPhysicalChans(deviceName, names, 2000);
        QString nameAOChannel = QString(names);
        DAQmxGetDevDOLines(deviceName, names, 2000);
        QString nameDOChannel = QString(names);
        numAOChannel = nameAOChannel.count("ao");
        numAIChannel = nameAIChannel.count("ai");
        numP0Channel = nameDOChannel.count("port0");
    }
    else {
        numAOChannel = 4;
        numAIChannel = 32;
        numP0Channel = 32;
    }

    // Setting secured channel name
    QVector<QVector<QString>> *secured;
    if ((rig == "ocpi-1")||(rig == "ocpi-lsk"))
        secured = &ocpi1Secured;
    else // including "ocpi-2" and "dummy" ocpi
        secured = &ocpi2Secured;
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

CFErrorCode ControlWaveform::lookUpWave(QString wn, QVector <QString> &wavName,
        QJsonObject wavelist, int &waveIdx)
{
    int i;

    for (i = 0; i < wavName.size(); i++)
    {
        if (wn.compare(wavName[i]) == 0) {
            waveIdx = i;
            return NO_CF_ERROR;
        }
    }

    // If there is no matched string, find this wave from wavelist JSON object
    // and add this new array to waveList vector 
    QJsonArray jsonWave = wavelist[wn].toArray();
    QVector <int> *wav = new QVector<int>;
    int sum = 0;
    if (jsonWave.size() == 0) {
        errorMsg.append(QString("Can not find %1 waveform\n").arg(wn));
        return ERR_WAVEFORM_MISSING;
    }
    for (int j = 0; j < jsonWave.size(); j+=2) { 
        wav->push_back(sum);    // change run length to sample index number and save
        wav->push_back(jsonWave[j+1].toInt());
        sum += jsonWave[j].toInt();
    }
    wav->push_back(sum); // save total sample number to the last element
    waveList.push_back(wav);
    wavName.push_back(wn);
    waveIdx = i;
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::loadJsonDocument(QJsonDocument &loadDoc)
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
    QString version = alldata[STR_Version].toString();
    if (version != CF_VERSION)
        return ERR_INVALID_VERSION;

    metadata = alldata[STR_Metadata].toObject();
    QString rig = metadata[STR_Rig].toString();
    if (rig != this->rig)
        return ERR_RIG_MISMATCHED;
    sampleRate = metadata[STR_SampleRate].toInt();
    totalSampleNum = metadata[STR_TotalSamples].toInt();
    exposureTime = metadata[STR_Exposure].toDouble();
    nStacks = metadata[STR_Stacks].toInt();
    nFrames = metadata[STR_Frames].toInt();
    analog = alldata[STR_Analog].toObject();
    digital = alldata[STR_Digital].toObject();
    wavelist = alldata[STR_WaveList].toObject();
    bidirection = metadata[STR_BiDir].toBool();
    QStringList analogKeys = analog.keys();
    QStringList digitalKeys = digital.keys();
    QJsonObject obj;
    CFErrorCode err = NO_CF_ERROR;

    errorMsg.clear();
    for (int i = 0; i < analogKeys.size(); i++) {
        obj = analog[analogKeys[i]].toObject();
        QString port = obj[STR_Channel].toString();
        if (port.left(2) == STR_AOHEADER) {
            int j = port.mid(2).toInt();
            QString reservedName = channelSignalList[j][1];
            if ((reservedName != "")&&(reservedName != analogKeys[i])) {
                errorMsg.append(QString("%1 channel is already reserved as a '%2' signal\n")
                    .arg(channelSignalList[j][0])
                    .arg(channelSignalList[j][1]));
                err |= ERR_INVALID_PORT_ASSIGNMENT;
            }
            else
                channelSignalList[j][1] = analogKeys[i];
        }
        else if (port.left(2) == STR_AIHEADER) {
            int j = port.mid(2).toInt();
            QString reservedName = channelSignalList[j + getAIBegin()][1];
            if ((reservedName != "") && (reservedName != analogKeys[i])) {
                errorMsg.append(QString("%1 channel is already reserved as a '%2' signal\n")
                    .arg(channelSignalList[j + getAIBegin()][0])
                    .arg(channelSignalList[j + getAIBegin()][1]));
                err |= ERR_INVALID_PORT_ASSIGNMENT;
            }
            else
                channelSignalList[j + getAIBegin()][1] = analogKeys[i];
        }
        else {
            errorMsg.append(QString("%1 channel can not be used as an 'analog waveform'\n")
                .arg(port));
            err |= ERR_INVALID_PORT_ASSIGNMENT;
        }
    }

    for (int i = 0; i < digitalKeys.size(); i++) {
        obj = digital[digitalKeys[i]].toObject();
        QString port = obj[STR_Channel].toString();
        if (port.left(3) == STR_P0HEADER) {
            int j = port.mid(3).toInt();
            QString reservedName = channelSignalList[j + getP0Begin()][1];
            if ((reservedName != "") && (reservedName != digitalKeys[i])) {
                errorMsg.append(QString("%1 channel is already reserved as a '%2' signal\n")
                    .arg(channelSignalList[j + getP0Begin()][0])
                    .arg(channelSignalList[j + getP0Begin()][1]));
                err |= ERR_INVALID_PORT_ASSIGNMENT;
            }
            else
                channelSignalList[j + getP0Begin()][1] = digitalKeys[i];
        }
        else {
            errorMsg.append(QString("%1 channel can not be used as a 'digital pulse'\n")
                .arg(port));
            err |= ERR_INVALID_PORT_ASSIGNMENT;
        }
    }

    // The order of this should be match with ControlSignal
    QVector<QString> wavName;
    QJsonArray control;
    for (int i = 0; i < channelSignalList.size(); i++) {
//        qDebug() << "[" + conName[i] + "]\n";
        QVector<int> *cwf = new QVector<int>;
        if (i < getP0Begin()) {
            control = (analog[channelSignalList[i][1]].toObject())[STR_Seq].toArray();
        }
        else {
            control = (digital[channelSignalList[i][1]].toObject())[STR_Seq].toArray();
        }
        if (!control.isEmpty()) {
            for (int i = 0; i < control.size(); i += 2) {
                int waveIdx = 0;
                int repeat = control[i].toInt();
                QString wn = control[i + 1].toString();
                err |= lookUpWave(wn, wavName, wavelist, waveIdx);
                cwf->push_back(repeat);
                cwf->push_back(waveIdx);
//                qDebug() << wn + "(" << waveIdx << ") " << repeat << "\n";
            }
        }
        controlList.push_back(cwf);
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
    if (waveList.isEmpty())
        return 0;
    int  sampleNum = waveList[waveIdx]->last();
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
        sum += repeat*waveList[waveIdx]->last();
    }
    return sum;
}

QString ControlWaveform::getSignalName(int ctrlIdx)
{
    return channelSignalList[ctrlIdx][1];
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

int ControlWaveform::getNumChannel(void)
{
    return numChannel;
}

bool ControlWaveform::getCtrlSampleValue(int ctrlIdx, int idx, int &value)
{
    if (controlList.isEmpty())
        return false;
    int controlIdx, repeat, waveIdx;
    int sampleIdx = 0, prevSampleIdx, sampleIdxInWave;
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

bool ControlWaveform::readControlWaveform(QVector<int> &dest, int ctrlIdx,
    int begin, int end, int downSampleRate)
{
    if (controlList.isEmpty())
        return false;

    int controlIdx = 0, repeat, waveIdx, repeatLeft;
    int sampleIdx = begin, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;

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

bool ControlWaveform::isPiezoWaveEmpty(void)
{
    bool empty = true;
    if((rig=="ocpi-1")|| (rig == "ocpi-lsk")) {
        empty &= isEmpty(STR_piezo);
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
        empty &= isEmpty(STR_camera);
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
    if ((rig == "ocpi-1") || (rig == "ocpi-lsk")) {
        empty &= isEmpty(STR_laser);
    }
    else {
        /*
        empty &= isEmpty(STR_488nm_laser);
        empty &= isEmpty(STR_561nm_laser);
        empty &= isEmpty(STR_405nm_laser);
        empty &= isEmpty(STR_445nm_laser);
        empty &= isEmpty(STR_514nm_laser);
        empty |= isEmpty(STR_all_lasers);
        */
        empty &= isEmpty(STR_all_lasers); // For the time being, just check this channel
    }
    return empty;
}

#define  SPEED_CHECK_INTERVAL 100
CFErrorCode ControlWaveform::positionerSpeedCheck(int maxSpeed, int ctrlIdx, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    double distance, time, speed;
    int controlIdx, repeat, waveIdx;
    int sampleIdx, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;
    QVector<bool> usedWaveIdx(waveList.size(),false);
    int wave0, wave1;
    int margin_interval, margin_instant;

    time = static_cast<double>(SPEED_CHECK_INTERVAL) / static_cast<double>(sampleRate); // sec

    for (controlIdx = 0; controlIdx < controlList[ctrlIdx]->size(); controlIdx += 2) {
        repeat = controlList[ctrlIdx]->at(controlIdx);
        waveIdx = controlList[ctrlIdx]->at(controlIdx + 1);
        int waveLength = getWaveSampleNum(waveIdx);
        if (waveLength < SPEED_CHECK_INTERVAL)
            return ERR_SHORT_WAVEFORM;
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
                getWaveSampleValue(waveIdx, i, wave0);
                // 1.1 Speed check : But, this cannot filter out periodic signal which has same period as 'interval'
                if (i < getWaveSampleNum(waveIdx) - margin_interval) { // If margin == interval, no wraparound check
                    getWaveSampleValue(waveIdx, (i + SPEED_CHECK_INTERVAL) % getWaveSampleNum(waveIdx), wave1);
                    distance = abs(wave1 - wave0); // original one was... abs(wave1 - wave0) - 1; Why '-1"?
                    if (distance > 1) {
                        speed = distance / time; // piezostep/sec
                        if (speed > maxSpeed) {
                            return ERR_PIEZO_SPEED_FAST;
                        }
                    }
                }
                // 1.2 Instantaneous change check : This will improve a defect of 1.1
                if (i < getWaveSampleNum(waveIdx) - margin_instant) {
                    getWaveSampleValue(waveIdx, (i + 1) % getWaveSampleNum(waveIdx), wave1);
                    if (abs(wave1 - wave0) >= 2)
                        return ERR_PIEZO_INSTANT_CHANGE;
                }
            }
            usedWaveIdx[waveIdx] = true;
        }
        // 2. If control signal is compoused of several waveforms, we need to check connection points.
        if (controlIdx >= 2) {
            // 2.1 Speed check
            int oldWaveIdx = controlList[ctrlIdx]->at(controlIdx - 1);
            for (int i = getWaveSampleNum(oldWaveIdx) - SPEED_CHECK_INTERVAL, j = 0; i < getWaveSampleNum(oldWaveIdx); i++, j++) {
                getWaveSampleValue(oldWaveIdx, i, wave0);
                getWaveSampleValue(waveIdx, j, wave1);
                distance = abs(wave1 - wave0); // original one was... abs(wave1 - wave0) - 1; Why '-1"?
                if (distance > 1) {
                    speed = distance / time; // piezostep/sec
                    if (speed > maxSpeed) {
                        return ERR_PIEZO_SPEED_FAST;
                    }
                }
            }
            // 2.2 Instantaneous change check
            getWaveSampleValue(oldWaveIdx, getWaveSampleNum(oldWaveIdx)-1, wave0);
            getWaveSampleValue(waveIdx, 0, wave1);
            if (abs(wave1 - wave0) >= 2)
                return ERR_PIEZO_INSTANT_CHANGE;
        }
    }
    return NO_CF_ERROR;
}

CFErrorCode ControlWaveform::laserSpeedCheck(double maxFreq, int ctrlIdx, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    int waveIdx, controlIdx;
    double freq, durationInSamples;
    QVector<bool> usedWaveIdx(waveList.size(), false);

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

CFErrorCode ControlWaveform::waveformValidityCheck(int maxPiezoSpeed, int maxLaserFreq)
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
                CFErrorCode err = positionerSpeedCheck(maxPiezoSpeed, i, sampleNum);
                if (err & (ERR_PIEZO_SPEED_FAST | ERR_PIEZO_INSTANT_CHANGE)) {
                    errorMsg.append(QString("%1 control is too fast\n").arg(channelSignalList[i][1]));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("%1 sample # is different from total sample #\n").arg(channelSignalList[i][1]));
                }
                errorCode |= err;
            }
            // laser speed check
            else if ((channelSignalList[i][1].right(5) == STR_laser)|| (channelSignalList[i][1] == STR_all_lasers)) {
                CFErrorCode err = laserSpeedCheck(maxLaserFreq, i, sampleNum); // OCPI-1 should be less then 25Hz
                if (err & ERR_LASER_SPEED_FAST) {
                    errorMsg.append(QString("%1 control is too fast\n").arg(channelSignalList[i][1]));
                }
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    err |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("%1 sample # is different from total sample #\n").arg(channelSignalList[i][1]));
                }
                errorCode |= err;
            }
            // For other signals, just check sample numbers.
            else {
                sampleNum = getCtrlSampleNum(i);
                if ((sampleNum != 0) && (sampleNum != totalSampleNum)) {
                    errorCode |= ERR_SAMPLE_NUM_MISMATCHED;
                    errorMsg.append(QString("%1 sample # is different from total sample #\n").arg(channelSignalList[i][1]));
                }
            }
        }
    }
    return errorCode;
}

QString ControlWaveform::getErrorMsg(void)
{
    return errorMsg;
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

int ControlWaveform::genControlFileJSON(void)
{
    enum SaveFormat {
        Json, Binary
    };
    qint16 piezo;
    bool shutter, laser, ttl2;
    double piezom1, piezo0, piezop1, piezoavg = 0.;

    /* For metadata */
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
    QJsonArray piezo1_001;
    //  paramters specified by user
    int piezoDelaySamples = 100;
    int piezoRisingSamples = 6000;
    int piezoFallingSamples = 4000;
    int piezoWaitSamples = 800;
    double piezoStartPos = 100.;
    double piezoStopPos = 800.;

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

    // generating piezo waveform
    int perStackSamples = piezoRisingSamples + piezoFallingSamples + piezoWaitSamples;
    genWaveform(piezo1_001, piezoStartPos, piezoStopPos, piezoDelaySamples,
        piezoRisingSamples, piezoFallingSamples, perStackSamples);

    /// camera shutter pulse ///
    QJsonArray camera1_001;
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
    QJsonArray laser1_001;
    // paramters specified by user
    int laserControlMarginSamples = 400; // laser control begin after piezo start and stop before piezo stop
    // generating laser shutter pulse
    stakShutterCtrlSamples = piezoRisingSamples - 2 * laserControlMarginSamples; // full stack period sample number
    int laserShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    genPulse(laser1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, laserShutterCtrlSamples, laserShutterCtrlSamples);

    /// stimulus valve pulse ///
    QJsonArray valve_on, valve_off;
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
    QJsonObject wavelist;
    wavelist["delay_wave"] = initDelay_wav;
    wavelist["delay_pulse"] = initDelay_pulse;
    wavelist["positioner1_001"] = piezo1_001;
    wavelist["camera1_001"] = camera1_001;
    wavelist["laser1_001"] = laser1_001;
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
            piezo1Seq = { 1, "delay_wave", nStacks, "positioner1_001" };
            piezo1[STR_Channel] = port;
            piezo1[STR_Seq] = piezo1Seq;
            analog[sig] = piezo1; // secured port for piezo
        }

        if (port == QString(STR_P0HEADER).append("5")) { // P0.5
            camera1Seq = { 1, "delay_pulse", nStacks, "camera1_001" };
            camera1[STR_Channel] = port;
            camera1[STR_Seq] = camera1Seq;
            digital[sig] = camera1; // secured port for camera1
        }

        if (port == QString(STR_P0HEADER).append("4")) { // P0.4
            laser1Seq = { 1, "delay_pulse", nStacks, "laser1_001" };
            laser1[STR_Channel] = port;
            laser1[STR_Seq] = laser1Seq;
            digital[sig] = laser1; // secured port for laser
        }

        if (port == QString(STR_P0HEADER).append("0")) { // P0.0
            stimulus1Seq.append(1);
            stimulus1Seq.append("delay_pulse");
            stimulus1Seq.append(2); // stack 1, 2
            stimulus1Seq.append("valve_off");
            for (int i = 3; i <= nStacks; i++) { // stack 3 ~ nStack
                int j = i % 4;
                if (j == 0) {
                    stimulus1Seq.append(2);
                    stimulus1Seq.append("valve_on");
                }
                else if (j == 2) {
                    stimulus1Seq.append(2);
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
            stimulus2Seq.append(2);
            stimulus2Seq.append("valve_off");
            for (int i = 3; i <= nStacks; i++) { // stack 3 ~ nStack
                int j = i % 4;
                if (j == 0) {
                    stimulus2Seq.append(2);
                    stimulus2Seq.append("valve_off");
                }
                else if (j == 2) {
                    stimulus2Seq.append(2);
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
    totalSamples = initDelaySamples + nStacks * perStackSamples;

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

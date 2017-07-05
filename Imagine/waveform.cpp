#include "waveform.h"
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
void ControlWaveform::initControlWaveform(QString rig)
{
    // Count all control channels
    if (rig != "dummy") {
        /*
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
        */
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
    if (rig == "ocpi-lsk") {
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

int ControlWaveform::raw2Zpos(int raw)
{
    double vol = static_cast<double>(raw) / piezo10Vint16Value* maxPiezoPos;
    return static_cast<int>(vol + 0.5);
}

float64 ControlWaveform::raw2Voltage(int raw)
{
    double vol = static_cast<double>(raw) / piezo10Vint16Value * 10.; // *10: 0-10vol range for piezo
    return static_cast<float64>(vol);
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
        wav->push_back(jsonWave[j+1].toInt()); // This is a raw data format
        if (dataType == 1) {// For the piezo data type, we will keep another waveform data as a voltage
                            // format(max 10.0V) and position format(max maxPiezoPos) for later use.
                            // This is just for display
//            dwav->push_back(static_cast<float64>(sum));
            wav_p->push_back(raw2Zpos(jsonWave[j + 1].toInt()));
            wav_v->push_back(raw2Voltage(jsonWave[j + 1].toInt()));
        }
        sum += jsonWave[j].toInt();
    }
    wav->push_back(sum); // save total sample number to the last element
//    if (dataType == 1) {
//        dwav->push_back(static_cast<float64>(sum));
//    }
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

    // Parsing
    // 1. Metadata
    alldata = loadDoc.object();
    QString version = alldata[STR_Version].toString();
    if (version != CF_VERSION)
        return ERR_INVALID_VERSION;

    metadata = alldata[STR_Metadata].toObject();
    QString rig = metadata[STR_Rig].toString();
    if (this->rig == "dummy") { // dummy ocpi can read .json for any rig
        this->rig = rig;
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
        file.close();
        delete seTmp;
        initControlWaveform(rig);
    }
    else if (rig == this->rig) {
        maxPiezoPos = se->globalObject().property("maxposition").toNumber();
        maxPiezoSpeed = se->globalObject().property("maxspeed").toNumber();
        maxLaserFreq = se->globalObject().property("maxlaserfreq").toNumber();
        initControlWaveform(this->rig);
    }
    else
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
    QJsonArray control;

    // 2. Read signal names and position them accoding to their channel names
    // - Analog signals (Output/Input)
    errorMsg.clear();
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

// Read control signal data at index 'idx' to variable 'value'
template<class Typ>
bool ControlWaveform::getCtrlSampleValue(int ctrlIdx, int idx, Typ &value, PiezoDataType dataType)
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
    isValid = getWaveSampleValue(waveIdx, sampleIdxInWave, value, dataType);
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

int findValuePos(QVector<int> *wav, QVector<int> *wav_p, int begin, int end, int sampleIdx)
{
    int mid = (begin + end) / 2;
    int midIdx = mid * 2;
    int x0, x1;

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
            return wav_p->at(midIdx / 2);
        }
        else {
            x0 = begin;
            x1 = mid;
            return findValuePos(wav, wav_p, x0, x1, sampleIdx);
        }
    }
}

float64 findValueVol(QVector<int> *wav, QVector<float64> *wav_v, int begin, int end, int sampleIdx)
{
    int mid = (begin + end) / 2;
    int midIdx = mid * 2;
    int x0, x1;

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
            return wav_v->at(midIdx/2);
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
    if ((rig == "ocpi-1") || (rig == "ocpi-lsk")) {
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
CFErrorCode ControlWaveform::positionerSpeedCheck(int maxPos, int maxSpeed, int ctrlIdx, int &dataSize)
{
    dataSize = getCtrlSampleNum(ctrlIdx);
    if (dataSize == 0)
        return NO_CF_ERROR;
    double distance, time, speed;
    int controlIdx, repeat, waveIdx;
    int sampleIdx, startSampleIdx = 0, prevStartSampleIdx, sampleIdxInWave;
    QVector<bool> usedWaveIdx(waveList_Raw.size(),false);
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
                getWaveSampleValue(waveIdx, i, wave0, PDT_Z_POSITION);
                if ((wave0 < 0)||(wave0 > maxPos)) // Invalid position value
                    return ERR_PIEZO_VALUE_INVALID;
                // 1.1 Speed check : But, this cannot filter out periodic signal which has same period as 'interval'
                if (i < getWaveSampleNum(waveIdx) - margin_interval) { // If margin == interval, no wraparound check
                    getWaveSampleValue(waveIdx, (i + SPEED_CHECK_INTERVAL) % getWaveSampleNum(waveIdx), wave1, PDT_Z_POSITION);
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
                    getWaveSampleValue(waveIdx, (i + 1) % getWaveSampleNum(waveIdx), wave1, PDT_Z_POSITION);
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
                getWaveSampleValue(oldWaveIdx, i, wave0, PDT_Z_POSITION);
                getWaveSampleValue(waveIdx, j, wave1, PDT_Z_POSITION);
                distance = abs(wave1 - wave0); // original one was... abs(wave1 - wave0) - 1; Why '-1"?
                if (distance > 1) {
                    speed = distance / time; // piezostep/sec
                    if (speed > maxSpeed) {
                        return ERR_PIEZO_SPEED_FAST;
                    }
                }
            }
            // 2.2 Instantaneous change check
            getWaveSampleValue(oldWaveIdx, getWaveSampleNum(oldWaveIdx)-1, wave0, PDT_Z_POSITION);
            getWaveSampleValue(waveIdx, 0, wave1, PDT_Z_POSITION);
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
                CFErrorCode err = positionerSpeedCheck(maxPiezoPos, maxPiezoSpeed, i, sampleNum);
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

void genBiDirection(QVariantList &vlarray, QVariantList refArray, int nDelay, int nRising, int nIdleTime, int numOnSample)
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
    QVariant pre = 0, cur;
    for (int i = 0; i < buf2.size(); i++) {
        cur = buf2.at(i);
        if ((pre == 0) && (cur == 1)) {
            for (int j = 0; j < numOnSample; j++)
                buf2.replace(i + j, 1);
        }
        pre = cur;
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

int ControlWaveform::genControlFileJSON(void)
{
    enum SaveFormat {
        Json, Binary
    };
    enum WaveType {
        Triangle, Triangle_bidirection, Sinusoidal
    } wtype;
    bool bidircection;

    wtype = Triangle;

    if (wtype == Triangle) {
        bidircection = false;
        genTriangle(bidircection);
    }
    else if (wtype == Triangle_bidirection) {
        bidircection = true;
        genTriangle(bidircection);
    }
    if (wtype == Sinusoidal) {
        bidircection = true;
        genSinusoidal(bidircection);
    }
    return 0;
}

int ControlWaveform::genTriangle(bool bidir)
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
    long totalSamples = 0;      // this will be calculated later;
    int nStacks = 10;           // number of stack
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
    int piezoRisingSamples = 6000;
    int piezoFallingSamples = 4000;
    int piezoWaitSamples = 800;
    double piezoStartPos = PIEZO_10V_UINT16_VALUE / 4.;// 100.;
    double piezoStopPos = PIEZO_10V_UINT16_VALUE; // 400.;

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
    int eachShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    int exposureSamples = static_cast<int>(exposureTime*static_cast<double>(sampleRate));  // (0.008~0.012sec)*(sampling rate) is optimal
    if (exposureSamples > eachShutterCtrlSamples - 50 * (sampleRate / 10000))
        return 1;
    int eachShutterOnSamples = exposureSamples;
    genPulse(camera1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);

    /// laser shutter pulse ///
    QJsonArray laser1_001;
    // paramters specified by user
    int laserControlMarginSamples = 400; // laser control begin after piezo start and stop before piezo stop
                                         // generating laser shutter pulse
    stakShutterCtrlSamples = piezoRisingSamples - 2 * laserControlMarginSamples; // full stack period sample number
    eachShutterCtrlSamples = stakShutterCtrlSamples / nFrames;     // one frame sample number
    eachShutterOnSamples = eachShutterCtrlSamples; // all high (means just one pulse)
    genPulse(laser1_001, piezoDelaySamples, piezoRisingSamples, piezoFallingSamples, perStackSamples,
        shutterControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples);

    /// stimulus valve pulse ///
    QJsonArray valve_on, valve_off;
    // paramters specified by user
    int valveDelaySamples = 0; // no delay
    int valveOnSamples = piezoRisingSamples + 2 * piezoDelaySamples;
    int valveOffSamples = piezoFallingSamples - piezoDelaySamples;
    int valveControlMarginSamples = 50;
    eachShutterCtrlSamples = valveOnSamples; // just one pulse
    eachShutterOnSamples = valveOnSamples; // all high
    // generating stimulus valve pulse
    genConstantWave(buf1, 0, perStackSamples);
    valve_off = QJsonArray::fromVariantList(buf1); // valve_off
    genPulse(valve_on, valveDelaySamples, valveOnSamples, valveOffSamples, perStackSamples,
        valveControlMarginSamples, eachShutterCtrlSamples, eachShutterOnSamples); // valve_on

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
    QJsonObject piezo1Mon, piezo2Mon, camera1Mon, camera2Mon;
    QJsonObject laser1Mon, stimulus1Mon, stimulus2Mon;

    for (int i = 0; i < channelSignalList.size(); i++) {
        QString port = channelSignalList[i][0];
        QString sig = channelSignalList[i][1];
        if (port == QString(STR_AOHEADER).append("0")) { // AO0
            piezo1Seq = { 1, "delay_wave", nStacks, "positioner1_001" };
            piezo1[STR_Channel] = port;
            piezo1[STR_Seq] = piezo1Seq;
            analog[sig] = piezo1; // secured port for piezo
        }

        if (port == QString(STR_AIHEADER).append("0")) { // AI0
            piezo1Mon[STR_Channel] = port;
            analog[sig] = piezo1Mon; // secured port for piezo monitor
        }

        if ((rig == "ocpi-1")|| (rig == "ocpi-lsk")) {
            if (port == QString(STR_AIHEADER).append("1")) { // AI1
                piezo2Mon[STR_Channel] = port;
                analog["horizontal piezo monitor"] = piezo2Mon; // secured port for piezo monitor
            }
        }
        else {
            if (port == QString(STR_AIHEADER).append("1")) { // AI1
                piezo2Mon[STR_Channel] = port;
                analog[sig] = piezo2Mon; // secured port for piezo monitor
            }
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
            camera2InPort = QString(STR_P0HEADER).append("25"); // P0.25
            if (port == camera2InPort) {
                camera2Mon[STR_Channel] = port;
                digital["camera2 frame monitor"] = camera2Mon;
            }
            if (port == QString(STR_P0HEADER).append("26")) { // p0.26
                stimulus1Mon[STR_Channel] = port;
                digital["heart beat"] = stimulus1Mon;
            }
        }
        else {
            camera1InPort = QString(STR_P0HEADER).append("24"); // P0.24
            if (port == camera1InPort) {
                camera1Mon[STR_Channel] = port;
                digital[sig] = camera1Mon; // secured port for camera1 mon
            }
            camera2InPort = QString(STR_P0HEADER).append("25"); // P0.25
            if (port == camera2InPort) {
                camera2Mon[STR_Channel] = port;
                digital[sig] = camera2Mon; // secured port for camera2 mon
            }
            if (port == QString(STR_P0HEADER).append("26")) { // p0.26
                stimulus1Mon[STR_Channel] = port;
                digital["heart beat"] = stimulus1Mon;
            }
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
    long totalSamples = 0;      // this will be calculated later;
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
    genBiDirection(buf11, buf1, piezoDelaySamples, getRunLengthSize(buf1) - piezoDelaySamples, idleTime, eachShutterOnSamples);
    QJsonArray piezo_cos = QJsonArray::fromVariantList(buf11);
    // buf22: bi-directional pulse
    genBiDirection(buf22, buf2, piezoDelaySamples, getRunLengthSize(buf2) - piezoDelaySamples, idleTime, eachShutterOnSamples);
    QJsonArray pulse1_cos = QJsonArray::fromVariantList(buf22);
    // buf22: bi-directional pulse
    genBiDirection(buf33, buf3, piezoDelaySamples, getRunLengthSize(buf3) - piezoDelaySamples, idleTime, eachShutterOnSamples);
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
AiWaveform::AiWaveform(QByteArray &data, int num)
{
    numAiCurveData = num;
    QDataStream aiStream(data);
    aiStream.setByteOrder(QDataStream::LittleEndian);//BigEndian, LittleEndian
    int perSampleSize = numAiCurveData * 2; // number of ai channel * 2 bytes
    int perSignalSize = data.size() / perSampleSize;
    totalSampleNum = (perSignalSize > MAX_AI_DI_SAMPLE_NUM) ? MAX_AI_DI_SAMPLE_NUM : perSignalSize;
    for (int i = 0; i < numAiCurveData; i++) {
        aiData.push_back(QVector<int>()); // add empty vectors to rows
    }
    for (int i = 0; i < totalSampleNum; i++)
    {
        short us;
        int wus; // warp arounded value
        for (int j = 0; j < numAiCurveData; j++) {
            aiStream >> us;
            if (us < (int)MIN_VALUE) // if us is too negative value than wrap around
                wus = -(int)us;
            else
                wus = (int)us;
            double y = static_cast<double>(wus)*1000. / PIEZO_10V_UINT16_VALUE; // int value to double(mV)
            if (y > maxy) maxy = y;
            if (y < miny) miny = y;
            aiData[j].push_back(y);
        }
    }
}

bool AiWaveform::readWaveform(QVector<int> &dest, int ctrlIdx, int begin, int end, int downSampleRate)
{
    if (end >= aiData[ctrlIdx].size())
        return false;
    for (int i = begin, j = 0; i <= end; i += downSampleRate) {
        dest.push_back(aiData[ctrlIdx][i]);
        if (i >= end - downSampleRate + 1)
            return true;
    }
    return false;
}

bool AiWaveform::getSampleValue(int ctrlIdx, int idx, int &value)
{
    if (idx >= aiData[ctrlIdx].size())
        return false;
    else {

        value = aiData[ctrlIdx][idx];
        return true;
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
DiWaveform::DiWaveform(QByteArray &data, QVector<int> diChNumList)
{
    numDiCurveData = diChNumList.size();
    if (!numDiCurveData)
        return;
    QDataStream diStream(data);
    int perSampleSize = 1; // 1 bytes
    int perSignalSize = data.size() / perSampleSize;
    totalSampleNum = (perSignalSize > MAX_AI_DI_SAMPLE_NUM) ? MAX_AI_DI_SAMPLE_NUM : perSignalSize;
    for (int i = 0; i < numDiCurveData; i++) {
        diData.push_back(QVector<int>()); // add empty vectors to rows
    }
    for (int i = 0; i < totalSampleNum; i++)
    {
        uInt8 us;
        diStream >> us;
        for (int j = 0; j < numDiCurveData; j++) {
            if (us &(1 << diChNumList[j]))
                diData[j].push_back(1);
            else
                diData[j].push_back(0);
        }
    }
}

bool DiWaveform::readWaveform(QVector<int> &dest, int ctrlIdx, int begin, int end, int downSampleRate)
{
    if (end >= diData[ctrlIdx].size())
        return false;
    for (int i = begin, j = 0; i <= end; i += downSampleRate) {
        dest.push_back(diData[ctrlIdx][i]);
        if (i >= end - downSampleRate + 1)
            return true;
    }
    return false;
}

bool DiWaveform::getSampleValue(int ctrlIdx, int idx, int &value)
{
    if (idx >= diData[ctrlIdx].size())
        return false;
    else {

        value = diData[ctrlIdx][idx];
        return true;
    }
}
#include "volpiezo.hpp"
#include "imagine.h"

#include <Windows.h>
#define ErrChk(func) if((error=(func))<0) goto Error; else

DAQ_CALLBACK_FP NiDaqDo::nSampleCallback = nullptr;
DAQ_CALLBACK_FP NiDaqAo::nSampleCallback = nullptr;
void* NiDaqDo::instance = nullptr;
void* NiDaqAo::instance = nullptr;

struct WaveData;

double VolPiezo::zpos2voltage(double um)
{
    return um / this->maxPos() * 10; // *10: 0-10vol range for piezo
}


bool VolPiezo::curPos(double* pos)
{
    NiDaqAiReadOne ai(ainame, 0); //channel 0: piezo
    float64 reading;
    ai.readOne(reading); //TODO: check error
//    double vol = ai.toPhyUnit(reading);
	double vol = reading;
    *pos = vol / 10 * this->maxPos();

    return true;
}

bool VolPiezo::moveTo(double to)
{
	double cur_pos = 0;
	double duration;
	curPos(&cur_pos);
	duration = (abs(to - cur_pos) / maxSpeed()) * 1e6;
	vector<Movement* > orig_movements = movements;
	vector<Movement* > empty_movements;
	movements = empty_movements;  //a hack, shouldn't have to hide prepared movements
	/*
    double vol = zpos2voltage(to);
    if (aoOnce == NULL){
        cleanup();
        int aoChannelForPiezo = 0; //TODO: put this configurable
        aoOnce = new NiDaqAoWriteOne(aoname, aoChannelForPiezo);
        Sleep(0.5 * 1000); //sleep 0.5s 
    }
    uInt16 sampleValue = aoOnce->toDigUnit(vol);
    //temp commented: 
    try{
        assert(aoOnce->writeOne(sampleValue));
    }
    catch (...){
        return false;
    }
	*/
	addMovement(max(cur_pos,0), to, duration, -1);
	prepareCmd(false);
	runCmd();
	Sleep(200); //otherwise it seems to be cleared before nidaq can use it.
	clearCmd();
	cleanup();
	movements = orig_movements;
	prepareCmd(false); //prepare again, leaving the movement command list as we found it.
	
    return true;
}//moveTo(),


bool VolPiezo::addMovement(double from, double to, double duration, int trigger)
{
    bool result = Positioner::addMovement(from, to, duration, trigger);
    if (!result) return result;

    if (movements.size() == 1){
        Movement& m = *movements[0];
        //m.duration += 0.06; //why are we doing this?
    }
    return result;
}//addMovement(),


bool VolPiezo::prepareCmd(bool useTrigger)
{
    //prepare for AO:
    if (aoOnce){
        delete aoOnce;
        aoOnce = nullptr;
    }

    int aoChannelForPiezo = 0; //TODO: put this configurable
    vector<int> aoChannels;
    aoChannels.push_back(aoChannelForPiezo);
    int aoChannelForTrigger = 1;
    aoChannels.push_back(aoChannelForTrigger);

    if (ao) cleanup();
    ao = new NiDaqAo(aoname, aoChannels);

    double totalTime = 0;
    for (unsigned idx = 0; idx < movements.size(); ++idx) totalTime += movements[idx]->duration / 1e6; //microsec to sec

    ao->cfgTiming(scanRateAo, int(scanRateAo*totalTime));

    if (ao->isError()) {
        cleanup();
        lastErrorMsg = "prepareCmd: failed to configure timing";
        return false;
    }
    if (useTrigger) ao->setTrigger("PFI0");
    if (useTrigger && ao->isError()) {
        cleanup();
        lastErrorMsg = "prepareCmd: failed to configure trigger";
        return false;
    }

    double piezoStartPos, piezoStopPos, preStop;
    //uInt16 * bufAo = ao->getOutputBuf();
	float64 * bufAo = ao->getOutputBuf();
    //uInt16 * buf = bufAo - 1;
	float64 * buf = bufAo - 1;
    for (unsigned idx = 0; idx < movements.size(); ++idx){
        const Movement& m = *movements[idx];
        if (_isnan(m.from)) piezoStartPos = preStop;  // ??? preStop value is not initialized yet
        else piezoStartPos = zpos2voltage(m.from);
        if (_isnan(m.to)) piezoStopPos = piezoStartPos;
        else piezoStopPos = zpos2voltage(m.to);

        preStop = piezoStopPos;

        //get buffer address and generate waveform:
        //int aoStartValue = ao->toDigUnit(piezoStartPos);
        //int aoStopValue = ao->toDigUnit(piezoStopPos);
		double aoStartValue = piezoStartPos;
		double aoStopValue = piezoStopPos;

        int nScansNow = int(scanRateAo*m.duration / 1e6);
        if (idx == movements.size() - 1){
            nScansNow = ao->nScans - (buf - bufAo + 1);
        }//if, need roundup correction
        for (int i = 0; i < nScansNow; i++){
            //*++buf = uInt16(double(i) / nScansNow*(aoStopValue - aoStartValue) + aoStartValue);
			*++buf = double(i) / nScansNow*(aoStopValue - aoStartValue) + aoStartValue;
        }
        /*
        for (int i = 0; i < nScansExtra; i++) {
            double value = (idx == 0) ? aoStopValue : aoStartValue;
            *++buf = value;
        }
        */
        if (nScansNow>0 || (nScansNow == 0 && idx != 0)){//todo: if nScansNow==0 && idx==0, do this at end of for loop
            *buf = aoStopValue; //precisely set the last sample; or if move in 0 sec, rewrite last move's last sample
        }
    }//for, each movement


    /// now the trigger
	int aoTTLHigh = 5.0; // ao->toDigUnit(5.0);
	int aoTTLLow = 0.0; // ao->toDigUnit(0);
    bufAo += ao->nScans;
    buf = bufAo;
    for (int i = 0; i < ao->nScans; i++){
        bufAo[i] = aoTTLLow;
    }
    for (unsigned idx = 0; idx < movements.size(); ++idx){
        const Movement& m = *movements[idx];
        int nScansNow = int(scanRateAo*m.duration / 1e6);

        // Some cameras trigger on a pulse, others use an "enable" gate
        // that requires TTL high throughout the whole acquisition period.
        // For generality, implement the latter behavior.
        if (m.trigger == 1){
            //for (int k = 0; k < nScansNow - 1; k++)  // set low on very last sample
            for (int k = 0; k < nScansNow; k++)
                    buf[k] = aoTTLHigh;
        }
        buf += nScansNow;
    }

    ao->updateOutputBuf();
    //todo: check error like
    // if(ao->isError()) goto exitpoint;

    return true;
}//prepareCmd(),

int VolPiezo::readWaveToBuffer(int num)
{
    if (idx >= totalSample) return 0;
    float64 * bufAo = ao->getOutputBuf();
    float64 * buf = bufAo - 1;
    int readNum = (idx + num < totalSample)? num : totalSample - idx;
    int end = idx + readNum;
    if (!waveData->piezo1.empty())
        for (unsigned i = idx; i < end; ++i) {
            *++buf = zpos2voltage(waveData->piezo1[i]);
        }
    if (!waveData->piezo2.empty())
        for (unsigned i = idx; i < end; ++i) {
            *++buf = zpos2voltage(waveData->piezo2[i]);
        }
    idx += readNum;
    return readNum;
}

int VolPiezo::readConWaveToBuffer(int num)
{
    if (idx >= totalSample) return 0;
    float64 * bufAo = ao->getOutputBuf();
    float64 * buf = bufAo - 1;
    int readNum = (idx + num < totalSample) ? num : totalSample - idx;
    int end = idx + readNum;
    if (!conWaveData->isEmpty(positioner1)) {
        QVector<int> waveData;
        conWaveData->readControlWaveform(waveData, positioner1, idx, end - 1, 1);
        for (unsigned i = 0; i < readNum; ++i) {
            *++buf = zpos2voltage(waveData[i]);
        }
    }
    if (!conWaveData->isEmpty(positioner2)) {
        QVector<int> waveData;
        conWaveData->readControlWaveform(waveData, positioner2, idx, end - 1, 1);
        for (unsigned i = 0; i < readNum; ++i) {
            *++buf = zpos2voltage(waveData[i]);
        }
    }
    idx += readNum;
    return readNum;
}

bool VolPiezo::prepareCmd(WaveData *waveData)
{
    if (ao) cleanup();

    //prepare for AO:
    if (aoOnce) {
        delete aoOnce;
        aoOnce = nullptr;
    }

    int aoChannelForPiezo1 = 0; //TODO: put this configurable
    int aoChannelForPiezo2 = 1;
    vector<int> aoChannels;
    if(!waveData->piezo1.empty())
        aoChannels.push_back(aoChannelForPiezo1);
    if (!waveData->piezo2.empty())
        aoChannels.push_back(aoChannelForPiezo2);

    int totalSample = waveData->sampleNum;

    ao = new NiDaqAo(aoname, aoChannels);

    ao->cfgTiming(waveData->sampleRate, totalSample);

    if (ao->isError()) {
        cleanup();
        lastErrorMsg = "prepareCmd: failed to configure timing";
        return false;
    }

    idx = 0;
    readWaveToBuffer(totalSample);

    ao->updateOutputBuf();

    return true;
}//prepareCmd(),

void VolPiezo::writeNextSamples(void)
{
    readConWaveToBuffer(blockSize);
    ao->updateOutputBuf(blockSize);
    return;
}

bool VolPiezo::prepareCmdBuffered(WaveData *waveData)
{
    if (ao) cleanup();

    //prepare for AO:
    if (aoOnce) {
        delete aoOnce;
        aoOnce = nullptr;
    }

    this->waveData = waveData;
    int aoChannelForPiezo1 = 0; //TODO: put this configurable
    int aoChannelForPiezo2 = 1;
    vector<int> aoChannels;
    if (!waveData->piezo1.empty())
        aoChannels.push_back(aoChannelForPiezo1);
    if (!waveData->piezo2.empty())
        aoChannels.push_back(aoChannelForPiezo2);

    totalSample = waveData->sampleNum;

    ao = new NiDaqAo(aoname, aoChannels);

    ao->cfgTimingBuffered(waveData->sampleRate, totalSample);
    if (ao->isError()) {
        cleanup();
        lastErrorMsg = "VolPiezo::prepareCmdBuffered: failed to configure timing";
        return false;
    }

    blockSize = ao->getBlockSize();
    ao->setNSampleCallback(CallbackWrapper,this);
    idx = 0;
    ao->updateOutputBuf(readWaveToBuffer(2 * blockSize));

    return true;
}//prepareCmd(),

bool VolPiezo::prepareCmdBuffered(ControlWaveform *conWaveData)
{
    if (ao) cleanup();

    //prepare for AO:
    if (aoOnce) {
        delete aoOnce;
        aoOnce = nullptr;
    }

    this->conWaveData = conWaveData;
    int aoChannelForPiezo1 = 0; //TODO: put this configurable
    int aoChannelForPiezo2 = 1;
    vector<int> aoChannels;
    if (!conWaveData->isEmpty(positioner1))
        aoChannels.push_back(aoChannelForPiezo1);
    if (!conWaveData->isEmpty(positioner2))
        aoChannels.push_back(aoChannelForPiezo2);

    totalSample = conWaveData->totalSampleNum;

    ao = new NiDaqAo(aoname, aoChannels);

    ao->cfgTimingBuffered(conWaveData->sampleRate, totalSample);
    if (ao->isError()) {
        cleanup();
        lastErrorMsg = "VolPiezo::prepareCmdBuffered: failed to configure timing";
        return false;
    }

    blockSize = ao->getBlockSize();
    ao->setNSampleCallback(CallbackWrapper, this);
    idx = 0;
    ao->updateOutputBuf(readConWaveToBuffer(2 * blockSize));

    return true;
}//prepareCmd(),

bool VolPiezo::runCmd()
{
    //start piezo movement (and may also trigger the camera):
    ao->start(); //todo: check error

    return true;
}


bool VolPiezo::waitCmd()
{
    //wait until piezo's orig position is reset
    ao->wait(-1); //wait until DAQ task is finished
    //stop ao task:
    ao->stop();

    //wait 50ms to have Piezo settled (so ai thread can record its final position)
    //Sleep(0.05 * 1000); // *1000: sec -> ms

    return true;
}


bool VolPiezo::abortCmd()
{
    //stop ao task:
    ao->stop();

    return true;
}

void VolPiezo::cleanup()
{
    delete ao;
    ao = nullptr;

}

string VolPiezo::getSyncOut()
{
    return ao->getTrigOut();
}

string VolPiezo::getClkOut()
{
    return ao->getClkOut();
}


int DigitalOut::readPulseToBuffer(int num)
{
    if (idx >= totalSample) return 0;
    uInt32 * bufDo = dout->getOutputBuf();
    uInt32 * buf = bufDo - 1;
    int readNum = (idx + num < totalSample) ? num : totalSample - idx;
    int end = idx + readNum;
    for (unsigned i = idx; i < end; ++i) {
        uInt32 data = 0;
        if (!waveData->stimulus1.empty())
            if (waveData->stimulus1[i]) data |= (0x01 << doChannelForStimulus1);
        if (!waveData->stimulus2.empty())
            if (waveData->stimulus2[i]) data |= (0x01 << doChannelForStimulus2);
        if (!waveData->stimulus3.empty())
            if (waveData->stimulus3[i]) data |= (0x01 << doChannelForStimulus3);
        if (!waveData->stimulus4.empty())
            if (waveData->stimulus4[i]) data |= (0x01 << doChannelForStimulus4);
        if (!waveData->laser1.empty())
            if (waveData->laser1[i])    data |= (0x01 << doChannelForlaser1);
        if (!waveData->laser2.empty())
            if (waveData->laser2[i])    data |= (0x01 << doChannelForlaser2);
        if (!waveData->camera1.empty())
            if (waveData->camera1[i])   data |= (0x01 << doChannelForcamera1);
        if (!waveData->camera2.empty())
            if (waveData->camera2[i])   data |= (0x01 << doChannelForcamera2);
        *++buf = data;
    }
    idx += readNum;
    return readNum;
}


int DigitalOut::readConPulseToBuffer(int num)
{
    if (idx >= totalSample) return 0;
    uInt32 * bufDo = dout->getOutputBuf();
    uInt32 * buf = bufDo - 1;
    int readNum = (idx + num < totalSample) ? num : totalSample - idx;
    int end = idx + readNum;
    QVector<QVector<int>> waveData(8);
    conWaveData->readControlWaveform(waveData[doChannelForStimulus1], stimulus1, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForStimulus2], stimulus2, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForStimulus3], stimulus3, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForStimulus4], stimulus4, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForlaser1], laser1, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForlaser2], laser2, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForcamera1], camera1, idx, end - 1, 1);
    conWaveData->readControlWaveform(waveData[doChannelForcamera2], camera2, idx, end - 1, 1);
    for (unsigned i = 0; i < readNum; ++i) {
        uInt32 data = 0;
        for (int j = 0; j < 8; j++) {
            if (!waveData[j].isEmpty())
                if (waveData[j][i]) data |= (0x01 << j);
        }
        /*
        if (!waveData[doChannelForStimulus1].isEmpty())
            if (waveData[doChannelForStimulus1][i]) data |= (0x01 << doChannelForStimulus1);
        if (!waveData[doChannelForStimulus2].isEmpty())
            if (waveData[doChannelForStimulus2][i]) data |= (0x01 << doChannelForStimulus2);
        if (!waveData[doChannelForStimulus3].isEmpty())
            if (waveData[doChannelForStimulus3][i]) data |= (0x01 << doChannelForStimulus3);
        if (!waveData[doChannelForStimulus4].isEmpty())
            if (waveData[doChannelForStimulus4][i]) data |= (0x01 << doChannelForStimulus4);
        if (!waveData[doChannelForlaser1].isEmpty())
            if (waveData[doChannelForlaser1][i])    data |= (0x01 << doChannelForlaser1);
        if (!waveData[doChannelForlaser2].isEmpty())
            if (waveData[doChannelForlaser2][i])    data |= (0x01 << doChannelForlaser2);
        if (!waveData[doChannelForcamera1].isEmpty())
            if (waveData[doChannelForcamera1][i])   data |= (0x01 << doChannelForcamera1);
        if (!waveData[doChannelForcamera2].isEmpty())
            if (waveData[doChannelForcamera2][i])   data |= (0x01 << doChannelForcamera2);
            */
        *++buf = data;
    }
    idx += readNum;
    return readNum;
}

bool DigitalOut::prepareCmd(WaveData *waveData, string clkName)
{
    bool retVal;

    totalSample = waveData->sampleNum;

    if (dout) delete dout;
    dout = new NiDaqDo(doname);

    // DO uses AO sampling clock
    retVal = dout->cfgTiming(waveData->sampleRate, totalSample, clkName);
    if(!retVal)
        return false;

    if (dout->isError()) {
        lastErrorMsg = "prepareCmd: failed to configure timing";
        return false;
    }

    idx = 0;
    readPulseToBuffer(totalSample);

    dout->updateOutputBuf();

    return true;
}//prepareCmd(),

void DigitalOut::writeNextSamples(void)
{
    readConPulseToBuffer(blockSize);
    dout->updateOutputBuf(blockSize);
    return;
}

bool DigitalOut::prepareCmdBuffered(WaveData *waveData, string clkName)
{
    this->waveData = waveData;
    totalSample = waveData->sampleNum;

    if (dout) delete dout;
    dout = new NiDaqDo(doname);

    dout->cfgTimingBuffered(waveData->sampleRate, totalSample, clkName);
    if (dout->isError()) {
        lastErrorMsg = "DigitalOut::prepareCmdBuffered: failed to configure timing";
        return false;
    }

    blockSize = dout->getBlockSize();
    dout->setNSampleCallback(CallbackWrapper,this);
    idx = 0;
    dout->updateOutputBuf(readPulseToBuffer(2 * blockSize));

    return true;
}//prepareCmdBuffered(),

bool DigitalOut::prepareCmdBuffered(ControlWaveform *conWaveData, string clkName)
{
    this->conWaveData = conWaveData;
    totalSample = conWaveData->totalSampleNum;

    if (dout) delete dout;
    dout = new NiDaqDo(doname);

    dout->cfgTimingBuffered(conWaveData->sampleRate, totalSample, clkName);
    if (dout->isError()) {
        lastErrorMsg = "DigitalOut::prepareCmdBuffered: failed to configure timing";
        return false;
    }

    blockSize = dout->getBlockSize();
    dout->setNSampleCallback(CallbackWrapper, this);
    idx = 0;
    dout->updateOutputBuf(readConPulseToBuffer(2 * blockSize));

    return true;
}//prepareCmdBuffered(),

bool DigitalOut::runCmd()
{
    //start piezo movement (and may also trigger the camera):
    dout->start(); //todo: check error

    return true;
}

bool DigitalOut::waitCmd()
{
    //wait until piezo's orig position is reset
    dout->wait(-1); //wait until DAQ task is finished
                  //stop ao task:
    dout->stop();

    //wait 50ms to have Piezo settled (so ai thread can record its final position)
    //Sleep(0.05 * 1000); // *1000: sec -> ms

    return true;
}

bool DigitalOut::abortCmd()
{
    //stop ao task:
    dout->stop();

    return true;
}

void DigitalOut::cleanup()
{
    delete dout;
    dout = nullptr;
}

bool DigitalOut::singleOut(int lineIndex, bool newValue)
{
    bool retVal;
    if (!dout->isDone())
        dout->stop();
    retVal = dout->outputChannelOnce(lineIndex, newValue);
    return retVal;
}

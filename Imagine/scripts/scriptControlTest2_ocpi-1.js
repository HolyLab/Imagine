var ret1, ret2, ret3, ret11, ret22, ret33;
var estimatedRunTime;
var timeElpasedAfterDAQEnd;
var expectedTimeWillTakeToDAQStart;
var durationType = 
    {"recordStrt2DaqStrt":0,
     "DaqStrt2DaqEnd":1,
     "DaqEnd2RecordEnd":2,
     "DaqEnd2Now":3}
var msg;

// output filename
var paramCam1ConfigFile = "d:/test/OCPI_cfg1.txt";
var waveCam1ConfigFile = "d:/test/OCPI_cfg1_wav.txt"
var test1Cam1File = "d:/test/test1_cam1.imagine";
var test2Cam1File = "d:/test/test2_cam1.imagine";
var test3Cam1File = "d:/test/test3_cam1.imagine";
var test3WaveformFile = "d:/test/t_script_wav2.json";

// setup recording timeout time
var timeout
var timeoutMargin = 40; // ocpi-1 around 30sec

/* Check all the configurations before running them */
print("Checking the validity of all three configurations");
// loard OCPI_cfg1.txt (this is parameter control config file)
loadConfig(paramCam1ConfigFile);
setOutputFilename(test1Cam1File);
// check the 1st configuration
ret1 = validityCheck();

// OCPI_cfg1_wav.txt (waveform control : t_script_wav.json is specified in)
loadConfig(waveCam1ConfigFile); 
setOutputFilename(test2Cam1File);
// check the 2nd configuration
ret2 = validityCheck();

// replace t_script_wav.json with t_script_wav2.json
ret3 = loadWaveform(test3WaveformFile);
setOutputFilename(test3Cam1File);
// check the 3rd configuration
ret3 = validityCheck();

/* Execute all the configurations */
if(ret1 && ret2 && ret3) { // If all the configurations are valid 

    // setup the 1st configuration
    print("Recoding the 1st configuration");
    loadConfig(paramCam1ConfigFile);
    setOutputFilename(test1Cam1File);
    estimatedRunTime = applyConfiguration(); // this can be also used to apply this configuation to the system
    // Recording
    timeout = estimatedRunTime + timeoutMargin;
    ret11 = record(timeout);
    if(!ret11) stopRecord();

    // sleep 20000 msec
    delayBetweenRecording(20);

    // 2nd configuration
    print("Recoding the 2nd configuration");
    loadConfig(waveCam1ConfigFile); 
    setOutputFilename(test2Cam1File);
    estimatedRunTime = applyConfiguration();
    timeout = estimatedRunTime + timeoutMargin;
    ret22 = record(timeout);
    if(!ret22) stopRecord();

    // wait until specific time
    var targetDateTime = new Date("October 9, 2017 20:24:00");
    print("wait until "+ targetDateTime);
    while (new Date() < targetDateTime){
         sleepms(1000);
    }
    print("now is " + new Date());   

    // 3rd configuration
    // If we are here, we already loaded OCPI_cfg1_wav.txt.
    // So, we just replace waveform file
    print("Recoding the 3rd configuration");
    ret3 = loadWaveform(test3WaveformFile) ;
    setOutputFilename(test3Cam1File);
    estimatedRunTime = applyConfiguration();
    timeout = estimatedRunTime + timeoutMargin;
    ret33 = record(timeout);
    if (!ret33) stopRecord();
}

// Display execution results
print("All the recodings are finished");
var msg1 = "configuration 1 validity: "+ ret1 +", recording: " + ret11;
print(msg1);
var msg2 = "configuration 2 validity: "+ ret2 +", recording: " + ret22;
print(msg2);
var msg3 = "configuration 3 validity: "+ ret3 +", recording: " + ret33;
print(msg3);

function delayBetweenRecording(delay)
{
    print("Making " + delay + "  seconds delay between recordings");
    timeElpasedAfterDAQEnd = getTimeElapsed(durationType.DaqEnd2Now);
    print("Already passed " + timeElpasedAfterDAQEnd + " seconds");
    expectedTimeWillTakeToDAQStart = getTimeElapsed(durationType.recordStrt2DaqStrt);
    print("And, it will take " + expectedTimeWillTakeToDAQStart + " seconds to next DAQ start after recording start");
    var timeToWait = delay - timeElpasedAfterDAQEnd - expectedTimeWillTakeToDAQStart;
    if (timeToWait > 0) {
        print("So, we need additional " + timeToWait + " seconds to wait");
        sleepms(timeToWait * 1000);
    }
}

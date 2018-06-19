var ret1, ret2, ret3, ret11, ret22, ret33;

// output filename
var paramCam1ConfigFile = "d:/test/OCPI_cfg1.txt";
var paramCam2ConfigFile = "d:/test/OCPI_cfg1.txt"
var waveCam1ConfigFile = "d:/test/OCPI_cfg1_wav.txt"
var waveCam2ConfigFile = "d:/test/OCPI_cfg1_wav.txt"
var test1Cam1File = "d:/test/test1_cam1.imagine";
var test1Cam2File = "f:/test/test1_cam2.imagine";
var test2Cam1File = "d:/test/test2_cam1.imagine";
var test2Cam2File = "f:/test/test2_cam2.imagine";
var test3Cam1File = "d:/test/test3_cam1.imagine";
var test3Cam2File = "f:/test/test3_cam2.imagine";
var test3WaveformFile = "d:/test/t_script_wav2.json";

// setup recording timeout time
var timeout = 30; // 30sec
var timeoutMargin = 20;

/* Check all the configurations before running them */
print("Checking the validity of all three configurations");
// loard OCPI_cfg1.txt (this is parameter control config file)
loadConfig(paramCam1ConfigFile, paramCam2ConfigFile);
setOutputFilename(test1Cam1File, test1Cam2File);
// check the 1st configuration
ret1 = validityCheck();

// OCPI_cfg1_wav.txt (waveform control : t_script_wav.json is specified in)
loadConfig(waveCam1ConfigFile, waveCam2ConfigFile);
setOutputFilename(test2Cam1File, test2Cam2File);
// check the 2nd configuration
ret2 = validityCheck();

// replace t_script_wav.json with t_script_wav2.json
ret3 = loadWaveform(test3WaveformFile);
setOutputFilename(test3Cam1File, test3Cam2File);
// check the 3rd configuration
ret3 = validityCheck();

/* Execute all the configurations */
if(ret1 && ret2 && ret3) { // If all the configurations are valid

    var estimatedRunTime;
    // setup the 1st configuration
    print("Recoding the 1st configuration");
    loadConfig(paramCam1ConfigFile, paramCam2ConfigFile);
    setOutputFilename(test1Cam1File, test1Cam2File);
    applyConfiguration(); // this can be also used to apply this configuation to the system
    estimatedRunTime = getEstimatedRunTime();
    // Recording
    timeout = estimatedRunTime + timeoutMargin;
    ret11 =  record(timeout);
    if (!ret11) stopRecord();

    // sleep 3000 msec
    print("Waiting 3 seconds");
    sleepms(3000);

    // 2nd configuration
    print("Recoding the 2nd configuration");
    loadConfig(waveCam1ConfigFile, waveCam2ConfigFile);
    setOutputFilename(test2Cam1File, test2Cam2File);
    applyConfiguration();
    estimatedRunTime = getEstimatedRunTime();
    timeout = estimatedRunTime + timeoutMargin;
    ret22 = record(timeout);
    if (!ret22) stopRecord();

    // sleep 100000 msec
    print("Waiting 10 seconds");
    sleepms(10000);

    // 3rd configuration
    // If we are here, we already loaded OCPI_cfg1_wav.txt.
    // So, we just replace waveform file
    print("Recoding the 3rd configuration");
    ret3 = loadWaveform(test3WaveformFile) ;
    setOutputFilename(test3Cam1File, test3Cam2File);
    applyConfiguration();
    estimatedRunTime = getEstimatedRunTime();
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

var ret1, ret2, ret3, ret11, ret22, ret33;

// output filename
var paramCam1ConfigFile = "d:/test/OCPI_cfg1.txt";
var paramCam2ConfigFile = "d:/test/OCPI_cfg1.txt"
var waveCam1ConfigFile = "d:/test/OCPI_cfg1_wav.txt"
var waveCam2ConfigFile = "d:/test/OCPI_cfg1_wav.txt"
var test1Cam1File = "d:/test/test1_cam1.imagine";
var test1Cam2File = "d:/test/test1_cam2.imagine";
var test2Cam1File = "d:/test/test2_cam1.imagine";
var test2Cam2File = "d:/test/test2_cam2.imagine";
var test3Cam1File = "d:/test/test3_cam1.imagine";
var test3Cam2File = "d:/test/test3_cam2.imagine";
var test3WaveformFile = "d:/test/t_script_wav2.json";

// setup recording timeout time
var timeout = 30000; // 30000msec

/* Check all the configurations before running them */
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

    // setup the 1st configuration
    loadConfig(paramCam1ConfigFile, paramCam2ConfigFile);
    setOutputFilename(test1Cam1File, test1Cam2File);
    validityCheck(); // this can be also used to apply this configuation to the system
    // Recording
    ret11 =  record(timeout);

    // sleep 3000 msec
    sleep(3000);

    // 2nd configuration
    if(ret11) { // if the 1st recording is OK
        loadConfig(waveCam1ConfigFile, waveCam2ConfigFile); 
        setOutputFilename(test2Cam1File, test2Cam2File);
        validityCheck();
        ret22 =  record(timeout);
    }

    // sleep 100000 msec
    sleep(10000);

    // 3rd configuration
    if(ret22) {
        // If we are here, we already loaded OCPI_cfg1_wav.txt.
        // So, we just replace waveform file
        ret3 = loadWaveform(test3WaveformFile) ;
        setOutputFilename(test3Cam1File, test3Cam2File);
        validityCheck();
        ret33 =  record(timeout);
    }
}

// Display execution results
var msg1 = "config. validity: "+ ret1 +", recording: " + ret11;
print(msg1);
var msg2 = "config. validity: "+ ret2 +", recording: " + ret22;
print(msg2);
var msg3 = "config. validity: "+ ret3 +", recording: " + ret33;
print(msg3);

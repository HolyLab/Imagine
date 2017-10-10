if(rig=="dummy"){
    var camera = "dummy";
    var magnificationFactor = "1"
	var positioner = "dummy";
	var maxposition = "800"; // this value is for ocpi-2 positioner
	var maxspeed = "2000"; // this value is for ocpi-2 positioner
	var ctrlrsetup = "none" // no piezo controller setting
	var daq = "dummy";
	var doname = "Dev2/port0/line0:6";
	var diname = "Dev2/port0/line7";
	var aoname = "Dev2/ao";
	var ainame = "Dev2/ai";
	var maxlaserfreq = "1000"; // this shutter is fast but temporally set as 1000
	var maxgalvospeed = "100"; // 100V/sec
	var mingalvovoltage = "-10"; // 100V/sec
	var maxgalvovoltage = "10"; // 100V/sec
}


if(rig=="hs-ocpi"){
	//camera:
	var camera = "cooke";
	//var camera="avt";
	//var camera="andor";

	//piezo/stage/actuator:
	//var positioner = "dummy";
	//var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	var positioner = "pi";
	//var positioner = "thor";

	//daq:
	//var daq="dummy";
	var daq = "ni";
	var doname = "Dev1/port0/line0:7";
	var aoname = "Dev1/ao";
	var ainame = "Dev1/ai";
}

if(rig=="bakewell"){
	var camera="avt";
	var positioner = "thor";
	var daq="ni";
	var doname = "Dev1/port0/line0:7";
	var aoname = "Dev1/ao";
	var ainame = "Dev1/ai";
}

if(rig=="ocpi-1"){
	var camera="cooke";
	var magnificationFactor = "0.5"
	var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	var maxposition = "400"; // this value is for ocpi-1 positioner
	var maxspeed = "2000"; // this value is for ocpi-2 positioner
	var ctrlrsetup = "none" // piezo controller set by offline setup
	var daq = "ni";
	var doname = "Dev2/port0/line0:6";
	var diname = "Dev2/port0/line7";
	var aoname = "Dev2/ao";
	var ainame = "Dev2/ai";
	var maxlaserfreq = "25";
}

if(rig=="ocpi-2"){
	var camera="cooke";
	var magnificationFactor = "1"
	var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	var maxposition = "800"; // this value is for ocpi-2 positioner
	var maxspeed = "2000"; // this value is for ocpi-2 positioner
	var ctrlrsetup = "none" // no piezo controller setting
//	var ctrlrsetup = "COM3" // piezo controller set by serial comm. through COM3
	var daq = "ni";
	var doname = "Dev2/port0/line0:23";
	var diname = "Dev2/port0/line24:31";
	var aoname = "Dev2/ao";
	var ainame = "Dev2/ai";
	var maxlaserfreq = "1000"; // this shutter is fast but temporally set as 1000
	var maxgalvospeed = "100"; // 100V/sec
	var mingalvovoltage = "-10"; // 100V/sec
	var maxgalvovoltage = "10"; // 100V/sec
}

if (rig=="ocpi-1bidi") {
    var camera = "cooke";
    var positioner = "dummy";
    var daq = "dummy";
}

if (rig == "ocpi-lsk") {
    var camera = "cooke";
    var magnificationFactor = "1"
    var positioner = "volpiezo"; //NOTE: requires daq=="ni"
    var maxposition = "400"; // this value is for ocpi-2 positioner
    var maxspeed = "2000"; // this value is for ocpi-2 positioner
    var daq = "ni";
    var doname = "Dev2/port0/line0:23";
    var diname = "Dev2/port0/line24:31";
    var aoname = "Dev2/ao";
    var ainame = "Dev2/ai";
    var maxlaserfreq = "1000"; // this shutter is fast but temporally set as 1000
}

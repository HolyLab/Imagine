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
	var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	var daq="ni";
	var doname = "Dev2/port0/line0:7";
	var aoname = "Dev2/ao";
	var ainame = "Dev2/ai";
}

if (rig=="ocpi-1bidi") {
    var camera = "cooke";
    var positioner = "dummy";
    var daq = "dummy";
}

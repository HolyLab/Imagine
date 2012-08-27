if(rig=="hs-ocpi"){
	//camera:
	var camera = "cooke";
	//var camera="avt";
	//var camera="andor";

	//piezo/stage/actuator:
	var positioner = "dummy";
	//var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	//var positioner = "pi";
	//var positioner = "thor";

	//daq:
	var daq="dummy";
	//var daq="ni";
}

if(rig=="bakewell"){
	var camera="avt";
	var positioner = "thor";
	var daq="ni";
}

if(rig=="ocpi-1"){
	var camera="andor";
	var positioner = "volpiezo"; //NOTE: requires daq=="ni"
	var daq="ni";
}


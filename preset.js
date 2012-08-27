//hold preset params when imagine initially load

if(rig=="hs-ocpi"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:400,
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:89,
      exposureTime:0.0107,
      idleTime:0.7,
      triggerMode: "internal", //internal or external
   };
}

if(rig=="bakewell"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:400,
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:200,
      exposureTime:0.02,
      idleTime:0.7,
      triggerMode: "internal", //internal or external
   };
}

if(rig=="ocpi-1"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:400,
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:20,
      exposureTime:0.03,
      idleTime:0.7,
      triggerMode: "external", //internal or external
   };
}



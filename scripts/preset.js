//hold preset params when imagine initially load

if(rig=="hs-ocpi"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:400,
      initialPosition: 100, //the position when Imagine starts.
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:89,
      exposureTime:0.0107,
      idleTime:0.7,
      triggerMode: "internal", //internal or external
      gain:0,
   };
}

if(rig=="bakewell"){
   var preset={
      ///positioner's
      startPosition:200,
      stopPosition: 600,
      initialPosition: 500, //the position when Imagine starts.
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:100,
      exposureTime:0.02,
      idleTime:0.7,
      triggerMode: "internal", //internal or external
      gain:24,
   };
   if(positioner=="thor"){
      preset.startPosition=11000;
      preset.stopPosition= 20000;
      preset.initialPosition= 15000; 
   }
}

if(rig=="ocpi-1"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:400,
      initialPosition: 100, //the position when Imagine starts.
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:20,
      exposureTime:0.03,
      idleTime:0.7,
      triggerMode: "external", //internal or external
      gain:0,
   };
}

if(rig=="ocpi-2"){
   var preset={
      ///positioner's
      startPosition:100,
      stopPosition:800,
      initialPosition: 100, //the position when Imagine starts.
      travelBackTime:0,

      ///camera's
      numOfStacks:5,
      framesPerStack:20,
      exposureTime:0.03,
      idleTime:0.7,
      triggerMode: "external", //internal or external
      gain:0,
   };
}



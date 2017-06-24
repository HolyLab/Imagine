var checkRoi = function () {
   var hstart = this.hstart;
   var hend = this.hend;
   var vstart = this.vstart;
   var vend = this.vend;

   return hstart < hend && vstart < vend &&
       (hstart-1)%160==0 && (2560-hend)%160==0 &&
       vstart-1 == 2160-vend;
};

var onShutterInit = function () {
   var result=openShutterDevice("COM1");

   if (result) print("open shutter device: ok");
   else print("open shutter device: failed ");
   return result;
}

var onShutterFini = function () {
   var result=closeShutterDevice();

   if (result) print("close shutter device: ok");
   else print("close shutter device: failed ");
   return result;
}

var onShutterOpen = function () {
   //var result = system("E:\\zsguo\\laserctrl\\debug\\laserctrl.exe openShutter 1");
   var result=setShutterStatus(1, true);
   if (result) print("open shutter: ok");
   else print("open shutter: failed ");
   return result;
};


var onShutterClose = function () {
   //var result = system("E:\\zsguo\\laserctrl\\debug\\laserctrl.exe closeShutter 1");
   var result=setShutterStatus(1, false);
   if (result) print("close shutter: ok");
   else print("close shutter: failed");
   return result;
};


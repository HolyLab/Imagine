var onShutterOpen = function () {
   var stackIdx=this.currentStackIndex;
   var line;
   if(stackIdx%2==0) line=6;
   else line=7;

   var result=setDio(line, true);

   return result;
};


var onShutterClose = function () {
   var stackIdx=this.currentStackIndex;
   var line;
   if(stackIdx%2==0) line=6;
   else line=7;

   var result=setDio(line, false);

   return result;
};


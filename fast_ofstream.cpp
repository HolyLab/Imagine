#include "fast_ofstream.hpp"

//todo: make it private method of FastOfstream
//todo: when datasize is not dividable by 4k, WriteFile() might complain.
bool realWrite(FastOfstream* stream, char* buf, int datasize)
{
   assert(datasize>0);

   int nBytesWritten;
   DWORD dwWritten; //NOTE: unsigned
   char *pFrom=buf;
   DWORD dwLastError;

write_more:
   stream->nWrites++; // #calls to WriteFile()
   double timerValue=stream->timer.read();
   bool isGood=WriteFile(stream->hFile, pFrom, datasize, &dwWritten, NULL);
   nBytesWritten=(int)dwWritten;
   stream->times.push_back(stream->timer.read()-timerValue);
   if(!isGood) {
      dwLastError=GetLastError();
      cerr<<"WriteFile() failed: "<<dwLastError<<endl;
      __debugbreak();
      return false;
   }

   assert(nBytesWritten>=0);
   assert(datasize>=nBytesWritten);
   datasize-=nBytesWritten;
   if(datasize>0){
      pFrom+=nBytesWritten; //NOTE: pFrom should still be aligned
      goto write_more;
   }
   //assert(datasize==nBytesWritten);
   assert(datasize==0);

   return true;
}//realWrite(),


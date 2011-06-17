#ifndef FAST_OFSTREAM_HPP
#define FAST_OFSTREAM_HPP

#include <windows.h>


class FastOfstream {
   char* unalignedbuf, *buf;
   int bufsize; // buf[]'s size
   int datasize;
   HANDLE hFile;
   bool isGood; //Status

   FastOfstream(const FastOfstream&);
   FastOfstream& operator=(const FastOfstream&);

public:
   //@param bufsize default 64M
   FastOfstream(const char* filename, int bufsize_in_kb=65536){
      unalignedbuf=0;
      hFile=INVALID_HANDLE_VALUE;
      isGood=false;

      bufsize=bufsize_in_kb*1024;
      datasize=0;
      int alignment=1024*1024;
      unalignedbuf=new char[bufsize+alignment];
      buf=unalignedbuf+(alignment-(unsigned long)unalignedbuf%alignment);

      hFile=CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
         NULL);
      if(hFile==INVALID_HANDLE_VALUE){
         return;
      }

      isGood=true;
   }

   //conversion func
   operator void*() const{
      return isGood?(void*)this:0;
   }

   FastOfstream& write(const char* moredata, int morecount){
      //pad/copy (for alignment, otherwise sometimes we can use moredata directly) then write cur buf (repeatly)
      //leave the remainder in cur buf
      while(true){
         if(datasize==bufsize){
            flush(); //NOTE: will update datasize too
            if(!isGood) break;
         }
         
         //now 0<=datasize<bufsize

         int amount2cp=min(bufsize-datasize, morecount);
         for(int i=0; i<amount2cp; ++i){
            buf[datasize++]=*moredata++;
         }
         morecount-=amount2cp;
         if(!morecount) break;
      }//while,

      return *this;
   }//write(),

   bool flush(){
      if(datasize==0 || !isGood) goto skip_write;

      DWORD nWritten;

      if(!WriteFile(hFile, buf, datasize, &nWritten, NULL)){
         isGood=false;
      }
      else   assert(datasize==nWritten);

skip_write:
      datasize=0;

      return isGood;
   }

   void close(){
      if(hFile==INVALID_HANDLE_VALUE) return;

      flush();
      CloseHandle(hFile);
      hFile=INVALID_HANDLE_VALUE;
   }

   ~FastOfstream(){
      close();
      delete unalignedbuf;
   }

};//class, FastOfstream

#endif //FAST_OFSTREAM_HPP

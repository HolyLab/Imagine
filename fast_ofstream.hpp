#ifndef FAST_OFSTREAM_HPP
#define FAST_OFSTREAM_HPP

#include <windows.h>

#include <cassert>

#include <iostream>
#include <vector>
using namespace std;

#include "timer_g.hpp"

class FastOfstream {
   char* unalignedbuf, *buf;
   int bufsize; // buf[]'s size
   int datasize;
   HANDLE hFile;
   bool isGood; //Status

   //for debug
   int nWrites;
   vector<double> times;
   Timer_g timer;

   FastOfstream(const FastOfstream&);
   FastOfstream& operator=(const FastOfstream&);

public:
   class EOpenFile{};
   class EAllocBuf{};

   //@param bufsize default 64M
   FastOfstream(const char* filename, int bufsize_in_kb=65536){
      unalignedbuf=0;
      hFile=INVALID_HANDLE_VALUE;
      isGood=false;

      nWrites=0;
      timer.start();

      bufsize=bufsize_in_kb*1024;
      datasize=0;
      int alignment=1024*1024;
      unalignedbuf=new char[bufsize+alignment];
      if(!unalignedbuf) throw EAllocBuf();
      buf=unalignedbuf+(alignment-(unsigned long)unalignedbuf%alignment);

      hFile=CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
         NULL);
      if(hFile==INVALID_HANDLE_VALUE){
         throw EOpenFile();
      }

      isGood=true;
   }

   //conversion func
   operator void*() const{
      return isGood?(void*)this:0;
   }

   FastOfstream& write(const char* moredata, int morecount){
      double timerValue=timer.read();
      
      //pad/copy (for alignment, otherwise sometimes we can use moredata directly) then write cur buf (repeatly)
      //leave the remainder in cur buf
      while(true){
         if(datasize==bufsize){
            flush(); //NOTE: will update datasize too
            if(!isGood) break;
         }
         
         //now 0<=datasize<bufsize

         int amount2cp=min(bufsize-datasize, morecount);

         /* this has 10x performance hit when in debug mode
         for(int i=0; i<amount2cp; ++i){
            buf[datasize++]=*moredata++;
         }
         */
         //alternative:
         memcpy(buf+datasize, moredata, amount2cp);
         datasize+=amount2cp;
         moredata+=amount2cp;

         morecount-=amount2cp;
         if(!morecount) break;
      }//while,
      
      //cout<<"in write(), spent "<<timer.read()-timerValue<<endl;

      return *this;
   }//write(),

   //todo: when datasize is not of x times of phy sector size (4k for "adv format")
   bool flush(){
      if(datasize==0 || !isGood) goto skip_write;

      assert(datasize>0);

      int nBytesWritten;
      DWORD dwWritten; //NOTE: unsigned
      char *pFrom=buf;

write_more:
      nWrites++; // #calls to WriteFile()
      double timerValue=timer.read();
      isGood=WriteFile(hFile, pFrom, datasize, &dwWritten, NULL);
      nBytesWritten=(int)dwWritten;
      times.push_back(timer.read()-timerValue);
      if(!isGood) goto skip_write;
      
      assert(nBytesWritten>=0);
      assert(datasize>=nBytesWritten);
      datasize-=nBytesWritten;
      if(datasize>0){
         pFrom+=nBytesWritten; //NOTE: pFrom should still be aligned
         goto write_more;
      }
      //assert(datasize==nBytesWritten);
      assert(datasize==0);

skip_write:
      datasize=0;

      return isGood;
   }

   void close(){
      if(hFile==INVALID_HANDLE_VALUE) return;

      flush();
      CloseHandle(hFile);
      hFile=INVALID_HANDLE_VALUE;

      cout<<"#writes: "<<nWrites<<endl;
      cout<<"time for writes: ";
      for(auto i=times.begin(); i!=times.end(); ++i){
         cout<<*i<<" \t";
      }
      cout<<endl;
   }

   ~FastOfstream(){
      close();
      delete unalignedbuf;
   }

};//class, FastOfstream

#endif //FAST_OFSTREAM_HPP

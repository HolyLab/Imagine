#ifndef FAST_OFSTREAM_HPP
#define FAST_OFSTREAM_HPP

#include <windows.h>

#include <cassert>

//#include <boost/thread/future.hpp>
#include <future>
#include <iostream>
#include <vector>
using namespace std;

#include "timer_g.hpp"
#include "memcpy_g.h"

//typedef boost::unique_future future;

class FastOfstream;
bool realWrite(FastOfstream* stream, char* buf, int datasize);


class FastOfstream {
    char *allBuf;// 
    char *buf;   //cur buf. when is 1st buf: == allBuf; when is 2nd buf: == allBuf+bufsize
    int bufsize; //one buf's size
    int datasize;
    HANDLE hFile;
    bool isGood; //Status
    //boost::unique_future<bool> async_future;
    future<bool> async_future;

    //for debug
    int nWrites;
    vector<double> times;
    Timer_g timer;

    friend bool realWrite(FastOfstream* stream, char* buf, int datasize);

    FastOfstream(const FastOfstream&);
    FastOfstream& operator=(const FastOfstream&);

public:
    class EOpenFile{};
    class EAllocBuf{};

    //@param bufsize default 64M
    FastOfstream(const string& filename, int bufsize_in_4kb = 65536 / 4){
        allBuf = nullptr;
        hFile = INVALID_HANDLE_VALUE;
        isGood = false;

        nWrites = 0;
        timer.start();

        bufsize = bufsize_in_4kb * 4 * 1024;
        datasize = 0;
        allBuf = (char*)_aligned_malloc(bufsize * 2, 4 * 1024); //2 bufs
        if (!allBuf) throw EAllocBuf();
        buf = allBuf; //initially, cur buf is the first one

        hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
            NULL);
        if (hFile == INVALID_HANDLE_VALUE){
            throw EOpenFile();
        }

        isGood = true;
    }

    //conversion func
    operator void*() const{
        return isGood ? (void*)this : 0;
    }

    FastOfstream& write(const char* moredata, int morecount){
        double timerValue = timer.read();

        //pad/copy (for alignment, otherwise sometimes we can use moredata directly) then write cur buf (repeatly)
        //leave the remainder in cur buf
        while (true){
            if (datasize == bufsize){
                flush(); //NOTE: will update datasize too
                if (!isGood) break;
            }

            //now 0<=datasize<bufsize

            int amount2cp = min(bufsize - datasize, morecount);

            /* this has 10x performance hit when in debug mode
            for(int i=0; i<amount2cp; ++i){
            buf[datasize++]=*moredata++;
            }
            */
            //alternative:
            memcpy_g(buf + datasize, moredata, amount2cp);
            datasize += amount2cp;
            moredata += amount2cp;

            morecount -= amount2cp;
            if (!morecount) break;
        }//while,

        //cout<<"in write(), spent "<<timer.read()-timerValue<<endl;

        return *this;
    }//write(),


    //todo: when datasize is not of x times of phy sector size (4k for "adv format")
    //note: buf is switch to the other after flush()
    bool flush(){
        if (isGood) {
            ///wait for previous write's finish
            if (async_future.valid()){
                isGood = async_future.get();
            }

            if (datasize == 0 || !isGood) goto skip_write;

            //now still good and previous write is done and we do want to write sth

            async_future = async(launch::async, realWrite, this, buf, datasize);
            ///switch buf
            if (buf == allBuf) buf += bufsize;
            else buf = allBuf;
        }

    skip_write:
        datasize = 0;
        return isGood;
    }

    int remainingBufSize()const{
        return bufsize - datasize;
    }

    void close(){
        if (hFile == INVALID_HANDLE_VALUE) return;

        flush(); //write any pending
        flush(); //wait 4 the async writing done
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        cout << "#writes: " << nWrites << endl;
        /*
        cout<<"time for writes: ";
        for(auto i=times.begin(); i!=times.end(); ++i){
        cout<<*i<<" \t";
        }
        cout<<endl;
        */
    }

    ~FastOfstream(){
        close();
        _aligned_free(allBuf);
    }

};//class, FastOfstream

#endif //FAST_OFSTREAM_HPP

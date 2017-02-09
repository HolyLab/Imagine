#ifndef FAST_OFSTREAM_HPP
#define FAST_OFSTREAM_HPP

#include <windows.h>

#include <cassert>

//#include <boost/thread/future.hpp>
#include <future>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "timer_g.hpp"
#include "memcpy_g.h"

//typedef boost::unique_future future;

class FastOfstream {
public:
    class EOpenFile{};
    class EAllocBuf{};
    //Note that this is timer is not synchronized with timers in other Imagine threads
    Timer_g timer;

    //@param bufsize default 64M
    FastOfstream(const std::string& filename, __int64 total_size_bytes, int bufsize_in_8kb = 65536 / 8);
    FastOfstream(FastOfstream&& other); // move constructor

    //from MSDN
    __int64 myFileSeek(HANDLE hf, __int64 distance, DWORD MoveMethod, bool sector_align);

    //conversion func
    operator void*() const;

    void write(const char* moredata, int morecount);

    //note: buf is switched to the other after flush()
    bool flush();

    int remainingBufSize()const;
    bool getIsGood(); //getter for isGood
    void close();
    bool realWrite(int bytes_to_write, char * curBuf);

    ~FastOfstream();

private:
    string filename;
    char *allBuf;// 
    char *buf;   //cur buf. when is 1st buf: == allBuf; when is 2nd buf: == allBuf+bufsize
    int bufsize; //one buf's size
    int datasize;
    int bytes_to_truncate; //number of bytes to truncate after the last write is finished
    HANDLE hFile;
    bool isGood; //status of filestream.  set to false if write fails or if writes become misaligned with sectors in an unexpected way
    future<bool> async_future;

    //for debug
    int nWrites;
    vector<double> times;
    FastOfstream(const FastOfstream&);


};//class, FastOfstream

#endif //FAST_OFSTREAM_HPP

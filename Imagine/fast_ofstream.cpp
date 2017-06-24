#include "fast_ofstream.hpp"
#include <future>
using namespace std;

//@param bufsize default 64M
FastOfstream::FastOfstream(const string& filename, __int64 total_size_bytes, int bufsize_in_8kb) {
    this->filename = filename;
    allBuf = nullptr;
    hFile = INVALID_HANDLE_VALUE;
    isGood = false;
    bytes_to_truncate = 0;

    nWrites = 0;
    timer.start();

    //The RAID, as configured, uses an 8K unit size...
    bufsize = bufsize_in_8kb * 8 * 1024;
    datasize = 0;
    allBuf = (char*)_aligned_malloc(bufsize * 2, 32 * 1024); //2 bufs
    if (!allBuf) throw EAllocBuf();
    buf = allBuf; //initially, cur buf is the first one

    hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        throw EOpenFile();
    }

    //Ask file system to try to allocate a contiguous file on disk.  No guarantees.
    //seek to end of file
    myFileSeek(hFile, total_size_bytes, 1, true);
    SetEndOfFile(hFile);
    //seek back to beginning
    myFileSeek(hFile, -total_size_bytes, 1, true);

    isGood = true;
}

//use default move constructor
FastOfstream::FastOfstream(FastOfstream && other) = default;


//from MSDN
__int64 FastOfstream::myFileSeek(HANDLE hf, __int64 distance, DWORD MoveMethod, bool sector_align)
{
    LARGE_INTEGER li;
    LARGE_INTEGER liPost;

    if (sector_align) {
        li.QuadPart = distance - distance % 8192; //TODO: hardcoded 8k sector size (RAID specs as 8K)
    }
    else {
        li.QuadPart = distance; //TODO: hardcoded 8k sector size (RAID specs as 8K)
    }

    BOOL suc = SetFilePointerEx(hf,
        li,
        &liPost,
        MoveMethod);

    if (!suc)
    {
        OutputDebugStringW(L"File seeking failed\n");
//        assert(0 == 1); //should throw a real error here
    }
    return suc;
}

//conversion func
FastOfstream::operator void*() const {
    return isGood ? (void*)this : 0;
}

void FastOfstream::write(const char* moredata, int morecount) {
    double timerValue = timer.read();

    //pad/copy (for alignment, otherwise sometimes we can use moredata directly) then write cur buf (repeatly)
    //leave the remainder in cur buf
    while (true) {
        if (this->datasize == bufsize) {
            OutputDebugStringW(L"Flushing output file stream from fast ofstream write\n");
            flush(); //NOTE: will update datasize too
            if (!isGood) break;
        }

        int amount2cp = min(bufsize - datasize, morecount);

        /* this has 10x performance hit when in debug mode
        for(int i=0; i<amount2cp; ++i){
        buf[datasize++]=*moredata++;
        }
        */
        //alternative:
        //takes about 1.9 milliseconds for a full frame on PCO.Edge 4.2
        memcpy_g(buf + datasize, moredata, amount2cp);
        this->datasize += amount2cp;
        moredata += amount2cp;

        morecount -= amount2cp;
        if (!morecount) break;
    }//while,

    return;
}//write(),


 //note: buf is switched to the other after flush()
bool FastOfstream::flush() {
    if (isGood) {
        ///wait for previous write's finish
        if (async_future.valid()) {
            isGood = async_future.get(); //isGood should also be set inside the realWrite function, but not sure if the state is updating properly due to async call (see below)
        }

        if (datasize == 0 || !isGood) goto skip_write;

        //now still good and previous write is done and we do want to write sth
        //note that this async call doesn't seem to copy / move all of the member data correctly.
        //it works fine for now, but if this causes a problem in the future we may want to specify our own move constructor rather than the default one
        async_future = async(std::launch::async, &FastOfstream::realWrite, this, datasize, buf);
        ///switch buf
        if (buf == allBuf) buf += bufsize;
        else buf = allBuf;
    }

skip_write:
    datasize = 0;
    return isGood;
}

int FastOfstream::remainingBufSize()const {
    return bufsize - datasize;
}

bool FastOfstream::getIsGood() { return this->isGood; }

void FastOfstream::close() {
    if (hFile == INVALID_HANDLE_VALUE) return;

    flush(); //write any pending
    flush(); //wait 4 the async writing done
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    //now reopen without FILE_FLAG_NO_BUFFERING so that we can truncate the extra bytes
    hFile = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        throw EOpenFile();
    }
    //seek to end of file
    myFileSeek(hFile, -bytes_to_truncate, 2, false);
    SetEndOfFile(hFile);
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    cout << "#writes: " << nWrites << endl;
}

bool FastOfstream::realWrite(int bytes_to_write, char * curBuf)
{
    assert(bytes_to_write > 0);

    int nBytesWritten;
    DWORD dwWritten; //NOTE: unsigned
    DWORD dwLastError;
    int remainder = 0;

write_more:
    nWrites++; // #calls to WriteFile()
    double timerValue = timer.read();
    remainder = bytes_to_write % 4096;
    //We need to meet this condition: #bytes2write is an integer multiple of phy sector size (usually 4K sectors)
    //when not divisible by 4k (only happens when writing the last image), we write a few bytes of extra garbage
    if (remainder != 0) bytes_to_write = bytes_to_write - remainder + 4096;

    isGood = WriteFile(hFile, curBuf, bytes_to_write, &dwWritten, NULL);
    nBytesWritten = (int)dwWritten;
    times.push_back(timer.read() - timerValue);
    if (!isGood) {
        dwLastError = GetLastError();
        cerr << "WriteFile() failed: " << dwLastError << endl;
        __debugbreak();
        return false;
    }

    assert(nBytesWritten >= 0);
    assert(bytes_to_write >= nBytesWritten);
    bytes_to_write -= nBytesWritten;
    if (bytes_to_write > 0) { //hopefully nBytesWritten is always divisible by 4k, if not then image data will be misaligned by (4096 - remainder) bytes
        goto write_more;
    }
    assert(bytes_to_write == 0);
    if (remainder) {
        if (bytes_to_truncate != 0) isGood = false; //we should only have to update bytes_to_truncate once per recording (at the end)
        else bytes_to_truncate = 4096 - remainder;
    }
    return true;
}//realWrite(),

FastOfstream::~FastOfstream() {
    close();
    _aligned_free(allBuf);
}
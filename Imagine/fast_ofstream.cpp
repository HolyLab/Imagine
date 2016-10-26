#include "fast_ofstream.hpp"
#include <string>


//todo: make it private method of FastOfstream
bool realWrite(FastOfstream* stream, char* buf, int datasize)
{
    assert(datasize > 0);

    int nBytesWritten;
    DWORD dwWritten; //NOTE: unsigned
    char *pFrom = buf;
    DWORD dwLastError;
    Timer_g gt = stream->timer;

write_more:
    stream->nWrites++; // #calls to WriteFile()
    double timerValue = stream->timer.read();
    OutputDebugStringW((wstring(L"Time before realWrite:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
    bool isGood = WriteFile(stream->hFile, pFrom, datasize, &dwWritten, NULL);
    OutputDebugStringW((wstring(L"Time after realWrite:") + to_wstring(gt.read()) + wstring(L"\n")).c_str());
    nBytesWritten = (int)dwWritten;
    stream->times.push_back(stream->timer.read() - timerValue);
    if (!isGood) {
        dwLastError = GetLastError();
        cerr << "WriteFile() failed: " << dwLastError << endl;
        __debugbreak();
        return false;
    }

    assert(nBytesWritten >= 0);
    assert(datasize >= nBytesWritten);
    datasize -= nBytesWritten;
    if (datasize > 0){
        pFrom += nBytesWritten; //NOTE: pFrom should still be aligned
        int temp = datasize % 4096;
        //when not divisible by 4k (only happens when writing the last image):
        //write a few bytes extra
        if (temp != 0) datasize = datasize - temp + 4096;
        datasize = min(datasize, nBytesWritten); 
        goto write_more;
    }
    //assert(datasize==nBytesWritten);
    assert(datasize == 0);

    return true;
}//realWrite(),


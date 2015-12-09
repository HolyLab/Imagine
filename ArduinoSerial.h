#pragma once
#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>
#include "qbytearray.h"

// time we'll wait for arduino to restart after connecting (ms)
#define ARDUINO_WAIT_TIME 2000

class ArduinoThread;

class ArduinoSerial {
public:
    ArduinoSerial(const char *portName);
    ~ArduinoSerial();
    friend class ArduinoThread;
private:
    // connection status
    bool connected = false;
    // qt serial port
    QSerialPort *sPort = nullptr;
    // port name (eg. 'COM4')
    const char *pName = nullptr;

public:
    // Will return NULL if no data was there to be read.
    // numBytes isn't strict... just a time saver at this point,
    // in that we'll stop reading as soon as we have read *at least*
    // that many bytes.
    QByteArray ReadData(unsigned int numBytes = 0);
    // Writes data from a buffer through the Serial connection.
    // Returns true on success.
    bool WriteData(const char *buffer, unsigned int nbChar, bool clearBuff = true);
    //Check if we are actually connected
    bool IsConnected();
    // init and open the serial port
    void openSerialPort();
    // close the serial port, clear the variable
    void closeSerialPort();
};

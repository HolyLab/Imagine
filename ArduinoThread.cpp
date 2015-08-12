#include "ArduinoThread.h"
#include "qcoreapplication.h"
#include "qchar.h"
#include <Windows.h>
#include "qbytearray.h"
#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>
#include "qdebug.h"

#define ARDUINO_READBUFF     10000

#pragma region Lifecycle

ArduinoThread::ArduinoThread(const char *port) {
    // make sure we've got a non-null port name to work with
    if (port == NULL) throw 47;
    // init the serial object
    serial = new ArduinoSerial(port);
}

ArduinoThread::~ArduinoThread() {
    if (serial != NULL) {
        delete(serial);
        serial = nullptr;
    }
}

void ArduinoThread::run() {
    forever {
        // check if we should get out of here
        if (isInterruptionRequested()) return;
        // process incoming events (such as request to send a message to the duino)
        QCoreApplication::processEvents();
        /*this->exec();
        this->exit();*/
        // read incoming info, if it's there
        if (serial->sPort->bytesAvailable() > 0) readIncoming();
        // hang out for a few milliseconds... we'll speed this up later
        msleep(20);
    }
}

#pragma endregion

#pragma region Arduino Serial, Signals and Slots

void ArduinoThread::sendMessage(const char * message, int length) {
    // TODO: just call the serial method
    serial->WriteData(message, length);
}

void ArduinoThread::readIncoming() {
    // read whatever's coming in over serial... unrestricted size
    QByteArray readBytes = serial->ReadData();
    // bail if you read nothing
    if (readBytes == (QByteArray)NULL) return; // that's right. typecast null.
    // make a qstring
    QString out = QString(readBytes);
    // send out a message
    emit incomingArduino(out);
}

#pragma endregion
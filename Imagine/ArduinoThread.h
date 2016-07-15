#pragma once

#include "qthread.h"
#include "qstring.h"
#include "ArduinoSerial.h"

class ArduinoThread : public QThread {
    Q_OBJECT
public:
    ArduinoThread(const char *port = nullptr);
    ~ArduinoThread();
    void run();
    void ArduinoThread::readIncoming();
private:
    ArduinoSerial *serial = nullptr;
public slots:
    void sendMessage(const char *message, int length);
signals:
    void incomingArduino(QString message);
};


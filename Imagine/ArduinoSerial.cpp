#include "ArduinoSerial.h"
#include "qbytearray.h"
#include "qthread.h"
#include "qdebug.h"

#pragma region Lifecycle

ArduinoSerial::ArduinoSerial(const char *portName) {
    // set aside the port name
    pName = portName;
    // open the port immediately
    openSerialPort();
}

ArduinoSerial::~ArduinoSerial() {
    //Check if we are connected before trying to disconnect
    if (sPort != NULL) closeSerialPort();
}

#pragma endregion


#pragma region Serial

QByteArray ArduinoSerial::ReadData(unsigned int numBytes) {
    // make sure we at least have a port...
    if (sPort == NULL) return NULL;
    QByteArray ret = QByteArray{};
    // keep reading until you read enough or time out
    bool g2g = false;
    forever {
        g2g = (sPort->bytesAvailable() > 0 || sPort->waitForReadyRead(5));
        if (!g2g) break;
        ret.append(sPort->readAll());
        // break if we're over readLen
        if (numBytes > 0 && ret.size() >= numBytes) break;
    }
    return ret;
}

bool ArduinoSerial::WriteData(const char *buffer, unsigned int nbChar, bool clearBuff) {
    // make sure we at least have a port...
    if (sPort == NULL) return false;
    // clear the input buffer if requested
    if (clearBuff) sPort->clear(QSerialPort::Input);
    // send the data, wait until it's written
    QByteArray outBytes(buffer, nbChar);
    sPort->write(outBytes);
    bool stillWriting = true;
    forever {
        stillWriting = sPort->waitForBytesWritten(1);
        if (!stillWriting) break;
    }
    return true;
}

bool ArduinoSerial::IsConnected()
{
    //Simply return the connection status
    return this->connected;
}

void ArduinoSerial::openSerialPort() {
    // for now, no double-dipping
    if (sPort != NULL) return;
    // make, open and config a port object... cry if it doesn't work
    QSerialPort *p = new QSerialPort();
    p->setPortName(QString(pName));
    if (!p->open(QSerialPort::ReadWrite)) {
        qDebug() << "Unable to open port " << pName;
        delete(p);
        return;
    }
    QString err = NULL;
    if (!p->setBaudRate(QSerialPort::Baud9600)) {
        err = QString("Set baud rate %1 error.").arg(QSerialPort::Baud9600);
    }
    if (!p->setDataBits(QSerialPort::Data8)) {
        err = QString("Set data bits %1 error.").arg(QSerialPort::Data8);
    }
    if (!p->setParity(QSerialPort::NoParity)) {
        err = QString("Set parity %1 error.").arg(QSerialPort::NoParity);
    }
    if (!p->setStopBits(QSerialPort::OneStop)) {
        err = QString("Set stop bits %1 error.").arg(QSerialPort::OneStop);
    }
    if (!p->setFlowControl(QSerialPort::NoFlowControl)) {
        err = QString("Set flow %1 error.").arg(QSerialPort::NoFlowControl);
    }
    if (err != NULL) {
        qDebug() << err;
        p->close();
        delete(p);
        return;
    }
    // okay then
    sPort = p;
    connected = true;
    // wait 2s as the arduino board resets
    QThread::msleep(ARDUINO_WAIT_TIME);
}

void ArduinoSerial::closeSerialPort() {
    if (sPort == NULL) return;
    sPort->close();
    delete(sPort);
    sPort = NULL;
    connected = false;
}

#pragma endregion
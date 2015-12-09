#include "shutterctrl.h"

#include <bitset>
using namespace std;

#include <QtDebug>
#include <QtSerialPort/QSerialPort>

#include "ni_daq_g.hpp"

QSerialPort *port;
extern DaqDo * digOut;

bool openShutterDevice(const QString& dev)
{
    /* 1. First - create an instance of an object.
    */
    port = new QSerialPort();
    /* 2. Second - set the device name.
    */
    port->setPortName(dev);

    /* 3. Third - open the device.
    */
    if (port->open(QSerialPort::ReadWrite)) {
        /* 4. Fourth - now you can set the parameters. (after successfully opened port)
        */
        if (!port->setBaudRate(QSerialPort::Baud19200)) {
            qDebug() << "Set baud rate " << QSerialPort::Baud19200 << " error.";
            goto label;
        }

        if (!port->setDataBits(QSerialPort::Data8)) {
            qDebug() << "Set data bits " << QSerialPort::Data8 << " error.";
            goto label;
        }

        if (!port->setParity(QSerialPort::NoParity)) {
            qDebug() << "Set parity " << QSerialPort::NoParity << " error.";
            goto label;
        }

        if (!port->setStopBits(QSerialPort::OneStop)) {
            qDebug() << "Set stop bits " << QSerialPort::OneStop << " error.";
            goto label;
        }

        if (!port->setFlowControl(QSerialPort::NoFlowControl)) {
            qDebug() << "Set flow " << QSerialPort::NoFlowControl << " error.";
            goto label;
        }

        //Here, the new set parameters (for example)
        /*
        qDebug() << "= New parameters =";
        qDebug() << "Device name            : " << port->deviceName();
        qDebug() << "Baud rate              : " << port->baudRate();
        qDebug() << "Data bits              : " << port->dataBits();
        qDebug() << "Parity                 : " << port->parity();
        qDebug() << "Stop bits              : " << port->stopBits();
        qDebug() << "Flow                   : " << port->flowControl();
        qDebug() << "Char timeout, msec     : " << port->charIntervalTimeout();
        */
    }

    return true;

label:

    qDebug() << "failed to open device";
    return false;

}//openShutterDevice(),

bool closeShutterDevice()
{
    if (!port) return true;

    port->close();
    delete port;
    port = nullptr;
    return true;
}//closeShutterDevice(),

static int getShutterStatus()
{
    port->readAll(); //clear the un-readed data (such as ack from prev cmd)

    QByteArray tx, rx;
    tx = "02\r";
    int tmp = tx.length();
    int nWritten = port->write(tx);
    //qDebug() << "Writed is : " << nWritten << " bytes";
    port->waitForReadyRead(50);
    rx = port->readAll();
    //qDebug() << "Readed is : " << rx.size() << " bytes";

    bool ok;
    int result = rx.mid(2, 2).toInt(&ok, 16);

    return result;
}

//NOTE: line: 1-based numbering
bool setShutterStatus(int line, bool isOpen)
{
    unsigned int curStatus = getShutterStatus();
    bitset<4> bs((unsigned long long)curStatus);
    bs.set(line - 1, isOpen);
    int newStatus = bs.to_ulong();
    QByteArray tx = QString("01%1\r").arg(newStatus, 2, 16, QChar('0')).toLatin1();
    int nWritten = port->write(tx);
    //qDebug() << "Writed is : " << nWritten << " bytes";

    return true;
}//setShutterStatus(),

//NOTE: line: 0-based numbering
bool setDio(int line, bool isOpen)
{
    digOut->updateOutputBuf(line, isOpen);
    digOut->write();

    return true;
}

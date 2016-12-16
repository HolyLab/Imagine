#include "laserctrl.h"
#include <bitset>
using std::bitset;

#include "timer_g.hpp"

#include <QtDebug>
extern bool scriptMode;
extern QString scriptCmd;
extern QStringList scriptArgs;

LaserCtrlSerial::LaserCtrlSerial(const QString dev){
    portName = dev;
    port = NULL;
}

LaserCtrlSerial::~LaserCtrlSerial() {
    if (port != NULL) closeSerialPort();
}

bool LaserCtrlSerial::isPortOpen(void)
{
    return isPortOpened;
}

void LaserCtrlSerial::openSerialPort(QString portName)
{
    Timer_g timer;
    timer.start();

    if (portName == "DummyPort") {
	    goto readLaserStatus;
    }

    if (port != NULL) 
        closeSerialPort();
    /* 1. First - create an instance of an object. */
    port = new QSerialPort();
    /* 2. Second - set the device name. */
    port->setPortName(portName);   // was setDeviceName
    /* 3. Third - open the device. */
    if (port->open(QSerialPort::ReadWrite)) {
        /* 4. Fourth - now you can set the parameters. (after successfully opened port) */
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
     }

readLaserStatus:
    isPortOpened = true;
    getShutterStatus();
    for (int i = 1; i <= 4;i++)
        getTransStatus(true, i);
    qDebug() << "open device success";
    return;

label:
    qDebug() << "failed to open device";
    return;
}

void LaserCtrlSerial::closeSerialPort(void)
{
    isPortOpened = false;
    if (port == NULL) return;
    port->close();
    delete port;
    port = NULL;
}

int LaserCtrlSerial::readShutterStatus()
{
    if (port != NULL) port->readAll(); //clear the un-readed data (such as ack from prev cmd)

    tx = "02\r";
    int nWritten = 1;
    if (port != NULL) nWritten = port->write(tx);
    qDebug() << "Writed is : " << nWritten << " bytes";
    if (port != NULL) {
        port->waitForReadyRead(50);
        rx = port->readAll();
    }
    else
        rx = "0203";
    qDebug() << "Readed is : " << rx.size() << " bytes";

    bool ok;
    int result = rx.mid(2, 2).toInt(&ok, 16);

    return result;
}

void LaserCtrlSerial::getShutterStatus()
{
    int result = readShutterStatus();
    emit getShutterStatusReady(result);
}

//NOTE: line is 1-based numbering
void LaserCtrlSerial::getTransStatus(bool isAotf, int line)
{
   if (port != NULL) port->readAll(); //clear the un-readed data (such as ack from prev cmd)
   int op=isAotf?5:7;
   tx=QString("0%1%2\r").arg(op).arg(line-1, 2, 16, QChar('0')).toLatin1();
   int nWritten = 2;
   if (port != NULL) nWritten = port->write(tx);
   qDebug() << "Writed is : " << nWritten << " bytes";
   if (port != NULL) {
        port->waitForReadyRead(50);
        rx = port->readAll();
   }
   else
        rx = "0501F4";
   qDebug() << "Readed is : " << rx.size() << " bytes";

   bool ok;
   int result=rx.mid(2,4).toInt(&ok, 16);

   emit getTransStatusReady(isAotf, line, result);
}

//NOTE: line: 1-based numbering
void LaserCtrlSerial::setShutter(int line, bool isOpen)
{
    unsigned int curStatus = readShutterStatus();
    bitset<4> bs((unsigned long long)curStatus);
    bs.set(line-1, isOpen);
    int newStatus=bs.to_ulong();
    tx=QString("01%1\r").arg(newStatus, 2, 16, QChar('0')).toLatin1();
    int nWritten = 2;
    if (port != NULL) nWritten = port->write(tx);
    qDebug() << "Writed is : " << nWritten << " bytes";
}

void LaserCtrlSerial::setShutters(int status)
{
    tx = QString("01%1\r").arg(status, 2, 16, QChar('0')).toLatin1();
    int nWritten = 2;
    if (port != NULL) nWritten = port->write(tx);
    qDebug() << "Writed is : " << nWritten << " bytes";
}

void LaserCtrlSerial::setTrans(bool isAotf, int line, int value)
{
    if (port != NULL) port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    int op=isAotf?4:6;
    tx=QString("0%1%2%3\r").arg(op).arg(line-1, 2, 16, QChar('0'))
                .arg(value, 4, 16, QChar('0')).toLatin1();
    int nWritten = 3;
    if (port != NULL) nWritten = port->write(tx);
    qDebug() << "Writed is : " << nWritten << " bytes";
}
#include "piezoctrl.h"
#include <bitset>
using std::bitset;

#include "timer_g.hpp"

#include <QtDebug>

PiezoCtrlSerial::PiezoCtrlSerial(const QString dev){
    portName = dev;
    port = NULL;
}

PiezoCtrlSerial::~PiezoCtrlSerial() {
    if (port != NULL) closeSerialPort();
}

bool PiezoCtrlSerial::isPortOpen(void)
{
    return isPortOpened;
}

void PiezoCtrlSerial::openSerialPort(QString portName)
{
    Timer_g timer;
    timer.start();

    if (portName.left(3) != "COM") {
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
        if (!port->setBaudRate(QSerialPort::Baud115200)) {
            qDebug() << "Set baud rate " << QSerialPort::Baud115200 << " error.";
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

        if (!port->setFlowControl(QSerialPort::SoftwareControl)) {
            qDebug() << "Set flow " << QSerialPort::SoftwareControl << " error.";
            goto label;
        }
     }

readLaserStatus:
    isPortOpened = true;
    qDebug() << "open device success";
    return;

label:
    qDebug() << "failed to open device";
    return;
}

void PiezoCtrlSerial::closeSerialPort(void)
{
    isPortOpened = false;
    if (port == NULL) return;
    port->close();
    delete port;
    port = NULL;
}

void PiezoCtrlSerial::sendCommand(QString cmd)
{
    if (port != NULL) port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    bool isData;
    tx = QString("%1\r\n").arg(cmd).toLatin1();
    int nWritten = 0;
    if (port != NULL) {
        nWritten = port->write(tx);
        qDebug() << "Write : " << nWritten << " bytes";
        do {
            isData = port->waitForReadyRead(500);
        } while (isData);
        rx = port->readAll();
        qDebug() << "Read : " << rx.size() << " bytes";
        if (rx.size()) {
            if(rx.left(4)=="stat")
                emit sendStatusToGUI(rx);
            else
                emit sendValueToGUI(rx);
        }
    }
}
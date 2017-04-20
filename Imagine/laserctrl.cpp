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
    qDebug() << "open device success";
    return;

label:
    port->close();
    delete port;
    port = NULL;
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
    bool isData;
    if (port != NULL) {
        nWritten = port->write(tx);
        port->waitForBytesWritten(50);
        qDebug() << "Write : " << nWritten << " bytes";
        do {
            isData = port->waitForReadyRead(500);
        } while (isData);
        rx = port->readAll();
        qDebug() << "Read : " << rx.size() << " bytes";
    }
    else if(portName=="DummyPort")
        rx = "0203";

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
   bool isData;
   if (port != NULL) {
       nWritten = port->write(tx);
       qDebug() << "Write : " << nWritten << " bytes";
       do {
           isData = port->waitForReadyRead(500);
       } while (isData);
       rx = port->readAll();
       qDebug() << "Read : " << rx.size() << " bytes";
   }
   else if (portName == "DummyPort")
       rx = "0501F4";

   bool ok;
   int result=rx.mid(2,4).toInt(&ok, 16);

   emit getTransStatusReady(isAotf, line, result);
}

//NOTE: line: 1-based numbering
void LaserCtrlSerial::setShutter(int line, bool isOpen)
{
    if (port != NULL)
        rx = port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    unsigned int curStatus = readShutterStatus();
    bitset<4> bs((unsigned long long)curStatus);
    bs.set(line-1, isOpen);
    int newStatus=bs.to_ulong();
    tx=QString("01%1\r").arg(newStatus, 2, 16, QChar('0')).toLatin1();
    int nWritten = 2;
    bool isData;
    if (port != NULL) {
        nWritten = port->write(tx);
        qDebug() << "Write : " << nWritten << " bytes";
        do {
            isData = port->waitForReadyRead(10);
        } while (isData);
        rx = port->readAll();
    }
}

void LaserCtrlSerial::setShutters(int status)
{
    if (port != NULL)
        rx = port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    tx = QString("01%1\r").arg(status, 2, 16, QChar('0')).toLatin1();
    int nWritten = 2;
    bool isData;
    if (port != NULL) {
        nWritten = port->write(tx);
        qDebug() << "Write : " << nWritten << " bytes";
        do {
            isData = port->waitForReadyRead(10);
        } while (isData);
        rx = port->readAll();
    }
}

void LaserCtrlSerial::setTrans(bool isAotf, int line, int value)
{
    if (port != NULL)
        rx = port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    int op=isAotf?4:6;
    tx=QString("0%1%2%3\r").arg(op).arg(line-1, 2, 16, QChar('0'))
                .arg(value, 4, 16, QChar('0')).toLatin1();
    int nWritten = 3;
    bool isData;
    if (port != NULL) {
        nWritten = port->write(tx);
        qDebug() << "Write : " << nWritten << " bytes";
        do {
            isData = port->waitForReadyRead(10);
        } while (isData);
        rx = port->readAll();
    }
}


//NOTE: line is 1-based numbering
QByteArray LaserCtrlSerial::readLaserLineSetup(void)
{
    if (port != NULL)
        rx = port->readAll(); //clear the un-readed data (such as ack from prev cmd)
    tx = "08\r";
    int nWritten = 1;
    bool isData;
    if (port != NULL) {
        nWritten = port->write(tx);
        port->waitForBytesWritten(50);
        qDebug() << "Write : " << nWritten << " bytes";
        do{ 
            isData = port->waitForReadyRead(500);
        } while (isData);
        rx = port->readAll();
        qDebug() << "Read : " << rx.size() << " bytes";
    }
    else if (portName == "DummyPort")
        rx = "08131015EA0FD211621414001400000000"; // OCPI-II return this value

    /* example
    0x080FD2131015EA18EC00000000:
    The device has four lines with wavelengths 405 (0x0FD2), 488 (0x1310), 561 (0x15EA), 638
    (0x18EC) nm. The fifth line is zero (not used). The sixth line is zero, so stop counting. This is a
    single output system since there are no output control lines.

    0x080FD2131015EA18EC0014100E0000:
    The device has four lines with wavelengths 405 (0x0FD2), 488 (0x1310), 561 (0x15EA), 638
    (0x18EC) nm. The fifth line is 2 nm (0x0014), so is the second output control. The sixth line is 3
    nm (0x001E) so it controls the third output select. The seventh line is zero so stop counting.

    0x080FD2131015EA18EC1C840014001E0000
        The device has five lines with wavelengths 405 (0x0FD2), 488 (0x1310), 561 (0x15EA), 638
        (0x18EC), 730 (0x1C84) nm.The sixth line is 2 nm(0x0014), so is the second output control.
        The seventh line is 3 nm(0x001E) so it controls the third output select.The eighth line is zero so
        stop counting
    */

    QByteArray result = rx.mid(2, rx.size()-2);
    return result;
}

void LaserCtrlSerial::getLaserLineSetup(void)
{
    bool ok;

    QByteArray result = readLaserLineSetup();
    
    for(int i = 0; i < MAX_NUM_WAVELENGTH; i++) {
        int wl = result.mid(i * 4, 4).toInt(&ok, 16)/10;
        if (wl > 100) {
            wavelength[i] = wl;
            laserIndex[numLines] = i;
            numLines++;
        }
        else if (wl == 0)
            break;
    }
    emit getLaserLineSetupReady(numLines, laserIndex, wavelength);
}

int LaserCtrlSerial::getLaserIndex(int i)
{
    return laserIndex[i];
}

int LaserCtrlSerial::getNumLines(void)
{
    return numLines;
}

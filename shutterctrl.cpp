#include "shutterctrl.h"

#include <bitset>
using namespace std;

#include <QtDebug>

#include "abstractserial.h"
#include "ni_daq_g.hpp"

AbstractSerial *port;
extern DaqDo * digOut;

bool openShutterDevice(const QString& dev)
{
   /* 1. First - create an instance of an object.
   */
   port = new AbstractSerial();
   /* 2. Second - set the device name.
   */
   port->setDeviceName(dev);

   /* 3. Third - open the device.
   */
   if (port->open(AbstractSerial::ReadWrite)) {
      /* 4. Fourth - now you can set the parameters. (after successfully opened port)
      */
      if (!port->setBaudRate(AbstractSerial::BaudRate19200)) {
         qDebug() << "Set baud rate " <<  AbstractSerial::BaudRate19200 << " error.";
         goto label;
      }

      if (!port->setDataBits(AbstractSerial::DataBits8)) {
         qDebug() << "Set data bits " <<  AbstractSerial::DataBits8 << " error.";
         goto label;
      }

      if (!port->setParity(AbstractSerial::ParityNone)) {
         qDebug() << "Set parity " <<  AbstractSerial::ParityNone << " error.";
         goto label;
      }

      if (!port->setStopBits(AbstractSerial::StopBits1)) {
         qDebug() << "Set stop bits " <<  AbstractSerial::StopBits1 << " error.";
         goto label;
      }

      if (!port->setFlowControl(AbstractSerial::FlowControlOff)) {
         qDebug() << "Set flow " <<  AbstractSerial::FlowControlOff << " error.";
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

   qDebug()<< "failed to open device";
   return false;

}//openShutterDevice(),

bool closeShutterDevice()
{
   if(!port) return true;

   port->close();
   delete port;
   port=nullptr;
   return true;
}//closeShutterDevice(),

static int getShutterStatus()
{
   port->readAll(); //clear the un-readed data (such as ack from prev cmd)

   QByteArray tx, rx;
   tx="02\r";
   int tmp=tx.length();
   int nWritten = port->write(tx);
   //qDebug() << "Writed is : " << nWritten << " bytes";
   port->waitForReadyRead(50);
   rx = port->readAll();
   //qDebug() << "Readed is : " << rx.size() << " bytes";

   bool ok;
   int result=rx.mid(2,2).toInt(&ok, 16);

   return result;
}

//NOTE: line: 1-based numbering
bool setShutterStatus(int line, bool isOpen)
{
   unsigned int curStatus=getShutterStatus();
   bitset<4> bs((unsigned long long)curStatus);
   bs.set(line-1, isOpen);
   int newStatus=bs.to_ulong();
   QByteArray tx=QString("01%1\r").arg(newStatus, 2, 16, QChar('0')).toLatin1();
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

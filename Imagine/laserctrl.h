#ifndef LASERCTRL_H
#define LASERCTRL_H

#include <QString>
#include <QSerialPort>
//#include <shutterctrl.h>
#include <QThread>

#define MAX_NUM_WAVELENGTH 8

class Laser : public QObject {
    Q_OBJECT

private:
    QString deviceName = NULL;

public:
    Laser(const QString name) { deviceName = name; };
    ~Laser() {};
    QString getDeviceName (void) { return deviceName; };
};

class LaserCtrlSerial: public QObject{
    Q_OBJECT
    QThread laserCtrlThread;

private:
    QByteArray tx, rx;
    QString portName = NULL;
    QSerialPort *port;
    bool isPortOpened = false;

public:
    int wavelength[MAX_NUM_WAVELENGTH] = { 0, };
    int laserIndex[MAX_NUM_WAVELENGTH] = { 0, };
    int numLines = 0;
    LaserCtrlSerial(const QString dev);
    ~LaserCtrlSerial();
    int readShutterStatus(void);
    bool isPortOpen(void);
    void setPortName(QString portName) { this->portName = portName; };
    QString getPortName(void) { return portName; };
    QByteArray readLaserLineSetup(void);
    int getMAXnumLines(void) { return MAX_NUM_WAVELENGTH; };
    int getLaserIndex(int i);
    int getNumLines(void);

public slots:
    void openSerialPort(QString portName);
    void closeSerialPort(void);
    void getShutterStatus(void);
    void setShutter(int line, bool isOpen);
    void setShutters(int status);
    void getTransStatus(bool isAotf, int line);
    void setTrans(bool isAotf, int line, int value);
    void getLaserLineSetup(void);

signals:
    void getShutterStatusReady(int result);
    void getTransStatusReady(bool isAotf, int line, int status);
    void getLaserLineSetupReady(int numLines, int *laserIndex, int *wavelength);
};

#endif //LASERCTRL_H

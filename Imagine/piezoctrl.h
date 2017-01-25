#ifndef PIEZOCTRL_H
#define PIEZOCTRL_H

#include <QString>
#include <QSerialPort>
#include <QThread>

#define MAX_NUM_WAVELENGTH 8

class PiezoCtrlSerial : public QObject{
    Q_OBJECT
    QThread piezoCtrlThread;

private:
    QByteArray tx, rx;
    QString portName = NULL;
    QSerialPort *port;
    bool isPortOpened = false;

public:
    PiezoCtrlSerial(const QString dev);
    ~PiezoCtrlSerial();
    bool isPortOpen(void);
    void setPortName(QString portName) { this->portName = portName; };
    QString getPortName(void) { return portName; };

public slots:
    void openSerialPort(QString portName);
    void closeSerialPort(void);
    void sendCommand(QString cmd);

signals:
    void sendStatusToGUI(QByteArray rx);
    void sendValueToGUI(QByteArray rx);
};

#endif //PIEZOCTRL_H

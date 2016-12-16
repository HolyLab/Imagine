#ifndef IMG_APPLICATION
#define IMG_APPLICATION

#include "qapplication.h"
#include "qstring.h"
#include "imagine.h"
#include "camera_g.hpp"
#include "positioner.hpp"
#include "ArduinoThread.h"

/*
This is meant to be the top-level object of this application.
It's a good place to keep things that might otherwise be globals.
*/
class ImgApplication : public QApplication {
    Q_OBJECT
public:
    ImgApplication::ImgApplication(int &argc, char **argv);
    ImgApplication::~ImgApplication();

    // the main ui objects
    Imagine *imgOne = NULL;
    Imagine *imgTwo = NULL;

    // thread for monitoring arduino comm
    ArduinoThread *ardThread = nullptr;

    // Initialize the main window(s)
    void ImgApplication::initUI(Camera *cam1, Positioner *pos, Laser *laser = nullptr, Camera *cam2 = nullptr);

    // Show the main window(s).
    void ImgApplication::showUi();

    // Start the Arduino Comm Thread
    void ImgApplication::startArduinoThread(const char *port);
    // Kill the Arduino Comm Thread
    void ImgApplication::killArduinoThread();
    // Send a message to the attached Arduino, if there is one
    void ImgApplication::sendToArduino(const char *message, int length);
    void ImgApplication::sendToArduino(QString str);
public slots:
    void ardThreadFinished();
    void incomingArduinoMessage(QString message);
signals:
    void msgForArduino(const char *message, int length);
};

#endif
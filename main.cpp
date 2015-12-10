/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
**    All rights reserved.
** Author: All code authored by Zhongsheng Guo.
** License: This file may be used under the terms of the GNU General Public
**    License version 2.0 as published by the Free Software Foundation
**    and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
**
**    If this license is not appropriate for your use, please
**    contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).
**
** This file is provided WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**-------------------------------------------------------------------------*/

#include <filesystem>
using namespace std;

#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptable>
#include <QMetaType>

#include "imagine.h"
#include "ImgApplication.h"

//#include "andor_g.hpp"
//#include "avt_g.hpp"
#include "cooke.hpp"
#include "pco_errt.h"

#include "actuators/voltage/volpiezo.hpp"
//#include "Piezo_Controller.hpp"
#include "Actuator_Controller.hpp"
#include "actuators/dummy/dummypiezo.hpp"
#include "ni_daq_g.hpp"
#include "dummy_daq.hpp"
#include "shutterctrl.h"

#include "sc2_common.h"
#include "sc2_camexport.h"


extern QScriptEngine* se;
extern DaqDo* digOut;
extern QString daq;
extern string rig;

bool loadScript(const QString &filename)
{
    if (!QFile::exists(filename)) {
        QMessageBox::critical(0, "Imagine", QString("couldn't find script %1.").arg(filename),
            QMessageBox::Ok, QMessageBox::NoButton);
        return false;
    }
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::critical(0, "Imagine", QString("couldn't read script %1.").arg(filename),
            QMessageBox::Ok, QMessageBox::NoButton);
        return false;
    }
    QTextStream in(&file);
    QString jscode = in.readAll();
    QScriptProgram sp(jscode);
    QScriptValue jsobj = se->evaluate(sp);
    if (se->hasUncaughtException()){
        QMessageBox::critical(0, "Imagine",
            QString("There's problem when evaluating %1:\n%2\n%3.")
            .arg(filename)
            .arg(QString("   ... at line %1").arg(se->uncaughtExceptionLineNumber()))
            .arg(QString("   ... error msg: %1").arg(se->uncaughtException().toString())));
        return false;
    }

    return true;
}

bool run(const QString& cmd)
{
    return 0 == system(cmd.toLatin1());
}

QScriptValue runWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString x = context->argument(0).toString();
    return QScriptValue(se, run(x));
    //QScriptValue result=se->newObject();
    //result.setProperty("status", QScriptValue(se, ...));
    //result.setProperty("msg", QScriptValue(se, ...));
    //return result;
}

void getCamCount(int *camCount) {
    // so, this sucks, but for now the only way to get the camera count is
    // to iterate through camera numbers, starting with 0, and see how many
    // unique cameras we can open before getting an exception. great...

    HANDLE tCam = nullptr;
    int tErr = 0;
    int camNum = 0;
    tErr = PCO_OpenCamera(&tCam, 0);
    if (tErr != PCO_NOERROR) {
        /*char msg[16384];
        PCO_GetErrorText(tErr, msg, 16384);
        cout << msg << endl;*/
        return;
    }
    // before closing the camera, see how many other cameras you can open (ugh)
    getCamCount(camCount);
    *camCount += 1;

    // and now we can close the camera
    // (ignore the bad handle exception)
    tErr = PCO_CloseCamera(tCam);
}

int main(int argc, char *argv[])
{
    ImgApplication a(argc, argv);

    //rig = "hs-ocpi";
    rig = "ocpi-2";
    if (argc == 2) {
        rig = argv[1];
        cout << "The rig is: " << rig << endl;
    }
    else {
        cout << "The rig is default to: " << rig << endl;
    }

    se = new QScriptEngine();

    se->globalObject().setProperty("rig", QString::fromStdString(rig));

    QScriptValue svRun = se->newFunction(runWrapper);
    se->globalObject().setProperty("system", svRun);

    //shutter ctrl
    QScriptValue svRunOpendev = se->newFunction(openShutterDeviceWrapper);
    se->globalObject().setProperty("openShutterDevice", svRunOpendev);
    QScriptValue svRunClosedev = se->newFunction(closeShutterDeviceWrapper);
    se->globalObject().setProperty("closeShutterDevice", svRunClosedev);
    QScriptValue svRunSetShutter = se->newFunction(setShutterStatusWrapper);
    se->globalObject().setProperty("setShutterStatus", svRunSetShutter);
    QScriptValue svRunSetDio = se->newFunction(setDioWrapper);
    se->globalObject().setProperty("setDio", svRunSetDio);

    if (!loadScript(QString::fromStdString("imagine.js"))){
        return 1;
    }
    QString cameraVendor = se->globalObject().property("camera").toString();
    QString positionerType = se->globalObject().property("positioner").toString();
    daq = se->globalObject().property("daq").toString();
    QString doname;
    QString aoname;
    QString ainame;
    if (daq == "ni") {
        doname = se->globalObject().property("doname").toString();
        aoname = se->globalObject().property("aoname").toString();
        ainame = se->globalObject().property("ainame").toString();
    }

    cout << "using " << cameraVendor.toStdString() << " camera, "
        << positionerType.toStdString() << " positioner, "
        << daq.toStdString() << " daq." << endl;

    if (cameraVendor != "avt" && cameraVendor != "andor" && cameraVendor != "cooke"){
        QMessageBox::critical(0, "Imagine", "Unsupported camera."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return 1;

    }

    if (!loadScript(QString::fromStdString(rig) + ".js")){
        return 1;
    }

    // preset.js
    const char* homedir = getenv("USERPROFILE");
    string filename = string(homedir) + "/preset.js";
    if (!tr2::sys::exists(tr2::sys::path(filename))){
        filename = "preset.js";
    }
    cout << "preset file is: " << filename << endl;
    if (!loadScript(QString::fromStdString(filename))){
        return 1;
    }

    //show splash windows and init positioner/daq/camera:
    //SEE: QSplashScreen class ref
    QPixmap pixmap(":/images/Resources/splash.jpg");
    QSplashScreen *splash = new QSplashScreen(pixmap);
    splash->show();

    /*QMessageBox::information(0, "Imagine",
          "Please raise microscope.");*/
    int align = Qt::AlignBottom | Qt::AlignLeft;
    Qt::GlobalColor col = Qt::red;
    splash->showMessage(QString("Initialize the %1 actuator ...").arg(positionerType), align, col);
    Positioner *pos = nullptr;
    if (positionerType == "volpiezo") pos = new VolPiezo(ainame, aoname);
//    else if (positionerType == "pi") pos = new Piezo_Controller;
//#ifndef _WIN64
//    else if (positionerType == "thor") pos = new Actuator_Controller;
//#endif
//    else if (positionerType == "dummy") pos = new DummyPiezo;
    else {
        QMessageBox::critical(0, "Imagine", "Unsupported positioner."
            , QMessageBox::Ok, QMessageBox::NoButton);

        return 1;
    }

    splash->showMessage(QString("Initialize the %1 daq ...").arg(daq), align, col);
    if (daq == "ni") {
        digOut = new NiDaqDo(doname);
    }
    else if (daq == "dummy"){
        digOut = new DummyDaqDo();
    }
    else {
        QMessageBox::critical(0, "Imagine", "Unsupported daq.",
            QMessageBox::Ok, QMessageBox::NoButton);
        return 1;
    }

    // check out many cameras are attached
    splash->showMessage(QString("Counting cameras..."), align, col);
    int nCams = 0;
    getCamCount(&nCams);

    // not sure why this is positioned here
    splash->showMessage(QString("qAp is thinking about life ..."), align, col);
    qApp->processEvents();

    // init the first (only?) camera.
    // pointer will be deleted in clean-up of its owning data_acq_thread.
    splash->showMessage(QString("Initializing (%1) camera 1 ...").arg(cameraVendor), align, col);
    Camera *cam1;
//    if (cameraVendor == "avt") cam1 = new AvtCamera;
//    else if (cameraVendor == "andor") cam1 = new AndorCamera;
//    else if (cameraVendor == "cooke") cam1 = new CookeCamera;
	if (cameraVendor == "cooke") cam1 = new CookeCamera;
    else {
        QMessageBox::critical(0, "Imagine", "Unsupported camera.", QMessageBox::Ok, QMessageBox::NoButton);
        return 1;
    }
    if (!cam1->init()){
        splash->showMessage("Failed to initialize camera 1.", align, col);
        QMessageBox::critical(splash, "Imagine", "Failed to initialize camera 1.",
        QMessageBox::Ok, QMessageBox::NoButton);
        return 1;
    }

    Camera *cam2 = nullptr;
    if (nCams > 1) {
        // init the second camera
        // sorry for the copy pasta. perhaps put this in a function later
        splash->showMessage(QString("Initializing (%1) camera 2 ...").arg(cameraVendor), align, col);
        
//        if (cameraVendor == "avt") cam2 = new AvtCamera;
//        else if (cameraVendor == "andor") cam2 = new AndorCamera;
//        else if (cameraVendor == "cooke") cam2 = new CookeCamera;
		if (cameraVendor == "cooke") cam2 = new CookeCamera;
        else {
            QMessageBox::critical(0, "Imagine", "Unsupported camera.", QMessageBox::Ok, QMessageBox::NoButton);
            return 1;
        }
        if (!cam2->init()){
            splash->showMessage("Failed to initialize camera 1.", align, col);
            QMessageBox::critical(splash, "Imagine", "Failed to initialize camera 1.",
                QMessageBox::Ok, QMessageBox::NoButton);
            return 1;
        }
    }

    // get rid of the status message
    delete splash;

    // init and show the ui
    a.initUI(cam1, pos, cam2);
    a.showUi();

    // go!
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    return a.exec();
}

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

#include "andor_g.hpp"
#include "avt_g.hpp"
#include "cooke.hpp"

#include "actuators/voltage/volpiezo.hpp"
#include "Piezo_Controller.hpp"
#include "Actuator_Controller.hpp"
#include "actuators/dummy/dummypiezo.hpp"
#include "ni_daq_g.hpp"
#include "dummy_daq.hpp"
#include "shutterctrl.h"


extern Camera* pCamera;
extern Positioner* pPositioner;
extern QString positionerType;
extern QScriptEngine* se;
extern DaqDo* digOut;
extern QString daq;
extern QString doname;
extern QString aoname;
extern QString ainame;
extern string rig;

bool loadScript(const QString &filename)
{
   if(!QFile::exists(filename)) {
      QMessageBox::critical(0, "Imagine", QString("couldn't find script %1.").arg(filename)
         , QMessageBox::Ok, QMessageBox::NoButton);
      return false;
   }
   QFile file(filename);
   if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::critical(0, "Imagine", "couldn't read imagine.js."
         , QMessageBox::Ok, QMessageBox::NoButton);
      return false;
   }
   QTextStream in(&file);
   QString jscode=in.readAll();
   QScriptProgram sp(jscode);
   QScriptValue jsobj=se->evaluate(sp);
   if(se->hasUncaughtException()){
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
   return 0==system(cmd.toLatin1());
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


int main(int argc, char *argv[])
{
   QApplication a(argc, argv);

   rig="hs-ocpi";
   if(argc==2) {
      rig=argv[1];
      cout<<"The rig is: "<<rig<<endl;
   }
   else {
      cout<<"The rig is default to: "<<rig<<endl;
   }

   se=new QScriptEngine();

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

   if(!loadScript(QString::fromStdString("imagine.js"))){
      return 1;  
   }
   QString cameraVendor=se->globalObject().property("camera").toString();
   positionerType=se->globalObject().property("positioner").toString();
   daq=se->globalObject().property("daq").toString();
   if (daq=="ni") {
     doname=se->globalObject().property("doname").toString();
     aoname=se->globalObject().property("aoname").toString();
     ainame=se->globalObject().property("ainame").toString();
   }
   cout<<"using "<<cameraVendor.toStdString()<<" camera, "
      <<positionerType.toStdString()<<" positioner, "
      <<daq.toStdString()<<" daq."<<endl;

   if(cameraVendor!="avt" && cameraVendor!="andor" && cameraVendor!="cooke"){
      QMessageBox::critical(0, "Imagine", "Unsupported camera."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;

   }

   if(!loadScript(QString::fromStdString(rig)+".js")){
      return 1;  
   }

   // preset.js
   const char* homedir=getenv("USERPROFILE");
   string filename=string(homedir)+"/preset.js";
   if(!tr2::sys::exists(tr2::sys::path(filename))){
      filename="preset.js";
   }
   cout<<"preset file is: "<<filename<<endl;
   if(!loadScript(QString::fromStdString(filename))){
      return 1;  
   }

   //show splash windows and init positioner/daq/camera:
   //SEE: QSplashScreen class ref
   QPixmap pixmap(":/images/Resources/splash.jpg");
   QSplashScreen *splash = new QSplashScreen(pixmap);
   splash->show();

   QMessageBox::information(0, "Imagine", 
         "Please raise microscope.");

   splash->showMessage(QString("Initialize the %1 actuator ...").arg(positionerType), 
      Qt::AlignLeft|Qt::AlignBottom, Qt::red);
   if(positionerType=="volpiezo") pPositioner=new VolPiezo(ainame,aoname);
   else if(positionerType=="pi") pPositioner=new Piezo_Controller;
#ifndef _WIN64
   else if(positionerType=="thor") pPositioner=new Actuator_Controller;
#endif
   else if(positionerType=="dummy") pPositioner=new DummyPiezo;
   else {
      QMessageBox::critical(0, "Imagine", "Unsupported positioner."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;
   }

   splash->showMessage(QString("Initialize the %1 daq ...").arg(daq), 
      Qt::AlignLeft|Qt::AlignBottom, Qt::red);
   if(daq=="ni") {
      digOut=new NiDaqDo(doname);
   }
   else if(daq=="dummy"){
      digOut=new DummyDaqDo();
   }
   else {
      QMessageBox::critical(0, "Imagine", "Unsupported daq."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;
   }

   splash->showMessage(QString("Initialize the %1 camera ...").arg(cameraVendor), 
      Qt::AlignLeft|Qt::AlignBottom, Qt::red);
   qApp->processEvents();
   if(cameraVendor=="avt") pCamera=new AvtCamera;
   else if(cameraVendor=="andor") pCamera=new AndorCamera;
   else if(cameraVendor=="cooke") pCamera=new CookeCamera;
   else {
      QMessageBox::critical(0, "Imagine", "Unsupported camera."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;
   }
   if(!pCamera->init()){
      splash->showMessage("Failed to initialize the camera.", 
         Qt::AlignLeft|Qt::AlignBottom, Qt::red);

      QMessageBox::critical(splash, "Imagine", "Failed to initialize the camera."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;
   }

   delete splash;

   Imagine w;
   w.show();
   a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
   return a.exec();
}

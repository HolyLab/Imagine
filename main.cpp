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

#include <QtGui/QApplication>
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

#include "volpiezo.hpp"
#include "Piezo_Controller.hpp"
#include "Actuator_Controller.hpp"


extern Camera* pCamera;
extern Positioner* pPositioner;
extern QScriptEngine* se;

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
   return 0==system(cmd.toAscii());
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

   se=new QScriptEngine();
   QScriptValue svRun = se->newFunction(runWrapper);
   se->globalObject().setProperty("system", svRun); 

   if(!loadScript(QString::fromStdString("imagine.js"))){
      return 1;  
   }
   QString cameraVendor=se->globalObject().property("camera").toString();
   QString positionerType=se->globalObject().property("positioner").toString();

   if(cameraVendor!="avt" && cameraVendor!="andor" && cameraVendor!="cooke"){
      QMessageBox::critical(0, "Imagine", "Unsupported camera."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;

   }

   if(!loadScript(cameraVendor+".js")){
      return 1;  
   }


   if(positionerType=="volpiezo") pPositioner=new VolPiezo;
   else if(positionerType=="pi") pPositioner=new Piezo_Controller;
   else if(positionerType=="thor") pPositioner=new Actuator_Controller;
   else {
      QMessageBox::critical(0, "Imagine", "Unsupported positioner."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;
   }

   if(cameraVendor=="avt") pCamera=new AvtCamera;
   else if(cameraVendor=="andor") pCamera=new AndorCamera;
   else if(cameraVendor=="cooke") pCamera=new CookeCamera;

   //show splash windows and init camera:
   //see: QSplashScreen class ref
   QPixmap pixmap("splash.jpg");
   QSplashScreen *splash = new QSplashScreen(pixmap);
   splash->show();
   splash->showMessage(QString("Initialize the %1 camera ...").arg(QString::fromStdString(pCamera->vendor)), 
      Qt::AlignLeft|Qt::AlignBottom, Qt::red);

   //qApp->processEvents();
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

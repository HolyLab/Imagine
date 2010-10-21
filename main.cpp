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

#include "imagine.h"

#include "andor_g.hpp"
#include "avt_g.hpp"



extern Camera* pCamera;



int main(int argc, char *argv[])
{
   QApplication a(argc, argv);

   QString cameraVendor="avt";
   if(argc>1) cameraVendor=argv[1];
   if(cameraVendor!="avt" && cameraVendor!="andor"){
      QMessageBox::critical(0, "Imagine", "please specify the camera (avt or andor) on the command line."
         , QMessageBox::Ok, QMessageBox::NoButton);

      return 1;

   }

   if(cameraVendor=="avt") pCamera=new AvtCamera;
   else if(cameraVendor=="andor") pCamera=new AndorCamera;
   else {
      //todo: error msg
      return 1;
   }

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

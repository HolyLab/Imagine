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


//todo: 
extern AndorCamera camera;



int main(int argc, char *argv[])
{
   QApplication a(argc, argv);

   //show splash windows and init camera:
   //see: QSplashScreen class ref
   QPixmap pixmap("splash.jpg");
   QSplashScreen *splash = new QSplashScreen(pixmap);
   splash->show();
   splash->showMessage("Initialize the Andor camera ...", 
      Qt::AlignLeft|Qt::AlignBottom, Qt::red);
   //qApp->processEvents();
   if(!camera.init()){
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

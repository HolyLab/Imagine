Imagine 1.0
=======================

Copyright (C) 2005-2008 Timothy E. Holy and Zhongsheng Guo 
   All rights reserved.
Author: All code authored by Zhongsheng Guo.
License: This program may be used under the terms of the GNU General Public
   License version 2.0 as published by the Free Software Foundation
   and appearing at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   If this license is not appropriate for your use, please
   contact holy@wustl.edu or zsguo@pcg.wustl.edu for other license(s).

This program is provided WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

----------------------------------------------------------------------------
Qwt
---
The histogram in this program is based on the work of the Qwt project 
(http://qwt.sf.net).

----------------------------------------------------------------------------
Additional files for compilation
--------------------------------
   You need 6 more files which are not included in the source code package:
NIDAQmx.h and NIDAQmx.lib are from National Instrument's DAQ SDK;
Atmcd32d.h and Atmcd32m.lib are from Andor's Camera SDK;
histogram_item.cpp and histogram_item.h are from Qwt's sample code.
   This program uses Qt toolkit for the GUI (http://trolltech.com/products/qt).

----------------------------------------------------------------------------
Requirement
-----------
   The hardware controlled by this program is an Andor CCD camera and a National
Instrument multi-function DAQ card.
   This program is for Windows XP only. Before you can compile and run it,
you have to install Andor Camera's SDK (2.77+), National Instrument's DAQ SDK
(8.6+), Qt (4.3+), and Qwt (5.0.1+). You also need Microsoft Visual Studio 2005
or 2008 to compile the code.

----------------------------------------------------------------------------
How to compile, install, and run
--------------------------------
   Double click imagine.vcproj to open the project in Visual Studio.
You may need to change the project's settings to reflect where you installed
Qt.
   Once compiled, copy imagine.exe, logo.jpg, splash.jpg, atmcd32d.dll,
and qwt5.dll to a directory then you can double-click imagine.exe to run it
from there.

----------------------------------------------------------------------------
Usage
-----
   Current we don't have a user's guide yet. The program should be very
straightforward to use.

   In "Config and control" panel, you set up the parameters for file names,
camera, stimulus, and piezo control; when done, click "apply" button to
use the parameters.
   Once you set up the parameters, you can use menu items under Menu 
"Acquisition" to start & save acquisition, or start in live mode, or stop
either acquisition or the live mode.
   In "Histogram" panel, you can see the statistics of images.

   Under Menu "Hardware", you can open/close the shutter, monitor/adjust CCD
camera's temperature, or adjust the camera's heatsink fan speed.
   Under Menu "View", you can control the visibility of UI components,
and how acquired images should be displayed.

   At the bottom left corner, there is a log window where you can find
useful messages while running the program.
----------------------------------------------------------------------------

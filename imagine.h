/*-------------------------------------------------------------------------
** Copyright (C) 2005-2012 Timothy E. Holy and Zhongsheng Guo
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

#ifndef IMAGINE_H
#define IMAGINE_H

#include <QtGui/QMainWindow>

class QScrollArea;
class QString;
class QwtPlot;
class QwtPlotCurve;
class HistogramItem;
class CurveData;

#include "ui_imagine.h"

#include "data_acq_thread.hpp"
#include "andor_g.hpp"

enum ImagineStatus {eIdle=0, eRunning, eStopping};
enum ImagineAction {eNoAction=0, eAcqAndSave, eLive};

struct PiezoUiParam {
   bool valid;
   double start, stop, moveto;

   PiezoUiParam(){valid=false;}
};

class Imagine : public QMainWindow
{
    Q_OBJECT

public:
    Imagine(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~Imagine();

protected:
   void closeEvent(QCloseEvent *event);


private:
    Ui::ImagineClass ui;
    QScrollArea* scrollArea;
    DataAcqThread dataAcqThread;
    int minPixelValue, maxPixelValue;
    QwtPlot *histPlot;
    HistogramItem *histogram;
    QwtPlot *intenPlot;
    QwtPlotCurve *intenCurve;
    CurveData *intenCurveData;
    vector<PiezoUiParam> piezoUiParams;

    void calcMinMaxValues(Camera::PixelValue * frame, int imageW, int imageH);
    void updateStatus(ImagineStatus newStatus, ImagineAction newAction);
    double zpos2voltage(double um); //z-position to piezo voltage
    void updateImage();
    void updateHist(const AndorCamera::PixelValue * frame, 
       const int imageW, const int imageH);
    void updateIntenCurve(const AndorCamera::PixelValue * frame, 
       const int imageW, const int imageH, const int frameIdx);

    bool checkRoi();

private slots:
        void on_actionHeatsinkFan_triggered();
        void on_actionExit_triggered();
        void on_btnFastDecPosAndMove_clicked();
        void on_btnDecPosAndMove_clicked();
        void on_btnIncPosAndMove_clicked();
        void on_btnFastIncPosAndMove_clicked();
        void on_btnMovePiezo_clicked();
        void on_btnRefreshPos_clicked();
        void on_btnSetCurPosAsStop_clicked();
        void on_btnSetCurPosAsStart_clicked();
        void on_btnSelectFile_clicked();
        void on_btnOpenStimFile_clicked();
        void on_btnApply_clicked();
        void on_btnFullChipSize_clicked();
        void on_btnUseZoomWindow_clicked();
        void on_actionStartAcqAndSave_triggered();
        void on_actionStartLive_triggered();
        void on_actionStop_triggered();
        void on_actionOpenShutter_triggered();
        void on_actionCloseShutter_triggered();
        void on_actionTemperature_triggered();
        void on_actionAutoScaleOnFirstFrame_triggered();
        void on_actionStimuli_triggered();
        void on_actionConfig_triggered();
        void on_actionLog_triggered();
        void on_actionDisplayFullImage_triggered();
        //void on_actionColorizeSaturatedPixels_triggered();
        void on_tabWidgetCfg_currentChanged(int index);
        void on_comboBoxAxis_currentIndexChanged(int index);
        void on_doubleSpinBoxCurPos_valueChanged(double newValue);
        void on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue);
        void on_cbAutoSetPiezoTravelBackTime_stateChanged ( int state ) ;
        void readPiezoCurPos();

        void updateStatus(const QString &str);
        void appendLog(const QString& msg);
        void updateDisplay(const QByteArray &data16, long idx, int imageW, int imageH);
        void zoom_onMousePressed(QMouseEvent*);
        void zoom_onMouseMoved(QMouseEvent*);
        void zoom_onMouseReleased(QMouseEvent*);

};

#endif // IMAGINE_H

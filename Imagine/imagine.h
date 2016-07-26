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

#include <QtWidgets/QMainWindow>
#include <QTimer>


class QScrollArea;
class QString;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotHistogramItem;
//class CurveData;

#include "ui_imagine.h"

#include "data_acq_thread.hpp"
//#include "andor_g.hpp"
#include "positioner.hpp"
#include "timer_g.hpp"
#include "Pixmapper.h"
#include <qwt_plot_histogram.h>

enum ImagineStatus { eIdle = 0, eRunning, eStopping };
enum ImagineAction { eNoAction = 0, eAcqAndSave, eLive };

struct PiezoUiParam {
    bool valid;
    double start, stop, moveto;

    PiezoUiParam(){ valid = false; }
};

class Imagine : public QMainWindow
{
    Q_OBJECT
    QThread pixmapperThread;
public:
    Imagine(Camera *cam, Positioner *pos = nullptr,
        QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Imagine();
    DataAcqThread dataAcqThread;
    Timer_g gTimer;
    
    Pixmapper *pixmapper = nullptr;
    bool isPixmapping = false;
    QPixmap pixmap;
    QImage image;
    QByteArray lastRawImg;
    int lastImgH = 0;
    int lastImgW = 0;
    double factor = 0.0;

    // display coords in the unit of original image
    int L = -1;
    int W;
    int H;
    int T;
protected:
    void closeEvent(QCloseEvent *event);


private:
    Ui::ImagineClass ui;
    QScrollArea* scrollArea;
    int minPixelValue, maxPixelValue;
    int minPixelValueByUser, maxPixelValueByUser;
    int nContinousBlackFrames;
    QwtPlot *histPlot;
    QwtPlotHistogram *histogram;
    QwtPlot *intenPlot;
    QwtPlotCurve *intenCurve;
    //    CurveData *intenCurveData;
    vector<PiezoUiParam> piezoUiParams;
    bool modified;
    bool paramOK;

    void calcMinMaxValues(Camera::PixelValue * frame, int imageW, int imageH);
    void updateStatus(ImagineStatus newStatus, ImagineAction newAction);
    double zpos2voltage(double um); //z-position to piezo voltage
    void updateImage();
	void updateHist(const Camera::PixelValue * frame,
        const int imageW, const int imageH);
    void updateIntenCurve(const Camera::PixelValue * frame,
        const int imageW, const int imageH, const int frameIdx);

    bool checkRoi();
    bool loadPreset();
    void preparePlots();
    QPoint calcPos(const QPoint& pos);

private slots:
//    void on_actionHeatsinkFan_triggered();
    void on_actionExit_triggered();
    void on_btnFastDecPosAndMove_clicked();
    void on_btnDecPosAndMove_clicked();
    void on_btnIncPosAndMove_clicked();
    void on_btnFastIncPosAndMove_clicked();
    void on_btnMovePiezo_clicked();
    void on_btnMoveToStartPos_clicked();
    void on_btnMoveToStopPos_clicked();
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
    //void on_actionTemperature_triggered();
    void on_actionAutoScaleOnFirstFrame_triggered();
    void on_actionContrastMin_triggered();
    void on_actionContrastMax_triggered();
    void on_actionStimuli_triggered();
    void on_actionConfig_triggered();
    void on_actionLog_triggered();
    void on_actionIntensity_triggered();
    void on_actionHistogram_triggered();
    void on_actionDisplayFullImage_triggered();
    //void on_actionColorizeSaturatedPixels_triggered();
    void on_tabWidgetCfg_currentChanged(int index);
    void on_comboBoxAxis_currentIndexChanged(int index);
    void on_doubleSpinBoxCurPos_valueChanged(double newValue);
    void on_spinBoxSpinboxSteps_valueChanged(int newValue);
    void on_spinBoxNumOfDecimalDigits_valueChanged(int newValue);
    void on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue);
    void on_cbAutoSetPiezoTravelBackTime_stateChanged(int state);
    void readPiezoCurPos();
    void onModified();

    void updateStatus(const QString &str);
    void appendLog(const QString& msg);
    void updateDisplay(const QByteArray &data16, long idx, int imageW, int imageH);
    void zoom_onMousePressed(QMouseEvent*);
    void zoom_onMouseMoved(QMouseEvent*);
    void zoom_onMouseReleased(QMouseEvent*);

public slots:
    // handle pixmap of recently acquired frame
    void handlePixmap(const QPixmap &pxmp, const QImage &img);
signals:
    void makePixmap(const QByteArray &ba, const int imageW, const int imageH,
        const double scaleFactor, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal, const int maxPixVal, bool colorizeSat);
};

#endif // IMAGINE_H

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
#include <QThread>
#include <QSettings>


class QScrollArea;
class QString;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotHistogramItem;
class CurveData;

#include "ui_imagine.h"

#include "data_acq_thread.hpp"
//#include "andor_g.hpp"
#include "positioner.hpp"
#include "Pixmapper.h"
#include <qwt_plot_histogram.h>
#include "laserctrl.h"
#include "piezoctrl.h"
#include "waveform.h"

#define READ_STRING_SETTING(prefs, var, emptyValue)\
  ui.##var->setText( prefs.value(#var).toString() );\
  if (ui.##var->text().isEmpty() == true) { ui.##var->setText(emptyValue); }

#define READ_SETTING(prefs, var, ok, temp, default, type)\
  ok = false;\
  temp = prefs.value(#var).to##type(&ok);\
  if (false == ok) {temp = default;}\
  ui.##var->setValue(temp);

#define READ_VALUE(prefs, var, ok, temp, default, type)\
  ok = false;\
  temp = prefs.value(#var).to##type(&ok);\
  if (false == ok) {temp = default;}\
  ui.##var = temp;

#define WRITE_STRING_SETTING(prefs, var)\
  prefs.setValue(#var , ui.##var->text());

#define WRITE_SETTING(prefs, var)\
  prefs.setValue(#var, ui.##var->value());

#define WRITE_COMBO_SETTING(prefs, var)\
  prefs.setValue(#var, ui.##var->currentIndex());

#define READ_COMBO_SETTING(prefs, var, emptyValue)\
  { QString s = prefs.value(#var).toString();\
  if (s.isEmpty() == false) {\
    bool ok = false; int bb = prefs.value(#var).toInt(&ok);\
  ui.##var->setCurrentIndex(bb); } else { ui.##var->setCurrentIndex(emptyValue); } }

#define READ_BOOL_SETTING(prefs, var, emptyValue)\
  { QString s = prefs.value(#var).toString();\
  if (s.isEmpty() == false) {\
    bool bb = prefs.value(#var).toBool();\
  ui.##var->setChecked(bb); } else { ui.##var->setChecked(emptyValue); } }

#define WRITE_BOOL_SETTING(prefs, var, b)\
    prefs.setValue(#var, (b) );

#define WRITE_CHECKBOX_SETTING(prefs, var)\
    prefs.setValue(#var, ui.##var->isChecked() );

#define WRITE_VALUE(prefs, var)\
    prefs.setValue(#var, ui.##var);

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
    QThread laserCtrlThread;
    QThread piezoCtrlThread;
public:
    Imagine(Camera *cam, Positioner *pos = NULL, Laser *laser = NULL, Imagine *mImagine = NULL,
        QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Imagine();
    DataAcqThread *dataAcqThread;
    
	Imagine *masterImagine; //Positioner and stimulus control / interface is restricted to only the master instance
    Pixmapper *pixmapper = nullptr;
    bool isPixmapping = false;
    QPixmap pixmap;
    QImage image;
    QByteArray lastRawImg;
    int lastImgH = 0;
    int lastImgW = 0;
    double factor = 0.0;
    int maxROIHSize;
    int maxROIVSize;
    int roiStepsHor;
    bool isUsingSoftROI = false;

    // for laser control
    LaserCtrlSerial *laserCtrlSerial = nullptr;
    // for piezo controller setup
    PiezoCtrlSerial *piezoCtrlSerial = nullptr;
    int maxLaserFreq;

    // display coords in the unit of original image
    int L = -1;
    int W;
    int H;
    int T;
protected:
    void closeEvent(QCloseEvent *event);


private:
    QScrollArea* scrollArea;
    int minPixelValue, maxPixelValue;
    int minPixelValueByUser, maxPixelValueByUser;
    int nContinousBlackFrames;
    QwtPlot *histPlot;
    QwtPlotHistogram *histogram;
    QwtPlot *intenPlot;
    QwtPlotCurve *intenCurve;
    CurveData *intenCurveData;
    QwtPlot *conWavPlot;
    QwtPlotCurve *conPiezoCurve;
    CurveData *conPiezoCurveData = NULL;
    QwtPlotCurve *conShutterCurve;
    CurveData *conShutterCurveData = NULL;
    QwtPlotCurve *piezoSpeedCurve;
    CurveData *piezoSpeedCurveData = NULL;
    QwtPlotCurve *conLaserCurve;
    CurveData *conLaserCurveData = NULL;
    QwtPlotCurve *conTTL1Curve;
    CurveData *conTTL1CurveData = NULL;
    QwtPlotCurve *conTTL2Curve;
    CurveData *conTTL2CurveData = NULL;
    QwtPlot *conReadWavPlot;
    QwtPlotCurve *conReadPiezoCurve;
    QwtPlotCurve *conReadStimuliCurve;
    QwtPlotCurve *conReadCameraCurve;
    QwtPlotCurve *conReadHeartbeatCurve;
    CurveData *conReadPiezoCurveData = NULL;
    CurveData *conReadStimuliCurveData = NULL;
    CurveData *conReadCameraCurveData = NULL;
    CurveData *conReadHeartbeatCurveData = NULL;
    vector<PiezoUiParam> piezoUiParams;
    QString m_OpenDialogLastDirectory;
    WaveData *waveData;
    ControlWaveform *conWaveData = NULL;

    bool modified;
    bool paramOK;
    int numLaserShutters = 0;
    int laserShutterIndex[8] = { 0, };
    QString file=""; // config file name
    void calcMinMaxValues(Camera::PixelValue * frame, int imageW, int imageH);
    void updateStatus(ImagineStatus newStatus, ImagineAction newAction);
    double zpos2voltage(double um); //z-position to piezo voltage
    void updateImage();
	void updateHist(const Camera::PixelValue * frame,
        const int imageW, const int imageH);
    void updateIntenCurve(const Camera::PixelValue * frame,
        const int imageW, const int imageH, const int frameIdx);
    template<class Type> void setCurveData(CurveData *curveData, QwtPlotCurve *curve,
        std::vector<Type> &wave, int start, int end, int yoffset);
    template<class Type> void setCurveData(CurveData *curveData, QwtPlotCurve *curve,
        QVector<Type> &wave, int start, int end, int factor, int amplitude, int yoffset);
    bool checkRoi();
    bool loadPreset();
    void preparePlots();
    QPoint calcPos(const QPoint& pos);

    // for laser control from this line
    void changeLaserShutters(void);
    void changeLaserTrans(bool isAotf, int line);
    // for laser control until this line
    void writeSettings(QString file);
    void readSettings(QString file);
    void writeComments(QString file);
    void readComments(QString file);
    void updateConWave(const int frameIdx, const int value);
    void ControlFileLoad(QByteArray &data1, QByteArray &data2);
    void updateControlWaveform_old(QString fn);
    void readControlWaveform(QString fn);
    void updateControlWaveform(int leftEnd, int rightEnd);
    void updataSpeedData(int newValue);
    void updataSpeedData(int newValue, int start, int end);
    bool convertJASONtoWave(vector<double> &wave, QJsonArray &jsonWave, int num);
    bool convertJASONtoPulse(vector<int> &pulse, QJsonArray &jsonPulse, int num);
    bool waveformValidityCheck(void);

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
    void on_spinBoxHstart_editingFinished();
    void on_spinBoxHend_editingFinished();
    void on_spinBoxVstart_valueChanged(int newValue);
    void on_spinBoxVend_valueChanged(int newValue);
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
	void set_safe_piezo_params();
    void on_doubleSpinBoxBoxIdleTimeBtwnStacks_valueChanged(double newValue);
	void on_doubleSpinBoxPiezoTravelBackTime_valueChanged(double newValue);
	void on_doubleSpinBoxStopPos_valueChanged(double newValue);
	void on_doubleSpinBoxStartPos_valueChanged(double newValue);
    void on_cbAutoSetPiezoTravelBackTime_stateChanged(int state);
    void readPiezoCurPos();
    void onModified();

    void updateStatus(const QString &str);
    void appendLog(const QString& msg);
    void updateDisplay(const QByteArray &data16, long idx, int imageW, int imageH);
    void zoom_onMousePressed(QMouseEvent*);
    void zoom_onMouseMoved(QMouseEvent*);
    void zoom_onMouseReleased(QMouseEvent*);

    // for laser control from this line
    void on_groupBoxLaser_clicked(bool checked);
    void on_btnOpenPort_clicked();
    void on_btnClosePort_clicked();
    void on_cbLine1_clicked(bool checked);
    void on_cbLine2_clicked(bool checked);
    void on_cbLine3_clicked(bool checked);
    void on_cbLine4_clicked(bool checked);
    void on_cbLine5_clicked(bool checked);
    void on_cbLine6_clicked(bool checked);
    void on_cbLine7_clicked(bool checked);
    void on_cbLine8_clicked(bool checked);
    void on_aotfLine1_valueChanged();
    void on_aotfLine2_valueChanged();
    void on_aotfLine3_valueChanged();
    void on_aotfLine4_valueChanged();
    void on_aotfLine5_valueChanged();
    void on_aotfLine6_valueChanged();
    void on_aotfLine7_valueChanged();
    void on_aotfLine8_valueChanged();
    void displayShutterStatus(int status);
    void displayTransStatus(bool isaotf, int line, int status);
    void displayLaserGUI(int numLines, int *laserIndex, int *wavelength);
    void on_doubleSpinBox_aotfLine1_valueChanged();
    void on_doubleSpinBox_aotfLine2_valueChanged();
    void on_doubleSpinBox_aotfLine3_valueChanged();
    void on_doubleSpinBox_aotfLine4_valueChanged();
    void on_doubleSpinBox_aotfLine5_valueChanged();
    void on_doubleSpinBox_aotfLine6_valueChanged();
    void on_doubleSpinBox_aotfLine7_valueChanged();
    void on_doubleSpinBox_aotfLine8_valueChanged();
    // for laser control until this line
    void on_actionSave_Configuration_triggered();
    void on_actionLoad_Configuration_triggered();
    // for piezo controller setup from this line
    void displayPiezoCtrlStatus(QByteArray rx);
    void displayPiezoCtrlValue(QByteArray rx);
    void on_btnSendSerialCmd_clicked();
    void on_btnPzOpenPort_clicked();
    void on_btnPzClosePort_clicked();
    // for piezo controller setup until this line
    void on_btnConWavOpen_clicked();
    void on_btnReadWavOpen_clicked();
    void on_cbPiezoReadWav_clicked(bool checked);
    void on_cbStimuliReadWav_clicked(bool checked);
    void on_cbCameraReadWav_clicked(bool checked);
    void on_cbHeartReadWav_clicked(bool checked);
    void on_cbPiezoConWav_clicked(bool checked);
    void on_cbLaserConWav_clicked(bool checked);
    void on_cbCameraConWav_clicked(bool checked);
    void on_cbTTL1ConWav_clicked(bool checked);
    void on_cbTTL2ConWav_clicked(bool checked);
    void on_spinBoxPiezoSampleRate_valueChanged(int newValue);
    void on_sbWavDsplyRight_valueChanged(int value);
    void on_sbWavDsplyLeft_valueChanged(int value);
    void on_btnWavDsplyReset_clicked();

public:
    Ui::ImagineClass ui;
public slots:
    // handle pixmap of recently acquired frame
    void handlePixmap(const QPixmap &pxmp, const QImage &img);
signals:
    void makePixmap(const QByteArray &ba, const int imageW, const int imageH,
        const double scaleFactor, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal, const int maxPixVal, bool colorizeSat);

    // for laser control from this line
    void openLaserSerialPort(QString portName);
    void closeLaserSerialPort(void);
    void getLaserShutterStatus(void);
    void setLaserShutter(int line, bool isOpen);
    void setLaserShutters(int status);
    void getLaserTransStatus(bool isAotf, int line);
    void setLaserTrans(bool isAotf, int line, int value);
    void getLaserLineSetupStatus(void);
    // for laser control until this line
    // for piezo controller setup from this line
    void openPiezoCtrlSerialPort(QString portName);
    void closePiezoCtrlSerialPort(void);
    void sendPiezoCtrlCmd(QString cmd);
    // for piezo controller setup until this line
};

#endif // IMAGINE_H

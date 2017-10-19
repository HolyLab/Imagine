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
//#include <QTimer>
#include <QThread>
#include <QSettings>
#include <QSplitter>

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
#include "script.h"

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

#define WRITE_VARIABLE_SETTING(prefs, var)\
  prefs.setValue(#var, var);

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

#define READ_CHECKBOX_SETTING(prefs, var, emptyValue)\
  { QString s = prefs.value(#var).toString();\
  if (s.isEmpty() == false) {\
    bool bb = prefs.value(#var).toBool();\
  ui.##var->setChecked(bb); } else { ui.##var->setChecked(emptyValue); } }

#define READ_BOOL_SETTING(prefs, var, emptyValue)\
  { QString s = prefs.value(#var).toString();\
  if (s.isEmpty() == false) {\
    ##var = prefs.value(#var).toBool();}\
  else { ##var = emptyValue; }}

#define WRITE_BOOL_SETTING(prefs, var)\
    prefs.setValue(#var, (var) );

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

class ImagineData
{
public:
    // data for cam file
    bool valid = false;
    int version;
    QString appVersion;
    QString dateNtime;
    QString rig;
    QString stimlabelList;
    int imgWidth;
    int imgHeight;
    int nStacks;
    int framesPerStack;
    double exposure;
    QString pixelOrder;
    QString binning;
    QString dataType;
    int bytesPerPixel;
    int imageSizePixels;
    int imageSizeBytes;

    // data for display control
    bool camValid = false;
    bool enable = false;
    int currentFrameIdx;
    int currentStackIdx;
    int stackPlaySpeed = 0;
    int framePlaySpeed = 0;
    QFile camFile;
    QByteArray camImage;
};

typedef struct {
    unsigned short red;
    unsigned short green;
    unsigned short blue;
} ColorPixelValue;

class QProcess;

class Imagine : public QMainWindow
{
    Q_OBJECT
    QThread pixmapperThread;
    QThread laserCtrlThread;
    QThread piezoCtrlThread;
    QThread imagePlayThread;
    QThread scriptThread;
public:
    Imagine(QString rig, Camera *cam, Positioner *pos = NULL, Laser *laser = NULL,
        Imagine *mImagine = NULL, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Imagine();
    DataAcqThread *dataAcqThread;
    ImagineScript *imagineScript = nullptr;

    Ui::ImagineClass ui;
    QString rig;
    Imagine *masterImagine = nullptr; //Positioner and stimulus control / interface is restricted to only the master instance
    Imagine *slaveImagine = nullptr;
    Pixmapper *pixmapper = nullptr;
    ImagePlay *imagePlay = nullptr;
    bool isPixmapping = false;
    bool isUpdatingDisplay = false;
    bool masterImageReady = false;
    bool slaveImageReady = false;
    QPixmap pixmap;
    QImage image;
    int updateMethod;
    QByteArray* pLastRawImg;
    double factor = 0.0;
    double factor2 = 0.0;
    int maxROIHSize;
    int maxROIVSize;
    int roiStepsHor;
    bool isUsingSoftROI = false;
    ImagineData img1, img2;
    int imgWidth, imgHeight;
    int imgFramesPerStack, imgNStacks;
    int imgFrameIdx, imgStackIdx;
    QColor img1Color, img2Color;
    int alpha;
    ImagineStatus curStatus;

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

    void setSlaveWindow(Imagine *sImagine);
    void setROIMinMaxSize();

protected:
    void closeEvent(QCloseEvent *event);

private:
    QScrollArea* scrollArea;
    int* pMinPixelValue;
    int* pMaxPixelValue;
    int minPixelValue, maxPixelValue;
    int minPixelValue2, maxPixelValue2;
    int minPixelValueByUser, maxPixelValueByUser;
    int minPixelValueByUser2, maxPixelValueByUser2;
    int nContinousBlackFrames;
    QwtPlot *histPlot;
    QwtPlotHistogram *histogram, *histogram2;
    QwtPlot *intenPlot;
    QwtPlotCurve *intenCurve;
    CurveData *intenCurveData;
    QwtPlot *conWavPlot;
    QwtPlotCurve *piezoSpeedCurve;
    QwtPlot *conReadWavPlot;
    int numAoCurve = 2, numDoCurve = 6;
    QVector<QwtPlotCurve *> outCurve;
    QVector<CurveData *> outCurveData;
    QVector<QCheckBox*> cbAoDo;
    QVector<QComboBox*> comboBoxAoDo;
    int numAiCurve = 2, numDiCurve = 7;
    QVector<QwtPlotCurve *> inCurve;
    int numAiCurveData, numDiCurveData;
    QVector<CurveData *> inCurveData;
    QVector<QCheckBox*> cbAiDi;
    QVector<QComboBox*> comboBoxAiDi;
    vector<PiezoUiParam> piezoUiParams;
    AiWaveform *aiWaveData = NULL;
    DiWaveform *diWaveData = NULL;
    SampleIdx dsplyTotalSampleNum;
    double aiDiMaxy;
    double aiDiMiny;
    double aoDoMaxy;
    double aoDoMiny;
    double aiAmplitude;
    double aoAmplitude;
    QString m_OpenDialogLastDirectory;
    QString m_OpenConWaveDialogLastDirectory;
    QString wavFilename;
    bool modified;
    bool paramOK;
    int numLaserShutters = 0;
    int laserShutterIndex[8] = { 0, };
    QString file=""; // config file name
    QString imageFilename = "";
    bool isApplyCommander = false;
    bool isRecordCommander = false;
    bool reqFromScript = false;
    uInt32 laserTTLSig = 0;
    QProcess *proc = NULL;
    bool masterLaserShutter = false;

    void calcMinMaxValues(Camera::PixelValue * frame, int *min, int *max, int imageW, int imageH);
    void calcMinMaxValues(Camera::PixelValue * frame1, Camera::PixelValue * frame2, int imageW, int imageH);
    void updateStatus(ImagineStatus newStatus, ImagineAction newAction);
    void updateImage(int updateMethod, bool isForce);
    void updateHist(QwtPlotHistogram *histogram, const Camera::PixelValue * frame,
        const int imageW, const int imageH, double &maxcount, unsigned int &maxintensity);
    void updateIntenCurve(const Camera::PixelValue * frame,
        const int imageW, const int imageH, const int frameIdx);
    template<class Type> void setCurveData(CurveData *curveData, QwtPlotCurve *curve,
        QVector<Type> &wave, int start, int end, int factor, double amplitude, double yoffset);
    bool checkRoi();
    bool loadPreset();
    void preparePlots();
    QPoint calcPos(const QPoint& pos);

    // for laser control from this line
    void changeLaserShutters(void);
    void changeLaserTrans(bool isAotf, int line);
    void changeLaserShuttersTTL(int line, bool onOff);
    // for laser control until this line
    void writeSettings(QString file);
    bool readSettings(QString file);
    void writeComments(QString file);
    void readComments(QString file);
    bool readControlWaveformFile(QString fn);
    void updateControlWaveform(SampleIdx leftEnd, SampleIdx rightEnd);
    void updatePiezoSpeedCurve(SampleIdx leftEnd, SampleIdx rightEnd);
    void updateAiDiWaveform(SampleIdx leftEnd, SampleIdx rightEnd);
    void updataSpeedData(CurveData *curveData, int newValue, int start, int end);
    bool waveformValidityCheck(void);
    bool loadConWavDataAndPlot(SampleIdx leftEnd, SampleIdx rightEnd, int curveIdx);
    bool loadAiDiWavDataAndPlot(SampleIdx leftEnd, SampleIdx rightEnd, int curveIdx);
    bool readImagineFile(QString file, ImagineData &img);
    bool openAndReadCamFile(QString filename, ImagineData &img);
    bool setupPlayCamParam(ImagineData &img);
    bool readCamFileImages();
    void readCamFileImagesAndUpdate();
    void updateCameraImage();
    void reconfigDisplayTab();
    void holdDisplayCamFile();
    void stopDisplayCamFile();
    void applyImgColor(QWidget *widget, QColor color);
    void setStackIndexSpeed(int speed1, int speed2);
    void setFrameIndexSpeed(int speed1, int speed2);
    bool compareDimensions();
    void setupDimensions(int stacks, int frames, int width, int height);
    void showOutCurve(int idx, bool checked);
    void showInCurve(int idx, bool checked);
    bool applySetting();
    void startAcqAndSave();
    void startLive();
    void stopAcqOrLive();
    bool duplicateParameters(Ui_ImagineClass* destUi);
    void scriptStopRecord();
    void scriptApplyAndReport(bool preRetVal);
    void scriptJustReport(bool preRetVal);
    bool readConfigFiles(const QString &file1, const QString &file2);
    bool outputFileValidCheck();
    void clearStimulus();
    bool readAndApplyStimulusFile(QString stimFilename);
    void clearConWavPlot();
    void rearrangeTabWindow();
    void displayConWaveData();
    void enableMismatchCorrection(QByteArray &img1, QByteArray &img2, int width, int height);
    bool autoFindMismatchParameters(QByteArray &img1, QByteArray &img2, int width, int height);
    void displayImageUpdate(void);
    void calHomogeneousTramsformMatrix();
    bool outputFileExistCheckNDelete(QVector<QString> &filenames);
    void applyIndividualLasers();

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
    void on_actionContrastMin1_triggered();
    void on_actionContrastMax1_triggered();
    void on_actionContrastMin2_triggered();
    void on_actionContrastMax2_triggered();
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
    void on_doubleSpinBoxExpTime_valueChanged();
    void on_sbObjectiveLens_valueChanged(int newValue);

    void updateStatus(const QString &str);
    void appendLog(const QString& msg);
    void updateLiveImagePlay(const QByteArray &data16, long idx, int imageW, int imageH);
    void updateSlaveLiveImagePlay(const QByteArray &data16, long idx, int imageW, int imageH);
    void updateDisplay(long idx, int updateMethod);
    void updateDisplayColor(long idx);
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
    void on_cbLineTTL1_clicked(bool checked);
    void on_cbLineTTL2_clicked(bool checked);
    void on_cbLineTTL3_clicked(bool checked);
    void on_cbLineTTL4_clicked(bool checked);
    void on_cbLineTTL5_clicked(bool checked);
    void on_cbLineTTL6_clicked(bool checked);
    void on_cbLineTTL7_clicked(bool checked);
    void on_cbLineTTL8_clicked(bool checked);
    void on_cbDualOutSW1_clicked(bool checked);
    void on_cbDualOutSW2_clicked(bool checked);
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
    // Ai, Di read waveform
    void on_btnReadAiWavOpen_clicked();
    void on_cbAI0Wav_clicked(bool checked);
    void on_cbAI1Wav_clicked(bool checked);
    void on_cbDI0Wav_clicked(bool checked);
    void on_cbDI1Wav_clicked(bool checked);
    void on_cbDI2Wav_clicked(bool checked);
    void on_cbDI3Wav_clicked(bool checked);
    void on_cbDI4Wav_clicked(bool checked);
    void on_cbDI5Wav_clicked(bool checked);
    void on_cbDI6Wav_clicked(bool checked);
    void on_comboBoxAI0_currentIndexChanged(int index);
    void on_comboBoxAI1_currentIndexChanged(int index);
    void on_comboBoxDI0_currentIndexChanged(int index);
    void on_comboBoxDI1_currentIndexChanged(int index);
    void on_comboBoxDI2_currentIndexChanged(int index);
    void on_comboBoxDI3_currentIndexChanged(int index);
    void on_comboBoxDI4_currentIndexChanged(int index);
    void on_comboBoxDI5_currentIndexChanged(int index);
    void on_comboBoxDI6_currentIndexChanged(int index);
    // Waveform control
    void on_btnConWavOpen_clicked();
    void on_cbWaveformEnable_clicked(bool state);
    void on_cbAO0Wav_clicked(bool checked);
    void on_cbAO1Wav_clicked(bool checked);
    void on_cbDO0Wav_clicked(bool checked);
    void on_cbDO1Wav_clicked(bool checked);
    void on_cbDO2Wav_clicked(bool checked);
    void on_cbDO3Wav_clicked(bool checked);
    void on_cbDO4Wav_clicked(bool checked);
    void on_cbDO5Wav_clicked(bool checked);
    void on_sbWavDsplyRight_valueChanged(int value);
    void on_sbWavDsplyLeft_valueChanged(int value);
    void on_sbWavDsplyTop_valueChanged(int value);
    void on_sbWavDsplyBottom_valueChanged(int value);
    void on_btnWavDsplyReset_clicked();
    void on_btnConWavList_clicked();
    void on_sbAiDiDsplyRight_valueChanged(int value);
    void on_sbAiDiDsplyLeft_valueChanged(int value);
    void on_sbAiDiDsplyTop_valueChanged(int value);
    void on_sbAiDiDsplyBottom_valueChanged(int value);
    void on_btnAiDiDsplyReload_clicked();
    void on_btnAiDiDsplyReset_clicked();
    void on_comboBoxAO0_currentIndexChanged(int index);
    void on_comboBoxAO1_currentIndexChanged(int index);
    void on_comboBoxDO0_currentIndexChanged(int index);
    void on_comboBoxDO1_currentIndexChanged(int index);
    void on_comboBoxDO2_currentIndexChanged(int index);
    void on_comboBoxDO3_currentIndexChanged(int index);
    void on_comboBoxDO4_currentIndexChanged(int index);
    void on_comboBoxDO5_currentIndexChanged(int index);

    void on_btnImg1LoadFile_clicked();
    void on_btnImg2LoadFile_clicked();
    void on_cbCam1Enable_clicked(bool checked);
    void on_cbCam2Enable_clicked(bool checked);
    void on_rbImgCameraEnable_toggled(bool checked);
    void on_cbImg1Enable_clicked(bool checked);
    void on_cbImg2Enable_clicked(bool checked);
    void on_rbImgFileEnable_toggled(bool checked);
    void on_pbFramePlayback_clicked();
    void on_pbFramePlay_clicked();
    void on_pbFrameFastBackward_clicked();
    void on_pbFrameFastForward_clicked();
    void on_pbFramePause_clicked();
    void on_pbStackPlayback_clicked();
    void on_pbStackPlay_clicked();
    void on_pbStackFastBackward_clicked();
    void on_pbStackFastForward_clicked();
    void on_pbStackPause_clicked();
    void on_sbFrameIdx_valueChanged(int newValue);
    void on_sbStackIdx_valueChanged(int newValue);
    void on_hsFrameIdx_valueChanged(int newValue);
    void on_hsStackIdx_valueChanged(int newValue);
    void on_hsFrameIdx_sliderReleased();
    void on_hsStackIdx_sliderReleased();
    void on_pbImg1Color_clicked();
    void on_pbImg2Color_clicked();
    void on_hsBlending_valueChanged();
    void on_hsBlending_sliderReleased();
    void on_cbEnableMismatch_clicked(bool checked);
    void on_sbXTranslation_valueChanged(int newValue);
    void on_sbXTranslation_editingFinished();
    void on_sbYTranslation_valueChanged(int newValue);
    void on_sbYTranslation_editingFinished();
    void on_dsbRotationAngle_valueChanged(double newValue);
    void on_dsbRotationAngle_editingFinished();

    void on_actionViewHelp_triggered();

    void on_pbSaveImage_clicked();
    void on_btnOpenScriptFile_clicked();
    void on_btnScriptExecute_clicked();
    void on_btnScriptStop_clicked();
    void on_textEditScriptFileContent_cursorPositionChanged();
    void on_btnScriptUndo_clicked();
    void on_btnScriptRedo_clicked();
    void on_btnScriptSave_clicked();

public slots:
    // handle pixmap of recently acquired frame
    void handlePixmap(const QPixmap &pxmp, const QImage &img);
    void handleColorPixmap(const QPixmap &pxmp, const QImage &img);
    void readNextCamImages(int stack1, int frameIdx1, int stack2, int frameIdx2);
    void configValidityCheck(const QString &file1, const QString &file2);
    void configRecord();
    void loadWaveform(const QString &file);
    void setFilename(const QString &file1, const QString &file2);
    void loadConfig(const QString &file1, const QString &file2);
    void applyLaserSettings(void);

signals:
    void makePixmap(const QByteArray &ba, const int imageW, const int imageH,
        const double scaleFactor, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal, const int maxPixVal, bool colorizeSat);
    void makeColorPixmap(const QByteArray &ba1, const QByteArray &ba2,
        QColor color1, QColor color2, int alpha, const int imageW, const int imageH,
        const double scaleFactor1, const double scaleFactor2, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal1, const int maxPixVal1, const int minPixVal2,
        const int maxPixVal2, bool colorizeSatx);
    void startIndexRunning(int strtStackIdx1, int strtFrameIdx1, int nStacks1, int framesPerStack1,
        int strtStackIdx2, int strtFrameIdx2, int nStacks2, int framesPerStack2);
    void evaluateScript(void);

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

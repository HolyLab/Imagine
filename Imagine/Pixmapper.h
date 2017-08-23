#pragma once
#include "qobject.h"
#include "qimage.h"
#include "qpixmap.h"
#include "qpen.h"

typedef struct{
    double alpha, beta, gamma;
    double shiftX, shiftY, shiftZ;
    bool isOk;
} TransformParam;

class Pixmapper : public QObject {
    Q_OBJECT
public:
    Pixmapper(QObject *parent = 0);
    ~Pixmapper();
    TransformParam param;

private:
    // checked in handleImg to see if we're ready for another img
    QPen pen;
    void transform(const QByteArray &srcImg, QByteArray &destImg, int imageW, int imageH,
                        int dLeft, int dTop, int dWidth, int dHeight);
public slots:
    // null execution if an img is already being processed
    void handleImg(const QByteArray &ba, const int imageW, const int imageH,
        const double scaleFactor, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal, const int maxPixVal, bool colorizeSat);
    void handleColorImg(const QByteArray &ba1, const QByteArray &ba2,
        QColor color1, QColor color2, int alpha, const int imageW, const int imageH,
        const double scaleFactor1, const double scaleFactor2, const double dAreaSize,
        const int dLeft, const int dTop, const int dWidth, const int dHeight,
        const int xDown, const int xCur, const int yDown, const int yCur,
        const int minPixVal1, const int maxPixVal1, const int minPixVal2,
        const int maxPixVal2, bool colorizeSat);
signals:
    void pixmapReady(const QPixmap &pxmp, const QImage &img);
    void pixmapColorReady(const QPixmap &pxmp, const QImage &img);
};

class ImagePlay : public QObject {
    Q_OBJECT
public:
    ImagePlay(QObject *parent = 0);
    ~ImagePlay();
    int stackPlaySpeed1 = 0;
    int framePlaySpeed1 = 0;
    int stackPlaySpeed2 = 0;
    int framePlaySpeed2 = 0;
    int stackIdx1, frameIdx1, stackIdx2, frameIdx2;
    bool isRunning = false;

public
    slots:
    void indexRun(int strtStackIdx1, int strtFrameIdx1, int nStacks1, int framesPerStack1,
                int strtStackIdx2, int strtFrameIdx2, int nStacks2, int framesPerStack2);
    signals:
    void nextIndexIsReady(int nextStackIdx1, int nextFrameIdx1,
                        int nextStackIdx2, int nextFrameIdx2);
};

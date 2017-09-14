#include "Pixmapper.h"
#include "qpainter.h"
#include "imagine.h"
#include <algorithm>
#include "camera_g.hpp"

#pragma region Lifecycle

Pixmapper::Pixmapper(QObject *parent) : QObject(parent) {
    param.isOk = false;
}


Pixmapper::~Pixmapper() {

}

#pragma endregion

#pragma region Pixification

void Pixmapper::handleImg(const QByteArray &ba, const int imageW, const int imageH,
    const double scaleFactor, const double dAreaSize,
    const int dLeft, const int dTop, const int dWidth, const int dHeight,
    const int xDown, const int xCur, const int yDown, const int yCur,
    const int minPixVal, const int maxPixVal, bool colorizeSat)
{
    // NOTE: about colormap/contrast/autoscale:
    // Because Qt doesn't support 16bit grayscale image, we have to use 8bit index
    // image. Then changing colormap (which is much faster than scaling the real data)
    // is not a good option because when we map 14 bit value to 8 bit, 
    // we map 64 possible values into one value and if original values are in small
    // range, we've lost the contrast so that changing colormap won't help. 
    // So I have to scale the real data.
    QImage image = QImage(imageW, imageH, QImage::Format_Indexed8);
    //set to 256 level grayscale
    image.setColorCount(256);
    for (int i = 0; i < 256; i++){
        image.setColor(i, qRgb(i, i, i)); //set color table entry
    }

    // user may changed the colorizing option during acq.
    if (colorizeSat) {
        image.setColor(0, qRgb(0, 0, 255));
        image.setColor(255, qRgb(255, 0, 0));
    }
    else {
        image.setColor(0, qRgb(0, 0, 0));
        image.setColor(255, qRgb(255, 255, 255));
    }

    Camera::PixelValue * tp = (Camera::PixelValue *)ba.constData();
    for (int row = 0; row < imageH; ++row){
        unsigned char * rowData = image.scanLine(row);
        for (int col = 0; col<imageW; ++col){
            Camera::PixelValue inten = *tp++;
            int index;
            if (inten >= 16383 && colorizeSat) index = 255; // FIXME 16-bit
            else if (inten > maxPixVal) index = 254;
            else if (inten < minPixVal) index = 0;
            else {
                index = (inten - minPixVal)*scaleFactor;
            }
            rowData[col] = index;
        }//for, each scan line
    }//for, each row


    // TODO: user should be able to adjust this param or fit to width/height
    double maxWidth = image.width()*dAreaSize;
    double maxHeight = image.height()*dAreaSize;
    
    // crop and zoom the image from the original
    QImage cropedImage = image.copy(dLeft, dTop, dWidth, dHeight);
    double zoomFactor = (std::min)(maxWidth / dWidth, maxHeight / dHeight);
    QImage scaledImage = cropedImage.scaledToHeight(cropedImage.height()*zoomFactor);

    QPixmap pixmap = QPixmap::fromImage(scaledImage);
    {
        QPainter painter(&pixmap);
        painter.setPen(pen);
        if (xDown!=xCur || yDown!=yCur){
            QPoint lt((std::min)(xDown, xCur), (std::min)(yDown, yCur));
            QPoint rb((std::max)(xDown, xCur), (std::max)(yDown, yCur));
            painter.drawRect(QRect(lt, rb));
        }
    }//scope for QPainter obj
    emit pixmapReady(pixmap, image);
}

void Pixmapper::handleColorImg(const QByteArray &ba1, const QByteArray &ba2,
    QColor color1, QColor color2, int alpha, const int imageW, const int imageH,
    const double scaleFactor1, const double scaleFactor2, const double dAreaSize,
    const int dLeft, const int dTop, const int dWidth, const int dHeight,
    const int xDown, const int xCur, const int yDown, const int yCur,
    const int minPixVal1, const int maxPixVal1, const int minPixVal2,
    const int maxPixVal2, bool colorizeSat)
{
    QImage image = QImage(imageW, imageH, QImage::Format_RGB30); // 10,10,10 bits,  QImage::Format_RGB32 8,8,8 bits
    /*
    //set to 30bit rgb color
    image.setColorCount((1<<10)*(1 << 10)*(1 << 10));
    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 1024; j++) {
            for (int k = 0; k < 1024; k++) {
                image.setColor(i* 1048576+j*1024+k, qRgb(i, j, k)); //set color table entry
            }
        }
    }
    */
    Camera::PixelValue * tp1 = (Camera::PixelValue *)ba1.constData();
    Camera::PixelValue * tp2 = (Camera::PixelValue *)ba2.constData();
    QByteArray transformedImage;
    if (param.isOk) {
        transform(ba2, transformedImage, imageW, imageH, dLeft, dTop, dWidth, dHeight);
        tp2 = (Camera::PixelValue *)transformedImage.constData();
    }

    for (int row = 0; row < imageH; ++row) {
        QRgb* rowData = (QRgb*)image.scanLine(row);
        for (int col = 0; col<imageW; ++col) {
            Camera::PixelValue inten1 = *tp1++;
            Camera::PixelValue inten2 = *tp2++;
            int index1, index2;
            if (inten1 > maxPixVal1) index1 = 1023;// 14bit to 10bit
            else if (inten1 < minPixVal1) index1 = 0;
            else {
                index1 = (inten1 - minPixVal1)*scaleFactor1;
            }
            if (inten2 > maxPixVal2) index2 = 1023;// 14bit to 10bit
            else if (inten2 < minPixVal2) index2 = 0;
            else {
                index2 = (inten2 - minPixVal2)*scaleFactor2;
            }
            unsigned short red, green, blue;
            red = (index1*(100-alpha)*color1.red() + index2*alpha*color2.red()) / 25600;
            green = (index1*(100 - alpha)*color1.green() + index2*alpha*color2.green()) / 25600;
            blue = (index1*(100 - alpha)*color1.blue() + index2*alpha*color2.blue()) / 25600;
            rowData[col] = ((red << 20) + (green << 10) + blue) | 0xc0000000;
            /*
            if (inten1>=maxPixVal)
                red = red;
            if (inten2>=maxPixVal)
                red = red;
            red = (row + col) * 1023 / (imageH + imageW) ;
            rowData[col] = (red << 20) + (green << 10) + blue;
            if(row+col>(imageH+imageW)/2)
                rowData[col] = 0x3ff00000 | 0xc0000000;
            else
                rowData[col] = 0x000003ff | 0xc0000000;
                */
        }//for, each scan line
    }//for, each row


     // TODO: user should be able to adjust this param or fit to width/height
    double maxWidth = image.width()*dAreaSize;
    double maxHeight = image.height()*dAreaSize;

    // crop and zoom the image from the original
    QImage cropedImage = image.copy(dLeft, dTop, dWidth, dHeight);
    double zoomFactor = (std::min)(maxWidth / dWidth, maxHeight / dHeight);
    QImage scaledImage = cropedImage.scaledToHeight(cropedImage.height()*zoomFactor);

    QPixmap pixmap = QPixmap::fromImage(scaledImage);
    {
        QPainter painter(&pixmap);
        painter.setPen(pen);
        if (xDown != xCur || yDown != yCur) {
            QPoint lt((std::min)(xDown, xCur), (std::min)(yDown, yCur));
            QPoint rb((std::max)(xDown, xCur), (std::max)(yDown, yCur));
            painter.drawRect(QRect(lt, rb));
        }
    }//scope for QPainter obj
    emit pixmapColorReady(pixmap, image);
}

void Pixmapper::transform(const QByteArray &srcImg, QByteArray &destImg, int imageW, int imageH,
                        int dLeft, int dTop, int dWidth, int dHeight)
{
    Camera::PixelValue * tp1 = (Camera::PixelValue *)srcImg.constData();
//    Camera::PixelValue * tp2 = (Camera::PixelValue *)destImg.constData();
    // full transform(imageW X imageH) or cropped one(dLeft, dTop, dWidth, dHeight)?
    for (int row = 0; row < imageH; ++row) {
        Camera::PixelValue inten;
        if ((row >= dTop) && (row < dTop + dHeight)) {
            for (int col = 0; col < imageW; ++col) {
                if ((col >= dLeft) && (col < dLeft + dWidth)) {
                    // calculate transform
                    int i, j, k;
                    j = param.a[0][0]*col + param.a[0][1] *row + param.a[0][2];
                    i = param.a[1][0]*col + param.a[1][1] *row + param.a[1][2];
                    if ((i >= 0) && (i < imageH) &&
                        (j >= 0) && (j < imageW)) {
                        inten = tp1[i*imageW + j];// *tp1++;
                    }
                    else {
                        inten = 0;
                    }
                }
                else {
                    inten = 0;
                }
                destImg.append((const char *)&inten, sizeof(Camera::PixelValue));
            }
        }
        else {
            for (int col = 0; col < imageW; ++col) {
                inten = 0;
                destImg.append((const char *)&inten, sizeof(Camera::PixelValue));
            }
        }
    }
}
#pragma endregion


#pragma region Lifecycle

ImagePlay::ImagePlay(QObject *parent) : QObject(parent) {

}

ImagePlay::~ImagePlay() {

}

void ImagePlay::indexRun(int strtStackIdx1, int strtFrameIdx1, int nStacks1, int framesPerStack1,
                        int strtStackIdx2, int strtFrameIdx2, int nStacks2, int framesPerStack2)
{
    stackIdx1 = strtStackIdx1;
    frameIdx1 = strtFrameIdx1;
    stackIdx2 = strtStackIdx2;
    frameIdx2 = strtFrameIdx2;

    isRunning = true;
    while (stackPlaySpeed1 || framePlaySpeed1 || stackPlaySpeed2 || framePlaySpeed2)
    {
        stackIdx1 += stackPlaySpeed1;
        if (stackIdx1 < 0)
            stackIdx1 = nStacks1 - 1;
        else if (stackIdx1 >= nStacks1)
            stackIdx1 = 0;
        frameIdx1 += framePlaySpeed1;
        if (frameIdx1 < 0)
            frameIdx1 = framesPerStack1 - 1;
        else if (frameIdx1 >= framesPerStack1)
            frameIdx1 = 0;
        stackIdx2 += stackPlaySpeed2;
        if (stackIdx2 < 0)
            stackIdx2 = nStacks2 - 1;
        else if (stackIdx2 >= nStacks2)
            stackIdx2 = 0;
        frameIdx2 += framePlaySpeed2;
        if (frameIdx2 < 0)
            frameIdx2 = framesPerStack1 - 2;
        else if (frameIdx2 >= framesPerStack2)
            frameIdx2 = 0;
        QThread::msleep(100);//500
        emit nextIndexIsReady(stackIdx1, frameIdx1, stackIdx2, frameIdx2);
    }
    isRunning = false;
}
#pragma endregion
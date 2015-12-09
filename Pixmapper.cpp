#include "Pixmapper.h"
#include "qpainter.h"
#include <algorithm>
#include "camera_g.hpp"

#pragma region Lifecycle

Pixmapper::Pixmapper(QObject *parent) : QObject(parent) {

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
    double zoomFactor = std::min(maxWidth / dWidth, maxHeight / dHeight);
    QImage scaledImage = cropedImage.scaledToHeight(cropedImage.height()*zoomFactor);

    QPixmap pixmap = QPixmap::fromImage(scaledImage);
    {
        QPainter painter(&pixmap);
        painter.setPen(pen);
        if (xDown!=xCur || yDown!=yCur){
            QPoint lt(std::min(xDown, xCur), std::min(yDown, yCur));
            QPoint rb(std::max(xDown, xCur), std::max(yDown, yCur));
            painter.drawRect(QRect(lt, rb));
        }
    }//scope for QPainter obj
    emit pixmapReady(pixmap, image);
}

#pragma endregion
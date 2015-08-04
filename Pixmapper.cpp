#include "Pixmapper.h"
#include "qpainter.h"
#include <algorithm>

#pragma region Lifecycle

Pixmapper::Pixmapper(QObject *parent) : QObject(parent) {

}


Pixmapper::~Pixmapper() {

}

#pragma endregion

#pragma region Pixification

void Pixmapper::handleImg(const QImage &img, const int xDown,
    const int xCur, const int yDown, const int yCur)
{
    if (isPixing) return;
    isPixing = true;
    pixify(img, xDown, xCur, yDown, yCur);
    isPixing = false;
}

void Pixmapper::pixify(const QImage img, const int xDown,
    const int xCur, const int yDown, const int yCur)
{
    QPixmap pixmap = QPixmap::fromImage(img);
    {
        QPainter painter(&pixmap);
        painter.setPen(pen);
        if (xDown!=xCur || yDown!=yCur){
            QPoint lt(std::min(xDown, xCur), std::min(yDown, yCur));
            QPoint rb(std::max(xDown, xCur), std::max(yDown, yCur));
            painter.drawRect(QRect(lt, rb));
        }
    }//scope for QPainter obj
    emit pixmapReady(pixmap);
}

#pragma endregion
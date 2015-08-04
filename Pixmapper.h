#pragma once
#include "qobject.h"
#include "qimage.h"
#include "qpixmap.h"
#include "qpen.h"

class Pixmapper : public QObject {
    Q_OBJECT
public slots:
    // null execution if an img is already being processed
    void handleImg(const QImage &img, const int xDown,
        const int xCur, const int yDown, const int yCur);
signals:
    void pixmapReady(const QPixmap &pxmp);
public:
    Pixmapper(QObject *parent = 0);
    ~Pixmapper();
private:
    // actually handles the pixmap generation, calls pixmapReady on success
    void pixify(const QImage img, const int xDown,
        const int xCur, const int yDown, const int yCur);
    // checked in handleImg to see if we're ready for another img
    bool isPixing = false;
    QPen pen;
};

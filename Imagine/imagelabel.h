/*-------------------------------------------------------------------------
** Copyright (C) 2005-2008 Timothy E. Holy and Zhongsheng Guo
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

#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    ImageLabel(QWidget *parent);
    ~ImageLabel();

    //void registerMousePressHandler();

signals:
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    void mouseReleased(QMouseEvent *event);

protected:
    void mousePressEvent(QMouseEvent *event){
        emit mousePressed(event);
    }
    void mouseMoveEvent(QMouseEvent *event){
        emit mouseMoved(event);
    }
    void mouseReleaseEvent(QMouseEvent *event){
        emit mouseReleased(event);
    }

private:

};

#endif // IMAGELABEL_H

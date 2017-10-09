/*-------------------------------------------------------------------------
** Copyright (C) 2005-2010 Timothy E. Holy and Zhongsheng Guo
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


#include <QString>
#include <QFileInfo>
#include <QDir>


#include "misc.hpp"

QString replaceExtName(QString filename, QString newExtname)
{
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if (ext.isEmpty()){
        return filename + "." + newExtname;
    }
    else {
        filename.chop(ext.size());
        return filename + newExtname;
    }
}//replaceExtName()

QString addSuffixToBaseName(QString filename, QString suffix)
{
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if (ext.isEmpty()) {
        return filename + suffix;
    }
    else {
        filename.chop(ext.size()+1);
        return filename + suffix + "." + ext;
    }
}//addSuffixToBaseName()

bool CheckFileExtention(QString filename)
{
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if (ext!="imagine")
        return false;
    return true;
}//CheckFileExtention()

bool CheckAndMakeFilePath(QString filename)
{
    QFileInfo fi(filename);
    QDir dir(fi.path());
    bool retVal = true;
    if (!dir.exists())
        retVal = dir.mkpath(fi.path());
    return retVal;
}//CheckAndMakeFilePath()

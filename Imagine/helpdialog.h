/*-------------------------------------------------------------------------
** Copyright (C) 2017-2019 Timothy E. Holy and Dae Woo Kim
**    All rights reserved.
** Author: All code authored by Dae Woo Kim.
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

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QTextBrowser>
#include <QVector>
#include <QVectorIterator>
#include "ui_helpdialog.h"

class HelpDialog : public QDialog, private Ui::HelpDialogClass
{
    Q_OBJECT
public:
    QVector<QUrl> link;
    int curIdx = -1;
//    QVectorIterator<QUrl> curIdx = QVectorIterator<QUrl>(link);
    HelpDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~HelpDialog();
    QTextBrowser *helpHtml;

    void initHelp(QString filename);
    void openNewLink(QUrl link);

private slots:
    void on_pbHelpPrevious_clicked();
    void on_pbHelpNext_clicked();
    void on_pbHelpSearch_clicked();
};
#endif // HELPDIALOG_H

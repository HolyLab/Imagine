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

#include "helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
    helpHtml = new QTextBrowser;
    setupUi(this);
}

HelpDialog::~HelpDialog()
{
    delete helpHtml;
}

void HelpDialog::initHelp(QString filename)
{
    openNewLink(QUrl(filename));
    vlHelpMain->addWidget(helpHtml);
}

void HelpDialog::openNewLink(QUrl url)
{
    if (curIdx < link.size()-1) {
        // delete curIdx+1 to end
        while (link.size() > curIdx+1)
            link.pop_back();
    }
    link.push_back(url);
    curIdx++;
    helpHtml->setSource(url);
}

void HelpDialog::on_pbHelpPrevious_clicked()
{
    if (!link.isEmpty()) {
        if (curIdx > 0) {
            curIdx--;
            helpHtml->setSource(link.at(curIdx));
        }
        if (curIdx <= 0) {
            pbHelpPrevious->setDisabled(true);
        }
    }
}

void HelpDialog::on_pbHelpNext_clicked()
{
    if (curIdx < link.size() - 1)
    {
        curIdx++;
        helpHtml->setSource(link.at(curIdx));
    }
    if (curIdx >= link.size() - 1) {
        pbHelpNext->setDisabled(true);
    }
    return;
}

void HelpDialog::on_pbHelpSearch_clicked()
{
    return;
}



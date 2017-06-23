/********************************************************************************
** Form generated from reading UI file 'helpdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HELPDIALOG_H
#define UI_HELPDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_HelpDialogClass
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *pbHelpPrevious;
    QPushButton *pbHelpNext;
    QSpacerItem *horizontalSpacer_2;
    QLineEdit *leHelpSearchText;
    QPushButton *pbHelpSearch;
    QHBoxLayout *horizontalLayout;
    QGroupBox *gbHelpList;
    QVBoxLayout *vlHelpList;
    QListView *lvHelpList;
    QGroupBox *gbHelpMain;
    QVBoxLayout *vlHelpMain;

    void setupUi(QDialog *HelpDialogClass)
    {
        if (HelpDialogClass->objectName().isEmpty())
            HelpDialogClass->setObjectName(QStringLiteral("HelpDialogClass"));
        HelpDialogClass->setWindowModality(Qt::ApplicationModal);
        HelpDialogClass->resize(609, 398);
        HelpDialogClass->setModal(true);
        verticalLayout = new QVBoxLayout(HelpDialogClass);
        verticalLayout->setSpacing(3);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        groupBox = new QGroupBox(HelpDialogClass);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setMaximumSize(QSize(16777215, 50));
        horizontalLayout_3 = new QHBoxLayout(groupBox);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        pbHelpPrevious = new QPushButton(groupBox);
        pbHelpPrevious->setObjectName(QStringLiteral("pbHelpPrevious"));

        horizontalLayout_3->addWidget(pbHelpPrevious);

        pbHelpNext = new QPushButton(groupBox);
        pbHelpNext->setObjectName(QStringLiteral("pbHelpNext"));

        horizontalLayout_3->addWidget(pbHelpNext);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        leHelpSearchText = new QLineEdit(groupBox);
        leHelpSearchText->setObjectName(QStringLiteral("leHelpSearchText"));

        horizontalLayout_3->addWidget(leHelpSearchText);

        pbHelpSearch = new QPushButton(groupBox);
        pbHelpSearch->setObjectName(QStringLiteral("pbHelpSearch"));

        horizontalLayout_3->addWidget(pbHelpSearch);


        verticalLayout->addWidget(groupBox);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        gbHelpList = new QGroupBox(HelpDialogClass);
        gbHelpList->setObjectName(QStringLiteral("gbHelpList"));
        gbHelpList->setMaximumSize(QSize(150, 16777215));
        vlHelpList = new QVBoxLayout(gbHelpList);
        vlHelpList->setObjectName(QStringLiteral("vlHelpList"));
        lvHelpList = new QListView(gbHelpList);
        lvHelpList->setObjectName(QStringLiteral("lvHelpList"));

        vlHelpList->addWidget(lvHelpList);


        horizontalLayout->addWidget(gbHelpList);

        gbHelpMain = new QGroupBox(HelpDialogClass);
        gbHelpMain->setObjectName(QStringLiteral("gbHelpMain"));
        vlHelpMain = new QVBoxLayout(gbHelpMain);
        vlHelpMain->setObjectName(QStringLiteral("vlHelpMain"));

        horizontalLayout->addWidget(gbHelpMain);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(HelpDialogClass);

        QMetaObject::connectSlotsByName(HelpDialogClass);
    } // setupUi

    void retranslateUi(QDialog *HelpDialogClass)
    {
        HelpDialogClass->setWindowTitle(QApplication::translate("HelpDialogClass", "About", 0));
        groupBox->setTitle(QString());
        pbHelpPrevious->setText(QApplication::translate("HelpDialogClass", "Previous", 0));
        pbHelpNext->setText(QApplication::translate("HelpDialogClass", "Next", 0));
        pbHelpSearch->setText(QApplication::translate("HelpDialogClass", "Search", 0));
        gbHelpList->setTitle(QString());
        gbHelpMain->setTitle(QString());
    } // retranslateUi

};

namespace Ui {
    class HelpDialogClass: public Ui_HelpDialogClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HELPDIALOG_H

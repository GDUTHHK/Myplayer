/********************************************************************************
** Form generated from reading UI file 'MyMainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYMAINWINDOW_H
#define UI_MYMAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MyMainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_8;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLineEdit *urlLine;
    QPushButton *openButton;
    QPushButton *closeButton;
    QWidget *widget_2;
    QGridLayout *gridLayout_2;
    QWidget *openGLWidget;
    QWidget *onvif_search;
    QVBoxLayout *verticalLayout_3;
    QWidget *widget_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *timeoutLabel;
    QSpinBox *timeoutSpinBox;
    QSpacerItem *horizontalSpacer;
    QPushButton *searchButton;
    QPushButton *stopSearchButton;
    QPushButton *clearButton;
    QWidget *widget_4;
    QHBoxLayout *horizontalLayout_4;
    QLabel *statusLabel;
    QTableView *deviceTableView;
    QWidget *widget_5;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *useDeviceButton;
    QPushButton *refreshButton;
    QSpacerItem *horizontalSpacer_2;
    QLabel *deviceCountLabel;
    QWidget *config;
    QWidget *widget_8;
    QVBoxLayout *verticalLayout;
    QWidget *widget_6;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label;
    QRadioButton *radioButton;
    QRadioButton *radioButton_2;
    QWidget *widget_9;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_3;
    QRadioButton *radioButton_4;
    QRadioButton *radioButton_5;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MyMainWindow)
    {
        if (MyMainWindow->objectName().isEmpty())
            MyMainWindow->setObjectName(QString::fromUtf8("MyMainWindow"));
        MyMainWindow->resize(770, 630);
        centralwidget = new QWidget(MyMainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout_8 = new QHBoxLayout(centralwidget);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        widget = new QWidget(tab);
        widget->setObjectName(QString::fromUtf8("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        urlLine = new QLineEdit(widget);
        urlLine->setObjectName(QString::fromUtf8("urlLine"));

        horizontalLayout->addWidget(urlLine);

        openButton = new QPushButton(widget);
        openButton->setObjectName(QString::fromUtf8("openButton"));

        horizontalLayout->addWidget(openButton);

        closeButton = new QPushButton(widget);
        closeButton->setObjectName(QString::fromUtf8("closeButton"));

        horizontalLayout->addWidget(closeButton);


        verticalLayout_2->addWidget(widget);

        widget_2 = new QWidget(tab);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        gridLayout_2 = new QGridLayout(widget_2);
        gridLayout_2->setSpacing(0);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        openGLWidget = new QWidget(widget_2);
        openGLWidget->setObjectName(QString::fromUtf8("openGLWidget"));

        gridLayout_2->addWidget(openGLWidget, 0, 0, 1, 1);


        verticalLayout_2->addWidget(widget_2);

        tabWidget->addTab(tab, QString());
        onvif_search = new QWidget();
        onvif_search->setObjectName(QString::fromUtf8("onvif_search"));
        verticalLayout_3 = new QVBoxLayout(onvif_search);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        widget_3 = new QWidget(onvif_search);
        widget_3->setObjectName(QString::fromUtf8("widget_3"));
        horizontalLayout_2 = new QHBoxLayout(widget_3);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        timeoutLabel = new QLabel(widget_3);
        timeoutLabel->setObjectName(QString::fromUtf8("timeoutLabel"));

        horizontalLayout_2->addWidget(timeoutLabel);

        timeoutSpinBox = new QSpinBox(widget_3);
        timeoutSpinBox->setObjectName(QString::fromUtf8("timeoutSpinBox"));

        horizontalLayout_2->addWidget(timeoutSpinBox);

        horizontalSpacer = new QSpacerItem(362, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        searchButton = new QPushButton(widget_3);
        searchButton->setObjectName(QString::fromUtf8("searchButton"));

        horizontalLayout_2->addWidget(searchButton);

        stopSearchButton = new QPushButton(widget_3);
        stopSearchButton->setObjectName(QString::fromUtf8("stopSearchButton"));

        horizontalLayout_2->addWidget(stopSearchButton);

        clearButton = new QPushButton(widget_3);
        clearButton->setObjectName(QString::fromUtf8("clearButton"));

        horizontalLayout_2->addWidget(clearButton);


        verticalLayout_3->addWidget(widget_3);

        widget_4 = new QWidget(onvif_search);
        widget_4->setObjectName(QString::fromUtf8("widget_4"));
        horizontalLayout_4 = new QHBoxLayout(widget_4);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        statusLabel = new QLabel(widget_4);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        horizontalLayout_4->addWidget(statusLabel);


        verticalLayout_3->addWidget(widget_4);

        deviceTableView = new QTableView(onvif_search);
        deviceTableView->setObjectName(QString::fromUtf8("deviceTableView"));

        verticalLayout_3->addWidget(deviceTableView);

        widget_5 = new QWidget(onvif_search);
        widget_5->setObjectName(QString::fromUtf8("widget_5"));
        horizontalLayout_3 = new QHBoxLayout(widget_5);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        useDeviceButton = new QPushButton(widget_5);
        useDeviceButton->setObjectName(QString::fromUtf8("useDeviceButton"));

        horizontalLayout_3->addWidget(useDeviceButton);

        refreshButton = new QPushButton(widget_5);
        refreshButton->setObjectName(QString::fromUtf8("refreshButton"));

        horizontalLayout_3->addWidget(refreshButton);

        horizontalSpacer_2 = new QSpacerItem(479, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        deviceCountLabel = new QLabel(widget_5);
        deviceCountLabel->setObjectName(QString::fromUtf8("deviceCountLabel"));

        horizontalLayout_3->addWidget(deviceCountLabel);


        verticalLayout_3->addWidget(widget_5);

        tabWidget->addTab(onvif_search, QString());
        config = new QWidget();
        config->setObjectName(QString::fromUtf8("config"));
        widget_8 = new QWidget(config);
        widget_8->setObjectName(QString::fromUtf8("widget_8"));
        widget_8->setGeometry(QRect(50, 30, 202, 92));
        verticalLayout = new QVBoxLayout(widget_8);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        widget_6 = new QWidget(widget_8);
        widget_6->setObjectName(QString::fromUtf8("widget_6"));
        horizontalLayout_5 = new QHBoxLayout(widget_6);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        label = new QLabel(widget_6);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_5->addWidget(label);

        radioButton = new QRadioButton(widget_6);
        radioButton->setObjectName(QString::fromUtf8("radioButton"));

        horizontalLayout_5->addWidget(radioButton);

        radioButton_2 = new QRadioButton(widget_6);
        radioButton_2->setObjectName(QString::fromUtf8("radioButton_2"));

        horizontalLayout_5->addWidget(radioButton_2);


        verticalLayout->addWidget(widget_6);

        widget_9 = new QWidget(widget_8);
        widget_9->setObjectName(QString::fromUtf8("widget_9"));
        horizontalLayout_7 = new QHBoxLayout(widget_9);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_3 = new QLabel(widget_9);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_7->addWidget(label_3);

        radioButton_4 = new QRadioButton(widget_9);
        radioButton_4->setObjectName(QString::fromUtf8("radioButton_4"));

        horizontalLayout_7->addWidget(radioButton_4);

        radioButton_5 = new QRadioButton(widget_9);
        radioButton_5->setObjectName(QString::fromUtf8("radioButton_5"));

        horizontalLayout_7->addWidget(radioButton_5);


        verticalLayout->addWidget(widget_9);

        tabWidget->addTab(config, QString());

        horizontalLayout_8->addWidget(tabWidget);

        MyMainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MyMainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MyMainWindow->setStatusBar(statusbar);

        retranslateUi(MyMainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MyMainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MyMainWindow)
    {
        MyMainWindow->setWindowTitle(QCoreApplication::translate("MyMainWindow", "StreamPlayer", nullptr));
        urlLine->setText(QCoreApplication::translate("MyMainWindow", "rtsp://120.77.0.8/live/livestream", nullptr));
        openButton->setText(QCoreApplication::translate("MyMainWindow", "\346\222\255\346\224\276", nullptr));
        closeButton->setText(QCoreApplication::translate("MyMainWindow", "\345\205\263\351\227\255", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("MyMainWindow", "\350\247\206\351\242\221\346\222\255\346\224\276", nullptr));
        timeoutLabel->setText(QCoreApplication::translate("MyMainWindow", "TextLabel", nullptr));
        searchButton->setText(QCoreApplication::translate("MyMainWindow", "\345\274\200\345\247\213\346\220\234\347\264\242", nullptr));
        stopSearchButton->setText(QCoreApplication::translate("MyMainWindow", "\345\201\234\346\255\242\346\220\234\347\264\242", nullptr));
        clearButton->setText(QCoreApplication::translate("MyMainWindow", "\346\270\205\347\251\272\345\210\227\350\241\250", nullptr));
        statusLabel->setText(QCoreApplication::translate("MyMainWindow", "TextLabel", nullptr));
        useDeviceButton->setText(QCoreApplication::translate("MyMainWindow", "\344\275\277\347\224\250\350\257\245\350\256\276\345\244\207", nullptr));
        refreshButton->setText(QCoreApplication::translate("MyMainWindow", "\345\210\267\346\226\260", nullptr));
        deviceCountLabel->setText(QCoreApplication::translate("MyMainWindow", "\350\256\276\345\244\207\346\225\260\351\207\217\357\274\232", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(onvif_search), QCoreApplication::translate("MyMainWindow", "ONvif\346\220\234\347\264\242", nullptr));
        label->setText(QCoreApplication::translate("MyMainWindow", "\350\247\243\347\240\201\357\274\232", nullptr));
        radioButton->setText(QCoreApplication::translate("MyMainWindow", "CPU", nullptr));
        radioButton_2->setText(QCoreApplication::translate("MyMainWindow", "GPU", nullptr));
        label_3->setText(QCoreApplication::translate("MyMainWindow", "\350\247\243\347\240\201\345\231\250\357\274\232", nullptr));
        radioButton_4->setText(QCoreApplication::translate("MyMainWindow", "H264", nullptr));
        radioButton_5->setText(QCoreApplication::translate("MyMainWindow", "H265", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(config), QCoreApplication::translate("MyMainWindow", "\345\217\202\346\225\260\350\256\276\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MyMainWindow: public Ui_MyMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYMAINWINDOW_H

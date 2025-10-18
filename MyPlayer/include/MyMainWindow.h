#pragma once

#include <QMainWindow>
#include <memory>
#include <string>
#include "MediaPlayer.h"
//#include "tools/onvif/OnvifDeviceSearcher.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MyMainWindow; }
QT_END_NAMESPACE
class IMediaFactory; 

class MyMainWindow  : public QMainWindow
{
	Q_OBJECT

public:
	explicit MyMainWindow(const std::string& configPath, QWidget *parent = nullptr);
	~MyMainWindow();
private:
	void Initialization();
	std::unique_ptr<IPlayer> createPlayer(const Properties& config);
private slots:
	void on_openButton_clicked();
	void on_closeButton_clicked();
//    // ������ONVIF������زۺ���
//    void on_searchButton_clicked();
//    void on_stopSearchButton_clicked();
//    void on_clearButton_clicked();
//    void on_useDeviceButton_clicked();
//    void on_deviceTableView_selectionChanged();
//
//    // ONVIF�������źŴ���
//    void onDeviceFound(const OnvifDeviceInfo& device);
//    void onSearchStarted();
//    void onSearchFinished(int deviceCount);
//    void onSearchError(const QString& error);
//
private:
	Ui::MyMainWindow* ui;
	std::shared_ptr<IMediaFactory> m_factory;
	std::unique_ptr<IPlayer> player;
	Properties config;
	std::string m_configPath;
//
//    OnvifDeviceSearcher* m_onvifSearcher;
//    OnvifDeviceModel* m_deviceModel;
};


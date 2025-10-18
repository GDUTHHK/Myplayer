#include "MyMainWindow.h"
#include "ui_MyMainWindow.h"
#include "factories/JsonConfigFactory.h"
#include"include/MediaPlayerFacade.h"
#include <regex>
#include <QMessageBox>
#include <QOpenGLContext>

static std::string getType(const std::string& _url)
{
	std::regex ip_regex("([a-zA-Z]+)://(\\d{1,3}(\\.\\d{1,3}){3})(:\\d+)?\\S*");
	std::smatch results;
	try
	{
		if (std::regex_match(_url, results, ip_regex))
		{
			return results[1].str();
		}
		else return std::string();
	}
	catch (std::regex_error e)
	{
		std::cerr << e.what() << '\t' << e.code() << std::endl;
		return std::string();
	}
}

MyMainWindow::MyMainWindow(const std::string& configPath,QWidget *parent)
	: QMainWindow(parent),ui(new Ui::MyMainWindow), m_configPath(configPath)
{
	ui->setupUi(this);
	ui->openGLWidget->setAttribute(Qt::WA_NativeWindow);
	//ui->openGLWidget->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
	m_factory = std::make_shared<JsonConfigFactory>(m_configPath);
}

MyMainWindow::~MyMainWindow()
{
	if (player) {
		player->stop();
	}
	delete ui;
}
void MyMainWindow::Initialization() {
	auto jsonFactory = std::dynamic_pointer_cast<JsonConfigFactory>(m_factory);
	if (jsonFactory) {
		config = jsonFactory->getBaseConfig();
		std::cout << "Loaded base config from JSON." << std::endl;
	} else {
		std::cout << "Failed to get base config from JSON." << std::endl;
	}

	std::string url = ui->urlLine->text().toStdString();
	std::string urlType = getType(url);
	std::transform(urlType.begin(), urlType.end(), urlType.begin(), [](unsigned char c) {
		return toupper(c); });
	void* windowHandle = reinterpret_cast<void*>(ui->openGLWidget->winId());

	config["protocol_type"] = urlType;
	config["url"] = url;
	config["render_enabled"] = true;
	config["render_window_handle"] = windowHandle;
}

std::unique_ptr<IPlayer> MyMainWindow::createPlayer(const Properties& config)
{
    if (!m_factory) {
        return nullptr;
    }
    
    try {
        return std::make_unique<MediaPlayerFacade>(m_factory, config);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create player: " << e.what() << std::endl;
        return nullptr;
    }
}

void MyMainWindow::on_openButton_clicked(){
	if (player && player->isRunning()) {
		player->stop();
	}
	Initialization();
	std::string url = ui->urlLine->text().toStdString();
	if (url.empty()) {
		std::cout<< "URL is empty" << std::endl;
		return;
	}

	WId wid = ui->openGLWidget->winId();
	void* handle = reinterpret_cast<void*>(wid);
	config["render_window_handle"] = handle;

	player = createPlayer(config);
	if (!player) {
		std::cout << "Failed to create player." << std::endl;
		return;
	}

	if (player->start()) {
		std::cout << "Player started for URL:"<<url<<std::endl;
	}
	else {
		std::cout << "Failed to start player." << std::endl;
		player.reset();
	}
}

//void MyMainWindow::on_searchButton_clicked() {
//	int timeoutMs = ui->timeoutSpinBox->value() * 1000;
//	m_onvifSearcher->startSearch(timeoutMs);
//}
//
//void MyMainWindow::on_stopSearchButton_clicked() {
//	m_onvifSearcher->stopSearch();
//}
//
//void MyMainWindow::on_clearButton_clicked() {
//	m_deviceModel->clearDevices();
//	m_onvifSearcher->clearDevices();
//	ui->deviceCountLabel->setText("�豸����: 0");
//	ui->useDeviceButton->setEnabled(false);
//}
//
//void MyMainWindow::on_useDeviceButton_clicked() {
//	auto selection = ui->deviceTableView->selectionModel()->selectedRows();
//	if (selection.isEmpty()) return;
//
//	int row = selection.first().row();
//	auto device = m_deviceModel->getDevice(row);
//
//	if (!device.endpoint.isEmpty()) {
//		// ��ONVIF�豸��ַ����URL�����
//		// ������Ը�����Ҫ����ONVIF��ַ��RTSP����ַ��ת��
//		ui->urlLine->setText(device.endpoint);
//
//		// �л������ű�ǩҳ
//		ui->tabWidget->setCurrentIndex(0);
//
//		QMessageBox::information(this, "�豸ѡ��",
//			QString("��ѡ���豸��%1\n��ַ��%2").arg(device.deviceName).arg(device.endpoint));
//	}
//}
//
//void MyMainWindow::on_deviceTableView_selectionChanged() {
//	auto selection = ui->deviceTableView->selectionModel()->selectedRows();
//	ui->useDeviceButton->setEnabled(!selection.isEmpty());
//}
//
//void MyMainWindow::onDeviceFound(const OnvifDeviceInfo& device) {
//	m_deviceModel->addDevice(device);
//	int count = m_deviceModel->rowCount();
//	ui->deviceCountLabel->setText(QString("�豸����: %1").arg(count));
//	ui->statusLabel->setText(QString("���ҵ� %1 ���豸...").arg(count));
//}
//
//void MyMainWindow::onSearchStarted() {
//	ui->searchButton->setEnabled(false);
//	ui->stopSearchButton->setEnabled(true);
//	ui->statusLabel->setText("��������ONVIF�豸...");
//	ui->deviceCountLabel->setText("�豸����: 0");
//}
//
//void MyMainWindow::onSearchFinished(int deviceCount) {
//	ui->searchButton->setEnabled(true);
//	ui->stopSearchButton->setEnabled(false);
//	ui->statusLabel->setText(QString("������ɣ����ҵ� %1 ���豸").arg(deviceCount));
//}
//
//void MyMainWindow::onSearchError(const QString& error) {
//	ui->searchButton->setEnabled(true);
//	ui->stopSearchButton->setEnabled(false);
//	ui->statusLabel->setText(QString("��������%1").arg(error));
//	QMessageBox::warning(this, "��������", error);
//}
//
//
void MyMainWindow::on_closeButton_clicked() {
	if (player) {
		player->stop();
		std::cout << "Player stopped." << std::endl;
		player.reset();
	}
}
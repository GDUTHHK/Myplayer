#include <iostream>
#include <thread>
#include <chrono>
#include <QWidget>
#include <QApplication>
#include "MyMainWindow.h"
#include "MediaPlayerFacade.h"
#include "factories/JsonConfigFactory.h"

int main(int argc , char * argv[]) {
	QApplication app(argc,argv);
	MyMainWindow *w = new MyMainWindow("E:\\rtsp\\MyPlayer\\x64\\Debug\\config.json");
	w->show();
	return app.exec();
}
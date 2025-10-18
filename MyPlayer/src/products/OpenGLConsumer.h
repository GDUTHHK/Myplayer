#pragma once
#include<thread>
#include<atomic>
#include<queue>
#include<mutex>
#include<condition_variable>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include "internal/PrivateQueue.hpp"
#include "internal/IDataConsumer.h"
#include"internal/EventDispatcher.h"
class OpenGLConsumer :public IDataConsumer {
public:
	explicit OpenGLConsumer(const Properties& config,std::shared_ptr<EventDispatcher> dispatcher);
	//OpenGLConsumer(const std::string& name, const Properties& config);
	~OpenGLConsumer()override;
	bool initialize(const Properties& props) override;
	bool start() override;
	void stop() override;
	bool isliving();

	OpenGLConsumer(const OpenGLConsumer&) = delete;
	OpenGLConsumer& operator = (const OpenGLConsumer&) = delete;
	OpenGLConsumer(OpenGLConsumer&&) = delete;
	OpenGLConsumer& operator = (OpenGLConsumer&&) = delete;
private:
	void initializeOpenGl();
	void paintOpenGL(const DecodedFrame& frame);
	void processingLoop();
private:
	WId m_winId;
	std::shared_ptr<EventDispatcher> m_dispatcher;
	const Properties m_config;
	const std::string m_name;
	std::atomic<bool> m_isRunning{ false };
	std::thread m_thread;
	PrivateQueue<DecodedFrame> m_frameQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_queueCond;

	QWindow* m_window = nullptr;
	QOffscreenSurface* m_surface = nullptr;

	std::unique_ptr<QOpenGLContext> m_context; // 恢复为 unique_ptr
	std::unique_ptr<QOpenGLBuffer> m_vbo;
	std::unique_ptr<QOpenGLShaderProgram> m_program;
	std::unique_ptr<QOpenGLTexture> m_textureY;
	std::unique_ptr<QOpenGLTexture> m_textureU;
	std::unique_ptr<QOpenGLTexture> m_textureV;

	int m_frameWidth = 0;
	int m_frameHeight = 0;
};



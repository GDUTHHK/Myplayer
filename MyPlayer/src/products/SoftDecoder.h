#pragma once

#include<mutex>
#include "internal/IVideoDecoder.h"
#include "internal/Share.h"
#include "internal/PrivateQueue.hpp"


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
class SoftDecoder:public IVideoDecoder{
public:
	SoftDecoder(const Properties& config);
	~SoftDecoder();
	void start() override;
	bool initialize(const Properties& config);
	bool configure(const ST_MEDIA_INFO* info);
	bool frameQueue(const ST_FRAME*);
	void setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback);
	void release();
private:
	void decodeThread();

	const Properties m_config;
	AVCodecContext* m_pCodecContext = nullptr;
	AVCodec* m_pCodec = nullptr;
	AVPacket* m_pPacket = nullptr;
	AVFrame* m_pFrame = nullptr;

	std::thread m_thread;
	std::mutex m_mutex;
	std::atomic<bool> m_running;
	PrivateQueue<ST_FRAME*> m_frameQueue;
	std::function<void(std::shared_ptr<DecodedFrame>)> m_outputCallback;

};


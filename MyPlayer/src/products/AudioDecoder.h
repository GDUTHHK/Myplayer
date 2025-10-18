#include<atomic>
#include<mutex>
#include<thread>

#include"internal/IAudioDecoder.h"
#include "internal/Share.h"
#include "internal/PrivateQueue.hpp"

extern "C" {
#include"libavcodec/avcodec.h"
#include"libavformat/avformat.h"
}

class AudioDecoder :public IAudioDecoder {
public:
	AudioDecoder(const Properties config);
	~AudioDecoder();

    void start() override;
    bool initialize(const Properties& config) override;
    bool configure(const ST_MEDIA_INFO* info) override;
    bool frameQueue(std::unique_ptr<ST_FRAME> frame) override;
    void setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback) override;
    void release() override;
private:
    void Loop();
private:
    Properties m_config;
    AVCodecContext* m_pCodecContext;
    AVCodec* m_pCodec;
    AVPacket* m_pPacket;
    AVFrame* m_pFrame;

    std::thread m_thread;
    std::mutex m_mutex;
    std::atomic<bool>m_running;
    PrivateQueue<std::unique_ptr<ST_FRAME>> m_frameQueue;
    std::function<void(std::shared_ptr<DecodedFrame>)> m_outputCallback;

};

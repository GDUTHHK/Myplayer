#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "internal/PrivateQueue.hpp"
#include "internal/IDataConsumer.h"
#include "internal/EventDispatcher.h"

// Windows Audio API
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

class AudioConsumer : public IDataConsumer {
public:
    explicit AudioConsumer(const Properties& config, std::shared_ptr<EventDispatcher> dispatcher);
    ~AudioConsumer() override;
    
    bool initialize(const Properties& props) override;
    bool start() override;
    void stop() override;
    bool isLiving();

    // 禁止拷贝和移动
    AudioConsumer(const AudioConsumer&) = delete;
    AudioConsumer& operator=(const AudioConsumer&) = delete;
    AudioConsumer(AudioConsumer&&) = delete;
    AudioConsumer& operator=(AudioConsumer&&) = delete;

private:
    void initializeAudio();
    void playAudio(const DecodedFrame& frame);
    void processingLoop();
    void onAudioFrame(std::shared_ptr<DecodedFrame> frame);
    
    // Windows Audio相关方法
#ifdef _WIN32
    bool setupWaveOut();
    void cleanupWaveOut();
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

private:
    std::shared_ptr<EventDispatcher> m_dispatcher;
    const Properties m_config;
    std::atomic<bool> m_isRunning{false};
    std::thread m_thread;
    PrivateQueue<DecodedFrame> m_frameQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCond;
    
    // 音频参数
    int m_sampleRate{48000};
    int m_channels{2};
    int m_bitsPerSample{16};
    
    // Windows Audio
#ifdef _WIN32
    HWAVEOUT m_waveOut;
    std::vector<WAVEHDR> m_waveHeaders;
    std::vector<std::vector<char>> m_audioBuffers;
    std::mutex m_audioMutex;
    static constexpr int BUFFER_COUNT = 4;
    static constexpr int BUFFER_SIZE = 4096;
#endif
};
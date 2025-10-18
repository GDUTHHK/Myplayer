#include "AudioConsumer.h"
#include <iostream>
#include <algorithm>
#include "factories/GenericFactory.h"  // 添加工厂头文件

AudioConsumer::AudioConsumer(const Properties& config, std::shared_ptr<EventDispatcher> dispatcher)
    : m_dispatcher(dispatcher)
    , m_config(config)
#ifdef _WIN32
    , m_waveOut(nullptr)
#endif
{
    // 从配置中读取音频参数
    if (config.find("sample_rate") != config.end()) {
        try {
            m_sampleRate = std::any_cast<int>(config.at("sample_rate"));
        } catch (const std::bad_any_cast& e) {
            std::string str_val = std::any_cast<std::string>(config.at("sample_rate"));
            m_sampleRate = std::stoi(str_val);
        }
    }
    
    if (config.find("channels") != config.end()) {
        try {
            m_channels = std::any_cast<int>(config.at("channels"));
        } catch (const std::bad_any_cast& e) {
            std::string str_val = std::any_cast<std::string>(config.at("channels"));
            m_channels = std::stoi(str_val);
        }
    }
    
    if (config.find("bits_per_sample") != config.end()) {
        try {
            m_bitsPerSample = std::any_cast<int>(config.at("bits_per_sample"));
        } catch (const std::bad_any_cast& e) {
            std::string str_val = std::any_cast<std::string>(config.at("bits_per_sample"));
            m_bitsPerSample = std::stoi(str_val);
        }
    }
    
    std::cout << "AudioConsumer created with: " << m_sampleRate << "Hz, " 
              << m_channels << " channels, " << m_bitsPerSample << " bits" << std::endl;
}

AudioConsumer::~AudioConsumer() {
    if (m_isRunning) {
        stop();
    }
}

bool AudioConsumer::initialize(const Properties& props) {
    try {
        initializeAudio();
        
        // 订阅音频帧事件
        if (m_dispatcher) {
            m_dispatcher->subscribe("DecodedAudioFrame", 
                [this](std::shared_ptr<DecodedFrame> frame) {
                    this->onAudioFrame(frame);
                });
            std::cout << "AudioConsumer subscribed to DecodedAudioFrame events" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "AudioConsumer initialize failed: " << e.what() << std::endl;
        return false;
    }
}

bool AudioConsumer::start() {
    if (m_isRunning) {
        return true;
    }
    
    m_isRunning = true;
    m_thread = std::thread(&AudioConsumer::processingLoop, this);
    
    std::cout << "AudioConsumer started" << std::endl;
    return true;
}

void AudioConsumer::stop() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    m_queueCond.notify_all();
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
#ifdef _WIN32
    cleanupWaveOut();
#endif
    
    std::cout << "AudioConsumer stopped" << std::endl;
}

bool AudioConsumer::isLiving() {
    return m_isRunning;
}

void AudioConsumer::initializeAudio() {
#ifdef _WIN32
    if (!setupWaveOut()) {
        throw std::runtime_error("Failed to setup WaveOut");
    }
#endif
}

void AudioConsumer::onAudioFrame(std::shared_ptr<DecodedFrame> frame) {
    if (!m_isRunning || !frame) {
        return;
    }
    
    // 将帧添加到队列
    DecodedFrame frameData = *frame;  // 复制数据
    m_frameQueue.push(std::move(frameData));
    m_queueCond.notify_one();
}

void AudioConsumer::processingLoop() {
    while (m_isRunning) {
        DecodedFrame frame;
        if (m_frameQueue.pop(frame)) {
            playAudio(frame);
        } else {
            // 队列为空，等待新帧
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCond.wait_for(lock, std::chrono::milliseconds(10));
        }
    }
}

void AudioConsumer::playAudio(const DecodedFrame& frame) {
#ifdef _WIN32
    if (!m_waveOut || frame.data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_audioMutex);
    
    for (size_t i = 0; i < m_waveHeaders.size(); ++i) {
        if (m_waveHeaders[i].dwFlags & WHDR_DONE || !(m_waveHeaders[i].dwFlags & WHDR_INQUEUE)) {
            // 缓冲区可用
            WAVEHDR& header = m_waveHeaders[i];
            auto& buffer = m_audioBuffers[i];
            
            // 复制音频数据
            size_t copySize = std::min(frame.data.size(), buffer.size());
            std::memcpy(buffer.data(), frame.data.data(), copySize);
            
            header.lpData = buffer.data();
            header.dwBufferLength = static_cast<DWORD>(copySize);
            header.dwFlags = 0;
            
            // 准备并播放缓冲区
            MMRESULT result = waveOutPrepareHeader(m_waveOut, &header, sizeof(WAVEHDR));
            if (result == MMSYSERR_NOERROR) {
                result = waveOutWrite(m_waveOut, &header, sizeof(WAVEHDR));
                if (result != MMSYSERR_NOERROR) {
                    std::cerr << "waveOutWrite failed: " << result << std::endl;
                    waveOutUnprepareHeader(m_waveOut, &header, sizeof(WAVEHDR));
                }
            } else {
                std::cerr << "waveOutPrepareHeader failed: " << result << std::endl;
            }
            break;
        }
    }
#endif
}

#ifdef _WIN32
bool AudioConsumer::setupWaveOut() {
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = m_channels;
    waveFormat.nSamplesPerSec = m_sampleRate;
    waveFormat.wBitsPerSample = m_bitsPerSample;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;
    
    MMRESULT result = waveOutOpen(&m_waveOut, WAVE_MAPPER, &waveFormat, 
                                 reinterpret_cast<DWORD_PTR>(waveOutProc), 
                                 reinterpret_cast<DWORD_PTR>(this), 
                                 CALLBACK_FUNCTION);
    
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveOutOpen failed: " << result << std::endl;
        return false;
    }
    
    // 创建音频缓冲区
    m_waveHeaders.resize(BUFFER_COUNT);
    m_audioBuffers.resize(BUFFER_COUNT);
    
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        m_audioBuffers[i].resize(BUFFER_SIZE);
        m_waveHeaders[i].dwFlags = WHDR_DONE;
    }
    
    return true;
}

void AudioConsumer::cleanupWaveOut() {
    if (m_waveOut) {
        waveOutReset(m_waveOut);
        for (auto& header : m_waveHeaders) {
            if (header.dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(m_waveOut, &header, sizeof(WAVEHDR));
            }
        }
        
        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }
    
    m_waveHeaders.clear();
    m_audioBuffers.clear();
}

void CALLBACK AudioConsumer::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, 
                                        DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        // 缓冲区播放完成，可以重用
        AudioConsumer* consumer = reinterpret_cast<AudioConsumer*>(dwInstance);
        WAVEHDR* header = reinterpret_cast<WAVEHDR*>(dwParam1);
        
        if (consumer && header) {
            std::lock_guard<std::mutex> lock(consumer->m_audioMutex);
            waveOutUnprepareHeader(hwo, header, sizeof(WAVEHDR));
            header->dwFlags |= WHDR_DONE;
        }
    }
}
#endif

namespace {
    Registrar<AudioConsumer, IDataConsumer, const Properties&, std::shared_ptr<EventDispatcher>> registrar("audio_consumer");
}
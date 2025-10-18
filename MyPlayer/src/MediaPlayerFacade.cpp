#include "MediaPlayerFacade.h"
#include "MediaPipeline.h"
#include "IDataConsumer.h"

MediaPlayerFacade::MediaPlayerFacade(std::shared_ptr<IMediaFactory> factory, const Properties& config){
    m_dispatcher = std::make_shared<EventDispatcher>();
    m_factory = factory;

    auto client = m_factory->createStreamClient(config);
    auto videoDecoder = m_factory->createVideoDecoder(config);
    auto audioDecoder = m_factory->createAudioDecoder(config);
    
    m_pipeline = std::make_unique<MediaPipelineImpl>(std::move(client), std::move(videoDecoder), std::move(audioDecoder), m_dispatcher, config);
    
    // 创建视频渲染消费者
    if (config.count("render_enabled") && std::any_cast<bool>(config.at("render_enabled"))) {
        auto renderConsumer = m_factory->createDataConsumer("render", config, m_dispatcher);
        if (renderConsumer) {
            m_consumers.push_back(std::move(renderConsumer));
        }
    }
    
    // 创建音频播放消费者
    if (config.count("audio_enabled") && std::any_cast<bool>(config.at("audio_enabled"))) {
        auto audioConsumer = m_factory->createDataConsumer("audio", config, m_dispatcher);
        if (audioConsumer) {
            m_consumers.push_back(std::move(audioConsumer));
        }
    }

    // 初始化所有消费者
    for (const auto& consumer : m_consumers) {
        if (!consumer || !consumer->initialize(config)) {
            throw std::runtime_error("initialize consumer failed");
        }
    }
}

MediaPlayerFacade::~MediaPlayerFacade() {
    if (isRunning()) {
        stop();
    }
}

bool MediaPlayerFacade::start() {
    for (const auto& consumer : m_consumers) {
        if (!consumer->start())
            return false;
    }
    return m_pipeline->start();
}

void MediaPlayerFacade::stop() {
    m_pipeline->stop();
    for (const auto& consumer : m_consumers) {
        consumer->stop();
    }
}

bool MediaPlayerFacade::isRunning() const{
    return m_pipeline && m_pipeline->isRunning();
}


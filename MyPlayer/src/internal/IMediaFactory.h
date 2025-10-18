// IMediaFactory.h
#pragma once
#include "IProtocolHandler.h"
#include "IVideoDecoder.h"
#include "IAudioDecoder.h"
#include "IDataConsumer.h"
#include"EventDispatcher.h"
#include <memory>

class IMediaFactory {
public:
    virtual ~IMediaFactory() = default;
    virtual std::unique_ptr<IProtocolHandler> createStreamClient(const Properties& config) = 0;
    virtual std::unique_ptr<IVideoDecoder> createVideoDecoder(const Properties& config) = 0;
    virtual std::unique_ptr<IAudioDecoder> createAudioDecoder(const Properties& config) = 0;
    virtual std::unique_ptr<IDataConsumer> createDataConsumer(const std::string& name, const Properties& config, 
                                                                    std::shared_ptr<EventDispatcher> dispatcher) = 0;
};
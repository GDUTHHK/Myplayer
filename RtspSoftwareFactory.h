#pragma once

#include "iostream"
#include "products/RtspClient.h"
#include "products/SoftDecoder.h"
#include "internal/IMediaFactory.h"
#include "products/OpenGLConsumer.h"

class RtspSoftwareFactory : public IMediaFactory {
public:
    std::unique_ptr<IProtocolHandler> createStreamClient(const Properties& config) override {
        return std::make_unique<RtspClient>(config);
    };
    std::unique_ptr<IVideoDecoder> createVideoDecoder(const Properties& config) override {
        return std::make_unique<SoftDecoder>(config);
    };
    std::unique_ptr<IDataConsumer> createDataConsumer(const std::string& name, const Properties& config,
        std::shared_ptr<EventDispatcher> dispatcher) override {
        if (name == "render")
            return std::make_unique<OpenGLConsumer>(dispatcher);
        return nullptr;
    };
};
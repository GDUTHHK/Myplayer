#pragma once

#include "internal/IMediaFactory.h"
#include <string>
#include "internal/Share.h"
#include "nlohmann/json.hpp"

class JsonConfigFactory : public IMediaFactory {
public:
    explicit JsonConfigFactory(const std::string& config_path = "config.json");

    std::unique_ptr<IProtocolHandler> createStreamClient(const Properties& config) override;
    std::unique_ptr<IVideoDecoder> createVideoDecoder(const Properties& config) override;
    std::unique_ptr<IAudioDecoder> createAudioDecoder(const Properties& config) override;
    std::unique_ptr<IDataConsumer> createDataConsumer(const std::string& name, const Properties& config,
        std::shared_ptr<EventDispatcher> dispatcher) override;

    Properties getBaseConfig() const;
    std::string getConfigValue(const std::string& key, const std::string& defaultValue = "") const;
    bool getConfigBool(const std::string& key, bool defaultValue = false) const;
    int getConfigInt(const std::string& key, int defaultValue = 0) const;

private:
    nlohmann::json config_json_;
    std::string config_path_;
};
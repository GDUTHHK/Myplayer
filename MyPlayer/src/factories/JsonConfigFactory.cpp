#include "JsonConfigFactory.h"
#include "GenericFactory.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

JsonConfigFactory::JsonConfigFactory(const std::string& config_path) : config_path_(config_path) {
    std::ifstream f(config_path_);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open config file: " + config_path_);
    }
    try {
        f >> config_json_;
        std::cout << "Config file loaded successfully from: " << config_path << std::endl;
    }
    catch (nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
    }
}

std::unique_ptr<IProtocolHandler> JsonConfigFactory::createStreamClient(const Properties& config) {
    if (!config_json_.contains("stream_client")) return nullptr;
    std::string client_name = config_json_["stream_client"];
    std::cout << "Creating stream client: " << client_name << std::endl;
    return ProtocolHandlerFactory::instance().create(client_name, config);
}

std::unique_ptr<IVideoDecoder> JsonConfigFactory::createVideoDecoder(const Properties& config) {
    if (!config_json_.contains("video_decoder")) return nullptr;
    std::string decoder_name = config_json_["video_decoder"];
    std::cout << "Creating video decoder: " << decoder_name << std::endl;
    return VideoDecoderFactory::instance().create(decoder_name, config);
}

std::unique_ptr<IAudioDecoder> JsonConfigFactory::createAudioDecoder(const Properties& config) {
    if (!config_json_.contains("audio_decoder")) return nullptr;
    std::string decoder_name = config_json_["audio_decoder"];
    std::cout << "Creating audio decoder: " << decoder_name << std::endl;
    return AudioDecoderFactory::instance().create(decoder_name, config);
}

std::unique_ptr<IDataConsumer> JsonConfigFactory::createDataConsumer(const std::string& name, const Properties& config,
    std::shared_ptr<EventDispatcher> dispatcher) {
    if (config_json_.contains("data_consumers") && config_json_["data_consumers"].contains(name)) {
        std::string consumer_name = config_json_["data_consumers"][name];
        std::cout << "Creating data consumer for '" << name << "': " << consumer_name << std::endl;
        return DataConsumerFactory::instance().create(consumer_name, config, dispatcher);
    }
    return nullptr;
}

Properties JsonConfigFactory::getBaseConfig() const {
    Properties props;
    for (auto& [key, value] : config_json_.items()) {
        if (value.is_string()) {
            props[key] = value.get<std::string>();
        } else if (value.is_boolean()) {
            props[key] = value.get<bool>();
        } else if (value.is_number_integer()) {
            props[key] = value.get<int>();
        } else if (value.is_number_float()) {
            props[key] = value.get<double>();
        }
    }
    return props;
}


std::string JsonConfigFactory::getConfigValue(const std::string& key, const std::string& defaultValue) const {
    return config_json_.value(key, defaultValue);
}

bool JsonConfigFactory::getConfigBool(const std::string& key, bool defaultValue) const {
    return config_json_.value(key, defaultValue);
}

int JsonConfigFactory::getConfigInt(const std::string& key, int defaultValue) const {
    return config_json_.value(key, defaultValue);
}

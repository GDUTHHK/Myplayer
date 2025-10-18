#pragma once

#include <string>
#include <memory>
#include<functional>
#include<map>
#include<iostream>

#include "internal/EventDispatcher.h"
#include "internal/IProtocolHandler.h"
#include "internal/IVideoDecoder.h"
#include "internal/IDataConsumer.h"
#include "internal/IAudioDecoder.h" 

template <typename Base, typename... Args>
class GenericFactory {
public:
    using Creator = std::function<std::unique_ptr<Base>(Args...)>;

    static GenericFactory& instance() {
        static GenericFactory instance;
        return instance;
    }

    void regist(const std::string& name, Creator creator) {
        if (creators_.find(name) != creators_.end()) {
            std::cout << "Warning: Re-registering factory for " << name << std::endl;
        }
        std::cout << "have regist name:  " << name << std::endl;
        creators_[name] = creator;
    }

    std::unique_ptr<Base> create(const std::string& name, Args... args) {
        auto it = creators_.find(name);
        if (it == creators_.end()) {
            std::cerr << "Error: No creator registered for " << name << std::endl;
            return nullptr;
        }
        return it->second(std::forward<Args>(args)...);
    } 

private:
    GenericFactory() = default;
    ~GenericFactory() = default;
    GenericFactory(const GenericFactory&) = delete;
    GenericFactory& operator=(const GenericFactory&) = delete;

    std::map<std::string, Creator> creators_;
};

using ProtocolHandlerFactory = GenericFactory<IProtocolHandler, const Properties&>;
using VideoDecoderFactory = GenericFactory<IVideoDecoder, const Properties&>;
using AudioDecoderFactory = GenericFactory<IAudioDecoder, const Properties&>;
using DataConsumerFactory = GenericFactory<IDataConsumer, const Properties&, std::shared_ptr<EventDispatcher>>;

template <typename T , typename Base, typename... Args>
class Registrar {
public:
    explicit Registrar(const std::string& name) {
        GenericFactory<Base, Args...>::instance().regist(name, [](Args... args) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        });
    }
};
#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <any>
#include <memory>
// Share.h 提供了 DecodedFrame 等类型的定义
#include "Share.h" 

class EventDispatcher {
public:
    void subscribe(const std::string& topic, std::function<void(std::shared_ptr<DecodedFrame>)> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscribers[topic].push_back(std::move(callback));
    }

    void publish(const std::string& topic, std::shared_ptr<DecodedFrame> frame) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_subscribers.find(topic) == m_subscribers.end()) {
            return;
        }
        for (const auto& callback : m_subscribers.at(topic)) {
            callback(frame);
        }
    }

private:
    std::map<std::string, std::vector<std::function<void(std::shared_ptr<DecodedFrame>)>>> m_subscribers;
    std::mutex m_mutex;
};
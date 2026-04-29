#pragma once
#include "logger.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <algorithm>

namespace ams {

struct EventPayload {
    std::string sourceName;
    std::string eventName;
    long long timestamp;
    // We don't need to copy the whole map here because Observers 
    // can just request a fresh Snapshot using the sourceName!
};

using EventHandler = std::function<void(const EventPayload&)>;

class EventBus {
public:
    void subscribe(const std::string& eventName, const std::string& subscriberId, EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_[eventName].push_back({subscriberId, handler});
        AMS_DEV_LOG("EventBus", "Subscriber '", subscriberId, "' registered for event '", eventName, "'");
    }

    void unsubscribe(const std::string& eventName, const std::string& subscriberId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(eventName);
        if (it != subscriptions_.end()) {
            auto& handlers = it->second;
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [&subscriberId](const auto& pair) { return pair.first == subscriberId; }),
                handlers.end()
            );
        }
    }

    void publish(const std::string& eventName, const EventPayload& payload) {
        std::vector<EventHandler> handlers;
        {
            // Lock only to copy the handlers, preventing deadlocks!
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscriptions_.find(eventName);
            if (it != subscriptions_.end()) {
                for (const auto& pair : it->second) {
                    handlers.push_back(pair.second);
                }
            }
        }
        
        for (auto& h : handlers) {
            try {
                h(payload);
            } catch (const std::exception& e) {
                AMS_ERROR_LOG("EventBus", "Handler error: ", e.what());
            }
        }
    }

private:
    std::map<std::string, std::vector<std::pair<std::string, EventHandler>>> subscriptions_;
    std::mutex mutex_;
};

} // namespace ams
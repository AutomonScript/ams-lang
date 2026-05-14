#pragma once
#include "snapshot.hpp"
#include "signal_context.hpp"
#include "logger.hpp"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace ams {

class TrackCopyManager {
public:
    TrackCopyManager() = default;
    ~TrackCopyManager() {
        // Detect memory leaks: warn if contexts remain after program termination
        std::lock_guard<std::mutex> lock(mutex_);
        if (!contexts_.empty()) {
            AMS_WARN_LOG("TrackCopyManager", "Memory leak detected: ", contexts_.size(), 
                         " context(s) remain after program termination");
            for (const auto& pair : contexts_) {
                AMS_WARN_LOG("TrackCopyManager", "  - Context ID: ", pair.first, 
                             ", RefCount: ", pair.second.refCount);
            }
        }
    }

    // Create a TRACK copy from a SOURCE
    std::string createTrackCopy(
        const std::string& sourceName,
        const std::map<std::string, Snapshot::Value>& trackVars
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string contextId = generateContextId();
        Snapshot sourceData(trackVars);
        
        auto context = std::make_shared<SignalContext>(
            sourceName,
            sourceData,
            "",  // No event name for SOURCE contexts
            Snapshot()  // Empty event data
        );
        
        contexts_[contextId] = ContextData{context, 1, std::chrono::steady_clock::now()};
        
        AMS_DEV_LOG("TrackCopyManager", "Created TRACK copy for SOURCE '", sourceName, 
                    "' with ID: ", contextId, " (", trackVars.size(), " variables)");
        
        return contextId;
    }
    
    // Create a SignalContext for EVENT → OBSERVER with transitive SOURCE data
    std::string createEventContext(
        const std::string& eventName,
        const std::map<std::string, Snapshot::Value>& eventVars,
        const std::string& sourceContextId
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Retrieve the SOURCE context
        auto sourceIt = contexts_.find(sourceContextId);
        if (sourceIt == contexts_.end()) {
            AMS_ERROR_LOG("TrackCopyManager", "Source context not found: ", sourceContextId);
            // Return empty context to prevent crash
            std::string contextId = generateContextId();
            auto context = std::make_shared<SignalContext>("", Snapshot(), eventName, Snapshot(eventVars));
            contexts_[contextId] = ContextData{context, 1, std::chrono::steady_clock::now()};
            return contextId;
        }
        
        auto sourceContext = sourceIt->second.context;
        Snapshot eventData(eventVars);
        
        // Create new context with both SOURCE and EVENT data
        std::string contextId = generateContextId();
        auto context = std::make_shared<SignalContext>(
            sourceContext->getSourceName(),
            sourceContext->getSourceSnapshot(),
            eventName,
            eventData
        );
        
        contexts_[contextId] = ContextData{context, 1, std::chrono::steady_clock::now()};
        
        AMS_DEV_LOG("TrackCopyManager", "Created EVENT context for '", eventName, 
                    "' with ID: ", contextId, " (inherits SOURCE '", 
                    sourceContext->getSourceName(), "')");
        
        return contextId;
    }
    
    // Retrieve a SignalContext by ID
    std::shared_ptr<SignalContext> getContext(const std::string& contextId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = contexts_.find(contextId);
        if (it != contexts_.end()) {
            return it->second.context;
        }
        
        AMS_WARN_LOG("TrackCopyManager", "Context not found: ", contextId);
        return nullptr;
    }
    
    // Increment reference count (when a new entity receives the context)
    void addReference(const std::string& contextId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = contexts_.find(contextId);
        if (it != contexts_.end()) {
            it->second.refCount++;
            AMS_DEV_LOG("TrackCopyManager", "Incremented refCount for context ", contextId, 
                        " to ", it->second.refCount);
        } else {
            AMS_WARN_LOG("TrackCopyManager", "Attempted to add reference to non-existent context: ", contextId);
        }
    }
    
    // Decrement reference count and deallocate if zero
    void releaseContext(const std::string& contextId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = contexts_.find(contextId);
        if (it == contexts_.end()) {
            AMS_WARN_LOG("TrackCopyManager", "Attempted to release non-existent context: ", contextId);
            return;
        }
        
        it->second.refCount--;
        AMS_DEV_LOG("TrackCopyManager", "Decremented refCount for context ", contextId, 
                    " to ", it->second.refCount);
        
        if (it->second.refCount <= 0) {
            contexts_.erase(it);
            AMS_DEV_LOG("TrackCopyManager", "Deallocated context: ", contextId);
        }
    }

private:
    struct ContextData {
        std::shared_ptr<SignalContext> context;
        int refCount;
        std::chrono::steady_clock::time_point createdAt;
    };
    
    std::map<std::string, ContextData> contexts_;
    std::mutex mutex_;
    
    // Generate unique context ID
    std::string generateContextId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;
        
        auto now = std::chrono::steady_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count();
        
        uint64_t random = dis(gen);
        
        std::ostringstream oss;
        oss << "ctx_" << std::hex << std::setfill('0') 
            << std::setw(12) << timestamp 
            << "_" << std::setw(8) << (random & 0xFFFFFFFF);
        
        return oss.str();
    }
};

} // namespace ams

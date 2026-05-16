#pragma once

#include "../stdlib/source/log_source.hpp"
#include "../stdlib/source/log_data.hpp"
#include "../stdlib/source/log_record.hpp"
#include "../stdlib/source/filter_criteria.hpp"
#include "runtime.hpp"
#include "event_bus.hpp"
#include "scheduler.hpp"
#include "variable_registry.hpp"
#include "track_copy_manager.hpp"
#include "logger.hpp"
#include <memory>
#include <string>
#include <functional>
#include <map>

namespace ams {
namespace runtime {

/**
 * LOG_SOURCE Integration Layer
 * 
 * This module provides integration between LOG_SOURCE and the AMS Runtime system,
 * enabling LOG_SOURCE to work within SOURCE blocks with CHECK scheduling, SIGNAL
 * propagation, and TRACK variable support.
 * 
 * Key Features:
 * - CHECK scheduling integration (EVERY, AT, CONTINUOUSLY)
 * - hasUpdated() polling mechanism for change detection
 * - SIGNAL propagation when new log entries are detected
 * - TRACK variable support for LOG_DATA
 * - EventBus integration for LOG_SOURCE events
 * - Scheduler integration for periodic checks
 */

/**
 * Wrapper class for LOG_SOURCE that integrates with the AMS Runtime
 * 
 * This class manages the lifecycle of a LOG_SOURCE within a SOURCE block,
 * handling scheduling, change detection, and signal propagation.
 */
class ManagedLogSource {
public:
    /**
     * Create a managed LOG_SOURCE
     * 
     * @param sourceName The name of the SOURCE block
     * @param filepath Path to the log file
     * @param mode Access mode (READ or WRITE)
     * @param runtime Reference to the AMS Runtime
     */
    static std::shared_ptr<ManagedLogSource> create(
        const std::string& sourceName,
        const std::string& filepath,
        stdlib::source::AccessMode mode,
        Runtime& runtime
    ) {
        // Open the LOG_SOURCE
        auto result = stdlib::source::LOG_SOURCE::open(filepath, mode);
        if (result.isErr()) {
            AMS_ERROR_LOG("ManagedLogSource", "Failed to open LOG_SOURCE '", sourceName, 
                         "' at '", filepath, "': ", result.getErrorMessage());
            return nullptr;
        }
        
        auto managed = std::shared_ptr<ManagedLogSource>(
            new ManagedLogSource(sourceName, result.unwrap(), runtime)
        );
        
        AMS_DEV_LOG("ManagedLogSource", "Created managed LOG_SOURCE '", sourceName, 
                   "' for file '", filepath, "'");
        
        return managed;
    }
    
    /**
     * Get the underlying LOG_SOURCE
     */
    stdlib::source::LOG_SOURCE& getLogSource() {
        return logSource_;
    }
    
    /**
     * Check if the log file has been updated
     * 
     * This method is called by CHECK statements to determine if the SOURCE
     * block should be executed.
     * 
     * @return true if the log file has new entries, false otherwise
     */
    bool hasUpdated() {
        return logSource_.hasUpdated();
    }
    
    /**
     * Register a CHECK schedule for this LOG_SOURCE
     * 
     * This method sets up periodic polling of hasUpdated() and triggers
     * the SOURCE block execution when updates are detected.
     * 
     * @param checkType Type of check ("EVERY", "AT", "CONTINUOUSLY")
     * @param intervalMs Interval in milliseconds (for EVERY)
     * @param callback Function to call when updates are detected
     */
    void registerCheck(
        const std::string& checkType,
        int intervalMs,
        std::function<void()> callback
    ) {
        if (checkType == "EVERY") {
            runtime_.getScheduler().schedulePeriodic(
                sourceName_ + "_check",
                intervalMs,
                [this, callback]() {
                    if (this->hasUpdated()) {
                        AMS_DEV_LOG("ManagedLogSource", "LOG_SOURCE '", sourceName_, 
                                   "' detected updates, executing SOURCE block");
                        callback();
                    }
                }
            );
            AMS_DEV_LOG("ManagedLogSource", "Registered EVERY ", intervalMs, 
                       "ms check for LOG_SOURCE '", sourceName_, "'");
        }
        else if (checkType == "CONTINUOUSLY") {
            runtime_.getScheduler().runContinuously(
                sourceName_ + "_check",
                [this, callback]() {
                    if (this->hasUpdated()) {
                        AMS_DEV_LOG("ManagedLogSource", "LOG_SOURCE '", sourceName_, 
                                   "' detected updates, executing SOURCE block");
                        callback();
                    }
                }
            );
            AMS_DEV_LOG("ManagedLogSource", "Registered CONTINUOUSLY check for LOG_SOURCE '", 
                       sourceName_, "'");
        }
        else if (checkType == "AT") {
            // AT scheduling would require specific time-of-day support
            // For now, we'll use a periodic check with the given interval
            runtime_.getScheduler().schedulePeriodic(
                sourceName_ + "_check",
                intervalMs,
                [this, callback]() {
                    if (this->hasUpdated()) {
                        AMS_DEV_LOG("ManagedLogSource", "LOG_SOURCE '", sourceName_, 
                                   "' detected updates, executing SOURCE block");
                        callback();
                    }
                }
            );
            AMS_DEV_LOG("ManagedLogSource", "Registered AT check for LOG_SOURCE '", 
                       sourceName_, "'");
        }
    }
    
    /**
     * Emit a SIGNAL event when conditions are met
     * 
     * This method publishes an event to the EventBus, triggering any
     * EVENT blocks that are listening for this SOURCE.
     * 
     * @param condition The condition that triggered the signal
     * @param trackVars TRACK variables to include in the signal context
     */
    void emitSignal(
        bool condition,
        const std::map<std::string, Snapshot::Value>& trackVars
    ) {
        if (!condition) {
            return;
        }
        
        // Create a TRACK copy with the current state
        std::string contextId = runtime_.getTrackCopyManager().createTrackCopy(
            sourceName_,
            trackVars
        );
        
        // Publish event to EventBus
        EventPayload payload;
        payload.sourceName = sourceName_;
        payload.eventName = sourceName_;  // SOURCE events use the SOURCE name
        payload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        payload.contextId = contextId;
        
        runtime_.getEventBus().publish(sourceName_, payload);
        
        AMS_DEV_LOG("ManagedLogSource", "LOG_SOURCE '", sourceName_, 
                   "' emitted SIGNAL with context ID: ", contextId);
    }
    
    /**
     * Get the SOURCE name
     */
    const std::string& getSourceName() const {
        return sourceName_;
    }
    
private:
    ManagedLogSource(
        const std::string& sourceName,
        stdlib::source::LOG_SOURCE logSource,
        Runtime& runtime
    ) : sourceName_(sourceName)
      , logSource_(std::move(logSource))
      , runtime_(runtime)
    {
    }
    
    std::string sourceName_;
    stdlib::source::LOG_SOURCE logSource_;
    Runtime& runtime_;
};

/**
 * Helper function to convert LOG_DATA to a Snapshot::Value
 * 
 * Since LOG_DATA is a complex type that doesn't fit into the standard
 * Snapshot::Value variant, we store a reference ID that can be used
 * to retrieve the actual LOG_DATA from a registry.
 */
class LogDataRegistry {
public:
    static LogDataRegistry& instance() {
        static LogDataRegistry registry;
        return registry;
    }
    
    /**
     * Store LOG_DATA and return a reference ID
     */
    std::string store(const stdlib::source::LOG_DATA& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string id = generateId();
        storage_[id] = data;
        
        AMS_DEV_LOG("LogDataRegistry", "Stored LOG_DATA with ID: ", id, 
                   " (", data.count(), " records)");
        
        return id;
    }
    
    /**
     * Retrieve LOG_DATA by reference ID
     */
    stdlib::source::LOG_DATA get(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = storage_.find(id);
        if (it != storage_.end()) {
            return it->second;
        }
        
        AMS_WARN_LOG("LogDataRegistry", "LOG_DATA not found for ID: ", id);
        return stdlib::source::LOG_DATA();  // Return empty LOG_DATA
    }
    
    /**
     * Check if a reference ID exists
     */
    bool has(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return storage_.find(id) != storage_.end();
    }
    
    /**
     * Remove LOG_DATA by reference ID
     */
    void remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        storage_.erase(id);
        AMS_DEV_LOG("LogDataRegistry", "Removed LOG_DATA with ID: ", id);
    }
    
private:
    LogDataRegistry() = default;
    
    std::string generateId() {
        static std::atomic<uint64_t> counter{0};
        return "logdata_" + std::to_string(counter++);
    }
    
    mutable std::mutex mutex_;
    std::map<std::string, stdlib::source::LOG_DATA> storage_;
};

/**
 * Helper function to create a FilterCriteria by log level
 * 
 * This is used in generated code for expressions like:
 *   LOG_DATA errors = logs.filter(byLevel(ERROR))
 */
inline stdlib::source::FilterCriteria byLevel(stdlib::source::LogLevel level) {
    return stdlib::source::FilterCriteria::byLevel(level);
}

/**
 * Helper function to create a FilterCriteria by time range
 */
inline stdlib::source::FilterCriteria byTimeRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end
) {
    return stdlib::source::FilterCriteria::byTimeRange(start, end);
}

/**
 * Helper function to create a FilterCriteria by message substring
 */
inline stdlib::source::FilterCriteria byMessage(const std::string& substring) {
    return stdlib::source::FilterCriteria::byMessage(substring);
}

} // namespace runtime
} // namespace ams

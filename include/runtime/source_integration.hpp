#pragma once

#include "../stdlib/source/log_source.hpp"
#include "../stdlib/source/log_data.hpp"
#include "../stdlib/source/log_record.hpp"
#include "../stdlib/source/filter_criteria.hpp"
#include "runtime.hpp"
#include "event_bus.hpp"
#include "scheduler.hpp"
#include "variable_registry.hpp"
#include "snapshot.hpp"
#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace ams {
namespace runtime {

/**
 * @brief Integration layer for LOG_SOURCE with AMS Runtime
 * 
 * This class provides helper functions and utilities for integrating
 * LOG_SOURCE instances with the AMS Runtime system, including:
 * - SOURCE block registration
 * - CHECK scheduling (EVERY, AT, CONTINUOUSLY)
 * - SIGNAL propagation when hasUpdated() returns true
 * - TRACK variable support for LOG_DATA
 * - EventBus integration for LOG_SOURCE events
 */
class SourceIntegration {
public:
    /**
     * @brief Register a LOG_SOURCE with the runtime for periodic checking
     * 
     * This function sets up a periodic task that:
     * 1. Calls hasUpdated() on the LOG_SOURCE
     * 2. If updated, publishes an event to the EventBus
     * 3. Updates tracked variables in the VariableRegistry
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param logSource Shared pointer to the LOG_SOURCE instance
     * @param intervalMs Check interval in milliseconds
     * @param unit Time unit ("MS", "SEC", "MIN", "HOUR")
     */
    static void registerPeriodicCheck(
        Runtime& runtime,
        const std::string& sourceName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource,
        int intervalMs,
        const std::string& unit
    ) {
        runtime.schedulePeriodic(sourceName, intervalMs, unit, [&runtime, sourceName, logSource]() {
            if (logSource && logSource->isOpen() && logSource->hasUpdated()) {
                // Publish update event
                EventPayload payload;
                payload.sourceName = sourceName;
                payload.eventName = sourceName + "_Updated";
                payload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                
                runtime.getEventBus().publish(sourceName + "_Updated", payload);
                
                // Also publish to generic LOG_SOURCE_UPDATED event
                runtime.getEventBus().publish("LOG_SOURCE_UPDATED", payload);
            }
        });
    }
    
    /**
     * @brief Register a LOG_SOURCE with one-time check scheduling
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param logSource Shared pointer to the LOG_SOURCE instance
     * @param delayMs Delay before check in milliseconds
     */
    static void registerOnceCheck(
        Runtime& runtime,
        const std::string& sourceName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource,
        int delayMs
    ) {
        runtime.scheduleOnce(sourceName, delayMs, [&runtime, sourceName, logSource]() {
            if (logSource && logSource->isOpen() && logSource->hasUpdated()) {
                EventPayload payload;
                payload.sourceName = sourceName;
                payload.eventName = sourceName + "_Updated";
                payload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                
                runtime.getEventBus().publish(sourceName + "_Updated", payload);
                runtime.getEventBus().publish("LOG_SOURCE_UPDATED", payload);
            }
        });
    }
    
    /**
     * @brief Register a LOG_SOURCE for continuous checking
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param logSource Shared pointer to the LOG_SOURCE instance
     */
    static void registerContinuousCheck(
        Runtime& runtime,
        const std::string& sourceName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource
    ) {
        runtime.runContinuously(sourceName, [&runtime, sourceName, logSource]() {
            if (logSource && logSource->isOpen() && logSource->hasUpdated()) {
                EventPayload payload;
                payload.sourceName = sourceName;
                payload.eventName = sourceName + "_Updated";
                payload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                
                runtime.getEventBus().publish(sourceName + "_Updated", payload);
                runtime.getEventBus().publish("LOG_SOURCE_UPDATED", payload);
            }
            
            // Small sleep to prevent busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    
    /**
     * @brief Register a TRACK variable for LOG_DATA
     * 
     * This function registers a tracked variable that will be updated
     * whenever the LOG_SOURCE is checked and has updates.
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param varName The name of the tracked variable
     * @param logSource Shared pointer to the LOG_SOURCE instance
     * @param filterFunc Function that filters/processes the LOG_DATA
     */
    static void registerTrackedVariable(
        Runtime& runtime,
        const std::string& sourceName,
        const std::string& varName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource,
        std::function<stdlib::source::LOG_DATA(const stdlib::source::LOG_SOURCE&)> filterFunc
    ) {
        // Subscribe to the source's update event
        runtime.getEventBus().subscribe(
            sourceName + "_Updated",
            sourceName + "_" + varName + "_tracker",
            [&runtime, sourceName, varName, logSource, filterFunc](const EventPayload& payload) {
                if (logSource && logSource->isOpen()) {
                    // Apply the filter function to get LOG_DATA
                    stdlib::source::LOG_DATA data = filterFunc(*logSource);
                    
                    // Store the count as an integer in the variable registry
                    // (Since Snapshot::Value doesn't support LOG_DATA directly,
                    // we store metadata about the data)
                    runtime.updateVar(sourceName, varName + "_count", static_cast<int>(data.count()));
                    runtime.updateVar(sourceName, varName + "_is_single", data.isSingle());
                    runtime.updateVar(sourceName, varName + "_is_empty", data.isEmpty());
                }
            }
        );
    }
    
    /**
     * @brief Register a SIGNAL condition based on LOG_SOURCE state
     * 
     * This function sets up a signal condition that will trigger an event
     * when the condition evaluates to true.
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param signalName The name of the signal event
     * @param logSource Shared pointer to the LOG_SOURCE instance
     * @param condition Function that evaluates the signal condition
     */
    static void registerSignalCondition(
        Runtime& runtime,
        const std::string& sourceName,
        const std::string& signalName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource,
        std::function<bool(const stdlib::source::LOG_SOURCE&)> condition
    ) {
        // Subscribe to the source's update event
        runtime.getEventBus().subscribe(
            sourceName + "_Updated",
            sourceName + "_" + signalName + "_condition",
            [&runtime, sourceName, signalName, logSource, condition](const EventPayload& payload) {
                if (logSource && logSource->isOpen()) {
                    // Evaluate the condition
                    if (condition(*logSource)) {
                        // Trigger the signal event
                        EventPayload signalPayload;
                        signalPayload.sourceName = sourceName;
                        signalPayload.eventName = signalName;
                        signalPayload.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                        
                        runtime.getEventBus().publish(signalName, signalPayload);
                    }
                }
            }
        );
    }
    
    /**
     * @brief Helper function to create a FilterCriteria by log level
     * 
     * This is a convenience function for generated code to easily create
     * filter criteria for common use cases.
     * 
     * @param level The log level to filter by
     * @return FilterCriteria configured for the specified level
     */
    static stdlib::source::FilterCriteria createLevelFilter(stdlib::source::LogLevel level) {
        return stdlib::source::FilterCriteria::byLevel(level);
    }
    
    /**
     * @brief Helper function to create a FilterCriteria by message substring
     * 
     * @param substring The substring to search for in messages
     * @return FilterCriteria configured for message substring matching
     */
    static stdlib::source::FilterCriteria createMessageFilter(const std::string& substring) {
        return stdlib::source::FilterCriteria::byMessage(substring);
    }
    
    /**
     * @brief Helper function to create a FilterCriteria by time range
     * 
     * @param start Start time of the range
     * @param end End time of the range
     * @return FilterCriteria configured for time range filtering
     */
    static stdlib::source::FilterCriteria createTimeRangeFilter(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end
    ) {
        return stdlib::source::FilterCriteria::byTimeRange(start, end);
    }
};

/**
 * @brief Helper class for managing LOG_SOURCE instances in generated code
 * 
 * This class provides a convenient wrapper for LOG_SOURCE that integrates
 * with the runtime and provides easy access to common operations.
 */
class ManagedLogSource {
public:
    /**
     * @brief Create a managed LOG_SOURCE
     * 
     * @param runtime The AMS Runtime instance
     * @param sourceName The name of the SOURCE block
     * @param filepath Path to the log file
     * @param mode Access mode (READ or WRITE)
     */
    static std::shared_ptr<ManagedLogSource> create(
        Runtime& runtime,
        const std::string& sourceName,
        const std::string& filepath,
        stdlib::source::AccessMode mode
    ) {
        auto result = stdlib::source::LOG_SOURCE::open(filepath, mode);
        if (result.isErr()) {
            AMS_ERROR_LOG("ManagedLogSource", "Failed to open LOG_SOURCE: ", result.getErrorMessage());
            return nullptr;
        }
        
        auto logSource = std::make_shared<stdlib::source::LOG_SOURCE>(std::move(result.unwrap()));
        return std::make_shared<ManagedLogSource>(runtime, sourceName, logSource);
    }
    
    /**
     * @brief Get the underlying LOG_SOURCE
     */
    std::shared_ptr<stdlib::source::LOG_SOURCE> getLogSource() const {
        return logSource_;
    }
    
    /**
     * @brief Register periodic checking with the runtime
     */
    void enablePeriodicCheck(int intervalMs, const std::string& unit) {
        SourceIntegration::registerPeriodicCheck(runtime_, sourceName_, logSource_, intervalMs, unit);
    }
    
    /**
     * @brief Register continuous checking with the runtime
     */
    void enableContinuousCheck() {
        SourceIntegration::registerContinuousCheck(runtime_, sourceName_, logSource_);
    }
    
    /**
     * @brief Track a filtered LOG_DATA variable
     */
    void trackVariable(
        const std::string& varName,
        std::function<stdlib::source::LOG_DATA(const stdlib::source::LOG_SOURCE&)> filterFunc
    ) {
        SourceIntegration::registerTrackedVariable(runtime_, sourceName_, varName, logSource_, filterFunc);
    }
    
    /**
     * @brief Register a signal condition
     */
    void registerSignal(
        const std::string& signalName,
        std::function<bool(const stdlib::source::LOG_SOURCE&)> condition
    ) {
        SourceIntegration::registerSignalCondition(runtime_, sourceName_, signalName, logSource_, condition);
    }
    
private:
    ManagedLogSource(
        Runtime& runtime,
        const std::string& sourceName,
        std::shared_ptr<stdlib::source::LOG_SOURCE> logSource
    )
        : runtime_(runtime)
        , sourceName_(sourceName)
        , logSource_(logSource)
    {
    }
    
    Runtime& runtime_;
    std::string sourceName_;
    std::shared_ptr<stdlib::source::LOG_SOURCE> logSource_;
};

} // namespace runtime
} // namespace ams

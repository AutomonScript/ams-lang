#pragma once
#include "snapshot.hpp"
#include "logger.hpp"
#include <string>
#include <chrono>

namespace ams {

class SignalContext {
public:
    SignalContext(
        const std::string& sourceName,
        const Snapshot& sourceData,
        const std::string& eventName = "",
        const Snapshot& eventData = Snapshot()
    ) : sourceName_(sourceName),
        sourceData_(sourceData),
        eventName_(eventName),
        eventData_(eventData),
        timestamp_(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()) {}
    
    // Access SOURCE TRACK variables
    int getSourceInt(const std::string& varName) const {
        if (!hasSourceVar(varName)) {
            AMS_WARN_LOG("SignalContext", "SOURCE variable '", varName, "' not found, returning default value 0");
            return 0;
        }
        try {
            return sourceData_.getInt(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing SOURCE.", varName, ": ", e.what());
            return 0;
        }
    }
    
    double getSourceDouble(const std::string& varName) const {
        if (!hasSourceVar(varName)) {
            AMS_WARN_LOG("SignalContext", "SOURCE variable '", varName, "' not found, returning default value 0.0");
            return 0.0;
        }
        try {
            return sourceData_.getDouble(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing SOURCE.", varName, ": ", e.what());
            return 0.0;
        }
    }
    
    std::string getSourceString(const std::string& varName) const {
        if (!hasSourceVar(varName)) {
            AMS_WARN_LOG("SignalContext", "SOURCE variable '", varName, "' not found, returning default value \"\"");
            return "";
        }
        try {
            return sourceData_.getString(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing SOURCE.", varName, ": ", e.what());
            return "";
        }
    }
    
    bool getSourceBool(const std::string& varName) const {
        if (!hasSourceVar(varName)) {
            AMS_WARN_LOG("SignalContext", "SOURCE variable '", varName, "' not found, returning default value false");
            return false;
        }
        try {
            return sourceData_.getBool(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing SOURCE.", varName, ": ", e.what());
            return false;
        }
    }
    
    // Access EVENT variables (for OBSERVERs)
    int getEventInt(const std::string& varName) const {
        if (!hasEventVar(varName)) {
            AMS_WARN_LOG("SignalContext", "EVENT variable '", varName, "' not found, returning default value 0");
            return 0;
        }
        try {
            return eventData_.getInt(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing EVENT.", varName, ": ", e.what());
            return 0;
        }
    }
    
    double getEventDouble(const std::string& varName) const {
        if (!hasEventVar(varName)) {
            AMS_WARN_LOG("SignalContext", "EVENT variable '", varName, "' not found, returning default value 0.0");
            return 0.0;
        }
        try {
            return eventData_.getDouble(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing EVENT.", varName, ": ", e.what());
            return 0.0;
        }
    }
    
    std::string getEventString(const std::string& varName) const {
        if (!hasEventVar(varName)) {
            AMS_WARN_LOG("SignalContext", "EVENT variable '", varName, "' not found, returning default value \"\"");
            return "";
        }
        try {
            return eventData_.getString(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing EVENT.", varName, ": ", e.what());
            return "";
        }
    }
    
    bool getEventBool(const std::string& varName) const {
        if (!hasEventVar(varName)) {
            AMS_WARN_LOG("SignalContext", "EVENT variable '", varName, "' not found, returning default value false");
            return false;
        }
        try {
            return eventData_.getBool(varName);
        } catch (const std::exception& e) {
            AMS_ERROR_LOG("SignalContext", "Error accessing EVENT.", varName, ": ", e.what());
            return false;
        }
    }
    
    // Check if variable exists
    bool hasSourceVar(const std::string& varName) const {
        return sourceData_.has(varName);
    }
    
    bool hasEventVar(const std::string& varName) const {
        return eventData_.has(varName);
    }
    
    // Metadata
    const std::string& getSourceName() const { return sourceName_; }
    const std::string& getEventName() const { return eventName_; }
    long long getTimestamp() const { return timestamp_; }
    
    // Internal accessors for TrackCopyManager
    const Snapshot& getSourceSnapshot() const { return sourceData_; }
    const Snapshot& getEventSnapshot() const { return eventData_; }

private:
    std::string sourceName_;
    Snapshot sourceData_;
    std::string eventName_;
    Snapshot eventData_;
    long long timestamp_;
};

} // namespace ams
